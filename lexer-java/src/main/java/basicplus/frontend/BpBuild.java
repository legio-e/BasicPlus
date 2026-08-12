// ============================================================
// BpBuild.java
// Fichero de proyecto JSON para el frontend (compilador) y el IDE.
// Distinto del .bpproject de la VM:
//   - .bpproject  → run-time: .mod a ejecutar + dirs donde buscar imports
//                   en formato compilado.
//   - .bpbuild    → build-time: fuentes .bp, dependencias, output dir,
//                   nombre del módulo principal del proyecto.
//
// Formato JSON (campos opcionales marcados):
//
//   {
//     "projectDir":   "./",                  // OPCIONAL — default = dir del .bpbuild
//     "sourceDir":    "src",                 // dir con los .bp del proyecto
//     "outDir":       "out",                 // dir donde se emiten .mod y .bpi
//     "main":         "App",                 // nombre lógico del módulo principal
//     "dependencies": [                      // OPCIONAL — n entradas
//       "../shared/lib",                     //   dir entero (todos sus .mod/.bpi)
//       "../utils/Logger.mod"                //   un módulo específico
//     ]
//   }
//
// Resolución de rutas:
//   - projectDir relativa → al directorio del .bpbuild.
//   - sourceDir / outDir / dependencies relativas → al projectDir.
//   - Paths absolutos respetados tal cual.
//
// Mismo parser JSON ligero (objetos, arrays, strings, numbers, bools, null,
// comentarios //) que usan VmConfig.java en miVM y similares.
// ============================================================
package basicplus.frontend;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class BpBuild {

    /** Path absoluto del directorio del proyecto (== `projectDir` resuelto). */
    public String projectDir;
    /** Path absoluto al directorio de fuentes. */
    public String sourceDir;
    /** Path absoluto al directorio de salida (.mod + .bpi). */
    public String outDir;
    /** Nombre lógico del módulo principal (sin extensión). */
    public String main;
    /** Cada entrada es un path absoluto que puede ser un directorio o un
     *  fichero .mod / .bpi específico. El compilador la prueba al
     *  resolver imports cuando no encuentra la dep en sources/outDir. */
    public List<String> dependencies = new ArrayList<>();
    /** Path absoluto al .bpbuild de origen (informativo). */
    public String sourcePath;

    /** AOT (H12): si true, al subir al device el IDE compila las funciones
     *  `function native` del proyecto a un `.mdn` nativo y lo sube junto al
     *  `.mod`. Si false (default), todo se interpreta — el .mod es suficiente. */
    public boolean aotEnabled = false;
    /** Target de la compilación AOT. "arm" = Cortex-M33 (RP2350 + STM32: mismo
     *  .mdn PIC para ambos). Futuro (V4): "esp32" (Xtensa / RISC-V). */
    public String aotTarget = "arm";

    /**
     * V5/H8 — las FAMILIAS del proyecto, cuando son varias (`aot.targets`).
     * Vacía = proyecto de una sola familia, y manda {@link #aotTarget}.
     *
     * <p>De un proyecto con varias sale <b>un</b> `.mod` y <b>varios</b>
     * `.mdn`, uno por familia y con su doble extensión (`SQLite.mdn.RISCV`).
     * No son builds distintos: es el MISMO `.bp`, el MISMO `.mod` y el MISMO
     * C intermedio; lo único que se repite es compilar ese C con cada
     * toolchain. Así es imposible que dos `.mdn` del pack no se correspondan
     * con el bytecode que llevan al lado.
     *
     * <p>Los nombres válidos salen de {@link NpackReloc#DESTINOS} y se
     * comprueban AL LEER el `.bpbuild` — un target mal escrito se ve en el
     * fichero, que es donde se arregla, y no tres pasos después en forma de
     * familia que falta en el pack.
     */
    public List<String> aotTargets = new ArrayList<>();

    /**
     * V5/H8 — la VERSIÓN del contenido del pack (`"pack": {"version": "3.53.4"}`).
     *
     * <p>Va a la CABECERA del pack, no al manifest: el formato ya tiene
     * `version_contenido` (≤16 B) y llevaba vacío desde H3. Es un campo
     * estructurado que el dispositivo lee sin parsear texto.
     */
    public String packVersion;

    /** V5/H8 — observaciones libres del pack. Éstas SÍ van al manifest: no hay
     *  sitio para ellas en la cabecera y no las lee nadie automáticamente. */
    public String packNotas;

    /**
     * V5/H8 — el fourcc de la API que este pack PUBLICA, si su motor nativo
     * publica alguna (`"provides": "SQLI"`).
     *
     * <p>Hace falta porque el nombre del `.npk` y el fourcc de su API son cosas
     * distintas: la entrada se llama `sqlite` y el motor se anuncia como
     * `'SQLI'`. Deducir uno del otro sería adivinar —cortar a 4 y subir a
     * mayúsculas funciona aquí y falla en cuanto haya un nombre que no se
     * parezca—, así que se declara.
     *
     * <p>Sirve para una cosa concreta: que un pack que importa de SU PROPIO
     * motor no se declare dependiente de sí mismo.
     */
    public String packProvides;

    /** Salida del build (H3 Packs): "normal" (default) o "pack". En "pack", tras
     *  el build normal se empaquetan los .mod/.mdn del outDir + los resources en
     *  un pack EJECUTABLE (el `main` va al manifest). */
    public String out = "normal";

    /** V5/H5 — base de datos del proyecto, para el ORM. Path ABSOLUTO ya
     *  resuelto, o null si el proyecto no declara ninguna.
     *
     *  ⚠️ Es la base **del PC**, no la del dispositivo, y **NO se copia al
     *  micro**. Decisión de Eduardo, y el motivo es que la base no es un
     *  artefacto: es un DATO. Todo lo demás que sube el IDE —.mod, resources—
     *  se deriva del proyecto y se puede regenerar; la base acumula lo que no
     *  existe en ningún otro sitio, así que copiarla en cada Run convertiría
     *  «ejecutar» en «perder los datos de campo».
     *
     *  Sirve para que la herramienta VERIFIQUE las entidades contra el esquema
     *  real y para generar los DAOs. En ejecución, el programa abre la ruta que
     *  le dé la gana (`/sd/medidas.db` en la placa) — son dos cosas distintas y
     *  conviene no confundirlas.
     *
     *  📌 Y en la placa, la base va en la SD, no en la flash interna: **las
     *  bases tienden a crecer** y llenar la flash del micro es cuestión de
     *  tiempo. La SD se carga desde un lector en el PC o desde el IDE. */
    public String database;

    /** Lo que el usuario escribió en el .bpbuild, sin resolver. Para que un
     *  diagnóstico pueda citar su texto y no un absoluto que no reconoce. */
    public String databaseRaw;

    /** Avisos recogidos al cargar el proyecto. NO son errores: el proyecto se
     *  carga igual y se puede compilar.
     *
     *  Existen porque hay problemas que no deben impedir compilar pero tampoco
     *  pueden pasar callados — una `database` vacía o que apunta a un fichero
     *  que no está. Si eso abortara la carga, un clon limpio sin el `.db` no
     *  podría ni compilar el programa; y si no dijera nada, trabajarías
     *  creyendo que hay una base detrás. */
    public final List<String> warnings = new ArrayList<>();

    /** El objeto JSON tal cual se parseó (LinkedHashMap → conserva el orden de
     *  claves). save() lo re-serializa tras actualizar los campos editables → se
     *  conservan rutas relativas y campos no modelados; solo se pierden los
     *  comentarios //. */
    private Map<String, Object> raw;

    /**
     * Carga y valida un .bpbuild desde disco. Lanza IOException si:
     *   - el JSON está malformado;
     *   - faltan sourceDir / outDir / main;
     *   - sourceDir o projectDir apuntan a un directorio inexistente.
     *   - outDir se crea on demand cuando se compila, así que no se exige.
     */
    public static BpBuild load(Path file) throws IOException {
        String text = new String(Files.readAllBytes(file), StandardCharsets.UTF_8);
        Object parsed = new JsonParser(text, file.toString()).parseTopValue();
        if (!(parsed instanceof Map)) {
            throw new IOException(file + ": el .bpbuild debe ser un objeto JSON {}");
        }
        @SuppressWarnings("unchecked")
        Map<String, Object> map = (Map<String, Object>) parsed;

        BpBuild b = new BpBuild();
        b.raw = map;   // para save(): re-serializa esto conservando lo no editado
        b.sourcePath = file.toAbsolutePath().toString();
        Path fileDir = file.toAbsolutePath().getParent();
        if (fileDir == null) fileDir = file.toAbsolutePath();

        // projectDir: opcional, default = dir del .bpbuild
        Object pdVal = map.get("projectDir");
        Path projectDirPath;
        if (pdVal == null) {
            projectDirPath = fileDir;
        } else if (pdVal instanceof String) {
            projectDirPath = fileDir.resolve((String) pdVal).toAbsolutePath().normalize();
        } else {
            throw new IOException(file + ": 'projectDir' debe ser string");
        }
        if (!Files.isDirectory(projectDirPath))
            throw new IOException(file + ": projectDir no es un directorio: " + projectDirPath);
        b.projectDir = projectDirPath.toString();

        // sourceDir: obligatorio
        Object sdVal = map.get("sourceDir");
        if (!(sdVal instanceof String) || ((String) sdVal).isEmpty())
            throw new IOException(file + ": falta 'sourceDir' (string)");
        Path sourceDirPath = projectDirPath.resolve((String) sdVal).toAbsolutePath().normalize();
        if (!Files.isDirectory(sourceDirPath))
            throw new IOException(file + ": sourceDir no es un directorio: " + sourceDirPath);
        b.sourceDir = sourceDirPath.toString();

        // outDir: obligatorio
        Object odVal = map.get("outDir");
        if (!(odVal instanceof String) || ((String) odVal).isEmpty())
            throw new IOException(file + ": falta 'outDir' (string)");
        b.outDir = projectDirPath.resolve((String) odVal).toAbsolutePath().normalize().toString();

        // main: obligatorio
        Object mainVal = map.get("main");
        if (!(mainVal instanceof String) || ((String) mainVal).isEmpty())
            throw new IOException(file + ": falta 'main' (nombre del módulo principal)");
        b.main = (String) mainVal;

        // dependencies: opcional, lista de strings
        Object depsVal = map.get("dependencies");
        if (depsVal != null) {
            if (!(depsVal instanceof List))
                throw new IOException(file + ": 'dependencies' debe ser un array de strings");
            for (Object e : (List<?>) depsVal) {
                if (!(e instanceof String))
                    throw new IOException(file + ": dependencies debe contener sólo strings");
                String entry = ((String) e).trim();
                if (entry.isEmpty()) continue;
                Path resolved = projectDirPath.resolve(entry).toAbsolutePath().normalize();
                // No exigimos que exista al cargar — el compilador intenta
                // cada entry al resolver imports y la descarta si no aplica.
                b.dependencies.add(resolved.toString());
            }
        }

        // database: opcional (V5/H5). Relativa al projectDir, o absoluta.
        //
        // Ni la cadena vacía ni un fichero que no está ABORTAN la carga: son
        // AVISOS. Cargar el proyecto no puede fallar por un fichero de DATOS —
        // un clon limpio sin el `.db` tiene que poder compilar el programa—,
        // pero tampoco pueden pasar callados, o trabajarías creyendo que hay
        // una base detrás cuando no la hay. Criterio de Eduardo.
        //
        // Lo único que sí es error es que no sea una cadena: eso es el fichero
        // de proyecto mal escrito, no un dato que falta.
        Object dbVal = map.get("database");
        if (dbVal != null) {
            if (!(dbVal instanceof String))
                throw new IOException(file + ": 'database' debe ser string");
            String raw = ((String) dbVal).trim();
            if (raw.isEmpty()) {
                b.warnings.add("'database' está vacía; quítala del proyecto si no hay base de datos");
            } else {
                b.databaseRaw = raw;
                Path dbPath = projectDirPath.resolve(raw).toAbsolutePath().normalize();
                b.database = dbPath.toString();
                if (!Files.isRegularFile(dbPath)) {
                    b.warnings.add("'database' apunta a '" + raw + "' y ahí no hay ningún fichero ("
                            + dbPath + "); las entidades no se podrán contrastar contra el esquema");
                } else if (b.databaseIsInsideResources()) {
                    // El agujero que ya existe: `resources/` se copia al micro en
                    // cada Run (#260), así que una base ahí se machacaría en cada
                    // ejecución — justo el accidente que evita "la BD no se copia".
                    b.warnings.add("'database' está dentro de resources/, y esa carpeta se copia al "
                            + "dispositivo en cada Run: la base se sobreescribiría y perderías los datos. "
                            + "Sácala de ahí");
                }
            }
        }

        // aot: opcional, { "enabled": bool, "target": string | "targets": [string] }
        Object aotVal = map.get("aot");
        if (aotVal != null) {
            if (!(aotVal instanceof Map))
                throw new IOException(file + ": 'aot' debe ser un objeto JSON {}");
            @SuppressWarnings("unchecked")
            Map<String, Object> aot = (Map<String, Object>) aotVal;
            Object en = aot.get("enabled");
            if (en instanceof Boolean) b.aotEnabled = (Boolean) en;
            Object tg  = aot.get("target");
            Object tgs = aot.get("targets");

            /* Las dos a la vez NO se resuelven eligiendo una: son dos respuestas
             * distintas a la misma pregunta, y cualquier preferencia que
             * pusiéramos aquí sería una regla que nadie recuerda. Se dice. */
            if (tg != null && tgs != null)
                throw new IOException(file + ": 'aot' trae 'target' y 'targets' a la"
                    + " vez. Son la misma cosa dicha de dos formas — deja sólo una"
                    + " ('targets' si el proyecto va a varias familias).");

            if (tg instanceof String && !((String) tg).isEmpty()) {
                b.aotTarget = validarTarget(((String) tg).trim(), file);
            }
            if (tgs != null) {
                if (!(tgs instanceof List))
                    throw new IOException(file + ": 'aot.targets' debe ser una lista"
                        + " JSON de familias, p.ej. [\"arm\", \"riscv\"]");
                for (Object o : (List<?>) tgs) {
                    if (!(o instanceof String))
                        throw new IOException(file + ": 'aot.targets' sólo admite"
                            + " nombres de familia (strings)");
                    String t = validarTarget(((String) o).trim(), file);
                    if (!b.aotTargets.contains(t)) b.aotTargets.add(t);
                }
                if (b.aotTargets.isEmpty())
                    throw new IOException(file + ": 'aot.targets' está vacía. Si el"
                        + " proyecto no tiene código nativo, quita el bloque 'aot';"
                        + " si lo tiene, di para qué familias.");
                /* El singular queda apuntando a la primera: lo que se sube al
                 * device en un Run sin placa conectada tiene que ser ALGO
                 * concreto, y la primera de la lista es lo menos sorprendente. */
                b.aotTarget = b.aotTargets.get(0);
            }
        }

        // pack: opcional, { "version": string, "notas": string } — metadatos del
        // pack. Separado de `out` a propósito: `out` dice CÓMO se construye, esto
        // dice QUÉ se está construyendo.
        Object packVal = map.get("pack");
        if (packVal != null) {
            if (!(packVal instanceof Map))
                throw new IOException(file + ": 'pack' debe ser un objeto JSON {}");
            @SuppressWarnings("unchecked")
            Map<String, Object> pk = (Map<String, Object>) packVal;
            Object v = pk.get("version");
            if (v instanceof String && !((String) v).isEmpty()) {
                String s = ((String) v).trim();
                /* El campo de la cabecera son 16 B UTF-8. Cortarlo en silencio
                 * daría una versión PLAUSIBLE y equivocada — el peor fallo. */
                if (s.getBytes(StandardCharsets.UTF_8).length > 16)
                    throw new IOException(file + ": 'pack.version' no cabe — el campo"
                        + " de la cabecera son 16 bytes y '" + s + "' ocupa "
                        + s.getBytes(StandardCharsets.UTF_8).length + ".");
                b.packVersion = s;
            }
            Object n = pk.get("notas");
            if (n instanceof String && !((String) n).isEmpty()) b.packNotas = ((String) n).trim();
            Object pv = pk.get("provides");
            if (pv instanceof String && !((String) pv).isEmpty()) {
                String s = ((String) pv).trim();
                if (s.length() != 4)
                    throw new IOException(file + ": 'pack.provides' es un fourcc de"
                        + " EXACTAMENTE 4 caracteres (el que publica el motor, p.ej."
                        + " \"SQLI\"); '" + s + "' tiene " + s.length() + ".");
                b.packProvides = s;
            }
        }

        // out: opcional, "normal" (default) | "pack"
        Object outVal = map.get("out");
        if (outVal != null) {
            if (!(outVal instanceof String))
                throw new IOException(file + ": 'out' debe ser string");
            String o = ((String) outVal).trim().toLowerCase();
            if (!o.equals("normal") && !o.equals("pack"))
                throw new IOException(file + ": 'out' debe ser 'normal' o 'pack', no '" + o + "'");
            b.out = o;
        }
        return b;
    }

    /**
     * Persiste el proyecto al `.bpbuild`, actualizando SOLO los campos editables
     * por el IDE (`out`, `aot`); el resto (sourceDir/outDir/main/dependencies) se
     * conserva TAL CUAL estaba en el fichero — rutas relativas incluidas — porque
     * re-serializa el mapa JSON crudo. Se pierden únicamente los comentarios //.
     */
    public void save(Path file) throws IOException {
        Map<String, Object> m = (raw != null) ? raw : new LinkedHashMap<>();
        // out: solo se escribe si es "pack" (default "normal" = ausente, sin ensuciar).
        if ("pack".equals(out)) m.put("out", "pack");
        else                    m.remove("out");
        // aot: objeto {enabled, target|targets}. Solo si está activo (off = ausente).
        if (aotEnabled) {
            Map<String, Object> aot = new LinkedHashMap<>();
            aot.put("enabled", Boolean.TRUE);
            // Se reescribe COMO SE DECLARÓ: quien puso `targets` no debe encontrarse
            // un `target` al guardar desde el IDE (y perder familias sin enterarse).
            if (!aotTargets.isEmpty()) aot.put("targets", new ArrayList<>(aotTargets));
            else                       aot.put("target",  aotTarget);
            m.put("aot", aot);
        } else {
            m.remove("aot");
        }
        StringBuilder sb = new StringBuilder();
        writeJson(m, sb, 0);
        sb.append('\n');
        Files.write(file, sb.toString().getBytes(StandardCharsets.UTF_8));
    }

    // -- serializador JSON mínimo (los tipos que produce el JsonParser de arriba) --
    private static void writeJson(Object v, StringBuilder sb, int indent) {
        if (v instanceof Map) {
            @SuppressWarnings("unchecked")
            Map<String, Object> m = (Map<String, Object>) v;
            if (m.isEmpty()) { sb.append("{}"); return; }
            sb.append("{\n");
            int i = 0;
            for (Map.Entry<String, Object> e : m.entrySet()) {
                indent(sb, indent + 1);
                writeStr(e.getKey(), sb);
                sb.append(": ");
                writeJson(e.getValue(), sb, indent + 1);
                if (++i < m.size()) sb.append(',');
                sb.append('\n');
            }
            indent(sb, indent); sb.append('}');
        } else if (v instanceof List) {
            List<?> l = (List<?>) v;
            if (l.isEmpty()) { sb.append("[]"); return; }
            sb.append("[\n");
            for (int i = 0; i < l.size(); i++) {
                indent(sb, indent + 1);
                writeJson(l.get(i), sb, indent + 1);
                if (i + 1 < l.size()) sb.append(',');
                sb.append('\n');
            }
            indent(sb, indent); sb.append(']');
        } else if (v instanceof String) {
            writeStr((String) v, sb);
        } else if (v instanceof Boolean || v instanceof Long || v instanceof Integer) {
            sb.append(v.toString());
        } else if (v == null) {
            sb.append("null");
        } else {
            writeStr(v.toString(), sb);
        }
    }

    private static void indent(StringBuilder sb, int n) { for (int i = 0; i < n; i++) sb.append("  "); }

    private static void writeStr(String s, StringBuilder sb) {
        sb.append('"');
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '"':  sb.append("\\\""); break;
                case '\\': sb.append("\\\\"); break;
                case '\n': sb.append("\\n"); break;
                case '\r': sb.append("\\r"); break;
                case '\t': sb.append("\\t"); break;
                default:   sb.append(c);
            }
        }
        sb.append('"');
    }

    @Override public String toString() {
        return "BpBuild{projectDir=" + projectDir
                + ", sourceDir=" + sourceDir
                + ", outDir=" + outDir
                + ", main=" + main
                + ", dependencies=" + dependencies
                + (sourcePath == null ? "" : ", source=" + sourcePath)
                + "}";
    }

    /**
     * Un target sólo vale si está en el catálogo de familias. Se comprueba al
     * LEER: escribir "risc-v" o "RISCV" en vez de "riscv" no debe descubrirse
     * cuando falte esa familia en el pack, sino aquí.
     */
    private static String validarTarget(String t, Path file) throws IOException {
        NpackReloc.Destino d = NpackReloc.porTargetAot(t);
        if (d == null)
            throw new IOException(file + ": familia AOT desconocida '" + t
                + "'. Las que hay: " + NpackReloc.targetsConocidos() + ".");
        return d.targetAot;              /* canónico: siempre en minúsculas */
    }

    public List<String> dependencies() { return Collections.unmodifiableList(dependencies); }

    // ---- V5/H5: la base de datos del proyecto ----

    /** ¿El proyecto declara base de datos? */
    public boolean hasDatabase() { return database != null; }

    /** ¿Y existe el fichero? Falso también si no se declara ninguna. */
    public boolean databaseExists() {
        return database != null && Files.isRegularFile(Paths.get(database));
    }

    /**
     * ¿La base cae DENTRO de `resources/`? Entonces hay un problema serio, y no
     * se ve a simple vista: `resources/` se copia al micro en cada Run (#260),
     * así que una base ahí se machacaría en cada ejecución — exactamente el
     * accidente que la decisión de «la BD no se copia» quiere evitar.
     *
     * El mecanismo para provocarlo YA existe, y `resources/` es el sitio donde
     * cualquiera dejaría un fichero de datos si no le damos otro. Por eso esto
     * se comprueba en vez de suponer que nadie lo hará.
     */
    public boolean databaseIsInsideResources() {
        if (database == null || projectDir == null) return false;
        Path res = Paths.get(projectDir).resolve("resources").toAbsolutePath().normalize();
        return Paths.get(database).startsWith(res);
    }

    // ============================================================
    // Mini parser JSON — copia de la del VmConfig pero soportando arrays
    // (que el .bpproject de la VM también necesitaba).
    // ============================================================
    private static final class JsonParser {
        private final String src;
        private final String filename;
        private int pos;

        JsonParser(String src, String filename) {
            this.src = src; this.filename = filename; this.pos = 0;
        }

        Object parseTopValue() throws IOException {
            skipWs();
            Object v = parseValue();
            skipWs();
            if (pos < src.length()) throw err("texto extra tras el valor raíz");
            return v;
        }

        private Object parseValue() throws IOException {
            char c = peek();
            if (c == '{')                          return parseObject();
            if (c == '[')                          return parseArray();
            if (c == '"')                          return parseString();
            if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
            if (c == 't' || c == 'f')              return parseBool();
            if (c == 'n')                          { parseLiteral("null"); return null; }
            throw err("valor JSON inválido empezando por '" + c + "'");
        }

        private Map<String, Object> parseObject() throws IOException {
            expect('{');
            Map<String, Object> m = new LinkedHashMap<>();
            skipWs();
            if (peek() == '}') { pos++; return m; }
            while (true) {
                skipWs();
                String key = parseString();
                skipWs();
                expect(':');
                skipWs();
                Object val = parseValue();
                if (m.containsKey(key)) throw err("clave duplicada: '" + key + "'");
                m.put(key, val);
                skipWs();
                char c = peek();
                if (c == ',') { pos++; continue; }
                if (c == '}') { pos++; return m; }
                throw err("se esperaba ',' o '}', vi '" + c + "'");
            }
        }

        private List<Object> parseArray() throws IOException {
            expect('[');
            List<Object> out = new ArrayList<>();
            skipWs();
            if (peek() == ']') { pos++; return out; }
            while (true) {
                skipWs();
                out.add(parseValue());
                skipWs();
                char c = peek();
                if (c == ',') { pos++; continue; }
                if (c == ']') { pos++; return out; }
                throw err("se esperaba ',' o ']', vi '" + c + "'");
            }
        }

        private String parseString() throws IOException {
            if (peek() != '"') throw err("se esperaba string");
            pos++;
            StringBuilder sb = new StringBuilder();
            while (pos < src.length()) {
                char c = src.charAt(pos++);
                if (c == '"') return sb.toString();
                if (c == '\\') {
                    if (pos >= src.length()) throw err("escape sin terminar");
                    char nxt = src.charAt(pos++);
                    switch (nxt) {
                        case '"':  sb.append('"'); break;
                        case '\\': sb.append('\\'); break;
                        case '/':  sb.append('/'); break;
                        case 'n':  sb.append('\n'); break;
                        case 'r':  sb.append('\r'); break;
                        case 't':  sb.append('\t'); break;
                        default: throw err("escape desconocido '\\" + nxt + "'");
                    }
                } else sb.append(c);
            }
            throw err("string sin cerrar");
        }

        private Long parseNumber() throws IOException {
            int start = pos;
            if (peek() == '-') pos++;
            while (pos < src.length() && Character.isDigit(src.charAt(pos))) pos++;
            if (pos == start || (pos == start + 1 && src.charAt(start) == '-'))
                throw err("número vacío");
            if (pos < src.length() && (src.charAt(pos) == '.' || src.charAt(pos) == 'e' || src.charAt(pos) == 'E'))
                throw err("decimales / notación científica no soportadas");
            try { return Long.parseLong(src.substring(start, pos)); }
            catch (NumberFormatException ex) { throw err("número inválido"); }
        }

        private Boolean parseBool() throws IOException {
            if (peek() == 't') { parseLiteral("true"); return Boolean.TRUE; }
            parseLiteral("false");
            return Boolean.FALSE;
        }

        private void parseLiteral(String lit) throws IOException {
            if (pos + lit.length() > src.length()
                    || !src.substring(pos, pos + lit.length()).equals(lit))
                throw err("se esperaba '" + lit + "'");
            pos += lit.length();
        }

        private void skipWs() {
            while (pos < src.length()) {
                char c = src.charAt(pos);
                if (Character.isWhitespace(c)) { pos++; continue; }
                if (c == '/' && pos + 1 < src.length() && src.charAt(pos + 1) == '/') {
                    while (pos < src.length() && src.charAt(pos) != '\n') pos++;
                    continue;
                }
                break;
            }
        }

        private void expect(char want) throws IOException {
            if (pos >= src.length() || src.charAt(pos) != want)
                throw err("se esperaba '" + want + "'");
            pos++;
        }

        private char peek() throws IOException {
            if (pos >= src.length()) throw err("EOF inesperado");
            return src.charAt(pos);
        }

        private IOException err(String msg) {
            int line = 1, col = 1;
            for (int i = 0; i < pos && i < src.length(); i++) {
                if (src.charAt(i) == '\n') { line++; col = 1; }
                else col++;
            }
            return new IOException(filename + ":" + line + ":" + col + ": " + msg);
        }
    }
}
