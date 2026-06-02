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

    private final SemanticInfo info = new SemanticInfo();
    private ModuleSymbol module;

    // ---------- Estado del recorrido (pase 3) ----------
    private ClassSymbol currentClass;
    private FunctionSymbol currentFunction;
    private boolean insideGetter;
    private boolean insideSetter;
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
    private boolean seenSuperCallInCtor;
    private boolean isFirstStmtOfCtor;

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
        declarePass(mod);
        injectImports(mod);
        resolveTypesPass(mod);
        bodyPass(mod);
        validateModuleEntryPoints(mod);
        verifyImplementsContract(mod);
        return info;
    }

    /**
     * Modo "interface-only": parsea + analiza solamente los pases que dependen
     * de declaraciones (no de bodies). Suficiente para extraer la firma
     * pública del módulo y emitir un .bpi sin necesidad de tener resueltos
     * los imports. No corre bodyPass ni valida entry points.
     */
    public SemanticInfo analyzeInterface(ModuleNode mod) {
        declarePass(mod);
        // No inyectamos imports: en modo interfaz no se procesan bodies y
        // los Foo.bar() internos no llegan a re-analizarse aquí.
        resolveTypesPass(mod);
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

        // class RuntimeError
        ClassSymbol rt = new ClassSymbol("RuntimeError", true, null, null, 0, 0);
        FunctionSymbol rtCtor = new FunctionSymbol("RuntimeError", true, false, false, rt, null);
        ParamSymbol msg = new ParamSymbol("msg", 0, 0);
        msg.type = PrimitiveType.STRING;
        rtCtor.params.add(msg);
        rtCtor.returnType = null;
        rtCtor.isConstructor = true;
        rt.constructor = rtCtor;
        rt.instanceMembers.tryDefine(rtCtor);
        // Field público `msg: string` — la clase sintetizada por MivmEmitter
        // lo expone (set en el constructor); aquí lo declaramos también
        // para que `catch e: RuntimeError ... print e.msg` typechee.
        VarSymbol rtMsg = new VarSymbol("msg", true, false, rt, false, 0, 0);
        rtMsg.type = PrimitiveType.STRING;
        rt.instanceMembers.tryDefine(rtMsg);
        s.tryDefine(rt);

        // input(): string
        FunctionSymbol input = new FunctionSymbol("input", true, false, true, null, null);
        input.returnType = PrimitiveType.STRING;
        s.tryDefine(input);

        // parseInt(s: string): integer
        FunctionSymbol pi = new FunctionSymbol("parseInt", true, false, true, null, null);
        ParamSymbol piA = new ParamSymbol("s", 0, 0); piA.type = PrimitiveType.STRING;
        pi.params.add(piA);
        pi.returnType = PrimitiveType.INTEGER;
        s.tryDefine(pi);

        // parseFloat(s: string): float
        FunctionSymbol pf = new FunctionSymbol("parseFloat", true, false, true, null, null);
        ParamSymbol pfA = new ParamSymbol("s", 0, 0); pfA.type = PrimitiveType.STRING;
        pf.params.add(pfA);
        pf.returnType = PrimitiveType.FLOAT;
        s.tryDefine(pf);

        // strlen(s: string): integer
        addBuiltin(s, "strlen", PrimitiveType.INTEGER, new String[]{"s"}, new BpType[]{PrimitiveType.STRING});

        // ---- Numéricas integer ----
        addBuiltin(s, "abs",  PrimitiveType.INTEGER, new String[]{"x"},      new BpType[]{PrimitiveType.INTEGER});
        addBuiltin(s, "min",  PrimitiveType.INTEGER, new String[]{"a","b"},  new BpType[]{PrimitiveType.INTEGER, PrimitiveType.INTEGER});
        addBuiltin(s, "max",  PrimitiveType.INTEGER, new String[]{"a","b"},  new BpType[]{PrimitiveType.INTEGER, PrimitiveType.INTEGER});

        // ---- Numéricas float ----
        addBuiltin(s, "sqrt",   PrimitiveType.FLOAT, new String[]{"x"},     new BpType[]{PrimitiveType.FLOAT});
        addBuiltin(s, "pow",    PrimitiveType.FLOAT, new String[]{"b","e"}, new BpType[]{PrimitiveType.FLOAT, PrimitiveType.FLOAT});
        addBuiltin(s, "random", PrimitiveType.FLOAT, new String[]{},        new BpType[]{});

        // ---- Conversión float → integer ----
        addBuiltin(s, "floor", PrimitiveType.INTEGER, new String[]{"x"}, new BpType[]{PrimitiveType.FLOAT});
        addBuiltin(s, "ceil",  PrimitiveType.INTEGER, new String[]{"x"}, new BpType[]{PrimitiveType.FLOAT});
        addBuiltin(s, "round", PrimitiveType.INTEGER, new String[]{"x"}, new BpType[]{PrimitiveType.FLOAT});

        // ---- Conversión a string ----
        addBuiltin(s, "intToString",   PrimitiveType.STRING, new String[]{"n"}, new BpType[]{PrimitiveType.INTEGER});
        addBuiltin(s, "floatToString", PrimitiveType.STRING, new String[]{"x"}, new BpType[]{PrimitiveType.FLOAT});
        addBuiltin(s, "boolToString",  PrimitiveType.STRING, new String[]{"b"}, new BpType[]{PrimitiveType.BOOLEAN});

        // ---- Strings ----
        addBuiltin(s, "upper",  PrimitiveType.STRING,  new String[]{"s"},                  new BpType[]{PrimitiveType.STRING});
        addBuiltin(s, "lower",  PrimitiveType.STRING,  new String[]{"s"},                  new BpType[]{PrimitiveType.STRING});
        addBuiltin(s, "trim",   PrimitiveType.STRING,  new String[]{"s"},                  new BpType[]{PrimitiveType.STRING});
        addBuiltin(s, "substring",  PrimitiveType.STRING,  new String[]{"s","start","end"}, new BpType[]{PrimitiveType.STRING, PrimitiveType.INTEGER, PrimitiveType.INTEGER});
        addBuiltin(s, "indexOf",    PrimitiveType.INTEGER, new String[]{"s","target"},      new BpType[]{PrimitiveType.STRING, PrimitiveType.STRING});
        addBuiltin(s, "startsWith", PrimitiveType.BOOLEAN, new String[]{"s","prefix"},      new BpType[]{PrimitiveType.STRING, PrimitiveType.STRING});
        addBuiltin(s, "endsWith",   PrimitiveType.BOOLEAN, new String[]{"s","suffix"},      new BpType[]{PrimitiveType.STRING, PrimitiveType.STRING});
        addBuiltin(s, "contains",   PrimitiveType.BOOLEAN, new String[]{"s","sub"},         new BpType[]{PrimitiveType.STRING, PrimitiveType.STRING});
        addBuiltin(s, "charAt",     PrimitiveType.STRING,  new String[]{"s","i"},           new BpType[]{PrimitiveType.STRING, PrimitiveType.INTEGER});

        // ---- Strings extra ----
        BpType STRING_ARRAY = new ArrayType(PrimitiveType.STRING);
        addBuiltin(s, "split",   STRING_ARRAY,         new String[]{"s","sep"},                         new BpType[]{PrimitiveType.STRING, PrimitiveType.STRING});
        addBuiltin(s, "replace", PrimitiveType.STRING, new String[]{"s","target","replacement"},        new BpType[]{PrimitiveType.STRING, PrimitiveType.STRING, PrimitiveType.STRING});

        // ---- Matemáticas extra (float) ----
        //   `exp` queda implícito (pareja natural con muchos usos cotidianos).
        //   sin/cos/tan/pi/e/ln/log10 se MOVIERON a Math.mod como intrínsecos
        //   (decisión: las matemáticas de "namespace académico" mejor con import).
        addBuiltin(s, "exp",       PrimitiveType.FLOAT, new String[]{"x"}, new BpType[]{PrimitiveType.FLOAT});
        addBuiltin(s, "randomInt", PrimitiveType.INTEGER, new String[]{"lo","hi"}, new BpType[]{PrimitiveType.INTEGER, PrimitiveType.INTEGER});

        // ---- Tiempo ----
        addBuiltin(s, "now",      PrimitiveType.INTEGER, new String[]{},     new BpType[]{});
        addBuiltin(s, "sleep",    VoidType.INSTANCE,     new String[]{"ms"}, new BpType[]{PrimitiveType.INTEGER});
        addBuiltin(s, "sleepSec", VoidType.INSTANCE,     new String[]{"s"},  new BpType[]{PrimitiveType.INTEGER});
        addBuiltin(s, "sleepUs",  VoidType.INSTANCE,     new String[]{"us"}, new BpType[]{PrimitiveType.INTEGER});

        // ---- Debug ----
        addBuiltin(s, "gc",    VoidType.INSTANCE,     new String[]{},     new BpType[]{});

        // ---- I/O archivos ----
        addBuiltin(s, "readFile",   PrimitiveType.STRING,  new String[]{"path"},           new BpType[]{PrimitiveType.STRING});
        addBuiltin(s, "writeFile",  VoidType.INSTANCE,     new String[]{"path","content"}, new BpType[]{PrimitiveType.STRING, PrimitiveType.STRING});
        addBuiltin(s, "appendFile", VoidType.INSTANCE,     new String[]{"path","content"}, new BpType[]{PrimitiveType.STRING, PrimitiveType.STRING});
        addBuiltin(s, "fileExists", PrimitiveType.BOOLEAN, new String[]{"path"},           new BpType[]{PrimitiveType.STRING});
        addBuiltin(s, "listDir",    STRING_ARRAY,          new String[]{"path"},           new BpType[]{PrimitiveType.STRING});

        // ---- L10: casts de tipos estrechos ----
        // `byte(i)`, `int8(i)`, `word(i)`, `int16(i)`: tomar un integer
        // y devolver el tipo estrecho correspondiente. La VM hace check
        // de rango en runtime (I32_TO_{U8,I8,U16,I16} → BpThreadFault si
        // se sale). El SemanticAnalyzer pre-evalúa literales para
        // dar el error en compile-time cuando es posible. `short` es
        // alias gramatical de `int16` y se normaliza en el parser.
        addBuiltin(s, "byte",  PrimitiveType.UINT8,
                new String[]{"i"}, new BpType[]{PrimitiveType.INTEGER});
        addBuiltin(s, "int8",  PrimitiveType.INT8,
                new String[]{"i"}, new BpType[]{PrimitiveType.INTEGER});
        addBuiltin(s, "word",  PrimitiveType.UINT16,
                new String[]{"i"}, new BpType[]{PrimitiveType.INTEGER});
        addBuiltin(s, "int16", PrimitiveType.INT16,
                new String[]{"i"}, new BpType[]{PrimitiveType.INTEGER});

        // ---- Arrays ----
        // move(src, dst, srcStart, dstStart, count) → void
        // Polimórfico: src y dst pueden ser arrays de cualquier tipo (i8/i16/i32/ref).
        // La VM valida en runtime que ambos sean arrays del MISMO tipo de elemento.
        // Usamos `any` para no forzar inferencia de un tipo de elemento concreto en el
        // frontend; el chequeo estructural lo hace el runtime.
        addBuiltin(s, "move", VoidType.INSTANCE,
                new String[]{"src","dst","srcStart","dstStart","count"},
                new BpType[]{AnyType.INSTANCE, AnyType.INSTANCE,
                             PrimitiveType.INTEGER, PrimitiveType.INTEGER, PrimitiveType.INTEGER});

        // ---- Soporte stdlib (List, StringBuilder). Los nombres con __ son
        //      internal-ish: BP los puede llamar pero típicamente sólo lo
        //      hacen las clases sintetizadas. ----
        BpType INT_ARRAY = new ArrayType(PrimitiveType.INTEGER);
        addBuiltin(s, "__newRefArray",  INT_ARRAY,           new String[]{"cap"},           new BpType[]{PrimitiveType.INTEGER});
        addBuiltin(s, "__growRefArray", INT_ARRAY,           new String[]{"old","newCap"},  new BpType[]{INT_ARRAY, PrimitiveType.INTEGER});
        addBuiltin(s, "__growIntArray", INT_ARRAY,           new String[]{"old","newCap"},  new BpType[]{INT_ARRAY, PrimitiveType.INTEGER});
        // Versión llamable desde user BP (sin __): aloca int array de tamaño `size`
        // inicializado a ceros. Equivalente a __growIntArray(null, size).
        addBuiltin(s, "newIntArray",    INT_ARRAY,           new String[]{"size"},          new BpType[]{PrimitiveType.INTEGER});
        // H1.1 (V2) — byte[] first-class: aloca un byte[] (UINT8, 1 byte/elem,
        // zero-init). El emisor lo traduce a NEWARRAY_I8 inline (la VM ya lo
        // implementa; sin builtin nuevo). Paralelo a newIntArray.
        BpType BYTE_ARRAY = new ArrayType(PrimitiveType.UINT8);
        addBuiltin(s, "newByteArray",   BYTE_ARRAY,          new String[]{"size"},          new BpType[]{PrimitiveType.INTEGER});
        // H2 (V2) — conversión string <-> byte[] (UTF-8). string y byte[] son
        // el mismo layout TYPE_ARRAY_I8; la conversión copia los bytes a un
        // objeto nuevo (defensiva, por la inmutabilidad del string).
        //   toBytes(s): byte[]   — los bytes UTF-8 del string.
        //   fromBytes(b): string — interpreta los bytes como UTF-8.
        addBuiltin(s, "toBytes",   BYTE_ARRAY,           new String[]{"s"}, new BpType[]{PrimitiveType.STRING});
        addBuiltin(s, "fromBytes", PrimitiveType.STRING, new String[]{"b"}, new BpType[]{BYTE_ARRAY});
        // H3 — diagnóstico de heap (SOLO VM-Java; mide fragmentación del GC).
        addBuiltin(s, "heapFrag",  PrimitiveType.STRING, new String[]{},      new BpType[]{});
        addBuiltin(s, "heapMap",   PrimitiveType.STRING, new String[]{"cols"}, new BpType[]{PrimitiveType.INTEGER});
        // H1.2 (V2) — long[]: aloca un long[] (8 bytes/elem, zero-init).
        // El emisor lo traduce a NEWARRAY_I64 inline (la VM ya lo implementa).
        BpType LONG_ARRAY = new ArrayType(PrimitiveType.LONG);
        addBuiltin(s, "newLongArray",   LONG_ARRAY,          new String[]{"size"},          new BpType[]{PrimitiveType.INTEGER});
        // H1.3 (V2) — double[]: aloca un double[] (8 bytes/elem, zero-init).
        // Reusa NEWARRAY_I64 (almacenamiento opaco de 8 bytes).
        BpType DOUBLE_ARRAY = new ArrayType(PrimitiveType.DOUBLE);
        addBuiltin(s, "newDoubleArray", DOUBLE_ARRAY,        new String[]{"size"},          new BpType[]{PrimitiveType.INTEGER});
        addBuiltin(s, "__charsToString", PrimitiveType.STRING, new String[]{"chars","len"},   new BpType[]{INT_ARRAY, PrimitiveType.INTEGER});
        addBuiltin(s, "charCodeAt",     PrimitiveType.INTEGER, new String[]{"s","i"},          new BpType[]{PrimitiveType.STRING, PrimitiveType.INTEGER});

        // ---- Threading ----
        addBuiltin(s, "yield",          VoidType.INSTANCE,     new String[]{},                 new BpType[]{});
        addBuiltin(s, "__threadStart",  VoidType.INSTANCE,     new String[]{"t"},              new BpType[]{AnyType.INSTANCE});
        addBuiltin(s, "__threadJoin",   VoidType.INSTANCE,     new String[]{"t"},              new BpType[]{AnyType.INSTANCE});

        // ---- Sync (Mutex) ----
        addBuiltin(s, "__mutexCreate",  PrimitiveType.INTEGER, new String[]{},                 new BpType[]{});
        addBuiltin(s, "__mutexLock",    VoidType.INSTANCE,     new String[]{"m"},              new BpType[]{AnyType.INSTANCE});
        addBuiltin(s, "__mutexUnlock",  VoidType.INSTANCE,     new String[]{"m"},              new BpType[]{AnyType.INSTANCE});

        // ---- Clase stdlib: List (lista dinámica de refs a objetos) ----
        // Las firmas que toman/devuelven `any` permiten almacenar instancias
        // de cualquier clase de usuario sin fricción de tipos.
        ClassSymbol listCls = new ClassSymbol("List", true, null, null, 0, 0);
        {
            FunctionSymbol ctor = makeMethod("List", listCls, null, true);
            ctor.isConstructor = true;
            listCls.constructor = ctor;
            listCls.instanceMembers.tryDefine(ctor);
            listCls.instanceMembers.tryDefine(makeMethod("add",    listCls, VoidType.INSTANCE,
                    new String[]{"item"}, new BpType[]{AnyType.INSTANCE}));
            listCls.instanceMembers.tryDefine(makeMethod("get",    listCls, AnyType.INSTANCE,
                    new String[]{"idx"}, new BpType[]{PrimitiveType.INTEGER}));
            listCls.instanceMembers.tryDefine(makeMethod("set",    listCls, VoidType.INSTANCE,
                    new String[]{"idx","item"}, new BpType[]{PrimitiveType.INTEGER, AnyType.INSTANCE}));
            listCls.instanceMembers.tryDefine(makeMethod("length", listCls, PrimitiveType.INTEGER,
                    new String[]{}, new BpType[]{}));
            listCls.instanceMembers.tryDefine(makeMethod("remove", listCls, VoidType.INSTANCE,
                    new String[]{"idx"}, new BpType[]{PrimitiveType.INTEGER}));
        }
        s.tryDefine(listCls);

        // ---- Clase stdlib: Thread (multitarea cooperativa) ----
        // Subclasea para tu worker, sobreescribe run(), llama start()/join().
        // El stackSize del constructor se reserva al hacer start(); 0 = default.
        ClassSymbol threadCls = new ClassSymbol("Thread", true, null, null, 0, 0);
        {
            FunctionSymbol ctor = makeMethod("Thread", threadCls, null,
                    new String[]{"stackSize"}, new BpType[]{PrimitiveType.INTEGER});
            ctor.isConstructor = true;
            threadCls.constructor = ctor;
            threadCls.instanceMembers.tryDefine(ctor);
            // El método run() es VIRTUAL: la subclase del usuario lo
            // sobreescribe con su lógica. Tiene que ser el PRIMER método de
            // la vtable (slot 0) por convención con __threadStart en la VM.
            threadCls.instanceMembers.tryDefine(makeMethod("run",   threadCls, VoidType.INSTANCE,
                    new String[]{}, new BpType[]{}));
            threadCls.instanceMembers.tryDefine(makeMethod("start", threadCls, VoidType.INSTANCE,
                    new String[]{}, new BpType[]{}));
            threadCls.instanceMembers.tryDefine(makeMethod("join",  threadCls, VoidType.INSTANCE,
                    new String[]{}, new BpType[]{}));
        }
        s.tryDefine(threadCls);

        // ---- Clase stdlib: Mutex (sincronización entre threads BP) ----
        // Construye uno con `Mutex()`. Llama lock() para tomarlo (se bloquea
        // si está tomado por otro thread); unlock() lo libera y despierta al
        // siguiente waiter. NO es reentrante: lock() dos veces desde el mismo
        // thread es error.
        ClassSymbol mutexCls = new ClassSymbol("Mutex", true, null, null, 0, 0);
        {
            FunctionSymbol ctor = makeMethod("Mutex", mutexCls, null, true);
            ctor.isConstructor = true;
            mutexCls.constructor = ctor;
            mutexCls.instanceMembers.tryDefine(ctor);
            mutexCls.instanceMembers.tryDefine(makeMethod("lock",   mutexCls, VoidType.INSTANCE,
                    new String[]{}, new BpType[]{}));
            mutexCls.instanceMembers.tryDefine(makeMethod("unlock", mutexCls, VoidType.INSTANCE,
                    new String[]{}, new BpType[]{}));
        }
        s.tryDefine(mutexCls);

        // ---- Clase stdlib: SyncList (lista thread-safe) ----
        // Hereda de List; cada método público está envuelto con lock/unlock
        // contra un Mutex por instancia. Útil cuando varios threads BP
        // comparten una colección. Para read-modify-write atómicos
        // compuestos (e.g. "si está vacía, añade"), el usuario debe seguir
        // tomando un Mutex propio: SyncList sólo garantiza atomicidad por
        // operación individual.
        ClassSymbol syncListCls = new ClassSymbol("SyncList", true, "List", null, 0, 0);
        syncListCls.baseClass = listCls;
        {
            FunctionSymbol ctor = makeMethod("SyncList", syncListCls, null, true);
            ctor.isConstructor = true;
            syncListCls.constructor = ctor;
            syncListCls.instanceMembers.tryDefine(ctor);
            // Overrides explícitos (mismas firmas que List) para documentar
            // que el método del subtipo se llama (con locking).
            syncListCls.instanceMembers.tryDefine(makeMethod("add",    syncListCls, VoidType.INSTANCE,
                    new String[]{"item"}, new BpType[]{AnyType.INSTANCE}));
            syncListCls.instanceMembers.tryDefine(makeMethod("get",    syncListCls, AnyType.INSTANCE,
                    new String[]{"idx"}, new BpType[]{PrimitiveType.INTEGER}));
            syncListCls.instanceMembers.tryDefine(makeMethod("set",    syncListCls, VoidType.INSTANCE,
                    new String[]{"idx","item"}, new BpType[]{PrimitiveType.INTEGER, AnyType.INSTANCE}));
            syncListCls.instanceMembers.tryDefine(makeMethod("length", syncListCls, PrimitiveType.INTEGER,
                    new String[]{}, new BpType[]{}));
            syncListCls.instanceMembers.tryDefine(makeMethod("remove", syncListCls, VoidType.INSTANCE,
                    new String[]{"idx"}, new BpType[]{PrimitiveType.INTEGER}));
            // Específico de SyncList: pop atómico (queue-like).
            syncListCls.instanceMembers.tryDefine(makeMethod("pop",    syncListCls, AnyType.INSTANCE,
                    new String[]{}, new BpType[]{}));
            // N5: popBlocking espera hasta haber un item (yield-spin internamente).
            syncListCls.instanceMembers.tryDefine(makeMethod("popBlocking", syncListCls, AnyType.INSTANCE,
                    new String[]{}, new BpType[]{}));
        }
        s.tryDefine(syncListCls);

        // ---- Clase stdlib: OwnerList (List propietaria de sus elementos) ----
        // Hereda de List la API completa (add, get, set, length, remove). La
        // diferencia es semántica: al liberar la OwnerList (FREE_REF directo
        // o vía variable `owner`), se liberan también todos los objetos
        // contenidos. Implementado vía cascada en TYPE_ARRAY_REF + bitmap
        // owner del field `items` en el descriptor de OwnerList.
        ClassSymbol ownerListCls = new ClassSymbol("OwnerList", true, "List", null, 0, 0);
        ownerListCls.baseClass = listCls;
        {
            FunctionSymbol ctor = makeMethod("OwnerList", ownerListCls, null, true);
            ctor.isConstructor = true;
            ownerListCls.constructor = ctor;
            ownerListCls.instanceMembers.tryDefine(ctor);
            // Método propio: removeAndFree(idx) libera el item ANTES de
            // desplazar. Útil cuando se quiere quitar UN elemento concreto
            // sin destruir la lista entera.
            ownerListCls.instanceMembers.tryDefine(makeMethod("removeAndFree", ownerListCls, VoidType.INSTANCE,
                    new String[]{"idx"}, new BpType[]{PrimitiveType.INTEGER}));
            // El resto de métodos (add, get, set, length, remove) se heredan
            // del baseClass List.
        }
        s.tryDefine(ownerListCls);

        // ---- Clase stdlib: StringBuilder (construcción eficiente de strings) ----
        ClassSymbol sbCls = new ClassSymbol("StringBuilder", true, null, null, 0, 0);
        {
            FunctionSymbol ctor = makeMethod("StringBuilder", sbCls, null, true);
            ctor.isConstructor = true;
            sbCls.constructor = ctor;
            sbCls.instanceMembers.tryDefine(ctor);
            sbCls.instanceMembers.tryDefine(makeMethod("appendStr",  sbCls, VoidType.INSTANCE,
                    new String[]{"s"}, new BpType[]{PrimitiveType.STRING}));
            sbCls.instanceMembers.tryDefine(makeMethod("appendChar", sbCls, VoidType.INSTANCE,
                    new String[]{"c"}, new BpType[]{PrimitiveType.INTEGER}));
            sbCls.instanceMembers.tryDefine(makeMethod("appendInt",  sbCls, VoidType.INSTANCE,
                    new String[]{"n"}, new BpType[]{PrimitiveType.INTEGER}));
            sbCls.instanceMembers.tryDefine(makeMethod("toString",   sbCls, PrimitiveType.STRING,
                    new String[]{}, new BpType[]{}));
            sbCls.instanceMembers.tryDefine(makeMethod("length",     sbCls, PrimitiveType.INTEGER,
                    new String[]{}, new BpType[]{}));
        }
        s.tryDefine(sbCls);

        BUILTIN_SCOPE_CACHE = s;
        return s;
    }

    /**
     * Helper para crear un FunctionSymbol "fake" (sin AST) que represente un
     * método de una clase stdlib sintetizada.
     */
    private static FunctionSymbol makeMethod(String name, ClassSymbol owner, BpType returnType,
                                             String[] paramNames, BpType[] paramTypes) {
        FunctionSymbol fn = new FunctionSymbol(name, true, false, false, owner, null);
        fn.returnType = returnType;
        for (int i = 0; i < paramNames.length; i++) {
            ParamSymbol p = new ParamSymbol(paramNames[i], 0, 0);
            p.type = paramTypes[i];
            fn.params.add(p);
        }
        return fn;
    }

    /** Helper para el constructor: misma firma que makeMethod pero retorno = null. */
    private static FunctionSymbol makeMethod(String name, ClassSymbol owner, BpType returnType, boolean ctor) {
        FunctionSymbol fn = new FunctionSymbol(name, true, false, false, owner, null);
        fn.returnType = returnType;
        return fn;
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
        module = new ModuleSymbol(mod.name, builtinScope(), mod.line, mod.column);
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
        else if (decl instanceof ClassDef)     declareClass((ClassDef) decl);
        else if (decl instanceof EnumDef)      declareEnum((EnumDef) decl);
    }

    private void declareClass(ClassDef cd) {
        if (currentClass != null) {
            err(cd.line, cd.column, "no se permiten clases anidadas");
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
                Scope target = isStaticDecl(m) ? cls.staticMembers : cls.instanceMembers;
                declareTopLevel(m, cls, target);
            }
        } finally { currentClass = save; }

        // Constructor: función con el nombre de la clase (en miembros de instancia)
        Symbol maybeCtor = cls.instanceMembers.tryLookup(cls.name);
        if (maybeCtor instanceof FunctionSymbol) {
            FunctionSymbol fCtor = (FunctionSymbol) maybeCtor;
            cls.constructor = fCtor;
            fCtor.isConstructor = true;
            if (fCtor.astNode.returnType != null)
                err(fCtor.line, fCtor.column, "el constructor no puede declarar tipo de retorno");
        }
    }

    private void declareEnum(EnumDef ed) {
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
            if (v.isOwner && isStatic) {
                err(n.line, n.column, "'var owner' no compatible con miembro estático: '" + n.name + "'");
            }
            VarSymbol sym = new VarSymbol(n.name, v.isPublic, isStatic, owner, false, n.line, n.column);
            sym.decl = v;
            sym.isOwner = v.isOwner;
            if (!scope.tryDefine(sym))
                err(n.line, n.column, "nombre duplicado: '" + n.name + "' en " + scope.tag);
            if (!info.declSymbols.containsKey(v))
                info.declSymbols.put(v, sym);
        }
    }

    private void declareFunc(FuncDef f, ClassSymbol owner, Scope scope) {
        boolean isStatic = f.name.isStatic();
        validateStaticPrefix(f.name, owner);
        if (f.isFinal && owner == null)
            err(f.line, f.column, "'final' solo es válido en métodos de clase");
        if (f.isIntrinsic && owner != null)
            err(f.line, f.column, "'intrinsic' sólo es válido en funciones a nivel módulo, no en métodos");
        if (f.isIntrinsic && !f.isPublic)
            err(f.line, f.column, "'intrinsic' implica visibilidad pública — añade 'public'");
        FunctionSymbol sym = new FunctionSymbol(f.name.name, f.isPublic, f.isFinal, isStatic, owner, f);
        sym.isIntrinsic = f.isIntrinsic;
        for (Param p : f.params)
            sym.params.add(new ParamSymbol(p.name, p.line, p.column));
        if (!scope.tryDefine(sym))
            err(f.line, f.column, "nombre duplicado: '" + f.name.name + "' en " + scope.tag);
        info.declSymbols.put(f, sym);
    }

    private void declareProperty(PropertyDef p, ClassSymbol owner, Scope scope) {
        boolean isStatic = p.name.isStatic();
        validateStaticPrefix(p.name, owner);
        if (p.isFinal && owner == null)
            err(p.line, p.column, "'final' solo es válido en propiedades de clase");
        if (p.isOwner && isStatic)
            err(p.line, p.column, "'property owner' no compatible con miembro estático: '" + p.name.name + "'");
        if (p.isOwner && owner == null)
            err(p.line, p.column, "'property owner' solo válida en propiedades de instancia de clase");
        PropertySymbol sym = new PropertySymbol(p.name.name, p.isPublic, p.isFinal, isStatic, owner, p);
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

    private void resolveTypesPass(ModuleNode mod) {
        // 1) extends de cada clase
        for (Symbol s : module.members.getSymbols()) {
            if (s instanceof ClassSymbol) {
                ClassSymbol cls = (ClassSymbol) s;
                if (cls.baseClassName != null) {
                    ClassSymbol base = resolveBaseClassName(cls.baseClassName);
                    if (base != null) {
                        cls.baseClass = base;
                    } else {
                        err(cls.line, cls.column, "clase base '" + cls.baseClassName + "' no existe");
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

        // 2) tipos de vars/consts/props/parámetros + retorno de funciones
        for (ITopLevelDecl def : mod.defs) resolveDefTypes(def, null);
        for (Symbol s : module.members.getSymbols()) {
            if (s instanceof ClassSymbol) {
                ClassSymbol cls = (ClassSymbol) s;
                for (ITopLevelDecl m : cls.astNode.members)
                    resolveDefTypes(m, cls);
            }
        }
    }

    private void resolveDefTypes(ITopLevelDecl def, ClassSymbol owner) {
        if (def instanceof ConstDecl) {
            ConstDecl c = (ConstDecl) def;
            Symbol sym = info.declSymbols.get(c);
            if (sym instanceof ConstSymbol && c.type != null)
                ((ConstSymbol) sym).type = resolveType(c.type);
        } else if (def instanceof VarDecl) {
            VarDecl v = (VarDecl) def;
            BpType t = resolveType(v.type);
            for (DeclName n : v.names) {
                Symbol sym;
                if (owner == null) sym = module.members.resolve(n.name);
                else sym = n.isStatic() ? owner.staticMembers.resolve(n.name)
                                         : owner.instanceMembers.resolve(n.name);
                if (sym instanceof VarSymbol) ((VarSymbol) sym).type = t;
            }
        } else if (def instanceof FuncDef) {
            FuncDef f = (FuncDef) def;
            Symbol sym = info.declSymbols.get(f);
            if (sym instanceof FunctionSymbol) {
                FunctionSymbol fs = (FunctionSymbol) sym;
                for (int i = 0; i < f.params.size(); i++)
                    fs.params.get(i).type = resolveType(f.params.get(i).type);
                if (f.returnType != null) fs.returnType = resolveType(f.returnType);
            }
        } else if (def instanceof PropertyDef) {
            PropertyDef p = (PropertyDef) def;
            Symbol sym = info.declSymbols.get(p);
            if (sym instanceof PropertySymbol)
                ((PropertySymbol) sym).type = resolveType(p.type);
        }
    }

    private BpType resolveType(TypeRef t) {
        if (t instanceof SimpleTypeRef) {
            SimpleTypeRef st = (SimpleTypeRef) t;
            switch (st.name) {
                case "integer": return PrimitiveType.INTEGER;
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
                // H5 (V2) — tipo raíz universal expresable en fuente. Es el
                // mismo `any` interno que usan List/SyncList, ahora con nombre
                // OO. Habilita contenedores genéricos escritos en BP puro
                // (Map). Modelo de objetos estilo Java: los primitivos se
                // envuelven a mano (Integer(x), ...) para meterlos en
                // contenedores. Serializa como "any" en .bpi (mismo tipo).
                case "Object":  return AnyType.INSTANCE;
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
        // L2 v2 — tipo cualificado `Mod.Type`: lookup en el namespace
        // importado bajo el alias.
        int dot = st.name.indexOf('.');
        if (dot >= 0) {
            String alias = st.name.substring(0, dot);
            String simple = st.name.substring(dot + 1);
            Symbol nsSym = module.members.resolve(alias);
            if (nsSym instanceof Symbol.ImportedNamespaceSymbol) {
                Symbol.ImportedNamespaceSymbol ns = (Symbol.ImportedNamespaceSymbol) nsSym;
                Symbol cand = ns.classes.get(simple);
                if (cand == null) cand = ns.enums.get(simple);
                if (cand instanceof ClassSymbol) return new ClassType((ClassSymbol) cand);
                if (cand instanceof EnumSymbol)  return new EnumType((EnumSymbol) cand);
            }
            err(st.line, st.column, "tipo cualificado '" + st.name + "' no encontrado");
            return ErrorType.INSTANCE;
        }
        Symbol sym = module.members.resolve(st.name);
        if (sym instanceof ClassSymbol) return new ClassType((ClassSymbol) sym);
        if (sym instanceof EnumSymbol)  return new EnumType((EnumSymbol) sym);
        // L2: si el nombre del tipo no resuelve en el scope local, buscamos
        // en los namespaces importados. Si hay UNA coincidencia, la usamos;
        // si hay varias (collision), pedimos qualificación con `Mod.Cls`.
        Symbol fromImports = null;
        String importsAlias = null;
        for (Symbol m : module.members.getSymbols()) {
            if (m instanceof Symbol.ImportedNamespaceSymbol) {
                Symbol.ImportedNamespaceSymbol ns = (Symbol.ImportedNamespaceSymbol) m;
                Symbol cand = ns.classes.get(st.name);
                if (cand == null) cand = ns.enums.get(st.name);
                if (cand != null) {
                    if (fromImports != null && fromImports != cand) {
                        err(st.line, st.column, "tipo '" + st.name
                                + "' ambiguo: aparece en '" + importsAlias
                                + "' y '" + ns.name + "'; usa Module.Tipo");
                        return ErrorType.INSTANCE;
                    }
                    fromImports = cand;
                    importsAlias = ns.name;
                }
            }
        }
        if (fromImports instanceof ClassSymbol)
            return new ClassType((ClassSymbol) fromImports);
        if (fromImports instanceof EnumSymbol)
            return new EnumType((EnumSymbol) fromImports);
        err(st.line, st.column, "tipo '" + st.name + "' no encontrado");
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
                checkOverridesAndFinal(cls);
                for (ITopLevelDecl m : cls.astNode.members)
                    bodyForDef(m, cls);
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

    private void analyzeConstInit(ConstDecl c) {
        Symbol s = info.declSymbols.get(c);
        if (!(s instanceof ConstSymbol)) return;
        ConstSymbol cs = (ConstSymbol) s;
        BpType t = analyzeExpr(c.value, null, cs.type);
        if (cs.type == null) cs.type = t;
        else if (!cs.type.isAssignableFrom(t)
                && !acceptsNarrowFromLiteral(cs.type, t, c.value))
            err(c.line, c.column, narrowAssignErrorMessage(cs.type, t, c.value,
                    "valor de tipo '" + t.display() + "' no asignable a const de tipo '" + cs.type.display() + "'"));
    }

    private void analyzeVarInit(VarDecl v) {
        if (v.init == null) return;
        Symbol sym = info.declSymbols.get(v);
        BpType t = (sym instanceof VarSymbol) ? ((VarSymbol) sym).type : null;
        BpType got = analyzeExpr(v.init, null, t);
        if (t != null && !t.isAssignableFrom(got)
                && !acceptsNarrowFromLiteral(t, got, v.init))
            err(v.line, v.column, narrowAssignErrorMessage(t, got, v.init,
                    "valor de tipo '" + got.display() + "' no asignable a var de tipo '" + t.display() + "'"));
        if (t != null && t.isScalar() && got instanceof NullType)
            err(v.line, v.column, "no se puede asignar 'null' a un tipo escalar '" + t.display() + "'");
        // L8: el inicializador de una var/static a NIVEL MÓDULO se ignora hoy
        //     (no se emite código de asignación). Avisamos al usuario para que
        //     mueva la asignación a la función inicializadora del módulo.
        //     Vars locales y campos de instancia/static de clase no se ven
        //     afectados (esos sí se inicializan).
        if (sym instanceof VarSymbol) {
            VarSymbol vs = (VarSymbol) sym;
            if (vs.ownerClass == null && !vs.isLocal) {
                warn(v.line, v.column, "inicializador de var a nivel módulo se ignora; "
                        + "mueve la asignación a la función inicializadora del módulo");
            }
        }
    }

    private void analyzeFunction(FuncDef f) {
        Symbol s = info.declSymbols.get(f);
        if (!(s instanceof FunctionSymbol)) return;
        FunctionSymbol fs = (FunctionSymbol) s;

        FunctionSymbol save = currentFunction;
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

        analyzeBody(f.body, localScope);

        currentFunction = save;
    }

    private void analyzeProperty(PropertyDef p) {
        Symbol s = info.declSymbols.get(p);
        if (!(s instanceof PropertySymbol)) return;
        PropertySymbol ps = (PropertySymbol) s;

        if (p.init != null) {
            BpType t = analyzeExpr(p.init, null, ps.type);
            if (ps.type != null && !ps.type.isAssignableFrom(t))
                err(p.line, p.column, "valor inicial de tipo '" + t.display() + "' no asignable a propiedad '" + ps.type.display() + "'");
            // L8: el init de una property a NIVEL MÓDULO se ignora hoy.
            //     (Propiedades de clase se inicializan vía constructor.)
            if (ps.ownerClass == null) {
                warn(p.line, p.column, "inicializador de property a nivel módulo se ignora; "
                        + "asigna el valor en la función inicializadora del módulo");
            }
        }
        if (p.getter != null) {
            insideGetter = true;
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
            } finally {
                currentFunction = save;
                insideGetter = false;
            }
        }
        if (p.setter != null) {
            insideSetter = true;
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
            } finally {
                currentFunction = save;
                insideSetter = false;
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

        if      (s instanceof VarDecl)      analyzeLocalVarDecl((VarDecl) s, scope);
        else if (s instanceof ConstDecl)    analyzeLocalConstDecl((ConstDecl) s, scope);
        else if (s instanceof AssignStmt)   analyzeAssign((AssignStmt) s, scope);
        else if (s instanceof IfStmt)       analyzeIf((IfStmt) s, scope);
        else if (s instanceof SwitchStmt)   analyzeSwitch((SwitchStmt) s, scope);
        else if (s instanceof ParallelStmt) analyzeParallel((ParallelStmt) s, scope);
        else if (s instanceof WhileStmt)    analyzeWhile((WhileStmt) s, scope);
        else if (s instanceof DoLoopStmt)   analyzeDoLoop((DoLoopStmt) s, scope);
        else if (s instanceof ForStmt)      analyzeFor((ForStmt) s, scope);
        else if (s instanceof TryStmt)      analyzeTry((TryStmt) s, scope);
        else if (s instanceof ReturnStmt)   analyzeReturn((ReturnStmt) s, scope);
        else if (s instanceof Ast.DestructAssignStmt) analyzeDestructAssign((Ast.DestructAssignStmt) s, scope);
        else if (s instanceof ThrowStmt)    analyzeExpr(((ThrowStmt) s).value, scope, null);
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
            if (loopDepth == 0 && switchDepth == 0)
                err(b.line, b.column, "'break' fuera de bucle o switch");
        }
        else if (s instanceof ContinueStmt) {
            ContinueStmt cs = (ContinueStmt) s;
            if (loopDepth == 0)
                err(cs.line, cs.column, "'continue' fuera de bucle");
        }
        else if (s instanceof ExprStmt) {
            ExprStmt es = (ExprStmt) s;
            analyzeExpr(es.expr, scope, null);
            if (!(es.expr instanceof CallExpr || es.expr instanceof SuperCallExpr))
                err(es.line, es.column, "se esperaba una llamada como sentencia");
        }
    }

    private void analyzeLocalVarDecl(VarDecl v, Scope scope) {
        BpType t = resolveType(v.type);
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
            if (!scope.tryDefine(sym))
                err(n.line, n.column, "variable duplicada en este scope: '" + n.name + "'");
            if (!info.declSymbols.containsKey(v)) info.declSymbols.put(v, sym);
        }
        if (v.init != null) {
            BpType got = analyzeExpr(v.init, scope, t);
            if (!t.isAssignableFrom(got)
                    && !acceptsNarrowFromLiteral(t, got, v.init))
                err(v.line, v.column, narrowAssignErrorMessage(t, got, v.init,
                        "valor de tipo '" + got.display() + "' no asignable a variable de tipo '" + t.display() + "'"));
            if (t.isScalar() && got instanceof NullType)
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
        BpType t = (declared != null) ? declared : got;
        ConstSymbol sym = new ConstSymbol(c.name.name, false, false, null, c.line, c.column);
        sym.type = t;
        if (!scope.tryDefine(sym))
            err(c.line, c.column, "const duplicada: '" + c.name.name + "'");
        if (declared != null && !declared.isAssignableFrom(got)
                && !acceptsNarrowFromLiteral(declared, got, c.value))
            err(c.line, c.column, narrowAssignErrorMessage(declared, got, c.value,
                    "valor de tipo '" + got.display() + "' no asignable a const de tipo '" + declared.display() + "'"));
        info.declSymbols.put(c, sym);
    }

    private void analyzeAssign(AssignStmt a, Scope scope) {
        BpType lhsT = analyzeExpr(a.target, scope, null);
        BpType rhsT = analyzeExpr(a.value, scope, lhsT);

        // target debe ser asignable
        boolean okTarget = a.target instanceof IdentifierExpr
                || a.target instanceof MemberAccessExpr
                || a.target instanceof IndexExpr
                || a.target instanceof FieldExpr;
        if (!okTarget)
            err(a.line, a.column, "el operando izquierdo no es asignable");

        if (a.target instanceof FieldExpr && !insideSetter)
            err(a.line, a.column, "'field' solo puede usarse dentro de get/set");

        if (a.op == AssignOpKind.PLUS_ASSIGN || a.op == AssignOpKind.MINUS_ASSIGN) {
            boolean ok = (lhsT.isNumeric() && rhsT.isNumeric())
                    || (a.op == AssignOpKind.PLUS_ASSIGN
                        && lhsT instanceof PrimitiveType && ((PrimitiveType) lhsT).tag == PrimitiveType.Kind.STRING
                        && rhsT instanceof PrimitiveType && ((PrimitiveType) rhsT).tag == PrimitiveType.Kind.STRING);
            if (!ok) {
                String opStr = a.op == AssignOpKind.PLUS_ASSIGN ? "+=" : "-=";
                err(a.line, a.column, "operandos incompatibles para '" + opStr + "': '" + lhsT.display() + "' y '" + rhsT.display() + "'");
            }
        } else {
            if (rhsT instanceof NullType && lhsT.isScalar())
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
            try {
                Scope bodyScope = new Scope("parallel-case-" + br.varName, filteredModule);
                analyzeBody(br.body, bodyScope);
            } finally {
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
        switchDepth++;
        try {
            Set<String> seen = new HashSet<>();
            for (CaseClause cc : sw.cases) {
                for (IExpr v : cc.values) {
                    BpType t = analyzeExpr(v, scope, subjT);
                    if (!subjT.isAssignableFrom(t) && !t.isAssignableFrom(subjT)) {
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
        try {
            analyzeBody(dl.body, new Scope("do", scope));
            if (dl.condition != null) analyzeBoolCondition(dl.condition, scope);
        } finally { loopDepth--; }
    }

    private void analyzeFor(ForStmt f, Scope scope) {
        Scope inner = new Scope("for", scope);
        BpType iterT;
        if (f.range instanceof ForNumericRange) {
            ForNumericRange nr = (ForNumericRange) f.range;
            BpType fromT = analyzeExpr(nr.from, scope, PrimitiveType.INTEGER);
            BpType toT   = analyzeExpr(nr.to,   scope, PrimitiveType.INTEGER);
            if (nr.step != null) analyzeExpr(nr.step, scope, PrimitiveType.INTEGER);
            if (!PrimitiveType.INTEGER.isAssignableFrom(fromT)
                    || !PrimitiveType.INTEGER.isAssignableFrom(toT))
                err(f.line, f.column, "los límites del 'for' numérico deben ser integer");
            iterT = PrimitiveType.INTEGER;
        } else if (f.range instanceof ForInRange) {
            ForInRange inr = (ForInRange) f.range;
            BpType collT = analyzeExpr(inr.iterable, scope, null);
            if (collT instanceof ArrayType) iterT = ((ArrayType) collT).element;
            else {
                err(f.line, f.column, "se esperaba array en 'for in', encontrado '" + collT.display() + "'");
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
                    Symbol sym = module.members.resolve(cl.exceptionType);
                    if (sym instanceof ClassSymbol) excT = new ClassType((ClassSymbol) sym);
                    else {
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
        if (tr.finallyBody != null) analyzeBody(tr.finallyBody, new Scope("finally", scope));
    }

    private void analyzeReturn(ReturnStmt r, Scope scope) {
        if (currentFunction == null) { err(r.line, r.column, "'return' fuera de función"); return; }
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
        // Si el tipo de retorno es tupla pero el valor no es una tupla literal.
        if (currentFunction.returnType instanceof BpType.TupleType) {
            err(r.line, r.column, "debe devolver una tupla literal '(...)' de "
                    + ((BpType.TupleType) currentFunction.returnType).elements.size() + " elementos");
            analyzeExpr(r.value, scope, null);
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
        else if (e instanceof CallExpr)      t = analyzeCall((CallExpr) e, scope);
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
        } else {
            checkArgs(sc.line, sc.column, baseCtor.params, sc.args, scope);
        }
        return VoidType.INSTANCE;
    }

    private BpType analyzeField(FieldExpr fe) {
        if (!insideGetter && !insideSetter) {
            err(fe.line, fe.column, "'field' solo es válido dentro de get/set");
            return ErrorType.INSTANCE;
        }
        return ErrorType.INSTANCE;
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

    private BpType analyzeIdentifier(IdentifierExpr id, Scope scope) {
        Symbol sym = (scope != null) ? scope.resolve(id.name) : null;

        // Caso especial: si el identificador coincide con el nombre de la
        // clase actual queremos referenciar la CLASE (no el constructor,
        // que es un miembro de instancia con el mismo nombre).
        if (sym == null && currentClass != null && id.name.equals(currentClass.name)) {
            Symbol clsSym = module.members.resolve(id.name);
            if (clsSym instanceof ClassSymbol) sym = clsSym;
        }

        if (sym == null && currentClass != null && currentFunction != null && !currentFunction.isStatic)
            sym = currentClass.lookupInstance(id.name);
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
            err(id.line, id.column, "identificador no resuelto: '" + id.name + "'");
            return ErrorType.INSTANCE;
        }
        info.exprSymbols.put(id, sym);
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
            checkVisibility(ma.line, ma.column, sub, ct.cls);
            info.exprSymbols.put(ma, sub);
            return typeOfSymbol(sub);
        }

        if (tgtT instanceof ErrorType) return ErrorType.INSTANCE;
        err(ma.line, ma.column, "el tipo '" + tgtT.display() + "' no tiene miembros");
        return ErrorType.INSTANCE;
    }

    private BpType analyzeIndex(IndexExpr ix, Scope scope) {
        BpType t = analyzeExpr(ix.target, scope, null);
        BpType idxT = analyzeExpr(ix.index, scope, PrimitiveType.INTEGER);
        if (!PrimitiveType.INTEGER.isAssignableFrom(idxT))
            err(ix.line, ix.column, "índice debe ser integer, no '" + idxT.display() + "'");
        if (t instanceof ArrayType) return ((ArrayType) t).element;
        if (t instanceof ErrorType) return ErrorType.INSTANCE;
        err(ix.line, ix.column, "el tipo '" + t.display() + "' no es indexable");
        return ErrorType.INSTANCE;
    }

    private BpType analyzeCall(CallExpr ce, Scope scope) {
        Symbol target = null;
        boolean rejectedByModuleVisibility = false;
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
                if (ce.args.size() != 1)
                    err(ce.line, ce.column, id.name + "(): se esperaba 1 argumento");
                else if (at != null && !at.isNumeric() && !(at instanceof ErrorType))
                    err(ce.line, ce.column, id.name + "(): se esperaba un valor numérico, no '" + at.display() + "'");
                return castT;
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
            analyzeExpr(ma, scope, null);
            target = info.exprSymbols.get(ma);
        }

        // Llamada a clase ⇒ construcción
        if (target instanceof ClassSymbol) {
            ClassSymbol cls = (ClassSymbol) target;
            FunctionSymbol ctor = cls.findConstructor();
            if (ctor == null) {
                if (!ce.args.isEmpty())
                    err(ce.line, ce.column, "la clase '" + cls.name + "' no define constructor; no admite argumentos");
                for (IExpr a : ce.args) analyzeExpr(a, scope, null);
            } else {
                checkArgs(ce.line, ce.column, ctor.params, ce.args, scope);
            }
            return new ClassType(cls);
        }

        if (target instanceof FunctionSymbol) {
            FunctionSymbol fn = (FunctionSymbol) target;
            checkArgs(ce.line, ce.column, fn.params, ce.args, scope);
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

        for (IExpr a : ce.args) analyzeExpr(a, scope, null);
        if (target == null && ce.callee instanceof IdentifierExpr)
            err(ce.line, ce.column, "no se puede llamar a '" + ((IdentifierExpr) ce.callee).name + "'");
        return ErrorType.INSTANCE;
    }

    private void checkArgs(int line, int col, List<ParamSymbol> ps, List<IExpr> args, Scope scope) {
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
        if (!(sym instanceof ClassSymbol)) {
            err(ie.line, ie.column, "'instanceof' espera un nombre de clase, '"
                    + ie.typeName + "' no lo es");
        } else {
            info.exprSymbols.put(ie, sym);   // emisor podrá leer el class symbol
        }
        return PrimitiveType.BOOLEAN;
    }

    private BpType analyzeBinary(BinaryExpr b, Scope scope) {
        BpType lt = analyzeExpr(b.left,  scope, null);
        BpType rt = analyzeExpr(b.right, scope, null);
        switch (b.op) {
            case "+":
                if (isString(lt) || isString(rt)) return PrimitiveType.STRING;
                if (lt.isNumeric() && rt.isNumeric()) return promote(lt, rt);
                err(b.line, b.column, "'+' incompatible: '" + lt.display() + "' y '" + rt.display() + "'");
                return ErrorType.INSTANCE;
            case "-": case "*": case "/":
                if (lt.isNumeric() && rt.isNumeric()) return promote(lt, rt);
                err(b.line, b.column, "'" + b.op + "' requiere numéricos, encontrados '" + lt.display() + "' y '" + rt.display() + "'");
                return ErrorType.INSTANCE;
            case "mod":
                // L10/H1.2/H1.3 — mod opera en el tipo promovido (i32/i64/f32/f64;
                // float/double usan fmod). Resultado = promote(lt, rt).
                if (lt.isNumeric() && rt.isNumeric()) return promote(lt, rt);
                err(b.line, b.column, "'mod' requiere numéricos");
                return ErrorType.INSTANCE;
            case "&": case "|": case "xor": case "shl": case "shr":
                // H1.2 — bitwise/shift en i64 si algún operando es long.
                if ((isIntegerLike(lt) || isLong(lt)) && (isIntegerLike(rt) || isLong(rt)))
                    return (isLong(lt) || isLong(rt)) ? PrimitiveType.LONG : PrimitiveType.INTEGER;
                err(b.line, b.column, "'" + b.op + "' requiere integer/long");
                return ErrorType.INSTANCE;
            case "and": case "or":
                if (PrimitiveType.BOOLEAN.isAssignableFrom(lt) && PrimitiveType.BOOLEAN.isAssignableFrom(rt))
                    return PrimitiveType.BOOLEAN;
                err(b.line, b.column, "'" + b.op + "' requiere boolean");
                return ErrorType.INSTANCE;
            case "==": case "!=":
                if (!comparableForEquality(lt, rt))
                    err(b.line, b.column, "'" + b.op + "' incompatible: '" + lt.display() + "' vs '" + rt.display() + "'");
                return PrimitiveType.BOOLEAN;
            case "<": case ">": case "<=": case ">=":
                if (lt.isNumeric() && rt.isNumeric()) return PrimitiveType.BOOLEAN;
                if (isString(lt) && isString(rt)) return PrimitiveType.BOOLEAN;
                err(b.line, b.column, "'" + b.op + "' requiere numéricos o strings");
                return ErrorType.INSTANCE;
        }
        return ErrorType.INSTANCE;
    }

    private static boolean isString(BpType t) {
        return t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.STRING;
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
        if (a instanceof NullType && b.isReference()) return true;
        if (b instanceof NullType && a.isReference()) return true;
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
    // OVERRIDES Y FINAL
    // ============================================================
    private void checkOverridesAndFinal(ClassSymbol cls) {
        if (cls.baseClass == null) return;
        for (Symbol s : cls.instanceMembers.getSymbols()) {
            if (s instanceof FunctionSymbol) {
                FunctionSymbol fn = (FunctionSymbol) s;
                Symbol baseSym = cls.baseClass.lookupInstance(fn.name);
                if (baseSym instanceof FunctionSymbol) {
                    FunctionSymbol baseFn = (FunctionSymbol) baseSym;
                    if (baseFn.isFinal)
                        err(fn.line, fn.column, "'" + fn.name + "' está marcada 'final' en la clase base, no se puede sobreescribir");
                    if (!fn.hasSameSignatureAs(baseFn))
                        err(fn.line, fn.column, "sobreescritura de '" + fn.name + "' con firma incompatible");
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
            }
        }
    }

    // ============================================================
    // VISIBILIDAD
    // ============================================================
    private void checkVisibility(int line, int col, Symbol sub, ClassSymbol owner) {
        boolean isPublic;
        if      (sub instanceof FunctionSymbol) isPublic = ((FunctionSymbol) sub).isPublic;
        else if (sub instanceof VarSymbol)      isPublic = ((VarSymbol) sub).isPublic;
        else if (sub instanceof ConstSymbol)    isPublic = ((ConstSymbol) sub).isPublic;
        else if (sub instanceof PropertySymbol) isPublic = ((PropertySymbol) sub).isPublic;
        else                                    isPublic = true;

        if (currentClass != null && owner != null
                && (currentClass == owner || currentClass.isSubclassOf(owner)))
            return;
        if (!isPublic)
            err(line, col, "miembro privado '" + sub.name + "' inaccesible aquí");
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

    private void warn(int line, int col, String msg) {
        info.diagnostics.add(new SemanticDiagnostic(SemanticDiagnostic.Kind.WARNING, msg, line, col));
    }
}
