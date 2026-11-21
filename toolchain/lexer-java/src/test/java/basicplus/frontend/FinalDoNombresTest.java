package basicplus.frontend;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * #842, #843 y #844 (19-nov; salieron al escribir el volumen de JC del D1 de V8). #842: una clase `final` no se extiende
 * (manual §9.6) —en el mismo módulo, desde otro por su interfaz (bpi 13), y en JC—, y la traducción BP ⇄ JC la conserva.
 * #843: en JC, `do i++; while (c);` —un `do` sin llaves— compila; el nombre del bucle va delante de su llave. #844: la
 * traducción JC → BP para ante un nombre que en BP es una palabra reservada, como hace la de BP → JC con las de JC.
 */
class FinalDoNombresTest {

    @TempDir Path tmp;

    @BeforeEach
    void stdlib() throws Exception { Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath(); }

    @AfterEach
    void limpia() { Main.STDLIB_DIR = null; }

    /** Como en CompilaJcTest: un proyecto con estos ficheros; {ok, lo que dijo el compilador}. */
    private Object[] compila(String dir, String main, String... ficheros) throws Exception {
        Path base = tmp.resolve(dir);
        Path src = Files.createDirectories(base.resolve("src"));
        Path out = Files.createDirectories(base.resolve("out"));
        for (int i = 0; i < ficheros.length; i += 2)
            Files.write(src.resolve(ficheros[i]), ficheros[i + 1].getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = base.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = main;
        PrintStream out0 = System.out, err0 = System.err;
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(buf, true, "UTF-8");
        boolean ok;
        try {
            System.setOut(ps);
            System.setErr(ps);
            ok = Main.buildProject(b, "mivm", false);
        } finally {
            System.setOut(out0);
            System.setErr(err0);
        }
        return new Object[]{ ok, new String(buf.toByteArray(), StandardCharsets.UTF_8) };
    }

    private Path escribe(String nombre, String texto) throws Exception {
        Path f = tmp.resolve(nombre);
        Files.write(f, texto.getBytes(StandardCharsets.UTF_8));
        return f;
    }

    // ------------------------------------------------------------ #842

    private static String claseBase(boolean esFinal) {
        return "  public " + (esFinal ? "final " : "") + "class Base\n"
             + "    public function Base()\n    end Base\n  end Base\n";
    }

    private static final String HIJA_Y_MAIN = "  public class Hija extends Base\n"
            + "    public function Hija()\n    end Hija\n  end Hija\n"
            + "  function Main()\n    var h: Hija := Hija()\n    print h == null\n  end Main\n";

    @Test @DisplayName("#842: una clase final no se extiende en su módulo; sin final, sí")
    void finalEnSuModulo() throws Exception {
        Object[] mal = compila("mismo", "T", "T.bp", "module T\n" + claseBase(true) + HIJA_Y_MAIN + "end T\n");
        assertFalse((Boolean) mal[0], "tenía que fallar:\n" + mal[1]);
        assertTrue(((String) mal[1]).contains("la clase 'Base' es final: no se puede extender"), (String) mal[1]);

        Object[] bien = compila("control", "T", "T.bp", "module T\n" + claseBase(false) + HIJA_Y_MAIN + "end T\n");
        assertTrue((Boolean) bien[0], (String) bien[1]);
    }

    @Test @DisplayName("#842: tampoco desde otro módulo: el final viaja en su interfaz")
    void finalDesdeOtroModulo() throws Exception {
        String usa = "module T\n  import Lib\n"
                + "  public class Hija extends Lib.Base\n    public function Hija()\n    end Hija\n  end Hija\n"
                + "  function Main()\n    var h: Hija := Hija()\n    print h == null\n  end Main\nend T\n";
        Object[] mal = compila("otro", "T", "Lib.bp", "module Lib\n" + claseBase(true) + "end Lib\n", "T.bp", usa);
        assertFalse((Boolean) mal[0], "tenía que fallar:\n" + mal[1]);
        assertTrue(((String) mal[1]).contains("la clase 'Lib.Base' es final: no se puede extender"), (String) mal[1]);

        Object[] bien = compila("otro-control", "T", "Lib.bp", "module Lib\n" + claseBase(false) + "end Lib\n", "T.bp", usa);
        assertTrue((Boolean) bien[0], (String) bien[1]);
    }

    @Test @DisplayName("#842: en JC, igual")
    void finalEnJc() throws Exception {
        String jc = "module T\n{\n   public final class Base\n   {\n   }\n   public class Hija : Base\n   {\n   }\n"
                + "   function void Main()\n   {\n      Hija h = new Hija();\n      println(boolToString(h == null));\n   }\n}\n";
        Object[] mal = compila("jc", "T", "T.jc", jc);
        assertFalse((Boolean) mal[0], "tenía que fallar:\n" + mal[1]);
        assertTrue(((String) mal[1]).contains("la clase 'Base' es final: no se puede extender"), (String) mal[1]);

        Object[] bien = compila("jc-control", "T", "T.jc", jc.replace("public final class", "public class"));
        assertTrue((Boolean) bien[0], (String) bien[1]);
    }

    @Test @DisplayName("#842: la traducción BP → JC → BP conserva el final")
    void finalAlTraducir() throws Exception {
        Path bp = escribe("Fin.bp", "module Fin\n" + claseBase(true) + "  function Main()\n    var b: Base := Base()\n"
                + "    print b == null\n  end Main\nend Fin\n");
        Traductor.Resultado aJc = Traductor.aJc(bp);
        assertNull(aJc.error, aJc.error);
        assertTrue(aJc.texto.contains("public final class Base"), aJc.texto);
        Traductor.Resultado aBp = Traductor.aBp(escribe("Fin.jc", aJc.texto));
        assertNull(aBp.error, aBp.error);
        assertTrue(aBp.texto.contains("public final class Base"), aBp.texto);
    }

    // ------------------------------------------------------------ #843

    @Test @DisplayName("#843: en JC, un do sin llaves compila; y el do con nombre, con sus llaves, también")
    void doSinLlaves() throws Exception {
        String sinLlaves = "module D\n{\n   function void Main()\n   {\n      int i = 0;\n      do i++; while (i < 3);\n"
                + "      println(intToString(i));\n   }\n}\n";
        Object[] r = compila("do", "D", "D.jc", sinLlaves);
        assertTrue((Boolean) r[0], (String) r[1]);

        String conNombre = "module E\n{\n   function void Main()\n   {\n      int i = 0;\n"
                + "      do vuelta { i++; } while (i < 3);\n      println(intToString(i));\n   }\n}\n";
        Object[] n = compila("do-nombre", "E", "E.jc", conNombre);
        assertTrue((Boolean) n[0], (String) n[1]);
    }

    // ------------------------------------------------------------ #844

    @Test @DisplayName("#844: JC → BP para ante un nombre reservado en BP (step, Next), sin renombrarlo")
    void nombreReservadoEnBp() throws Exception {
        Traductor.Resultado step = Traductor.aBp(escribe("Paso.jc", "module Paso\n{\n   function void Main()\n   {\n"
                + "      int step = 2;\n      println(intToString(step * 3));\n   }\n}\n"));
        assertNull(step.texto, step.texto);
        assertTrue(step.error.contains("no soportado: el nombre «step», que en BP es una palabra reservada (línea 5"),
                step.error);

        Traductor.Resultado next = Traductor.aBp(escribe("Sig.jc", "module Sig\n{\n   function void Next()\n   {\n   }\n"
                + "   function void Main()\n   {\n      Next();\n   }\n}\n"));
        assertNull(next.texto, next.texto);
        assertTrue(next.error.contains("«Next»") && next.error.contains("en BP es una palabra reservada"), next.error);

        Traductor.Resultado bien = Traductor.aBp(escribe("Bien.jc", "module Bien\n{\n   function void Main()\n   {\n"
                + "      int paso = 2;\n      println(intToString(paso * 3));\n   }\n}\n"));
        assertNull(bien.error, bien.error);
    }
}
