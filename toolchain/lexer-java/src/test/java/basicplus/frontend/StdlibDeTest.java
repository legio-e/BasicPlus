package basicplus.frontend;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import org.junit.jupiter.api.Assumptions;

/**
 * H1 P3 (20-sep) — la stdlib que necesita un test que COMPILE una clase. Desde que
 * {@code Object} vive en {@code Core.bp} (como {@code List} desde #450), toda clase desciende
 * de {@code Core.Object} y el compilador tiene que ver {@code Core.mod}: igual que cualquier
 * proyecto real, que la recibe del IDE o del {@code .bpbuild}. Si no está, se dice por qué.
 *
 * <p>En MOD9 vale la versionada ({@code ../../bpstdlib}). En MOD8 (formato de emergencia) no: una
 * clase con base externa necesita el LAYOUT de la base, que sólo publica la interfaz bpi 8, así
 * que se compila {@code Core.bp} como MOD8 en el temporal del test (Core no importa nada).
 */
final class StdlibDeTest {
    private StdlibDeTest() {}

    /** La carpeta con el {@code Core.mod} adecuado al {@link Main#FORMATO} vigente. */
    static File paraFormato(Path tmp) throws IOException {
        File std = new File("../../bpstdlib");
        Assumptions.assumeTrue(new File(std, "Core.bp").isFile(), "hace falta ../../bpstdlib/Core.bp");
        if (Main.FORMATO != 8) {
            Assumptions.assumeTrue(new File(std, "Core.mod").isFile(),
                    "hace falta ../../bpstdlib/Core.mod (compila la stdlib antes de este test)");
            return std.getAbsoluteFile();
        }
        Path std8 = Files.createDirectories(tmp.resolve("std8"));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = std.getAbsolutePath();
        b.outDir     = std8.toString();
        b.main       = "Core";
        b.version    = 7;
        if (!Main.buildProject(b, "mivm", false))
            throw new IOException("no se pudo compilar Core.bp como MOD8 para el test");
        return std8.toFile();
    }
}
