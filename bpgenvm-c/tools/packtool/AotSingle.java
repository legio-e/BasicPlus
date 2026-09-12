import com.mycompany.bpide.AotBuild;
import com.mycompany.bpide.IdePrefs;
import java.nio.file.Paths;

/** El AOT de UN .bp por linea de comandos, como el Run del IDE (`FrmMain.runAotPass` →
 *  `AotBuild.buildSingle`): compila las `native` para `target` (arm | riscv) y las FUNDE en
 *  el `<Mod>.mod` de outDir si esta (N1.4); si no, deja el `.mdn` suelto.
 *      AotSingle <fichero.bp> <outDir> <arm|riscv> */
public class AotSingle {
    public static void main(String[] a) throws Exception {
        AotBuild.Result r = AotBuild.buildSingle(Paths.get(a[0]), Paths.get(a[1]), Paths.get(a[1]),
                a[2], IdePrefs.load(), s -> System.out.println("[aot] " + s));
        for (String w : r.warnings) System.out.println("[aot] AVISO: " + w);
        System.out.println("[aot] toolchainMissing=" + r.toolchainMissing + " mdn=" + r.mdnFiles);
        if (r.toolchainMissing || r.mdnFiles.isEmpty()) System.exit(1);
    }
}
