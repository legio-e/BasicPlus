// ============================================================
// BpProject.java
// Fichero de proyecto JSON que describe QUÉ se ejecuta y DÓNDE
// están los módulos importados. Distinto del BpVM.cfg (que
// configura la VM en sí — memoria, stdlib). El BpProject vive en
// el proyecto del usuario y le dice a la VM:
//
//   - cuál es el .mod principal (donde está `main`);
//   - en qué directorios buscar los .mod de los imports.
//
// La VM acepta como argumento alternativo al .mod un fichero
// .bpproject. Detecta el tipo por extensión.
//
// Formato JSON (todos los campos opcionales salvo `main`):
//
//   {
//     "main":        "out/MyApp.mod",
//     "modulePaths": ["out", "../lib", "/abs/path/to/modules"]
//   }
//
// Reglas:
//   - `main`: ruta al .mod del módulo de arranque. Puede ser
//     absoluta o relativa al directorio del .bpproject.
//   - `modulePaths`: directorios donde se buscan .mod importados,
//     en orden. Paths relativos se resuelven respecto al
//     directorio del .bpproject. Se concatenan a la lista que la
//     VM ya tiene (stdlibDir de BpVM.cfg + cwd como fallback).
//
// El parser JSON es el mismo (ligero, plano) que VmConfig — esta
// vez con un caso especial para arrays de strings.
// ============================================================
package edu.bpgenvm.config;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class BpProject {

    /** Path absoluto al .mod principal (resuelto desde el .bpproject). */
    public String mainModPath;
    /** Directorios donde buscar imports, absolutos y en orden de prioridad. */
    public List<String> modulePaths = new ArrayList<>();
    /** Path absoluto del fichero .bpproject del que se cargó (informativo). */
    public String sourcePath;

    /**
     * Carga y resuelve un .bpproject. Lanza IOException si el JSON está mal
     * formado, falta `main`, o el .mod no existe en disco.
     */
    public static BpProject load(Path file) throws IOException {
        String text = new String(Files.readAllBytes(file), StandardCharsets.UTF_8);
        Object parsed = new JsonParser(text, file.toString()).parseTopValue();
        if (!(parsed instanceof Map)) {
            throw new IOException(file + ": el .bpproject debe ser un objeto JSON {}");
        }
        @SuppressWarnings("unchecked")
        Map<String, Object> map = (Map<String, Object>) parsed;

        BpProject p = new BpProject();
        p.sourcePath = file.toAbsolutePath().toString();
        Path projectDir = file.toAbsolutePath().getParent();
        if (projectDir == null) projectDir = file.toAbsolutePath();

        // main: obligatorio
        Object mainVal = map.get("main");
        if (!(mainVal instanceof String) || ((String) mainVal).isEmpty()) {
            throw new IOException(file + ": falta el campo 'main' (string)");
        }
        Path mainPath = projectDir.resolve((String) mainVal).toAbsolutePath().normalize();
        if (!Files.isRegularFile(mainPath)) {
            throw new IOException(file + ": el fichero indicado en 'main' no existe: " + mainPath);
        }
        p.mainModPath = mainPath.toString();

        // modulePaths: opcional, lista de strings
        Object pathsVal = map.get("modulePaths");
        if (pathsVal != null) {
            if (!(pathsVal instanceof List)) {
                throw new IOException(file + ": 'modulePaths' debe ser un array de strings");
            }
            List<?> raw = (List<?>) pathsVal;
            for (Object e : raw) {
                if (!(e instanceof String)) {
                    throw new IOException(file + ": 'modulePaths' debe contener sólo strings, no "
                            + (e == null ? "null" : e.getClass().getSimpleName()));
                }
                String entry = ((String) e).trim();
                if (entry.isEmpty()) continue;
                Path resolved = projectDir.resolve(entry).toAbsolutePath().normalize();
                // No exigimos que exista al cargar — la VM intenta cada path
                // al resolver imports y simplemente lo descarta si no aplica.
                // (Permite proyectos con paths opcionales o aún no creados.)
                p.modulePaths.add(resolved.toString());
            }
        }
        return p;
    }

    /** Forma legible para logs/errores. */
    @Override public String toString() {
        return "BpProject{main=" + mainModPath
                + ", modulePaths=" + modulePaths
                + (sourcePath == null ? "" : ", source=" + sourcePath)
                + "}";
    }

    /** Lista inmutable de modulePaths absolutos. */
    public List<String> modulePaths() {
        return Collections.unmodifiableList(modulePaths);
    }

    // ============================================================
    // Mini parser JSON (objects, arrays, strings, numbers, bools,
    // null; comentarios //). Compartido conceptualmente con VmConfig
    // pero con soporte de arrays (que VmConfig no necesita).
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
                if (m.containsKey(key))
                    throw err("clave duplicada: '" + key + "'");
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
            int line = 1, col = 1;
            for (int i = 0; i < pos && i < src.length(); i++) {
                if (src.charAt(i) == '\n') { line++; col = 1; }
                else col++;
            }
            return new IOException(filename + ":" + line + ":" + col + ": " + msg);
        }
    }
}
