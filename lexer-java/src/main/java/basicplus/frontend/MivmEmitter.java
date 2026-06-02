// ============================================================
// MivmEmitter.java
// Backend alternativo a JvmEmitter: emite un .mod ejecutable por
// la VM bpgenvm (artefacto edu:bpgenvm) en lugar de un .class JVM.
//
// Fase A — alcance soportado:
//   - Módulo + funciones top-level con params i32/f32/string.
//   - Vars y consts a nivel módulo (data block).
//   - Locales en funciones.
//   - Literales i32, f32, string, bool, null.
//   - Aritmética + comparaciones (i32 y f32, con promoción int→float).
//   - and / or (sin corto-circuito) y not.
//   - if / elseif / else, while, do-loop, for := to [step].
//   - switch / case / default (chain de comparaciones).
//   - print multi-item con separadores ',' (espacio) y ';' (nada).
//   - return [valor].
//   - Llamada a función top-level (fn(args)).
//   - Inicializador de módulo: el .mod arranca llamando a la función
//     con el mismo nombre del módulo, luego a Main (si existen).
//   - Concatenación de strings con '+': helper "_strconcat" inyectado
//     en el módulo on-demand.
//
// Fase B (pendiente): clases + herencia + this/super + estáticos +
// constructores. Stubs marcados con TODO Fase B.
//
// Compatible con JDK 8.
// ============================================================
package basicplus.frontend;

import basicplus.frontend.Ast.*;
import basicplus.frontend.BpType.*;
import basicplus.frontend.Symbol.*;

import edu.bpgenvm.bytecode.Builtin;
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.generador.ModWriter;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public final class MivmEmitter {

    private final ModuleNode moduleAst;
    private final SemanticInfo info;
    private final ModWriter w = new ModWriter();

    public final List<String> errors = new ArrayList<>();

    /**
     * Path (típicamente absoluto) al fichero fuente .bp del módulo. Se
     * vuelca al .dbg para que la VM, al imprimir un stack trace, pueda
     * citar el fichero original. Null si no se sabe.
     */
    private String sourcePath = null;
    public void setSourcePath(String sourcePath) { this.sourcePath = sourcePath; }

    /**
     * Mapeo (relPc → línea fuente). Se rellena durante {@link #emitStmt}
     * justo antes de despachar cada sentencia: capturamos el offset del
     * próximo opcode y la línea de la sentencia AST. Se serializa en el
     * .dbg al final.
     */
    private final List<int[]> debugLineMap = new ArrayList<>();

    /** Cache de literales string en data block: valor → nombre del símbolo. */
    private final Map<String, String> stringPool = new HashMap<>();
    private int stringPoolCounter = 0;

    /** Track de qué helpers inyectamos para no repetirlos. */
    private final Set<String> emittedHelpers = new HashSet<>();

    /** Pila de scopes (un scope por función en emisión). */
    private final Deque<FuncScope> scopeStack = new ArrayDeque<>();

    /** Stack de etiquetas para break/continue. */
    private final Deque<LoopLabels> loopStack = new ArrayDeque<>();

    /** Nombre del backing field al emitir el cuerpo de un getter/setter (para resolver 'field'). */
    private String currentPropertyField = null;
    /** Backing global activo cuando emitimos el cuerpo de un getter/setter
     *  de una property a NIVEL MÓDULO. Análogo a currentPropertyField pero
     *  apunta al nombre del global (sin clase contenedora). */
    private String currentModulePropertyBacking = null;

    /** Pila de try frames activos en el punto de emisión. Cada frame lleva su
     *  finallyBody y un flag handlerActive (false cuando estamos emitiendo el
     *  cuerpo del catch — el handler ya fue consumido por el THROW). */
    private final Deque<TryFrame> tryStack = new ArrayDeque<>();

    /** Contador para generar nombres únicos de locales temporales (e.g. __newref_N).
     *  Necesario para que construcciones anidadas no compartan slot. */
    private int tempCounter = 0;

    /** Map del nombre lógico del módulo importado al fromPath declarado en BP.
     *  Las claves son strings dotted que se ven en los imports del .mod:
     *    - "Foo"          (import Foo)
     *    - "lib.sub.Foo"  (import lib.sub.Foo)
     *  Valor "" ⇒ no hay fromPath (el loader usará la convención por defecto). */
    private final Map<String, String> importFromPaths = new HashMap<>();

    /**
     * Dado el nombre cualificado completo del símbolo importado (e.g.
     * "lib.sub.Foo.bar"), devuelve el fromPath asociado al módulo que lo
     * provee, o "" si no se declaró fromPath. El módulo se identifica
     * descartando el último segmento del nombre cualificado.
     */
    @SuppressWarnings("unused")
    private String fromPathFor(String qualifiedExternalSymbol) {
        if (qualifiedExternalSymbol == null) return "";
        int lastDot = qualifiedExternalSymbol.lastIndexOf('.');
        if (lastDot < 0) return "";
        String modulePart = qualifiedExternalSymbol.substring(0, lastDot);
        String fp = importFromPaths.get(modulePart);
        return (fp == null) ? "" : fp;
    }

    private static final class TryFrame {
        final List<IStmt> finallyBody;   // null si no hay finally
        /** Cuántos TRY_BEGINs siguen vigentes para este frame.
         *  En try body con N catches: N. Tras entrar a un catch: 0 (la unwind
         *  los pop'eó todos). Wrap-around dentro del catch: 1. */
        int activeHandlers;
        TryFrame(List<IStmt> fb, int n) { this.finallyBody = fb; this.activeHandlers = n; }
    }

    private static final class FuncScope {
        final FunctionSymbol fs;
        /** Locales declarados (nombre → ya está pasado a ModWriter). */
        final Set<String> locals = new HashSet<>();
        /** Locales owner; al salir de scope (o en cualquier camino que JUMPea
         *  al endLabel) hay que emitir FREE_REF de cada uno. Mantenemos orden
         *  de declaración para que un objeto que se liberó primero no se
         *  reutilice mientras se libera otro que aún apunta a memoria viva.
         *  Iteración natural = orden de declaración. */
        final java.util.LinkedHashSet<String> ownerLocals = new java.util.LinkedHashSet<>();
        /** True si la función tiene tipo de retorno (no void). */
        final boolean returnsValue;
        /** Etiqueta del único punto de salida físico de la función. */
        final int endLabel;
        /** Si != null, esta función es el cuerpo de un getter/setter de una
         *  sync property de CLASE. Al hacer return interno, hay que emitir
         *  un unlock contra this.__syncMutex antes del JUMP a endLabel
         *  (el unlock de fall-through ya está al final del body manualmente,
         *  pero un return interno lo saltaría — B2). */
        String syncClassName = null;
        /** Análogo a syncClassName pero para sync property a nivel MÓDULO.
         *  Cuando es true, el unlock se hace contra el Mutex global del
         *  módulo. Mutuamente exclusivo con syncClassName. */
        boolean syncModule = false;
        /** H1.2 (V2): la función devuelve long → return de 8 bytes (LRET). */
        boolean returnsLong = false;
        FuncScope(FunctionSymbol fs, boolean returnsValue, int endLabel) {
            this.fs = fs;
            this.returnsValue = returnsValue;
            this.endLabel = endLabel;
        }
    }

    private static final class LoopLabels {
        final int continueLabel;
        final int breakLabel;
        LoopLabels(int cont, int brk) { this.continueLabel = cont; this.breakLabel = brk; }
    }

    public MivmEmitter(ModuleNode moduleAst, SemanticInfo info) {
        this.moduleAst = moduleAst;
        this.info = info;
    }

    // ============================================================
    // ENTRADA
    // ============================================================

    public void emitTo(Path outputDir) throws IOException {
        w.addModulo(moduleAst.name);
        if (moduleAst.library != null && !moduleAst.library.isEmpty()) {
            w.setLibrary(moduleAst.library);
        }

        // 0) Registrar imports declarados: nos quedamos con el fromPath (si lo
        //    hubo) indexado por el nombre lógico cualificado del módulo. Cuando
        //    se generen CALL_EXT para funciones de ese módulo, le pasaremos
        //    este fromPath al ModWriter para que viaje dentro del .mod.
        importFromPaths.clear();
        if (moduleAst.imports != null) {
            for (ImportNode imp : moduleAst.imports) {
                if (imp.path == null || imp.path.isEmpty()) continue;
                StringBuilder sb = new StringBuilder();
                for (int i = 0; i < imp.path.size(); i++) {
                    if (i > 0) sb.append('.');
                    sb.append(imp.path.get(i));
                }
                String key = sb.toString();
                String from = (imp.fromPath == null) ? "" : imp.fromPath;
                importFromPaths.put(key, from);
            }
        }

        // 1) Strings literales — pase previo de scan para precargar el pool.
        //    (Opcional: declararlos lazy también funciona, pero hacerlo aquí
        //    los deja en data block antes que cualquier global de usuario.)
        collectStringLiterals(moduleAst);

        // 2) Globals y consts a nivel módulo.
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof VarDecl)   emitModuleVarDecl((VarDecl) d);
            else if (d instanceof ConstDecl) emitModuleConstDecl((ConstDecl) d);
        }

        // 2-bis) Backing fields de properties a nivel módulo. Se declaran
        //        como globals privados; las funciones get/set se emiten
        //        en la fase 5 (top-level funcs).
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof PropertyDef) emitModulePropertyBacking((PropertyDef) d);
        }

        // 2-ter) Mutex compartido para sync properties de módulo, si las hay.
        //        Es un único Mutex por módulo (no uno por property) — más
        //        simple y suficiente: la operación atómica es por accesor.
        if (hasModuleSyncProperty()) {
            w.declareGlobal(MODULE_SYNC_MUTEX_GLOBAL);
        }

        // 2.5) Flag de idempotencia para __init: declarado siempre porque
        //      __init existe siempre (incluso si el módulo no tiene
        //      inicializador propio ni imports).
        w.addConstantInt("__initialized", 0);

        // 3a) Clases stdlib built-in: RuntimeError, List, StringBuilder, Mutex.
        //     Se sintetizan ANTES de __init y __startup porque __init puede
        //     necesitar Mutex (por sync properties a nivel módulo).
        //     Emitidas siempre. Pequeñas; los .mods ganan ~200 bytes pero a
        //     cambio cualquier programa puede usarlas sin import explícito.
        synthesizeRuntimeErrorClass();
        synthesizeListClass();
        synthesizeOwnerListClass();
        synthesizeStringBuilderClass();
        synthesizeThreadClass();
        synthesizeMutexClass();
        synthesizeSyncListClass();

        // 3b) __init: corre dependencias (CALL_EXT a cada import.__init) y luego
        //     el inicializador del módulo si existe. Idempotente.
        emitInitFunction();

        // 3c) __startup: bootstrap interno (no exportado). Llama a __init
        //     local y luego al main(arg) del usuario con arg="".
        emitStartupWrapper();

        // 4a-bis) Subclases anónimas de Thread sintetizadas por los `parallel`
        //         statements del módulo. Se emiten ANTES de las clases de
        //         usuario para que cualquier método/función pueda referenciar
        //         su __init / descriptor.
        synthesizeAllParallelClasses();

        // 4b) Clases del usuario (ANTES de las funciones del módulo, para que
        //     Main pueda referenciar Cls.__init y class descriptors al construir).
        //     Los EnumDef no requieren emisión: sus valores ya están en EnumSymbol.values
        //     y se inlinean como literales int en emitMemberAccess.
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof ClassDef) emitClassDef((ClassDef) d);
        }

        // 5) Funciones del módulo (initializer + Main del usuario + libres).
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof FuncDef) emitFunctionDef((FuncDef) d, null);
        }

        // 5-bis) Getters y setters de properties de módulo. Después de las
        //        funciones de usuario para que addFunction no parta a una
        //        función abierta.
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof PropertyDef) emitModulePropertyAccessors((PropertyDef) d);
        }

        // 6) Helpers pendientes (e.g. __strconcat) tras user functions.
        flushHelpers();

        Files.createDirectories(outputDir);
        Path out = outputDir.resolve(w.getCanonicalFilename());
        w.writeToFile(out.toString());

        // Volcar la información de depuración a un fichero .dbg con el mismo
        // nombre base que el .mod. La VM lo carga opcionalmente al cargar el
        // módulo y lo usa para enriquecer los stack traces con file:line.
        writeDbgFile(outputDir);
    }

    /**
     * Serializa el {@link #debugLineMap} a un fichero .dbg en la misma
     * carpeta que el .mod. Formato textual y autodescriptivo:
     *
     * <pre>
     *   dbg 1
     *   module &lt;nombre&gt;
     *   source &lt;path al .bp&gt;     (línea opcional; sólo si conocemos el path)
     *   lines
     *   &lt;relPc&gt; &lt;line&gt;
     *   ...
     * </pre>
     *
     * Pierde el .col por simplicidad — la mayoría de stack traces sólo cita
     * file:line. Si en el futuro se quiere col, basta extender la sección.
     */
    private void writeDbgFile(Path outputDir) throws IOException {
        String modFilename = w.getCanonicalFilename();          // ej. "Json.mod" o "lib.Json.mod"
        String dbgFilename = modFilename.substring(0, modFilename.length() - 4) + ".dbg";
        Path dbgOut = outputDir.resolve(dbgFilename);
        StringBuilder sb = new StringBuilder();
        // .dbg v2: añade sección `properties` con (name, type, csOff del
        // backing global). El runtime la usa para que el debugger del IDE
        // muestre las properties públicas del módulo en el panel de
        // variables sin ejecutar BP code (lee bytes directos del data block).
        sb.append("dbg 2\n");
        sb.append("module ").append(moduleAst.name).append('\n');
        if (sourcePath != null && !sourcePath.isEmpty()) {
            sb.append("source ").append(sourcePath).append('\n');
        }
        sb.append("lines\n");
        for (int[] pair : debugLineMap) {
            sb.append(pair[0]).append(' ').append(pair[1]).append('\n');
        }
        // Sección properties: una línea por public property a nivel módulo.
        // Formato: "<name> <type> <csOff>" donde csOff es el offset (relativo
        // a CS del módulo) del backing global __prop_<name>.
        boolean wroteHeader = false;
        for (ITopLevelDecl d : moduleAst.defs) {
            if (!(d instanceof PropertyDef)) continue;
            PropertyDef pd = (PropertyDef) d;
            if (!pd.isPublic) continue;
            BpType t = (info.declSymbols.get(pd) instanceof PropertySymbol)
                    ? ((PropertySymbol) info.declSymbols.get(pd)).type
                    : (pd.type != null ? typeRefToBpType(pd.type) : null);
            String typeStr = dbgTypeTag(t);
            if (typeStr == null) continue;   // tipo no inspeccionable
            Integer csOff = w.getDataSymbolOffset(moduleBackingName(pd.name.name));
            if (csOff == null) continue;
            if (!wroteHeader) { sb.append("properties\n"); wroteHeader = true; }
            sb.append(pd.name.name).append(' ').append(typeStr).append(' ').append(csOff).append('\n');
        }
        Files.write(dbgOut, sb.toString().getBytes(java.nio.charset.StandardCharsets.UTF_8));
    }

    /** Tag corto del tipo para .dbg properties. null si no se puede inspeccionar. */
    private static String dbgTypeTag(BpType t) {
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case INTEGER: return "integer";
                case FLOAT:   return "float";
                case STRING:  return "string";
                case BOOLEAN: return "boolean";
            }
        }
        // Tipos clase: el slot es una ref (4 bytes); el debugger los mostrará
        // como "ref@addr". Etiquetamos como "ref".
        if (t instanceof ClassType) return "ref";
        return null;
    }

    // ============================================================
    // Wrapper main de la VM
    // ============================================================

    /**
     * Emite `__init`, exportado como `<Module>.__init`. Es idempotente:
     *   - Si `__initialized` ya es 1, retorna inmediatamente.
     *   - Si es 0, lo pone a 1, llama recursivamente a __init de cada
     *     módulo importado (CALL_EXT cualificado), y finalmente al
     *     inicializador del usuario (`function Modulename()`) si existe.
     *
     * Lo llaman:
     *   - __startup local (al cargar el módulo como root).
     *   - __init de otro módulo que nos importa (vía CALL_EXT).
     */
    private void emitInitFunction() {
        try {
            w.addFunction("__init", true);
            w.declareLocal("__discard");

            // if __initialized != 0: return 0
            w.emitGetGlobal("__initialized");
            int doInitLabel = w.newLabel();
            w.emitJumpIfFalse(doInitLabel);   // salta si stack-top == 0
            w.emitPushInt(0);
            w.emitRet();
            w.declareLabel(doInitLabel);

            // __initialized = 1
            w.emitPushInt(1);
            w.emitSetGlobal("__initialized");

            // Inicializar mutex compartido para sync properties de módulo, si
            // las hay. Se hace ANTES de las dependencias por simetría con la
            // inicialización de un objeto: lo nuestro primero, luego cascada.
            if (hasModuleSyncProperty()) {
                int id = tempCounter++;
                String newref  = "__modmtx_newref_"  + id;
                String discard = "__modmtx_discard_" + id;
                w.declareLocal(newref);
                w.declareLocal(discard);
                w.emitNewObject("Mutex");
                w.emitSetLocal(newref);
                w.emitGetLocal(newref);
                w.emitCall("Mutex.__init");
                w.emitSetLocal(discard);
                w.emitGetLocal(newref);
                w.emitSetGlobal(MODULE_SYNC_MUTEX_GLOBAL);
            }

            // Cascada de inicializaciones de dependencias (cada import por
            // su qualified name). Recordamos que CALL_EXT entra al módulo
            // dueño cambiando CS, así que es seguro reentrar incluso si el
            // dep es a su vez importer de otros.
            //
            // FILTRO: sólo invocamos __init de los imports cuya .bpi fue
            // efectivamente cargada (ImportedNamespaceSymbol presente en
            // los miembros del módulo). Los imports "phantom" (declarados
            // pero sin interface en disco) se omiten para no provocar un
            // fallo de link en tiempo de carga.
            //
            // NOTA cross-binding: usamos library/moduleName del IMPL
            // (almacenados en ImportedNamespaceSymbol). Para `import I:M`,
            // la interfaz no tiene .mod ni __init; debemos llamar al __init
            // del impl real.
            if (moduleAst.imports != null && info.module != null) {
                for (ImportNode imp : moduleAst.imports) {
                    if (imp.path == null || imp.path.isEmpty()) continue;
                    String alias = imp.path.get(imp.path.size() - 1);
                    Symbol sym = info.module.members.tryLookup(alias);
                    if (!(sym instanceof Symbol.ImportedNamespaceSymbol)) continue;
                    Symbol.ImportedNamespaceSymbol ns = (Symbol.ImportedNamespaceSymbol) sym;
                    // Optimización — módulos all-intrinsic (todas las funcs
                    // son `intrinsic`, sin properties/vars/clases) NO emiten
                    // .mod con __init (su .bpi viaja solo). Saltar el
                    // CALL_EXT evita un "Símbolo no resuelto" en linkAll.
                    // Cubre Math, IO y similares.
                    if (isAllIntrinsicNamespace(ns)) continue;
                    StringBuilder sb = new StringBuilder();
                    if (!ns.library.isEmpty()) sb.append(ns.library).append('.');
                    sb.append(ns.moduleName).append(".__init");
                    w.emitCallExt(sb.toString(), ns.fromPath);
                    w.emitSetLocal("__discard");
                }
            }

            // Inicializador del propio módulo (function Modulename()) si
            // está declarado.
            if (info.module != null && info.module.initializer != null) {
                w.emitCall(info.module.initializer.name);
                w.emitSetLocal("__discard");
            }

            w.emitPushInt(0);
            w.emitRet();
        } catch (IOException e) {
            errors.add("error emitiendo __init: " + e.getMessage());
        }
    }

    /**
     * Emite el bootstrap `__startup`, no exportado. Es a donde apunta
     * mainOffset del .mod cuando el módulo arranca como root:
     *   - Llama a `__init` local (que cascadea las dependencias).
     *   - Llama a `main(arg)` del usuario con arg="" (o a `Main()` legacy).
     *   - HALT.
     */
    private void emitStartupWrapper() {
        try {
            w.addFunction("__startup", false);   // INTERNAL: no exportado
            w.declareLocal("__discard");

            // Inicialización del propio módulo y todas sus dependencias.
            w.emitCall("__init");
            w.emitSetLocal("__discard");

            if (info.module != null && info.module.mainFunction != null) {
                Symbol.FunctionSymbol fm = info.module.mainFunction;
                if (fm.params.size() == 1) {
                    String emptyStrSym = internString("");
                    w.emitLeaGlobal(emptyStrSym);
                }
                w.emitCall(fm.name);
                w.emitSetLocal("__discard");
            }
            w.emit(OpCode.HALT);
            w.setMainEntry("__startup");
        } catch (IOException e) {
            errors.add("error emitiendo __startup: " + e.getMessage());
        }
    }

    // ============================================================
    // GLOBALS (vars/consts a nivel módulo)
    // ============================================================

    private void emitModuleVarDecl(VarDecl vd) {
        // Por ahora soportamos sólo declaración simple (1 nombre).
        BpType declT = vd.type != null ? typeRefToBpType(vd.type) : null;   // H1.2
        for (DeclName dn : vd.names) {
            if (dn.isStatic()) { errors.add("var estática de clase a nivel módulo no soportada: " + dn.name); continue; }
            // Reservamos 4 bytes (8 si es long) en data block. El inicializador
            // se ejecuta en el module initializer; los init a nivel módulo se
            // ignoran (L8) → el usuario asigna en código.
            if (is8Byte(declT)) w.declareGlobalLong(dn.name);   // H1.2/H1.3: long/double 8 bytes
            else w.declareGlobal(dn.name);
        }
        // Si la VarDecl tiene init y NO hay un init explícito en el initializer del
        // módulo, deberíamos emitir la asignación al arranque. Para simplificar,
        // dejamos que el initializer haga el trabajo (es la convención que sigue
        // JvmEmitter y los fase1/fase2 lo respetan).
    }

    private void emitModuleConstDecl(ConstDecl cd) {
        if (cd.name.isStatic()) {
            errors.add("const estática de clase a nivel módulo no soportada: " + cd.name.name);
            return;
        }
        // Para constantes con valor literal conocido (int/float), las dejamos
        // en data block ya inicializadas. Para otras, declaramos como global
        // y dejamos al initializer asignar.
        BpType t = info.exprTypes.get(cd.value);
        if (t instanceof PrimitiveType) {
            PrimitiveType pt = (PrimitiveType) t;
            if (cd.value instanceof IntLitExpr && pt.tag == PrimitiveType.Kind.INTEGER) {
                w.addConstantInt(cd.name.name, (int) ((IntLitExpr) cd.value).value);
                return;
            }
            if (cd.value instanceof FloatLitExpr && pt.tag == PrimitiveType.Kind.FLOAT) {
                w.addConstantFloat(cd.name.name, (float) ((FloatLitExpr) cd.value).value);
                return;
            }
        }
        // Fallback: global mutable, asignado por el initializer.
        w.declareGlobal(cd.name.name);
    }

    // ============================================================
    // PROPERTIES A NIVEL MÓDULO
    // ============================================================
    //
    // Se traducen a:
    //   - un global privado __prop_<name> que es el backing field
    //   - una función pública __prop_get_<name>() que retorna el valor
    //   - una función pública __prop_set_<name>(__val) que escribe el valor
    //
    // Esto permite que otros módulos importen la property y la lean/escriban
    // a través de CALL_EXT sin exponer el global directamente (las vars de
    // módulo NO son cross-module en BP por diseño).
    //
    // Para sync: se declara un único Mutex global del módulo (compartido por
    // todas las sync properties del módulo) y el accesor envuelve su cuerpo
    // con lock/unlock contra él.

    /** Prefijo del backing global de una property de módulo. */
    private static final String MODULE_PROP_BACKING_PREFIX = "__prop_";
    /** Prefijo del getter sintético. */
    private static final String MODULE_PROP_GET_PREFIX     = "__prop_get_";
    /** Prefijo del setter sintético. */
    private static final String MODULE_PROP_SET_PREFIX     = "__prop_set_";
    /** Global del Mutex compartido para sync properties de módulo. */
    static final String MODULE_SYNC_MUTEX_GLOBAL    = "__prop_mutex";

    static String moduleBackingName(String propName) { return MODULE_PROP_BACKING_PREFIX + propName; }

    /** L10 — devuelve el opcode VM para el cast `tipoNarrow(x)` o null si
     *  el nombre no es uno de los casts. */
    /** H1.1 (V2) — alocadores de arrays narrow → opcode NEWARRAY_* directo.
     *  Igual que los casts (narrowCastOpFor), evitan un CALL_BUILTIN/builtin
     *  nuevo en la VM: ésta ya implementa NEWARRAY_I8/I16. null si no aplica. */
    private static OpCode arrayAllocOpFor(String fnName) {
        switch (fnName) {
            case "newByteArray": return OpCode.NEWARRAY_I8;
            case "newLongArray": return OpCode.NEWARRAY_I64;   // H1.2 (V2)
            case "newDoubleArray": return OpCode.NEWARRAY_I64; // H1.3 (V2): 8 bytes opacos
            default:             return null;
        }
    }

    private static OpCode narrowCastOpFor(String fnName) {
        switch (fnName) {
            case "byte":  return OpCode.I32_TO_U8;
            case "int8":  return OpCode.I32_TO_I8;
            case "word":  return OpCode.I32_TO_U16;
            case "int16": return OpCode.I32_TO_I16;
            default:      return null;
        }
    }
    static String moduleGetterName(String propName)  { return MODULE_PROP_GET_PREFIX     + propName; }
    static String moduleSetterName(String propName)  { return MODULE_PROP_SET_PREFIX     + propName; }

    /** Construye el qualified name de un accesor externo (`lib.Mod.__prop_get_x`). */
    private static String buildExternalAccessorName(PropertySymbol ps, boolean setter) {
        StringBuilder sb = new StringBuilder();
        if (ps.externalLibrary != null && !ps.externalLibrary.isEmpty()) {
            sb.append(ps.externalLibrary).append('.');
        }
        sb.append(ps.externalModule).append('.');
        sb.append(setter ? moduleSetterName(ps.name) : moduleGetterName(ps.name));
        return sb.toString();
    }

    /** Devuelve true si algún PropertyDef a nivel módulo está marcado sync. */
    private boolean hasModuleSyncProperty() {
        if (moduleAst.defs == null) return false;
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof PropertyDef && ((PropertyDef) d).isSync) return true;
        }
        return false;
    }

    /**
     * Reserva el global backing para una property a nivel módulo. La
     * inicialización (si tiene init expression) la hace el initializer del
     * módulo, igual que con VarDecl a nivel módulo.
     */
    private void emitModulePropertyBacking(PropertyDef pd) {
        if (pd.name.isStatic()) {
            errors.add("property estática de clase a nivel módulo no soportada: " + pd.name.name);
            return;
        }
        w.declareGlobal(moduleBackingName(pd.name.name));
    }

    /** Lock/unlock contra el mutex compartido del módulo. */
    private void emitModuleSyncLock() throws IOException {
        int id = tempCounter++;
        String discard = "__modSyncLock_" + id;
        declareLocal(discard);
        w.emitGetGlobal(MODULE_SYNC_MUTEX_GLOBAL);
        w.emitInvokeVirtual("Mutex", "lock", 0);
        w.emitSetLocal(discard);
    }
    private void emitModuleSyncUnlock() throws IOException {
        int id = tempCounter++;
        String discard = "__modSyncUnlock_" + id;
        declareLocal(discard);
        w.emitGetGlobal(MODULE_SYNC_MUTEX_GLOBAL);
        w.emitInvokeVirtual("Mutex", "unlock", 0);
        w.emitSetLocal(discard);
    }

    /**
     * Emite getter y setter de una property a nivel módulo. Ambos públicos
     * para que se puedan llamar vía CALL_EXT desde otros módulos.
     *
     * Limitaciones conocidas:
     *   - Custom getter/setter con `return` interno + sync: el JUMP al
     *     endLabel salta el unlock y deja el mutex tomado. Misma limitación
     *     que para sync de clase. Documentado.
     */
    private void emitModulePropertyAccessors(PropertyDef pd) {
        try {
            if (pd.name.isStatic()) return;   // ya reportado en backing
            BpType propType = (pd.type != null) ? typeRefToBpType(pd.type) : null;
            // Si el resolveType del analyzer cargó un tipo más rico, usarlo.
            Symbol psym = info.declSymbols.get(pd);
            if (psym instanceof PropertySymbol && ((PropertySymbol) psym).type != null) {
                propType = ((PropertySymbol) psym).type;
            }

            String backing = moduleBackingName(pd.name.name);
            String getter  = moduleGetterName(pd.name.name);
            String setter  = moduleSetterName(pd.name.name);

            // ---- Getter ----
            w.addFunction(getter, pd.isPublic);
            FunctionSymbol gfs = new FunctionSymbol(getter, pd.isPublic, false, false, null, null);
            gfs.returnType = propType;
            beginFunctionScope(gfs, propType);
            currentModulePropertyBacking = backing;
            if (pd.isSync) scopeStack.peek().syncModule = true;
            try {
                if (pd.isSync) emitModuleSyncLock();
                if (pd.getter != null) {
                    for (IStmt s : pd.getter.body) emitStmt(s);
                } else {
                    // auto-getter: __result := __prop_<name>
                    w.emitGetGlobal(backing);
                    w.emitSetLocal("__result");
                }
                if (pd.isSync) emitModuleSyncUnlock();
                emitFunctionEnd();
            } finally {
                currentModulePropertyBacking = null;
                scopeStack.pop();
            }

            // ---- Setter ----
            w.addFunction(setter, pd.isPublic);
            w.declareParam("__val");
            FunctionSymbol sfs = new FunctionSymbol(setter, pd.isPublic, false, false, null, null);
            beginFunctionScope(sfs, null);
            currentModulePropertyBacking = backing;
            if (pd.isSync) scopeStack.peek().syncModule = true;
            try {
                if (pd.isSync) emitModuleSyncLock();
                if (pd.setter != null) {
                    declareLocal(pd.setter.paramName);
                    w.emitGetParam("__val");
                    w.emitSetLocal(pd.setter.paramName);
                    for (IStmt s : pd.setter.body) emitStmt(s);
                } else {
                    // auto-setter: __prop_<name> := __val
                    w.emitGetParam("__val");
                    w.emitSetGlobal(backing);
                }
                if (pd.isSync) emitModuleSyncUnlock();
                emitFunctionEnd();
            } finally {
                currentModulePropertyBacking = null;
                scopeStack.pop();
            }
        } catch (IOException e) {
            errors.add("error emitiendo property de módulo '" + pd.name.name + "': " + e.getMessage());
        }
    }

    // ============================================================
    // FUNCIONES
    // ============================================================

    private void emitFunctionDef(FuncDef fn, ClassSymbol owner) {
        try {
            FunctionSymbol fs = (FunctionSymbol) info.declSymbols.get(fn);
            if (fs == null) {
                errors.add("falta FunctionSymbol para " + fn.name.name);
                return;
            }

            // Funciones intrinsic: sólo signature en el .bpi, no se emite cuerpo
            // en el .mod. Cada call-site las reemplaza por opcodes inline.
            if (fs.isIntrinsic) {
                // Verificamos contra el registro para detectar typos en el nombre
                // antes de que el .bpi llegue a un consumidor.
                String qn = (moduleAst != null ? moduleAst.name : "?") + "." + fs.name;
                if (!Intrinsics.isKnown(qn)) {
                    errors.add(qn + ": función intrínseca desconocida — añade su"
                            + " emisor a basicplus.frontend.Intrinsics");
                }
                return;
            }

            String name = fs.name;  // funciones top-level: name simple
            // Siempre exportadas (true) para que aparezcan en stack traces aunque
            // sean privadas en BP. La privacidad de BP se enforza en compile-time
            // (lookup en SemanticAnalyzer), no a nivel de runtime.
            w.addFunction(name, true);

            // Declarar params (H1.2: width-aware, long = 8 bytes)
            declareParamsWidthAware(fs);

            beginFunctionScope(fs, null);
            try {
                for (IStmt s : fn.body) emitStmt(s);
                emitFunctionEnd();
            } finally {
                scopeStack.pop();
            }
        } catch (IOException e) {
            errors.add("error en función " + fn.name.name + ": " + e.getMessage());
        }
    }

    // ============================================================
    // CLASES
    // ============================================================

    private void emitClassDef(ClassDef cd) {
        try {
            ClassSymbol cls = (ClassSymbol) info.declSymbols.get(cd);
            if (cls == null) { errors.add("falta ClassSymbol para " + cd.name); return; }

            // 1) Pre-declarar static vars como globals cualificados.
            //    (lo hacemos antes de la apertura de clase porque addClass exige no
            //    estar dentro de otra clase abierta, y declareGlobal puede llamarse
            //    en cualquier momento siempre que estemos fuera de la apertura.)
            for (Symbol s : cls.staticMembers.getSymbols()) {
                if (s instanceof VarSymbol) {
                    VarSymbol v = (VarSymbol) s;
                    w.declareGlobal(cd.name + "." + v.name);
                }
            }

            // 2) Apertura de clase + fields + métodos virtuales.
            // L2 v3 — si el padre es EXTERNO (otro módulo), pasamos al
            // ModWriter el layout binario del parent (numFields/numMethods
            // + bitmaps) leído del ClassSymbol stub. El ModWriter reserva
            // placeholders para preservar slot/field numbering, escribe
            // vt[slot]=-1 para los slots heredados, y registra un fixup
            // que el loader aplica para parchear parentOff (CS-relative al
            // CS de este módulo).
            String parentForModWriter = null;
            edu.bpgenvm.generador.ModWriter.ExternalParentLayout externalParent = null;
            if (cls.baseClass != null) {
                if (cls.baseClass.isExternal) {
                    SemanticInfo.ClassBinaryLayout layout = (cls.baseClass.binaryLayout != null)
                            ? cls.baseClass.binaryLayout : null;
                    if (layout == null) {
                        errors.add("L2 v3: parent cross-module '" + cls.baseClass.name
                                + "' sin layout binario en .bpi — recompila el módulo dueño");
                        return;
                    }
                    StringBuilder qname = new StringBuilder();
                    if (cls.baseClass.externalLibrary != null && !cls.baseClass.externalLibrary.isEmpty())
                        qname.append(cls.baseClass.externalLibrary).append('.');
                    qname.append(cls.baseClass.externalModule).append('.').append(cls.baseClass.name);
                    parentForModWriter = qname.toString();
                    externalParent = new edu.bpgenvm.generador.ModWriter.ExternalParentLayout(
                            layout.numFields, layout.numMethods,
                            layout.fieldBitmap, layout.ownerBitmap);
                } else {
                    parentForModWriter = cls.baseClass.name;
                }
            }
            w.addClass(cd.name, parentForModWriter, externalParent);

            // ¿Esta clase declara su propio __syncMutex (porque tiene sync
            // properties Y ningún ancestor lo ha declarado ya)? El field se
            // hereda automáticamente vía addClass(parent), así que sólo lo
            // declaramos en el primer eslabón de la cadena que lo necesita.
            boolean declaresSyncMutex = hasOwnSyncProperty(cd) && !ancestorHasSyncProperty(cls);
            if (declaresSyncMutex) {
                // ref-type (apunta a una instancia de Mutex), no owner (los
                // Mutex son pequeños y compartibles si fueran refs externas).
                w.declareField("__syncMutex", true, false);
            }

            // Fields de instancia, en orden de declaración del AST (para que el
            // slot 0..N coincida con el orden lógico).
            for (ITopLevelDecl m : cd.members) {
                if (m instanceof VarDecl) {
                    VarDecl vd = (VarDecl) m;
                    for (DeclName dn : vd.names) {
                        if (!dn.isStatic()) {
                            BpType t = (vd.type != null) ? typeRefToBpType(vd.type) : null;
                            // Si no tenemos el tipo ya, lo sacamos del Symbol.
                            VarSymbol vs = (VarSymbol) cls.instanceMembers.tryLookup(dn.name);
                            if (vs != null) t = vs.type;
                            boolean isRef = isRefType(t);
                            // BUG-6: long/double = campo de 8 bytes (2 slots).
                            w.declareField(dn.name, isRef, vd.isOwner, is8Byte(t));
                        }
                    }
                }
            }

            // Properties PRIMERO (backing field + getter + setter), antes de los
            // métodos normales que pueden invocar getX/setX por su nombre.
            for (ITopLevelDecl m : cd.members) {
                if (m instanceof PropertyDef) {
                    emitPropertyDef(cls, (PropertyDef) m);
                }
            }

            // Métodos de instancia (no constructores, no estáticos) vía addMethod.
            for (ITopLevelDecl m : cd.members) {
                if (m instanceof FuncDef) {
                    FuncDef fn = (FuncDef) m;
                    FunctionSymbol fs = (FunctionSymbol) info.declSymbols.get(fn);
                    if (fs == null || fs.isStatic || fs.isConstructor) continue;
                    emitInstanceMethod(fn, fs);
                }
            }

            w.endClass();

            // L2 v3 — anotamos el layout binario (numFields/numMethods + bitmaps)
            // en el ClassSymbol para que ModuleInterface.extractClass lo emita
            // al .bpi. Sólo es relevante para clases públicas (las que otros
            // módulos pueden heredar) pero lo anotamos siempre — es cheap.
            edu.bpgenvm.generador.ModWriter.ClassLayoutInfo li = w.getClassLayout(cd.name);
            if (li != null) {
                cls.binaryLayout = new SemanticInfo.ClassBinaryLayout(
                        li.numFields, li.numMethods,
                        li.fieldBitmap, li.ownerBitmap);
            }

            // L2 v3 — exportar el descriptor de cada clase pública como data
            // symbol, igual que B3 v2 hace con RuntimeError. Permite que un
            // módulo importador con `extends X` resuelva el parent en linkAll
            // (vía globalSymbolTable) y parche parentOff del child.
            if (cd.isPublic) {
                w.exportDataSymbol(cd.name);
            }

            // 3) Constructores y métodos estáticos como funciones normales.
            boolean hasUserCtor = false;
            for (ITopLevelDecl m : cd.members) {
                if (m instanceof FuncDef) {
                    FuncDef fn = (FuncDef) m;
                    FunctionSymbol fs = (FunctionSymbol) info.declSymbols.get(fn);
                    if (fs == null) continue;
                    if (fs.isConstructor) { emitConstructorFn(cls, fn, fs); hasUserCtor = true; }
                    else if (fs.isStatic) emitStaticMethodFn(cls, fn, fs);
                }
            }

            // 4) Si la clase necesita inicializar __syncMutex pero no tiene
            //    constructor de usuario, sintetizamos un __init mínimo que
            //    sólo hace `this.__syncMutex := Mutex()`. Sin esto, las
            //    sync properties operarían sobre un mutex "null" y los
            //    locks petarían.
            if (declaresSyncMutex && !hasUserCtor) {
                synthesizeSyncMutexCtor(cls);
            }

            // 5) L2: factory cross-module. Para cada `public class` emitimos
            //    una función pública `__cls_new_<Cls>(args)` con la misma
            //    firma que el constructor. Permite que módulos importadores
            //    construyan instancias sin necesidad de un opcode nuevo
            //    (NEW_OBJECT en otro módulo requiere conocer el class_ptr,
            //    cosa que no se puede expresar en bytecode portable).
            //    Para clases privadas no emitimos factory (no cross-module).
            if (cd.isPublic) {
                synthesizeCrossModuleFactory(cls);
                // L2 v3 — si hay ctor, sintetiza también `__cls_init_<Cls>(this, args)`
                // que un subclass cross-module puede llamar desde `super(...)` sin
                // pasar por NEW_OBJECT. El nombre no contiene puntos, así que el
                // loader lo interpreta como símbolo de este módulo (no como
                // submódulo cualificado).
                if (cls.constructor != null) {
                    synthesizeCrossModuleInit(cls);
                }
            }
        } catch (IOException e) {
            errors.add("error en clase " + cd.name + ": " + e.getMessage());
        }
    }

    /**
     * Emite `__cls_new_<Cls>(args)`: hace NEW_OBJECT local + invoca el ctor
     * (si existe) + devuelve el ref. La factoría es pública y vive en el
     * data block del módulo dueño, así que su class_ptr siempre es válido.
     * El importador la llama vía CALL_EXT.
     *
     * Si la clase no tiene ctor de usuario pero TIENE __init sintético
     * (por sync property), también se llama. Si no tiene __init en absoluto,
     * sólo NEW_OBJECT + return.
     */
    private void synthesizeCrossModuleFactory(ClassSymbol cls) throws IOException {
        String factoryName = "__cls_new_" + cls.name;
        w.addFunction(factoryName, true);   // pública: cross-module

        FunctionSymbol ctor = cls.constructor;
        // Declarar params iguales al ctor (sin contar `this`).
        if (ctor != null) {
            for (ParamSymbol p : ctor.params) {
                // BUG-6: width-aware — long/double = 8 bytes. Sin esto la
                // factoría cross-module declaraba el param a 4 bytes y un arg
                // long/double se corrompía al pasarlo al __init.
                w.declareParam(p.name, is8Byte(p.type) ? 8 : 4);
            }
        }
        // returnType=AnyType para evitar dependencia de tipo: el caller
        // assigna a `ClassType` (que acepta any por L1/escape hatch).
        FunctionSymbol synthFs = new FunctionSymbol(factoryName, true, false, false, null, null);
        synthFs.returnType = BpType.AnyType.INSTANCE;
        beginFunctionScope(synthFs, BpType.AnyType.INSTANCE);
        try {
            String newref  = "__factory_newref";
            String discard = "__factory_discard";
            declareLocal(newref);
            declareLocal(discard);
            // newref := new Cls
            w.emitNewObject(cls.name);
            w.emitSetLocal(newref);
            // newref.__init(args) si hay ctor (de usuario o sintético).
            //  - Si hay ctor de usuario → emitConstructorFn lo emitió.
            //  - Si hay sync mutex sin ctor → synthesizeSyncMutexCtor lo emitió.
            //  - Si no hay ninguno → no llamamos a __init (no existe).
            boolean hasInit = (ctor != null);
            // Detectar __init sintético via cls.constructor (lo setea synthesizeSyncMutexCtor).
            // (Si llegamos aquí y ctor != null por synth, mismo path.)
            if (hasInit) {
                w.emitGetLocal(newref);
                if (ctor != null) {
                    for (ParamSymbol p : ctor.params) w.emitGetParam(p.name);
                }
                w.emitCall(cls.name + ".__init");
                w.emitSetLocal(discard);
            }
            // __result := newref
            w.emitGetLocal(newref);
            w.emitSetLocal("__result");
            emitFunctionEnd();
        } finally { scopeStack.pop(); }
    }

    /**
     * L2 v3 — Emite `__cls_init_<Cls>(this, args)`: llama a `<Cls>.__init`
     * localmente. La factoría es pública y se llama desde super(...)
     * cross-module via CALL_EXT. El nombre sin puntos se interpreta como
     * símbolo plano del módulo dueño en el loader (parts[len-2]=módulo).
     */
    private void synthesizeCrossModuleInit(ClassSymbol cls) throws IOException {
        String initName = "__cls_init_" + cls.name;
        w.addFunction(initName, true);   // pública: cross-module
        w.declareParam("this");
        FunctionSymbol ctor = cls.constructor;
        for (ParamSymbol p : ctor.params) {
            w.declareParam(p.name, is8Byte(p.type) ? 8 : 4);   // BUG-6: long/double = 8 bytes
        }
        FunctionSymbol synthFs = new FunctionSymbol(initName, true, false, false, null, null);
        beginFunctionScope(synthFs, null);   // void
        try {
            // Empujamos this + args, luego CALL local al __init real.
            w.emitGetParam("this");
            for (ParamSymbol p : ctor.params) {
                w.emitGetParam(p.name);
            }
            w.emitCall(cls.name + ".__init");
            emitFunctionEnd();
        } finally { scopeStack.pop(); }
    }

    /** true si el namespace importado es 100% intrínsecos sin estado: todas
     *  las functions tienen isIntrinsic=true y no hay properties, vars ni
     *  clases que requieran inicialización en runtime. Consts y enums son
     *  literales por sí mismos — no necesitan __init. */
    private static boolean isAllIntrinsicNamespace(Symbol.ImportedNamespaceSymbol ns) {
        if (!ns.properties.isEmpty()) return false;
        if (!ns.classes.isEmpty())    return false;
        // Si no hay functions, también consideramos "todo lo no-trivial es
        // estado" y forzamos init (defensivo). En la práctica los stdlib
        // intrinsic-only siempre exportan al menos una function.
        if (ns.functions.isEmpty()) return false;
        for (Symbol.FunctionSymbol fs : ns.functions.values()) {
            if (!fs.isIntrinsic) return false;
        }
        return true;
    }

    /** ¿Algún PropertyDef en esta ClassDef tiene isSync? */
    private boolean hasOwnSyncProperty(ClassDef cd) {
        for (ITopLevelDecl m : cd.members) {
            if (m instanceof PropertyDef && ((PropertyDef) m).isSync) return true;
        }
        return false;
    }

    /** ¿Algún ancestor de cls declaró __syncMutex (= tiene sync property)? */
    private boolean ancestorHasSyncProperty(ClassSymbol cls) {
        ClassSymbol p = cls.baseClass;
        while (p != null) {
            for (Symbol s : p.instanceMembers.getSymbols()) {
                if (s instanceof PropertySymbol && ((PropertySymbol) s).isSync) return true;
            }
            p = p.baseClass;
        }
        return false;
    }

    /**
     * Emite `this.__syncMutex := Mutex()` en bytecode. Reutilizable desde
     * constructores sintéticos o como prólogo de constructores de usuario
     * en clases que declaran sync properties.
     */
    private void emitSyncMutexInit(String className) throws IOException {
        int id = tempCounter++;
        String newref  = "__syncmtx_newref_"  + id;
        String discard = "__syncmtx_discard_" + id;
        declareLocal(newref);
        declareLocal(discard);
        w.emitNewObject("Mutex");
        w.emitSetLocal(newref);
        w.emitGetLocal(newref);
        w.emitCall("Mutex.__init");
        w.emitSetLocal(discard);
        w.emitGetParam("this");
        w.emitGetLocal(newref);
        w.emitSetField(className, "__syncMutex");
    }

    /**
     * Constructor default sintetizado para clases con sync properties que
     * el usuario no construyó explícitamente. Sólo inicializa __syncMutex.
     *
     * Registra además un FunctionSymbol "ctor" en cls.constructor para que
     * emitConstruction llame al __init (sin esto, una clase con sync
     * property y sin ctor de usuario salía con __syncMutex=null y el
     * primer lock petaba con "INVOKE_VIRTUAL sobre null receiver").
     */
    private void synthesizeSyncMutexCtor(ClassSymbol cls) throws IOException {
        w.addFunction(cls.name + ".__init", false);
        w.declareParam("this");
        beginFunctionScope(makeSynthFs("__init", null), null);
        try {
            emitSyncMutexInit(cls.name);
            emitFunctionEnd();
        } finally { scopeStack.pop(); }

        // Si no había ctor registrado, registra uno sintético sin params.
        // emitConstruction llamará a Cls.__init y se inicializará __syncMutex.
        if (cls.constructor == null) {
            FunctionSymbol synthCtor = new FunctionSymbol(cls.name, true, false, false, cls, null);
            synthCtor.isConstructor = true;
            cls.constructor = synthCtor;
            // No lo metemos en instanceMembers porque el analizador ya
            // validó las llamadas (sin ctor → sin args). Sólo necesitamos
            // que findConstructor lo encuentre.
        }
    }

    /** Helpers lock/unlock contra this.__syncMutex (resuelto vía className). */
    private void emitSyncLock(String className) throws IOException {
        int id = tempCounter++;
        String discard = "__syncLock_discard_" + id;
        declareLocal(discard);
        w.emitGetParam("this");
        w.emitGetField(className, "__syncMutex");
        w.emitInvokeVirtual("Mutex", "lock", 0);
        w.emitSetLocal(discard);
    }
    private void emitSyncUnlock(String className) throws IOException {
        int id = tempCounter++;
        String discard = "__syncUnlock_discard_" + id;
        declareLocal(discard);
        w.emitGetParam("this");
        w.emitGetField(className, "__syncMutex");
        w.emitInvokeVirtual("Mutex", "unlock", 0);
        w.emitSetLocal(discard);
    }

    private void emitInstanceMethod(FuncDef fn, FunctionSymbol fs) throws IOException {
        // BUG-4 / Opción A: solo los métodos PÚBLICOS van a la vtable (despacho
        // virtual, sobreescribibles, visibles cross-module). Los PRIVADOS se
        // despachan por CALL directo (ver el call-site) → fuera de la vtable,
        // así su orden coincide con el .bpi y no hay desfase de slots.
        if (fs.isPublic) {
            w.addMethod(fs.name);          // vtable + función; declara "this"
        } else {
            w.addPrivateMethod(fs.name);   // solo función llamable; declara "this"
        }
        declareParamsWidthAware(fs);
        beginFunctionScope(fs, null);
        try {
            for (IStmt s : fn.body) emitStmt(s);
            emitFunctionEnd();
        } finally { scopeStack.pop(); }
    }

    private void emitConstructorFn(ClassSymbol cls, FuncDef fn, FunctionSymbol fs) throws IOException {
        // `<Cls>.__init` queda como función intra-módulo (no exportada). El
        // acceso cross-module pasa por la factoría `__cls_init_<Cls>` que se
        // sintetiza separadamente — su nombre sin puntos evita la ambigüedad
        // del parser de imports (parts[len-2] = módulo).
        w.addFunction(cls.name + ".__init", false);
        w.declareParam("this");
        declareParamsWidthAware(fs);
        beginFunctionScope(fs, null);   // constructor = void
        try {
            // Si esta clase introduce __syncMutex, hay que inicializarlo
            // ANTES de cualquier código de usuario para que cualquier acceso
            // a una sync property dentro del propio constructor encuentre el
            // mutex listo. La búsqueda de "introduce __syncMutex" replica el
            // criterio usado en emitClassDef. Si el field ya está heredado,
            // el __init del padre lo habrá inicializado vía super().
            ClassDef cd = (fs.ownerClass != null && fs.ownerClass.decl instanceof ClassDef)
                    ? (ClassDef) fs.ownerClass.decl : null;
            if (cd != null && hasOwnSyncProperty(cd) && !ancestorHasSyncProperty(cls)) {
                emitSyncMutexInit(cls.name);
            }
            for (IStmt s : fn.body) emitStmt(s);
            emitFunctionEnd();
        } finally { scopeStack.pop(); }
    }

    private void emitStaticMethodFn(ClassSymbol cls, FuncDef fn, FunctionSymbol fs) throws IOException {
        w.addFunction(cls.name + "." + fs.name, fs.isPublic);
        declareParamsWidthAware(fs);
        beginFunctionScope(fs, null);
        try {
            for (IStmt s : fn.body) emitStmt(s);
            emitFunctionEnd();
        } finally { scopeStack.pop(); }
    }

    private void emitPropertyDef(ClassSymbol cls, PropertyDef pd) throws IOException {
        PropertySymbol ps = (PropertySymbol) info.declSymbols.get(pd);
        if (ps != null && ps.isStatic) {
            errors.add("property estática no soportada todavía: " + cls.name + "." + pd.name.name);
            return;
        }
        BpType propType = (ps != null) ? ps.type : (pd.type != null ? typeRefToBpType(pd.type) : null);
        boolean isRef = isRefType(propType);

        // 1) Backing field (con flag isOwner si la propiedad es owner; el VM
        //    libera recursivamente este campo cuando la instancia se destruye).
        //    BUG-6: long/double = campo de 8 bytes (2 slots).
        w.declareField(pd.name.name, isRef, pd.isOwner, is8Byte(propType));

        // 2) Getter (custom o auto). Devuelve el tipo de la property.
        //    Si la property es sync, envolvemos cuerpo con lock/unlock contra
        //    this.__syncMutex. Un return interno en el body custom emitirá
        //    el unlock antes del JUMP al endLabel (B2) — el unlock del
        //    fall-through al final del body no se duplica porque el return
        //    JUMPea por encima de él.
        String getterSimple = "get" + capitalize(pd.name.name);
        w.addMethod(getterSimple);
        FunctionSymbol getterFs = new FunctionSymbol(getterSimple, pd.isPublic, false, false, cls, null);
        getterFs.returnType = propType;
        beginFunctionScope(getterFs, propType);
        currentPropertyField = pd.name.name;
        if (pd.isSync) scopeStack.peek().syncClassName = cls.name;
        try {
            if (pd.isSync) emitSyncLock(cls.name);
            if (pd.getter != null) {
                for (IStmt s : pd.getter.body) emitStmt(s);
            } else {
                // auto-getter: result := this.<name>
                w.emitGetParam("this");
                w.emitGetField(cls.name, pd.name.name);
                w.emitSetLocal("__result");
            }
            if (pd.isSync) emitSyncUnlock(cls.name);
            emitFunctionEnd();
        } finally { currentPropertyField = null; scopeStack.pop(); }

        // 3) Setter (custom o auto). Es void.
        String setterSimple = "set" + capitalize(pd.name.name);
        w.addMethod(setterSimple);
        w.declareParam("__val");
        FunctionSymbol setterFs = new FunctionSymbol(setterSimple, pd.isPublic, false, false, cls, null);
        beginFunctionScope(setterFs, null);
        currentPropertyField = pd.name.name;
        if (pd.isSync) scopeStack.peek().syncClassName = cls.name;
        try {
            if (pd.isSync) emitSyncLock(cls.name);
            if (pd.setter != null) {
                // Alias el param al nombre que el usuario escribió.
                declareLocal(pd.setter.paramName);
                w.emitGetParam("__val");
                w.emitSetLocal(pd.setter.paramName);
                for (IStmt s : pd.setter.body) emitStmt(s);
            } else {
                // auto-setter: this.<name> := __val
                w.emitGetParam("this");
                w.emitGetParam("__val");
                emitSetFieldAuto(cls.name, pd.name.name);
            }
            if (pd.isSync) emitSyncUnlock(cls.name);
            emitFunctionEnd();
        } finally { currentPropertyField = null; scopeStack.pop(); }
    }

    private static String capitalize(String s) {
        if (s == null || s.isEmpty()) return s;
        return Character.toUpperCase(s.charAt(0)) + s.substring(1);
    }

    private boolean isRefType(BpType t) {
        if (t == null) return false;
        if (t instanceof ClassType) return true;
        if (t instanceof ArrayType) return true;
        if (t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.STRING) return true;
        return false;
    }

    // ============================================================
    // STATEMENTS
    // ============================================================

    private void emitStmt(IStmt s) throws IOException {
        // Captura del mapeo (relPc → línea origen) para el .dbg. El relPc
        // es el offset del próximo byte que se emitirá: el comienzo del
        // primer opcode de la sentencia. Si dos sentencias caen en el
        // mismo relPc (no emiten código), conservamos sólo la primera.
        if (s instanceof Node) {
            int line = ((Node) s).line;
            if (line > 0) {
                int relPc = w.getCurrentBytecodeOffset();
                if (debugLineMap.isEmpty() || debugLineMap.get(debugLineMap.size() - 1)[0] != relPc) {
                    debugLineMap.add(new int[]{relPc, line});
                }
            }
        }
        if (s instanceof AssignStmt)  emitAssign((AssignStmt) s);
        else if (s instanceof IfStmt)       emitIf((IfStmt) s);
        else if (s instanceof WhileStmt)    emitWhile((WhileStmt) s);
        else if (s instanceof DoLoopStmt)   emitDoLoop((DoLoopStmt) s);
        else if (s instanceof ForStmt)      emitFor((ForStmt) s);
        else if (s instanceof SwitchStmt)   emitSwitch((SwitchStmt) s);
        else if (s instanceof ParallelStmt) emitParallel((ParallelStmt) s);
        else if (s instanceof ReturnStmt)   emitReturn((ReturnStmt) s);
        else if (s instanceof PrintStmt)    emitPrint((PrintStmt) s);
        else if (s instanceof ExprStmt)     emitExprStmt((ExprStmt) s);
        else if (s instanceof VarDecl)      emitLocalVarDecl((VarDecl) s);
        else if (s instanceof ConstDecl)    emitLocalConstDecl((ConstDecl) s);
        else if (s instanceof BreakStmt)    emitBreak();
        else if (s instanceof ContinueStmt) emitContinue();
        else if (s instanceof ThrowStmt)    emitThrowStmt((ThrowStmt) s);
        else if (s instanceof TryStmt)      emitTryStmt((TryStmt) s);
        else {
            errors.add("sentencia no soportada: " + s.getClass().getSimpleName());
        }
    }

    private void emitLocalVarDecl(VarDecl vd) throws IOException {
        BpType declT = vd.type != null ? typeRefToBpType(vd.type)
                                       : (vd.init != null ? info.exprTypes.get(vd.init) : null);
        for (DeclName dn : vd.names) {
            if (is8Byte(declT)) declareLocalLong(dn.name);   // H1.2/H1.3: long/double = 8 bytes
            else declareLocal(dn.name);
            if (vd.isOwner) {
                scopeStack.peek().ownerLocals.add(dn.name);
            }
            if (vd.init != null) {
                emitExpr(vd.init);
                coerceToTarget(vd.init, declT);
                w.emitSetLocal(dn.name);
                // Si esta var es owner y la fuente es otra var owner local
                // (IdentifierExpr → ParamSymbol/VarSymbol con isOwner=true),
                // se trata de un MOVE: nullificamos la fuente.
                if (vd.isOwner) {
                    emitMoveNullifySource(vd.init);
                }
            }
        }
    }

    /** True si el campo (var o backing-field-de-property) de la clase es owner. */
    private boolean isOwnerInstanceField(String className, String fieldName) {
        Symbol cs = (info.module != null) ? info.module.members.resolve(className) : null;
        if (!(cs instanceof ClassSymbol)) return false;
        Symbol m = ((ClassSymbol) cs).lookupInstance(fieldName);
        if (m instanceof VarSymbol)      return ((VarSymbol) m).isOwner;
        if (m instanceof PropertySymbol) return ((PropertySymbol) m).isOwner;
        return false;
    }

    /** Emite SET_FIELD o SET_FIELD_OWNER según corresponda. */
    private void emitSetFieldAuto(String className, String fieldName) throws IOException {
        if (isOwnerInstanceField(className, fieldName)) {
            w.emitSetFieldOwner(className, fieldName);
        } else {
            w.emitSetField(className, fieldName);
        }
    }

    /** True si `src` es un IdentifierExpr que resuelve al mismo símbolo que `target`. */
    private boolean isSameOwnerVar(IExpr src, Symbol target) {
        if (!(src instanceof IdentifierExpr)) return false;
        return info.exprSymbols.get(src) == target;
    }

    /** Si `src` es un IdentifierExpr que resuelve a una VarSymbol owner local,
     *  emite código que pone esa variable a 0 (move: la fuente cede ownership). */
    private void emitMoveNullifySource(IExpr src) throws IOException {
        if (!(src instanceof IdentifierExpr)) return;
        Symbol sym = info.exprSymbols.get(src);
        if (!(sym instanceof VarSymbol)) return;
        VarSymbol v = (VarSymbol) sym;
        if (!v.isOwner) return;
        if (!v.isLocal) return;     // por ahora sólo locales (no campos)
        emitInt(0);
        w.emitSetLocal(v.name);
    }

    private void emitLocalConstDecl(ConstDecl cd) throws IOException {
        declareLocal(cd.name.name);
        emitExpr(cd.value);
        w.emitSetLocal(cd.name.name);
    }

    private void declareLocal(String name) {
        FuncScope scope = scopeStack.peek();
        if (scope.locals.add(name)) {
            w.declareLocal(name);
        }
    }

    private void declareLocalLong(String name) {   // H1.2 (V2): local de 8 bytes
        FuncScope scope = scopeStack.peek();
        if (scope.locals.add(name)) {
            w.declareLocalLong(name);
        }
    }

    // H1.2 (V2): declara los params de fs con ancho por tipo (long = 8 bytes).
    private void declareParamsWidthAware(FunctionSymbol fs) {
        for (ParamSymbol p : fs.params) {
            w.declareParam(p.name, is8Byte(p.type) ? 8 : 4);   // H1.2/H1.3: long/double
        }
    }

    private void emitAssign(AssignStmt a) throws IOException {
        IExpr target = a.target;
        BpType tType = info.exprTypes.get(target);
        if (target instanceof IdentifierExpr) {
            Symbol sym = info.exprSymbols.get(target);
            if (sym == null) { errors.add("identificador sin símbolo: " + ((IdentifierExpr) target).name); return; }

            // Para targets owner:
            //   - locales: emitir FREE antes del SET_LOCAL.
            //   - instance fields (implicit this): el FREE lo hace SET_FIELD_OWNER
            //     dentro de storeToSymbol → emitSetFieldAuto.
            //   - properties owner: el FREE lo hace el setter internamente.
            //   - En todos los casos, si la fuente es otra var owner local, nullificarla.
            boolean targetIsOwnerLocal = (sym instanceof VarSymbol)
                    && ((VarSymbol) sym).isOwner && ((VarSymbol) sym).isLocal;
            boolean targetIsOwnerInstanceField = (sym instanceof VarSymbol)
                    && ((VarSymbol) sym).isOwner
                    && ((VarSymbol) sym).ownerClass != null
                    && !((VarSymbol) sym).isStatic
                    && !((VarSymbol) sym).isLocal;
            boolean targetIsOwnerProperty = (sym instanceof PropertySymbol)
                    && ((PropertySymbol) sym).isOwner;
            boolean targetIsOwner = targetIsOwnerLocal || targetIsOwnerInstanceField || targetIsOwnerProperty;
            String targetName = ((IdentifierExpr) target).name;

            switch (a.op) {
                case ASSIGN:
                    if (targetIsOwnerLocal) {
                        // Detectar auto-asignación 'p := p' para no destruir el objeto.
                        if (!isSameOwnerVar(a.value, sym)) {
                            w.emitGetLocal(targetName);
                            w.emitFreeRef();
                        } else {
                            return; // no-op
                        }
                    }
                    emitExpr(a.value);
                    coerceToTarget(a.value, tType);
                    storeToSymbol(sym, targetName);
                    if (targetIsOwner) {
                        emitMoveNullifySource(a.value);
                    }
                    return;
                case PLUS_ASSIGN:
                case MINUS_ASSIGN: {
                    loadFromSymbol(sym, ((IdentifierExpr) target).name);
                    emitExpr(a.value);
                    coerceToTarget(a.value, tType);
                    emitCompoundOp(a.op, tType);
                    storeToSymbol(sym, ((IdentifierExpr) target).name);
                    return;
                }
            }
        }
        if (target instanceof IndexExpr) {
            IndexExpr ix = (IndexExpr) target;
            switch (a.op) {
                case ASSIGN:
                    emitExpr(ix.target);
                    emitExpr(ix.index);
                    emitExpr(a.value);
                    coerceToTarget(a.value, tType);
                    w.emit(astoreOpFor(ix.target));   // H1.1b — ancho del elemento
                    return;
                case PLUS_ASSIGN:
                case MINUS_ASSIGN: {
                    // Asumimos target/index sin side-effects (lo habitual).
                    emitExpr(ix.target);
                    emitExpr(ix.index);
                    emitExpr(ix.target);
                    emitExpr(ix.index);
                    w.emit(aloadOpFor(ix.target));    // H1.1b — ancho del elemento
                    emitExpr(a.value);
                    coerceToTarget(a.value, tType);
                    emitCompoundOp(a.op, tType);
                    w.emit(astoreOpFor(ix.target));   // H1.1b — ancho del elemento
                    return;
                }
            }
        }
        if (target instanceof FieldExpr) {
            // field := value  (sólo válido dentro del cuerpo de un getter/setter custom)
            // Dos modos:
            //   - dentro del accesor de una property de CLASE → field es this.<backingField>
            //   - dentro del accesor de una property de MÓDULO → field es el global __prop_<name>
            if (currentModulePropertyBacking != null) {
                String backing = currentModulePropertyBacking;
                switch (a.op) {
                    case ASSIGN:
                        emitExpr(a.value);
                        coerceToTarget(a.value, tType);
                        w.emitSetGlobal(backing);
                        return;
                    case PLUS_ASSIGN:
                    case MINUS_ASSIGN:
                        w.emitGetGlobal(backing);
                        emitExpr(a.value);
                        coerceToTarget(a.value, tType);
                        emitCompoundOp(a.op, tType);
                        w.emitSetGlobal(backing);
                        return;
                }
            }
            if (currentPropertyField == null) {
                errors.add("'field' fuera de un getter/setter de propiedad"); return;
            }
            ClassSymbol cls = scopeStack.peek().fs.ownerClass;
            boolean fieldIsOwner = isOwnerInstanceField(cls.name, currentPropertyField);
            switch (a.op) {
                case ASSIGN:
                    w.emitGetParam("this");
                    emitExpr(a.value);
                    coerceToTarget(a.value, tType);
                    emitSetFieldAuto(cls.name, currentPropertyField);
                    if (fieldIsOwner) emitMoveNullifySource(a.value);
                    return;
                case PLUS_ASSIGN:
                case MINUS_ASSIGN:
                    // Compound assign sobre owner no aplica: el operando viene
                    // calculado, no es una transferencia. Pero igual emitSetFieldAuto
                    // libera el viejo correctamente.
                    w.emitGetParam("this");
                    w.emitGetParam("this");
                    w.emitGetField(cls.name, currentPropertyField);
                    emitExpr(a.value);
                    coerceToTarget(a.value, tType);
                    emitCompoundOp(a.op, tType);
                    emitSetFieldAuto(cls.name, currentPropertyField);
                    return;
            }
        }
        if (target instanceof MemberAccessExpr) {
            MemberAccessExpr ma = (MemberAccessExpr) target;
            Symbol mSym = info.exprSymbols.get(ma);
            // Property: emitir INVOKE_VIRTUAL setX (clase) o CALL/CALL_EXT al
            // setter sintético (módulo).
            if (mSym instanceof PropertySymbol) {
                PropertySymbol ps = (PropertySymbol) mSym;
                // ---------- Property a nivel módulo (importada o local) ----------
                if (ps.ownerClass == null) {
                    switch (a.op) {
                        case ASSIGN: {
                            emitExpr(a.value);
                            coerceToTarget(a.value, tType);
                            if (ps.isExternal) {
                                w.emitCallExt(buildExternalAccessorName(ps, /*setter*/true), ps.externalFromPath);
                            } else {
                                w.emitCall(moduleSetterName(ps.name));
                            }
                            declareLocal("__discard");
                            w.emitSetLocal("__discard");
                            return;
                        }
                        case PLUS_ASSIGN:
                        case MINUS_ASSIGN: {
                            // val_actual := getter(); val_nuevo := val_actual op rhs; setter(val_nuevo)
                            if (ps.isExternal) {
                                w.emitCallExt(buildExternalAccessorName(ps, /*setter*/false), ps.externalFromPath);
                            } else {
                                w.emitCall(moduleGetterName(ps.name));
                            }
                            emitExpr(a.value);
                            coerceToTarget(a.value, tType);
                            emitCompoundOp(a.op, tType);
                            if (ps.isExternal) {
                                w.emitCallExt(buildExternalAccessorName(ps, /*setter*/true), ps.externalFromPath);
                            } else {
                                w.emitCall(moduleSetterName(ps.name));
                            }
                            declareLocal("__discard");
                            w.emitSetLocal("__discard");
                            return;
                        }
                    }
                }
                // ---------- Property de clase (instancia) ----------
                String setter = "set" + capitalize(ps.name);
                switch (a.op) {
                    case ASSIGN: {
                        emitExpr(ma.target);
                        emitExpr(a.value);
                        coerceToTarget(a.value, tType);
                        emitInvokeVirtualSmart(ps.ownerClass, setter, 1);
                        declareLocal("__discard");
                        w.emitSetLocal("__discard");
                        // El FREE del valor anterior lo hace el propio setter
                        // (via SET_FIELD_OWNER del backing field si es owner).
                        // Aquí sólo gestionamos el MOVE: nullificar fuente si es owner.
                        if (ps.isOwner) emitMoveNullifySource(a.value);
                        return;
                    }
                    case PLUS_ASSIGN:
                    case MINUS_ASSIGN: {
                        emitExpr(ma.target);
                        emitExpr(ma.target);
                        emitInvokeVirtualSmart(ps.ownerClass, "get" + capitalize(ps.name), 0);
                        emitExpr(a.value);
                        coerceToTarget(a.value, tType);
                        emitCompoundOp(a.op, tType);
                        emitInvokeVirtualSmart(ps.ownerClass, setter, 1);
                        declareLocal("__discard");
                        w.emitSetLocal("__discard");
                        return;
                    }
                }
            }
            if (!(mSym instanceof VarSymbol)) {
                errors.add("asignación a member-access no es campo ni property: " + ma.member);
                return;
            }
            VarSymbol v = (VarSymbol) mSym;
            if (v.isStatic) {
                String qname = v.ownerClass.name + "." + v.name;
                switch (a.op) {
                    case ASSIGN:
                        emitExpr(a.value);
                        coerceToTarget(a.value, tType);
                        w.emitSetGlobal(qname);
                        return;
                    case PLUS_ASSIGN:
                    case MINUS_ASSIGN:
                        w.emitGetGlobal(qname);
                        emitExpr(a.value);
                        coerceToTarget(a.value, tType);
                        emitCompoundOp(a.op, tType);
                        w.emitSetGlobal(qname);
                        return;
                }
            } else {
                // Instance field via target (this.x o p.x)
                switch (a.op) {
                    case ASSIGN:
                        emitExpr(ma.target);
                        emitExpr(a.value);
                        coerceToTarget(a.value, tType);
                        emitSetFieldAuto(v.ownerClass.name, v.name);
                        if (v.isOwner) emitMoveNullifySource(a.value);
                        return;
                    case PLUS_ASSIGN:
                    case MINUS_ASSIGN: {
                        // Asumimos ma.target sin side-effects (es this.x o p.x):
                        //   push ref, push ref, GET_FIELD → ref, val
                        //   push value, op → ref, newval
                        //   SET_FIELD (pop newval, pop ref)
                        emitExpr(ma.target);
                        emitExpr(ma.target);
                        w.emitGetField(v.ownerClass.name, v.name);
                        emitExpr(a.value);
                        coerceToTarget(a.value, tType);
                        emitCompoundOp(a.op, tType);
                        emitSetFieldAuto(v.ownerClass.name, v.name);
                        return;
                    }
                }
            }
        }
        errors.add("asignación a target no soportado: " + target.getClass().getSimpleName());
    }

    private void emitCompoundOp(AssignOpKind op, BpType tType) throws IOException {
        boolean plus = (op == AssignOpKind.PLUS_ASSIGN);
        if (tType instanceof PrimitiveType && ((PrimitiveType) tType).tag == PrimitiveType.Kind.FLOAT) {
            w.emit(plus ? OpCode.FADD : OpCode.FSUB);
        } else if (isDouble(tType)) {
            // BUG-6 secuela: += / -= sobre double = aritmética de 8 bytes (DADD/DSUB).
            w.emit(plus ? OpCode.DADD : OpCode.DSUB);
        } else if (isLong(tType)) {
            // BUG-6 secuela: += / -= sobre long = aritmética de 8 bytes (LADD/LSUB).
            w.emit(plus ? OpCode.LADD : OpCode.LSUB);
        } else if (tType instanceof PrimitiveType && ((PrimitiveType) tType).tag == PrimitiveType.Kind.STRING) {
            if (!plus) { errors.add("operador -= no aplicable a string"); return; }
            emitStringConcatCall();
        } else {
            w.emit(plus ? OpCode.ADD : OpCode.SUB);
        }
    }

    private void emitIf(IfStmt s) throws IOException {
        // if cond then body
        //   [elseif cond_i body_i]*
        //   [else body_else]
        // endif
        int endLabel = w.newLabel();
        int nextLabel = w.newLabel();

        // then
        emitExpr(s.then_.condition);
        w.emitJumpIfFalse(nextLabel);
        for (IStmt st : s.then_.body) emitStmt(st);
        w.emitJump(endLabel);
        w.declareLabel(nextLabel);

        // elseifs
        for (IfClause ec : s.elseIfs) {
            int falseLabel = w.newLabel();
            emitExpr(ec.condition);
            w.emitJumpIfFalse(falseLabel);
            for (IStmt st : ec.body) emitStmt(st);
            w.emitJump(endLabel);
            w.declareLabel(falseLabel);
        }

        // else
        if (s.else_ != null) {
            for (IStmt st : s.else_) emitStmt(st);
        }

        w.declareLabel(endLabel);
    }

    private void emitWhile(WhileStmt s) throws IOException {
        int top = w.newLabel();
        int end = w.newLabel();
        w.declareLabel(top);
        emitExpr(s.condition);
        w.emitJumpIfFalse(end);
        loopStack.push(new LoopLabels(top, end));
        try {
            for (IStmt st : s.body) emitStmt(st);
        } finally { loopStack.pop(); }
        w.emitJump(top);
        w.declareLabel(end);
    }

    private void emitDoLoop(DoLoopStmt s) throws IOException {
        int top = w.newLabel();
        int end = w.newLabel();
        w.declareLabel(top);
        loopStack.push(new LoopLabels(top, end));
        try {
            for (IStmt st : s.body) emitStmt(st);
        } finally { loopStack.pop(); }
        if (s.condition != null) {
            emitExpr(s.condition);
            // continúa mientras condition sea true → si falsa, sale; si verdadera, vuelve arriba
            w.emitJumpIfFalse(end);
            w.emitJump(top);
        } else {
            w.emitJump(top);
        }
        w.declareLabel(end);
    }

    private void emitFor(ForStmt s) throws IOException {
        if (s.range instanceof ForNumericRange) {
            ForNumericRange r = (ForNumericRange) s.range;
            // i := from
            declareLocal(s.iteratorName);
            emitExpr(r.from);
            w.emitSetLocal(s.iteratorName);
            // pre-compute end (puede ser una expresión arbitraria)
            String endLocal = "__for_end_" + System.identityHashCode(s);
            declareLocal(endLocal);
            emitExpr(r.to);
            w.emitSetLocal(endLocal);
            // step (default 1)
            int step = 1;
            if (r.step != null) {
                if (r.step instanceof IntLitExpr) step = (int) ((IntLitExpr) r.step).value;
                else { errors.add("for step solo soporta literal int por ahora"); return; }
            }
            int top = w.newLabel();
            int end = w.newLabel();
            w.declareLabel(top);
            // Comparación: si step > 0 → mientras i <= end; si step < 0 → mientras i >= end
            w.emitGetLocal(s.iteratorName);
            w.emitGetLocal(endLocal);
            w.emit(step >= 0 ? OpCode.LE : OpCode.GE);
            w.emitJumpIfFalse(end);
            loopStack.push(new LoopLabels(top, end));
            try {
                for (IStmt st : s.body) emitStmt(st);
            } finally { loopStack.pop(); }
            // i += step
            w.emitGetLocal(s.iteratorName);
            emitInt(step);
            w.emit(OpCode.ADD);
            w.emitSetLocal(s.iteratorName);
            w.emitJump(top);
            w.declareLabel(end);
        } else if (s.range instanceof ForInRange) {
            // Desugar: for n in iterable do BODY next n
            //   __arr := iterable
            //   __idx := 0
            //   __end := __arr.length
            //   while __idx < __end do
            //     n := __arr[__idx]
            //     BODY
            //     __idx += 1
            //   endwh
            ForInRange r = (ForInRange) s.range;
            String arrLocal = "__forin_arr_" + Integer.toHexString(System.identityHashCode(s));
            String idxLocal = "__forin_idx_" + Integer.toHexString(System.identityHashCode(s));
            String endLocal = "__forin_end_" + Integer.toHexString(System.identityHashCode(s));
            declareLocal(arrLocal);
            declareLocal(idxLocal);
            declareLocal(endLocal);
            declareLocal(s.iteratorName);

            emitExpr(r.iterable);
            w.emitSetLocal(arrLocal);
            emitInt(0);
            w.emitSetLocal(idxLocal);
            w.emitGetLocal(arrLocal);
            w.emit(OpCode.ALEN);
            w.emitSetLocal(endLocal);

            int top = w.newLabel();
            int end = w.newLabel();
            w.declareLabel(top);
            w.emitGetLocal(idxLocal);
            w.emitGetLocal(endLocal);
            w.emit(OpCode.LT);
            w.emitJumpIfFalse(end);

            // n := __arr[__idx]
            w.emitGetLocal(arrLocal);
            w.emitGetLocal(idxLocal);
            w.emit(OpCode.ALOAD);
            w.emitSetLocal(s.iteratorName);

            loopStack.push(new LoopLabels(top, end));
            try {
                for (IStmt st : s.body) emitStmt(st);
            } finally { loopStack.pop(); }

            w.emitGetLocal(idxLocal);
            emitInt(1);
            w.emit(OpCode.ADD);
            w.emitSetLocal(idxLocal);
            w.emitJump(top);
            w.declareLabel(end);
        } else {
            errors.add("for range desconocido: " + s.range.getClass().getSimpleName());
        }
    }

    private void emitSwitch(SwitchStmt s) throws IOException {
        // Estrategia naïf: guardar subject en local, cadena de comparaciones.
        String subj = "__switch_" + System.identityHashCode(s);
        declareLocal(subj);
        emitExpr(s.subject);
        w.emitSetLocal(subj);

        int end = w.newLabel();
        for (CaseClause cc : s.cases) {
            int bodyLabel = w.newLabel();
            int nextCase = w.newLabel();
            for (int i = 0; i < cc.values.size(); i++) {
                // push subj, push value, EQ → 1 si iguales, 0 si no.
                //   JUMP_IF_FALSE skip   → si distintos, mira el siguiente valor.
                //   JUMP bodyLabel       → si iguales, salta al cuerpo del case.
                w.emitGetLocal(subj);
                emitExpr(cc.values.get(i));
                w.emit(OpCode.EQ);
                int skip = w.newLabel();
                w.emitJumpIfFalse(skip);
                w.emitJump(bodyLabel);
                w.declareLabel(skip);
            }
            w.emitJump(nextCase);
            w.declareLabel(bodyLabel);
            for (IStmt st : cc.body) emitStmt(st);
            w.emitJump(end);
            w.declareLabel(nextCase);
        }
        if (s.defaultBody != null) {
            for (IStmt st : s.defaultBody) emitStmt(st);
        }
        w.declareLabel(end);
    }

    /**
     * Emite un statement {@code parallel ... endpar}. Azúcar sintáctico:
     *   - Para cada rama, ya existe una subclase anónima de Thread sintetizada
     *     a nivel de módulo (ver {@link #synthesizeAllParallelClasses}).
     *   - Aquí emitimos el flujo equivalente:
     *       construir cada thread → start() todos → default body → join() todos.
     *   - Tras endpar las variables {@code tI} ya están joineadas y son
     *     inspeccionables en el scope encerrador.
     */
    private void emitParallel(ParallelStmt p) throws IOException {
        // 1) Construir cada thread y almacenarlo en su variable visible.
        for (ParallelBranch br : p.branches) {
            declareLocal(br.varName);
            int id = tempCounter++;
            String newref  = "__par_newref_"  + id;
            String discard = "__par_discard_" + id;
            declareLocal(newref);
            declareLocal(discard);
            w.emitNewObject(br.synthesizedClassName);
            w.emitSetLocal(newref);
            // Llamar al constructor: this + stackSize
            w.emitGetLocal(newref);
            emitExpr(br.stackSizeExpr);
            coerceToTarget(br.stackSizeExpr, PrimitiveType.INTEGER);
            w.emitCall(br.synthesizedClassName + ".__init");
            w.emitSetLocal(discard);
            // Bind a la variable visible por el usuario.
            w.emitGetLocal(newref);
            w.emitSetLocal(br.varName);
        }
        // 2) start() en cada thread.
        for (ParallelBranch br : p.branches) {
            int id = tempCounter++;
            String discard = "__par_start_discard_" + id;
            declareLocal(discard);
            w.emitGetLocal(br.varName);
            w.emitInvokeVirtual("Thread", "start", 0);
            w.emitSetLocal(discard);   // INVOKE_VIRTUAL deja un dummy en stack
        }
        // 3) Cuerpo del default (si existe): corre en el thread llamador.
        if (p.defaultBody != null) {
            for (IStmt s : p.defaultBody) emitStmt(s);
        }
        // 4) join() en cada thread.
        for (ParallelBranch br : p.branches) {
            int id = tempCounter++;
            String discard = "__par_join_discard_" + id;
            declareLocal(discard);
            w.emitGetLocal(br.varName);
            w.emitInvokeVirtual("Thread", "join", 0);
            w.emitSetLocal(discard);
        }
    }

    /**
     * Pre-scan: recorre TODOS los bodies del módulo (funciones top-level y
     * métodos de clase, recursivamente) buscando {@link ParallelStmt}s y
     * emite la subclase anónima de Thread correspondiente a cada rama.
     * Se llama UNA VEZ desde el pipeline principal antes de las clases del
     * usuario, para que cualquier referencia a {@code __Par_X.__init} dentro
     * de un método o función esté ya resuelta.
     */
    private void synthesizeAllParallelClasses() {
        java.util.List<ParallelStmt> found = new java.util.ArrayList<>();
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof FuncDef) {
                collectParallelStmts(((FuncDef) d).body, found);
            } else if (d instanceof ClassDef) {
                for (ITopLevelDecl m : ((ClassDef) d).members) {
                    if (m instanceof FuncDef) {
                        collectParallelStmts(((FuncDef) m).body, found);
                    } else if (m instanceof PropertyDef) {
                        PropertyDef pd = (PropertyDef) m;
                        if (pd.getter != null) collectParallelStmts(pd.getter.body, found);
                        if (pd.setter != null) collectParallelStmts(pd.setter.body, found);
                    }
                }
            }
        }
        for (ParallelStmt p : found) {
            for (ParallelBranch br : p.branches) {
                try { synthesizeParallelBranchClass(br); }
                catch (IOException e) {
                    errors.add("emit parallel branch '" + br.varName + "': " + e.getMessage());
                }
            }
        }
    }

    /** Walker recursivo: encuentra todos los ParallelStmt en una lista de stmts. */
    private void collectParallelStmts(java.util.List<IStmt> body, java.util.List<ParallelStmt> out) {
        if (body == null) return;
        for (IStmt s : body) {
            if (s instanceof ParallelStmt) {
                ParallelStmt p = (ParallelStmt) s;
                out.add(p);
                for (ParallelBranch br : p.branches) {
                    collectParallelStmts(br.body, out);   // parallels anidados
                }
                if (p.defaultBody != null) collectParallelStmts(p.defaultBody, out);
            } else if (s instanceof IfStmt) {
                IfStmt iff = (IfStmt) s;
                if (iff.then_ != null) collectParallelStmts(iff.then_.body, out);
                for (IfClause ei : iff.elseIfs) collectParallelStmts(ei.body, out);
                if (iff.else_ != null) collectParallelStmts(iff.else_, out);
            } else if (s instanceof WhileStmt) {
                collectParallelStmts(((WhileStmt) s).body, out);
            } else if (s instanceof DoLoopStmt) {
                collectParallelStmts(((DoLoopStmt) s).body, out);
            } else if (s instanceof ForStmt) {
                collectParallelStmts(((ForStmt) s).body, out);
            } else if (s instanceof SwitchStmt) {
                SwitchStmt sw = (SwitchStmt) s;
                for (CaseClause cc : sw.cases) collectParallelStmts(cc.body, out);
                if (sw.defaultBody != null) collectParallelStmts(sw.defaultBody, out);
            } else if (s instanceof TryStmt) {
                TryStmt tr = (TryStmt) s;
                collectParallelStmts(tr.body, out);
                for (CatchClause cl : tr.catches) collectParallelStmts(cl.body, out);
                if (tr.finallyBody != null) collectParallelStmts(tr.finallyBody, out);
            }
        }
    }

    /**
     * Emite UNA subclase anónima de Thread correspondiente a una rama de
     * parallel. La clase tiene:
     *   - Ningún field nuevo (todos los fields se heredan de Thread).
     *   - run() (slot 0 → override de Thread.run()) cuyo cuerpo es br.body.
     *   - Constructor(stackSize) que llama a super(stackSize) = Thread.__init.
     */
    private void synthesizeParallelBranchClass(ParallelBranch br) throws IOException {
        String name = br.synthesizedClassName;
        if (name == null) {
            errors.add("parallel branch sin synthesizedClassName en línea " + br.line);
            return;
        }
        w.addClass(name, "Thread");

        // --- run() (override del slot 0 de Thread)
        w.addMethod("run");
        beginFunctionScope(makeSynthFs("run", null), null);
        try {
            for (IStmt s : br.body) emitStmt(s);
            emitFunctionEnd();
        } finally { scopeStack.pop(); }

        w.endClass();

        // --- Constructor __init(this, stackSize) → super(stackSize)
        w.addFunction(name + ".__init", false);
        w.declareParam("this");
        w.declareParam("stackSize");
        beginFunctionScope(makeSynthFs("__init", null), null);
        try {
            // super(stackSize) → Thread.__init(this, stackSize)
            w.emitGetParam("this");
            w.emitGetParam("stackSize");
            w.emitCall("Thread.__init");
            declareLocal("__discardSuper");
            w.emitSetLocal("__discardSuper");
            emitFunctionEnd();
        } finally { scopeStack.pop(); }
    }

    private void emitReturn(ReturnStmt s) throws IOException {
        FuncScope scope = scopeStack.peek();
        if (scope.returnsValue) {
            if (s.value != null) {
                emitExpr(s.value);
                coerceToTarget(s.value, scope.fs.returnType);
            } else {
                emitInt(0); // 'return' sin valor en función con tipo: usamos 0 como default
            }
            w.emitSetLocal("__result");
        }
        // Recorrer la pila de try frames innermost first. Por cada uno:
        //   - emitir tantos TRY_END como handlers siga teniendo activos
        //     (en un try body con N catches → N; tras entrar al catch → 0;
        //     dentro de un wrap-around → 1).
        //   - inlinear el finally body si lo hay.
        java.util.List<TryFrame> frames = new java.util.ArrayList<>(tryStack);
        for (TryFrame f : frames) {
            for (int i = 0; i < f.activeHandlers; i++) w.emitTryEnd();
            if (f.finallyBody != null) {
                for (IStmt st : f.finallyBody) emitStmt(st);
            }
        }
        // Liberación determinista de owners locales antes de salir.
        emitFreeOwnerLocals();
        // B2: si estamos dentro del cuerpo de un getter/setter de una sync
        //     property, hay que liberar el mutex ANTES del JUMP al endLabel,
        //     porque el unlock de fall-through al final del body quedará
        //     saltado por este JUMP. Sin esto, un return interno deja el
        //     mutex tomado y los siguientes locks petan o se cuelgan.
        if (scope.syncClassName != null) {
            emitSyncUnlock(scope.syncClassName);
        } else if (scope.syncModule) {
            emitModuleSyncUnlock();
        }
        w.emitJump(scope.endLabel);
    }

    /** Emite FREE_REF para cada owner local del scope actual (en orden de
     *  declaración). El VM hace no-op si la ref es null. */
    private void emitFreeOwnerLocals() throws IOException {
        FuncScope scope = scopeStack.peek();
        for (String name : scope.ownerLocals) {
            w.emitGetLocal(name);
            w.emitFreeRef();
        }
    }

    /**
     * Abre un FuncScope nuevo: asigna endLabel, decide returnsValue según el
     * tipo de retorno, y declara la local implícita "__result" si procede.
     * El caller emite los params via declareParam ANTES, y los stmts después.
     */
    private void beginFunctionScope(FunctionSymbol fs, BpType returnTypeOverride) {
        BpType rt = (returnTypeOverride != null) ? returnTypeOverride : fs.returnType;
        boolean returnsValue = rt != null && !(rt instanceof BpType.VoidType);
        boolean returnsLong = is8Byte(rt);   // H1.2/H1.3: return de 8 bytes (long/double) → LRET
        FuncScope scope = new FuncScope(fs, returnsValue, w.newLabel());
        scope.returnsLong = returnsLong;
        scopeStack.push(scope);
        if (returnsValue) {
            // __result va declarada antes que cualquier local de usuario para no
            // depender de un slot fijo: se accede siempre por nombre.
            if (returnsLong) declareLocalLong("__result");   // H1.2 (V2)
            else declareLocal("__result");
        }
    }

    /**
     * Emite el punto único de salida de la función: si el flujo "cae" hasta
     * aquí sin un return explícito, hay que liberar owners locales también.
     * Después declara endLabel y emite RET.
     *
     * Los returns explícitos ya hicieron su propia liberación + JUMP al endLabel,
     * así que entre el JUMP y el endLabel no se ejecuta nada.
     */
    private void emitFunctionEnd() throws IOException {
        FuncScope scope = scopeStack.peek();
        // Fall-through path: liberar owners locales antes del RET.
        emitFreeOwnerLocals();
        w.declareLabel(scope.endLabel);
        if (scope.returnsValue) {
            w.emitGetLocal("__result");
        } else {
            emitInt(0);
        }
        if (scope.returnsLong) w.emitLRet();   // H1.2 (V2): return de 8 bytes
        else w.emitRet();
    }

    private void emitPrint(PrintStmt s) throws IOException {
        for (int i = 0; i < s.items.size(); i++) {
            PrintItem it = s.items.get(i);
            if (i > 0 && it.leadingSep == PrintSep.COMMA) {
                // Espacio entre items separados por coma.
                emitInt(' ');
                w.emit(OpCode.PRINT_CHAR);
            }
            if (it.expr == null) continue;
            emitExpr(it.expr);
            BpType t = info.exprTypes.get(it.expr);
            if (t instanceof PrimitiveType) {
                switch (((PrimitiveType) t).tag) {
                    case INTEGER:
                    // L10 — narrow ints se imprimen como integer (el valor en
                    // pila ya está extendido a i32 por el load).
                    case INT8:
                    case UINT8:
                    case INT16:
                    case UINT16:
                                  w.emit(OpCode.PRINT_NONL); break;
                    case LONG:    w.emit(OpCode.LPRINT_NONL); break;   // H1.2 (V2)
                    case DOUBLE:  w.emit(OpCode.DPRINT_NONL); break;   // H1.3 (V2)
                    case FLOAT:   w.emit(OpCode.FPRINT_NONL); break;
                    case STRING:  w.emit(OpCode.PRINT_STR_NONL); break;
                    case BOOLEAN:
                        // Convertimos a "true"/"false" via builtin BOOL_TO_STRING para
                        // que el output coincida con el backend JVM.
                        w.emit(OpCode.CALL_BUILTIN);
                        w.emitShort((short) Builtin.BOOL_TO_STRING.id);
                        w.emit(OpCode.PRINT_STR_NONL);
                        break;
                }
            } else if (t instanceof EnumType) {
                w.emit(OpCode.PRINT_NONL);
            } else if (t == null || t instanceof BpType.ErrorType) {
                // Sucede con variables de 'catch' genérico (analyzer las tipa como
                // ErrorType). En BP los throws son típicamente strings.
                w.emit(OpCode.PRINT_STR_NONL);
            } else {
                // ref u otro — imprimir como int por ahora
                w.emit(OpCode.PRINT_NONL);
            }
        }
        w.emit(OpCode.PRINT_NL);
    }

    private void emitExprStmt(ExprStmt s) throws IOException {
        // Llamada como statement → descartar el valor de retorno.
        emitExpr(s.expr);
        // Si la expresión devuelve algo, lo descartamos. Para llamadas, RET siempre
        // empuja un return value (aunque sea dummy). Lo descartamos con un SET_LOCAL.
        String discard = "__discard";
        declareLocal(discard);
        w.emitSetLocal(discard);
    }

    private void emitTryStmt(TryStmt s) throws IOException {
        if (s.catches.isEmpty()) {
            errors.add("'try' sin cláusulas catch no soportado todavía");
            return;
        }
        int N = s.catches.size();
        List<IStmt> finallyBody = (s.finallyBody != null && !s.finallyBody.isEmpty())
                ? s.finallyBody : null;

        int endLabel     = w.newLabel();
        int rethrowLabel = (finallyBody != null) ? w.newLabel() : -1;
        int[] handlerLabels = new int[N];
        for (int i = 0; i < N; i++) handlerLabels[i] = w.newLabel();

        // === Emitir N TRY_BEGINs en orden INVERSO ===
        // El último que emitimos queda más arriba en el handler stack ⇒ se
        // prueba primero. Queremos que el primer catch del source (handlerLabels[0])
        // sea el primero en probarse, así que emitimos del último al primero.
        for (int i = N - 1; i >= 0; i--) {
            w.emitTryBegin(handlerLabels[i], s.catches.get(i).exceptionType);
        }

        TryFrame bodyFrame = new TryFrame(finallyBody, N);
        tryStack.push(bodyFrame);
        try {
            for (IStmt st : s.body) emitStmt(st);
        } finally {
            tryStack.pop();
        }
        // Cierre normal del try body: pop N handlers + finally inline + JUMP.
        for (int i = 0; i < N; i++) w.emitTryEnd();
        if (finallyBody != null) {
            for (IStmt st : finallyBody) emitStmt(st);
        }
        w.emitJump(endLabel);

        // === Cada catch handler ===
        for (int i = 0; i < N; i++) {
            CatchClause cc = s.catches.get(i);
            w.declareLabel(handlerLabels[i]);
            if (cc.varName != null) {
                declareLocal(cc.varName);
                w.emitSetLocal(cc.varName);
            } else {
                declareLocal("__discard");
                w.emitSetLocal("__discard");
            }
            // Al entrar al handler, el VM ya pop'eó todos los N handlers.
            // El catch_context lleva activeHandlers=0 pero mantiene el finally
            // para los returns dentro del catch.
            TryFrame catchContext = new TryFrame(finallyBody, 0);
            TryFrame wrapFrame = null;
            if (finallyBody != null) {
                w.emitTryBegin(rethrowLabel, null);
                wrapFrame = new TryFrame(null, 1);
            }
            tryStack.push(catchContext);
            if (wrapFrame != null) tryStack.push(wrapFrame);
            try {
                for (IStmt st : cc.body) emitStmt(st);
            } finally {
                if (wrapFrame != null) tryStack.pop();
                tryStack.pop();
            }
            if (finallyBody != null) {
                w.emitTryEnd();  // pop wrap
                for (IStmt st : finallyBody) emitStmt(st);
            }
            w.emitJump(endLabel);
        }

        // === Rethrow handler compartido (sólo si hay finally) ===
        if (finallyBody != null) {
            w.declareLabel(rethrowLabel);
            declareLocal("__pending_exc");
            w.emitSetLocal("__pending_exc");
            for (IStmt st : finallyBody) emitStmt(st);
            w.emitGetLocal("__pending_exc");
            w.emitThrow();
        }

        w.declareLabel(endLabel);
    }

    private void emitThrowStmt(ThrowStmt s) throws IOException {
        emitExpr(s.value);
        // B2 (parte 2): si estamos dentro del cuerpo de un getter/setter
        // de una sync property, hay que liberar el mutex antes de
        // propagar la excepción. Sin esto, un throw deja el mutex tomado
        // y los próximos lock's del mismo objeto petan con "re-entrada".
        // Las exceptional paths necesitan el mismo tratamiento que
        // `return`, no sólo el flujo normal de salida.
        FuncScope scope = scopeStack.peek();
        if (scope.syncClassName != null) {
            emitSyncUnlock(scope.syncClassName);
        } else if (scope.syncModule) {
            emitModuleSyncUnlock();
        }
        w.emitThrow();
    }

    private void emitBreak() throws IOException {
        if (loopStack.isEmpty()) { errors.add("break fuera de bucle"); return; }
        w.emitJump(loopStack.peek().breakLabel);
    }

    private void emitContinue() throws IOException {
        if (loopStack.isEmpty()) { errors.add("continue fuera de bucle"); return; }
        w.emitJump(loopStack.peek().continueLabel);
    }

    // ============================================================
    // EXPRESIONES
    // ============================================================

    private void emitExpr(IExpr e) throws IOException {
        if (e instanceof IntLitExpr)       emitInt((int) ((IntLitExpr) e).value);
        else if (e instanceof LongLitExpr) {   // H1.2 (V2): LPUSH + 8 bytes
            w.emit(OpCode.LPUSH);
            w.emitLong(((LongLitExpr) e).value);
        }
        else if (e instanceof DoubleLitExpr) {   // H1.3 (V2): DPUSH + 8 bytes (bits raw f64)
            w.emit(OpCode.DPUSH);
            w.emitLong(Double.doubleToRawLongBits(((DoubleLitExpr) e).value));
        }
        else if (e instanceof FloatLitExpr) {
            w.emit(OpCode.FPUSH);
            // ModWriter no expone emitPushFloat directamente; lo escribimos a mano.
            // Pero sí tiene emitPushFloat: lo emite con el opcode FPUSH y los bits.
            // Aquí ya emití el opcode, así que sigo manualmente con emitInt de los bits.
            // OJO: si FPUSH tiene un operando IMM_F32 de 4 bytes, debo escribir los bits float.
            // ModWriter.emitInt(...) escribe 4 bytes de un int al codeStream.
            w.emitInt(Float.floatToRawIntBits((float) ((FloatLitExpr) e).value));
        }
        else if (e instanceof StringLitExpr) {
            String name = internString(((StringLitExpr) e).value);
            w.emitLeaGlobal(name);
        }
        else if (e instanceof BoolLitExpr)  emitInt(((BoolLitExpr) e).value ? 1 : 0);
        else if (e instanceof NullLitExpr)  emitInt(0);
        else if (e instanceof IdentifierExpr) emitIdentifier((IdentifierExpr) e);
        else if (e instanceof UnaryExpr)    emitUnary((UnaryExpr) e);
        else if (e instanceof BinaryExpr)   emitBinary((BinaryExpr) e);
        else if (e instanceof ParenExpr)    emitExpr(((ParenExpr) e).inner);
        else if (e instanceof CallExpr)         emitCall((CallExpr) e);
        else if (e instanceof MemberAccessExpr) emitMemberAccess((MemberAccessExpr) e);
        else if (e instanceof ThisExpr)         w.emitGetParam("this");
        else if (e instanceof SuperExpr)        w.emitGetParam("this");  // como expresión, super == this con tipo del padre
        else if (e instanceof SuperCallExpr)    emitSuperCall((SuperCallExpr) e);
        else if (e instanceof ArrayLitExpr)     emitArrayLit((ArrayLitExpr) e);
        else if (e instanceof IndexExpr)        emitIndexLoad((IndexExpr) e);
        else if (e instanceof FieldExpr)        emitFieldExpr();
        else if (e instanceof InstanceOfExpr)   emitInstanceOf((InstanceOfExpr) e);
        else errors.add("expresión no soportada: " + e.getClass().getSimpleName());
    }

    private void emitIdentifier(IdentifierExpr id) throws IOException {
        Symbol sym = info.exprSymbols.get(id);
        if (sym == null) { errors.add("identificador sin símbolo: " + id.name); return; }
        loadFromSymbol(sym, id.name);
    }

    private void emitUnary(UnaryExpr u) throws IOException {
        emitExpr(u.operand);
        BpType t = info.exprTypes.get(u.operand);
        if ("-".equals(u.op)) {
            if (isFloat(t)) {
                w.emit(OpCode.FNEG);
            } else if (isLong(t)) {   // H1.2 (V2)
                w.emit(OpCode.LNEG);
            } else if (isDouble(t)) {   // H1.3 (V2)
                w.emit(OpCode.DNEG);
            } else {
                w.emit(OpCode.NEG);
            }
        } else if ("not".equals(u.op)) {
            w.emit(OpCode.NOT);
        } else {
            errors.add("operador unario no soportado: " + u.op);
        }
    }

    private void emitBinary(BinaryExpr b) throws IOException {
        BpType tl = info.exprTypes.get(b.left);
        BpType tr = info.exprTypes.get(b.right);
        BpType tb = info.exprTypes.get(b);
        boolean stringOp = (tl instanceof PrimitiveType && ((PrimitiveType) tl).tag == PrimitiveType.Kind.STRING)
                          || (tr instanceof PrimitiveType && ((PrimitiveType) tr).tag == PrimitiveType.Kind.STRING);

        if ("+".equals(b.op) && stringOp) {
            emitExpr(b.left);
            coerceToString(tl);
            emitExpr(b.right);
            coerceToString(tr);
            emitStringConcatCall();
            return;
        }

        // Igualdad de strings: BP semánticamente compara contenido, no
        // referencia. El opcode EQ del VM compara enteros, así que para
        // string ==/string != tenemos que llamar a __strequals (helper
        // inyectado) que recorre los chars.
        if (("==".equals(b.op) || "!=".equals(b.op)) && stringOp) {
            emitExpr(b.left);
            coerceToString(tl);
            emitExpr(b.right);
            coerceToString(tr);
            emitStringEqualsCall();
            if ("!=".equals(b.op)) {
                w.emit(OpCode.NOT);
            }
            return;
        }

        // -------- AND / OR con short-circuit --------
        // Los lógicos `and` y `or` evalúan en cortocircuito: si el LHS ya
        // determina el resultado, el RHS NO se evalúa. Esto importa
        // semánticamente cuando RHS tiene efectos colaterales (llamadas
        // a funciones con side effects, asignaciones, lectura de HW, etc.)
        // y suele ser idiomático en BP/Pascal escribir cosas como:
        //   if obj != null and obj.value > 0 then ...
        // donde el RHS no es seguro si LHS es false.
        //
        // `and also` / `or else` se tratan igual (mismo nombre, mismo
        // patrón) — alias heredados de Ada que pedían short-circuit
        // explícito.
        if ("and".equals(b.op) || "and also".equals(b.op)) {
            emitShortCircuitAnd(b);
            return;
        }
        if ("or".equals(b.op) || "or else".equals(b.op)) {
            emitShortCircuitOr(b);
            return;
        }

        // H1.3 (V2): torre de promoción double > float > long > int.
        boolean useDouble = isDouble(tl) || isDouble(tr) || isDouble(tb);
        boolean useFloat  = !useDouble && (isFloat(tl) || isFloat(tr) || isFloat(tb));
        boolean useLong   = !useDouble && !useFloat && (isLong(tl) || isLong(tr) || isLong(tb));
        emitExpr(b.left);
        promoteNumeric(tl, useDouble, useFloat, useLong);
        emitExpr(b.right);
        promoteNumeric(tr, useDouble, useFloat, useLong);

        switch (b.op) {
            case "+": w.emit(useDouble ? OpCode.DADD : useFloat ? OpCode.FADD : useLong ? OpCode.LADD : OpCode.ADD); break;
            case "-": w.emit(useDouble ? OpCode.DSUB : useFloat ? OpCode.FSUB : useLong ? OpCode.LSUB : OpCode.SUB); break;
            case "*": w.emit(useDouble ? OpCode.DMUL : useFloat ? OpCode.FMUL : useLong ? OpCode.LMUL : OpCode.MUL); break;
            case "/": w.emit(useDouble ? OpCode.DDIV : useFloat ? OpCode.FDIV : useLong ? OpCode.LDIV : OpCode.DIV); break;
            case "mod": w.emit(useDouble ? OpCode.DMOD : useFloat ? OpCode.FMOD : useLong ? OpCode.LMOD : OpCode.MOD); break;
            case "==": w.emit(useDouble ? OpCode.DEQ : useFloat ? OpCode.FEQ : useLong ? OpCode.LEQ : OpCode.EQ); break;
            case "!=": w.emit(useDouble ? OpCode.DNEQ : useFloat ? OpCode.FNEQ : useLong ? OpCode.LNEQ : OpCode.NEQ); break;
            case "<":  w.emit(useDouble ? OpCode.DLT : useFloat ? OpCode.FLT : useLong ? OpCode.LLT : OpCode.LT); break;
            case "<=": w.emit(useDouble ? OpCode.DLE : useFloat ? OpCode.FLE : useLong ? OpCode.LLE : OpCode.LE); break;
            case ">":  w.emit(useDouble ? OpCode.DGT : useFloat ? OpCode.FGT : useLong ? OpCode.LGT : OpCode.GT); break;
            case ">=": w.emit(useDouble ? OpCode.DGE : useFloat ? OpCode.FGE : useLong ? OpCode.LGE : OpCode.GE); break;
            case "&": w.emit(useLong ? OpCode.LBAND : OpCode.BAND); break;
            case "|": w.emit(useLong ? OpCode.LBOR : OpCode.BOR); break;
            case "xor": w.emit(useLong ? OpCode.LBXOR : OpCode.BXOR); break;
            case "shl": w.emit(useLong ? OpCode.LSHL : OpCode.SHL); break;
            case "shr": w.emit(useLong ? OpCode.LSHR_S : OpCode.SHR_S); break;
            default:
                errors.add("operador binario no soportado: " + b.op);
        }
    }

    /**
     * AND lógico con short-circuit. Si LHS evalúa a false el RHS NO se
     * evalúa, dejando el resultado a false directamente.
     *
     * Esquema emitido:
     * <pre>
     *     emit LHS
     *     JUMP_IF_FALSE shortL    ; si LHS == false → salta sin evaluar RHS
     *     emit RHS                ; resultado = RHS
     *     JUMP end
     * shortL:
     *     PUSH 0                  ; resultado = false
     * end:
     * </pre>
     */
    private void emitShortCircuitAnd(BinaryExpr b) throws IOException {
        int shortL = w.newLabel();
        int end    = w.newLabel();
        emitExpr(b.left);
        w.emitJumpIfFalse(shortL);
        emitExpr(b.right);
        w.emitJump(end);
        w.declareLabel(shortL);
        w.emitPushInt(0);
        w.declareLabel(end);
    }

    /**
     * OR lógico con short-circuit. Si LHS evalúa a true el RHS NO se
     * evalúa, dejando el resultado a true directamente.
     *
     * Como la VM solo expone JUMP_IF_FALSE (no JUMP_IF_TRUE), negamos
     * LHS con NOT antes del salto y aprovechamos el mismo opcode:
     *
     * <pre>
     *     emit LHS
     *     NOT                     ; LHS true → 0, LHS false → 1
     *     JUMP_IF_FALSE shortL    ; salta si !LHS == false, o sea, si LHS era true
     *     emit RHS                ; resultado = RHS (caso LHS false)
     *     JUMP end
     * shortL:
     *     PUSH 1                  ; resultado = true (caso LHS true)
     * end:
     * </pre>
     *
     * Coste: 1 NOT extra vs. AND short-circuit, pero seguimos ahorrando
     * la evaluación completa de RHS, que era el objetivo.
     */
    private void emitShortCircuitOr(BinaryExpr b) throws IOException {
        int shortL = w.newLabel();
        int end    = w.newLabel();
        emitExpr(b.left);
        w.emit(OpCode.NOT);
        w.emitJumpIfFalse(shortL);
        emitExpr(b.right);
        w.emitJump(end);
        w.declareLabel(shortL);
        w.emitPushInt(1);
        w.declareLabel(end);
    }

    private void emitCall(CallExpr c) throws IOException {
        // H1.3b (V2) — casts numéricos generales: integer()/long()/float()/double().
        // No son símbolos; se reconocen por nombre y emiten la conversión
        // type-directed según el tipo del argumento.
        if (c.callee instanceof IdentifierExpr
                && isNumericCastName(((IdentifierExpr) c.callee).name)
                && c.args.size() == 1) {
            emitExpr(c.args.get(0));
            emitNumericConversion(info.exprTypes.get(c.args.get(0)), ((IdentifierExpr) c.callee).name);
            return;
        }
        // Caso 1: callee es IdentifierExpr → función top-level o construction.
        if (c.callee instanceof IdentifierExpr) {
            Symbol sym = info.exprSymbols.get(c.callee);
            if (sym instanceof ClassSymbol) {
                emitConstruction((ClassSymbol) sym, c.args);
                return;
            }
            if (sym instanceof FunctionSymbol) {
                FunctionSymbol fs = (FunctionSymbol) sym;
                if (fs.isConstructor && fs.ownerClass != null) {
                    emitConstruction(fs.ownerClass, c.args);
                    return;
                }
                // ¿Built-in del analyzer? → CALL_BUILTIN id, salvo casts L10.
                if (SemanticAnalyzer.isBuiltin(fs)) {
                    // L10 — casts a tipos estrechos: NO son CALL_BUILTIN, son un
                    // opcode único I32_TO_{U8,I8,U16,I16} tras evaluar el arg.
                    // El check de rango lo hace la VM (RuntimeError si se sale).
                    OpCode castOp = narrowCastOpFor(fs.name);
                    if (castOp != null) {
                        if (c.args.size() != 1) {
                            errors.add(fs.name + "(): se esperaba 1 argumento");
                            return;
                        }
                        emitExpr(c.args.get(0));
                        coerceToTarget(c.args.get(0), PrimitiveType.INTEGER);
                        try { w.emit(castOp); }
                        catch (IOException ex) { errors.add("emit " + castOp + ": " + ex.getMessage()); }
                        return;
                    }
                    // H1.1 (V2) — alocadores de arrays narrow (newByteArray):
                    // NO son CALL_BUILTIN, son un opcode NEWARRAY_* directo
                    // (la VM ya lo implementa → cero cambio de VM). Igual
                    // patrón que los casts L10 de arriba.
                    OpCode arrAllocOp = arrayAllocOpFor(fs.name);
                    if (arrAllocOp != null) {
                        if (c.args.size() != 1) {
                            errors.add(fs.name + "(): se esperaba 1 argumento");
                            return;
                        }
                        emitExpr(c.args.get(0));
                        coerceToTarget(c.args.get(0), PrimitiveType.INTEGER);
                        try { w.emit(arrAllocOp); }
                        catch (IOException ex) { errors.add("emit " + arrAllocOp + ": " + ex.getMessage()); }
                        return;
                    }
                    Builtin b = Builtin.byName(fs.name);
                    if (b == null) {
                        errors.add("built-in '" + fs.name + "' no soportado en miVM");
                        return;
                    }
                    for (int i = 0; i < c.args.size(); i++) {
                        emitExpr(c.args.get(i));
                        if (i < fs.params.size()) coerceToTarget(c.args.get(i), fs.params.get(i).type);
                    }
                    try {
                        w.emit(OpCode.CALL_BUILTIN);
                        w.emitShort((short) b.id);
                    } catch (IOException ex) { errors.add("emit CALL_BUILTIN: " + ex.getMessage()); }
                    return;
                }
                // Top-level function (módulo o ya cualificado).
                // Si es external (cargada desde un .bpi), pasa por CALL_EXT salvo
                // que sea una función intrinsic — en ese caso, el emisor mete
                // opcodes en lugar de generar una llamada (los args quedan en pila
                // exactamente igual; el lambda del registro asume ese contrato).
                for (int i = 0; i < c.args.size(); i++) {
                    emitExpr(c.args.get(i));
                    if (i < fs.params.size()) coerceToTarget(c.args.get(i), fs.params.get(i).type);
                }
                if (fs.isIntrinsic) {
                    String qn = (fs.isExternal ? fs.externalModule : (moduleAst != null ? moduleAst.name : "?"))
                            + "." + fs.name;
                    Intrinsics.IntrinsicEmitter ie = Intrinsics.lookup(qn);
                    if (ie == null) {
                        errors.add("intrínseco no implementado en el emisor mivm: " + qn);
                        return;
                    }
                    ie.emit(w);
                    return;
                }
                if (fs.isExternal) {
                    w.emitCallExt(fs.externalQualifiedName(), fs.externalFromPath);
                } else {
                    w.emitCall(fs.name);
                }
                return;
            }
            errors.add("call: identificador no resuelve a función/clase: " + ((IdentifierExpr) c.callee).name);
            return;
        }

        // Caso 2: callee es MemberAccessExpr → método estático / instancia / super.X.
        if (c.callee instanceof MemberAccessExpr) {
            MemberAccessExpr ma = (MemberAccessExpr) c.callee;
            Symbol mSym = info.exprSymbols.get(c.callee);
            // L2: `Module.ClassName(args)` — construcción cross-module.
            if (mSym instanceof ClassSymbol) {
                emitConstruction((ClassSymbol) mSym, c.args);
                return;
            }
            if (!(mSym instanceof FunctionSymbol)) {
                errors.add("call: member-access no resuelve a función: " + ma.member);
                return;
            }
            FunctionSymbol fs = (FunctionSymbol) mSym;
            // Cross-library: Foo.bar() donde Foo es un módulo importado.
            // FunctionSymbol viene con isExternal y ownerClass==null.
            if (fs.isExternal) {
                for (int i = 0; i < c.args.size(); i++) {
                    emitExpr(c.args.get(i));
                    if (i < fs.params.size()) coerceToTarget(c.args.get(i), fs.params.get(i).type);
                }
                if (fs.isIntrinsic) {
                    // Intrínseco cross-module: en lugar de CALL_EXT, el emisor
                    // mete opcodes inline. No se añade entrada a la tabla de
                    // imports, así que el .mod del módulo dueño no es necesario
                    // en runtime.
                    String qn = fs.externalModule + "." + fs.name;
                    Intrinsics.IntrinsicEmitter ie = Intrinsics.lookup(qn);
                    if (ie == null) {
                        errors.add("intrínseco no implementado en el emisor mivm: " + qn);
                        return;
                    }
                    ie.emit(w);
                    return;
                }
                w.emitCallExt(fs.externalQualifiedName(), fs.externalFromPath);
                return;
            }
            if (fs.isStatic) {
                for (int i = 0; i < c.args.size(); i++) {
                    emitExpr(c.args.get(i));
                    if (i < fs.params.size()) coerceToTarget(c.args.get(i), fs.params.get(i).type);
                }
                w.emitCall(fs.ownerClass.name + "." + fs.name);
                return;
            }
            // Método de instancia: ¿es super.foo() o p.foo()?
            if (ma.target instanceof SuperExpr) {
                // Direct call al método del padre (sin vtable):
                //   push this, push args, CALL Parent.simpleName
                w.emitGetParam("this");
                for (int i = 0; i < c.args.size(); i++) {
                    emitExpr(c.args.get(i));
                    if (i < fs.params.size()) coerceToTarget(c.args.get(i), fs.params.get(i).type);
                }
                w.emitCall(fs.ownerClass.name + "." + fs.name);
                return;
            }
            // Despacho. push receiver, push args.
            //   - PÚBLICO: INVOKE_VIRTUAL por slot/nombre (polimorfismo).
            //   - PRIVADO (no-public, no-external): CALL directo "Cls.metodo"
            //     (BUG-4 / Opción A: no está en la vtable). El receiver ya
            //     empujado hace de `this`, igual que super.foo().
            emitExpr(ma.target);
            for (int i = 0; i < c.args.size(); i++) {
                emitExpr(c.args.get(i));
                if (i < fs.params.size()) coerceToTarget(c.args.get(i), fs.params.get(i).type);
            }
            if (!fs.isPublic && !fs.isExternal) {
                w.emitCall(fs.ownerClass.name + "." + fs.name);
            } else {
                // BUG-6: numArgs = slots de 4 bytes (long/double = 2), no nº de args.
                emitInvokeVirtualSmart(fs.ownerClass, fs.name, argSlotCount(fs));
            }
            return;
        }
        errors.add("call con callee no soportado: " + c.callee.getClass().getSimpleName());
    }

    /**
     * Despacha INVOKE_VIRTUAL al modo correcto según si la clase es externa.
     * Para clases locales usa la lookup normal por nombre de método; para
     * clases importadas (.bpi v4 ClassSig) usa el slot precalculado del stub.
     */
    private void emitInvokeVirtualSmart(ClassSymbol cls, String methodName, int nArgs) throws IOException {
        if (cls.isExternal) {
            Integer slot = cls.externalMethodSlots.get(methodName);
            if (slot == null) {
                errors.add("método externo no encontrado en stub: " + cls.name + "." + methodName);
                return;
            }
            w.emitInvokeVirtualBySlot(slot, nArgs);
        } else {
            w.emitInvokeVirtual(cls.name, methodName, nArgs);
        }
    }

    private void emitArrayLit(ArrayLitExpr a) throws IOException {
        int n = a.elements.size();
        // Reservamos un temp único para no pisar otros __arr concurrentes.
        String tmp = "__arr_" + Integer.toHexString(System.identityHashCode(a));
        declareLocal(tmp);
        emitInt(n);
        w.emit(OpCode.NEWARRAY);
        w.emitSetLocal(tmp);
        for (int i = 0; i < n; i++) {
            w.emitGetLocal(tmp);
            emitInt(i);
            emitExpr(a.elements.get(i));
            // TODO: coerce a tipo del elemento si supiéramos el tipo array de contexto.
            w.emit(OpCode.ASTORE);
        }
        w.emitGetLocal(tmp);
    }

    private void emitIndexLoad(IndexExpr ix) throws IOException {
        emitExpr(ix.target);
        emitExpr(ix.index);
        w.emit(aloadOpFor(ix.target));
    }

    /**
     * H1.1b (V2) — opcode de carga de elemento según el tipo de elemento del
     * array. `byte`/`word`/... son almacenamiento estrecho (1-2 bytes/elem);
     * el load extiende el valor a i32 en la pila (zero-ext para unsigned,
     * sign-ext para signed). Si el array no es de un tipo narrow conocido,
     * cae al ALOAD genérico de 4 bytes (comportamiento previo intacto).
     */
    private OpCode aloadOpFor(IExpr arrayExpr) {
        BpType t = info.exprTypes.get(arrayExpr);
        if (t instanceof ArrayType) {
            BpType el = ((ArrayType) t).element;
            if (el instanceof PrimitiveType) {
                switch (((PrimitiveType) el).tag) {
                    case UINT8:  return OpCode.ALOAD_U8;
                    case INT8:   return OpCode.ALOAD_I8;
                    case UINT16: return OpCode.ALOAD_U16;
                    case INT16:  return OpCode.ALOAD_I16;
                    case LONG:   return OpCode.ALOAD_I64;   // H1.2 (V2)
                    case DOUBLE: return OpCode.ALOAD_I64;   // H1.3 (V2): 8 bytes opacos
                    default:     break;
                }
            }
        }
        return OpCode.ALOAD;
    }

    /**
     * H1.1b (V2) — opcode de store de elemento según el tipo de elemento. El
     * store trunca el i32 de la pila al ancho del elemento (idéntico para
     * signed/unsigned: misma truncación de bits). Default: ASTORE de 4 bytes.
     */
    private OpCode astoreOpFor(IExpr arrayExpr) {
        BpType t = info.exprTypes.get(arrayExpr);
        if (t instanceof ArrayType) {
            BpType el = ((ArrayType) t).element;
            if (el instanceof PrimitiveType) {
                switch (((PrimitiveType) el).tag) {
                    case UINT8: case INT8:    return OpCode.ASTORE_I8;
                    case UINT16: case INT16:  return OpCode.ASTORE_I16;
                    case LONG:                return OpCode.ASTORE_I64;   // H1.2 (V2)
                    case DOUBLE:              return OpCode.ASTORE_I64;   // H1.3 (V2)
                    default:                  break;
                }
            }
        }
        return OpCode.ASTORE;
    }

    /**
     * Sintetiza la clase built-in RuntimeError en el .mod actual. Layout:
     *   class RuntimeError { var msg: string }
     *   RuntimeError.__init(this, msg) { this.msg := msg }
     * Necesario para que `throw RuntimeError(msg)` y `catch err: RuntimeError` tengan
     * un descriptor de clase real en data block.
     */
    private void synthesizeRuntimeErrorClass() {
        // Si el módulo del usuario ya declara su propia clase RuntimeError,
        // dejamos la suya (mayor flexibilidad). El built-in solo se inyecta
        // como fallback.
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof ClassDef && "RuntimeError".equals(((ClassDef) d).name)) {
                return;
            }
        }
        try {
            w.addClass("RuntimeError", null);
            w.declareField("msg", true);
            w.endClass();
            // B3 v2 — exportamos el descriptor para que builtins nativos
            // del VM puedan instanciar RuntimeError sin parsear el data
            // block. Nombre convencional: `__cls_RuntimeError`.
            w.exportDataSymbol("RuntimeError");

            w.addFunction("RuntimeError.__init", false);
            w.declareParam("this");
            w.declareParam("msg");
            FunctionSymbol synth = new FunctionSymbol("__init", false, false, false, null, null);
            beginFunctionScope(synth, null);  // void
            try {
                w.emitGetParam("this");
                w.emitGetParam("msg");
                w.emitSetField("RuntimeError", "msg");
                emitFunctionEnd();
            } finally {
                scopeStack.pop();
            }
        } catch (IOException e) {
            errors.add("emit RuntimeError builtin: " + e.getMessage());
        }
    }

    /** Helper: emite CALL_BUILTIN <id>. */
    private void emitBuiltinCall(Builtin b) {
        try {
            w.emit(OpCode.CALL_BUILTIN);
            w.emitShort((short) b.id);
        } catch (IOException e) {
            errors.add("emit CALL_BUILTIN(" + b.name() + "): " + e.getMessage());
        }
    }

    /**
     * Sintetiza la clase stdlib List: lista dinámica de refs a objetos.
     * Layout:
     *   class List { var items: integer[];  var size: integer;  var cap: integer; }
     * Métodos: add(item), get(idx): any, set(idx,item), length(): integer, remove(idx).
     */
    private void synthesizeListClass() {
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof ClassDef && "List".equals(((ClassDef) d).name)) return;
        }
        try {
            w.addClass("List", null);
            w.declareField("items", true);
            w.declareField("size",  false);
            w.declareField("cap",   false);

            // -------------------------- add(item) --------------------------
            w.addMethod("add");
            w.declareParam("item");
            beginFunctionScope(makeSynthFs("add", null), null);
            try {
                int afterGrow = w.newLabel();
                // if size >= cap → grow
                w.emitGetParam("this"); w.emitGetField("List", "size");
                w.emitGetParam("this"); w.emitGetField("List", "cap");
                w.emit(OpCode.GE);                          // 1 = grow, 0 = skip
                w.emitJumpIfFalse(afterGrow);

                // cap := cap * 2
                w.emitGetParam("this");
                w.emitGetParam("this"); w.emitGetField("List", "cap");
                emitInt(2); w.emit(OpCode.MUL);
                w.emitSetField("List", "cap");

                // items := __growRefArray(items, cap)
                w.emitGetParam("this");
                w.emitGetParam("this"); w.emitGetField("List", "items");
                w.emitGetParam("this"); w.emitGetField("List", "cap");
                emitBuiltinCall(Builtin.GROW_REF_ARRAY);
                w.emitSetField("List", "items");

                w.declareLabel(afterGrow);

                // items[size] := item
                w.emitGetParam("this"); w.emitGetField("List", "items");
                w.emitGetParam("this"); w.emitGetField("List", "size");
                w.emitGetParam("item");
                w.emit(OpCode.ASTORE);

                // size := size + 1
                w.emitGetParam("this");
                w.emitGetParam("this"); w.emitGetField("List", "size");
                emitInt(1); w.emit(OpCode.ADD);
                w.emitSetField("List", "size");

                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // -------------------------- get(idx): any --------------------------
            w.addMethod("get");
            w.declareParam("idx");
            beginFunctionScope(makeSynthFs("get", BpType.AnyType.INSTANCE), null);
            try {
                // __result := items[idx]
                w.emitGetParam("this"); w.emitGetField("List", "items");
                w.emitGetParam("idx");
                w.emit(OpCode.ALOAD);
                w.emitSetLocal("__result");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // -------------------------- set(idx, item) --------------------------
            w.addMethod("set");
            w.declareParam("idx");
            w.declareParam("item");
            beginFunctionScope(makeSynthFs("set", null), null);
            try {
                w.emitGetParam("this"); w.emitGetField("List", "items");
                w.emitGetParam("idx");
                w.emitGetParam("item");
                w.emit(OpCode.ASTORE);
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // -------------------------- length(): integer --------------------------
            w.addMethod("length");
            beginFunctionScope(makeSynthFs("length", PrimitiveType.INTEGER), null);
            try {
                w.emitGetParam("this"); w.emitGetField("List", "size");
                w.emitSetLocal("__result");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // -------------------------- remove(idx) --------------------------
            // for (i := idx; i < size-1; i++) items[i] := items[i+1]
            // size := size - 1
            // items[size] := 0   (limpia el slot final para que el GC libere)
            w.addMethod("remove");
            w.declareParam("idx");
            beginFunctionScope(makeSynthFs("remove", null), null);
            try {
                declareLocal("i");
                // i := idx
                w.emitGetParam("idx"); w.emitSetLocal("i");

                int loopTop = w.newLabel();
                int loopEnd = w.newLabel();
                w.declareLabel(loopTop);

                // while i < size-1
                w.emitGetLocal("i");
                w.emitGetParam("this"); w.emitGetField("List", "size");
                emitInt(1); w.emit(OpCode.SUB);
                w.emit(OpCode.LT);
                w.emitJumpIfFalse(loopEnd);

                // items[i] := items[i+1]
                w.emitGetParam("this"); w.emitGetField("List", "items");  // arr (target)
                w.emitGetLocal("i");                                       // idx
                w.emitGetParam("this"); w.emitGetField("List", "items");  // arr (source)
                w.emitGetLocal("i"); emitInt(1); w.emit(OpCode.ADD);       // i+1
                w.emit(OpCode.ALOAD);                                       // val = items[i+1]
                w.emit(OpCode.ASTORE);                                      // items[i] := val

                // i := i + 1
                w.emitGetLocal("i"); emitInt(1); w.emit(OpCode.ADD);
                w.emitSetLocal("i");

                w.emitJump(loopTop);
                w.declareLabel(loopEnd);

                // size := size - 1
                w.emitGetParam("this");
                w.emitGetParam("this"); w.emitGetField("List", "size");
                emitInt(1); w.emit(OpCode.SUB);
                w.emitSetField("List", "size");

                // items[size] := 0   (deja libre la última ref para GC)
                w.emitGetParam("this"); w.emitGetField("List", "items");
                w.emitGetParam("this"); w.emitGetField("List", "size");
                emitInt(0);
                w.emit(OpCode.ASTORE);

                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // -------------------------- endClass + Constructor --------------------------
            w.endClass();

            w.addFunction("List.__init", false);
            w.declareParam("this");
            beginFunctionScope(makeSynthFs("__init", null), null);
            try {
                w.emitGetParam("this");
                emitInt(8); emitBuiltinCall(Builtin.NEW_REF_ARRAY);
                w.emitSetField("List", "items");
                w.emitGetParam("this"); emitInt(0); w.emitSetField("List", "size");
                w.emitGetParam("this"); emitInt(8); w.emitSetField("List", "cap");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }
        } catch (IOException e) {
            errors.add("emit List builtin: " + e.getMessage());
        }
    }

    /**
     * Sintetiza OwnerList: List propietaria de sus elementos.
     * - Hereda de List todos los fields (items, size, cap) y vtable
     *   (add, get, set, length, remove).
     * - Promueve el field `items` a OWNER en su propio class descriptor.
     *   En tiempo de FREE_REF, el VM verá items marcado como owner y, como
     *   el array es de tipo TYPE_ARRAY_REF, cascadeará liberando primero
     *   cada slot (los objetos contenidos) y después el array mismo.
     * - El constructor `OwnerList()` simplemente delega en `List.__init`
     *   (mismos defaults: cap=8, array vacío).
     */
    private void synthesizeOwnerListClass() {
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof ClassDef && "OwnerList".equals(((ClassDef) d).name)) return;
        }
        try {
            w.addClass("OwnerList", "List");   // hereda fields + métodos
            w.setFieldOwner("items", true);     // ← clave de la cascada

            // -------------------------- removeAndFree(idx) --------------------------
            // 1. FREE_REF(items[idx])   ← libera el objeto (cascadea si tiene owners)
            // 2. items[idx] := 0        ← evita doble-free durante el shift
            // 3. shift items[i] := items[i+1] para i = idx .. size-2
            // 4. size := size - 1
            // 5. items[size] := 0
            w.addMethod("removeAndFree");
            w.declareParam("idx");
            beginFunctionScope(makeSynthFs("removeAndFree", null), null);
            try {
                declareLocal("i");

                // 1. liberar items[idx]
                w.emitGetParam("this"); w.emitGetField("OwnerList", "items");
                w.emitGetParam("idx");
                w.emit(OpCode.ALOAD);          // push items[idx]
                w.emitFreeRef();                // pop & free (cascade si owner)

                // 2. items[idx] := 0
                w.emitGetParam("this"); w.emitGetField("OwnerList", "items");
                w.emitGetParam("idx");
                emitInt(0);
                w.emit(OpCode.ASTORE);

                // 3. for (i := idx; i < size-1; i++): items[i] := items[i+1]
                w.emitGetParam("idx");
                w.emitSetLocal("i");

                int top = w.newLabel();
                int end = w.newLabel();
                w.declareLabel(top);
                w.emitGetLocal("i");
                w.emitGetParam("this"); w.emitGetField("OwnerList", "size");
                emitInt(1); w.emit(OpCode.SUB);
                w.emit(OpCode.LT);
                w.emitJumpIfFalse(end);

                // items[i] := items[i+1]
                w.emitGetParam("this"); w.emitGetField("OwnerList", "items");
                w.emitGetLocal("i");
                w.emitGetParam("this"); w.emitGetField("OwnerList", "items");
                w.emitGetLocal("i"); emitInt(1); w.emit(OpCode.ADD);
                w.emit(OpCode.ALOAD);
                w.emit(OpCode.ASTORE);

                // i := i + 1
                w.emitGetLocal("i"); emitInt(1); w.emit(OpCode.ADD);
                w.emitSetLocal("i");
                w.emitJump(top);
                w.declareLabel(end);

                // 4. size := size - 1
                w.emitGetParam("this");
                w.emitGetParam("this"); w.emitGetField("OwnerList", "size");
                emitInt(1); w.emit(OpCode.SUB);
                w.emitSetField("OwnerList", "size");

                // 5. items[size] := 0
                w.emitGetParam("this"); w.emitGetField("OwnerList", "items");
                w.emitGetParam("this"); w.emitGetField("OwnerList", "size");
                emitInt(0);
                w.emit(OpCode.ASTORE);

                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            w.endClass();

            w.addFunction("OwnerList.__init", false);
            w.declareParam("this");
            beginFunctionScope(makeSynthFs("__init", null), null);
            try {
                // Delegamos en List.__init (same module → CALL intra-módulo).
                w.emitGetParam("this");
                w.emitCall("List.__init");
                declareLocal("__discardSuper");
                w.emitSetLocal("__discardSuper");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }
        } catch (IOException e) {
            errors.add("emit OwnerList builtin: " + e.getMessage());
        }
    }

    /**
     * Sintetiza StringBuilder. Layout:
     *   class StringBuilder { var chars: integer[];  var size: integer;  var cap: integer; }
     * Métodos: appendStr(s), appendChar(c), appendInt(n), toString(): string, length(): integer.
     */
    private void synthesizeStringBuilderClass() {
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof ClassDef && "StringBuilder".equals(((ClassDef) d).name)) return;
        }
        try {
            w.addClass("StringBuilder", null);
            // chars NO es ref a objetos (es array i32 de codepoints); GC no
            // necesita escanear su contenido como refs. Pero el SLOT chars sí
            // es ref al array → declareField(., true) para que el GC siga el
            // array como root.
            w.declareField("chars", true);
            w.declareField("size",  false);
            w.declareField("cap",   false);

            // -------------------------- appendChar(c) --------------------------
            // if size >= cap: cap := cap * 2; chars := __growIntArray(chars, cap)
            // chars[size] := c; size := size + 1
            w.addMethod("appendChar");
            w.declareParam("c");
            beginFunctionScope(makeSynthFs("appendChar", null), null);
            try {
                int afterGrow = w.newLabel();
                w.emitGetParam("this"); w.emitGetField("StringBuilder", "size");
                w.emitGetParam("this"); w.emitGetField("StringBuilder", "cap");
                w.emit(OpCode.GE);
                w.emitJumpIfFalse(afterGrow);

                w.emitGetParam("this");
                w.emitGetParam("this"); w.emitGetField("StringBuilder", "cap");
                emitInt(2); w.emit(OpCode.MUL);
                w.emitSetField("StringBuilder", "cap");

                w.emitGetParam("this");
                w.emitGetParam("this"); w.emitGetField("StringBuilder", "chars");
                w.emitGetParam("this"); w.emitGetField("StringBuilder", "cap");
                emitBuiltinCall(Builtin.GROW_INT_ARRAY);
                w.emitSetField("StringBuilder", "chars");

                w.declareLabel(afterGrow);

                // chars[size] := c
                w.emitGetParam("this"); w.emitGetField("StringBuilder", "chars");
                w.emitGetParam("this"); w.emitGetField("StringBuilder", "size");
                w.emitGetParam("c");
                w.emit(OpCode.ASTORE);

                // size := size + 1
                w.emitGetParam("this");
                w.emitGetParam("this"); w.emitGetField("StringBuilder", "size");
                emitInt(1); w.emit(OpCode.ADD);
                w.emitSetField("StringBuilder", "size");

                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // -------------------------- appendStr(s) --------------------------
            // for i := 0; i < strlen(s); i++:  this.appendChar(charCodeAt(s, i))
            w.addMethod("appendStr");
            w.declareParam("s");
            beginFunctionScope(makeSynthFs("appendStr", null), null);
            try {
                declareLocal("i");
                declareLocal("n");
                // n := strlen(s)
                w.emitGetParam("s");
                emitBuiltinCall(Builtin.STRLEN);
                w.emitSetLocal("n");
                // i := 0
                emitInt(0); w.emitSetLocal("i");

                int top = w.newLabel();
                int end = w.newLabel();
                w.declareLabel(top);
                w.emitGetLocal("i"); w.emitGetLocal("n"); w.emit(OpCode.LT);
                w.emitJumpIfFalse(end);

                // this.appendChar(charCodeAt(s, i))
                w.emitGetParam("this");                  // receiver para INVOKE_VIRTUAL
                w.emitGetParam("s"); w.emitGetLocal("i");
                emitBuiltinCall(Builtin.CHAR_CODE_AT);   // → c en stack
                w.emitInvokeVirtual("StringBuilder", "appendChar", 1);
                // INVOKE_VIRTUAL retorna algo en stack (return value). Lo descartamos:
                declareLocal("__discardSB");
                w.emitSetLocal("__discardSB");

                w.emitGetLocal("i"); emitInt(1); w.emit(OpCode.ADD); w.emitSetLocal("i");
                w.emitJump(top);
                w.declareLabel(end);
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // -------------------------- appendInt(n) --------------------------
            // this.appendStr(intToString(n))
            w.addMethod("appendInt");
            w.declareParam("n");
            beginFunctionScope(makeSynthFs("appendInt", null), null);
            try {
                w.emitGetParam("this");
                w.emitGetParam("n");
                emitBuiltinCall(Builtin.INT_TO_STRING);
                w.emitInvokeVirtual("StringBuilder", "appendStr", 1);
                declareLocal("__discardSBI");
                w.emitSetLocal("__discardSBI");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // -------------------------- toString(): string --------------------------
            // __result := __charsToString(chars, size)
            w.addMethod("toString");
            beginFunctionScope(makeSynthFs("toString", PrimitiveType.STRING), null);
            try {
                w.emitGetParam("this"); w.emitGetField("StringBuilder", "chars");
                w.emitGetParam("this"); w.emitGetField("StringBuilder", "size");
                emitBuiltinCall(Builtin.CHARS_TO_STRING);
                w.emitSetLocal("__result");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // -------------------------- length(): integer --------------------------
            w.addMethod("length");
            beginFunctionScope(makeSynthFs("length", PrimitiveType.INTEGER), null);
            try {
                w.emitGetParam("this"); w.emitGetField("StringBuilder", "size");
                w.emitSetLocal("__result");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // -------------------------- endClass + Constructor --------------------------
            w.endClass();

            w.addFunction("StringBuilder.__init", false);
            w.declareParam("this");
            beginFunctionScope(makeSynthFs("__init", null), null);
            try {
                w.emitGetParam("this");
                emitInt(64); w.emit(OpCode.NEWARRAY);
                w.emitSetField("StringBuilder", "chars");
                w.emitGetParam("this"); emitInt(0);  w.emitSetField("StringBuilder", "size");
                w.emitGetParam("this"); emitInt(64); w.emitSetField("StringBuilder", "cap");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }
        } catch (IOException e) {
            errors.add("emit StringBuilder builtin: " + e.getMessage());
        }
    }

    /** FunctionSymbol "fake" para los métodos sintetizados de stdlib. */
    private FunctionSymbol makeSynthFs(String name, BpType returnType) {
        FunctionSymbol fs = new FunctionSymbol(name, true, false, false, null, null);
        fs.returnType = returnType;
        return fs;
    }

    /**
     * Sintetiza la clase stdlib Thread. Layout obligado por el VM:
     *   class Thread {
     *     var __tid: integer        // slot 0 — id del ThreadContext (0 = unstarted)
     *     var __stackSize: integer  // slot 1 — leído por __threadStart
     *   }
     * Vtable: slot 0 = run() (que la subclase del usuario sobreescribe).
     *
     *   function Thread(stackSize: integer)   // ctor: guarda stackSize, tid = 0
     *   function run()                         // VIRTUAL, body vacío
     *   function start()                       // calls __threadStart(this)
     *   function join()                        // calls __threadJoin(this)
     */
    private void synthesizeThreadClass() {
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof ClassDef && "Thread".equals(((ClassDef) d).name)) return;
        }
        try {
            w.addClass("Thread", null);
            w.declareField("__tid",       false);   // slot 0
            w.declareField("__stackSize", false);   // slot 1

            // -------------------------- run() (virtual, body vacío) --------------------------
            // PRIMER método declarado → slot 0 de la vtable (asumido por __threadStart).
            w.addMethod("run");
            beginFunctionScope(makeSynthFs("run", null), null);
            try {
                emitFunctionEnd();   // void; flujo cae hasta el ret final
            } finally { scopeStack.pop(); }

            // -------------------------- start() --------------------------
            w.addMethod("start");
            beginFunctionScope(makeSynthFs("start", null), null);
            try {
                w.emitGetParam("this");
                emitBuiltinCall(Builtin.THREAD_START);
                declareLocal("__discardStart");
                w.emitSetLocal("__discardStart");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // -------------------------- join() --------------------------
            w.addMethod("join");
            beginFunctionScope(makeSynthFs("join", null), null);
            try {
                w.emitGetParam("this");
                emitBuiltinCall(Builtin.THREAD_JOIN);
                declareLocal("__discardJoin");
                w.emitSetLocal("__discardJoin");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            w.endClass();

            // -------------------------- Constructor Thread(stackSize) --------------------------
            w.addFunction("Thread.__init", false);
            w.declareParam("this");
            w.declareParam("stackSize");
            beginFunctionScope(makeSynthFs("__init", null), null);
            try {
                // this.__tid := 0
                w.emitGetParam("this");
                emitInt(0);
                w.emitSetField("Thread", "__tid");
                // this.__stackSize := stackSize
                w.emitGetParam("this");
                w.emitGetParam("stackSize");
                w.emitSetField("Thread", "__stackSize");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }
        } catch (IOException e) {
            errors.add("emit Thread builtin: " + e.getMessage());
        }
    }

    /**
     * Sintetiza la clase {@code Mutex} en el módulo del usuario si no existe.
     * Tiene un único field {@code __mid} (slot 0) que guarda el índice del
     * mutex en la lista interna de la VM. Tres métodos:
     *   - constructor: pide un id nuevo a la VM (__mutexCreate)
     *   - lock(): __mutexLock(this) — bloquea si está tomado por otro
     *   - unlock(): __mutexUnlock(this) — libera y hand-off al primer waiter
     */
    /**
     * Sintetiza SyncList: lista thread-safe.
     *
     *   class SyncList extends List
     *     var __mutex: Mutex     // mutex propio por instancia
     *
     *     // Cada método envuelve la lógica de List con lock/unlock.
     *     // Los overrides invocan al método del padre via CALL List.<m>
     *     // (despacho directo, no virtual: evita una indirección y un
     *     // re-locking accidental si algún día algo más extendiese SyncList).
     *
     *     public add(item: any)        // append at�mico
     *     public remove(idx: integer)  // borra por �ndice at�micamente
     *     public get(idx: integer): any
     *     public set(idx: integer, item: any)
     *     public length(): integer
     *     public pop(): any            // borra y retorna el �ltimo (queue-like)
     *
     * El constructor SyncList():
     *   super()            ← inicializa items[8], size=0, cap=8
     *   this.__mutex := Mutex()
     *
     * Disponible siempre, sin import. El usuario puede declarar su propia
     * clase SyncList y entonces este builtin se omite.
     */
    private void synthesizeSyncListClass() {
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof ClassDef && "SyncList".equals(((ClassDef) d).name)) return;
        }
        try {
            w.addClass("SyncList", "List");
            // ref-type, no owner. Compartir un Mutex entre dos SyncList sería
            // raro pero no es nuestro caso; un Mutex pequeño por instancia.
            w.declareField("__mutex", true, false);

            // ---- add(item) ----
            w.addMethod("add");
            w.declareParam("item");
            beginFunctionScope(makeSynthFs("add", null), null);
            try {
                emitSyncListLock();
                // super.add(item)
                w.emitGetParam("this");
                w.emitGetParam("item");
                w.emitCall("List.add");
                declareLocal("__discardSuper");
                w.emitSetLocal("__discardSuper");
                emitSyncListUnlock();
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // ---- remove(idx) ----
            w.addMethod("remove");
            w.declareParam("idx");
            beginFunctionScope(makeSynthFs("remove", null), null);
            try {
                emitSyncListLock();
                w.emitGetParam("this");
                w.emitGetParam("idx");
                w.emitCall("List.remove");
                declareLocal("__discardSuper");
                w.emitSetLocal("__discardSuper");
                emitSyncListUnlock();
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // ---- get(idx): any ----
            w.addMethod("get");
            w.declareParam("idx");
            beginFunctionScope(makeSynthFs("get", BpType.AnyType.INSTANCE), null);
            try {
                emitSyncListLock();
                w.emitGetParam("this");
                w.emitGetParam("idx");
                w.emitCall("List.get");
                w.emitSetLocal("__result");   // List.get retorna any
                emitSyncListUnlock();
                emitFunctionEnd();            // emitirá GET_LOCAL __result + RET
            } finally { scopeStack.pop(); }

            // ---- set(idx, item) ----
            w.addMethod("set");
            w.declareParam("idx");
            w.declareParam("item");
            beginFunctionScope(makeSynthFs("set", null), null);
            try {
                emitSyncListLock();
                w.emitGetParam("this");
                w.emitGetParam("idx");
                w.emitGetParam("item");
                w.emitCall("List.set");
                declareLocal("__discardSuper");
                w.emitSetLocal("__discardSuper");
                emitSyncListUnlock();
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // ---- length(): integer ----
            w.addMethod("length");
            beginFunctionScope(makeSynthFs("length", PrimitiveType.INTEGER), null);
            try {
                emitSyncListLock();
                w.emitGetParam("this");
                w.emitCall("List.length");
                w.emitSetLocal("__result");
                emitSyncListUnlock();
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // ---- pop(): any ----
            // Primitiva producer/consumer:
            //   if size == 0 → __result := 0 (lo dejamos como sentinel)
            //   else        → __result := items[size-1]; size := size-1; items[size] := 0
            w.addMethod("pop");
            beginFunctionScope(makeSynthFs("pop", BpType.AnyType.INSTANCE), null);
            try {
                emitSyncListLock();
                int emptyLabel = w.newLabel();
                int endLabel   = w.newLabel();

                // if size == 0 → empty branch
                w.emitGetParam("this"); w.emitGetField("List", "size");
                emitInt(0);
                w.emit(OpCode.EQ);
                int notEmpty = w.newLabel();
                w.emitJumpIfFalse(notEmpty);
                emitInt(0);                      // sentinel "lista vacía"
                w.emitSetLocal("__result");
                w.emitJump(endLabel);

                w.declareLabel(notEmpty);
                // last := items[size-1]
                w.emitGetParam("this"); w.emitGetField("List", "items");
                w.emitGetParam("this"); w.emitGetField("List", "size");
                emitInt(1); w.emit(OpCode.SUB);
                w.emit(OpCode.ALOAD);
                w.emitSetLocal("__result");
                // size := size - 1
                w.emitGetParam("this");
                w.emitGetParam("this"); w.emitGetField("List", "size");
                emitInt(1); w.emit(OpCode.SUB);
                w.emitSetField("List", "size");
                // items[size] := 0   (limpia slot, ayuda al GC con refs)
                w.emitGetParam("this"); w.emitGetField("List", "items");
                w.emitGetParam("this"); w.emitGetField("List", "size");
                emitInt(0);
                w.emit(OpCode.ASTORE);

                w.declareLabel(endLabel);
                // (emptyLabel queda no usado — lo dejamos para no romper la estructura;
                //  alternativa: declararlo donde sale del if. Mantenemos el código
                //  simple: el JUMP a endLabel ya nos saca del else.)
                w.declareLabel(emptyLabel);
                emitSyncListUnlock();
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // ---- popBlocking(): any ----
            // Variante de pop que ESPERA hasta haber un item. Implementación
            // v1: spin con yield cooperativo entre intentos. Cada iteración
            // toma el mutex, comprueba size; si > 0, extrae y devuelve; si
            // no, suelta el mutex y cede la CPU. No requiere primitivas
            // nuevas en la VM (yield builtin existe). Trade-off: el thread
            // bloqueado consume slot del scheduler entre yields (vs. un
            // wait/notify auténtico que lo parquea hasta notify). Acepta-
            // ble para programas BP típicos; si se vuelve relevante, se
            // sustituye por condvar real.
            w.addMethod("popBlocking");
            beginFunctionScope(makeSynthFs("popBlocking", BpType.AnyType.INSTANCE), null);
            try {
                int loopTop = w.newLabel();
                int doneOk  = w.newLabel();
                w.declareLabel(loopTop);

                emitSyncListLock();
                // if size > 0: tomar item y salir.
                w.emitGetParam("this"); w.emitGetField("List", "size");
                emitInt(0);
                w.emit(OpCode.GT);
                int retry = w.newLabel();
                w.emitJumpIfFalse(retry);

                // last := items[size-1]; size -= 1; items[size] := 0
                w.emitGetParam("this"); w.emitGetField("List", "items");
                w.emitGetParam("this"); w.emitGetField("List", "size");
                emitInt(1); w.emit(OpCode.SUB);
                w.emit(OpCode.ALOAD);
                w.emitSetLocal("__result");
                w.emitGetParam("this");
                w.emitGetParam("this"); w.emitGetField("List", "size");
                emitInt(1); w.emit(OpCode.SUB);
                w.emitSetField("List", "size");
                w.emitGetParam("this"); w.emitGetField("List", "items");
                w.emitGetParam("this"); w.emitGetField("List", "size");
                emitInt(0);
                w.emit(OpCode.ASTORE);
                emitSyncListUnlock();
                w.emitJump(doneOk);

                // Empty: soltar mutex, dormir un poco, reintentar.
                // sleep(1) en vez de yield() porque yield no parquea el
                // thread (sólo lo re-encola), provocando que un consumer
                // sin items keep burning CPU y monopolizando el scheduler
                // — los workers se quedan ocupados ejecutando popBlocking
                // y el producer no llega a ser dispatched. Un sleep mínimo
                // garantiza que el thread BLOCKED_SLEEP no consume slot
                // hasta que pase un quantum.
                w.declareLabel(retry);
                emitSyncListUnlock();
                emitInt(1);                                          // ms
                emitBuiltinCall(edu.bpgenvm.bytecode.Builtin.SLEEP);
                declareLocal("__discardYield");
                w.emitSetLocal("__discardYield");
                w.emitJump(loopTop);

                w.declareLabel(doneOk);
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            w.endClass();

            // ---- Constructor SyncList() ----
            w.addFunction("SyncList.__init", false);
            w.declareParam("this");
            beginFunctionScope(makeSynthFs("__init", null), null);
            try {
                // super() = List.__init(this): inicializa items[8], size=0, cap=8.
                w.emitGetParam("this");
                w.emitCall("List.__init");
                declareLocal("__discardSuper");
                w.emitSetLocal("__discardSuper");

                // this.__mutex := Mutex()
                int id = tempCounter++;
                String newref  = "__newmtx_"  + id;
                String discard = "__discardMtx_" + id;
                declareLocal(newref);
                declareLocal(discard);
                w.emitNewObject("Mutex");
                w.emitSetLocal(newref);
                w.emitGetLocal(newref);
                w.emitCall("Mutex.__init");
                w.emitSetLocal(discard);
                w.emitGetParam("this");
                w.emitGetLocal(newref);
                w.emitSetField("SyncList", "__mutex");

                emitFunctionEnd();
            } finally { scopeStack.pop(); }
        } catch (IOException e) {
            errors.add("emit SyncList builtin: " + e.getMessage());
        }
    }

    /** Helper: emite `this.__mutex.lock()` con discard del retorno. */
    private void emitSyncListLock() throws IOException {
        int id = tempCounter++;
        String discard = "__slLock_" + id;
        declareLocal(discard);
        w.emitGetParam("this");
        w.emitGetField("SyncList", "__mutex");
        w.emitInvokeVirtual("Mutex", "lock", 0);
        w.emitSetLocal(discard);
    }
    /** Helper: emite `this.__mutex.unlock()` con discard del retorno. */
    private void emitSyncListUnlock() throws IOException {
        int id = tempCounter++;
        String discard = "__slUnlock_" + id;
        declareLocal(discard);
        w.emitGetParam("this");
        w.emitGetField("SyncList", "__mutex");
        w.emitInvokeVirtual("Mutex", "unlock", 0);
        w.emitSetLocal(discard);
    }

    private void synthesizeMutexClass() {
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof ClassDef && "Mutex".equals(((ClassDef) d).name)) return;
        }
        try {
            w.addClass("Mutex", null);
            w.declareField("__mid", false);   // slot 0

            // -------------------------- lock() --------------------------
            w.addMethod("lock");
            beginFunctionScope(makeSynthFs("lock", null), null);
            try {
                w.emitGetParam("this");
                emitBuiltinCall(Builtin.MUTEX_LOCK);
                declareLocal("__discardLock");
                w.emitSetLocal("__discardLock");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            // -------------------------- unlock() --------------------------
            w.addMethod("unlock");
            beginFunctionScope(makeSynthFs("unlock", null), null);
            try {
                w.emitGetParam("this");
                emitBuiltinCall(Builtin.MUTEX_UNLOCK);
                declareLocal("__discardUnlock");
                w.emitSetLocal("__discardUnlock");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }

            w.endClass();

            // -------------------------- Constructor Mutex() --------------------------
            w.addFunction("Mutex.__init", false);
            w.declareParam("this");
            beginFunctionScope(makeSynthFs("__init", null), null);
            try {
                // this.__mid := __mutexCreate()
                w.emitGetParam("this");
                emitBuiltinCall(Builtin.MUTEX_CREATE);
                w.emitSetField("Mutex", "__mid");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }
        } catch (IOException e) {
            errors.add("emit Mutex builtin: " + e.getMessage());
        }
    }

    private void emitInstanceOf(InstanceOfExpr ie) throws IOException {
        emitExpr(ie.target);
        // El analyzer dejó el ClassSymbol en exprSymbols si el tipo es válido.
        Symbol cls = info.exprSymbols.get(ie);
        if (!(cls instanceof ClassSymbol)) {
            errors.add("instanceof con clase no resuelta: " + ie.typeName);
            // dejar un 0 en la pila para no descuadrar
            emitInt(0);
            return;
        }
        w.emitInstanceOf(((ClassSymbol) cls).name);
    }

    private void emitFieldExpr() throws IOException {
        if (currentModulePropertyBacking != null) {
            // Property de módulo: `field` lee directamente el global backing.
            w.emitGetGlobal(currentModulePropertyBacking);
            return;
        }
        if (currentPropertyField == null) {
            errors.add("'field' fuera de un getter/setter de propiedad");
            return;
        }
        ClassSymbol cls = scopeStack.peek().fs.ownerClass;
        w.emitGetParam("this");
        w.emitGetField(cls.name, currentPropertyField);
    }

    private void emitSuperCall(SuperCallExpr sc) throws IOException {
        // super(args) dentro de un constructor: llama directamente al __init del padre.
        FuncScope sc2 = scopeStack.peek();
        if (sc2 == null || sc2.fs.ownerClass == null || sc2.fs.ownerClass.baseClass == null) {
            errors.add("super(...) fuera de constructor con padre"); return;
        }
        ClassSymbol parent = sc2.fs.ownerClass.baseClass;
        FunctionSymbol parentCtor = parent.findConstructor();
        w.emitGetParam("this");
        for (int i = 0; i < sc.args.size(); i++) {
            emitExpr(sc.args.get(i));
            if (parentCtor != null && i < parentCtor.params.size()) {
                coerceToTarget(sc.args.get(i), parentCtor.params.get(i).type);
            }
        }
        if (parent.isExternal) {
            // L2 v3 — emitimos CALL_EXT a `__cls_init_<Cls>(this, args)` —
            // la factoría sintetizada en el módulo dueño que internamente llama
            // a `<Cls>.__init`. El símbolo `__cls_init_X` no tiene puntos, así
            // que el loader lo interpreta como símbolo del módulo dueño
            // (parts[len-2]=módulo). El receiver `this` del child es válido
            // como ref de Cls porque los fields heredados ocupan los slots
            // 0..N-1 del descriptor.
            StringBuilder qname = new StringBuilder();
            if (parent.externalLibrary != null && !parent.externalLibrary.isEmpty())
                qname.append(parent.externalLibrary).append('.');
            qname.append(parent.externalModule).append('.').append("__cls_init_").append(parent.name);
            w.emitCallExt(qname.toString(), parent.externalFromPath);
            return;
        }
        w.emitCall(parent.name + ".__init");
    }

    /**
     * Emite la construcción de un objeto:
     *   NEW_OBJECT Cls; SET_LOCAL __newref;
     *   GET_LOCAL __newref; push args...; CALL Cls.__init;
     *   SET_LOCAL __discard; GET_LOCAL __newref   ← restaura la ref para la expresión
     */
    private void emitConstruction(ClassSymbol cls, List<IExpr> args) throws IOException {
        // L2: si la clase vive en otro módulo, no podemos emitir NEW_OBJECT
        // local (no tenemos su class_ptr). En su lugar invocamos la factoría
        // `__cls_new_<Cls>(args)` que el módulo dueño sintetiza: hace el
        // NEW_OBJECT en SU contexto y devuelve el ref.
        if (cls.isExternal) {
            // Empujar args en orden — la factoría tiene la misma firma que el ctor.
            FunctionSymbol ctor = cls.constructor;
            for (int i = 0; i < args.size(); i++) {
                emitExpr(args.get(i));
                if (ctor != null && i < ctor.params.size())
                    coerceToTarget(args.get(i), ctor.params.get(i).type);
            }
            StringBuilder qname = new StringBuilder();
            if (cls.externalLibrary != null && !cls.externalLibrary.isEmpty())
                qname.append(cls.externalLibrary).append('.');
            qname.append(cls.externalModule).append('.').append("__cls_new_").append(cls.name);
            w.emitCallExt(qname.toString(), cls.externalFromPath);
            // La factoría deja el ref en la pila como retorno; lo dejamos ahí.
            return;
        }

        // Nombres ÚNICOS por construcción para que llamadas anidadas como
        // Caja(Animal("Rex")) no compartan slot (el __newref del outer no se
        // pisa cuando se emite la construcción del arg).
        int id = tempCounter++;
        String newref  = "__newref_" + id;
        String discard = "__discard_" + id;
        declareLocal(newref);
        declareLocal(discard);
        w.emitNewObject(cls.name);
        w.emitSetLocal(newref);

        FunctionSymbol ctor = cls.findConstructor();
        if (ctor != null) {
            w.emitGetLocal(newref);
            for (int i = 0; i < args.size(); i++) {
                emitExpr(args.get(i));
                if (i < ctor.params.size()) coerceToTarget(args.get(i), ctor.params.get(i).type);
            }
            // El ctor puede ser heredado: usar el nombre de SU clase dueña, no de la subclase.
            w.emitCall(ctor.ownerClass.name + ".__init");
            w.emitSetLocal(discard);
        }
        w.emitGetLocal(newref);
    }

    private void emitMemberAccess(MemberAccessExpr ma) throws IOException {
        // Acceso a valor de enum: Enum.NAME → constante int.
        // Cubre tanto `Enum.NAME` (target = IdentifierExpr) como
        // `Namespace.Enum.NAME` (target = MemberAccessExpr resuelto a EnumSymbol).
        {
            Symbol targetSym = info.exprSymbols.get(ma.target);
            if (targetSym instanceof EnumSymbol) {
                Long val = ((EnumSymbol) targetSym).values.get(ma.member);
                if (val != null) {
                    emitInt(val.intValue());
                    return;
                }
                errors.add("valor de enum no encontrado: " + ((EnumSymbol) targetSym).name + "." + ma.member);
                return;
            }
        }
        Symbol mSym = info.exprSymbols.get(ma);

        // Const cross-module con valor literal conocido: inlinar.
        if (mSym instanceof ConstSymbol) {
            ConstSymbol c = (ConstSymbol) mSym;
            if (c.literalValue != null) {
                emitConstLiteral(c.literalValue);
                return;
            }
        }

        if (mSym instanceof PropertySymbol) {
            PropertySymbol ps = (PropertySymbol) mSym;
            // Property a nivel módulo (importada vía .bpi): CALL_EXT al getter.
            if (ps.ownerClass == null) {
                if (ps.isExternal) {
                    w.emitCallExt(buildExternalAccessorName(ps, /*setter*/false), ps.externalFromPath);
                } else {
                    w.emitCall(moduleGetterName(ps.name));
                }
                return;
            }
            emitExpr(ma.target);
            emitInvokeVirtualSmart(ps.ownerClass, "get" + capitalize(ps.name), 0);
            return;
        }
        if (mSym instanceof ConstSymbol) {
            ConstSymbol c = (ConstSymbol) mSym;
            if (c.ownerClass != null && c.isStatic) {
                w.emitGetGlobal(c.ownerClass.name + "." + c.name);
            } else {
                w.emitGetGlobal(c.name);
            }
            return;
        }
        if (mSym instanceof VarSymbol) {
            VarSymbol v = (VarSymbol) mSym;
            if (v.ownerClass == null) {                       // global de módulo
                w.emitGetGlobal(v.name);
            } else if (v.isStatic) {                          // static var de clase
                w.emitGetGlobal(v.ownerClass.name + "." + v.name);
            } else {                                          // campo de instancia
                emitExpr(ma.target);
                w.emitGetField(v.ownerClass.name, v.name);
            }
            return;
        }
        errors.add("member-access no soportado: " + ma.member);
    }

    /**
     * Inlinea un literal cuyo valor proviene de una const importada (.bpi).
     * El valor llega como Long/Double/String/Boolean.
     */
    private void emitConstLiteral(Object v) throws IOException {
        if (v instanceof Long || v instanceof Integer) {
            emitInt(((Number) v).intValue());
        } else if (v instanceof Boolean) {
            emitInt(((Boolean) v) ? 1 : 0);
        } else if (v instanceof Double || v instanceof Float) {
            w.emitPushFloat(((Number) v).floatValue());
        } else if (v instanceof String) {
            String sym = internString((String) v);
            w.emitLeaGlobal(sym);
        } else {
            errors.add("const literal de tipo no soportado: " + v.getClass().getSimpleName());
        }
    }

    // ============================================================
    // HELPERS
    // ============================================================

    private void emitInt(int v) {
        try {
            // Elige automáticamente la variante compacta PUSH_0..4 / PUSH_NEG1
            // cuando aplica; cae a PUSH+i32 en otro caso.
            w.emitPushInt(v);
        } catch (IOException e) {
            errors.add("emit int: " + e.getMessage());
        }
    }

    private boolean isFloat(BpType t) {
        return t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.FLOAT;
    }

    private boolean isLong(BpType t) {   // H1.2 (V2)
        return t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.LONG;
    }

    private boolean isDouble(BpType t) {   // H1.3 (V2)
        return t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.DOUBLE;
    }

    /** H1.3 — tipos de 8 bytes (2 slots): long y double comparten storage. */
    private boolean is8Byte(BpType t) {
        return isLong(t) || isDouble(t);
    }

    /** BUG-6: nº de slots de 4 bytes que ocupan los argumentos (excluye `this`).
     *  INVOKE_VIRTUAL localiza el receptor en sp-4-numArgs*4, así que su operando
     *  numArgs debe contar SLOTS, no argumentos: un parámetro long/double ocupa 2
     *  slots. Para métodos all-4-byte coincide con el nº de argumentos. */
    private int argSlotCount(FunctionSymbol fs) {
        int slots = 0;
        for (int i = 0; i < fs.params.size(); i++) {
            slots += is8Byte(fs.params.get(i).type) ? 2 : 1;
        }
        return slots;
    }

    private static boolean isNumericCastName(String n) {   // H1.3b (V2)
        return "integer".equals(n) || "long".equals(n) || "float".equals(n) || "double".equals(n);
    }

    /** H1.3b — emite la conversión numérica del valor ya en pila (de tipo srcT)
     *  al tipo destino del cast. Nada si ya coincide. */
    private void emitNumericConversion(BpType srcT, String target) throws IOException {
        boolean sInt    = srcT instanceof PrimitiveType && ((PrimitiveType) srcT).isIntegerLike();
        boolean sLong   = isLong(srcT);
        boolean sFloat  = isFloat(srcT);
        boolean sDouble = isDouble(srcT);
        switch (target) {
            case "integer":
                if (sLong)        w.emit(OpCode.I64_TO_I32);
                else if (sFloat)  w.emit(OpCode.F2I);
                else if (sDouble) w.emit(OpCode.D2I);
                break;  // int/narrow: ya es i32 en pila
            case "long":
                if (sInt)         w.emit(OpCode.I32_TO_I64);
                else if (sFloat)  w.emit(OpCode.F2L);
                else if (sDouble) w.emit(OpCode.D2L);
                break;  // long: nada
            case "float":
                if (sInt)         w.emit(OpCode.I2F);
                else if (sLong)   w.emit(OpCode.L2F);
                else if (sDouble) w.emit(OpCode.D2F);
                break;  // float: nada
            case "double":
                if (sInt)         w.emit(OpCode.I2D);
                else if (sLong)   w.emit(OpCode.L2D);
                else if (sFloat)  w.emit(OpCode.F2D);
                break;  // double: nada
        }
    }

    /** H1.3 — convierte el operando ya en pila al tipo de la op binaria
     *  (torre double > float > long > int). narrow/int cargan como i32. */
    private void promoteNumeric(BpType t, boolean useDouble, boolean useFloat, boolean useLong) throws IOException {
        if (useDouble) {
            if (!isDouble(t)) w.emit(isFloat(t) ? OpCode.F2D : isLong(t) ? OpCode.L2D : OpCode.I2D);
        } else if (useFloat) {
            if (!isFloat(t)) w.emit(isLong(t) ? OpCode.L2F : OpCode.I2F);
        } else if (useLong) {
            if (!isLong(t)) w.emit(OpCode.I32_TO_I64);
        }
    }

    private BpType typeRefToBpType(TypeRef ref) {
        // Útil mientras no haya un mapeo expuesto desde el analyzer.
        if (ref instanceof SimpleTypeRef) {
            // BUG-6: case-SENSITIVE. Los primitivos se escriben en minúscula
            // (integer/long/double/...). Tras el dekeyword, las clases pueden
            // llamarse Long/Double/Float/Integer/Boolean/String — NO deben
            // colisionar con el primitivo homónimo. Un .toLowerCase() aquí hacía
            // que `var o: Long` (clase, ref de 4 bytes) se tratara como el
            // primitivo long (8 bytes) → SET_LOCAL_L/GET_LOCAL_L corrompía la ref.
            String n = ((SimpleTypeRef) ref).name;
            switch (n) {
                case "integer": return PrimitiveType.INTEGER;
                case "float":   return PrimitiveType.FLOAT;
                case "string":  return PrimitiveType.STRING;
                case "boolean": return PrimitiveType.BOOLEAN;
                case "long":    return PrimitiveType.LONG;     // H1.2 (V2)
                case "double":  return PrimitiveType.DOUBLE;   // H1.3 (V2)
                case "byte":    return PrimitiveType.UINT8;
                case "int8":    return PrimitiveType.INT8;
                case "word":    return PrimitiveType.UINT16;
                case "int16":   return PrimitiveType.INT16;
            }
        }
        return null;
    }

    /** Empuja a la pila la versión del valor convertida a target type si hace falta. */
    private void coerceToTarget(IExpr src, BpType target) throws IOException {
        if (target == null) return;
        BpType srcT = info.exprTypes.get(src);
        if (srcT == null) return;
        if (!(target instanceof PrimitiveType)) return;
        PrimitiveType pt = (PrimitiveType) target;
        // int → float (promoción numérica)
        if (pt.tag == PrimitiveType.Kind.FLOAT && srcT instanceof PrimitiveType) {
            PrimitiveType s = (PrimitiveType) srcT;
            if (s.isIntegerLike()) { w.emit(OpCode.I2F); return; }          // int/narrow -> f32
            if (s.tag == PrimitiveType.Kind.LONG) { w.emit(OpCode.L2F); return; }  // H1.3 long -> f32
        }
        // H1.3 — cualquier numérico más estrecho -> double (widening).
        if (pt.tag == PrimitiveType.Kind.DOUBLE && srcT instanceof PrimitiveType) {
            PrimitiveType s = (PrimitiveType) srcT;
            if (s.isIntegerLike())                 { w.emit(OpCode.I2D); return; }
            if (s.tag == PrimitiveType.Kind.LONG)  { w.emit(OpCode.L2D); return; }
            if (s.tag == PrimitiveType.Kind.FLOAT) { w.emit(OpCode.F2D); return; }
            // double -> double: nada.
        }
        // H1.2 — int/narrow → long (widening) en asignación/param/return.
        if (pt.tag == PrimitiveType.Kind.LONG
                && srcT instanceof PrimitiveType
                && ((PrimitiveType) srcT).isIntegerLike()) {
            w.emit(OpCode.I32_TO_I64);
            return;
        }
        // any primitive → string (cuando el contexto pide string, e.g. concat)
        if (pt.tag == PrimitiveType.Kind.STRING) {
            coerceToString(srcT);
        }
    }

    /** Convierte el valor en el tope de la pila a una ref-a-string si su tipo de
     *  origen es primitivo (int / float / bool). Si ya es string o no es primitivo,
     *  lo deja como está. */
    private void coerceToString(BpType src) throws IOException {
        if (!(src instanceof PrimitiveType)) return;
        switch (((PrimitiveType) src).tag) {
            case STRING: return;
            case INTEGER:
                w.emit(OpCode.CALL_BUILTIN);
                w.emitShort((short) Builtin.INT_TO_STRING.id);
                break;
            case FLOAT:
                w.emit(OpCode.CALL_BUILTIN);
                w.emitShort((short) Builtin.FLOAT_TO_STRING.id);
                break;
            case BOOLEAN:
                w.emit(OpCode.CALL_BUILTIN);
                w.emitShort((short) Builtin.BOOL_TO_STRING.id);
                break;
        }
    }

    private void loadFromSymbol(Symbol sym, String name) throws IOException {
        if (sym instanceof ParamSymbol) {
            w.emitGetParam(sym.name);
        } else if (sym instanceof VarSymbol) {
            VarSymbol v = (VarSymbol) sym;
            if (v.isLocal) w.emitGetLocal(v.name);
            else if (v.ownerClass == null) w.emitGetGlobal(v.name);
            else if (v.isStatic) w.emitGetGlobal(v.ownerClass.name + "." + v.name);
            else {
                // Campo de instancia con implicit this (estamos en un método de la clase).
                w.emitGetParam("this");
                w.emitGetField(v.ownerClass.name, v.name);
            }
        } else if (sym instanceof PropertySymbol) {
            PropertySymbol ps = (PropertySymbol) sym;
            if (ps.ownerClass == null) {
                // Property a nivel módulo: CALL al getter sintético.
                if (ps.isExternal) {
                    String qname = buildExternalAccessorName(ps, /*setter*/false);
                    w.emitCallExt(qname, ps.externalFromPath);
                } else {
                    w.emitCall(moduleGetterName(ps.name));
                }
                return;
            }
            if (ps.isStatic) { errors.add("property estática no soportada: " + name); return; }
            w.emitGetParam("this");
            emitInvokeVirtualSmart(ps.ownerClass, "get" + capitalize(ps.name), 0);
        } else if (sym instanceof ConstSymbol) {
            ConstSymbol c = (ConstSymbol) sym;
            if (c.ownerClass != null && c.isStatic) {
                w.emitGetGlobal(c.ownerClass.name + "." + c.name);
            } else {
                w.emitGetGlobal(sym.name);
            }
        } else if (sym instanceof FunctionSymbol) {
            errors.add("no se puede usar función como valor: " + name);
        } else {
            errors.add("símbolo no soportado en load: " + sym.getClass().getSimpleName());
        }
    }

    private void storeToSymbol(Symbol sym, String name) throws IOException {
        if (sym instanceof ParamSymbol) {
            w.emitSetParam(sym.name);
        } else if (sym instanceof VarSymbol) {
            VarSymbol v = (VarSymbol) sym;
            if (v.isLocal) w.emitSetLocal(v.name);
            else if (v.ownerClass == null) w.emitSetGlobal(v.name);
            else if (v.isStatic) w.emitSetGlobal(v.ownerClass.name + "." + v.name);
            else {
                // Campo de instancia con implicit this. Pila: ..., val. Necesitamos ref, val.
                String temp = "__stf_" + (stringPoolCounter++);
                declareLocal(temp);
                w.emitSetLocal(temp);
                w.emitGetParam("this");
                w.emitGetLocal(temp);
                emitSetFieldAuto(v.ownerClass.name, v.name);
            }
        } else if (sym instanceof PropertySymbol) {
            PropertySymbol ps = (PropertySymbol) sym;
            if (ps.ownerClass == null) {
                // Property a nivel módulo: pila ..., val → CALL setter.
                if (ps.isExternal) {
                    String qname = buildExternalAccessorName(ps, /*setter*/true);
                    w.emitCallExt(qname, ps.externalFromPath);
                } else {
                    w.emitCall(moduleSetterName(ps.name));
                }
                declareLocal("__discard");
                w.emitSetLocal("__discard");
                return;
            }
            if (ps.isStatic) { errors.add("property estática no soportada: " + name); return; }
            // Pila: ..., val. Necesitamos this, val para INVOKE_VIRTUAL setX, 1.
            String temp = "__stp_" + (stringPoolCounter++);
            declareLocal(temp);
            w.emitSetLocal(temp);
            w.emitGetParam("this");
            w.emitGetLocal(temp);
            emitInvokeVirtualSmart(ps.ownerClass, "set" + capitalize(ps.name), 1);
            declareLocal("__discard");
            w.emitSetLocal("__discard");
        } else if (sym instanceof ConstSymbol) {
            w.emitSetGlobal(sym.name);
        } else {
            errors.add("símbolo no soportado en store: " + sym.getClass().getSimpleName());
        }
    }

    // ============================================================
    // STRING POOL
    // ============================================================

    private void collectStringLiterals(ModuleNode mod) {
        // Walk recursivo a la búsqueda de StringLitExpr. Para Fase A es opcional;
        // intern() lo crea bajo demanda. Lo dejamos vacío y dejamos que cada
        // emit de StringLitExpr lo internalice (en orden de aparición).
    }

    private String internString(String value) {
        String existing = stringPool.get(value);
        if (existing != null) return existing;
        String name = "__str_" + (stringPoolCounter++);
        try { w.addConstantString(name, value); }
        catch (Exception e) { errors.add("intern string: " + e.getMessage()); }
        stringPool.put(value, name);
        return name;
    }

    // ============================================================
    // STDLIB HELPERS (inyectados al final)
    // ============================================================

    private void emitStringConcatCall() throws IOException {
        ensureStrconcatHelper();
        w.emitCall("__strconcat");
    }

    private void emitStringEqualsCall() throws IOException {
        ensureStrequalsHelper();
        w.emitCall("__strequals");
    }

    private void ensureStrequalsHelper() {
        // Sólo marca; el body se emite en flushHelpers tras las funciones
        // del usuario para que el addFunction del helper no parta a la
        // función que lo está usando.
        emittedHelpers.add("__strequals");
    }

    private void ensureStrconcatHelper() {
        if (!emittedHelpers.add("__strconcat")) return;
        // Construimos la función al vuelo. Como addFunction cierra la función actual,
        // necesitamos emitir el helper ANTES de empezar la función que lo usa.
        // Pero para simplificar, lo emitimos justo cuando se necesita. ModWriter
        // cierra la función actual automáticamente; tras el helper, hay que volver
        // a abrir la función original — eso es lo que NO podemos hacer trivialmente.
        //
        // Estrategia alternativa: marcamos que se necesita y lo emitimos al final
        // (después de todas las funciones del usuario). Eso significa que addFunction
        // ya habrá movido a la siguiente función, pero el helper puede ir tras Main
        // sin problema, y los CALL ya están parcheados por nombre en writeToFile.
        //
        // Por tanto: postergamos. Aquí solo marcamos; emit real en flush.
    }

    /** Emite los helpers postergados. Debe llamarse antes de writeToFile. */
    private void flushHelpers() throws IOException {
        if (emittedHelpers.contains("__strconcat")) {
            emitStrconcatBody();
        }
        if (emittedHelpers.contains("__strequals")) {
            emitStrequalsBody();
        }
    }

    private void emitStrconcatBody() throws IOException {
        w.addFunction("__strconcat", false);
        w.declareParam("a");
        w.declareParam("b");
        w.declareLocal("la");
        w.declareLocal("lb");
        w.declareLocal("out");
        w.declareLocal("i");

        // la = a.length
        w.emitGetParam("a"); w.emit(OpCode.ALEN); w.emitSetLocal("la");
        // lb = b.length
        w.emitGetParam("b"); w.emit(OpCode.ALEN); w.emitSetLocal("lb");
        // out = new byte[la + lb]  (H2: strings son byte[] UTF-8)
        w.emitGetLocal("la"); w.emitGetLocal("lb"); w.emit(OpCode.ADD);
        w.emit(OpCode.NEWARRAY_I8); w.emitSetLocal("out");

        // copy a
        emitInt(0); w.emitSetLocal("i");
        int top1 = w.newLabel(); int end1 = w.newLabel();
        w.declareLabel(top1);
        w.emitGetLocal("i"); w.emitGetLocal("la"); w.emit(OpCode.LT);
        w.emitJumpIfFalse(end1);
        w.emitGetLocal("out");
        w.emitGetLocal("i");
        w.emitGetParam("a"); w.emitGetLocal("i"); w.emit(OpCode.ALOAD_U8);
        w.emit(OpCode.ASTORE_I8);
        w.emitGetLocal("i"); emitInt(1); w.emit(OpCode.ADD); w.emitSetLocal("i");
        w.emitJump(top1);
        w.declareLabel(end1);

        // copy b at offset la
        emitInt(0); w.emitSetLocal("i");
        int top2 = w.newLabel(); int end2 = w.newLabel();
        w.declareLabel(top2);
        w.emitGetLocal("i"); w.emitGetLocal("lb"); w.emit(OpCode.LT);
        w.emitJumpIfFalse(end2);
        w.emitGetLocal("out");
        w.emitGetLocal("i"); w.emitGetLocal("la"); w.emit(OpCode.ADD);
        w.emitGetParam("b"); w.emitGetLocal("i"); w.emit(OpCode.ALOAD_U8);
        w.emit(OpCode.ASTORE_I8);
        w.emitGetLocal("i"); emitInt(1); w.emit(OpCode.ADD); w.emitSetLocal("i");
        w.emitJump(top2);
        w.declareLabel(end2);

        w.emitGetLocal("out");
        w.emitRet();
    }

    /**
     * Helper {@code __strequals(a, b): integer}: compara contenido de dos
     * strings BP (arrays i32 de code points). Devuelve 1 si son iguales o
     * ambos null, 0 si difieren. Maneja correctamente referencias null y
     * la trivialidad ref-ref.
     *
     * Algoritmo:
     *   if (a == 0 && b == 0) return 1
     *   if (a == 0 || b == 0) return 0
     *   if (a == b) return 1   // misma referencia
     *   if (len(a) != len(b)) return 0
     *   for i = 0..len(a)-1: if a[i] != b[i] return 0
     *   return 1
     */
    private void emitStrequalsBody() throws IOException {
        w.addFunction("__strequals", false);
        w.declareParam("a");
        w.declareParam("b");
        w.declareLocal("la");
        w.declareLocal("i");

        // Caso a == 0
        int aNotZero = w.newLabel();
        w.emitGetParam("a"); emitInt(0); w.emit(OpCode.EQ);
        w.emitJumpIfFalse(aNotZero);
            // a==0: si b==0, return 1; si no, return 0.
            int bAlsoZero = w.newLabel();
            w.emitGetParam("b"); emitInt(0); w.emit(OpCode.EQ);
            w.emitJumpIfFalse(bAlsoZero);
                emitInt(1); w.emitRet();
            w.declareLabel(bAlsoZero);
            emitInt(0); w.emitRet();
        w.declareLabel(aNotZero);

        // Caso b == 0 (y a != 0)
        int bNotZero = w.newLabel();
        w.emitGetParam("b"); emitInt(0); w.emit(OpCode.EQ);
        w.emitJumpIfFalse(bNotZero);
            emitInt(0); w.emitRet();
        w.declareLabel(bNotZero);

        // Caso a == b (misma referencia)
        int refsDiffer = w.newLabel();
        w.emitGetParam("a"); w.emitGetParam("b"); w.emit(OpCode.EQ);
        w.emitJumpIfFalse(refsDiffer);
            emitInt(1); w.emitRet();
        w.declareLabel(refsDiffer);

        // la = ALEN(a); lb = ALEN(b)
        w.emitGetParam("a"); w.emit(OpCode.ALEN); w.emitSetLocal("la");
        // if len(a) != len(b) → return 0
        int lenEqual = w.newLabel();
        w.emitGetLocal("la");
        w.emitGetParam("b"); w.emit(OpCode.ALEN);
        w.emit(OpCode.EQ);
        w.emitJumpIfFalse(lenEqual);
            // las longitudes son iguales → continuar al loop
            // i = 0
            emitInt(0); w.emitSetLocal("i");
            int loopTop = w.newLabel();
            int loopEnd = w.newLabel();
            w.declareLabel(loopTop);
            w.emitGetLocal("i"); w.emitGetLocal("la"); w.emit(OpCode.LT);
            w.emitJumpIfFalse(loopEnd);
                // a[i] vs b[i]
                int charsEqual = w.newLabel();
                w.emitGetParam("a"); w.emitGetLocal("i"); w.emit(OpCode.ALOAD_U8);
                w.emitGetParam("b"); w.emitGetLocal("i"); w.emit(OpCode.ALOAD_U8);
                w.emit(OpCode.EQ);
                w.emitJumpIfFalse(charsEqual);
                    // i++
                    w.emitGetLocal("i"); emitInt(1); w.emit(OpCode.ADD); w.emitSetLocal("i");
                    w.emitJump(loopTop);
                w.declareLabel(charsEqual);
                    // chars difieren → return 0
                    emitInt(0); w.emitRet();
            w.declareLabel(loopEnd);
            // recorrido completo → todos iguales → return 1
            emitInt(1); w.emitRet();
        w.declareLabel(lenEqual);
        // longitudes distintas → return 0
        emitInt(0); w.emitRet();
    }
}
