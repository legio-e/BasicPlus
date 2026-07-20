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

    public PackEntry(String tipo, String nombre, byte[] data) {
        this.tipo = tipo;
        this.nombre = nombre;
        this.data = data;
    }

    @Override public String toString() {
        return "PackEntry{" + tipo + ":" + nombre + " (" + data.length + " B)}";
    }
}
