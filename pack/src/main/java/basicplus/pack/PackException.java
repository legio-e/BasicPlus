package basicplus.pack;

/**
 * Error de formato al leer o construir un pack. La regla es: entrada malformada
 * → mensaje CLARO, nunca un crash (ESPECIFICACION-PACKS-V4 §7.8).
 */
public final class PackException extends Exception {

    private static final long serialVersionUID = 1L;

    public PackException(String msg) { super(msg); }
}
