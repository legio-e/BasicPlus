// ============================================================
// SemanticAnalyzer.java
// Análisis semántico de BASICPLUS en tres pases (port del C#).
//
//   1) DECLARACIONES — construye ModuleSymbol y los símbolos
//      top-level. Para cada clase, separa miembros de instancia
//      vs estáticos (basado en el prefijo cualificado del nombre).
//      Identifica constructores, inicializador de módulo y Main.
//
//   2) RESOLUCIÓN DE TIPOS — resuelve TypeRef → BpType, resuelve
//      'extends' a ClassSymbol, detecta ciclos, asigna tipo a
//      parámetros, vars, consts, propiedades.
//
//   3) CUERPOS — recorre cada función, resuelve identificadores
//      con scope, chequea tipos, aplica reglas contextuales
//      (this/super/field/final/estáticos), checa visibilidad,
//      switch duplicates+exhaustividad enum, catch order, etc.
//
// Compatible con JDK 8.
// ============================================================
package basicplus.frontend;

import basicplus.frontend.Ast.*;
import basicplus.frontend.BpType.*;
import basicplus.frontend.Symbol.*;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

public final class SemanticAnalyzer {

    /** El modulo raiz del lenguaje: importado implicitamente en todos, y de
     *  donde salen `Object`, `List` y los envoltorios. Su nombre manda al
     *  resolver un tipo sin cualificar. */
    private static final String CORE_MODULE = "Core";


    private final SemanticInfo info = new SemanticInfo();
    private ModuleSymbol module;

    /**
     * Anti-cascada: nombres ya reportados como "identificador no resuelto"
     * (se reporta 1 sola vez por nombre; los usos siguientes se silencian y
     * propagan ErrorType). Per-instancia = per-módulo.
     */
    private final java.util.Set<String> reportedUnresolved = new java.util.HashSet<>();

    /** V6/#458 — ¿este módulo declara `import Core`?
     *
     *  Desde que la norma es «si usas un tipo de Core, importa Core» (Eduardo,
     *  30-ago), el motivo MÁS probable de un nombre sin resolver es justo ése. Y
     *  sin decirlo, la norma se limita a cambiar un error confuso por otro: quien
     *  escribe `List` y lee «tipo no encontrado» se va a mirar si lo ha escrito
     *  mal, no a añadir un import. */
    private boolean coreImportado() {
        for (Symbol.ImportedNamespaceSymbol ns : preloadedImports)
            if ("Core".equals(ns.moduleName)) return true;
        return false;
    }

    /** Los nombres de clase que EXPORTA `Core`, para la pista de abajo.
     *
     *  Los rellena `Main` leyendo la interfaz de Core, y NO se escriben aquí a
     *  mano: una lista copiada viviría en dos sitios y una de las dos se quedaría
     *  rancia. Si Core no se puede cargar el conjunto queda vacío y la pista
     *  simplemente no sale — degradar en silencio es correcto para un mensaje de
     *  ayuda, que no puede permitirse mentir. */
    private static final java.util.Set<String> NOMBRES_DE_CORE = new java.util.HashSet<>();

    public static void registrarNombresDeCore(java.util.Collection<String> nombres) {
        if (nombres != null) NOMBRES_DE_CORE.addAll(nombres);
    }

    /** El sufijo de la pista para el nombre `n`, o cadena vacía.
     *
     *  ⚠️ Sólo si `n` ES de verdad una clase de Core. La primera versión la pegaba
     *  a CUALQUIER identificador sin resolver, y el censo de samples la pilló
     *  diciéndole a `SQLite` que se importara Core — o sea mandando a mirar donde
     *  no es, que es justo el pecado que esta pista venía a evitar. */
    private String pistaCore(String n) {
        if (coreImportado() || !NOMBRES_DE_CORE.contains(n)) return "";
        return " — `" + n + "` vive en `Core`: este módulo necesita `import Core`";
    }

    /**
     * Anti-cascada: tipos ya reportados como "no encontrado" (1 vez por nombre).
     * Un tipo mal escrito usado en N declaraciones daba N errores; ahora 1.
     */
    private final java.util.Set<String> reportedUnknownType = new java.util.HashSet<>();

    /** H5.a-E3 — clase ESTÁTICA del receptor de cada `obj.metodo` resuelto a
     *  función. La sobrecarga de métodos se resuelve sobre esa clase (ve las suyas
     *  y las heredadas); `lookupInstance` solo devuelve una, así que analyzeCall
     *  necesita saber desde dónde juntar los candidatos. */
    private final java.util.Map<Ast.IExpr, ClassSymbol> maRecvClass = new java.util.IdentityHashMap<>();

    // ---------- Estado del recorrido (pase 3) ----------
    private ClassSymbol currentClass;
    private FunctionSymbol currentFunction;
    /** L8 v3 — true mientras se analiza el cuerpo de una `function native`
     *  (el AotCEmitter no genera arrays fijos locales → error honesto). */
    private boolean currentFunctionIsNative = false;
    private boolean insideGetter;
    private boolean insideSetter;
    /** #547 — el tipo de la property cuyo get/set se analiza: el de {@code field}. */
    private BpType currentPropType;
    /**
     * Profundidad de anidamiento de cuerpos de {@code case} dentro de un
     * {@code parallel}. Cuando > 0, la resolución de identificadores rechaza
     * acceder a VarSymbols del módulo (encapsulación de objeto): cada thread
     * debe ser autocontenido.
     */
    private int parallelCaseDepth;
    private String setterParamName;
    private int loopDepth;
    private int switchDepth;
    /** V7 #674 (9-oct, Eduardo: «Prohibirlo») — los `finally` abiertos, con {loopDepth, switchDepth} al entrar en cada
     *  uno: un `return` dentro de un `finally`, o un `break`/`continue` que sale de él (su bucle o su switch está FUERA),
     *  es un error —taparía la excepción que fuera en vuelo; y el compilador se caía con un `StackOverflowError`—. */
    private final java.util.ArrayDeque<int[]> finallysAbiertos = new java.util.ArrayDeque<>();
    /** V8/H8 — los bucles `do` con nombre que están abiertos, de dentro afuera: {nombre, su loopDepth}. */
    private java.util.ArrayDeque<Object[]> buclesConNombre = new java.util.ArrayDeque<>();
    private boolean seenSuperCallInCtor;
    private boolean isFirstStmtOfCtor;

    /** V8 #698 — el módulo se leyó de un `.jc`: se comprueba que el `new` y el `(T) x` escritos casan con lo que
     *  cada llamada es ({@link #revisaJc}). */
    private boolean enJc = false;
    /** V8 H7.3 — el módulo con el que se calculan las claves de MOD9 (`area()d`) al comprobar una interfaz: las
     *  mismas que casará el cargador. */
    private String moduloDeLasClaves = null;
    /** V8 #698 — las llamadas `MiClase(…)` que delegan en otro constructor (#511); en JC, el `: this(…)`. */
    private final Set<CallExpr> delegacionesCtor = java.util.Collections.newSetFromMap(new java.util.IdentityHashMap<>());
    /** Las conversiones primitivas de BP, `integer(x)`…: las mismas que escribe EscritorJc como `(T) x`. */
    private static final Set<String> CONVERSIONES_PRIMITIVAS = new HashSet<>(java.util.Arrays.asList(
            "integer", "long", "float", "double", "byte", "int8", "word", "int16"));

    /** Imports cargados desde .bpi antes de analizar, indexados por alias. */
    private final java.util.List<Symbol.ImportedNamespaceSymbol> preloadedImports = new java.util.ArrayList<>();

    /** Interface .bpi pre-cargada contra la que se debe verificar conformidad. */
    private ModuleInterface implementsContract = null;

    /**
     * Registra un módulo importado (típicamente cargado desde un .bpi) bajo
     * su alias. Debe llamarse ANTES de analyze(). Tras declarePass, el
     * namespace se introduce en module.members; cualquier colisión con
     * declaraciones del usuario produce un error.
     */
    public void registerImport(Symbol.ImportedNamespaceSymbol ns) {
        preloadedImports.add(ns);
    }

    /**
     * Registra la interfaz contra la que este módulo declara `implements`.
     * Tras el análisis, verificamos que todas las firmas declaradas en la
     * interfaz están implementadas con la misma signatura.
     */
    public void setImplementsContract(ModuleInterface iface) {
        this.implementsContract = iface;
    }

    public SemanticInfo analyze(ModuleNode mod) {
        enJc = mod.escritoEnJc;
        moduloDeLasClaves = MivmEmitter.moduloLocalDe(mod);
        cadenasNacenVacias(mod);
        declarePass(mod);
        injectImports(mod);
        resolveTypesPass(mod);
        bodyPass(mod);
        validateAnnotations(mod);
        validateModuleEntryPoints(mod);
        verifyImplementsContract(mod);
        return info;
    }

    /**
     * V8 #774 (Eduardo, 2-nov: «si a fuese una variable de módulo o de clase entonces var a:string; estaría inicializada
     * a:string:=""») — una variable de módulo, un campo o una property corta de tipo `string` sin valor nace `""`: se le
     * pone aquí, antes de nada, el inicializador `""`, y lo demás (el emisor, el AOT, el escritor BP ↔ JC) la ve como si
     * el programa lo hubiera escrito. Hasta ahora nacía «sin valor» (la referencia nula, que casi todo leía como "" salvo
     * `==`: #773). Una LOCAL sin valor no: sigue indeterminada (#540). Una property con su get o su set lleva su propio
     * almacén, y un array fijo nace a cero.
     */
    private static void cadenasNacenVacias(ModuleNode mod) {
        for (ITopLevelDecl d : mod.defs) {
            if (d instanceof VarDecl) cadenaVacia((VarDecl) d);
            else if (d instanceof PropertyDef) cadenaVacia((PropertyDef) d);
            else if (d instanceof ClassDef && !((ClassDef) d).isInterface)   // una interfaz no guarda nada: sus property, sin valor
                for (ITopLevelDecl m : ((ClassDef) d).members) {
                    if (m instanceof VarDecl) cadenaVacia((VarDecl) m);
                    else if (m instanceof PropertyDef) cadenaVacia((PropertyDef) m);
                }
        }
    }

    private static boolean esTipoCadena(TypeRef t) {
        return t instanceof SimpleTypeRef && "string".equals(((SimpleTypeRef) t).name) && ((SimpleTypeRef) t).argumentos == null;
    }

    private static void cadenaVacia(VarDecl vd) {
        if (vd.init == null && !vd.isOwner && esTipoCadena(vd.type)) vd.init = vacia(vd);
    }

    private static void cadenaVacia(PropertyDef pd) {
        if (pd.init == null && !pd.isOwner && pd.getter == null && pd.setter == null && esTipoCadena(pd.type)) pd.init = vacia(pd);
    }

    private static StringLitExpr vacia(Node donde) {
        StringLitExpr e = new StringLitExpr("", donde.line, donde.column);
        e.implicito = true;
        return e;
    }

    /**
     * Modo "interface-only": parsea + analiza solamente los pases que dependen
     * de declaraciones (no de bodies). Suficiente para extraer la firma
     * pública del módulo y emitir un .bpi sin necesidad de tener resueltos
     * los imports. No corre bodyPass ni valida entry points.
     */
    public SemanticInfo analyzeInterface(ModuleNode mod) {
        // V8 #774 — también aquí: el "" de un campo string sin valor puede darle a su clase un constructor sintético
        // (sintetizarCtores, en resolveTypesPass), y eso es interfaz. Sin esto, la pasada de interfaz no lo veía y la
        // emisión sí («la interfaz de 'Gui' cambió entre la pasada de interfaz y la emisión»: Gui.Component).
        cadenasNacenVacias(mod);
        declarePass(mod);
        // H6.a — SÍ inyectamos los imports precargados (registerImport). Aunque
        // en modo interfaz no se procesan bodies, las FIRMAS públicas pueden
        // referenciar TIPOS importados (p.ej. `func crear(): C.Caja`), y
        // resolveTypesPass necesita el namespace del import en scope para
        // resolverlos; sin esto la firma queda con tipo <error> y se cae de la
        // interfaz. Los imports los provee la pasada de interfaz transitiva del
        // driver (Main.compileInterface) antes de llamar aquí.
        injectImports(mod);
        resolveTypesPass(mod);
        // V5/H5 — las anotaciones se validan TAMBIÉN aquí. Un módulo con
        // entidades se compila muchas veces en modo interfaz (como dependencia
        // de otro), y si sólo validara la pasada completa, un `@BD` mal escrito
        // pasaría desapercibido en todas ellas.
        validateAnnotations(mod);
        return info;
    }

    private void injectImports(ModuleNode mod) {
        if (module == null || preloadedImports.isEmpty()) return;
        for (Symbol.ImportedNamespaceSymbol ns : preloadedImports) {
            if (!module.members.tryDefine(ns)) {
                err(mod.line, mod.column,
                    "el alias de import '" + ns.name + "' colisiona con una declaración local");
            }
        }
        // #248 — alias sin cualificar de las clases de Core: `RuntimeError(...)`
        // y `catch e: Exception` se escriben igual que siempre, resolviendo a
        // las clases ÚNICAS de Core (stubs del .bpi). tryDefine cede ante una
        // clase local homónima del usuario (compat #220).
        for (Symbol.ImportedNamespaceSymbol ns : preloadedImports) {
            if ("Core".equals(ns.moduleName)) {
                Symbol exc = ns.classes.get("Exception");
                Symbol rte = ns.classes.get("RuntimeError");
                if (exc != null) module.members.tryDefine(exc);
                if (rte != null) module.members.tryDefine(rte);
                /* #450 — `List` se escribe SIN cualificar, como siempre. Antes lo
                 * conseguia siendo una clase sintetizada; ahora vive en Core y se
                 * aliasa igual que Exception. Los envoltorios van con ella: quien
                 * usa `Object` de comodin escribe `Integer(o)`, no
                 * `Core.Integer(o)`. */
                // H1 P3 (20-sep) — TODAS las clases públicas de Core, sin lista a mano: Object,
                // Thread, Mutex, StringBuilder, List, los envoltorios… (la raíz entera).
                for (Symbol c : ns.classes.values()) module.members.tryDefine(c);
            } else if ("Collections".equals(ns.moduleName)) {
                /* #450 — idem para las dos que se quedaron en Collections. */
                for (String n : new String[]{"SyncList", "OwnerList"}) {
                    Symbol c = ns.classes.get(n);
                    if (c != null) module.members.tryDefine(c);
                }
            }
        }
        enlazarInterfacesImportadas();
    }

    /** V8 H7.3 (#756) — los stubs importados: una interfaz cuelga de Object (como una del módulo: sus métodos valen sobre
     *  ella, y se asigna a Object), y las interfaces de cada stub, resueltas a los stubs de este análisis por su nombre
     *  (`I`, del mismo módulo; `Mod.I`, de otro). Una de un módulo que éste no importa, un stub sin miembros: basta para
     *  comparar tipos (ClassSymbol.mismaClase compara por módulo y nombre). */
    private void enlazarInterfacesImportadas() {
        Symbol obj = module.members.resolve("Object");
        for (Symbol.ImportedNamespaceSymbol ns : preloadedImports) {
            for (ClassSymbol c : ns.classes.values()) {
                if (c.isInterface && c.baseClass == null && obj instanceof ClassSymbol) c.baseClass = (ClassSymbol) obj;
                if (!c.interfaces.isEmpty() || c.interfaceNames.isEmpty()) continue;
                for (String n : c.interfaceNames) {
                    ClassSymbol i = stubDeInterfaz(ns, n);
                    if (i != null) c.interfaces.add(i);
                }
            }
        }
    }

    private ClassSymbol stubDeInterfaz(Symbol.ImportedNamespaceSymbol desde, String n) {
        int dot = n.lastIndexOf('.');
        if (dot < 0) return desde.classes.get(n);
        String mod = n.substring(0, dot), simple = n.substring(dot + 1);
        for (Symbol.ImportedNamespaceSymbol ns : preloadedImports) {
            String canon = (ns.library == null || ns.library.isEmpty()) ? ns.moduleName : ns.library + "." + ns.moduleName;
            if (canon.equals(mod) && ns.classes.get(simple) != null) return ns.classes.get(simple);
        }
        ClassSymbol s = new ClassSymbol(simple, true, null, null, 0, 0);
        s.isExternal = true;
        s.isInterface = true;
        int d2 = mod.lastIndexOf('.');
        s.externalModule = d2 < 0 ? mod : mod.substring(d2 + 1);
        s.externalLibrary = d2 < 0 ? "" : mod.substring(0, d2);
        return s;
    }

    // ============================================================
    // STDLIB BUILT-IN — clases y funciones pre-registradas
    //   - RuntimeError(msg: string)        → java.lang.RuntimeException
    //   - input(): string                   → Scanner.nextLine()
    //   - parseInt(s: string): integer      → Long.parseLong
    //   - parseFloat(s: string): float      → Double.parseDouble
    //   - strlen(s: string): integer        → s.length()
    // ============================================================
    private static Scope BUILTIN_SCOPE_CACHE;

    private static Scope builtinScope() {
        if (BUILTIN_SCOPE_CACHE != null) return BUILTIN_SCOPE_CACHE;
        Scope s = new Scope("builtin", null);

        // #248 — RuntimeError ya NO se sintetiza aquí: vive en Core (stdlib) y
        // llega como stub via el `import Core` implícito (injectImports lo
        // aliasa sin cualificar). Así hay UNA clase real por VM en lugar de
        // una copia por módulo.

        // H1 P2 (20-sep) — las intrínsecas GLOBALES (`strlen`, `abs`, `parseInt`, las `__` del
        // compilador…) ya NO se declaran aquí: viven en `bpstdlib/Core.bp` (el módulo raíz) con
        // su firma y su bytecode (`inline …`), y `Main.cargarRaiz` las registra en este scope al
        // arrancar cada compilación (registrarGlobalDeRaiz). Aquí quedan sólo las CLASES del
        // lenguaje (Object, Thread, Mutex, StringBuilder…): P3.
        // Las CUATRO excepciones: los casts estrechos `byte(i)`/`int8(i)`/`word(i)`/`int16(i)`.
        // Son palabras reservadas del lenguaje (tipos) y BP no puede declarar una función con
        // ese nombre, así que siguen aquí, como los casts `integer()`/`long()` (numericCastTarget).
        addBuiltin(s, "byte",  PrimitiveType.UINT8,  new String[]{"i"}, new BpType[]{PrimitiveType.INTEGER});
        addBuiltin(s, "int8",  PrimitiveType.INT8,   new String[]{"i"}, new BpType[]{PrimitiveType.INTEGER});
        addBuiltin(s, "word",  PrimitiveType.UINT16, new String[]{"i"}, new BpType[]{PrimitiveType.INTEGER});
        addBuiltin(s, "int16", PrimitiveType.INT16,  new String[]{"i"}, new BpType[]{PrimitiveType.INTEGER});


        // H1 P3 (20-sep) — Object, Thread, Mutex y StringBuilder ya NO se declaran aquí (eran 20
        // makeMethod sin AST): viven en `bpstdlib/Core.bp` como clases BP y llegan por el
        // `import Core` que el parser pone en todo módulo. Este scope se queda con los 4 casts.

        BUILTIN_SCOPE_CACHE = s;
        return s;
    }

    /**
     * L10 — ¿Se admite asignar `sourceExpr` (de tipo `source`) a una variable
     * de tipo `target` aun cuando `target.isAssignableFrom(source)` sea false?
     *
     * El único caso especial: `target` es un tipo estrecho (byte/int8/word/int16),
     * `source` es INTEGER, y `sourceExpr` es un literal cuyo valor cabe en el
     * rango del tipo estrecho. Ejemplos:
     *   var b: byte := 250        ✓  (250 ∈ [0,255])
     *   var b: byte := -1         ✗  (-1 fuera de [0,255])
     *   var b: byte := someInt    ✗  (no es literal: requiere byte(someInt))
     */
    static boolean acceptsNarrowFromLiteral(BpType target, BpType source, IExpr sourceExpr) {
        if (!(target instanceof PrimitiveType)) return false;
        PrimitiveType t = (PrimitiveType) target;
        if (!t.isNarrowInteger()) return false;
        if (!(source instanceof PrimitiveType
                && ((PrimitiveType) source).tag == PrimitiveType.Kind.INTEGER)) return false;
        Long lit = constantIntValueOf(sourceExpr);
        if (lit == null) return false;
        return lit >= t.rangeMin() && lit <= t.rangeMax();
    }

    /** Si la expresión es un literal entero (con paréntesis y unario `-`
     *  opcionales), devuelve su valor. Null si no es un literal constante
     *  reconocido. NO hace constant-folding general — sólo casos triviales. */
    static Long constantIntValueOf(IExpr e) {
        while (e instanceof ParenExpr) e = ((ParenExpr) e).inner;
        if (e instanceof IntLitExpr) return ((IntLitExpr) e).value;
        if (e instanceof UnaryExpr) {
            UnaryExpr u = (UnaryExpr) e;
            if ("-".equals(u.op)) {
                Long inner = constantIntValueOf(u.operand);
                if (inner != null) return -inner;
            }
        }
        return null;
    }

    // ============================================================
    // H8.1 — Parámetros por defecto: validación + helpers de literal
    // ============================================================

    /** Valida y normaliza los valores por defecto de los parámetros de una función:
     *  deben ser literales constantes asignables al tipo del parámetro, y todos los
     *  que tengan default deben ir AL FINAL. Cada default válido se normaliza a una
     *  expresión literal reconstruida desde su valor (misma forma local/cross-module). */
    private void resolveParamDefaults(FunctionSymbol fs, FuncDef f) {
        resolveParamDefaults(fs.params, f.params, f);
    }

    /** H5.c — la misma validación sirve para la firma de un EVENTO: sus parámetros
     *  son parámetros con las mismas reglas. Se comparte el cuerpo en vez de
     *  duplicarlo para que los defaults de eventos y de funciones no puedan
     *  divergir. */
    private void resolveParamDefaults(List<ParamSymbol> params, List<Param> decl, Node fallback) {
        boolean seenDefault = false;
        for (int i = 0; i < params.size(); i++) {
            ParamSymbol ps = params.get(i);
            IExpr raw = ps.defaultExpr;
            if (raw == null) {
                if (seenDefault) {
                    Node nn = (i < decl.size()) ? (Node) decl.get(i) : fallback;
                    err(nn.line, nn.column, "el parámetro '" + ps.name
                        + "' sin valor por defecto no puede ir tras uno con valor por defecto");
                }
                continue;
            }
            seenDefault = true;
            Node rawNode = (Node) raw;
            BpType natTy = literalNaturalType(raw);
            if (natTy == null) {
                err(rawNode.line, rawNode.column, "el valor por defecto del parámetro '"
                    + ps.name + "' debe ser una constante literal");
                ps.defaultExpr = null;
                continue;
            }
            BpType pt = ps.type;
            if (pt instanceof PrimitiveType && ((PrimitiveType) pt).isNarrowInteger()) {
                err(rawNode.line, rawNode.column, "valor por defecto en parámetro de tipo estrecho ('"
                    + pt.display() + "') no soportado todavía");
                ps.defaultExpr = null;
                continue;
            }
            if (pt != null && !pt.isAssignableFrom(natTy)) {
                err(rawNode.line, rawNode.column, "valor por defecto del parámetro '" + ps.name
                    + "': '" + natTy.display() + "' no asignable a '" + pt.display() + "'");
                ps.defaultExpr = null;
                continue;
            }
            ps.defaultExpr = literalExprFromValue(constLiteralValue(raw), pt, rawNode.line, rawNode.column);
        }
    }

    /** Tipo natural de una expresión literal constante (int/long/float/double/string/
     *  boolean, con paréntesis y unario `-` opcionales), o null si no es una. */
    private static BpType literalNaturalType(IExpr e) {
        while (e instanceof ParenExpr) e = ((ParenExpr) e).inner;
        if (e instanceof UnaryExpr) {
            UnaryExpr u = (UnaryExpr) e;
            if ("-".equals(u.op)) {
                BpType inner = literalNaturalType(u.operand);
                return (inner != null && inner.isNumeric()) ? inner : null;
            }
            return null;
        }
        if (e instanceof IntLitExpr)    return PrimitiveType.INTEGER;
        if (e instanceof LongLitExpr)   return PrimitiveType.LONG;
        if (e instanceof FloatLitExpr)  return PrimitiveType.FLOAT;
        if (e instanceof DoubleLitExpr) return PrimitiveType.DOUBLE;
        if (e instanceof StringLitExpr) return PrimitiveType.STRING;
        if (e instanceof BoolLitExpr)   return PrimitiveType.BOOLEAN;
        return null;
    }

    /** Valor (Long/Double/String/Boolean) de un literal constante, o null. */
    static Object constLiteralValue(IExpr e) {
        while (e instanceof ParenExpr) e = ((ParenExpr) e).inner;
        if (e instanceof UnaryExpr) {
            UnaryExpr u = (UnaryExpr) e;
            if ("-".equals(u.op)) {
                Object v = constLiteralValue(u.operand);
                if (v instanceof Long)   return -((Long) v);
                if (v instanceof Double) return -((Double) v);
            }
            return null;
        }
        if (e instanceof IntLitExpr)    return ((IntLitExpr) e).value;
        if (e instanceof LongLitExpr)   return ((LongLitExpr) e).value;
        if (e instanceof FloatLitExpr)  return ((FloatLitExpr) e).value;
        if (e instanceof DoubleLitExpr) return ((DoubleLitExpr) e).value;
        if (e instanceof StringLitExpr) return ((StringLitExpr) e).value;
        if (e instanceof BoolLitExpr)   return ((BoolLitExpr) e).value;
        return null;
    }

    /** Reconstruye una expresión literal desde su valor, con la forma adecuada al
     *  tipo del parámetro (para emitir igual que un literal escrito a mano). */
    static IExpr literalExprFromValue(Object v, BpType paramType, int line, int col) {
        PrimitiveType.Kind k = (paramType instanceof PrimitiveType) ? ((PrimitiveType) paramType).tag : null;
        if (v instanceof Long) {
            long lv = (Long) v;
            return (k == PrimitiveType.Kind.LONG) ? new LongLitExpr(lv, line, col)
                                                  : new IntLitExpr(lv, line, col);
        }
        if (v instanceof Double) {
            double dv = (Double) v;
            return (k == PrimitiveType.Kind.DOUBLE) ? new DoubleLitExpr(dv, line, col)
                                                    : new FloatLitExpr(dv, line, col);
        }
        if (v instanceof String)  return new StringLitExpr((String) v, line, col);
        if (v instanceof Boolean) return new BoolLitExpr((Boolean) v, line, col);
        return new IntLitExpr(0, line, col);
    }

    /** Clona una expresión literal constante con la posición dada (para inyectarla
     *  como argumento en un call-site sin compartir el nodo entre llamadas). */
    private static IExpr cloneConstExpr(IExpr e, int line, int col) {
        if (e instanceof IntLitExpr)    return new IntLitExpr(((IntLitExpr) e).value, line, col);
        if (e instanceof LongLitExpr)   return new LongLitExpr(((LongLitExpr) e).value, line, col);
        if (e instanceof FloatLitExpr)  return new FloatLitExpr(((FloatLitExpr) e).value, line, col);
        if (e instanceof DoubleLitExpr) return new DoubleLitExpr(((DoubleLitExpr) e).value, line, col);
        if (e instanceof StringLitExpr) return new StringLitExpr(((StringLitExpr) e).value, line, col);
        if (e instanceof BoolLitExpr)   return new BoolLitExpr(((BoolLitExpr) e).value, line, col);
        if (e instanceof UnaryExpr) {
            UnaryExpr u = (UnaryExpr) e;
            return new UnaryExpr(u.op, cloneConstExpr(u.operand, line, col), line, col);
        }
        if (e instanceof ParenExpr)
            return new ParenExpr(cloneConstExpr(((ParenExpr) e).inner, line, col), line, col);
        return e;
    }

    private static void addBuiltin(Scope s, String name, BpType returnType,
                                    String[] paramNames, BpType[] paramTypes) {
        FunctionSymbol fn = new FunctionSymbol(name, true, false, true, null, null);
        fn.returnType = returnType;
        for (int i = 0; i < paramNames.length; i++) {
            ParamSymbol p = new ParamSymbol(paramNames[i], 0, 0);
            p.type = paramTypes[i];
            fn.params.add(p);
        }
        s.tryDefine(fn);
    }

    /**
     * #388 — la clase BUILT-IN que se llama así (List, Map, StringBuilder…), o
     * null si no hay ninguna.
     *
     * <p>Existe para el resolvedor de imports: una firma cross-module que
     * devuelva o tome `List` llega como {@code UnresolvedClassRef("List")}, y él
     * sólo sabía mirar entre las clases DEL MÓDULO importado — donde `List` no
     * está, porque no es de nadie: la declara aquí el propio analizador. El tipo
     * se quedaba sin resolver y `dao.list(w).length()` decía «el tipo 'List' no
     * tiene miembros», aunque `var l: List := dao.list(w)` sí funcionara.
     */
    /** H1 P2 — ¿se está compilando el módulo raíz? Es `Core`, por nombre (el módulo del sistema). */
    private boolean esRaiz() { return module != null && "Core".equals(module.name); }
    /** H1 P3 (20-sep) — ¿es `cls` LA clase Object del lenguaje (la de Core: el stub importado, o la
     *  local si se compila Core)? No una clase de usuario que se llame igual. */
    boolean esObject(ClassSymbol cls) {
        return cls != null && "Object".equals(cls.name)
                && (cls.isExternal ? CORE_MODULE.equals(cls.externalModule) : esRaiz());
    }
    /** H1 P2/P3 — LA RAÍZ, SIN CUALIFICAR. Toda función pública de Core (el `import Core` que el
     *  parser pone siempre) entra en el scope `raiz` de ESTE analizador, entre el builtin y el
     *  módulo: `strlen`, `abs`, `len`… se escriben peladas en todo programa, como en la unidad
     *  `System` de Turbo Pascal. Por analizador, no por JVM: sus firmas nombran clases de Core
     *  (`Object[]`) y ésas son los stubs de ESTE módulo. Las sobrecargas ya vienen encadenadas en
     *  el namespace del import (P2c). Una función local del usuario con el mismo nombre gana
     *  (el scope del módulo está por encima). */
    private void registrarRaiz(Scope raiz) {
        for (Symbol.ImportedNamespaceSymbol ns : preloadedImports) {
            if (!CORE_MODULE.equals(ns.moduleName)) continue;
            for (FunctionSymbol f : ns.functions.values())
                if (f.isPublic) raiz.tryDefine(f);
        }
    }

    /** ¿Es 's' un símbolo built-in (no del usuario)? */
    public static boolean isBuiltin(Symbol sym) {
        Scope s = builtinScope();
        for (Symbol candidate : s.getSymbols())
            if (candidate == sym) return true;
        return false;
    }

    // ============================================================
    // PASE 1 — Declaraciones
    // ============================================================
    private void declarePass(ModuleNode mod) {
        Scope raiz = new Scope("raiz", builtinScope());   // H1 P3: las globales de Core, por analizador
        module = new ModuleSymbol(mod.name, raiz, mod.line, mod.column);
        registrarRaiz(raiz);
        module.decl = mod;
        info.module = module;
        info.declSymbols.put(mod, module);

        for (ITopLevelDecl def : mod.defs)
            declareTopLevel(def, null, module.members);

        // ¿inicializador de módulo? — función con el mismo nombre del módulo
        Symbol maybeInit = module.members.tryLookup(module.name);
        if (maybeInit instanceof FunctionSymbol) {
            FunctionSymbol fInit = (FunctionSymbol) maybeInit;
            module.initializer = fInit;
            fInit.isModuleInitializer = true;
            if (fInit.isStatic)
                err(fInit.line, fInit.column, "el inicializador del módulo no puede ser estático");
        }

        // Entry point: 'main' (preferido, con arg: string) o 'Main' (legacy, sin params).
        Symbol maybeMain = module.members.tryLookup("main");
        if (maybeMain == null) maybeMain = module.members.tryLookup("Main");
        if (maybeMain instanceof FunctionSymbol)
            module.mainFunction = (FunctionSymbol) maybeMain;

        // E1: detectar colisiones entre los accesores sintéticos que va a
        //     emitir el backend para cada property y nombres ya en uso por
        //     funciones de usuario. Esta validación se hace TRAS la fase de
        //     declaración para que se vean todas las funciones del módulo o
        //     de cada clase.
        validatePropertyAccessorCollisions(mod);
    }

    /**
     * Detecta el caso E1: el usuario declara `property X` y al mismo tiempo
     * una función/método con el nombre sintético que el backend va a usar
     * para el getter o setter. Sin esta validación, el ModWriter peta con
     * un "función duplicada" críptico al emitir.
     *
     * Reglas de naming (deben coincidir con MivmEmitter):
     *   - Property de CLASE  → getX / setX (slots de vtable de la clase)
     *   - Property de MÓDULO → __prop_get_X / __prop_set_X (funciones del módulo)
     */
    private void validatePropertyAccessorCollisions(ModuleNode mod) {
        for (ITopLevelDecl def : mod.defs) {
            if (def instanceof PropertyDef) {
                PropertyDef p = (PropertyDef) def;
                String getName = "__prop_get_" + p.name.name;
                String setName = "__prop_set_" + p.name.name;
                Symbol got = module.members.tryLookup(getName);
                Symbol sot = module.members.tryLookup(setName);
                if (got instanceof FunctionSymbol)
                    err(p.line, p.column,
                        "el accessor sintético '" + getName + "' colisiona con una función ya declarada");
                if (sot instanceof FunctionSymbol)
                    err(p.line, p.column,
                        "el accessor sintético '" + setName + "' colisiona con una función ya declarada");
            } else if (def instanceof ClassDef) {
                ClassDef cd = (ClassDef) def;
                ClassSymbol cls = (ClassSymbol) info.declSymbols.get(cd);
                if (cls == null) continue;
                for (ITopLevelDecl m : cd.members) {
                    if (!(m instanceof PropertyDef)) continue;
                    PropertyDef p = (PropertyDef) m;
                    String capName = Character.toUpperCase(p.name.name.charAt(0))
                                   + p.name.name.substring(1);
                    String getName = "get" + capName;
                    String setName = "set" + capName;
                    Scope members = p.name.isStatic() ? cls.staticMembers : cls.instanceMembers;
                    Symbol got = members.tryLookup(getName);
                    Symbol sot = members.tryLookup(setName);
                    if (got instanceof FunctionSymbol)
                        err(p.line, p.column,
                            "el accessor sintético '" + cls.name + "." + getName
                            + "' colisiona con un método ya declarado en la clase");
                    if (sot instanceof FunctionSymbol)
                        err(p.line, p.column,
                            "el accessor sintético '" + cls.name + "." + setName
                            + "' colisiona con un método ya declarado en la clase");
                }
            }
        }
    }

    private void declareTopLevel(ITopLevelDecl decl, ClassSymbol owner, Scope scope) {
        if      (decl instanceof ConstDecl)    declareConst((ConstDecl) decl, owner, scope);
        else if (decl instanceof VarDecl)      declareVar((VarDecl) decl, owner, scope);
        else if (decl instanceof FuncDef)      declareFunc((FuncDef) decl, owner, scope);
        else if (decl instanceof PropertyDef)  declareProperty((PropertyDef) decl, owner, scope);
        else if (decl instanceof Ast.EventDef) declareEvent((Ast.EventDef) decl, owner, scope);
        else if (decl instanceof ClassDef)     declareClass((ClassDef) decl);
        else if (decl instanceof EnumDef)      declareEnum((EnumDef) decl);
    }

    /**
     * H5.c — declara un evento. Sólo dentro de una clase: a nivel de módulo no
     * habría receptor que disparar ni nadie a quien pertenecer, y el diseño
     * (docs/V4_IDEAS.md §H5.c) dice que un evento lo dispara SU objeto.
     */
    private void declareEvent(Ast.EventDef ed, ClassSymbol owner, Scope scope) {
        if (owner == null) {
            err(ed.line, ed.column, "un evento sólo puede declararse dentro de una clase: '"
                    + ed.name.name + "'");
            return;
        }
        if (ed.name.isStatic()) {
            err(ed.line, ed.column, "un evento no puede ser static: '" + ed.name.name
                    + "'. Lo dispara UNA instancia, y sin instancia no hay quién");
            return;
        }
        Symbol.EventSymbol sym = new Symbol.EventSymbol(ed.name.name, owner, ed.line, ed.column);
        sym.decl = ed;
        for (Param p : ed.params) {
            ParamSymbol psym = new ParamSymbol(p.name, p.line, p.column);
            psym.defaultExpr = p.defaultValue;   // H8.1: se normaliza al resolver tipos
            sym.params.add(psym);
        }
        if (!scope.tryDefine(sym))
            err(ed.line, ed.column, "nombre duplicado: '" + ed.name.name + "' en " + scope.tag);
        if (!info.declSymbols.containsKey(ed)) info.declSymbols.put(ed, sym);
    }

    /** 14-sep — `__cls_…` y `__prop_…` son del compilador: nombres de factorías,
     *  alias y accesores que exporta el módulo. Una función de usuario llamada
     *  `__cls_s_Base_cuantos` producía DOS exports con el mismo nombre y distinto
     *  offset, y cada VM se quedaba con uno (7 en miVM, 999 en la VM-C: paridad
     *  rota). Se corta aquí, con línea. */
    private void checkNombreReservado(String nombre, int line, int col) { checkNombreReservado(nombre, line, col, false); }
    /** M4 (21-sep) — el prefijo `__` es DEL SISTEMA: lo llevan las primitivas de la VM (Core.bp y las
     *  intrínsecas de cada módulo), los nombres que sintetiza el compilador (`__init`, `__prop_get_X`,
     *  `__syncMutex`, `__cls_new_X`…) y el puente que la VM llama por nombre (`Gui.__guiDispatch`).
     *  Fuera de Core, un identificador de usuario con `__` sólo puede ser una intrínseca (cuerpo
     *  `inline`) o una función pública (el puente); todo lo demás —variables, campos, parámetros,
     *  clases, consts, enums, properties, funciones privadas— es error con línea. Antes el lexer lo
     *  aceptaba y compartía espacio con los sintéticos: el choque salía como error de enlace, lejos. */
    private void checkNombreReservado(String nombre, int line, int col, boolean puenteOIntrinseca) {
        if (nombre == null || !nombre.startsWith("__")) return;
        if (nombre.startsWith("__cls_") || nombre.startsWith("__prop_")) {
            err(line, col, "'" + nombre + "' es un nombre reservado del compilador (los que empiezan por `__cls_` o `__prop_`)");
            return;
        }
        if (esRaiz() || puenteOIntrinseca) return;
        err(line, col, "'" + nombre + "' empieza por `__`, que es del sistema (primitivas de la VM y nombres del compilador): elige otro nombre");
    }

    /** #527 — mismo criterio que el emisor (hasOwnSyncProperty / ancestorHasSyncProperty):
     *  quién introduce el `__syncMutex` y por tanto necesita el ctor sintético. */
    private static boolean tieneSyncPropia(ClassDef cd) {
        for (ITopLevelDecl m : cd.members)
            if (m instanceof PropertyDef && ((PropertyDef) m).isSync) return true;
        return false;
    }
    /** #527/#511 — ver el punto 2.a de resolveTypesPass. */
    private void sintetizarCtores(ClassSymbol cls, int prof) {
        if (cls.isExternal || cls.astNode == null || cls.constructor != null || prof > 64) return;
        if (cls.baseClass != null) sintetizarCtores(cls.baseClass, prof + 1);   // el padre primero
        boolean porSync = tieneSyncPropia(cls.astNode) && !ancestroConSync(cls);
        if (!porSync && !tieneInicializadores(cls.astNode) && !tieneCamposFijos(cls.astNode)) return;   // #680
        ClassSymbol base = cls.baseClass;
        FunctionSymbol baseCtor = (base != null) ? base.findConstructor() : null;
        FunctionSymbol cabeza = null, cola = null;
        int cuantos = 0;
        for (FunctionSymbol c = baseCtor; c != null || cabeza == null; c = (c != null) ? c.nextOverload : null) {
            FunctionSymbol synth = new FunctionSymbol(cls.name, true, false, false, cls, null);
            synth.isConstructor = true;
            if (c != null) {
                for (ParamSymbol p : c.params) {
                    ParamSymbol q = new ParamSymbol(p.name, p.line, p.column);
                    q.type = p.type; q.defaultExpr = p.defaultExpr;
                    synth.params.add(q);
                }
                synth.implicitSuper = true; synth.implicitSuperCtor = c;   // super(MIS params) a ESA sobrecarga
                ClassSymbol save = currentClass; currentClass = cls;
                try { checkVisibilidadCtor(cls.line, cls.column, c, base); } finally { currentClass = save; }   // #515
            }
            if (cabeza == null) cabeza = synth; else { cola.nextOverload = synth; }
            cola = synth; cuantos++;
            if (c == null) break;
        }
        if (cuantos > 1)
            for (FunctionSymbol k = cabeza; k != null; k = k.nextOverload) { k.overloaded = true; k.slotKey = (k == cabeza) ? k.name : k.overloadMangle(); }
        cls.constructor = cabeza;
    }

    /** #680 — ¿la clase declara un campo de INSTANCIA de tamaño fijo `tipo[N]`? Su cabecera (la longitud) la escribe el
     *  `__init` de la clase (`emitInitDeCamposFijos`), así que sin constructor de usuario hace falta uno sintético: sin
     *  él, una clase con sólo campos fijos daba `len` 0 y «índice fuera de rango» al primer acceso, en las dos VMs. */
    static boolean tieneCamposFijos(ClassDef cd) {
        for (ITopLevelDecl m : cd.members) {
            if (!(m instanceof VarDecl)) continue;
            VarDecl v = (VarDecl) m;
            if (v.type instanceof ArrayTypeRef && ((ArrayTypeRef) v.type).size != null
                    && !v.names.isEmpty() && !v.names.get(0).isStatic()) return true;
        }
        return false;
    }

    /** #511 — ¿algún campo o property de INSTANCIA de la clase lleva `:= valor`? */
    static boolean tieneInicializadores(ClassDef cd) {
        for (ITopLevelDecl m : cd.members) {
            if (m instanceof VarDecl && ((VarDecl) m).init != null
                    && !((VarDecl) m).names.isEmpty() && !((VarDecl) m).names.get(0).isStatic()) return true;
            if (m instanceof PropertyDef && ((PropertyDef) m).init != null && !((PropertyDef) m).name.isStatic()) return true;
        }
        return false;
    }
    private static boolean ancestroConSync(ClassSymbol cls) {
        for (ClassSymbol p = cls.baseClass; p != null; p = p.baseClass)
            for (Symbol s : p.instanceMembers.getSymbols())
                if (s instanceof PropertySymbol && ((PropertySymbol) s).isSync) return true;
        return false;
    }

    /** V8 H7.3 (`#756`) — lo que cabe en una interfaz: métodos sin cuerpo y properties en forma corta, de instancia;
     *  lo que una clase exporta (Eduardo: «no se puede exportar nada mas que métodos y propiedades así que para la
     *  interfaces casi no hay restricciones»). Lo demás, un error que dice qué es lo que sobra. */
    private boolean miembroDeInterfazValido(ITopLevelDecl m, ClassSymbol iface) {
        Node n = (Node) m;
        String quien = "la interfaz '" + iface.name + "'";
        String motivo = null;
        if (m instanceof FuncDef) {
            FuncDef f = (FuncDef) m;
            if (f.isDestructor || f.name.name.equals(iface.name))
                motivo = quien + " no tiene constructor ni destructor: no se construye";
            else if (f.name.isStatic())
                motivo = quien + " no tiene métodos estáticos: son de una clase";
            else if (f.isFinal || f.isPrivate || f.isNative || f.isEventHandler || f.isIntrinsic)
                motivo = "un método de una interfaz es sólo su firma: sin 'final', 'private', 'native' ni 'event'";
        } else if (m instanceof PropertyDef) {
            PropertyDef p = (PropertyDef) m;
            if (p.name.isStatic())
                motivo = quien + " no tiene properties estáticas: son de una clase";
            else if (!p.isShortForm || p.getter != null || p.setter != null || p.init != null)
                motivo = "una property de una interfaz es sólo su nombre y su tipo: sin get, set ni valor";
            else if (p.isFinal || p.isPrivate || p.isOwner || p.isSync)
                motivo = "una property de una interfaz es sólo su nombre y su tipo: sin 'final', 'private', 'owner' ni"
                        + " 'sync'";
        } else if (m instanceof VarDecl) {
            motivo = quien + " no tiene campos: sólo métodos y properties";
        } else if (m instanceof ConstDecl) {
            motivo = quien + " no tiene constantes: sólo métodos y properties";
        } else if (m instanceof Ast.EventDef) {
            motivo = quien + " no tiene eventos: sólo métodos y properties";
        } else {
            motivo = quien + ": sólo métodos y properties";
        }
        if (motivo == null) return true;
        err(n.line, n.column, motivo);
        return false;
    }

    private void declareClass(ClassDef cd) {
        if (currentClass != null) {
            err(cd.line, cd.column, "no se permiten clases anidadas");
            return;
        }
        checkNombreReservado(cd.name, cd.line, cd.column);
        // N9 (21-sep, con H1 P3) — las CUATRO clases del lenguaje son de Core y todo programa las ve:
        // una copia local (`class Thread` con sólo `run`) ganaba en silencio a la de verdad y dejaba
        // a `parallel`, a `sync` y al padre implícito de toda clase apuntando a la del usuario.
        if (!esRaiz() && ("Object".equals(cd.name) || "Thread".equals(cd.name) || "Mutex".equals(cd.name) || "StringBuilder".equals(cd.name))) {
            err(cd.line, cd.column, "'" + cd.name + "' es una clase del lenguaje (Core): no se puede redeclarar");
            return;
        }
        ClassSymbol cls = new ClassSymbol(cd.name, cd.isPublic, cd.baseClass, cd, cd.line, cd.column);
        if (!module.members.tryDefine(cls))
            err(cd.line, cd.column, "nombre duplicado: '" + cd.name + "'");
        info.declSymbols.put(cd, cls);

        ClassSymbol save = currentClass;
        currentClass = cls;
        try {
            for (ITopLevelDecl m : cd.members) {
                if (m instanceof ClassDef || m instanceof EnumDef) {
                    Node n = (Node) m;
                    err(n.line, n.column, "no se permiten clases ni enums dentro de una clase");
                    continue;
                }
                if (cls.isInterface && !miembroDeInterfazValido(m, cls)) continue;
                Scope target = isStaticDecl(m) ? cls.staticMembers : cls.instanceMembers;
                declareTopLevel(m, cls, target);
            }
        } finally { currentClass = save; }

        // Constructor: función con el nombre de la clase (en miembros de instancia)
        Symbol maybeCtor = cls.instanceMembers.tryLookup(cls.name);
        if (maybeCtor instanceof FunctionSymbol) {
            FunctionSymbol fCtor = (FunctionSymbol) maybeCtor;
            cls.constructor = fCtor;   // cabeza de la cadena (findConstructor)
            // H5.a-E4 — con constructores SOBRECARGADOS hay varios en la cadena:
            // TODOS son constructores. Marcar solo la cabeza dejaba a las demás
            // sin emitir (y peor: como no llevaban el flag, el layout las habría
            // metido en la VTABLE, que es justo donde un ctor no debe estar).
            for (FunctionSymbol c = fCtor; c != null; c = c.nextOverload) {
                c.isConstructor = true;
                if (c.astNode != null && c.astNode.returnType != null)
                    err(c.line, c.column, "el constructor no puede declarar tipo de retorno");
            }
        }
    }

    private void declareEnum(EnumDef ed) {
        checkNombreReservado(ed.name, ed.line, ed.column);
        EnumSymbol en = new EnumSymbol(ed.name, ed.isPublic, ed.line, ed.column);
        en.decl = ed;
        if (!module.members.tryDefine(en))
            err(ed.line, ed.column, "nombre duplicado: '" + ed.name + "'");
        info.declSymbols.put(ed, en);

        long next = 0;
        Set<String> seen = new HashSet<>();
        for (EnumValue v : ed.values) {
            if (!seen.add(v.name))
                err(v.line, v.column, "valor duplicado en enum '" + ed.name + "': '" + v.name + "'");
            long val = (v.explicitValue != null) ? v.explicitValue : next;
            en.values.put(v.name, val);
            next = val + 1;
        }
    }

    private void declareConst(ConstDecl c, ClassSymbol owner, Scope scope) {
        boolean isStatic = c.name.isStatic();
        validateStaticPrefix(c.name, owner);
        checkNombreReservado(c.name.name, c.line, c.column);
        // L11.b — una const «a secas» (sin cualificar) dentro de una clase no tiene
        // sentido: una const es un valor de compilación común a todas las instancias,
        // así que es de CLASE, no de objeto. Antes se aceptaba muda y nacía como
        // miembro de instancia en silencio. Ahora se exige la forma estática
        // `const Cls.NAME := …` (o mover la const a nivel de módulo).
        if (owner != null && !isStatic) {
            err(c.line, c.column, "una const dentro de una clase debe ser estática: escribe"
                    + " `const " + owner.name + "." + c.name.name + " := …` (constante de clase;"
                    + " añade `public` si se lee fuera) o muévela a nivel de módulo");
        }
        ConstSymbol sym = new ConstSymbol(c.name.name, c.isPublic, isStatic, owner, c.line, c.column);
        sym.decl = c;
        if (!scope.tryDefine(sym))
            err(c.line, c.column, "nombre duplicado: '" + c.name.name + "' en " + scope.tag);
        info.declSymbols.put(c, sym);
    }

    private void declareVar(VarDecl v, ClassSymbol owner, Scope scope) {
        if (v.isOwner && owner == null && !"module".equals(scope.tag)) {
            // 'var owner' a nivel módulo: lo permitimos (dura toda la ejecución).
            // Aquí (owner==null && scope no es "module") sería caso raro;
            // no bloqueamos.
        }
        for (DeclName n : v.names) {
            boolean isStatic = n.isStatic();
            validateStaticPrefix(n, owner);
            checkNombreReservado(n.name, v.line, v.column);
            if (v.isOwner && isStatic) {
                err(n.line, n.column, "'var owner' no compatible con miembro estático: '" + n.name + "'");
            }
            // L11.a — NINGUNA variable puede ser public, ni de clase ni de módulo: un
            // field no cruza la frontera del .bpi, que sólo exporta el get/set de una
            // property. Antes sólo se vetaba en clase (owner != null); como clase y
            // módulo comparten parseDefStmt(), el flag se heredaba mudo a nivel módulo.
            // La norma del modelo de objetos: público ⇒ property.
            if (v.isPublic) {
                String ambito = (owner != null) ? "de clase" : "de módulo";
                err(n.line, n.column, "una variable " + ambito + " no puede ser public: '" + n.name
                        + "'. El estado sale del módulo sólo por getters y setters:"
                        + " declárala `public property " + n.name + "` y su"
                        + " backing field queda privado");
            }
            VarSymbol sym = new VarSymbol(n.name, v.isPublic, isStatic, owner, false, n.line, n.column);
            sym.decl = v;
            sym.isOwner = v.isOwner;
            sym.isPrivate = v.isPrivate;   // #508
            if (!scope.tryDefine(sym))
                err(n.line, n.column, "nombre duplicado: '" + n.name + "' en " + scope.tag);
            if (!info.declSymbols.containsKey(v))
                info.declSymbols.put(v, sym);
        }
    }

    private void declareFunc(FuncDef f, ClassSymbol owner, Scope scope) {
        boolean isStatic = f.name.isStatic();
        validateStaticPrefix(f.name, owner);
        String nombre = f.name.name;
        if (f.isDestructor) {
            // V7/H1 (21-sep) — `function ~Clase()`: el contrario del constructor. Se llama como su
            // clase, no lleva parámetros ni retorno, y por dentro es el método `__dtor` (así el de
            // la hija SOBRESCRIBE al del padre por nombre, como cualquier método).
            if (owner == null) { err(f.line, f.column, "un destructor ('~') sólo puede ir dentro de una clase"); return; }
            if (!owner.name.equals(f.dtorEscrito)) err(f.line, f.column, "el destructor se llama como su clase: '~" + owner.name + "', no '~" + f.dtorEscrito + "'");
            if (!f.params.isEmpty()) err(f.line, f.column, "el destructor no lleva parámetros");
            if (f.returnType != null) err(f.line, f.column, "el destructor no devuelve nada");
            if (isStatic) err(f.line, f.column, "el destructor es de instancia");
            if (owner.instanceMembers.tryLookup(Symbol.FunctionSymbol.DTOR) != null) err(f.line, f.column, "la clase '" + owner.name + "' ya tiene destructor");
        } else {
            checkNombreReservado(nombre, f.line, f.column, f.isIntrinsic || f.isPublic);
        }
        if (f.isFinal && owner == null)
            err(f.line, f.column, "'final' solo es válido en métodos de clase");
        if (f.isIntrinsic && owner != null)
            err(f.line, f.column, "una función con cuerpo 'inline' sólo puede ir a nivel de módulo, no como método");
        // H1 P2 (20-sep) — una intrínseca PRIVADA vale: en Core.bp (la raíz) las 172 que no son
        // globales son sólo la tabla de ids de la VM (nadie las llama; no se exportan). Antes
        // «'intrinsic' implica visibilidad pública» — con `intrinsic` sin cuerpo tenía sentido.
        // #390 — `private` sólo tiene sentido DENTRO de una clase: a nivel de
        // módulo la visibilidad ya la da `public`/nada, y ahí no hay descendientes
        // a quien proteger. Decirlo evita que alguien lo escriba esperando algo.
        if (f.isPrivate && owner == null)
            err(f.line, f.column, "'private' solo es válido en miembros de clase;"
                    + " a nivel de módulo, lo que no es 'public' ya es privado del módulo");
        // V8/H7.1 (#747) — `iif(c, a, b)` es el ternario: el nombre es del lenguaje, como el de una conversión.
        if (owner == null && Ast.TERNARIO.equals(nombre))
            err(f.line, f.column, "'" + Ast.TERNARIO + "' es el ternario de BP, iif(condición, a, b): una función no puede"
                    + " llamarse así");
        FunctionSymbol sym = new FunctionSymbol(nombre, f.isDestructor || f.isPublic, f.isFinal, isStatic, owner, f);
        sym.isEventHandler = f.isEventHandler;   // H5.c — `function event`
        sym.isDestructor = f.isDestructor;       // V7/H1
        sym.isPrivate = f.isPrivate && owner != null;   // #390
        sym.isIntrinsic = f.isIntrinsic;
        if (f.isIntrinsic && f.body != null && !f.body.isEmpty()) {   // H1 (20-sep): cuerpo `inline`; AQUÍ (al declarar) y
            List<InlineStmt> ops = new ArrayList<>();                  // no al analizar el cuerpo: la pasada de interfaz lo necesita
            for (IStmt st : f.body) if (st instanceof InlineStmt) ops.add((InlineStmt) st);
            sym.inlineOps = ops;
        }
        for (Param p : f.params) {
            checkNombreReservado(p.name, p.line, p.column);   // M4
            ParamSymbol psym = new ParamSymbol(p.name, p.line, p.column);
            psym.defaultExpr = p.defaultValue;   // H8.1: crudo aquí; se valida/normaliza al resolver tipos
            sym.params.add(psym);
        }
        if (!scope.tryDefine(sym)) {
            // H5.a — sobrecarga: si el nombre ya lo tiene OTRA función, no es
            // error: se ENCADENA y la resolución por firma en la llamada elige.
            // La validación de firma-duplicada va después, con los tipos ya
            // resueltos (validateOverloads). Vale para funciones libres, métodos
            // estáticos y —desde E3— métodos de INSTANCIA (cada sobrecarga se
            // lleva su propio slot de vtable, ver assignSlotKeys). Colisión con
            // var/const/property sigue siendo "nombre duplicado".
            Symbol prev = scope.tryLookup(nombre);
            if (prev instanceof FunctionSymbol) {
                FunctionSymbol head = (FunctionSymbol) prev;
                FunctionSymbol tail = head;
                while (tail.nextOverload != null) tail = tail.nextOverload;
                tail.nextOverload = sym;
                head.overloaded = true;
                sym.overloaded  = true;
            } else {
                err(f.line, f.column, "nombre duplicado: '" + nombre + "' en " + scope.tag);
            }
        }
        info.declSymbols.put(f, sym);
    }

    private void declareProperty(PropertyDef p, ClassSymbol owner, Scope scope) {
        boolean isStatic = p.name.isStatic();
        validateStaticPrefix(p.name, owner);
        checkNombreReservado(p.name.name, p.line, p.column);
        if (p.isFinal && owner == null)
            err(p.line, p.column, "'final' solo es válido en propiedades de clase");
        if (p.isOwner && isStatic)
            err(p.line, p.column, "'property owner' no compatible con miembro estático: '" + p.name.name + "'");
        if (p.isOwner && owner == null)
            err(p.line, p.column, "'property owner' solo válida en propiedades de instancia de clase");
        PropertySymbol sym = new PropertySymbol(p.name.name, p.isPublic, p.isFinal, isStatic, owner, p);
        sym.isPrivate = p.isPrivate;   // #508
        if (!scope.tryDefine(sym))
            err(p.line, p.column, "nombre duplicado: '" + p.name.name + "' en " + scope.tag);
        info.declSymbols.put(p, sym);
    }

    private void validateStaticPrefix(DeclName n, ClassSymbol owner) {
        if (n.classQualifier == null) return;
        if (owner == null) {
            err(n.line, n.column, "prefijo de clase '" + n.classQualifier + ".' no válido a nivel módulo");
        } else if (!n.classQualifier.equals(owner.name)) {
            err(n.line, n.column, "prefijo '" + n.classQualifier + ".' no coincide con la clase '" + owner.name + "'");
        }
    }

    /** True si la declaración tiene prefijo cualificado (=> miembro estático). */
    private static boolean isStaticDecl(ITopLevelDecl d) {
        if (d instanceof ConstDecl)    return ((ConstDecl) d).name.isStatic();
        if (d instanceof VarDecl) {
            VarDecl v = (VarDecl) d;
            return !v.names.isEmpty() && v.names.get(0).isStatic();
        }
        if (d instanceof FuncDef)      return ((FuncDef) d).name.isStatic();
        if (d instanceof PropertyDef)  return ((PropertyDef) d).name.isStatic();
        return false;
    }

    // ============================================================
    // PASE 2 — Resolución de tipos
    // ============================================================
    /** L2 v2 — resuelve un nombre de clase base, soportando tanto el
     *  caso local (`Foo`) como el cross-module (`Mod.Foo`). */
    private ClassSymbol resolveBaseClassName(String name) {
        int dot = name.indexOf('.');
        if (dot < 0) {
            Symbol s = module.members.resolve(name);
            return (s instanceof ClassSymbol) ? (ClassSymbol) s : null;
        }
        String alias = name.substring(0, dot);
        String simple = name.substring(dot + 1);
        Symbol nsSym = module.members.resolve(alias);
        if (nsSym instanceof Symbol.ImportedNamespaceSymbol) {
            Symbol.ImportedNamespaceSymbol ns = (Symbol.ImportedNamespaceSymbol) nsSym;
            return ns.classes.get(simple);
        }
        return null;
    }

    /** V8 H7.3 (`#756`) — resuelve las interfaces de la cabecera de cada clase del módulo (y las de las que hereda
     *  cada interfaz): que existan, que sean interfaces y no clases, que no se repitan; y que no hereden en ciclo. Una
     *  clase que desciende de `Thread` no implementa interfaces: un `Thread` no es un `Object` (es `root class`), y el
     *  valor de una interfaz lo es siempre (decidido por Eduardo, la 4 de V8_IDEAS §E10). */
    private void resolverInterfaces() {
        for (Symbol s : module.members.getSymbols()) {
            if (!(s instanceof ClassSymbol)) continue;
            ClassSymbol cls = (ClassSymbol) s;
            if (cls.isExternal) continue;
            cls.interfaces.clear();
            boolean jc = cls.astNode != null && cls.astNode.primeroPuedeSerInterfaz;
            for (String n : cls.interfaceNames) {
                ClassSymbol i = resolveBaseClassName(n);
                if (i == null) {
                    int dot = n.indexOf('.');
                    String alias = (dot > 0) ? n.substring(0, dot) : null;
                    if (alias != null && !(module.members.resolve(alias) instanceof Symbol.ImportedNamespaceSymbol))
                        err(cls.line, cls.column, "'" + cls.name + "' nombra la interfaz '" + n + "' y este modulo no"
                                + " importa '" + alias + "': anade `import " + alias + "`");
                    else
                        err(cls.line, cls.column, "la interfaz '" + n + "' no existe");
                    continue;
                }
                if (!i.isInterface) {
                    if (cls.isInterface)
                        err(cls.line, cls.column, "'" + n + "' es una clase: una interfaz sólo hereda de interfaces");
                    else if (jc)
                        err(cls.line, cls.column, "'" + n + "' es una clase: el padre va el primero tras ':', y sólo uno");
                    else
                        err(cls.line, cls.column, "'" + n + "' es una clase, no una interfaz: va en 'extends'");
                    continue;
                }
                boolean repetida = false;
                for (ClassSymbol j : cls.interfaces) if (ClassSymbol.mismaClase(i, j)) repetida = true;
                if (repetida) {
                    err(cls.line, cls.column, "'" + cls.name + "' nombra dos veces la interfaz '" + n + "'");
                    continue;
                }
                cls.interfaces.add(i);
            }
            if (!cls.interfaces.isEmpty() && !cls.isInterface) {
                for (ClassSymbol c = cls; c != null; c = c.baseClass) {
                    if (c.esDelLenguaje() && "Thread".equals(c.name)) {
                        err(cls.line, cls.column, "'" + cls.name + "' desciende de Thread y un Thread no es un Object:"
                                + " no puede implementar interfaces (el valor de una interfaz siempre es un Object)");
                        cls.interfaces.clear();
                        break;
                    }
                }
            }
        }
        // ciclos: una interfaz que, subiendo por sus padres, vuelve a sí misma
        for (Symbol s : module.members.getSymbols()) {
            if (!(s instanceof ClassSymbol) || !((ClassSymbol) s).isInterface || ((ClassSymbol) s).isExternal) continue;
            ClassSymbol cls = (ClassSymbol) s;
            if (heredaEnCiclo(cls, cls, new HashSet<>())) {
                err(cls.line, cls.column, "ciclo de herencia entre interfaces en '" + cls.name + "'");
                cls.interfaces.clear();
            }
        }
    }

    private static boolean heredaEnCiclo(ClassSymbol origen, ClassSymbol c, Set<ClassSymbol> vistos) {
        if (!vistos.add(c)) return false;
        for (ClassSymbol p : c.interfaces) {
            if (p == origen || heredaEnCiclo(origen, p, vistos)) return true;
        }
        return false;
    }

    private void resolveTypesPass(ModuleNode mod) {
        // 1) extends de cada clase
        for (Symbol s : module.members.getSymbols()) {
            if (s instanceof ClassSymbol) {
                ClassSymbol cls = (ClassSymbol) s;
                // V8 H7.3 (#756) — en JC, `class C : A, B` no dice si `A` es el padre o una interfaz: si es una
                // interfaz, va con las demás, y el padre es Object
                if (cls.astNode != null && cls.astNode.primeroPuedeSerInterfaz && cls.baseClassName != null) {
                    ClassSymbol primero = resolveBaseClassName(cls.baseClassName);
                    if (primero != null && primero.isInterface) {
                        cls.interfaceNames.add(0, cls.baseClassName);
                        cls.baseClassName = null;
                    }
                }
                if (cls.isRoot && !esRaiz()) {
                    err(cls.line, cls.column, "'root class' sólo puede escribirse en el módulo raíz (Core): toda clase desciende de Object");
                    cls.isRoot = false;
                }
                if (cls.isRoot && cls.baseClassName != null) {
                    err(cls.line, cls.column, "una 'root class' no puede llevar 'extends'");
                    cls.isRoot = false;
                }
                if (cls.baseClassName == null && !cls.isRoot && !cls.isExternal) {
                    // H1 P3 (20-sep) — sin `extends`: la base es Object, la clase del lenguaje que
                    // vive en Core (el stub del import implícito; la local, si se compila Core).
                    Symbol obj = module.members.resolve("Object");
                    if (obj instanceof ClassSymbol && esObject((ClassSymbol) obj)) cls.baseClass = (ClassSymbol) obj;
                    else if (obj instanceof ClassSymbol && ((ClassSymbol) obj).isExternal) cls.baseClass = (ClassSymbol) obj;   // Core de otra biblioteca
                    else err(cls.line, cls.column, "no encuentro la clase Object del lenguaje (¿falta Core.mod?): '" + cls.name + "' no tiene base");
                }
                if (cls.baseClassName != null) {
                    ClassSymbol base = resolveBaseClassName(cls.baseClassName);
                    if (base != null && base.isInterface) {
                        // V8 H7.3 — una interfaz no es un padre: se implementa
                        err(cls.line, cls.column, "'" + cls.baseClassName + "' es una interfaz: no se extiende, se"
                                + " implementa (`class " + cls.name + " implements " + cls.baseClassName + "`)");
                        Symbol obj = module.members.resolve("Object");   // y sigue como si no tuviera padre
                        if (obj instanceof ClassSymbol) cls.baseClass = (ClassSymbol) obj;
                    } else if (base != null) {
                        cls.baseClass = base;
                    } else {
                        // V6/#498 — el caso que de verdad se da: la clase viene de un modulo
                        // IMPORTADO (Collections) y su base es de OTRO que este modulo NO
                        // importa (Core.List) — los imports no son transitivos (#492). El
                        // mensaje generico salia en [0:0] («clase base 'Core.List' no
                        // existe») y mandaba a buscar una clase que existe de sobra. Ahora
                        // dice lo que falta y donde ponerlo.
                        int dot = cls.baseClassName.indexOf('.');
                        String alias = (dot > 0) ? cls.baseClassName.substring(0, dot) : null;
                        if (alias != null && !(module.members.resolve(alias) instanceof Symbol.ImportedNamespaceSymbol)) {
                            err(cls.line, cls.column, "la clase '" + cls.name + "' extiende '" + cls.baseClassName
                                + "' y este modulo no importa '" + alias + "': anade `import " + alias
                                + "` (los imports no son transitivos)");
                        } else {
                            err(cls.line, cls.column, "clase base '" + cls.baseClassName + "' no existe");
                        }
                    }
                }
            }
        }
        // ciclos
        for (Symbol s : module.members.getSymbols()) {
            if (s instanceof ClassSymbol) {
                ClassSymbol cls = (ClassSymbol) s;
                Set<ClassSymbol> visited = new HashSet<>();
                ClassSymbol c = cls;
                while (c != null) {
                    if (!visited.add(c)) {
                        err(cls.line, cls.column, "ciclo de herencia detectado en '" + cls.name + "'");
                        cls.baseClass = null;
                        break;
                    }
                    c = c.baseClass;
                }
            }
        }

        // 1.a) V8 H7.3 (#756) — las interfaces que implementa cada clase, y de las que hereda cada interfaz
        resolverInterfaces();

        // 1.b) H5.c — un miembro NO puede llamarse igual que un evento heredado
        // (ni al revés). Ver validateEventShadowing: con la cadena de bases ya
        // resuelta y antes de tocar tipos, que es cuando el nombre es lo único
        // que hace falta.
        validateEventShadowing();

        // 2) tipos de vars/consts/props/parámetros + retorno de funciones
        for (ITopLevelDecl def : mod.defs) resolveDefTypes(def, null);
        for (Symbol s : module.members.getSymbols()) {
            if (s instanceof ClassSymbol) {
                ClassSymbol cls = (ClassSymbol) s;
                // #248: los stubs aliasados de Core (Exception/RuntimeError) son
                // externos sin AST — nada que resolver aquí.
                if (cls.isExternal || cls.astNode == null) continue;
                for (ITopLevelDecl m : cls.astNode.members)
                    resolveDefTypes(m, cls);
            }
        }

        // 2.a) #527/#511 (19-sep) — los constructores SINTÉTICOS de una clase sin constructor que
        // los necesita (una `sync property` propia: el mutex; campos o properties con `:= valor`:
        // los inicializadores). Se registran AQUÍ, con los tipos ya resueltos, para que (1) la
        // clase siga ofreciendo las MISMAS firmas que heredaba —en BasicPlus una clase sin
        // constructor hereda los del padre (`Tarea(4096)` con `Tarea extends Thread`)—: uno
        // por sobrecarga del padre, que hace `super(esos args)` y luego lo suyo; (2) `S(5)`
        // no resuelva al ctor del padre a secas saltándose el mutex (era `#517`); y (3) la
        // interfaz los exporte como propios. Los padres primero: el sintético del hijo copia
        // la cadena EFECTIVA del padre, que puede ser sintética también.
        for (Symbol s : module.members.getSymbols())
            if (s instanceof ClassSymbol) sintetizarCtores((ClassSymbol) s, 0);

        // H5.a — con los tipos ya resueltos, valida que no haya dos sobrecargas
        // con la MISMA firma (mismo nombre + mismos tipos de parámetros).
        validateOverloads();
        // H5.a-E3 — y asigna la CLAVE de cada función (nombre pelado o mangleada).
        // Debe ir DESPUÉS de resolver tipos (hacen falta las firmas) y DESPUÉS de
        // resolver baseClass (el grupo de un método abarca la herencia).
        assignSlotKeys();

        // H1 P3 (20-sep) — toString()/compareTo() ya no se inyectan: los hereda de Core.Object.

        // #529 (19-sep) — el TOPE de una clase CERRADA (toda su cadena en este módulo): la
        // ranura de método de INVOKE_VIRTUAL y la de campo de GET/SET_FIELD son de 8 bits,
        // 256 entradas. Es un límite del lenguaje y se dice AQUÍ, con la línea de la clase;
        // antes reventaba el writer sin línea («Method slot u8 desbordado: 256»). Una clase
        // ABIERTA (extiende a una de otro módulo) va por referencia en MOD9 (idx u16) y no
        // lo tiene.
        java.util.Map<String, ModuleInterface.LayoutAndNames> layouts = new java.util.HashMap<>();
        for (Symbol s : module.members.getSymbols()) {
            if (!(s instanceof ClassSymbol)) continue;
            ClassSymbol cls = (ClassSymbol) s;
            if (cls.isExternal || cls.astNode == null) continue;   // H1 P3: el tope, para toda clase (en MOD9 todas son abiertas)
            ModuleInterface.LayoutAndNames l;
            try { l = ModuleInterface.computeClassLayout(cls, layouts); } catch (RuntimeException e) { continue; }   // lo dirá el emisor
            if (l.numMethods > 256)
                err(cls.line, cls.column, "la clase '" + cls.name + "' tiene " + l.numMethods + " métodos (con los heredados y los accesores) y una clase cerrada admite 256 como máximo (la ranura de la vtable es de 8 bits)");
            if (l.numFields > 256)
                err(cls.line, cls.column, "la clase '" + cls.name + "' ocupa " + l.numFields + " ranuras de campos (con las heredadas; una referencia, un long o un double ocupan 2) y una clase cerrada admite 256 como máximo (la ranura de campo es de 8 bits)");
        }
    }

    /**
     * H5.c — el nombre de un evento heredado NO se puede reutilizar como miembro
     * en una subclase (ni al revés).
     *
     * Sin esto el compilador CALLA y el resultado es el peor de los mundos: la
     * base declara `event onClick`, la subclase declara `function onClick()`
     * creyendo que lo sobrescribe, compila sin un diagnóstico... y ese método no
     * se llama JAMÁS, porque `raise` no despacha por vtable — va al suscriptor
     * que guarda el objeto en `ev$recv`/`ev$dest`. Medido con un spike al migrar
     * el GUI (#324), donde 14 demos hacían exactamente eso.
     *
     * Se comprueba por NOMBRE y sólo contra ANCESTROS: el choque dentro de una
     * misma clase ya lo caza scope.tryDefine.
     */
    private void validateEventShadowing() {
        for (Symbol s : module.members.getSymbols()) {
            if (!(s instanceof ClassSymbol)) continue;
            ClassSymbol cls = (ClassSymbol) s;
            if (cls.isExternal) continue;
            checkShadowedEvents(cls, cls.instanceMembers);
            checkShadowedEvents(cls, cls.staticMembers);
        }
    }

    private void checkShadowedEvents(ClassSymbol cls, Symbol.Scope own) {
        for (Symbol mine : own.getSymbols()) {
            for (ClassSymbol a = cls.baseClass; a != null; a = a.baseClass) {
                Symbol theirs = a.instanceMembers.tryLookup(mine.name);
                if (theirs == null) theirs = a.staticMembers.tryLookup(mine.name);
                if (theirs == null) continue;
                boolean mineEv   = mine   instanceof Symbol.EventSymbol;
                boolean theirsEv = theirs instanceof Symbol.EventSymbol;
                if (mineEv == theirsEv) break;   // miembro/miembro o evento/evento: no es cosa mía
                if (theirsEv) {
                    err(mine.line, mine.column, "'" + mine.name + "' es un EVENTO heredado de '"
                            + a.name + "': no se puede redeclarar como miembro de '" + cls.name
                            + "'. Un evento no se sobrescribe, se SUSCRIBE: "
                            + "obj." + mine.name + " := destino::handler");
                } else {
                    err(mine.line, mine.column, "el evento '" + mine.name + "' choca con un miembro"
                            + " del mismo nombre heredado de '" + a.name + "'; renombra uno de los dos");
                }
                break;
            }
        }
    }

    private void resolveDefTypes(ITopLevelDecl def, ClassSymbol owner) {
        if (def instanceof ConstDecl) {
            ConstDecl c = (ConstDecl) def;
            Symbol sym = info.declSymbols.get(c);
            if (sym instanceof ConstSymbol && c.type != null) {
                ((ConstSymbol) sym).type = resolveType(c.type);
            } else if (sym instanceof ConstSymbol && c.value != null) {
                // V7/H2 (15-sep) — `const INFO := 1` SIN tipo: se infería en bodyPass,
                // que la pasada de INTERFAZ no corre, así que la const se caía de la
                // interfaz («tipo no exportable: <null>») y la interfaz de la pasada
                // difería de la emitida — con el sello (ifaceCrc) eso es un rechazo en
                // el cargador. Un inicializador LITERAL (o un miembro de enum) no
                // necesita scope ni puede fallar: se infiere aquí, y bodyPass sólo
                // confirma. Era el bug aparcado «la pasada interfaz-only tira firmas».
                Long folded = foldEnumMemberToLiteral(c.value);
                if (folded != null || constLiteralValue(c.value) != null) {
                    ConstSymbol cs = (ConstSymbol) sym;
                    cs.type = analyzeExpr(c.value, null, null);
                    cs.literalValue = (folded != null) ? folded : constLiteralValue(c.value);
                }
            }
        } else if (def instanceof VarDecl) {
            VarDecl v = (VarDecl) def;
            BpType t = resolveType(v.type);
            // V7/H1 #19 (21-sep, decisión de Eduardo): un array fijo `tipo[N]` NO es una referencia —
            // es almacenamiento en sitio— y sólo puede ser LOCAL o `private var` de clase. A nivel de
            // módulo, fuera (hasta hoy se horneaba en el data block).
            Integer fixedN = null;
            if (v.type instanceof ArrayTypeRef && ((ArrayTypeRef) v.type).size != null) {
                ArrayTypeRef at = (ArrayTypeRef) v.type;
                Object n = constLiteralValue(at.size);
                if (owner == null) {
                    err(at.line, at.column, "un array fijo `tipo[N]` sólo puede ser una variable local o un campo `private var` de clase;"
                        + " a nivel de módulo escribe `tipo[]` (con `tipo[n]` o un literal `[…]`)");
                } else if (!v.isPrivate) {
                    err(at.line, at.column, "un array fijo `tipo[N]` de clase tiene que ser `private var` (no es una referencia: sólo se indexa desde la propia clase)");
                } else if (!(n instanceof Long) || (Long) n <= 0) {
                    err(at.line, at.column, "el tamaño de un array fijo debe ser un literal entero > 0");
                } else if (!isBakeableArrayElement(t)) {
                    err(at.line, at.column, "array de tamaño fijo solo admite elementos primitivos "
                        + "(integer/float/long/double/boolean/byte/word/int8/int16); para string o clases usa 'tipo[]'");
                } else if (1 + ((Long) n * anchoDeElemento(t) + 3) / 4 > 255) {
                    err(at.line, at.column, "un array fijo de clase admite hasta 255 ranuras (1 + N·ancho/4): " + n + " elementos de "
                        + anchoDeElemento(t) + " bytes no caben; usa 'tipo[]'");
                } else {
                    fixedN = (int) (long) (Long) n;
                }
            }
            for (DeclName n : v.names) {
                Symbol sym;
                if (owner == null) sym = module.members.resolve(n.name);
                else sym = n.isStatic() ? owner.staticMembers.resolve(n.name)
                                         : owner.instanceMembers.resolve(n.name);
                if (sym instanceof VarSymbol) {
                    ((VarSymbol) sym).type = t;
                    if (fixedN != null && !n.isStatic())
                        ((VarSymbol) sym).fixedArraySize = fixedN;
                    else if (fixedN != null)
                        err(n.line, n.column, "un array fijo no puede ser estático");
                }
            }
        } else if (def instanceof FuncDef) {
            FuncDef f = (FuncDef) def;
            Symbol sym = info.declSymbols.get(f);
            if (sym instanceof FunctionSymbol) {
                FunctionSymbol fs = (FunctionSymbol) sym;
                for (int i = 0; i < f.params.size(); i++)
                    fs.params.get(i).type = resolveType(f.params.get(i).type);
                if (f.returnType != null) fs.returnType = resolveType(f.returnType);
                resolveParamDefaults(fs, f);   // H8.1
            }
        } else if (def instanceof Ast.EventDef) {
            // H5.c — la FIRMA del evento se resuelve aquí, en la misma pasada que
            // la de las funciones: si no, el `::` compararía contra tipos a null y
            // ninguna sobrecarga casaría nunca.
            Ast.EventDef ed = (Ast.EventDef) def;
            Symbol sym = info.declSymbols.get(ed);
            if (sym instanceof Symbol.EventSymbol) {
                Symbol.EventSymbol es = (Symbol.EventSymbol) sym;
                for (int i = 0; i < ed.params.size(); i++)
                    es.params.get(i).type = resolveType(ed.params.get(i).type);
                resolveParamDefaults(es.params, ed.params, ed);
            }
        } else if (def instanceof PropertyDef) {
            PropertyDef p = (PropertyDef) def;
            Symbol sym = info.declSymbols.get(p);
            if (sym instanceof PropertySymbol)
                ((PropertySymbol) sym).type = resolveType(p.type);
        }
    }

    /** H5.a — tras resolver los tipos, valida que no haya dos sobrecargas con la
     *  MISMA firma (mismo nombre + mismos tipos de parámetros) en un scope: sería
     *  ambiguo siempre y colisionaría en el emisor. Recorre las cadenas de
     *  funciones libres (módulo) y estáticas (por clase). */
    private void validateOverloads() {
        for (Symbol s : module.members.getSymbols()) checkOverloadChain(s);
        for (Symbol s : module.members.getSymbols()) {
            if (s instanceof ClassSymbol) {
                ClassSymbol cls = (ClassSymbol) s;
                for (Symbol m : cls.staticMembers.getSymbols()) checkOverloadChain(m);
            }
        }
    }

    /** H5.a-E3 — asigna {@link FunctionSymbol#slotKey} a TODA función del módulo.
     *  Regla ÚNICA: dentro de un grupo (mismo nombre), la PRIMERA firma se queda
     *  con el nombre pelado y las siguientes se manglean. Para métodos, el grupo
     *  abarca la CADENA DE HERENCIA: un método con la misma firma que uno heredado
     *  es un OVERRIDE y hereda su clave (⇒ reusa su slot); con firma nueva es una
     *  SOBRECARGA y toma una clave nueva (⇒ slot nuevo).
     *  Ser aditivo es lo que permite que una base ya compilada en OTRO módulo no
     *  tenga que re-manglearse cuando una subclase añade una sobrecarga. */
    private void assignSlotKeys() {
        for (Symbol s : module.members.getSymbols()) {
            if (s instanceof FunctionSymbol) assignKeysInChain((FunctionSymbol) s, null);
        }
        for (Symbol s : module.members.getSymbols()) {
            if (s instanceof ClassSymbol) assignKeysInClass((ClassSymbol) s);
        }
    }

    /** Recorre las clases en orden base-antes-que-subclase (la clave de un
     *  override se copia de la del padre, así que el padre debe estar listo). */
    private void assignKeysInClass(ClassSymbol cls) {
        if (cls == null || cls.keysAssigned) return;
        cls.keysAssigned = true;
        if (cls.baseClass != null) assignKeysInClass(cls.baseClass);
        for (Symbol s : cls.staticMembers.getSymbols())
            if (s instanceof FunctionSymbol) assignKeysInChain((FunctionSymbol) s, null);
        for (Symbol s : cls.instanceMembers.getSymbols())
            if (s instanceof FunctionSymbol) assignKeysInChain((FunctionSymbol) s, cls);
    }

    /** Asigna las claves de una cadena de sobrecargas (mismo nombre, mismo scope).
     *  {@code owner} != null ⇒ métodos de instancia: se mira la herencia. */
    private void assignKeysInChain(FunctionSymbol head, ClassSymbol owner) {
        // Claves ya usadas por ese nombre en los ANCESTROS (para no pisarlas), y
        // firma→clave heredada (para detectar overrides).
        java.util.Set<String> taken = new java.util.HashSet<>();
        java.util.Map<String, String> inheritedBySig = new java.util.LinkedHashMap<>();
        if (owner != null) {
            for (ClassSymbol a = owner.baseClass; a != null; a = a.baseClass) {
                Symbol as = a.instanceMembers.tryLookup(head.name);
                if (!(as instanceof FunctionSymbol)) continue;
                for (FunctionSymbol o = (FunctionSymbol) as; o != null; o = o.nextOverload) {
                    taken.add(o.slotKey());
                    inheritedBySig.putIfAbsent(o.signatureKey(), o.slotKey());
                }
            }
        }
        for (FunctionSymbol f = head; f != null; f = f.nextOverload) {
            String inherited = inheritedBySig.get(f.signatureKey());
            if (inherited != null) {
                f.slotKey = inherited;          // OVERRIDE: misma clave ⇒ mismo slot
                continue;
            }
            if (!taken.contains(f.name)) {
                f.slotKey = f.name;             // 1ª firma del grupo: nombre pelado
            } else {
                f.slotKey = f.overloadMangle(); // sobrecarga: clave nueva ⇒ slot nuevo
            }
            taken.add(f.slotKey);
        }
    }

    /** H5.a-E3 — todas las sobrecargas de {@code name} visibles en {@code cls},
     *  mirando la clase y sus ancestros. Una firma que ya apareció en una subclase
     *  OCULTA a la del ancestro (es su override), así que gana la más derivada. */
    private List<FunctionSymbol> collectInstanceCandidates(ClassSymbol cls, String name) {
        List<FunctionSymbol> out = new ArrayList<>();
        java.util.Set<String> seenSig = new java.util.HashSet<>();
        for (ClassSymbol c : cls.dondeBuscar()) {   // V8 H7.3: en una interfaz, también las de las que hereda
            Symbol s = c.instanceMembers.tryLookup(name);
            if (!(s instanceof FunctionSymbol)) continue;
            for (FunctionSymbol o = (FunctionSymbol) s; o != null; o = o.nextOverload) {
                if (seenSig.add(o.signatureKey())) out.add(o);
            }
        }
        return out;
    }

    private void checkOverloadChain(Symbol s) {
        if (!(s instanceof FunctionSymbol)) return;
        FunctionSymbol head = (FunctionSymbol) s;
        if (!head.overloaded) return;
        for (FunctionSymbol a = head; a != null; a = a.nextOverload) {
            for (FunctionSymbol b = a.nextOverload; b != null; b = b.nextOverload) {
                if (a.hasSameSignatureAs(b)) {
                    int ln  = (b.astNode != null) ? b.astNode.line   : head.line;
                    int col = (b.astNode != null) ? b.astNode.column : head.column;
                    err(ln, col, "sobrecarga duplicada: '" + head.name
                            + "' con la misma firma de parámetros ya declarada");
                }
            }
        }
    }

    /** V8/H7.2 (#749) — ¿es la List de Core (o una hija: SyncList, OwnerList)? No una clase de usuario que se llame igual. */
    private boolean esLista(ClassSymbol cls) {
        for (ClassSymbol c = cls; c != null; c = c.baseClass)
            if ("List".equals(c.name) && (c.isExternal ? CORE_MODULE.equals(c.externalModule) : esRaiz())) return true;
        return false;
    }

    /** V8/H7.2 (#749) — ¿es el Map de Collections? */
    private boolean esMapa(ClassSymbol cls) {
        return cls != null && "Map".equals(cls.name)
                && (cls.isExternal ? "Collections".equals(cls.externalModule) : module != null && "Collections".equals(module.name));
    }

    /**
     * V8/H7.2 (#749) — los genéricos de lo que ya hay (Eduardo, 31-oct): `List<Object>` es la List de siempre, y
     * `Collections.Map<integer, Object>` y `Collections.Map<string, Object>`, el Map de siempre con la llave dicha. La
     * misma clase y el mismo bytecode; los argumentos sirven para comprobar la llave del Map al compilar. Lo demás
     * (`List<integer>`, los genéricos propios), de momento no: es error, y se devuelve la clase sin argumentos para no
     * encadenar errores.
     */
    private BpType resolveTipoGenerico(SimpleTypeRef st) {
        BpType base = resolveNamedType(st);
        if (base instanceof ErrorType) return base;
        List<BpType> args = new ArrayList<>();
        for (TypeRef a : st.argumentos) args.add(resolveType(a));
        for (BpType a : args) if (a instanceof ErrorType) return base;
        if (!(base instanceof ClassType)) {
            err(st.line, st.column, "'" + st.name + "' no es una clase: no lleva argumentos de tipo");
            return base;
        }
        ClassSymbol cls = ((ClassType) base).cls;
        if (esLista(cls)) {
            if (args.size() == 1 && esObjetoRaiz(args.get(0))) return new ClassType(cls, args);
            err(st.line, st.column, "de momento, una lista es " + cls.name + "<Object>: con otro tipo, más adelante (los"
                    + " genéricos de verdad)");
            return base;
        }
        if (esMapa(cls)) {
            if (args.size() != 2)
                err(st.line, st.column, "un Map lleva dos argumentos de tipo, la llave y el valor: Map<integer, Object> o"
                        + " Map<string, Object>");
            else if (!(args.get(0) == PrimitiveType.INTEGER || isString(args.get(0))))
                err(st.line, st.column, "la llave de un Map es integer o string, no '" + args.get(0).display() + "'");
            else if (!esObjetoRaiz(args.get(1)))
                err(st.line, st.column, "de momento, el valor de un Map es Object, no '" + args.get(1).display()
                        + "': con otro tipo, más adelante (los genéricos de verdad)");
            else
                return new ClassType(cls, args);
            return base;
        }
        err(st.line, st.column, "'" + cls.name + "' no es genérica: de momento, los genéricos son List<Object> y"
                + " Collections.Map<integer|string, Object>");
        return base;
    }

    /** V8/H7.2 (#749) — los métodos del Map que llevan la llave delante. */
    private static final java.util.Set<String> METODOS_CON_LLAVE = new java.util.HashSet<>(java.util.Arrays.asList(
            "put", "get", "containsKey", "remove", "getString", "getInteger", "getLong", "getFloat", "getDouble",
            "getBoolean"));

    /** V8/H7.2 (#749) — en un `Map<integer|string, Object>`, la llave de put/get/containsKey/remove/getX es la de su
     *  tipo: si no, un error al compilar (en un Map de llaves string, una integer fallaba al ejecutar). */
    private void revisaLlaveDeMap(CallExpr ce) {
        if (!(ce.callee instanceof MemberAccessExpr) || ce.args.isEmpty()) return;
        MemberAccessExpr ma = (MemberAccessExpr) ce.callee;
        if (!METODOS_CON_LLAVE.contains(ma.member)) return;
        BpType rt = info.exprTypes.get(ma.target);
        if (!(rt instanceof ClassType) || ((ClassType) rt).argumentos == null || !esMapa(((ClassType) rt).cls)) return;
        BpType llave = ((ClassType) rt).argumentos.get(0);
        BpType at = info.exprTypes.get(ce.args.get(0));
        if (at == null || at instanceof ErrorType) return;
        boolean ok = (llave == PrimitiveType.INTEGER) ? (at instanceof PrimitiveType && ((PrimitiveType) at).isIntegerLike())
                                                     : isString(at);
        if (!ok)
            err(ce.line, ce.column, "la llave de este Map es " + llave.display() + ", no '" + at.display() + "' ("
                    + ma.member + ")");
    }

    /** V8/H7.2 (#749) — un Map se crea con la fábrica de su llave: `Collections.numericMap()` para la integer y
     *  `Collections.Map()` (o `stringMap()`) para la string. Si el tipo declarado dice una y el valor es la otra, error:
     *  el comparador no casaría con las llaves. Sólo se mira cuando el valor ES la llamada; si viene de otra función,
     *  no se sabe. */
    private void revisaMapNuevo(BpType destino, IExpr valor, int line, int column) {
        if (!(destino instanceof ClassType) || ((ClassType) destino).argumentos == null) return;
        ClassType ct = (ClassType) destino;
        if (!esMapa(ct.cls) || !(valor instanceof CallExpr)) return;
        Symbol s = info.exprSymbols.get(((CallExpr) valor).callee);
        String fabrica = null;
        if (s instanceof ClassSymbol && esMapa((ClassSymbol) s)) fabrica = "Map";
        else if (s instanceof FunctionSymbol && ((FunctionSymbol) s).ownerClass == null
                && ("numericMap".equals(s.name) || "stringMap".equals(s.name))) fabrica = s.name;
        if (fabrica == null) return;
        boolean llaveEntera = ct.argumentos.get(0) == PrimitiveType.INTEGER;
        if (llaveEntera && !"numericMap".equals(fabrica))
            err(line, column, "una llave integer necesita Collections.numericMap(): Collections." + fabrica
                    + "() es de llaves string");
        else if (!llaveEntera && "numericMap".equals(fabrica))
            err(line, column, "una llave string necesita Collections.Map(): Collections.numericMap() es de llaves"
                    + " numéricas");
    }

    private BpType resolveType(TypeRef t) {
        if (t instanceof SimpleTypeRef && ((SimpleTypeRef) t).argumentos != null)
            return resolveTipoGenerico((SimpleTypeRef) t);   // V8/H7.2 (#749)
        if (t instanceof SimpleTypeRef) {
            SimpleTypeRef st = (SimpleTypeRef) t;
            switch (st.name) {
                case "integer": return PrimitiveType.INTEGER;
                case "any":     if (esRaiz()) return AnyType.INSTANCE; break;   // H1 P2: sólo Core (la raíz) declara `any`
                case "float":   return PrimitiveType.FLOAT;
                case "string":  return PrimitiveType.STRING;
                case "boolean": return PrimitiveType.BOOLEAN;
                // L10 — tipos estrechos. `short` ya fue traducido a "int16" por el parser.
                case "byte":    return PrimitiveType.UINT8;
                case "int8":    return PrimitiveType.INT8;
                case "word":    return PrimitiveType.UINT16;
                case "int16":   return PrimitiveType.INT16;
                case "long":    return PrimitiveType.LONG;   // H1.2 (V2)
                case "double":  return PrimitiveType.DOUBLE; // H1.3 (V2)
                // `Object` NO tiene caso aquí a propósito: cae al `default` y
                // resuelve, como cualquier otro nombre, a la CLASE dada de alta
                // arriba (la raíz que el emisor sintetiza desde H5.1.a).
                //
                // Hasta V5 devolvía `AnyType.INSTANCE`. Aquello venía de H5 (V2),
                // cuando la clase raíz aún no existía y se quiso un tipo comodín
                // expresable en fuente para escribir `Map` en BP puro. Pero `any`
                // es un hueco SIN etiqueta, así que el valor no sabe lo que es y
                // decide quien lo lee: una cadena en un `Object` imprimía su
                // handle, un `double` salía por sus 32 bits bajos, y estrechar a
                // una clase equivocada daba un número plausible en vez de fallar
                // (#389). Y contradecía al manual §4.5, que dice que `any` NO se
                // puede escribir en BP source: éste era el agujero por el que sí.
                default:        return resolveNamedType(st);
            }
        }
        if (t instanceof ArrayTypeRef)
            return new ArrayType(resolveType(((ArrayTypeRef) t).element));
        if (t instanceof Ast.TupleTypeRef) {
            java.util.List<BpType> elems = new java.util.ArrayList<>();
            for (TypeRef e : ((Ast.TupleTypeRef) t).elements) elems.add(resolveType(e));
            return new BpType.TupleType(elems);
        }
        return ErrorType.INSTANCE;
    }

    private BpType resolveNamedType(SimpleTypeRef st) {
        return resolveNamedType(st, true);
    }

    /** `avisa`: si el nombre no es un tipo, lo dice y da ErrorType; sin él, da null y no dice nada (V8/H8, `A[n]`). */
    private BpType resolveNamedType(SimpleTypeRef st, boolean avisa) {
        // L2 v2 — tipo cualificado `Mod.Type`: lookup en el namespace
        // importado bajo el alias.
        int dot = st.name.indexOf('.');
        if (dot >= 0) {
            String alias = st.name.substring(0, dot);
            String simple = st.name.substring(dot + 1);
            // BUG-2 bonus: auto-nombre 'Mod.Cls' dentro del propio módulo Mod.
            if (module != null && alias.equals(module.name)) {
                Symbol local = module.members.resolve(simple);
                if (local instanceof ClassSymbol) return new ClassType((ClassSymbol) local);
                if (local instanceof EnumSymbol)  return new EnumType((EnumSymbol) local);
            }
            Symbol nsSym = module.members.resolve(alias);
            if (nsSym instanceof Symbol.ImportedNamespaceSymbol) {
                Symbol.ImportedNamespaceSymbol ns = (Symbol.ImportedNamespaceSymbol) nsSym;
                Symbol cand = ns.classes.get(simple);
                if (cand == null) cand = ns.enums.get(simple);
                if (cand instanceof ClassSymbol) return new ClassType((ClassSymbol) cand);
                if (cand instanceof EnumSymbol)  return new EnumType((EnumSymbol) cand);
            }
            if (!avisa) return null;
            if (reportedUnknownType.add(st.name))
                err(st.line, st.column, "tipo cualificado '" + st.name + "' no encontrado");
            return ErrorType.INSTANCE;
        }
        Symbol sym = module.members.resolve(st.name);
        if (sym instanceof ClassSymbol) return new ClassType((ClassSymbol) sym);
        if (sym instanceof EnumSymbol)  return new EnumType((EnumSymbol) sym);
        // L2: si el nombre del tipo no resuelve en el scope local, buscamos
        // en los namespaces importados. Si hay UNA coincidencia, la usamos;
        // si hay varias (collision), pedimos qualificación con `Mod.Cls`.
        // Se recogen TODOS los candidatos antes de decidir: si el error saltara
        // dentro del bucle, el orden de visita mandaria — y con `Core` visitado
        // el ultimo nunca llegaria a ganar.
        Symbol fromImports = null;
        String importsAlias = null;
        Symbol deCore = null;
        boolean choque = false;
        String otroAlias = null;
        for (Symbol m : module.members.getSymbols()) {
            if (m instanceof Symbol.ImportedNamespaceSymbol) {
                Symbol.ImportedNamespaceSymbol ns = (Symbol.ImportedNamespaceSymbol) m;
                Symbol cand = ns.classes.get(st.name);
                if (cand == null) cand = ns.enums.get(st.name);
                if (cand == null) continue;
                if (CORE_MODULE.equals(ns.moduleName)) deCore = cand;
                if (fromImports != null && fromImports != cand) {
                    choque = true; otroAlias = ns.name;
                } else if (fromImports == null) {
                    importsAlias = ns.name;
                }
                if (fromImports == null) fromImports = cand;
            }
        }
        // `Core` gana siempre: se importa IMPLICITAMENTE en todos los modulos y es
        // donde viven los tipos raiz del lenguaje (Object, List, los envoltorios),
        // asi que un `List` pelado significa el de Core POR DEFINICION.
        //
        // Sin esto, TODO programa del ORM dejo de compilar: importa `SQLite` y
        // `Orm`, los dos importan `Core`, y cada interfaz reexporta las clases que
        // expone SIN decir de quien son — asi que el semantico veia dos simbolos
        // distintos donde hay UNA sola clase, y pedia elegir entre dos cosas
        // iguales. Se vio documentando el ORM, no compilando la stdlib.
        if (deCore != null) {
            if (deCore instanceof ClassSymbol) return new ClassType((ClassSymbol) deCore);
            if (deCore instanceof EnumSymbol)  return new EnumType((EnumSymbol) deCore);
        }
        if (choque) {
            err(st.line, st.column, "tipo '" + st.name
                    + "' ambiguo: aparece en '" + importsAlias
                    + "' y '" + otroAlias + "'; usa Module.Tipo");
            return ErrorType.INSTANCE;
        }
        if (fromImports instanceof ClassSymbol)
            return new ClassType((ClassSymbol) fromImports);
        if (fromImports instanceof EnumSymbol)
            return new EnumType((EnumSymbol) fromImports);
        if (!avisa) return null;
        if (reportedUnknownType.add(st.name))
            err(st.line, st.column, "tipo '" + st.name + "' no encontrado" + pistaCore(st.name));
        return ErrorType.INSTANCE;
    }

    // ============================================================
    // PASE 3 — Cuerpos
    // ============================================================
    private void bodyPass(ModuleNode mod) {
        for (ITopLevelDecl def : mod.defs)
            bodyForDef(def, null);
        for (Symbol s : module.members.getSymbols()) {
            if (s instanceof ClassSymbol) {
                ClassSymbol cls = (ClassSymbol) s;
                // #248: stubs aliasados de Core — externos, sin AST ni cuerpos.
                if (cls.isExternal || cls.astNode == null) continue;
                if (cls.isInterface) continue;   // V8 H7.3: una interfaz no tiene cuerpos ni redefine nada
                checkOverridesAndFinal(cls);
                checkInterfaces(cls);
                for (ITopLevelDecl m : cls.astNode.members)
                    bodyForDef(m, cls);
                checkCiclosDeDelegacion(cls);   // #511
            }
        }
    }

    private void bodyForDef(ITopLevelDecl def, ClassSymbol owner) {
        ClassSymbol save = currentClass;
        currentClass = owner;
        try {
            if      (def instanceof ConstDecl)   analyzeConstInit((ConstDecl) def);
            else if (def instanceof VarDecl)     analyzeVarInit((VarDecl) def);
            else if (def instanceof FuncDef)     analyzeFunction((FuncDef) def);
            else if (def instanceof PropertyDef) analyzeProperty((PropertyDef) def);
        } finally { currentClass = save; }
    }

    /**
     * V8 #777 (Eduardo, 2-nov: «una constante solamente puede ser un string real, al no ser una variable no contiene
     * referencias y como no contiene referencias no puede ser nulo. A mi me recuerda a int e int?») — el `null` de #774
     * es de las variables: una constante es un valor. Antes llegaba al emisor, que escribe una constante como un literal
     * del bloque de datos y paraba con un error interno («Símbolo de datos no declarado»).
     */
    private boolean constanteNula(ConstDecl c, BpType valor) {
        if (!(valor instanceof NullType)) return false;
        err(c.line, c.column, "la constante '" + c.name.name + "' no puede ser null: una constante es un valor, no "
                + "una variable, y no referencia nada (una variable sí puede valer null)");
        return true;
    }

    private void analyzeConstInit(ConstDecl c) {
        Symbol s = info.declSymbols.get(c);
        if (!(s instanceof ConstSymbol)) return;
        ConstSymbol cs = (ConstSymbol) s;
        BpType t = analyzeExpr(c.value, null, cs.type);
        boolean nula = constanteNula(c, t);
        if (cs.type == null) cs.type = t;
        else if (!cs.type.isAssignableFrom(t)
                && !acceptsNarrowFromLiteral(cs.type, t, c.value))
            err(c.line, c.column, narrowAssignErrorMessage(cs.type, t, c.value,
                    "valor de tipo '" + t.display() + "' no asignable a const de tipo '" + cs.type.display() + "'"));
        // L8 v2 — el valor literal viaja en el símbolo y el emisor lo INLINA en
        // cada uso (mismo mecanismo que las const importadas de .bpi). Antes,
        // una const de módulo string/long/double/bool caía a un global a CERO
        // que nadie asignaba (miVM petaba al leerla como string; VM-C imprimía
        // vacío — divergencia). Un init no-literal a nivel módulo nunca
        // funcionó → ahora es error claro en vez de 0 silencioso.
        // M6 — «optimización enum.member → valor», ANTES de aplicar la norma de
        // const. Si el init es un acceso a miembro de enum, su valor es el int
        // subyacente (lo mismo que el emisor inlina en emitMemberAccess). Con eso
        // `const K := Color.RED` se reduce a `const K := <int>` y cumple la norma de
        // const por construcción, sin que la norma tenga que conocer los enums. Ambas
        // VMs ven un simple push de int (sin divergencia).
        Long folded = foldEnumMemberToLiteral(c.value);
        cs.literalValue = (folded != null) ? folded : constLiteralValue(c.value);
        // N17 — una const NO-LOCAL requiere un valor literal: el emisor INLINA ese
        // literal en cada lectura. Sin literal no hay backing global declarado y la
        // lectura emitía GET_GLOBAL "Cls.K"/"K" → RuntimeException del ModWriter
        // ("símbolo de datos no declarado") que tumbaba el compilador. Antes el
        // error solo cubría nivel módulo (ownerClass==null); una const de INSTANCIA
        // de clase no-literal petaba el emisor. Ahora da diagnóstico limpio. V8 #789:
        // la de clase de JC (`private const int X = 1 + 2;`) también: lo mismo.
        if (!nula && cs.literalValue == null) {   // un null ya lo dijo #777
            String donde = (cs.ownerClass == null)
                    ? "usa una var asignada en la función inicializadora del módulo"
                    : "usa un campo (var) asignado en el constructor de la clase";
            err(c.line, c.column, "una const requiere un valor literal "
                    + "(entero/float/long/double/boolean/string, '-' opcional); "
                    + "para un valor calculado, " + donde);
        }
    }

    /** M6 — «optimización enum.member → valor»: pliega un acceso a miembro de enum
     *  (`Color.RED` o `Ns.Color.RED`) a su literal int, o null si no es eso. Espeja
     *  lo que hace el emisor en emitMemberAccess: el símbolo del target es el
     *  EnumSymbol y su mapa `values` da el int del miembro. Requiere que
     *  analyzeExpr(value) ya haya anotado info.exprSymbols (esta se invoca después).
     *  Sirve a cualquier consumidor de valores literales (hoy: la norma de const). */
    private Long foldEnumMemberToLiteral(IExpr e) {
        while (e instanceof ParenExpr) e = ((ParenExpr) e).inner;
        if (e instanceof MemberAccessExpr) {
            MemberAccessExpr ma = (MemberAccessExpr) e;
            Symbol tgt = info.exprSymbols.get(ma.target);
            if (tgt instanceof EnumSymbol)
                return ((EnumSymbol) tgt).values.get(ma.member);
        }
        return null;
    }

    private void analyzeVarInit(VarDecl v) {
        if (v.init == null) return;
        Symbol sym = info.declSymbols.get(v);
        BpType t = (sym instanceof VarSymbol) ? ((VarSymbol) sym).type : null;
        BpType got = analyzeExpr(v.init, null, t);
        // L8 v2 — array de TAMAÑO FIJO de módulo: el init es un inicializador
        // de COMPILE-TIME (se hornea elemento a elemento en el data block),
        // no un valor runtime — la asignabilidad de tipos array no aplica
        // (p.ej. [1,2,250] es integer[] pero hornea perfectamente en byte[4]).
        // validateFixedArrayInit hace la validación correcta por elemento.
        if (sym instanceof VarSymbol && ((VarSymbol) sym).fixedArraySize >= 0) {
            validateFixedArrayInit(v, (VarSymbol) sym);
            return;
        }
        if (t != null && !t.isAssignableFrom(got)
                && !acceptsNarrowFromLiteral(t, got, v.init))
            err(v.line, v.column, narrowAssignErrorMessage(t, got, v.init,
                    "valor de tipo '" + got.display() + "' no asignable a var de tipo '" + t.display() + "'"));
        revisaMapNuevo(t, v.init, v.line, v.column);   // V8/H7.2 (#749)
        if (t != null && t.isScalar() && !isString(t) && got instanceof NullType)   // #774: una variable string, sí
            err(v.line, v.column, "no se puede asignar 'null' a un tipo escalar '" + t.display() + "'");
        // L8 v2 — init de var a NIVEL MÓDULO: un literal escalar/string (con '-' opcional) se HORNEA en el
        // data block del .mod (string: ref asignada por __init). #590 (V7, 27-sep, Eduardo) — cualquier otra
        // expresión (un array literal, una llamada, un objeto…) la calcula __globales al cargar el módulo, y lo
        // mismo el de una `var` ESTÁTICA de clase: una global siempre vale lo que dice su declaración. Hasta hoy
        // se ignoraba con un aviso (y el de un estático, sin él). Los campos de instancia: #511.
    }

    /** L8 v2 — `var t: tipo[N] := [...]` de módulo: el init debe ser un array
     *  literal con TODOS los elementos literales (numéricos o boolean según el
     *  tipo) y longitud <= N; las posiciones restantes quedan a cero. */
    private void validateFixedArrayInit(VarDecl v, VarSymbol vs) {
        if (!(v.init instanceof ArrayLitExpr)) {
            err(v.line, v.column, "el init de un array de tamaño fijo debe ser un "
                    + "array literal [a, b, ...] con elementos literales");
            return;
        }
        ArrayLitExpr lit = (ArrayLitExpr) v.init;
        if (lit.elements.size() > vs.fixedArraySize) {
            err(v.line, v.column, "el array literal tiene " + lit.elements.size()
                    + " elementos pero el tamaño declarado es " + vs.fixedArraySize);
            return;
        }
        for (IExpr e : lit.elements) {
            Object val = constLiteralValue(e);
            if (val == null || val instanceof String) {
                err(((Ast.Node) e).line, ((Ast.Node) e).column,
                    "los elementos del init de un array fijo deben ser "
                    + "literales numéricos o boolean");
                return;
            }
        }
    }

    /** L8 v2 — ¿tipo de elemento horneable en el data block? Primitivos sí;
     *  string/clases/refs NO (necesitan heap visible para el GC). */
    private static boolean isBakeableArrayElement(BpType t) {
        if (!(t instanceof ArrayType)) return false;
        BpType el = ((ArrayType) t).element;
        if (!(el instanceof PrimitiveType)) return false;
        switch (((PrimitiveType) el).tag) {
            case STRING: return false;
            default:     return true;
        }
    }

    private void analyzeFunction(FuncDef f) {
        Symbol s = info.declSymbols.get(f);
        if (!(s instanceof FunctionSymbol)) return;
        FunctionSymbol fs = (FunctionSymbol) s;

        FunctionSymbol save = currentFunction;
        boolean saveNative = currentFunctionIsNative;
        currentFunctionIsNative = f.isNative;   // L8 v3: guard de T[N] en native
        currentFunction = fs;
        Scope localScope = new Scope("locals", currentClass == null ? module.members : null);

        // Parámetros como locales: usamos el ParamSymbol DIRECTAMENTE (no creamos
        // un VarSymbol "wrapper") para que info.exprSymbols apunte al mismo
        // objeto que el JvmEmitter guarda en ctx.paramSlots.
        for (ParamSymbol p : fs.params) {
            if (!localScope.tryDefine(p))
                err(p.line, p.column, "parámetro duplicado: '" + p.name + "'");
        }

        isFirstStmtOfCtor = fs.isConstructor;
        seenSuperCallInCtor = false;
        if (enJc && fs.isConstructor && currentClass != null) {   // V8 #698: la delegación no lleva `new`
            CallExpr d = llamadaAlPropioCtor(f.body, currentClass);
            if (d != null) delegacionesCtor.add(d);
        }

        analyzeBody(f.body, localScope);
        // #540 (V7/H1) — la asignación definida de las locales, sobre el cuerpo ya anotado.
        new AsignacionDefinida(info, this::err).revisa(f.body);

        // #511 (19-sep) — DELEGACIÓN: `MiClase(args)` como PRIMERA sentencia de un constructor
        // inicializa ESTE objeto por esa otra sobrecarga (Eduardo: «la primera línea del
        // constructor dice cómo se construye el objeto»). En cualquier otro sitio `MiClase(...)`
        // es un objeto nuevo. El que delega no hace super() ni inicializadores: los hace el
        // delegado. Antes esto compilaba como un objeto nuevo que se tiraba (mudo).
        if (fs.isConstructor && currentClass != null) {
            CallExpr d = llamadaAlPropioCtor(f.body, currentClass);
            if (d != null) {
                Symbol anotado = info.exprSymbols.get(d);
                FunctionSymbol elegido = (anotado instanceof FunctionSymbol) ? (FunctionSymbol) anotado : currentClass.findConstructor();
                if (elegido == fs)
                    err(d.line, d.column, "el constructor '" + currentClass.name + "(" + firmaDe(fs) + ")' no puede delegar en sí mismo (sería recursivo)");
                else if (seenSuperCallInCtor)
                    err(d.line, d.column, "un constructor que delega en '" + currentClass.name + "(" + firmaDe(elegido) + ")' no puede llamar además a 'super(...)': lo hace el delegado");
                else if (elegido != null && elegido.ownerClass == currentClass)
                    fs.delegaA = elegido;
            }
        }

        // Auto-super (modelo Java): un constructor que no llama super() explícito
        // invoca implícitamente al constructor del padre SIN argumentos. Si el
        // constructor del padre EXIGE argumentos, la llamada debe ser explícita →
        // error. Si el padre no tiene constructor (NEW_OBJECT ya inicializa los
        // campos), no hay nada que llamar.
        if (fs.isConstructor && !seenSuperCallInCtor && fs.delegaA == null
                && currentClass != null && currentClass.baseClass != null) {
            FunctionSymbol baseCtor = currentClass.baseClass.findConstructor();
            if (baseCtor != null) {
                // V7/H2 (#509) — con sobrecarga, la sin argumentos puede no ser la
                // cabeza: se busca en la cadena y se guarda CUÁL para el emisor.
                FunctionSymbol sinArgs = null;
                for (FunctionSymbol c = baseCtor; c != null; c = c.nextOverload)
                    if (c.params.isEmpty()) { sinArgs = c; break; }
                if (sinArgs != null) {
                    fs.implicitSuper = true;   // el emisor inyecta super()
                    fs.implicitSuperCtor = sinArgs;
                    checkVisibilidadCtor(f.line, f.column, sinArgs, currentClass.baseClass);   // #515
                } else {
                    err(f.line, f.column, "el constructor de '" + currentClass.name
                            + "' debe llamar a 'super(...)': el constructor de la clase base '"
                            + currentClass.baseClass.name + "' requiere argumentos");
                }
            }
        }

        currentFunction = save;
        currentFunctionIsNative = saveNative;
    }

    /** #511 — la llamada `MiClase(args)` si es la PRIMERA sentencia del cuerpo, o null. */
    private static CallExpr llamadaAlPropioCtor(List<IStmt> body, ClassSymbol cls) {
        if (body == null || body.isEmpty() || !(body.get(0) instanceof ExprStmt)) return null;
        IExpr e = ((ExprStmt) body.get(0)).expr;
        if (!(e instanceof CallExpr)) return null;
        CallExpr ce = (CallExpr) e;
        if (ce.callee instanceof IdentifierExpr && ((IdentifierExpr) ce.callee).name.equals(cls.name)) return ce;
        return null;
    }
    private static String firmaDe(FunctionSymbol f) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < f.params.size(); i++) {
            if (i > 0) sb.append(", ");
            sb.append(f.params.get(i).type != null ? f.params.get(i).type.display() : "?");
        }
        return sb.toString();
    }

    /** #515 (19-sep) — la visibilidad de un constructor es la de cualquier miembro (Eduardo: «si se
     *  indica private es privado, si se indica public es público y si no se dice nada es
     *  protegido»): `private` → sólo la propia clase lo construye o delega en él; protegido →
     *  además sus subclases (`super(...)`, o construirla); `public` → cualquiera. Vale igual
     *  para la construcción, `super(...)` explícito o implícito y la delegación. Antes no se
     *  comprobaba en ningún sitio. */
    private void checkVisibilidadCtor(int line, int col, FunctionSymbol ctor, ClassSymbol cls) {
        if (ctor == null || cls == null) return;
        ClassSymbol declarante = (ctor.ownerClass != null) ? ctor.ownerClass : cls;
        if (ctor.isPrivate) {
            if (currentClass != declarante)
                err(line, col, "el constructor '" + declarante.name + "(" + firmaDe(ctor) + ")' es private: sólo lo usa '"
                    + declarante.name + "'. Si quieres que también lo usen sus subclases, quítale 'private' — un constructor sin marcar ya es protegido");
            return;
        }
        if (ctor.isPublic) return;
        if (currentClass != null && (currentClass == declarante || currentClass.isSubclassOf(declarante))) return;
        err(line, col, "el constructor '" + declarante.name + "(" + firmaDe(ctor) + ")' es protegido: lo usan esa clase y sus subclases"
            + " (desde 'super(...)' o construyéndola), no desde fuera. Hazlo 'public' si tiene que construirse desde cualquier sitio");
    }

    /** #511 — un ciclo de delegación (A → B → A) no termina nunca: se comprueba con todos los
     *  constructores de la clase ya analizados. */
    private void checkCiclosDeDelegacion(ClassSymbol cls) {
        for (FunctionSymbol c = cls.constructor; c != null; c = c.nextOverload) {
            if (c.delegaA == null) continue;
            java.util.List<FunctionSymbol> camino = new ArrayList<>();
            FunctionSymbol k = c;
            while (k != null && !camino.contains(k)) { camino.add(k); k = k.delegaA; }
            if (k == c) {
                StringBuilder sb = new StringBuilder();
                for (FunctionSymbol p : camino) sb.append(cls.name).append('(').append(firmaDe(p)).append(") → ");
                sb.append(cls.name).append('(').append(firmaDe(c)).append(')');
                err(c.line, c.column, "los constructores de '" + cls.name + "' se delegan en ciclo: " + sb + " (no terminaría nunca)");
                return;
            }
        }
    }

    private void analyzeProperty(PropertyDef p) {
        Symbol s = info.declSymbols.get(p);
        if (!(s instanceof PropertySymbol)) return;
        PropertySymbol ps = (PropertySymbol) s;

        if (p.init != null) {
            BpType t = analyzeExpr(p.init, null, ps.type);
            if (ps.type != null && !ps.type.isAssignableFrom(t))
                err(p.line, p.column, "valor inicial de tipo '" + t.display() + "' no asignable a propiedad '" + ps.type.display() + "'");
            // #590 (V7, 27-sep, Eduardo) — el init de una property de MÓDULO o ESTÁTICA de clase, como el de
            //     una `var` de módulo: un literal se hornea en su backing, lo demás lo calcula __globales al
            //     cargar el módulo. Hasta hoy se ignoraba SIEMPRE, con aviso: `property s: string := "hola"`
            //     valía null. (#511: las de INSTANCIA se inicializan en el constructor, tras el super().)
        }
        if (p.getter != null) {
            insideGetter = true;
            currentPropType = ps.type;   // #547
            // Tratamos el cuerpo del getter como una función con return type
            // = tipo de la property; esto habilita `return value` dentro y
            // permite el typecheck del return correctamente.
            FunctionSymbol save = currentFunction;
            FunctionSymbol fakeGet = new FunctionSymbol("get" + ps.name, ps.isPublic, false,
                                                       ps.isStatic, ps.ownerClass, null);
            fakeGet.returnType = ps.type;
            currentFunction = fakeGet;
            try {
                Scope localScope = new Scope("getter", null);
                analyzeBody(p.getter.body, localScope);
                new AsignacionDefinida(info, this::err).revisa(p.getter.body);   // #540
            } finally {
                currentFunction = save;
                insideGetter = false;
                currentPropType = null;
            }
        }
        if (p.setter != null) {
            insideSetter = true;
            currentPropType = ps.type;   // #547
            setterParamName = p.setter.paramName;
            // Setter es void; un `return` sin valor permitido para early-exit.
            FunctionSymbol save = currentFunction;
            FunctionSymbol fakeSet = new FunctionSymbol("set" + ps.name, ps.isPublic, false,
                                                       ps.isStatic, ps.ownerClass, null);
            fakeSet.returnType = null;  // void
            currentFunction = fakeSet;
            try {
                Scope localScope = new Scope("setter", null);
                VarSymbol pv = new VarSymbol(p.setter.paramName, false, false, null, true,
                                             p.setter.line, p.setter.column);
                pv.type = ps.type;
                localScope.tryDefine(pv);
                analyzeBody(p.setter.body, localScope);
                new AsignacionDefinida(info, this::err).revisa(p.setter.body);   // #540
            } finally {
                currentFunction = save;
                insideSetter = false;
                currentPropType = null;
                setterParamName = null;
            }
        }
    }

    // ============================================================
    // STATEMENTS
    // ============================================================
    private void analyzeBody(List<IStmt> body, Scope scope) {
        boolean returnSeen = false;
        for (IStmt s : body) {
            if (returnSeen) {
                Node n = (Node) s;
                warn(n.line, n.column, "código inalcanzable después de 'return'");
            }
            analyzeStmt(s, scope);
            if (s instanceof ReturnStmt) returnSeen = true;
        }
    }

    private void analyzeStmt(IStmt s, Scope scope) {
        // Solo super(...) como sentencia preserva 'soy la primera del ctor'.
        boolean isSuperCallStmt = s instanceof ExprStmt && ((ExprStmt) s).expr instanceof SuperCallExpr;
        if (!isSuperCallStmt) isFirstStmtOfCtor = false;

        if      (s instanceof InlineStmt)   analyzeInline((InlineStmt) s);   // H1 (20-sep)
        else if (s instanceof VarDecl)      analyzeLocalVarDecl((VarDecl) s, scope);
        else if (s instanceof ConstDecl)    analyzeLocalConstDecl((ConstDecl) s, scope);
        else if (s instanceof AssignStmt)   analyzeAssign((AssignStmt) s, scope);
        else if (s instanceof Ast.RaiseStmt) analyzeRaise((Ast.RaiseStmt) s, scope);
        else if (s instanceof IfStmt)       analyzeIf((IfStmt) s, scope);
        else if (s instanceof SwitchStmt)   analyzeSwitch((SwitchStmt) s, scope);
        else if (s instanceof ParallelStmt) analyzeParallel((ParallelStmt) s, scope);
        else if (s instanceof WhileStmt)    analyzeWhile((WhileStmt) s, scope);
        else if (s instanceof DoLoopStmt)   analyzeDoLoop((DoLoopStmt) s, scope);
        else if (s instanceof ForStmt)      analyzeFor((ForStmt) s, scope);
        else if (s instanceof TryStmt)      analyzeTry((TryStmt) s, scope);
        else if (s instanceof Ast.ReleaseOwnerStmt) { }   // V7/H1: lo pone el parser en un `try var`; sólo el emisor lo mira
        else if (s instanceof ReturnStmt) {
            if (!finallysAbiertos.isEmpty())   // V7 #674
                err(((ReturnStmt) s).line, ((ReturnStmt) s).column, "un 'finally' no puede salir con 'return': "
                    + "taparia la excepcion que fuera en vuelo (#674); devuelve el valor despues del 'endtry'");
            analyzeReturn((ReturnStmt) s, scope);
        }
        else if (s instanceof Ast.DestructAssignStmt) analyzeDestructAssign((Ast.DestructAssignStmt) s, scope);
        else if (s instanceof ThrowStmt) {
            // #248 — solo se lanzan instancias de Exception (o descendientes).
            ThrowStmt th = (ThrowStmt) s;
            BpType thrown = analyzeExpr(th.value, scope, null);
            if (thrown instanceof ClassType) {
                ClassSymbol tcls = ((ClassType) thrown).cls;
                if (!descendsFromException(tcls)) {
                    err(th.line, th.column, "solo se puede lanzar una excepción: '"
                        + tcls.name + "' no desciende de Exception (#248)");
                }
            } else if (!(thrown instanceof ErrorType) && !(thrown instanceof AnyType)) {
                err(th.line, th.column,
                    "solo se puede lanzar una instancia de Exception (#248)");
            }
        }
        else if (s instanceof PrintStmt) {
            for (PrintItem it : ((PrintStmt) s).items)
                if (it.expr != null) {
                    BpType pt = analyzeExpr(it.expr, scope, null);
                    if (pt instanceof BpType.TupleType)
                        err(((Node) it.expr).line, ((Node) it.expr).column,
                            "no se puede imprimir una tupla; desempaquétala con '{ ... } := ...'");
                }
        }
        else if (s instanceof BreakStmt) {
            BreakStmt b = (BreakStmt) s;
            if (b.nombre != null) saltoConNombre("break", b.nombre, b.line, b.column);
            else if (loopDepth == 0 && switchDepth == 0)
                err(b.line, b.column, "'break' fuera de bucle o switch");
            else if (!finallysAbiertos.isEmpty()   // V7 #674: su bucle o su switch, fuera del finally
                     && loopDepth + switchDepth == finallysAbiertos.peek()[0] + finallysAbiertos.peek()[1])
                err(b.line, b.column, "un 'finally' no puede salir con 'break': taparia la excepcion que fuera en "
                    + "vuelo (#674)");
        }
        else if (s instanceof ContinueStmt) {
            ContinueStmt cs = (ContinueStmt) s;
            if (cs.nombre != null) saltoConNombre("continue", cs.nombre, cs.line, cs.column);
            else if (loopDepth == 0)
                err(cs.line, cs.column, "'continue' fuera de bucle");
            else if (!finallysAbiertos.isEmpty() && loopDepth == finallysAbiertos.peek()[0])   // V7 #674
                err(cs.line, cs.column, "un 'finally' no puede salir con 'continue': taparia la excepcion que fuera "
                    + "en vuelo (#674)");
        }
        else if (s instanceof ExprStmt) {
            ExprStmt es = (ExprStmt) s;
            analyzeExpr(es.expr, scope, null);
            if (!(es.expr instanceof CallExpr || es.expr instanceof SuperCallExpr))
                err(es.line, es.column, "se esperaba una llamada como sentencia");
            else if (es.expr instanceof CallExpr && esTernario((CallExpr) es.expr))
                err(es.line, es.column, Ast.TERNARIO + "(…) es un valor, no una sentencia: para elegir qué hacer, un if"
                        + " (como en C#)");   // V8/H7.1 (#747)
        }
    }

    private void analyzeLocalVarDecl(VarDecl v, Scope scope) {
        for (DeclName n : v.names) checkNombreReservado(n.name, v.line, v.column);   // M4
        BpType t = resolveType(v.type);
        // L8 v3 — local `tipo[N]` (N literal entero > 0): array HEAP de N
        // elementos a cero (el alloc zero-inicializa en ambas VMs), con
        // binding fija como el de módulo (la var no es reasignable; sus
        // elementos sí). Antes el [N] se ignoraba EN SILENCIO y la ref
        // quedaba sin inicializar (divergencia dual-VM).
        Integer fixedN = null;
        if (v.type instanceof ArrayTypeRef && ((ArrayTypeRef) v.type).size != null) {
            ArrayTypeRef at = (ArrayTypeRef) v.type;
            Object n = constLiteralValue(at.size);
            if (!(n instanceof Long) || (Long) n <= 0) {
                err(at.line, at.column,
                    "el tamaño de un array fijo debe ser un literal entero > 0");
            } else if (!isBakeableArrayElement(t)) {
                err(at.line, at.column,
                    "array de tamaño fijo solo admite elementos primitivos "
                    + "(integer/float/long/double/boolean/byte/word/int8/int16); "
                    + "para string o clases usa 'tipo[]'");
            } else if (currentFunctionIsNative) {
                err(at.line, at.column,
                    "array de tamaño fijo local no soportado en función native "
                    + "(el emisor AOT no lo genera); usa `var v: tipo[] := tipo[n]`");
            } else {
                fixedN = (int) (long) (Long) n;
            }
        }
        if (v.isOwner && !(t instanceof ClassType)) {
            err(v.line, v.column, "'var owner' requiere un tipo clase; '"
                    + t.display() + "' no lo es");
        }
        for (DeclName n : v.names) {
            if (n.isStatic())
                err(n.line, n.column, "no se puede declarar miembro estático en un cuerpo de función");
            VarSymbol sym = new VarSymbol(n.name, false, false, null, true, n.line, n.column);
            sym.type = t;
            sym.isOwner = v.isOwner;
            if (fixedN != null) sym.fixedArraySize = fixedN;
            if (!scope.tryDefine(sym))
                err(n.line, n.column, "variable duplicada en este scope: '" + n.name + "'");
            if (!info.declSymbols.containsKey(v)) info.declSymbols.put(v, sym);
        }
        if (v.init != null) {
            BpType got = analyzeExpr(v.init, scope, t);
            // L8 v3 — init de array fijo: literal validado POR ELEMENTO
            // (longitud <= N, resto a cero), igual que a nivel módulo; la
            // asignabilidad general de arrays no aplica ([1,2,250] es
            // integer[] pero entra perfectamente en byte[4]).
            if (fixedN != null) {
                Symbol fs = info.declSymbols.get(v);
                if (fs instanceof VarSymbol) validateFixedArrayInit(v, (VarSymbol) fs);
                return;
            }
            if (!t.isAssignableFrom(got)
                    && !acceptsNarrowFromLiteral(t, got, v.init))
                err(v.line, v.column, narrowAssignErrorMessage(t, got, v.init,
                        "valor de tipo '" + got.display() + "' no asignable a variable de tipo '" + t.display() + "'"));
            revisaMapNuevo(t, v.init, v.line, v.column);   // V8/H7.2 (#749)
            if (t.isScalar() && !isString(t) && got instanceof NullType)       // #774: una variable string, sí
                err(v.line, v.column, "no se puede asignar 'null' a un tipo escalar '" + t.display() + "'");
        }
    }

    /** Mensaje extendido para el error de asignación cuando lhs es narrow:
     *  si la fuente es un literal entero que no cabe en el narrow target,
     *  damos el rango exacto en lugar del mensaje genérico. */
    private static String narrowAssignErrorMessage(BpType target, BpType source,
                                                   IExpr sourceExpr, String fallback) {
        if (!(target instanceof PrimitiveType)) return fallback;
        PrimitiveType t = (PrimitiveType) target;
        if (!t.isNarrowInteger()) return fallback;
        Long lit = constantIntValueOf(sourceExpr);
        if (lit != null && (source instanceof PrimitiveType
                && ((PrimitiveType) source).tag == PrimitiveType.Kind.INTEGER)) {
            return "literal " + lit + " fuera del rango de " + t.display()
                    + " (" + t.rangeMin() + ".." + t.rangeMax() + ")";
        }
        return "asignación a '" + t.display() + "' requiere cast explícito: "
                + t.display() + "(<valor>)";
    }

    private void analyzeLocalConstDecl(ConstDecl c, Scope scope) {
        if (c.name.isStatic())
            err(c.line, c.column, "no se puede declarar const estática en un cuerpo de función");
        BpType declared = (c.type == null) ? null : resolveType(c.type);
        BpType got = analyzeExpr(c.value, scope, declared);
        constanteNula(c, got);
        BpType t = (declared != null) ? declared : got;
        ConstSymbol sym = new ConstSymbol(c.name.name, false, false, null, c.line, c.column);
        sym.type = t;
        sym.isLocal = true;
        // L8 v2 — si el init es literal, el emisor lo inlina en cada lectura
        // (mismo mecanismo que las const de módulo); si no, la const vive en
        // un slot local de única asignación.
        sym.literalValue = constLiteralValue(c.value);
        if (!scope.tryDefine(sym))
            err(c.line, c.column, "const duplicada: '" + c.name.name + "'");
        if (declared != null && !declared.isAssignableFrom(got)
                && !acceptsNarrowFromLiteral(declared, got, c.value))
            err(c.line, c.column, narrowAssignErrorMessage(declared, got, c.value,
                    "valor de tipo '" + got.display() + "' no asignable a const de tipo '" + declared.display() + "'"));
        info.declSymbols.put(c, sym);
    }

    /** #324 tanda 2c — ¿es la forma `__methodRef(obj, "nombre")`? Se reconoce por
     *  la FORMA (nombre + 2 argumentos), no por un símbolo: es una forma del
     *  emisor, no una función que exista en ninguna parte. Mismo patrón que el
     *  `Thread(obj::metodo(args))` de #325. */
    static boolean isMethodRefByName(Ast.IExpr e) {
        if (!(e instanceof Ast.CallExpr)) return false;
        Ast.CallExpr c = (Ast.CallExpr) e;
        return (c.callee instanceof Ast.IdentifierExpr)
                && "__methodRef".equals(((Ast.IdentifierExpr) c.callee).name)
                && c.args.size() == 2;
    }

    /**
     * H5.c — ¿es `a` una suscripción a un evento? Si lo es, la valida entera y
     * devuelve true (el llamante no sigue por el camino normal).
     *
     * Un evento no tiene tipo y una referencia a método tampoco, así que aquí no
     * se unifica nada: se comprueba que el par encaja. Las formas válidas:
     *   ev := obj::metodo               suscribir (el slot lo pone el compilador)
     *   ev := __methodRef(obj, "n")     suscribir con el nombre resuelto en
     *                                   EJECUCIÓN — para cargadores (#324 2c)
     *   ev := null                      desuscribir
     */
    private boolean analyzeEventSubscription(AssignStmt a, Scope scope) {
        Symbol.EventSymbol ev = resolveEventTarget(a.target, scope);
        if (ev == null) {
            // Si NO es un evento pero le asignan un `::`, el error va aquí y no
            // en el camino genérico, que hablaría de tipos y despistaría.
            if (a.value instanceof Ast.MethodRefExpr)
                err(a.line, a.column, "'::' sólo se puede asignar a un evento; "
                        + "una referencia a método no es un valor que se pueda guardar en otro sitio");
            return false;
        }
        if (a.op != Ast.AssignOpKind.ASSIGN) {
            err(a.line, a.column, "a un evento sólo se le asigna ( := ): no admite += ni -=."
                    + " Un evento tiene UN suscriptor, así que suscribirse es asignar y"
                    + " desuscribirse es asignar null");
            return true;
        }
        // Desuscripción explícita.
        if (a.value instanceof Ast.NullLitExpr) return true;

        // #324 tanda 2c — TERCERA forma, para cargadores: __methodRef(obj, "nombre").
        //
        // `obj::metodo` lo resuelve el COMPILADOR, así que no sirve cuando el
        // nombre del handler sale de un fichero en EJECUCIÓN — que es lo que
        // hace el cargador de Forms al leer el .win. Sin esto, Gui.bp tenía que
        // llevar su propio (receptor, slot) a mano en tres campos: exactamente
        // el duplicado que esta tanda viene a borrar.
        //
        // ⚠️ Resolver por nombre NO comprueba la firma. Con `::` el semántico
        // valida argumentos y visibilidad; aquí no hay a quién mirar hasta que
        // se ejecuta. Es el precio, y no es una regresión: el camino de Forms
        // que sustituye (__guiInvokeBySlot) tenía el mismo agujero.
        if (isMethodRefByName(a.value)) {
            Ast.CallExpr c = (Ast.CallExpr) a.value;
            BpType ot = analyzeExpr(c.args.get(0), scope, null);
            if (!(ot instanceof BpType.ClassType) && !(ot instanceof AnyType)) {
                err(a.line, a.column, "__methodRef: el primer argumento tiene que ser"
                        + " el objeto que atiende el evento, y aquí es '"
                        + (ot != null ? ot.display() : "desconocido") + "'");
            }
            BpType nt = analyzeExpr(c.args.get(1), scope, null);
            if (nt != PrimitiveType.STRING) {
                err(a.line, a.column, "__methodRef: el segundo argumento es el NOMBRE"
                        + " del método (string), y aquí es '"
                        + (nt != null ? nt.display() : "desconocido") + "'");
            }
            return true;
        }

        if (!(a.value instanceof Ast.MethodRefExpr)) {
            err(a.line, a.column, "a un evento se le asigna una referencia a método"
                    + " (`obj::metodo`), `__methodRef(obj, \"nombre\")` si el nombre"
                    + " sale de un fichero, o `null` para desuscribir");
            return true;
        }
        Ast.MethodRefExpr mr = (Ast.MethodRefExpr) a.value;
        BpType recvT = analyzeExpr(mr.target, scope, null);
        Symbol.ClassSymbol rc = (recvT instanceof BpType.ClassType) ? ((BpType.ClassType) recvT).cls : null;
        if (rc == null) {
            err(mr.line, mr.column, "el receptor de '::' tiene que ser un objeto"
                    + " (para poder llamar a su método), y aquí es '"
                    + (recvT != null ? recvT.display() : "desconocido") + "'");
            return true;
        }
        Symbol ms = rc.lookupInstance(mr.method);
        if (!(ms instanceof FunctionSymbol)) {
            err(mr.line, mr.column, "la clase '" + rc.name + "' no tiene un método '"
                    + mr.method + "' al que apuntar");
            return true;
        }
        // H5.c (revisión) — de todas las sobrecargas con ese nombre, elegir la que
        // CASA con la firma declarada del evento. Aquí es donde `::` se convierte
        // en un sitio de resolución de sobrecarga: si Panel tiene pulsado(b: Boton)
        // y pulsado(s: Slider), la firma del evento decide cuál.
        FunctionSymbol fs = pickHandlerForEvent((FunctionSymbol) ms, ev, mr, rc);
        if (fs == null) return true;
        if (fs.isStatic) {
            err(mr.line, mr.column, "'" + mr.method + "' es static: no tiene receptor,"
                    + " así que no se puede referenciar con `obj::" + mr.method + "`");
            return true;
        }
        // El destino es el slot de vtable, y tiene que existir AHORA (en
        // compilación): si no, el emisor escribiría un -1 que el despachador
        // interpretaría como basura en tiempo de ejecución.
        if (!fs.isPublic && !fs.isEventHandler) {
            err(mr.line, mr.column, "'" + mr.method + "' no se puede colgar de un evento:"
                    + " decláralo `function event " + mr.method + "(...)`. El destino de un"
                    + " evento es un slot de vtable, y una función privada sin marcar no lo"
                    + " tiene — marcarla se lo da SIN hacerla llamable por su nombre desde fuera");
            return true;
        }
        // OJO: el slot se pide con la CLAVE de la sobrecarga elegida, no con el
        // nombre pelado. Con `pulsado(Boton,integer)` y `pulsado(string)`, el
        // nombre pelado devuelve siempre el slot de la PRIMERA declarada (regla
        // H5.a), así que si la que casa con el evento es la segunda, colgaríamos
        // el handler equivocado — mudo, y con la firma equivocada al despachar.
        int slot = rc.slotOf(MivmEmitter.emitName(fs));
        if (slot < 0) {
            err(mr.line, mr.column, "'" + mr.method + "' no tiene slot de vtable en '"
                    + rc.name + "' — no se puede referenciar");
            return true;
        }
        info.exprSymbols.put(mr, fs);
        info.methodRefSlots.put(mr, slot);
        return true;
    }

    /**
     * H5.c — elige la sobrecarga de `cand` (y su cadena de sobrecargas) cuya
     * firma casa con la del evento. null + error si ninguna.
     *
     * "Casar" es: mismo número de parámetros y mismo tipo en cada posición. No
     * se admite que el handler tome MENOS parámetros que el evento aunque sea
     * cómodo (lo hace JavaScript): el despachador tendría N valores y el frame
     * esperaría M, y eso es un desajuste de aridad sobre una pila cruda — la
     * misma clase de fallo que costó H1 entera (los pops de 4 en pila de 8).
     */
    private FunctionSymbol pickHandlerForEvent(FunctionSymbol cand, Symbol.EventSymbol ev,
                                               Ast.MethodRefExpr mr, Symbol.ClassSymbol rc) {
        List<FunctionSymbol> cands = new ArrayList<>();
        for (FunctionSymbol f = cand; f != null; f = f.nextOverload) cands.add(f);
        for (FunctionSymbol f : cands) {
            if (f.params.size() != ev.params.size()) continue;
            boolean ok = true;
            for (int i = 0; i < f.params.size(); i++) {
                BpType a = f.params.get(i).type, b = ev.params.get(i).type;
                // V10 (16-sep, H3): por TIPO (sameAs); el display de una clase importada es
                // «Lib.Base» y el del handler «Base» — el mismo símbolo, textos distintos.
                if (a == null || b == null || !(a.sameAs(b) || a.display().equals(b.display()))) { ok = false; break; }
            }
            if (ok) return f;
        }
        err(mr.line, mr.column, "'" + rc.name + "." + mr.method + "' no casa con la firma del"
                + " evento '" + ev.name + "' " + signatureOf(ev)
                + (cands.size() > 1 ? " (se probaron " + cands.size() + " sobrecargas)" : ""));
        return null;
    }

    private String signatureOf(Symbol.EventSymbol ev) {
        StringBuilder sb = new StringBuilder("(");
        for (int i = 0; i < ev.params.size(); i++) {
            if (i > 0) sb.append(", ");
            BpType t = ev.params.get(i).type;
            sb.append(ev.params.get(i).name).append(": ").append(t != null ? t.display() : "?");
        }
        return sb.append(")").toString();
    }

    /** El target de la asignación, ¿es un evento? null si no. */
    private Symbol.EventSymbol resolveEventTarget(IExpr target, Scope scope) {
        if (target instanceof MemberAccessExpr) {
            MemberAccessExpr ma = (MemberAccessExpr) target;
            BpType t = analyzeExpr(ma.target, scope, null);
            Symbol.ClassSymbol cs = (t instanceof BpType.ClassType) ? ((BpType.ClassType) t).cls : null;
            if (cs == null) return null;
            Symbol s = cs.lookupInstance(ma.member);
            if (s instanceof Symbol.EventSymbol) {
                info.exprSymbols.put(ma, s);
                return (Symbol.EventSymbol) s;
            }
            return null;
        }
        if (target instanceof IdentifierExpr && currentClass != null) {
            // `onClick := ...` dentro de la propia clase (this implícito).
            Symbol s = currentClass.lookupInstance(((IdentifierExpr) target).name);
            if (s instanceof Symbol.EventSymbol) {
                info.exprSymbols.put(target, s);
                return (Symbol.EventSymbol) s;
            }
        }
        return null;
    }

    /** Un `::` fuera de una suscripción no significa nada: hay que decirlo. */
    private void rejectStrayMethodRef(IExpr e) {
        if (e instanceof Ast.MethodRefExpr) {
            Ast.MethodRefExpr m = (Ast.MethodRefExpr) e;
            err(m.line, m.column, "'::' sólo vale para suscribir a un evento"
                    + " (`algo.onX := obj::metodo`); no es un valor de primera clase");
        }
    }

    /**
     * H5.c — `raise onClick(args)`. Sólo dentro de la clase que declara el
     * evento o de una descendiente: lo dispara SU objeto y nadie más.
     */
    private void analyzeRaise(Ast.RaiseStmt r, Scope scope) {
        if (currentClass == null) {
            err(r.line, r.column, "'raise' sólo vale dentro de una clase: un evento lo dispara"
                    + " su propio objeto, y fuera de una clase no hay quién");
            return;
        }
        Symbol s = currentClass.lookupInstance(r.event);
        if (!(s instanceof Symbol.EventSymbol)) {
            err(r.line, r.column, "'" + r.event + "' no es un evento de '" + currentClass.name
                    + "' ni de sus clases base");
            return;
        }
        Symbol.EventSymbol ev = (Symbol.EventSymbol) s;
        int given = r.args.size(), declared = ev.params.size();
        if (given > declared) {
            err(r.line, r.column, "el evento '" + r.event + "' declara " + declared
                    + " parámetro(s) y el raise pasa " + given);
            return;
        }
        // H8.1 — los que faltan tienen que tener valor por defecto; si no, error
        // AQUÍ, en el disparo, que es donde el programador puede arreglarlo.
        for (int i = given; i < declared; i++) {
            if (ev.params.get(i).defaultExpr == null) {
                err(r.line, r.column, "falta el argumento '" + ev.params.get(i).name
                        + "' del evento '" + r.event + "', y no tiene valor por defecto");
                return;
            }
        }
        // ...y se rellenan AQUÍ, igual que en las llamadas normales, para que el
        // emisor vea siempre una lista completa y ya analizada.
        for (int i = given; i < declared; i++)
            r.args.add(cloneConstExpr(ev.params.get(i).defaultExpr, r.line, r.column));

        for (int i = 0; i < r.args.size(); i++) {
            BpType at = analyzeExpr(r.args.get(i), scope, ev.params.get(i).type);
            BpType pt = ev.params.get(i).type;
            if (at != null && pt != null && !at.display().equals(pt.display())
                    && !pt.isAssignableFrom(at)) {
                err(r.line, r.column, "argumento " + (i + 1) + " del raise de '" + r.event
                        + "': se esperaba '" + pt.display() + "' y es '" + at.display() + "'");
            }
        }
        info.declSymbols.put(r, s);
    }

    private void analyzeAssign(AssignStmt a, Scope scope) {
        // V7/H1 (21-sep) — un owner local no se guarda en un campo ni en un array: escaparía.
        if ((a.target instanceof MemberAccessExpr || a.target instanceof IndexExpr) && esOwnerLocal(a.value, scope))
            err(a.line, a.column, "un `var owner` local no puede escapar a un campo o a un array: se destruye al salir de la función");
        // H5.c — SUSCRIPCIÓN: `algo.onClick := obj::metodo` (o `:= null` para
        // desuscribir). Se resuelve ANTES del camino normal porque ni el evento
        // ni la referencia a método tienen tipo: no hay nada que unificar, hay
        // que comprobar que encajan el uno con el otro.
        if (analyzeEventSubscription(a, scope)) return;
        rejectStrayMethodRef(a.value);

        BpType lhsT = analyzeExpr(a.target, scope, null);
        BpType rhsT = analyzeExpr(a.value, scope, lhsT);

        // target debe ser asignable
        boolean okTarget = a.target instanceof IdentifierExpr
                || a.target instanceof MemberAccessExpr
                || (a.target instanceof IndexExpr && !info.arraysNuevos.contains(a.target))   // V8/H8: `A[n] := x`
                || a.target instanceof FieldExpr;
        if (!okTarget)
            err(a.line, a.column, "el operando izquierdo no es asignable");

        if (a.target instanceof FieldExpr && !insideSetter)
            err(a.line, a.column, "'field' solo puede usarse dentro de get/set");

        // L8 v2 — un array de tamaño fijo de módulo tiene binding FIJA (vive
        // horneado en el data block, no hay slot de ref que reasignar).
        // Sus ELEMENTOS sí son escribibles (buf[i] := x).
        if (a.target instanceof IdentifierExpr) {
            Symbol ts = info.exprSymbols.get(a.target);
            if (ts instanceof VarSymbol && ((VarSymbol) ts).fixedArraySize >= 0)
                err(a.line, a.column, "'" + ((VarSymbol) ts).name + "' es un array de tamaño "
                        + "fijo: no es reasignable (escribe en sus elementos)");
        }

        // L8 v2 — una const no es asignable en ningún ámbito. Además el emisor
        // mivm ya no le materializa slot (el valor viaja inlined), así que un
        // store generaría una referencia a un símbolo inexistente.
        if (a.target instanceof IdentifierExpr || a.target instanceof MemberAccessExpr) {
            Symbol ts = info.exprSymbols.get(a.target);
            if (ts instanceof ConstSymbol)
                err(a.line, a.column, "'" + ts.name + "' es una const: no se puede asignar");
            // 20-sep — asignar a un MÉTODO (`c.m := 5`) lo rechazaba sólo el emisor, sin línea
            // («asignación a member-access no es campo ni property»); aquí, con la línea.
            if (ts instanceof FunctionSymbol)
                err(a.line, a.column, "'" + ts.name + "' es un método, no un campo: no se puede asignar");
        }

        if (a.op != AssignOpKind.ASSIGN) {
            // Anti-cascada (#232): si un operando ya es <error> (p.ej. el target
            // es un identificador no resuelto), el error real ya se reportó
            // aguas arriba. No emitimos un "incompatible" espurio — que además
            // filtraría el tipo interno '<error>' al usuario.
            //
            // V8/H7.1 (#747) — la regla de C# (Eduardo, 31-oct: «Los tipos como en C#»): `x op= e` opera como
            // `x op e`, con su tipo r. Si r se guarda solo en el de x, sin más (`l += 1`, `s += 5`, `d *= 2`). Si no,
            // vale sólo si `e` se guardaría sola en x —o es un desplazamiento—, y entonces el resultado se recorta al
            // tipo de x: es lo que pasa con los pequeños, que operan en integer (`b += 1` con b byte). `x *= 1.5` con
            // x integer es un error, como en C#: no se pierde nada en silencio.
            if (!(lhsT instanceof ErrorType) && !(rhsT instanceof ErrorType)) {
                String op = a.op.binario();
                String visto = a.op.enBp();
                BpType r = tipoBinario(op, visto, lhsT, rhsT, a.line, a.column);
                if (!(r instanceof ErrorType)) {
                    boolean directo = lhsT.isAssignableFrom(r);
                    boolean recortado = !directo && r.isNumeric() && lhsT.isNumeric()
                            && (lhsT.isAssignableFrom(rhsT) || acceptsNarrowFromLiteral(lhsT, rhsT, a.value)
                                || "shl".equals(op) || "shr".equals(op));
                    if (directo || recortado)
                        info.compuestos.put(a, r);
                    else
                        err(a.line, a.column, "'" + visto + "': '" + lhsT.display() + "' " + visto.substring(0, visto.length() - 1)
                                + " '" + rhsT.display() + "' da '" + r.display() + "', que no se guarda solo en '"
                                + lhsT.display() + "' (como en C#: escribe la conversión, x := " + lhsT.display() + "(x "
                                + visto.substring(0, visto.length() - 1) + " …))");
                }
            }
        } else {
            revisaMapNuevo(lhsT, a.value, a.line, a.column);   // V8/H7.2 (#749)
            if (rhsT instanceof NullType && lhsT.isScalar() && !isString(lhsT))   // #774: una variable string, sí
                err(a.line, a.column, "no se puede asignar 'null' a un tipo escalar '" + lhsT.display() + "'");
            else if (!lhsT.isAssignableFrom(rhsT)
                    && !acceptsNarrowFromLiteral(lhsT, rhsT, a.value))
                err(a.line, a.column, narrowAssignErrorMessage(lhsT, rhsT, a.value,
                        "no se puede asignar '" + rhsT.display() + "' a '" + lhsT.display() + "'"));
        }
    }

    private void analyzeIf(IfStmt iff, Scope scope) {
        analyzeBoolCondition(iff.then_.condition, scope);
        analyzeBody(iff.then_.body, new Scope("if-then", scope));
        for (IfClause ei : iff.elseIfs) {
            analyzeBoolCondition(ei.condition, scope);
            analyzeBody(ei.body, new Scope("elseif", scope));
        }
        if (iff.else_ != null)
            analyzeBody(iff.else_, new Scope("else", scope));
    }

    /**
     * Analiza un statement {@code parallel}. Cada rama es azúcar para una
     * subclase anónima de Thread con un run() cuyo cuerpo es {@code br.body}.
     *
     * Semántica de visibilidad en el cuerpo del case (encapsulación de
     * objeto, igual que el método de una clase normal):
     *   - VISIBLE: stdlib (Thread, Mutex, List, StringBuilder, builtins como
     *     print/yield/intToString...), clases del usuario, funciones del
     *     usuario, consts del módulo, locales declarados en el propio cuerpo.
     *   - NO visible: VARIABLES globales del módulo y locales del encerrador.
     *     Esto fuerza que cada thread sea autocontenido y evita race
     *     conditions accidentales por escritura concurrente a globals.
     *   - {@code this} NO está disponible (la rama no declara campos
     *     accesibles al usuario).
     *
     * La variable {@code tI} del case se declara en la scope encerradora
     * como tipo Thread; tras el {@code endpar} ya está joineada y se puede
     * inspeccionar. El bloque {@code default} SÍ se analiza normalmente en
     * una sub-scope del caller (puede ver locales y globals del caller).
     */
    private void analyzeParallel(ParallelStmt p, Scope scope) {
        if (module == null) return;
        Symbol threadSym = scope.resolve("Thread");
        if (!(threadSym instanceof ClassSymbol)) {
            err(p.line, p.column, "'parallel' requiere la clase Thread builtin");
            return;
        }
        ClassSymbol threadCls = (ClassSymbol) threadSym;
        BpType threadType = new ClassType(threadCls);

        // Construye una scope "filtrada del módulo" que esconde los
        // VarSymbol globales pero deja ver el resto (funciones, clases,
        // enums, consts) y, vía parent, todo lo stdlib.
        Scope filteredModule = buildParallelCaseParentScope();

        for (int i = 0; i < p.branches.size(); i++) {
            ParallelBranch br = p.branches.get(i);
            // 1) stackSize debe ser integer.
            BpType stackT = analyzeExpr(br.stackSizeExpr, scope, PrimitiveType.INTEGER);
            if (!PrimitiveType.INTEGER.isAssignableFrom(stackT)) {
                err(br.line, br.column,
                        "stackSize de Thread debe ser integer, no '" + stackT.display() + "'");
            }
            // 2) Cuerpo de la rama → run() de una subclase anónima de Thread.
            //    Lo analizamos con currentFunction temporal de retorno void y
            //    scope padre = filteredModule (sin globals, sin locales del caller).
            FunctionSymbol save = currentFunction;
            FunctionSymbol fakeRun = new FunctionSymbol("run", true, false, false, null, null);
            fakeRun.returnType = VoidType.INSTANCE;
            currentFunction = fakeRun;
            parallelCaseDepth++;
            java.util.ArrayDeque<Object[]> bucles = buclesConNombre;   // V8/H8: la rama es otro hilo: los de fuera no se ven
            buclesConNombre = new java.util.ArrayDeque<>();
            try {
                Scope bodyScope = new Scope("parallel-case-" + br.varName, filteredModule);
                analyzeBody(br.body, bodyScope);
            } finally {
                buclesConNombre = bucles;
                parallelCaseDepth--;
                currentFunction = save;
            }
            // 3) Declarar la variable tI en la scope encerradora.
            VarSymbol v = new VarSymbol(br.varName, false, false, null, true,
                                        br.line, br.column);
            v.type = threadType;
            if (!scope.tryDefine(v)) {
                err(br.line, br.column,
                        "variable duplicada en parallel: '" + br.varName + "'");
            }
            // 4) Nombre único para la clase sintetizada (lo usará el emitter).
            br.synthesizedClassName = "__Par_" + p.line + "_" + p.column + "_" + i;
        }
        // 5) default → block normal en sub-scope del caller (ve TODO lo del caller).
        if (p.defaultBody != null) {
            analyzeBody(p.defaultBody, new Scope("parallel-default", scope));
        }
    }

    /**
     * Construye una scope que actúa como "vista filtrada" del módulo: copia
     * todos los símbolos top-level EXCEPTO {@link VarSymbol}s, y encadena
     * como parent el scope built-in (donde viven stdlib types y builtins).
     * Resultado: las funciones, clases, enums y consts del módulo se ven,
     * pero las variables globales no.
     */
    private Scope buildParallelCaseParentScope() {
        Scope filtered = new Scope("parallel-case-module-filtered", module.members.parent);
        for (Symbol sym : module.members.getSymbols()) {
            if (sym instanceof VarSymbol) continue;   // ban globals
            filtered.tryDefine(sym);
        }
        return filtered;
    }

    private void analyzeSwitch(SwitchStmt sw, Scope scope) {
        BpType subjT = analyzeExpr(sw.subject, scope, null);
        String noValido = sujetoDeSwitchNoValido(subjT);   // #546
        if (noValido != null) {
            Node ns = (Node) sw.subject;
            err(ns.line, ns.column, noValido);
        }
        switchDepth++;
        try {
            Set<String> seen = new HashSet<>();
            for (CaseClause cc : sw.cases) {
                for (IExpr v : cc.values) {
                    BpType t = analyzeExpr(v, scope, subjT);
                    if (noValido == null && esReal(t)) {   // #546: `case 2.5` con un sujeto entero
                        Node nn = (Node) v;
                        err(nn.line, nn.column, "valor de 'case' de tipo '" + t.display()
                                + "': un real no se compara por igualdad exacta; usa if");
                    } else if (!subjT.isAssignableFrom(t) && !t.isAssignableFrom(subjT)) {
                        Node nn = (Node) v;
                        err(nn.line, nn.column, "valor de 'case' tipo '" + t.display() + "' incompatible con switch tipo '" + subjT.display() + "'");
                    }
                    String key = exprKey(v);
                    if (key != null && !seen.add(key)) {
                        Node nn = (Node) v;
                        err(nn.line, nn.column, "valor 'case' duplicado: " + key);
                    }
                }
                analyzeBody(cc.body, new Scope("case", scope));
            }
            if (sw.defaultBody != null)
                analyzeBody(sw.defaultBody, new Scope("default", scope));

            // Exhaustividad sobre enum
            if (sw.defaultBody == null && subjT instanceof EnumType) {
                EnumType et = (EnumType) subjT;
                List<String> missing = et.en.values.keySet().stream()
                        .filter(n -> !seen.contains(et.en.name + "." + n))
                        .collect(Collectors.toList());
                if (!missing.isEmpty())
                    warn(sw.line, sw.column,
                         "switch sobre enum '" + et.en.name + "' no exhaustivo, faltan: " + String.join(", ", missing));
            }
        } finally { switchDepth--; }
    }

    /** #546 (24-sep, decisión de Eduardo) — el sujeto de un switch: un entero (integer, long y los
     *  estrechos), un boolean, un enum o un string. Un real NO —la igualdad exacta de reales es una
     *  trampa— y una referencia tampoco (para comparar objetos está `if`). Devuelve el error, o null si
     *  vale. Antes se admitía todo, y con un sujeto de 8 B no casaba nunca ningún case. */
    private static String sujetoDeSwitchNoValido(BpType t) {
        if (t == null || t instanceof ErrorType || t.isEnum()) return null;
        if (t instanceof PrimitiveType) {
            if (esReal(t))
                return "switch sobre '" + t.display() + "': un real no se compara por igualdad exacta "
                     + "(0.1 + 0.2 no es 0.3); usa if, con un margen si hace falta";
            return null;
        }
        return "switch sobre '" + t.display() + "': el sujeto de un switch es un entero, un boolean, "
             + "un enum o un string; para comparar referencias, usa if";
    }

    /** #546 — float o double. */
    private static boolean esReal(BpType t) {
        return t instanceof PrimitiveType && (((PrimitiveType) t).tag == PrimitiveType.Kind.FLOAT
                                           || ((PrimitiveType) t).tag == PrimitiveType.Kind.DOUBLE);
    }

    private String exprKey(IExpr e) {
        if (e instanceof IntLitExpr)    return "int:"   + ((IntLitExpr) e).value;
        if (e instanceof LongLitExpr)   return "long:"  + ((LongLitExpr) e).value;
        if (e instanceof DoubleLitExpr) return "double:" + ((DoubleLitExpr) e).value;
        if (e instanceof FloatLitExpr)  return "float:" + ((FloatLitExpr) e).value;
        if (e instanceof StringLitExpr) return "str:"   + ((StringLitExpr) e).value;
        if (e instanceof BoolLitExpr)   return "bool:"  + ((BoolLitExpr) e).value;
        if (e instanceof MemberAccessExpr) {
            MemberAccessExpr m = (MemberAccessExpr) e;
            if (m.target instanceof IdentifierExpr)
                return ((IdentifierExpr) m.target).name + "." + m.member;
        }
        if (e instanceof IdentifierExpr) return ((IdentifierExpr) e).name;
        return null;
    }

    private void analyzeWhile(WhileStmt w, Scope scope) {
        analyzeBoolCondition(w.condition, scope);
        loopDepth++;
        try { analyzeBody(w.body, new Scope("while", scope)); }
        finally { loopDepth--; }
    }

    private void analyzeDoLoop(DoLoopStmt dl, Scope scope) {
        loopDepth++;
        if (dl.nombre != null) {
            for (Object[] b : buclesConNombre)
                if (dl.nombre.equals(b[0])) err(dl.line, dl.column, "ya hay un bucle '" + dl.nombre + "' abierto: el de dentro lo taparía");
            buclesConNombre.push(new Object[] { dl.nombre, loopDepth });
        }
        try {
            analyzeBody(dl.body, new Scope("do", scope));
            if (dl.condition != null) analyzeBoolCondition(dl.condition, scope);
        } finally {
            if (dl.nombre != null) buclesConNombre.pop();
            loopDepth--;
        }
    }

    /** V8/H8 (Eduardo, 4-nov: «poder hacer un break a un loop concreto») — `break miLoop` o `continue miLoop`: tiene que ser
     *  un `do miLoop` abierto, y como el `break` sin nombre, no puede salir de un `finally` (#674). */
    private void saltoConNombre(String que, String nombre, int line, int column) {
        for (Object[] b : buclesConNombre) {
            if (!nombre.equals(b[0])) continue;
            if (!finallysAbiertos.isEmpty() && (Integer) b[1] <= finallysAbiertos.peek()[0])
                err(line, column, "un 'finally' no puede salir con '" + que + "': taparia la excepcion que fuera en vuelo (#674)");
            return;
        }
        err(line, column, "'" + que + " " + nombre + "': no hay ningún bucle 'do " + nombre + "' abierto");
    }

    private void analyzeFor(ForStmt f, Scope scope) {
        Scope inner = new Scope("for", scope);
        BpType iterT;
        if (f.range instanceof ForNumericRange) {
            ForNumericRange nr = (ForNumericRange) f.range;
            BpType fromT = analyzeExpr(nr.from, scope, PrimitiveType.INTEGER);
            BpType toT   = analyzeExpr(nr.to,   scope, PrimitiveType.INTEGER);
            if (nr.step != null) {
                BpType stepT = analyzeExpr(nr.step, scope, PrimitiveType.INTEGER);
                // #506 — el step es integer (constante con signo o expresión); 0 escrito no
                // avanza nunca y se rechaza aquí.
                if (!(stepT instanceof ErrorType) && !PrimitiveType.INTEGER.isAssignableFrom(stepT))
                    err(f.line, f.column, "el 'step' del 'for' debe ser integer, no '" + stepT.display() + "'");
                Long k = MivmEmitter.constanteEntera(nr.step);
                if (k != null && k == 0) err(f.line, f.column, "el 'step' del 'for' no puede ser 0 (el bucle no avanzaría nunca)");
            }
            if (!PrimitiveType.INTEGER.isAssignableFrom(fromT)
                    || !PrimitiveType.INTEGER.isAssignableFrom(toT))
                err(f.line, f.column, "los límites del 'for' numérico deben ser integer");
            iterT = PrimitiveType.INTEGER;
        } else if (f.range instanceof ForInRange) {
            ForInRange inr = (ForInRange) f.range;
            arrayFijoPermitido.add(inr.iterable);   // V7/H1 #19: recorrer un array fijo sí (es indexarlo)
            BpType collT = analyzeExpr(inr.iterable, scope, null);
            if (collT instanceof ArrayType) iterT = ((ArrayType) collT).element;
            else if (collT instanceof ClassType && esLista(((ClassType) collT).cls)) {
                // V8/H7.2 (#749) — una List (List<Object>): x es cada elemento, un Object.
                iterT = resolveNamedType(new SimpleTypeRef("Object", f.line, f.column));
                info.forDeListas.add(f);
            } else if (collT instanceof ErrorType) {
                iterT = ErrorType.INSTANCE;
            } else {
                err(f.line, f.column, "se esperaba un array o una List en 'for in', encontrado '" + collT.display() + "'");
                iterT = ErrorType.INSTANCE;
            }
        } else {
            iterT = ErrorType.INSTANCE;
        }
        VarSymbol iv = new VarSymbol(f.iteratorName, false, false, null, true, f.line, f.column);
        iv.type = iterT;
        inner.tryDefine(iv);
        loopDepth++;
        try { analyzeBody(f.body, inner); }
        finally { loopDepth--; }
    }

    private void analyzeTry(TryStmt tr, Scope scope) {
        if (tr.recurso != null) analyzeLocalVarDecl(tr.recurso, scope);   // V7/H1: el recurso, un owner local del ámbito de fuera
        analyzeBody(tr.body, new Scope("try", scope));
        boolean seenCatchAll = false;
        for (CatchClause cl : tr.catches) {
            if (seenCatchAll)
                err(cl.line, cl.column, "catch inalcanzable: hay un 'catch' sin tipo previo");
            if (cl.exceptionType == null) seenCatchAll = true;
            Scope inner = new Scope("catch", scope);
            if (cl.varName != null) {
                BpType excT = null;
                if (cl.exceptionType != null) {
                    ClassSymbol csym = resolveCatchClass(cl.exceptionType);
                    if (csym != null) {
                        // #248 — toda excepción desciende de Exception (Core).
                        if (!descendsFromException(csym)) {
                            err(cl.line, cl.column, "la clase de un catch debe descender de "
                                + "Exception: '" + cl.exceptionType + "' no lo hace (#248)");
                        }
                        excT = new ClassType(csym);
                        info.catchClassSymbols.put(cl, csym);   // BUG-2: el emisor decide local vs ext
                    } else {
                        err(cl.line, cl.column, "tipo de excepción '" + cl.exceptionType + "' no es una clase");
                        excT = ErrorType.INSTANCE;
                    }
                }
                VarSymbol cv = new VarSymbol(cl.varName, false, false, null, true, cl.line, cl.column);
                cv.type = excT;
                inner.tryDefine(cv);
            }
            analyzeBody(cl.body, inner);
        }
        if (tr.finallyBody != null) {
            finallysAbiertos.push(new int[] { loopDepth, switchDepth });   // V7 #674
            try {
                analyzeBody(tr.finallyBody, new Scope("finally", scope));
            } finally {
                finallysAbiertos.pop();
            }
        }
    }

    /** #248 — true si la clase desciende de Exception (por NOMBRE en la cadena
     *  de bases, que con Core es la clase única; una clase local llamada
     *  Exception del usuario también vale — sombra deliberada). */
    private static boolean descendsFromException(ClassSymbol cls) {
        ClassSymbol c = cls;
        int guard = 0;
        while (c != null && guard++ < 64) {
            if ("Exception".equals(c.name)) return true;
            c = c.baseClass;
        }
        return false;
    }

    /** BUG-2 — resuelve el tipo de un catch: local ('MyError'), importado
     *  ('Mod.MyError') o auto-cualificado ('Mod.MyError' dentro del propio Mod).
     *  Devuelve el ClassSymbol, o null si no resuelve a una clase. */
    private ClassSymbol resolveCatchClass(String name) {
        int dot = name.indexOf('.');
        if (dot >= 0) {
            String alias  = name.substring(0, dot);
            String simple = name.substring(dot + 1);
            if (module != null && alias.equals(module.name)) {
                Symbol local = module.members.resolve(simple);
                return (local instanceof ClassSymbol) ? (ClassSymbol) local : null;
            }
            Symbol nsSym = module.members.resolve(alias);
            if (nsSym instanceof Symbol.ImportedNamespaceSymbol) {
                Symbol cand = ((Symbol.ImportedNamespaceSymbol) nsSym).classes.get(simple);
                return (cand instanceof ClassSymbol) ? (ClassSymbol) cand : null;
            }
            return null;
        }
        Symbol sym = module.members.resolve(name);
        return (sym instanceof ClassSymbol) ? (ClassSymbol) sym : null;
    }

    /** V7/H1 (21-sep) — ¿es `e` un `var owner` LOCAL? (V5_IDEAS: «owner NO puede escapar»: al
     *  salir del ámbito se destruye y se libera, así que devolverlo o guardarlo en un campo
     *  dejaría una referencia a un objeto muerto.) */
    private boolean esOwnerLocal(IExpr e, Scope scope) {
        if (!(e instanceof IdentifierExpr)) return false;
        Symbol s = scope.resolve(((IdentifierExpr) e).name);
        return s instanceof VarSymbol && ((VarSymbol) s).isOwner && ((VarSymbol) s).isLocal;
    }
    private void analyzeReturn(ReturnStmt r, Scope scope) {
        if (currentFunction == null) { err(r.line, r.column, "'return' fuera de función"); return; }
        if (r.value != null && esOwnerLocal(r.value, scope))
            err(r.line, r.column, "un `var owner` local no puede escapar: se destruye al salir de la función (devuelve una copia, o no lo declares owner)");
        if (currentFunction.isConstructor && r.value != null)
            err(r.line, r.column, "el constructor no puede retornar un valor");
        if (r.value == null) {
            if (currentFunction.returnType != null && !currentFunction.isConstructor)
                err(r.line, r.column, "falta el valor de retorno (tipo '" + currentFunction.returnType.display() + "')");
            return;
        }
        // Tupla de retorno: `return (e1, e2, ...)`. Chequeo por elementos con
        // coerción (int→long, etc.), no por sameAs estricto.
        if (r.value instanceof Ast.TupleExpr) {
            Ast.TupleExpr te = (Ast.TupleExpr) r.value;
            if (!(currentFunction.returnType instanceof BpType.TupleType)) {
                err(r.line, r.column, "esta función no devuelve una tupla; no uses '(a, b)' en return");
                for (IExpr el : te.elements) analyzeExpr(el, scope, null);
                return;
            }
            BpType.TupleType tt = (BpType.TupleType) currentFunction.returnType;
            if (te.elements.size() != tt.elements.size())
                err(r.line, r.column, "la tupla de retorno tiene " + te.elements.size()
                        + " elementos; se esperaban " + tt.elements.size());
            int n = Math.min(te.elements.size(), tt.elements.size());
            for (int i = 0; i < n; i++) {
                BpType et = analyzeExpr(te.elements.get(i), scope, tt.elements.get(i));
                if (!tt.elements.get(i).isAssignableFrom(et) && !(et instanceof ErrorType))
                    err(r.line, r.column, "elemento " + (i + 1) + " de la tupla: '" + et.display()
                            + "' incompatible con '" + tt.elements.get(i).display() + "'");
            }
            info.exprTypes.put(te, tt);
            return;
        }
        // H8.2 — tupla de retorno NO-literal: `return t` (var de tipo tupla) o
        // `return f()` (llamada que devuelve tupla). Permitido si el tipo del valor
        // es una tupla asignable al retorno declarado (la tupla es first-class).
        if (currentFunction.returnType instanceof BpType.TupleType) {
            BpType t = analyzeExpr(r.value, scope, currentFunction.returnType);
            if (!currentFunction.returnType.isAssignableFrom(t) && !(t instanceof ErrorType))
                err(r.line, r.column, "return tipo '" + t.display() + "' incompatible con declarado '"
                        + currentFunction.returnType.display() + "'");
            return;
        }
        BpType t = analyzeExpr(r.value, scope, currentFunction.returnType);
        if (currentFunction.returnType != null && !currentFunction.returnType.isAssignableFrom(t)) {
            err(r.line, r.column, "return tipo '" + t.display() + "' incompatible con declarado '" + currentFunction.returnType.display() + "'");
        } else if (currentFunction.returnType == null && !currentFunction.isConstructor) {
            // N3: hasta hoy un `return value` en función void se aceptaba
            //     silenciosamente. Lo destapa el fix de B2 (analyzeProperty
            //     ahora setea currentFunction al fake del setter, así que
            //     el `return 99` de un setter pasaba por aquí sin queja).
            err(r.line, r.column, "esta función no declara tipo de retorno; no puede devolver un valor");
        }
    }

    private static boolean isUnderscore(IExpr e) {
        return e instanceof IdentifierExpr && "_".equals(((IdentifierExpr) e).name);
    }

    private void analyzeDestructAssign(Ast.DestructAssignStmt s, Scope scope) {
        // El RHS debe ser una llamada que devuelve una tupla.
        BpType rt = analyzeExpr(s.value, scope, null);
        if (!(rt instanceof BpType.TupleType)) {
            if (!(rt instanceof ErrorType))
                err(s.line, s.column, "el lado derecho de '{ ... } :=' debe ser una llamada que "
                        + "devuelve una tupla (tipo actual: '" + rt.display() + "')");
            for (IExpr tg : s.targets) if (!isUnderscore(tg)) analyzeExpr(tg, scope, null);
            return;
        }
        BpType.TupleType tt = (BpType.TupleType) rt;
        if (s.targets.size() != tt.elements.size())
            err(s.line, s.column, "el desempaquetado tiene " + s.targets.size()
                    + " variables; la tupla tiene " + tt.elements.size() + " elementos");
        int n = Math.min(s.targets.size(), tt.elements.size());
        for (int i = 0; i < n; i++) {
            IExpr tg = s.targets.get(i);
            if (isUnderscore(tg)) continue;   // '_' = descartar
            BpType targetT = analyzeExpr(tg, scope, tt.elements.get(i));
            if (!targetT.isAssignableFrom(tt.elements.get(i)) && !(targetT instanceof ErrorType))
                err(s.line, s.column, "variable " + (i + 1) + " del desempaquetado: no se puede "
                        + "asignar '" + tt.elements.get(i).display() + "' a '" + targetT.display() + "'");
        }
    }

    private void analyzeBoolCondition(IExpr e, Scope scope) {
        BpType t = analyzeExpr(e, scope, PrimitiveType.BOOLEAN);
        if (!PrimitiveType.BOOLEAN.isAssignableFrom(t) && !(t instanceof ErrorType)) {
            Node n = (Node) e;
            err(n.line, n.column, "se esperaba boolean, encontrado '" + t.display() + "'");
        }
    }

    // ============================================================
    // EXPRESIONES — devuelven su tipo
    // ============================================================
    private BpType analyzeExpr(IExpr e, Scope scope, BpType expected) {
        BpType t;
        if      (e instanceof IntLitExpr)    t = PrimitiveType.INTEGER;
        else if (e instanceof LongLitExpr)   t = PrimitiveType.LONG;   // H1.2 (V2)
        else if (e instanceof DoubleLitExpr) t = PrimitiveType.DOUBLE; // H1.3 (V2)
        else if (e instanceof FloatLitExpr)  t = PrimitiveType.FLOAT;
        else if (e instanceof StringLitExpr) t = PrimitiveType.STRING;
        else if (e instanceof BoolLitExpr)   t = PrimitiveType.BOOLEAN;
        else if (e instanceof NullLitExpr)   t = NullType.INSTANCE;
        else if (e instanceof ThisExpr)      t = analyzeThis((ThisExpr) e);
        else if (e instanceof SuperExpr)     t = analyzeSuperRef((SuperExpr) e);
        else if (e instanceof SuperCallExpr) t = analyzeSuperCall((SuperCallExpr) e, scope);
        else if (e instanceof FieldExpr)     t = analyzeField((FieldExpr) e);
        else if (e instanceof ParenExpr)     t = analyzeExpr(((ParenExpr) e).inner, scope, expected);
        else if (e instanceof ArrayLitExpr)  t = analyzeArrayLit((ArrayLitExpr) e, scope, expected);
        else if (e instanceof IdentifierExpr) t = analyzeIdentifier((IdentifierExpr) e, scope);
        else if (e instanceof MemberAccessExpr) t = analyzeMemberAccess((MemberAccessExpr) e, scope);
        else if (e instanceof IndexExpr)     t = analyzeIndex((IndexExpr) e, scope);
        else if (e instanceof CallExpr && esTernario((CallExpr) e))
            t = analyzeTernario((CallExpr) e, scope, expected);   // V8/H7.1 (#747): iif(c, a, b), c ? a : b en JC
        else if (e instanceof CallExpr) {
            t = analyzeCall((CallExpr) e, scope);
            if (enJc) revisaJc((CallExpr) e, t);
            revisaLlaveDeMap((CallExpr) e);   // V8/H7.2 (#749)
        }
        else if (e instanceof UnaryExpr)     t = analyzeUnary((UnaryExpr) e, scope);
        else if (e instanceof BinaryExpr)    t = analyzeBinary((BinaryExpr) e, scope);
        else if (e instanceof InstanceOfExpr) t = analyzeInstanceOf((InstanceOfExpr) e, scope);
        else if (e instanceof Ast.TupleExpr) {
            // Una tupla literal solo es válida como valor directo de `return`
            // (lo maneja analyzeReturn). Aquí = uso indebido.
            Ast.TupleExpr te = (Ast.TupleExpr) e;
            for (IExpr el : te.elements) analyzeExpr(el, scope, null);
            err(((Node) e).line, ((Node) e).column,
                "una tupla '(a, b)' solo puede aparecer como valor de 'return'");
            t = ErrorType.INSTANCE;
        }
        else if (e instanceof Ast.BoundCallExpr) {
            // #325 — una llamada preparada sólo vale dentro de `Thread(...)`, que
            // la intercepta en analyzeCall. Si llega hasta aquí es que está en
            // cualquier otro sitio, y no es un valor que se pueda guardar.
            Ast.BoundCallExpr bc = (Ast.BoundCallExpr) e;
            analyzeExpr(bc.target, scope, null);
            for (IExpr a : bc.args) analyzeExpr(a, scope, null);
            err(bc.line, bc.column, "'obj::metodo(args)' es una llamada preparada, no un valor:"
                + " en V4 sólo vale dentro de 'Thread(...)' para decirle al thread qué ejecutar");
            t = ErrorType.INSTANCE;
        }
        else                                  t = ErrorType.INSTANCE;
        info.exprTypes.put(e, t);
        return t;
    }

    private BpType analyzeThis(ThisExpr th) {
        if (currentClass == null || currentFunction == null || currentFunction.isStatic) {
            err(th.line, th.column, "'this' solo es válido en métodos de instancia");
            return ErrorType.INSTANCE;
        }
        return new ClassType(currentClass);
    }

    private BpType analyzeSuperRef(SuperExpr sup) {
        if (currentClass == null || currentClass.baseClass == null) {
            err(sup.line, sup.column, "'super' solo es válido en una subclase");
            return ErrorType.INSTANCE;
        }
        if (currentFunction == null || currentFunction.isStatic) {
            err(sup.line, sup.column, "'super' solo es válido en métodos de instancia");
            return ErrorType.INSTANCE;
        }
        return new ClassType(currentClass.baseClass);
    }

    /** H1 (20-sep) — una línea `inline`: el mnemónico existe (OpCode), los operandos son los
     *  que ese opcode lleva (hoy: ninguno, o UNO de i32/u16/u8 — lo que necesita la stdlib),
     *  y sólo dentro de una función cuyo cuerpo sea TODO inline (una intrínseca). */
    private void analyzeInline(InlineStmt in) {
        if (currentFunction == null || !currentFunction.isIntrinsic) {
            err(in.line, in.column, "'inline' sólo puede ir en una función cuyo cuerpo sea todo líneas 'inline' (una intrínseca)");
            return;
        }
        edu.bpgenvm.bytecode.OpCode op;
        try { op = edu.bpgenvm.bytecode.OpCode.valueOf(in.mnemonic); }
        catch (IllegalArgumentException e) { err(in.line, in.column, "'" + in.mnemonic + "' no es un opcode (ver docs/OPCODES.md)"); return; }
        int esperados;
        switch (op.operandKind) {
            case NONE: esperados = 0; break;
            case IMM_I32: case IDX_U16: case IMM_U8: esperados = 1; break;
            default:
                err(in.line, in.column, "'inline " + in.mnemonic + "': el compilador no sabe escribir todavía los operandos de ese opcode (" + op.operandKind + ")");
                return;
        }
        if (in.operands.size() != esperados)
            err(in.line, in.column, "'inline " + in.mnemonic + "' lleva " + esperados + " operando" + (esperados == 1 ? "" : "s") + ", no " + in.operands.size());
        else if (op.operandKind == edu.bpgenvm.bytecode.OpCode.OperandKind.IDX_U16 && (in.operands.get(0) < 0 || in.operands.get(0) > 0xFFFF))
            err(in.line, in.column, "'inline " + in.mnemonic + "': el operando tiene que estar entre 0 y 65535");
        else if (op.operandKind == edu.bpgenvm.bytecode.OpCode.OperandKind.IMM_U8 && (in.operands.get(0) < 0 || in.operands.get(0) > 0xFF))
            err(in.line, in.column, "'inline " + in.mnemonic + "': el operando tiene que estar entre 0 y 255");
    }

    private BpType analyzeSuperCall(SuperCallExpr sc, Scope scope) {
        if (currentFunction == null || !currentFunction.isConstructor) {
            err(sc.line, sc.column, "'super(...)' solo es válido dentro de un constructor");
            return ErrorType.INSTANCE;
        }
        if (!isFirstStmtOfCtor)
            err(sc.line, sc.column, "'super(...)' debe ser la primera sentencia del constructor");
        if (currentClass == null || currentClass.baseClass == null) {
            err(sc.line, sc.column, "la clase no tiene clase base");
            return ErrorType.INSTANCE;
        }
        seenSuperCallInCtor = true;
        FunctionSymbol baseCtor = currentClass.baseClass.findConstructor();
        if (baseCtor == null) {
            if (!sc.args.isEmpty())
                err(sc.line, sc.column, "la clase base '" + currentClass.baseClass.name + "' no define constructor; super() no puede llevar argumentos");
        } else if (baseCtor.overloaded) {
            // V7/H2 (#509) — `super(5)` con constructor sobrecargado sólo veía la
            // cabeza («se esperaban 0, se pasaron 1»). Misma resolución por firma que
            // `Base(5)` (H5.a-E4), anotada en el nodo para que el emisor llame al
            // `__init` de la sobrecarga elegida.
            List<FunctionSymbol> cands = new ArrayList<>();
            for (FunctionSymbol c = baseCtor; c != null; c = c.nextOverload) cands.add(c);
            FunctionSymbol chosen = resolveOverloadArgs(cands, baseCtor.name, sc.args, sc.line, sc.column, scope);
            if (chosen == null) return ErrorType.INSTANCE;
            info.exprSymbols.put(sc, chosen);
            checkVisibilidadCtor(sc.line, sc.column, chosen, currentClass.baseClass);   // #515
        } else {
            checkArgs(sc.line, sc.column, baseCtor.params, sc.args, scope);
            checkVisibilidadCtor(sc.line, sc.column, baseCtor, currentClass.baseClass);   // #515
        }
        return VoidType.INSTANCE;
    }

    private BpType analyzeField(FieldExpr fe) {
        if (!insideGetter && !insideSetter) {
            err(fe.line, fe.column, "'field' solo es válido dentro de get/set");
            return ErrorType.INSTANCE;
        }
        // #547 (24-sep) — `field` es el backing field: tiene el tipo de su property. Era ErrorType,
        // que por el anti-cascada (#232) lo admite TODO: `field := "hola"` en una integer compilaba,
        // y el emisor, que convierte al tipo del destino, no convertía nada — `field := 7` en una
        // property long apilaba 4 B para un store de 8 (la pila, descuadrada).
        return currentPropType != null ? currentPropType : ErrorType.INSTANCE;
    }

    private BpType analyzeArrayLit(ArrayLitExpr al, Scope scope, BpType expected) {
        BpType elemHint = (expected instanceof ArrayType) ? ((ArrayType) expected).element : null;
        BpType inferred = null;
        for (IExpr e : al.elements) {
            BpType t = analyzeExpr(e, scope, elemHint);
            if (inferred == null) inferred = t;
            else if (!inferred.sameAs(t)) {
                if (inferred.isAssignableFrom(t)) {
                    /* OK */
                } else if (t.isAssignableFrom(inferred)) {
                    inferred = t;
                } else {
                    err(al.line, al.column, "elementos del array de tipos incompatibles: '" + inferred.display() + "' y '" + t.display() + "'");
                    inferred = ErrorType.INSTANCE;
                }
            }
        }
        BpType elemFinal = (inferred != null) ? inferred : (elemHint != null ? elemHint : ErrorType.INSTANCE);
        return new ArrayType(elemFinal);
    }

    /** V7/H1 #19 — las expresiones en las que un array fijo PUEDE aparecer: el objetivo de un
     *  `[i]` y el argumento de `len(…)`. En cualquier otra —asignarlo, pasarlo, devolverlo,
     *  imprimirlo— es error: no es una referencia, es almacenamiento en sitio. */
    private final java.util.Set<IExpr> arrayFijoPermitido = java.util.Collections.newSetFromMap(new java.util.IdentityHashMap<IExpr, Boolean>());
    private void exigirUsoDeArrayFijo(IExpr e, VarSymbol vs, int line, int col) {
        if (vs.fixedArraySize < 0 || arrayFijoPermitido.contains(e)) return;
        err(line, col, "'" + vs.name + "' es un array fijo: no es una referencia, sólo se indexa ('" + vs.name + "[i]') o se mide ('len(" + vs.name + ")')");
    }
    /** El ancho en bytes de un elemento primitivo (para las ranuras de un array fijo de clase). */
    static int anchoDeElemento(BpType t) {
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case INT8: case UINT8: return 1;
                case INT16: case UINT16: return 2;
                case LONG: case DOUBLE: return 8;
                default: return 4;
            }
        }
        return 4;
    }
    private BpType analyzeIdentifier(IdentifierExpr id, Scope scope) {
        Symbol sym = (scope != null) ? scope.resolve(id.name) : null;

        // Caso especial: si el identificador coincide con el nombre de la
        // clase actual queremos referenciar la CLASE (no el constructor,
        // que es un miembro de instancia con el mismo nombre).
        if (sym == null && currentClass != null && id.name.equals(currentClass.name)) {
            Symbol clsSym = module.members.resolve(id.name);
            if (clsSym instanceof ClassSymbol) sym = clsSym;
        }

        if (sym == null && currentClass != null && currentFunction != null && !currentFunction.isStatic) {
            sym = currentClass.lookupInstance(id.name);
            // #510 (V7) — el CONSTRUCTOR de una clase base vive en sus instanceMembers
            // con el nombre de la clase: dentro de `Der extends Base`, `Base.MAX` o
            // `Base.cuantos()` resolvía `Base` al ctor (tipo void → «el tipo 'void' no
            // tiene miembros»). Un identificador pelado nunca nombra un constructor
            // (`Base(...)` va por analyzeCall y `super(...)` es SuperExpr): se descarta
            // y se cae a module.members, donde está la clase. Es el caso especial de
            // arriba (la clase ACTUAL) generalizado a cualquier ancestro.
            if (sym instanceof FunctionSymbol && ((FunctionSymbol) sym).isConstructor) sym = null;
        }
        if (sym == null) {
            Symbol moduleSym = module.members.resolve(id.name);
            // Gating: dentro de una clase, los símbolos de "estado" del módulo
            // (functions, vars, consts, properties) sólo son accesibles si son
            // public. Tipos (clases, enums) y namespaces de imports SIEMPRE
            // accesibles. Builtins también (no viven en module.members directo,
            // sólo en su parent scope).
            if (moduleSym != null && currentClass != null
                    && module.members.tryLookup(id.name) == moduleSym
                    && !classCanAccessModuleSymbol(moduleSym)) {
                err(id.line, id.column,
                    "la clase '" + currentClass.name + "' no puede acceder al símbolo '"
                    + id.name + "' del módulo (no es public)");
                return ErrorType.INSTANCE;
            }
            // Encapsulación de los cuerpos de `case` dentro de un `parallel`:
            // un thread no puede leer/escribir variables globales (sí consts,
            // funciones, clases). Forzamos al usuario a coordinar via objetos
            // compartidos y mutex en vez de globals mutables.
            if (parallelCaseDepth > 0 && moduleSym instanceof VarSymbol
                    && module.members.tryLookup(id.name) == moduleSym) {
                err(id.line, id.column,
                    "el cuerpo de un 'case' dentro de 'parallel' no puede acceder a la variable global '"
                    + id.name + "'; usa una const, una función o un objeto compartido");
                return ErrorType.INSTANCE;
            }
            sym = moduleSym;
        }
        if (sym == null) {
            // Anti-cascada: reportar cada nombre no resuelto UNA vez.
            if (reportedUnresolved.add(id.name))
                err(id.line, id.column, "identificador no resuelto: '" + id.name + "'" + pistaCore(id.name));
            return ErrorType.INSTANCE;
        }
        info.exprSymbols.put(id, sym);
        if (sym instanceof Symbol.EventSymbol) return eventoComoValor(id, (Symbol.EventSymbol) sym);   // V8 #771
        if (sym instanceof VarSymbol) exigirUsoDeArrayFijo(id, (VarSymbol) sym, id.line, id.column);   // V7/H1 #19
        return typeOfSymbol(sym);
    }

    /**
     * Política de encapsulación módulo ↔ clase. Una clase declarada dentro de
     * un módulo sólo puede ver los símbolos públicos del módulo (functions,
     * properties, vars, consts). Tipos (clases, enums) y namespaces son
     * visibles siempre — son nombres de tipo, no estado del módulo.
     */
    private static boolean classCanAccessModuleSymbol(Symbol sym) {
        if (sym instanceof ClassSymbol)              return true;
        if (sym instanceof EnumSymbol)               return true;
        if (sym instanceof Symbol.ImportedNamespaceSymbol) return true;
        if (sym instanceof FunctionSymbol)           return ((FunctionSymbol) sym).isPublic;
        if (sym instanceof PropertySymbol)           return ((PropertySymbol) sym).isPublic;
        if (sym instanceof VarSymbol)                return ((VarSymbol) sym).isPublic;
        if (sym instanceof ConstSymbol)              return ((ConstSymbol) sym).isPublic;
        return true;
    }

    private BpType analyzeMemberAccess(MemberAccessExpr ma, Scope scope) {
        BpType tgtT = analyzeExpr(ma.target, scope, null);

        // Acceso estático: ClassName.member o EnumName.MEMBER o ImportedModule.func
        if (ma.target instanceof IdentifierExpr) {
            IdentifierExpr id = (IdentifierExpr) ma.target;
            Symbol idSym = info.exprSymbols.get(id);
            if (idSym instanceof Symbol.ImportedNamespaceSymbol) {
                Symbol.ImportedNamespaceSymbol ns = (Symbol.ImportedNamespaceSymbol) idSym;
                // Resolución por orden: class > enum > const > property > function.
                Symbol.ClassSymbol clsSym = ns.classes.get(ma.member);
                if (clsSym != null) {
                    info.exprSymbols.put(ma, clsSym);
                    // Como tipo: ClassType apuntando al stub externo.
                    return new ClassType(clsSym);
                }
                EnumSymbol enumSym = ns.enums.get(ma.member);
                if (enumSym != null) {
                    info.exprSymbols.put(ma, enumSym);
                    return new EnumType(enumSym);
                }
                ConstSymbol constSym = ns.consts.get(ma.member);
                if (constSym != null) {
                    info.exprSymbols.put(ma, constSym);
                    return typeOfSymbol(constSym);
                }
                PropertySymbol propSym = ns.properties.get(ma.member);
                if (propSym != null) {
                    info.exprSymbols.put(ma, propSym);
                    return typeOfSymbol(propSym);
                }
                FunctionSymbol fn = ns.functions.get(ma.member);
                if (fn != null) {
                    info.exprSymbols.put(ma, fn);
                    return typeOfSymbol(fn);
                }
                err(ma.line, ma.column, "el módulo importado '" + ns.name +
                        "' no expone '" + ma.member + "'");
                return ErrorType.INSTANCE;
            }
            if (idSym instanceof ClassSymbol) {
                ClassSymbol cs = (ClassSymbol) idSym;
                Symbol sub = cs.lookupStatic(ma.member);
                if (sub == null) {
                    err(ma.line, ma.column, "'" + cs.name + "' no tiene miembro estático '" + ma.member + "'");
                    return ErrorType.INSTANCE;
                }
                checkVisibility(ma.line, ma.column, sub, cs);
                info.exprSymbols.put(ma, sub);
                return typeOfSymbol(sub);
            }
            if (idSym instanceof EnumSymbol) {
                EnumSymbol es = (EnumSymbol) idSym;
                if (!es.values.containsKey(ma.member)) {
                    err(ma.line, ma.column, "el enum '" + es.name + "' no tiene valor '" + ma.member + "'");
                    return ErrorType.INSTANCE;
                }
                return new EnumType(es);
            }
        }

        // Acceso a valor de enum cuando el target NO es identifier directo
        // (caso típico: `Ns.Enum.MEMBER`, donde `Ns.Enum` ya fue resuelto a
        // EnumSymbol y queda anotado en exprSymbols).
        if (tgtT instanceof EnumType) {
            Symbol tgtSym = info.exprSymbols.get(ma.target);
            if (tgtSym instanceof EnumSymbol) {
                EnumSymbol es = (EnumSymbol) tgtSym;
                if (!es.values.containsKey(ma.member)) {
                    err(ma.line, ma.column, "el enum '" + es.name + "' no tiene valor '" + ma.member + "'");
                    return ErrorType.INSTANCE;
                }
                return new EnumType(es);
            }
        }

        // L2 v3.d — Acceso STÁTICO cuando el target NO es identifier directo
        // (caso típico: `Lib.Cls.STATIC_MEMBER`, donde `Lib.Cls` ya fue
        // resuelto a ClassSymbol y queda anotado en exprSymbols). Si el target
        // es un ClassType pero el exprSymbol del target es directamente un
        // ClassSymbol (no una instancia), buscamos en staticMembers primero.
        if (tgtT instanceof ClassType) {
            Symbol tgtSym = info.exprSymbols.get(ma.target);
            if (tgtSym instanceof ClassSymbol) {
                ClassSymbol cs = (ClassSymbol) tgtSym;
                Symbol staticSub = cs.lookupStatic(ma.member);
                if (staticSub != null) {
                    checkVisibility(ma.line, ma.column, staticSub, cs);
                    info.exprSymbols.put(ma, staticSub);
                    return typeOfSymbol(staticSub);
                }
                // Si no hay static, sigue con acceso de instancia para no
                // romper ningún uso existente — en práctica con un target tipo
                // ClassType (no una instancia concreta) es ambiguo, pero
                // dejamos el path de instancia por compat.
            }
        }

        // Acceso de instancia
        if (tgtT instanceof ClassType) {
            ClassType ct = (ClassType) tgtT;
            Symbol sub = ct.cls.lookupInstance(ma.member);
            if (sub == null) {
                err(ma.line, ma.column, "'" + ct.cls.name + "' no tiene miembro de instancia '" + ma.member + "'");
                return ErrorType.INSTANCE;
            }
            checkVisibility(ma.line, ma.column, sub, ct.cls, !(ma.target instanceof SuperExpr));   // #760: super.m, sobre this
            if (sub instanceof VarSymbol) exigirUsoDeArrayFijo(ma, (VarSymbol) sub, ma.line, ma.column);   // V7/H1 #19
            // V7/H1: ni `x.__dtor()` ni `super.__dtor()`. V7 #668: salvo en el módulo raíz —Core.__destruye es quien lo
            // llama cuando el lenguaje libera un objeto (su owner, el GC o el final del programa)—.
            if (Symbol.FunctionSymbol.DTOR.equals(ma.member) && !esRaiz())
                err(ma.line, ma.column, "el destructor lo llama el lenguaje (al liberar el objeto), no se llama desde el código");
            info.exprSymbols.put(ma, sub);
            if (sub instanceof Symbol.EventSymbol) return eventoComoValor(ma, (Symbol.EventSymbol) sub);   // V8 #771
            // H5.a-E3 — la sobrecarga de métodos se resuelve sobre la clase del
            // RECEPTOR (ve las suyas y las heredadas); lookupInstance devuelve una
            // sola, así que anotamos la clase para que analyzeCall pueda juntar
            // todos los candidatos de la cadena de herencia.
            if (sub instanceof FunctionSymbol) maRecvClass.put(ma, ct.cls);
            return typeOfSymbol(sub);
        }

        if (tgtT instanceof ErrorType) return ErrorType.INSTANCE;
        err(ma.line, ma.column, "el tipo '" + tgtT.display() + "' no tiene miembros");
        return ErrorType.INSTANCE;
    }

    private BpType analyzeIndex(IndexExpr ix, Scope scope) {
        BpType elemento = elementoDeArrayNuevo(ix, scope);
        if (elemento != null) {
            if (elemento instanceof ErrorType) return ErrorType.INSTANCE;
            if (enJc && !ix.conNew) {
                String n = nombreLlamado(ix.target);
                err(ix.line, ix.column, "'" + n + "[n]' crea un array: en JC se escribe 'new " + nombreJc(n) + "[n]'");
            }
            BpType tamT = analyzeExpr(ix.index, scope, PrimitiveType.INTEGER);
            if (!PrimitiveType.INTEGER.isAssignableFrom(tamT))
                err(ix.line, ix.column, "el tamaño de un array es un integer, no '" + tamT.display() + "'");
            info.arraysNuevos.add(ix);
            return new ArrayType(elemento);
        }
        if (ix.conNew) {
            err(ix.line, ix.column, "tras 'new' va el tipo de los elementos, y '" + nombreLlamado(ix.target)
                    + "' no es un tipo");
            return ErrorType.INSTANCE;
        }
        arrayFijoPermitido.add(ix.target);   // V7/H1 #19: indexar un array fijo sí
        BpType t = analyzeExpr(ix.target, scope, null);
        BpType idxT = analyzeExpr(ix.index, scope, PrimitiveType.INTEGER);
        if (!PrimitiveType.INTEGER.isAssignableFrom(idxT))
            err(ix.line, ix.column, "índice debe ser integer, no '" + idxT.display() + "'");
        if (t instanceof ArrayType) return ((ArrayType) t).element;
        if (t instanceof ErrorType) return ErrorType.INSTANCE;
        err(ix.line, ix.column, "el tipo '" + t.display() + "' no es indexable");
        return ErrorType.INSTANCE;
    }

    /** V8/H8 — los nombres de los tipos primitivos, como los deja el parser en una expresión (`integer[10]`). */
    private static final java.util.Set<String> TIPOS_PRIMITIVOS = new java.util.HashSet<>(java.util.Arrays.asList(
            "integer", "float", "string", "boolean", "byte", "int8", "word", "int16", "long", "double"));

    /** V8/H8 — `A[n]` (`new A[n]` en JC) crea un array de `n` casillas de `A`, a cero o a null. El árbol es el de indexar,
     *  porque BP no los distingue al leer: los distingue lo que es el nombre de la izquierda. Da el tipo del elemento, o
     *  null si no es un tipo. Un nombre con valor (una variable, un campo…) gana y se indexa, como gana en `A(…)`; en JC
     *  no se mira: tras `new` sólo va un tipo. */
    private BpType elementoDeArrayNuevo(IndexExpr ix, Scope scope) {
        String nombre;
        if (ix.target instanceof IdentifierExpr) {
            nombre = ((IdentifierExpr) ix.target).name;
            if (!ix.conNew && tieneValor(nombre, scope)) return null;
            if (TIPOS_PRIMITIVOS.contains(nombre)) return resolveType(new SimpleTypeRef(nombre, ix.line, ix.column));
        } else if (ix.target instanceof MemberAccessExpr
                && ((MemberAccessExpr) ix.target).target instanceof IdentifierExpr) {
            String modulo = ((IdentifierExpr) ((MemberAccessExpr) ix.target).target).name;
            if (tieneValor(modulo, scope)) return null;   // `objeto.lista[i]`
            nombre = modulo + "." + ((MemberAccessExpr) ix.target).member;
        } else {
            return null;
        }
        return resolveNamedType(new SimpleTypeRef(nombre, ix.line, ix.column), false);
    }

    /** ¿Nombra `nombre`, aquí, algo que se pueda indexar? Se busca en el orden de {@link #analyzeIdentifier}. Una función
     *  no: `byte` es la conversión `byte(x)`, y el nombre de una clase es también su constructor. */
    private boolean tieneValor(String nombre, Scope scope) {
        Symbol s = (scope != null) ? scope.resolve(nombre) : null;
        if (s == null && currentClass != null && currentFunction != null && !currentFunction.isStatic)
            s = currentClass.lookupInstance(nombre);
        if (s == null) s = module.members.resolve(nombre);
        return s != null && !(s instanceof FunctionSymbol) && !(s instanceof ClassSymbol) && !(s instanceof EnumSymbol)
                && !(s instanceof Symbol.ImportedNamespaceSymbol);
    }

    /**
     * #325 — `Thread(obj::metodo(args) [, pila])`.
     *
     * Comprueba la llamada preparada: que el receptor sea un objeto, que el
     * método exista y no sea static (sin receptor no hay a quién llamar), y que
     * los argumentos encajen — resolviendo sobrecarga si la hay, igual que en
     * una llamada normal. Anota el método elegido en el nodo para que el emisor
     * sepa qué meter en el `run()` que sintetiza.
     *
     * Los argumentos se comprueban (y se evalúan, en ejecución) AQUÍ, en el
     * llamante: lo que el thread recibe son valores, no una promesa.
     */
    private void analyzeThreadOfBoundCall(CallExpr ce, Scope scope) {
        Ast.BoundCallExpr bc = (Ast.BoundCallExpr) ce.args.get(0);
        if (ce.args.size() > 2) {
            err(ce.line, ce.column, "Thread(): se esperaba la llamada a ejecutar y,"
                    + " como mucho, el tamaño de pila — Thread(obj::metodo(args), 8192)");
        }
        // 2º argumento opcional: la pila. Sin él, la de por defecto de la VM.
        for (int i = 1; i < ce.args.size(); i++) {
            BpType st = analyzeExpr(ce.args.get(i), scope, PrimitiveType.INTEGER);
            if (st != null && !(st instanceof ErrorType) && !PrimitiveType.INTEGER.isAssignableFrom(st))
                err(ce.line, ce.column, "Thread(): el tamaño de pila es un entero, no '"
                        + st.display() + "'");
        }

        BpType recvT = analyzeExpr(bc.target, scope, null);
        ClassSymbol rc = (recvT instanceof ClassType) ? ((ClassType) recvT).cls : null;
        if (rc == null) {
            if (!(recvT instanceof ErrorType))
                err(bc.line, bc.column, "el receptor de '::' tiene que ser un objeto"
                        + " (para poder llamar a su método), y aquí es '"
                        + (recvT != null ? recvT.display() : "desconocido") + "'");
            for (IExpr a : bc.args) analyzeExpr(a, scope, null);
            return;
        }
        Symbol ms = rc.lookupInstance(bc.method);
        if (!(ms instanceof FunctionSymbol)) {
            err(bc.line, bc.column, "la clase '" + rc.name + "' no tiene un método '"
                    + bc.method + "' que el thread pueda ejecutar");
            for (IExpr a : bc.args) analyzeExpr(a, scope, null);
            return;
        }
        FunctionSymbol fn = (FunctionSymbol) ms;
        // Sobrecarga: los tipos de los argumentos deciden, igual que en una
        // llamada normal. Se envuelve en un CallExpr sintético porque el
        // resolvedor trabaja sobre uno — comparte la MISMA lista de args, así
        // que los defaults que rellene quedan puestos en la llamada preparada.
        if (fn.overloaded) {
            CallExpr synth = new CallExpr(null, bc.args, bc.line, bc.column);
            FunctionSymbol chosen = resolveOverloadCall(fn, synth, scope);
            if (chosen == null) return;
            fn = chosen;
        }
        if (fn.isStatic) {
            err(bc.line, bc.column, "'" + bc.method + "' es static: no tiene receptor,"
                    + " así que no se puede escribir como 'obj::" + bc.method + "'");
            return;
        }
        // Lanzar algo en un thread no da acceso a lo que no lo tendría escrito a
        // mano: `lookupInstance` también encuentra los privados, así que la
        // visibilidad se comprueba igual que en una llamada normal.
        checkVisibility(bc.line, bc.column, fn, rc, !(bc.target instanceof SuperExpr));   // #760: como x.m, por el objeto
        checkArgs(bc.line, bc.column, fn.params, bc.args, scope);
        info.exprSymbols.put(bc, fn);
        info.boundCalls.add(bc);   // el emisor sintetiza una subclase por cada uno
    }

    /** V8/H7.1 (#747) — ¿es `iif(…)`, el ternario? Por el nombre: es del lenguaje (una función no puede llamarse así). */
    private static boolean esTernario(CallExpr ce) {
        return ce.callee instanceof IdentifierExpr && Ast.TERNARIO.equals(((IdentifierExpr) ce.callee).name);
    }

    /**
     * V8/H7.1 (#747) — el ternario, `iif(c, a, b)`: el valor de `a` si `c`, y si no el de `b`; sólo se calcula el que
     * toca (lo despliega el emisor con saltos). Su tipo, el de las dos ramas: el mismo; o números, y gana el más ancho;
     * o `null` y una referencia; o una clase y su hija, y gana el padre. Un `var owner` local no puede ser una rama
     * (Eduardo, 31-oct: el ternario no es el origen de un movimiento; con un `if`, sí).
     */
    private BpType analyzeTernario(CallExpr ce, Scope scope, BpType esperado) {
        if (ce.args.size() != 3) {
            for (IExpr a : ce.args) analyzeExpr(a, scope, null);
            err(ce.line, ce.column, Ast.TERNARIO + "(condición, a, b): se esperaban 3 argumentos, se pasaron " + ce.args.size());
            return ErrorType.INSTANCE;
        }
        analyzeBoolCondition(ce.args.get(0), scope);
        IExpr ea = ce.args.get(1), eb = ce.args.get(2);
        BpType ta = analyzeExpr(ea, scope, esperado);
        BpType tb = analyzeExpr(eb, scope, esperado);
        for (IExpr rama : new IExpr[] { ea, eb })
            if (esOwnerLocal(rama, scope))
                err(ce.line, ce.column, "'" + ((IdentifierExpr) rama).name + "' es un `var owner` local: " + Ast.TERNARIO
                        + " no mueve un owner (escríbelo con un if)");
        if (ta instanceof ErrorType || tb instanceof ErrorType) return ErrorType.INSTANCE;
        BpType r = tipoDeLasRamas(ta, tb);
        if (r == null) {
            err(ce.line, ce.column, Ast.TERNARIO + ": las dos ramas tienen que dar el mismo tipo (o números, o null y una"
                    + " referencia, o una clase y su hija): '" + ta.display() + "' y '" + tb.display() + "'");
            return ErrorType.INSTANCE;
        }
        info.ternarios.add(ce);
        return r;
    }

    /** El tipo de un ternario con estas dos ramas, o null si no casan. */
    private static BpType tipoDeLasRamas(BpType a, BpType b) {
        if (a instanceof VoidType || b instanceof VoidType) return null;
        if (a instanceof NullType && b instanceof NullType) return null;   // sin tipo
        if (a.isNumeric() && b.isNumeric()) return promote(a, b);
        if (a instanceof NullType) return b.isScalar() && !isString(b) ? null : b;   // #774: `c ? "a" : null`, un string
        if (b instanceof NullType) return a.isScalar() && !isString(a) ? null : a;
        if (a.isAssignableFrom(b)) return a;
        if (b.isAssignableFrom(a)) return b;
        return null;
    }

    /** V8 #778 (Eduardo, 3-nov: «Para los Enum una función nueva, algo como Enum.getName(miEnum.enun4) o similar»; y
     *  antes: «en tiempo de compilación debería poderse») — `Enum.getName(c)`: el nombre del valor de un enum. Lo
     *  resuelve el compilador, que sabe los nombres de cada enum: el de este módulo, por su declaración; el de otro, por
     *  su interfaz (sus nombres y valores, en el orden en que se declararon, #522). El emisor escribe en el módulo, una
     *  vez por enum, la función valor -> nombre, y la llama: ni la VM ni Core cambian (declarar `Enum` en Core cambiaría
     *  su interfaz, y todo programa la lleva sellada). `Enum` no es un símbolo de nadie: si el programa declara uno con
     *  ese nombre, gana el suyo (null aquí). null si la llamada no es ésta. */
    private BpType nombreDeEnum(CallExpr ce, MemberAccessExpr ma, Scope scope) {
        if (!(ma.target instanceof IdentifierExpr) || !"getName".equals(ma.member)) return null;
        String quien = ((IdentifierExpr) ma.target).name;
        if (!"Enum".equals(quien)) return null;
        if (module.members.resolve(quien) != null || (scope != null && scope.resolve(quien) != null)) return null;
        if (ce.args.size() != 1) {
            for (IExpr a : ce.args) analyzeExpr(a, scope, null);
            err(ce.line, ce.column, "Enum.getName(): se esperaba 1 argumento, el valor de un enum");
            return ErrorType.INSTANCE;
        }
        BpType at = analyzeExpr(ce.args.get(0), scope, null);
        if (at instanceof ErrorType) return ErrorType.INSTANCE;
        if (!(at instanceof EnumType)) {
            err(ce.line, ce.column, "Enum.getName(): se esperaba el valor de un enum, no '" + at.display() + "'");
            return ErrorType.INSTANCE;
        }
        info.nombresDeEnum.put(ce, (EnumType) at);
        return PrimitiveType.STRING;
    }

    private BpType analyzeCall(CallExpr ce, Scope scope) {
        Symbol target = null;
        boolean rejectedByModuleVisibility = false;
        if (ce.callee instanceof IdentifierExpr && "len".equals(((IdentifierExpr) ce.callee).name) && ce.args.size() == 1)
            arrayFijoPermitido.add(ce.args.get(0));   // V7/H1 #19: medir un array fijo sí
        if (ce.callee instanceof IdentifierExpr) {
            IdentifierExpr id = (IdentifierExpr) ce.callee;
            // H1.3b — casts numéricos generales: integer()/long()/float()/double().
            // No son símbolos (son keywords de tipo); se resuelven aquí.
            BpType castT = numericCastTarget(id.name);
            if (castT != null) {
                BpType at = null;
                for (int i = 0; i < ce.args.size(); i++) {
                    BpType a = analyzeExpr(ce.args.get(i), scope, null);
                    if (i == 0) at = a;
                }
                // V8/H6 (#754) — un enum, también: su entero (Eduardo, 31-oct: «la conversión de Enum a entero ha de
                // ser válida siempre que se haga con la sintaxis correcta. El compilador ya sabe el valor entero del enum
                // así que es el compilador el que ha de hacer la conversión»). Al revés, de un entero a un enum, sigue
                // sin valer (#522).
                if (ce.args.size() != 1)
                    err(ce.line, ce.column, id.name + "(): se esperaba 1 argumento");
                else if (at != null && !at.isNumeric() && !(at instanceof EnumType) && !(at instanceof ErrorType))
                    err(ce.line, ce.column, id.name + "(): se esperaba un valor numérico, no '" + at.display() + "'");
                return castT;
            }
            // #389 — CONVERSIÓN DE REFERENCIA: `Cosa(o)`, `string(o)`.
            // La misma regla que ya rige entre primitivos —el nombre del tipo
            // como constructor: `byte(someInt)` (L10), `integer(d)` (H1.3b)—
            // extendida a referencias. Subir a `Object` es implícito; bajar de
            // `Object` lo escribe el programador, como en cualquier lenguaje.
            //
            // Aquí NO se comprueba nada en ejecución: la conversión sólo fija el
            // tipo estático. Lo que arregla hoy es que el estrechamiento SE VEA
            // en el código, en vez de colarse mudo y dar un número plausible.
            // `string(o)` — el único destino de conversión que NO es una clase,
            // así que se resuelve aquí. El de las clases va más abajo, en el
            // punto donde confluyen el nombre simple y el CUALIFICADO
            // (`Collections.Long(o)`): allí basta escribir la regla una vez.
            if ("string".equals(id.name) && ce.args.size() == 1) {
                BpType at = analyzeExpr(ce.args.get(0), scope, null);
                if (esObjetoRaiz(at)) {
                    info.refCasts.add(ce);
                    return PrimitiveType.STRING;
                }
            }
            target = module.members.resolve(id.name);
            // Gating: si estamos dentro de una clase y el match en el módulo
            // es un símbolo privado (function/var/const/property no public),
            // lo descartamos y seguimos buscando en scope/clase. Si tampoco
            // hay alternativa, daremos un error específico de visibilidad
            // en lugar del genérico "identificador no resuelto".
            if (target != null && currentClass != null
                    && module.members.tryLookup(id.name) == target
                    && !classCanAccessModuleSymbol(target)) {
                rejectedByModuleVisibility = true;
                target = null;
            }
            if (target == null && scope != null) target = scope.resolve(id.name);
            if (target == null && currentClass != null && currentFunction != null && !currentFunction.isStatic)
                target = currentClass.lookupInstance(id.name);
            if (target != null) info.exprSymbols.put(id, target);
            else if (rejectedByModuleVisibility) {
                err(id.line, id.column,
                    "la clase '" + currentClass.name + "' no puede llamar a '"
                    + id.name + "' del módulo (no es public)");
                for (IExpr a : ce.args) analyzeExpr(a, scope, null);
                return ErrorType.INSTANCE;
            }
        } else if (ce.callee instanceof MemberAccessExpr) {
            MemberAccessExpr ma = (MemberAccessExpr) ce.callee;
            BpType nombre = nombreDeEnum(ce, ma, scope);   // V8 #778: Enum.getName(c)
            if (nombre != null) return nombre;
            analyzeExpr(ma, scope, null);
            target = info.exprSymbols.get(ma);
        }

        // #522 — `Color(1)`: un enum no se construye ni se convierte desde un entero (el
        // tipo es nominal y cerrado: `integer(c)` tampoco vale). Antes caía al final en
        // silencio y lo paraba el emisor sin línea (o, cualificado, seguía y devolvía basura).
        if (target instanceof EnumSymbol) {
            for (IExpr a : ce.args) analyzeExpr(a, scope, null);
            err(ce.line, ce.column, "'" + ((EnumSymbol) target).name
                + "' es un enum: no se llama ni se convierte desde un entero (usa "
                + ((EnumSymbol) target).name + ".VALOR)");
            return ErrorType.INSTANCE;
        }
        // Llamada a clase ⇒ construcción
        if (target instanceof ClassSymbol) {
            ClassSymbol cls = (ClassSymbol) target;
            // #325 — `Thread(obj::metodo(args) [, pila])`: un thread cuyo cuerpo
            // es esa llamada. Se intercepta antes del camino normal de
            // constructor porque el 1er argumento no es un valor, es lo que va a
            // EJECUTAR el thread. El emisor sintetiza la subclase.
            if ("Thread".equals(cls.name) && !ce.args.isEmpty()
                    && ce.args.get(0) instanceof Ast.BoundCallExpr) {
                analyzeThreadOfBoundCall(ce, scope);
                return new ClassType(cls);
            }
            // #389 — CONVERSIÓN DE REFERENCIA: `Cosa(o)` con `o` de tipo
            // `Object`. Va aquí y no en la rama del identificador porque a este
            // punto llegan las dos formas de nombrar la clase —simple y
            // CUALIFICADA (`Collections.Long(o)`)— y la cualificada es
            // justamente la del caso que originó #389: un DAO que devuelve
            // `Object` y una entidad que vive en otro módulo.
            //
            // Sólo se convierte lo que viene de la RAÍZ: cualquier otro
            // argumento es una construcción normal y no hay que robársela.
            // Y si la clase declara un constructor que SÍ acepta un `Object`,
            // gana el suyo.
            //
            // LIMITACIÓN conocida: con constructores SOBRECARGADOS se mira el
            // que devuelve `findConstructor()`. Una clase que sobrecargue un
            // constructor de `Object` entre varios podría quedar tapada; hoy no
            // existe ninguna, y forzarlo pediría resolver la sobrecarga antes de
            // saber si esto es una llamada.
            // V8 H7.3 (#756) — una interfaz no se construye: `Forma(x)` (JC: `(IForma) x`) es siempre la
            // conversión, desde cualquier objeto, y la comprueba la VM.
            if (cls.isInterface) {
                if (ce.args.size() == 1) {
                    BpType at0 = analyzeExpr(ce.args.get(0), scope, null);
                    if (at0 instanceof ClassType || at0 instanceof NullType || at0 instanceof ErrorType) {
                        info.refCasts.add(ce);
                        return new ClassType(cls);
                    }
                    err(ce.line, ce.column, "'" + at0.display() + "' no se convierte a la interfaz '" + cls.name
                            + "': sólo un objeto");
                    return ErrorType.INSTANCE;
                }
                err(ce.line, ce.column, "'" + cls.name + "' es una interfaz: no se construye");
                for (IExpr a : ce.args) analyzeExpr(a, scope, null);
                return ErrorType.INSTANCE;
            }
            if (ce.args.size() == 1) {
                BpType at0 = analyzeExpr(ce.args.get(0), scope, null);
                // L19 (21-sep) — el cast baja también desde cualquier ANTEPASADO, no sólo desde
                // Object: `Perro(a)` con `a: Animal`. Antes se tomaba por constructor («se
                // esperaban 0, se pasaron 1») y había que pasar por Object. El emisor ya hacía
                // CHECKCAST contra cualquier clase; sólo faltaba que el semántico lo aceptara.
                // V8 H7.3 — y desde una interfaz: `Cuadrado(f)` con `f: Forma`.
                boolean desdeInterfaz = at0 instanceof ClassType && ((ClassType) at0).cls.isInterface;
                if (esObjetoRaiz(at0) || esAntepasadoDe(at0, cls) || desdeInterfaz) {
                    FunctionSymbol c1 = cls.findConstructor();
                    boolean ctorLoAcepta = c1 != null && c1.params.size() == 1
                            && c1.params.get(0).type != null
                            && c1.params.get(0).type.isAssignableFrom(at0);
                    if (!ctorLoAcepta) {
                        info.refCasts.add(ce);
                        return new ClassType(cls);
                    }
                }
            }
            FunctionSymbol ctor = cls.findConstructor();
            if (ctor == null) {
                if (!ce.args.isEmpty())
                    err(ce.line, ce.column, "la clase '" + cls.name + "' no define constructor; no admite argumentos");
                for (IExpr a : ce.args) analyzeExpr(a, scope, null);
            } else if (ctor.overloaded) {
                // H5.a-E4 — constructores sobrecargados: elige por firma y ANOTA la
                // elección en el nodo de llamada (el emisor la lee para saber a qué
                // `__init` llamar). Los ctors no van a vtable: es un CALL directo.
                FunctionSymbol chosen = resolveOverloadCall(ctor, ce, scope);
                if (chosen == null) return ErrorType.INSTANCE;
                info.exprSymbols.put(ce, chosen);
                checkVisibilidadCtor(ce.line, ce.column, chosen, cls);   // #515
            } else {
                checkArgs(ce.line, ce.column, ctor.params, ce.args, scope);
                checkVisibilidadCtor(ce.line, ce.column, ctor, cls);   // #515
            }
            return new ClassType(cls);
        }

        if (target instanceof FunctionSymbol) {
            FunctionSymbol fn = (FunctionSymbol) target;
            // H5.a-E3 — método de instancia: los candidatos son los de la clase del
            // RECEPTOR + los heredados (una firma redeclarada en la subclase oculta
            // a la del ancestro). Puede haber sobrecarga aunque `fn.overloaded` sea
            // false: la otra firma puede vivir en la base.
            ClassSymbol recv = maRecvClass.get(ce.callee);
            List<FunctionSymbol> cands = (recv != null)
                    ? collectInstanceCandidates(recv, fn.name) : null;
            if (cands != null && cands.size() > 1) {
                FunctionSymbol chosen = resolveOverloadCall(cands, fn.name, ce, scope);
                if (chosen == null) return ErrorType.INSTANCE;
                fn = chosen;
                info.exprSymbols.put(ce.callee, fn);
            } else if (fn.overloaded) {
                // H5.a — varias sobrecargas con este nombre: elige por firma.
                FunctionSymbol chosen = resolveOverloadCall(fn, ce, scope);
                if (chosen == null) return ErrorType.INSTANCE;   // ambigua / ninguna encaja
                fn = chosen;
                info.exprSymbols.put(ce.callee, fn);   // el emisor emite la sobrecarga elegida
            } else {
                checkArgs(ce.line, ce.column, fn.params, ce.args, scope);
            }
            // L10 — pre-evaluación de literal en casts a tipo estrecho:
            // si el arg es una constante entera fuera del rango, error
            // en compile-time en lugar de esperar al runtime check.
            if (fn.returnType instanceof PrimitiveType
                    && ((PrimitiveType) fn.returnType).isNarrowInteger()
                    && isBuiltin(fn) && ce.args.size() == 1) {
                Long lit = constantIntValueOf(ce.args.get(0));
                if (lit != null) {
                    PrimitiveType rt = (PrimitiveType) fn.returnType;
                    if (lit < rt.rangeMin() || lit > rt.rangeMax()) {
                        err(ce.line, ce.column, fn.name + "(" + lit
                                + "): literal fuera del rango de " + rt.display()
                                + " (" + rt.rangeMin() + ".." + rt.rangeMax() + ")");
                    }
                }
            }
            return (fn.returnType != null) ? fn.returnType : VoidType.INSTANCE;
        }

        // H1 P2c (20-sep) — `len(x)` ya NO es un caso especial: son dos sobrecargas de la raíz
        // (`len(s: string)` y `len(a: any[])`, en Core.bp) y se resuelven como cualquier otra.
        for (IExpr a : ce.args) analyzeExpr(a, scope, null);
        if (target == null && ce.callee instanceof IdentifierExpr)
            err(ce.line, ce.column, "no se puede llamar a '" + ((IdentifierExpr) ce.callee).name + "'");
        return ErrorType.INSTANCE;
    }

    /** V8 #698 — JC escribe lo que BP deduce de los tipos: `new X(…)` en una construcción (punto 22) y `(T) x` en una
     *  conversión (punto 23). El árbol es el mismo, la llamada `X(…)` de BP con las marcas `conNew` y `esConversion`,
     *  y quien sabe qué es cada llamada es el análisis: aquí se mira que lo escrito casa con lo que es, con la misma
     *  regla con la que EscritorJc lo escribe. La primera sentencia de un constructor que llama a su propia clase
     *  delega en otro constructor (#511): en JC es el `: this(…)` de la cabecera, sin `new`. */
    private void revisaJc(CallExpr ce, BpType t) {
        if (t instanceof ErrorType) return;   // lo que fallaba ya está dicho
        String n = nombreLlamado(ce.callee);   // como se escribió; el tipo, como se escribe en JC:
        String tipo = nombreJc(n);
        boolean conversion = ce.args.size() == 1 && (info.refCasts.contains(ce) || (ce.callee instanceof IdentifierExpr
                && CONVERSIONES_PRIMITIVAS.contains(((IdentifierExpr) ce.callee).name)));
        boolean construye = !conversion && info.exprSymbols.get(ce.callee) instanceof ClassSymbol;
        if (delegacionesCtor.contains(ce)) {
            if (ce.conNew)
                err(ce.line, ce.column, "'new " + n + "(…)' como primera sentencia de su constructor delega en otro"
                        + " constructor (#511): en JC se escribe ': this(…)', en la cabecera del constructor");
            return;
        }
        if (conversion) {
            if (ce.conNew)
                err(ce.line, ce.column, "'new " + n + "(x)' es una conversión, no crea un objeto: en JC se escribe '("
                        + tipo + ") x'");
            else if (!ce.esConversion)
                err(ce.line, ce.column, "'" + n + "(x)' es una conversión: en JC se escribe '(" + tipo + ") x'");
        } else if (construye) {
            if (ce.esConversion) {
                BpType at = ce.args.isEmpty() ? null : info.exprTypes.get(ce.args.get(0));
                err(ce.line, ce.column, "'(" + n + ") x' no es una conversión"
                        + (at != null ? ": un '" + nombreJc(at.display()) + "' no se convierte en '" + n
                                + "' (se baja desde Object o desde un antepasado)" : "")
                        + "; para crear un objeto, 'new " + n + "(…)'");
            } else if (!ce.conNew) {
                err(ce.line, ce.column, "'" + n + "(…)' crea un objeto: en JC se escribe 'new " + n + "(…)'");
            }
        } else if (ce.conNew) {
            err(ce.line, ce.column, "'new' crea un objeto de una clase, y '" + n + "' no es una clase");
        } else if (ce.esConversion) {
            err(ce.line, ce.column, "'(" + n + ") x' no es una conversión: '" + n + "' no es un tipo");
        }
    }

    /** El nombre de un tipo primitivo como se escribe en JC (punto 6), para los mensajes de {@link #revisaJc}; lo
     *  demás, tal cual. (Los otros mensajes del análisis dicen los nombres de BP.) */
    private static String nombreJc(String tipo) {
        switch (tipo) {
            case "integer": return "int";
            case "boolean": return "bool";
            case "int8":    return "sbyte";
            case "int16":   return "short";
            case "word":    return "ushort";
            default:        return tipo;
        }
    }

    /** El nombre de lo que se llama, para los mensajes: `f`, `Lib.Clase`, `obj.metodo`. */
    private static String nombreLlamado(IExpr callee) {
        if (callee instanceof IdentifierExpr) return ((IdentifierExpr) callee).name;
        if (callee instanceof MemberAccessExpr) {
            MemberAccessExpr ma = (MemberAccessExpr) callee;
            return nombreLlamado(ma.target) + "." + ma.member;
        }
        if (callee instanceof ThisExpr) return "this";
        return "…";
    }

    private void checkArgs(int line, int col, List<ParamSymbol> ps, List<IExpr> args, Scope scope) {
        // H8.1 — sustitución en el llamante: rellena los args finales omitidos con el
        // valor por defecto de su parámetro (clon del literal). Idempotente: tras
        // rellenar, args.size() == ps.size(), así que re-ejecutar no vuelve a añadir.
        if (args.size() < ps.size()) {
            boolean allDefaulted = true;
            for (int i = args.size(); i < ps.size(); i++)
                if (ps.get(i).defaultExpr == null) { allDefaulted = false; break; }
            if (allDefaulted) {
                int provided = args.size();
                for (int i = provided; i < ps.size(); i++)
                    args.add(cloneConstExpr(ps.get(i).defaultExpr, line, col));
            }
        }
        if (ps.size() != args.size())
            err(line, col, "número de argumentos incorrecto: se esperaban " + ps.size() + ", se pasaron " + args.size());
        int n = Math.min(ps.size(), args.size());
        for (int i = 0; i < n; i++) {
            BpType t = analyzeExpr(args.get(i), scope, ps.get(i).type);
            if (ps.get(i).type != null && !ps.get(i).type.isAssignableFrom(t)) {
                Node nn = (Node) args.get(i);
                err(nn.line, nn.column, "argumento " + (i + 1) + ": '" + t.display() + "' no asignable a '" + ps.get(i).type.display() + "'");
            }
        }
        for (int i = n; i < args.size(); i++) analyzeExpr(args.get(i), scope, null);
    }

    /** H5.a — elige la sobrecarga que mejor encaja con los tipos de los
     *  argumentos de la llamada. Coste por argumento: 0 si el tipo coincide
     *  EXACTO (sameAs), 1 si es asignable con ensanchado (isAssignableFrom),
     *  rechazo si no. Gana el de menor coste total; empate = ambigüedad; ninguno
     *  = error. Rellena los defaults omitidos del elegido (H8.1). Devuelve la
     *  sobrecarga elegida, o null si ya emitió un error. */
    private FunctionSymbol resolveOverloadCall(FunctionSymbol head, CallExpr ce, Scope scope) {
        List<FunctionSymbol> cands = new ArrayList<>();
        for (FunctionSymbol c = head; c != null; c = c.nextOverload) cands.add(c);
        return resolveOverloadCall(cands, head.name, ce, scope);
    }

    private FunctionSymbol resolveOverloadCall(List<FunctionSymbol> cands, String name,
                                               CallExpr ce, Scope scope) {
        return resolveOverloadArgs(cands, name, ce.args, ce.line, ce.column, scope);
    }

    /** V7/H2 (#509) — el núcleo de la resolución por firma, sobre una lista de
     *  argumentos: lo comparten la llamada normal (CallExpr) y `super(...)`
     *  (SuperCallExpr, que no es un CallExpr). `args` es la lista DEL NODO: los
     *  defaults omitidos se rellenan en ella. */
    private FunctionSymbol resolveOverloadArgs(List<FunctionSymbol> cands, String name,
                                               List<IExpr> args, int line, int column, Scope scope) {
        // Tipo estático (natural, sin hint de parámetro) de cada argumento: la
        // selección de sobrecarga se hace sobre esos tipos.
        List<BpType> at = new ArrayList<>();
        for (IExpr a : args) at.add(analyzeExpr(a, scope, null));

        List<FunctionSymbol> encajan = new ArrayList<>();
        for (FunctionSymbol c : cands) if (overloadMatchCost(c, at) >= 0) encajan.add(c);
        if (encajan.isEmpty()) {
            err(line, column, "ninguna sobrecarga de '" + name
                    + "' acepta los argumentos " + argTypeList(at));
            return null;
        }
        // V8 #772 (Eduardo, 3-nov: «como en Java»; y su ejemplo: «Si haces un try ... catch, las excepciones hay que
        // ponerlas de mas específicas a mas genéricas […] Es el compilador el que tiene que ordenar los posibles cast,
        // la VM como siempre no necesita pensar») — de las que encajan, la MÁS ESPECÍFICA: la que se puede pasar a todas
        // las demás, como `catch (Animal)` va antes que `catch (Object)`. Antes ganaba la de menos «puntos» de encaje
        // (0 el tipo exacto, 1 subiendo), y subir a `Animal` o a `Object` costaba lo mismo: `Mete(Object)` y
        // `Mete(Animal)` con un `Perro` era «ambigua». Las que tienen los mismos tipos en lo que la llamada da (difieren
        // en los valores por defecto), la que rellena MENOS (14-sep). La VM no cambia: la llamada sale ya elegida.
        List<FunctionSymbol> maximas = new ArrayList<>();
        for (FunctionSymbol c : encajan) {
            boolean aTodas = true;
            for (FunctionSymbol d : encajan)
                if (d != c && !masEspecifica(c, d, at.size())) { aTodas = false; break; }
            if (aTodas) maximas.add(c);
        }
        FunctionSymbol best = null;
        boolean empate = false;
        int menosDefaults = Integer.MAX_VALUE;
        for (FunctionSymbol c : maximas) {
            int defaults = c.params.size() - at.size();   // params omitidos que hay que rellenar
            if (defaults < menosDefaults) { menosDefaults = defaults; best = c; empate = false; }
            else if (defaults == menosDefaults) empate = true;
        }
        if (best == null || empate) {
            // Las que compiten: las que ninguna otra supera (más específica que ella y no al revés).
            List<String> compiten = new ArrayList<>();
            for (FunctionSymbol c : encajan) {
                boolean superada = false;
                for (FunctionSymbol d : encajan)
                    if (d != c && masEspecifica(d, c, at.size()) && !masEspecifica(c, d, at.size())) { superada = true; break; }
                if (!superada) compiten.add(firmaDe(name, c));
            }
            err(line, column, "llamada ambigua a '" + name + "' con " + argTypeList(at) + ": encajan "
                    + String.join(" y ", compiten) + ", y ninguna es más específica que las otras; para elegir una, pasa"
                    + " el argumento en una variable del tipo de su parámetro");
            return null;
        }
        // Rellena los defaults omitidos del elegido (clon del literal), como checkArgs.
        for (int i = args.size(); i < best.params.size(); i++)
            args.add(cloneConstExpr(best.params.get(i).defaultExpr, line, column));
        return best;
    }

    /** V8 #772 — ¿se puede pasar un `desde` donde va un `hacia`? Para ordenar las sobrecargas, de lo más específico a lo
     *  más genérico. Un parámetro sin tipo (null) lo acepta todo: el más genérico. */
    private static boolean pasaA(BpType desde, BpType hacia) {
        if (hacia == null) return true;
        if (desde == null) return false;
        return hacia.sameAs(desde) || hacia.isAssignableFrom(desde);
    }

    /** V8 #772 — ¿es `a` al menos tan específica como `b` en las `n` posiciones que da la llamada? Lo es si cada
     *  parámetro suyo se puede pasar al de `b` (un `Animal` donde va un `Object`, un `integer` donde va un `long`). */
    private static boolean masEspecifica(FunctionSymbol a, FunctionSymbol b, int n) {
        for (int i = 0; i < n; i++)
            if (!pasaA(a.params.get(i).type, b.params.get(i).type)) return false;
        return true;
    }

    /** V8 #772 — `f(Perro, Object)`: la firma de una sobrecarga, para el mensaje de la llamada ambigua. */
    private static String firmaDe(String name, FunctionSymbol c) {
        StringBuilder sb = new StringBuilder(name).append('(');
        for (int i = 0; i < c.params.size(); i++) {
            if (i > 0) sb.append(", ");
            BpType t = c.params.get(i).type;
            sb.append(t == null ? "?" : t.display());
        }
        return sb.append(')').toString();
    }

    /** Coste de encaje de una sobrecarga con los tipos de argumento dados, o -1
     *  si no aplica (aridad fuera del rango [requeridos, total] o algún argumento
     *  no asignable ni con ensanchado). */
    private int overloadMatchCost(FunctionSymbol fn, List<BpType> at) {
        int nArgs = at.size();
        int required = 0;
        for (ParamSymbol p : fn.params) if (p.defaultExpr == null) required++;
        if (nArgs < required || nArgs > fn.params.size()) return -1;
        int cost = 0;
        for (int i = 0; i < nArgs; i++) {
            BpType pt = fn.params.get(i).type, a = at.get(i);
            if (a instanceof ErrorType) continue;                // no encadenar diagnósticos
            if (pt == null) { cost += 1; continue; }
            if (pt.sameAs(a)) continue;                          // exacto → coste 0
            if (pt.isAssignableFrom(a)) { cost += 1; continue; } // ensanchado → coste 1
            return -1;                                           // no asignable
        }
        return cost;
    }

    private String argTypeList(List<BpType> at) {
        StringBuilder sb = new StringBuilder("(");
        for (int i = 0; i < at.size(); i++) {
            if (i > 0) sb.append(", ");
            sb.append(at.get(i) == null ? "?" : at.get(i).display());
        }
        return sb.append(")").toString();
    }

    private BpType analyzeUnary(UnaryExpr u, Scope scope) {
        BpType t = analyzeExpr(u.operand, scope, null);
        if ("-".equals(u.op)) {
            if (!t.isNumeric() && !(t instanceof ErrorType))
                err(u.line, u.column, "'-' unario requiere numérico, no '" + t.display() + "'");
            return t.isNumeric() ? t : ErrorType.INSTANCE;
        }
        if ("not".equals(u.op)) {
            if (!PrimitiveType.BOOLEAN.isAssignableFrom(t) && !(t instanceof ErrorType))
                err(u.line, u.column, "'not' requiere boolean, no '" + t.display() + "'");
            return PrimitiveType.BOOLEAN;
        }
        return ErrorType.INSTANCE;
    }

    private BpType analyzeInstanceOf(InstanceOfExpr ie, Scope scope) {
        // Lado izquierdo: cualquier expresión. No exigimos tipo concreto;
        // un instanceof sobre algo no-ref simplemente dará false en runtime.
        analyzeExpr(ie.target, scope, null);
        // Lado derecho: nombre de clase.
        Symbol sym = scope.resolve(ie.typeName);
        if (sym == null) sym = module.members.resolve(ie.typeName);
        // V7/H2 (#523, D7) — `instanceof Mod.Cls` (o una clase importada sin cualificar):
        // se resuelve como cualquier tipo nombrado, por el mismo camino que `var b: Lib.Base`.
        if (sym == null) {
            BpType t = resolveNamedType(new SimpleTypeRef(ie.typeName, ie.line, ie.column));
            if (t instanceof ClassType) sym = ((ClassType) t).cls;
            else if (t instanceof ErrorType) return PrimitiveType.BOOLEAN;   // ya se dijo «tipo cualificado … no encontrado»
        }
        if (!(sym instanceof ClassSymbol)) {
            err(ie.line, ie.column, "'instanceof' espera un nombre de clase, '"
                    + ie.typeName + "' no lo es");
        } else {
            info.exprSymbols.put(ie, sym);   // emisor podrá leer el class symbol
        }
        return PrimitiveType.BOOLEAN;
    }

    private BpType analyzeBinary(BinaryExpr b, Scope scope) {
        BpType ev = comparacionDeEvento(b, scope);   // V8 #771: `ev != null`, ¿tiene suscriptor?
        if (ev != null) return ev;
        BpType lt = analyzeExpr(b.left,  scope, null);
        BpType rt = analyzeExpr(b.right, scope, null);
        // Anti-cascada (poisoning): si un operando ya es <error>, el error real
        // ya se reportó aguas arriba. No re-reportamos un "incompatible" espurio;
        // propagamos ErrorType en silencio.
        if (lt instanceof ErrorType || rt instanceof ErrorType) return ErrorType.INSTANCE;
        return tipoBinario(b.op, b.op, lt, rt, b.line, b.column);
    }

    /**
     * V8 #771 (Eduardo, 3-nov: «b) si») — `ev == null` y `ev != null` con un evento: «¿tiene suscriptor?», como en C#
     * (`if (Click != null)`). Sólo eso, y sólo dentro de su clase o de una hija, como `raise`: fuera, un evento sólo se
     * suscribe. El emisor lee el campo del receptor (la suscripción lo escribe; `:= null` lo borra). Cualquier otro uso
     * de un evento como valor es un error (analyzeIdentifier, analyzeMemberAccess). null si no es esa forma.
     */
    private BpType comparacionDeEvento(BinaryExpr b, Scope scope) {
        if (!"==".equals(b.op) && !"!=".equals(b.op)) return null;
        IExpr lado = (b.right instanceof Ast.NullLitExpr) ? b.left : (b.left instanceof Ast.NullLitExpr) ? b.right : null;
        if (lado == null) return null;
        // un nombre a pelo que antes es una local o un parámetro no es el evento (el orden de analyzeIdentifier)
        if (lado instanceof IdentifierExpr && scope != null && scope.resolve(((IdentifierExpr) lado).name) != null) return null;
        // `obj.ev`, con `obj` un nombre o `this`: resolveEventTarget lo analiza, y si no era un evento el análisis normal
        // lo hace otra vez; con algo más (una llamada…) un error suyo saldría dos veces
        if (lado instanceof MemberAccessExpr) {
            IExpr obj = ((MemberAccessExpr) lado).target;
            if (!(obj instanceof IdentifierExpr) && !(obj instanceof Ast.ThisExpr)) return null;
        } else if (!(lado instanceof IdentifierExpr)) return null;
        Symbol.EventSymbol ev = resolveEventTarget(lado, scope);
        if (ev == null) return null;
        boolean suya = false;
        for (ClassSymbol c = currentClass; c != null && !suya; c = c.baseClass) suya = (c == ev.ownerClass);
        if (!suya)
            err(b.line, b.column, "'" + ev.name + "' es un evento: compararlo con null («¿tiene suscriptor?») sólo vale"
                    + " dentro de su clase, como 'raise'; fuera, un evento sólo se suscribe");
        info.eventosNulos.add(b);
        return PrimitiveType.BOOLEAN;
    }

    /** V8 #771 — un evento usado como valor (fuera de una suscripción, un `raise` o la comparación con null). */
    private BpType eventoComoValor(IExpr e, Symbol.EventSymbol ev) {
        err(((Ast.Node) e).line, ((Ast.Node) e).column, "'" + ev.name + "' es un evento, no un valor: se suscribe con '"
                + ev.name + " := obj::metodo', se desuscribe con ':= null', se dispara con 'raise' y, dentro de su"
                + " clase, '" + ev.name + " != null' dice si tiene suscriptor");
        return ErrorType.INSTANCE;
    }

    /** El tipo de `l op r`, o ErrorType con su error dicho. Lo comparten la expresión binaria y la asignación compuesta
     *  (V8/H7.1 #747: `x op= e` opera como `x op e`); {@code visto} es cómo se escribió el operador, para el mensaje. */
    private BpType tipoBinario(String op, String visto, BpType lt, BpType rt, int line, int column) {
        switch (op) {
            case "+":
                if (isString(lt) || isString(rt)) {
                    // #522 — el otro operando tiene que TENER conversión a string: el emisor
                    // (coerceToString) sólo convierte primitivos, enums (su entero, como
                    // `print`) y objetos (toString). Un array, una tupla, `null` o una
                    // llamada void pasaban aquí y el concat leía su valor como handle de
                    // string: heap roto en las dos VMs sin ningún diagnóstico.
                    BpType otro = isString(lt) ? rt : lt;
                    if (!convertibleAString(otro)) {
                        err(line, column, "'" + visto + "' incompatible: '" + lt.display() + "' y '" + rt.display()
                            + "' (no hay conversión a string)");
                        return ErrorType.INSTANCE;
                    }
                    return PrimitiveType.STRING;
                }
                if (lt.isNumeric() && rt.isNumeric()) return promote(lt, rt);
                err(line, column, "'" + visto + "' incompatible: '" + lt.display() + "' y '" + rt.display() + "'");
                return ErrorType.INSTANCE;
            case "-": case "*": case "/":
                if (lt.isNumeric() && rt.isNumeric()) return promote(lt, rt);
                err(line, column, "'" + visto + "' requiere numéricos, encontrados '" + lt.display() + "' y '" + rt.display() + "'");
                return ErrorType.INSTANCE;
            case "mod":
                // L10/H1.2/H1.3 — mod opera en el tipo promovido (i32/i64/f32/f64;
                // float/double usan fmod). Resultado = promote(lt, rt).
                if (lt.isNumeric() && rt.isNumeric()) return promote(lt, rt);
                err(line, column, "'" + visto + "' requiere numéricos");
                return ErrorType.INSTANCE;
            case "^":
                // H7 — potencia. base y exp ENTEROS (int/long) → resultado entero
                // (IPOW/LPOW, exacto). En cuanto hay float/double → resultado
                // float (o double si alguno es double): DPOW. El tipo resultado
                // codifica el camino para el emisor (int/long=enteros, float/double=DPOW).
                if (lt.isNumeric() && rt.isNumeric()) {
                    boolean baseInt = isIntegerLike(lt) || isLong(lt);
                    boolean expInt  = isIntegerLike(rt) || isLong(rt);
                    if (baseInt && expInt)
                        return (isLong(lt) || isLong(rt)) ? PrimitiveType.LONG : PrimitiveType.INTEGER;
                    boolean anyDouble =
                            (lt instanceof PrimitiveType && ((PrimitiveType) lt).tag == PrimitiveType.Kind.DOUBLE)
                         || (rt instanceof PrimitiveType && ((PrimitiveType) rt).tag == PrimitiveType.Kind.DOUBLE);
                    return anyDouble ? PrimitiveType.DOUBLE : PrimitiveType.FLOAT;
                }
                err(line, column, "'" + visto + "' requiere numéricos, encontrados '" + lt.display() + "' y '" + rt.display() + "'");
                return ErrorType.INSTANCE;
            case "&": case "|": case "xor": case "shl": case "shr":
                // H1.2 — bitwise/shift en i64 si algún operando es long.
                if ((isIntegerLike(lt) || isLong(lt)) && (isIntegerLike(rt) || isLong(rt)))
                    return (isLong(lt) || isLong(rt)) ? PrimitiveType.LONG : PrimitiveType.INTEGER;
                // V8/H8 (Eduardo, 4-nov: «el 12 si») — `&`, `|` y `xor` entre dos boolean, como en Java y en C#: calculan
                // los dos lados (`and` y `or` se saltan el segundo). Un boolean es 0 o 1: la operación de bits es la lógica.
                if (!"shl".equals(op) && !"shr".equals(op) && isBoolean(lt) && isBoolean(rt)) return PrimitiveType.BOOLEAN;
                err(line, column, "'" + visto + "' requiere integer/long"
                    + ("shl".equals(op) || "shr".equals(op) ? "" : ", o dos boolean"));
                return ErrorType.INSTANCE;
            case "and": case "or":
                if (PrimitiveType.BOOLEAN.isAssignableFrom(lt) && PrimitiveType.BOOLEAN.isAssignableFrom(rt))
                    return PrimitiveType.BOOLEAN;
                err(line, column, "'" + visto + "' requiere boolean");
                return ErrorType.INSTANCE;
            case "==": case "!=":
                if (!comparableForEquality(lt, rt))
                    err(line, column, "'" + visto + "' incompatible: '" + lt.display() + "' vs '" + rt.display() + "'");
                return PrimitiveType.BOOLEAN;
            case "<": case ">": case "<=": case ">=":
                if (lt.isNumeric() && rt.isNumeric()) return PrimitiveType.BOOLEAN;
                if (isString(lt) && isString(rt)) return PrimitiveType.BOOLEAN;
                err(line, column, "'" + visto + "' requiere numéricos o strings");
                return ErrorType.INSTANCE;
        }
        return ErrorType.INSTANCE;
    }

    private static boolean isString(BpType t) {
        return t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.STRING;
    }

    /** #522 — lo que `coerceToString` del emisor sabe convertir en una concatenación:
     *  primitivos, enums (a su entero), objetos (toString) y `any` (la escapatoria de la
     *  stdlib). Arrays, tuplas, `null` y void, no. */
    private static boolean convertibleAString(BpType t) {
        return t instanceof PrimitiveType || t instanceof EnumType || t instanceof ClassType
            || t instanceof AnyType || t instanceof ErrorType;
    }

    /** L10 — `integer-like`: INTEGER o cualquiera de los tipos estrechos.
     *  Para operaciones que en la pila VM son i32 (mod, bitwise) y que
     *  no tienen sentido sobre float. */
    private static boolean isIntegerLike(BpType t) {
        return t instanceof PrimitiveType && ((PrimitiveType) t).isIntegerLike();
    }

    private static boolean isLong(BpType t) {   // H1.2 (V2)
        return t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.LONG;
    }

    private static boolean isBoolean(BpType t) {
        return t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.BOOLEAN;
    }

    /**
     * #389 — destino de una conversión de REFERENCIA escrita con el nombre del
     * tipo (`Cosa(o)`, `string(o)`), o null si esa llamada no lo es.
     *
     * NO adivina. Sólo entra cuando la clase destino **no declara un constructor
     * de un argumento**: si lo declara, gana el constructor y esto se aparta. Es
     * la misma regla de "si lo declaras tú, yo me quito" con la que el emisor
     * trata `Object`, `List`, `SyncList` y `OwnerList`.
     *
     * Y por eso analizar aquí el argumento no duplica trabajo ni diagnósticos:
     * si llegamos, la llamada por el camino normal iba a fallar de todas formas
     * por aridad ("se esperaban 0, se pasaron 1").
     */
    /** #389 — ¿es el tipo la clase RAÍZ `Object`? (la built-in, no una clase de
     *  usuario que se llame igual). */
    private boolean esObjetoRaiz(BpType t) {
        return t instanceof BpType.ClassType && esObject(((BpType.ClassType) t).cls);
    }
    /** L19 — ¿es `t` una clase de la que `cls` desciende (estrictamente)? Para el cast hacia abajo. */
    private static boolean esAntepasadoDe(BpType t, ClassSymbol cls) {
        if (!(t instanceof BpType.ClassType)) return false;
        ClassSymbol a = ((BpType.ClassType) t).cls;
        for (ClassSymbol c = cls.baseClass; c != null; c = c.baseClass)
            if (c == a || (a.esDelLenguaje() && c.esDelLenguaje() && a.name.equals(c.name))) return true;
        return false;
    }

    /** H1.3b (V2) — destino de un cast numérico general integer()/long()/
     *  float()/double(), o null si el nombre no es uno de ellos. */
    private static BpType numericCastTarget(String name) {
        switch (name) {
            case "integer": return PrimitiveType.INTEGER;
            case "long":    return PrimitiveType.LONG;
            case "float":   return PrimitiveType.FLOAT;
            case "double":  return PrimitiveType.DOUBLE;
            default:        return null;
        }
    }

    private static BpType promote(BpType a, BpType b) {
        // H1.3 — torre de promoción: double > float > long > int.
        boolean aIsDouble = a instanceof PrimitiveType && ((PrimitiveType) a).tag == PrimitiveType.Kind.DOUBLE;
        boolean bIsDouble = b instanceof PrimitiveType && ((PrimitiveType) b).tag == PrimitiveType.Kind.DOUBLE;
        if (aIsDouble || bIsDouble) return PrimitiveType.DOUBLE;
        boolean aIsFloat = a instanceof PrimitiveType && ((PrimitiveType) a).tag == PrimitiveType.Kind.FLOAT;
        boolean bIsFloat = b instanceof PrimitiveType && ((PrimitiveType) b).tag == PrimitiveType.Kind.FLOAT;
        if (aIsFloat || bIsFloat) return PrimitiveType.FLOAT;
        // H1.2 — si alguno es long (y ninguno float), el resultado es long.
        boolean aIsLong = a instanceof PrimitiveType && ((PrimitiveType) a).tag == PrimitiveType.Kind.LONG;
        boolean bIsLong = b instanceof PrimitiveType && ((PrimitiveType) b).tag == PrimitiveType.Kind.LONG;
        if (aIsLong || bIsLong) return PrimitiveType.LONG;
        return PrimitiveType.INTEGER;
    }

    private static boolean comparableForEquality(BpType a, BpType b) {
        if (a instanceof ErrorType || b instanceof ErrorType) return true;
        if (a.sameAs(b)) return true;
        // V8 H7.3 — una interfaz, con cualquier objeto: la identidad (una clase que no la implementa puede tener hijas
        // que sí, como en Java y C#)
        if (a instanceof BpType.ClassType && b instanceof BpType.ClassType
                && (((BpType.ClassType) a).cls.isInterface || ((BpType.ClassType) b).cls.isInterface)) return true;
        if (a instanceof NullType && b.isReference()) return true;
        if (b instanceof NullType && a.isReference()) return true;
        // #774 — una variable string, con null: ¿no referencia ninguna cadena? (__strequals: null sólo es igual a null)
        if ((a instanceof NullType && isString(b)) || (b instanceof NullType && isString(a))) return true;
        if (a.isNumeric() && b.isNumeric()) return true;
        return false;
    }

    private BpType typeOfSymbol(Symbol s) {
        if (s instanceof VarSymbol)      return ((VarSymbol) s).type   != null ? ((VarSymbol) s).type   : ErrorType.INSTANCE;
        if (s instanceof ConstSymbol)    return ((ConstSymbol) s).type != null ? ((ConstSymbol) s).type : ErrorType.INSTANCE;
        if (s instanceof ParamSymbol)    return ((ParamSymbol) s).type != null ? ((ParamSymbol) s).type : ErrorType.INSTANCE;
        if (s instanceof PropertySymbol) return ((PropertySymbol) s).type != null ? ((PropertySymbol) s).type : ErrorType.INSTANCE;
        if (s instanceof FunctionSymbol) {
            FunctionSymbol f = (FunctionSymbol) s;
            return f.returnType != null ? f.returnType : VoidType.INSTANCE;
        }
        if (s instanceof ClassSymbol)    return new ClassType((ClassSymbol) s);
        if (s instanceof EnumSymbol)     return new EnumType((EnumSymbol) s);
        return ErrorType.INSTANCE;
    }

    // ============================================================
    // V8 H7.3 (#756) — LAS INTERFACES QUE IMPLEMENTA UNA CLASE
    // ============================================================
    /**
     * Que la clase tenga, propio o heredado, cada miembro de las interfaces de su cabecera (y de las que éstas
     * heredan): un método de instancia público con el mismo nombre, los mismos parámetros y el mismo retorno —la clave
     * de MOD9, la que casará el cargador—, o una property pública con el mismo nombre y el mismo tipo. Y las
     * anotaciones (Eduardo, V8_IDEAS §E10, «La 5»): `@implement{ A, B }`, obligatoria en lo que la clase declara e
     * implementa un miembro de una interfaz de su cabecera, con una o varias de las interfaces de la clase, y cada una
     * con ese miembro; `@override{}`, opcional, sólo en lo que redefine uno del padre. Lo heredado no se marca.
     */
    private void checkInterfaces(ClassSymbol cls) {
        List<ClassSymbol> deLaCabecera = new ArrayList<>();
        for (ClassSymbol i : cls.interfaces) i.interfacesAplanadas(deLaCabecera);
        // lo que la clase declara e implementa un miembro de una interfaz de su cabecera (→ @implement obligatoria)
        java.util.Map<Symbol, List<ClassSymbol>> implementan = new java.util.IdentityHashMap<>();
        for (ClassSymbol iface : deLaCabecera) {
            for (Symbol m : iface.instanceMembers.getSymbols()) {
                if (m instanceof FunctionSymbol) {
                    for (FunctionSymbol f = (FunctionSymbol) m; f != null; f = f.nextOverload) {
                        String k = claveMiembro(f);
                        if (k == null) continue;
                        FunctionSymbol impl = null, otro = null;
                        for (FunctionSymbol g : collectInstanceCandidates(cls, f.name)) {
                            if (g.isStatic || g.isConstructor || g.ownerClass == null || g.ownerClass.isInterface) continue;
                            if (k.equals(claveMiembro(g))) { impl = g; break; }
                            if (otro == null) otro = g;
                        }
                        if (impl == null) {
                            err(cls.line, cls.column, "'" + cls.name + "' implementa '" + iface.name + "' y no tiene '"
                                    + firmaCompleta(f) + "'" + (otro != null ? " (tiene '" + firmaCompleta(otro) + "')" : ""));
                        } else if (!impl.isPublic) {
                            err(lineaDe(impl, cls), columnaDe(impl, cls), "'" + impl.name + "' implementa '" + iface.name
                                    + "." + f.name + "' y tiene que ser public: es lo que la interfaz exporta");
                        } else if (impl.ownerClass == cls) {
                            implementan.computeIfAbsent(impl, x -> new ArrayList<>()).add(iface);
                        }
                    }
                } else if (m instanceof PropertySymbol) {
                    PropertySymbol p = (PropertySymbol) m;
                    Symbol q = cls.lookupInstance(p.name);
                    PropertySymbol impl = (q instanceof PropertySymbol && !((PropertySymbol) q).isStatic) ? (PropertySymbol) q : null;
                    String tp = claveTipo(p.type), tq = impl != null ? claveTipo(impl.type) : null;
                    if (impl == null || tp == null || !tp.equals(tq)) {
                        err(cls.line, cls.column, "'" + cls.name + "' implementa '" + iface.name + "' y no tiene la"
                                + " property '" + p.name + ": " + (p.type != null ? p.type.display() : "?") + "'"
                                + (impl != null && impl.type != null ? " (la suya es '" + impl.type.display() + "')" : ""));
                    } else if (!impl.isPublic) {
                        err(lineaDe(impl, cls), columnaDe(impl, cls), "la property '" + impl.name + "' implementa '"
                                + iface.name + "." + p.name + "' y tiene que ser public: es lo que la interfaz exporta");
                    } else if (impl.ownerClass == cls) {
                        implementan.computeIfAbsent(impl, x -> new ArrayList<>()).add(iface);
                    }
                }
            }
        }
        info.implementanInterfaz.addAll(implementan.keySet());
        // las anotaciones de lo que la clase declara
        List<ClassSymbol> todas = cls.todasLasInterfaces();
        for (ITopLevelDecl d : cls.astNode.members) {
            if (!(d instanceof FuncDef) && !(d instanceof PropertyDef)) continue;
            Node n = (Node) d;
            Symbol sym = info.declSymbols.get(d);
            if (sym == null) continue;
            Ast.Annotation impl = n.anotacion(ANN_IMPLEMENT);
            Ast.Annotation ov = n.anotacion(ANN_OVERRIDE);
            List<ClassSymbol> deQuien = implementan.get(sym);
            if (deQuien != null && impl == null) {
                List<String> nombres = new ArrayList<>();
                for (ClassSymbol i : cls.interfaces) if (deQuien.stream().anyMatch(j -> i.implementa(j))) nombres.add(i.name);
                err(n.line, n.column, "'" + sym.name + "' implementa '" + deQuien.get(0).name + "." + sym.name
                        + "': márcalo con @implement{ " + String.join(", ", nombres) + " }");
            }
            if (impl != null) {
                if (impl.entries.isEmpty())
                    err(impl.line, impl.column, "@implement nombra las interfaces que implementa: @implement{ "
                            + (cls.interfaces.isEmpty() ? "Interfaz" : cls.interfaces.get(0).name) + " }");
                for (Ast.AnnEntry e : impl.entries) {
                    if (e.value != null) {
                        err(e.line, e.column, "@implement lleva nombres de interfaces, sin '='");
                        continue;
                    }
                    ClassSymbol i = null;
                    int cuantas = 0;
                    for (ClassSymbol j : todas) if (j.name.equals(e.key)) { i = j; cuantas++; }
                    if (i == null) {
                        err(e.line, e.column, "'" + e.key + "' no es una interfaz de '" + cls.name + "'");
                    } else if (cuantas > 1) {
                        err(e.line, e.column, "'" + cls.name + "' implementa dos interfaces que se llaman '" + e.key
                                + "': no se sabe cuál");
                    } else if (!interfazTiene(i, sym)) {
                        err(e.line, e.column, "'" + e.key + "' no tiene '" + (sym instanceof FunctionSymbol
                                ? firmaCompleta((FunctionSymbol) sym) : "property " + sym.name) + "'");
                    }
                }
            }
            if (ov != null) {
                if (!ov.entries.isEmpty()) err(ov.line, ov.column, "@override no lleva nada dentro: @override{}");
                if (!redefineDelPadre(cls, sym))
                    err(ov.line, ov.column, "'" + sym.name + "' lleva @override y no redefine nada de '"
                            + (cls.baseClass != null ? cls.baseClass.name : "?") + "'");
            }
        }
    }

    /** ¿Tiene la interfaz (o una de las que hereda) ese miembro, con la misma clave? */
    private boolean interfazTiene(ClassSymbol iface, Symbol sym) {
        List<ClassSymbol> l = new ArrayList<>();
        iface.interfacesAplanadas(l);
        for (ClassSymbol i : l) {
            Symbol m = i.instanceMembers.tryLookup(sym.name);
            if (sym instanceof FunctionSymbol && m instanceof FunctionSymbol) {
                String k = claveMiembro((FunctionSymbol) sym);
                for (FunctionSymbol f = (FunctionSymbol) m; f != null; f = f.nextOverload)
                    if (k != null && k.equals(claveMiembro(f))) return true;
            } else if (sym instanceof PropertySymbol && m instanceof PropertySymbol) {
                String a = claveTipo(((PropertySymbol) sym).type), b = claveTipo(((PropertySymbol) m).type);
                if (a != null && a.equals(b)) return true;
            }
        }
        return false;
    }

    /** ¿Redefine `sym` (método o property de `cls`) uno no privado de algún padre, con la misma clave? */
    private boolean redefineDelPadre(ClassSymbol cls, Symbol sym) {
        for (ClassSymbol a = cls.baseClass; a != null; a = a.baseClass) {
            Symbol m = a.instanceMembers.tryLookup(sym.name);
            if (sym instanceof FunctionSymbol && m instanceof FunctionSymbol) {
                String k = claveMiembro((FunctionSymbol) sym);
                for (FunctionSymbol f = (FunctionSymbol) m; f != null; f = f.nextOverload)
                    if (!f.isPrivate && !f.isStatic && k != null && k.equals(claveMiembro(f))) return true;
            } else if (sym instanceof PropertySymbol && m instanceof PropertySymbol) {
                if (!((PropertySymbol) m).isPrivate) return true;
            }
        }
        return false;
    }

    /** La clave de MOD9 de un método (`area()d`), o null si un tipo no está resuelto (ya lo dijo otro error). */
    private String claveMiembro(FunctionSymbol f) {
        try { return Symbol.ClaveCanonica.metodo(f, moduloDeLasClaves); }
        catch (Symbol.ClaveCanonica.TipoSinResolver e) { return null; }
    }

    private String claveTipo(BpType t) {
        try { return Symbol.ClaveCanonica.tipo(t, moduloDeLasClaves); }
        catch (Symbol.ClaveCanonica.TipoSinResolver e) { return null; }
    }

    /** `area(): double`, `mueve(dx: integer, dy: integer)`: la firma, para los mensajes. */
    private static String firmaCompleta(FunctionSymbol f) {
        StringBuilder sb = new StringBuilder(f.name).append('(');
        for (int i = 0; i < f.params.size(); i++) {
            ParamSymbol p = f.params.get(i);
            if (i > 0) sb.append(", ");
            sb.append(p.name).append(": ").append(p.type != null ? p.type.display() : "?");
        }
        sb.append(')');
        if (f.returnType != null) sb.append(": ").append(f.returnType.display());
        return sb.toString();
    }

    private static int lineaDe(Symbol s, ClassSymbol cls) { return s.line > 0 ? s.line : cls.line; }
    private static int columnaDe(Symbol s, ClassSymbol cls) { return s.line > 0 ? s.column : cls.column; }

    // ============================================================
    // OVERRIDES Y FINAL
    // ============================================================
    private void checkOverridesAndFinal(ClassSymbol cls) {
        if (cls.baseClass == null) return;
        for (Symbol s : cls.instanceMembers.getSymbols()) {
            if (s instanceof FunctionSymbol) {
                // H5.a-E3 — con sobrecarga, "mismo nombre en la base" YA NO implica
                // override: solo lo es si coincide la FIRMA. Un método con el mismo
                // nombre y firma distinta es una sobrecarga legítima del grupo
                // heredado (el programador ve la cadena como un solo espacio de
                // nombres) y se lleva su propio slot. Se recorre la cadena de
                // sobrecargas de la base buscando la firma exacta.
                for (FunctionSymbol fn = (FunctionSymbol) s; fn != null; fn = fn.nextOverload) {
                    FunctionSymbol baseFn = null;
                    for (ClassSymbol a = cls.baseClass; a != null && baseFn == null; a = a.baseClass) {
                        Symbol as = a.instanceMembers.tryLookup(fn.name);
                        for (FunctionSymbol o = (as instanceof FunctionSymbol) ? (FunctionSymbol) as : null;
                             o != null; o = o.nextOverload) {
                            if (fn.hasSameSignatureAs(o)) { baseFn = o; break; }
                        }
                    }
                    if (baseFn == null) continue;   // sobrecarga nueva, no override
                    if (baseFn.isFinal)
                        err(fn.line, fn.column, "'" + fn.name + "' está marcada 'final' en la clase base, no se puede sobreescribir");
                    String bret = baseFn.returnType != null ? baseFn.returnType.display() : "void";
                    String fret = fn.returnType   != null ? fn.returnType.display()   : "void";
                    if (!bret.equals(fret))
                        err(fn.line, fn.column, "sobreescritura de '" + fn.name + "' cambia el tipo de retorno ('" + bret + "' vs '" + fret + "')");
                }
            } else if (s instanceof PropertySymbol) {
                PropertySymbol pp = (PropertySymbol) s;
                Symbol baseSym = cls.baseClass.lookupInstance(pp.name);
                if (baseSym instanceof PropertySymbol && ((PropertySymbol) baseSym).isFinal)
                    err(pp.line, pp.column, "propiedad '" + pp.name + "' es final en la base, no se puede sobreescribir");
            } else if (s instanceof VarSymbol && !((VarSymbol) s).isStatic) {
                // 14-sep — un campo no puede REDECLARAR uno protegido de un ancestro: en
                // el mismo módulo lo cazaba el ModWriter sin línea («Campo ya declarado»)
                // y cruzando la frontera nadie: la hija obtenía un SEGUNDO campo con el
                // mismo nombre y `this.q` eran dos variables. Los private no se ven
                // (ni viajan), así que ahí sí se puede reutilizar el nombre.
                VarSymbol vv = (VarSymbol) s;
                for (ClassSymbol a = cls.baseClass; a != null; a = a.baseClass) {
                    Symbol as = a.instanceMembers.tryLookup(vv.name);
                    if (as instanceof VarSymbol && !((VarSymbol) as).isPrivate && !((VarSymbol) as).isStatic) {
                        err(vv.line, vv.column, "el campo '" + vv.name + "' ya existe en la clase base '" + a.name
                                + "' (protegido): no se puede redeclarar");
                        break;
                    }
                }
            }
        }
    }

    // ============================================================
    // VISIBILIDAD
    // ============================================================
    /** Un miembro estático, por el nombre de su clase (`A.m`): no hay objeto. */
    private void checkVisibility(int line, int col, Symbol sub, ClassSymbol owner) {
        checkVisibility(line, col, sub, owner, false);
    }

    /** {@code sobreObjeto}: se usa sobre un objeto (`x.m`, `obj::m`) y {@code owner} es la clase de ese objeto (#760);
     *  false si no hay objeto (lo estático) o si el objeto es `this` (`super.m`). */
    private void checkVisibility(int line, int col, Symbol sub, ClassSymbol owner, boolean sobreObjeto) {
        // V8 H7.3 — lo de una interfaz es público por definición: es lo que exporta quien la implementa
        if (sub instanceof FunctionSymbol && ((FunctionSymbol) sub).ownerClass != null
                && ((FunctionSymbol) sub).ownerClass.isInterface) return;
        if (sub instanceof PropertySymbol && ((PropertySymbol) sub).ownerClass != null
                && ((PropertySymbol) sub).ownerClass.isInterface) return;
        boolean isPublic;
        if      (sub instanceof FunctionSymbol) isPublic = ((FunctionSymbol) sub).isPublic;
        else if (sub instanceof VarSymbol)      isPublic = ((VarSymbol) sub).isPublic;
        else if (sub instanceof ConstSymbol)    isPublic = ((ConstSymbol) sub).isPublic;
        else if (sub instanceof PropertySymbol) isPublic = ((PropertySymbol) sub).isPublic;
        else                                    isPublic = true;

        // #390 — `private` es de su clase Y DE NADIE MÁS, ni de sus hijas. Se
        // comprueba ANTES del atajo de abajo, que es justo el que deja pasar a
        // las descendientes: sin esto, `private` no se distinguiría de no poner
        // nada. (El atajo, tal cual estaba, YA implementaba *protected* — sólo
        // que sin nombre y sin nada más estricto al lado.)
        // OJO: `owner` es la clase del RECEPTOR, no la que DECLARA el miembro. Con
        // `hija.metodoDeLaBase()` llega la HIJA, porque lookupInstance sube por la
        // cadena y devuelve el símbolo de la base. Esa diferencia no importaba
        // mientras «clase o descendiente» tuvieran los mismos permisos; con
        // `private` sí, porque es justo a la hija a quien hay que decirle que no.
        // Para eso se pregunta por la clase DECLARANTE (FunctionSymbol.ownerClass).
        // #508 — y lo mismo para campos y properties, que hasta el 14-sep no llevaban la
        // marca (el parser la tiraba) y una subclase leía un `private var` sin queja.
        boolean esPrivado;
        ClassSymbol declarante;
        if      (sub instanceof FunctionSymbol) { esPrivado = ((FunctionSymbol) sub).isPrivate; declarante = ((FunctionSymbol) sub).ownerClass; }
        else if (sub instanceof VarSymbol)      { esPrivado = ((VarSymbol) sub).isPrivate;      declarante = ((VarSymbol) sub).ownerClass; }
        else if (sub instanceof PropertySymbol) { esPrivado = ((PropertySymbol) sub).isPrivate; declarante = ((PropertySymbol) sub).ownerClass; }
        else                                    { esPrivado = false; declarante = null; }
        if (esPrivado) {
            if (currentClass == null || currentClass != declarante) {
                err(line, col, "'" + sub.name + "' es private de '"
                        + (declarante != null ? declarante.name : "?") + "': sólo lo usa"
                        + " esa clase. Si quieres que también lo usen sus descendientes,"
                        + " quítale 'private' — un miembro sin marcar ya es protegido.");
            }
            return;
        }

        if (isPublic) return;
        // #760 (Eduardo, 1-nov) — lo protegido es de la familia, y se pregunta por la clase que lo DECLARA, como con
        // `private`: lo usan esa clase y sus descendientes. Y sobre un objeto (`x.m`), además, el objeto ha de ser de la
        // clase que accede o de una hija suya: en un C, hija de B, el campo de A es un campo de B; en un objeto que es
        // sólo un A, es el de otra clase, y se pide por su getter (la regla de Java). Hasta hoy se preguntaba por la
        // clase del objeto —«¿B desciende de C?»—: no dejaba lo de una hija, y sí lo del padre.
        ClassSymbol clase = (declarante != null) ? declarante : owner;
        boolean deLaFamilia = currentClass != null && clase != null
                && (currentClass == clase || currentClass.isSubclassOf(clase));
        boolean sobreUnoSuyo = !sobreObjeto || owner == null
                || owner == currentClass || owner.isSubclassOf(currentClass);
        if (deLaFamilia && sobreUnoSuyo) return;
        if (deLaFamilia)
            err(line, col, "'" + sub.name + "' es protegido de '" + clase.name + "': desde '" + currentClass.name
                    + "' se usa sobre un '" + currentClass.name + "' o una hija suya, y este objeto es un '"
                    + owner.name + "', de otra clase. " + ((sub instanceof VarSymbol)
                        ? "Léelo por una property pública suya."
                        : "Hazlo 'public' si tiene que usarse sobre otros objetos."));
        else
            err(line, col, "'" + sub.name + "' es protegido de '"
                    + (clase != null ? clase.name : "?") + "': lo ven esa clase y sus"
                    + " descendientes, no desde fuera. " + ((sub instanceof VarSymbol)
                        ? "Léelo por una property pública suya (un campo no puede ser public)."
                        : "Hazlo 'public' si tiene que usarse desde cualquier sitio."));
    }

    // ============================================================
    // CONFORMIDAD CON INTERFAZ (`module X implements Lib.Iface`)
    // ============================================================
    /**
     * La interfaz es un contrato MÍNIMO. Por cada símbolo declarado en la
     * interfaz, el módulo impl tiene que:
     *   - exponer ese símbolo (función / const / enum) con el mismo nombre,
     *   - declararlo `public` (la visibilidad del impl tiene que ser igual
     *     a la de la interfaz; cualquier visibilidad más estricta — es
     *     decir, privada — rompe el contrato cross-module),
     *   - usar tipos compatibles (parámetros y retorno equivalentes en
     *     funciones, mismo tipo en consts, mismos miembros y valores en
     *     enums).
     *
     * El impl PUEDE además exponer libremente otras funciones, consts o
     * enums públicas adicionales no exigidas por la interfaz — son sus
     * "extras" propios — y puede tener cuantos helpers privados quiera.
     * Ese conjunto no se verifica: el contrato sólo fija el mínimo.
     */
    private void verifyImplementsContract(ModuleNode mod) {
        if (implementsContract == null) return;
        if (mod.implementsName == null) return;
        if (!implementsContract.isInterface) {
            err(mod.line, mod.column,
                "'" + mod.implementsName + "' no es una interfaz (no se declaró 'module interface')");
            return;
        }

        // 1) Funciones declaradas en la interfaz: cada una debe existir en el
        //    módulo con misma signatura.
        for (ModuleInterface.FuncSig req : implementsContract.functions) {
            Symbol s = module.members.tryLookup(req.name);
            if (!(s instanceof FunctionSymbol)) {
                err(mod.line, mod.column,
                    "no implementa la función '" + req.name + "' requerida por '" + mod.implementsName + "'");
                continue;
            }
            FunctionSymbol fs = (FunctionSymbol) s;
            if (!fs.isPublic) {
                err(fs.line, fs.column,
                    "la función '" + fs.name + "' requerida por '" + mod.implementsName + "' debe ser public");
            }
            if (fs.params.size() != req.params.size()) {
                err(fs.line, fs.column, "la función '" + fs.name + "': se esperan "
                        + req.params.size() + " params, declarados " + fs.params.size());
                continue;
            }
            for (int i = 0; i < req.params.size(); i++) {
                BpType wanted = req.params.get(i).type;
                BpType got    = fs.params.get(i).type;
                if (wanted != null && got != null && !wanted.sameAs(got)) {
                    err(fs.line, fs.column, "la función '" + fs.name + "': param " + (i + 1)
                            + " es '" + got.display() + "', se esperaba '" + wanted.display() + "'");
                }
            }
            // returnType: null en .bpi/Sig representa void.
            BpType wantedRet = req.returnType;
            BpType gotRet    = fs.returnType;
            boolean retOk = (wantedRet == null && (gotRet == null || gotRet instanceof VoidType))
                         || (wantedRet != null && gotRet != null && wantedRet.sameAs(gotRet));
            if (!retOk) {
                String w = (wantedRet == null) ? "void" : wantedRet.display();
                String g = (gotRet == null) ? "void" : gotRet.display();
                err(fs.line, fs.column, "la función '" + fs.name + "': retorno declarado '" + g
                        + "', se esperaba '" + w + "'");
            }
        }

        // 2) Constantes declaradas en la interfaz.
        for (ModuleInterface.ConstSig req : implementsContract.consts) {
            Symbol s = module.members.tryLookup(req.name);
            if (!(s instanceof ConstSymbol)) {
                err(mod.line, mod.column,
                    "no implementa la constante '" + req.name + "' requerida por '" + mod.implementsName + "'");
                continue;
            }
            ConstSymbol cs = (ConstSymbol) s;
            if (!cs.isPublic) {
                err(cs.line, cs.column, "la constante '" + cs.name + "' debe ser public");
            }
            if (cs.type != null && req.type != null && !req.type.sameAs(cs.type)) {
                err(cs.line, cs.column, "la constante '" + cs.name + "': tipo '"
                        + cs.type.display() + "', se esperaba '" + req.type.display() + "'");
            }
        }

        // 3) Enums declarados en la interfaz: mismo nombre, mismo conjunto de
        //    miembros con mismos valores.
        for (ModuleInterface.EnumSig req : implementsContract.enums) {
            Symbol s = module.members.tryLookup(req.name);
            if (!(s instanceof EnumSymbol)) {
                err(mod.line, mod.column,
                    "no implementa el enum '" + req.name + "' requerido por '" + mod.implementsName + "'");
                continue;
            }
            EnumSymbol es = (EnumSymbol) s;
            if (!es.isPublic) {
                err(es.line, es.column, "el enum '" + es.name + "' debe ser public");
            }
            for (java.util.Map.Entry<String, Long> kv : req.values.entrySet()) {
                Long v = es.values.get(kv.getKey());
                if (v == null) {
                    err(es.line, es.column, "el enum '" + es.name
                            + "' no tiene el miembro '" + kv.getKey() + "' requerido");
                } else if (!v.equals(kv.getValue())) {
                    err(es.line, es.column, "el enum '" + es.name
                            + "." + kv.getKey() + "' vale " + v + ", se esperaba " + kv.getValue());
                }
            }
        }
    }

    // ============================================================
    // ENTRY POINTS
    // ============================================================
    // ============================================================
    // V5/H5 — ANOTACIONES  (`@BD{ ... }`)
    // ============================================================
    //
    // El parser acepta una anotación delante de CUALQUIER declaración y no
    // interpreta las claves. Toda la semántica está aquí, y por dos motivos:
    //
    //   · un `@BD` en el sitio equivocado da un mensaje que explica el
    //     problema, en vez de un error de sintaxis que no dice nada;
    //   · añadir una clave nueva es tocar esta función y nada más — la
    //     gramática no se entera.
    //
    // ⚠️ Esto corre SIEMPRE, se vaya a generar un DAO o no. Si sólo se validara
    // al generar, un `@BD{ tabl = "medidas" }` mal escrito no se vería hasta la
    // primera generación — o peor, saldría un DAO con la tabla vacía.

    private static final String ANN_BD = "BD";
    /** Claves que hoy admite `@BD` sobre una CLASE. */
    private static final Set<String> BD_CLAVES_CLASE = new HashSet<>(java.util.Arrays.asList("tabla"));
    /** Claves que hoy admite `@BD` sobre una PROPERTY.
     *
     *  `pk` es una BANDERA (sin valor) y marca la clave primaria. Está en
     *  notación de base de datos —criterio de Eduardo— y no traducida, porque
     *  PK es lo que pone en cualquier esquema. */
    private static final Set<String> BD_CLAVES_PROP  = new HashSet<>(java.util.Arrays.asList("columna", "pk"));

    /** V8 H7.3 (`#756`) — `@implement{ A, B }`: el método (o la property) implementa uno de esas interfaces; es
     *  obligatoria en lo que implementa una interfaz de la cabecera de su clase. `@override{}`: redefine uno del padre;
     *  opcional («es mas claro y los que vengan de Java lo podrian hechar de menos», Eduardo). Aquí sólo el sitio; lo
     *  que dicen se comprueba con los tipos resueltos ({@link #checkInterfaces}). */
    private static final String ANN_IMPLEMENT = "implement";
    private static final String ANN_OVERRIDE = "override";

    private void validateAnnotations(ModuleNode mod) {
        for (ITopLevelDecl def : mod.defs) {
            if (def instanceof ClassDef) {
                ClassDef cd = (ClassDef) def;
                Ast.Annotation bdClase = null;
                for (Ast.Annotation a : cd.annotations) {
                    if (!nombreConocido(a) || !unaVez(cd, a)) continue;
                    if (ANN_BD.equals(a.name)) bdClase = a;
                    else err(a.line, a.column, "'@" + a.name + "' va sobre un método o una property de una clase, no"
                            + " sobre la " + (cd.isInterface ? "interfaz" : "clase"));
                }
                boolean esEntidad = bdClase != null;
                if (esEntidad) validarBdEnClase(cd, bdClase);
                List<PropertyDef> pks = new ArrayList<>();
                List<Ast.Annotation> pksAnn = new ArrayList<>();
                for (ITopLevelDecl m : cd.members) {
                    if (!(m instanceof Node)) continue;
                    Node n = (Node) m;
                    for (Ast.Annotation a : n.annotations) {
                        if (!nombreConocido(a) || !unaVez(n, a)) continue;
                        if (!ANN_BD.equals(a.name)) {
                            if (!(m instanceof FuncDef) && !(m instanceof PropertyDef))
                                err(a.line, a.column, "'@" + a.name + "' va sobre un método o una property");
                            else if (cd.isInterface)
                                err(a.line, a.column, "'@" + a.name + "' va en la clase que implementa, no en la interfaz");
                            continue;
                        }
                        if (!(m instanceof PropertyDef)) {
                            err(a.line, a.column, "dentro de una clase, '@" + a.name
                                    + "' sólo puede ir sobre una `property`");
                            continue;
                        }
                        PropertyDef p = (PropertyDef) m;
                        validarBdEnProperty(p, a, cd, esEntidad);
                        if (esEntidad && a.get("pk") != null) { pks.add(p); pksAnn.add(a); }
                    }
                }
                // Una entidad necesita saber cuál es su clave primaria: sin ella
                // no hay `loadById`, ni `update`, ni `delete`. Y sólo UNA: las
                // claves compuestas no están soportadas, y decirlo aquí es mejor
                // que emitir un DAO que hace algo raro con la segunda.
                if (esEntidad && pks.isEmpty()) {
                    err(bdClase.line, bdClase.column, "la entidad '" + cd.name
                            + "' no dice cuál es su clave primaria: marca una property con `@BD{ pk }`");
                } else if (pks.size() > 1) {
                    for (int i = 1; i < pks.size(); i++) {
                        Ast.Annotation extra = pksAnn.get(i);
                        err(extra.line, extra.column, "'" + cd.name
                                + "' ya marcó '" + pks.get(0).name.name + "' como `pk`; las claves compuestas"
                                + " todavía no están soportadas");
                    }
                }
            } else if (def instanceof Node) {
                for (Ast.Annotation a : ((Node) def).annotations) {
                    if (!nombreConocido(a)) continue;
                    if (ANN_BD.equals(a.name))
                        err(a.line, a.column, "'@" + a.name
                                + "' sólo puede ir sobre una `class` (la entidad) o sobre una `property` suya");
                    else
                        err(a.line, a.column, "'@" + a.name + "' va sobre un método o una property de una clase");
                }
            }
        }
    }

    /** V8 H7.3 — cada anotación, una vez por declaración. */
    private boolean unaVez(Node n, Ast.Annotation a) {
        for (Ast.Annotation b : n.annotations) {
            if (b == a) return true;
            if (b.name.equals(a.name)) {
                err(a.line, a.column, "'@" + a.name + "' repetida: una vez por declaración");
                return false;
            }
        }
        return true;
    }

    private boolean nombreConocido(Ast.Annotation a) {
        if (ANN_BD.equals(a.name) || ANN_IMPLEMENT.equals(a.name) || ANN_OVERRIDE.equals(a.name)) return true;
        err(a.line, a.column, "anotación desconocida '@" + a.name
                + "'; las que existen son '@" + ANN_BD + "', '@" + ANN_IMPLEMENT + "' y '@" + ANN_OVERRIDE + "'");
        return false;
    }

    private void validarBdEnClase(ClassDef cd, Ast.Annotation a) {
        clavesConocidas(a, BD_CLAVES_CLASE, "una clase");

        Ast.AnnEntry tabla = a.get("tabla");
        if (tabla == null) {
            err(a.line, a.column, "'@BD' sobre la clase '" + cd.name
                    + "' tiene que decir a qué tabla corresponde: `@BD{ tabla = \"...\" }`");
        } else if (!(tabla.value instanceof String)) {
            err(tabla.line, tabla.column, "'tabla' tiene que ser una cadena, por ejemplo `tabla = \"medidas\"`");
        } else if (((String) tabla.value).isEmpty()) {
            err(tabla.line, tabla.column, "'tabla' no puede estar vacía");
        }
    }

    private void validarBdEnProperty(PropertyDef p, Ast.Annotation a, ClassDef cd, boolean claseEsEntidad) {
        // Una propiedad no puede mapear a una columna si su clase no es una
        // entidad: no hay tabla contra la que mapear.
        if (!claseEsEntidad) {
            err(a.line, a.column, "'@BD' sobre '" + p.name.name + "' no tiene sentido: su clase '"
                    + cd.name + "' no está marcada como entidad. Añade `@BD{ tabla = \"...\" }` a la clase");
            return;
        }
        if (p.name.isStatic()) {
            err(a.line, a.column, "'@BD' no vale en una property estática ('" + p.name.name
                    + "'): una columna pertenece a la FILA, no a la clase");
            return;
        }

        clavesConocidas(a, BD_CLAVES_PROP, "una property");

        Ast.AnnEntry col = a.get("columna");
        if (col != null) {
            if (!(col.value instanceof String))
                err(col.line, col.column, "'columna' tiene que ser una cadena, por ejemplo `columna = \"sensor_id\"`");
            else if (((String) col.value).isEmpty())
                err(col.line, col.column, "'columna' no puede estar vacía");
        }
        // Sin `columna`, la columna se llama como la property. Es deliberado:
        // nada de convertir `sensorId` en `sensor_id` por nuestra cuenta — una
        // regla más que recordar y que muerde cuando no la esperas.
    }

    /** Señala las claves que esta anotación no admite, una por una y en su sitio. */
    private void clavesConocidas(Ast.Annotation a, Set<String> validas, String donde) {
        for (Ast.AnnEntry e : a.entries) {
            if (validas.contains(e.key)) continue;
            err(e.line, e.column, "'@" + a.name + "' sobre " + donde + " no conoce la clave '" + e.key
                    + "'; hoy admite: " + String.join(", ", validas));
        }
    }

    private void validateModuleEntryPoints(ModuleNode mod) {
        // Las interfaces son contratos puros: sin main, sin entry-point. No
        // tiene sentido avisar de su ausencia.
        if (mod.isInterface) return;
        if (module.mainFunction == null) {
            warn(mod.line, mod.column,
                "el módulo '" + mod.name + "' no define 'main' ni 'Main' (no podrá ser módulo principal)");
            return;
        }
        FunctionSymbol m = module.mainFunction;
        // Formas válidas:
        //   - 'Main' legacy: sin parámetros
        //   - 'main' nuevo: cero o un parámetro de tipo string
        if (m.params.size() > 1) {
            err(m.line, m.column, "'" + m.name + "' acepta como mucho un parámetro (de tipo string)");
        } else if (m.params.size() == 1) {
            BpType pt = m.params.get(0).type;
            if (pt == null || !PrimitiveType.STRING.sameAs(pt)) {
                err(m.line, m.column,
                    "'" + m.name + "' sólo admite un parámetro de tipo 'string' (recibido '"
                        + (pt == null ? "<null>" : pt.display()) + "')");
            }
            // V8 #764 (2-nov) — su valor por defecto tiene que caber en el argumento de ejecución: el tope de las dos VMs
            // (ModFormat.RUN_ARG_MAX, en bytes de UTF-8). Antes la VM-C lo cortaba sin decirlo, y la VM-Java no.
            ParamSymbol p0 = m.params.get(0);
            if (p0.defaultExpr instanceof StringLitExpr) {
                int n = ((StringLitExpr) p0.defaultExpr).value.getBytes(java.nio.charset.StandardCharsets.UTF_8).length;
                if (n > edu.bpgenvm.bytecode.ModFormat.RUN_ARG_MAX)
                    err(((StringLitExpr) p0.defaultExpr).line, ((StringLitExpr) p0.defaultExpr).column,
                        "el valor por defecto de '" + p0.name + "' no cabe: " + n + " bytes (el tope del argumento es "
                            + edu.bpgenvm.bytecode.ModFormat.RUN_ARG_MAX + ")");
            }
        }
        if (m.returnType != null && !(m.returnType instanceof VoidType))
            err(m.line, m.column, "'" + m.name + "' no puede declarar tipo de retorno");
    }

    // ============================================================
    // Diagnostics helpers
    // ============================================================
    private void err(int line, int col, String msg) {
        info.diagnostics.add(new SemanticDiagnostic(SemanticDiagnostic.Kind.ERROR, msg, line, col));
    }
    /** #516 — un error detectado al IMPORTAR (una firma ajena con un enum de un módulo que
     *  este no importa): va por el mismo canal que los semánticos, sin línea. */
    public void errorDeImport(String msg) {
        info.diagnostics.add(new SemanticDiagnostic(SemanticDiagnostic.Kind.ERROR, msg, 0, 0));
    }

    private void warn(int line, int col, String msg) {
        info.diagnostics.add(new SemanticDiagnostic(SemanticDiagnostic.Kind.WARNING, msg, line, col));
    }
}
