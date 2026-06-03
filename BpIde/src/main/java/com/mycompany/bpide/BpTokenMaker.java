/*
 * BpTokenMaker — TokenMaker de RSyntaxTextArea para BasicPlus.
 *
 * Resaltado por lexer (no por regex sobre todo el texto como el viejo
 * BpSyntaxHighlighter). Distingue: comentarios `//`, strings "...", números,
 * identificadores (con lookup de keyword/tipo/literal) y separadores.
 *
 * El set de keywords/tipos se mantiene en sync con basicplus.frontend.Lexer.
 * NOTA (IDE-6): incluye `native` y los tipos de V2 (long/double/byte/word/
 * short/int8/int16) que el resaltado viejo no tenía.
 *
 * Se registra en FrmMain vía:
 *   AbstractTokenMakerFactory atmf =
 *       (AbstractTokenMakerFactory) TokenMakerFactory.getDefaultInstance();
 *   atmf.putMapping("text/bp", "com.mycompany.bpide.BpTokenMaker");
 * y el editor usa  setSyntaxEditingStyle("text/bp").
 */
package com.mycompany.bpide;

import javax.swing.text.Segment;

import org.fife.ui.rsyntaxtextarea.AbstractTokenMaker;
import org.fife.ui.rsyntaxtextarea.RSyntaxUtilities;
import org.fife.ui.rsyntaxtextarea.Token;
import org.fife.ui.rsyntaxtextarea.TokenMap;
import org.fife.ui.rsyntaxtextarea.TokenTypes;

public class BpTokenMaker extends AbstractTokenMaker {

    /** Estado del scanner (no son campos de la base en esta versión de RSTA). */
    private int currentTokenStart;
    private int currentTokenType;

    /** Palabras reservadas, tipos y literales booleanos de BP. Sync con Lexer. */
    @Override
    public TokenMap getWordsToHighlight() {
        TokenMap tm = new TokenMap();
        final int KW = TokenTypes.RESERVED_WORD;
        final int DT = TokenTypes.DATA_TYPE;
        final int LB = TokenTypes.LITERAL_BOOLEAN;

        String[] keywords = {
            // estructura / módulos
            "module", "library", "from", "end", "import", "interface", "implements",
            // declaraciones
            "const", "var", "function", "class", "extends", "enum",
            "property", "get", "set", "endprop", "endget", "endset",
            // visibilidad / modificadores / ownership / AOT
            "public", "final", "sync", "intrinsic", "native", "owner",
            // instancia / base
            "this", "super", "field",
            // control de flujo
            "if", "then", "elseif", "else", "endif",
            "switch", "case", "default", "endsw",
            "while", "do", "endwh", "loop",
            "for", "to", "step", "next", "in",
            "break", "continue", "return", "parallel", "endpar",
            // excepciones
            "try", "catch", "finally", "endtry", "throw",
            // E/S
            "print",
            // operadores en forma de palabra
            "and", "or", "not", "xor", "mod", "shl", "shr", "instanceof",
            // literal nulo
            "null"
        };
        for (String k : keywords) tm.put(k, KW);

        String[] types = {
            "integer", "float", "string", "boolean",
            "long", "double", "byte", "word", "short", "int8", "int16",
            "any", "Object", "void"
        };
        for (String t : types) tm.put(t, DT);

        tm.put("true", LB);
        tm.put("false", LB);
        return tm;
    }

    /** Un identificador que matchee una keyword/tipo se recolorea. */
    @Override
    public void addToken(Segment segment, int start, int end, int tokenType, int startOffset) {
        if (tokenType == TokenTypes.IDENTIFIER) {
            int value = wordsToHighlight.get(segment, start, end);
            if (value != -1) {
                tokenType = value;
            }
        }
        super.addToken(segment, start, end, tokenType, startOffset);
    }

    /** Comentario de línea `//` (para toggle-comment del editor). */
    @Override
    public String[] getLineCommentStartAndEnd(int languageIndex) {
        return new String[] { "//", null };
    }

    @Override
    public boolean getMarkOccurrencesOfTokenType(int type) {
        return type == TokenTypes.IDENTIFIER;
    }

    /** Empieza un token nuevo en `i` según el carácter `c` (transición desde NULL). */
    private void startToken(Segment text, int i, int end, char c, int newStartOffset) {
        currentTokenStart = i;
        if (c == ' ' || c == '\t') {
            currentTokenType = TokenTypes.WHITESPACE;
        } else if (c == '"') {
            currentTokenType = TokenTypes.LITERAL_STRING_DOUBLE_QUOTE;
        } else if (c == '/' && i < end - 1 && text.array[i + 1] == '/') {
            currentTokenType = TokenTypes.COMMENT_EOL;
        } else if (RSyntaxUtilities.isDigit(c)) {
            currentTokenType = TokenTypes.LITERAL_NUMBER_DECIMAL_INT;
        } else if (Character.isJavaIdentifierStart(c)) {
            currentTokenType = TokenTypes.IDENTIFIER;
        } else {
            // Separador / operador de 1 carácter — token plano.
            addToken(text, i, i, TokenTypes.IDENTIFIER, newStartOffset + i);
            currentTokenType = TokenTypes.NULL;
        }
    }

    @Override
    public Token getTokenList(Segment text, int startTokenType, int startOffset) {
        resetTokenList();

        char[] array = text.array;
        int offset = text.offset;
        int count = text.count;
        int end = offset + count;
        int newStartOffset = startOffset - offset;

        currentTokenStart = offset;
        currentTokenType = startTokenType;

        for (int i = offset; i < end; i++) {
            char c = array[i];
            switch (currentTokenType) {

                case TokenTypes.NULL:
                    startToken(text, i, end, c, newStartOffset);
                    break;

                case TokenTypes.WHITESPACE:
                    if (c != ' ' && c != '\t') {
                        addToken(text, currentTokenStart, i - 1, TokenTypes.WHITESPACE,
                                 newStartOffset + currentTokenStart);
                        startToken(text, i, end, c, newStartOffset);
                    }
                    break;

                case TokenTypes.IDENTIFIER:
                    if (!Character.isJavaIdentifierPart(c)) {
                        addToken(text, currentTokenStart, i - 1, TokenTypes.IDENTIFIER,
                                 newStartOffset + currentTokenStart);
                        startToken(text, i, end, c, newStartOffset);
                    }
                    break;

                case TokenTypes.LITERAL_NUMBER_DECIMAL_INT:
                    // Dígitos + punto + sufijos (L/d/f) + hex (0x..) — laxo a propósito.
                    if (!(RSyntaxUtilities.isDigit(c) || c == '.' || c == 'x' || c == 'X'
                          || c == 'L' || c == 'd' || c == 'f'
                          || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                        addToken(text, currentTokenStart, i - 1, TokenTypes.LITERAL_NUMBER_DECIMAL_INT,
                                 newStartOffset + currentTokenStart);
                        startToken(text, i, end, c, newStartOffset);
                    }
                    break;

                case TokenTypes.COMMENT_EOL:
                    // `//` hasta fin de línea (el Segment es 1 línea).
                    i = end - 1;
                    addToken(text, currentTokenStart, i, TokenTypes.COMMENT_EOL,
                             newStartOffset + currentTokenStart);
                    currentTokenType = TokenTypes.NULL;
                    break;

                case TokenTypes.LITERAL_STRING_DOUBLE_QUOTE:
                    if (c == '"') {
                        addToken(text, currentTokenStart, i, TokenTypes.LITERAL_STRING_DOUBLE_QUOTE,
                                 newStartOffset + currentTokenStart);
                        currentTokenType = TokenTypes.NULL;
                    }
                    break;
            }
        }

        // Fin de segmento: cerrar el token abierto.
        switch (currentTokenType) {
            case TokenTypes.NULL:
                addNullToken();
                break;
            default:
                addToken(text, currentTokenStart, end - 1, currentTokenType,
                         newStartOffset + currentTokenStart);
                addNullToken();
                break;
        }

        return firstToken;
    }
}
