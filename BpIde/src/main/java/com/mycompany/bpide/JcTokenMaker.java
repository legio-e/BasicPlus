/*
 * JcTokenMaker — TokenMaker de RSyntaxTextArea para JC (V8 #698): BasicPlus con la ropa de Java y C#.
 *
 * El hermano de BpTokenMaker, con lo que JC tiene de más: los comentarios de bloque, que pueden seguir en la línea
 * siguiente (el estado pasa de una línea a otra: la última pieza de la línea es un COMMENT_MULTILINE sin cerrar, y una
 * línea vacía en medio lo deja pasar), los escapes de las cadenas (`\"` no la cierra) y los caracteres, `'a'`.
 *
 * Las palabras, las de basicplus.frontend.JcLexer.RESERVADAS (es del paquete del compilador: se copian aquí, como
 * BpTokenMaker copia las del Lexer de BP). Distinguen mayúsculas, como en JC.
 *
 * Se registra en FrmMain con el estilo "text/jc", y el plegado, por llaves, con el CurlyFoldParser de RSTA.
 */
package com.mycompany.bpide;

import javax.swing.text.Segment;

import org.fife.ui.rsyntaxtextarea.AbstractTokenMaker;
import org.fife.ui.rsyntaxtextarea.RSyntaxUtilities;
import org.fife.ui.rsyntaxtextarea.Token;
import org.fife.ui.rsyntaxtextarea.TokenMap;
import org.fife.ui.rsyntaxtextarea.TokenTypes;

public class JcTokenMaker extends AbstractTokenMaker {

    private int currentTokenStart;
    private int currentTokenType;
    /** Dentro de una cadena o un carácter: el carácter anterior era una `\` (el siguiente no la cierra). */
    private boolean escapado;

    @Override
    public TokenMap getWordsToHighlight() {
        TokenMap tm = new TokenMap();
        final int KW = TokenTypes.RESERVED_WORD;
        final int DT = TokenTypes.DATA_TYPE;
        final int LB = TokenTypes.LITERAL_BOOLEAN;

        String[] keywords = {
            "module", "library", "interface", "implements", "extends", "import", "from",
            "const", "function", "class", "enum", "public", "private", "protected", "final", "native", "sync", "owner",
            "event", "static", "var", "new", "is", "base", "field",
            "if", "else", "while", "do", "for", "foreach", "in", "break", "continue", "return",
            "switch", "case", "default", "parallel", "try", "catch", "finally", "throw", "raise",
            "print", "println", "inline", "this", "super", "null"
        };
        for (String k : keywords) tm.put(k, KW);

        String[] types = {
            "int", "long", "float", "double", "bool", "string", "byte", "sbyte", "short", "ushort", "void"
        };
        for (String t : types) tm.put(t, DT);

        tm.put("true", LB);
        tm.put("false", LB);
        return tm;
    }

    @Override
    public void addToken(Segment segment, int start, int end, int tokenType, int startOffset) {
        if (tokenType == TokenTypes.IDENTIFIER) {
            int value = wordsToHighlight.get(segment, start, end);
            if (value != -1) tokenType = value;
        }
        super.addToken(segment, start, end, tokenType, startOffset);
    }

    @Override
    public String[] getLineCommentStartAndEnd(int languageIndex) {
        return new String[] { "//", null };
    }

    @Override
    public boolean getMarkOccurrencesOfTokenType(int type) {
        return type == TokenTypes.IDENTIFIER;
    }

    /** Empieza una pieza nueva en `i` según el carácter `c`. */
    private void startToken(Segment text, int i, int end, char c, int newStartOffset) {
        currentTokenStart = i;
        escapado = false;
        if (c == ' ' || c == '\t') {
            currentTokenType = TokenTypes.WHITESPACE;
        } else if (c == '"') {
            currentTokenType = TokenTypes.LITERAL_STRING_DOUBLE_QUOTE;
        } else if (c == '\'') {
            currentTokenType = TokenTypes.LITERAL_CHAR;
        } else if (c == '/' && i < end - 1 && text.array[i + 1] == '/') {
            currentTokenType = TokenTypes.COMMENT_EOL;
        } else if (c == '/' && i < end - 1 && text.array[i + 1] == '*') {
            currentTokenType = TokenTypes.COMMENT_MULTILINE;
        } else if (RSyntaxUtilities.isDigit(c)) {
            currentTokenType = TokenTypes.LITERAL_NUMBER_DECIMAL_INT;
        } else if (Character.isJavaIdentifierStart(c)) {
            currentTokenType = TokenTypes.IDENTIFIER;
        } else {
            addToken(text, i, i, TokenTypes.IDENTIFIER, newStartOffset + i);   // un signo
            currentTokenType = TokenTypes.NULL;
        }
    }

    /** Una línea vacía dentro de un comentario de bloque no tiene piezas: el comentario sigue en la de abajo. */
    @Override
    public int getLastTokenTypeOnLine(Segment text, int initialTokenType) {
        if (text.count == 0)
            return initialTokenType == TokenTypes.COMMENT_MULTILINE ? TokenTypes.COMMENT_MULTILINE : TokenTypes.NULL;
        return super.getLastTokenTypeOnLine(text, initialTokenType);
    }

    @Override
    public Token getTokenList(Segment text, int startTokenType, int startOffset) {
        resetTokenList();
        if (text.count == 0) {   // nada que pintar (y el estado lo da getLastTokenTypeOnLine)
            addNullToken();
            return firstToken;
        }

        char[] array = text.array;
        int offset = text.offset;
        int count = text.count;
        int end = offset + count;
        int newStartOffset = startOffset - offset;

        currentTokenStart = offset;
        currentTokenType = startTokenType;
        escapado = false;
        // Un `/*` abierto en la línea anterior sigue aquí: la pieza empieza en el primer carácter.
        boolean comentarioQueSigue = startTokenType == TokenTypes.COMMENT_MULTILINE;

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
                    // Cifras, el punto, el exponente, hex y binario, los sufijos (f, L) y el `_`: laxo a propósito.
                    if (!(RSyntaxUtilities.isDigit(c) || c == '.' || c == '_' || c == 'x' || c == 'X'
                          || c == 'L' || c == 'l' || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                        addToken(text, currentTokenStart, i - 1, TokenTypes.LITERAL_NUMBER_DECIMAL_INT,
                                 newStartOffset + currentTokenStart);
                        startToken(text, i, end, c, newStartOffset);
                    }
                    break;

                case TokenTypes.COMMENT_EOL:
                    i = end - 1;
                    addToken(text, currentTokenStart, i, TokenTypes.COMMENT_EOL,
                             newStartOffset + currentTokenStart);
                    currentTokenType = TokenTypes.NULL;
                    break;

                case TokenTypes.COMMENT_MULTILINE:
                    // Se cierra con `*/`; el `*` del `/*` que lo abrió no cuenta (`/*/` no se cierra; `/**/`, sí).
                    if (c == '/' && i > currentTokenStart + (comentarioQueSigue ? 0 : 2) && array[i - 1] == '*') {
                        addToken(text, currentTokenStart, i, TokenTypes.COMMENT_MULTILINE,
                                 newStartOffset + currentTokenStart);
                        currentTokenType = TokenTypes.NULL;
                        comentarioQueSigue = false;
                    }
                    break;

                case TokenTypes.LITERAL_STRING_DOUBLE_QUOTE:
                case TokenTypes.LITERAL_CHAR:
                    if (escapado) {
                        escapado = false;
                    } else if (c == '\\') {
                        escapado = true;
                    } else if (c == (currentTokenType == TokenTypes.LITERAL_CHAR ? '\'' : '"')) {
                        addToken(text, currentTokenStart, i, currentTokenType, newStartOffset + currentTokenStart);
                        currentTokenType = TokenTypes.NULL;
                    }
                    break;
            }
        }

        switch (currentTokenType) {
            case TokenTypes.NULL:
                addNullToken();
                break;
            case TokenTypes.COMMENT_MULTILINE:
                // Sin cerrar: la línea acaba en el comentario, y sin la pieza nula del final, la siguiente lo sigue.
                addToken(text, currentTokenStart, end - 1, currentTokenType, newStartOffset + currentTokenStart);
                break;
            default:
                // Una cadena sin cerrar no pasa a la línea siguiente (en JC, una cadena no cruza líneas).
                addToken(text, currentTokenStart, end - 1, currentTokenType, newStartOffset + currentTokenStart);
                addNullToken();
                break;
        }

        return firstToken;
    }
}
