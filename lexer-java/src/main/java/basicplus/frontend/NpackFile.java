package basicplus.frontend;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * El fichero `.npk` — leer y escribir (V5/H8).
 *
 * <h3>Dos estados del mismo fichero</h3>
 * <table>
 *   <tr><th></th><th>reloc_count</th><th>sello</th><th>lleva tabla</th></tr>
 *   <tr><td><b>distribuido</b></td><td>&gt; 0</td><td>a cero</td><td>sí</td></tr>
 *   <tr><td><b>grabado</b></td><td>0</td><td>puesto</td><td>no</td></tr>
 * </table>
 *
 * Que el contador sea 0 ES la prueba de que pasó por el IDE — no hay un campo
 * «ya_realojado» aparte que pueda contradecirlo. El dispositivo lo comprueba y
 * rechaza el distribuido con `E_SIN_RELOC` diciendo qué hacer.
 *
 * <h3>La tabla es un contrato SÓLO DEL PC</h3>
 * Esto es lo que hace barato el paso: <b>el firmware nunca parsea la tabla</b>.
 * Mira `reloc_count != 0` y para ahí — no necesita saber cómo es una entrada.
 * O sea que este formato se puede definir, cambiar y ampliar sin tocar una
 * línea de C ni arriesgar nada de lo que ya corre en placa.
 *
 * <p>Por eso la pinza de D7 (mismo contrato clavado en los dos lados) NO aplica
 * aquí: sólo hay un lado. La que sí sigue valiendo es la de la CABECERA, que
 * lee el dispositivo y que este escritor respeta byte a byte.
 *
 * <h3>Reparto</h3>
 * <pre>
 *   +0                          cabecera                    64 B
 *   +64                         imagen de flash             flash_bytes
 *   +64+flash_bytes             imagen inicial de .data     data_bytes
 *   +64+flash_bytes+data_bytes  la tabla                    reloc_count * 16
 * </pre>
 * La tabla va DETRÁS de todo a propósito: así el pack grabado es exactamente
 * este fichero truncado, y las imágenes no se mueven ni un byte al podarla.
 */
public final class NpackFile {

    private NpackFile() { }

    /** Cabecera: 64 B. Lo fija `BPVM_NPACK_HDR_BYTES` del lado C. */
    public static final int HDR_BYTES  = 64;
    public static final int MAGIC      = 0x42504E50;   /* 'BPNP' */
    public static final int FORMAT     = 1;
    /** Bytes por entrada de la tabla. Sólo lo lee este fichero. */
    public static final int RELOC_ENTRY = 16;
    /** `flags` de la cabecera: la entrada es Thumb → bit 0 al saltar. */
    public static final int F_THUMB    = 0x1;

    /** Bits del byte de flags de UNA entrada de la tabla. */
    private static final int R_SITIO_EN_DATA = 0x1;
    private static final int R_DESTINO_RAM   = 0x2;

    /** Lo que hace falta para escribir un `.npk`, además de la imagen. */
    public static final class Meta {
        public String  floatAbi = "";   /* "softfp" / "ilp32f" … */
        public int     arch;            /* MDN_ARCH_* */
        public int     entryOff;        /* offset de la entrada, SIN el bit Thumb */
        public boolean entryThumb;      /* ARM: la funcion es Thumb */
    }

    /**
     * Escribe el `.npk` SIN RELOCALIZAR: con su tabla y el sello a cero.
     *
     * <p>Es lo que se distribuye. El dispositivo lo rechaza a propósito
     * (`E_SIN_RELOC`) porque todavía no tiene dirección: la pone el IDE al
     * grabar.
     */
    public static byte[] escribirSinRelocalizar(NpackReloc.Imagen img, Meta meta) {
        return escribir(img, meta, /*sello*/ false, /*conTabla*/ true);
    }

    /**
     * Escribe el `.npk` YA RELOCALIZADO: sello puesto y `reloc_count = 0`.
     * Es lo que se graba. La tabla ya no viaja — hizo su trabajo.
     */
    public static byte[] escribirGrabable(NpackReloc.Imagen img, Meta meta) {
        return escribir(img, meta, /*sello*/ true, /*conTabla*/ false);
    }

    private static byte[] escribir(NpackReloc.Imagen img, Meta meta,
                                   boolean sello, boolean conTabla) {
        byte[] h = new byte[HDR_BYTES];
        NpackReloc.pon32(h,  0, MAGIC);
        pon16(h,  4, FORMAT);
        pon16(h,  6, meta.arch);
        ponCadena(h, 8, meta.floatAbi, 8);          /* char float_abi[8] */
        NpackReloc.pon32(h, 16, meta.entryOff);
        NpackReloc.pon32(h, 20, meta.entryThumb ? F_THUMB : 0);
        NpackReloc.pon32(h, 24, img.flash.length);
        NpackReloc.pon32(h, 28, img.data.length);
        NpackReloc.pon32(h, 32, img.bssBytes);
        /* El SELLO. En el distribuido va a CERO y no es un descuido: nadie le
         * ha dado sitio todavia. Juzgarlo antes de relocalizar seria juzgar una
         * direccion que no existe — por eso la escalera mira `reloc_count`
         * ANTES que el sello (arreglado el 12-ago; ver bpvm_npack.c). */
        NpackReloc.pon32(h, 36, sello ? img.baseFlash : 0);
        NpackReloc.pon32(h, 40, sello ? img.baseRam   : 0);
        NpackReloc.pon32(h, 44, conTabla ? img.sitios.size() : 0);
        /* 48..63 = reserved[4], ya a cero. */

        ByteArrayOutputStream out = new ByteArrayOutputStream();
        out.write(h, 0, h.length);
        out.write(img.flash, 0, img.flash.length);
        out.write(img.data,  0, img.data.length);
        if (conTabla) {
            for (NpackReloc.Sitio s : img.sitios) {
                byte[] e = new byte[RELOC_ENTRY];
                NpackReloc.pon32(e, 0, s.off);
                pon16(e, 4, s.tipo);
                e[6] = (byte) ((s.enData ? R_SITIO_EN_DATA : 0)
                             | (s.aRam   ? R_DESTINO_RAM   : 0));
                e[7] = 0;
                NpackReloc.pon32(e,  8, s.valorSim);
                NpackReloc.pon32(e, 12, s.addend);
                out.write(e, 0, e.length);
            }
        }
        return out.toByteArray();
    }

    /** Lo que se recupera de un `.npk` leído. */
    public static final class Leido {
        public final NpackReloc.Imagen imagen;
        public final Meta meta;
        public final boolean relocalizado;   /* reloc_count == 0 */
        Leido(NpackReloc.Imagen i, Meta m, boolean r) {
            this.imagen = i; this.meta = m; this.relocalizado = r;
        }
    }

    /**
     * Lee un `.npk`. `destino` dice cómo interpretarlo (REL/RELA, secciones);
     * se comprueba que la arquitectura del fichero es la suya.
     *
     * <p>Las bases que devuelve son las del SELLO. En un fichero sin
     * relocalizar el sello está a cero, así que el llamante tiene que decir a
     * qué bases se enlazó — por eso van como parámetro y no se adivinan.
     */
    public static Leido leer(byte[] b, NpackReloc.Destino destino,
                             int baseFlashEnlace, int baseRamEnlace)
            throws NpackReloc.RelocException {
        if (b.length < HDR_BYTES)
            throw new NpackReloc.RelocException("no llega ni a la cabecera ("
                    + b.length + " B de " + HDR_BYTES + ")");
        if (NpackReloc.leer32(b, 0) != MAGIC)
            throw new NpackReloc.RelocException("no es un pack nativo (magic)");
        int formato = leer16(b, 4);
        if (formato != FORMAT)
            throw new NpackReloc.RelocException("formato " + formato
                    + ", esperaba " + FORMAT);

        Meta m = new Meta();
        m.arch       = leer16(b, 6);
        m.floatAbi   = leerCadena(b, 8, 8);
        m.entryOff   = NpackReloc.leer32(b, 16);
        m.entryThumb = (NpackReloc.leer32(b, 20) & F_THUMB) != 0;
        if (m.arch != destino.machine)
            throw new NpackReloc.RelocException("el pack es de la arquitectura "
                    + m.arch + " y el destino '" + destino.nombre + "' espera "
                    + destino.machine);

        int flashBytes = NpackReloc.leer32(b, 24);
        int dataBytes  = NpackReloc.leer32(b, 28);
        int bssBytes   = NpackReloc.leer32(b, 32);
        int selloFlash = NpackReloc.leer32(b, 36);
        int selloRam   = NpackReloc.leer32(b, 40);
        int nRelocs    = NpackReloc.leer32(b, 44);

        long fin = (long) HDR_BYTES + flashBytes + dataBytes
                 + (long) nRelocs * RELOC_ENTRY;
        if (fin > b.length)
            throw new NpackReloc.RelocException("el fichero dice medir " + fin
                    + " B y sólo tiene " + b.length + " — truncado o corrupto");

        byte[] flash = new byte[flashBytes];
        System.arraycopy(b, HDR_BYTES, flash, 0, flashBytes);
        byte[] data = new byte[dataBytes];
        System.arraycopy(b, HDR_BYTES + flashBytes, data, 0, dataBytes);

        List<NpackReloc.Sitio> sitios = new ArrayList<>();
        int base = HDR_BYTES + flashBytes + dataBytes;
        for (int i = 0; i < nRelocs; i++) {
            int e = base + i * RELOC_ENTRY;
            int off   = NpackReloc.leer32(b, e);
            int tipo  = leer16(b, e + 4);
            int fl    = b[e + 6] & 0xFF;
            int valor = NpackReloc.leer32(b, e + 8);
            int add   = NpackReloc.leer32(b, e + 12);
            sitios.add(NpackReloc.sitio(
                    (fl & R_SITIO_EN_DATA) != 0, off, tipo, valor, add,
                    (fl & R_DESTINO_RAM) != 0));
        }

        /* Si trae sello, esas son sus bases; si no, las que diga el llamante. */
        boolean yaEsta = (nRelocs == 0);
        int bf = yaEsta ? selloFlash : baseFlashEnlace;
        int br = yaEsta ? selloRam   : baseRamEnlace;
        return new Leido(NpackReloc.imagen(destino, flash, data, bssBytes, bf, br, sitios),
                         m, yaEsta);
    }

    // ─────────────────────────────────────────────────────────── utilidades ──

    private static void pon16(byte[] b, int off, int v) {
        b[off] = (byte) v; b[off + 1] = (byte) (v >>> 8);
    }
    private static int leer16(byte[] b, int off) {
        return (b[off] & 0xFF) | ((b[off + 1] & 0xFF) << 8);
    }
    /** NUL-terminada y NUL-rellenada, como el `char[8]` del lado C. */
    private static void ponCadena(byte[] b, int off, String s, int len) {
        byte[] t = s.getBytes(java.nio.charset.StandardCharsets.US_ASCII);
        int n = Math.min(t.length, len - 1);       /* siempre queda el NUL */
        System.arraycopy(t, 0, b, off, n);
    }
    private static String leerCadena(byte[] b, int off, int len) {
        int n = 0;
        while (n < len && b[off + n] != 0) n++;
        return new String(b, off, n, java.nio.charset.StandardCharsets.US_ASCII);
    }
}
