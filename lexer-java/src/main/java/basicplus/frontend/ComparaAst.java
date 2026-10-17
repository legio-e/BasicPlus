// ============================================================
// ComparaAst.java
// V8 #698 (JC) — ¿dos árboles son el mismo programa? Compara campo a campo,
// sin contar las líneas ni las columnas.
//
// Es la vara de medir de «equivalente» del traductor (V8_IDEAS §E2): el árbol
// del original y el del BP reescrito tienen que ser iguales salvo en las
// posiciones. Se compara justo tras el parser, antes de que el semántico
// rellene nada (los nombres sintetizados, por ejemplo, siguen a null en los dos).
// Por reflexión: así un nodo o un campo nuevo del AST entra solo en la
// comparación, sin que haya que acordarse de añadirlo aquí.
// ============================================================
package basicplus.frontend;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.util.List;

final class ComparaAst {

    private ComparaAst() {}

    /** null si son iguales; si no, la ruta del primer campo distinto y la línea del original. */
    static String primeraDiferencia(Object a, Object b) {
        return comparar(a, b, "modulo", 0);
    }

    private static String comparar(Object a, Object b, String ruta, int linea) {
        if (a == b) return null;
        if (a instanceof Ast.Node) linea = ((Ast.Node) a).line;
        if (a == null || b == null) return donde(ruta, linea) + ": " + corto(a) + " / " + corto(b);
        if (a.getClass() != b.getClass())
            return donde(ruta, linea) + ": " + a.getClass().getSimpleName() + " / " + b.getClass().getSimpleName();
        if (a instanceof List) {
            List<?> la = (List<?>) a, lb = (List<?>) b;
            if (la.size() != lb.size()) return donde(ruta, linea) + ": " + la.size() + " elementos / " + lb.size();
            for (int i = 0; i < la.size(); i++) {
                String d = comparar(la.get(i), lb.get(i), ruta + "[" + i + "]", linea);
                if (d != null) return d;
            }
            return null;
        }
        if (a instanceof Double || a instanceof Float) {
            return Double.compare(((Number) a).doubleValue(), ((Number) b).doubleValue()) == 0
                    ? null : donde(ruta, linea) + ": " + a + " / " + b;
        }
        if (a instanceof String || a instanceof Number || a instanceof Boolean || a instanceof Character
                || a instanceof Enum) {
            return a.equals(b) ? null : donde(ruta, linea) + ": " + corto(a) + " / " + corto(b);
        }
        // Un objeto del árbol: todos sus campos, también los heredados, menos las posiciones.
        for (Class<?> c = a.getClass(); c != null && c != Object.class; c = c.getSuperclass()) {
            for (Field f : c.getDeclaredFields()) {
                if (Modifier.isStatic(f.getModifiers())) continue;
                if (f.getName().equals("line") || f.getName().equals("column")) continue;
                f.setAccessible(true);
                Object va, vb;
                try {
                    va = f.get(a);
                    vb = f.get(b);
                } catch (IllegalAccessException ex) {
                    return donde(ruta, linea) + ": no se puede leer el campo " + f.getName();
                }
                String d = comparar(va, vb, ruta + "." + f.getName(), linea);
                if (d != null) return d;
            }
        }
        return null;
    }

    private static String donde(String ruta, int linea) {
        return ruta + (linea > 0 ? " (línea " + linea + " del original)" : "");
    }

    private static String corto(Object o) {
        if (o == null) return "null";
        String s = (o instanceof String) ? "\"" + o + "\"" : o.toString();
        if (!(o instanceof String || o instanceof Number || o instanceof Boolean || o instanceof Enum))
            s = o.getClass().getSimpleName();
        return s.length() > 60 ? s.substring(0, 60) + "…" : s;
    }
}
