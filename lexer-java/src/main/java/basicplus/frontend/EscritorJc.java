// ============================================================
// EscritorJc.java
// V8 #698 (JC, paso 2) — escribe JC desde el árbol de BP: AST → JC.
//
// JC es BP con la ropa de Java y C# (V8_IDEAS §E2): el mismo árbol, escrito con
// la sintaxis que decidió Eduardo el 17-oct, pieza a pieza (§E2, «La sintaxis de
// JC, según la decide Eduardo», puntos 1–32, y el ejemplo de la primera fase).
// Ésta es la primera fase: las clases, el `switch` y `parallel` van a una
// segunda, y lo que todavía no tiene forma en JC se dice con su motivo y su
// línea (NoSoportado), nunca en silencio.
//
// A diferencia de EscritorBp, éste escribe desde el árbol ANALIZADO
// (Main.analizarParaTraducir). En BP crear, llamar y convertir se escriben
// igual —`List()`, `f()`, `integer(x)`, `Persona(o)`— y sólo el análisis sabe
// qué es cada uno; en JC son `new List()`, `f()`, `(int) x` y `(Persona) o`. Y
// una constante que en BP no escribe su tipo lo lleva en JC: el que deduce el
// compilador.
//
// La precedencia es la de C# (punto 18). El árbol de BP sigue la de BP, así que
// donde las dos no casan —los operadores de bits, los desplazamientos y el
// `not`— el escritor pone los paréntesis que hagan falta. Los que escribió el
// programador están en el árbol (ParenExpr) y salen tal cual.
//
// La opción del compilador (Main): --escribir-jc, con un fichero (el JC por la
// salida) o con una carpeta (las cuentas: escritos, no soportados por motivo, y
// los que no se analizan).
// ============================================================
package basicplus.frontend;

import basicplus.frontend.Ast.*;
import basicplus.frontend.BpType.ArrayType;
import basicplus.frontend.BpType.ClassType;
import basicplus.frontend.BpType.EnumType;
import basicplus.frontend.BpType.PrimitiveType;

import java.io.IOException;
import java.io.PrintStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public final class EscritorJc {

    /** Lo que JC todavía no sabe escribir: el motivo y la línea en el original. */
    public static final class NoSoportado extends RuntimeException {
        public final String motivo;
        public final int linea;
        NoSoportado(String motivo, Object n) {
            super("no soportado: " + motivo + " (línea " + lineaDe(n) + " del original)");
            this.motivo = motivo;
            this.linea = lineaDe(n);
        }
        private static int lineaDe(Object n) { return (n instanceof Node) ? ((Node) n).line : 0; }
    }

    /** Tres espacios, como el ejemplo de Eduardo (V8_IDEAS §E2, punto 2). */
    private static final String SANGRIA = "   ";

    /** Lo que en JC es palabra reservada y en BP puede ser un nombre. El traductor lo dice, sin renombrar nada (§E2,
     *  «Lo que choca»). Las demás palabras de JC ya lo son en BP, que no las deja usar como nombre. */
    private static final Set<String> RESERVADAS_SOLO_EN_JC = new HashSet<>(Arrays.asList(
            "int", "long", "float", "double", "bool", "string", "sbyte", "ushort",
            "void", "new", "is", "static", "foreach", "protected", "base", "println"));

    /** Las conversiones a un tipo primitivo, que en BP son una llamada con el nombre del tipo (`integer(x)`, `byte(x)`).
     *  El analizador las reconoce por el nombre (numericCastTarget; las de los estrechos, el parser). */
    private static final Set<String> CONVERSIONES_PRIMITIVAS = new HashSet<>(Arrays.asList(
            "integer", "long", "float", "double", "byte", "int8", "word", "int16"));

    // La precedencia de C#, de la más floja a la más fuerte. La potencia, la de BP (y la de Python): más fuerte que el
    // menos, con la base primaria y el exponente unario (punto 18).
    private static final int P_TERNARIO = 0, P_O = 1, P_Y = 2, P_OBIT = 3, P_XOR = 4, P_YBIT = 5, P_IGUAL = 6, P_REL = 7,
            P_DESPL = 8, P_SUMA = 9, P_MULT = 10, P_UNARIO = 11, P_POT = 12, P_PRIMARIO = 13;

    private final StringBuilder sb = new StringBuilder();
    private final SemanticInfo info;
    private final Map<Object, Integer> argsEscritos;
    private int nivel;
    /** La clase que se está escribiendo, o null en el módulo. */
    private String clase;
    /** V8 H7.3 — se escribe una interfaz: sus miembros, sin `protected` (son de quien la implementa) y sin cuerpo. */
    private boolean enInterfazDeClase;

    private EscritorJc(SemanticInfo info, Map<Object, Integer> argsEscritos, int nivel) {
        this.info = info;
        this.argsEscritos = argsEscritos;
        this.nivel = nivel;
    }

    /** El JC de un módulo entero, desde su árbol analizado. {@code argsEscritos}: cuántos argumentos escribió el
     *  programador en cada llamada, contados antes del análisis ({@link #contarArgumentos}). Lanza {@link NoSoportado}
     *  si hay algo que JC todavía no sabe escribir. */
    public static String escribir(ModuleNode m, SemanticInfo info, Map<Object, Integer> argsEscritos) {
        EscritorJc e = new EscritorJc(info, argsEscritos, 0);
        e.modulo(m);
        return e.sb.toString();
    }

    /** Apunta, antes del análisis, cuántos argumentos lleva cada llamada del árbol: el análisis añade al final los
     *  valores por defecto que faltan (H8.1, `checkArgs`), y lo que se traduce es lo que escribió el programador. */
    static void contarArgumentos(Object n, Map<Object, Integer> out) {
        if (n == null) return;
        if (n instanceof List) {
            for (Object o : (List<?>) n) contarArgumentos(o, out);
            return;
        }
        if (!n.getClass().getName().startsWith("basicplus.frontend.Ast$")) return;
        if (n instanceof CallExpr)      out.put(n, ((CallExpr) n).args.size());
        if (n instanceof RaiseStmt)     out.put(n, ((RaiseStmt) n).args.size());
        if (n instanceof BoundCallExpr) out.put(n, ((BoundCallExpr) n).args.size());
        if (n instanceof SuperCallExpr) out.put(n, ((SuperCallExpr) n).args.size());
        for (Class<?> c = n.getClass(); c != null && c != Object.class; c = c.getSuperclass()) {
            for (java.lang.reflect.Field f : c.getDeclaredFields()) {
                if (java.lang.reflect.Modifier.isStatic(f.getModifiers()) || f.getType().isPrimitive()) continue;
                f.setAccessible(true);
                try {
                    contarArgumentos(f.get(n), out);
                } catch (IllegalAccessException ignored) {
                    // un campo que no se lee no tiene llamadas que contar
                }
            }
        }
    }

    /** Los argumentos que escribió el programador. */
    private List<IExpr> escritos(CallExpr c) { return escritos(c, c.args); }

    private List<IExpr> escritos(Object llamada, List<IExpr> args) {
        Integer n = argsEscritos.get(llamada);
        return (n == null || n >= args.size()) ? args : args.subList(0, n);
    }

    // ============================================================
    // LÍNEAS
    // ============================================================
    private void linea(String s) {
        for (int i = 0; i < nivel; i++) sb.append(SANGRIA);
        sb.append(s).append('\n');
    }

    private void blanco() { sb.append('\n'); }

    /** `{` … `}`, cada llave en su línea, como en el ejemplo de Eduardo. */
    private void bloque(List<IStmt> body) {
        linea("{");
        nivel++;
        for (IStmt s : body) sentencia(s);
        nivel--;
        linea("}");
    }

    // ============================================================
    // MÓDULO E IMPORTS
    // ============================================================
    private void modulo(ModuleNode m) {
        if (m.library != null && !m.library.isEmpty()) linea("library " + cadena(m.library) + ";");
        StringBuilder h = new StringBuilder("module ");
        if (m.isInterface) h.append("interface ");
        h.append(nombre(m.name, m));
        if (m.implementsName != null) h.append(m.isInterface ? " extends " : " implements ").append(m.implementsName);
        linea(h.toString());
        linea("{");
        nivel++;
        boolean hayImports = false;
        for (ImportNode imp : m.imports) {
            if (imp.implicito) continue;            // el `import Core` que pone el parser
            linea(importa(imp));
            hayImports = true;
        }
        if (hayImports) blanco();
        for (int i = 0; i < m.defs.size(); i++) {
            ITopLevelDecl d = m.defs.get(i);
            // Las de una línea (constantes, variables y propiedades cortas) van juntas; lo demás, separado.
            if (i > 0 && !(deUnaLinea(m.defs.get(i - 1)) && deUnaLinea(d))) blanco();
            declaracion(d, m.isInterface);
        }
        nivel--;
        linea("}");
    }

    private static boolean deUnaLinea(ITopLevelDecl d) {
        if (!((Node) d).annotations.isEmpty()) return false;
        return d instanceof ConstDecl || d instanceof VarDecl || d instanceof EventDef
            || (d instanceof PropertyDef && ((PropertyDef) d).isShortForm);
    }

    private static String importa(ImportNode imp) {
        StringBuilder s = new StringBuilder("import ").append(String.join(".", imp.path));
        if (imp.boundImpl != null) s.append(" : ").append(imp.boundImpl);
        if (imp.packNativoMarca != null)  s.append(" from pack ").append(cadena(imp.packNativoMarca));
        else if (imp.packName != null)    s.append(" from pack ").append(imp.packName);
        else if (imp.fromPath != null)    s.append(" from ").append(cadena(imp.fromPath));
        return s.append(';').toString();
    }

    // ============================================================
    // DECLARACIONES
    // ============================================================
    /** Una declaración del módulo, o un miembro de la clase que se está escribiendo ({@link #clase}). */
    private void declaracion(ITopLevelDecl d, boolean enInterfaz) {
        Node n = (Node) d;
        for (Annotation a : n.annotations) linea(EscritorBp.anotacion(a));   // las de BP (punto 27)
        if (d instanceof ConstDecl)        linea(constante((ConstDecl) d, true) + ";");
        else if (d instanceof VarDecl)     linea(variable((VarDecl) d, true) + ";");
        else if (d instanceof FuncDef)     funcion((FuncDef) d, enInterfaz);
        else if (d instanceof PropertyDef) propiedad((PropertyDef) d);
        else if (d instanceof EnumDef)     linea(enumerado((EnumDef) d));
        else if (d instanceof ClassDef)    claseDef((ClassDef) d, enInterfaz);
        else if (d instanceof EventDef)    linea(evento((EventDef) d));
        else throw new NoSoportado(d.getClass().getSimpleName(), d);
    }

    /** En el módulo, `public`, `private` o nada. En una clase, lo que BP no marca es protegido (lo ven la clase y sus
     *  hijas), y JC lo escribe: `protected` (punto 34). */
    private String visibilidad(boolean isPublic, boolean isPrivate) {
        if (isPublic) return "public ";
        if (isPrivate) return "private ";
        return clase != null && !enInterfazDeClase ? "protected " : "";
    }

    /** `static`, para lo que en BP lleva delante el nombre de su clase (punto 20); ese nombre es el de la clase en la
     *  que está, y en JC no se escribe. */
    private String estatico(DeclName n, Object donde) {
        if (n.classQualifier == null) return "";
        if (clase == null || !n.classQualifier.equals(clase))
            throw new NoSoportado("un miembro con el nombre de otra clase delante (" + n.classQualifier + ")", donde);
        return "static ";
    }

    /** `public class Perro : Animal, IMascota { … }` (punto 34), con `root` delante si BP lo pone; y V8 H7.3 (`#756`),
     *  `public interface IForma : IBase { … }`, con sus funciones sin cuerpo. Como en C#, el padre y las interfaces van
     *  en la misma lista, el padre primero. */
    private void claseDef(ClassDef c, boolean enInterfaz) {
        if (clase != null) throw new NoSoportado("una clase dentro de otra", c);
        StringBuilder h = new StringBuilder(c.isPublic ? "public " : "");
        if (c.isRoot) h.append("root ");
        h.append(c.isInterface ? "interface " : "class ").append(nombre(c.name, c));
        List<String> lista = new ArrayList<>();
        if (c.baseClass != null) lista.add(c.baseClass);
        lista.addAll(c.interfaces);
        if (!lista.isEmpty()) h.append(" : ").append(String.join(", ", lista));
        linea(h.toString());
        linea("{");
        nivel++;
        clase = c.name;
        enInterfazDeClase = c.isInterface;
        for (int i = 0; i < c.members.size(); i++) {
            ITopLevelDecl m = c.members.get(i);
            if (i > 0 && !(deUnaLinea(c.members.get(i - 1)) && deUnaLinea(m))) blanco();
            declaracion(m, enInterfaz || c.isInterface);
        }
        enInterfazDeClase = false;
        clase = null;
        nivel--;
        linea("}");
    }

    /** `event cambia(Medida m);` y `raise cambia(this);` (punto 34). */
    private String evento(EventDef e) {
        return estatico(e.name, e) + "event " + nombre(e.name.name, e) + "(" + parametros(e.params) + ");";
    }

    /** `const int MAX = 10`. El tipo, siempre, como en C#: si BP no lo escribe, el que deduce el compilador (punto 29).
     *  En una clase, BP le pone delante el nombre de la clase; en C# una constante ya es de la clase, y JC no lo
     *  escribe (punto 34). {@code miembro}: del módulo o de la clase, y no de un cuerpo. */
    private String constante(ConstDecl c, boolean miembro) {
        boolean deClase = c.name.classQualifier != null;
        if (deClase && !(miembro && clase != null && c.name.classQualifier.equals(clase)))
            throw new NoSoportado("una constante con el nombre de otra clase delante (" + c.name.classQualifier + ")", c);
        if (!deClase && miembro && clase != null)
            throw new NoSoportado("una constante de clase sin el nombre de la clase delante", c);
        String t;
        if (c.type != null) {
            t = tipo(c.type);
        } else {
            Symbol s = info.declSymbols.get(c);
            BpType bt = (s instanceof Symbol.ConstSymbol) ? ((Symbol.ConstSymbol) s).type : null;
            if (bt == null) bt = info.exprTypes.get(c.value);
            t = tipoDeducido(bt, c);
        }
        return (miembro ? visibilidad(c.isPublic, false) : "") + "const " + t + " " + nombre(c.name.name, c) + " = "
             + expr(c.value);
    }

    /** `int a, b`, `owner Persona p = …`. El valor de BP es de todos los nombres, y en JC se repite: `int a = 3, b = 3`
     *  (punto 5). Un array de tamaño fijo, como en C: el tamaño detrás de cada nombre, `int nums[10]` (punto 33).
     *  {@code miembro}: del módulo o un campo de la clase, con su visibilidad; si no, una local. */
    private String variable(VarDecl v, boolean miembro) {
        if (v.type == null) throw new NoSoportado("una variable sin tipo", v);
        StringBuilder s = new StringBuilder(miembro ? visibilidad(v.isPublic, v.isPrivate) : "");
        String estat = v.names.isEmpty() ? "" : estatico(v.names.get(0), v);
        for (DeclName dn : v.names)
            if (!estatico(dn, v).equals(estat)) throw new NoSoportado("un var con nombres de clase y de objeto", v);
        if (!miembro && !estat.isEmpty()) throw new NoSoportado("una variable local con el nombre de una clase", v);
        s.append(estat);
        if (v.isOwner) s.append("owner ");
        String tamano = "";
        TypeRef t = v.type;
        if (t instanceof ArrayTypeRef && ((ArrayTypeRef) t).size != null) {
            tamano = "[" + expr(((ArrayTypeRef) t).size) + "]";
            t = ((ArrayTypeRef) t).element;
        }
        s.append(tipo(t)).append(' ');
        String valor = v.init != null && !Ast.implicito(v.init) ? expr(v.init) : null;   // #774: el "" implícito, no
        for (int i = 0; i < v.names.size(); i++) {
            DeclName dn = v.names.get(i);
            if (i > 0) s.append(", ");
            s.append(nombre(dn.name, v)).append(tamano);
            if (valor != null) s.append(" = ").append(valor);
        }
        return s.toString();
    }

    /**
     * `function int f(int a, int b = 1)` y su bloque; `void` si no devuelve nada (punto 20). Sin cuerpo (interfaz,
     * native de un pack), con `;`; si el cuerpo es una instrucción, `inline … ;` (punto 26). En una clase (punto 34):
     * el constructor, sin tipo, con la primera sentencia de BP —`super(…)` o la delegación `Clase(…)`— como el
     * `: base(…)` y el `: this(…)` de C#; el destructor, `function ~Clase()`; y `static` lo de la clase.
     */
    private void funcion(FuncDef f, boolean enInterfaz) {
        boolean ctor = clase != null && f.name.classQualifier == null && f.name.name.equals(clase) && !f.isDestructor;
        StringBuilder h = new StringBuilder();
        // El destructor de C# no lleva marca: sólo se escribe la que ponga BP.
        if (f.isDestructor) h.append(f.isPublic ? "public " : (f.isPrivate ? "private " : ""));
        else h.append(visibilidad(f.isPublic, f.isPrivate));
        h.append(estatico(f.name, f));
        if (f.isFinal)  h.append("final ");
        if (f.isNative) h.append("native ");
        h.append("function ");
        if (f.isEventHandler) h.append("event ");
        List<IStmt> body = f.body;
        String inicial = "";
        if (f.isDestructor || ctor) {
            if (f.returnType != null) throw new NoSoportado("un constructor o un destructor con tipo", f);
            h.append(f.isDestructor ? "~" + nombre(f.dtorEscrito, f) : nombre(f.name.name, f));
            IExpr primera = (ctor && !body.isEmpty() && body.get(0) instanceof ExprStmt) ? ((ExprStmt) body.get(0)).expr : null;
            if (primera instanceof SuperCallExpr) {
                inicial = " : base(" + lista(escritos(primera, ((SuperCallExpr) primera).args)) + ")";
                body = body.subList(1, body.size());
            } else if (primera instanceof CallExpr && ((CallExpr) primera).callee instanceof IdentifierExpr
                    && ((IdentifierExpr) ((CallExpr) primera).callee).name.equals(clase)) {
                inicial = " : this(" + lista(escritos(primera, ((CallExpr) primera).args)) + ")";
                body = body.subList(1, body.size());
            }
        } else {
            h.append(f.returnType != null ? tipo(f.returnType) : "void").append(' ').append(nombre(f.name.name, f));
        }
        h.append('(').append(parametros(f.params)).append(')').append(inicial);
        if (enInterfaz || f.isPackExtern) {
            linea(h + ";");
            return;
        }
        if (!ctor && !f.isDestructor && body.size() == 1 && body.get(0) instanceof InlineStmt) {
            linea(h + " inline " + ((InlineStmt) body.get(0)).texto() + ";");
            return;
        }
        linea(h.toString());
        bloque(body);
    }

    private String parametros(List<Param> ps) {
        List<String> out = new ArrayList<>();
        for (Param p : ps) {
            String s = tipo(p.type) + " " + nombre(p.name, p);
            if (p.defaultValue != null) s += " = " + expr(p.defaultValue);
            out.add(s);
        }
        return String.join(", ", out);
    }

    /**
     * La propiedad corta de BP es la automática de C#: `public int n { get; set; }`, y con valor, `… = 0;` (punto 32).
     * La larga, con su `get { … }` y su `set { … }` (punto 34): el `set` de C#, sin nombre, cuando BP llama `value` a su
     * parámetro; si no, `set(v) { … }`, con el nombre de BP.
     */
    private void propiedad(PropertyDef p) {
        StringBuilder s = new StringBuilder(visibilidad(p.isPublic, p.isPrivate));
        s.append(estatico(p.name, p));
        if (p.isFinal) s.append("final ");
        if (p.isSync)  s.append("sync ");
        if (p.isOwner) s.append("owner ");
        s.append(tipo(p.type)).append(' ').append(nombre(p.name.name, p));
        if (p.isShortForm) {
            s.append(" { get; set; }");
            if (p.init != null && !Ast.implicito(p.init)) s.append(" = ").append(expr(p.init)).append(';');
            linea(s.toString());
            return;
        }
        linea(s.toString());
        linea("{");
        nivel++;
        if (p.getter != null) {
            linea("get");
            bloque(p.getter.body);
        }
        if (p.setter != null) {
            linea("value".equals(p.setter.paramName) ? "set" : "set(" + nombre(p.setter.paramName, p) + ")");
            bloque(p.setter.body);
        }
        nivel--;
        linea(p.init != null && !Ast.implicito(p.init) ? "} = " + expr(p.init) + ";" : "}");
    }

    /** `public enum Color { Rojo, Verde = 5 }` (punto 25). */
    private static String enumerado(EnumDef e) {
        List<String> vs = new ArrayList<>();
        for (EnumValue v : e.values)
            vs.add(nombre(v.name, v) + (v.explicitValue != null ? " = " + v.explicitValue : ""));
        return (e.isPublic ? "public " : "") + "enum " + nombre(e.name, e) + " { " + String.join(", ", vs) + " }";
    }

    /** Un nombre del programa. Si en JC es una palabra reservada, el traductor lo dice: no renombra (§E2). */
    private static String nombre(String n, Object donde) {
        if (RESERVADAS_SOLO_EN_JC.contains(n))
            throw new NoSoportado("el nombre «" + n + "», que en JC es una palabra reservada", donde);
        return n;
    }

    // ============================================================
    // TIPOS (los de C#, punto 6)
    // ============================================================
    private String tipo(TypeRef t) {
        if (t instanceof SimpleTypeRef) {
            SimpleTypeRef st = (SimpleTypeRef) t;
            return tipoSimple(st.name) + (st.argumentos == null ? "" : argumentosDeTipo(st.argumentos));
        }
        if (t instanceof ArrayTypeRef) {
            ArrayTypeRef a = (ArrayTypeRef) t;
            // El tamaño va detrás del nombre (variable): un tipo con tamaño en otro sitio no tiene forma en JC.
            if (a.size != null) throw new NoSoportado("un array de tamaño fijo fuera de una declaración de variable", t);
            return tipo(a.element) + "[]";
        }
        if (t instanceof TupleTypeRef) {
            List<String> es = new ArrayList<>();
            for (TypeRef e : ((TupleTypeRef) t).elements) es.add(tipo(e));
            return "(" + String.join(", ", es) + ")";
        }
        throw new NoSoportado("el tipo " + t.getClass().getSimpleName(), t);
    }

    /** V8/H7.2 (#749) — `<Object>`, `<int, Object>`. */
    private String argumentosDeTipo(List<TypeRef> args) {
        List<String> as = new ArrayList<>();
        for (TypeRef a : args) as.add(tipo(a));
        return "<" + String.join(", ", as) + ">";
    }

    private static String tipoSimple(String n) {
        switch (n) {
            case "integer": return "int";
            case "boolean": return "bool";
            case "int8":    return "sbyte";
            case "word":    return "ushort";
            case "int16":   return "short";
            default:        return n;   // long, float, double, string, byte, any y los nombres de clase
        }
    }

    /** El tipo que deduce el compilador, escrito en JC. */
    private static String tipoDeducido(BpType t, Object donde) {
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case INTEGER: return "int";
                case LONG:    return "long";
                case FLOAT:   return "float";
                case DOUBLE:  return "double";
                case BOOLEAN: return "bool";
                case STRING:  return "string";
                case UINT8:   return "byte";
                case INT8:    return "sbyte";
                case UINT16:  return "ushort";
                case INT16:   return "short";
                default:      break;
            }
        }
        if (t instanceof ArrayType) return tipoDeducido(((ArrayType) t).element, donde) + "[]";
        if (t instanceof ClassType || t instanceof EnumType) return t.display();
        throw new NoSoportado("el tipo que deduce el compilador: " + (t == null ? "ninguno" : t.display()), donde);
    }

    // ============================================================
    // SENTENCIAS
    // ============================================================
    private void sentencia(IStmt s) {
        if (s instanceof VarDecl)                 linea(variable((VarDecl) s, false) + ";");
        else if (s instanceof ConstDecl)          linea(constante((ConstDecl) s, false) + ";");
        else if (s instanceof AssignStmt)         linea(asignacion((AssignStmt) s) + ";");
        else if (s instanceof DestructAssignStmt) linea(desempaquetado((DestructAssignStmt) s) + ";");
        else if (s instanceof ExprStmt)           linea(expr(((ExprStmt) s).expr) + ";");
        else if (s instanceof IfStmt)             si((IfStmt) s);
        else if (s instanceof WhileStmt)          mientras((WhileStmt) s);
        else if (s instanceof DoLoopStmt)         hacer((DoLoopStmt) s);
        else if (s instanceof ForStmt)            para((ForStmt) s);
        else if (s instanceof TryStmt)            intento((TryStmt) s);
        else if (s instanceof ReturnStmt)         linea(devolver((ReturnStmt) s) + ";");
        else if (s instanceof ThrowStmt)          linea("throw " + expr(((ThrowStmt) s).value) + ";");
        else if (s instanceof PrintStmt)          linea(imprime((PrintStmt) s) + ";");
        else if (s instanceof BreakStmt)          linea("break;");
        else if (s instanceof ContinueStmt)       linea("continue;");
        else if (s instanceof SwitchStmt)         segun((SwitchStmt) s);
        else if (s instanceof ParallelStmt)       paralelo((ParallelStmt) s);
        else if (s instanceof RaiseStmt)          linea(dispara((RaiseStmt) s) + ";");
        else if (s instanceof InlineStmt)         throw new NoSoportado("un inline entre otras sentencias (punto 26)", s);
        else throw new NoSoportado(s.getClass().getSimpleName(), s);
    }

    /** La sentencia de un `if`/`while`/`for` sin llaves, en la MISMA línea que su cabecera. Si ocupa más de una (un
     *  bloque que empieza ahí), las demás siguen debajo con su sangría. */
    private void enLinea(String cabecera, IStmt s) {
        EscritorJc sub = new EscritorJc(info, argsEscritos, nivel);
        sub.sentencia(s);
        String[] ls = sub.sb.toString().split("\n", -1);
        linea(cabecera + " " + ls[0].trim());
        for (int i = 1; i < ls.length; i++) if (!ls[i].isEmpty()) sb.append(ls[i]).append('\n');
    }

    private String asignacion(AssignStmt a) {
        String op;
        switch (a.op) {
            case ASSIGN:       op = " = "; break;
            case PLUS_ASSIGN:  op = " += "; break;
            case MINUS_ASSIGN: op = " -= "; break;
            case MUL_ASSIGN:   op = " *= "; break;   // los demás compuestos, los de C# (V8/H7.1 #747)
            case DIV_ASSIGN:   op = " /= "; break;
            case MOD_ASSIGN:   op = " %= "; break;
            case AND_ASSIGN:   op = " &= "; break;
            case OR_ASSIGN:    op = " |= "; break;
            case XOR_ASSIGN:   op = " ^= "; break;
            case SHL_ASSIGN:   op = " <<= "; break;
            case SHR_ASSIGN:   op = " >>= "; break;
            default: throw new NoSoportado("la asignación «" + a.op + "»", a);
        }
        return expr(a.target) + op + expr(a.value);
    }

    /** `{ n, s } := par()` de BP es el de C#: `(n, s) = par()` (punto 15). */
    private String desempaquetado(DestructAssignStmt d) {
        return "(" + lista(d.targets) + ") = " + expr(d.value);
    }

    /** El `if` de C (punto 8). Sin `else` y sin llaves, el de una línea de BP; lo demás, con llaves siempre, y el
     *  `elseif` de BP es el `else if`. */
    private void si(IfStmt s) {
        String cab = "if (" + expr(s.then_.condition) + ")";
        if (s.isSingleLine) {
            if (s.then_.body.size() != 1 || !s.elseIfs.isEmpty() || s.else_ != null)
                throw new NoSoportado("un if de una línea con más de una sentencia", s);
            enLinea(cab, s.then_.body.get(0));
            return;
        }
        linea(cab);
        bloque(s.then_.body);
        for (IfClause c : s.elseIfs) {
            linea("else if (" + expr(c.condition) + ")");
            bloque(c.body);
        }
        if (s.else_ != null) {
            linea("else");
            bloque(s.else_);
        }
    }

    private void mientras(WhileStmt w) {
        String cab = "while (" + expr(w.condition) + ")";
        if (w.isSingleLine) {
            if (w.body.size() != 1) throw new NoSoportado("un while de una línea con más de una sentencia", w);
            enLinea(cab, w.body.get(0));
            return;
        }
        linea(cab);
        bloque(w.body);
    }

    /** `do { … } while (c);` es el `do … loop c` de BP. El de BP sin condición no tiene forma en JC (punto 9). */
    private void hacer(DoLoopStmt d) {
        if (d.condition == null) throw new NoSoportado("un do … loop sin condición (punto 9)", d);
        linea("do");
        linea("{");
        nivel++;
        for (IStmt s : d.body) sentencia(s);
        nivel--;
        linea("} while (" + expr(d.condition) + ");");
    }

    /** El `for` de BP con la ropa de C: `i <= b` si sube y `i >= b` si baja, y el paso, un número (punto 11). El `for …
     *  in` es el `foreach (var x in …)` de C# (punto 16). */
    private void para(ForStmt f) {
        String it = nombre(f.iteratorName, f);
        String cab;
        if (f.range instanceof ForNumericRange) {
            ForNumericRange r = (ForNumericRange) f.range;
            String cmp, paso;
            if (r.step == null) {
                cmp = "<=";
                paso = it + "++";
            } else if (r.step instanceof IntLitExpr) {
                cmp = "<=";
                paso = it + " += " + ((IntLitExpr) r.step).value;
            } else if (r.step instanceof UnaryExpr && "-".equals(((UnaryExpr) r.step).op)
                    && ((UnaryExpr) r.step).operand instanceof IntLitExpr) {
                long v = ((IntLitExpr) ((UnaryExpr) r.step).operand).value;
                cmp = ">=";
                paso = v == 1 ? it + "--" : it + " -= " + v;
            } else {
                throw new NoSoportado("un for cuyo paso no es un número (punto 11)", f);
            }
            cab = "for (int " + it + " = " + expr(r.from) + "; " + it + " " + cmp + " " + expr(r.to, P_DESPL)
                + "; " + paso + ")";
        } else if (f.range instanceof ForInRange) {
            cab = "foreach (var " + it + " in " + expr(((ForInRange) f.range).iterable) + ")";
        } else {
            throw new NoSoportado(f.range.getClass().getSimpleName(), f);
        }
        if (f.isSingleLine) {
            if (f.body.size() != 1) throw new NoSoportado("un for de una línea con más de una sentencia", f);
            enLinea(cab, f.body.get(0));
            return;
        }
        linea(cab);
        bloque(f.body);
    }

    /** El `try` de BP con llaves (punto 13): `catch (T e)`, `catch` a secas, y el recurso, `try (T r = e)`. */
    private void intento(TryStmt t) {
        if (t.recurso != null) {
            VarDecl r = t.recurso;
            if (r.names.size() != 1 || r.type == null || r.init == null) throw new NoSoportado("un try var sin su forma", t);
            linea("try (" + tipo(r.type) + " " + nombre(r.names.get(0).name, t) + " = " + expr(r.init) + ")");
        } else {
            linea("try");
        }
        bloque(t.body);
        for (CatchClause c : t.catches) {
            if (c.varName == null) linea("catch");
            else linea("catch (" + (c.exceptionType != null ? c.exceptionType : "Exception") + " "
                       + nombre(c.varName, c) + ")");
            bloque(c.body);
        }
        List<IStmt> fin = t.finallyBody;
        if (fin != null && t.recurso != null) {
            // El parser añade al final el ReleaseOwnerStmt que suelta el recurso: no se escribe.
            IStmt ultima = fin.isEmpty() ? null : fin.get(fin.size() - 1);
            if (!(ultima instanceof ReleaseOwnerStmt)) throw new NoSoportado("el finally de un try var", t);
            fin = fin.subList(0, fin.size() - 1);
            if (fin.isEmpty()) fin = null;
        }
        if (fin != null) {
            linea("finally");
            bloque(fin);
        }
    }

    private String devolver(ReturnStmt r) {
        return r.value == null ? "return" : "return " + expr(r.value);
    }

    /** El `switch` de C# con el significado de BP (punto 35): `case 1, 2:` y `default:`, sin caer de un caso al
     *  siguiente y sin `break` que haga falta. */
    private void segun(SwitchStmt s) {
        linea("switch (" + expr(s.subject) + ")");
        linea("{");
        nivel++;
        for (CaseClause c : s.cases) {
            linea("case " + lista(c.values) + ":");
            seccion(c.body);
        }
        if (s.defaultBody != null) {
            linea("default:");
            seccion(s.defaultBody);
        }
        nivel--;
        linea("}");
    }

    /** `parallel { case t1 = new Thread(4096): … default: … }` (punto 36). */
    private void paralelo(ParallelStmt p) {
        linea("parallel");
        linea("{");
        nivel++;
        for (ParallelBranch b : p.branches) {
            linea("case " + nombre(b.varName, b) + " = new Thread(" + expr(b.stackSizeExpr) + "):");
            seccion(b.body);
        }
        if (p.defaultBody != null) {
            linea("default:");
            seccion(p.defaultBody);
        }
        nivel--;
        linea("}");
    }

    /** Las sentencias de un `case` o de un `default`, un nivel más adentro, sin llaves. */
    private void seccion(List<IStmt> body) {
        nivel++;
        for (IStmt st : body) sentencia(st);
        nivel--;
    }

    private String dispara(RaiseStmt r) {
        return "raise " + nombre(r.event, r) + "(" + lista(escritos(r, r.args)) + ")";
    }

    /**
     * El `print` de JC no pone nada entre las piezas ni salta solo (punto 17): la coma de BP, que pone un espacio, es
     * una pieza `" "`. El salto que BP pone al final es el `println` (Eduardo, 18-oct, #703): la línea se cierra y sale
     * en su momento, como en BP. Un separador al final (`print a;`, #700) es el `print`, que no salta.
     */
    private String imprime(PrintStmt p) {
        List<String> piezas = new ArrayList<>();
        boolean salto = true;
        for (int i = 0; i < p.items.size(); i++) {
            PrintItem it = p.items.get(i);
            if (i > 0 && it.leadingSep == PrintSep.COMMA) piezas.add("\" \"");
            if (it.expr != null) piezas.add(expr(it.expr));
            else if (i == p.items.size() - 1) salto = false;
        }
        return (salto ? "println(" : "print(") + String.join(", ", piezas) + ")";
    }

    // ============================================================
    // EXPRESIONES (la precedencia de C#, punto 18)
    // ============================================================
    private String lista(List<IExpr> es) {
        List<String> out = new ArrayList<>();
        for (IExpr e : es) out.add(expr(e));
        return String.join(", ", out);
    }

    private String expr(IExpr e) { return expr(e, 0); }

    /** La expresión, entre paréntesis si su precedencia en C# es menor que la que pide su sitio. */
    private String expr(IExpr e, int minima) {
        String s = texto(e);
        return precedencia(e) < minima ? "(" + s + ")" : s;
    }

    private int precedencia(IExpr e) {
        if (e instanceof CallExpr && esTernario((CallExpr) e)) return P_TERNARIO;
        if (e instanceof BinaryExpr)     return precedenciaBinaria(((BinaryExpr) e).op, e);
        if (e instanceof UnaryExpr)      return P_UNARIO;
        if (e instanceof InstanceOfExpr) return P_REL;
        if (e instanceof CallExpr && esConversion((CallExpr) e)) return P_UNARIO;
        return P_PRIMARIO;
    }

    private String texto(IExpr e) {
        if (e instanceof IntLitExpr)       return Long.toString(((IntLitExpr) e).value);
        if (e instanceof LongLitExpr)      return Long.toString(((LongLitExpr) e).value) + "L";
        if (e instanceof FloatLitExpr)     return Double.toString(((FloatLitExpr) e).value) + "f";   // en C#, 1.5 es double
        if (e instanceof DoubleLitExpr)    return Double.toString(((DoubleLitExpr) e).value);
        if (e instanceof StringLitExpr)    return cadena(((StringLitExpr) e).value);
        if (e instanceof BoolLitExpr)      return ((BoolLitExpr) e).value ? "true" : "false";
        if (e instanceof NullLitExpr)      return "null";
        if (e instanceof IdentifierExpr)   return nombre(((IdentifierExpr) e).name, e);
        if (e instanceof ArrayLitExpr)     return "[" + lista(((ArrayLitExpr) e).elements) + "]";
        if (e instanceof ParenExpr)        return "(" + expr(((ParenExpr) e).inner) + ")";
        if (e instanceof TupleExpr)        return "(" + lista(((TupleExpr) e).elements) + ")";
        if (e instanceof MemberAccessExpr) {
            MemberAccessExpr m = (MemberAccessExpr) e;
            return expr(m.target, P_PRIMARIO) + "." + nombre(m.member, e);
        }
        if (e instanceof IndexExpr) {
            IndexExpr i = (IndexExpr) e;
            return expr(i.target, P_PRIMARIO) + "[" + expr(i.index) + "]";
        }
        if (e instanceof CallExpr)         return llamada((CallExpr) e);
        if (e instanceof UnaryExpr) {
            UnaryExpr u = (UnaryExpr) e;
            String dentro = expr(u.operand, P_UNARIO);
            if ("not".equalsIgnoreCase(u.op)) return "!" + dentro;
            if ("-".equals(u.op)) return dentro.startsWith("-") ? "-(" + dentro + ")" : "-" + dentro;
            throw new NoSoportado("el operador «" + u.op + "»", e);
        }
        if (e instanceof BinaryExpr) {
            BinaryExpr b = (BinaryExpr) e;
            String op = operador(b.op, b);
            if ("**".equals(op)) return expr(b.left, P_PRIMARIO) + " ** " + expr(b.right, P_UNARIO);
            int p = precedenciaBinaria(b.op, b);
            return expr(b.left, p) + " " + op + " " + expr(b.right, p + 1);
        }
        if (e instanceof InstanceOfExpr) {
            InstanceOfExpr i = (InstanceOfExpr) e;
            return expr(i.target, P_DESPL) + " is " + i.typeName;   // el `instanceof` de BP (punto 24)
        }
        if (e instanceof ThisExpr)         return "this";
        if (e instanceof FieldExpr)        return "field";
        if (e instanceof SuperExpr)        return "base";   // el `super` de BP es el `base` de C# (punto 34)
        if (e instanceof SuperCallExpr)
            throw new NoSoportado("super(…) fuera de la primera sentencia de un constructor", e);
        if (e instanceof MethodRefExpr) {
            MethodRefExpr m = (MethodRefExpr) e;
            return expr(m.target, P_PRIMARIO) + "::" + nombre(m.method, e);   // el `::` de Java (punto 34)
        }
        if (e instanceof BoundCallExpr) {
            BoundCallExpr b = (BoundCallExpr) e;
            return expr(b.target, P_PRIMARIO) + "::" + nombre(b.method, e) + "(" + lista(escritos(b, b.args)) + ")";
        }
        throw new NoSoportado(e.getClass().getSimpleName(), e);
    }

    /** `integer(x)` y la conversión de referencia de BP, `Persona(o)` (`refCasts`), son el `(T) x` de C# (punto 23). */
    private boolean esConversion(CallExpr c) {
        if (escritos(c).size() != 1) return false;
        if (c.callee instanceof IdentifierExpr && CONVERSIONES_PRIMITIVAS.contains(((IdentifierExpr) c.callee).name))
            return true;
        return info.refCasts.contains(c);
    }

    /** El `iif(c, a, b)` de BP es el ternario de C#, `c ? a : b` (V8/H7.1 #747): lo decide el análisis. */
    private boolean esTernario(CallExpr c) {
        return c.esTernario || info.ternarios.contains(c);
    }

    /** Una conversión, `(T) x`; una construcción, `new T(…)` (el nombre es de una clase: punto 22); un ternario; o una
     *  llamada. */
    private String llamada(CallExpr c) {
        if (esTernario(c)) {
            // La condición, por encima del ternario (uno dentro, entre paréntesis); las ramas, como en C#: la de la
            // derecha encadena sin paréntesis (`a ? x : b ? y : z`), y la del medio, si es otro ternario, los lleva.
            IExpr si = c.args.get(1);
            String medio = (si instanceof CallExpr && esTernario((CallExpr) si)) ? "(" + expr(si) + ")" : expr(si);
            return expr(c.args.get(0), P_O) + " ? " + medio + " : " + expr(c.args.get(2));
        }
        if (esConversion(c)) {
            String t = (c.callee instanceof IdentifierExpr)
                    ? tipoSimple(((IdentifierExpr) c.callee).name)
                    : expr(c.callee, P_PRIMARIO);
            return "(" + t + ") " + expr(c.args.get(0), P_UNARIO);
        }
        // V8/H7.2 (#749) — `new Collections.Map<int, Object>()`, escrito así en JC, es en BP `Collections.numericMap()`:
        // la marca dice la clase y los argumentos de tipo escritos.
        if (c.claseNew != null && c.argumentosNew != null)
            return "new " + c.claseNew + argumentosDeTipo(c.argumentosNew) + "(" + lista(escritos(c)) + ")";
        String targs = (c.argumentosNew != null) ? argumentosDeTipo(c.argumentosNew) : "";
        String llamado = expr(c.callee, P_PRIMARIO) + targs + "(" + lista(escritos(c)) + ")";
        return (info.exprSymbols.get(c.callee) instanceof Symbol.ClassSymbol) ? "new " + llamado : llamado;
    }

    private static String operador(String op, Object donde) {
        switch (op.toLowerCase()) {
            case "or":  return "||";
            case "and": return "&&";
            case "xor": return "^";
            case "mod": return "%";
            case "shl": return "<<";
            case "shr": return ">>";
            case "^":   return "**";
            case "+": case "-": case "*": case "/": case "&": case "|":
            case "==": case "!=": case "<": case ">": case "<=": case ">=":
                return op;
            default:
                throw new NoSoportado("el operador «" + op + "»", donde);
        }
    }

    private static int precedenciaBinaria(String op, Object donde) {
        switch (op.toLowerCase()) {
            case "or":  return P_O;
            case "and": return P_Y;
            case "|":   return P_OBIT;
            case "xor": return P_XOR;
            case "&":   return P_YBIT;
            case "==": case "!=": return P_IGUAL;
            case "<": case ">": case "<=": case ">=": return P_REL;
            case "shl": case "shr": return P_DESPL;
            case "+": case "-": return P_SUMA;
            case "*": case "/": case "mod": return P_MULT;
            case "^":   return P_POT;
            default:
                throw new NoSoportado("el operador «" + op + "»", donde);
        }
    }

    /** Un literal de cadena de JC: los escapes de BP y, para los demás caracteres de control, el de cuatro cifras
     *  hexadecimales de C#, que el lexer de JC tiene que leer (punto 19). */
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
                default:
                    if (c < 0x20 || c == 0x7f) s.append('\\').append('u').append(String.format("%04x", (int) c));
                    else s.append(c);
            }
        }
        return s.append('"').toString();
    }

    // ============================================================
    // LA OPCIÓN DEL COMPILADOR: --escribir-jc
    // ============================================================

    /** Lo que sale de traducir un fichero: ESCRITO (con su JC), NO SOPORTADO, NO SE ANALIZA o FALLO. */
    static final class Resultado {
        String clase;
        String jc;
        String detalle;
        String motivo;   // el de NoSoportado, sin la línea: para las cuentas
    }

    static Resultado traducir(Path f) {
        Resultado r = new Resultado();
        Main.Analizado a = Main.analizarParaTraducir(f);
        if (a.modulo == null) {
            r.clase = "NO SE ANALIZA";
            r.detalle = a.error;
            return r;
        }
        try {
            r.jc = escribir(a.modulo, a.info, a.argsEscritos);
            r.clase = "ESCRITO";
        } catch (NoSoportado ns) {
            r.clase = "NO SOPORTADO";
            r.detalle = ns.getMessage();
            r.motivo = ns.motivo;
        } catch (RuntimeException ex) {
            r.clase = "FALLO";
            r.detalle = "el escritor falló: " + ex;
        }
        return r;
    }

    // ============================================================
    // LA IDA Y VUELTA: --comprobar-jc (paso 3)
    // ============================================================

    /**
     * El árbol del original (recién leído, sin analizar), escrito en JC y leído con JcParser, tiene que ser el mismo,
     * con lo que se acordó que no cuenta (ComparaAst.primeraDiferenciaJc). EQUIVALENTE | DISTINTO | JC ILEGIBLE (el
     * escritor y el parser no casan) | NO SOPORTADO | NO SE ANALIZA | NO SE LEE | FALLO.
     */
    static String comprobarJcFichero(Path f) throws IOException {
        String fuente = new String(Files.readAllBytes(f), java.nio.charset.StandardCharsets.UTF_8);
        Lexer lx = new Lexer(fuente);
        List<Token> ts = lx.tokenize();
        if (!lx.getErrors().isEmpty()) return "NO SE LEE: " + lx.getErrors().get(0);
        Parser ps = new Parser(ts);
        ModuleNode a = ps.parseModule();
        if (!ps.getErrors().isEmpty() || a == null)
            return "NO SE LEE: " + (ps.getErrors().isEmpty() ? "el parser no devolvió módulo" : ps.getErrors().get(0));
        Resultado r = traducir(f);
        if (!"ESCRITO".equals(r.clase)) return r.clase + ": " + r.detalle;
        ModuleNode b;
        try {
            b = JcParser.leer(r.jc);
        } catch (JcLexer.Error ex) {
            return "JC ILEGIBLE: " + ex.getMessage() + "  ←  " + lineaDe(r.jc, ex.linea);
        } catch (RuntimeException ex) {
            return "FALLO: el parser de JC falló: " + ex;
        }
        String d = ComparaAst.primeraDiferenciaJc(a, b);
        return d == null ? "EQUIVALENTE" : "DISTINTO: " + d;
    }

    private static String lineaDe(String texto, int n) {
        String[] ls = texto.split("\n", -1);
        return (n >= 1 && n <= ls.length) ? ls[n - 1].trim() : "";
    }

    /** --comprobar-jc: la ida y vuelta de un fichero, o de todos los .bp de una carpeta, con las cuentas. */
    static int comprobarJc(Path p) throws IOException {
        PrintStream out = new PrintStream(new java.io.FileOutputStream(java.io.FileDescriptor.out), true, "UTF-8");
        if (!Files.isDirectory(p)) {
            String r = comprobarJcFichero(p);
            out.println(p.getFileName() + ": " + r);
            return r.startsWith("EQUIVALENTE") ? 0 : 1;
        }
        List<Path> fs;
        try (Stream<Path> w = Files.walk(p)) {
            fs = w.filter(x -> x.toString().endsWith(".bp")).sorted().collect(Collectors.toList());
        }
        Map<String, Integer> cuentas = new TreeMap<>();
        for (Path f : fs) {
            String r = comprobarJcFichero(f);
            cuentas.merge(r.split(":")[0], 1, Integer::sum);
            if (!r.startsWith("EQUIVALENTE") && !r.startsWith("NO SOPORTADO") && !r.startsWith("NO SE ANALIZA"))
                out.println(p.relativize(f) + ": " + r);
        }
        out.println("---- " + fs.size() + " ficheros: " + cuentas);
        int malos = cuentas.getOrDefault("DISTINTO", 0) + cuentas.getOrDefault("JC ILEGIBLE", 0)
                  + cuentas.getOrDefault("FALLO", 0);
        return malos == 0 ? 0 : 1;
    }

    /** --escribir-jc: el JC de un fichero, por la salida; con una carpeta, las cuentas de todos sus .bp. 0 = bien;
     *  2 = algo no soportado; 3 = no se analiza o el escritor falló. */
    static int escribirJc(Path p) throws IOException {
        PrintStream out = new PrintStream(new java.io.FileOutputStream(java.io.FileDescriptor.out), true, "UTF-8");
        if (!Files.isDirectory(p)) {
            Resultado r = traducir(p);
            if (r.jc != null) {
                out.print(r.jc);
                return 0;
            }
            System.err.println(r.clase + ": " + r.detalle);
            return "NO SOPORTADO".equals(r.clase) ? 2 : 3;
        }
        List<Path> fs;
        try (Stream<Path> w = Files.walk(p)) {
            fs = w.filter(x -> x.toString().endsWith(".bp")).sorted().collect(Collectors.toList());
        }
        Map<String, Integer> cuentas = new TreeMap<>();
        Map<String, Integer> motivos = new TreeMap<>();
        for (Path f : fs) {
            Resultado r = traducir(f);
            cuentas.merge(r.clase, 1, Integer::sum);
            if (r.motivo != null) motivos.merge(r.motivo, 1, Integer::sum);
            if (!"ESCRITO".equals(r.clase)) out.println(p.relativize(f) + ": " + r.clase + ": " + r.detalle);
        }
        out.println("---- " + fs.size() + " ficheros: " + cuentas);
        if (!motivos.isEmpty()) out.println("---- no soportados, por motivo: " + motivos);
        if (cuentas.containsKey("FALLO") || cuentas.containsKey("NO SE ANALIZA")) return 3;
        return cuentas.getOrDefault("ESCRITO", 0) == fs.size() ? 0 : 2;
    }
}
