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
 *   compilador BP 1.0.0 · BpIde-5.0-shaded.jar · 2026-08-08 16:13
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
        String aviso = avisoDeDesfase();
        if (aviso != null) System.out.println(aviso);
    }

    /**
     * #429 — ¿ME HE QUEDADO RANCIO? Devuelve el aviso, o null si no hay motivo.
     *
     * <h3>El caso, que ha pasado DOS veces en dos días</h3>
     * BpIde empaqueta su PROPIA copia del compilador en el fat-jar. Al tocar el
     * frontend y no reempaquetar el IDE, el IDE sigue compilando con la copia
     * vieja: el mismo `.bp` da 0 errores por línea de órdenes y un error absurdo
     * desde el IDE («no puede utilizar long en código nativo»). El banner de
     * arriba ya deja verlo —si uno pone las dos líneas una encima de otra— pero
     * eso exige sospecharlo primero. Esto lo dice sin que nadie sospeche nada.
     *
     * <h3>Cómo se decide, sin adivinar</h3>
     * No hay heurística: se comparan FECHAS de ficheros que existen o no.
     * <ul>
     *   <li>Si esta copia ES el frontend (su jar o sus clases), no hay nada que
     *       comparar — se calla.</li>
     *   <li>Si está EMBEBIDA en otro artefacto y al lado hay un frontend
     *       construido MÁS TARDE, eso es un desfase y se dice.</li>
     *   <li>Si no hay frontend al lado —una distribución, la máquina de un
     *       usuario— no se dice nada. Ausencia de dato no es dato.</li>
     * </ul>
     *
     * <p>Nunca lanza, como todo lo de esta clase: un fallo averiguando esto no
     * puede tumbar una compilación.
     */
    public static String avisoDeDesfase() {
        try {
            File mio = procedencia();
            if (mio == null) return null;
            long tMio = mio.lastModified();
            if (tMio <= 0) return null;

            String nombre = mio.getName();
            // ¿Soy YO el frontend? Entonces no hay copia de la que desfasarse.
            if (nombre.startsWith("basicplus-frontend")) return null;
            if (mio.isFile() && nombre.endsWith(".class")
                    && mio.getPath().replace('\\', '/').contains("/lexer-java/target/classes/")) {
                return null;
            }

            File frontend = buscarFrontend();
            if (frontend == null) return null;
            long tFront = frontend.lastModified();
            if (tFront <= 0 || tFront <= tMio) return null;

            /* CON SEGUNDOS, y no es un detalle: el desfase que hay que cazar
             * suele ser de minutos o menos —se toca el frontend, se compila, y
             * se olvida reempaquetar el IDE—, así que con precisión de minuto
             * las dos fechas salen IGUALES y el aviso parece contradecirse a sí
             * mismo. Un aviso cuya evidencia no se ve se lee como falsa alarma,
             * y a la tercera ya no lo mira nadie. */
            SimpleDateFormat f = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            return "  AVISO: este compilador va EMBEBIDO y es MAS VIEJO que el del arbol."
                 + System.lineSeparator()
                 + "         embebido: " + nombreCorto(mio) + "  " + f.format(new Date(tMio))
                 + System.lineSeparator()
                 + "         al lado  : " + frontend.getName() + "  " + f.format(new Date(tFront))
                 + System.lineSeparator()
                 + "         Reconstruye el artefacto que lo embebe (BpIde) o compilaras"
                 + " con reglas viejas.";
        } catch (Throwable t) {
            return null;
        }
    }

    /** El frontend construido que haya en el árbol, si lo hay. Se prueban las
     *  rutas del repo desde el directorio de trabajo — nada de buscar por el
     *  disco: o está donde se construye, o no cuenta. */
    private static File buscarFrontend() {
        String[] candidatos = {
            "lexer-java/target/basicplus-frontend.jar",
            "../lexer-java/target/basicplus-frontend.jar",
            "lexer-java/target/classes/basicplus/frontend/Version.class",
            "../lexer-java/target/classes/basicplus/frontend/Version.class",
        };
        for (String c : candidatos) {
            File f = new File(c);
            if (f.isFile()) return f;
        }
        return null;
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
