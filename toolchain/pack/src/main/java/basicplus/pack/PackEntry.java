package basicplus.pack;

/**
 * Una entrada de fichero dentro de un pack: la clave de resolución {@code (tipo,
 * nombre)} + los bytes. El pack es agnóstico del formato del contenido: guarda
 * bytes + tipo; el consumidor interpreta.
 */
public final class PackEntry {

    /** FourCC = la extensión en minúsculas (mod/mdn/img/win/fnt/cfg/mft…). */
    public final String tipo;
    /** Nombre; junto al tipo forma la clave de resolución {@code (tipo, nombre)}. */
    public final String nombre;
    /** Bytes crudos del fichero. */
    public final byte[] data;

    /**
     * V5/H8 — dónde empiezan los DATOS de esta entrada dentro de la imagen del
     * pack, o -1 si la entrada no viene de leer una (la construyó el que
     * empaqueta, y entonces todavía no hay imagen).
     *
     * <p>Hace falta para PARCHEAR en sitio: el `.npk` se relocaliza cuando ya
     * está dentro del pack, porque su dirección depende de dónde caiga el pack
     * entero. Sin esto habría que repetir aquí el recorrido de la tabla de
     * entradas, y el formato quedaría leído en dos sitios.
     */
    public final int dataOff;

    public PackEntry(String tipo, String nombre, byte[] data) {
        this(tipo, nombre, data, -1);
    }

    public PackEntry(String tipo, String nombre, byte[] data, int dataOff) {
        this.dataOff = dataOff;
        this.tipo = tipo;
        this.nombre = nombre;
        this.data = data;
    }

    @Override public String toString() {
        return "PackEntry{" + tipo + ":" + nombre + " (" + data.length + " B)}";
    }
}
