package basicplus.frontend;

import basicplus.frontend.Ast.*;

import java.util.ArrayDeque;
import java.util.Deque;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * #540 (V7/H1) — la ASIGNACIÓN DEFINIDA de las variables locales.
 *
 * <p>La regla es de Eduardo (24-sep): <i>«Las variables globales siempre se inicializan ya que van en el
 * bloque de constantes del módulo. Las variables locales si no se inicializan su valor es indeterminado.
 * En teoría utilizar una variable local sin inicializar es un error.»</i> Y es error de compilación, como
 * en Java. Hasta ahora no se detectaba, y el PC lo TAPABA: en un programa recién arrancado la pila está a
 * cero, así que el programa «funcionaba» en las dos VMs y en la placa salía basura.
 *
 * <p>Se vigila cada local declarada SIN inicializador ({@code var x: T}), salvo los arrays fijos (se ponen
 * a cero al declararlos). Leerla en un punto al que se llega sin haberle dado valor por ALGÚN camino es un
 * error, una vez por variable. En BP asignar es una sentencia, nunca una expresión: las expresiones sólo
 * leen. El diseño, en {@code docs/V7_IDEAS.md} §H1.7.
 *
 * <p>Corre al final del análisis de cada cuerpo, sobre el AST ya anotado: el uso se casa con su
 * declaración por el {@code VarSymbol} que el semántico dejó en {@code exprSymbols}, cuyo nombre, línea y
 * columna son los del {@code DeclName} ({@code declSymbols} sólo guarda el primero de {@code var x, y}).
 */
final class AsignacionDefinida {

    interface Reporte { void err(int line, int col, String msg); }

    private final SemanticInfo info;
    private final Reporte rep;
    /** Las vigiladas de este cuerpo: nombre@línea:columna de su declaración. */
    private final Set<String> vigiladas = new HashSet<>();
    private final Set<String> dichas = new HashSet<>();
    /** Los destinos de break/continue abiertos: bucles y switch (break sale del switch; continue, no). */
    private final Deque<Salida> salidas = new ArrayDeque<>();

    AsignacionDefinida(SemanticInfo info, Reporte rep) { this.info = info; this.rep = rep; }

    void revisa(List<IStmt> cuerpo) {
        if (cuerpo == null) return;
        Estado e = new Estado();
        bloque(cuerpo, e);
    }

    // ─── el estado: las vigiladas CON valor en este punto; `vivo` = se llega aquí ───

    private static final class Estado {
        Set<String> con = new HashSet<>();
        boolean vivo = true;

        Estado copia() { Estado c = new Estado(); c.con.addAll(con); c.vivo = vivo; return c; }

        /** Cruce de caminos: tienen valor las que lo tienen por TODOS los que llegan. Un camino que no
         *  llega no cuenta (vale como «todas»). */
        static Estado cruce(Estado a, Estado b) {
            if (a == null) return b;
            if (b == null) return a;
            if (!a.vivo) return b.copia();
            if (!b.vivo) return a.copia();
            Estado r = a.copia();
            r.con.retainAll(b.con);
            return r;
        }

        void tomaDe(Estado o) { con = new HashSet<>(o.con); vivo = o.vivo; }
    }

    private static final class Salida {
        final boolean esBucle;
        String nombre;      // V8/H8: el de un `do miLoop`
        Estado alSalir;     // cruce de los break
        Estado alSeguir;    // cruce de los continue (sólo bucles)
        Salida(boolean esBucle) { this.esBucle = esBucle; }
    }

    // ─── sentencias ───

    private void bloque(List<IStmt> cuerpo, Estado e) {
        if (cuerpo == null) return;
        for (IStmt s : cuerpo) sentencia(s, e);
    }

    private Estado desde(List<IStmt> cuerpo, Estado base) {
        Estado e = base.copia();
        bloque(cuerpo, e);
        return e;
    }

    private void sentencia(IStmt s, Estado e) {
        if (s instanceof VarDecl) {
            declara((VarDecl) s, e);
        } else if (s instanceof AssignStmt) {
            AssignStmt a = (AssignStmt) s;
            lee(a.value, e);
            String k = (a.target instanceof IdentifierExpr) ? clave((IdentifierExpr) a.target) : null;
            if (k != null) {
                if (a.op != AssignOpKind.ASSIGN) usa((IdentifierExpr) a.target, k, e);   // x += v lee x
                e.con.add(k);
            } else {
                lee(a.target, e);          // a[i] := v y o.campo := v leen a y o
            }
        } else if (s instanceof DestructAssignStmt) {
            DestructAssignStmt d = (DestructAssignStmt) s;
            lee(d.value, e);
            for (IExpr t : d.targets) {
                String k = (t instanceof IdentifierExpr) ? clave((IdentifierExpr) t) : null;
                if (k != null) e.con.add(k); else lee(t, e);
            }
        } else if (s instanceof IfStmt) {
            IfStmt f = (IfStmt) s;
            lee(f.then_.condition, e);
            Estado base = e.copia();
            Estado r = desde(f.then_.body, base);
            if (f.elseIfs != null) {
                for (IfClause c : f.elseIfs) {
                    lee(c.condition, base);
                    r = Estado.cruce(r, desde(c.body, base));
                }
            }
            r = Estado.cruce(r, f.else_ != null ? desde(f.else_, base) : base.copia());
            e.tomaDe(r);
        } else if (s instanceof SwitchStmt) {
            SwitchStmt w = (SwitchStmt) s;
            lee(w.subject, e);
            Estado base = e.copia();
            Salida sal = new Salida(false);
            salidas.push(sal);
            Estado r = null;
            try {
                for (CaseClause c : w.cases) {
                    if (c.values != null) for (IExpr v : c.values) lee(v, base);
                    r = Estado.cruce(r, desde(c.body, base));
                }
                r = Estado.cruce(r, w.defaultBody != null ? desde(w.defaultBody, base) : base.copia());
            } finally { salidas.pop(); }
            e.tomaDe(Estado.cruce(r, sal.alSalir));
        } else if (s instanceof WhileStmt) {
            WhileStmt w = (WhileStmt) s;
            lee(w.condition, e);
            Estado base = e.copia();
            Salida sal = bucle(w.body, base, w.nombre);
            if (esCierto(w.condition)) {
                // `while true`: sólo se sale por sus break (si no hay, lo de después no se alcanza)
                Estado r = sal.alSalir != null ? sal.alSalir : noAlcanzado();
                e.tomaDe(r);
            } else {
                e.tomaDe(base);   // el cuerpo puede no ejecutarse; los break sólo añaden
            }
        } else if (s instanceof DoLoopStmt) {
            DoLoopStmt d = (DoLoopStmt) s;
            Salida sal = new Salida(true);
            sal.nombre = d.nombre;
            salidas.push(sal);
            Estado fin;
            try { fin = desde(d.body, e); } finally { salidas.pop(); }
            Estado enLaCondicion = Estado.cruce(fin, sal.alSeguir);
            if (d.condition != null) {
                lee(d.condition, enLaCondicion);
                e.tomaDe(Estado.cruce(enLaCondicion, sal.alSalir));
            } else {
                e.tomaDe(sal.alSalir != null ? sal.alSalir : noAlcanzado());   // bucle infinito
            }
        } else if (s instanceof ForStmt) {
            ForStmt f = (ForStmt) s;
            if (f.range instanceof ForNumericRange) {
                ForNumericRange r = (ForNumericRange) f.range;
                lee(r.from, e); lee(r.to, e); lee(r.step, e);
            } else if (f.range instanceof ForInRange) {
                lee(((ForInRange) f.range).iterable, e);
            }
            Estado base = e.copia();
            bucle(f.body, base, f.nombre);
            e.tomaDe(base);
        } else if (s instanceof TryStmt) {
            TryStmt t = (TryStmt) s;
            if (t.recurso != null) declara(t.recurso, e);   // `try var r := ...`
            Estado base = e.copia();
            Estado r = desde(t.body, base);
            // la excepción puede saltar en cualquier punto del cuerpo: cada catch parte de ANTES
            if (t.catches != null) for (CatchClause c : t.catches) r = Estado.cruce(r, desde(c.body, base));
            if (t.finallyBody != null) {
                Estado f = desde(t.finallyBody, base);
                if (!f.vivo) r.vivo = false;
                else if (r.vivo) for (String k : f.con) r.con.add(k);   // lo que el finally asigna, lo tiene
            }
            e.tomaDe(r);
        } else if (s instanceof ParallelStmt) {
            ParallelStmt p = (ParallelStmt) s;
            for (ParallelBranch b : p.branches) {
                lee(b.stackSizeExpr, e);
                // cada rama es el run() de un hilo: no ve las locales de quien la lanza
                new AsignacionDefinida(info, rep).revisa(b.body);
            }
            if (p.defaultBody != null) bloque(p.defaultBody, e);
        } else if (s instanceof ReturnStmt) {
            lee(((ReturnStmt) s).value, e);
            e.vivo = false;
        } else if (s instanceof ThrowStmt) {
            lee(((ThrowStmt) s).value, e);
            e.vivo = false;
        } else if (s instanceof BreakStmt) {
            String n = ((BreakStmt) s).nombre;
            Salida sal = n == null ? salidas.peek() : salidaDe(n);
            if (sal != null) sal.alSalir = Estado.cruce(sal.alSalir, e.copia());
            e.vivo = false;
        } else if (s instanceof ContinueStmt) {
            String n = ((ContinueStmt) s).nombre;
            for (Salida sal : salidas) {               // el bucle más interior (un switch no es destino), o el de ese nombre
                if (n == null ? sal.esBucle : n.equals(sal.nombre)) { sal.alSeguir = Estado.cruce(sal.alSeguir, e.copia()); break; }
            }
            e.vivo = false;
        } else if (s instanceof PrintStmt) {
            for (PrintItem it : ((PrintStmt) s).items) lee(it.expr, e);
        } else if (s instanceof ExprStmt) {
            lee(((ExprStmt) s).expr, e);
        } else if (s instanceof RaiseStmt) {
            RaiseStmt r = (RaiseStmt) s;
            if (r.args != null) for (IExpr a : r.args) lee(a, e);
        }
        // InlineStmt (bytecode escrito a mano) y ReleaseOwnerStmt (sintetizado): nada que mirar
    }

    private Salida bucle(List<IStmt> cuerpo, Estado base, String nombre) {
        Salida sal = new Salida(true);
        sal.nombre = nombre;
        salidas.push(sal);
        try { desde(cuerpo, base); } finally { salidas.pop(); }
        return sal;
    }

    private Salida salidaDe(String nombre) {
        for (Salida s : salidas) if (nombre.equals(s.nombre)) return s;
        return null;
    }

    private static Estado noAlcanzado() { Estado e = new Estado(); e.vivo = false; return e; }

    private static boolean esCierto(IExpr c) {
        while (c instanceof ParenExpr) c = ((ParenExpr) c).inner;
        return (c instanceof BoolLitExpr) && ((BoolLitExpr) c).value;
    }

    private void declara(VarDecl v, Estado e) {
        if (v.init != null) { lee(v.init, e); return; }
        Symbol s0 = info.declSymbols.get(v);
        if (s0 instanceof Symbol.VarSymbol && ((Symbol.VarSymbol) s0).fixedArraySize >= 0) return;   // a cero
        for (DeclName n : v.names) {
            String k = n.name + "@" + n.line + ":" + n.column;
            vigiladas.add(k);
            e.con.remove(k);    // declarar sin valor (también de nuevo, en cada vuelta de un bucle)
        }
    }

    // ─── expresiones: sólo leen ───

    private void lee(IExpr x, Estado e) {
        if (x == null) return;
        if (x instanceof IdentifierExpr) {
            String k = clave((IdentifierExpr) x);
            if (k != null) usa((IdentifierExpr) x, k, e);
        } else if (x instanceof ParenExpr) {
            lee(((ParenExpr) x).inner, e);
        } else if (x instanceof MemberAccessExpr) {
            lee(((MemberAccessExpr) x).target, e);
        } else if (x instanceof IndexExpr) {
            lee(((IndexExpr) x).target, e); lee(((IndexExpr) x).index, e);
        } else if (x instanceof CallExpr) {
            lee(((CallExpr) x).callee, e);
            for (IExpr a : ((CallExpr) x).args) lee(a, e);
        } else if (x instanceof BinaryExpr) {
            lee(((BinaryExpr) x).left, e); lee(((BinaryExpr) x).right, e);
        } else if (x instanceof UnaryExpr) {
            lee(((UnaryExpr) x).operand, e);
        } else if (x instanceof ArrayLitExpr) {
            for (IExpr a : ((ArrayLitExpr) x).elements) lee(a, e);
        } else if (x instanceof TupleExpr) {
            for (IExpr a : ((TupleExpr) x).elements) lee(a, e);
        } else if (x instanceof InstanceOfExpr) {
            lee(((InstanceOfExpr) x).target, e);
        } else if (x instanceof SuperCallExpr) {
            for (IExpr a : ((SuperCallExpr) x).args) lee(a, e);
        } else if (x instanceof MethodRefExpr) {
            lee(((MethodRefExpr) x).target, e);
        } else if (x instanceof BoundCallExpr) {
            lee(((BoundCallExpr) x).target, e);
            for (IExpr a : ((BoundCallExpr) x).args) lee(a, e);
        }
        // literales, this, super, FieldExpr: nada que leer
    }

    /** La clave de una vigilada, o null si el identificador no es una de ellas. */
    private String clave(IdentifierExpr ie) {
        Symbol s = info.exprSymbols.get(ie);
        if (!(s instanceof Symbol.VarSymbol) || !((Symbol.VarSymbol) s).isLocal) return null;
        String k = s.name + "@" + s.line + ":" + s.column;
        return vigiladas.contains(k) ? k : null;
    }

    private void usa(IdentifierExpr ie, String k, Estado e) {
        if (!e.vivo || e.con.contains(k)) return;
        if (dichas.add(k)) {
            rep.err(ie.line, ie.column, "la variable local '" + ie.name + "' se usa sin tener valor: sin"
                    + " inicializador, su valor es indeterminado. Dale uno al declararla (var " + ie.name
                    + ": ... := ...) o asignala antes, en todos los caminos");
        }
        e.con.add(k);   // un error por variable: no se repite en cada uso
    }
}
