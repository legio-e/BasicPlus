// ============================================================
// Json.java
// Helpers JSON para el wire IDE↔VM. Cubre:
//   - Emisión: escape de strings (escape).
//   - Parseo recursivo: parse(src) devuelve Object (Map<String,Object>,
//     List<Object>, String, Long, Boolean, null).
//   - Compat: parseFlatObject(src) sigue funcionando — asume top-level
//     objeto y devuelve el Map; los valores pueden ser primitivos o
//     anidados (el caller los castea según contexto).
//
// Por qué no Jackson/Gson: el contrato del wire es muy regular y el
// tráfico es pequeño. Mantener el parser local evita un dep transitivo
// y deja explícita la superficie del protocolo. Si en algún momento
// el wire se complica (binary chunks de upload de ficheros, etc.),
// reevaluar.
// ============================================================
package edu.bpgenvm.util;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class Json {
    private Json() {}

    // ============================================================
    // Emisión
    // ============================================================

    /** Escapa un String para meterlo entre comillas en JSON. */
    public static String escape(String s) {
        if (s == null) return "null";
        StringBuilder sb = new StringBuilder(s.length() + 8);
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '"':  sb.append("\\\""); break;
                case '\\': sb.append("\\\\"); break;
                case '\b': sb.append("\\b");  break;
                case '\f': sb.append("\\f");  break;
                case '\n': sb.append("\\n");  break;
                case '\r': sb.append("\\r");  break;
                case '\t': sb.append("\\t");  break;
                default:
                    if (c < 0x20) {
                        sb.append(String.format("\\u%04x", (int) c));
                    } else {
                        sb.append(c);
                    }
            }
        }
        return sb.toString();
    }

    /** Devuelve `"<escape(s)>"`. */
    public static String quote(String s) {
        return "\"" + escape(s) + "\"";
    }

    /**
     * Serializa un valor (el árbol que devuelve {@link #parse}: Map/List/String/
     * Long/Integer/Boolean/null) a JSON compacto. Permite el round-trip
     * parse→modificar→write (lo usa el horneado de Forms del IDE sobre el .win).
     */
    public static String write(Object v) {
        StringBuilder sb = new StringBuilder();
        writeValue(sb, v);
        return sb.toString();
    }

    @SuppressWarnings("unchecked")
    private static void writeValue(StringBuilder sb, Object v) {
        if (v == null) { sb.append("null"); return; }
        if (v instanceof String)  { sb.append(quote((String) v)); return; }
        if (v instanceof Boolean) { sb.append(((Boolean) v) ? "true" : "false"); return; }
        if (v instanceof Long || v instanceof Integer
                || v instanceof Double || v instanceof Float) { sb.append(v.toString()); return; }
        if (v instanceof Map) {
            sb.append('{');
            boolean first = true;
            for (Map.Entry<String, Object> e : ((Map<String, Object>) v).entrySet()) {
                if (!first) sb.append(',');
                first = false;
                sb.append(quote(e.getKey())).append(':');
                writeValue(sb, e.getValue());
            }
            sb.append('}');
            return;
        }
        if (v instanceof List) {
            sb.append('[');
            boolean first = true;
            for (Object e : (List<Object>) v) {
                if (!first) sb.append(',');
                first = false;
                writeValue(sb, e);
            }
            sb.append(']');
            return;
        }
        sb.append(quote(v.toString()));   // fallback defensivo
    }

    // ============================================================
    // Parsing
    // ============================================================

    /**
     * Parsea un valor JSON arbitrario. Devuelve uno de:
     *   - Map&lt;String,Object&gt;  (objetos)
     *   - List&lt;Object&gt;        (arrays)
     *   - String, Long, Boolean, null
     *
     * Lanza IllegalArgumentException si la sintaxis no es válida o hay
     * texto extra tras el valor raíz.
     */
    public static Object parse(String src) {
        Parser p = new Parser(src);
        p.skipWs();
        Object v = p.parseValue();
        p.skipWs();
        if (p.pos < p.src.length())
            throw p.err("texto extra tras el valor raíz");
        return v;
    }

    /** Conveniencia: parsea esperando un objeto top-level. Devuelve el
     *  Map directamente. Lanza si la raíz no es objeto. */
    @SuppressWarnings("unchecked")
    public static Map<String, Object> parseFlatObject(String src) {
        Object v = parse(src);
        if (!(v instanceof Map))
            throw new IllegalArgumentException("se esperaba objeto JSON top-level");
        return (Map<String, Object>) v;
    }

    // ============================================================
    // Getters tipados con default
    // ============================================================

    public static String getString(Map<String, Object> m, String k, String def) {
        Object v = m.get(k);
        return (v instanceof String) ? (String) v : def;
    }
    public static long getLong(Map<String, Object> m, String k, long def) {
        Object v = m.get(k);
        if (v instanceof Long) return (Long) v;
        if (v instanceof Integer) return (Integer) v;
        return def;
    }
    public static boolean getBool(Map<String, Object> m, String k, boolean def) {
        Object v = m.get(k);
        return (v instanceof Boolean) ? (Boolean) v : def;
    }

    @SuppressWarnings("unchecked")
    public static List<Object> getList(Map<String, Object> m, String k) {
        Object v = m.get(k);
        return (v instanceof List) ? (List<Object>) v : null;
    }

    @SuppressWarnings("unchecked")
    public static Map<String, Object> getMap(Map<String, Object> m, String k) {
        Object v = m.get(k);
        return (v instanceof Map) ? (Map<String, Object>) v : null;
    }

    // ============================================================
    // Parser
    // ============================================================
    private static final class Parser {
        final String src;
        int pos;
        Parser(String src) { this.src = src; this.pos = 0; }

        char peek() {
            if (pos >= src.length()) throw err("EOF inesperado");
            return src.charAt(pos);
        }

        void skipWs() {
            while (pos < src.length() && Character.isWhitespace(src.charAt(pos))) pos++;
        }

        Object parseValue() {
            skipWs();
            char c = peek();
            if (c == '{') return parseObject();
            if (c == '[') return parseArray();
            if (c == '"') return parseString();
            if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
            if (c == 't' || c == 'f') return parseBool();
            if (c == 'n') { expectLit("null"); return null; }
            throw err("valor JSON inválido empezando por '" + c + "'");
        }

        Map<String, Object> parseObject() {
            if (peek() != '{') throw err("se esperaba '{'");
            pos++;
            Map<String, Object> m = new LinkedHashMap<>();
            skipWs();
            if (peek() == '}') { pos++; return m; }
            while (true) {
                skipWs();
                String key = parseString();
                skipWs();
                if (peek() != ':') throw err("se esperaba ':' tras la clave '" + key + "'");
                pos++;
                skipWs();
                Object val = parseValue();
                m.put(key, val);
                skipWs();
                char c = peek();
                if (c == ',') { pos++; continue; }
                if (c == '}') { pos++; return m; }
                throw err("se esperaba ',' o '}', vi '" + c + "'");
            }
        }

        List<Object> parseArray() {
            if (peek() != '[') throw err("se esperaba '['");
            pos++;
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

        String parseString() {
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
                        case 'b':  sb.append('\b'); break;
                        case 'f':  sb.append('\f'); break;
                        case 'n':  sb.append('\n'); break;
                        case 'r':  sb.append('\r'); break;
                        case 't':  sb.append('\t'); break;
                        case 'u':
                            if (pos + 4 > src.length()) throw err("\\u sin 4 hex");
                            String hex = src.substring(pos, pos + 4);
                            pos += 4;
                            sb.append((char) Integer.parseInt(hex, 16));
                            break;
                        default: throw err("escape desconocido '\\" + nxt + "'");
                    }
                } else sb.append(c);
            }
            throw err("string sin cerrar");
        }

        Long parseNumber() {
            int start = pos;
            if (peek() == '-') pos++;
            while (pos < src.length() && Character.isDigit(src.charAt(pos))) pos++;
            if (pos == start || (pos == start + 1 && src.charAt(start) == '-'))
                throw err("número vacío");
            int intEnd = pos;
            boolean hasFloat = false;
            // Wire v1 es integer-only por diseño. Si llega un float (violación
            // de spec del otro lado), antes lanzábamos y el reader tragaba
            // silenciosa → 10s de timeout del caller (tarea #155). Ahora
            // consumimos todo el literal numérico y truncamos a long.
            if (pos < src.length() && src.charAt(pos) == '.') {
                hasFloat = true;
                pos++;
                while (pos < src.length() && Character.isDigit(src.charAt(pos))) pos++;
            }
            if (pos < src.length() && (src.charAt(pos) == 'e' || src.charAt(pos) == 'E')) {
                hasFloat = true;
                pos++;
                if (pos < src.length() && (src.charAt(pos) == '+' || src.charAt(pos) == '-')) pos++;
                while (pos < src.length() && Character.isDigit(src.charAt(pos))) pos++;
            }
            try {
                if (hasFloat) {
                    System.err.println("[Json] warn: float en wire (spec: int-only): "
                        + src.substring(start, pos) + " — truncando a long");
                    return (long) Double.parseDouble(src.substring(start, pos));
                }
                return Long.parseLong(src.substring(start, intEnd));
            } catch (NumberFormatException ex) { throw err("número inválido"); }
        }

        Boolean parseBool() {
            if (peek() == 't') { expectLit("true"); return Boolean.TRUE; }
            expectLit("false");
            return Boolean.FALSE;
        }

        void expectLit(String lit) {
            if (pos + lit.length() > src.length()
                    || !src.substring(pos, pos + lit.length()).equals(lit))
                throw err("se esperaba '" + lit + "'");
            pos += lit.length();
        }

        IllegalArgumentException err(String msg) {
            return new IllegalArgumentException("JSON[" + pos + "]: " + msg);
        }
    }
}
