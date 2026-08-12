package basicplus.frontend;

import java.io.File;
import java.net.URL;
import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * QUIÉN está compilando. Una línea, al principio de cada build, haya errores o
 * no — idea de Eduardo (12-ago-2026) tras perder una mañana con esto.
 *
 * <h3>El caso que la motiva</h3>
 * El mismo proyecto daba <b>0 errores</b> por línea de órdenes y <b>17 errores
 * de sintaxis</b> desde el IDE. La causa: BpIde empaqueta su propia copia del
 * compilador, y la suya era de tres días antes — no conocía una sintaxis
 * (<code>from pack "NOMBRE"</code>) que el fuente ya usaba. Nada en la salida
 * permitía verlo; había que ir a mirar fechas de jars.
 *
 * <h3>Por qué el NÚMERO de versión no basta</h3>
 * Éste es el punto, y es lo que distingue esto de un banner decorativo: los dos
 * artefactos del incidente habrían dicho lo mismo. El número sólo cambia cuando
 * alguien lo sube en el pom; entre dos builds del mismo día es idéntico. Lo que
 * de verdad identifica al compilador es <b>de dónde salió y de cuándo es</b>:
 *
 * <pre>
 *   compilador BP 1.0.0 · BpIde-4.0-shaded.jar · 2026-08-08 16:13
 *   compilador BP dev   · target/classes       · 2026-08-11 10:05
 * </pre>
 *
 * Puestas una encima de otra, el diagnóstico es inmediato. Emparenta con la
 * lección del sello de build del firmware: un sello que no identifica la
 * imagen no sirve de nada, y encima da confianza falsa.
 *
 * <h3>Reglas</h3>
 * <ul>
 *   <li><b>Nunca lanza.</b> Un fallo averiguando la procedencia jamás puede
 *       tumbar una compilación: todo va en try/catch y lo desconocido se dice,
 *       no se calla ni se inventa.</li>
 *   <li>Fecha en <code>yyyy-MM-dd HH:mm</code>, que se ordena sola y no depende
 *       del idioma de la máquina.</li>
 * </ul>
 */
public final class Version {

    private Version() { }

    /** Versión del pom vía manifest, o "dev" si corre desde clases sueltas. */
    private static String numero() {
        try {
            Package p = Version.class.getPackage();
            String v = (p == null) ? null : p.getImplementationVersion();
            return (v == null || v.isEmpty()) ? "dev" : v;
        } catch (Throwable t) {
            return "?";
        }
    }

    /**
     * El fichero del que salió este código: el .jar, o el directorio de clases.
     * Devuelve null si no se puede averiguar (cargador exótico, sin permisos).
     */
    private static File procedencia() {
        try {
            URL u = Version.class.getProtectionDomain().getCodeSource().getLocation();
            if (u == null) return null;
            File f = new File(u.toURI());
            // Si es un DIRECTORIO, su fecha no dice nada: cambia al crear
            // cualquier subcarpeta y no al recompilar una clase. La que sirve es
            // la de nuestro propio .class, que es codigo del compilador.
            if (f.isDirectory()) {
                URL propio = Version.class.getResource("Version.class");
                if (propio != null && "file".equals(propio.getProtocol())) {
                    File c = new File(propio.toURI());
                    if (c.isFile()) return c;
                }
            }
            return f;
        } catch (Throwable t) {
            return null;
        }
    }

    /** Nombre corto para enseñar: el del jar, o "<carpeta>/classes" si son clases. */
    private static String nombreCorto(File f) {
        String n = f.getName();
        if (n.endsWith(".jar")) return n;
        // Para clases sueltas el nombre del .class no informa; interesa la raíz.
        return "clases sueltas";
    }

    /**
     * Imprime la línea. Se llama desde las TRES raíces de build de {@code Main}
     * (compilación full, sólo-interfaz y sólo-diagnósticos), que es por donde
     * pasan tanto la CLI como el IDE. Tres llamadas y no una porque no existe
     * un embudo común más abajo; lo que sí está en un solo sitio es QUÉ se
     * imprime, que es lo que importa mantener.
     *
     * <p>Ojo con la tentación de imprimirlo una vez por JVM: el IDE vive
     * arrancado toda la sesión, así que sólo saldría en la primera compilación
     * y faltaría justo el día que hace falta.
     */
    public static void banner() {
        System.out.println(linea());
    }

    /**
     * La línea completa. Nunca null, nunca lanza.
     *
     * <p>Separador ASCII a propósito: la consola de Windows va en cp850/cp1252 y
     * cualquier cosa fuera de ASCII sale como '?'. Ya se probó con '·'.
     */
    public static String linea() {
        StringBuilder sb = new StringBuilder("compilador BP ").append(numero());
        File f = procedencia();
        if (f == null) {
            sb.append(" | procedencia desconocida");
        } else {
            sb.append(" | ").append(nombreCorto(f));
            try {
                long t = f.lastModified();
                if (t > 0) sb.append(" | ")
                        .append(new SimpleDateFormat("yyyy-MM-dd HH:mm").format(new Date(t)));
            } catch (Throwable ignore) { /* la fecha es un extra, no un requisito */ }
        }
        return sb.toString();
    }
}
