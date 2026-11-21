/*
 * Esquema — lee la estructura de una base SQLite (V5/H5).
 * ============================================================================
 *
 * Sólo se usa para VERIFICAR que las entidades cuadran con la base real. No
 * hace falta para generar: eso sale entero de la entidad anotada.
 *
 * ─── POR QUÉ JDBC, Y POR QUÉ "BLANDO" ──────────────────────────────────────
 *
 * Se barajaron tres formas de que Java viera el esquema:
 *
 *   · **JDBC** — correcto para cualquier base y 30 líneas de código. Cuesta
 *     una dependencia de 13 MB (lleva binarios nativos de varias plataformas).
 *   · **Leer el fichero .db a mano** — sin dependencias, pero es reimplementar
 *     un formato de fichero, y equivocarse en silencio daría avisos falsos.
 *     Justo la clase de fallo que este proyecto no deja pasar.
 *   · **Conducir nuestra propia VM-C** — el mismo SQLite que la placa, pero
 *     ata el compilador a un binario construido y a un pack.
 *
 * Se eligió JDBC porque aquí lo que importa es que la LECTURA sea cierta: un
 * esquema mal leído produce avisos falsos, y a los dos días nadie los mira.
 *
 * Pero la dependencia es BLANDA a propósito: este fichero se compila contra
 * `java.sql` y nada más, así que el driver sólo hace falta en EJECUCIÓN. Si no
 * está, `leer()` devuelve null con su motivo y el compilador sigue tan tranquilo
 * — la verificación es opcional por diseño (todo son avisos), así que quedarse
 * sin ella nunca puede impedir compilar.
 *
 * ⚠️ La base se abre en SÓLO LECTURA. Es del usuario y puede tener datos que no
 * están en ningún otro sitio; una herramienta de compilación no tiene por qué
 * poder tocarla.
 */
package basicplus.frontend;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class Esquema {

    /** Una columna tal como la declara la tabla. */
    public static final class Col {
        public final String nombre;
        public final String tipoDeclarado;   // lo que puso el CREATE TABLE (orientativo: es AFINIDAD)
        public final boolean pk;
        Col(String nombre, String tipoDeclarado, boolean pk) {
            this.nombre = nombre; this.tipoDeclarado = tipoDeclarado; this.pk = pk;
        }
    }

    /** tabla -> columnas, en el orden en que las declara la tabla. */
    public final Map<String, List<Col>> tablas = new LinkedHashMap<>();
    /** Por qué no se pudo leer, o null si se leyó bien. */
    public String motivoFallo;

    private Esquema() { }

    public static Esquema noDisponible(String motivo) {
        Esquema e = new Esquema();
        e.motivoFallo = motivo;
        return e;
    }

    public boolean disponible() { return motivoFallo == null; }

    public List<Col> columnasDe(String tabla) {
        for (Map.Entry<String, List<Col>> e : tablas.entrySet())
            if (e.getKey().equalsIgnoreCase(tabla)) return e.getValue();
        return null;
    }

    /**
     * Lee el esquema de `rutaDb`. Nunca lanza: si algo va mal devuelve un
     * Esquema con `motivoFallo` puesto, porque no poder verificar no puede
     * tumbar una compilación.
     */
    public static Esquema leer(String rutaDb) {
        Esquema es = new Esquema();
        // `mode=ro` sobre URI: además de no tocar nada, evita que un typo en la
        // ruta CREE una base vacía y nos haga avisar de que faltan 40 tablas.
        String url = "jdbc:sqlite:file:" + rutaDb.replace('\\', '/') + "?mode=ro";
        try (Connection cn = DriverManager.getConnection(url)) {
            List<String> nombres = new ArrayList<>();
            try (Statement st = cn.createStatement();
                 ResultSet rs = st.executeQuery(
                         "SELECT name FROM sqlite_master WHERE type='table'"
                       + " AND name NOT LIKE 'sqlite_%' ORDER BY name")) {
                while (rs.next()) nombres.add(rs.getString(1));
            }
            for (String t : nombres) {
                List<Col> cols = new ArrayList<>();
                try (Statement st = cn.createStatement();
                     ResultSet rs = st.executeQuery("PRAGMA table_info(\"" + t.replace("\"", "\"\"") + "\")")) {
                    while (rs.next()) {
                        cols.add(new Col(rs.getString("name"),
                                         rs.getString("type"),
                                         rs.getInt("pk") > 0));
                    }
                }
                es.tablas.put(t, cols);
            }
            return es;
        } catch (SQLException ex) {
            String m = ex.getMessage() == null ? "" : ex.getMessage();
            if (m.toLowerCase().contains("no suitable driver")) {
                // OJO con el mensaje: «no suitable driver» sale TANTO si falta
                // sqlite-jdbc como si está pero le falta a ÉL su propia
                // dependencia (slf4j-api) — el driver no llega a registrarse y
                // JDBC no distingue los dos casos. Con Maven slf4j viene por
                // transitividad, así que esto sólo aparece con un classpath
                // montado a mano; pero nombrar sólo al driver mandaría a buscar
                // lo que no es. Medido al probar el verificador.
                return noDisponible("no se pudo registrar el driver de SQLite: falta sqlite-jdbc"
                        + " en el classpath, o su dependencia slf4j-api");
            }
            return noDisponible("no se pudo abrir la base: " + m);
        } catch (Throwable ex) {
            // Incluye NoClassDefFoundError si el driver está a medias.
            return noDisponible("no se pudo leer la base: " + ex);
        }
    }
}
