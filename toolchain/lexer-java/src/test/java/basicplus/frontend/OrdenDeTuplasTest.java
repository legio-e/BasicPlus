package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * V8 #704 (24-oct) — el MISMO programa da el MISMO .mod. Las clases ocultas de las tuplas ({@code __Tuple_<forma>})
 * salían en el orden de un IdentityHashMap ({@code info.exprTypes}), el de los hashCode de identidad de los nodos, que
 * dependen de lo que la JVM haya hecho antes: con otro compilador, o compilando dos veces en la misma JVM, otro orden y
 * otro .mod, sin cambiar lo que hace. Y dos formas que caen en la misma clase (todas las referencias son {@code r})
 * dejaban en sus campos la clave del tipo de la última que se miraba.
 *
 * <p>Aquí: un módulo que usa tuplas de otros —las de Str y Time, y dos de referencias que caen en {@code __Tuple_ri}—,
 * compilado {@link #VECES} veces en la misma JVM, removiendo la memoria entre una y otra para que los hashCode de
 * identidad salgan otros: tiene que salir el mismo .mod byte a byte.
 */
class OrdenDeTuplasTest {

    private static final int VECES = 12;

    private static final String LIB =
          "module TupRefs\n"
        + "  public class Perro\n"
        + "    public function Perro()\n"
        + "    end Perro\n"
        + "  end Perro\n"
        + "  public class Gato\n"
        + "    public function Gato()\n"
        + "    end Gato\n"
        + "  end Gato\n"
        + "  public function unPerro(): (Perro, integer)\n"
        + "    return (Perro(), 1)\n"
        + "  end unPerro\n"
        + "  public function unGato(): (Gato, integer)\n"
        + "    return (Gato(), 2)\n"
        + "  end unGato\n"
        + "end TupRefs\n";

    private static final String MAIN =
          "module TupUsa\n"
        + "  import Str\n"
        + "  import Time\n"
        + "  import TupRefs\n"
        + "  function Main()\n"
        + "    var ok: boolean\n"
        + "    var n: integer\n"
        + "    var l: long\n"
        + "    var d: double\n"
        + "    var a: integer\n"
        + "    var m: integer\n"
        + "    var dia: integer\n"
        + "    { ok, n } := Str.parseInt(\"12\")\n"
        + "    { ok, l } := Str.parseLong(\"34\")\n"
        + "    { ok, d } := Str.parseDouble(\"5.5\")\n"
        + "    { a, m, dia } := Time.civilDesdeDias(0)\n"
        + "    var p: TupRefs.Perro\n"
        + "    var g: TupRefs.Gato\n"
        + "    { p, n } := TupRefs.unPerro()\n"
        + "    { g, n } := TupRefs.unGato()\n"
        + "    print n\n"
        + "  end Main\n"
        + "end TupUsa\n";

    /** Compila el proyecto en {@code dir} y devuelve los bytes de TupUsa.mod. */
    private static byte[] compila(Path tmp, Path dir) throws Exception {
        Path src = Files.createDirectories(dir.resolve("src"));
        Path out = Files.createDirectories(dir.resolve("out"));
        Files.write(src.resolve("TupRefs.bp"), LIB.getBytes(StandardCharsets.UTF_8));
        Files.write(src.resolve("TupUsa.bp"), MAIN.getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = dir.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = "TupUsa";
        String antes = Main.STDLIB_DIR;
        try {
            Main.STDLIB_DIR = StdlibDeTest.paraFormato(tmp).getAbsolutePath();
            assertTrue(Main.buildProject(b, "mivm", false), "debe compilar sin errores");
        } finally {
            Main.STDLIB_DIR = antes;
        }
        return Files.readAllBytes(out.resolve("TupUsa.mod"));
    }

    /** Remueve la memoria: objetos nuevos con su hashCode de identidad pedido, para que los de la compilación
     *  siguiente salgan otros (el generador de la JVM avanza con cada uno). */
    private static void remueve(int vuelta) {
        int suma = 0;
        for (int k = 0; k < 997 * (vuelta + 1); k++) suma += System.identityHashCode(new Object());
        if (suma == 42) System.out.print("");   // que el bucle no se quite
    }

    @Test
    @DisplayName("el mismo programa, compilado varias veces en la misma JVM, da el mismo .mod")
    void elMismoProgramaDaElMismoMod(@TempDir Path tmp) throws Exception {
        byte[] primero = null;
        int distintos = 0;
        for (int i = 0; i < VECES; i++) {
            remueve(i);
            byte[] mod = compila(tmp, tmp.resolve("v" + i));
            if (primero == null) primero = mod;
            else if (!Arrays.equals(primero, mod)) distintos++;
        }
        assertEquals(0, distintos, "de " + VECES + " compilaciones del mismo programa, " + distintos
                + " dieron otro .mod que la primera");
    }
}
