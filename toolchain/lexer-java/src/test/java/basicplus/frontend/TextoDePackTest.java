package basicplus.frontend;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
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
 * V8 #724 (27-oct) — EL TEXTO QUE DEVUELVE UN PACK. Hasta #724 el puente lo contaba hasta 512 bytes y lo copiaba a una
 * cadena tal cual: lo de más se perdía sin avisar y lo que no era UTF-8 entraba roto. Ahora lo copia entero a un
 * {@code byte[]} y, si la native devuelve {@code string}, lo pasa por {@code fromBytes} (la regla de #711); con
 * {@code byte[]}, los bytes tal cual. Lo hace una función de la cabecera, fuera del puente, para que la native siga
 * siendo DIRECTA. Aquí se mira el C que sale; lo que hace en marcha, con un pack de mentira, lo mira
 * {@code make test-packtexto} (bpgenvm-c).
 */
class TextoDePackTest {

    private static final String PACK = String.join("\n",
        "module X",
        "  import Core",
        "  import v1 from pack \"MARC\"",
        "  public native function texto(n: integer): string",
        "  public native function bytes(n: integer): byte[]",
        "end X", "");

    @AfterEach
    void limpia() { Main.STDLIB_DIR = null; }

    private static String emite(Path dir, String fuente) throws Exception {
        Main.STDLIB_DIR = StdlibDeTest.paraFormato(dir).getAbsolutePath();
        Path bp = dir.resolve("X.bp");
        Files.write(bp, fuente.getBytes(StandardCharsets.UTF_8));
        AotMain.AotResult r = AotMain.emitAotC(bp, dir.resolve("out"), /*mdnMode*/ false);
        assertNotNull(r.cFile, "X tiene natives externas: debe emitir C (el puente al pack)");
        return new String(Files.readAllBytes(r.cFile), StandardCharsets.UTF_8);
    }

    @Test @DisplayName("#724: string → copia y fromBytes; byte[] → los bytes tal cual; sin el corte de 512")
    void stringYBytes(@TempDir Path tmp) throws Exception {
        String c = emite(tmp, PACK);
        // las dos le dan el `const char*` del pack a la misma función, con el indicador de descodificar
        assertTrue(c.contains("aot_X__texto_de_pack(vm, r, 0x4D415243u, 1u, nm, 1); }"), "string: descodifica");
        assertTrue(c.contains("aot_X__texto_de_pack(vm, r, 0x4D415243u, 1u, nm, 0); }"), "byte[]: tal cual");
        // la función: hasta el NUL con el tope nuevo (que no corta: pack_fallo, motivo 3), y fromBytes por el despachador
        assertTrue(c.contains("while (k < BPVM_AOT_PACK_TEXTO && r[k]) k++;"), c);
        assertTrue(c.contains("H->pack_fallo(vm, marca, ver, nm, 3);"), c);
        assertTrue(c.contains("H->call_builtin(vm, " + edu.bpgenvm.bytecode.Builtin.FROM_BYTES.id + ", tp, a, 'r');"), c);
        // y el puente de antes, ya no: ni el corte en 512 ni la cadena sin descodificar
        assertFalse(c.contains("BPVM_AOT_PACK_STR && r[k]"), "el corte de 512 en lo que vuelve");
        assertFalse(c.contains("string_from_cstr(vm, r, k)"), "la cadena sin descodificar");
    }

    @Test @DisplayName("#724: las dos siguen DIRECTAS (fromBytes no cede, y va fuera del puente)")
    void siguenDirectas(@TempDir Path tmp) throws Exception {
        String c = emite(tmp, PACK);
        assertTrue(c.contains("\"X.texto\", thunk_X_texto, BPVM_AOT_DIRECTA)"), "texto, directa");
        assertTrue(c.contains("\"X.bytes\", thunk_X_bytes, BPVM_AOT_DIRECTA)"), "bytes, directa");
    }

    @Test @DisplayName("#724: sin texto que vuelva del pack, el C no lleva la función")
    void sinTextoNoLaLleva(@TempDir Path tmp) throws Exception {
        String c = emite(tmp, String.join("\n",
            "module X",
            "  import Core",
            "  import v1 from pack \"MARC\"",
            "  public native function suma(a: integer, b: integer): integer",
            "end X", ""));
        assertFalse(c.contains("__texto_de_pack"), "una suma no devuelve texto");
    }

    @Test @DisplayName("#724: un byte[] como PARÁMETRO sigue sin cruzar, y se dice")
    void byteArrayParametroNo(@TempDir Path tmp) throws Exception {
        Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
        Path bp = tmp.resolve("X.bp");
        Files.write(bp, String.join("\n",
            "module X",
            "  import Core",
            "  import v1 from pack \"MARC\"",
            "  public native function mete(b: byte[]): integer",
            "end X", "").getBytes(StandardCharsets.UTF_8));
        AotCEmitter.UnsupportedAotException ex = assertThrows(AotCEmitter.UnsupportedAotException.class,
                () -> AotMain.emitAotC(bp, tmp.resolve("out"), false));
        assertTrue(ex.getMessage().contains("'mete'") && ex.getMessage().contains("byte[] como lo que DEVUELVE"),
                ex.getMessage());
    }
}
