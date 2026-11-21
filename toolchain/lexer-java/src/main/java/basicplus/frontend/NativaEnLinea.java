package basicplus.frontend;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * V7 #583 (9-oct) — ¿una línea del fuente cae DENTRO de una función o un método {@code native}?
 *
 * <p>Para el IDE: un breakpoint ahí no para en la placa —la native corre como código máquina; en el PC, que la corre
 * interpretada, sí—, y el IDE lo dice al ponerlo. Eduardo: <i>«No depuramos las natives. Si quieres ejecutar paso a paso
 * le quitas native, para verificar el algoritmo vale. La haces native cuando estás seguro que funciona.»</i>
 *
 * <p>Por el TEXTO y no por el árbol: el editor tiene el fuente a medio escribir y no siempre compila. Manda la
 * declaración de {@code function} más cercana por encima de la línea —con {@code native} entre sus modificadores o
 * sin él— y su {@code end <nombre>} por debajo (el nombre entero: {@code Mi.crear}, {@code ~Recurso}). Una función
 * sin su {@code end} (a medio escribir) no afirma nada.
 */
public final class NativaEnLinea {

    private NativaEnLinea() {}

    /** Los modificadores (las palabras de delante) y el nombre de una declaración de función. */
    private static final Pattern DECL = Pattern.compile(
            "^\\s*((?:[A-Za-z]+\\s+)*)function\\s+(~?[A-Za-z_][A-Za-z0-9_]*(?:\\.[A-Za-z_][A-Za-z0-9_]*)?)");

    /**
     * El nombre de la native que contiene la línea {@code linea} (1-based) de {@code fuente}, o {@code null} si no
     * está dentro de ninguna. La línea de la declaración y la de su {@code end} cuentan como dentro.
     */
    public static String nativaQueContiene(String fuente, int linea) {
        if (fuente == null || linea < 1) return null;
        String[] ls = fuente.split("\r?\n", -1);
        if (linea > ls.length) return null;
        for (int i = linea - 1; i >= 0; i--) {
            Matcher m = DECL.matcher(ls[i]);
            if (!m.find()) continue;
            boolean nativa = (" " + m.group(1).trim() + " ").contains(" native ");
            String nombre = m.group(2);
            Pattern fin = Pattern.compile("^\\s*end\\s+" + Pattern.quote(nombre) + "(?![A-Za-z0-9_.])");
            for (int j = i + 1; j < ls.length; j++) {
                if (fin.matcher(ls[j]).find()) {
                    return (nativa && j + 1 >= linea) ? nombre : null;
                }
            }
            return null;
        }
        return null;
    }
}
