package basicplus.frontend;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * De un `.elf` a un `.npk` DISTRIBUIBLE (V5/H8).
 *
 * <p>Es la pieza que faltaba: el oráculo ya sabía construir la imagen y la
 * tabla, pero sólo en memoria. Esto la escribe en disco, que es lo que entra
 * en el pack.
 *
 * <h3>Qué produce, y qué NO</h3>
 * Un `.npk` <b>sin relocalizar</b>: con su tabla y el sello a cero. Es lo que
 * se reparte. Ponerle dirección es cosa del IDE al grabar, porque hasta
 * entonces nadie sabe en qué placa va a caer ni dónde queda su zona de packs.
 *
 * <p>Lo contrario —relocalizar aquí, a una dirección supuesta— es lo que hacía
 * el `pack.py` de las pruebas, y por eso cada placa necesitaba su pack.
 *
 * <h3>El ELF de entrada</h3>
 * Enlazado en la base baja (`-Ttext=0`) y <b>con `--emit-relocs`</b>. Sin eso
 * el enlazador se come la tabla y aquí saldría un `.npk` de cero
 * relocalizaciones — que no es «uno que no necesita ninguna», es <b>una
 * mentira</b>: `reloc_count == 0` significa «ya realojado» (es la prueba de que
 * pasó por el IDE). El dispositivo lo daría por bueno, miraría el sello, lo
 * vería a cero y diría «realojado para OTRA direccion». Un diagnóstico
 * desconcertante a 400 km del error real.
 *
 * <p>Por eso este programa se planta antes de escribir nada.
 *
 * <h3>Uso</h3>
 * <pre>
 *   java basicplus.frontend.NpackBuild &lt;entrada.elf&gt; &lt;destino&gt; &lt;salida.npk&gt;
 *                                      [--entry &lt;simbolo&gt;] [--umbral-ram 0x…]
 * </pre>
 */
public final class NpackBuild {

    private NpackBuild() { }

    /** El símbolo por el que la VM entra al motor. Contrato con `bpvm_npack.c`. */
    private static final String ENTRADA_POR_DEFECTO = "bp_pack_init";

    public static void main(String[] args) throws Exception {
        if (args.length < 3) {
            System.err.println("Uso: NpackBuild <entrada.elf>"
                    + " <arm-cortex-m33|riscv32-esp-p4> <salida.npk>"
                    + " [--entry <simbolo>] [--umbral-ram 0x20000000]");
            System.exit(2);
        }
        Path elfPath = Paths.get(args[0]);
        NpackReloc.Destino d = NpackReloc.porNombre(args[1]);
        if (d == null) {
            System.err.println("destino desconocido: " + args[1]
                    + " (conocidos: " + NpackReloc.targetsConocidos() + ")");
            System.exit(2);
        }
        Path salida  = Paths.get(args[2]);
        String simEntrada = ENTRADA_POR_DEFECTO;
        int umbral = 0x20000000;
        for (int i = 3; i < args.length; i++) {
            if ("--entry".equals(args[i]) && i + 1 < args.length) simEntrada = args[++i];
            else if ("--umbral-ram".equals(args[i]) && i + 1 < args.length)
                umbral = (int) Long.parseLong(args[++i].replace("0x", ""), 16);
        }

        System.out.println(Version.linea());
        System.out.println("npk: " + elfPath.getFileName() + " -> "
                + salida.getFileName() + " | destino " + d.nombre);

        Elf32 e = Elf32.parse(Files.readAllBytes(elfPath));
        if (e.machine() != d.machine) {
            System.err.println("el .elf es de la maquina " + e.machine()
                    + " y el destino '" + d.nombre + "' espera " + d.machine);
            System.exit(1);
        }

        NpackReloc.Imagen img = NpackReloc.desdeElf(e, d, umbral);
        System.out.printf("  imagen  : flash %d B (%s) | datos %d B (%s) | bss %d B%n",
                img.flash.length, NpackReloc.detalle(e, d.flashSecs),
                img.data.length,  NpackReloc.detalle(e, d.dataSecs), img.bssBytes);
        System.out.printf("  enlazado: flash 0x%08X  ram 0x%08X%n",
                img.baseFlash, img.baseRam);

        /* ── EL GUARDIÁN ────────────────────────────────────────────────────
         * Ver arriba: cero relocalizaciones NO es un caso válido aquí, es un
         * enlace sin `--emit-relocs`. Se para ANTES de escribir el fichero,
         * que es donde el error todavía se entiende. */
        if (img.sitios.isEmpty()) {
            System.err.println();
            System.err.println("  ERROR: este .elf no trae NI UNA relocalizacion.");
            System.err.println("  Casi seguro que se enlazo sin `--emit-relocs`, y el");
            System.err.println("  enlazador se las comio. Un .npk con reloc_count = 0");
            System.err.println("  significa YA REALOJADO — o sea que escribirlo aqui");
            System.err.println("  seria firmar una direccion que nadie ha dado, y la");
            System.err.println("  placa lo rechazaria diciendo otra cosa.");
            System.err.println();
            System.err.println("  Anade  -Wl,--emit-relocs  al enlace y repite.");
            System.exit(1);
        }

        /* La entrada. Su valor en un ELF ya enlazado es la DIRECCIÓN; lo que
         * guarda el `.npk` es el OFFSET dentro de la imagen de flash, porque
         * la dirección va a cambiar al grabar. */
        Elf32.Symbol ent = null;
        for (Elf32.Symbol s : e.symbols()) {
            if (simEntrada.equals(s.name)) { ent = s; break; }
        }
        if (ent == null) {
            System.err.println("  ERROR: no encuentro el simbolo de entrada '"
                    + simEntrada + "' en el .elf.");
            System.err.println("  Es por donde la VM entra al motor. Si se llama de otra"
                    + " forma, dilo con --entry.");
            System.exit(1);
        }

        NpackFile.Meta meta = new NpackFile.Meta();
        meta.arch     = e.machine();
        meta.floatAbi = d.floatAbi;
        /* El bit 0 de una direccion de funcion ARM dice «esto es Thumb»; no es
         * parte de la direccion. Se guarda aparte para que el salto lo vuelva a
         * poner. En RISC-V no existe: ahi el bit 0 es direccion de verdad. */
        boolean thumb = (d.machine == NpackReloc.Elf32Machine.ARM) && ((ent.value & 1) != 0);
        meta.entryThumb = thumb;
        meta.entryOff   = (thumb ? (ent.value & ~1) : ent.value) - img.baseFlash;

        if (meta.entryOff < 0 || meta.entryOff >= img.flash.length) {
            System.err.printf("  ERROR: la entrada '%s' cae en 0x%08X, FUERA de la"
                    + " imagen de flash (0x%08X..0x%08X).%n", simEntrada, ent.value,
                    img.baseFlash, img.baseFlash + img.flash.length);
            System.exit(1);
        }

        byte[] npk = NpackFile.escribirSinRelocalizar(img, meta);
        Files.write(salida, npk);

        System.out.printf("  entrada : '%s' en flash+0x%X%s%n", simEntrada, meta.entryOff,
                (d.machine == NpackReloc.Elf32Machine.ARM)
                        ? (thumb ? " (Thumb)" : " (ARM, no Thumb)") : "");
        System.out.printf("  tabla   : %d relocalizacion(es) x %d B%n",
                img.sitios.size(), NpackFile.RELOC_ENTRY);
        System.out.printf("  escrito : %d B  (sello a CERO: lo pone el IDE al grabar)%n",
                npk.length);

        /* IDA Y VUELTA antes de darlo por bueno. Cuesta milisegundos y lo que
         * comprueba no es que «parezca bien», sino que lo que sale del fichero
         * es lo mismo que entro. Un fichero que no se puede releer no sirve. */
        NpackFile.Leido rl = NpackFile.leer(npk, d, img.baseFlash, img.baseRam);
        if (rl.relocalizado || rl.imagen.sitios.size() != img.sitios.size()
                || rl.imagen.flash.length != img.flash.length
                || rl.imagen.data.length  != img.data.length) {
            System.err.println("  ERROR: el fichero recien escrito no se relee igual.");
            System.exit(1);
        }
        System.out.println("  releido : correcto");
    }
}
