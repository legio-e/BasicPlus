import com.mycompany.bpide.AotBuild;
import com.mycompany.bpide.IdePrefs;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/** Los `.mdn` de un pack por linea de comandos — el MISMO paso que el IDE da antes
 *  de montar un pack (`FrmMain.pasoAotDelPack` → `AotBuild.buildPackTargets`), que
 *  el CLI del frontend no tiene: `--project` monta el pack con los `.mdn` que haya
 *  en outDir y calla si faltan. Uso:
 *      PackAot <sourceDir> <outDir> <projectDir> <arm,riscv> <fuente.bp>...
 *  Deja `<Mod>.mdn.ARMV8` / `<Mod>.mdn.RISCV` en outDir (con la doble extension). */
public class PackAot {
    public static void main(String[] a) throws Exception {
        Path src = Paths.get(a[0]), out = Paths.get(a[1]), proj = Paths.get(a[2]);
        List<String> targets = Arrays.asList(a[3].split(","));
        List<Path> fuentes = new ArrayList<>();
        for (int i = 4; i < a.length; i++) fuentes.add(Paths.get(a[i]));
        AotBuild.Result r = AotBuild.buildPackTargets(src, fuentes.isEmpty() ? null : fuentes,
                out, proj, targets, IdePrefs.load(), s -> System.out.println("[aot] " + s));
        for (String w : r.warnings) System.out.println("[aot] AVISO: " + w);
        System.out.println("[aot] toolchainMissing=" + r.toolchainMissing + " mdn=" + r.mdnFiles);
        if (r.toolchainMissing || r.mdnFiles.isEmpty()) System.exit(1);
    }
}
