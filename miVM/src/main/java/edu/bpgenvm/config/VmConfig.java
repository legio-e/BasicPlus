// ============================================================
// VmConfig.java
// Configuración global de la VM cargada desde un fichero JSON.
// Pasable a la VM vía `bpgenvm --config <ruta>`. Si no se pasa
// nada y existe `BpVM.cfg` en el cwd o en el directorio del .mod
// raíz, se carga automáticamente.
//
// Formato JSON aceptado (todos los campos opcionales):
//
//   {
//     "memorySize": 524288,         // bytes totales del array memory
//     "stackBase":  262144,         // offset donde empiezan los stacks
//     "stdlibDir":  "C:/.../stdlib" // path absoluto al directorio de stdlib
//   }
//
// Reglas de los valores por defecto (si el campo está ausente):
//   - memorySize: 524288 (512 KiB)
//   - stackBase:  memorySize / 2  (mitad para heap, mitad para stacks)
//   - stdlibDir:  null  (no se busca en stdlib; sólo cwd / fromPath)
//
// Parser ligero hand-rolled (JSON plano: objects, strings, numbers,
// booleans, null; no arrays ni anidados). Soporta comentarios `//`.
// Suficiente para la configuración actual y futura razonable.
// ============================================================
package edu.bpgenvm.config;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.LinkedHashMap;
import java.util.Map;

public final class VmConfig {

    public static final int DEFAULT_MEMORY_SIZE = 512 * 1024;
    public static final int DEFAULT_STACK_BASE  = 256 * 1024;
    /** Nombre canónico del fichero de configuración por defecto. */
    public static final String DEFAULT_FILENAME = "BpVM.cfg";

    public int    memorySize = DEFAULT_MEMORY_SIZE;
    public int    stackBase  = DEFAULT_STACK_BASE;
    public String stdlibDir  = null;
    /** Path real de donde se cargó (null si es config default sintética). */
    public String sourcePath = null;

    /** Configuración por defecto, equivalente a no pasar fichero. */
    public static VmConfig defaults() {
        return new VmConfig();
    }

    /**
     * Carga config desde el path indicado. Lanza IOException si no se
     * puede leer o si el JSON está mal formado.
     */
    public static VmConfig load(Path file) throws IOException {
        String text = new String(Files.readAllBytes(file), StandardCharsets.UTF_8);
        Map<String, Object> map = new JsonParser(text, file.toString()).parseTopObject();
        VmConfig c = new VmConfig();
        c.sourcePath = file.toAbsolutePath().toString();
        if (map.containsKey("memorySize")) c.memorySize = toInt(map.get("memorySize"), "memorySize", file);
        if (map.containsKey("stackBase"))  c.stackBase  = toInt(map.get("stackBase"),  "stackBase",  file);
        if (map.containsKey("stdlibDir"))  c.stdlibDir  = toString(map.get("stdlibDir"), "stdlibDir", file);

        // Si memorySize está pero stackBase no, derivar a mitad-mitad.
        if (map.containsKey("memorySize") && !map.containsKey("stackBase")) {
            c.stackBase = c.memorySize / 2;
        }
        c.validate(file);
        return c;
    }

    /**
     * Intenta cargar config en este orden:
     *   1) cwd/BpVM.cfg
     *   2) <dir del .mod raíz>/BpVM.cfg  (si rootModPath != null)
     * Devuelve defaults() si no encuentra ninguno.
     */
    public static VmConfig loadDefaultFor(Path rootModPath) {
        // 1) cwd
        Path cwd = Paths.get(System.getProperty("user.dir")).resolve(DEFAULT_FILENAME);
        if (Files.exists(cwd)) {
            try { return load(cwd); }
            catch (IOException ex) {
                System.err.println("Advertencia: no se pudo leer " + cwd + ": " + ex.getMessage());
            }
        }
        // 2) junto al .mod
        if (rootModPath != null) {
            Path parent = rootModPath.toAbsolutePath().getParent();
            if (parent != null) {
                Path next = parent.resolve(DEFAULT_FILENAME);
                if (Files.exists(next)) {
                    try { return load(next); }
                    catch (IOException ex) {
                        System.err.println("Advertencia: no se pudo leer " + next + ": " + ex.getMessage());
                    }
                }
            }
        }
        return defaults();
    }

    private void validate(Path file) throws IOException {
        if (memorySize <= 0)
            throw new IOException(file + ": memorySize debe ser > 0");
        if (stackBase <= 0)
            throw new IOException(file + ": stackBase debe ser > 0");
        if (stackBase >= memorySize)
            throw new IOException(file + ": stackBase (" + stackBase
                    + ") debe ser < memorySize (" + memorySize + ")");
        if (stdlibDir != null && stdlibDir.isEmpty())
            stdlibDir = null;
    }

    @Override public String toString() {
        return "VmConfig{"
                + "memorySize=" + memorySize
                + ", stackBase=" + stackBase
                + ", stdlibDir=" + (stdlibDir == null ? "<ninguno>" : stdlibDir)
                + (sourcePath == null ? "" : ", source=" + sourcePath)
                + "}";
    }

    // ============================================================
    // Helpers de coerción
    // ============================================================

    private static int toInt(Object v, String fieldName, Path file) throws IOException {
        if (v instanceof Long) {
            long n = (Long) v;
            if (n < Integer.MIN_VALUE || n > Integer.MAX_VALUE)
                throw new IOException(file + ": " + fieldName + " fuera de rango int: " + n);
            return (int) n;
        }
        throw new IOException(file + ": " + fieldName + " debe ser entero, no " + describe(v));
    }

    private static String toString(Object v, String fieldName, Path file) throws IOException {
        if (v instanceof String) return (String) v;
        throw new IOException(file + ": " + fieldName + " debe ser string, no " + describe(v));
    }

    private static String describe(Object v) {
        if (v == null) return "null";
        return v.getClass().getSimpleName().toLowerCase();
    }

    // ============================================================
    // Mini parser JSON
    //
    // Soporta: { ... }, strings con escapes mínimos, números enteros
    // con signo, true/false/null, comentarios // hasta fin de línea.
    // NO soporta: arrays, objetos anidados, números con decimales,
    // notación científica. Suficiente para BpVM.cfg.
    // ============================================================
    private static final class JsonParser {
        private final String src;
        private final String filename;
        private int pos;

        JsonParser(String src, String filename) {
            this.src = src;
            this.filename = filename;
            this.pos = 0;
        }

        Map<String, Object> parseTopObject() throws IOException {
            skipWs();
            Map<String, Object> m = parseObject();
            skipWs();
            if (pos < src.length())
                throw err("texto extra tras el objeto raíz");
            return m;
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
                if (m.containsKey(key))
                    throw err("clave duplicada en el objeto: '" + key + "'");
                m.put(key, val);
                skipWs();
                char c = peek();
                if (c == ',') { pos++; continue; }
                if (c == '}') { pos++; return m; }
                throw err("se esperaba ',' o '}', pero vi '" + c + "'");
            }
        }

        private Object parseValue() throws IOException {
            char c = peek();
            if (c == '"')                          return parseString();
            if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
            if (c == 't' || c == 'f')              return parseBool();
            if (c == 'n')                          { parseLiteral("null"); return null; }
            if (c == '{')                          throw err("objetos anidados no soportados");
            if (c == '[')                          throw err("arrays no soportados");
            throw err("valor JSON inválido empezando por '" + c + "'");
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
                } else {
                    sb.append(c);
                }
            }
            throw err("string sin cerrar");
        }

        private Long parseNumber() throws IOException {
            int start = pos;
            if (peek() == '-') pos++;
            while (pos < src.length() && Character.isDigit(src.charAt(pos))) pos++;
            if (pos == start || (pos == start + 1 && src.charAt(start) == '-'))
                throw err("número vacío");
            // Rechazar punto/expo para mantener la promesa de "entero".
            if (pos < src.length() && (src.charAt(pos) == '.' || src.charAt(pos) == 'e' || src.charAt(pos) == 'E'))
                throw err("decimales / notación científica no soportadas");
            try { return Long.parseLong(src.substring(start, pos)); }
            catch (NumberFormatException ex) { throw err("número inválido: " + src.substring(start, pos)); }
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
            // Calcular línea/columna
            int line = 1, col = 1;
            for (int i = 0; i < pos && i < src.length(); i++) {
                if (src.charAt(i) == '\n') { line++; col = 1; }
                else col++;
            }
            return new IOException(filename + ":" + line + ":" + col + ": " + msg);
        }
    }
}
