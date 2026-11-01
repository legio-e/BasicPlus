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
//
// Paso 3 — la ida y vuelta por JC tiene su propio modo (primeraDiferenciaJc),
// con lo que se acordó que no cuenta (V8_IDEAS §E2, puntos 5, 15, 17, 18, 22,
// 23 y 29): los paréntesis, que cada lenguaje pone según su precedencia y el
// emisor atraviesa; la mayúscula de un operador de BP (`MOD` y `mod`); un `var`
// de varios nombres con un valor, que es uno por nombre (medido: el mismo
// `.mod`); el tipo de una constante que BP no escribe; y el `print`, que se
// compara por lo que imprime (la coma de BP es un " ", y su salto, un "\n").
// Las marcas del `new`, de las conversiones y del ternario (CallExpr) no cuentan
// nunca: no son parte del programa.
// ============================================================
package basicplus.frontend;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.List;

final class ComparaAst {

    private final boolean jc;

    private ComparaAst(boolean jc) { this.jc = jc; }

    /** null si son iguales; si no, la ruta del primer campo distinto y la línea del original. */
    static String primeraDiferencia(Object a, Object b) {
        return new ComparaAst(false).comparar(a, b, "modulo", 0);
    }

    /** La de la ida y vuelta por JC: lo mismo, sin lo que se acordó que no cuenta (arriba). */
    static String primeraDiferenciaJc(Object a, Object b) {
        return new ComparaAst(true).comparar(a, b, "modulo", 0);
    }

    private String comparar(Object a, Object b, String ruta, int linea) {
        if (jc) {
            while (a instanceof Ast.ParenExpr) a = ((Ast.ParenExpr) a).inner;
            while (b instanceof Ast.ParenExpr) b = ((Ast.ParenExpr) b).inner;
        }
        if (a == b) return null;
        if (a instanceof Ast.Node) linea = ((Ast.Node) a).line;
        if (a == null || b == null) return donde(ruta, linea) + ": " + corto(a) + " / " + corto(b);
        if (a.getClass() != b.getClass())
            return donde(ruta, linea) + ": " + a.getClass().getSimpleName() + " / " + b.getClass().getSimpleName();
        if (a instanceof List) {
            List<?> la = (List<?>) a, lb = (List<?>) b;
            if (jc) {
                la = expandir(la);
                lb = expandir(lb);
            }
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
        if (jc && a instanceof Ast.PrintStmt)
            return comparar(piezas((Ast.PrintStmt) a), piezas((Ast.PrintStmt) b), ruta + ".piezas", linea);
        // Un objeto del árbol: todos sus campos, también los heredados, menos las posiciones.
        for (Class<?> c = a.getClass(); c != null && c != Object.class; c = c.getSuperclass()) {
            for (Field f : c.getDeclaredFields()) {
                if (Modifier.isStatic(f.getModifiers())) continue;
                String n = f.getName();
                if (n.equals("line") || n.equals("column")) continue;
                if (n.equals("conNew") || n.equals("esConversion") || n.equals("esTernario") || n.equals("escritoEnJc")
                        || n.equals("argumentosNew") || n.equals("claseNew") || n.equals("primeroPuedeSerInterfaz"))
                    continue;   // las marcas de JC (CallExpr, ModuleNode, ClassDef)
                // V8 H7.3 — JC escribe el padre y las interfaces en una lista (`class C : A, B`) y no sabe cuál es cuál:
                // se compara la lista entera, el padre primero
                if (jc && a instanceof Ast.ClassDef && n.equals("interfaces")) continue;
                if (jc && a instanceof Ast.ClassDef && n.equals("baseClass")) {
                    String d = comparar(supertipos((Ast.ClassDef) a), supertipos((Ast.ClassDef) b), ruta + ".supertipos", linea);
                    if (d != null) return d;
                    continue;
                }
                f.setAccessible(true);
                Object va, vb;
                try {
                    va = f.get(a);
                    vb = f.get(b);
                } catch (IllegalAccessException ex) {
                    return donde(ruta, linea) + ": no se puede leer el campo " + n;
                }
                if (jc && a instanceof Ast.ConstDecl && n.equals("type") && (va == null || vb == null)) continue;
                if (jc && a instanceof Ast.BinaryExpr && n.equals("op")) {
                    if (!((String) va).equalsIgnoreCase((String) vb))
                        return donde(ruta + ".op", linea) + ": " + corto(va) + " / " + corto(vb);
                    continue;
                }
                String d = comparar(va, vb, ruta + "." + n, linea);
                if (d != null) return d;
            }
        }
        return null;
    }

    /** Un `var` de varios nombres con un valor es uno por nombre: BP da el valor a cada uno (punto 5). */
    private static List<?> expandir(List<?> l) {
        boolean hay = false;
        for (Object o : l) if (multiConValor(o)) { hay = true; break; }
        if (!hay) return l;
        List<Object> out = new ArrayList<>();
        for (Object o : l) {
            if (!multiConValor(o)) { out.add(o); continue; }
            Ast.VarDecl v = (Ast.VarDecl) o;
            for (int i = 0; i < v.names.size(); i++) {
                List<Ast.DeclName> uno = new ArrayList<>();
                uno.add(v.names.get(i));
                Ast.VarDecl w = new Ast.VarDecl(v.isPublic, v.isOwner, uno, v.type, v.init, v.line, v.column);
                w.isPrivate = v.isPrivate;
                if (i == 0) w.annotations.addAll(v.annotations);
                out.add(w);
            }
        }
        return out;
    }

    /** El padre (si lo hay) y las interfaces de una clase, en una lista: lo que JC escribe tras `:`. */
    private static List<String> supertipos(Ast.ClassDef c) {
        List<String> l = new ArrayList<>();
        if (c.baseClass != null) l.add(c.baseClass);
        l.addAll(c.interfaces);
        return l;
    }

    private static boolean multiConValor(Object o) {
        return o instanceof Ast.VarDecl && ((Ast.VarDecl) o).names.size() > 1 && ((Ast.VarDecl) o).init != null;
    }

    /** Lo que imprime un `print`, pieza a pieza: la coma de BP es un " ", y su salto, un "\n" al final, salvo con un
     *  separador al final (punto 17). */
    private static List<Object> piezas(Ast.PrintStmt p) {
        List<Object> out = new ArrayList<>();
        boolean salto = true;
        for (int i = 0; i < p.items.size(); i++) {
            Ast.PrintItem it = p.items.get(i);
            if (i > 0 && it.leadingSep == Ast.PrintSep.COMMA) out.add(new Ast.StringLitExpr(" ", 0, 0));
            if (it.expr != null) out.add(it.expr);
            else if (i == p.items.size() - 1) salto = false;
        }
        if (salto) out.add(SALTO);
        return out;
    }

    /** El salto del `print` de BP, el `println` de JC: no es una pieza `"\n"`. Los bytes son los mismos, pero sólo el
     *  salto cierra la línea del hilo y la saca en su momento (V8 #703); un `"\n"` escrito espera con el resto. */
    private static final String SALTO = "el salto del print";

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
