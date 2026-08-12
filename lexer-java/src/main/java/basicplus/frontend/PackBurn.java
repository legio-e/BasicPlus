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
 * <p>Además el IDE YA tiene que rehacer el pack por el `.npk`, así que la poda
 * del `.mdn` se apunta a un viaje que ya se hacía.
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

    /** Qué se hizo, para el log del IDE. */
    public static final class Resultado {
        public final byte[] pack;
        public final int relocalizados;   /* .npk realojados */
        public final int podados;         /* entradas de otras familias quitadas */
        public final List<String> detalle = new ArrayList<>();
        Resultado(byte[] p, int r, int d) { pack = p; relocalizados = r; podados = d; }
    }

    /**
     * Prepara el pack para `destino`.
     *
     * @param baseFlash dónde va a quedar el CÓDIGO del `.npk` en la zona de
     *   packs (no la base del pack: el código empieza tras la cabecera).
     * @param baseRam   base del bloque de RAM que la placa le va a dar.
     */
    public static Resultado paraDestino(byte[] packBytes, NpackReloc.Destino destino,
                                        int baseFlash, int baseRam)
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
        int relocs = 0, podadas = 0;
        Resultado tmp = null;

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
                salida.add(new PackEntry("npk", pelado,
                        relocalizar(e, destino, baseFlash, baseRam)));
                relocs++;
            } else {
                /* El `.mdn` no se toca: es position-independent. Sólo pierde el
                 * sufijo, que era para que el IDE supiera cuál era el suyo. */
                salida.add(new PackEntry(e.tipo, pelado, e.data));
            }
        }

        /* ¿Había familias y ninguna era la nuestra? Eso NO puede pasar callando:
         * se grabaria un pack sin motor y el fallo saldria mucho despues. */
        if (relocs == 0 && !ajenas.isEmpty() && !hayAlgoMio(p, sufMio)) {
            throw new BurnException("este pack no trae nada para '" + destino.nombre
                    + "' (sufijo " + destino.sufijo + "). Lleva: " + ajenas
                    + ". Hay que construirlo para este destino.");
        }

        byte[] nuevo;
        try {
            nuevo = PackWriter.build(p.nombre, p.versionContenido, p.fechaUnix, salida);
        } catch (PackException ex) {
            throw new BurnException("no se puede rearmar el pack: " + ex.getMessage());
        }
        Resultado r = new Resultado(nuevo, relocs, podadas);
        r.detalle.add("destino " + destino.nombre + " (" + destino.sufijo + ")");
        r.detalle.add(relocs + " .npk relocalizado(s) y sellado(s)");
        r.detalle.add(podadas + " entrada(s) de otras familias podada(s)"
                + (ajenas.isEmpty() ? "" : " " + ajenas));
        r.detalle.add(salida.size() + " entradas en el pack a grabar");
        return r;
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

    private static byte[] relocalizar(PackEntry e, NpackReloc.Destino d,
                                      int baseFlash, int baseRam)
            throws BurnException {
        try {
            /* Las bases de ENLACE del .npk sin relocalizar: el fichero las trae
             * a cero (no tiene sello todavia), asi que se le dicen. Son las del
             * enlace de referencia con que se construyo: flash 0, RAM el umbral. */
            NpackFile.Leido l = NpackFile.leer(e.data, d, 0, 0x20000000);
            if (l.relocalizado) {
                throw new BurnException("'" + e.nombre + "' ya viene relocalizado"
                    + " (sin tabla). Un pack distribuido debe traerla: se realoja"
                    + " AL GRABAR, no antes.");
            }
            NpackReloc.Imagen puesta =
                    NpackReloc.relocalizar(l.imagen, baseFlash, baseRam);
            return NpackFile.escribirGrabable(puesta, l.meta);
        } catch (NpackReloc.RelocException ex) {
            throw new BurnException("'" + e.nombre + "': " + ex.getMessage());
        }
    }
}
