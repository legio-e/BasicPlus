package basicplus.frontend;

import basicplus.pack.PackEntry;
import basicplus.pack.PackException;
import basicplus.pack.PackReader;
import basicplus.pack.PackWriter;

import java.util.ArrayList;
import java.util.List;

/**
 * LO QUE SE GRABA — transforma el pack universal en el de UNA placa (V5/H8, D6).
 *
 * <pre>
 *   sqlite.pack  (lo que se reparte)      →  para el P4
 *     npk  sqlite.RISCV                        npk  sqlite   ← relocalizado y sellado
 *     npk  sqlite.ARMV8                        mod  SQLite
 *     mod  SQLite          (portable)          mdn  SQLite
 *     mdn  SQLite.RISCV                     (los de ARMV8 NO viajan)
 *     mdn  SQLite.ARMV8
 * </pre>
 *
 * <h3>DOS PASOS, y el orden no es opcional</h3>
 * Grabar impone una secuencia que obliga a partir esto en dos:
 *
 * <pre>
 *   1. podar(pack, destino)      → el pack de ESTA familia. Ya tiene su tamaño
 *                                  FINAL, que es lo que hay que declarar.
 *   2. PACK_BURN_BEGIN(tamaño)   → el dispositivo elige el hueco y DICE dónde
 *                                  cae y qué RAM le da.
 *   3. sellar(prep, flash, ram)  → parchea en sitio y pone el sello.
 * </pre>
 *
 * <p>No se puede hacer del tirón porque <b>la dirección no se sabe hasta el
 * paso 2</b>, y el tamaño hay que darlo en el paso 2. La clave para que encaje:
 * quitar la tabla de relocalizaciones SÍ cambia el tamaño, pero relocalizar y
 * sellar NO — son bytes parcheados en sitio. Así que se poda primero (tabla
 * fuera, tamaño ya definitivo) y se sella después.
 *
 * <h3>Dos disparadores INDEPENDIENTES</h3>
 * Matiz de Eduardo, y no es un detalle: un pack puede necesitar uno, el otro,
 * los dos o ninguno.
 * <ul>
 *   <li><b>lleva `.npk`</b> → hay que relocalizarlo y sellarlo.</li>
 *   <li><b>lleva `.mdn`</b> → hay que rehacer el pack quitando los de las otras
 *       familias. Puede pasar SIN `.npk`: una librería con funciones `native`
 *       que se apoyan en el puente AOT de la VM no tiene motor que realojar,
 *       pero sí tiene un `.mdn` por familia.</li>
 *   <li><b>ninguno</b> → el pack viaja TAL CUAL. Universal literal, y es el
 *       caso común: la mayoría de librerías no tienen una sola `native`.</li>
 * </ul>
 *
 * <h3>Por qué poda el PC y no el micro</h3>
 * El micro también podría buscar `<Modulo>.<DESTINO>` y caer a `<Modulo>`.
 * Pero entonces cada placa cargaría en su flash con los `.mdn` de las demás, y
 * la tabla de destinos entraría en el firmware. <b>En la placa, lo que no está
 * no puede fallar.</b> Y el coste aquí es tiempo de PC en una operación que se
 * hace de vez en cuando.
 *
 * <h3>Cero cambios en el micro</h3>
 * Tras la poda, el dispositivo encuentra `npk sqlite` y `mdn SQLite` — los
 * mismos nombres de siempre. Su búsqueda (`bpvm_pack_find(zona, len, "mdn",
 * modulo, …)`) no se entera de que existieron hermanas.
 */
public final class PackBurn {

    private PackBurn() { }

    /** No se puede preparar el pack para esa placa, y se dice por qué. */
    public static final class BurnException extends Exception {
        public BurnException(String m) { super(m); }
    }

    /**
     * El pack ya podado para una familia, a la espera de dirección.
     *
     * <p>Si {@link #necesitaDireccion} es false ya está listo: no lleva motor
     * nativo y `bytes` es lo que se graba tal cual.
     */
    /* El bloque de borrado MAYOR de las familias que soportamos, y por eso el
     * que se usa al rearmar: RP2350 y ESP32 borran en 4 KB, pero el STM32 U5 lo
     * hace en paginas de 8 KB. `PackWriter` alinea el tamano total al bloque que
     * se le diga, y su valor por defecto es 4096 —"(Pico/ESP)", lo dice su propio
     * comentario—, asi que un pack rearmado caia la mitad de las veces en un
     * multiplo de 4 K que NO lo es de 8 K y el STM32 lo rechazaba con BAD_ALIGN.
     * Alinear siempre al mayor cuesta como mucho 4 KB de flash por pack y vale
     * para las tres familias sin preguntarle nada al dispositivo. */
    private static final int BLOQUE_GRABADO = 8192;

    public static final class Preparado {
        /** El pack de esta familia. Tamaño DEFINITIVO (la tabla ya no está). */
        public final byte[] bytes;
        /** ¿Lleva `.npk`? Entonces hay que sellarlo antes de grabar. */
        public final boolean necesitaDireccion;
        public final int relocalizaciones;   /* cuántas hay que aplicar */
        public final int podados;            /* entradas de otras familias fuera */
        public final List<String> detalle = new ArrayList<>();

        final int npkOff;                    /* dónde vive el .npk dentro de `bytes` */
        final NpackReloc.Imagen img;
        final NpackFile.Meta meta;
        final NpackReloc.Destino destino;
        /* Las entradas y la cabecera, para REARMAR al sellar. El pack lleva un
         * CRC de contenido: parchear los bytes en sitio lo dejaría mal, y
         * recalcularlo aquí sería tener la aritmética del formato en dos
         * sitios. Se rearma con la librería, que es quien lo sabe.
         * (Lo cazó la prueba contra `ld`: "crc_contenido no cuadra".) */
        final List<PackEntry> entradas;
        final int idxNpk;
        final String nombrePack, versionPack;
        final long fechaPack;

        Preparado(byte[] b, int off, NpackReloc.Imagen i, NpackFile.Meta m,
                  NpackReloc.Destino d, int podados,
                  List<PackEntry> entradas, int idxNpk,
                  String nombrePack, String versionPack, long fechaPack) {
            this.bytes = b; this.npkOff = off; this.img = i; this.meta = m;
            this.destino = d; this.podados = podados;
            this.entradas = entradas; this.idxNpk = idxNpk;
            this.nombrePack = nombrePack; this.versionPack = versionPack;
            this.fechaPack = fechaPack;
            this.necesitaDireccion = (off >= 0);
            this.relocalizaciones  = (i != null) ? i.sitios.size() : 0;
        }
    }

    /**
     * PASO 1 — deja el pack con lo de ESTA familia y nada más.
     *
     * <p>El `.npk` sale ya sin su tabla (por eso el tamaño es el definitivo)
     * pero SIN relocalizar y SIN sello: todavía no tiene dirección.
     */
    public static Preparado podar(byte[] packBytes, NpackReloc.Destino destino)
            throws BurnException {
        PackReader.Pack p;
        try {
            p = PackReader.read(packBytes);
        } catch (PackException ex) {
            throw new BurnException("no se puede leer el pack: " + ex.getMessage());
        }

        String sufMio = "." + destino.sufijo;
        List<PackEntry> salida = new ArrayList<>();
        List<String> ajenas = new ArrayList<>();
        int podadas = 0;
        String nombreNpk = null;
        NpackReloc.Imagen img = null;
        NpackFile.Meta meta = null;

        for (PackEntry e : p.entries) {
            int punto = e.nombre.lastIndexOf('.');
            String suf = (punto > 0) ? e.nombre.substring(punto) : null;

            /* Sin sufijo → portable (.mod, resources, el manifest). Viaja. */
            if (suf == null || !esSufijoDeDestino(suf)) { salida.add(e); continue; }

            if (!suf.equals(sufMio)) {                 /* de otra familia: fuera */
                podadas++;
                if (!ajenas.contains(suf.substring(1))) ajenas.add(suf.substring(1));
                continue;
            }

            String pelado = e.nombre.substring(0, punto);
            if ("npk".equals(e.tipo)) {
                if (img != null)
                    throw new BurnException("el pack trae DOS motores para '"
                        + destino.nombre + "' ('" + nombreNpk + "' y '" + pelado
                        + "'). Cuál arrancaría no está definido: sobra uno.");
                try {
                    /* Las bases de ENLACE del .npk sin relocalizar: el fichero las
                     * trae a cero (no tiene sello todavía), así que se le dicen.
                     * Son las del enlace de referencia con que se construyó. */
                    NpackFile.Leido l = NpackFile.leer(e.data, destino, 0, 0x20000000);
                    if (l.relocalizado)
                        throw new BurnException("'" + e.nombre + "' ya viene"
                            + " relocalizado (sin tabla). Un pack distribuido debe"
                            + " traerla: se realoja AL GRABAR, no antes.");
                    img  = l.imagen;
                    meta = l.meta;
                    nombreNpk = pelado;
                    /* Sin tabla YA: el tamaño que declaremos al dispositivo tiene
                     * que ser el definitivo. Sellar luego no lo cambia. */
                    salida.add(new PackEntry("npk", pelado,
                            NpackFile.escribirSinSellar(l.imagen, l.meta)));
                } catch (NpackReloc.RelocException ex) {
                    throw new BurnException("'" + e.nombre + "': " + ex.getMessage());
                }
            } else {
                /* El `.mdn` no se toca: es position-independent. Sólo pierde el
                 * sufijo, que era para que el IDE supiera cuál era el suyo. */
                salida.add(new PackEntry(e.tipo, pelado, e.data));
            }
        }

        /* ¿Había familias y ninguna era la nuestra? Eso NO puede pasar callando:
         * se grabaría un pack sin motor y el fallo saldría mucho después. */
        if (img == null && !ajenas.isEmpty() && !hayAlgoMio(p, sufMio))
            throw new BurnException("este pack no trae nada para '" + destino.nombre
                    + "' (sufijo " + destino.sufijo + "). Lleva: " + ajenas
                    + ". Hay que construirlo para este destino.");

        /* REARMAR SOLO SI HACE FALTA. Las dos condiciones son de Eduardo (22-ago):
         *   1. que haya codigo nativo PARA NOSOTROS  -> hay que sellarlo
         *   2. que haya entradas de otros micros      -> hay que podarlas
         * Si no se cumple ninguna, el fichero YA es lo que hay que grabar, y
         * reserializarlo es trabajo inutil que ademas le cambia el tamano. Un
         * `.pack` construido por el proyecto viene alineado; el rearmado no tenia
         * por que caer en el mismo sitio. Mandar el original tambien da una
         * propiedad que se agradece: lo que se graba es, byte a byte, lo que se
         * construyo y se verifico en el PC. */
        byte[] nuevo;
        if (podadas == 0 && img == null) {
            nuevo = packBytes;
        } else {
            try {
                nuevo = PackWriter.build(p.nombre, p.versionContenido, p.fechaUnix,
                                         salida, BLOQUE_GRABADO);
            } catch (PackException ex) {
                throw new BurnException("no se puede rearmar el pack: " + ex.getMessage());
            }
        }

        /* Dónde ha quedado el motor DENTRO del pack ya montado. Se relee en vez
         * de calcularlo: el que sabe dónde pone cada entrada es el formato, y
         * repetir aquí su aritmética sería tenerla en dos sitios. */
        int npkOff = -1;
        if (img != null) {
            try {
                for (PackEntry e : PackReader.read(nuevo).entries)
                    if ("npk".equals(e.tipo) && e.nombre.equals(nombreNpk)) npkOff = e.dataOff;
            } catch (PackException ex) {
                throw new BurnException("el pack recién montado no se relee: " + ex.getMessage());
            }
            if (npkOff < 0)
                throw new BurnException("no encuentro el motor '" + nombreNpk
                        + "' en el pack recién montado");
        }

        int idxNpk = -1;
        for (int i = 0; i < salida.size(); i++)
            if ("npk".equals(salida.get(i).tipo) && salida.get(i).nombre.equals(nombreNpk)) idxNpk = i;

        Preparado prep = new Preparado(nuevo, npkOff, img, meta, destino, podadas,
                salida, idxNpk, p.nombre, p.versionContenido, p.fechaUnix);
        prep.detalle.add("destino " + destino.nombre + " (" + destino.sufijo + ")");
        prep.detalle.add(podadas + " entrada(s) de otras familias podada(s)"
                + (ajenas.isEmpty() ? "" : " " + ajenas));
        prep.detalle.add(salida.size() + " entradas, " + nuevo.length + " B");
        if (img != null)
            prep.detalle.add("motor '" + nombreNpk + "' en el pack+0x"
                    + Integer.toHexString(npkOff) + " — "
                    + img.sitios.size() + " relocalizacion(es) pendientes de direccion");
        else
            prep.detalle.add("sin motor nativo: se graba tal cual");
        return prep;
    }

    /**
     * PASO 2 — con la dirección que ha dicho el dispositivo: relocaliza el motor
     * y lo sella.
     *
     * @param baseFlash dirección que verá la CPU para el CÓDIGO del motor. NO es
     *   la del pack ni la de la entrada: el código empieza tras la cabecera del
     *   `.npk`. Confundirlos son 64 bytes de desfase y un salto a mitad de
     *   instrucción, así que lo calcula {@link #baseDelCodigo}.
     * @param baseRam base del bloque de RAM que la placa le da.
     */
    public static byte[] sellar(Preparado prep, int baseFlash, int baseRam)
            throws BurnException {
        if (!prep.necesitaDireccion) return prep.bytes;
        if (baseRam == 0)
            throw new BurnException("esta placa no da RAM a packs nativos"
                    + " (ramBase = 0), asi que el motor no podria arrancar."
                    + " No se graba a medias.");
        try {
            NpackReloc.Imagen puesta = NpackReloc.relocalizar(prep.img, baseFlash, baseRam);
            byte[] npk = NpackFile.escribirGrabable(puesta, prep.meta);

            /* Debe medir EXACTAMENTE lo mismo que el sin-sellar: de eso depende
             * que el tamaño declarado al dispositivo siga valiendo. Si un día
             * dejaran de medir igual, esto lo dice en vez de mandar un pack de
             * otro tamaño del que se anunció. */
            int antes = prep.entradas.get(prep.idxNpk).data.length;
            if (npk.length != antes)
                throw new BurnException("el motor sellado mide " + npk.length
                        + " B y el podado medía " + antes + " — el tamaño ya se"
                        + " declaró al empezar a grabar y no puede cambiar");

            List<PackEntry> finales = new ArrayList<>(prep.entradas);
            finales.set(prep.idxNpk, new PackEntry("npk",
                    prep.entradas.get(prep.idxNpk).nombre, npk));
            /* MISMO bloque que en `podar`: este rearmado tiene que medir
             * exactamente igual que el podado, porque el tamano ya se declaro
             * al dispositivo. Con el bloque por defecto (4 KB) podria no
             * cuadrar, y la comprobacion de abajo lo cazaria demasiado tarde. */
            byte[] out = PackWriter.build(prep.nombrePack, prep.versionPack,
                                          prep.fechaPack, finales, BLOQUE_GRABADO);
            if (out.length != prep.bytes.length)
                throw new BurnException("el pack sellado mide " + out.length
                        + " B y el podado medía " + prep.bytes.length);
            return out;
        } catch (NpackReloc.RelocException ex) {
            throw new BurnException("relocalizando el motor: " + ex.getMessage());
        } catch (PackException ex) {
            throw new BurnException("rearmando el pack sellado: " + ex.getMessage());
        }
    }

    /**
     * La dirección del CÓDIGO, a partir de dónde cae el pack.
     *
     * <p>Está aquí y no en el llamante porque los 64 bytes de la cabecera son
     * justo el error que `bpvm_npack.h` avisa de no cometer: «confundirlos son
     * 64 bytes de desfase y un salto a mitad de instruccion».
     */
    public static int baseDelCodigo(Preparado prep, int direccionDelPack) {
        return direccionDelPack + prep.npkOff + NpackFile.HDR_BYTES;
    }

    /** ¿Hay ALGUNA entrada con nuestro sufijo? (para el aviso de arriba). */
    private static boolean hayAlgoMio(PackReader.Pack p, String sufMio) {
        for (PackEntry e : p.entries) if (e.nombre.endsWith(sufMio)) return true;
        return false;
    }

    /**
     * Los sufijos que reconocemos como «de destino» salen de la TABLA, no de
     * una heurística: `Hola.v2` no es un destino, es un nombre con un punto.
     */
    private static boolean esSufijoDeDestino(String sufijoConPunto) {
        return NpackReloc.porSufijo(sufijoConPunto.substring(1)) != null;
    }
}
