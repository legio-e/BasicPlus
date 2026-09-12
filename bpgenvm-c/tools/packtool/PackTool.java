import basicplus.frontend.NpackReloc;
import basicplus.frontend.PackBurn;
import java.nio.file.Files;
import java.nio.file.Paths;

/** Sella un pack para UNA placa, como el panel de Packs del IDE (PackBurn.podar + sellar).
 *  Uso: PackTool podar <pack> <arch|-> <salida>          -> escribe el pack podado (tamano FINAL)
 *       PackTool sellar <pack> <arch> <flashAddr> <ramBase> <salida>  -> podado + sellado */
public class PackTool {
    public static void main(String[] a) throws Exception {
        String op = a[0];
        byte[] img = Files.readAllBytes(Paths.get(a[1]));
        NpackReloc.Destino d = a[2].equals("-") ? null : NpackReloc.porTargetAot(a[2]);
        if (!a[2].equals("-") && d == null) { System.err.println("arch desconocida: " + a[2]); System.exit(2); }
        PackBurn.Preparado prep = PackBurn.podar(img, d);
        for (String s : prep.detalle) System.out.println("  " + s);
        if (op.equals("podar")) {
            Files.write(Paths.get(a[3]), prep.bytes);
            System.out.println("podado: " + prep.bytes.length + " B, necesitaDireccion=" + prep.necesitaDireccion
                               + ", relocalizaciones=" + prep.relocalizaciones);
            return;
        }
        long flash = Long.decode(a[3]), ram = Long.decode(a[4]);
        byte[] out = prep.bytes;
        if (prep.necesitaDireccion) {
            int codigo = PackBurn.baseDelCodigo(prep, (int) flash);
            System.out.println(String.format("pack en 0x%08X · motor en 0x%08X · RAM 0x%08X — %d sitio(s)",
                                             flash, codigo, ram, prep.relocalizaciones));
            out = PackBurn.sellar(prep, codigo, (int) ram);
        }
        Files.write(Paths.get(a[5]), out);
        System.out.println("sellado: " + out.length + " B");
    }
}
