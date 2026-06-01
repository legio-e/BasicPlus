// ============================================================
// Lexer.java
// Analizador léxico de BASICPLUS — port a Java del Lexer en C#.
//
// Características:
//   - Palabras clave CASE-INSENSITIVE.
//   - Identificadores: empiezan por letra ASCII y siguen con
//     letras, dígitos o '_'.
//   - Literales:
//       * decimal:  42
//       * hex:      0xFF, 0XFF
//       * binario:  0b1010, 0B1010
//       * float con exponente:  1.5e-3, 2.0E+10
//   - Strings con escapes: \n \t \r \\ \" \0
//   - Comentarios: '//' línea, '/* ... */' bloque (no anidado).
//   - NEWLINE significativo (terminador de sentencias).
//   - Acumula errores en una lista (no lanza excepciones).
// ============================================================
package basicplus.frontend;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public final class Lexer {

    private final String source;
    private final List<LexerError> errors = new ArrayList<>();

    private int pos = 0;
    private int line = 1;
    private int column = 1;

    public Lexer(String source) {
        if (source == null) throw new IllegalArgumentException("source no puede ser null");
        this.source = source;
    }

    public List<LexerError> getErrors() {
        return errors;
    }

    // ---------- Diccionario de palabras clave (case-insensitive) ----------
    // Las claves se guardan SIEMPRE en minúsculas; al consultar normalizamos
    // el lexema con toLowerCase().
    private static final Map<String, TokenType> KEYWORDS = buildKeywords();

    private static Map<String, TokenType> buildKeywords() {
        Map<String, TokenType> m = new HashMap<>();
        // estructura
        m.put("module",   TokenType.MODULE);
        m.put("library",   TokenType.LIBRARY);
        m.put("from",      TokenType.FROM);
        m.put("end",       TokenType.END);
        m.put("import",    TokenType.IMPORT);
        m.put("interface", TokenType.INTERFACE);
        m.put("implements",TokenType.IMPLEMENTS);
        // declaraciones
        m.put("const",    TokenType.CONST);
        m.put("var",      TokenType.VAR);
        m.put("function", TokenType.FUNCTION);
        m.put("class",    TokenType.CLASS);
        m.put("extends",  TokenType.EXTENDS);
        m.put("enum",     TokenType.ENUM);
        m.put("property", TokenType.PROPERTY);
        m.put("get",      TokenType.GET);
        m.put("set",      TokenType.SET);
        m.put("endprop",  TokenType.ENDPROP);
        m.put("endget",   TokenType.ENDGET);
        m.put("endset",   TokenType.ENDSET);
        // visibilidad y herencia
        m.put("public",    TokenType.PUBLIC);
        m.put("final",     TokenType.FINAL);
        m.put("sync",      TokenType.SYNC);
        m.put("intrinsic", TokenType.INTRINSIC);
        m.put("native",    TokenType.NATIVE);   /* H3 #162 — modificador AOT */
        // ownership
        m.put("owner",    TokenType.OWNER);
        // instancia / clase base
        m.put("this",     TokenType.THIS);
        m.put("super",    TokenType.SUPER);
        // control de flujo
        m.put("if",       TokenType.IF);
        m.put("then",     TokenType.THEN);
        m.put("elseif",   TokenType.ELSEIF);
        m.put("else",     TokenType.ELSE);
        m.put("endif",    TokenType.ENDIF);
        m.put("switch",   TokenType.SWITCH);
        m.put("case",     TokenType.CASE);
        m.put("default",  TokenType.DEFAULT);
        m.put("endsw",    TokenType.ENDSW);
        m.put("while",    TokenType.WHILE);
        m.put("do",       TokenType.DO);
        m.put("endwh",    TokenType.ENDWH);
        m.put("loop",     TokenType.LOOP);
        m.put("for",      TokenType.FOR);
        m.put("to",       TokenType.TO);
        m.put("step",     TokenType.STEP);
        m.put("next",     TokenType.NEXT);
        m.put("in",       TokenType.IN);
        m.put("break",    TokenType.BREAK);
        m.put("continue", TokenType.CONTINUE);
        m.put("return",   TokenType.RETURN);
        m.put("parallel", TokenType.PARALLEL);
        m.put("endpar",   TokenType.ENDPAR);
        // errores
        m.put("try",      TokenType.TRY);
        m.put("catch",    TokenType.CATCH);
        m.put("finally",  TokenType.FINALLY);
        m.put("endtry",   TokenType.ENDTRY);
        m.put("throw",    TokenType.THROW);
        // E/S
        m.put("print",    TokenType.PRINT);
        // tipos escalares: NO son palabras reservadas (H5 / 2026-06-01).
        // Las keywords son case-insensitive, así que reservar `integer`
        // bloqueaba `Integer` como nombre de clase (los envoltorios boxed
        // estilo Java: Integer/Long/Float/Double/Boolean). Solución: que
        // `integer`/`float`/... sean IDENTIFICADORES normales. Sigue todo
        // funcionando porque:
        //   - tipo `var x: integer` → parseType rama IDENTIFIER →
        //     SimpleTypeRef("integer") → resolveType lo mapea a PrimitiveType.
        //   - cast `integer(x)` → rama IDENTIFIER → CallExpr(Identifier
        //     "integer") = MISMO AST que el cast keyword → el semántico/
        //     emisor lo reconocen por nombre.
        //   - `Integer` es un identificador DISTINTO (case-sensitive) →
        //     queda libre como nombre de clase.
        // (Los tipos estrechos byte/int8/word/int16/short siguen reservados
        //  de momento; se pueden liberar igual si se necesitan como clase.)
        // tipos enteros estrechos (L10)
        m.put("byte",     TokenType.BYTE);
        m.put("int8",     TokenType.INT8);
        m.put("word",     TokenType.WORD);
        m.put("int16",    TokenType.INT16);
        m.put("short",    TokenType.SHORT);
        // literales
        m.put("true",     TokenType.TRUE);
        m.put("false",    TokenType.FALSE);
        m.put("null",     TokenType.NULL);
        // operadores con forma de palabra
        m.put("and",        TokenType.AND);
        m.put("or",         TokenType.OR);
        m.put("not",        TokenType.NOT);
        m.put("xor",        TokenType.XOR);
        m.put("mod",        TokenType.MOD);
        m.put("shl",        TokenType.SHL);
        m.put("shr",        TokenType.SHR);
        m.put("instanceof", TokenType.INSTANCEOF);
        // contextual
        m.put("field",    TokenType.FIELD);
        return Collections.unmodifiableMap(m);
    }

    // ============================================================
    // PUNTO DE ENTRADA
    // ============================================================
    public List<Token> tokenize() {
        List<Token> tokens = new ArrayList<>();

        while (true) {
            skipInlineWhitespaceAndComments();
            if (isAtEnd()) break;

            int startLine = line;
            int startColumn = column;
            char c = peek();

            // Newline significativo
            if (c == '\r' || c == '\n') {
                tokens.add(scanNewline(startLine, startColumn));
                continue;
            }

            // Identificador / palabra clave
            if (isAlpha(c)) {
                tokens.add(scanIdentifierOrKeyword(startLine, startColumn));
                continue;
            }

            // Número
            if (isDigit(c)) {
                tokens.add(scanNumber(startLine, startColumn));
                continue;
            }

            // String
            if (c == '"') {
                tokens.add(scanString(startLine, startColumn));
                continue;
            }

            // Operadores y signos
            Token op = scanOperator(startLine, startColumn);
            if (op != null) {
                tokens.add(op);
                continue;
            }

            // Carácter desconocido
            errors.add(new LexerError(
                    String.format("carácter inesperado: '%s' (U+%04X)", c, (int) c),
                    startLine, startColumn));
            advance();
        }

        tokens.add(new Token(TokenType.EOF, "", null, line, column));
        return tokens;
    }

    // ============================================================
    // Manejo de cursor
    // ============================================================
    private boolean isAtEnd() {
        return pos >= source.length();
    }

    private char peek() {
        return peek(0);
    }

    private char peek(int offset) {
        return (pos + offset < source.length()) ? source.charAt(pos + offset) : '\0';
    }

    /** Consume un carácter no-newline (solo avanza columna). */
    private void advance() {
        if (isAtEnd()) return;
        pos++;
        column++;
    }

    /** Consume \n o \r\n actualizando línea y columna. */
    private void consumeNewline() {
        if (isAtEnd()) return;
        if (peek() == '\r' && peek(1) == '\n') {
            pos += 2;
        } else {
            pos++;
        }
        line++;
        column = 1;
    }

    // ============================================================
    // Espacios y comentarios
    // ============================================================
    private void skipInlineWhitespaceAndComments() {
        while (!isAtEnd()) {
            char c = peek();
            if (c == ' ' || c == '\t') {
                advance();
            } else if (c == '/' && peek(1) == '/') {
                // Comentario de línea: hasta el final de la línea (sin consumir el NEWLINE).
                advance(); advance();
                while (!isAtEnd() && peek() != '\n' && peek() != '\r') advance();
            } else if (c == '/' && peek(1) == '*') {
                // Comentario de bloque: /* ... */ (no anidado).
                int startLine = line;
                int startColumn = column;
                advance(); advance(); // consume '/*'
                while (!isAtEnd()) {
                    if (peek() == '*' && peek(1) == '/') {
                        advance(); advance();
                        break;
                    }
                    if (peek() == '\r' || peek() == '\n') {
                        consumeNewline();
                    } else {
                        advance();
                    }
                }
                if (isAtEnd()) {
                    errors.add(new LexerError(
                            "comentario de bloque sin cerrar (esperaba '*/')",
                            startLine, startColumn));
                }
            } else {
                break;
            }
        }
    }

    // ============================================================
    // Newlines
    // ============================================================
    private Token scanNewline(int startLine, int startColumn) {
        String lexeme;
        if (peek() == '\r' && peek(1) == '\n') {
            lexeme = "\r\n";
        } else {
            lexeme = String.valueOf(peek());
        }
        consumeNewline();
        return new Token(TokenType.NEWLINE, lexeme, null, startLine, startColumn);
    }

    // ============================================================
    // Identificadores y palabras clave
    // ============================================================
    private Token scanIdentifierOrKeyword(int startLine, int startColumn) {
        int start = pos;
        while (!isAtEnd() && isAlphaNumericOrUnderscore(peek())) advance();
        String lexeme = source.substring(start, pos);
        TokenType kw = KEYWORDS.get(lexeme.toLowerCase());
        if (kw != null) {
            // Para keywords no guardamos valor: el tipo ya lo identifica.
            return new Token(kw, lexeme, null, startLine, startColumn);
        }
        // Identificador normal: valor = el propio nombre (case-sensitive).
        return new Token(TokenType.IDENTIFIER, lexeme, lexeme, startLine, startColumn);
    }

    // ============================================================
    // Números: decimal, hex, binario, float con exponente
    // ============================================================
    private Token scanNumber(int startLine, int startColumn) {
        int start = pos;

        // Hex: 0x... | 0X...
        if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
            advance(); advance(); // consume 0x
            int hexStart = pos;
            while (!isAtEnd() && isHexDigit(peek())) advance();
            String hexLex = source.substring(start, pos);
            if (pos == hexStart) {
                errors.add(new LexerError("literal hexadecimal vacío después de '0x'", startLine, startColumn));
                return new Token(TokenType.INTEGER_LIT, hexLex, 0L, startLine, startColumn);
            }
            try {
                long val = Long.parseLong(hexLex.substring(2), 16);
                return new Token(TokenType.INTEGER_LIT, hexLex, val, startLine, startColumn);
            } catch (NumberFormatException nfe) {
                errors.add(new LexerError("literal hexadecimal inválido o fuera de rango: " + hexLex,
                        startLine, startColumn));
                return new Token(TokenType.INTEGER_LIT, hexLex, 0L, startLine, startColumn);
            }
        }

        // Bin: 0b... | 0B...
        if (peek() == '0' && (peek(1) == 'b' || peek(1) == 'B')) {
            advance(); advance(); // consume 0b
            int binStart = pos;
            while (!isAtEnd() && isBinDigit(peek())) advance();
            String binLex = source.substring(start, pos);
            if (pos == binStart) {
                errors.add(new LexerError("literal binario vacío después de '0b'", startLine, startColumn));
                return new Token(TokenType.INTEGER_LIT, binLex, 0L, startLine, startColumn);
            }
            try {
                long val = Long.parseLong(binLex.substring(2), 2);
                return new Token(TokenType.INTEGER_LIT, binLex, val, startLine, startColumn);
            } catch (NumberFormatException nfe) {
                errors.add(new LexerError("literal binario inválido o fuera de rango: " + binLex,
                        startLine, startColumn));
                return new Token(TokenType.INTEGER_LIT, binLex, 0L, startLine, startColumn);
            }
        }

        // Decimal — parte entera
        while (!isAtEnd() && isDigit(peek())) advance();

        boolean isFloat = false;

        // Parte fraccionaria: solo si tras '.' hay otro dígito
        // (para no consumir '.' en accesos a miembro como 'arr.length').
        if (peek() == '.' && isDigit(peek(1))) {
            isFloat = true;
            advance(); // '.'
            while (!isAtEnd() && isDigit(peek())) advance();
        }

        // Exponente opcional: e[+-]?digit+
        if (peek() == 'e' || peek() == 'E') {
            isFloat = true;
            advance(); // 'e'
            if (peek() == '+' || peek() == '-') advance();
            int expStart = pos;
            while (!isAtEnd() && isDigit(peek())) advance();
            if (pos == expStart) {
                errors.add(new LexerError("exponente sin dígitos en literal float",
                        startLine, startColumn));
            }
        }

        String lex = source.substring(start, pos);
        // H1.3 (V2): sufijo d/D → literal double (f64). Aplica a entero o float
        // (5d, 1.5d, 2e3d). Sin sufijo, un literal con punto/exponente es float (f32).
        if (peek() == 'd' || peek() == 'D') {
            advance();
            try {
                double dv = Double.parseDouble(lex);
                return new Token(TokenType.DOUBLE_LIT, lex, dv, startLine, startColumn);
            } catch (NumberFormatException nfe) {
                errors.add(new LexerError("literal double inválido: " + lex, startLine, startColumn));
                return new Token(TokenType.DOUBLE_LIT, lex, 0.0, startLine, startColumn);
            }
        }
        if (isFloat) {
            try {
                double d = Double.parseDouble(lex);
                return new Token(TokenType.FLOAT_LIT, lex, d, startLine, startColumn);
            } catch (NumberFormatException nfe) {
                errors.add(new LexerError("literal float inválido: " + lex, startLine, startColumn));
                return new Token(TokenType.FLOAT_LIT, lex, 0.0, startLine, startColumn);
            }
        } else {
            // H1.2 (V2): sufijo L/l tras un entero decimal → literal long (i64).
            boolean isLong = false;
            if (peek() == 'L' || peek() == 'l') { advance(); isLong = true; }
            TokenType ity = isLong ? TokenType.LONG_LIT : TokenType.INTEGER_LIT;
            try {
                long n = Long.parseLong(lex);
                return new Token(ity, lex, n, startLine, startColumn);
            } catch (NumberFormatException nfe) {
                errors.add(new LexerError("literal entero inválido o fuera de rango: " + lex,
                        startLine, startColumn));
                return new Token(ity, lex, 0L, startLine, startColumn);
            }
        }
    }

    // ============================================================
    // String: "..." con escapes \n \t \r \\ \" \0
    // No puede contener NEWLINE sin escapar.
    // ============================================================
    private Token scanString(int startLine, int startColumn) {
        int startPos = pos;
        advance(); // consume comilla de apertura

        StringBuilder sb = new StringBuilder();
        boolean terminated = false;

        while (!isAtEnd()) {
            char c = peek();

            if (c == '"') {
                advance();
                terminated = true;
                break;
            }
            if (c == '\n' || c == '\r') {
                errors.add(new LexerError(
                        "salto de línea dentro de literal de cadena (se esperaba '\"')",
                        line, column));
                break; // no consumimos el NEWLINE: lo tokeniza la pasada principal.
            }
            if (c == '\\') {
                advance();
                if (isAtEnd()) {
                    errors.add(new LexerError(
                            "escape sin completar al final del fuente", line, column));
                    break;
                }
                char esc = peek();
                switch (esc) {
                    case 'n':  sb.append('\n'); advance(); break;
                    case 't':  sb.append('\t'); advance(); break;
                    case 'r':  sb.append('\r'); advance(); break;
                    case '\\': sb.append('\\'); advance(); break;
                    case '"':  sb.append('"');  advance(); break;
                    case '0':  sb.append('\0'); advance(); break;
                    default:
                        errors.add(new LexerError(
                                "secuencia de escape desconocida: '\\" + esc + "'",
                                line, column));
                        advance();
                        break;
                }
            } else {
                sb.append(c);
                advance();
            }
        }

        if (!terminated) {
            errors.add(new LexerError("literal de cadena no terminado", startLine, startColumn));
        }

        String lexeme = source.substring(startPos, pos);
        return new Token(TokenType.STRING_LIT, lexeme, sb.toString(), startLine, startColumn);
    }

    // ============================================================
    // Operadores y signos de puntuación
    // ============================================================
    private Token scanOperator(int startLine, int startColumn) {
        char c = peek();
        char n = peek(1);

        // Operadores de dos caracteres primero.
        if (c == ':' && n == '=') return make2(TokenType.ASSIGN,        ":=", startLine, startColumn);
        if (c == '+' && n == '=') return make2(TokenType.PLUS_ASSIGN,   "+=", startLine, startColumn);
        if (c == '-' && n == '=') return make2(TokenType.MINUS_ASSIGN,  "-=", startLine, startColumn);
        if (c == '=' && n == '=') return make2(TokenType.EQ,            "==", startLine, startColumn);
        if (c == '!' && n == '=') return make2(TokenType.NEQ,           "!=", startLine, startColumn);
        if (c == '<' && n == '=') return make2(TokenType.LE,            "<=", startLine, startColumn);
        if (c == '>' && n == '=') return make2(TokenType.GE,            ">=", startLine, startColumn);

        // Operadores de un carácter.
        switch (c) {
            case '+': return make1(TokenType.PLUS,      "+", startLine, startColumn);
            case '-': return make1(TokenType.MINUS,     "-", startLine, startColumn);
            case '*': return make1(TokenType.STAR,      "*", startLine, startColumn);
            case '/': return make1(TokenType.SLASH,     "/", startLine, startColumn);
            case '|': return make1(TokenType.BAR,       "|", startLine, startColumn);
            case '&': return make1(TokenType.AMP,       "&", startLine, startColumn);
            case '<': return make1(TokenType.LT,        "<", startLine, startColumn);
            case '>': return make1(TokenType.GT,        ">", startLine, startColumn);
            case '(': return make1(TokenType.LPAREN,    "(", startLine, startColumn);
            case ')': return make1(TokenType.RPAREN,    ")", startLine, startColumn);
            case '[': return make1(TokenType.LBRACKET,  "[", startLine, startColumn);
            case ']': return make1(TokenType.RBRACKET,  "]", startLine, startColumn);
            case ',': return make1(TokenType.COMMA,     ",", startLine, startColumn);
            case ';': return make1(TokenType.SEMICOLON, ";", startLine, startColumn);
            case ':': return make1(TokenType.COLON,     ":", startLine, startColumn);
            case '.': return make1(TokenType.DOT,       ".", startLine, startColumn);
        }
        return null;
    }

    private Token make1(TokenType t, String lex, int startLine, int startColumn) {
        advance();
        return new Token(t, lex, null, startLine, startColumn);
    }

    private Token make2(TokenType t, String lex, int startLine, int startColumn) {
        advance(); advance();
        return new Token(t, lex, null, startLine, startColumn);
    }

    // ============================================================
    // Predicados léxicos
    // ============================================================
    private static boolean isAlpha(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    private static boolean isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    private static boolean isAlphaNumericOrUnderscore(char c) {
        return isAlpha(c) || isDigit(c) || c == '_';
    }

    private static boolean isHexDigit(char c) {
        return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    private static boolean isBinDigit(char c) {
        return c == '0' || c == '1';
    }
}
