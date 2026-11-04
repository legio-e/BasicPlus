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
 * V8/H8 (4-nov) — `break` y `continue` a un `do` con nombre. Que salen igual en las dos VMs, con sus `try` y sus `finally`, lo prueba
 * BucleConNombre, del corpus; en JC, el mismo corpus; en el AOT, AotCoberturaTest. Aquí, lo que el compilador deja y lo que no.
 */
class BucleConNombreTest {

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

    private static String modulo(String nombre, String... cuerpo) {
        return "module " + nombre + "\n  import Core\n  function Main()\n" + String.join("\n", cuerpo) + "\n  end Main\nend " + nombre + "\n";
    }

    @Test @DisplayName("break y continue a un do abierto, desde un for y un while")
    void aUnoAbierto(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Vale", modulo("Vale",
            "    do fuera", "      for i := 1 to 2 do", "        var k: integer := 0", "        while k < 2 do",
            "          k += 1", "          if k == 1 then", "            continue fuera", "          endif", "          break fuera",
            "        endwh", "      next i", "    loop false"));
        assertTrue((Boolean) r[0], "tiene que compilar:\n" + r[1]);
    }

    @Test @DisplayName("un nombre que no es de un do abierto: el error lo dice")
    void noAbierto(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Mal", modulo("Mal", "    do a", "      print 1", "    loop false", "    while true do", "      break a", "    endwh"));
        assertFalse((Boolean) r[0]);
        assertTrue(((String) r[1]).contains("'break a': no hay ningún bucle 'do a' abierto"), (String) r[1]);
    }

    @Test @DisplayName("dos abiertos con el mismo nombre, no")
    void repetido(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Mal2", modulo("Mal2", "    do a", "      do a", "        break a", "      loop false", "    loop false"));
        assertFalse((Boolean) r[0]);
        assertTrue(((String) r[1]).contains("ya hay un bucle 'a' abierto"), (String) r[1]);
    }

    @Test @DisplayName("salir de un finally, no; un bucle de dentro del finally, sí")
    void finallyNoSale(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Mal3", modulo("Mal3", "    do a", "      try", "        print 1", "      finally", "        continue a",
            "      endtry", "    loop false"));
        assertFalse((Boolean) r[0]);
        assertTrue(((String) r[1]).contains("un 'finally' no puede salir con 'continue'"), (String) r[1]);
        Object[] r2 = compila(tmp, "Vale2", modulo("Vale2", "    try", "      print 1", "    finally", "      do b", "        break b",
            "      loop false", "    endtry"));
        assertTrue((Boolean) r2[0], "tiene que compilar:\n" + r2[1]);
    }
}
