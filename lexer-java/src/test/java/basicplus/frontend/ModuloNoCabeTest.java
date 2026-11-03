package basicplus.frontend;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V8 #768 (3-nov) — los datos de un módulo (sus literales, sus globales, sus constantes y el descriptor de cada clase)
 * caben en 64 KB (Eduardo: «si utilizas un word pasamos a 64K»). Uno que no cabe es un error del programa, UNO, que
 * dice cuánto ocupan ya y qué es lo que no cabe, y sin artefacto. Antes, con 32 KB, salía «intern string: Offset CS
 * fuera de rango short… (data block excede 32 KB)» y detrás, en cascada, «Símbolo de datos no declarado (forward ref
 * no permitida): __str_0», que despistaba. El programa se genera aquí: uno de verdad pasa de 64 KB de fuente (por eso
 * no está en samples/errores). Que lo de 33 000 bytes corre igual en las dos VMs lo prueba DatosGrandes, del corpus.
 */
class ModuloNoCabeTest {

    private static String repite(char c, int n) {
        StringBuilder sb = new StringBuilder(n);
        for (int i = 0; i < n; i++) sb.append(c);
        return sb.toString();
    }

    private static String modulo(String nombre, String... literales) {
        StringBuilder sb = new StringBuilder("module " + nombre + "\nimport Core\n");
        for (int i = 0; i < literales.length; i++) sb.append("var t").append(i).append(": string := \"").append(literales[i]).append("\"\n");
        sb.append("public function main()\n  var n: integer := 0\n");
        for (int i = 0; i < literales.length; i++) sb.append("  n := n + len(t").append(i).append(")\n");
        sb.append("  print n\nend main\nend ").append(nombre).append("\n");
        return sb.toString();
    }

    /** Como en EmisorConErroresTest: un proyecto con la stdlib de ../bpstdlib; {ok, lo que dijo el compilador}. */
    private static Object[] compila(Path tmp, String main, String fuente) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve(main + ".bp"), fuente.getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString(); b.sourceDir = src.toString(); b.outDir = out.toString(); b.main = main;
        b.version = 7;
        File std = new File("../bpstdlib");
        Assumptions.assumeTrue(new File(std, "Core.mod").isFile(), "hace falta ../bpstdlib/Core.mod");
        b.dependencies.add(std.getAbsolutePath());
        PrintStream out0 = System.out, err0 = System.err;
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(buf, true, "UTF-8");
        boolean ok;
        try { System.setOut(ps); System.setErr(ps); ok = Main.buildProject(b, "mivm", false); }
        finally { System.setOut(out0); System.setErr(err0); }
        return new Object[]{ ok, new String(buf.toByteArray(), StandardCharsets.UTF_8) };
    }

    @Test @DisplayName("más de 64 KB de datos: un error, el que lo dice, y ni .mod ni .dbg (el rancio, borrado)")
    void noCabe(@TempDir Path tmp) throws IOException {
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(out.resolve("Lleno.mod"), new byte[]{ 1, 2, 3 });   // el gemelo rancio
        Files.write(out.resolve("Lleno.dbg"), new byte[]{ 4 });
        Object[] r = compila(tmp, "Lleno", modulo("Lleno", repite('a', 40000), repite('b', 40000)));
        String t = (String) r[1];
        assertFalse((Boolean) r[0], "el build tiene que fallar:\n" + t);
        assertTrue(t.contains("el módulo 'Lleno' no cabe: sus datos (los literales, las globales y las constantes) pasan"
                + " de 64 KB, el tope (65532 bytes)"), t);
        assertTrue(t.contains(" y el literal \"bbbbbbbbbbbbbbbbbbbbbbbb…\" (40000 bytes) no cabe."), t);
        assertEquals(1, t.split("no cabe: sus datos", -1).length - 1, "uno solo:\n" + t);
        assertFalse(t.contains("no declarado"), "sin la cascada de antes:\n" + t);
        assertFalse(t.contains("intern string"), t);
        assertTrue(t.contains("NO compilado:"), t);
        assertFalse(Files.exists(out.resolve("Lleno.mod")), "el .mod rancio sigue ahi");
        assertFalse(Files.exists(out.resolve("Lleno.dbg")), "el .dbg rancio sigue ahi");
    }

    @Test @DisplayName("33 000 bytes de datos (hasta el 3-nov, por encima del tope de 32 KB) compilan")
    void cabe(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Cabe", modulo("Cabe", repite('a', 33000)));
        String t = (String) r[1];
        assertTrue((Boolean) r[0], "tiene que compilar:\n" + t);
        assertTrue(Files.exists(tmp.resolve("out").resolve("Cabe.mod")), t);
    }
}
