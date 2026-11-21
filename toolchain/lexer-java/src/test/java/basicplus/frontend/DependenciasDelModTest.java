package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.junit.jupiter.api.io.TempDir;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V7/H8 #538 (28-sep) — las dependencias que el IDE sube a la placa salen de los {@code .mod} compilados
 * ({@link DependenciasDelMod}), no de las líneas {@code import} del fuente: el {@code Core} implícito, las de segunda
 * mano, un módulo de biblioteca ({@code com.acme.lib.LibMarcaBase}) y la regla de la stdlib primero.
 */
class DependenciasDelModTest {

    /** Compila {@code fuentes} (nombre → texto) como un proyecto con {@code main} y devuelve el directorio de salida. */
    private static Path compila(Path tmp, String main, Path stdlib, String[][] fuentes) throws Exception {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        for (String[] f : fuentes) Files.write(src.resolve(f[0]), f[1].getBytes("UTF-8"));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = main;
        String antes = Main.STDLIB_DIR;
        try {
            Main.STDLIB_DIR = stdlib.toString();
            assertTrue(Main.buildProject(b, "mivm", false), "debe compilar sin errores");
        } finally {
            Main.STDLIB_DIR = antes;
        }
        return out;
    }

    private static List<String> nombres(List<Path> ps) {
        List<String> r = new ArrayList<>();
        for (Path p : ps) r.add(p.getFileName().toString());
        return r;
    }

    @Test
    @Timeout(120)
    @DisplayName("sin ningún import escrito: Core, que lo pone el parser")
    void coreImplicito(@TempDir Path tmp) throws Exception {
        Path std = StdlibDeTest.paraFormato(tmp).toPath();
        Path out = compila(tmp, "SinImports", std, new String[][] {{"SinImports.bp",
                "module SinImports\n  function main()\n    print \"hola\"\n  end main\nend SinImports\n"}});
        List<Path> deps = DependenciasDelMod.paraSubir(out.resolve("SinImports.mod"), Arrays.asList(out, std), std);
        assertTrue(nombres(deps).contains("Core.mod"), "falta Core: " + nombres(deps));
        assertEquals(std.resolve("Core.mod").toAbsolutePath().normalize(),
                deps.get(nombres(deps).indexOf("Core.mod")).toAbsolutePath().normalize(), "Core, de la stdlib");
    }

    @Test
    @Timeout(120)
    @DisplayName("import Machine: Machine y Core; import Gui: también Json, de segunda mano")
    void directasYTransitivas(@TempDir Path tmp) throws Exception {
        Path std = StdlibDeTest.paraFormato(tmp).toPath();
        Path out = compila(tmp, "UsaGui", std, new String[][] {{"UsaGui.bp",
                "module UsaGui\n  import Machine\n  import Gui\n  function main()\n"
                        + "    print Machine.getMicro()\n    Gui.setRotation(0)\n  end main\nend UsaGui\n"}});
        List<String> deps = nombres(DependenciasDelMod.paraSubir(out.resolve("UsaGui.mod"), Arrays.asList(out, std), std));
        for (String m : new String[] {"Machine.mod", "Gui.mod", "Core.mod", "Json.mod"}) {
            assertTrue(deps.contains(m), "falta " + m + ": " + deps);
        }
        assertFalse(deps.contains("UsaGui.mod"), "el principal no es una dependencia suya: " + deps);
    }

    @Test
    @Timeout(120)
    @DisplayName("un módulo de biblioteca: com.acme.lib.LibMarcaBase (la regex cogía «com»)")
    void moduloDeBiblioteca(@TempDir Path tmp) throws Exception {
        Path std = StdlibDeTest.paraFormato(tmp).toPath();
        Path samples = Paths.get("../../bpgenvm-c/samples");
        Path out = compila(tmp, "LibMarcaUsa", std, new String[][] {
                {"LibMarcaUsa.bp", new String(Files.readAllBytes(samples.resolve("LibMarcaUsa.bp")), "UTF-8")},
                {"LibMarcaBase.bp", new String(Files.readAllBytes(samples.resolve("LibMarcaBase.bp")), "UTF-8")}});
        List<String> deps = nombres(DependenciasDelMod.paraSubir(out.resolve("LibMarcaUsa.mod"), Arrays.asList(out, std), std));
        assertTrue(deps.contains("com.acme.lib.LibMarcaBase.mod"), "falta el de la biblioteca: " + deps);
        assertTrue(deps.contains("Core.mod"), "falta Core: " + deps);
    }

    @Test
    @Timeout(120)
    @DisplayName("la stdlib primero: un Core.mod rancio en out/ no gana al de la stdlib")
    void stdlibPrimero(@TempDir Path tmp) throws Exception {
        Path std = StdlibDeTest.paraFormato(tmp).toPath();
        Path out = compila(tmp, "SinImports", std, new String[][] {{"SinImports.bp",
                "module SinImports\n  function main()\n    print \"hola\"\n  end main\nend SinImports\n"}});
        Files.write(out.resolve("Core.mod"), new byte[] {1, 2, 3});   // un Core «rancio» en el out/ del proyecto
        List<Path> deps = DependenciasDelMod.paraSubir(out.resolve("SinImports.mod"), Arrays.asList(out, std), std);
        Path core = deps.get(nombres(deps).indexOf("Core.mod"));
        assertEquals(std.resolve("Core.mod").toAbsolutePath().normalize(), core.toAbsolutePath().normalize(),
                "tiene que salir el Core de la stdlib, no el de out/");
    }
}
