// ============================================================
// JcLexer.java
// V8 #698 (JC, paso 3) — el escáner de JC.
//
// JC es BP con la ropa de Java y C# (V8_IDEAS §E2). Éste es el escáner de su
// sintaxis (puntos 1–36):
//   - las palabras reservadas DISTINGUEN mayúsculas (punto 1), al revés que en
//     BP; los nombres, como en BP, también;
//   - los saltos de línea no cuentan: las sentencias acaban en `;` (punto 4);
//   - los literales, los de C# (punto 19): `1.5` es double y `1.5f`, float; un
//     entero es long si no cabe en 32 bits, como en BP (#385); `'a'` es un
//     entero, su punto de código; y en las cadenas, los escapes de C#;
//   - los comentarios, `//` y `/* … */`.
// El parser (JcParser) lee estos tokens y construye el MISMO árbol que el
// parser de BP.
// ============================================================
package basicplus.frontend;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

final class JcLexer {

    /** Un error de JC, con su línea y su columna. `lexico` si lo da el escáner y no el parser: el compilador lo dice
     *  como los de BP, «error léxico» o «error sintáctico» (V8 #698). */
    static final class Error extends RuntimeException {
        final String mensaje;
        final int linea, columna;
        final boolean lexico;
        Error(String mensaje, int linea, int columna) { this(mensaje, linea, columna, true); }
        Error(String mensaje, int linea, int columna, boolean lexico) {
            super("[" + linea + ":" + columna + "] " + mensaje);
            this.mensaje = mensaje;
            this.linea = linea;
            this.columna = columna;
            this.lexico = lexico;
        }
    }

    enum Tipo { NOMBRE, RESERVADA, ENTERO, LARGO, FLOTANTE, DOBLE, CADENA, SIGNO, FIN }

    static final class Tok {
        final Tipo tipo;
        final String texto;   // el lexema; en una CADENA, su valor
        final Object valor;   // Long, Double o String
        final int linea, columna;
        Tok(Tipo tipo, String texto, Object valor, int linea, int columna) {
            this.tipo = tipo;
            this.texto = texto;
            this.valor = valor;
            this.linea = linea;
            this.columna = columna;
        }
        boolean es(String s) { return (tipo == Tipo.RESERVADA || tipo == Tipo.SIGNO) && texto.equals(s); }
        @Override public String toString() { return tipo == Tipo.FIN ? "el final" : "'" + texto + "'"; }
    }

    /** Las palabras reservadas de JC (puntos 1–36). */
    static final Set<String> RESERVADAS = new HashSet<>(Arrays.asList(
            "module", "library", "interface", "implements", "extends", "import", "from",
            "const", "function", "class", "enum", "public", "private", "protected", "final", "native", "sync", "owner",
            "event", "static", "void", "var", "new", "is", "base", "field",
            "if", "else", "while", "do", "for", "foreach", "in", "break", "continue", "return",
            "switch", "case", "default", "parallel", "try", "catch", "finally", "throw", "raise",
            "print", "println", "inline", "this", "super", "true", "false", "null",
            "int", "long", "float", "double", "bool", "string", "byte", "sbyte", "short", "ushort"));

    /** Los signos de tres caracteres, que se miran antes que los de dos: los compuestos de los desplazamientos
     *  (V8/H7.1 #747). */
    private static final String[] SIGNOS_3 = { "<<=", ">>=" };
    /** Los signos de dos caracteres, que se miran antes que los de uno. */
    private static final String[] SIGNOS_2 = {
            "**", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "++", "--", "+=", "-=", "::",
            "*=", "/=", "%=", "&=", "|=", "^=" };   // los compuestos de C# (V8/H7.1 #747; antes del 31-oct, un error)
    private static final String SIGNOS_1 = "+-*/%&|^!~<>=(){}[],;:.@?";   // `?`, el ternario (V8/H7.1 #747)

    private final String src;
    private int pos = 0, linea = 1, columna = 1;

    private JcLexer(String src) { this.src = src; }

    static List<Tok> leer(String fuente) {
        JcLexer lx = new JcLexer(fuente);
        List<Tok> out = new ArrayList<>();
        while (true) {
            Tok t = lx.siguiente();
            out.add(t);
            if (t.tipo == Tipo.FIN) return out;
        }
    }

    // ============================================================
    // CURSOR
    // ============================================================
    private boolean fin() { return pos >= src.length(); }
    private char ver() { return fin() ? '\0' : src.charAt(pos); }
    private char ver(int n) { return pos + n < src.length() ? src.charAt(pos + n) : '\0'; }

    private char avanza() {
        char c = src.charAt(pos++);
        if (c == '\n') { linea++; columna = 1; } else columna++;
        return c;
    }

    private Error error(String msg) { return new Error(msg, linea, columna); }

    // ============================================================
    // TOKENS
    // ============================================================
    private Tok siguiente() {
        saltaBlancosYComentarios();
        int l = linea, c = columna;
        if (fin()) return new Tok(Tipo.FIN, "", null, l, c);
        char ch = ver();
        if (Character.isLetter(ch) || ch == '_') return nombre(l, c);
        if (Character.isDigit(ch)) return numero(l, c);
        if (ch == '"') return cadena(l, c);
        if (ch == '\'') return caracter(l, c);
        if (ch == '@' && ver(1) == '"') throw error("JC no tiene las cadenas @\"…\" de C#, de momento");
        if (ch == '$' && ver(1) == '"') throw error("JC no tiene las cadenas $\"…\" de C# (la interpolación), de momento");
        if (ch == '?' && (ver(1) == '?' || ver(1) == '.'))
            throw error("JC no tiene el " + (ver(1) == '?' ? "??" : "?.") + " de C#, de momento (el ternario, c ? a : b, sí)");
        for (String s : SIGNOS_3) {
            if (src.startsWith(s, pos)) {
                avanza();
                avanza();
                avanza();
                return new Tok(Tipo.SIGNO, s, null, l, c);
            }
        }
        for (String s : SIGNOS_2) {
            if (src.startsWith(s, pos)) {
                avanza();
                avanza();
                return new Tok(Tipo.SIGNO, s, null, l, c);
            }
        }
        if (SIGNOS_1.indexOf(ch) >= 0) {
            avanza();
            return new Tok(Tipo.SIGNO, String.valueOf(ch), null, l, c);
        }
        throw error(String.format("carácter inesperado: '%s' (U+%04X)", ch, (int) ch));
    }

    private void saltaBlancosYComentarios() {
        while (!fin()) {
            char ch = ver();
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                avanza();
            } else if (ch == '/' && ver(1) == '/') {
                while (!fin() && ver() != '\n') avanza();
            } else if (ch == '/' && ver(1) == '*') {
                int l = linea, c = columna;
                avanza();
                avanza();
                while (!(ver() == '*' && ver(1) == '/')) {
                    if (fin()) throw new Error("comentario /* sin cerrar", l, c);
                    avanza();
                }
                avanza();
                avanza();
            } else {
                return;
            }
        }
    }

    private Tok nombre(int l, int c) {
        int ini = pos;
        while (!fin() && (Character.isLetterOrDigit(ver()) || ver() == '_')) avanza();
        String s = src.substring(ini, pos);
        if (s.length() > 63)   // el mismo tope que BP (V10)
            throw new Error("identificador demasiado largo (" + s.length() + " caracteres; el máximo es 63): '"
                    + s.substring(0, 20) + "…'", l, c);
        return new Tok(RESERVADAS.contains(s) ? Tipo.RESERVADA : Tipo.NOMBRE, s, null, l, c);
    }

    /** Los números (punto 19): `0x`, `0b` y `_` como en BP; `L` long; `f` float; `d` o nada con punto o exponente,
     *  double. Un entero sin sufijo es long si no cabe en 32 bits (#385). */
    private Tok numero(int l, int c) {
        int ini = pos;
        if (ver() == '0' && (ver(1) == 'x' || ver(1) == 'X' || ver(1) == 'b' || ver(1) == 'B')) {
            boolean hex = ver(1) == 'x' || ver(1) == 'X';
            avanza();
            avanza();
            int ini2 = pos;
            while (!fin() && (digito(ver(), hex) || (ver() == '_' && pos > ini2 && digito(ver(1), hex)))) avanza();
            String cifras = src.substring(ini2, pos).replace("_", "");
            if (cifras.isEmpty()) throw new Error("literal " + (hex ? "hexadecimal" : "binario") + " vacío", l, c);
            long v;
            try { v = Long.parseLong(cifras, hex ? 16 : 2); }
            catch (NumberFormatException ex) { throw new Error("literal fuera de rango: " + src.substring(ini, pos), l, c); }
            return entero(v, sufijoL(), src.substring(ini, pos), l, c);
        }
        while (!fin() && (Character.isDigit(ver()) || (ver() == '_' && Character.isDigit(ver(1))))) avanza();
        boolean real = false;
        if (ver() == '.' && Character.isDigit(ver(1))) {
            real = true;
            avanza();
            while (!fin() && (Character.isDigit(ver()) || (ver() == '_' && Character.isDigit(ver(1))))) avanza();
        }
        if (ver() == 'e' || ver() == 'E') {
            real = true;
            avanza();
            if (ver() == '+' || ver() == '-') avanza();
            if (!Character.isDigit(ver())) throw new Error("exponente sin cifras", l, c);
            while (!fin() && Character.isDigit(ver())) avanza();
        }
        String lex = src.substring(ini, pos);
        String cifras = lex.replace("_", "");
        char suf = ver();
        if (suf == 'f' || suf == 'F' || suf == 'd' || suf == 'D') {
            avanza();
            double d = Double.parseDouble(cifras);
            boolean flotante = suf == 'f' || suf == 'F';
            return new Tok(flotante ? Tipo.FLOTANTE : Tipo.DOBLE, lex + suf, d, l, c);
        }
        if (suf == 'u' || suf == 'U' || suf == 'm' || suf == 'M')
            throw new Error("JC no tiene el sufijo '" + suf + "' de C#: BP no tiene enteros sin signo de 32 y 64 bits"
                    + " ni decimal", l, c);
        if (real) return new Tok(Tipo.DOBLE, lex, Double.parseDouble(cifras), l, c);   // en C#, 1.5 es double
        long v;
        try { v = Long.parseLong(cifras); }
        catch (NumberFormatException ex) { throw new Error("literal entero fuera de rango: " + lex, l, c); }
        return entero(v, sufijoL(), lex, l, c);
    }

    private boolean sufijoL() {
        if (ver() == 'L' || ver() == 'l') { avanza(); return true; }
        return false;
    }

    private static Tok entero(long v, boolean sufijoL, String lex, int l, int c) {
        boolean largo = sufijoL || v < Integer.MIN_VALUE || v > Integer.MAX_VALUE;   // #385
        return new Tok(largo ? Tipo.LARGO : Tipo.ENTERO, lex, v, l, c);
    }

    private static boolean digito(char ch, boolean hex) {
        return hex ? Character.digit(ch, 16) >= 0 : (ch == '0' || ch == '1');
    }

    private Tok cadena(int l, int c) {
        avanza();   // la comilla
        StringBuilder sb = new StringBuilder();
        while (true) {
            if (fin() || ver() == '\n') throw new Error("literal de cadena sin cerrar", l, c);
            char ch = avanza();
            if (ch == '"') break;
            if (ch == '\\') escape(sb);
            else sb.append(ch);
        }
        return new Tok(Tipo.CADENA, sb.toString(), sb.toString(), l, c);
    }

    /** `'a'` es un entero: su punto de código (punto 19). */
    private Tok caracter(int l, int c) {
        avanza();   // la comilla
        StringBuilder sb = new StringBuilder();
        while (true) {
            if (fin() || ver() == '\n') throw new Error("carácter sin cerrar", l, c);
            char ch = avanza();
            if (ch == '\'') break;
            if (ch == '\\') escape(sb);
            else sb.append(ch);
        }
        String s = sb.toString();
        if (s.isEmpty() || s.codePointCount(0, s.length()) != 1)
            throw new Error("un carácter entre comillas simples es UNO: '" + s + "'", l, c);
        return new Tok(Tipo.ENTERO, "'" + s + "'", (long) s.codePointAt(0), l, c);
    }

    /** Los escapes de C# (punto 19), tras la barra. */
    private void escape(StringBuilder sb) {
        if (fin()) throw error("escape sin completar al final del fuente");
        char e = avanza();
        switch (e) {
            case 'n':  sb.append('\n'); break;
            case 't':  sb.append('\t'); break;
            case 'r':  sb.append('\r'); break;
            case '0':  sb.append('\0'); break;
            case '\\': sb.append('\\'); break;
            case '"':  sb.append('"'); break;
            case '\'': sb.append('\''); break;
            case 'a':  sb.append((char) 7); break;
            case 'b':  sb.append('\b'); break;
            case 'f':  sb.append('\f'); break;
            case 'v':  sb.append((char) 11); break;
            case 'u':  sb.append((char) hex(4, 4)); break;
            case 'U':  sb.appendCodePoint(hex(8, 8)); break;
            case 'x':  sb.append((char) hex(1, 4)); break;
            default:   throw error("secuencia de escape desconocida: '\\" + e + "'");
        }
    }

    private int hex(int min, int max) {
        int v = 0, n = 0;
        while (n < max && !fin() && Character.digit(ver(), 16) >= 0) {
            v = v * 16 + Character.digit(avanza(), 16);
            n++;
        }
        if (n < min) throw error("faltan cifras hexadecimales en el escape");
        return v;
    }
}
