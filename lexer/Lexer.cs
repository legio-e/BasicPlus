// ============================================================
// Lexer.cs
// Analizador léxico de BASICPLUS.
//
// Convierte un texto fuente en una lista de tokens según la
// gramática EBNF del lenguaje. Características:
//   - Palabras clave CASE-INSENSITIVE.
//   - Identificadores empiezan por letra ASCII y siguen con
//     letras, dígitos o '_'.
//   - Literales: decimal, hex (0x...), binario (0b...),
//     float con exponente (1.5e-3), string con escapes
//     (\n \t \r \\ \" \0).
//   - Comentarios: '//' línea, '/* ... */' bloque (no anidado).
//   - NEWLINE significativo (terminador de sentencia).
//   - Acumula errores en una lista (no lanza excepciones).
// ============================================================

using System.Globalization;
using System.Text;

namespace BasicPlus.Lexer;

public sealed class Lexer
{
    private readonly string _source;
    private readonly List<LexerError> _errors = new();

    private int _pos = 0;
    private int _line = 1;
    private int _column = 1;

    public IReadOnlyList<LexerError> Errors => _errors;

    public Lexer(string source)
    {
        _source = source ?? throw new ArgumentNullException(nameof(source));
    }

    // ---------- Diccionario de palabras clave (case-insensitive) ----------
    private static readonly Dictionary<string, TokenType> Keywords =
        new(StringComparer.OrdinalIgnoreCase)
        {
            // estructura
            { "module",   TokenType.Module },
            { "end",      TokenType.End },
            { "import",   TokenType.Import },
            // declaraciones
            { "const",    TokenType.Const },
            { "var",      TokenType.Var },
            { "function", TokenType.Function },
            { "class",    TokenType.Class },
            { "extends",  TokenType.Extends },
            { "enum",     TokenType.Enum },
            { "property", TokenType.Property },
            { "get",      TokenType.Get },
            { "set",      TokenType.Set },
            { "endprop",  TokenType.EndProp },
            { "endget",   TokenType.EndGet },
            { "endset",   TokenType.EndSet },
            // visibilidad y herencia
            { "public",   TokenType.Public },
            { "final",    TokenType.Final },
            // instancia / clase base
            { "this",     TokenType.This },
            { "super",    TokenType.Super },
            // control de flujo
            { "if",       TokenType.If },
            { "then",     TokenType.Then },
            { "elseif",   TokenType.ElseIf },
            { "else",     TokenType.Else },
            { "endif",    TokenType.EndIf },
            { "switch",   TokenType.Switch },
            { "case",     TokenType.Case },
            { "default",  TokenType.Default },
            { "endsw",    TokenType.EndSw },
            { "while",    TokenType.While },
            { "do",       TokenType.Do },
            { "endwh",    TokenType.EndWh },
            { "loop",     TokenType.Loop },
            { "for",      TokenType.For },
            { "to",       TokenType.To },
            { "step",     TokenType.Step },
            { "next",     TokenType.Next },
            { "in",       TokenType.In },
            { "break",    TokenType.Break },
            { "continue", TokenType.Continue },
            { "return",   TokenType.Return },
            // errores
            { "try",      TokenType.Try },
            { "catch",    TokenType.Catch },
            { "finally",  TokenType.Finally },
            { "endtry",   TokenType.EndTry },
            { "throw",    TokenType.Throw },
            // E/S
            { "print",    TokenType.Print },
            // tipos
            { "integer",  TokenType.Integer },
            { "float",    TokenType.Float },
            { "string",   TokenType.String },
            { "boolean",  TokenType.Boolean },
            // literales
            { "true",     TokenType.True },
            { "false",    TokenType.False },
            { "null",     TokenType.Null },
            // operadores con forma de palabra
            { "and",      TokenType.And },
            { "or",       TokenType.Or },
            { "not",      TokenType.Not },
            { "xor",      TokenType.Xor },
            { "mod",      TokenType.Mod },
            { "shl",      TokenType.Shl },
            { "shr",      TokenType.Shr },
            // contextual
            { "field",    TokenType.Field },
        };

    // ============================================================
    // Punto de entrada
    // ============================================================
    public List<Token> Tokenize()
    {
        var tokens = new List<Token>();

        while (true)
        {
            SkipInlineWhitespaceAndComments();
            if (IsAtEnd()) break;

            int startLine = _line;
            int startColumn = _column;
            char c = Peek();

            // Newline significativo
            if (c == '\r' || c == '\n')
            {
                tokens.Add(ScanNewline(startLine, startColumn));
                continue;
            }

            // Identificador / palabra clave
            if (IsAlpha(c))
            {
                tokens.Add(ScanIdentifierOrKeyword(startLine, startColumn));
                continue;
            }

            // Número
            if (IsDigit(c))
            {
                tokens.Add(ScanNumber(startLine, startColumn));
                continue;
            }

            // String
            if (c == '"')
            {
                tokens.Add(ScanString(startLine, startColumn));
                continue;
            }

            // Operadores y signos
            Token? op = ScanOperator(startLine, startColumn);
            if (op is not null)
            {
                tokens.Add(op);
                continue;
            }

            // Carácter desconocido: error y avanzar para no quedar atascados
            _errors.Add(new LexerError($"carácter inesperado: '{c}' (U+{(int)c:X4})", startLine, startColumn));
            Advance();
        }

        tokens.Add(new Token(TokenType.Eof, string.Empty, null, _line, _column));
        return tokens;
    }

    // ============================================================
    // Manejo de cursor
    // ============================================================
    private bool IsAtEnd() => _pos >= _source.Length;

    /// <summary>Devuelve el carácter en el offset indicado sin consumirlo.</summary>
    private char Peek(int offset = 0) =>
        _pos + offset < _source.Length ? _source[_pos + offset] : '\0';

    /// <summary>Consume un carácter no-newline (solo avanza columna).</summary>
    private void Advance()
    {
        if (IsAtEnd()) return;
        _pos++;
        _column++;
    }

    /// <summary>Consume \n o \r\n actualizando línea y columna.</summary>
    private void ConsumeNewline()
    {
        if (IsAtEnd()) return;
        if (Peek() == '\r' && Peek(1) == '\n')
        {
            _pos += 2;
        }
        else
        {
            _pos++;
        }
        _line++;
        _column = 1;
    }

    // ============================================================
    // Espacios y comentarios
    // ============================================================
    private void SkipInlineWhitespaceAndComments()
    {
        while (!IsAtEnd())
        {
            char c = Peek();
            if (c == ' ' || c == '\t')
            {
                Advance();
            }
            else if (c == '/' && Peek(1) == '/')
            {
                // Comentario de línea: hasta el final de la línea (sin consumir el NEWLINE).
                Advance(); Advance();
                while (!IsAtEnd() && Peek() != '\n' && Peek() != '\r')
                    Advance();
            }
            else if (c == '/' && Peek(1) == '*')
            {
                // Comentario de bloque: /* ... */ (no anidado).
                int startLine = _line;
                int startColumn = _column;
                Advance(); Advance(); // consume '/*'
                while (!IsAtEnd())
                {
                    if (Peek() == '*' && Peek(1) == '/')
                    {
                        Advance(); Advance();
                        break;
                    }
                    if (Peek() == '\r' || Peek() == '\n')
                        ConsumeNewline();
                    else
                        Advance();
                }
                if (IsAtEnd())
                {
                    _errors.Add(new LexerError("comentario de bloque sin cerrar (esperaba '*/')", startLine, startColumn));
                }
            }
            else
            {
                break;
            }
        }
    }

    // ============================================================
    // Newlines
    // ============================================================
    private Token ScanNewline(int line, int column)
    {
        string lexeme;
        if (Peek() == '\r' && Peek(1) == '\n')
        {
            lexeme = "\r\n";
        }
        else
        {
            lexeme = Peek().ToString();
        }
        ConsumeNewline();
        return new Token(TokenType.Newline, lexeme, null, line, column);
    }

    // ============================================================
    // Identificadores y palabras clave
    // ============================================================
    private Token ScanIdentifierOrKeyword(int line, int column)
    {
        int start = _pos;
        while (!IsAtEnd() && IsAlphaNumericOrUnderscore(Peek()))
            Advance();

        string lexeme = _source.Substring(start, _pos - start);

        // Palabras clave: case-insensitive.
        if (Keywords.TryGetValue(lexeme, out TokenType kwType))
        {
            // Para keywords no guardamos valor — el tipo ya lo identifica.
            return new Token(kwType, lexeme, null, line, column);
        }

        // Identificador ordinario: case-sensitive (preservamos la caja).
        return new Token(TokenType.Identifier, lexeme, lexeme, line, column);
    }

    // ============================================================
    // Números: decimal, hex, binario, float con exponente
    // ============================================================
    private Token ScanNumber(int line, int column)
    {
        int start = _pos;

        // Hex: 0x... | 0X...
        if (Peek() == '0' && (Peek(1) == 'x' || Peek(1) == 'X'))
        {
            Advance(); Advance(); // consume 0x
            int hexStart = _pos;
            while (!IsAtEnd() && IsHexDigit(Peek())) Advance();
            string hexLex = _source.Substring(start, _pos - start);
            if (_pos == hexStart)
            {
                _errors.Add(new LexerError("literal hexadecimal vacío después de '0x'", line, column));
                return new Token(TokenType.IntegerLit, hexLex, 0L, line, column);
            }
            try
            {
                long val = Convert.ToInt64(hexLex.Substring(2), 16);
                return new Token(TokenType.IntegerLit, hexLex, val, line, column);
            }
            catch (OverflowException)
            {
                _errors.Add(new LexerError($"literal hexadecimal demasiado grande: {hexLex}", line, column));
                return new Token(TokenType.IntegerLit, hexLex, 0L, line, column);
            }
        }

        // Bin: 0b... | 0B...
        if (Peek() == '0' && (Peek(1) == 'b' || Peek(1) == 'B'))
        {
            Advance(); Advance(); // consume 0b
            int binStart = _pos;
            while (!IsAtEnd() && IsBinDigit(Peek())) Advance();
            string binLex = _source.Substring(start, _pos - start);
            if (_pos == binStart)
            {
                _errors.Add(new LexerError("literal binario vacío después de '0b'", line, column));
                return new Token(TokenType.IntegerLit, binLex, 0L, line, column);
            }
            try
            {
                long val = Convert.ToInt64(binLex.Substring(2), 2);
                return new Token(TokenType.IntegerLit, binLex, val, line, column);
            }
            catch (OverflowException)
            {
                _errors.Add(new LexerError($"literal binario demasiado grande: {binLex}", line, column));
                return new Token(TokenType.IntegerLit, binLex, 0L, line, column);
            }
        }

        // Decimal — parte entera
        while (!IsAtEnd() && IsDigit(Peek())) Advance();

        bool isFloat = false;

        // Parte fraccionaria: solo si después del '.' hay otro dígito.
        // (Si vemos algo como '0.', no consumimos el '.' — podría ser
        //  acceso a miembro: 'arr.length' u otra construcción.)
        if (Peek() == '.' && IsDigit(Peek(1)))
        {
            isFloat = true;
            Advance(); // '.'
            while (!IsAtEnd() && IsDigit(Peek())) Advance();
        }

        // Exponente opcional: e[+-]?digit+
        if (Peek() == 'e' || Peek() == 'E')
        {
            isFloat = true;
            Advance(); // 'e'
            if (Peek() == '+' || Peek() == '-') Advance();
            int expStart = _pos;
            while (!IsAtEnd() && IsDigit(Peek())) Advance();
            if (_pos == expStart)
            {
                _errors.Add(new LexerError("exponente sin dígitos en literal float", line, column));
            }
        }

        string lex = _source.Substring(start, _pos - start);
        if (isFloat)
        {
            if (double.TryParse(lex, NumberStyles.Float, CultureInfo.InvariantCulture, out double d))
                return new Token(TokenType.FloatLit, lex, d, line, column);
            _errors.Add(new LexerError($"literal float inválido: {lex}", line, column));
            return new Token(TokenType.FloatLit, lex, 0.0, line, column);
        }
        else
        {
            if (long.TryParse(lex, NumberStyles.Integer, CultureInfo.InvariantCulture, out long n))
                return new Token(TokenType.IntegerLit, lex, n, line, column);
            _errors.Add(new LexerError($"literal entero inválido o fuera de rango: {lex}", line, column));
            return new Token(TokenType.IntegerLit, lex, 0L, line, column);
        }
    }

    // ============================================================
    // String: "...." con escapes \n \t \r \\ \" \0
    // No puede contener NEWLINE sin escapar.
    // ============================================================
    private Token ScanString(int line, int column)
    {
        int startPos = _pos;
        Advance(); // consume comilla de apertura

        var sb = new StringBuilder();
        bool terminated = false;

        while (!IsAtEnd())
        {
            char c = Peek();

            if (c == '"')
            {
                Advance();
                terminated = true;
                break;
            }
            if (c == '\n' || c == '\r')
            {
                _errors.Add(new LexerError("salto de línea dentro de literal de cadena (se esperaba '\"')", _line, _column));
                break; // no consumimos el NEWLINE: lo tokeniza la pasada principal.
            }
            if (c == '\\')
            {
                Advance();
                if (IsAtEnd())
                {
                    _errors.Add(new LexerError("escape sin completar al final del fuente", _line, _column));
                    break;
                }
                char esc = Peek();
                switch (esc)
                {
                    case 'n':  sb.Append('\n'); Advance(); break;
                    case 't':  sb.Append('\t'); Advance(); break;
                    case 'r':  sb.Append('\r'); Advance(); break;
                    case '\\': sb.Append('\\'); Advance(); break;
                    case '"':  sb.Append('"');  Advance(); break;
                    case '0':  sb.Append('\0'); Advance(); break;
                    default:
                        _errors.Add(new LexerError($"secuencia de escape desconocida: '\\{esc}'", _line, _column));
                        Advance();
                        break;
                }
            }
            else
            {
                sb.Append(c);
                Advance();
            }
        }

        if (!terminated)
        {
            _errors.Add(new LexerError("literal de cadena no terminado", line, column));
        }

        string lexeme = _source.Substring(startPos, _pos - startPos);
        return new Token(TokenType.StringLit, lexeme, sb.ToString(), line, column);
    }

    // ============================================================
    // Operadores y signos de puntuación
    // ============================================================
    private Token? ScanOperator(int line, int column)
    {
        char c = Peek();
        char n = Peek(1);

        // Operadores de dos caracteres primero.
        switch (c)
        {
            case ':' when n == '=': return Make2(TokenType.Assign, ":=", line, column);
            case '+' when n == '=': return Make2(TokenType.PlusAssign, "+=", line, column);
            case '-' when n == '=': return Make2(TokenType.MinusAssign, "-=", line, column);
            case '=' when n == '=': return Make2(TokenType.Eq, "==", line, column);
            case '!' when n == '=': return Make2(TokenType.Neq, "!=", line, column);
            case '<' when n == '=': return Make2(TokenType.Le, "<=", line, column);
            case '>' when n == '=': return Make2(TokenType.Ge, ">=", line, column);
        }

        // Operadores de un carácter.
        switch (c)
        {
            case '+': return Make1(TokenType.Plus,      "+", line, column);
            case '-': return Make1(TokenType.Minus,     "-", line, column);
            case '*': return Make1(TokenType.Star,      "*", line, column);
            case '/': return Make1(TokenType.Slash,     "/", line, column);
            case '|': return Make1(TokenType.Bar,       "|", line, column);
            case '&': return Make1(TokenType.Amp,       "&", line, column);
            case '<': return Make1(TokenType.Lt,        "<", line, column);
            case '>': return Make1(TokenType.Gt,        ">", line, column);
            case '(': return Make1(TokenType.LParen,    "(", line, column);
            case ')': return Make1(TokenType.RParen,    ")", line, column);
            case '[': return Make1(TokenType.LBracket,  "[", line, column);
            case ']': return Make1(TokenType.RBracket,  "]", line, column);
            case ',': return Make1(TokenType.Comma,     ",", line, column);
            case ';': return Make1(TokenType.Semicolon, ";", line, column);
            case ':': return Make1(TokenType.Colon,     ":", line, column);
            case '.': return Make1(TokenType.Dot,       ".", line, column);
        }

        return null;
    }

    private Token Make1(TokenType t, string lexeme, int line, int column)
    {
        Advance();
        return new Token(t, lexeme, null, line, column);
    }

    private Token Make2(TokenType t, string lexeme, int line, int column)
    {
        Advance(); Advance();
        return new Token(t, lexeme, null, line, column);
    }

    // ============================================================
    // Predicados léxicos
    // ============================================================
    private static bool IsAlpha(char c) =>
        (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');

    private static bool IsDigit(char c) =>
        c >= '0' && c <= '9';

    private static bool IsAlphaNumericOrUnderscore(char c) =>
        IsAlpha(c) || IsDigit(c) || c == '_';

    private static bool IsHexDigit(char c) =>
        IsDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');

    private static bool IsBinDigit(char c) =>
        c == '0' || c == '1';
}
