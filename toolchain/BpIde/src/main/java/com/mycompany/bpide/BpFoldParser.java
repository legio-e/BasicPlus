/*
 * BpFoldParser — FoldParser de RSyntaxTextArea para BasicPlus (IDE-5).
 *
 * BP delimita bloques por PALABRAS (no por llaves): un "abridor" al inicio de
 * línea (function, class, if, while, for, switch, try, parallel, property,
 * get, set, module, ...) y un "cerrador" (end, endif, endwh, endsw, next,
 * endtry, endpar, endprop, endget, endset). En BP bien formado los bloques
 * anidan correctamente, así que basta una pila implícita (puntero currentFold
 * + getParent(), como CurlyFoldParser): abridor → push; cerrador → pop.
 *
 * Best-effort: se detecta la PRIMERA palabra de cada línea (ignorando líneas
 * en blanco y comentarios `//`); keywords a media línea o dentro de strings no
 * disparan plegado. Bloques de una sola línea se descartan.
 *
 * Registro (FrmMain.setupMvp):
 *   FoldParserManager.get().addFoldParserMapping("text/bp", new BpFoldParser());
 * y el editor con setCodeFoldingEnabled(true).
 */
package com.mycompany.bpide;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import javax.swing.text.BadLocationException;
import javax.swing.text.Element;

import org.fife.ui.rsyntaxtextarea.RSyntaxTextArea;
import org.fife.ui.rsyntaxtextarea.folding.Fold;
import org.fife.ui.rsyntaxtextarea.folding.FoldParser;
import org.fife.ui.rsyntaxtextarea.folding.FoldType;

public class BpFoldParser implements FoldParser {

    private static final Set<String> OPENERS = new HashSet<>(Arrays.asList(
        "module", "library", "interface", "class", "function", "enum",
        "property", "get", "set",
        "if", "switch", "while", "for", "try", "parallel"
    ));

    private static final Set<String> CLOSERS = new HashSet<>(Arrays.asList(
        "end", "endif", "endsw", "endwh", "next", "endtry", "endpar",
        "endprop", "endget", "endset"
    ));

    /** Modificadores que pueden PRECEDER a un abridor de bloque y hay que
     *  saltar: `public final native function ...`, `public class ...`, etc. */
    private static final Set<String> MODIFIERS = new HashSet<>(Arrays.asList(
        "public", "final", "sync", "intrinsic", "native", "owner"
    ));

    @Override
    public List<Fold> getFolds(RSyntaxTextArea textArea) {
        List<Fold> folds = new ArrayList<>();
        Fold current = null;
        Element root = textArea.getDocument().getDefaultRootElement();
        int lineCount = root.getElementCount();

        try {
            for (int line = 0; line < lineCount; line++) {
                Element le = root.getElement(line);
                int lineStart = le.getStartOffset();
                int lineEnd = le.getEndOffset();
                String text = textArea.getText(lineStart, lineEnd - lineStart);

                int ws = leadingWhitespace(text);
                String word = significantWord(text, ws);   // salta modificadores
                if (word.isEmpty()) continue;
                int wordOffset = lineStart + ws;

                if (OPENERS.contains(word)) {
                    if (current == null) {
                        current = new Fold(FoldType.CODE, textArea, wordOffset);
                        folds.add(current);
                    } else {
                        current = current.createChild(FoldType.CODE, wordOffset);
                    }
                } else if (CLOSERS.contains(word) && current != null) {
                    current.setEndOffset(wordOffset);
                    Fold parent = current.getParent();
                    // Bloque de una sola línea → no es plegable; descártalo.
                    if (current.isOnSingleLine()) {
                        if (!current.removeFromParent() && !folds.isEmpty()) {
                            folds.remove(folds.size() - 1);
                        }
                    }
                    current = parent;
                }
            }
        } catch (BadLocationException ble) {
            // best-effort: si el documento cambia bajo nuestros pies, devolvemos
            // lo acumulado.
        }

        return folds;
    }

    /** Nº de espacios/tabs iniciales. */
    private static int leadingWhitespace(String s) {
        int i = 0;
        while (i < s.length() && (s.charAt(i) == ' ' || s.charAt(i) == '\t')) i++;
        return i;
    }

    /** Primera palabra "significativa" tras la sangría, SALTANDO modificadores
     *  (public/final/sync/intrinsic/native/owner) que pueden preceder a un
     *  abridor de bloque (p.ej. `public native function`). "" si la línea es un
     *  comentario `//` o no empieza por identificador. */
    private static String significantWord(String s, int from) {
        int n = s.length();
        int p = from;
        while (p < n) {
            if (p + 1 < n && s.charAt(p) == '/' && s.charAt(p + 1) == '/') return "";
            int start = p;
            while (p < n && (Character.isLetterOrDigit(s.charAt(p)) || s.charAt(p) == '_')) p++;
            if (p == start) return "";              // no es identificador
            String w = s.substring(start, p);
            if (!MODIFIERS.contains(w)) return w;    // primera no-modificador
            while (p < n && (s.charAt(p) == ' ' || s.charAt(p) == '\t')) p++;  // saltar a la siguiente
        }
        return "";
    }
}
