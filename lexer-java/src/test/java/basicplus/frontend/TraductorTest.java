package basicplus.frontend;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V8 #698 — `Traductor`, la traducción BP ↔ JC que llama el IDE: el texto, o por qué no, sin salir y sin escribir nada
 * por la salida (la consola del IDE lo vería).
 */
class TraductorTest {

    @TempDir Path tmp;

    @BeforeEach
    void stdlib() throws IOException { Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath(); }

    @AfterEach
    void limpia() { Main.STDLIB_DIR = null; }

    private static final String BP = String.join("\n",
        "module Traduce",
        "  function main()",
        "    var n: integer := 3",
        "    for i := 1 to n do",
        "      print \"vuelta\", i",
        "    next i",
        "    print \"sin salto\";",
        "  end main",
        "end Traduce", "");

    private Path escribe(String nombre, String texto) throws IOException {
        Path f = tmp.resolve(nombre);
        Files.write(f, texto.getBytes(StandardCharsets.UTF_8));
        return f;
    }

    /** Lo que `accion` escribe por System.out y System.err mientras corre. */
    private static String loQueEscribe(Runnable accion) {
        PrintStream out0 = System.out, err0 = System.err;
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        try {
            PrintStream ps = new PrintStream(buf, true, "UTF-8");
            System.setOut(ps);
            System.setErr(ps);
            accion.run();
        } catch (IOException e) {
            throw new RuntimeException(e);
        } finally {
            System.setOut(out0);
            System.setErr(err0);
        }
        return new String(buf.toByteArray(), StandardCharsets.UTF_8);
    }

    @Test
    @DisplayName("BP → JC → BP: el texto de cada lado, el mismo árbol al volver, y nada por la salida")
    void idaYVuelta() throws IOException {
        Path bp = escribe("Traduce.bp", BP);
        Traductor.Resultado[] r = new Traductor.Resultado[1];
        assertEquals("", loQueEscribe(() -> r[0] = Traductor.aJc(bp)), "el IDE vería esto en su consola");
        assertNull(r[0].error, r[0].error);
        String jc = r[0].texto;
        assertTrue(jc.contains("for (int i = 1; i <= n; i++)"), jc);
        assertTrue(jc.contains("println(\"vuelta\", \" \", i);"), jc);
        assertTrue(jc.contains("print(\"sin salto\");"), jc);

        Path jcf = escribe("Traduce.jc", jc);
        Traductor.Resultado[] v = new Traductor.Resultado[1];
        assertEquals("", loQueEscribe(() -> v[0] = Traductor.aBp(jcf)));
        assertNull(v[0].error, v[0].error);
        Path vuelta = escribe("Vuelta.bp", v[0].texto);
        Ast.ModuleNode a = new Parser(new Lexer(BP).tokenize()).parseModule();
        Ast.ModuleNode b = new Parser(new Lexer(new String(Files.readAllBytes(vuelta), StandardCharsets.UTF_8))
                .tokenize()).parseModule();
        assertNull(ComparaAst.primeraDiferenciaJc(a, b), v[0].texto);
    }

    @Test
    @DisplayName("lo que no se traduce dice por qué: un nombre reservado en JC, un JC que no se lee")
    void errores() throws IOException {
        Traductor.Resultado r = Traductor.aJc(escribe("Base.bp",
                "module Base1\n  function main()\n    var base: integer := 1\n    print base\n  end main\nend Base1\n"));
        assertNull(r.texto);
        assertTrue(r.error.startsWith("NO SOPORTADO") && r.error.contains("«base»"), r.error);

        Traductor.Resultado m = Traductor.aBp(escribe("Malo.jc", "module Malo\n{\n   int x = ;\n}\n"));
        assertNull(m.texto);
        assertTrue(m.error.startsWith("el original no se lee: [3:12]"), m.error);
    }
}
