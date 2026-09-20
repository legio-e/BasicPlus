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
 * V7/H2 `#521` — un `.mod` con ERRORES DEL EMISOR no se escribe. Antes se escribía igual,
 * Main decía «compilado:», y el programa enlazaba y corría con basura idéntica en las dos
 * VMs (así se coló `#524`: la intrínseca desconocida daba error de emisor y aun así salía
 * un `.mod` con `CALL_BUILTIN 3`). Se afirma: build en rojo, «NO compilado» con el error,
 * ni `.mod` ni `.dbg` en el out — y el gemelo rancio de una compilación anterior, borrado.
 */
class EmisorConErroresTest {

    /** Un fuente VÁLIDO: el error del emisor se fuerza con `MivmEmitter.PRUEBA_ERROR_DE_EMISOR`.
     *  Hasta el 20-sep (mañana) el caso era una `intrinsic` sin registro; hasta el 20-sep (tarde),
     *  asignar a un MÉTODO (`c.m := 5`), que ahora rechaza el semántico con línea (ver abajo). No
     *  queda ningún error que sólo dé el emisor desde un fuente válido — que es lo deseable. */
    private static final String REFL = String.join("\n",
        "module Refl", "import Core",
        "public class C", "  public function C()", "  end C",
        "  public function m(): integer", "    return 1", "  end m", "end C",
        "public function main()", "  var c: C := C()", "  print c.m()", "end main",
        "end Refl", "");

    private static Object[] compila(Path tmp, String main, String[]... fuentes) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        for (String[] f : fuentes) Files.write(src.resolve(f[0] + ".bp"), f[1].getBytes(StandardCharsets.UTF_8));
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

    @Test @DisplayName("errores del emisor: build en rojo, «NO compilado», y ni .mod ni .dbg (el rancio, borrado)")
    void sinArtefacto(@TempDir Path tmp) throws IOException {
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(out.resolve("Refl.mod"), new byte[]{ 1, 2, 3 });   // el gemelo rancio
        Files.write(out.resolve("Refl.dbg"), new byte[]{ 4 });
        Object[] r;
        MivmEmitter.PRUEBA_ERROR_DE_EMISOR = true;
        try { r = compila(tmp, "Refl", new String[]{"Refl", REFL}); }
        finally { MivmEmitter.PRUEBA_ERROR_DE_EMISOR = false; }
        String t = (String) r[1];
        assertFalse((Boolean) r[0], "el build tiene que fallar:\n" + t);
        assertTrue(t.contains("[prueba] error del emisor forzado"), t);
        assertTrue(t.contains("NO compilado:"), t);
        for (String linea : t.split("\n"))
            assertFalse(linea.trim().startsWith("compilado:"), "no puede decir «compilado:»: " + linea);
        assertFalse(Files.exists(out.resolve("Refl.mod")), "el .mod rancio sigue ahi");
        assertFalse(Files.exists(out.resolve("Refl.dbg")), "el .dbg rancio sigue ahi");
    }
    @Test @DisplayName("asignar a un método (c.m := 5) lo rechaza el SEMÁNTICO, con línea")
    void asignarAUnMetodo(@TempDir Path tmp) throws IOException {
        String src = REFL.replace("  print c.m()", "  c.m := 5");
        Object[] r = compila(tmp, "Refl", new String[]{"Refl", src});
        String t = (String) r[1];
        assertFalse((Boolean) r[0], "el build tiene que fallar:\n" + t);
        assertTrue(t.contains("'m' es un método, no un campo"), t);
        assertTrue(t.contains("[12:"), "con la línea: " + t);
    }
}
