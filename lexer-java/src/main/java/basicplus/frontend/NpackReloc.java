package basicplus.frontend;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

/**
 * RELOCALIZADOR de packs nativos (V5/H8) — mueve una imagen ya enlazada a otra
 * dirección, parcheando lo que haga falta.
 *
 * <h3>Qué problema resuelve</h3>
 * Un `.npk` se ejecuta EN SITIO desde la flash, así que su código lleva dentro
 * direcciones absolutas. Hasta ahora esas direcciones se fijaban a mano y por
 * adelantado con un script, lo que ataba el pack a una posición concreta: si
 * cambiaba —por meterlo en un pack con más entradas— el sello dejaba de cuadrar.
 * Con esto, quien graba calcula la dirección real y reloca ahí.
 *
 * <h3>NO se re-enlaza: se RE-BASA</h3>
 * Esto es lo que hace el problema tratable. La imagen ya viene enlazada en una
 * dirección conocida; relocalizar es aplicar un DELTA:
 *
 * <pre>
 *   delta = base_nueva - base_enlazada
 * </pre>
 *
 * No hay que resolver símbolos, ni ordenar secciones, ni nada de lo que hace un
 * enlazador. Sólo sumar un desplazamiento en los sitios que la tabla señala.
 *
 * <h3>Y aun así las dos familias difieren en el MOTOR, no sólo al escribir</h3>
 * <ul>
 *   <li><b>REL</b> (ARM): el addend está DENTRO de la palabra. Se lee lo que
 *       hay, se le suma el delta, y se vuelve a escribir.</li>
 *   <li><b>RELA</b> (RISC-V): la dirección final se calcula ENTERA
 *       ({@code valor + addend + delta}) y se escribe fresca. No se puede leer
 *       y modificar porque en {@code HI20}/{@code LO12} habría que recomponer
 *       20+12 bits repartidos en dos instrucciones para volver a partirlos.</li>
 * </ul>
 *
 * <h3>Lo que no está en la tabla, GRITA</h3>
 * Cada destino declara qué tipos parchea y cuáles ignora POR SER
 * PC-RELATIVOS —el código se mueve en bloque, así que la distancia no cambia—.
 * Un tipo que no esté en ninguna de las dos listas es un error, no un caso
 * ignorado: «correcto y mudo» no distingue «no hacía falta» de «se me olvidó».
 *
 * <p>Y aquí este puerto mejora al prototipo Python: aquél filtraba por el NOMBRE
 * que imprime {@code readelf}, que trunca a 16 caracteres — tuvo que aceptar
 * {@code R_RISCV_RVC_BRANC} además de {@code R_RISCV_RVC_BRANCH} para que no se
 * le colara uno. Leyendo el ELF se usa el número y ese fallo no puede existir.
 *
 * <h3>Quién lo juzga</h3>
 * El oráculo es <b>el propio enlazador</b>: se enlaza lo mismo en la dirección
 * destino con {@code ld -Ttext=…} y se comparan los bytes. No hay expectativas
 * escritas a mano que puedan estar mal.
 */
public final class NpackReloc {

    private NpackReloc() { }

    /* ── Tipos de relocalización, MEDIDOS sobre nuestros propios binarios ──
     *
     * No copiados de una tabla: sacados con `readelf -r` de los .o/.elf que
     * producen nuestros dos toolchains (el byte bajo de r_info). Lo que no
     * aparece aquí es porque NO lo emiten hoy — y si algún día lo emiten, el
     * relocalizador lo dirá en vez de tragárselo. */
    private static final int R_ARM_ABS32       = 2;
    private static final int R_ARM_THM_CALL    = 10;   /* bl      — PC-relativa */
    private static final int R_ARM_THM_JUMP24  = 30;   /* b.w     — PC-relativa */

    private static final int R_RISCV_32        = 1;
    private static final int R_RISCV_HI20      = 26;
    private static final int R_RISCV_LO12_I    = 27;
    private static final int R_RISCV_LO12_S    = 28;

    /* ── Los que NO se tocan, por TRES motivos distintos ───────────────────
     *
     * Meterlos todos en un saco llamado «PC-relativas» sería mentir en el
     * código: sólo el primer grupo lo es. Y el que venga detrás se fiaría.
     *
     * El `mini` sólo ejercitaba 5 tipos; el SQLite entero tiene 21. Por eso el
     * oráculo se pasa por lo GRANDE antes de tocar la placa.
     *
     * ── CUÁLES ESTÁN VERIFICADOS Y CUÁLES NO ──
     * El censo del SQLite RISC-V, restringido a las secciones QUE VIAJAN
     * (`.text .rodata .eh_frame .data .sdata` — el resto no se graba):
     * <pre>
     *   a parchear   1056 R_RISCV_32   +  1952 HI20 + 2055 LO12_I + 24 LO12_S
     *   ignorados   11150 CALL_PLT  553 BRANCH  283 RVC_JUMP  123 RVC_BRANCH
     *                  46 RELAX  |  4 32_PCREL  |  4 ADD32 + 4 SUB32
     * </pre>
     * Suma exacta: 5087 a parchear, verificados byte a byte contra `ld`.
     *
     * <p><b>Los pares de 16/8/6 bits y los ULEB no aparecen aquí.</b> El censo
     * sobre el fichero ENTERO da ~19.000, pero todos viven en `.debug_*`, que no
     * viaja. O sea que su entrada en esta lista está razonada, no ejercitada: si
     * un día viajaran, sería la primera vez. Se quedan porque la razón es sólida
     * (una resta de dos etiquetas que se mueven juntas no cambia) y porque
     * parchearlas sí sería un fallo — pero conviene saber cuál es cuál. */

    /* (a) PC-RELATIVAS: el código se mueve en bloque, la distancia no cambia. */
    private static final int R_RISCV_BRANCH     = 16;
    private static final int R_RISCV_CALL_PLT   = 19;   /* auipc+jalr */
    private static final int R_RISCV_RVC_BRANCH = 44;
    private static final int R_RISCV_RVC_JUMP   = 45;
    private static final int R_RISCV_32_PCREL   = 57;

    /* (b) DIFERENCIAS DE ETIQUETA: `.L2 - .L1`, que usan las tablas de CFI y
     *     de desenrollado. Las dos etiquetas se desplazan lo mismo, así que la
     *     resta sale idéntica — parchearlas sería CORROMPERLAS.
     *     Vienen en PAREJAS y eso se ve en el censo: ADD32 161 / SUB32 161,
     *     SET_ULEB128 6638 / SUB_ULEB128 6638. */
    private static final int R_RISCV_ADD16      = 34;
    private static final int R_RISCV_ADD32      = 35;
    private static final int R_RISCV_SUB8       = 37;
    private static final int R_RISCV_SUB16      = 38;
    private static final int R_RISCV_SUB32      = 39;
    private static final int R_RISCV_SUB6       = 52;
    private static final int R_RISCV_SET6       = 53;
    private static final int R_RISCV_SET8       = 54;
    private static final int R_RISCV_SET16      = 55;
    private static final int R_RISCV_SET_ULEB128 = 60;
    private static final int R_RISCV_SUB_ULEB128 = 61;

    /* (c) MARCAS: no son relocalizaciones. `RELAX` le dice al enlazador que
     *     PUEDE acortar el par de instrucciones anterior — y nosotros
     *     compilamos y enlazamos con `-mno-relax` / `--no-relax`
     *     precisamente para que no lo haga. No hay nada que escribir. */
    private static final int R_RISCV_RELAX      = 51;

    /** Cómo se escribe una dirección ya calculada, en la palabra de `off`. */
    private interface Parcheador {
        /** @return false si no sabe tratar ese tipo (→ el llamante GRITA). */
        boolean escribir(byte[] b, int off, int tipo, int valorFinal);
    }

    /**
     * UN DESTINO. Todo lo que el silicio hace distinto, en un sitio y por
     * escrito — si algo NO está aquí es que es común, y ésa es justamente la
     * afirmación que esta clase quiere poder sostener.
     *
     * <p>La llave del pack es el destino ENTERO, no la arquitectura: dentro de
     * ARM, un Cortex-M0+ (ARMv6-M) no ejecuta el código de un M33 (ARMv8-M) y
     * los dos son `EM_ARM`. Ver docs/V5_IDEAS.md.
     */
    public static final class Destino {
        public final String nombre;
        /* El SUFIJO de la doble extension (D1): `sqlite.npk.RISCV`. Vive AQUI y
         * no se teclea en el build, el IDE y los docs por separado — tres sitios
         * para el mismo dato es el error que cuesta caro (#299, #315). */
        public final String sufijo;
        /* La convencion de coma flotante, tal como la escribe la cabecera del
         * .npk y la compara el dispositivo. Un desajuste NO da error: da
         * NUMEROS MAL, asi que sale de aqui y no de quien empaqueta. */
        public final String floatAbi;
        public final int machine;          /* e_machine que debe traer el ELF */
        public final boolean rela;         /* true = addend explícito */
        public final List<String> flashSecs, dataSecs, bssSecs;
        final Set<Integer> ignorar;        /* PC-relativas: NO se tocan */
        final Parcheador patch;

        Destino(String nombre, String sufijo, String floatAbi,
                int machine, boolean rela,
                String[] flashSecs, String[] dataSecs, String[] bssSecs,
                Integer[] ignorar, Parcheador patch) {
            this.nombre = nombre; this.sufijo = sufijo; this.floatAbi = floatAbi;
            this.machine = machine; this.rela = rela;
            this.flashSecs = Arrays.asList(flashSecs);
            this.dataSecs  = Arrays.asList(dataSecs);
            this.bssSecs   = Arrays.asList(bssSecs);
            this.ignorar   = new LinkedHashSet<>(Arrays.asList(ignorar));
            this.patch     = patch;
        }
    }

    /** ARM Cortex-M33 (ARMv8-M Mainline, FPU simple): Metro RP2350 y STM32. */
    public static final Destino ARM_CORTEX_M33 = new Destino(
            "arm-cortex-m33", "ARMV8", "softfp", Elf32Machine.ARM, /*rela*/ false,
            new String[]{".text", ".rodata"},
            new String[]{".data"},
            new String[]{".bss"},
            new Integer[]{R_ARM_THM_CALL, R_ARM_THM_JUMP24},
            (b, off, tipo, val) -> {
                if (tipo == R_ARM_ABS32) { pon32(b, off, val); return true; }
                return false;
            });

    /** RISC-V RV32 del ESP32-P4 (ilp32f). */
    public static final Destino RISCV32_ESP_P4 = new Destino(
            "riscv32-esp-p4", "RISCV", "ilp32f", Elf32Machine.RISCV, /*rela*/ true,
            /* `.eh_frame` viaja aunque nadie desenrolle: son unos cientos de
             * bytes y asi la imagen de flash es LITERALMENTE lo que el
             * enlazador puso, sin huecos que justificar. */
            new String[]{".text", ".rodata", ".eh_frame"},
            new String[]{".data", ".sdata"},
            new String[]{".sbss", ".bss"},
            new Integer[]{
                /* (a) PC-relativas */
                R_RISCV_BRANCH, R_RISCV_CALL_PLT, R_RISCV_RVC_BRANCH,
                R_RISCV_RVC_JUMP, R_RISCV_32_PCREL,
                /* (b) diferencias de etiqueta — se cancelan */
                R_RISCV_ADD16, R_RISCV_ADD32, R_RISCV_SUB8, R_RISCV_SUB16,
                R_RISCV_SUB32, R_RISCV_SUB6, R_RISCV_SET6, R_RISCV_SET8,
                R_RISCV_SET16, R_RISCV_SET_ULEB128, R_RISCV_SUB_ULEB128,
                /* (c) marcas sin efecto */
                R_RISCV_RELAX},
            (b, off, tipo, val) -> {
                if (tipo == R_RISCV_32) { pon32(b, off, val); return true; }
                if (tipo == R_RISCV_HI20) {
                    /* El +0x800 es el ACARREO del signo: LO12 se interpreta con
                     * signo, asi que cuando el bit 11 esta a 1 resta 0x1000 y
                     * HI20 tiene que llevar uno de mas para compensar. Sin esto
                     * la direccion sale 4 KB por debajo — y no da error, da un
                     * puntero plausible a otro sitio. */
                    int i = leer32(b, off) & 0x00000FFF;
                    pon32(b, off, i | (((val + 0x800) >>> 12) << 12));
                    return true;
                }
                if (tipo == R_RISCV_LO12_I) {          /* tipo I: imm[11:0] en 31..20 */
                    int i = leer32(b, off) & 0x000FFFFF;
                    pon32(b, off, i | ((val & 0xFFF) << 20));
                    return true;
                }
                if (tipo == R_RISCV_LO12_S) {          /* tipo S: imm partido 11:5 / 4:0 */
                    int i = leer32(b, off) & 0x01FFF07F;
                    pon32(b, off, i | (((val >>> 5) & 0x7F) << 25) | ((val & 0x1F) << 7));
                    return true;
                }
                return false;
            });

    /** Los `e_machine` que nos importan. Mismo catálogo que `mdn_format.h`. */
    public static final class Elf32Machine {
        public static final int ARM   = 40;
        public static final int RISCV = 243;
        private Elf32Machine() { }
    }

    // ─────────────────────────────────────────────────────────── la imagen ──

    /** Un sitio que hay que parchear al mover la imagen. */
    public static final class Sitio {
        public final boolean enData;   /* false = el sitio esta en la imagen de flash */
        public final int off;          /* offset dentro de esa imagen */
        public final int tipo;
        public final int valorSim;     /* valor del simbolo, tal como quedo enlazado */
        public final int addend;       /* RELA: explicito. REL: 0 (esta en la palabra) */
        public final boolean aRam;     /* el DESTINO vive en RAM (.data) o en flash */

        Sitio(boolean enData, int off, int tipo, int valorSim, int addend, boolean aRam) {
            this.enData = enData; this.off = off; this.tipo = tipo;
            this.valorSim = valorSim; this.addend = addend; this.aRam = aRam;
        }
    }

    /** Imagen enlazada + su tabla, lista para re-basar. */
    public static final class Imagen {
        public final Destino destino;
        public final byte[] flash, data;
        public final int bssBytes;
        public final int baseFlash, baseRam;   /* donde esta enlazada AHORA */
        public final List<Sitio> sitios;

        Imagen(Destino d, byte[] flash, byte[] data, int bssBytes,
               int baseFlash, int baseRam, List<Sitio> sitios) {
            this.destino = d; this.flash = flash; this.data = data;
            this.bssBytes = bssBytes; this.baseFlash = baseFlash;
            this.baseRam = baseRam; this.sitios = sitios;
        }
    }

    /** Lo que no se sabe tratar: un tipo fuera de la tabla del destino. */
    public static final class RelocException extends Exception {
        public RelocException(String m) { super(m); }
    }

    // ─────────────────────────────────────────────── construir la imagen ──

    /**
     * Arma la {@link Imagen} a partir de un ELF YA ENLAZADO con
     * {@code --emit-relocs} (que es lo que conserva la tabla tras enlazar).
     *
     * @param umbralRam frontera flash/RAM del enlace de referencia. Todo símbolo
     *   por encima vive en RAM, por debajo en flash. Con la flash enlazada en 0
     *   no hay ambigüedad posible.
     *
     * <p>La clasificación —tanto del SITIO a parchear como de su DESTINO— se
     * hace por la DIRECCIÓN, nunca por el nombre de la sección. Así una sección
     * nueva (`.sdata` apareció con RISC-V) cae donde toca sola, sin que nadie
     * tenga que acordarse de añadirla a una lista.
     */
    public static Imagen desdeElf(Elf32 e, Destino d, int umbralRam)
            throws RelocException {
        if (e.machine() != d.machine) {
            throw new RelocException("el ELF es de la arquitectura " + e.machine()
                    + " y el destino '" + d.nombre + "' espera " + d.machine);
        }
        byte[] flash = concatenar(e, d.flashSecs, "flash");
        byte[] data  = concatenar(e, d.dataSecs,  "datos");
        int baseFlash = baseDe(e, d.flashSecs, 0);
        int bss = 0;
        for (String n : d.bssSecs) {
            Elf32.Section s = seccion(e, n);
            if (s != null) bss += s.size;
        }

        List<Sitio> sitios = new ArrayList<>();
        List<String> viajan = new ArrayList<>(d.flashSecs);
        viajan.addAll(d.dataSecs);
        for (String sec : viajan) {
            for (Elf32.Reloc r : e.relocs(sec)) {
                if (d.ignorar.contains(r.type)) continue;   /* PC-relativa: no se toca */
                Elf32.Symbol sim = e.symbol(r.symIdx);
                int valor = (sim != null) ? sim.value : 0;
                /* `r.offset` en un ELF enlazado es la DIRECCIÓN del sitio. */
                boolean sitioEnData = Integer.compareUnsigned(r.offset, umbralRam) >= 0;
                int off = sitioEnData ? r.offset - umbralRam : r.offset - baseFlash;
                boolean destinoRam = Integer.compareUnsigned(valor, umbralRam) >= 0;
                sitios.add(new Sitio(sitioEnData, off, r.type, valor, r.addend, destinoRam));
            }
        }
        return new Imagen(d, flash, data, bss, baseFlash, umbralRam, sitios);
    }

    private static Elf32.Section seccion(Elf32 e, String nombre) {
        for (Elf32.Section s : e.sections()) if (nombre.equals(s.name)) return s;
        return null;
    }

    private static int baseDe(Elf32 e, List<String> secs, int siNoHay) {
        int base = -1;
        for (String n : secs) {
            Elf32.Section s = seccion(e, n);
            if (s == null || s.size == 0) continue;
            if (base < 0 || Integer.compareUnsigned(s.addr, base) < 0) base = s.addr;
        }
        return (base < 0) ? siNoHay : base;
    }

    /**
     * Arma la imagen POR DIRECCIÓN: reserva desde la sección más baja hasta el
     * final de la más alta y coloca cada una en su sitio.
     *
     * <p>⚠️ NO es concatenar. El primer intento pegaba las secciones una detrás
     * de otra y el oráculo lo cazó al primer disparo: el enlazador ALINEA, y
     * entre `.text` (que acababa en 0x72) y `.rodata` (que empieza en 0x74)
     * había 2 bytes de relleno. Con la concatenación todo lo posterior salía
     * corrido dos bytes.
     *
     * <p>El relleno es parte de la imagen — es exactamente lo que hace
     * `objcopy -O binary`, que es como se extraía en el prototipo. Va a cero:
     * son huecos de alineación, nadie los lee.
     */
    private static byte[] concatenar(Elf32 e, List<String> secs, String cual)
            throws RelocException {
        List<Elf32.Section> hay = new ArrayList<>();
        for (String n : secs) {
            Elf32.Section s = seccion(e, n);
            if (s != null && s.size > 0) hay.add(s);
        }
        if (hay.isEmpty()) return new byte[0];
        hay.sort((a, b) -> Integer.compareUnsigned(a.addr, b.addr));

        int base = hay.get(0).addr;
        int fin  = base;
        for (Elf32.Section s : hay) {
            int f = s.addr + s.size;
            if (Integer.compareUnsigned(f, fin) > 0) fin = f;
        }
        long tam = Integer.toUnsignedLong(fin) - Integer.toUnsignedLong(base);
        if (tam < 0 || tam > (1 << 26)) {   /* 64 MB: un pack asi no es un pack */
            throw new RelocException("la imagen de " + cual + " saldria de "
                    + tam + " B — las secciones no forman un rango razonable");
        }
        byte[] img = new byte[(int) tam];
        for (Elf32.Section s : hay) {
            byte[] b = e.getSectionBytes(s.name);
            System.arraycopy(b, 0, img, s.addr - base, b.length);
        }
        return img;
    }

    /** Las secciones que forman una imagen, para diagnóstico. */
    public static String detalle(Elf32 e, List<String> secs) {
        StringBuilder sb = new StringBuilder();
        for (String n : secs) {
            Elf32.Section s = seccion(e, n);
            if (s == null || s.size == 0) continue;
            if (sb.length() > 0) sb.append(" + ");
            sb.append(n.substring(1)).append(' ').append(s.size);
        }
        return sb.length() == 0 ? "(vacia)" : sb.toString();
    }

    // ────────────────────────────────────────────────────────── el motor ──

    /**
     * Aplica los dos deltas (flash y RAM) sobre una COPIA de la imagen.
     *
     * @return la imagen re-basada. La original no se toca — así el llamante
     *         puede relocalizar la misma fuente a dos sitios distintos, que es
     *         justo lo que hace el pack de dos arquitecturas.
     */
    public static Imagen relocalizar(Imagen img, int baseFlash, int baseRam)
            throws RelocException {
        int dFlash = baseFlash - img.baseFlash;
        int dRam   = baseRam   - img.baseRam;

        byte[] flash = img.flash.clone();
        byte[] data  = img.data.clone();

        for (Sitio s : img.sitios) {
            byte[] b = s.enData ? data : flash;
            if (s.off < 0 || s.off + 4 > b.length) {
                throw new RelocException("relocalizacion fuera de la imagen: "
                        + (s.enData ? "data" : "flash") + "+0x"
                        + Integer.toHexString(s.off) + " (imagen de " + b.length + " B)");
            }
            int delta = s.aRam ? dRam : dFlash;
            int fin = img.destino.rela
                    ? s.valorSim + s.addend + delta   /* RELA: entera y fresca */
                    : leer32(b, s.off) + delta;       /* REL: el addend ya esta dentro */

            if (!img.destino.patch.escribir(b, s.off, s.tipo, fin)) {
                throw new RelocException("tipo de relocalizacion SIN TRATAR: "
                        + s.tipo + " en " + (s.enData ? "data" : "flash")
                        + "+0x" + Integer.toHexString(s.off)
                        + " (destino " + img.destino.nombre + "). Si es PC-relativa hay que"
                        + " anadirla a `ignorar`; si no, escribir su parcheador.");
            }
        }
        return new Imagen(img.destino, flash, data, img.bssBytes,
                          baseFlash, baseRam, img.sitios);
    }

    // ──────────────────────────────────────────────── leer/escribir 32 bits ──

    static int leer32(byte[] b, int off) {
        return  (b[off]     & 0xFF)
             | ((b[off + 1] & 0xFF) << 8)
             | ((b[off + 2] & 0xFF) << 16)
             | ((b[off + 3] & 0xFF) << 24);
    }

    static void pon32(byte[] b, int off, int v) {
        b[off]     = (byte)  v;
        b[off + 1] = (byte) (v >>> 8);
        b[off + 2] = (byte) (v >>> 16);
        b[off + 3] = (byte) (v >>> 24);
    }

    /** Los tipos que este destino IGNORA por PC-relativos (para diagnóstico). */
    public static List<Integer> pcRelativas(Destino d) {
        return new ArrayList<>(d.ignorar);
    }

    /* ── Fábricas para quien reconstruye una imagen desde un fichero ──
     *
     * Existen para que {@link NpackFile} pueda rehacer lo que leyó del disco.
     * Los constructores siguen siendo de paquete a propósito: una `Imagen` sólo
     * se arma leyendo un ELF (desdeElf) o leyendo un `.npk` — no a mano, que es
     * como se cuelan reparto y bases que no se corresponden. */

    /** Rehace un {@link Sitio} tal cual venía en la tabla del fichero. */
    public static Sitio sitio(boolean enData, int off, int tipo,
                              int valorSim, int addend, boolean aRam) {
        return new Sitio(enData, off, tipo, valorSim, addend, aRam);
    }

    /** Rehace una {@link Imagen} tal cual venía en el fichero. */
    public static Imagen imagen(Destino d, byte[] flash, byte[] data, int bssBytes,
                                int baseFlash, int baseRam, List<Sitio> sitios) {
        return new Imagen(d, flash, data, bssBytes, baseFlash, baseRam, sitios);
    }
}
