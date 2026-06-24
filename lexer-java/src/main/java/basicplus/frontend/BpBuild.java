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

        // aot: opcional, objeto { "enabled": bool, "target": string }
        Object aotVal = map.get("aot");
        if (aotVal != null) {
            if (!(aotVal instanceof Map))
                throw new IOException(file + ": 'aot' debe ser un objeto JSON {}");
            @SuppressWarnings("unchecked")
            Map<String, Object> aot = (Map<String, Object>) aotVal;
            Object en = aot.get("enabled");
            if (en instanceof Boolean) b.aotEnabled = (Boolean) en;
            Object tg = aot.get("target");
            if (tg instanceof String && !((String) tg).isEmpty()) b.aotTarget = ((String) tg).trim();
        }
        return b;
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

    public List<String> dependencies() { return Collections.unmodifiableList(dependencies); }

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
