package com.mycompany.bpide;

/**
 * Breakpoint inmutable identificado por basename del fichero fuente
 * (e.g., "Json.bp") + número de línea 1-based.
 *
 * <p>Usamos basename en lugar del path completo porque el .dbg guarda el
 * path absoluto del .bp en el momento de la compilación, pero el usuario
 * puede mover el proyecto. Hacer match por basename es robusto pero
 * implica que dos ficheros con el mismo nombre en proyectos distintos
 * "colisionan"; aceptable para MVP.</p>
 */
public final class Breakpoint {
    public final String file;
    public final int    line;

    public Breakpoint(String file, int line) {
        this.file = file;
        this.line = line;
    }

    @Override public boolean equals(Object o) {
        if (!(o instanceof Breakpoint)) return false;
        Breakpoint b = (Breakpoint) o;
        return line == b.line && file.equals(b.file);
    }
    @Override public int hashCode() { return 31 * file.hashCode() + line; }
    @Override public String toString() { return file + ":" + line; }
}
