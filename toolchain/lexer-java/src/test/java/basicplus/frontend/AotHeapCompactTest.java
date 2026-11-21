package basicplus.frontend;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

/**
 * V7/H7 #595 (28-sep) — {@code Heap.compact()} NO se llama desde una {@code native}: es un error de compilación.
 *
 * <p>Compactar MUEVE los objetos del heap, y una native guarda sus direcciones en variables de C que nadie sabría
 * poner al día. Las dos VMs ya lo rechazarían en ejecución, pero por caminos distintos —en miVM la native es
 * bytecode con su marco, y no es Main; en la VM-C no abre marco BP y lo delata el hilo—, y con una {@code Main}
 * native no coincidirían: miVM la vería como Main. Así que se corta al compilar, igual para las dos. El control:
 * desde Main, compila.
 */
class AotHeapCompactTest {

    @AfterEach
    void limpia() { Main.STDLIB_DIR = null; }

    private static Path escribe(Path dir, String modulo, String fuente) throws Exception {
        Path bp = dir.resolve(modulo + ".bp");
        Files.write(bp, fuente.getBytes(StandardCharsets.UTF_8));
        return bp;
    }

    private static String modulo(String nombre, boolean enNative) {
        return String.join("\n",
            "module " + nombre,
            "  import Heap",
            "  native function suma(n: integer): integer",
            enNative ? "    Heap.compact()" : "",
            "    return n + 1",
            "  end suma",
            "  function Main()",
            "    print suma(41)",
            "    Heap.compact()",
            "  end Main",
            "end " + nombre, "");
    }

    @Test @DisplayName("#595: Heap.compact() dentro de una native — error AOT con su nombre")
    void desdeUnaNativeEsError(@TempDir Path tmp) throws Exception {
        Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
        Path bp = escribe(tmp, "CompNat", modulo("CompNat", true));
        AotCEmitter.UnsupportedAotException ex = assertThrows(AotCEmitter.UnsupportedAotException.class,
                () -> AotMain.emitAotC(bp, tmp.resolve("out"), true));
        assertTrue(ex.getMessage().contains("Heap.compact()") && ex.getMessage().contains("native"), ex.getMessage());
        // Y en el compile normal (el del IDE, el de compat), el mismo error aborta: no sale .mod.
        assertFalse(Main.compileFile(bp, tmp.resolve("mods"), "mivm"), "una native con Heap.compact() no debe compilar");
    }

    @Test @DisplayName("#595: el control — desde Main compila, y la native sin compact se traduce")
    void desdeMainCompila(@TempDir Path tmp) throws Exception {
        Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
        Path bp = escribe(tmp, "CompMain", modulo("CompMain", false));
        assertTrue(Main.compileFile(bp, tmp.resolve("mods"), "mivm"), "Heap.compact() desde Main debe compilar");
        assertTrue(Files.exists(AotMain.emitAotC(bp, tmp.resolve("out"), true).cFile));
    }
}
