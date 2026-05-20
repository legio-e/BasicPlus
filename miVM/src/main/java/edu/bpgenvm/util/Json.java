// ============================================================
// Json.java
// Helpers JSON minimalistas para el wire IDE↔VM. NO es un parser
// completo — sólo cubre lo que el protocolo necesita:
//   - Emisión: escape de strings.
//   - Parseo: objetos JSON PLANOS (sin anidamiento) con valores
//     string, número (long), boolean, null. No arrays. No objetos
//     dentro de objetos. Si alguna vez los necesitamos, refactorizamos
//     hacia VmConfig.JsonParser (que sí los soporta) o un parser de
//     verdad.
//
// Por qué un parser cojo: el protocolo del debugger usa mensajes muy
// regulares (`{"cmd":"continue"}`, `{"cmd":"setBreakpoint","file":"x.bp",
// "line":10,"enabled":true}`). Mantener el parser tonto evita un binding
// transitivo a Jackson/Gson y deja explícita la superficie del wire.
// ============================================================
package edu.bpgenvm.util;

import java.util.LinkedHashMap;
import java.util.Map;

public final class Json {
    private Json() {}

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

    /**
     * Parsea un objeto JSON PLANO. Devuelve un Map preservando orden de
     * llaves. Valores admitidos: String, Long, Boolean, null.
     *
     * Lanza IllegalArgumentException si la sintaxis no encaja en lo
     * que admitimos.
     */
    public static Map<String, Object> parseFlatObject(String src) {
        FlatParser p = new FlatParser(src);
        p.skipWs();
        if (p.peek() != '{') throw p.err("se esperaba '{'");
        p.pos++;
        Map<String, Object> out = new LinkedHashMap<>();
        p.skipWs();
        if (p.peek() == '}') { p.pos++; return out; }
        while (true) {
            p.skipWs();
            String key = p.parseString();
            p.skipWs();
            if (p.peek() != ':') throw p.err("se esperaba ':' tras la clave '" + key + "'");
            p.pos++;
            p.skipWs();
            Object val = p.parseValue();
            out.put(key, val);
            p.skipWs();
            char c = p.peek();
            if (c == ',') { p.pos++; continue; }
            if (c == '}') { p.pos++; break; }
            throw p.err("se esperaba ',' o '}', vi '" + c + "'");
        }
        return out;
    }

    /** Get tipado con default si la clave no está o no es del tipo. */
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

    // ============================================================
    // Parser
    // ============================================================
    private static final class FlatParser {
        final String src;
        int pos;
        FlatParser(String src) { this.src = src; this.pos = 0; }

        char peek() {
            if (pos >= src.length()) throw err("EOF inesperado");
            return src.charAt(pos);
        }

        void skipWs() {
            while (pos < src.length() && Character.isWhitespace(src.charAt(pos))) pos++;
        }

        Object parseValue() {
            char c = peek();
            if (c == '"') return parseString();
            if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
            if (c == 't' || c == 'f') return parseBool();
            if (c == 'n') { expectLit("null"); return null; }
            throw err("valor JSON inválido empezando por '" + c + "'");
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
            if (pos < src.length() && (src.charAt(pos) == '.' || src.charAt(pos) == 'e'
                    || src.charAt(pos) == 'E'))
                throw err("decimales/cientifica no soportados en wire");
            try { return Long.parseLong(src.substring(start, pos)); }
            catch (NumberFormatException ex) { throw err("número inválido"); }
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
