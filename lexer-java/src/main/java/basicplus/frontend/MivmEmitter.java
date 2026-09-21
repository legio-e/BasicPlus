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

    /** H6.a — bytes de la interfaz (texto del .bpi) a embeber en el .mod. Lo
     *  fija el driver de compilación (Main) antes de emitTo. Null ⇒ .mod v5. */
    /** H6.a — proveedor de la interfaz a embeber en el .mod. Es un SUPPLIER y no
     *  un byte[] a propósito: la interfaz lleva el layout binario de cada clase
     *  (numFields/numMethods), y ESE LO CALCULA EL EMISOR. Si se extrae antes de
     *  emitir, `cls.binaryLayout` aún es null y ModuleInterface cae en su rama de
     *  reconstrucción desde el AST, que cuenta CAMPOS (no slots: una ref son 2) y
     *  sólo los métodos PROPIOS (sin los heredados de Object) → el .mod publica un
     *  layout MENOR que el real, y cualquier subclase de otro módulo coloca sus
     *  miembros ENCIMA de los heredados. Se invoca en emitTo, ya emitido todo. */
    private java.util.function.Supplier<byte[]> interfaceSupplier = null;

    public final List<String> errors = new ArrayList<>();

    /** H6.a — fija CÓMO obtener la interfaz a embeber en el .mod (sección
     *  `interface`). Se registra antes de {@link #emitTo}, pero el supplier se
     *  INVOCA al final de la emisión, cuando el layout binario de las clases ya
     *  está calculado (ver el campo interfaceSupplier). */
    public void setInterfaceSupplier(java.util.function.Supplier<byte[]> supplier) {
        this.interfaceSupplier = supplier;
    }

    /**
     * Path (típicamente absoluto) al fichero fuente .bp del módulo. Se
     * vuelca al .dbg para que la VM, al imprimir un stack trace, pueda
     * citar el fichero original. Null si no se sabe.
     */
    private String sourcePath = null;
    public void setSourcePath(String sourcePath) { this.sourcePath = sourcePath; }

    /** V7/H2 (MOD8, #500) — la generación de este módulo y el sello (gen, ifaceCrc)
     *  de cada dependencia por nombre canónico; los rellena Main. */
    private int generacion = 0;
    private java.util.Map<String, int[]> versionesDeps = java.util.Collections.emptyMap();
    public void setGeneracion(int gen) { this.generacion = gen; }

    // ============================================================
    // V7/H2 (#523) — ENLACE TARDÍO (MOD9): lo que el emisor decide
    // ============================================================
    //
    // Con `formato == 9` el .mod no lleva ningún número que dependa de otro módulo:
    // una clase de OTRO módulo, o una clase ABIERTA (regla R1: algún `extends` de su
    // cadena cruza módulo), se usa SIEMPRE por referencia simbólica (regla R2) —
    // método, campo, evento, descriptor— y el cargador la resuelve contra lo que
    // tenga delante. Lo local y cerrado sigue horneado como siempre. Las claves son
    // las canónicas de Symbol.ClaveCanonica (regla R3). Diseño: V7_IDEAS §H2.7.
    //
    // El ModWriter hace solo la parte de las clases abiertas LOCALES (sus emit* por
    // nombre pasan a _REF si la clase es abierta); aquí va lo que sólo el emisor
    // sabe: qué clase es abierta, las claves de cada miembro, y las referencias a
    // clases EXTERNAS.
    private int formato = 8;
    public void setFormato(int f) { this.formato = f; formatoActual = f; }
    private boolean mod9() { return formato == 9; }
    /** Los nombres cross-module son funciones ESTÁTICAS (las usa también el AOT), así que el
     *  formato y el módulo local se publican aquí mientras conviven MOD8 y MOD9. */
    static int formatoActual = 8;
    static String moduloLocalActual = "";
    private static boolean mod9Estatico() { return formatoActual == 9; }

    /* R4 (D1): en MOD9 TODO lo que cruza lleva la firma canónica completa —también lo que ya
     * era simbólico: funciones de módulo `f(i)i`, factorías `__cls_new_Cls(i)`, inits
     * `__cls_init_Cls(i)`, el `super` `__cls_m_Cls#k()s`, los estáticos `__cls_s_Cls#m(i)i`,
     * `__cls_sg_Cls#x()i` / `__cls_ss_Cls#x(i)V`, los accesores de módulo `__prop_get_x()i`.
     * Así cambiar la firma de la CABEZA (hoy pelada: `f`, `__cls_new_Cls`) deja de casar en
     * silencio (mezcla.py «f_firma»). En MOD8 todo sigue como estaba. */
    /** `(códigos)` de los parámetros, o "" en MOD8. */
    static String firmaParams(FunctionSymbol f) {
        if (!mod9Estatico()) return "";
        if (f == null) return "()";   // sin constructor propio ni heredado: la factoría sin argumentos
        StringBuilder sb = new StringBuilder("(");
        for (Symbol.ParamSymbol pr : f.params) sb.append(Symbol.ClaveCanonica.tipo(pr.type, moduloLocalActual));
        return sb.append(')').toString();
    }
    /** `(códigos)ret`, o "" en MOD8. */
    static String firmaCompleta(FunctionSymbol f) {
        if (!mod9Estatico()) return "";
        return firmaParams(f) + Symbol.ClaveCanonica.tipo(f.returnType, moduloLocalActual);
    }
    /** El nombre canónico de una función de módulo o método tal como CRUZA: MOD9 `f(i)i`; MOD8 la clave de siempre. */
    static String nombreCruce(FunctionSymbol f) {
        return mod9Estatico() ? Symbol.ClaveCanonica.metodo(f, moduloLocalActual) : f.slotKey();
    }

    /** El nombre cualificado (library incluida) del módulo que se está compilando. */
    private String moduloLocal() { return moduloLocalDe(moduleAst); }
    static String moduloLocalDe(Ast.ModuleNode m) {
        return (m.library == null || m.library.isEmpty()) ? m.name : m.library + "." + m.name;
    }
    /** V7/H2 (#523) — los nombres R4 (`crossModuleNewName`, `nombreCruce`…) leen el formato
     *  y el módulo local de estos dos estáticos, que `emitTo` fija al emitir el `.mod`. El
     *  AOT los necesita ANTES (la validación de las `native` corre delante de la emisión, y
     *  `AotMain` construye el `.mdn` sin emisor mivm): el `.mdn` tiene que pedir al
     *  intérprete los MISMOS nombres que el `.mod` exporta. */
    static void fijarContextoEstatico(int formato, Ast.ModuleNode m) {
        formatoActual = formato; moduloLocalActual = moduloLocalDe(m);
    }
    /** R1: abierta = algún eslabón de su cadena de `extends` vive en otro módulo. */
    static boolean esAbierta(ClassSymbol c) {
        for (ClassSymbol a = c; a != null; a = a.baseClass) if (a.isExternal) return true;
        return false;
    }
    /** R2: ¿esta clase se usa por referencia? (externa o abierta; las del lenguaje no). */
    private boolean porRef(ClassSymbol c) { return mod9() && c != null && (c.isExternal || esAbierta(c)); }
    /** El módulo dueño de una clase externa en la subtabla deps (3.2). */
    private int depIdxDe(ClassSymbol externa) {
        String canon = (externa.externalLibrary == null || externa.externalLibrary.isEmpty())
                ? externa.externalModule : externa.externalLibrary + "." + externa.externalModule;
        int i = w.depIdxDe(canon);
        if (i < 0) throw new RuntimeException("la clase externa '" + externa.name + "' es de '" + canon + "', que no esta en la subtabla deps: bug del emisor");
        return i;
    }
    private int depRef(ClassSymbol c)      { return c.isExternal ? depIdxDe(c) : ModWriter.DEP_LOCAL; }
    /** El nombre de la clase tal como la publica su dueño: `[lib.]Mod.Cls` si es externa, el simple si es de aquí. */
    private String claseRef(ClassSymbol c) { return c.isExternal ? Symbol.ClaveCanonica.clase(c, moduloLocal()) : c.name; }
    private int refClase(ClassSymbol c)    { return w.requireRef(ModWriter.REF_CLASE, depRef(c), claseRef(c), ""); }
    private int refMet(ClassSymbol receptor, String clave) { return w.requireRef(ModWriter.REF_MET, depRef(receptor), claseRef(receptor), clave); }
    private int refCampo(ClassSymbol c, String clave)      { return w.requireRef(ModWriter.REF_CAMPO, depRef(c), claseRef(c), clave); }
    private int refEvento(ClassSymbol c, String clave, boolean dest) {
        return w.requireRef(dest ? ModWriter.REF_EV_DEST : ModWriter.REF_EV_RECV, depRef(c), claseRef(c), clave);
    }
    /** V7/H2 (#523, A1) — el compilador ASUME que S es subtipo de T (un upcast implícito:
     *  asignación, argumento, return, elemento de array…) y luego llama a T sobre el
     *  objeto. Si en ejecución la cadena de S ya no pasa por T («`Derived` deja de
     *  extender `Base`» en otra versión de Lib), el despacho por la ranura de T sobre un
     *  S daría basura; Java lanza `VerifyError`. La ref SUBTIPO deja la asunción escrita
     *  y el cargador la verifica al enlazar (E10). Hace falta sólo cuando T es EXTERNA:
     *  con T local, todo el tramo S..T es local (una clase ajena no puede descender de
     *  una de aquí) y se compiló junto. Nunca con T del lenguaje (`Object`: cada módulo
     *  sintetiza el suyo y el cargador la cualificaría con el consumidor → E10 falso). La
     *  clave es el nombre cualificado de T CON PUNTOS (como registra el cargador), no un
     *  código de tipo. Dedupe en el escritor: N sitios = una entrada. */
    private void asumirSubtipo(BpType s, BpType t) {
        if (!mod9() || !(s instanceof ClassType) || !(t instanceof ClassType)) return;
        ClassSymbol S = ((ClassType) s).cls, T = ((ClassType) t).cls;
        if (S == null || T == null || S == T || !T.isExternal || T.esDelLenguaje() || S.esDelLenguaje()) return;   // H1 P3: Object es el techo de todo (y Thread el de sus hijas): nada que verificar
        if (!porRef(S)) return;   // una cerrada local no desciende de una externa: si llega aquí es ErrorType/any
        w.requireRef(ModWriter.REF_SUBTIPO, depRef(S), claseRef(S), Symbol.ClaveCanonica.clase(T, moduloLocal()));
    }
    /** La clave canónica de un método/función, o null (y error) si algún tipo no está resuelto. */
    private String claveDe(FunctionSymbol fs) {
        if (!mod9()) return "";   // en MOD8 no se calcula (ni se exige que los tipos de un stub resuelvan)
        try { return Symbol.ClaveCanonica.metodo(fs, moduloLocal()); }
        catch (Symbol.ClaveCanonica.TipoSinResolver e) { errors.add(e.getMessage() + " (en '" + fs.name + "')"); return "?"; }
    }
    private String claveCampo(String nombre, BpType t) {
        if (!mod9()) return "";
        try { return Symbol.ClaveCanonica.campo(nombre, t, moduloLocal()); }
        catch (Symbol.ClaveCanonica.TipoSinResolver e) { errors.add(e.getMessage() + " (campo '" + nombre + "')"); return "?"; }
    }
    private String claveEvento(Symbol.EventSymbol ev) {
        if (!mod9()) return "";
        try { return Symbol.ClaveCanonica.evento(ev, moduloLocal()); }
        catch (Symbol.ClaveCanonica.TipoSinResolver e) { errors.add(e.getMessage() + " (evento '" + ev.name + "')"); return "?"; }
    }
    /** Las claves de los dos accesores de una property: `getX()t` / `setX(t)V`. */
    private String claveGetter(String prop, BpType t) { return "get" + capitalize(prop) + "()" + tipoClave(t); }
    private String claveSetter(String prop, BpType t) { return "set" + capitalize(prop) + "(" + tipoClave(t) + ")V"; }
    private String tipoClave(BpType t) {
        if (!mod9()) return "";
        try { return Symbol.ClaveCanonica.tipo(t, moduloLocal()); }
        catch (Symbol.ClaveCanonica.TipoSinResolver e) { errors.add(e.getMessage()); return "?"; }
    }
    /** ¿Sobreescribe `clave` a un método de algún ancestro? (el cargador la casará por clave) */
    private boolean esOverride(ClassSymbol cls, String nombre, String clave) {
        for (ClassSymbol a = cls.baseClass; a != null; a = a.baseClass) {
            Symbol m = a.instanceMembers.tryLookup(nombre);
            for (FunctionSymbol o = (m instanceof FunctionSymbol) ? (FunctionSymbol) m : null; o != null; o = o.nextOverload) {
                // V10 (16-sep, E-R2-2): lo PRIVADO de un ancestro no se sobreescribe — el cargador lo
                // salta (A2) y da ranura nueva; marcarlo override aquí era un E2 falso al enlazar.
                if (o.isPrivate) continue;
                try { if (Symbol.ClaveCanonica.metodo(o, moduloLocal()).equals(clave)) return true; }
                catch (Symbol.ClaveCanonica.TipoSinResolver e) { /* un ancestro con tipo sin resolver no casa */ }
            }
        }
        return false;
    }
    /** ¿Redeclara una property de un ancestro? (sus accesores sobreescriben los de él) */
    private boolean esOverrideProperty(ClassSymbol cls, String prop) {
        for (ClassSymbol a = cls.baseClass; a != null; a = a.baseClass) {
            Symbol p = a.instanceMembers.tryLookup(prop);
            if (p instanceof PropertySymbol && !((PropertySymbol) p).isPrivate) return true;   // V10 (F10): la privada no cuenta
        }
        return false;
    }
    /** La clase del método que se está emitiendo (el `this` implícito), o null fuera de una clase. */
    private ClassSymbol claseActual() {
        return (scopeStack.isEmpty() || scopeStack.peek().fs == null) ? null : scopeStack.peek().fs.ownerClass;
    }
    /** H1 P3 (20-sep) — una clase del LENGUAJE (Object, Thread, Mutex, StringBuilder): vive en
     *  Core.bp; aquí es el stub del `import Core` implícito (o la clase local, compilando Core). */
    private ClassSymbol claseDelLenguaje(String nombre) {
        Symbol cs = info.module.members.resolve(nombre);
        if (cs instanceof ClassSymbol) return (ClassSymbol) cs;
        throw new RuntimeException("la clase del lenguaje '" + nombre + "' no está (¿falta Core.mod?): bug del emisor");
    }
    private FunctionSymbol metodoDe(ClassSymbol cls, String nombre) {
        Symbol m = cls.lookupInstance(nombre);
        if (m instanceof FunctionSymbol) return (FunctionSymbol) m;
        throw new RuntimeException("la clase del lenguaje '" + cls.name + "' no tiene '" + nombre + "': bug del emisor");
    }
    /** `Clase()` de una clase del lenguaje sin argumentos: deja la referencia nueva en la pila. */
    private void construirDelLenguaje(String nombre) throws IOException {
        emitConstruction(claseDelLenguaje(nombre), new ArrayList<IExpr>());
    }
    /** `ref.metodo()` sobre una clase del lenguaje (el receptor ya está en la pila). */
    private void invocarDelLenguaje(String clase, String metodo, int nArgs) throws IOException {
        ClassSymbol cls = claseDelLenguaje(clase);
        emitInvokeVirtualSmart(cls, cls, metodo, claveDe(metodoDe(cls, metodo)), nArgs);
    }
    /** H1 P3 — abre una clase SINTETIZADA (`parallel`, `Thread(obj::m())`) cuyo padre es una
     *  clase del lenguaje: en MOD9, abierta contra `Core.Thread` (el cargador la construye); en
     *  MOD8, con el layout que la interfaz de Core publica; compilando Core, el padre es local. */
    private void abrirClaseSintetizada(String name, ClassSymbol base) throws IOException {
        if (!base.isExternal) { w.addClass(name, base.name); return; }
        if (mod9()) { w.addClassAbierta(name, false, Symbol.ClaveCanonica.clase(base, moduloLocal()), depIdxDe(base)); return; }
        edu.bpgenvm.generador.ModWriter.ExternalParentLayout lay = layoutPadreExterno(base, name);
        if (lay == null) return;
        w.addClass(name, nombreCualificado(base), lay);
    }
    private String nombreCualificado(ClassSymbol c) {
        StringBuilder qname = new StringBuilder();
        if (c.externalLibrary != null && !c.externalLibrary.isEmpty()) qname.append(c.externalLibrary).append('.');
        return qname.append(c.externalModule).append('.').append(c.name).toString();
    }
    /** MOD8: el layout binario del padre externo (bpi 8), con los nombres de sus métodos por ranura
     *  para que un override reutilice la ranura heredada. null (y error) si la interfaz no lo trae. */
    private edu.bpgenvm.generador.ModWriter.ExternalParentLayout layoutPadreExterno(ClassSymbol base, String hija) {
        SemanticInfo.ClassBinaryLayout layout = base.binaryLayout;
        if (layout == null) {
            errors.add("L2 v3: parent cross-module '" + base.name + "' de '" + hija
                    + "' sin layout binario en .bpi (bpi 9, enlace tardio) — compila con --formato=9, o recompila el dueño en MOD8");
            return null;
        }
        String[] mNames = new String[layout.numMethods];
        if (base.externalMethodSlots != null) {
            for (java.util.Map.Entry<String, Integer> me : base.externalMethodSlots.entrySet()) {
                int sl = me.getValue();
                if (sl >= 0 && sl < mNames.length) mNames[sl] = me.getKey();
            }
        }
        return new edu.bpgenvm.generador.ModWriter.ExternalParentLayout(
                layout.numFields, layout.numMethods, layout.fieldBitmap, layout.ownerBitmap, mNames);
    }

    /** El nombre cualificado con el que se pide una función de OTRO módulo: `[lib.]Mod.f(i)i` en MOD9,
     *  `[lib.]Mod.f` / `Mod.f$s` en MOD8 (externalQualifiedName). */
    private String nombreExterno(FunctionSymbol fs) {
        if (!mod9()) return fs.externalQualifiedName();
        StringBuilder sb = new StringBuilder();
        if (fs.externalLibrary != null && !fs.externalLibrary.isEmpty()) sb.append(fs.externalLibrary).append('.');
        return sb.append(fs.externalModule).append('.').append(claveDe(fs)).toString();
    }

    /** La clase ESTÁTICA del receptor de una expresión (para la ref), o `porDefecto`. */
    private ClassSymbol receptorDe(IExpr target, ClassSymbol porDefecto) {
        BpType t = info.exprTypes.get(target);
        return (t instanceof ClassType) ? ((ClassType) t).cls : porDefecto;
    }
    public void setVersionesDeps(java.util.Map<String, int[]> m) { this.versionesDeps = (m == null) ? java.util.Collections.emptyMap() : m; }
    /** El CRC de la sección interface tal como se escribió (válido tras emitTo). */
    public int getInterfaceCrc() { return w.getInterfaceCrc(); }

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

    /** L8 v2 — vars string de módulo con init literal: pares
     *  {nombreVar, símbolo del literal interned}. El __init sintético emite
     *  LEA literal + SET_GLOBAL var (la dirección es de load-time). */
    private final List<String[]> pendingStrVarInits = new ArrayList<>();

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
        /** V7/H1 (21-sep) — un destructor encadena al del padre en el PUNTO ÚNICO de salida
         *  (tras el endLabel, antes del RET): así corre también tras un `return` del cuerpo. */
        boolean encadenarDtorDelPadre = false;
        /** V7/H1 — la función tiene `var owner` locales: su cuerpo va dentro de un TRY que, si
         *  sale una excepción, destruye y libera los owners antes de relanzarla. */
        boolean guardaOwners = false;
        /** True si la función tiene tipo de retorno (no void). */
        final boolean returnsValue;
        int ownersHandler = -1;   // V7/H1: la etiqueta del manejador que guarda los owners
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

    /** Andamio de PRUEBA (20-sep) — `#521` afirma que un error del EMISOR deja el build en rojo
     *  sin `.mod`; desde que el semántico rechaza también `c.m := 5`, no queda ningún error que
     *  sólo dé el emisor desde un fuente válido, así que EmisorConErroresTest lo fuerza aquí.
     *  Nunca se pone a true fuera de un test. */
    static boolean PRUEBA_ERROR_DE_EMISOR = false;

    public void emitTo(Path outputDir) throws IOException {
        if (PRUEBA_ERROR_DE_EMISOR) errors.add("[prueba] error del emisor forzado (EmisorConErroresTest)");
        w.setFormato(formato);   // V7/H2 (#523): 8 o 9
        formatoActual = formato; moduloLocalActual = moduloLocal();
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
        // V7/H2 (MOD8, #500) — la generación propia y el SELLO de cada dependencia:
        // por cada namespace importado (con `import I:M` el que cuenta es el IMPL, que
        // es contra quien van los CALL_EXT), su nombre canónico `[lib.]Mod` y el
        // (gen, ifaceCrc) que Main leyó al resolver su interfaz. Sin sello es un bug
        // del compilador: se dice, no se escribe (0,0) en silencio.
        w.setGeneracion(generacion);
        if (moduleAst.imports != null && info.module != null) {
            for (ImportNode imp : moduleAst.imports) {
                if (imp.path == null || imp.path.isEmpty()) continue;
                Symbol sym = info.module.members.tryLookup(imp.path.get(imp.path.size() - 1));
                if (!(sym instanceof Symbol.ImportedNamespaceSymbol)) continue;   // «phantom»: sin interfaz, sin CALL_EXT
                Symbol.ImportedNamespaceSymbol ns = (Symbol.ImportedNamespaceSymbol) sym;
                String canon = (ns.library == null || ns.library.isEmpty()) ? ns.moduleName : ns.library + "." + ns.moduleName;
                int[] sello = versionesDeps.get(canon);
                if (sello == null)
                    throw new RuntimeException("el módulo '" + canon + "' se importa sin sello de versión (gen, ifaceCrc): bug del compilador");
                w.declareDependency(canon, ns.fromPath, sello[0], sello[1]);
            }
            // Y la marca EXTIENDE: cada clase de este módulo cuya base vive en una
            // dependencia (norma de Eduardo: un padre nunca más reciente que sus hijos).
            for (ITopLevelDecl d : moduleAst.defs) {
                if (!(d instanceof ClassDef)) continue;
                Symbol cs = info.declSymbols.get(d);
                if (!(cs instanceof ClassSymbol)) continue;
                ClassSymbol base = ((ClassSymbol) cs).baseClass;
                if (base == null || !base.isExternal || base.externalModule == null || base.externalModule.isEmpty()) continue;
                String canon = (base.externalLibrary == null || base.externalLibrary.isEmpty())
                        ? base.externalModule : base.externalLibrary + "." + base.externalModule;
                w.markDependency(canon, edu.bpgenvm.generador.ModWriter.DEP_EXTIENDE);
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
            // H1.8: el Mutex es un OBJETO = REFERENCIA (8B). Era declareGlobal (4B):
            // el __init lo guarda con emitGetLocal(newref) —que empuja 8B, porque el
            // local SÍ es declareLocalLong— y emitSetGlobal popeaba 4 → handle
            // TRUNCADO (gen perdida) + fuga de 4B en la pila. Misma firma que #10.
            declareGlobalRef(MODULE_SYNC_MUTEX_GLOBAL);
        }

        // 2.5) Flag de idempotencia para __init: declarado siempre porque
        //      __init existe siempre (incluso si el módulo no tiene
        //      inicializador propio ni imports).
        w.addConstantInt("__initialized", 0);

        // 3a) H1 P3 (20-sep) — Object, Thread, Mutex y StringBuilder YA NO SE SINTETIZAN aquí:
        //     viven en BP en `Core.bp` (como List desde #450) y todo módulo las referencia por
        //     enlace tardío (#523). Hasta hoy cada .mod llevaba su copia (≈1,3 KB).

        // 3a-bis) Clases de tupla `__Tuple_<sig>` para cada forma de retorno
        //         múltiple usada en el módulo. Antes de las funciones (phase 5)
        //         que las construyen con NEW_OBJECT.
        collectTupleShapes();
        synthesizeTupleClasses();

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

        // 4a-ter) #325 — subclases de Thread de cada `Thread(obj::metodo(args))`.
        //         Mismo motivo y mismo sitio que las de `parallel`.
        synthesizeAllThreadBodyClasses();

        // 4b) Clases del usuario (ANTES de las funciones del módulo, para que
        //     Main pueda referenciar Cls.__init y class descriptors al construir).
        //     Los EnumDef no requieren emisión: sus valores ya están en EnumSymbol.values
        //     y se inlinean como literales int en emitMemberAccess.
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof ClassDef) emitClassDef((ClassDef) d);
        }

        // 4c-bis) #325 — los cuerpos de los `Thread(obj::metodo(args))`. Aquí y
        //         no en 4a-ter por el mismo motivo que el helper de arriba: la
        //         llamada necesita la clase del receptor ya registrada.
        synthesizeAllThreadBodyFns();

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
        // #521 (16-sep) — con ERRORES DEL EMISOR no hay artefacto. Antes el .mod se
        // escribía igual, Main decía «compilado:», y el programa enlazaba y corría con
        // basura idéntica en las dos VMs («verificar el artefacto, no el log» al
        // revés: el log decía error y el artefacto existía). Y se borra el gemelo
        // rancio de una compilación anterior, que si no taparía el fallo.
        if (!errors.isEmpty()) {
            String modName = w.getCanonicalFilename();
            Files.deleteIfExists(out);
            Files.deleteIfExists(outputDir.resolve(modName.substring(0, modName.length() - 4) + ".dbg"));
            return;
        }
        // H6.a — embebe la interfaz del módulo en el .mod (sección `interface`,
        // formato v6). Si no se ha fijado, writeToFile emite v5 como antes.
        // El supplier se invoca AQUÍ, no antes: a estas alturas ya se han emitido
        // todas las clases, así que `cls.binaryLayout` lleva el layout REAL (el que
        // contó el ModWriter) y la interfaz publica ese, no una reconstrucción.
        if (interfaceSupplier != null) {
            byte[] ib = interfaceSupplier.get();
            if (ib != null) w.setInterfaceBytes(ib);
        }
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
        sb.append("dbg 3\n");
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
        // Sección vars (.dbg v3 — H6.a.1): por función, params + locales
        // user-visible (los temporales sintéticos `__*` los filtra ModWriter).
        // Permite al host resolver locales por nombre. Formato:
        //   vars
        //   func <qualifiedName> <startRelPc>
        //   <varName> <signedOffset> <sizeBytes> <isArray:0|1>
        // <signedOffset> es relativo a bp: negativo = param, positivo = local.
        // El .mod NO cambia; la VM-C ignora el .dbg por completo.
        java.util.Map<String, java.util.List<ModWriter.VarDebugInfo>> fvars = w.getFunctionVarsDebug();
        if (fvars != null && !fvars.isEmpty()) {
            boolean wroteVarsHeader = false;
            for (java.util.Map.Entry<String, java.util.List<ModWriter.VarDebugInfo>> e : fvars.entrySet()) {
                Integer start = w.getFunctionStartRel(e.getKey());
                if (start == null) continue;          // función sin dirección (no debería ocurrir)
                if (!wroteVarsHeader) { sb.append("vars\n"); wroteVarsHeader = true; }
                sb.append("func ").append(e.getKey()).append(' ').append(start).append('\n');
                for (ModWriter.VarDebugInfo v : e.getValue()) {
                    sb.append(v.name).append(' ').append(v.offset).append(' ')
                      .append(v.sizeBytes).append(' ').append(v.isArray ? 1 : 0)
                      .append(' ').append(v.typeTag)   // H6.a.2: tag de tipo BP (5ª col)
                      .append('\n');
                }
            }
        }
        Files.write(dbgOut, sb.toString().getBytes(java.nio.charset.StandardCharsets.UTF_8));
    }

    /**
     * H6.a.2 — tag de tipo BP para la sección `vars` del `.dbg`. El debugger lo
     * usa para renderizar el valor (string→texto, double→f64, boolean→true/false,
     * ref→@handle). Tags canónicos que entiende DebugServer:
     *   integer · long · float · double · string · boolean · ref · "?"
     * Los enteros estrechos (byte/word/int8/int16) → integer (la VM opera en i32);
     * los enums son int-backed → integer; las clases/arrays(heap) → ref.
     */
    private static String varDbgTypeTag(BpType t) {
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case INTEGER: case INT8: case UINT8: case INT16: case UINT16: return "integer";
                case LONG:    return "long";
                case FLOAT:   return "float";
                case DOUBLE:  return "double";
                case STRING:  return "string";
                case BOOLEAN: return "boolean";
            }
        }
        if (t instanceof EnumType)  return "integer";   // enum respaldado por int
        if (t instanceof ClassType) return "ref";       // handle de objeto
        if (t instanceof ArrayType) return "ref";       // handle de array en heap (locales inline → isArray)
        return "?";                                     // any / desconocido → valor crudo
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

            // L8 v2 — vars string de módulo con init literal: el LITERAL vive
            // horneado en el data block (interned), pero la var guarda una REF
            // y la dirección solo se conoce al cargar → se asigna aquí (LEA +
            // SET_GLOBAL). El usuario no escribe nada; la var sigue siendo
            // reasignable después.
            for (String[] p : pendingStrVarInits) {
                w.emitLeaGlobal(p[1]);
                w.emitSetGlobal(p[0]);
            }

            // Inicializar mutex compartido para sync properties de módulo, si
            // las hay. Se hace ANTES de las dependencias por simetría con la
            // inicialización de un objeto: lo nuestro primero, luego cascada.
            if (hasModuleSyncProperty()) {
                int id = tempCounter++;
                String newref  = "__modmtx_newref_"  + id;
                String discard = "__modmtx_discard_" + id;
                declareLocalRef(newref);   // H1.8: el Mutex es una ref (era declareLocalLong = un 8 a mano)
                w.declareLocal(discard);
                construirDelLenguaje("Mutex");   // H1 P3: Core.Mutex (deja la ref en la pila)
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
                    // #386 — el argumento de Main sale de su VALOR POR DEFECTO
                    // (H8.1), y sólo vale "" si no declara ninguno.
                    //
                    // Antes se horneaba "" mirando únicamente que hubiera UN
                    // parámetro: `Main(arg: string := "/sd/medidas.db")`
                    // compilaba sin una palabra y llegaba vacío, así que todo
                    // `if arg != "" then ...` era código muerto sin avisar
                    // (samples/SqlDemo.bp lo tenía, y el LEEME de H4 llegó a
                    // prometer algo imposible). Aceptar un valor y tirarlo es
                    // peor que no aceptarlo.
                    //
                    // Idea de Eduardo: no hace falta mecanismo nuevo — el
                    // lenguaje YA tiene dónde escribir ese valor. Sin default el
                    // comportamiento es idéntico al de siempre, así que no se
                    // mueve nada de lo que ya funciona.
                    //
                    // El literal se lee directo y no se emite como expresión
                    // porque aquí no hay ámbito: el semántico ya garantizó que
                    // el único parámetro de Main es `string` y que su default
                    // es asignable a él, o sea que si hay default es un
                    // StringLitExpr. Pasar el runtime el argumento DE VERDAD
                    // (wire + IDE + los 3 firmwares) sigue pendiente aparte.
                    ParamSymbol p0 = fm.params.get(0);
                    String valor = (p0.defaultExpr instanceof StringLitExpr)
                                 ? ((StringLitExpr) p0.defaultExpr).value
                                 : "";
                    w.emitLeaGlobal(internString(valor));
                    /* V6/#412 — y AQUI deja de ser el defecto horneado: el
                     * literal pasa a ser el ARGUMENTO DEL BUILTIN, que devuelve
                     * el argumento de ejecucion si lo hay y si no una copia del
                     * defecto — SIEMPRE alojada en el heap. Es la linea que el
                     * comentario de #386 dejaba pendiente («pasar el runtime el
                     * argumento DE VERDAD»), y con ella `Main` recibe siempre
                     * una cadena del heap en vez de un literal de la zona de
                     * datos: se acaba la asimetria de #389. */
                    w.emit(OpCode.CALL_BUILTIN);
                    w.emitShort((short) Builtin.RUN_ARG.id);
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
        BpType declT = vd.type != null ? typeRefToBpType(vd.type) : null;   // H1.2
        for (DeclName dn : vd.names) {
            if (dn.isStatic()) { errors.add("var estática de clase a nivel módulo no soportada: " + dn.name); continue; }

            Symbol msym = (info.module != null) ? info.module.members.tryLookup(dn.name) : null;
            VarSymbol vs = (msym instanceof VarSymbol) ? (VarSymbol) msym : null;

            // L8 v2 — array de TAMAÑO FIJO `tipo[N]`: la var ES el array,
            // horneado en el data block como [u32 N][elems] — la misma forma
            // que un array heap visto desde el user_ref (HEAP_LAYOUT §2), así
            // que ALOAD/ASTORE/len operan tal cual. Leer la var = LEA de la
            // dirección del símbolo (binding fija; el semántico bloquea la
            // reasignación). Cero heap, cero código de arranque.
            if (vs != null && vs.fixedArraySize >= 0) {
                emitFixedModuleArray(vs, vd);
                continue;
            }

            // L8 v2 — init LITERAL: el valor va HORNEADO en el slot del data
            // block (cero código de arranque). Para string, el literal queda
            // interned en el data block y __init asigna la REF (la dirección
            // es de load-time, no horneable) — la var sigue reasignable.
            // Cualquier otro init se ignora como siempre (aviso L8).
            BpType t = (declT != null) ? declT : (vs != null ? vs.type : null);
            Object lit = (vd.init != null) ? SemanticAnalyzer.constLiteralValue(vd.init) : null;
            if (lit != null && bakeModuleVarInit(dn.name, t, lit)) continue;

            // Sin init horneable: slot a cero, 4 bytes (8 si long/double).
            declareGlobalByType(dn.name, t);   // H1.8: el ancho lo decide el TIPO (era if/else aquí)
        }
    }

    /** L8 v2 — hornea el init literal de una var de módulo en su slot del
     *  data block. Devuelve false si la combinación tipo/literal no es
     *  horneable (el caller declara el slot a cero, comportamiento clásico). */
    private boolean bakeModuleVarInit(String name, BpType t, Object lit) {
        PrimitiveType.Kind k = (t instanceof PrimitiveType) ? ((PrimitiveType) t).tag : null;
        if (k == null) {   // sin tipo declarado ni inferido: deducir del literal
            if (lit instanceof Long)         k = PrimitiveType.Kind.INTEGER;
            else if (lit instanceof Double)  k = PrimitiveType.Kind.FLOAT;
            else if (lit instanceof Boolean) k = PrimitiveType.Kind.BOOLEAN;
            else if (lit instanceof String)  k = PrimitiveType.Kind.STRING;
            else return false;
        }
        switch (k) {
            case STRING:
                if (!(lit instanceof String)) return false;
                declareGlobalRef(name);   // #10 (censo V4): la REF de string es 8B (era declareGlobal=4B → truncaba el handle + fuga de pila en __init). H1.8: por el carril de ref, no un 8 a mano
                pendingStrVarInits.add(new String[]{ name, internString((String) lit) });
                return true;
            case LONG:
                w.addConstantLong(name, litAsLong(lit));
                return true;
            case DOUBLE:
                w.addConstantLong(name, Double.doubleToRawLongBits(litAsDouble(lit)));
                return true;
            case FLOAT:
                w.addConstantFloat(name, (float) litAsDouble(lit));
                return true;
            case BOOLEAN:
                w.addConstantInt(name, (lit instanceof Boolean && (Boolean) lit) ? 1 : 0);
                return true;
            default:
                // integer + narrow (byte/word/int8/int16): slot de 4 bytes con
                // el valor (los stores posteriores ya truncan al ancho).
                if (!(lit instanceof Number)) return false;
                w.addConstantInt(name, (int) litAsLong(lit));
                return true;
        }
    }

    /** L8 v2 — data symbol [u32 N][elems empaquetados al ancho del elemento]
     *  de un array fijo de módulo. Los literales del init (validados por el
     *  semántico) rellenan el principio; el resto queda a cero. */
    private void emitFixedModuleArray(VarSymbol vs, VarDecl vd) {
        int n = vs.fixedArraySize;
        BpType el = (vs.type instanceof ArrayType) ? ((ArrayType) vs.type).element : null;
        PrimitiveType.Kind k = (el instanceof PrimitiveType)
                ? ((PrimitiveType) el).tag : PrimitiveType.Kind.INTEGER;
        List<IExpr> elems = (vd.init instanceof ArrayLitExpr)
                ? ((ArrayLitExpr) vd.init).elements : java.util.Collections.<IExpr>emptyList();
        try {
            switch (k) {
                case UINT8: case INT8: {
                    byte[] v = new byte[n];
                    for (int i = 0; i < elems.size(); i++)
                        v[i] = (byte) litAsLong(SemanticAnalyzer.constLiteralValue(elems.get(i)));
                    w.addConstantArrayInt8(vs.name, v);
                    break;
                }
                case UINT16: case INT16: {
                    short[] v = new short[n];
                    for (int i = 0; i < elems.size(); i++)
                        v[i] = (short) litAsLong(SemanticAnalyzer.constLiteralValue(elems.get(i)));
                    w.addConstantArrayInt16(vs.name, v);
                    break;
                }
                case LONG: {
                    long[] v = new long[n];
                    for (int i = 0; i < elems.size(); i++)
                        v[i] = litAsLong(SemanticAnalyzer.constLiteralValue(elems.get(i)));
                    w.addConstantArrayLong(vs.name, v);
                    break;
                }
                case DOUBLE: {
                    long[] v = new long[n];
                    for (int i = 0; i < elems.size(); i++)
                        v[i] = Double.doubleToRawLongBits(litAsDouble(SemanticAnalyzer.constLiteralValue(elems.get(i))));
                    w.addConstantArrayLong(vs.name, v);
                    break;
                }
                case FLOAT: {
                    float[] v = new float[n];
                    for (int i = 0; i < elems.size(); i++)
                        v[i] = (float) litAsDouble(SemanticAnalyzer.constLiteralValue(elems.get(i)));
                    w.addConstantArrayFloat(vs.name, v);
                    break;
                }
                default: {   // INTEGER / BOOLEAN: 4 bytes por elemento
                    int[] v = new int[n];
                    for (int i = 0; i < elems.size(); i++)
                        v[i] = (int) litAsLong(SemanticAnalyzer.constLiteralValue(elems.get(i)));
                    w.addConstantArray(vs.name, v);
                    break;
                }
            }
        } catch (Exception e) {
            errors.add("array fijo de módulo '" + vs.name + "': " + e.getMessage());
        }
    }

    /** Literal (Long/Double/Boolean de constLiteralValue) → long. */
    private static long litAsLong(Object lit) {
        if (lit instanceof Number)  return ((Number) lit).longValue();
        if (lit instanceof Boolean) return ((Boolean) lit) ? 1 : 0;
        return 0;
    }

    /** Literal numérico → double. */
    private static double litAsDouble(Object lit) {
        if (lit instanceof Number) return ((Number) lit).doubleValue();
        return 0.0;
    }

    private void emitModuleConstDecl(ConstDecl cd) {
        if (cd.name.isStatic()) {
            errors.add("const estática de clase a nivel módulo no soportada: " + cd.name.name);
            return;
        }
        // L8 v2 — las const de módulo con valor literal se INLINAN en cada uso
        // (loadFromSymbol/emitMemberAccess leen ConstSymbol.literalValue, el
        // mismo mecanismo que las const importadas de .bpi). Mantenemos el data
        // symbol de int/float-literal por compatibilidad (debug, convención de
        // offsets #172); el resto de tipos ya NO materializa símbolo. El
        // antiguo fallback "global a cero asignado por el initializer" era
        // mentira (nadie asignaba) → string/long/double/bool leían 0/basura;
        // hoy un init no-literal es error semántico.
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
        // Sin símbolo: el valor viaja inlined en cada uso.
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

    // L6 — static property de clase: backing global cualificado + accesores como
    // funciones cualificadas (mismo naming-space que los static methods `Cls.f`).
    static String staticBackingName(String clsName, String propName) { return clsName + ".__prop_" + propName; }
    static String staticGetterName(String clsName, String propName)  { return clsName + ".__prop_get_" + propName; }
    static String staticSetterName(String clsName, String propName)  { return clsName + ".__prop_set_" + propName; }

    /* V7/H2 (#492-B) — ALIAS PLANOS de los estáticos de clase. La función local
     * se llama `Cls.m` (con punto) y un import `Mod.Cls.m` lo parten mal los dos
     * cargadores (leen library=Mod, módulo=Cls). El dueño exporta un alias sin
     * puntos al mismo offset (ModWriter.addExportAlias: cero bytecode) y el
     * consumidor lo importa. DEFINICIÓN y LLAMADA usan SIEMPRE estos helpers —
     * misma regla que emitName/crossModuleMethodName. La clave de un método es
     * emitName(fs) = slotKey (con el `$…` de la sobrecarga). */
    /* Dos reglas de forma, medidas el 14-sep por la revisión adversarial:
     * (1) el separador clase/miembro es '#' y no '_': '_' es un carácter legal de
     *     identificador y `A_b.c` / `A.b_c` daban el MISMO nombre (`__cls_m_` lo
     *     arrastraba desde #451: un `super.c()` llamaba al método equivocado);
     * (2) la clave del miembro va SIEMPRE por FIRMA (`name$códigos`, overloadMangle),
     *     nunca por posición en la cadena (`emitName` = slotKey): si la interfaz
     *     omite la cabeza (private, tipo no exportable) el consumidor renumera y
     *     llamaba a la sobrecarga equivocada en silencio. Con la firma, los dos
     *     lados calculan lo mismo sin saber qué omitió el otro. */
    static String crossModuleStaticName(ClassSymbol cls, FunctionSymbol fs) { return "__cls_s_" + cls.name + "#" + (mod9Estatico() ? Symbol.ClaveCanonica.metodo(fs, moduloLocalActual) : fs.overloadMangle()); }
    static String crossModuleStaticGetter(String clsName, String propName, BpType t) { return "__cls_sg_" + clsName + "#" + propName + (mod9Estatico() ? "()" + Symbol.ClaveCanonica.tipo(t, moduloLocalActual) : ""); }
    static String crossModuleStaticSetter(String clsName, String propName, BpType t) { return "__cls_ss_" + clsName + "#" + propName + (mod9Estatico() ? "(" + Symbol.ClaveCanonica.tipo(t, moduloLocalActual) + ")V" : ""); }

    /** `[lib.]Mod.<plano>` para un símbolo del módulo dueño de una clase EXTERNA. */
    private static String qualifyForOwner(ClassSymbol owner, String plain) {
        StringBuilder qn = new StringBuilder();
        if (owner.externalLibrary != null && !owner.externalLibrary.isEmpty())
            qn.append(owner.externalLibrary).append('.');
        qn.append(owner.externalModule).append('.').append(plain);
        return qn.toString();
    }

    /** Llamada a un método ESTÁTICO (args ya en pila): local por nombre, o CALL_EXT
     *  al alias si la clase que lo DECLARA es de otro módulo (`Hija.cuantos()` con
     *  Hija local hija de una Base externa resuelve al símbolo de Base). */
    private void emitStaticCall(FunctionSymbol fs) throws IOException {
        ClassSymbol duena = fs.ownerClass;
        if (duena != null && duena.isExternal) {
            String qn = qualifyForOwner(duena, crossModuleStaticName(duena, fs));
            w.emitCallExt(qn, fromPathFor(qn));
        } else {
            w.emitCall(duena.name + "." + emitName(fs));   // H5.a
        }
    }
    private void emitStaticGetter(PropertySymbol ps) throws IOException {
        ClassSymbol duena = ps.ownerClass;
        if (duena != null && duena.isExternal) {
            String qn = qualifyForOwner(duena, crossModuleStaticGetter(duena.name, ps.name, ps.type));
            w.emitCallExt(qn, fromPathFor(qn));
        } else {
            w.emitCall(staticGetterName(duena.name, ps.name));
        }
    }
    private void emitStaticSetter(PropertySymbol ps) throws IOException {
        ClassSymbol duena = ps.ownerClass;
        if (duena != null && duena.isExternal) {
            String qn = qualifyForOwner(duena, crossModuleStaticSetter(duena.name, ps.name, ps.type));
            w.emitCallExt(qn, fromPathFor(qn));
        } else {
            w.emitCall(staticSetterName(duena.name, ps.name));
        }
    }

    /** Construye el qualified name de un accesor externo (`lib.Mod.__prop_get_x`). */
    private static String buildExternalAccessorName(PropertySymbol ps, boolean setter) {
        StringBuilder sb = new StringBuilder();
        if (ps.externalLibrary != null && !ps.externalLibrary.isEmpty()) {
            sb.append(ps.externalLibrary).append('.');
        }
        sb.append(ps.externalModule).append('.');
        sb.append(setter ? moduleSetterName(ps.name) : moduleGetterName(ps.name));
        sb.append(sufijoAccesor(ps.type, setter));   // V7/H2 (#523, R4): `()i` / `(i)V` en MOD9
        return sb.toString();
    }
    /** R4: el sufijo tipado de un accesor de property (`()t` getter, `(t)V` setter), "" en MOD8. */
    static String sufijoAccesor(BpType t, boolean setter) {
        if (!mod9Estatico()) return "";
        String c = Symbol.ClaveCanonica.tipo(t, moduloLocalActual);
        return setter ? "(" + c + ")V" : "()" + c;
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
        // H1.8 (#16): por TIPO. Era declareGlobal (4B) siempre → una property de módulo
        // de tipo ref (`public property nombre: string`) tenía el backing a 4B y sus
        // accesores emitían GET/SET_GLOBAL de 4B sobre un handle de 8B (verificado en
        // el disasm). El de CLASE (static property) ya era width-aware — el gemelo de
        // MÓDULO se quedó atrás, como en #10. Mismo propType que los accesores.
        declareGlobalByType(moduleBackingName(pd.name.name), modulePropertyType(pd));
    }

    /** H1.8 — tipo de una property de módulo, con la misma resolución que usan sus
     *  accesores (AST y, si el analyzer cargó uno más rico, el del símbolo). */
    private BpType modulePropertyType(PropertyDef pd) {
        BpType t = (pd.type != null) ? typeRefToBpType(pd.type) : null;
        Symbol psym = info.declSymbols.get(pd);
        if (psym instanceof PropertySymbol && ((PropertySymbol) psym).type != null) {
            t = ((PropertySymbol) psym).type;
        }
        return t;
    }

    /** Lock/unlock contra el mutex compartido del módulo. */
    private void emitModuleSyncLock() throws IOException {
        int id = tempCounter++;
        String discard = "__modSyncLock_" + id;
        declareLocal(discard);
        w.emitGetGlobal(MODULE_SYNC_MUTEX_GLOBAL);
        invocarDelLenguaje("Mutex", "lock", 0);
        w.emitSetLocal(discard);
    }
    private void emitModuleSyncUnlock() throws IOException {
        int id = tempCounter++;
        String discard = "__modSyncUnlock_" + id;
        declareLocal(discard);
        w.emitGetGlobal(MODULE_SYNC_MUTEX_GLOBAL);
        invocarDelLenguaje("Mutex", "unlock", 0);
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
            if (mod9() && pd.isPublic) {   // V7/H2 (#523, R4): los nombres con los que cruzan
                w.addExportAlias(getter + sufijoAccesor(propType, false), getter);
                w.addExportAlias(setter + sufijoAccesor(propType, true), setter);
            }

            // ---- Getter ----
            w.addFunction(getter, pd.isPublic);
            FunctionSymbol gfs = new FunctionSymbol(getter, pd.isPublic, false, false, null, null);
            gfs.returnType = propType;
            beginFunctionScope(gfs, propType);
            currentModulePropertyBacking = backing;
            if (pd.isSync) scopeStack.peek().syncModule = true;
            try {
                int guarda = -1;
                if (pd.isSync) { emitModuleSyncLock(); guarda = abrirGuardaSync(); }
                if (pd.getter != null) {
                    for (IStmt s : pd.getter.body) emitStmt(s);
                } else {
                    // auto-getter: __result := __prop_<name>
                    w.emitGetGlobal(backing);
                    w.emitSetLocal("__result");
                }
                if (pd.isSync) cerrarGuardaSync(guarda, null);   // #527 (b)
                emitFunctionEnd();
            } finally {
                currentModulePropertyBacking = null;
                scopeStack.pop();
            }

            // ---- Setter ----
            w.addFunction(setter, pd.isPublic);
            declareParamByType("__val", propType);   // el setter de property de MÓDULO ahora declara __val
                                       // por su TIPO (propType) — si la property es ref (string/objeto),
                                       // va a 8B sola. Antes era 4B fijo → drift. (El de CLASE ya lo era.)
            FunctionSymbol sfs = new FunctionSymbol(setter, pd.isPublic, false, false, null, null);
            beginFunctionScope(sfs, null);
            currentModulePropertyBacking = backing;
            if (pd.isSync) scopeStack.peek().syncModule = true;
            try {
                int guarda = -1;
                if (pd.isSync) { emitModuleSyncLock(); guarda = abrirGuardaSync(); }
                if (pd.setter != null) {
                    // #527 (de paso) — por su TIPO, como el de clase: era declareLocal (4 B) y
                    // un setter escrito de una property string/objeto/long de módulo truncaba
                    // el valor (hallazgo 34: con generación 0 funciona igual).
                    declareLocalByType(pd.setter.paramName, propType, null);
                    w.emitGetParam("__val");
                    w.emitSetLocal(pd.setter.paramName);
                    for (IStmt s : pd.setter.body) emitStmt(s);
                } else {
                    // auto-setter: __prop_<name> := __val
                    w.emitGetParam("__val");
                    w.emitSetGlobal(backing);
                }
                if (pd.isSync) cerrarGuardaSync(guarda, null);   // #527 (b)
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
                // H1 (20-sep) — una intrínseca lleva su bytecode en el cuerpo (`inline …`) y no se
                // emite en el .mod: cada llamada lo inlina. (Hasta hoy: la palabra `intrinsic` +
                // un registro a mano en Intrinsics.java, que ya no existe.)
                if (fs.inlineOps == null) errors.add(fs.name + ": intrínseca sin cuerpo inline");
                return;
            }

            String name = emitName(fs);  // H5.a: mangleado si sobrecargada; si no, pelado
            // Siempre exportadas (true) para que aparezcan en stack traces aunque
            // sean privadas en BP. La privacidad de BP se enforza en compile-time
            // (lookup en SemanticAnalyzer), no a nivel de runtime.
            w.addFunction(name, true);
            // V7/H2 (#523, R4) — y en MOD9 el nombre con el que CRUZA: `f(i)i`. El pelado se
            // queda (stack traces, y el parque MOD8 que aún pida `Mod.f`).
            if (mod9() && fs.isPublic && !name.equals("main")) w.addExportAlias(claveDe(fs), name);

            // Declarar params (H1.2: width-aware, long = 8 bytes)
            declareParamsWidthAware(fs);

            beginFunctionScope(fs, null);
            abrirGuardaDeOwners(fn.body);   // V7/H1
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
                    // H1.8 (hermano de #16): por TIPO. Era declareGlobal (4B) siempre →
                    // una `var` estática de clase de tipo ref quedaba a 4B, mientras que
                    // la static PROPERTY de aquí al lado ya era width-aware.
                    declareGlobalByType(cd.name + "." + v.name, v.type);
                } else if (s instanceof PropertySymbol) {
                    // L6 — backing de static property: global cualificado, width-aware.
                    PropertySymbol ps = (PropertySymbol) s;
                    String backing = staticBackingName(cd.name, ps.name);
                    declareGlobalByType(backing, ps.type);   // H1.8: el ancho lo decide el TIPO
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
            boolean abierta9 = mod9() && esAbierta(cls);   // V7/H2 (#523): sin placeholders del padre; su layout no hace falta (bpi 9 no lo trae)
            if (cls.baseClass != null && !abierta9) {
                if (cls.baseClass.isExternal) {
                    parentForModWriter = nombreCualificado(cls.baseClass);
                    externalParent = layoutPadreExterno(cls.baseClass, cls.name);
                    if (externalParent == null) return;
                } else {
                    parentForModWriter = cls.baseClass.name;
                }
            }
            // H1 P3 (20-sep) — sin base = `root class` (Object, Thread; sólo en Core): el semántico
            // ya puso Core.Object a toda clase sin `extends`, así que aquí null es «no hereda nada».
            if (abierta9) {
                // V7/H2 (#523) — ABIERTA: sin placeholders del padre y sin descriptor; el
                // cargador la construye contra el padre real. Su padre es o externo (dep)
                // u otra abierta de este módulo (local).
                ClassSymbol base = cls.baseClass;
                if (base.isExternal) w.addClassAbierta(cd.name, cd.isPublic, Symbol.ClaveCanonica.clase(base, moduloLocal()), depIdxDe(base));
                else                 w.addClassAbierta(cd.name, cd.isPublic, base.name, ModWriter.DEP_LOCAL);
            } else {
                w.addClass(cd.name, parentForModWriter, externalParent);
            }

            // ¿Esta clase declara su propio __syncMutex (porque tiene sync
            // properties Y ningún ancestor lo ha declarado ya)? El field se
            // hereda automáticamente vía addClass(parent), así que sólo lo
            // declaramos en el primer eslabón de la cadena que lo necesita.
            boolean declaresSyncMutex = hasOwnSyncProperty(cd) && !ancestorHasSyncProperty(cls);
            if (declaresSyncMutex) {
                // ref-type (apunta a una instancia de Mutex), no owner (los
                // Mutex son pequeños y compartibles si fueran refs externas).
                w.declareField("__syncMutex", true, false);
                if (mod9()) w.setFieldMeta("__syncMutex", "__syncMutex:LMutex;", false);
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
                            boolean isRef = isGcRef(t);   // #14: any/Object también → GC lo traza
                            // BUG-6: long/double = campo de 8 bytes (2 slots).
                            w.declareField(dn.name, isRef, vd.isOwner, occupies8Bytes(t));
                            if (mod9()) w.setFieldMeta(dn.name, claveCampo(dn.name, t), vd.isPrivate);   // V7/H2 (#523)
                        }
                    }
                }
            }

            // Properties PRIMERO (backing field + getter + setter), antes de los
            // métodos normales que pueden invocar getX/setX por su nombre.
            // L6: las static properties NO van aquí (no son fields/métodos de
            // instancia) — se emiten tras endClass como funciones cualificadas.
            for (ITopLevelDecl m : cd.members) {
                if (m instanceof PropertyDef && !((PropertyDef) m).name.isStatic()) {
                    emitPropertyDef(cls, (PropertyDef) m);
                }
            }

            // H5.c — EVENTOS, justo DESPUÉS de las properties y antes de los
            // métodos. El orden importa: tiene que ser EL MISMO que el de
            // computeClassLayout (que también los pone al final de los campos),
            // o el cross-check de #299 aborta la compilación. Ponerlos al final
            // deja intacto el layout de toda clase que no tenga eventos.
            //
            // Dos campos por evento, no uno de 3 slots: declareField sólo sabe
            // de 1 o 2 slots (is8), y partirlo así hace que el bit de GC caiga
            // solo donde debe — en el receptor, no en el destino.
            for (ITopLevelDecl m : cd.members) {
                if (m instanceof EventDef) {
                    String ev = ((EventDef) m).name.name;
                    w.declareField(ev + "$recv", true,  false, true);   // handle 8B, ref
                    w.declareField(ev + "$dest", false, false, false);  // 4B, NO ref
                    if (mod9()) {   // V7/H2 (#523): el evento publica su clave y la ranura del recv
                        Symbol es = cls.instanceMembers.tryLookup(ev);
                        if (es instanceof Symbol.EventSymbol) w.declareEventMeta(claveEvento((Symbol.EventSymbol) es), ev + "$recv");
                        else errors.add("evento '" + ev + "' sin simbolo en " + cd.name);
                    }
                }
            }

            // Pre-pasada: registra los slots de vtable de TODOS los métodos
            // públicos de instancia ANTES de emitir ningún cuerpo, para que una
            // llamada this.m() a un método declarado más abajo (referencia
            // forward) resuelva su slot. Sin esto el orden de emisión obligaba a
            // declarar el callee antes que el caller (bug task_cf2a924b). Las
            // accesoras de property ya se registraron arriba, así que los slots
            // de los métodos normales quedan a continuación, en orden de
            // declaración — layout de vtable idéntico al anterior.
            for (ITopLevelDecl m : cd.members) {
                if (m instanceof FuncDef) {
                    FuncDef fn = (FuncDef) m;
                    FunctionSymbol fs = (FunctionSymbol) info.declSymbols.get(fn);
                    // #390 — la MISMA pregunta que emitInstanceMethod y que
                    // computeClassLayout: si no se declara aquí la ranura de un
                    // método virtual, el layout dice que la tiene y el emisor no la
                    // reserva. Ése fue el fallo de la 1ª pasada.
                    if (fs == null || fs.isStatic || fs.isConstructor || !fs.esVirtual()) continue;
                    w.declareMethodSlot(emitName(fs));   // H5.a-E3: clave de slot
                    if (mod9()) {   // V7/H2 (#523): la clave canónica ANTES de emitir cuerpos (los this.m() la piden)
                        String clave = claveDe(fs);
                        w.setMethodMeta(emitName(fs), clave, esOverride(cls, fs.name, clave), fs.isPrivate);
                    }
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
                // Robustez ABI: además del descriptor, exporta un marker por
                // (método, slot) de la vtable efectiva. El importador valida por
                // nombre que el método sigue en el slot que horneó (ver
                // ModWriter.exportVtableMarkers + el lado importador).
                w.exportVtableMarkers(cd.name);
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

            // 3b) L6 — static properties: accesores como funciones cualificadas
            //     (Cls.__prop_get_x / Cls.__prop_set_x) sobre el backing global
            //     pre-declarado en el paso 1.
            for (ITopLevelDecl m : cd.members) {
                if (m instanceof PropertyDef && ((PropertyDef) m).name.isStatic()) {
                    emitStaticPropertyAccessors(cls, (PropertyDef) m);
                }
            }

            // 4) Si la clase necesita inicializar __syncMutex pero no tiene
            //    constructor de usuario, sintetizamos un __init mínimo que
            //    sólo hace `this.__syncMutex := Mutex()`. Sin esto, las
            //    sync properties operarían sobre un mutex "null" y los
            //    locks petarían.
            if ((declaresSyncMutex || SemanticAnalyzer.tieneInicializadores(cd)) && !hasUserCtor) {
                synthesizeSyncMutexCtor(cls, declaresSyncMutex, cd);   // #511: también por inicializadores
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
                synthesizeCrossModuleInit(cls);
                /* #451 — y una factoria por METODO, por el mismo motivo y con la
                 * misma forma. `super.metodo()` desde otro modulo no tenia a que
                 * llamar: los metodos no se exportan, se llega a ellos por vtable
                 * — y un `super` es justo lo contrario, un salto NO virtual a la
                 * implementacion del padre.
                 *
                 * Lo puso sobre la pista Eduardo: «si declaras una clase que hereda
                 * de otra, aunque no lo escribas, se hace la llamada al constructor
                 * de super». Y esa SI funciona cross-module — porque el ctor tiene
                 * su factoria `__cls_init_<Cls>`. Esto es lo mismo para los demas
                 * metodos. */
                synthesizeCrossModuleMethods(cls);
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
        // V7/H2 (#509/#513) — una factoría por cada constructor EFECTIVO de la clase:
        // la cadena de findConstructor(), que puede ser HEREDADA. Antes se miraba sólo
        // `cls.constructor` (el propio): una derivada sin ctor propio exportaba
        // `__cls_new_Der` con 0 params y sin llamar a ningún __init, y `Mod.Der(7)`
        // compilaba y perdía el 7 en silencio (las dos VMs imprimían 0). Y sólo la
        // cabeza: la 2ª sobrecarga no tenía factoría.
        FunctionSymbol efectivo = cls.findConstructor();
        if (efectivo == null) { synthesizeCrossModuleFactoryDe(cls, null); return; }
        for (FunctionSymbol c = efectivo; c != null; c = c.nextOverload) synthesizeCrossModuleFactoryDe(cls, c);
    }

    private void synthesizeCrossModuleFactoryDe(ClassSymbol cls, FunctionSymbol ctor) throws IOException {
        String factoryName = crossModuleNewName(cls, ctor);
        w.addFunction(factoryName, true);   // pública: cross-module

        // Declarar params iguales al ctor (sin contar `this`).
        if (ctor != null) {
            for (ParamSymbol p : ctor.params) {
                // BUG-6: width-aware — long/double = 8 bytes. Sin esto la
                // factoría cross-module declaraba el param a 4 bytes y un arg
                // long/double se corrompía al pasarlo al __init.
                declareParamByType(p.name, p.type);   // H1.8 (antes: ?8:4 aqui)
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
            declareLocalLong(newref);
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
                for (ParamSymbol p : ctor.params) w.emitGetParam(p.name);
                emitCallInitOf(ctor);   // V7/H2: el __init de la clase que lo declara (local o externa), con su sufijo
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
    /** #451 — una factoria PUBLICA por metodo de instancia, para que un
     *  `super.metodo()` de otro modulo tenga a que llamar. Gemela de
     *  {@link #synthesizeCrossModuleInit}, incluido el detalle que la hace
     *  funcionar: **el nombre no lleva puntos**, asi que el loader lo lee como
     *  simbolo plano de este modulo y no como `modulo.submodulo`.
     *
     *  Solo para clases publicas: son las unicas que otro modulo puede heredar.
     *  Es ADITIVO — anade exports, no mueve ninguno—, asi que un `.mod` ya
     *  compilado sigue valiendo. */
    private void synthesizeCrossModuleMethods(ClassSymbol cls) throws IOException {
        if (cls == null) return;   /* el llamante ya filtra por clase publica */
        for (Symbol sym : cls.instanceMembers.getSymbols()) {
            if (!(sym instanceof FunctionSymbol)) continue;
            // 14-sep — la CADENA entera: getSymbols() sólo da la cabeza, y las
            // sobrecargas 2..N no tenían thunk (un `super.f("a")` cross-module no
            // tenía a qué llamar).
            for (FunctionSymbol fs = (FunctionSymbol) sym; fs != null; fs = fs.nextOverload)
                synthesizeCrossModuleMethod(cls, fs);
        }
    }

    private void synthesizeCrossModuleMethod(ClassSymbol cls, FunctionSymbol fs) throws IOException {
        {
            if (fs.isConstructor || fs.isStatic || !fs.isPublic) return;
            /* SOLO los declarados en ESTA clase. Los heredados (toString,
             * compareTo...) no tienen implementacion local a la que llamar:
             * generarles factoria daba «Funcion no encontrada: Base.toString».
             * Y no hace falta — quien herede de Base y quiera el toString del
             * abuelo sube por la cadena, que es donde vive. */
            if (fs.astNode == null) return;   /* sin cuerpo aqui: es heredado */
            String fname = crossModuleMethodName(cls, fs);
            w.addFunction(fname, true);   // publica: cross-module
            declareParamRef("this");
            for (ParamSymbol p : fs.params) declareParamByType(p.name, p.type);
            FunctionSymbol synthFs = new FunctionSymbol(fname, true, false, false, null, null);
            synthFs.returnType = fs.returnType;
            beginFunctionScope(synthFs, fs.returnType);
            try {
                w.emitGetParam("this");
                for (ParamSymbol p : fs.params) w.emitGetParam(p.name);
                w.emitCall(cls.name + "." + emitName(fs));   // CALL local, NO virtual
                if (fs.returnType != null && !(fs.returnType instanceof BpType.VoidType)) {
                    w.emitSetLocal("__result");
                }
                emitFunctionEnd();
            } finally { scopeStack.pop(); }
        }
    }

    /** Nombre plano de la factoria de un metodo (sin puntos, ver arriba). Por FIRMA
     *  y con '#': ver crossModuleStaticName. */
    static String crossModuleMethodName(ClassSymbol cls, FunctionSymbol fs) {
        return "__cls_m_" + cls.name + "#" + (mod9Estatico() ? Symbol.ClaveCanonica.metodo(fs, moduloLocalActual) : fs.overloadMangle());
    }

    private void synthesizeCrossModuleInit(ClassSymbol cls) throws IOException {
        // V7/H2 (#509) — una por cada sobrecarga: `__cls_init_<Cls>` (cabeza, como
        // hasta hoy: aditivo) y `__cls_init_<Cls>$clave` para las demás. Por la
        // cadena EFECTIVA (findConstructor), igual que la factoría y que la
        // interfaz: si la clase no tiene ctor propio, delega en el de la clase que
        // lo declara (local o de otro módulo) — así un nieto de otro módulo hace
        // `super(...)` sobre esta clase sin importar al abuelo (14-sep).
        for (FunctionSymbol ctor = cls.findConstructor(); ctor != null; ctor = ctor.nextOverload) {
            String initName = crossModuleInitName(cls, ctor);
            w.addFunction(initName, true);   // pública: cross-module
            declareParamRef("this");   // H1.8: receptor = ref (era un 8 a mano)
            for (ParamSymbol p : ctor.params) {
                declareParamByType(p.name, p.type);   // H1.8 (antes: ?8:4 aquí). BUG-6: long/double = 8 bytes
            }
            FunctionSymbol synthFs = new FunctionSymbol(initName, true, false, false, null, null);
            beginFunctionScope(synthFs, null);   // void
            try {
                // Empujamos this + args, luego el __init real (por su clase declarante).
                w.emitGetParam("this");
                for (ParamSymbol p : ctor.params) {
                    w.emitGetParam(p.name);
                }
                emitCallInitOf(ctor);
                emitFunctionEnd();
            } finally { scopeStack.pop(); }
        }
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
        declareLocalLong(newref);
        declareLocal(discard);
        w.emitGetParam("this");
        construirDelLenguaje("Mutex");   // H1 P3: Core.Mutex
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
    private void synthesizeSyncMutexCtor(ClassSymbol cls, boolean conMutex, ClassDef cd) throws IOException {
        // #527/#511 — uno por sobrecarga de la cadena que registró el semántico (una por cada
        // constructor del padre, con sus mismos parámetros; o una sin args): mutex si toca,
        // `super(mis params)` a la sobrecarga del padre que le corresponde, inicializadores.
        for (FunctionSymbol c = cls.constructor; c != null; c = c.nextOverload) {
            if (c.astNode != null) continue;   // de usuario: no es sintético
            w.addFunction(ctorInitName(cls, c), false);
            declareParamRef("this");   // H1.8: receptor = ref (era un 8 a mano)
            declareParamsWidthAware(c);
            beginFunctionScope(c, null);
            try {
                if (conMutex) emitSyncMutexInit(cls.name);
                if (c.implicitSuper && c.implicitSuperCtor != null) {
                    w.emitGetParam("this");
                    for (ParamSymbol p : c.params) w.emitGetParam(p.name);
                    emitCallInitOf(c.implicitSuperCtor);
                    declareLocal("__discardSuper");
                    w.emitSetLocal("__discardSuper");
                }
                if (cd != null) emitInicializadores(cls, cd);   // #511
                emitFunctionEnd();
            } finally { scopeStack.pop(); }
        }
        if (cls.constructor != null) return;
        // (red: si el semántico no registró ninguno, el mínimo de siempre)
        w.addFunction(cls.name + ".__init", false);
        declareParamRef("this");
        beginFunctionScope(makeSynthFs("__init", null), null);
        try {
            if (conMutex) emitSyncMutexInit(cls.name);
            emitFunctionEnd();
        } finally { scopeStack.pop(); }

        // Si no había ctor registrado, registra uno sintético sin params.
        // emitConstruction llamará a Cls.__init y se inicializará __syncMutex.
        // (#527: desde el 19-sep lo registra el semántico; esto queda de red.)
        if (cls.constructor == null) {
            FunctionSymbol synthCtor = new FunctionSymbol(cls.name, true, false, false, cls, null);
            synthCtor.isConstructor = true;
            cls.constructor = synthCtor;
            // No lo metemos en instanceMembers porque el analizador ya
            // validó las llamadas (sin ctor → sin args). Sólo necesitamos
            // que findConstructor lo encuentre.
        }
    }

    /** #527 (b) — abre la GUARDA de un accesor `sync`: un handler que atrapa CUALQUIER
     *  excepción que salga del cuerpo (nacida en una llamada, un fallo de ejecución o un
     *  `throw`), suelta el mutex y la relanza. Antes sólo el `throw` escrito en el propio
     *  accesor soltaba el cerrojo (emitThrowStmt) y el siguiente lock decía «re-entrada».
     *  Devuelve la etiqueta del handler; {@link #cerrarGuardaSync} la cierra. El `return`
     *  interno ya pasa por tryStack (TRY_END) y por el unlock de FuncScope. */
    private int abrirGuardaSync() throws IOException {
        int handler = w.newLabel();
        w.emitTryBegin(handler, null);   // null = cualquier excepción
        tryStack.push(new TryFrame(null, 1));
        return handler;
    }
    /** El cierre de {@link #abrirGuardaSync}: fall-through (pop del handler, unlock, salto
     *  al final) y el handler (guardar la excepción, unlock, relanzar). `className` null =
     *  property de MÓDULO (mutex global). */
    private void cerrarGuardaSync(int handler, String className) throws IOException {
        tryStack.pop();
        w.emitTryEnd();
        if (className != null) emitSyncUnlock(className); else emitModuleSyncUnlock();
        int fin = w.newLabel();
        w.emitJump(fin);
        w.declareLabel(handler);
        declareLocalLong("__sync_exc");   // la excepción es una REF de 8 B (hallazgo 34)
        w.emitSetLocal("__sync_exc");
        if (className != null) emitSyncUnlock(className); else emitModuleSyncUnlock();
        w.emitGetLocal("__sync_exc");
        w.emitThrow();
        w.declareLabel(fin);
    }

    /** Helpers lock/unlock contra this.__syncMutex (resuelto vía className). */
    private void emitSyncLock(String className) throws IOException {
        int id = tempCounter++;
        String discard = "__syncLock_discard_" + id;
        declareLocal(discard);
        w.emitGetParam("this");
        w.emitGetField(className, "__syncMutex");
        invocarDelLenguaje("Mutex", "lock", 0);
        w.emitSetLocal(discard);
    }
    private void emitSyncUnlock(String className) throws IOException {
        int id = tempCounter++;
        String discard = "__syncUnlock_discard_" + id;
        declareLocal(discard);
        w.emitGetParam("this");
        w.emitGetField(className, "__syncMutex");
        invocarDelLenguaje("Mutex", "unlock", 0);
        w.emitSetLocal(discard);
    }

    private void emitInstanceMethod(FuncDef fn, FunctionSymbol fs) throws IOException {
        // BUG-4 / Opción A: solo los métodos PÚBLICOS van a la vtable (despacho
        // virtual, sobreescribibles, visibles cross-module). Los PRIVADOS se
        // despachan por CALL directo (ver el call-site) → fuera de la vtable,
        // así su orden coincide con el .bpi y no hay desfase de slots.
        // H5.c — `function event` también entra en la vtable aunque sea privada:
        // el destino de un evento ES un slot, así que sin slot no hay a dónde
        // apuntar. Sigue sin ser llamable POR SU NOMBRE desde fuera (eso lo
        // guarda el semántico) — es la misma idea que ya valía para las
        // properties privadas, cuyos accesores llevan años en la vtable.
        // La condición tiene que ser LA MISMA que en computeClassLayout o el
        // cross-check de #299 aborta.
        /* [V6/N1.1] Un metodo `native` EXPORTA su nombre; los demas no, como
         * siempre. El registro AOT busca el simbolo en la tabla de exportacion
         * para sacar su direccion — sin nombre no hay a que engancharse, aunque
         * el mecanismo (secuestro por direccion en OP_CALL) sirva igual para
         * funciones y metodos. */
        boolean exportarParaAot = fn.isNative && !fn.isIntrinsic;
        if (fs.esVirtual()) {
            w.addMethod(emitName(fs), exportarParaAot);          // vtable + función; declara "this"
        } else {
            w.addPrivateMethod(emitName(fs), exportarParaAot);   // solo función llamable; declara "this"
        }
        declareParamsWidthAware(fs);
        beginFunctionScope(fs, null);
        scopeStack.peek().encadenarDtorDelPadre = fs.isDestructor;   // V7/H1: ~Hija() acaba llamando a ~Padre()
        abrirGuardaDeOwners(fn.body);   // V7/H1
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
        w.addFunction(ctorInitName(cls, fs), false);   // H5.a-E4: sufijo si sobrecargado
        declareParamRef("this");   // H1.8: receptor = ref (era un 8 a mano)
        declareParamsWidthAware(fs);
        beginFunctionScope(fs, null);   // constructor = void
        abrirGuardaDeOwners(fn.body);   // V7/H1
        try {
            // Si esta clase introduce __syncMutex, hay que inicializarlo
            // ANTES de cualquier código de usuario para que cualquier acceso
            // a una sync property dentro del propio constructor encuentre el
            // mutex listo. La búsqueda de "introduce __syncMutex" replica el
            // criterio usado en emitClassDef. Si el field ya está heredado,
            // el __init del padre lo habrá inicializado vía super().
            ClassDef cd = (fs.ownerClass != null && fs.ownerClass.decl instanceof ClassDef)
                    ? (ClassDef) fs.ownerClass.decl : null;
            // #511 (19-sep) — DELEGACIÓN: `MiClase(args)` de primera sentencia inicializa ESTE
            // objeto por esa otra sobrecarga; ni mutex, ni super(), ni inicializadores aquí
            // (los hace el delegado). El resto del cuerpo, después.
            if (fs.delegaA != null) {
                CallExpr d = (CallExpr) ((ExprStmt) fn.body.get(0)).expr;
                w.emitGetParam("this");
                for (int i = 0; i < d.args.size(); i++) {
                    emitExpr(d.args.get(i));
                    if (i < fs.delegaA.params.size()) coerceToTarget(d.args.get(i), fs.delegaA.params.get(i).type);
                }
                emitCallInitOf(fs.delegaA);
                declareLocal("__discardDelega");
                w.emitSetLocal("__discardDelega");
                for (int i = 1; i < fn.body.size(); i++) emitStmt(fn.body.get(i));
                emitFunctionEnd();
                return;
            }
            if (cd != null && hasOwnSyncProperty(cd) && !ancestorHasSyncProperty(cls)) {
                emitSyncMutexInit(cls.name);
            }
            // Auto-super (modelo Java): si el usuario no escribió super(), el
            // compilador inyecta aquí la llamada al constructor del padre SIN
            // argumentos (lo marcó el semántico). Va antes del cuerpo → el padre
            // queda inicializado primero, igual que un super() escrito a mano.
            if (fs.implicitSuper) emitImplicitSuperCall(cls, fs);
            // #511 — los inicializadores de campos y properties de instancia, UNA vez, tras el
            // super() (escrito: justo después de esa primera sentencia; inyectado: aquí) y antes
            // del cuerpo, en orden de declaración. Antes el `:= valor` de un campo se aceptaba y
            // se tiraba en silencio (x=0).
            boolean superEscrito = !fn.body.isEmpty() && fn.body.get(0) instanceof ExprStmt
                    && ((ExprStmt) fn.body.get(0)).expr instanceof SuperCallExpr;
            if (!superEscrito && cd != null) emitInicializadores(cls, cd);
            for (int i = 0; i < fn.body.size(); i++) {
                emitStmt(fn.body.get(i));
                if (i == 0 && superEscrito && cd != null) emitInicializadores(cls, cd);
            }
            emitFunctionEnd();
        } finally { scopeStack.pop(); }
    }

    /** #511 — `this.<campo> := valor` por cada campo/property de instancia con `:= valor`, en
     *  orden de declaración. Un array de tamaño fijo como campo no se inicializa aquí. */
    private void emitInicializadores(ClassSymbol cls, ClassDef cd) throws IOException {
        for (ITopLevelDecl m : cd.members) {
            if (m instanceof VarDecl) {
                VarDecl vd = (VarDecl) m;
                if (vd.init == null || vd.names.isEmpty() || vd.names.get(0).isStatic()) continue;
                Symbol s0 = info.declSymbols.get(vd);
                if (s0 instanceof VarSymbol && ((VarSymbol) s0).fixedArraySize >= 0) continue;
                BpType t = (s0 instanceof VarSymbol) ? ((VarSymbol) s0).type : null;
                for (DeclName n : vd.names) {
                    w.emitGetParam("this");
                    emitExpr(vd.init);
                    coerceToTarget(vd.init, t);
                    emitSetFieldAuto(cls.name, n.name);
                }
            } else if (m instanceof PropertyDef) {
                PropertyDef pd = (PropertyDef) m;
                if (pd.init == null || pd.name.isStatic()) continue;
                Symbol s0 = info.declSymbols.get(pd);
                BpType t = (s0 instanceof PropertySymbol) ? ((PropertySymbol) s0).type : null;
                w.emitGetParam("this");
                emitExpr(pd.init);
                coerceToTarget(pd.init, t);
                emitSetFieldAuto(cls.name, pd.name.name);   // el backing, como un campo (Java)
            }
        }
    }

    /** Auto-super: emite la llamada implícita al constructor del padre (sin
     *  argumentos) — equivalente a un super() escrito por el usuario. El semántico
     *  ya garantizó que el padre tiene un constructor invocable sin args. Mismo
     *  camino que emitSuperCall (CALL local / CALL_EXT a __cls_init_X si external),
     *  pero sin empujar argumentos; descarta el valor de retorno del __init. */
    private void emitImplicitSuperCall(ClassSymbol cls, FunctionSymbol ctorFs) throws IOException {
        ClassSymbol parent = (cls != null) ? cls.baseClass : null;
        if (parent == null || parent.findConstructor() == null) return;
        // V7/H2 (#509/#513) — la sobrecarga sin args que eligió el semántico (puede
        // no ser la cabeza), llamada por la clase que la DECLARA: si el padre no
        // tiene ctor propio, es la del abuelo, y ése puede vivir en otro módulo.
        FunctionSymbol elegido = (ctorFs != null && ctorFs.implicitSuperCtor != null)
                ? ctorFs.implicitSuperCtor : parent.findConstructor();
        w.emitGetParam("this");
        emitCallInitOf(elegido);
        declareLocal("__discardSuper");
        w.emitSetLocal("__discardSuper");
    }

    /** H5.a — nombre con el que esta función se registra en el .mod y se invoca:
     *  si participa en un conjunto de sobrecargas, se manglea por firma para que
     *  las tablas por-nombre del ModWriter no colisionen; si no, el nombre pelado.
     *  DEFINICIÓN y LLAMADA deben usar SIEMPRE este mismo helper. */
    static String emitName(FunctionSymbol fs) {
        return fs.slotKey();
    }

    /** H5.a-E4 — nombre del `__init` de un constructor. Los ctors NO van a la
     *  vtable (CALL directo), así que basta distinguirlos por nombre: el PRIMERO
     *  se queda con `<Cls>.__init` y las sobrecargas añaden el sufijo de su clave
     *  (`<Cls>.__init$dd`). Al ser aditivo, un `.mod` ya compilado con un solo
     *  constructor no cambia de símbolo. */
    static String ctorInitName(ClassSymbol cls, FunctionSymbol ctor) {
        return cls.name + ".__init" + ctorSuffix(ctor);
    }

    /** V7/H2 (#509) — el sufijo de clave de un constructor: "" para la cabeza,
     *  "$dd" para las sobrecargas. UN helper para `<Cls>.__init`, `__cls_init_<Cls>`
     *  y `__cls_new_<Cls>` a los dos lados de la frontera. */
    static String ctorSuffix(FunctionSymbol ctor) {
        if (ctor == null || ctor.slotKey().equals(ctor.name)) return "";
        return ctor.slotKey().substring(ctor.name.length());
    }
    static String crossModuleInitName(ClassSymbol cls, FunctionSymbol ctor) { return "__cls_init_" + cls.name + (mod9Estatico() ? firmaParams(ctor) : ctorSuffix(ctor)); }
    static String crossModuleNewName(ClassSymbol cls, FunctionSymbol ctor)  { return "__cls_new_" + cls.name + (mod9Estatico() ? firmaParams(ctor) : ctorSuffix(ctor)); }

    /** V7/H2 (#509/#513) — `this` (+ args) ya en pila: llama al `__init` del
     *  constructor `ctor` por la clase que lo DECLARA (puede ser una base, y puede
     *  vivir en otro módulo: entonces CALL_EXT a su `__cls_init_<Owner>$s`). */
    private void emitCallInitOf(FunctionSymbol ctor) throws IOException {
        ClassSymbol owner = ctor.ownerClass;
        if (owner != null && owner.isExternal) {
            String qn = qualifyForOwner(owner, crossModuleInitName(owner, ctor));
            w.emitCallExt(qn, fromPathFor(qn));
        } else {
            w.emitCall(ctorInitName(owner, ctor));
        }
    }

    private void emitStaticMethodFn(ClassSymbol cls, FuncDef fn, FunctionSymbol fs) throws IOException {
        w.addFunction(cls.name + "." + emitName(fs), fs.isPublic);
        // V7/H2 (#492-B) — alias plano importable, para todo lo que la interfaz
        // publica (clase pública, método no private; lo protegido lo llama una
        // hija de otro módulo y checkVisibility ya decide quién puede).
        if (cls.astNode != null && cls.astNode.isPublic && !fs.isPrivate)
            w.addExportAlias(crossModuleStaticName(cls, fs), cls.name + "." + emitName(fs));
        declareParamsWidthAware(fs);
        beginFunctionScope(fs, null);
        abrirGuardaDeOwners(fn.body);   // V7/H1
        try {
            for (IStmt s : fn.body) emitStmt(s);
            emitFunctionEnd();
        } finally { scopeStack.pop(); }
    }

    private void emitPropertyDef(ClassSymbol cls, PropertyDef pd) throws IOException {
        PropertySymbol ps = (PropertySymbol) info.declSymbols.get(pd);
        if (ps != null && ps.isStatic) {
            // L6: las static properties se emiten en emitStaticPropertyAccessors
            // (tras endClass) — el loop de properties de instancia ya las filtra.
            return;
        }
        BpType propType = (ps != null) ? ps.type : (pd.type != null ? typeRefToBpType(pd.type) : null);
        boolean isRef = isGcRef(propType);   // #14: any/Object también → GC lo traza

        // 1) Backing field (con flag isOwner si la propiedad es owner; el VM
        //    libera recursivamente este campo cuando la instancia se destruye).
        //    BUG-6: long/double = campo de 8 bytes (2 slots).
        w.declareField(pd.name.name, isRef, pd.isOwner, occupies8Bytes(propType));
        // V7/H2 (#523) — el backing es de la clase y de nadie más (se accede por los
        // accesores): viaja marcado privado. Los accesores, con su clave `getX()t`/`setX(t)V`.
        boolean propOverride = mod9() && esOverrideProperty(cls, pd.name.name);
        if (mod9()) w.setFieldMeta(pd.name.name, claveCampo(pd.name.name, propType), true);

        // 2) Getter (custom o auto). Devuelve el tipo de la property.
        //    Si la property es sync, envolvemos cuerpo con lock/unlock contra
        //    this.__syncMutex. Un return interno en el body custom emitirá
        //    el unlock antes del JUMP al endLabel (B2) — el unlock del
        //    fall-through al final del body no se duplica porque el return
        //    JUMPea por encima de él.
        String getterSimple = "get" + capitalize(pd.name.name);
        w.addMethod(getterSimple);
        if (mod9()) w.setMethodMeta(getterSimple, claveGetter(pd.name.name, propType), propOverride, pd.isPrivate);
        FunctionSymbol getterFs = new FunctionSymbol(getterSimple, pd.isPublic, false, false, cls, null);
        getterFs.returnType = propType;
        beginFunctionScope(getterFs, propType);
        currentPropertyField = pd.name.name;
        if (pd.isSync) scopeStack.peek().syncClassName = cls.name;
        try {
            int guarda = -1;
            if (pd.isSync) { emitSyncLock(cls.name); guarda = abrirGuardaSync(); }
            if (pd.getter != null) {
                for (IStmt s : pd.getter.body) emitStmt(s);
            } else {
                // auto-getter: result := this.<name>
                w.emitGetParam("this");
                w.emitGetField(cls.name, pd.name.name);
                w.emitSetLocal("__result");
            }
            if (pd.isSync) cerrarGuardaSync(guarda, cls.name);   // #527 (b)
            emitFunctionEnd();
        } finally { currentPropertyField = null; scopeStack.pop(); }

        // 3) Setter (custom o auto). Es void.
        String setterSimple = "set" + capitalize(pd.name.name);
        w.addMethod(setterSimple);
        if (mod9()) w.setMethodMeta(setterSimple, claveSetter(pd.name.name, propType), propOverride, pd.isPrivate);
        // BUG-6: el valor de una property long/double ocupa 8 bytes en el param.
        declareParamByType("__val", propType);   // H1.8 (antes: ?8:4 aquí)
        FunctionSymbol setterFs = new FunctionSymbol(setterSimple, pd.isPublic, false, false, cls, null);
        beginFunctionScope(setterFs, null);
        currentPropertyField = pd.name.name;
        if (pd.isSync) scopeStack.peek().syncClassName = cls.name;
        try {
            int guarda = -1;
            if (pd.isSync) { emitSyncLock(cls.name); guarda = abrirGuardaSync(); }
            if (pd.setter != null) {
                // Alias el param al nombre que el usuario escribió (BUG-6: 8 bytes si long/double).
                declareLocalByType(pd.setter.paramName, propType, null);   // H1.8
                w.emitGetParam("__val");
                w.emitSetLocal(pd.setter.paramName);
                for (IStmt s : pd.setter.body) emitStmt(s);
            } else {
                // auto-setter: this.<name> := __val
                w.emitGetParam("this");
                w.emitGetParam("__val");
                emitSetFieldAuto(cls.name, pd.name.name);
            }
            if (pd.isSync) cerrarGuardaSync(guarda, cls.name);   // #527 (b)
            emitFunctionEnd();
        } finally { currentPropertyField = null; scopeStack.pop(); }
    }

    /**
     * L6 — Accesores de una static property de clase, emitidos como funciones
     * cualificadas (`Cls.__prop_get_x` / `Cls.__prop_set_x`) DESPUÉS de endClass,
     * igual que los métodos estáticos. El backing es el global cualificado
     * `Cls.__prop_x` (pre-declarado width-aware en el paso 1 de emitClassDef,
     * junto a las static vars). Dentro de un accesor custom, `field` resuelve al
     * backing global (reusa currentModulePropertyBacking, el mecanismo de las
     * properties de módulo). El init (si lo hay) se ignora como en módulo (L8):
     * el global arranca a 0/null.
     *
     * Limitación v1: `sync static property` no soportada (error claro) — el mutex
     * compartido de módulo no se inicializa para clases, y el de instancia no
     * existe sin `this`.
     */
    private void emitStaticPropertyAccessors(ClassSymbol cls, PropertyDef pd) {
        try {
            if (pd.isSync) {
                errors.add("sync static property no soportada todavía: "
                        + cls.name + "." + pd.name.name);
                return;
            }
            PropertySymbol psym = (PropertySymbol) info.declSymbols.get(pd);
            BpType propType = (psym != null && psym.type != null) ? psym.type
                            : (pd.type != null ? typeRefToBpType(pd.type) : null);
            boolean p8 = occupies8Bytes(propType);

            String backing = staticBackingName(cls.name, pd.name.name);
            String getter  = staticGetterName(cls.name, pd.name.name);
            String setter  = staticSetterName(cls.name, pd.name.name);
            // V7/H2 (#492-B) — alias planos importables (ver crossModuleStaticName).
            if (cls.astNode != null && cls.astNode.isPublic && !pd.isPrivate) {
                w.addExportAlias(crossModuleStaticGetter(cls.name, pd.name.name, propType), getter);
                w.addExportAlias(crossModuleStaticSetter(cls.name, pd.name.name, propType), setter);
            }

            // ---- Getter ----
            w.addFunction(getter, pd.isPublic);
            FunctionSymbol gfs = new FunctionSymbol(getter, pd.isPublic, false, false, null, null);
            gfs.returnType = propType;
            beginFunctionScope(gfs, propType);
            currentModulePropertyBacking = backing;
            try {
                if (pd.getter != null) {
                    for (IStmt s : pd.getter.body) emitStmt(s);
                } else {
                    // auto-getter: __result := Cls.__prop_<name>
                    w.emitGetGlobal(backing);
                    w.emitSetLocal("__result");
                }
                emitFunctionEnd();
            } finally { currentModulePropertyBacking = null; scopeStack.pop(); }

            // ---- Setter ----
            w.addFunction(setter, pd.isPublic);
            w.declareParam("__val", p8 ? 8 : 4);
            FunctionSymbol sfs = new FunctionSymbol(setter, pd.isPublic, false, false, null, null);
            beginFunctionScope(sfs, null);
            currentModulePropertyBacking = backing;
            try {
                if (pd.setter != null) {
                    if (p8) declareLocalLong(pd.setter.paramName);
                    else declareLocal(pd.setter.paramName);
                    w.emitGetParam("__val");
                    w.emitSetLocal(pd.setter.paramName);
                    for (IStmt s : pd.setter.body) emitStmt(s);
                } else {
                    // auto-setter: Cls.__prop_<name> := __val
                    w.emitGetParam("__val");
                    w.emitSetGlobal(backing);
                }
                emitFunctionEnd();
            } finally { currentModulePropertyBacking = null; scopeStack.pop(); }
        } catch (IOException e) {
            errors.add("error emitiendo static property '" + cls.name + "."
                    + pd.name.name + "': " + e.getMessage());
        }
    }

    private static String capitalize(String s) {
        if (s == null || s.isEmpty()) return s;
        return Character.toUpperCase(s.charAt(0)) + s.substring(1);
    }

    /** ¿`t` es una REFERENCIA (handle de heap, REF_SIZE=8)?
     *
     *  H1.8 (Eduardo, repaso de tipos): **la fuente de verdad es el TIPO**, no una lista
     *  de `instanceof` escrita aquí a mano. Antes esto enumeraba ClassType/ArrayType/
     *  TupleType/string, y esa lista es el TERCER modo de fallo de toda la saga 4→8B: si
     *  le falta un tipo-ref, fallan a la vez TODOS los sitios que preguntan el ancho.
     *
     *  NO es teórico — es exactamente lo que pasó con las TUPLAS (#281):
     *    · 02-jun (c833d85) nacen las tuplas, con `TupleType.isReference() = true`.
     *    · 13-jul (3ff52b0) se caza el use-after-free y se arregla añadiendo TupleType
     *      a la lista de aquí.
     *  El tipo llevaba SEIS SEMANAS diciendo que era una referencia. Este método nunca se
     *  lo preguntó. El fix de #281 parcheó la reimplementación en vez de eliminarla.
     *
     *  Y dejaba a `UnresolvedClassRef` (isReference()=true desde siempre) fuera → una
     *  clase sin resolver se declaraba a 4B: el mismo bug esperando su turno.
     *
     *  Ahora un tipo NUEVO que declare `isReference()=true` queda cubierto SOLO. */
    static boolean isRefType(BpType t) {
        if (t == null) return false;
        if (t.isReference()) return true;   // ← el tipo manda (Class/Array/Tuple/Any/UnresolvedClassRef)
        // ÚNICA excepción: `string` ES una ref de heap, pero PrimitiveType NO sobreescribe
        // isReference() → dice false. NO se toca ahí porque isReference() lo consume el
        // SEMÁNTICO para la asignabilidad de `null` (SemanticAnalyzer:2613-2614) y
        // ModuleInterface:533: cambiarlo mueve la semántica del lenguaje, no el ancho.
        // Alinear string en isReference() es otra tarea, con sus propias pruebas.
        if (t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.STRING) return true;
        return false;
    }

    /** #14 — ¿el GC debe TRAZAR este campo/elemento como ref? Igual que isRefType
     *  PERO incluyendo `any`/Object: un campo `any` puede CONTENER una ref (además
     *  de un número). El marcado preciso del GC lo traza de forma conservadora
     *  (deref + valida contra el mapa de cabeceras; un número se salta, idéntico a
     *  un slot de un Object[]=ARRAY_REF). Sin este bit, un objeto cuya ÚNICA ref
     *  viva está en un campo `any` (el caso que lo destapó fue Component.__win de
     *  Forms, retirado en #324 tanda 2c; el bit sigue haciendo falta para
     *  cualquier otro campo `any`) se recolecta →
     *  use-after-free (hoy enmascarado por el scan conservador de pila; ver
     *  docs/V4_REF_AUDIT.md #14). Feed SOLO del bitmap de refs del GC — NO del ancho
     *  (el ancho de `any` ya lo cubre occupies8Bytes). */
    static boolean isGcRef(BpType t) {
        return isRefType(t) || (t instanceof BpType.AnyType);
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
        if (s instanceof Ast.RaiseStmt) { emitRaise((Ast.RaiseStmt) s); return; }
        if (s instanceof AssignStmt)  emitAssign((AssignStmt) s);
        else if (s instanceof Ast.DestructAssignStmt) emitDestructAssign((Ast.DestructAssignStmt) s);
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
        String declTag = varDbgTypeTag(declT);               // H6.a.2: tag para el .dbg
        // L8 v3 — local `tipo[N]`: array HEAP de N elementos. NEWARRAY_* deja
        // los elementos a cero (el alloc zero-inicializa en ambas VMs), así
        // que un init literal parcial solo escribe sus k primeros elementos.
        // El semántico ya validó N, el tipo de elemento y el literal, y
        // bloquea reasignar la var (binding fija, como el de módulo).
        Integer fixedN = localFixedArraySize(vd);
        if (fixedN != null) {
            // OJO: typeRefToBpType solo resuelve SimpleTypeRef (devuelve null
            // para arrays) — el tipo de ELEMENTO sale del VarSymbol que ya
            // resolvió el semántico.
            Symbol ds = info.declSymbols.get(vd);
            BpType vt = (ds instanceof VarSymbol) ? ((VarSymbol) ds).type : declT;
            BpType el = (vt instanceof ArrayType) ? ((ArrayType) vt).element : null;
            java.util.List<IExpr> elems = (vd.init instanceof ArrayLitExpr)
                    ? ((ArrayLitExpr) vd.init).elements
                    : java.util.Collections.<IExpr>emptyList();
            for (DeclName dn : vd.names) {
                // H1.8/#19 — REGRESIÓN del ensanchado 4→8B, restaurada. Era
                // `declareLocal(dn.name, declTag)` = 4B A PELO, ignorando `vt` (que está
                // resuelto 6 líneas más arriba): la var ES el array = un ArrayType = una
                // REFERENCIA de 8B. NEWARRAY empuja un handle de 8B y SET_LOCAL popeaba 4
                // → handle TRUNCADO (gen perdida) + fuga de 4B de pila → use-after-free.
                //
                // Esta rama hace `return` ANTES del carril centralizado de H1.8, así que
                // la centralización no la cubrió: hay que traerle el carril aquí.
                //
                // Lo delató `samples/LocalArrTest.bp` — el oráculo de L8 v3, que documenta
                // la paridad esperada byte a byte y llevaba ROTO desde el 4→8B sin que
                // nadie lo oyera (es un .bp suelto, no está en ninguna batería `make`).
                declareLocalByType(dn.name, vt, declTag);
                emitInt(fixedN);
                w.emit(newarrayOpForElement(el));
                w.emitSetLocal(dn.name);
                for (int i = 0; i < elems.size(); i++) {
                    IExpr e = elems.get(i);
                    w.emitGetLocal(dn.name);
                    emitInt(i);
                    emitExpr(e);
                    coerceToTarget(e, el);                 // p.ej. 1 → 1L en long[N]
                    w.emit(astoreOpForElement(el));
                }
            }
            return;
        }
        // H1.2a: declT (typeRefToBpType) es null para refs (clase/array) → resolvemos el
        // tipo semántico REAL del Symbol para decidir el ancho, igual que declareField.
        Symbol dsW = info.declSymbols.get(vd);
        BpType widthT = (dsW instanceof VarSymbol) ? ((VarSymbol) dsW).type : declT;
        for (DeclName dn : vd.names) {
            // H1.8: el ancho lo decide el TIPO (widthT). declTag va aparte A PROPOSITO:
            // declT es null para refs, asi que el .dbg las etiqueta "?" — mejorable, pero
            // es otra tarea; aqui el .dbg sale byte-identico.
            declareLocalByType(dn.name, widthT, declTag);   // H1.2/H1.3/H1.2a: long/double/ref = 8 bytes
            if (vd.isOwner) {
                scopeStack.peek().ownerLocals.add(dn.name);
                tiposDeOwners.put(dn.name, widthT);   // V7/H1: para llamar a su destructor al morir
            }
            if (vd.init != null) {
                emitExpr(vd.init);
                // V7/H2 (#523, A1): declT es null para una clase → el tipo real (widthT), para que
                // `var b: Lib.Base := h` anote la asunción SUBTIPO. Para primitivos, el de siempre.
                coerceToTarget(vd.init, declT != null ? declT : widthT);
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

    /** V7/H2 (#492-B) — lee/escribe el campo `v` sobre la referencia que hay en la pila.
     *  Si la clase que lo DECLARA es de otro módulo, el ModWriter no la conoce por nombre
     *  («Clase no declarada»): se va por la ranura que publicó su interfaz. Si es local,
     *  por nombre, como siempre. Es el gemelo de emitInvokeVirtualSmart para campos. */
    /** V10 (16-sep, EC-1/E-R2-1) — la clase que va en la ref: la ESTÁTICA del receptor si es externa o
     *  abierta (como en los métodos: así el cargador resuelve por la cadena REAL y un `Derived` que ya no
     *  extiende `Base` da E3, no la ranura de otro miembro); si no, la que declara el miembro. */
    private ClassSymbol claseDeLaRef(ClassSymbol receptor, ClassSymbol declarante) {
        return (receptor != null && porRef(receptor)) ? receptor : declarante;
    }
    private void emitGetFieldDe(VarSymbol v) throws IOException { emitGetFieldDe(v, null); }
    private void emitSetFieldDe(VarSymbol v) throws IOException { emitSetFieldDe(v, null); }
    private void emitGetFieldDe(VarSymbol v, ClassSymbol receptor) throws IOException {
        ClassSymbol c = claseDeLaRef(receptor, v.ownerClass);
        if (mod9() && c != null && porRef(c)) {   // V7/H2 (#523): por referencia
            w.emitGetFieldRef(refCampo(c, claveCampo(v.name, v.type)), occupies8Bytes(v.type));
            return;
        }
        if (v.ownerClass != null && v.ownerClass.isExternal) {
            if (v.externalSlot < 0) { errors.add("campo '" + v.name + "' de la clase externa '" + v.ownerClass.name + "' sin ranura publicada: recompila el módulo dueño (interfaz anterior a bpi 8)"); return; }
            w.emitGetFieldBySlot(v.externalSlot, occupies8Bytes(v.type));
        } else {
            w.emitGetField(v.ownerClass.name, v.name);
        }
    }
    private void emitSetFieldDe(VarSymbol v, ClassSymbol receptor) throws IOException {
        ClassSymbol c = claseDeLaRef(receptor, v.ownerClass);
        if (mod9() && c != null && porRef(c)) {   // V7/H2 (#523): por referencia
            int r = refCampo(c, claveCampo(v.name, v.type));
            if (v.isOwner) w.emitSetFieldOwnerRef(r); else w.emitSetFieldRef(r, occupies8Bytes(v.type));
            return;
        }
        if (v.ownerClass != null && v.ownerClass.isExternal) {
            if (v.externalSlot < 0) { errors.add("campo '" + v.name + "' de la clase externa '" + v.ownerClass.name + "' sin ranura publicada: recompila el módulo dueño (interfaz anterior a bpi 8)"); return; }
            if (v.isOwner) w.emitSetFieldOwnerBySlot(v.externalSlot);
            else           w.emitSetFieldBySlot(v.externalSlot, occupies8Bytes(v.type));
        } else {
            emitSetFieldAuto(v.ownerClass.name, v.name);
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
        // #292d: un owner local es SIEMPRE ref = 8 bytes. Anularlo con emitInt(0)
        // empujaba sólo 4 bytes, pero emitSetLocal ve el slot de 8B y emite
        // SET_LOCAL_L (saca 8) → underflow de 4 bytes → la fuente quedaba
        // [basura|0] (a==null daba false) y la PILA quedaba descuadrada 4 bytes,
        // reventando el siguiente INVOKE_VIRTUAL ("slot no resoluble"). Misma
        // familia que #292b: cero de 8 BYTES.
        w.emit(OpCode.LPUSH); w.emitLong(0L);
        w.emitSetLocal(v.name);
    }

    private void emitLocalConstDecl(ConstDecl cd) throws IOException {
        Symbol s = info.declSymbols.get(cd);
        ConstSymbol cs = (s instanceof ConstSymbol) ? (ConstSymbol) s : null;
        // Literal: las lecturas lo inlinan (literalValue) — ni slot ni store.
        if (cs != null && cs.literalValue != null) return;
        // No-literal: slot local de única asignación, width-aware (un long en
        // slot de 4 bytes desbalancearía la pila en el SET_LOCAL).
        BpType t = (cs != null && cs.type != null) ? cs.type : info.exprTypes.get(cd.value);
        declareLocalByType(cd.name.name, t);   // H1.8
        emitExpr(cd.value);
        coerceToTarget(cd.value, t);
        w.emitSetLocal(cd.name.name);
    }

    private void declareLocal(String name) { declareLocal(name, (String) null); }

    // H6.a.2: variante con tag de tipo BP para el .dbg.
    private void declareLocal(String name, String typeTag) {
        FuncScope scope = scopeStack.peek();
        if (scope.locals.add(name)) {
            w.declareLocal(name, typeTag);
        }
    }

    private void declareLocalLong(String name) { declareLocalLong(name, (String) null); }

    private void declareLocalLong(String name, String typeTag) {   // H1.2 (V2): local de 8 bytes
        FuncScope scope = scopeStack.peek();
        if (scope.locals.add(name)) {
            w.declareLocalLong(name, typeTag);
        }
    }

    // H1.2 (V2): declara los params de fs con ancho por tipo (long = 8 bytes).
    // H6.a.2: además pasa el tag de tipo BP para el .dbg.
    private void declareParamsWidthAware(FunctionSymbol fs) {
        for (ParamSymbol p : fs.params) {
            declareParamByType(p.name, p.type);   // H1.8 (antes: ?8:4 aqui)
        }
    }

    /** Ref-abstracción (Eduardo): declara UN param sintético por su TIPO, no por un
     *  ancho puesto a mano. Una REFERENCIA (string/objeto/array/any) toma el carril de
     *  8B automáticamente vía occupies8Bytes — y si cambia la repr de ref, se adapta
     *  solo. Gemelo mono-param de declareParamsWidthAware, para los métodos
     *  SINTETIZADOS que declaran sus params a mano (StringBuilder, Object, properties). */
    private void declareParamByType(String name, BpType type) {
        w.declareParam(name, occupies8Bytes(type) ? 8 : 4, varDbgTypeTag(type));
    }

    /** H1.8 (Eduardo) — gemelo de declareParamByType para los GLOBALES. Declara por el
     *  TIPO, no por un ancho puesto a mano: una REFERENCIA toma el carril de 8B sola.
     *
     *  Motivo: el ancho de los globales se decidía en cada sitio eligiendo el método
     *  (declareGlobal=4B vs declareGlobalLong=8B) y el patrón "se migró un camino y el
     *  gemelo se quedó atrás" REINCIDIÓ 3 veces (#9 __strequals, #10 var-string de
     *  módulo, #16 backing de property de módulo). Un global-ref de 4B trunca el handle
     *  (pierde la GENERACIÓN de la palabra alta) y además FUGA 4B de pila, porque el
     *  valor se empuja a 8: el bug no se ve hasta que el slot viene reciclado (gen>0),
     *  y entonces sale como use-after-free lejísimos de aquí.
     *
     *  type==null → no se puede decidir → 4B (el conservador de siempre). Lo que es ref
     *  POR CONSTRUCCIÓN (sintéticos) usa declareGlobalRef. */
    private void declareGlobalByType(String name, BpType type) {
        if (type != null && occupies8Bytes(type)) w.declareGlobalLong(name);
        else w.declareGlobal(name);
    }

    /** H1.8 — global que es una REFERENCIA por construcción (sintéticos, sin BpType a
     *  mano: el Mutex de las sync property de módulo). Una ref SIEMPRE es REF_SIZE=8. */
    private void declareGlobalRef(String name) {
        w.declareGlobalLong(name);
    }

    /** H1.8 — gemelo para los LOCALES. Colapsa el `if (occupies8Bytes(t)) declareLocalLong
     *  else declareLocal` que estaba REPETIDO en 6 sitios: la decisión de ancho vive en UN
     *  sitio y sale del tipo. (El fallo de las tuplas #281 fue del PREDICADO —`isRefType`
     *  no conocía `TupleType`— y por eso rompió los 6 a la vez: con un solo sitio, arreglar
     *  el predicado sigue arreglándolo todo, pero ya no hay 6 sitios que puedan divergir.) */
    private void declareLocalByType(String name, BpType type) {
        declareLocalByType(name, type, varDbgTypeTag(type));
    }

    /** H1.8 — variante con tag del .dbg EXPLÍCITO. Existe porque hay sitios donde el tipo
     *  que decide el ANCHO y el que etiqueta el .dbg NO son el mismo a propósito: en una
     *  var local, `declT` (typeRefToBpType) es **null para refs** y el ancho sale del tipo
     *  semántico del Symbol (`widthT`). Derivar aquí el tag del tipo de ancho "mejoraría"
     *  el .dbg de "?" a "ref" — pero eso ya no es centralizar el ancho, es cambiar el
     *  debugger de paso. H1.8 cambia UNA cosa: dónde se decide el ancho. El .dbg sale
     *  byte-idéntico. (Que el .dbg etiquete "?" los locales-ref es mejorable, pero es otra
     *  tarea con sus propias pruebas.) */
    private void declareLocalByType(String name, BpType type, String dbgTag) {
        if (type != null && occupies8Bytes(type)) declareLocalLong(name, dbgTag);
        else declareLocal(name, dbgTag);
    }

    /** H1.8 — local que es una REFERENCIA por construcción (sintéticos sin BpType: el
     *  `newref` del Mutex de módulo). Una ref SIEMPRE es REF_SIZE=8.
     *
     *  OJO — va a `w.` DIRECTO, no al wrapper `declareLocalLong(name,tag)` del emisor: los
     *  sintéticos se emiten en sitios (emitInitFunction) donde el scopeStack AÚN NO existe
     *  y el wrapper hace `scopeStack.peek()` → NPE. Lo cazó `samples/RefGlobals.bp` (el
     *  único del corpus con `sync property` de módulo) en el primer intento de H1.8 fase
     *  2/3: sin ese sample el .mod habría salido idéntico y el NPE se cuela.
     *
     *  Tag del .dbg = null, como el `w.declareLocalLong(newref)` original: H1.8 cambia el
     *  ANCHO, no el .dbg. */
    private void declareLocalRef(String name) {
        w.declareLocalLong(name);
    }

    /** Gemelo con tag de tipo para el .dbg. MISMA semántica que el de arriba —sin el
     *  `scope.locals.add` de declareLocal/declareLocalLong— y eso es DELIBERADO: al
     *  añadirle el guarda «por uniformidad» se rompió `modpropsync` (NPE en el emisor),
     *  porque el camino del Mutex de una sync property de módulo (línea ~538) declara
     *  un nombre que el scope ya conoce y necesita que el WRITER lo vea igualmente.
     *  Regla de la casa aplicada en carne propia: se cambia UNA cosa. La rareza queda
     *  anotada, no arreglada de paso. */
    private void declareLocalRef(String name, String typeTag) {
        w.declareLocalLong(name, typeTag);
    }

    /** H1.8 — param que es una REFERENCIA por construcción: el receptor `this` de los
     *  métodos SINTETIZADOS (List/StringBuilder/Object/tuplas), que no tienen un BpType
     *  de su clase a mano.
     *
     *  POR QUÉ IMPORTA aunque hoy sea correcto: estaba escrito `declareParam("this", 8)`
     *  — un **8 a mano**. Hoy acierta, pero NO SE ADAPTA: si vuelve a cambiar la
     *  representación de una ref (que es justo el propósito de la abstracción, y lo que
     *  provocó todo el 4→8B), estos sitios se quedarían atrás EXACTAMENTE igual que se
     *  quedaron entonces. Aquí el ancho lo pone el carril de ref, no un humano. */
    private void declareParamRef(String name) {
        w.declareParam(name, 8);   // REF_SIZE. Tag del .dbg = null, como el original: H1.8 cambia el ANCHO, no el .dbg
    }

    /**
     * H5.c — SUSCRIPCIÓN. `algo.onClick := obj::metodo` son DOS escrituras, y el
     * ORDEN importa:
     *
     *   1º  destino (el slot de vtable)
     *   2º  receptor (el handle)  <-- éste es el que "confirma"
     *
     * Porque el despachador lee al revés: mira el receptor y, sólo si hay uno
     * vivo, usa el destino. Escribiendo el receptor el ÚLTIMO, un lector que se
     * cuele entre las dos escrituras ve o bien "sin suscriptor" (y calla, que es
     * el comportamiento correcto) o bien el par completo. Nunca un receptor
     * nuevo con un destino viejo.
     *
     * Al desuscribir (`:= null`) el orden se invierte por el mismo motivo: se
     * borra el receptor PRIMERO, y a partir de ese instante ya no se dispara.
     *
     * @return true si era una suscripción y ya está emitida.
     */
    private boolean emitEventSubscription(AssignStmt a) throws IOException {
        Symbol sym = info.exprSymbols.get(a.target);
        if (!(sym instanceof Symbol.EventSymbol)) return false;
        Symbol.EventSymbol ev = (Symbol.EventSymbol) sym;
        String cls = ev.ownerClass.name;

        // El objeto dueño del evento: `algo` en `algo.onClick`, o `this` si se
        // escribió a pelo dentro de la clase.
        IExpr owner = (a.target instanceof MemberAccessExpr)
                ? ((MemberAccessExpr) a.target).target : null;

        if (a.value instanceof Ast.NullLitExpr) {
            emitEventOwner(owner);
            // null como REFERENCIA son 8 bytes (handle a cero), no un 0 de 4:
            // el campo del receptor es is8 y SET_FIELD_LONG espera la palabra
            // entera. Es el mismo cuidado del censo V4 #11.
            emitInt(0); w.emit(OpCode.I32_TO_I64);
            emitSetRecv(ev, cls);                     // receptor PRIMERO al borrar
            emitEventOwner(owner);
            w.emitPushInt(0);
            emitSetDest(ev, cls);
            return true;
        }

        // #324 tanda 2c — `ev := __methodRef(obj, "nombre")`: lo MISMO que la
        // forma de abajo, pero con el slot resuelto en EJECUCIÓN en vez de por
        // el compilador. Se emite el mismo par de campos; lo único que cambia es
        // de dónde sale el destino: de la tabla `Clase#metodo#slot` que el .mod
        // ya exporta, consultada con el builtin de reflexión (se llama
        // __guiSlotOf por historia — no tiene nada de GUI, es "slot del método
        // llamado así en la clase de este objeto").
        //
        // Nombre que no existe → -1, que NO es un slot válido: la suscripción
        // queda hecha pero al dispararse el despachador dice "slot no resoluble"
        // y descarta el evento. Falla donde se ve, no en silencio.
        if (SemanticAnalyzer.isMethodRefByName(a.value)) {
            Ast.CallExpr c = (Ast.CallExpr) a.value;
            emitEventOwner(owner);
            emitExpr(c.args.get(0));                  // el objeto que atiende
            emitExpr(c.args.get(1));                  // el nombre del método
            w.emit(OpCode.CALL_BUILTIN);              // → slot (o -1)
            w.emitShort((short) Builtin.GUI_SLOT_OF.id);
            emitSetDest(ev, cls);                     // destino primero
            emitEventOwner(owner);
            emitExpr(c.args.get(0));                  // y el receptor confirma
            emitSetRecv(ev, cls);
            return true;
        }

        Ast.MethodRefExpr mr = (Ast.MethodRefExpr) a.value;
        Integer slot = info.methodRefSlots.get(mr);
        if (slot == null) { errors.add("referencia a método sin slot resuelto: " + mr.method); return true; }

        emitEventOwner(owner);
        // V7/H2 (#523) — el slot del handler: si la clase del receptor es externa o
        // abierta, es un número que depende de otro módulo → PUSH_REF de su método.
        Symbol hsym = info.exprSymbols.get(mr);
        ClassSymbol rcHandler = receptorDe(mr.target, (hsym instanceof FunctionSymbol) ? ((FunctionSymbol) hsym).ownerClass : null);
        if (porRef(rcHandler) && hsym instanceof FunctionSymbol) w.emitPushRef(refMet(rcHandler, claveDe((FunctionSymbol) hsym)));
        else w.emitPushInt(slot);
        emitSetDest(ev, cls);                         // destino primero
        emitEventOwner(owner);
        emitExpr(mr.target);                          // el receptor
        emitSetRecv(ev, cls);                         // y confirma
        return true;
    }

    /* H5.c-E2 — los dos campos del evento. Si el evento viene de otro módulo, su
     * clase no está en este ModWriter y no hay nombre que resolver: el slot lo
     * publicó su dueño en la interfaz y aquí sólo se USA. Dentro del módulo, por
     * nombre como siempre. */
    private void emitSetRecv(Symbol.EventSymbol ev, String cls) throws IOException {
        if (mod9() && ev.ownerClass != null && ev.ownerClass.isExternal) { w.emitSetFieldRef(refEvento(ev.ownerClass, claveEvento(ev), false), true); return; }
        if (ev.externalRecvSlot >= 0) w.emitSetFieldBySlot(ev.externalRecvSlot, true);
        else                          w.emitSetField(cls, ev.recvField());
    }
    private void emitSetDest(Symbol.EventSymbol ev, String cls) throws IOException {
        if (mod9() && ev.ownerClass != null && ev.ownerClass.isExternal) { w.emitSetFieldRef(refEvento(ev.ownerClass, claveEvento(ev), true), false); return; }
        if (ev.externalRecvSlot >= 0) w.emitSetFieldBySlot(ev.externalRecvSlot + 2, false);
        else                          w.emitSetField(cls, ev.destField());
    }

    /** Empuja el objeto dueño del evento (o `this` si la forma era a pelo). */
    private void emitEventOwner(IExpr owner) throws IOException {
        if (owner != null) emitExpr(owner);
        else               w.emitGetParam("this");
    }

    /**
     * H5.c — `raise onClick(args)`. Empuja los cinco operandos y delega en el
     * builtin, que ENCOLA. Aquí no se decide nada: ni si hay suscriptor, ni si
     * el receptor sigue vivo. Esa comprobación vive en UN solo sitio (el
     * despachador), que es lo que hace que "sin destino vivo -> silencio" sea
     * una regla y no una repetición en cada sitio que dispara.
     *
     *   recv   receptor (handle 8B; null = sin suscriptor)
     *   dest   slot de vtable del handler
     *   sender el objeto que dispara = this
     *   kind   ordinal del evento en la jerarquía (lo resolvió el semántico)
     *   args   payload (Object) o null
     */
    private void emitRaise(Ast.RaiseStmt r) throws IOException {
        Symbol sym = info.declSymbols.get(r);
        if (!(sym instanceof Symbol.EventSymbol)) { errors.add("raise sin evento resuelto: " + r.event); return; }
        Symbol.EventSymbol ev = (Symbol.EventSymbol) sym;
        String cls = ev.ownerClass.name;
        int n = ev.params.size();

        // ---- primero los ARGUMENTOS, cada uno en su ancho NATURAL ----
        // El despachador monta con ellos un frame BYTE A BYTE IGUAL al que
        // montaría una llamada normal a ese método. Ésa es la garantía fuerte
        // contra los fallos de ancho (que en este proyecto no son teóricos: el
        // 4→8B costó H1 entera): no inventamos una convención propia, usamos la
        // que ya existe. A cambio la VM tiene que saber el ancho de cada
        // argumento — y se lo DECIMOS, no lo adivina, en la misma palabra que la
        // máscara de refs del GC (bits 0-3 = ref, bits 8-11 = 8 bytes).
        // (los omitidos ya los rellenó el semántico con su valor por defecto, H8.1)
        if (r.args.size() != n) { errors.add("raise de '" + r.event + "' con aridad sin normalizar"); return; }
        int masks = 0;
        for (int i = 0; i < n; i++) {
            Symbol.ParamSymbol p = ev.params.get(i);
            emitExpr(r.args.get(i));
            coerceToTarget(r.args.get(i), p.type);
            if (isGcRef(p.type))      masks |= (1 << i);          // el GC lo sigue
            if (occupies8Bytes(p.type)) masks |= (1 << (8 + i));  // ocupa 8 bytes
        }

        // ---- y encima la cabecera, para que el despachador la saque primero ----
        emitInt(masks);                                                // ref|ancho (4B)
        emitInt(n);                                                    // nº de argumentos (4B)
        // V7/H2 (#492-B) — un evento HEREDADO de una clase de otro módulo: sus campos no
        // están en este ModWriter («Clase no declarada: Base», petaba el compilador); la
        // interfaz publicó el slot del receptor (`event … recv n`), y el destino va en +2.
        // Mismo par de ranuras que ya usa el suscriptor (emitEventBind).
        if (mod9() && ev.ownerClass != null && ev.ownerClass.isExternal) {   // V7/H2 (#523): por referencia
            w.emitGetParam("this"); w.emitGetFieldRef(refEvento(ev.ownerClass, claveEvento(ev), true), false);    // destino (4B)
            w.emitGetParam("this"); w.emitGetFieldRef(refEvento(ev.ownerClass, claveEvento(ev), false), true);    // receptor (8B)
        } else if (ev.externalRecvSlot >= 0) {
            w.emitGetParam("this"); w.emitGetFieldBySlot(ev.externalRecvSlot + 2, false);   // destino (4B)
            w.emitGetParam("this"); w.emitGetFieldBySlot(ev.externalRecvSlot, true);        // receptor (8B)
        } else {
            w.emitGetParam("this"); w.emitGetField(cls, ev.destField());   // destino (4B)
            w.emitGetParam("this"); w.emitGetField(cls, ev.recvField());   // receptor (8B)
        }
        w.emit(OpCode.CALL_BUILTIN);
        w.emitShort((short) Builtin.EVENT_RAISE.id);
    }

    private void emitAssign(AssignStmt a) throws IOException {
        if (emitEventSubscription(a)) return;
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
                // ---------- L6: static property de clase ----------
                // V7/H2 (#492-B): local o de otro módulo, por emitStaticGetter/Setter.
                if (ps.isStatic) {
                    switch (a.op) {
                        case ASSIGN: {
                            emitExpr(a.value);
                            coerceToTarget(a.value, tType);
                            emitStaticSetter(ps);
                            declareLocal("__discard");
                            w.emitSetLocal("__discard");
                            return;
                        }
                        case PLUS_ASSIGN:
                        case MINUS_ASSIGN: {
                            // val := getter() op rhs; setter(val)
                            emitStaticGetter(ps);
                            emitExpr(a.value);
                            coerceToTarget(a.value, tType);
                            emitCompoundOp(a.op, tType);
                            emitStaticSetter(ps);
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
                        // BUG-6: numArgs = slots (long/double = 2).
                        // V10 (EC-1): el receptor ESTÁTICO en la ref, como en los métodos.
                        emitInvokeVirtualSmart(receptorDe(ma.target, ps.ownerClass), ps.ownerClass, setter, claveSetter(ps.name, ps.type), occupies8Bytes(tType) ? 2 : 1);
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
                        emitInvokeVirtualSmart(receptorDe(ma.target, ps.ownerClass), ps.ownerClass, "get" + capitalize(ps.name), claveGetter(ps.name, ps.type), 0);
                        emitExpr(a.value);
                        coerceToTarget(a.value, tType);
                        emitCompoundOp(a.op, tType);
                        emitInvokeVirtualSmart(receptorDe(ma.target, ps.ownerClass), ps.ownerClass, setter, claveSetter(ps.name, ps.type), occupies8Bytes(tType) ? 2 : 1);
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
                        emitSetFieldDe(v, receptorDe(ma.target, v.ownerClass));   // V7/H2: por ranura si la clase es externa
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
                        emitGetFieldDe(v, receptorDe(ma.target, v.ownerClass));   // V7/H2
                        emitExpr(a.value);
                        coerceToTarget(a.value, tType);
                        emitCompoundOp(a.op, tType);
                        emitSetFieldDe(v, receptorDe(ma.target, v.ownerClass));
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

    /** #506 — el valor de una constante entera escrita: un literal, o `-literal` (el parser lo
     *  deja como UnaryExpr sobre el literal). null si no es constante. */
    static Long constanteEntera(IExpr e) {
        if (e instanceof IntLitExpr) return ((IntLitExpr) e).value;
        if (e instanceof UnaryExpr && "-".equals(((UnaryExpr) e).op) && ((UnaryExpr) e).operand instanceof IntLitExpr)
            return -((IntLitExpr) ((UnaryExpr) e).operand).value;
        return null;
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
            declareLocal(s.iteratorName, "integer");   // H6.a.2: iterador numérico = integer
            emitExpr(r.from);
            w.emitSetLocal(s.iteratorName);
            // pre-compute end (puede ser una expresión arbitraria)
            String endLocal = "__for_end_" + System.identityHashCode(s);
            declareLocal(endLocal);
            emitExpr(r.to);
            w.emitSetLocal(endLocal);
            // step (default 1). #506 (19-sep): antes sólo un IntLitExpr positivo; `step -1` llega
            // como UnaryExpr("-", 1) y una variable ni se contemplaba («for step solo soporta
            // literal int por ahora»). Constante (con signo) → se pliega y la comparación se
            // elige aquí; expresión → se evalúa UNA vez a un local y la dirección se decide en
            // ejecución (step >= 0 → mientras i <= end; si no → mientras i >= end).
            Long stepK = (r.step == null) ? Long.valueOf(1) : constanteEntera(r.step);
            String stepLocal = null;
            if (stepK == null) {
                stepLocal = "__for_step_" + System.identityHashCode(s);
                declareLocal(stepLocal);
                emitExpr(r.step);
                w.emitSetLocal(stepLocal);
            }
            int top = w.newLabel();
            int end = w.newLabel();
            w.declareLabel(top);
            if (stepK != null) {
                w.emitGetLocal(s.iteratorName);
                w.emitGetLocal(endLocal);
                w.emit(stepK >= 0 ? OpCode.LE : OpCode.GE);
                w.emitJumpIfFalse(end);
            } else {
                int haciaAbajo = w.newLabel(), cuerpo = w.newLabel();
                w.emitGetLocal(stepLocal);
                emitInt(0);
                w.emit(OpCode.GE);
                w.emitJumpIfFalse(haciaAbajo);
                w.emitGetLocal(s.iteratorName);
                w.emitGetLocal(endLocal);
                w.emit(OpCode.LE);
                w.emitJumpIfFalse(end);
                w.emitJump(cuerpo);
                w.declareLabel(haciaAbajo);
                w.emitGetLocal(s.iteratorName);
                w.emitGetLocal(endLocal);
                w.emit(OpCode.GE);
                w.emitJumpIfFalse(end);
                w.declareLabel(cuerpo);
            }
            loopStack.push(new LoopLabels(top, end));
            try {
                for (IStmt st : s.body) emitStmt(st);
            } finally { loopStack.pop(); }
            // i += step
            w.emitGetLocal(s.iteratorName);
            if (stepK != null) emitInt(stepK.intValue()); else w.emitGetLocal(stepLocal);
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
            // H6.a.2: tag del iterador = tipo de elemento del iterable.
            BpType itElem = info.exprTypes.get(r.iterable);
            // H1.8/#19 — REGRESIÓN del 4→8B. `__forin_arr_` guarda EL ARRAY del bucle =
            // una REFERENCIA de 8B, y estaba `declareLocal(arrLocal)` = 4B A PELO. Se
            // ESCONDÍA porque el llamante también empujaba 4B (todo mal pero CONSISTENTE);
            // en cuanto un array llega bien a 8B (param `integer[]`, o el array fijo local
            // ya arreglado), `GET_LOCAL_L` empuja 8 y `SET_LOCAL` popeaba 4 → handle
            // truncado + 4B de fuga que descuadran la pila del bucle.
            // AFECTA A TODO `for x in <array>`, no solo a los arrays fijos.
            // Lo delató `samples/LocalArrTest.bp` (su `suma via funcion` = for-in sobre un
            // param `integer[]`). idx/end SÍ son enteros → 4B correcto.
            declareLocalByType(arrLocal, itElem, null);
            declareLocal(idxLocal);
            declareLocal(endLocal);
            BpType forinElem = (itElem instanceof ArrayType) ? ((ArrayType) itElem).element : null;
            String forinTag = (forinElem != null) ? varDbgTypeTag(forinElem) : null;
            // task_cf2a924b (Bug B): la variable de bucle debe declararse con el
            // ANCHO del elemento. long/double = 8 bytes; si se declara de 4 (como
            // antes) un `for x in double[]` dejaba x medio inicializado y
            // descuadraba locales/pila — p.ej. this.metodo(x) dentro del bucle leía
            // un receiver null ("INVOKE_VIRTUAL sobre null"). Narrow (byte/word)
            // se extiende a i32 en el ALOAD → local de 4 bytes, correcto.
            declareLocalByType(s.iteratorName, forinElem, forinTag);   // H1.8

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

            // n := __arr[__idx] — ALOAD width-aware (ALOAD_I64 para long/double,
            // ALOAD_I8/U8/I16/U16 para narrow); emitSetLocal usa el ancho del local.
            w.emitGetLocal(arrLocal);
            w.emitGetLocal(idxLocal);
            w.emit(aloadOpFor(r.iterable));
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
            // 🔴 hallazgo 34 (5-ago): esto era `declareLocal(br.varName)` — un local de
            // 4 BYTES para guardar un HANDLE de 8. El `emitSetLocal(br.varName)` de abajo
            // truncaba la referencia al thread, y el `emitGetLocal` de start()/join()
            // trabajaba con media referencia. No mordía mientras las generaciones de la
            // tabla de handles valían 0 —los 32 bits que se pierden son justo esos—, así
            // que el fallo sólo salía DESPUÉS de reciclar slots (agotar el heap y soltar).
            // Por eso `paralleltest_sugar` moría y su gemelo con clase declarada no: el
            // camino normal de `var x: Thread := ...` ya declaraba por TIPO.
            // Es el mismo animal que #369/#281/#293, y el sitio que se saltó la pasada H1.8.
            declareLocalRef(br.varName, "Thread");
            int id = tempCounter++;
            String newref  = "__par_newref_"  + id;
            String discard = "__par_discard_" + id;
            declareLocalRef(newref);   // H1.8: era declareLocalLong = un 8 a mano
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
            invocarDelLenguaje("Thread", "start", 0);
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
            invocarDelLenguaje("Thread", "join", 0);
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
        ClassSymbol thread = claseDelLenguaje("Thread");
        abrirClaseSintetizada(name, thread);

        w.addMethod("run");
        if (mod9()) w.setMethodMeta("run", claveDe(metodoDe(thread, "run")), true, false);   // override de Core.Thread.run
        beginFunctionScope(makeSynthFs("run", null), null);
        try {
            for (IStmt s : br.body) emitStmt(s);
            emitFunctionEnd();
        } finally { scopeStack.pop(); }

        w.endClass();

        // --- Constructor __init(this, stackSize) → super(stackSize)
        w.addFunction(name + ".__init", false);
        declareParamRef("this");   // H1.8: receptor = ref (era un 8 a mano)
        w.declareParam("stackSize");
        beginFunctionScope(makeSynthFs("__init", null), null);
        try {
            // super(stackSize) → Thread.__init(this, stackSize)
            w.emitGetParam("this");
            w.emitGetParam("stackSize");
            emitCallInitOf(thread.findConstructor());   // super(stackSize) → Core.Thread
            declareLocal("__discardSuper");
            w.emitSetLocal("__discardSuper");
            emitFunctionEnd();
        } finally { scopeStack.pop(); }
    }

    // ============================================================
    // #325 — `Thread(obj::metodo(args))`: un thread cuyo cuerpo es esa llamada.
    //
    // Es lo mismo que el usuario escribía a mano (subclasear Thread y meter la
    // llamada en run()), pero sin la ceremonia. Como el método destino se
    // conoce en COMPILACIÓN, el run() sintetizado LLAMA DIRECTAMENTE: no hay
    // eventos, ni cola, ni drenaje.
    //
    // Receptor y argumentos viajan como CAMPOS del objeto porque BP no tiene
    // closures. Eso además fija la semántica: los argumentos se evalúan en el
    // llamante, al construir, no cuando el thread arranca.
    // ============================================================

    /** Una subclase de Thread por cada `Thread(obj::m(args))` del módulo. Van
     *  antes que las clases de usuario, como las de `parallel`, para que
     *  cualquier función pueda referenciar su descriptor y su `__init`. */
    private void synthesizeAllThreadBodyClasses() throws IOException {
        int n = 0;
        for (Ast.BoundCallExpr bc : info.boundCalls) {
            bc.synthesizedClassName = "__Thr" + (n++) + "_" + bc.method;
            synthesizeThreadBodyClass(bc);
        }
    }

    private void synthesizeThreadBodyClass(Ast.BoundCallExpr bc) throws IOException {
        FunctionSymbol fn = (FunctionSymbol) info.exprSymbols.get(bc);
        BpType rt = info.exprTypes.get(bc.target);
        ClassSymbol rc = (rt instanceof BpType.ClassType) ? ((BpType.ClassType) rt).cls : null;
        if (fn == null || rc == null) {
            errors.add("Thread(obj::" + bc.method + "(...)): sin método o receptor resueltos"
                    + " (línea " + bc.line + ")");
            return;
        }
        String name = bc.synthesizedClassName;
        ClassSymbol thread = claseDelLenguaje("Thread");
        abrirClaseSintetizada(name, thread);

        // Campos: el receptor (ref ⇒ el GC lo traza, que es lo que mantiene vivo
        // al objeto mientras el thread no ha corrido) y un campo por argumento.
        w.declareField("__recv", true, false, true);   // handle 8B
        if (mod9()) w.setFieldMeta("__recv", "__recv:LObject;", true);
        for (int i = 0; i < bc.args.size(); i++) {
            BpType at = argTypeOf(fn, bc, i);
            w.declareField("__a" + i, isGcRef(at), false, occupies8Bytes(at));
            if (mod9()) w.setFieldMeta("__a" + i, claveCampo("__a" + i, at), true);
        }

        // run() — override del slot 0 de Thread, que es el punto de entrada.
        //
        // No lleva la llamada dentro: saca los campos y delega en la función
        // libre `__thrbody_N`. Es que si no, no hay orden posible — esta clase
        // tiene que emitirse ANTES que las del usuario (para que `emitNewObject`
        // encuentre su descriptor) y la llamada necesita que la clase del
        // receptor ya esté declarada, que es DESPUÉS. El CALL a una función
        // libre se resuelve por fixup en el link, así que no le importa el
        // orden — el mismo truco que synthesizeObjectCompareErrorHelper.
        w.addMethod("run");
        if (mod9()) w.setMethodMeta("run", claveDe(metodoDe(thread, "run")), true, false);   // override de Core.Thread.run
        beginFunctionScope(makeSynthFs("run", null), null);
        try {
            w.emitGetParam("this");
            w.emitGetField(name, "__recv");
            for (int i = 0; i < bc.args.size(); i++) {
                w.emitGetParam("this");
                w.emitGetField(name, "__a" + i);
            }
            w.emitCall(threadBodyFnName(bc));
            declareLocal("__discardRun");
            w.emitSetLocal("__discardRun");   // la llamada deja un valor; se tira
            emitFunctionEnd();
        } finally { scopeStack.pop(); }

        w.endClass();

        // __init(this, recv, a0..aN, stackSize) → super(stackSize) + guardar todo.
        w.addFunction(name + ".__init", false);
        declareParamRef("this");
        declareParamRef("__recv");
        for (int i = 0; i < bc.args.size(); i++) {
            declareParamByType("__a" + i, argTypeOf(fn, bc, i));
        }
        w.declareParam("stackSize");
        beginFunctionScope(makeSynthFs("__init", null), null);
        try {
            w.emitGetParam("this");
            w.emitGetParam("stackSize");
            emitCallInitOf(thread.findConstructor());   // super(stackSize) → Core.Thread
            declareLocal("__discardSuper");
            w.emitSetLocal("__discardSuper");

            w.emitGetParam("this");
            w.emitGetParam("__recv");
            w.emitSetField(name, "__recv");
            for (int i = 0; i < bc.args.size(); i++) {
                w.emitGetParam("this");
                w.emitGetParam("__a" + i);
                w.emitSetField(name, "__a" + i);
            }
            emitFunctionEnd();
        } finally { scopeStack.pop(); }
    }

    private static String threadBodyFnName(Ast.BoundCallExpr bc) {
        return "__thrbody" + bc.synthesizedClassName.substring("__Thr".length());
    }

    /** El cuerpo de verdad de cada `Thread(obj::metodo(args))`, como función
     *  libre: recibe receptor y argumentos y hace la llamada. Va DESPUÉS de las
     *  clases del usuario, que es cuando la clase del receptor ya existe para el
     *  ModWriter y su slot de vtable se puede resolver. */
    private void synthesizeAllThreadBodyFns() throws IOException {
        for (Ast.BoundCallExpr bc : info.boundCalls) {
            FunctionSymbol fn = (FunctionSymbol) info.exprSymbols.get(bc);
            BpType rt = info.exprTypes.get(bc.target);
            ClassSymbol rc = (rt instanceof BpType.ClassType) ? ((BpType.ClassType) rt).cls : null;
            if (fn == null || rc == null || bc.synthesizedClassName == null) continue;  // ya se avisó

            w.addFunction(threadBodyFnName(bc), false);
            declareParamRef("__recv");
            for (int i = 0; i < bc.args.size(); i++) {
                declareParamByType("__a" + i, argTypeOf(fn, bc, i));
            }
            beginFunctionScope(makeSynthFs("__thrbody", null), null);
            try {
                w.emitGetParam("__recv");
                for (int i = 0; i < bc.args.size(); i++) w.emitGetParam("__a" + i);
                // Mismo criterio que una llamada normal: un método privado no
                // está en la vtable y se llama directo.  (#390: lo que el
                // comentario decía con palabras ahora lo dice el código.)
                if (!fn.esVirtual() && !fn.isExternal) {
                    w.emitCall(fn.ownerClass.name + "." + emitName(fn));
                } else {
                    emitInvokeVirtualSmart(rc, fn.ownerClass, emitName(fn), claveDe(fn), argSlotCount(fn));
                }
                declareLocal("__discardBody");
                w.emitSetLocal("__discardBody");
                emitFunctionEnd();
            } finally { scopeStack.pop(); }
        }
    }

    /** Tipo con el que se declara el campo/parámetro del argumento i: el del
     *  PARÁMETRO (que manda para el ancho y la coerción), y sólo si no lo hay
     *  —no debería— el del argumento. */
    private BpType argTypeOf(FunctionSymbol fn, Ast.BoundCallExpr bc, int i) {
        if (i < fn.params.size() && fn.params.get(i).type != null) return fn.params.get(i).type;
        return info.exprTypes.get(bc.args.get(i));
    }

    /** Construcción de `Thread(obj::metodo(args) [, pila])`: NEW_OBJECT de la
     *  subclase sintetizada + su `__init` con receptor, argumentos y pila.
     *  Deja la ref en la pila, como cualquier construcción. */
    private void emitThreadOfBoundCall(List<IExpr> args) throws IOException {
        Ast.BoundCallExpr bc = (Ast.BoundCallExpr) args.get(0);
        FunctionSymbol fn = (FunctionSymbol) info.exprSymbols.get(bc);
        if (fn == null || bc.synthesizedClassName == null) {
            errors.add("Thread(obj::" + bc.method + "(...)): sin clase sintetizada"
                    + " (línea " + bc.line + ")");
            return;
        }
        int id = tempCounter++;
        String newref  = "__thrref_" + id;
        String discard = "__thrdisc_" + id;
        declareLocalLong(newref);   // el ref del objeto nuevo = handle de 8 bytes
        declareLocal(discard);
        w.emitNewObject(bc.synthesizedClassName);
        w.emitSetLocal(newref);

        w.emitGetLocal(newref);
        emitExpr(bc.target);                      // el receptor
        for (int i = 0; i < bc.args.size(); i++) {
            emitExpr(bc.args.get(i));
            coerceToTarget(bc.args.get(i), argTypeOf(fn, bc, i));
        }
        if (args.size() >= 2) {
            emitExpr(args.get(1));                // pila pedida
            coerceToTarget(args.get(1), PrimitiveType.INTEGER);
        } else {
            emitInt(0);                           // 0 = la de por defecto de la VM
        }
        w.emitCall(bc.synthesizedClassName + ".__init");
        w.emitSetLocal(discard);
        w.emitGetLocal(newref);
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

    /** Destruye y libera cada owner local del scope actual, en orden INVERSO al de declaración
     *  (V5_IDEAS, 9-ago: «un q sacado de un db tiene que morir ANTES que el db»): si no es null,
     *  su destructor (virtual: el de la clase real del objeto) y después FREE_REF; el VM hace
     *  no-op con una ref null, y el destructor no se llama sobre null. */
    private void emitFreeOwnerLocals() throws IOException {
        FuncScope scope = scopeStack.peek();
        java.util.List<String> names = new ArrayList<>(scope.ownerLocals);
        java.util.Collections.reverse(names);
        for (String name : names) {
            emitDestruirLocal(name);
            w.emitGetLocal(name);
            w.emitFreeRef();
        }
    }
    /** `if (x != null) x.~()`: el destructor, por la clase ESTÁTICA del local (despacho virtual). */
    private void emitDestruirLocal(String name) throws IOException {
        ClassSymbol cls = claseDeLocal(name);
        if (cls == null) return;   // no es una clase (el semántico ya lo dijo): sólo FREE_REF
        FunctionSymbol dtor = dtorDe(cls);
        if (dtor == null) return;
        int skip = w.newLabel();
        w.emitGetLocal(name);
        w.emit(OpCode.LPUSH); w.emitLong(0L);
        w.emit(OpCode.LNEQ);
        w.emitJumpIfFalse(skip);
        w.emitGetLocal(name);
        emitInvokeVirtualSmart(cls, dtor.ownerClass != null ? dtor.ownerClass : cls, Symbol.FunctionSymbol.DTOR, claveDe(dtor), 0);
        String discard = "__dtor_discard_" + (tempCounter++);
        declareLocal(discard);
        w.emitSetLocal(discard);
        w.declareLabel(skip);
    }
    /** El destructor visible desde `cls` (el suyo o el heredado; Core.Object lo declara vacío). */
    private static FunctionSymbol dtorDe(ClassSymbol cls) {
        Symbol m = cls.lookupInstance(Symbol.FunctionSymbol.DTOR);
        return (m instanceof FunctionSymbol) ? (FunctionSymbol) m : null;
    }
    /** La clase estática de un local owner, por su símbolo en el scope semántico de la función. */
    private ClassSymbol claseDeLocal(String name) {
        BpType t = tiposDeOwners.get(name);
        return (t instanceof ClassType) ? ((ClassType) t).cls : null;
    }
    /** V7/H1 — el tipo estático de cada owner local de la función en curso (lo apunta emitVarDecl). */
    private final java.util.Map<String, BpType> tiposDeOwners = new java.util.HashMap<>();
    /** V7/H1 — al final de un destructor: `this.~Padre()` (llamada NO virtual al del padre, como
     *  `super.m()`), para que toda la cadena libere lo suyo. Object y Thread son raíz: nada. */
    private void emitDtorDelPadre() throws IOException {
        ClassSymbol cls = claseActual();
        if (cls == null || cls.baseClass == null) return;
        ClassSymbol padre = cls.baseClass;
        FunctionSymbol fs = dtorDe(padre);
        if (fs == null) return;
        w.emitGetParam("this");
        if (mod9() && porRef(padre)) {
            w.emitCallRef(w.requireRef(ModWriter.REF_SUPER, depRef(padre), claseRef(padre), claveDe(fs)));
        } else {
            ClassSymbol duena = fs.ownerClass;
            if (duena != null && duena.isExternal) {
                StringBuilder qn = new StringBuilder();
                if (duena.externalLibrary != null && !duena.externalLibrary.isEmpty()) qn.append(duena.externalLibrary).append('.');
                qn.append(duena.externalModule).append('.').append(crossModuleMethodName(duena, fs));
                w.emitCallExt(qn.toString(), fromPathFor(qn.toString()));
            } else {
                w.emitCall(duena.name + "." + emitName(fs));
            }
        }
        String discard = "__dtorsuper_discard_" + (tempCounter++);
        declareLocal(discard);
        w.emitSetLocal(discard);
    }
    /** ¿Declara `var owner` locales (en cualquier bloque anidado)? Para abrir el TRY que los guarda. */
    private static boolean tieneOwnerLocales(List<IStmt> body) {
        if (body == null) return false;
        for (IStmt s : body) {
            if (s instanceof VarDecl && ((VarDecl) s).isOwner) return true;
            if (s instanceof IfStmt) { IfStmt i = (IfStmt) s; if (tieneOwnerLocales(i.then_.body) || tieneOwnerLocales(i.else_)) return true; for (IfClause c : i.elseIfs) if (tieneOwnerLocales(c.body)) return true; }
            if (s instanceof WhileStmt && tieneOwnerLocales(((WhileStmt) s).body)) return true;
            if (s instanceof ForStmt && tieneOwnerLocales(((ForStmt) s).body)) return true;
            if (s instanceof TryStmt) { TryStmt t = (TryStmt) s; if (tieneOwnerLocales(t.body) || tieneOwnerLocales(t.finallyBody)) return true; for (CatchClause c : t.catches) if (tieneOwnerLocales(c.body)) return true; }
        }
        return false;
    }
    /** V7/H1 — abre el TRY que guarda los owners de la función (si los hay): se llama justo
     *  después de beginFunctionScope y antes del primer statement del cuerpo. */
    private void abrirGuardaDeOwners(List<IStmt> body) throws IOException {
        if (!tieneOwnerLocales(body)) return;
        FuncScope scope = scopeStack.peek();
        scope.guardaOwners = true;
        scope.ownersHandler = w.newLabel();
        w.emitTryBegin(scope.ownersHandler, null);
        tryStack.push(new TryFrame(null, 1));
    }

    /**
     * Abre un FuncScope nuevo: asigna endLabel, decide returnsValue según el
     * tipo de retorno, y declara la local implícita "__result" si procede.
     * El caller emite los params via declareParam ANTES, y los stmts después.
     */
    private void beginFunctionScope(FunctionSymbol fs, BpType returnTypeOverride) {
        BpType rt = (returnTypeOverride != null) ? returnTypeOverride : fs.returnType;
        boolean returnsValue = rt != null && !(rt instanceof BpType.VoidType);
        boolean returnsLong = occupies8Bytes(rt);   // H1.2/H1.3: return de 8 bytes (long/double) → LRET
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
        if (scope.guardaOwners) {   // V7/H1: cierra el TRY que guarda los owners (camino normal)
            w.emitTryEnd();
            if (!tryStack.isEmpty()) tryStack.pop();
        }
        // Fall-through path: liberar owners locales antes del RET.
        emitFreeOwnerLocals();
        if (scope.guardaOwners) {
            // V7/H1 — el manejador: una excepción dentro del cuerpo destruye y libera los owners
            // (en orden inverso) y se relanza. El camino normal y los `return` ya han pasado por
            // emitFreeOwnerLocals y saltan por encima.
            int fin = w.newLabel();
            w.emitJump(fin);
            w.declareLabel(scope.ownersHandler);
            declareLocalLong("__pending_owner_exc");
            w.emitSetLocal("__pending_owner_exc");
            emitFreeOwnerLocals();
            w.emitGetLocal("__pending_owner_exc");
            w.emitThrow();
            w.declareLabel(fin);
        }
        w.declareLabel(scope.endLabel);
        if (scope.encadenarDtorDelPadre) emitDtorDelPadre();   // V7/H1: ~Hija() → ~Padre()
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
            } else if (t instanceof ClassType) {
                // H5.1.b — objeto: auto-toString() polimórfico con guarda null→"null".
                // El ref ya está en la pila. #526: por nombre sobre la clase estática.
                emitObjectToStringOnStack(((ClassType) t).cls);
                w.emit(OpCode.PRINT_STR_NONL);
            } else if (t == null || t instanceof BpType.ErrorType) {
                // Sucede con variables de 'catch' genérico (analyzer las tipa como
                // ErrorType). En BP los throws son típicamente strings.
                w.emit(OpCode.PRINT_STR_NONL);
            } else {
                // ref u otro (AnyType, etc.) — imprimir como int por ahora
                w.emit(OpCode.PRINT_NONL);
            }
        }
        w.emit(OpCode.PRINT_NL);
    }

    /**
     * H5.1.b — Convierte el ref de objeto del tope de pila a su representación
     * string via {@code toString()} (slot 0 de la vtable, polimórfico), con
     * guarda {@code null}→"null". Consume el ref y deja el string ref en pila.
     * Reutilizable por print y, más adelante, por concatenación con `+`.
     */
    /** #526 — `print obj` / `"" + obj`: toString despachado por NOMBRE (o por clave, R2) sobre
     *  la clase ESTÁTICA del valor, no por la ranura 0 «de toda instancia»: en una clase que
     *  desciende de Thread la 0 es run(). Con `cls == null` (un `Object` sin más) sigue la 0. */
    private void emitObjectToStringOnStack(ClassSymbol cls) throws IOException {
        String tmp = "__o2s_" + (stringPoolCounter++);
        // #292f (4→8B): tmp guarda un REF de objeto = 8 bytes. Con declareLocal (4B),
        // el SET_LOCAL truncaba el handle y dejaba la palabra `gen` HUÉRFANA en la
        // pila. El path no-null la reconsumía por CASUALIDAD (invokeVirtual saca 8
        // bytes = gen huérfano + idxTag re-empujado), pero el path null saltaba ese
        // invoke → fuga de 4 bytes → el concat siguiente leía el otro operando
        // desalineado → "No space in heap". Un ref = 8 bytes: declareLocalRef.
        declareLocalRef(tmp);
        w.emitSetLocal(tmp);                  // guarda el ref (8 bytes)
        int nullL = w.newLabel();
        int doneL = w.newLabel();
        // null-check a 8 BYTES: el handle es de 8B, así que el test == null hay que
        // hacerlo con LNEQ (como el path de usuario, #11). Un GET_LOCAL_L empuja 8
        // bytes y el JUMPIFFALSE sólo saca 4 → habría que igualar el ancho antes de
        // ramificar. LNEQ contra 0 consume los 8 bytes y deja un bool de 4B.
        w.emitGetLocal(tmp);                  // handle (8 bytes)
        w.emit(OpCode.LPUSH); w.emitLong(0L); // null = 8 bytes a cero
        w.emit(OpCode.LNEQ);                  // bool: handle != null
        w.emitJumpIfFalse(nullL);             // == null → "null"
        w.emitGetLocal(tmp);                  // this = ref (8 bytes)
        if (cls == null) {
            w.emitInvokeVirtualBySlot(0, 0);   // any/Object sin clase estática: toString es la ranura 0 por contrato (Core.bp)
        } else {
            Symbol ts = cls.lookupInstance("toString");
            ClassSymbol declarante = (ts instanceof FunctionSymbol && ((FunctionSymbol) ts).ownerClass != null) ? ((FunctionSymbol) ts).ownerClass : cls;
            emitInvokeVirtualSmart(cls, declarante, "toString", mod9() ? "toString()s" : "", 0);   // toString() → string ref
        }
        w.emitJump(doneL);
        w.declareLabel(nullL);
        w.emitLeaGlobal(internString("null"));
        w.declareLabel(doneL);
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
        List<IStmt> finallyBody = (s.finallyBody != null && !s.finallyBody.isEmpty())
                ? s.finallyBody : null;
        if (s.catches.isEmpty()) {
            if (finallyBody == null) {   // ni catch ni finally: el cuerpo a secas
                for (IStmt st : s.body) emitStmt(st);
                return;
            }
            // #532 (19-sep) — `try … finally` SIN catch (la gramática lo admite; el emisor lo
            // rechazaba «no soportado todavía»): la misma guarda que el rethrow de un
            // try/catch/finally, sin handlers de por medio. Salida normal: pop + finally en
            // línea; excepción: finally y relanzar; `return` dentro: TRY_END + finally
            // (emitReturn, por el TryFrame).
            int rethrow = w.newLabel(), fin = w.newLabel();
            w.emitTryBegin(rethrow, null);
            tryStack.push(new TryFrame(finallyBody, 1));
            try {
                for (IStmt st : s.body) emitStmt(st);
            } finally {
                tryStack.pop();
            }
            w.emitTryEnd();
            for (IStmt st : finallyBody) emitStmt(st);
            w.emitJump(fin);
            w.declareLabel(rethrow);
            declareLocalLong("__pending_exc");   // ref de 8 B
            w.emitSetLocal("__pending_exc");
            for (IStmt st : finallyBody) emitStmt(st);
            w.emitGetLocal("__pending_exc");
            w.emitThrow();
            w.declareLabel(fin);
            return;
        }
        int N = s.catches.size();

        int endLabel     = w.newLabel();
        int rethrowLabel = (finallyBody != null) ? w.newLabel() : -1;
        int[] handlerLabels = new int[N];
        for (int i = 0; i < N; i++) handlerLabels[i] = w.newLabel();

        // === Emitir N TRY_BEGINs en orden INVERSO ===
        // El último que emitimos queda más arriba en el handler stack ⇒ se
        // prueba primero. Queremos que el primer catch del source (handlerLabels[0])
        // sea el primero en probarse, así que emitimos del último al primero.
        for (int i = N - 1; i >= 0; i--) {
            CatchClause cc0 = s.catches.get(i);
            ClassSymbol exc = info.catchClassSymbols.get(cc0);
            if (exc != null && porRef(exc)) {
                w.emitTryBeginRef(handlerLabels[i], refClase(exc));   // V7/H2 (#523)
            } else if (exc != null && exc.isExternal) {
                // BUG-2 — clase de excepción cross-module: TRY_BEGIN_EXT con el
                // nombre cualificado; el clsOff i32 se parcha en link-time.
                StringBuilder qn = new StringBuilder();
                if (exc.externalLibrary != null && !exc.externalLibrary.isEmpty())
                    qn.append(exc.externalLibrary).append('.');
                qn.append(exc.externalModule).append('.').append(exc.name);
                w.emitTryBeginExt(handlerLabels[i], qn.toString());
            } else {
                w.emitTryBegin(handlerLabels[i], cc0.exceptionType);
            }
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
                declareLocalLong(cc.varName);   // H1.2a: el objeto de excepción cazado = ref 8 bytes
                w.emitSetLocal(cc.varName);
            } else {
                declareLocalLong("__discard");   // H1.2a: hay que sacar los 8 bytes del ref de excepción
                w.emitSetLocal("__discard");
            }
            // V10 (16-sep, EC-2) — al entrar al handler, las DOS VMs han sacado de la pila los
            // handlers de ENCIMA del que casa y ese mismo; los HERMANOS DE DEBAJO (los catch
            // posteriores de este try, empujados antes) seguían vivos. Un throw posterior, fuera
            // de todo try, volvía a entrar en un catch rancio de este try (y re-ejecutaba lo que
            // hubiera detrás): `1 catch RuntimeError / antes del throw / 2 catch Exception (NO
            // deberia salir) / antes del throw …` en las dos VMs. Se sacan aquí; así el
            // catch_context lleva activeHandlers=0 de verdad (y mantiene el finally para los
            // returns dentro del catch).
            for (int j = i + 1; j < N; j++) w.emitTryEnd();
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
            declareLocalLong("__pending_exc");   // #527 (de paso): era declareLocal (4 B) para una ref de 8 (hallazgo 34)
            w.emitSetLocal("__pending_exc");
            for (IStmt st : finallyBody) emitStmt(st);
            w.emitGetLocal("__pending_exc");
            w.emitThrow();
        }

        w.declareLabel(endLabel);
    }

    /** La clase `Exception` de Core tal como la ve este módulo (el stub importado), o null. */
    private ClassSymbol claseExceptionDeCore() {
        for (Symbol m : info.module.members.getSymbols())
            if (m instanceof Symbol.ImportedNamespaceSymbol && "Core".equals(((Symbol.ImportedNamespaceSymbol) m).moduleName))
                return ((Symbol.ImportedNamespaceSymbol) m).classes.get("Exception");
        return null;
    }
    private void emitThrowStmt(ThrowStmt s) throws IOException {
        emitExpr(s.value);
        // V10 (16-sep, EC-3/E-09): `throw e` ASUME e <: Core.Exception — el semántico lo comprobó
        // contra la versión de entonces; la ref SUBTIPO lo deja escrito para que el cargador lo
        // verifique contra la real (E10) en vez de lanzar un objeto que no es una excepción.
        BpType tv = info.exprTypes.get(s.value);
        ClassSymbol exc = claseExceptionDeCore();
        if (tv instanceof ClassType && exc != null) asumirSubtipo(tv, new ClassType(exc));
        // B2 (parte 2): si estamos dentro del cuerpo de un getter/setter
        // de una sync property, hay que liberar el mutex antes de
        // propagar la excepción. Sin esto, un throw deja el mutex tomado
        // y los próximos lock's del mismo objeto petan con "re-entrada".
        // Las exceptional paths necesitan el mismo tratamiento que
        // `return`, no sólo el flujo normal de salida.
        // #527 (b) — el unlock del `throw` escrito lo hace ahora la GUARDA del accesor
        // (abrirGuardaSync): aquí soltarlo sería un doble unlock. El `return` sigue
        // soltándolo él (emitReturn), que no pasa por el handler.
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
        else if (e instanceof NullLitExpr)  { emitInt(0); w.emit(OpCode.I32_TO_I64); }   // #11 (censo V4): null es una REF = handle de 8B (cero); antes emitInt(0) 4B descuadraba la pila y dejaba basura en el word alto
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
        else if (e instanceof Ast.TupleExpr)    emitTupleExpr((Ast.TupleExpr) e);
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

    // H7 — potencia '^'. El tipo resultado (tb, del semántico) elige el camino:
    // float/double → DPOW (cuadrados en f64 si el exponente es entero —incl. x^2
    // float—, exp·ln si es fraccionario), estrechando a f32 si tb es float;
    // long → LPOW; int → IPOW (ambos exponenciación por cuadrados entera, exacta).
    // Coercionamos cada operando al tipo que espera el opcode.
    private void emitPow(BinaryExpr b, BpType tl, BpType tr, BpType tb) throws IOException {
        if (isDouble(tb) || isFloat(tb)) {
            emitExpr(b.left);  promoteNumeric(tl, true, false, false);   // → double
            emitExpr(b.right); promoteNumeric(tr, true, false, false);   // → double
            w.emit(OpCode.DPOW);
            if (isFloat(tb)) w.emit(OpCode.D2F);                         // f64 → f32
        } else if (isLong(tb)) {
            emitExpr(b.left);  promoteNumeric(tl, false, false, true);   // → long
            emitExpr(b.right); promoteNumeric(tr, false, false, true);   // → long
            w.emit(OpCode.LPOW);
        } else {
            emitExpr(b.left);  promoteNumeric(tl, false, false, false);  // → int
            emitExpr(b.right); promoteNumeric(tr, false, false, false);  // → int
            w.emit(OpCode.IPOW);
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

        // H7 — potencia '^': tipado/cómputo propios (camino entero vs float), no
        // entra en la torre de promoción genérica de abajo.
        if ("^".equals(b.op)) {
            emitPow(b, tl, tr, tb);
            return;
        }

        // H1.3 (V2): torre de promoción double > float > long > int.
        boolean useDouble = isDouble(tl) || isDouble(tr) || isDouble(tb);
        boolean useFloat  = !useDouble && (isFloat(tl) || isFloat(tr) || isFloat(tb));
        boolean useLong   = !useDouble && !useFloat && (isLong(tl) || isLong(tr) || isLong(tb));
        // #11 (censo V4): igualdad de REFERENCIAS = comparar los 8 bytes ENTEROS del
        // handle (gen+idx). Dos handles con mismo idx y distinta gen son objetos
        // DISTINTOS → el EQ de 4B (solo idx|TAG) da falsos "iguales" + descuadra la
        // pila (16B apilados, 8B popeados). Aplica a class/array/tuple/any/null (todo
        // lo de 8B no-numérico); string ==/!= ya se resolvió arriba con __strequals.
        boolean use8ByteEq = useLong || (!useDouble && !useFloat
                             && (occupies8Bytes(tl) || occupies8Bytes(tr)));
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
            case "==": w.emit(useDouble ? OpCode.DEQ : useFloat ? OpCode.FEQ : use8ByteEq ? OpCode.LEQ : OpCode.EQ); break;
            case "!=": w.emit(useDouble ? OpCode.DNEQ : useFloat ? OpCode.FNEQ : use8ByteEq ? OpCode.LNEQ : OpCode.NEQ); break;
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
        // #389 — conversión de REFERENCIA (`Cosa(o)`, `string(o)`): en bytecode
        // no es NADA. Un handle es un handle; lo único que cambió es el tipo con
        // el que el compilador lo mira a partir de aquí. Quién es conversión y
        // quién es constructor lo decidió el semántico (`info.refCasts`), que es
        // el que sabe qué constructores declara la clase: aquí no se repite esa
        // regla, para que no pueda divergir.
        if (info.refCasts.contains(c)) {
            emitExpr(c.args.get(0));
            /* #389, la mitad DINÁMICA (16-ago): el cast ya no es solo una
             * reinterpretación — COMPRUEBA. Si el objeto no es de la clase
             * destino (o no es una cadena, en `string(o)`), CHECKCAST lanza un
             * RuntimeError atrapable que nombra el tipo esperado, en vez del
             * 504 disfrazado que salía después. `Object(x)` no emite nada: un
             * Object puede llevar legítimamente una cadena (string→Object es
             * ensanchamiento), y exigirle "ser objeto" la rechazaría. */
            BpType t = info.exprTypes.get(c);
            if (t instanceof PrimitiveType
                    && ((PrimitiveType) t).tag == PrimitiveType.Kind.STRING) {
                w.emitCheckCastString(internString("string"));
            } else if (t instanceof ClassType) {
                Symbol.ClassSymbol cls = ((ClassType) t).cls;
                if (!"Object".equals(cls.name)) {
                    /* #444 — si la clase es de OTRO módulo, su descriptor no está
                     * en nuestra tabla de símbolos y `emitCheckCast` reventaba con
                     * una traza de Java («Clase 'X' no declarada para CHECKCAST»).
                     * Construirla sí funcionaba —eso va por el módulo de origen—,
                     * así que el crash sólo salía al BAJAR, que es justo lo que
                     * exige usar `Object` de comodín con la stdlib.
                     * La variante _EXT lleva el nombre cualificado y el linker
                     * parcha el cls_off, igual que `TRY_BEGIN_EXT` en un catch. */
                    if (porRef(cls)) {
                        w.emitCheckCastRef(refClase(cls), internString(cls.name));   // V7/H2 (#523)
                    } else if (cls.isExternal) {
                        StringBuilder qn = new StringBuilder();
                        if (cls.externalLibrary != null && !cls.externalLibrary.isEmpty())
                            qn.append(cls.externalLibrary).append('.');
                        qn.append(cls.externalModule).append('.').append(cls.name);
                        w.emitCheckCastExt(qn.toString(), internString(cls.name));
                    } else {
                        w.emitCheckCast(cls.name, internString(cls.name));
                    }
                }
            }
            return;
        }
        // Caso 1: callee es IdentifierExpr → función top-level o construction.
        if (c.callee instanceof IdentifierExpr) {
            Symbol sym = info.exprSymbols.get(c.callee);
            if (sym instanceof ClassSymbol) {
                // H5.a-E4: el semántico anotó en el NODO de llamada qué sobrecarga
                // de constructor eligió (null si solo hay una).
                Symbol ck = info.exprSymbols.get(c);
                emitConstruction((ClassSymbol) sym, c.args,
                        (ck instanceof FunctionSymbol) ? (FunctionSymbol) ck : null);
                return;
            }
            if (sym instanceof FunctionSymbol) {
                FunctionSymbol fs = (FunctionSymbol) sym;
                if (fs.isConstructor && fs.ownerClass != null) {
                    emitConstruction(fs.ownerClass, c.args, fs);
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
                if (fs.isIntrinsic) {   // H1 (20-sep): el bytecode del cuerpo `inline`, aquí
                    if (fs.inlineOps == null) { errors.add("intrínseca sin cuerpo inline: " + fs.name); return; }
                    emitInline(fs.inlineOps);
                    return;
                }
                if (fs.isExternal) {
                    w.emitCallExt(nombreExterno(fs), fs.externalFromPath);
                } else {
                    w.emitCall(emitName(fs));   // H5.a
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
                Symbol ck = info.exprSymbols.get(c);   // V7/H2 (#509): la sobrecarga elegida, si la hay
                emitConstruction((ClassSymbol) mSym, c.args, (ck instanceof FunctionSymbol) ? (FunctionSymbol) ck : null);
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
                    // mete el bytecode que viene en la interfaz del dueño (H1, 20-sep:
                    // `func … inline …`). No se añade entrada a la tabla de imports,
                    // así que el .mod del módulo dueño no es necesario en runtime.
                    if (fs.inlineOps == null) { errors.add("intrínseca sin cuerpo inline en la interfaz: " + fs.externalModule + "." + fs.name); return; }
                    emitInline(fs.inlineOps);
                    return;
                }
                w.emitCallExt(nombreExterno(fs), fs.externalFromPath);
                return;
            }
            if (fs.isStatic) {
                for (int i = 0; i < c.args.size(); i++) {
                    emitExpr(c.args.get(i));
                    if (i < fs.params.size()) coerceToTarget(c.args.get(i), fs.params.get(i).type);
                }
                emitStaticCall(fs);   // V7/H2 (#492-B): CALL local o CALL_EXT al alias si la clase es externa
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
                /* #451 — si el padre vive en OTRO modulo, la llamada tiene que ser
                 * cross-module: `emitCall` busca por nombre en la tabla LOCAL y
                 * reventaba con «Funcion no encontrada: Base.pon». Es el mismo
                 * hueco que #444 pero en el `super`, y le pasa a cualquiera que
                 * extienda una clase importada, no solo a la stdlib. */
                /* Lo externo aqui es la CLASE DUENA, no el simbolo del metodo:
                 * `fs.isExternal` sale false para un metodo heredado de una clase
                 * importada (lo comprobado: la excepcion saltaba al ESCRIBIR el
                 * fichero, o sea que el CALL se habia emitido con nombre local). */
                ClassSymbol duena = fs.ownerClass;
                // V10/J-3 (16-sep) — en MOD9 `super.m()` a un padre de otro módulo va por la ref SUPER
                // (clase ESTÁTICA del padre, clave): el cargador resuelve «el m efectivo del padre» por su
                // cadena real. Con el nombre exacto `__cls_m_Padre#m()`, si `m` SUBÍA al abuelo en otra
                // versión de Lib, era rechazo al enlazar donde Java corre (invokespecial resuelve por la cadena).
                ClassSymbol padre = (claseActual() != null) ? claseActual().baseClass : null;
                if (mod9() && padre != null && porRef(padre)) {
                    w.emitCallRef(w.requireRef(ModWriter.REF_SUPER, depRef(padre), claseRef(padre), claveDe(fs)));
                    return;
                }
                if (duena != null && duena.isExternal) {
                    StringBuilder qn = new StringBuilder();
                    if (duena.externalLibrary != null && !duena.externalLibrary.isEmpty())
                        qn.append(duena.externalLibrary).append('.');
                    /* `Modulo.slotKey`, NO `Modulo.Clase.metodo`: el slotKey ya
                     * codifica la clase, y con tres componentes el loader lee
                     * «BaseMod.Base» como nombre de MODULO y busca un .mod que no
                     * existe. Es la misma forma que usa `externalQualifiedName`. */
                    qn.append(duena.externalModule).append('.')
                      .append(crossModuleMethodName(duena, fs));
                    w.emitCallExt(qn.toString(), fromPathFor(qn.toString()));
                } else {
                    w.emitCall(fs.ownerClass.name + "." + emitName(fs));   // H5.a-E3
                }
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
            if (!fs.esVirtual() && !fs.isExternal) {   // #390: no virtual ⇒ CALL directo
                w.emitCall(fs.ownerClass.name + "." + emitName(fs));   // H5.a-E3
            } else {
                // BUG-6: numArgs = slots de 4 bytes (long/double = 2), no nº de args.
                emitInvokeVirtualSmart(receptorDe(ma.target, fs.ownerClass), fs.ownerClass, emitName(fs), claveDe(fs), argSlotCount(fs));
            }
            return;
        }
        errors.add("call con callee no soportado: " + c.callee.getClass().getSimpleName());
    }

    /**
     * len(x) — intrínseco global, bajada type-directed (cero coste de VM: baja a
     * ops que ya existen y están en paridad dual-VM):
     *   array  → ALEN
     *   string → CALL_BUILTIN STRLEN (longitud en codepoints)
     *   clase con length()/size() (List/Map…) → INVOKE_VIRTUAL al método.
     */
    /**
     * Despacha INVOKE_VIRTUAL al modo correcto según si la clase es externa.
     * Para clases locales usa la lookup normal por nombre de método; para
     * clases importadas (.bpi v4 ClassSig) usa el slot precalculado del stub.
     */
    /** V7/H2 (#523) — el despacho con la clave canónica: si el receptor (su clase
     *  ESTÁTICA, como javac) es de otro módulo o abierto, va por referencia; si no, por
     *  el camino de siempre sobre la clase que DECLARA el método. */
    private void emitInvokeVirtualSmart(ClassSymbol receptor, ClassSymbol declarante, String methodName,
                                        String clave, int nArgs) throws IOException {
        if (porRef(receptor)) { w.emitInvokeVirtualRef(refMet(receptor, clave), nArgs); return; }
        if (porRef(declarante)) { w.emitInvokeVirtualRef(refMet(declarante, clave), nArgs); return; }
        emitInvokeVirtualSmart(declarante, methodName, nArgs);
    }
    private void emitInvokeVirtualSmart(ClassSymbol cls, String methodName, int nArgs) throws IOException {
        if (porRef(cls)) throw new RuntimeException("emitInvokeVirtualSmart sin clave sobre una clase por referencia: " + cls.name + "." + methodName + " (bug del emisor)");
        if (cls.isExternal) {
            Integer slot = cls.externalMethodSlots.get(methodName);
            if (slot == null) {
                errors.add("método externo no encontrado en stub: " + cls.name + "." + methodName);
                return;
            }
            w.emitInvokeVirtualBySlot(slot, nArgs);
            // Robustez ABI (etapa 2): requiere el marker del dueño
            // "Modulo.Clase#metodo#slot" para que el linker valide POR NOMBRE que
            // el método sigue en ESE slot. Si el dueño lo movió (reordenó/quitó),
            // el marker no existe → error de link claro en vez de misdispatch.
            // Compat hacia adelante: añadir métodos al final no mueve los markers.
            if (cls.externalModule != null && !cls.externalModule.isEmpty()) {
                // #518(a) — CON el prefijo de library (qualifyForOwner), como el resto de
                // símbolos cross-module: sin él la VM-C buscaba `LibB.mod` en vez de
                // `com.acme.lib.LibB.mod` y no enlazaba (miVM lo tapaba con su clave corta).
                String marker = qualifyForOwner(cls, cls.name + "#" + methodName + "#" + slot);
                w.requireExternalSymbol(marker, fromPathFor(marker));
            }
        } else {
            // #174b cross-check: el slot que AotCEmitter computará (ClassSymbol.slotOf)
            // DEBE coincidir con el que ModWriter usa para el .mod (la autoridad).
            // Si divergen → error ruidoso AQUÍ (nunca rotura silenciosa de paridad
            // en el AOT). Es la red de seguridad de la decisión B.
            //
            // (H1 P3, 20-sep: ya no hay clases «built-in» que saltarse — las del lenguaje
            // viven en Core.bp y tienen AST como cualquier otra.)
            {
                int feSlot = cls.slotOf(methodName);
                int mwSlot = w.methodSlotOrMinus1(cls.name, methodName);
                if (feSlot >= 0 && mwSlot >= 0 && feSlot != mwSlot) {
                    errors.add("BUG interno #174b: slot divergente para " + cls.name + "."
                            + methodName + " (frontend=" + feSlot + ", ModWriter=" + mwSlot
                            + ") — revisar ClassSymbol.ensureMethodSlots");
                }
            }
            w.emitInvokeVirtual(cls.name, methodName, nArgs);
        }
    }

    private void emitArrayLit(ArrayLitExpr a) throws IOException {
        int n = a.elements.size();
        // Reservamos un temp único para no pisar otros __arr concurrentes.
        String tmp = "__arr_" + Integer.toHexString(System.identityHashCode(a));
        // H1.8/#19 — REGRESIÓN del 4→8B. `__arr_` sostiene EL ARRAY que acaba de crear
        // NEWARRAY = una REFERENCIA de 8B, y estaba `declareLocal(tmp)` = 4B A PELO:
        // NEWARRAY empuja 8, SET_LOCAL popeaba 4 → handle truncado + fuga de 4B.
        // AFECTA A TODO LITERAL DE ARRAY del lenguaje (`[1,2,3]` en cualquier posición).
        // Lo delató `samples/arraytest.bp` (`var primos: integer[] := [2,3,5,7,11]`),
        // que además es el ÚNICO sample del repo con `enum`, y llevaba roto sin que
        // nadie lo ejecutara. Es ref POR CONSTRUCCIÓN (el temp no tiene BpType a mano:
        // el tipo de elemento del literal ni siquiera se conoce aquí — ver el TODO de
        // abajo), así que va por el carril de ref, no por ByType.
        declareLocalRef(tmp);

        // #442 — EL ANCHO DEL ELEMENTO. Antes iba NEWARRAY + ASTORE a pelo, o sea
        // 4 bytes por casilla SIEMPRE, y eso se llevaba por delante todo elemento
        // de 8 B: `long[]` daba 0 EN SILENCIO (familia de #385), `double[]`
        // reventaba, y `string[]`/clases daban «No space in heap» o un receptor
        // null. Las dos VMs igual → era del compilador.
        //
        // El `TODO` que había aquí decía «si supiéramos el tipo de contexto»: ya se
        // sabe. `analyzeArrayLit` lo calcula (con el tipo esperado como pista) y lo
        // deja en `info.exprTypes`. Con él se hace lo MISMO que ya hace una
        // asignación normal a un elemento (ver ASSIGN sobre IndexExpr): coerce al
        // tipo del elemento + store del ancho correcto.
        //
        // ⚠️ Y las REFERENCIAS no van por opcode: `NEWARRAY_I64` crea un
        // `TYPE_ARRAY_I64` de 8 bytes OPACOS que el GC no traza. Un array de refs
        // tiene que ser `TYPE_ARRAY_REF`, y eso sólo lo da el builtin
        // NEW_REF_ARRAY. Confundirlos no truncaría: dejaría los elementos sin
        // trazar → use-after-free.
        BpType arrT = info.exprTypes.get(a);
        BpType el   = (arrT instanceof ArrayType) ? ((ArrayType) arrT).element : null;
        // El predicado tiene que ser isRefType, NO «no es primitivo»: en BP
        // `string` ES un PrimitiveType y a la vez una referencia de heap (ver
        // isRefType, que lo dice y explica por qué no se toca isReference()).
        // Con la versión ingenua salía NEWARRAY (4 B/casilla) con ASTORE_I64
        // (8 B): el elemento 0 pisaba al 1 y el 1 se escribía fuera — `[0]` se
        // leía y `[1]` salía VACÍO. Lo delató el desensamblado, no el síntoma.
        boolean elemEsRef = isRefType(el);

        emitInt(n);
        if (elemEsRef)      emitBuiltinCall(Builtin.NEW_REF_ARRAY);
        else if (el != null) w.emit(newarrayOpForElement(el));
        else                 w.emit(OpCode.NEWARRAY);
        w.emitSetLocal(tmp);

        OpCode store = (el != null) ? astoreOpForElement(el) : OpCode.ASTORE;
        for (int i = 0; i < n; i++) {
            w.emitGetLocal(tmp);
            emitInt(i);
            emitExpr(a.elements.get(i));
            if (el != null) coerceToTarget(a.elements.get(i), el);
            w.emit(store);
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
            // H1.2a: elemento ref/any (objeto/string/array anidado/any) = 8 bytes.
            if (occupies8Bytes(el)) return OpCode.ALOAD_I64;
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
        if (t instanceof ArrayType) return astoreOpForElement(((ArrayType) t).element);
        return OpCode.ASTORE;
    }

    /** Variante de {@link #astoreOpFor} cuando ya se conoce el tipo de
     *  ELEMENTO (L8 v3: arrays fijos locales, sin expr de contexto). */
    private OpCode astoreOpForElement(BpType el) {
        if (el instanceof PrimitiveType) {
            switch (((PrimitiveType) el).tag) {
                case UINT8: case INT8:    return OpCode.ASTORE_I8;
                case UINT16: case INT16:  return OpCode.ASTORE_I16;
                case LONG:                return OpCode.ASTORE_I64;   // H1.2 (V2)
                case DOUBLE:              return OpCode.ASTORE_I64;   // H1.3 (V2)
                default:                  break;
            }
        }
        // H1.2a: elemento ref/any (objeto/string/array anidado/any) = 8 bytes.
        if (occupies8Bytes(el)) return OpCode.ASTORE_I64;
        return OpCode.ASTORE;
    }

    /** L8 v3 — opcode de alocación según el ancho del elemento (espejo de
     *  astoreOpForElement; todos zero-inicializan en ambas VMs). */
    private OpCode newarrayOpForElement(BpType el) {
        if (el instanceof PrimitiveType) {
            switch (((PrimitiveType) el).tag) {
                case UINT8: case INT8:    return OpCode.NEWARRAY_I8;
                case UINT16: case INT16:  return OpCode.NEWARRAY_I16;
                case LONG: case DOUBLE:   return OpCode.NEWARRAY_I64;
                default:                  break;
            }
        }
        return OpCode.NEWARRAY;
    }

    /** L8 v3 — N del `tipo[N]` de una declaración LOCAL, o null si no es un
     *  array de tamaño fijo válido (el semántico ya diagnosticó los inválidos). */
    private static Integer localFixedArraySize(VarDecl vd) {
        if (!(vd.type instanceof ArrayTypeRef)) return null;
        ArrayTypeRef at = (ArrayTypeRef) vd.type;
        if (at.size == null) return null;
        Object n = SemanticAnalyzer.constLiteralValue(at.size);
        return (n instanceof Long && (Long) n > 0) ? (int) (long) (Long) n : null;
    }

    /**
     * Sintetiza la clase built-in RuntimeError en el .mod actual. Layout:
     *   class RuntimeError { var msg: string }
     *   RuntimeError.__init(this, msg) { this.msg := msg }
     * Necesario para que `throw RuntimeError(msg)` y `catch err: RuntimeError` tengan
     * un descriptor de clase real en data block.
     */
    /**
     * H5.1.a — Clase raíz Object. Toda clase (de usuario o stdlib, EXCEPTO
     * Thread y sus subclases — `root class Thread` en Core.bp) desciende implícita-
     * mente de Object, cuya vtable fija dos slots:
     *   slot 0 = toString(): string
     *   slot 1 = compareTo(other): integer
     * Sin campos (numFields=0), parentOffset=0 (raíz). Se sintetiza la PRIMERA
     * para que cualquier otra clase pueda copiarla como parent LOCAL (addClass
     * "Object"): así toString/compareTo ocupan los slots 0/1 de TODA instancia
     * y INVOKE_VIRTUAL los despacha polimórficamente.
     *
     * Cuerpos por defecto (H5.1.b):
     *   toString  → "object@" + dirección (decimal del ref); sirve para depurar.
     *               La dirección difiere entre VMs (aceptado por el usuario): el
     *               output de un toString NO-overrideado no es byte-idéntico.
     *   compareTo → placeholder (return 0) de momento; H5.1.c lo hace throw
     *               (requiere ser invocable desde fuente para poder probarlo).
     */
    /** Helper: emite CALL_BUILTIN <id>. */
    private void emitBuiltinCall(Builtin b) {
        try {
            w.emit(OpCode.CALL_BUILTIN);
            w.emitShort((short) b.id);
        } catch (IOException e) {
            errors.add("emit CALL_BUILTIN(" + b.name() + "): " + e.getMessage());
        }
    }

    /** FunctionSymbol "fake" para los métodos sintetizados de stdlib. */
    private FunctionSymbol makeSynthFs(String name, BpType returnType) {
        FunctionSymbol fs = new FunctionSymbol(name, true, false, false, null, null);
        fs.returnType = returnType;
        return fs;
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
        if (porRef((ClassSymbol) cls)) { w.emitInstanceOfRef(refClase((ClassSymbol) cls)); return; }   // V7/H2 (#523, D7)
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

    /** H1 (20-sep) — el bytecode escrito en el cuerpo `inline` de una intrínseca, tal cual, en el
     *  sitio de llamada (los args ya están en la pila; deja UN valor, como CALL_BUILTIN). El
     *  semántico ya comprobó mnemónico y operandos. */
    private void emitInline(List<InlineStmt> ops) throws IOException {
        for (InlineStmt in : ops) {
            OpCode op = OpCode.valueOf(in.mnemonic);
            w.emit(op);
            switch (op.operandKind) {
                case NONE: break;
                case IMM_I32: w.emitInt(in.operands.get(0).intValue()); break;
                case IDX_U16: w.emitShort((short) in.operands.get(0).intValue()); break;
                case IMM_U8:  w.emitByte(in.operands.get(0).intValue()); break;
                default: errors.add("inline " + in.mnemonic + ": operandos " + op.operandKind + " no soportados"); return;
            }
        }
    }

    private void emitSuperCall(SuperCallExpr sc) throws IOException {
        // super(args) dentro de un constructor: llama directamente al __init del padre.
        FuncScope sc2 = scopeStack.peek();
        if (sc2 == null || sc2.fs.ownerClass == null || sc2.fs.ownerClass.baseClass == null) {
            errors.add("super(...) fuera de constructor con padre"); return;
        }
        ClassSymbol parent = sc2.fs.ownerClass.baseClass;
        // V7/H2 (#509) — la sobrecarga que eligió el semántico por firma (anotada en
        // el nodo); sin anotación, la única/heredada. Se llama por la clase que la
        // DECLARA (#513: el padre puede no tener ctor propio) — local por nombre, o
        // CALL_EXT a `__cls_init_<Owner>$clave` si vive en otro módulo: el símbolo
        // no tiene puntos, así que el loader lo lee como del módulo dueño, y `this`
        // del hijo vale como ref del padre porque los campos heredados ocupan los
        // slots 0..N-1.
        Symbol anotado = info.exprSymbols.get(sc);
        FunctionSymbol parentCtor = (anotado instanceof FunctionSymbol) ? (FunctionSymbol) anotado
                                                                        : parent.findConstructor();
        w.emitGetParam("this");
        for (int i = 0; i < sc.args.size(); i++) {
            emitExpr(sc.args.get(i));
            if (parentCtor != null && i < parentCtor.params.size()) {
                coerceToTarget(sc.args.get(i), parentCtor.params.get(i).type);
            }
        }
        if (parentCtor == null) { errors.add("super(...): la clase base '" + parent.name + "' no tiene constructor"); return; }
        emitCallInitOf(parentCtor);
    }

    /**
     * Emite la construcción de un objeto:
     *   NEW_OBJECT Cls; SET_LOCAL __newref;
     *   GET_LOCAL __newref; push args...; CALL Cls.__init;
     *   SET_LOCAL __discard; GET_LOCAL __newref   ← restaura la ref para la expresión
     */
    private void emitConstruction(ClassSymbol cls, List<IExpr> args) throws IOException {
        emitConstruction(cls, args, null);
    }

    /** H5.a-E4 — {@code chosenCtor} = la sobrecarga de constructor que eligió el
     *  semántico por firma (null ⇒ la única / la heredada). */
    private void emitConstruction(ClassSymbol cls, List<IExpr> args, FunctionSymbol chosenCtor) throws IOException {
        // #325 — `Thread(obj::metodo(args) [, pila])`: no se construye un Thread
        // pelado sino la subclase sintetizada que lleva la llamada dentro.
        if ("Thread".equals(cls.name) && !args.isEmpty()
                && args.get(0) instanceof Ast.BoundCallExpr) {
            emitThreadOfBoundCall(args);
            return;
        }
        // L2: si la clase vive en otro módulo, no podemos emitir NEW_OBJECT
        // local (no tenemos su class_ptr). En su lugar invocamos la factoría
        // `__cls_new_<Cls>(args)` que el módulo dueño sintetiza: hace el
        // NEW_OBJECT en SU contexto y devuelve el ref.
        if (cls.isExternal) {
            // Empujar args en orden — la factoría tiene la misma firma que el ctor.
            // V7/H2 (#509/#513): el ctor ELEGIDO (anotado por el semántico) o el
            // efectivo (findConstructor: puede ser heredado); la factoría del dueño
            // lleva el mismo sufijo de clave (`__cls_new_<Cls>$clave`).
            FunctionSymbol ctor = (chosenCtor != null) ? chosenCtor : cls.findConstructor();
            for (int i = 0; i < args.size(); i++) {
                emitExpr(args.get(i));
                if (ctor != null && i < ctor.params.size())
                    coerceToTarget(args.get(i), ctor.params.get(i).type);
            }
            String qn = qualifyForOwner(cls, crossModuleNewName(cls, ctor));
            w.emitCallExt(qn, cls.externalFromPath);
            // La factoría deja el ref en la pila como retorno; lo dejamos ahí.
            return;
        }

        // Nombres ÚNICOS por construcción para que llamadas anidadas como
        // Caja(Animal("Rex")) no compartan slot (el __newref del outer no se
        // pisa cuando se emite la construcción del arg).
        int id = tempCounter++;
        String newref  = "__newref_" + id;
        String discard = "__discard_" + id;
        declareLocalLong(newref);   // H1.2a: el ref del objeto nuevo = 8 bytes
        declareLocal(discard);
        w.emitNewObject(cls.name);
        w.emitSetLocal(newref);

        // H5.a-E4 — con constructores sobrecargados, el elegido lo decidió el
        // semántico (por firma) y lo anotó; si no hay anotación, el único/heredado.
        FunctionSymbol ctor = (chosenCtor != null) ? chosenCtor : cls.findConstructor();
        if (ctor != null) {
            w.emitGetLocal(newref);
            for (int i = 0; i < args.size(); i++) {
                emitExpr(args.get(i));
                if (i < ctor.params.size()) coerceToTarget(args.get(i), ctor.params.get(i).type);
            }
            // El ctor puede ser heredado: por SU clase dueña — que puede vivir en otro
            // módulo (hija local sin ctor de una base importada: petaba el ModWriter
            // con «Función no encontrada: Base.__init», sin línea; 14-sep).
            emitCallInitOf(ctor);
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

        // Const cross-module con valor literal conocido: inlinar (width-aware:
        // long/double importadas emiten LPUSH/DPUSH, no truncan a 4 bytes).
        if (mSym instanceof ConstSymbol) {
            ConstSymbol c = (ConstSymbol) mSym;
            if (c.literalValue != null) {
                emitConstLiteral(c);
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
            // L6 — static property: Cls.prop lee via CALL al getter cualificado
            // (el target es el NOMBRE de la clase, no una instancia — no se evalúa).
            if (ps.isStatic) {
                emitStaticGetter(ps);   // V7/H2 (#492-B)
                return;
            }
            emitExpr(ma.target);
            emitInvokeVirtualSmart(receptorDe(ma.target, ps.ownerClass), ps.ownerClass, "get" + capitalize(ps.name), claveGetter(ps.name, ps.type), 0);
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
                emitGetFieldDe(v, receptorDe(ma.target, v.ownerClass));   // V7/H2: por ranura si la clase es externa
            }
            return;
        }
        errors.add("member-access no soportado: " + ma.member);
    }

    /**
     * Inlinea un literal cuyo valor proviene de una const importada (.bpi).
     * El valor llega como Long/Double/String/Boolean.
     */
    /** L8 v2 — inline width-aware: el TIPO del símbolo decide el opcode
     *  (LPUSH/DPUSH de 8 bytes para long/double; FPUSH para float). La
     *  variante (Object) de abajo no distingue ancho y truncaba un long
     *  importado a int — esta es la entrada preferente. */
    private void emitConstLiteral(ConstSymbol c) throws IOException {
        Object v = c.literalValue;
        PrimitiveType.Kind k = (c.type instanceof PrimitiveType) ? ((PrimitiveType) c.type).tag : null;
        if (v instanceof Number) {
            if (k == PrimitiveType.Kind.LONG) {
                w.emit(OpCode.LPUSH);
                w.emitLong(((Number) v).longValue());
                return;
            }
            if (k == PrimitiveType.Kind.DOUBLE) {
                w.emit(OpCode.DPUSH);
                w.emitLong(Double.doubleToRawLongBits(((Number) v).doubleValue()));
                return;
            }
            if (k == PrimitiveType.Kind.FLOAT) {
                w.emitPushFloat(((Number) v).floatValue());
                return;
            }
        }
        emitConstLiteral(v);   // integer/narrow/bool/string (y fallback sin tipo)
    }

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

    static boolean isLong(BpType t) {   // H1.2 (V2)
        return t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.LONG;
    }

    static boolean isDouble(BpType t) {   // H1.3 (V2)
        return t instanceof PrimitiveType && ((PrimitiveType) t).tag == PrimitiveType.Kind.DOUBLE;
    }

    /** H1.3 — tipos NUMÉRICOS de 8 bytes (2 slots): long y double comparten storage. */
    static boolean is8Byte(BpType t) {
        return isLong(t) || isDouble(t);
    }

    /** H1.2a (V4) — ¿ocupa 8 bytes / 2 slots en pila, frame, campo y array?
     *  Numérico de 8 bytes (long/double) O una REFERENCIA (object/array/string):
     *  con el ensanchado de refs 4→8B, toda ref viaja por el carril de 8 bytes.
     *  Es el predicado de ANCHO; is8Byte sigue siendo solo el numérico. */
    static boolean occupies8Bytes(BpType t) {
        // H1.2a (V4): `any` = 8 bytes — es un valor opaco que contiene o un int
        // (zero-extended) o una ref flat; en el modelo plano ocupa 2 slots como
        // las refs. Sin esto, List/SyncList (any de punta a punta) truncan.
        return is8Byte(t) || isRefType(t) || (t instanceof BpType.AnyType)
               || (t instanceof BpType.NullType);   // #11: null es una ref = 8B (así no se re-ensancha en coerceToTarget)
    }

    /** BUG-6: nº de slots de 4 bytes que ocupan los argumentos (excluye `this`).
     *  INVOKE_VIRTUAL localiza el receptor en sp-4-numArgs*4, así que su operando
     *  numArgs debe contar SLOTS, no argumentos: un parámetro long/double ocupa 2
     *  slots. Para métodos all-4-byte coincide con el nº de argumentos. */
    private int argSlotCount(FunctionSymbol fs) {
        int slots = 0;
        for (int i = 0; i < fs.params.size(); i++) {
            slots += occupies8Bytes(fs.params.get(i).type) ? 2 : 1;
        }
        return slots;
    }

    // ============================================================
    // TUPLAS (retorno múltiple) — objeto sintético `__Tuple_<sig>` con N campos.
    // El nombre codifica el LAYOUT (anchos/ref-ness), no los tipos exactos, así
    // dos tuplas con misma forma comparten clase. Cero opcodes nuevos.
    // ============================================================
    private final java.util.LinkedHashMap<String, BpType.TupleType> tupleShapes =
            new java.util.LinkedHashMap<>();

    private static String tupleElemCode(BpType t) {
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case LONG:    return "l";
                case DOUBLE:  return "d";
                case FLOAT:   return "f";
                case BOOLEAN: return "b";
                case STRING:  return "s";
                default:      return "i";   // integer + estrechos → slot i32
            }
        }
        return "r";   // class/array/any/Object → ref de 4 bytes (GC traza)
    }
    private static String tupleClassName(BpType.TupleType tt) {
        StringBuilder sb = new StringBuilder("__Tuple_");
        for (BpType e : tt.elements) sb.append(tupleElemCode(e));
        return sb.toString();
    }

    /** Pre-pase: registra la forma de cada tupla usada en el módulo, tanto en el
     *  lado productor (funciones que la devuelven) como consumidor (llamadas que
     *  se desempaquetan, incl. cross-module: la forma viene del .bpi importado).
     *  Sintetizar la clase en CUALQUIER módulo que la toque es seguro: es un
     *  contenedor transparente con layout determinista por su firma. */
    private void collectTupleShapes() {
        for (ITopLevelDecl d : moduleAst.defs) {
            if (d instanceof FuncDef) registerTupleIfAny((FuncDef) d);
            else if (d instanceof ClassDef) {
                for (ITopLevelDecl m : ((ClassDef) d).members)
                    if (m instanceof FuncDef) registerTupleIfAny((FuncDef) m);
            }
        }
        for (BpType t : info.exprTypes.values())
            if (t instanceof BpType.TupleType)
                tupleShapes.put(tupleClassName((BpType.TupleType) t), (BpType.TupleType) t);
    }
    private void registerTupleIfAny(FuncDef fn) {
        Symbol s = info.declSymbols.get(fn);
        if (s instanceof FunctionSymbol) {
            BpType rt = ((FunctionSymbol) s).returnType;
            if (rt instanceof BpType.TupleType)
                tupleShapes.put(tupleClassName((BpType.TupleType) rt), (BpType.TupleType) rt);
        }
    }

    /** Sintetiza una clase oculta por forma de tupla usada en el módulo. */
    private void synthesizeTupleClasses() throws IOException {
        for (java.util.Map.Entry<String, BpType.TupleType> e : tupleShapes.entrySet()) {
            String cls = e.getKey();
            BpType.TupleType tt = e.getValue();
            w.addClass(cls, null);   // H1 P3: root — una tupla no tiene métodos ni padre (Object ya no es local)
            for (int i = 0; i < tt.elements.size(); i++) {
                BpType el = tt.elements.get(i);
                w.declareField("_" + i, isGcRef(el), false, occupies8Bytes(el));   // #14: elem any → GC lo traza
                if (mod9()) w.setFieldMeta("_" + i, claveCampo("_" + i, el), false);
            }
            w.endClass();
        }
    }

    /** Emite la construcción de una tupla `(e1, e2, ...)` y deja su ref en pila. */
    private void emitTupleExpr(Ast.TupleExpr te) throws IOException {
        BpType bt = info.exprTypes.get(te);
        if (!(bt instanceof BpType.TupleType)) { errors.add("tupla sin tipo resuelto"); return; }
        BpType.TupleType tt = (BpType.TupleType) bt;
        String cls = tupleClassName(tt);
        String temp = "__tup_" + (stringPoolCounter++);
        declareLocalLong(temp);   // #281: el ref del objeto-tupla es un handle de 8 bytes (no 4)
        w.emitNewObject(cls);
        w.emitSetLocal(temp);
        for (int i = 0; i < te.elements.size(); i++) {
            w.emitGetLocal(temp);
            emitExpr(te.elements.get(i));
            coerceToTarget(te.elements.get(i), tt.elements.get(i));
            w.emitSetField(cls, "_" + i);
        }
        w.emitGetLocal(temp);   // ref de la tupla = valor de retorno
    }

    /** Emite `{ a, b, ... } := call()`: evalúa la llamada, guarda la ref y
     *  desempaqueta cada campo en su lvalue (o lo descarta si es `_`). */
    private void emitDestructAssign(Ast.DestructAssignStmt s) throws IOException {
        BpType bt = info.exprTypes.get(s.value);
        if (!(bt instanceof BpType.TupleType)) { errors.add("destructuring sin tipo tupla"); return; }
        BpType.TupleType tt = (BpType.TupleType) bt;
        String cls = tupleClassName(tt);
        emitExpr(s.value);                       // ref de la tupla en pila
        String temp = "__dst_" + (stringPoolCounter++);
        declareLocalLong(temp);   // #281: el ref del objeto-tupla es un handle de 8 bytes (no 4)
        w.emitSetLocal(temp);
        int n = Math.min(s.targets.size(), tt.elements.size());
        for (int i = 0; i < n; i++) {
            IExpr tg = s.targets.get(i);
            if (tg instanceof IdentifierExpr && "_".equals(((IdentifierExpr) tg).name)) continue;
            BpType targetT = info.exprTypes.get(tg);
            BpType vt = (targetT != null) ? targetT : tt.elements.get(i);
            // Extrae el elemento (con ensanchado) a un temp; luego lo guarda en el lvalue.
            String valLocal = "__de_" + (stringPoolCounter++);
            declareLocalByType(valLocal, vt, null);   // H1.8. #281: elemento de 8B (string/long/double/ref/tupla)
            w.emitGetLocal(temp);
            w.emitGetField(cls, "_" + i);        // is8-aware → GET_FIELD_LONG si procede
            if (targetT != null) emitWiden(tt.elements.get(i), targetT);
            w.emitSetLocal(valLocal);
            storeToTupleTarget(tg, valLocal, vt);
        }
    }

    /** H8.2 — almacena el valor (alojado en el local `valLocal`) en el lvalue destino
     *  de un desempaquetado: variable simple, índice de array (arr[i]), o campo /
     *  property de objeto (obj.x). Reusa el mismo cableado que `emitAssign`. */
    private void storeToTupleTarget(IExpr tg, String valLocal, BpType vt) throws IOException {
        if (tg instanceof IdentifierExpr) {
            Symbol sym = info.exprSymbols.get(tg);
            if (sym == null) { errors.add("destino de desempaquetado sin símbolo"); return; }
            w.emitGetLocal(valLocal);
            storeToSymbol(sym, ((IdentifierExpr) tg).name);
            return;
        }
        if (tg instanceof IndexExpr) {
            IndexExpr ix = (IndexExpr) tg;
            emitExpr(ix.target);
            emitExpr(ix.index);
            w.emitGetLocal(valLocal);
            w.emit(astoreOpFor(ix.target));       // ancho del elemento (H1.1b)
            return;
        }
        if (tg instanceof MemberAccessExpr) {
            MemberAccessExpr ma = (MemberAccessExpr) tg;
            Symbol mSym = info.exprSymbols.get(ma);
            if (mSym instanceof PropertySymbol) {
                PropertySymbol ps = (PropertySymbol) mSym;
                if (ps.ownerClass == null) {                 // property de módulo
                    w.emitGetLocal(valLocal);
                    if (ps.isExternal) w.emitCallExt(buildExternalAccessorName(ps, true), ps.externalFromPath);
                    else               w.emitCall(moduleSetterName(ps.name));
                    declareLocal("__discard"); w.emitSetLocal("__discard");
                } else if (ps.isStatic) {                    // L6: static property
                    w.emitGetLocal(valLocal);
                    emitStaticSetter(ps);   // V7/H2 (#492-B)
                    declareLocal("__discard"); w.emitSetLocal("__discard");
                } else {                                     // property de clase (instancia)
                    emitExpr(ma.target);
                    w.emitGetLocal(valLocal);
                    emitInvokeVirtualSmart(receptorDe(ma.target, ps.ownerClass), ps.ownerClass, "set" + capitalize(ps.name), claveSetter(ps.name, ps.type), occupies8Bytes(vt) ? 2 : 1);
                    declareLocal("__discard"); w.emitSetLocal("__discard");
                }
                return;
            }
            if (mSym instanceof VarSymbol) {
                VarSymbol v = (VarSymbol) mSym;
                if (v.isStatic) {
                    w.emitGetLocal(valLocal);
                    w.emitSetGlobal(v.ownerClass.name + "." + v.name);
                } else {
                    emitExpr(ma.target);
                    w.emitGetLocal(valLocal);
                    emitSetFieldAuto(v.ownerClass.name, v.name);
                }
                return;
            }
            errors.add("destino de desempaquetado (member) no soportado: " + ma.member);
            return;
        }
        errors.add("destino de desempaquetado no soportado: " + tg.getClass().getSimpleName());
    }

    /** Conversión de ensanchado entre primitivos (mismo criterio que coerceToTarget),
     *  con tipos de origen/destino explícitos (no hay nodo IExpr de origen). */
    private void emitWiden(BpType from, BpType to) throws IOException {
        asumirSubtipo(from, to);   // V7/H2 (#523, A1): `{b, _} := f()` con b: Lib.Base
        if (!(to instanceof PrimitiveType) || !(from instanceof PrimitiveType)) return;
        PrimitiveType pf = (PrimitiveType) from, pt = (PrimitiveType) to;
        if (pf.tag == pt.tag) return;
        if (pt.tag == PrimitiveType.Kind.LONG && pf.isIntegerLike()) { w.emit(OpCode.I32_TO_I64); return; }
        if (pt.tag == PrimitiveType.Kind.DOUBLE) {
            if (pf.isIntegerLike())               { w.emit(OpCode.I2D); return; }
            if (pf.tag == PrimitiveType.Kind.LONG)  { w.emit(OpCode.L2D); return; }
            if (pf.tag == PrimitiveType.Kind.FLOAT) { w.emit(OpCode.F2D); return; }
        }
        if (pt.tag == PrimitiveType.Kind.FLOAT) {
            if (pf.isIntegerLike())               { w.emit(OpCode.I2F); return; }
            if (pf.tag == PrimitiveType.Kind.LONG)  { w.emit(OpCode.L2F); return; }
        }
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
        asumirSubtipo(srcT, target);   // V7/H2 (#523, A1): un upcast de referencia es un no-op en bytecode, pero se ANOTA
        // H1.2a (V4) — destino `any` = 8 bytes. Un valor de 4 bytes (int/narrow/
        // bool) debe ensancharse a 8B para ocupar los 2 slots que el llamante
        // reserva (numArgs cuenta `any` como 2). Sin esto el arg viaja 4B, el sp
        // queda -4 y el receptor del INVOKE_VIRTUAL se lee desalineado (null).
        // Lo ya de 8B (long/double/ref/string/any) viaja tal cual (flat).
        if (target instanceof BpType.AnyType) {
            if (!occupies8Bytes(srcT)) w.emit(OpCode.I32_TO_I64);
            return;
        }
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
        // H5.1.c-2 — objeto en concatenación `+`: auto-toString() polimórfico
        // (slot 0) con guarda null→"null". Mismo helper que usa print.
        if (src instanceof ClassType) {
            emitObjectToStringOnStack(((ClassType) src).cls);
            return;
        }
        if (src instanceof EnumType) {   // #522 — su entero, como `print c`
            w.emit(OpCode.CALL_BUILTIN);
            w.emitShort((short) Builtin.INT_TO_STRING.id);
            return;
        }
        if (!(src instanceof PrimitiveType)) return;
        switch (((PrimitiveType) src).tag) {
            case STRING: return;
            case INTEGER:
            // L8 v2 — narrow (byte/word/int8/int16): en la pila ya viajan
            // ensanchados a i32 (ALOAD_U8/I8/U16/I16 y GET_GLOBAL extienden),
            // así que INT_TO_STRING los convierte tal cual. Sin este case el
            // concat dejaba basura (sin conversión) → string vacío/corrupto.
            case UINT8: case INT8: case UINT16: case INT16:
                w.emit(OpCode.CALL_BUILTIN);
                w.emitShort((short) Builtin.INT_TO_STRING.id);
                break;
            case FLOAT:
                w.emit(OpCode.CALL_BUILTIN);
                w.emitShort((short) Builtin.FLOAT_TO_STRING.id);
                break;
            // L13 — long/double: builtins de 8 bytes con formateo canónico
            // GAP-4 (double) / decimal (long). Antes no existían y el concat
            // corrompía la pila.
            case LONG:
                w.emit(OpCode.CALL_BUILTIN);
                w.emitShort((short) Builtin.LONG_TO_STRING.id);
                break;
            case DOUBLE:
                w.emit(OpCode.CALL_BUILTIN);
                w.emitShort((short) Builtin.DOUBLE_TO_STRING.id);
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
            else if (v.ownerClass == null) {
                // L8 v2 — array de tamaño fijo: la var ES el array en el data
                // block; su "valor" es la DIRECCIÓN del símbolo (LEA), igual
                // que un literal string. ALOAD/len operan sobre ella tal cual.
                if (v.fixedArraySize >= 0) w.emitLeaGlobal(v.name);
                else w.emitGetGlobal(v.name);
            }
            else if (v.isStatic) w.emitGetGlobal(v.ownerClass.name + "." + v.name);
            else {
                // Campo de instancia con implicit this (estamos en un método de la clase).
                w.emitGetParam("this");
                emitGetFieldDe(v, claseActual());   // V7/H2: heredado de una clase externa → por ranura
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
            if (ps.isStatic) {
                // L6: CALL al getter cualificado (sin this).
                emitStaticGetter(ps);   // V7/H2 (#492-B)
                return;
            }
            w.emitGetParam("this");
            emitInvokeVirtualSmart(claseActual() != null ? claseActual() : ps.ownerClass, ps.ownerClass, "get" + capitalize(ps.name), claveGetter(ps.name, ps.type), 0);
        } else if (sym instanceof ConstSymbol) {
            ConstSymbol c = (ConstSymbol) sym;
            // L8 v2 — valor literal conocido: INLINE (PUSH/LPUSH/DPUSH/LEA),
            // el mismo camino que las const importadas de .bpi. Cubre lo que
            // el data symbol nunca cubrió (string/long/double/bool/negativos).
            if (c.literalValue != null) {
                emitConstLiteral(c);
                return;
            }
            if (c.isLocal) {
                // const local con init no-literal: valor de única asignación
                // en su slot local (emitLocalConstDecl).
                w.emitGetLocal(c.name);
                return;
            }
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
            else if (v.ownerClass == null) {
                // L8 v2 — defensa: el semántico ya bloquea reasignar un array
                // de tamaño fijo (no hay slot que escribir).
                if (v.fixedArraySize >= 0) {
                    errors.add("array de tamaño fijo de módulo no reasignable: " + v.name);
                    return;
                }
                w.emitSetGlobal(v.name);
            }
            else if (v.isStatic) w.emitSetGlobal(v.ownerClass.name + "." + v.name);
            else {
                // Campo de instancia con implicit this. Pila: ..., val. Necesitamos ref, val.
                // H1.2a: si el campo es long/double/ref, el temp es de 8 bytes (si no,
                // el valor 8B se trunca a 4B al pasar por el temp → SET_FIELD corrupto).
                boolean v8 = occupies8Bytes(v.type);
                String temp = "__stf_" + (stringPoolCounter++);
                if (v8) declareLocalLong(temp); else declareLocal(temp);
                w.emitSetLocal(temp);
                w.emitGetParam("this");
                w.emitGetLocal(temp);
                emitSetFieldDe(v, claseActual());   // V7/H2
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
            if (ps.isStatic) {
                // L6: pila ..., val → CALL al setter cualificado (consume val).
                emitStaticSetter(ps);   // V7/H2 (#492-B)
                declareLocal("__discard");
                w.emitSetLocal("__discard");
                return;
            }
            // Pila: ..., val. Necesitamos this, val para INVOKE_VIRTUAL setX.
            // BUG-6: si la property es long/double, el temp y el numArgs son de 8 bytes / 2 slots.
            boolean ps8 = occupies8Bytes(ps.type);
            String temp = "__stp_" + (stringPoolCounter++);
            if (ps8) declareLocalLong(temp); else declareLocal(temp);
            w.emitSetLocal(temp);
            w.emitGetParam("this");
            w.emitGetLocal(temp);
            emitInvokeVirtualSmart(claseActual() != null ? claseActual() : ps.ownerClass, ps.ownerClass, "set" + capitalize(ps.name), claveSetter(ps.name, ps.type), ps8 ? 2 : 1);
            declareLocal("__discard");
            w.emitSetLocal("__discard");
        } else if (sym instanceof ConstSymbol) {
            // El semántico rechaza asignar a const; si llegamos aquí es un bug
            // del frontend (las const ya no materializan slot escribible).
            errors.add("interno: store a const '" + sym.name + "'");
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
        declareParamRef("a");   // H1.8: string = ref, por el carril de ref (era un 8 a mano; #9 vivió aquí)
        declareParamRef("b");   // H1.8: idem
        w.declareLocal("la");
        w.declareLocal("lb");
        w.declareLocalLong("out");   // H1.2a: byte[] ref = 8 bytes
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
        w.emitLRet();   // H1.2a: devuelve un ref (8 bytes)
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
        declareParamRef("a");   // H1.8: string = ref, por el carril de ref (era un 8 a mano; #9 vivió aquí)
        declareParamRef("b");   // H1.8: idem
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
