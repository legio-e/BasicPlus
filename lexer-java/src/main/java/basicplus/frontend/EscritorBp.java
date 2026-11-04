// ============================================================
// EscritorBp.java
// V8 #698 (JC, paso 1) — escribe BP desde el árbol: AST → BP.
//
// Es la primera pieza del traductor BP ↔ JC (V8_IDEAS §E2). El método es de
// Eduardo (17-oct): se escribe BP desde el árbol del original y se comprueba
// que el resultado es EQUIVALENTE —el mismo árbol sin contar líneas ni
// columnas—; los saltos de línea y los comentarios no hace falta respetarlos.
// Y por incrementos: lo que el escritor todavía no sabe escribir lo dice con el
// nombre del nodo (NoSoportado), nunca en silencio.
//
// El árbol está muy pegado a la sintaxis: el parser no desazucara salvo en
// cuatro sitios, y los cuatro se reconocen aquí —los grupos de un `var` (salen
// en líneas separadas), el cuerpo de las `native` externas de un pack
// (`isPackExtern`: sólo la firma), el `finally` de `try var` (`recurso`) y el
// `if`/`while`/`for` de una línea (`isSingleLine`)—. El `import Core` que pone
// el parser (`implicito`) no se escribe. Los paréntesis del original están en el
// árbol (`ParenExpr`), así que las expresiones se escriben sin añadir ninguno.
//
// Dos opciones del compilador (Main):
//   --escribir-bp   saca el BP reescrito por la salida
//   --comprobar-bp  la ida y vuelta: EQUIVALENTE, DISTINTO, NO SOPORTADO...;
//                   con una carpeta, la recorre entera y da las cuentas.
// ============================================================
package basicplus.frontend;

import basicplus.frontend.Ast.*;

import java.io.IOException;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public final class EscritorBp {

    /** Lo que el escritor todavía no sabe escribir: el nombre del nodo y su línea en el original. */
    public static final class NoSoportado extends RuntimeException {
        public final String nodo;
        public final int linea;
        NoSoportado(Object n) {
            super("no soportado: " + nombre(n) + " (línea " + lineaDe(n) + " del original)");
            this.nodo = nombre(n);
            this.linea = lineaDe(n);
        }
        private static String nombre(Object n) { return n == null ? "null" : n.getClass().getSimpleName(); }
        private static int lineaDe(Object n) { return (n instanceof Node) ? ((Node) n).line : 0; }
    }

    private static final String SANGRIA = "  ";

    private final StringBuilder sb = new StringBuilder();
    private int nivel;
    /** V8 H7.3 — las interfaces que declara el módulo: con ellas se decide, en un árbol de JC, si el primero de
     *  `class C : A, B` es el padre o una interfaz (sin el semántico no se sabe de las importadas: van de padre, y
     *  si eran una interfaz, el compilador de BP lo dice). */
    private final java.util.Set<String> interfacesLocales = new java.util.HashSet<>();

    private EscritorBp(int nivel) { this.nivel = nivel; }

    /** El BP de un módulo entero. Lanza {@link NoSoportado} si hay algo que todavía no sabe escribir. */
    public static String escribir(ModuleNode m) {
        EscritorBp e = new EscritorBp(0);
        for (ITopLevelDecl d : m.defs)
            if (d instanceof ClassDef && ((ClassDef) d).isInterface) e.interfacesLocales.add(((ClassDef) d).name);
        e.modulo(m);
        return e.sb.toString();
    }

    // ============================================================
    // LÍNEAS
    // ============================================================
    private void linea(String s) {
        for (int i = 0; i < nivel; i++) sb.append(SANGRIA);
        sb.append(s).append('\n');
    }

    private void blanco() { sb.append('\n'); }

    // ============================================================
    // MÓDULO E IMPORTS
    // ============================================================
    private void modulo(ModuleNode m) {
        if (m.library != null && !m.library.isEmpty()) linea("library " + cadena(m.library));
        StringBuilder h = new StringBuilder("module ");
        if (m.isInterface) h.append("interface ");
        h.append(m.name);
        if (m.implementsName != null) h.append(m.isInterface ? " extends " : " implements ").append(m.implementsName);
        linea(h.toString());
        nivel++;
        boolean hayImports = false;
        for (ImportNode imp : m.imports) {
            if (imp.implicito) continue;            // el `import Core` que pone el parser
            linea(importa(imp));
            hayImports = true;
        }
        if (hayImports) blanco();
        for (int i = 0; i < m.defs.size(); i++) {
            if (i > 0) blanco();
            declaracion(m.defs.get(i), m.isInterface);
        }
        nivel--;
        linea("end " + m.name);
    }

    private static String importa(ImportNode imp) {
        StringBuilder s = new StringBuilder("import ").append(String.join(".", imp.path));
        if (imp.boundImpl != null) s.append(" : ").append(imp.boundImpl);
        if (imp.packNativoMarca != null)  s.append(" from pack ").append(cadena(imp.packNativoMarca));
        else if (imp.packName != null)    s.append(" from pack ").append(imp.packName);
        else if (imp.fromPath != null)    s.append(" from ").append(cadena(imp.fromPath));
        return s.toString();
    }

    // ============================================================
    // DECLARACIONES (del módulo y miembros de clase)
    // ============================================================
    private void declaracion(ITopLevelDecl d, boolean enInterfaz) {
        Node n = (Node) d;
        for (Annotation a : n.annotations) linea(anotacion(a));
        if (d instanceof ConstDecl)        linea(constante((ConstDecl) d));
        else if (d instanceof VarDecl)     linea(variable((VarDecl) d));
        else if (d instanceof FuncDef)     funcion((FuncDef) d, enInterfaz);
        else if (d instanceof PropertyDef) propiedad((PropertyDef) d);
        else if (d instanceof EventDef)    linea(evento((EventDef) d));
        else if (d instanceof ClassDef)    clase((ClassDef) d, enInterfaz);
        else if (d instanceof EnumDef)     enumerado((EnumDef) d);
        else throw new NoSoportado(d);
    }

    /** También la usa EscritorJc: las anotaciones de JC son las de BP (V8_IDEAS §E2, punto 27). */
    static String anotacion(Annotation a) {
        StringBuilder s = new StringBuilder("@").append(a.name).append("{");
        for (int i = 0; i < a.entries.size(); i++) {
            AnnEntry e = a.entries.get(i);
            s.append(i == 0 ? " " : ", ").append(e.key);
            if (e.value != null) s.append(" = ").append(valorAnotacion(e.value));
            if (i == a.entries.size() - 1) s.append(" ");
        }
        return s.append("}").toString();
    }

    private static String valorAnotacion(Object v) {
        if (v instanceof String)  return cadena((String) v);
        if (v instanceof Boolean) return v.toString();
        if (v instanceof Long)    return v.toString();
        if (v instanceof Double)  return Double.toString((Double) v);
        throw new NoSoportado(v);
    }

    private static String visibilidad(boolean isPublic, boolean isPrivate) {
        return isPublic ? "public " : (isPrivate ? "private " : "");
    }

    private String constante(ConstDecl c) {
        StringBuilder s = new StringBuilder(visibilidad(c.isPublic, false)).append("const ").append(nombre(c.name));
        if (c.type != null) s.append(": ").append(tipo(c.type));
        return s.append(" := ").append(expr(c.value)).toString();
    }

    private String variable(VarDecl v) {
        StringBuilder s = new StringBuilder(visibilidad(v.isPublic, v.isPrivate)).append("var ");
        if (v.isOwner) s.append("owner ");
        for (int i = 0; i < v.names.size(); i++) {
            if (i > 0) s.append(", ");
            s.append(nombre(v.names.get(i)));
        }
        s.append(": ").append(tipo(v.type));
        if (v.init != null && !Ast.implicito(v.init)) s.append(" := ").append(expr(v.init));   // #774: el "" implícito, no
        return s.toString();
    }

    private void funcion(FuncDef f, boolean enInterfaz) {
        StringBuilder h = new StringBuilder(visibilidad(f.isPublic, f.isPrivate));
        if (f.isFinal)  h.append("final ");
        if (f.isNative) h.append("native ");
        h.append("function ");
        if (f.isEventHandler) h.append("event ");
        String nombre = f.isDestructor ? "~" + f.dtorEscrito : nombre(f.name);
        h.append(nombre).append("(").append(parametros(f.params)).append(")");
        if (f.returnType != null) h.append(": ").append(tipo(f.returnType));
        linea(h.toString());
        // Sin cuerpo ni `end`: la función de un `module interface` y la `native` externa de un pack
        // (el cuerpo que lanza se lo pone el parser al leerla).
        if (enInterfaz || f.isPackExtern) return;
        cuerpo(f.body);
        linea("end " + nombre);
    }

    private String parametros(List<Param> ps) {
        StringBuilder s = new StringBuilder();
        for (int i = 0; i < ps.size(); i++) {
            Param p = ps.get(i);
            if (i > 0) s.append(", ");
            s.append(p.name).append(": ").append(tipo(p.type));
            if (p.defaultValue != null) s.append(" := ").append(expr(p.defaultValue));
        }
        return s.toString();
    }

    private String evento(EventDef e) {
        return "event " + nombre(e.name) + "(" + parametros(e.params) + ")";
    }

    private void propiedad(PropertyDef p) {
        StringBuilder h = new StringBuilder(visibilidad(p.isPublic, p.isPrivate));
        if (p.isFinal) h.append("final ");
        if (p.isSync)  h.append("sync ");
        h.append("property ");
        if (p.isOwner) h.append("owner ");
        h.append(nombre(p.name)).append(": ").append(tipo(p.type));
        if (p.init != null && !Ast.implicito(p.init)) h.append(" := ").append(expr(p.init));   // #774: el "" implícito, no
        linea(h.toString());
        if (p.isShortForm) return;
        nivel++;
        if (p.getter != null) {
            linea("get");
            cuerpo(p.getter.body);
            linea("endget");
        }
        if (p.setter != null) {
            linea("set(" + p.setter.paramName + ")");
            cuerpo(p.setter.body);
            linea("endset");
        }
        nivel--;
        linea("endprop");
    }

    private void clase(ClassDef c, boolean enInterfaz) {
        StringBuilder h = new StringBuilder(c.isPublic ? "public " : "");
        if (c.isRoot) h.append("root ");
        // V8 H7.3 (#756) — una interfaz: `interface Forma extends Base`; una clase: `class C extends B implements I, J`.
        // En un árbol de JC, el primero de `: A, B` puede ser una interfaz: si lo es del módulo, va con las demás.
        String base = c.baseClass;
        List<String> interfaces = new ArrayList<>(c.interfaces);
        if (c.primeroPuedeSerInterfaz && base != null && interfacesLocales.contains(base)) {
            interfaces.add(0, base);
            base = null;
        }
        h.append(c.isInterface ? "interface " : "class ").append(c.name);
        if (base != null) h.append(" extends ").append(base);
        if (!interfaces.isEmpty())
            h.append(c.isInterface ? " extends " : " implements ").append(String.join(", ", interfaces));
        linea(h.toString());
        nivel++;
        for (int i = 0; i < c.members.size(); i++) {
            ITopLevelDecl m = c.members.get(i);
            if (i > 0 && !(m instanceof VarDecl && c.members.get(i - 1) instanceof VarDecl)) blanco();
            declaracion(m, enInterfaz || c.isInterface);   // en una interfaz, sólo la firma
        }
        nivel--;
        linea("end " + c.name);
    }

    private void enumerado(EnumDef e) {
        linea((e.isPublic ? "public " : "") + "enum " + e.name);
        nivel++;
        for (EnumValue v : e.values) linea(v.name + (v.explicitValue != null ? " := " + v.explicitValue : ""));
        nivel--;
        linea("end " + e.name);
    }

    private static String nombre(DeclName d) {
        return d.classQualifier != null ? d.classQualifier + "." + d.name : d.name;
    }

    // ============================================================
    // TIPOS
    // ============================================================
    private String tipo(TypeRef t) {
        if (t instanceof SimpleTypeRef) {
            SimpleTypeRef st = (SimpleTypeRef) t;
            if (st.argumentos == null) return st.name;
            List<String> as = new ArrayList<>();   // V8/H7.2 (#749): `List<Object>`, `Collections.Map<integer, Object>`
            for (TypeRef a : st.argumentos) as.add(tipo(a));
            return st.name + "<" + String.join(", ", as) + ">";
        }
        if (t instanceof ArrayTypeRef) {
            ArrayTypeRef a = (ArrayTypeRef) t;
            return tipo(a.element) + "[" + (a.size != null ? expr(a.size) : "") + "]";
        }
        if (t instanceof TupleTypeRef) {
            List<String> es = new ArrayList<>();
            for (TypeRef e : ((TupleTypeRef) t).elements) es.add(tipo(e));
            return "(" + String.join(", ", es) + ")";
        }
        throw new NoSoportado(t);
    }

    // ============================================================
    // SENTENCIAS
    // ============================================================
    private void cuerpo(List<IStmt> body) {
        nivel++;
        for (IStmt s : body) sentencia(s);
        nivel--;
    }

    private void sentencia(IStmt s) {
        if (s instanceof VarDecl)              linea(variable((VarDecl) s));
        else if (s instanceof ConstDecl)       linea(constante((ConstDecl) s));
        else if (s instanceof AssignStmt)      linea(asignacion((AssignStmt) s));
        else if (s instanceof DestructAssignStmt) linea(desempaquetado((DestructAssignStmt) s));
        else if (s instanceof ExprStmt)        linea(expr(((ExprStmt) s).expr));
        else if (s instanceof IfStmt)          si((IfStmt) s);
        else if (s instanceof WhileStmt)       mientras((WhileStmt) s);
        else if (s instanceof DoLoopStmt)      hacer((DoLoopStmt) s);
        else if (s instanceof ForStmt)         para((ForStmt) s);
        else if (s instanceof SwitchStmt)      segun((SwitchStmt) s);
        else if (s instanceof ParallelStmt)    paralelo((ParallelStmt) s);
        else if (s instanceof TryStmt)         intento((TryStmt) s);
        else if (s instanceof ReturnStmt)      linea(devolver((ReturnStmt) s));
        else if (s instanceof ThrowStmt)       linea("throw " + expr(((ThrowStmt) s).value));
        else if (s instanceof RaiseStmt)       linea(dispara((RaiseStmt) s));
        else if (s instanceof PrintStmt)       linea(imprime((PrintStmt) s));
        else if (s instanceof InlineStmt)      linea("inline " + ((InlineStmt) s).texto());
        else if (s instanceof BreakStmt)       linea(conNombre("break", ((BreakStmt) s).nombre));
        else if (s instanceof ContinueStmt)    linea(conNombre("continue", ((ContinueStmt) s).nombre));
        else throw new NoSoportado(s);
    }

    /** La sentencia de un `if`/`while`/`for` de una línea, en la MISMA línea que su cabecera. Si
     *  ocupa más de una (un bloque que empieza ahí), las demás siguen debajo con su sangría. */
    private void enLinea(String cabecera, IStmt s) {
        EscritorBp sub = new EscritorBp(nivel);
        sub.sentencia(s);
        String[] ls = sub.sb.toString().split("\n", -1);
        linea(cabecera + " " + ls[0].trim());
        for (int i = 1; i < ls.length; i++) if (!ls[i].isEmpty()) sb.append(ls[i]).append('\n');
    }

    private String asignacion(AssignStmt a) {
        return expr(a.target) + " " + a.op.enBp() + " " + expr(a.value);   // `:=`, `+=`, `mod=`… (V8/H7.1 #747)
    }

    private String desempaquetado(DestructAssignStmt d) {
        return "{ " + lista(d.targets) + " } := " + expr(d.value);
    }

    private void si(IfStmt s) {
        if (s.isSingleLine) {
            if (s.then_.body.size() != 1 || !s.elseIfs.isEmpty() || s.else_ != null) throw new NoSoportado(s);
            enLinea("if " + expr(s.then_.condition) + " then", s.then_.body.get(0));
            return;
        }
        linea("if " + expr(s.then_.condition) + " then");
        cuerpo(s.then_.body);
        for (IfClause c : s.elseIfs) {
            linea("elseif " + expr(c.condition) + " then");
            cuerpo(c.body);
        }
        if (s.else_ != null) {
            linea("else");
            cuerpo(s.else_);
        }
        linea("endif");
    }

    private void mientras(WhileStmt w) {
        String cab = "while " + expr(w.condition) + " do";
        if (w.isSingleLine) {
            if (w.body.size() != 1) throw new NoSoportado(w);
            enLinea(cab, w.body.get(0));
            return;
        }
        linea(cab);
        cuerpo(w.body);
        linea("endwh");
    }

    private static String conNombre(String palabra, String nombre) { return nombre != null ? palabra + " " + nombre : palabra; }

    private void hacer(DoLoopStmt d) {
        linea(conNombre("do", d.nombre));
        cuerpo(d.body);
        linea(d.condition != null ? "loop " + expr(d.condition) : "loop");
    }

    private void para(ForStmt f) {
        String cab;
        if (f.range instanceof ForNumericRange) {
            ForNumericRange r = (ForNumericRange) f.range;
            cab = "for " + f.iteratorName + " := " + expr(r.from) + " to " + expr(r.to)
                    + (r.step != null ? " step " + expr(r.step) : "") + " do";
        } else if (f.range instanceof ForInRange) {
            cab = "for " + f.iteratorName + " in " + expr(((ForInRange) f.range).iterable) + " do";
        } else {
            throw new NoSoportado(f.range);
        }
        if (f.isSingleLine) {
            if (f.body.size() != 1) throw new NoSoportado(f);
            enLinea(cab, f.body.get(0));
            return;
        }
        linea(cab);
        cuerpo(f.body);
        linea("next");
    }

    private void segun(SwitchStmt s) {
        linea("switch " + expr(s.subject));
        nivel++;
        for (CaseClause c : s.cases) {
            linea("case " + lista(c.values));
            cuerpo(c.body);
        }
        if (s.defaultBody != null) {
            linea("default");
            cuerpo(s.defaultBody);
        }
        nivel--;
        linea("endsw");
    }

    private void paralelo(ParallelStmt p) {
        linea("parallel");
        nivel++;
        for (ParallelBranch b : p.branches) {
            linea("case " + b.varName + " := Thread(" + expr(b.stackSizeExpr) + ")");
            cuerpo(b.body);
        }
        if (p.defaultBody != null) {
            linea("default");
            cuerpo(p.defaultBody);
        }
        nivel--;
        linea("endpar");
    }

    private void intento(TryStmt t) {
        if (t.recurso != null) {
            VarDecl r = t.recurso;
            if (r.names.size() != 1) throw new NoSoportado(t);
            linea("try var " + nombre(r.names.get(0)) + ": " + tipo(r.type) + " := " + expr(r.init));
        } else {
            linea("try");
        }
        cuerpo(t.body);
        for (CatchClause c : t.catches) {
            if (c.varName == null) linea("catch");
            else linea("catch " + c.varName + (c.exceptionType != null ? ": " + c.exceptionType : ""));
            cuerpo(c.body);
        }
        List<IStmt> fin = t.finallyBody;
        if (fin != null && t.recurso != null) {
            // El parser añade al final el ReleaseOwnerStmt que suelta el recurso: no se escribe.
            IStmt ultima = fin.isEmpty() ? null : fin.get(fin.size() - 1);
            if (!(ultima instanceof ReleaseOwnerStmt)) throw new NoSoportado(t);
            fin = fin.subList(0, fin.size() - 1);
            if (fin.isEmpty()) fin = null;
        }
        if (fin != null) {
            linea("finally");
            cuerpo(fin);
        }
        linea("endtry");
    }

    private String devolver(ReturnStmt r) {
        if (r.value == null) return "return";
        return "return " + expr(r.value);
    }

    private String dispara(RaiseStmt r) {
        return "raise " + r.event + "(" + lista(r.args) + ")";
    }

    private String imprime(PrintStmt p) {
        StringBuilder s = new StringBuilder("print");
        for (PrintItem it : p.items) {
            switch (it.leadingSep) {
                case COMMA:     s.append(","); break;
                case SEMICOLON: s.append(";"); break;
                default: break;
            }
            if (it.expr != null) s.append(' ').append(expr(it.expr));
        }
        return s.toString();
    }

    // ============================================================
    // EXPRESIONES
    // ============================================================
    private String lista(List<IExpr> es) {
        List<String> out = new ArrayList<>();
        for (IExpr e : es) out.add(expr(e));
        return String.join(", ", out);
    }

    private String expr(IExpr e) {
        if (e instanceof IntLitExpr)       return Long.toString(((IntLitExpr) e).value);
        if (e instanceof LongLitExpr)      return Long.toString(((LongLitExpr) e).value) + "L";
        if (e instanceof FloatLitExpr)     return Double.toString(((FloatLitExpr) e).value);
        if (e instanceof DoubleLitExpr)    return Double.toString(((DoubleLitExpr) e).value) + "d";
        if (e instanceof StringLitExpr)    return cadena(((StringLitExpr) e).value);
        if (e instanceof BoolLitExpr)      return ((BoolLitExpr) e).value ? "true" : "false";
        if (e instanceof NullLitExpr)      return "null";
        if (e instanceof ThisExpr)         return "this";
        if (e instanceof FieldExpr)        return "field";
        if (e instanceof SuperExpr)        return "super";
        if (e instanceof SuperCallExpr)    return "super(" + lista(((SuperCallExpr) e).args) + ")";
        if (e instanceof IdentifierExpr)   return ((IdentifierExpr) e).name;
        if (e instanceof ArrayLitExpr)     return "[" + lista(((ArrayLitExpr) e).elements) + "]";
        if (e instanceof ParenExpr)        return "(" + expr(((ParenExpr) e).inner) + ")";
        if (e instanceof TupleExpr)        return "(" + lista(((TupleExpr) e).elements) + ")";
        if (e instanceof MemberAccessExpr) {
            MemberAccessExpr m = (MemberAccessExpr) e;
            return expr(m.target) + "." + m.member;
        }
        if (e instanceof IndexExpr) {
            IndexExpr i = (IndexExpr) e;
            return expr(i.target) + "[" + expr(i.index) + "]";
        }
        if (e instanceof CallExpr) {
            CallExpr c = (CallExpr) e;
            return expr(c.callee) + "(" + lista(c.args) + ")";
        }
        if (e instanceof UnaryExpr) {
            UnaryExpr u = (UnaryExpr) e;
            return "not".equals(u.op) ? "not " + expr(u.operand) : u.op + expr(u.operand);
        }
        if (e instanceof BinaryExpr) {
            BinaryExpr b = (BinaryExpr) e;
            return expr(b.left) + " " + b.op + " " + expr(b.right);
        }
        if (e instanceof InstanceOfExpr) {
            InstanceOfExpr i = (InstanceOfExpr) e;
            return expr(i.target) + " instanceof " + i.typeName;
        }
        if (e instanceof MethodRefExpr) {
            MethodRefExpr m = (MethodRefExpr) e;
            return expr(m.target) + "::" + m.method;
        }
        if (e instanceof BoundCallExpr) {
            BoundCallExpr b = (BoundCallExpr) e;
            return expr(b.target) + "::" + b.method + "(" + lista(b.args) + ")";
        }
        throw new NoSoportado(e);
    }

    /** Un literal de cadena con los escapes que entiende el lexer: \n \t \r \\ \" \0. */
    static String cadena(String v) {
        StringBuilder s = new StringBuilder("\"");
        for (int i = 0; i < v.length(); i++) {
            char c = v.charAt(i);
            switch (c) {
                case '\\': s.append("\\\\"); break;
                case '"':  s.append("\\\""); break;
                case '\n': s.append("\\n"); break;
                case '\t': s.append("\\t"); break;
                case '\r': s.append("\\r"); break;
                case '\0': s.append("\\0"); break;
                default:   s.append(c);
            }
        }
        return s.append('"').toString();
    }

    // ============================================================
    // LAS DOS OPCIONES DEL COMPILADOR
    // ============================================================

    /** Resultado de leer un fuente: el árbol, o el primer error de léxico o de sintaxis. */
    private static final class Leido {
        ModuleNode modulo;
        String error;
    }

    private static Leido leer(String fuente) {
        Leido r = new Leido();
        Lexer lexer = new Lexer(fuente);
        List<Token> tokens = lexer.tokenize();
        if (!lexer.getErrors().isEmpty()) { r.error = lexer.getErrors().get(0).toString(); return r; }
        Parser parser = new Parser(tokens);
        r.modulo = parser.parseModule();
        if (!parser.getErrors().isEmpty()) { r.error = parser.getErrors().get(0).toString(); r.modulo = null; }
        else if (r.modulo == null) r.error = "el parser no devolvió módulo";
        return r;
    }

    /** V8 #698 — un `.jc`, por el parser de JC: al mismo árbol, así que lo que sale es la traducción JC → BP. */
    private static Leido leerJc(String fuente) {
        Leido r = new Leido();
        try {
            r.modulo = JcParser.leer(fuente);
        } catch (JcLexer.Error e) {
            r.error = e.getMessage();
        }
        return r;
    }

    /** V8 #698 — el BP de un fichero, para quien lo pide desde dentro (`Traductor.aBp`, el IDE): de un `.jc`, su
     *  traducción; de un `.bp`, el mismo reescrito. El texto, o por qué no. */
    static Traductor.Resultado traducir(Path p) throws IOException {
        String fuente = new String(Files.readAllBytes(p), StandardCharsets.UTF_8);
        Leido l = Main.esJc(p) ? leerJc(fuente) : leer(fuente);
        if (l.modulo == null) return new Traductor.Resultado(null, "el original no se lee: " + l.error);
        try {
            return new Traductor.Resultado(escribir(l.modulo), null);
        } catch (NoSoportado ns) {
            return new Traductor.Resultado(null, ns.getMessage());
        }
    }

    /** --escribir-bp: el BP reescrito, por la salida; de un `.jc`, su traducción a BP. 0 = bien; 2 = no soportado;
     *  3 = el original no se lee. */
    static int escribirFichero(Path p) throws IOException {
        PrintStream out = new PrintStream(new java.io.FileOutputStream(java.io.FileDescriptor.out), true, "UTF-8");
        String fuente = new String(Files.readAllBytes(p), StandardCharsets.UTF_8);
        Leido l = Main.esJc(p) ? leerJc(fuente) : leer(fuente);
        if (l.modulo == null) { System.err.println("el original no se lee: " + l.error); return 3; }
        try {
            out.print(escribir(l.modulo));
            return 0;
        } catch (NoSoportado ns) {
            System.err.println(ns.getMessage());
            return 2;
        }
    }

    /** --comprobar-bp: la ida y vuelta de un fichero, o de todos los .bp de una carpeta. */
    static int comprobar(Path p) throws IOException {
        PrintStream out = new PrintStream(new java.io.FileOutputStream(java.io.FileDescriptor.out), true, "UTF-8");
        if (!Files.isDirectory(p)) {
            String r = comprobarFichero(p);
            out.println(p.getFileName() + ": " + r);
            return r.startsWith("EQUIVALENTE") ? 0 : 1;
        }
        List<Path> fs;
        try (Stream<Path> w = Files.walk(p)) {
            fs = w.filter(x -> x.toString().endsWith(".bp")).sorted().collect(Collectors.toList());
        }
        Map<String, Integer> cuentas = new TreeMap<>();
        Map<String, Integer> nodos = new TreeMap<>();
        for (Path f : fs) {
            String r = comprobarFichero(f);
            String clase = r.split(":")[0];
            cuentas.merge(clase, 1, Integer::sum);
            if (r.startsWith("NO SOPORTADO")) nodos.merge(r.substring("NO SOPORTADO: no soportado: ".length()).split(" ")[0], 1, Integer::sum);
            if (!r.startsWith("EQUIVALENTE")) out.println(p.relativize(f) + ": " + r);
        }
        out.println("---- " + fs.size() + " ficheros: " + cuentas);
        if (!nodos.isEmpty()) out.println("---- no soportados, por nodo: " + nodos);
        return cuentas.getOrDefault("EQUIVALENTE", 0) == fs.size() ? 0 : 1;
    }

    /** EQUIVALENTE | DISTINTO: ... | NO SOPORTADO: ... | INESTABLE: ... | NO SE LEE: ... (el original no parsea). */
    static String comprobarFichero(Path f) throws IOException {
        Leido a = leer(new String(Files.readAllBytes(f), StandardCharsets.UTF_8));
        if (a.modulo == null) return "NO SE LEE: " + a.error;
        String bp;
        try { bp = escribir(a.modulo); }
        catch (NoSoportado ns) { return "NO SOPORTADO: " + ns.getMessage(); }
        catch (RuntimeException ex) { return "DISTINTO: el escritor falló: " + ex; }
        Leido b = leer(bp);
        if (b.modulo == null) return "DISTINTO: el BP reescrito no se lee: " + b.error;
        String dif = ComparaAst.primeraDiferencia(a.modulo, b.modulo);
        if (dif != null) return "DISTINTO: " + dif;
        String otraVez = escribir(b.modulo);
        if (!otraVez.equals(bp)) return "INESTABLE: escribir el BP reescrito no da el mismo texto";
        return "EQUIVALENTE";
    }
}
