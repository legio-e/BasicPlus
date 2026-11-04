// ============================================================
// JcParser.java
// V8 #698 (JC, paso 3) — el parser de JC: de los tokens de JcLexer, al MISMO
// árbol que el parser de BP.
//
// La sintaxis es la que decidió Eduardo el 17-oct (V8_IDEAS §E2, puntos 1–36):
// la de C# donde no se dijo otra cosa, con lo de BP que C# no tiene. El árbol,
// en cambio, es el de BP, nodo a nodo y campo a campo: lo que el parser de BP
// fabrica, éste lo fabrica igual —el `import Core` implícito al final de los
// imports, el cuerpo que lanza de una `native` externa de un pack, el
// `ReleaseOwnerStmt` que cierra el `finally` de un recurso—, y lo que en JC se
// escribe distinto se convierte aquí:
//   - `print(a, b)` es el `print a; b;` de BP: piezas sin nada entre ellas y
//     sin salto (punto 17; en BP, #700);
//   - `v++` es `v += 1` (punto 12); `else if` es `elseif` (punto 8);
//   - el `for` de C es el de BP, y el paso un número (punto 11);
//   - `new X(…)` y `(T) x` son la llamada `X(…)` de BP, con una marca que la
//     comparación no cuenta (puntos 22 y 23);
//   - una declaración de varios nombres con algún valor da uno por nombre,
//     como los grupos de BP (punto 5).
// El primer error para el parser: no hay recuperación. El compilador lee un
// `.jc` con este parser y sigue con el árbol como si viniera de un `.bp`.
// ============================================================
package basicplus.frontend;

import basicplus.frontend.Ast.*;
import basicplus.frontend.JcLexer.Tipo;
import basicplus.frontend.JcLexer.Tok;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

final class JcParser {

    /** Los tipos de JC y su nombre en BP (punto 6). */
    private static final Map<String, String> TIPOS = new HashMap<>();
    static {
        TIPOS.put("int", "integer");
        TIPOS.put("long", "long");
        TIPOS.put("float", "float");
        TIPOS.put("double", "double");
        TIPOS.put("bool", "boolean");
        TIPOS.put("string", "string");
        TIPOS.put("byte", "byte");
        TIPOS.put("sbyte", "int8");
        TIPOS.put("short", "int16");
        TIPOS.put("ushort", "word");
    }

    private final List<Tok> toks;
    private int pos = 0;
    private boolean enInterfaz = false;
    /** V8 H7.3 — dentro de una `interface` de clase: sus funciones son sólo la firma. */
    private boolean enInterfazDeClase = false;
    /** La clase que se está leyendo, o null en el módulo. */
    private String clase = null;
    private String packNativoMarca = null;
    private int packNativoVersion = 0;

    private JcParser(List<Tok> toks) { this.toks = toks; }

    /** El árbol de un fuente JC. Lanza {@link JcLexer.Error} con la línea y la columna del primer error. */
    static ModuleNode leer(String fuente) {
        return new JcParser(JcLexer.leer(fuente)).modulo();
    }

    // ============================================================
    // CURSOR
    // ============================================================
    private Tok actual() { return toks.get(pos); }
    private Tok mira(int n) { return toks.get(Math.min(pos + n, toks.size() - 1)); }
    private boolean es(String s) { return actual().es(s); }
    private boolean esNombre() { return actual().tipo == Tipo.NOMBRE; }
    private boolean esFin() { return actual().tipo == Tipo.FIN; }

    private Tok avanza() {
        Tok t = actual();
        if (t.tipo != Tipo.FIN) pos++;
        return t;
    }

    private boolean acepta(String s) {
        if (!es(s)) return false;
        avanza();
        return true;
    }

    private Tok espera(String s) { return espera(s, "'" + s + "'"); }

    private Tok espera(String s, String que) {
        if (!es(s)) throw error("se esperaba " + que + ", encontrado " + actual());
        return avanza();
    }

    private String nombre(String que) {
        if (!esNombre()) {
            Tok t = actual();
            throw error("se esperaba " + que + ", encontrado " + t
                    + (t.tipo == Tipo.RESERVADA ? " — es una palabra reservada de JC" : ""));
        }
        return avanza().texto;
    }

    /** `a.b.c`, tal cual. */
    private String nombrePunteado(String que) {
        StringBuilder sb = new StringBuilder(nombre(que));
        while (es(".") && mira(1).tipo == Tipo.NOMBRE) {
            avanza();
            sb.append('.').append(avanza().texto);
        }
        return sb.toString();
    }

    private JcLexer.Error error(String msg) {
        Tok t = actual();
        return new JcLexer.Error(msg, t.linea, t.columna, false);
    }

    // ============================================================
    // MÓDULO E IMPORTS
    // ============================================================
    private ModuleNode modulo() {
        Tok inicio = actual();
        String library = "";
        if (acepta("library")) {
            if (actual().tipo != Tipo.CADENA) throw error("se esperaba una cadena tras 'library'");
            library = (String) avanza().valor;
            espera(";");
        }
        espera("module", "'module' al principio del fichero");
        boolean isInterface = acepta("interface");
        String name = nombre("el nombre del módulo");
        String implementsName = null;
        if (!isInterface && acepta("implements")) implementsName = nombrePunteado("el nombre de la interfaz");
        if (isInterface && acepta("extends")) implementsName = nombrePunteado("el nombre de la interfaz padre");
        if (isInterface && es("implements")) throw error("'module interface' no puede llevar 'implements' (usa 'extends')");
        enInterfaz = isInterface;
        espera("{", "'{' tras la cabecera del módulo");
        List<ImportNode> imports = new ArrayList<>();
        while (es("import")) imports.add(importa());
        List<ITopLevelDecl> defs = new ArrayList<>();
        while (!es("}")) {
            if (esFin()) throw error("falta la '}' que cierra el módulo");
            declaracion(defs);
        }
        espera("}");
        if (!esFin()) throw error("sobra " + actual() + " tras la '}' que cierra el módulo");
        // El `import Core` implícito, como el parser de BP (Parser.parseModule): al final, salvo en Core.
        if (!"Core".equals(name)) {
            boolean tieneCore = false;
            for (ImportNode imp : imports)
                if (!imp.path.isEmpty() && "Core".equals(imp.path.get(imp.path.size() - 1))) tieneCore = true;
            if (!tieneCore) {
                List<String> p = new ArrayList<>();
                p.add("Core");
                ImportNode imp = new ImportNode(p, null, inicio.linea, inicio.columna);
                imp.implicito = true;
                imports.add(imp);
            }
        }
        ModuleNode mn = new ModuleNode(library, name, isInterface, implementsName, imports, defs,
                                       inicio.linea, inicio.columna);
        mn.packNativoMarca = packNativoMarca;
        mn.packNativoVersion = packNativoVersion;
        mn.escritoEnJc = true;   // el análisis comprueba el `new` y el `(T) x` (puntos 22 y 23)
        return mn;
    }

    /** Los `import`, como en BP, con su `;` (punto 30). */
    private ImportNode importa() {
        Tok tok = espera("import");
        List<String> path = new ArrayList<>();
        path.add(nombre("el nombre del módulo"));
        while (acepta(".")) path.add(nombre("un nombre tras '.'"));
        String boundImpl = null;
        if (acepta(":")) boundImpl = nombrePunteado("el módulo que implementa la interfaz");
        String fromPath = null, packName = null, marcaNativa = null;
        if (acepta("from")) {
            if (esNombre() && "pack".equals(actual().texto)) {
                avanza();
                if (actual().tipo == Tipo.CADENA) {
                    String marca = (String) avanza().valor;
                    if (marca.length() != 4) throw error("la marca de un pack nativo son 4 caracteres, p.ej. \"SQLI\"");
                    if (path.size() != 1 || !path.get(0).matches("v[0-9]+"))
                        throw error("se esperaba la versión de la API — `import v1 from pack \"SQLI\";`");
                    packNativoMarca = marca;
                    marcaNativa = marca;
                    packNativoVersion = Integer.parseInt(path.get(0).substring(1));
                } else {
                    packName = nombre("el nombre del pack tras 'from pack'");
                }
            } else {
                if (actual().tipo != Tipo.CADENA) throw error("se esperaba una cadena o 'pack <nombre>' tras 'from'");
                fromPath = (String) avanza().valor;
            }
        }
        espera(";");
        ImportNode imp = new ImportNode(path, fromPath, boundImpl, packName, tok.linea, tok.columna);
        imp.packNativoMarca = marcaNativa;
        return imp;
    }

    // ============================================================
    // DECLARACIONES
    // ============================================================
    /** Una declaración del módulo, con sus anotaciones delante si las lleva (punto 27; varias desde H7.3, `#756`): se
     *  cuelgan de la primera. */
    private void declaracion(List<ITopLevelDecl> defs) {
        List<Annotation> anns = new ArrayList<>();
        while (es("@")) anns.add(anotacion());
        int antes = defs.size();
        declaracionSinAnotacion(defs);
        if (!anns.isEmpty() && defs.size() > antes) ((Node) defs.get(antes)).annotations.addAll(anns);
    }

    /** Una declaración del módulo, o un miembro de la clase que se está leyendo ({@link #clase}). */
    private void declaracionSinAnotacion(List<ITopLevelDecl> defs) {
        boolean isPublic = false, isPrivate = false, isProtected = false, isFinal = false, isSync = false;
        boolean isNative = false, isOwner = false, isStatic = false, isRoot = false;
        while (true) {
            if (es("public") || es("private") || es("protected")) {
                if (isPublic || isPrivate || isProtected)
                    throw error("'public', 'protected' y 'private' son excluyentes, y van una vez");
                String v = avanza().texto;
                if (v.equals("public")) isPublic = true;
                else if (v.equals("private")) isPrivate = true;
                else isProtected = true;   // lo que BP no marca en una clase (punto 34)
            } else if (acepta("final")) {
                isFinal = true;
            } else if (acepta("sync")) {
                isSync = true;
            } else if (acepta("native")) {
                isNative = true;
            } else if (acepta("owner")) {
                isOwner = true;
            } else if (acepta("static")) {
                isStatic = true;
            } else if (esNombre() && actual().texto.equals("root") && mira(1).es("class")) {
                avanza();   // `root class`, como en BP: palabra contextual
                isRoot = true;
            } else {
                break;
            }
        }
        if (clase == null && (isProtected || isStatic))
            throw error((isStatic ? "'static'" : "'protected'") + " sólo va en un miembro de una clase");
        if (es("class")) {
            if (clase != null) throw error("una clase dentro de otra: BP no las tiene");
            defs.add(claseDef(isPublic, isRoot));
            return;
        }
        if (isRoot) throw error("'root' sólo va delante de 'class'");
        if (es("interface")) {
            // V8 H7.3 (#756) — una interfaz de clase (la cabecera `module interface` es otra cosa: se lee antes)
            if (clase != null) throw error("una interfaz dentro de una clase: BP no las tiene");
            if (isPrivate || isProtected || isFinal || isSync || isNative || isOwner || isStatic)
                throw error("una interfaz sólo lleva 'public' delante: sus métodos son públicos, y los escribe quien la"
                        + " implementa");
            defs.add(interfazDef(isPublic));
            return;
        }
        String deClase = isStatic ? clase : null;   // el nombre de la clase delante, como en BP (punto 20)
        if (es("event")) {
            if (clase == null) throw error("un evento es de una clase");
            if (isPublic) throw error("un evento no se marca 'public': suscribirse ya es público, y dispararlo es de la"
                    + " clase y sus hijas");
            defs.add(evento(deClase));
            return;
        }
        if (isNative && !es("function")) throw error("'native' sólo se aplica a 'function'");
        if (es("const")) {
            if (isStatic) throw error("una constante no lleva 'static': ya es de la clase, como en C# (punto 34)");
            defs.add(constante(isPublic, clase));   // en una clase, BP le pone delante el nombre de la clase
            return;
        }
        if (es("function")) {
            defs.add(funcion(isPublic, isPrivate, isFinal, isNative, deClase));
            return;
        }
        if (es("enum")) {
            defs.add(enumerado(isPublic));
            return;
        }
        // Lo demás empieza por un tipo: una propiedad (si sigue una llave) o variables.
        TypeRef tipo = tipo();
        Tok n = actual();
        String nombre = nombre("el nombre de la variable o de la propiedad");
        if (es("{")) {
            defs.add(propiedad(isPublic, isPrivate, isFinal, isSync, isOwner, tipo, nombre, n, deClase));
            return;
        }
        if (isSync) throw error("'sync' sólo se aplica a una propiedad");
        for (VarDecl v : variables(isPublic, isOwner, tipo, n, nombre, deClase)) {
            v.isPrivate = isPrivate;
            defs.add(v);
        }
    }

    /** `public class Perro : Animal, IMascota { … }` (punto 34). Como en C#, el padre y las interfaces van en la misma
     *  lista, el padre primero (V8 H7.3, `#756`): el parser no sabe si el primero es una clase o una interfaz, así que
     *  lo pone de padre y lo marca (`primeroPuedeSerInterfaz`), y lo decide el semántico. */
    private ClassDef claseDef(boolean isPublic, boolean isRoot) {
        Tok tok = espera("class");
        String name = nombre("el nombre de la clase");
        String base = null;
        List<String> interfaces = new ArrayList<>();
        if (acepta(":")) {
            base = nombreDeTipo("la clase base tras ':'");
            while (acepta(",")) interfaces.add(nombreDeTipo("una interfaz tras ','"));
        }
        espera("{", "'{' tras la cabecera de la clase");
        List<ITopLevelDecl> members = miembros(name, "la clase");
        ClassDef cd = new ClassDef(isPublic, name, base, members, tok.linea, tok.columna);
        cd.isRoot = isRoot;
        cd.interfaces.addAll(interfaces);
        cd.primeroPuedeSerInterfaz = base != null;
        return cd;
    }

    /** V8 H7.3 (`#756`) — `public interface IForma : IBase { function double area(); string nombre { get; set; } }`:
     *  una interfaz de clase, como la de BP. Sus funciones son sólo la firma, con `;`. */
    private ClassDef interfazDef(boolean isPublic) {
        Tok tok = espera("interface");
        String name = nombre("el nombre de la interfaz");
        List<String> padres = new ArrayList<>();
        if (acepta(":")) {
            padres.add(nombreDeTipo("una interfaz tras ':'"));
            while (acepta(",")) padres.add(nombreDeTipo("una interfaz tras ','"));
        }
        espera("{", "'{' tras la cabecera de la interfaz");
        boolean antes = enInterfazDeClase;
        enInterfazDeClase = true;
        List<ITopLevelDecl> members;
        try {
            members = miembros(name, "la interfaz");
        } finally {
            enInterfazDeClase = antes;
        }
        ClassDef cd = new ClassDef(isPublic, name, null, members, tok.linea, tok.columna);
        cd.isInterface = true;
        cd.interfaces.addAll(padres);
        return cd;
    }

    /** `Forma` o `Lib.Forma`: el nombre de una clase o de una interfaz. */
    private String nombreDeTipo(String que) {
        String n = nombre(que);
        if (es(".") && mira(1).tipo == Tipo.NOMBRE) {
            avanza();
            n = n + "." + avanza().texto;
        }
        return n;
    }

    /** Los miembros de una clase o de una interfaz, hasta su `}` (que consume). */
    private List<ITopLevelDecl> miembros(String name, String que) {
        List<ITopLevelDecl> members = new ArrayList<>();
        clase = name;
        while (!es("}")) {
            if (esFin()) throw error("falta la '}' que cierra " + que + " " + name);
            declaracion(members);
        }
        clase = null;
        espera("}");
        return members;
    }

    /** `event cambia(Medida m);` (punto 34). */
    private EventDef evento(String deClase) {
        Tok tok = espera("event");
        Tok n = actual();
        String nm = nombre("el nombre del evento");
        espera("(", "'(': un evento declara su firma");
        List<Param> ps = new ArrayList<>();
        if (!es(")")) {
            ps.add(parametro());
            while (acepta(",")) ps.add(parametro());
        }
        espera(")");
        espera(";");
        if (ps.size() > Parser.MAX_EVENT_PARAMS)
            throw error("un evento admite como mucho " + Parser.MAX_EVENT_PARAMS + " parámetros");
        return new EventDef(new DeclName(deClase, nm, n.linea, n.columna), ps, tok.linea, tok.columna);
    }

    /** `@BD{ tabla = "x", clave }`: las de BP (punto 27). */
    private Annotation anotacion() {
        Tok at = espera("@");
        String nombre = nombre("el nombre de la anotación tras '@'");
        espera("{", "'{' tras '@" + nombre + "' (las llaves van aunque no lleven nada)");
        List<AnnEntry> entradas = new ArrayList<>();
        while (!es("}")) {
            Tok k = actual();
            String clave = nombre("el nombre de una clave dentro de '@" + nombre + "{ … }'");
            Object valor = null;
            if (acepta("=")) {
                Tok v = actual();
                switch (v.tipo) {
                    case CADENA: case ENTERO: case LARGO: case FLOTANTE: case DOBLE:
                        valor = v.valor;
                        avanza();
                        break;
                    default:
                        if (acepta("true")) valor = Boolean.TRUE;
                        else if (acepta("false")) valor = Boolean.FALSE;
                        else throw error("el valor de '" + clave + "' en '@" + nombre + "' tiene que ser un literal");
                }
            }
            entradas.add(new AnnEntry(clave, valor, k.linea, k.columna));
            if (!acepta(",")) break;
        }
        espera("}");
        return new Annotation(nombre, entradas, at.linea, at.columna);
    }

    /** `const int MAX = 10;`, con el tipo siempre (punto 29). {@code deClase}: en una clase, su nombre, que BP pone
     *  delante (punto 34); si no, null. */
    private ConstDecl constante(boolean isPublic, String deClase) {
        Tok tok = espera("const");
        TypeRef tipo = tipo();
        Tok n = actual();
        String nm = nombre("el nombre de la constante");
        espera("=", "'=' en la declaración de la constante");
        IExpr v = expresion();
        espera(";");
        return new ConstDecl(isPublic, new DeclName(deClase, nm, n.linea, n.columna), tipo, v, tok.linea, tok.columna);
    }

    /**
     * `int a, b;` · `int a = 1, b = 2;` · `int nums[10];` (puntos 5 y 33), tras el tipo y el primer nombre. Sin ningún
     * valor, un VarDecl con todos los nombres, como el `var a, b: integer` de BP; con algún valor, uno por nombre, como
     * sus grupos.
     */
    private List<VarDecl> variables(boolean isPublic, boolean isOwner, TypeRef tipo, Tok primero, String primerNombre,
                                    String deClase) {
        List<Tok> posiciones = new ArrayList<>();
        List<String> nombres = new ArrayList<>();
        List<IExpr> tamanos = new ArrayList<>();
        List<IExpr> valores = new ArrayList<>();
        Tok n = primero;
        String nm = primerNombre;
        while (true) {
            IExpr tamano = null;
            if (acepta("[")) {
                if (tipo instanceof ArrayTypeRef) throw error("un array de tamaño fijo es de un tipo simple: int nums[10] (punto 33)");
                tamano = expresion();
                espera("]");
            }
            IExpr valor = acepta("=") ? expresion() : null;
            posiciones.add(n);
            nombres.add(nm);
            tamanos.add(tamano);
            valores.add(valor);
            if (!acepta(",")) break;
            n = actual();
            nm = nombre("el nombre de la siguiente variable");
        }
        espera(";");
        boolean algunValor = false;
        for (IExpr v : valores) if (v != null) algunValor = true;
        List<VarDecl> out = new ArrayList<>();
        if (!algunValor && mismosTamanos(tamanos)) {
            List<DeclName> ds = new ArrayList<>();
            for (int i = 0; i < nombres.size(); i++)
                ds.add(new DeclName(deClase, nombres.get(i), posiciones.get(i).linea, posiciones.get(i).columna));
            out.add(new VarDecl(isPublic, isOwner, ds, conTamano(tipo, tamanos.get(0)), null,
                                primero.linea, primero.columna));
            return out;
        }
        for (int i = 0; i < nombres.size(); i++) {
            List<DeclName> ds = new ArrayList<>();
            ds.add(new DeclName(deClase, nombres.get(i), posiciones.get(i).linea, posiciones.get(i).columna));
            out.add(new VarDecl(isPublic, isOwner, ds, conTamano(tipo, tamanos.get(i)), valores.get(i),
                                posiciones.get(i).linea, posiciones.get(i).columna));
        }
        return out;
    }

    private static boolean mismosTamanos(List<IExpr> ts) {
        IExpr a = ts.get(0);
        for (IExpr b : ts) {
            if (a == null && b == null) continue;
            if (!(a instanceof IntLitExpr) || !(b instanceof IntLitExpr)) return false;
            if (((IntLitExpr) a).value != ((IntLitExpr) b).value) return false;
        }
        return true;
    }

    private static TypeRef conTamano(TypeRef t, IExpr tamano) {
        return tamano == null ? t : new ArrayTypeRef(t, tamano, ((Node) t).line, ((Node) t).column);
    }

    /** La propiedad corta de BP es la automática de C#: `public int n { get; set; }`, y con valor, `… = 0;` (punto
     *  32). La larga, `{ get { … } set { … } }` (punto 34): el `set` sin nombre es el de C#, cuyo parámetro es
     *  `value`; `set(v) { … }` le da otro, como en BP. */
    private PropertyDef propiedad(boolean isPublic, boolean isPrivate, boolean isFinal, boolean isSync, boolean isOwner,
                                  TypeRef tipo, String nombre, Tok n, String deClase) {
        espera("{");
        DeclName dn = new DeclName(deClase, nombre, n.linea, n.columna);
        GetterDef g = null;
        SetterDef s = null;
        boolean corta = esNombre() && actual().texto.equals("get") && mira(1).es(";");
        if (corta) {
            avanza();
            avanza();
            if (!(esNombre() && actual().texto.equals("set") && mira(1).es(";")))
                throw error("la propiedad corta es { get; set; } (punto 32)");
            avanza();
            avanza();
        } else {
            if (esNombre() && actual().texto.equals("get")) {
                Tok gt = avanza();
                g = new GetterDef(bloque(), gt.linea, gt.columna);
            }
            if (esNombre() && actual().texto.equals("set")) {
                Tok st = avanza();
                String param = "value";
                if (acepta("(")) {
                    param = nombre("el nombre del parámetro del set");
                    espera(")");
                }
                s = new SetterDef(param, bloque(), st.linea, st.columna);
            }
            if (g == null && s == null) throw error("una propiedad lleva get, set o los dos");
        }
        espera("}");
        IExpr init = null;
        if (acepta("=")) {
            init = expresion();
            espera(";");
        }
        PropertyDef p = new PropertyDef(isPublic, isFinal, isOwner, isSync, dn, tipo, init, g, s, corta,
                                        n.linea, n.columna);
        p.isPrivate = isPrivate;
        return p;
    }

    /** `public enum Color { Rojo, Verde = 5 }`, con el valor de BP: un entero escrito, sin signo (punto 25). */
    private EnumDef enumerado(boolean isPublic) {
        Tok tok = espera("enum");
        String n = nombre("el nombre del enum");
        espera("{");
        List<EnumValue> vs = new ArrayList<>();
        while (!es("}")) {
            Tok vt = actual();
            String vn = nombre("el nombre de un valor del enum");
            Long val = null;
            if (acepta("=")) {
                if (actual().tipo != Tipo.ENTERO) throw error("el valor de un enum es un entero escrito, sin signo (punto 25)");
                val = (Long) avanza().valor;
            }
            vs.add(new EnumValue(vn, val, vt.linea, vt.columna));
            if (!acepta(",")) break;
        }
        espera("}");
        return new EnumDef(isPublic, n, vs, tok.linea, tok.columna);
    }

    /**
     * `function int f(int a, int b = 1) { … }`; `void` si no devuelve nada (punto 20). Sin cuerpo, con `;`: la de un
     * `module interface` y la `native` externa de un pack. Con `inline …;`, la función de una instrucción (punto 26).
     * En una clase (punto 34): el constructor, sin tipo y con el nombre de la clase, con `: base(…)` o `: this(…)`,
     * que son la primera sentencia de BP; el destructor, `function ~Clase()`; y {@code deClase}, lo `static`.
     */
    private FuncDef funcion(boolean isPublic, boolean isPrivate, boolean isFinal, boolean isNative, String deClase) {
        Tok tok = espera("function");
        boolean isEventHandler = acepta("event");
        boolean isDestructor = false, ctor = false;
        String dtorEscrito = null;
        DeclName name;
        TypeRef ret = null;
        String n;
        if (acepta("~")) {
            if (clase == null) throw error("un destructor es de una clase");
            Tok nt = actual();
            dtorEscrito = nombre("el nombre de la clase tras '~' (el destructor se llama como su clase)");
            n = "__dtor";   // su nombre interno, como en BP
            name = new DeclName(null, n, nt.linea, nt.columna);
            isDestructor = true;
        } else if (esNombre() && mira(1).es("(")) {
            if (clase == null || !actual().texto.equals(clase))
                throw error("falta el tipo de la función; si no devuelve nada, void (un constructor, sin tipo, se llama"
                        + " como su clase)");
            Tok nt = avanza();
            n = nt.texto;
            name = new DeclName(null, n, nt.linea, nt.columna);
            ctor = true;
        } else {
            ret = acepta("void") ? null : tipo();
            Tok nt = actual();
            n = nombre("el nombre de la función");
            name = new DeclName(deClase, n, nt.linea, nt.columna);
        }
        if ((ctor || isDestructor) && deClase != null) throw error("un constructor o un destructor no lleva 'static'");
        espera("(", "'(' tras el nombre de la función");
        List<Param> ps = new ArrayList<>();
        if (!es(")")) {
            ps.add(parametro());
            while (acepta(",")) ps.add(parametro());
        }
        espera(")");
        // `: base(…)` y `: this(…)`: la primera sentencia del constructor de BP, `super(…)` o la delegación.
        IStmt inicial = null;
        if (acepta(":")) {
            if (!ctor) throw error("':' tras los parámetros va sólo en un constructor: : base(…) o : this(…)");
            Tok ini = actual();
            if (acepta("base")) {
                espera("(");
                inicial = new ExprStmt(new SuperCallExpr(argumentos(), ini.linea, ini.columna), ini.linea, ini.columna);
            } else if (acepta("this")) {
                Tok lp = espera("(");
                inicial = new ExprStmt(new CallExpr(new IdentifierExpr(clase, ini.linea, ini.columna), argumentos(),
                                                    lp.linea, lp.columna), ini.linea, ini.columna);
            } else {
                throw error("tras ':' va base(…) o this(…)");
            }
        }
        if (enInterfazDeClase) {
            // V8 H7.3 — la función de una interfaz de clase es sólo su firma, como en C#: `function double area();`
            if (inicial != null) throw error("una interfaz no tiene constructor");
            espera(";", "';': una función de una interfaz no lleva cuerpo");
            FuncDef fd = new FuncDef(isPublic, isFinal, false, false, name, ps, ret, new ArrayList<>(),
                                     tok.linea, tok.columna);
            fd.isPrivate = isPrivate;
            return fd;
        }
        if (enInterfaz) {
            // Como en BP: la función de una interfaz es sólo su firma.
            if (inicial != null) throw error("un constructor de un module interface no lleva : base(…) ni : this(…)");
            espera(";", "';': una función de un module interface no lleva cuerpo");
            FuncDef fd = new FuncDef(isPublic, isFinal, false, false, name, ps, ret, new ArrayList<>(),
                                     tok.linea, tok.columna);
            fd.isPrivate = isPrivate;
            return fd;
        }
        if (isNative && packNativoMarca != null) {
            espera(";", "';': en un módulo con `import … from pack \"" + packNativoMarca + "\"` las funciones native"
                    + " son externas y no llevan cuerpo");
            // El cuerpo que lanza, el mismo que pone el parser de BP (Parser.parseFuncDef).
            List<IStmt> cuerpoAviso = new ArrayList<>();
            List<IExpr> argsAviso = new ArrayList<>();
            argsAviso.add(new StringLitExpr(n + ": falta el codigo nativo del pack '" + packNativoMarca + "' v"
                    + packNativoVersion + ". Este modulo no puede funcionar sin el.", tok.linea, tok.columna));
            CallExpr crea = new CallExpr(new IdentifierExpr("RuntimeError", tok.linea, tok.columna),
                    argsAviso, tok.linea, tok.columna);
            crea.conNew = true;   // en JC, crear un objeto lleva `new` (el análisis lo mira; la comparación, no)
            cuerpoAviso.add(new ThrowStmt(crea, tok.linea, tok.columna));
            FuncDef ext = new FuncDef(isPublic, isFinal, false, true, name, ps, ret, cuerpoAviso, tok.linea, tok.columna);
            ext.isPackExtern = true;
            ext.isPrivate = isPrivate;
            return ext;
        }
        if (es(";"))
            throw error("una función sin cuerpo sólo puede ser de un module interface, o la native de un pack");
        List<IStmt> body;
        boolean intrinseca = false;
        if (es("inline")) {
            Tok it = avanza();
            String mnem = nombre("el mnemónico tras 'inline' (p. ej. CALL_BUILTIN)");
            List<Long> ops = new ArrayList<>();
            while (!es(";")) {
                boolean neg = acepta("-");
                if (actual().tipo != Tipo.ENTERO) throw error("el operando de 'inline' tiene que ser un entero");
                long v = (Long) avanza().valor;
                ops.add(neg ? -v : v);
            }
            espera(";");
            if (isNative) throw error("una función 'native' no puede tener cuerpo 'inline' (es una intrínseca: no hay"
                    + " nada que compilar a C)");
            body = new ArrayList<>();
            body.add(new InlineStmt(mnem, ops, it.linea, it.columna));
            intrinseca = true;
        } else {
            body = bloque();
        }
        if (inicial != null) {
            if (intrinseca) throw error("un constructor no es una función de una instrucción");
            body.add(0, inicial);
        }
        FuncDef fd = new FuncDef(isPublic, isFinal, intrinseca, isNative, name, ps, ret, body, tok.linea, tok.columna);
        fd.isEventHandler = isEventHandler;
        fd.isDestructor = isDestructor;
        fd.dtorEscrito = dtorEscrito;
        fd.isPrivate = isPrivate;
        return fd;
    }

    private Param parametro() {
        TypeRef tp = tipo();
        Tok n = actual();
        String nm = nombre("el nombre del parámetro");
        IExpr def = acepta("=") ? expresion() : null;
        return new Param(nm, tp, def, n.linea, n.columna);
    }

    // ============================================================
    // TIPOS (los de C#, con el nombre de BP en el árbol: punto 6)
    // ============================================================
    private TypeRef tipo() {
        Tok t = actual();
        TypeRef base;
        if (es("(")) {
            // La tupla, sólo como tipo de retorno; `(T)` es T, como en BP.
            avanza();
            List<TypeRef> es = new ArrayList<>();
            es.add(tipo());
            while (acepta(",")) es.add(tipo());
            espera(")");
            base = es.size() == 1 ? es.get(0) : new TupleTypeRef(es, t.linea, t.columna);
        } else if (t.tipo == Tipo.RESERVADA && TIPOS.containsKey(t.texto)) {
            avanza();
            base = new SimpleTypeRef(TIPOS.get(t.texto), t.linea, t.columna);
        } else if (t.tipo == Tipo.NOMBRE) {
            avanza();
            String n = t.texto;
            if (es(".") && mira(1).tipo == Tipo.NOMBRE) {
                avanza();
                n = n + "." + avanza().texto;
            }
            base = new SimpleTypeRef(n, t.linea, t.columna);
            if (es("<")) ((SimpleTypeRef) base).argumentos = argumentosDeTipo();   // V8/H7.2 (#749)
        } else {
            throw error("se esperaba un tipo, encontrado " + t);
        }
        if (es("[") && mira(1).es("]")) {
            avanza();
            avanza();
            base = new ArrayTypeRef(base, null, t.linea, t.columna);
        }
        return base;
    }

    /** V8/H7.2 (#749) — `<T, U>` tras el nombre de un tipo: los argumentos de tipo. Qué combinaciones valen lo dice el
     *  análisis (de momento, `List<Object>` y `Collections.Map<int|string, Object>`). */
    private List<TypeRef> argumentosDeTipo() {
        espera("<");
        List<TypeRef> args = new ArrayList<>();
        args.add(tipo());
        while (acepta(",")) args.add(tipo());
        if (es(">>")) throw error("los genéricos anidados (`<…<…>>`) no los tiene JC, de momento");
        espera(">", "'>' al cerrar los argumentos de tipo");
        return args;
    }

    /** ¿Empieza aquí una declaración de variables? Un tipo seguido de un nombre: `int x`, `List l`, `Mod.T x`,
     *  `List[] xs`, y con argumentos de tipo, `List<Object> l` (V8/H7.2: se mira hasta su `>`). */
    private boolean esDeclaracion() {
        Tok t = actual();
        if (t.tipo == Tipo.RESERVADA) return TIPOS.containsKey(t.texto);
        if (t.tipo != Tipo.NOMBRE) return false;
        int k = 1;
        if (mira(1).es(".") && mira(2).tipo == Tipo.NOMBRE) k = 3;
        if (mira(k).es("<")) {
            int prof = 0;
            for (int j = k; ; j++) {
                Tok x = mira(j);
                if (x.es("<")) prof++;
                else if (x.es(">")) { if (--prof == 0) { k = j + 1; break; } }
                else if (!(x.tipo == Tipo.NOMBRE || (x.tipo == Tipo.RESERVADA && TIPOS.containsKey(x.texto))
                           || x.es(",") || x.es("."))) return false;
            }
        }
        if (mira(k).es("[") && mira(k + 1).es("]")) k += 2;
        return mira(k).tipo == Tipo.NOMBRE;
    }

    // ============================================================
    // SENTENCIAS
    // ============================================================
    private List<IStmt> bloque() {
        espera("{");
        List<IStmt> out = new ArrayList<>();
        while (!es("}")) {
            if (esFin()) throw error("falta la '}' que cierra el bloque");
            sentencia(out);
        }
        espera("}");
        return out;
    }

    /** El cuerpo de un if, while, for…: un bloque, o una sentencia sola, que es la forma de una línea de BP. */
    private List<IStmt> cuerpo() {
        if (es("{")) return bloque();
        List<IStmt> out = new ArrayList<>();
        sentencia(out);
        return out;
    }

    private void sentencia(List<IStmt> out) {
        Tok t = actual();
        if (es("{")) throw error("un bloque suelto: en JC, como en BP, un bloque va con su if, while, for…");
        if (es("if"))       { out.add(si()); return; }
        if (es("while"))    { out.add(mientras()); return; }
        if (es("do"))       { out.add(hacer()); return; }
        if (es("for"))      { out.add(para()); return; }
        if (es("foreach"))  { out.add(paraCada()); return; }
        if (es("try"))      { out.add(intento()); return; }
        if (es("return"))   { out.add(devolver()); return; }
        if (es("print") || es("println")) { out.add(imprime()); return; }
        if (es("const"))    { out.add(constante(false, null)); return; }
        if (es("switch"))   { out.add(segun()); return; }
        if (es("parallel")) { out.add(paralelo()); return; }
        if (es("raise"))    { out.add(dispara()); return; }
        if (acepta("throw")) {
            IExpr v = expresion();
            espera(";");
            out.add(new ThrowStmt(v, t.linea, t.columna));
            return;
        }
        if (acepta("break") || acepta("continue")) {
            String n = esNombre() ? nombre("el nombre del bucle") : null;   // V8/H8: `break miLoop;`
            espera(";");
            out.add(t.texto.equals("break") ? new BreakStmt(n, t.linea, t.columna) : new ContinueStmt(n, t.linea, t.columna));
            return;
        }
        if (es("inline"))
            throw error("en JC, inline es el cuerpo entero de una función: function T f(…) inline …; (punto 26)");
        if (es("owner") || esDeclaracion()) {
            boolean owner = acepta("owner");
            TypeRef tipo = tipo();
            Tok n = actual();
            String nm = nombre("el nombre de la variable");
            out.addAll(variables(false, owner, tipo, n, nm, null));
            return;
        }
        if (es("(") && tuplaAdelante()) {
            int cierre = cierre();
            // `(string, int) t = par();`, una variable de tipo tupla; `(n, s) = par();`, el desempaquetado.
            if (cierre > 0 && toks.get(cierre + 1).tipo == Tipo.NOMBRE) {
                TypeRef tipo = tipo();
                Tok n = actual();
                String nm = nombre("el nombre de la variable");
                out.addAll(variables(false, false, tipo, n, nm, null));
                return;
            }
            if (cierre > 0 && toks.get(cierre + 1).es("=")) { out.add(desempaquetado()); return; }
        }
        out.add(asignacionOLlamada());
    }

    /** El índice de la `)` que cierra la `(` de aquí, o -1. */
    private int cierre() {
        int paren = 0;
        for (int i = pos; i < toks.size(); i++) {
            Tok t = toks.get(i);
            if (t.es("(")) paren++;
            else if (t.es(")") && --paren == 0) return i;
            else if (t.tipo == Tipo.FIN) return -1;
        }
        return -1;
    }

    /** El `if` de C (punto 8). Sin `else` y sin llaves, el de una línea de BP; `else if` es su `elseif`. */
    private IfStmt si() {
        Tok t = espera("if");
        espera("(");
        IExpr c = expresion();
        espera(")");
        boolean conLlaves = es("{");
        List<IStmt> then = cuerpo();
        List<IfClause> elseIfs = new ArrayList<>();
        List<IStmt> sino = null;
        boolean hayElse = false;
        while (es("else")) {
            hayElse = true;
            Tok e = avanza();
            if (acepta("if")) {
                espera("(");
                IExpr ec = expresion();
                espera(")");
                elseIfs.add(new IfClause(ec, cuerpo(), e.linea, e.columna));
                continue;
            }
            sino = cuerpo();
            break;
        }
        return new IfStmt(new IfClause(c, then, t.linea, t.columna), elseIfs, sino, !conLlaves && !hayElse,
                          t.linea, t.columna);
    }

    private WhileStmt mientras() {
        Tok t = espera("while");
        espera("(");
        IExpr c = expresion();
        espera(")");
        boolean conLlaves = es("{");
        return new WhileStmt(c, cuerpo(), !conLlaves, t.linea, t.columna);
    }

    /** `do { … } while (c);` es el `do … loop c` de BP (punto 9). */
    private DoLoopStmt hacer() {
        Tok t = espera("do");
        String n = esNombre() ? nombre("el nombre del bucle") : null;   // V8/H8: `do miLoop { … } while (c);`
        List<IStmt> body = cuerpo();
        espera("while", "'while' tras el cuerpo del do");
        espera("(");
        IExpr c = expresion();
        espera(")");
        espera(";");
        return new DoLoopStmt(body, c, n, t.linea, t.columna);
    }

    /** El `for` de BP con la ropa de C (punto 11): `int i = a; i <= b; i++` si sube, `i >= b` y `i--` si baja; el
     *  paso, un número. */
    private ForStmt para() {
        Tok t = espera("for");
        espera("(");
        espera("int", "'int': la variable del for es un int, como en BP (punto 11)");
        String i = nombre("el nombre de la variable del for");
        espera("=");
        IExpr desde = expresion();
        espera(";");
        if (!(esNombre() && actual().texto.equals(i))) throw error("la condición del for es sobre '" + i + "' (punto 11)");
        avanza();
        boolean sube;
        if (acepta("<=")) sube = true;
        else if (acepta(">=")) sube = false;
        else throw error("la condición del for es '" + i + " <= límite' si sube, o '" + i + " >= límite' si baja (punto 11)");
        IExpr hasta = expresion();
        espera(";");
        if (!(esNombre() && actual().texto.equals(i))) throw error("el incremento del for es de '" + i + "' (punto 11)");
        Tok it = avanza();
        IExpr paso;
        if (acepta("++")) {
            if (!sube) throw error("con '>=' el for baja: el incremento es " + i + "-- o " + i + " -= n");
            paso = null;
        } else if (acepta("--")) {
            if (sube) throw error("con '<=' el for sube: el incremento es " + i + "++ o " + i + " += n");
            paso = new UnaryExpr("-", new IntLitExpr(1, it.linea, it.columna), it.linea, it.columna);
        } else if (acepta("+=")) {
            if (!sube) throw error("con '>=' el for baja: el incremento es " + i + "-- o " + i + " -= n");
            paso = numeroDelPaso();
        } else if (acepta("-=")) {
            if (sube) throw error("con '<=' el for sube: el incremento es " + i + "++ o " + i + " += n");
            paso = new UnaryExpr("-", numeroDelPaso(), it.linea, it.columna);
        } else {
            throw error("el incremento del for es " + i + "++, " + i + "--, " + i + " += n o " + i + " -= n (punto 11)");
        }
        espera(")");
        boolean conLlaves = es("{");
        List<IStmt> body = cuerpo();
        return new ForStmt(i, new ForNumericRange(desde, hasta, paso, t.linea, t.columna), body, !conLlaves,
                           t.linea, t.columna);
    }

    private IExpr numeroDelPaso() {
        Tok n = actual();
        if (n.tipo != Tipo.ENTERO) throw error("el paso del for es un número (punto 11)");
        avanza();
        return new IntLitExpr((Long) n.valor, n.linea, n.columna);
    }

    /** `foreach (var x in lista)` es el `for x in lista do` de BP (punto 16). */
    private ForStmt paraCada() {
        Tok t = espera("foreach");
        espera("(");
        espera("var", "'var': la variable del foreach toma el tipo de los elementos (punto 16)");
        String x = nombre("el nombre de la variable del foreach");
        espera("in");
        IExpr col = expresion();
        espera(")");
        boolean conLlaves = es("{");
        List<IStmt> body = cuerpo();
        return new ForStmt(x, new ForInRange(col, t.linea, t.columna), body, !conLlaves, t.linea, t.columna);
    }

    /** El `try` de BP con llaves (punto 13): `catch (T e)`, `catch` a secas, y el recurso, `try (T r = e)`. */
    private TryStmt intento() {
        Tok t = espera("try");
        VarDecl recurso = null;
        if (acepta("(")) {
            Tok rt = actual();
            TypeRef tp = tipo();
            Tok n = actual();
            String rn = nombre("el nombre del recurso");
            espera("=");
            IExpr init = expresion();
            espera(")");
            List<DeclName> ns = new ArrayList<>();
            ns.add(new DeclName(null, rn, n.linea, n.columna));
            recurso = new VarDecl(false, true, ns, tp, init, rt.linea, rt.columna);   // owner, siempre (como en BP)
        }
        List<IStmt> body = bloque();
        List<CatchClause> catches = new ArrayList<>();
        while (es("catch")) {
            Tok ct = avanza();
            String var = null, tipoExc = null;
            if (acepta("(")) {
                tipoExc = nombrePunteado("el tipo de la excepción");
                var = nombre("el nombre de la excepción");
                espera(")");
            }
            catches.add(new CatchClause(var, tipoExc, bloque(), ct.linea, ct.columna));
        }
        List<IStmt> fin = acepta("finally") ? bloque() : null;
        if (recurso != null) {
            // El finally suelta el recurso, tras el del programa: lo mismo que el parser de BP.
            if (fin == null) fin = new ArrayList<>();
            fin.add(new ReleaseOwnerStmt(recurso.names.get(0).name, recurso.line, recurso.column));
        }
        TryStmt ts = new TryStmt(body, catches, fin, t.linea, t.columna);
        ts.recurso = recurso;
        return ts;
    }

    private ReturnStmt devolver() {
        Tok t = espera("return");
        IExpr v = null;
        if (!es(";")) {
            if (es("(") && tuplaAdelante()) {
                avanza();
                List<IExpr> el = new ArrayList<>();
                el.add(expresion());
                while (acepta(",")) el.add(expresion());
                espera(")");
                v = new TupleExpr(el, t.linea, t.columna);
            } else {
                v = expresion();
            }
        }
        espera(";");
        return new ReturnStmt(v, t.linea, t.columna);
    }

    /** ¿La `(` de aquí abre una lista con una coma a su nivel (una tupla), y no un paréntesis? */
    private boolean tuplaAdelante() {
        int paren = 0, otros = 0;
        for (int i = pos; i < toks.size(); i++) {
            Tok t = toks.get(i);
            if (t.es("(")) paren++;
            else if (t.es(")")) { paren--; if (paren == 0) return false; }
            else if (t.es("[") || t.es("{")) otros++;
            else if (t.es("]") || t.es("}")) otros--;
            else if (t.es(",") && paren == 1 && otros == 0) return true;
            else if (t.tipo == Tipo.FIN) return false;
        }
        return false;
    }

    /** `(n, s) = par();` es el desempaquetado de BP, `{ n, s } := par()` (punto 15). */
    private DestructAssignStmt desempaquetado() {
        Tok t = espera("(");
        List<IExpr> destinos = new ArrayList<>();
        destinos.add(postfijo());
        while (acepta(",")) destinos.add(postfijo());
        espera(")");
        espera("=");
        IExpr v = expresion();
        espera(";");
        return new DestructAssignStmt(destinos, v, t.linea, t.columna);
    }

    /** El `switch` de C# con el significado de BP (punto 35): `case 1, 2:` sin caer al siguiente, y sin `break` que
     *  haga falta. */
    private SwitchStmt segun() {
        Tok t = espera("switch");
        espera("(");
        IExpr sujeto = expresion();
        espera(")");
        espera("{");
        List<CaseClause> casos = new ArrayList<>();
        List<IStmt> def = null;
        while (es("case")) {
            Tok ct = avanza();
            List<IExpr> vals = new ArrayList<>();
            vals.add(expresion());
            while (acepta(",")) vals.add(expresion());
            espera(":", "':' tras los valores del case");
            casos.add(new CaseClause(vals, seccion(), ct.linea, ct.columna));
        }
        if (acepta("default")) {
            espera(":", "':' tras default");
            def = seccion();
        }
        espera("}", "'}' que cierra el switch (el default va el último)");
        return new SwitchStmt(sujeto, casos, def, t.linea, t.columna);
    }

    /** `parallel { case t1 = new Thread(4096): … default: … }` (punto 36). */
    private ParallelStmt paralelo() {
        Tok t = espera("parallel");
        espera("{");
        List<ParallelBranch> ramas = new ArrayList<>();
        List<IStmt> def = null;
        while (es("case")) {
            Tok ct = avanza();
            String var = nombre("el nombre del hilo tras 'case'");
            espera("=");
            espera("new", "'new Thread(tamaño de la pila)'");
            if (!(esNombre() && actual().texto.equals("Thread")))
                throw error("en un case de parallel va new Thread(tamaño de la pila)");
            avanza();
            espera("(");
            IExpr pila = expresion();
            espera(")");
            espera(":", "':' tras new Thread(…)");
            ramas.add(new ParallelBranch(var, pila, seccion(), ct.linea, ct.columna));
        }
        if (acepta("default")) {
            espera(":", "':' tras default");
            def = seccion();
        }
        espera("}", "'}' que cierra el parallel (el default va el último)");
        return new ParallelStmt(ramas, def, t.linea, t.columna);
    }

    /** Las sentencias de un case o de un default: hasta el siguiente case, el default o la llave. */
    private List<IStmt> seccion() {
        List<IStmt> out = new ArrayList<>();
        while (!es("case") && !es("default") && !es("}")) {
            if (esFin()) throw error("falta la '}' que cierra el switch o el parallel");
            sentencia(out);
        }
        return out;
    }

    /** `raise cambia(this);` (punto 34). */
    private RaiseStmt dispara() {
        Tok t = espera("raise");
        String ev = nombre("el nombre del evento tras 'raise'");
        espera("(", "'(' tras el nombre del evento (van aunque no lleve nada)");
        List<IExpr> args = argumentos();
        espera(";");
        return new RaiseStmt(ev, args, t.linea, t.columna);
    }

    /** El `print` de JC: piezas sin nada entre ellas y sin salto (punto 17). Es el `print a; b;` de BP, con el `;` al
     *  final (#700). El `println` (Eduardo, 18-oct, #703), lo mismo con el salto al final: el `print a; b` de BP, que
     *  cierra la línea del hilo y la saca en su momento; `println()`, sólo el salto. */
    private PrintStmt imprime() {
        boolean conSalto = es("println");
        Tok t = avanza();
        espera("(");
        List<PrintItem> items = new ArrayList<>();
        if (es(")")) {
            if (!conSalto) throw error("un print vacío no imprime nada; el salto de línea es println();");
        } else {
            items.add(new PrintItem(PrintSep.NONE, expresion()));
            while (acepta(",")) items.add(new PrintItem(PrintSep.SEMICOLON, expresion()));
        }
        espera(")");
        espera(";");
        if (!conSalto) items.add(new PrintItem(PrintSep.SEMICOLON, null));
        return new PrintStmt(items, t.linea, t.columna);
    }

    /** La asignación de este signo, o null: `=`, `+=`, `-=` y los demás compuestos de C# (V8/H7.1 #747). */
    private static AssignOpKind asignacionDe(Tok t) {
        if (t.tipo != Tipo.SIGNO) return null;
        switch (t.texto) {
            case "=":   return AssignOpKind.ASSIGN;
            case "+=":  return AssignOpKind.PLUS_ASSIGN;
            case "-=":  return AssignOpKind.MINUS_ASSIGN;
            case "*=":  return AssignOpKind.MUL_ASSIGN;
            case "/=":  return AssignOpKind.DIV_ASSIGN;
            case "%=":  return AssignOpKind.MOD_ASSIGN;
            case "&=":  return AssignOpKind.AND_ASSIGN;
            case "|=":  return AssignOpKind.OR_ASSIGN;
            case "^=":  return AssignOpKind.XOR_ASSIGN;
            case "<<=": return AssignOpKind.SHL_ASSIGN;
            case ">>=": return AssignOpKind.SHR_ASSIGN;
            default:    return null;
        }
    }

    /** Una asignación (`=`, los compuestos, y `v++` y `v--`, que son `+= 1` y `-= 1`: punto 12), o una llamada. */
    private IStmt asignacionOLlamada() {
        Tok t = actual();
        if (es("++") || es("--")) throw error("en JC, ++ y -- van detrás: v++; (punto 12)");
        IExpr lhs = postfijo();
        AssignOpKind k = asignacionDe(actual());
        if (k != null) {
            avanza();
            IExpr v = expresion();
            espera(";");
            return new AssignStmt(lhs, k, v, t.linea, t.columna);
        }
        if (es("++") || es("--")) {
            boolean mas = avanza().texto.equals("++");
            espera(";");
            return new AssignStmt(lhs, mas ? AssignOpKind.PLUS_ASSIGN : AssignOpKind.MINUS_ASSIGN,
                                  new IntLitExpr(1, t.linea, t.columna), t.linea, t.columna);
        }
        if (lhs instanceof CallExpr) {
            espera(";");
            return new ExprStmt(lhs, t.linea, t.columna);
        }
        throw error("se esperaba una asignación o una llamada como sentencia, encontrado " + actual());
    }

    // ============================================================
    // EXPRESIONES (la precedencia de C#, punto 18; el árbol, el de BP)
    // ============================================================
    /** El ternario de C#, `c ? a : b` (V8/H7.1 #747): el `iif(c, a, b)` de BP, con la marca de cómo se escribió. Por
     *  debajo de `||` y asociativo por la derecha, como en C#. */
    private IExpr expresion() {
        IExpr c = o();
        if (!es("?")) return c;
        Tok q = avanza();
        IExpr si = expresion();
        espera(":", "':' en el ternario: c ? a : b");
        IExpr no = expresion();
        List<IExpr> args = new ArrayList<>();
        args.add(c);
        args.add(si);
        args.add(no);
        CallExpr t = new CallExpr(new IdentifierExpr(Ast.TERNARIO, q.linea, q.columna), args, q.linea, q.columna);
        t.esTernario = true;
        return t;
    }

    private IExpr o() {
        IExpr l = y();
        while (es("||")) {
            Tok op = avanza();
            l = new BinaryExpr("or", l, y(), op.linea, op.columna);
        }
        return l;
    }

    private IExpr y() {
        IExpr l = oBit();
        while (es("&&")) {
            Tok op = avanza();
            l = new BinaryExpr("and", l, oBit(), op.linea, op.columna);
        }
        return l;
    }

    private IExpr oBit() {
        IExpr l = xor();
        while (es("|")) {
            Tok op = avanza();
            l = new BinaryExpr("|", l, xor(), op.linea, op.columna);
        }
        return l;
    }

    private IExpr xor() {
        IExpr l = yBit();
        while (es("^")) {
            Tok op = avanza();
            l = new BinaryExpr("xor", l, yBit(), op.linea, op.columna);
        }
        return l;
    }

    private IExpr yBit() {
        IExpr l = igualdad();
        while (es("&")) {
            Tok op = avanza();
            l = new BinaryExpr("&", l, igualdad(), op.linea, op.columna);
        }
        return l;
    }

    private IExpr igualdad() {
        IExpr l = relacion();
        while (es("==") || es("!=")) {
            Tok op = avanza();
            l = new BinaryExpr(op.texto, l, relacion(), op.linea, op.columna);
        }
        return l;
    }

    /** Las comparaciones y `is`, el `instanceof` de BP (punto 24). */
    private IExpr relacion() {
        IExpr l = desplazamiento();
        while (true) {
            if (es("<") || es(">") || es("<=") || es(">=")) {
                Tok op = avanza();
                l = new BinaryExpr(op.texto, l, desplazamiento(), op.linea, op.columna);
            } else if (es("is")) {
                Tok op = avanza();
                l = new InstanceOfExpr(l, nombrePunteado("el nombre de una clase tras 'is'"), op.linea, op.columna);
            } else {
                return l;
            }
        }
    }

    private IExpr desplazamiento() {
        IExpr l = suma();
        while (es("<<") || es(">>")) {
            Tok op = avanza();
            l = new BinaryExpr(op.texto.equals("<<") ? "shl" : "shr", l, suma(), op.linea, op.columna);
        }
        return l;
    }

    private IExpr suma() {
        IExpr l = producto();
        while (es("+") || es("-")) {
            Tok op = avanza();
            l = new BinaryExpr(op.texto, l, producto(), op.linea, op.columna);
        }
        return l;
    }

    private IExpr producto() {
        IExpr l = unario();
        while (es("*") || es("/") || es("%")) {
            Tok op = avanza();
            l = new BinaryExpr(op.texto.equals("%") ? "mod" : op.texto, l, unario(), op.linea, op.columna);
        }
        return l;
    }

    private IExpr unario() {
        Tok t = actual();
        if (acepta("-")) return new UnaryExpr("-", unario(), t.linea, t.columna);
        if (acepta("!")) return new UnaryExpr("not", unario(), t.linea, t.columna);
        if (es("++") || es("--")) throw error("++ y -- sólo como sentencia, detrás: v++; (punto 12)");
        if (es("~")) throw error("JC no tiene el ~ de C#: BP no tiene el no de bits");
        if (es("(") && esConversion()) return conversion();
        return potencia();
    }

    /** La potencia, `**` (punto 18): la base, primaria; el exponente, unario, que la hace asociativa por la derecha. */
    private IExpr potencia() {
        IExpr base = postfijo();
        if (es("**")) {
            Tok op = avanza();
            return new BinaryExpr("^", base, unario(), op.linea, op.columna);
        }
        return base;
    }

    /** `(T) x`, con la regla de C# (punto 23): un tipo del lenguaje entre los paréntesis; o un nombre, si tras el `)`
     *  viene un nombre, un literal, `(`, `!`, `~` o una palabra reservada (salvo `is`). */
    private boolean esConversion() {
        Tok t1 = mira(1);
        if (t1.tipo == Tipo.RESERVADA) return TIPOS.containsKey(t1.texto) && mira(2).es(")");
        if (t1.tipo != Tipo.NOMBRE) return false;
        int k = 2;
        if (mira(2).es(".") && mira(3).tipo == Tipo.NOMBRE) k = 4;
        if (!mira(k).es(")")) return false;
        Tok d = mira(k + 1);
        switch (d.tipo) {
            case NOMBRE: case ENTERO: case LARGO: case FLOTANTE: case DOBLE: case CADENA:
                return true;
            case RESERVADA:
                return !d.es("is");
            default:
                return d.es("(") || d.es("!") || d.es("~");
        }
    }

    private IExpr conversion() {
        Tok lp = espera("(");
        Tok t = avanza();
        IExpr callee;
        if (t.tipo == Tipo.RESERVADA) {
            callee = new IdentifierExpr(TIPOS.get(t.texto), t.linea, t.columna);
        } else {
            callee = new IdentifierExpr(t.texto, t.linea, t.columna);
            if (acepta(".")) {
                Tok m = avanza();
                callee = new MemberAccessExpr(callee, m.texto, m.linea, m.columna);
            }
        }
        espera(")");
        List<IExpr> args = new ArrayList<>();
        args.add(unario());
        CallExpr c = new CallExpr(callee, args, lp.linea, lp.columna);
        c.esConversion = true;
        return c;
    }

    private IExpr postfijo() {
        IExpr n = primario();
        while (true) {
            if (es(".")) {
                Tok op = avanza();
                String m = nombre("el nombre de un miembro tras '.'");
                n = new MemberAccessExpr(n, m, op.linea, op.columna);
                if (es("(")) {
                    Tok lp = avanza();
                    n = new CallExpr(n, argumentos(), lp.linea, lp.columna);
                }
            } else if (es("[")) {
                Tok op = avanza();
                IExpr i = expresion();
                espera("]");
                n = new IndexExpr(n, i, op.linea, op.columna);
            } else if (es("::")) {
                // `obj::metodo`, y `obj::metodo(args)` (la llamada preparada): el `::` de Java (punto 34). No encadena
                // más, como en BP.
                Tok op = avanza();
                String m = nombre("el nombre de un método tras '::'");
                if (es("(")) {
                    Tok lp = avanza();
                    return new BoundCallExpr(n, m, argumentos(), lp.linea, lp.columna);
                }
                return new MethodRefExpr(n, m, op.linea, op.columna);
            } else {
                return n;
            }
        }
    }

    /** Los argumentos, tras la `(`, hasta la `)` incluida. */
    private List<IExpr> argumentos() {
        List<IExpr> a = new ArrayList<>();
        if (!es(")")) {
            a.add(expresion());
            while (acepta(",")) a.add(expresion());
        }
        espera(")");
        return a;
    }

    private IExpr primario() {
        Tok t = actual();
        switch (t.tipo) {
            case ENTERO:   avanza(); return new IntLitExpr((Long) t.valor, t.linea, t.columna);
            case LARGO:    avanza(); return new LongLitExpr((Long) t.valor, t.linea, t.columna);
            case FLOTANTE: avanza(); return new FloatLitExpr((Double) t.valor, t.linea, t.columna);
            case DOBLE:    avanza(); return new DoubleLitExpr((Double) t.valor, t.linea, t.columna);
            case CADENA:   avanza(); return new StringLitExpr((String) t.valor, t.linea, t.columna);
            case NOMBRE: {
                avanza();
                IdentifierExpr id = new IdentifierExpr(t.texto, t.linea, t.columna);
                if (es("(")) {
                    Tok lp = avanza();
                    return new CallExpr(id, argumentos(), lp.linea, lp.columna);
                }
                return id;
            }
            default:
                break;
        }
        if (acepta("true"))  return new BoolLitExpr(true, t.linea, t.columna);
        if (acepta("false")) return new BoolLitExpr(false, t.linea, t.columna);
        if (acepta("null"))  return new NullLitExpr(t.linea, t.columna);
        if (acepta("(")) {
            IExpr dentro = expresion();
            espera(")");
            return new ParenExpr(dentro, t.linea, t.columna);
        }
        if (acepta("[")) {
            List<IExpr> el = new ArrayList<>();
            if (!es("]")) {
                el.add(expresion());
                while (acepta(",")) el.add(expresion());
            }
            espera("]");
            return new ArrayLitExpr(el, t.linea, t.columna);
        }
        if (acepta("new")) {
            // `new X(…)` es la `X(…)` de BP que construye (punto 22).
            Tok n = actual();
            String escrito = nombre("el nombre de la clase tras 'new'");
            IExpr callee = new IdentifierExpr(escrito, n.linea, n.columna);
            if (es(".") && mira(1).tipo == Tipo.NOMBRE) {
                avanza();
                Tok m = avanza();
                escrito = escrito + "." + m.texto;
                callee = new MemberAccessExpr(callee, m.texto, m.linea, m.columna);
            }
            // V8/H7.2 (#749) — `new List<Object>()`, `new Collections.Map<int, Object>()`: la construcción de BP, con
            // los argumentos de tipo de marca. Un Map de llave entera se crea en BP con `Collections.numericMap()` (su
            // comparador): eso es lo que queda en el árbol, y la marca dice cómo se escribió.
            List<TypeRef> targs = es("<") ? argumentosDeTipo() : null;
            Tok lp = espera("(", "'(' tras el nombre de la clase");
            List<IExpr> args = argumentos();
            boolean llaveEntera = targs != null && targs.size() == 2 && "Collections.Map".equals(escrito)
                    && args.isEmpty() && targs.get(0) instanceof SimpleTypeRef
                    && "integer".equals(((SimpleTypeRef) targs.get(0)).name);
            CallExpr c;
            if (llaveEntera) {
                c = new CallExpr(new MemberAccessExpr(((MemberAccessExpr) callee).target, "numericMap", lp.linea,
                                                      lp.columna), args, lp.linea, lp.columna);
                c.claseNew = escrito;
            } else {
                c = new CallExpr(callee, args, lp.linea, lp.columna);
                c.conNew = true;
            }
            c.argumentosNew = targs;
            return c;
        }
        if (acepta("this"))  return new ThisExpr(t.linea, t.columna);
        if (acepta("field")) return new FieldExpr(t.linea, t.columna);
        if (es("base")) {
            // `base.metodo()`: el `super` de BP (punto 34). El constructor de la base se llama con `: base(…)`.
            if (!mira(1).es(".")) throw error("tras 'base' va '.': base.metodo(); el constructor de la base, con : base(…)");
            avanza();
            return new SuperExpr(t.linea, t.columna);
        }
        if (es("super")) throw error("en JC es 'base', como en C# (punto 34)");
        if (t.tipo == Tipo.RESERVADA && TIPOS.containsKey(t.texto))
            throw error("en JC, una conversión se escribe (" + t.texto + ") x (punto 23)");
        throw error("se esperaba una expresión, encontrado " + t);
    }
}
