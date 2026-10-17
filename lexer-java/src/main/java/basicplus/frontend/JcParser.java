// ============================================================
// JcParser.java
// V8 #698 (JC, paso 3) — el parser de JC: de los tokens de JcLexer, al MISMO
// árbol que el parser de BP.
//
// La sintaxis es la que decidió Eduardo el 17-oct (V8_IDEAS §E2, puntos 1–33):
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
// Lo que la primera fase deja fuera (las clases, el switch, parallel…) es un
// error con su motivo. El primer error para el parser: no hay recuperación.
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
        return new JcLexer.Error(msg, t.linea, t.columna);
    }

    private JcLexer.Error segundaFase(String que) {
        return error(que + " va en la segunda fase de JC");
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
    /** Una declaración del módulo, con su anotación delante si la lleva (punto 27): se cuelga de la primera. */
    private void declaracion(List<ITopLevelDecl> defs) {
        Annotation ann = es("@") ? anotacion() : null;
        int antes = defs.size();
        declaracionSinAnotacion(defs);
        if (ann != null && defs.size() > antes) ((Node) defs.get(antes)).annotation = ann;
    }

    private void declaracionSinAnotacion(List<ITopLevelDecl> defs) {
        boolean isPublic = false, isPrivate = false, isFinal = false, isSync = false, isNative = false, isOwner = false;
        while (true) {
            if (es("public") || es("private")) {
                if (isPublic || isPrivate)
                    throw error("'public' y 'private' son excluyentes, y van una vez");
                if (avanza().texto.equals("public")) isPublic = true; else isPrivate = true;
            } else if (acepta("final")) {
                isFinal = true;
            } else if (acepta("sync")) {
                isSync = true;
            } else if (acepta("native")) {
                isNative = true;
            } else if (acepta("owner")) {
                isOwner = true;
            } else if (es("static")) {
                throw segundaFase("static, con las clases,");
            } else {
                break;
            }
        }
        if (es("class")) throw segundaFase("una clase");
        if (es("event")) throw segundaFase("un evento, con las clases,");
        if (isNative && !es("function")) throw error("'native' sólo se aplica a 'function'");
        if (es("const")) {
            defs.add(constante(isPublic));
            return;
        }
        if (es("function")) {
            defs.add(funcion(isPublic, isPrivate, isFinal, isNative));
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
            defs.add(propiedad(isPublic, isPrivate, isFinal, isSync, isOwner, tipo, nombre, n));
            return;
        }
        if (isSync) throw error("'sync' sólo se aplica a una propiedad");
        for (VarDecl v : variables(isPublic, isOwner, tipo, n, nombre)) {
            v.isPrivate = isPrivate;
            defs.add(v);
        }
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

    /** `const int MAX = 10;`, con el tipo siempre (punto 29). */
    private ConstDecl constante(boolean isPublic) {
        Tok tok = espera("const");
        TypeRef tipo = tipo();
        Tok n = actual();
        String nm = nombre("el nombre de la constante");
        espera("=", "'=' en la declaración de la constante");
        IExpr v = expresion();
        espera(";");
        return new ConstDecl(isPublic, new DeclName(null, nm, n.linea, n.columna), tipo, v, tok.linea, tok.columna);
    }

    /**
     * `int a, b;` · `int a = 1, b = 2;` · `int nums[10];` (puntos 5 y 33), tras el tipo y el primer nombre. Sin ningún
     * valor, un VarDecl con todos los nombres, como el `var a, b: integer` de BP; con algún valor, uno por nombre, como
     * sus grupos.
     */
    private List<VarDecl> variables(boolean isPublic, boolean isOwner, TypeRef tipo, Tok primero, String primerNombre) {
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
                ds.add(new DeclName(null, nombres.get(i), posiciones.get(i).linea, posiciones.get(i).columna));
            out.add(new VarDecl(isPublic, isOwner, ds, conTamano(tipo, tamanos.get(0)), null,
                                primero.linea, primero.columna));
            return out;
        }
        for (int i = 0; i < nombres.size(); i++) {
            List<DeclName> ds = new ArrayList<>();
            ds.add(new DeclName(null, nombres.get(i), posiciones.get(i).linea, posiciones.get(i).columna));
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
     *  32). La forma larga va con las clases. */
    private PropertyDef propiedad(boolean isPublic, boolean isPrivate, boolean isFinal, boolean isSync, boolean isOwner,
                                  TypeRef tipo, String nombre, Tok n) {
        espera("{");
        if (!(esNombre() && actual().texto.equals("get") && mira(1).es(";")))
            throw segundaFase("una propiedad con get y set escritos, con las clases,");
        avanza();
        avanza();
        if (!(esNombre() && actual().texto.equals("set") && mira(1).es(";")))
            throw error("la propiedad de JC es { get; set; } (punto 32)");
        avanza();
        avanza();
        espera("}");
        IExpr init = null;
        if (acepta("=")) {
            init = expresion();
            espera(";");
        }
        PropertyDef p = new PropertyDef(isPublic, isFinal, isOwner, isSync, new DeclName(null, nombre, n.linea, n.columna),
                                        tipo, init, null, null, true, n.linea, n.columna);
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
     */
    private FuncDef funcion(boolean isPublic, boolean isPrivate, boolean isFinal, boolean isNative) {
        Tok tok = espera("function");
        boolean isEventHandler = acepta("event");
        if (es("~")) throw segundaFase("el destructor, con las clases,");
        if (esNombre() && mira(1).es("("))
            throw error("falta el tipo de la función; si no devuelve nada, void (un constructor va con las clases, en la"
                    + " segunda fase de JC)");
        TypeRef ret = acepta("void") ? null : tipo();
        Tok nt = actual();
        String n = nombre("el nombre de la función");
        DeclName name = new DeclName(null, n, nt.linea, nt.columna);
        espera("(", "'(' tras el nombre de la función");
        List<Param> ps = new ArrayList<>();
        if (!es(")")) {
            ps.add(parametro());
            while (acepta(",")) ps.add(parametro());
        }
        espera(")");
        if (enInterfaz) {
            // Como en BP: la función de una interfaz es sólo su firma.
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
            cuerpoAviso.add(new ThrowStmt(new CallExpr(new IdentifierExpr("RuntimeError", tok.linea, tok.columna),
                    argsAviso, tok.linea, tok.columna), tok.linea, tok.columna));
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
        FuncDef fd = new FuncDef(isPublic, isFinal, intrinseca, isNative, name, ps, ret, body, tok.linea, tok.columna);
        fd.isEventHandler = isEventHandler;
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

    /** ¿Empieza aquí una declaración de variables? Un tipo seguido de un nombre: `int x`, `List l`, `Mod.T x`,
     *  `List[] xs`. */
    private boolean esDeclaracion() {
        Tok t = actual();
        if (t.tipo == Tipo.RESERVADA) return TIPOS.containsKey(t.texto);
        if (t.tipo != Tipo.NOMBRE) return false;
        int k = 1;
        if (mira(1).es(".") && mira(2).tipo == Tipo.NOMBRE) k = 3;
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
        if (es("print"))    { out.add(imprime()); return; }
        if (es("const"))    { out.add(constante(false)); return; }
        if (acepta("throw")) {
            IExpr v = expresion();
            espera(";");
            out.add(new ThrowStmt(v, t.linea, t.columna));
            return;
        }
        if (acepta("break"))    { espera(";"); out.add(new BreakStmt(t.linea, t.columna)); return; }
        if (acepta("continue")) { espera(";"); out.add(new ContinueStmt(t.linea, t.columna)); return; }
        if (es("switch"))   throw segundaFase("switch");
        if (es("parallel")) throw segundaFase("parallel");
        if (es("raise"))    throw segundaFase("raise, con los eventos,");
        if (es("inline"))
            throw error("en JC, inline es el cuerpo entero de una función: function T f(…) inline …; (punto 26)");
        if (es("owner") || esDeclaracion()) {
            boolean owner = acepta("owner");
            TypeRef tipo = tipo();
            Tok n = actual();
            String nm = nombre("el nombre de la variable");
            out.addAll(variables(false, owner, tipo, n, nm));
            return;
        }
        if (es("(") && tuplaAdelante()) {
            int cierre = cierre();
            // `(string, int) t = par();`, una variable de tipo tupla; `(n, s) = par();`, el desempaquetado.
            if (cierre > 0 && toks.get(cierre + 1).tipo == Tipo.NOMBRE) {
                TypeRef tipo = tipo();
                Tok n = actual();
                String nm = nombre("el nombre de la variable");
                out.addAll(variables(false, false, tipo, n, nm));
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
        List<IStmt> body = cuerpo();
        espera("while", "'while' tras el cuerpo del do");
        espera("(");
        IExpr c = expresion();
        espera(")");
        espera(";");
        return new DoLoopStmt(body, c, t.linea, t.columna);
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

    /** El `print` de JC: piezas sin nada entre ellas y sin salto (punto 17). Es el `print a; b;` de BP, con el `;` al
     *  final (#700). */
    private PrintStmt imprime() {
        Tok t = espera("print");
        espera("(");
        if (es(")")) throw error("un print vacío no imprime nada; el salto de línea se escribe: print(\"\\n\");");
        List<PrintItem> items = new ArrayList<>();
        items.add(new PrintItem(PrintSep.NONE, expresion()));
        while (acepta(",")) items.add(new PrintItem(PrintSep.SEMICOLON, expresion()));
        espera(")");
        espera(";");
        items.add(new PrintItem(PrintSep.SEMICOLON, null));
        return new PrintStmt(items, t.linea, t.columna);
    }

    /** Una asignación (`=`, `+=`, `-=`, y `v++` y `v--`, que son `+= 1` y `-= 1`: punto 12), o una llamada. */
    private IStmt asignacionOLlamada() {
        Tok t = actual();
        if (es("++") || es("--")) throw error("en JC, ++ y -- van detrás: v++; (punto 12)");
        IExpr lhs = postfijo();
        if (es("=") || es("+=") || es("-=")) {
            String op = avanza().texto;
            IExpr v = expresion();
            espera(";");
            AssignOpKind k = op.equals("+=") ? AssignOpKind.PLUS_ASSIGN
                           : op.equals("-=") ? AssignOpKind.MINUS_ASSIGN : AssignOpKind.ASSIGN;
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
        if (es("*=") || es("/=") || es("%=") || es("&=") || es("|=") || es("^="))
            throw error("JC no tiene los demás compuestos de C#: BP sólo tiene =, += y -= (punto 15)");
        throw error("se esperaba una asignación o una llamada como sentencia, encontrado " + actual());
    }

    // ============================================================
    // EXPRESIONES (la precedencia de C#, punto 18; el árbol, el de BP)
    // ============================================================
    private IExpr expresion() { return o(); }

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
                throw segundaFase("obj::metodo");
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
            IExpr callee = new IdentifierExpr(nombre("el nombre de la clase tras 'new'"), n.linea, n.columna);
            if (es(".") && mira(1).tipo == Tipo.NOMBRE) {
                avanza();
                Tok m = avanza();
                callee = new MemberAccessExpr(callee, m.texto, m.linea, m.columna);
            }
            Tok lp = espera("(", "'(' tras el nombre de la clase");
            CallExpr c = new CallExpr(callee, argumentos(), lp.linea, lp.columna);
            c.conNew = true;
            return c;
        }
        if (es("this") || es("super")) throw segundaFase(t.texto + ", con las clases,");
        if (t.tipo == Tipo.RESERVADA && TIPOS.containsKey(t.texto))
            throw error("en JC, una conversión se escribe (" + t.texto + ") x (punto 23)");
        throw error("se esperaba una expresión, encontrado " + t);
    }
}
