import com.mycompany.bpide.AotBuild;
import com.mycompany.bpide.IdePrefs;
import java.nio.file.Paths;

/** El AOT de UN .bp por linea de comandos, como el Run del IDE (`FrmMain.runAotPass` →
 *  `AotBuild.buildSingle`): compila las `native` para `target` y las FUNDE en el `<Mod>.mod` de
 *  outDir si esta (N1.4); si no, deja el `.mdn` suelto.
 *      AotSingle <fichero.bp> <outDir> <arm|riscv|riscv-imc|receta:NOMBRE>
 *  V7/H5 (25-sep) — `receta:NOMBRE` (la `aotReceta` del INFO de la placa, p. ej. RV32IMC_ILP32) elige
 *  el destino por la receta, como el IDE: la tanda la pasa tal cual y el catalogo de recetas y
 *  destinos sigue en UN sitio (NpackReloc), no copiado en Python. */
public class AotSingle {
    public static void main(String[] a) throws Exception {
        String target = a[2];
        if (target.startsWith("receta:")) {
            String receta = target.substring("receta:".length());
            basicplus.frontend.NpackReloc.Destino d = basicplus.frontend.NpackReloc.porReceta(receta);
            if (d == null) {
                System.out.println("[aot] ningun destino tiene la receta " + receta + ": el IDE tampoco sabria compilar para ella");
                System.exit(2);
            }
            System.out.println("[aot] receta " + receta + " -> destino " + d.targetAot);
            target = d.targetAot;
        }
        AotBuild.Result r = AotBuild.buildSingle(Paths.get(a[0]), Paths.get(a[1]), Paths.get(a[1]),
                target, IdePrefs.load(), s -> System.out.println("[aot] " + s));
        for (String w : r.warnings) System.out.println("[aot] AVISO: " + w);
        System.out.println("[aot] toolchainMissing=" + r.toolchainMissing + " mdn=" + r.mdnFiles);
        if (r.toolchainMissing) System.exit(1);
    }
}
