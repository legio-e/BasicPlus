package basicplus.frontend;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

/**
 * EL ORÁCULO del relocalizador (V5/H8).
 *
 * <h3>Quién juzga</h3>
 * <b>El propio enlazador.</b> Se le da el mismo código enlazado en la dirección
 * destino y se comparan los bytes con lo que produce {@link NpackReloc}. No hay
 * ni una expectativa escrita a mano: si `ld` y nosotros discrepamos, el que se
 * equivoca somos nosotros.
 *
 * <p>Ésa es la diferencia entre este test y uno normal. Un test con valores
 * esperados sólo comprueba que el código hace lo que su autor CREÍA; aquí la
 * referencia es una herramienta que lleva treinta años relocalizando y que no
 * ha visto nuestro código.
 *
 * <h3>Uso</h3>
 * <pre>
 *   java basicplus.frontend.NpackRelocOraculo &lt;origen.elf&gt; &lt;referencia.elf&gt; \
 *        &lt;destino&gt; &lt;baseFlash&gt; &lt;baseRam&gt; [umbralRam]
 * </pre>
 * donde `origen.elf` está enlazado en la base baja (flash 0) CON
 * `--emit-relocs`, y `referencia.elf` es lo mismo enlazado ya en el destino.
 *
 * <p>Devuelve 0 si coinciden, 1 si difieren, 2 si no pudo ni intentarlo.
 */
public final class NpackRelocOraculo {

    private NpackRelocOraculo() { }

    public static void main(String[] args) throws Exception {
        if (args.length < 5) {
            System.err.println("Uso: NpackRelocOraculo <origen.elf> <referencia.elf>"
                    + " <arm-cortex-m33|riscv32-esp-p4> <baseFlash> <baseRam> [umbralRam]");
            System.exit(2);
        }
        Path origen = Paths.get(args[0]), refer = Paths.get(args[1]);
        NpackReloc.Destino d = NpackReloc.porNombre(args[2]);
        if (d == null) { System.err.println("destino desconocido: " + args[2]); System.exit(2); }
        int baseFlash = (int) Long.parseLong(args[3].replace("0x", ""), 16);
        int baseRam   = (int) Long.parseLong(args[4].replace("0x", ""), 16);
        int umbral    = (args.length >= 6)
                ? (int) Long.parseLong(args[5].replace("0x", ""), 16) : 0x20000000;

        System.out.println(Version.linea());
        System.out.println("oraculo del relocalizador | destino " + d.nombre);

        Elf32 e = Elf32.parse(Files.readAllBytes(origen));
        NpackReloc.Imagen img = NpackReloc.desdeElf(e, d, umbral);
        System.out.printf("  origen    : flash %d B (%s) | datos %d B (%s) | bss %d B%n",
                img.flash.length, NpackReloc.detalle(e, d.flashSecs),
                img.data.length,  NpackReloc.detalle(e, d.dataSecs), img.bssBytes);
        System.out.printf("  enlazado  : flash 0x%08X  ram 0x%08X%n", img.baseFlash, img.baseRam);
        System.out.printf("  destino   : flash 0x%08X  ram 0x%08X%n", baseFlash, baseRam);
        System.out.println("  a parchear: " + img.sitios.size() + " relocalizacion(es)"
                + " (las PC-relativas no se tocan: el codigo se mueve en bloque)");

        /* ── IDA Y VUELTA POR EL FICHERO (opcional, `--via-npk`) ────────────
         *
         * Serializa el `.npk` SIN relocalizar, lo vuelve a leer, y sigue desde
         * ahi. Si el formato pierde algo —un tipo, un addend, el bit de "el
         * sitio esta en data"— la comparacion final contra `ld` lo caza.
         *
         * Es un test barato y de los que valen: no comprueba que el fichero
         * "parece bien", comprueba que lo que sale de el RELOCALIZA IGUAL. */
        boolean viaFichero = false;
        for (String a : args) if ("--via-npk".equals(a)) viaFichero = true;
        if (viaFichero) {
            NpackFile.Meta meta = new NpackFile.Meta();
            meta.arch     = e.machine();
            meta.floatAbi = d.floatAbi;
            meta.entryOff = 0;              /* el mini no declara entrada */
            byte[] npk = NpackFile.escribirSinRelocalizar(img, meta);
            NpackFile.Leido rl = NpackFile.leer(npk, d, img.baseFlash, img.baseRam);
            System.out.printf("  via .npk  : %d B escritos | %d relocs releidas | %s%n",
                    npk.length, rl.imagen.sitios.size(),
                    rl.relocalizado ? "YA RELOCALIZADO (mal: deberia traer tabla)"
                                    : "sin relocalizar (correcto: lo hara el IDE)");
            if (rl.relocalizado) { System.out.println("== el .npk sin relocalizar salio SIN tabla =="); System.exit(1); }
            img = rl.imagen;
        }

        NpackReloc.Imagen puesta = NpackReloc.relocalizar(img, baseFlash, baseRam);

        /* La REFERENCIA: el mismo codigo que ya enlazo `ld` en el destino. */
        Elf32 r = Elf32.parse(Files.readAllBytes(refer));
        NpackReloc.Imagen ref = NpackReloc.desdeElf(r, d, baseRam);

        boolean okF = comparar("flash", puesta.flash, ref.flash);
        boolean okD = comparar("datos", puesta.data,  ref.data);

        if (okF && okD) {
            System.out.println("== IDENTICO al enlazador ==");
            System.exit(0);
        }
        System.out.println("== DIFIERE del enlazador ==");
        System.exit(1);
    }

    /**
     * Compara y, si difiere, dice DÓNDE. Un «difieren 900 bytes» no sirve para
     * nada; los primeros offsets sí, porque señalan la relocalización culpable.
     */
    private static boolean comparar(String cual, byte[] mio, byte[] suyo) {
        if (mio.length != suyo.length) {
            System.out.printf("  %-6s: TAMANO distinto — mio %d B, del enlazador %d B%n",
                    cual, mio.length, suyo.length);
            return false;
        }
        List<Integer> dif = new ArrayList<>();
        for (int i = 0; i < mio.length; i++) if (mio[i] != suyo[i]) dif.add(i);
        if (dif.isEmpty()) {
            System.out.printf("  %-6s: %d B IDENTICOS%n", cual, mio.length);
            return true;
        }
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < Math.min(8, dif.size()); i++) {
            if (i > 0) sb.append(' ');
            sb.append(String.format("0x%X", dif.get(i)));
        }
        System.out.printf("  %-6s: DIFIEREN %d de %d bytes; primeros offsets: %s%n",
                cual, dif.size(), mio.length, sb);
        int p = dif.get(0);
        System.out.printf("           en 0x%X: yo pongo 0x%08X, el enlazador 0x%08X%n",
                p & ~3, NpackReloc.leer32(mio, p & ~3), NpackReloc.leer32(suyo, p & ~3));
        return false;
    }
}
