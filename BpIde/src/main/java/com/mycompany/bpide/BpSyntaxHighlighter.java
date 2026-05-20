package com.mycompany.bpide;

import java.awt.Color;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;
import javax.swing.JTextPane;
import javax.swing.SwingUtilities;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.text.BadLocationException;
import javax.swing.text.Style;
import javax.swing.text.StyleConstants;
import javax.swing.text.StyleContext;
import javax.swing.text.StyledDocument;

/**
 * Re-tokeniza el documento entero tras cada edición y aplica estilos a:
 * comentarios (// ...), strings ("..." con escapes), números (decimales y
 * 0xHEX), y palabras reservadas de BasicPlus.
 *
 * <p>El highlighter se instala vía {@link #install(JTextPane)}: añade un
 * {@link DocumentListener} que dispara un re-estilado completo en
 * {@code SwingUtilities.invokeLater} (debounce trivial). La aplicación de
 * estilos vía {@code setCharacterAttributes} NO dispara
 * insert/removeUpdate, sólo changedUpdate — y este listener lo ignora —
 * así que no hay recursión.</p>
 *
 * <p>Para un fichero típico de IDE (≤ 20 KiB) re-tokenizar entero es
 * suficientemente rápido. Para ficheros muy grandes habría que pasar a
 * tokenización incremental por línea.</p>
 */
public final class BpSyntaxHighlighter {

    // -----------------------------------------------------------------
    // Palabras reservadas de BasicPlus (debe coincidir con TokenType /
    // Lexer.buildKeywords del frontend). Resaltar palabras que NO son
    // reservadas no rompe, sólo no se colorean.
    // -----------------------------------------------------------------
    private static final Set<String> KEYWORDS = Collections.unmodifiableSet(new HashSet<>(Arrays.asList(
            // estructura / módulos
            "module", "library", "from", "end", "import", "interface", "implements",
            // declaraciones
            "const", "var", "function", "class", "extends", "enum",
            "property", "get", "set", "endprop", "endget", "endset",
            // visibilidad / modificadores / ownership
            "public", "final", "owner",
            // instancia / clase base
            "this", "super", "field",
            // control de flujo
            "if", "then", "elseif", "else", "endif",
            "switch", "case", "default", "endsw",
            "while", "do", "endwh", "loop",
            "for", "to", "step", "next", "in",
            "break", "continue", "return",
            "parallel", "endpar",
            // excepciones
            "try", "catch", "finally", "endtry", "throw",
            // E/S
            "print",
            // tipos primitivos
            "integer", "float", "string", "boolean",
            // literales especiales
            "true", "false", "null",
            // operadores en forma de palabra
            "and", "or", "not", "xor", "mod", "shl", "shr", "instanceof"
    )));

    private final JTextPane editor;
    private final Style keywordStyle;
    private final Style stringStyle;
    private final Style numberStyle;
    private final Style commentStyle;
    private final Style plainStyle;
    /** Si true ignoramos los próximos eventos del Document (estamos aplicando estilos). */
    private boolean applyingStyles = false;

    private BpSyntaxHighlighter(JTextPane editor) {
        this.editor = editor;
        StyledDocument doc = editor.getStyledDocument();
        Style base = StyleContext.getDefaultStyleContext().getStyle(StyleContext.DEFAULT_STYLE);

        plainStyle = doc.addStyle("bp-plain", base);
        StyleConstants.setForeground(plainStyle, Color.BLACK);

        keywordStyle = doc.addStyle("bp-keyword", base);
        StyleConstants.setForeground(keywordStyle, new Color(128, 0, 255));
        StyleConstants.setBold(keywordStyle, true);

        stringStyle = doc.addStyle("bp-string", base);
        StyleConstants.setForeground(stringStyle, new Color(163, 21, 21));

        numberStyle = doc.addStyle("bp-number", base);
        StyleConstants.setForeground(numberStyle, new Color(9, 134, 88));

        commentStyle = doc.addStyle("bp-comment", base);
        StyleConstants.setForeground(commentStyle, new Color(106, 153, 78));
        StyleConstants.setItalic(commentStyle, true);
    }

    /** Atacha el highlighter al editor y dispara un primer re-estilado. */
    public static BpSyntaxHighlighter install(JTextPane editor) {
        BpSyntaxHighlighter h = new BpSyntaxHighlighter(editor);
        editor.getStyledDocument().addDocumentListener(new DocumentListener() {
            @Override public void insertUpdate(DocumentEvent e)  { h.scheduleRestyle(); }
            @Override public void removeUpdate(DocumentEvent e)  { h.scheduleRestyle(); }
            @Override public void changedUpdate(DocumentEvent e) { /* ignore */ }
        });
        h.scheduleRestyle();
        return h;
    }

    private void scheduleRestyle() {
        if (applyingStyles) return;
        SwingUtilities.invokeLater(this::restyle);
    }

    private void restyle() {
        StyledDocument doc = editor.getStyledDocument();
        String text;
        try {
            text = doc.getText(0, doc.getLength());
        } catch (BadLocationException ex) {
            return;
        }
        applyingStyles = true;
        try {
            // Reset todo a plain — luego los tokens los sobrescriben.
            doc.setCharacterAttributes(0, text.length(), plainStyle, true);
            tokenize(doc, text);
        } finally {
            applyingStyles = false;
        }
    }

    /** Recorrido lineal del texto: en cada paso identifica el siguiente token. */
    private void tokenize(StyledDocument doc, String text) {
        int n = text.length();
        int i = 0;
        while (i < n) {
            char c = text.charAt(i);

            // Comentario de línea: // ... hasta '\n'.
            if (c == '/' && i + 1 < n && text.charAt(i + 1) == '/') {
                int j = i + 2;
                while (j < n && text.charAt(j) != '\n') j++;
                doc.setCharacterAttributes(i, j - i, commentStyle, true);
                i = j;
                continue;
            }

            // String literal: "..." con escapes \\ y \" reconocidos.
            if (c == '"') {
                int j = i + 1;
                while (j < n) {
                    char ch = text.charAt(j);
                    if (ch == '\\' && j + 1 < n) { j += 2; continue; }
                    if (ch == '"' || ch == '\n') break;
                    j++;
                }
                if (j < n && text.charAt(j) == '"') j++;   // incluir el cierre
                doc.setCharacterAttributes(i, j - i, stringStyle, true);
                i = j;
                continue;
            }

            // Número: decimal, fracción opcional, exponente opcional, o 0x...
            if (isDigit(c) || (c == '.' && i + 1 < n && isDigit(text.charAt(i + 1)))) {
                int j = i;
                // 0xHEX
                if (c == '0' && i + 1 < n && (text.charAt(i + 1) == 'x' || text.charAt(i + 1) == 'X')) {
                    j += 2;
                    while (j < n && isHex(text.charAt(j))) j++;
                } else {
                    while (j < n && isDigit(text.charAt(j))) j++;
                    if (j < n && text.charAt(j) == '.') {
                        j++;
                        while (j < n && isDigit(text.charAt(j))) j++;
                    }
                    if (j < n && (text.charAt(j) == 'e' || text.charAt(j) == 'E')) {
                        j++;
                        if (j < n && (text.charAt(j) == '+' || text.charAt(j) == '-')) j++;
                        while (j < n && isDigit(text.charAt(j))) j++;
                    }
                }
                doc.setCharacterAttributes(i, j - i, numberStyle, true);
                i = j;
                continue;
            }

            // Identificador / palabra reservada.
            if (Character.isJavaIdentifierStart(c)) {
                int j = i + 1;
                while (j < n && Character.isJavaIdentifierPart(text.charAt(j))) j++;
                String word = text.substring(i, j);
                if (KEYWORDS.contains(word)) {
                    doc.setCharacterAttributes(i, j - i, keywordStyle, true);
                }
                i = j;
                continue;
            }

            i++;
        }
    }

    private static boolean isDigit(char c) { return c >= '0' && c <= '9'; }
    private static boolean isHex(char c) {
        return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }
}
