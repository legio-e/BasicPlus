// ============================================================
// Symbol.java
// Tabla de símbolos: módulos, clases, funciones, propiedades,
// vars, consts, enums, parámetros, locales.
//
// Los símbolos son CLASES con identidad y referencias cíclicas
// (una clase apunta a su tipo, su tipo apunta a la clase).
//
// Subtipos como nested static classes en este archivo.
// El 'Scope' va en su propia clase nested aquí también.
// ============================================================
package basicplus.frontend;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public abstract class Symbol {

    public final String name;
    public final int line;
    public final int column;
    public Ast.Node decl;     // nodo del AST que originó el símbolo (mutable porque se setea tras construir)

    protected Symbol(String name, int line, int column) {
        this.name = name;
        this.line = line;
        this.column = column;
    }

    @Override public String toString() {
        return getClass().getSimpleName() + " '" + name + "'";
    }

    // ============================================================
    // Módulo (raíz)
    // ============================================================
    public static final class ModuleSymbol extends Symbol {
        public final Scope members;
        public FunctionSymbol initializer;     // función con el mismo nombre del módulo
        public FunctionSymbol mainFunction;    // 'Main' (relevante si es módulo principal)
        public ModuleSymbol(String name, int line, int column) {
            this(name, null, line, column);
        }
        public ModuleSymbol(String name, Scope parent, int line, int column) {
            super(name, line, column);
            this.members = new Scope("module", parent);
        }
    }

    // ============================================================
    // Clase
    // ============================================================
    public static final class ClassSymbol extends Symbol {
        public final boolean isPublic;
        public final Scope instanceMembers = new Scope("instance", null);
        public final Scope staticMembers   = new Scope("static",   null);
        /** V8/H8 (#793) — el tipo base de `@element{B}`: lo que guarda la clase, y lo que en `C<X>` se lee como X. Lo
         *  heredan sus hijas. null si no lo declara (`List` y sus hijas lo tienen sin marca: Object). */
        public BpType tipoElemento = null;
        /** Nombre crudo de 'extends X' (null si no hay). No es final desde V8 H7.3: en JC (`class C : A, B`) el
         *  semántico lo quita si `A` resulta ser una interfaz. */
        public String baseClassName;
        public ClassSymbol baseClass;      // resuelto en pase 2
        /** V8 H7.3 (`#756`) — una INTERFAZ: se guarda como una clase con su marca (Eduardo: «una clase y una interfaz
         *  se guardan igual o casi igual. Con un id…»). Su `baseClass` es Object (todo objeto lo es, y sus métodos
         *  valen sobre una interfaz); sus miembros, métodos sin cuerpo y properties; sus {@link #interfaces}, las de
         *  las que hereda. */
        public boolean isInterface;
        /** #842 (19-nov) — `final class`: ninguna clase la puede extender. La de otro módulo la trae su interfaz
         *  (`class X public final`, bpi 13). */
        public boolean isFinal;
        /** V8 H7.3 — las interfaces de la cabecera, como se escriben (`Forma`, `Lib.Medible`); en una interfaz, las de
         *  las que hereda. */
        public final java.util.List<String> interfaceNames = new java.util.ArrayList<>();
        /** V8 H7.3 — las mismas, resueltas (pase 2). */
        public final java.util.List<ClassSymbol> interfaces = new java.util.ArrayList<>();
        public FunctionSymbol constructor;
        public final Ast.ClassDef astNode;

        // ----- Clase externa (importada vía .bpi, L2) -----
        /** true si esta clase vive en otro módulo. astNode será null en ese caso. */
        public boolean isExternal;
        /** Library del módulo dueño (puede ser ""). */
        public String externalLibrary = "";
        /** Nombre lógico del módulo dueño. */
        public String externalModule = "";
        /** fromPath declarado en el import; "" si no hay. */
        public String externalFromPath = "";
        /** Mapeo nombre-de-método → slot del vtable en el módulo dueño.
         *  Sólo poblado para clases externas; el emisor lo usa para emitir
         *  INVOKE_VIRTUAL sin necesidad de tener la clase registrada en su
         *  propio ModWriter. */
        public final java.util.Map<String, Integer> externalMethodSlots = new java.util.HashMap<>();

        /** L2 v3 — layout binario de la clase externa (numFields/numMethods +
         *  bitmaps). Sólo poblado para clases externas con .bpi v6+. El emisor
         *  lo usa cuando un subclass local hereda de esta clase externa, para
         *  pasar el layout al ModWriter (que reserva placeholders y registra
         *  el class fixup). */
        public SemanticInfo.ClassBinaryLayout binaryLayout;

        /** H5.a-E3 — marca de la pasada `assignSlotKeys` (base antes que subclase). */
        public boolean keysAssigned;


        /** H1 P3 (20-sep) — `root class`: no desciende de nada (Object, Thread); sólo en la raíz. */
        public boolean isRoot = false;
        /** H1 P3 — ¿es una de las CUATRO clases del lenguaje (Object, Thread, Mutex, StringBuilder),
         *  las que Core.bp declara y el compilador conoce por nombre? En las claves canónicas van
         *  SIMPLES (`LObject;`, sin `Core/`): son del lenguaje, y así la ABI de las claves no cambió
         *  al pasarlas de Java a BP. Vale para el stub importado de Core y para la local al compilar Core. */
        public boolean esDelLenguaje() {
            if (isExternal && !"Core".equals(externalModule)) return false;
            return nombreDelLenguaje(name);
        }
        /** Los cuatro nombres, para quien sólo tiene el nombre (una referencia `Core.Object` sin resolver). */
        public static boolean nombreDelLenguaje(String n) {
            return "Object".equals(n) || "Thread".equals(n) || "Mutex".equals(n) || "StringBuilder".equals(n);
        }
        public ClassSymbol(String name, boolean isPublic, String baseClassName,
                           Ast.ClassDef ast, int line, int column) {
            super(name, line, column);
            this.isPublic = isPublic;
            this.isRoot = ast != null && ast.isRoot;
            this.baseClassName = baseClassName;
            this.astNode = ast;
            this.decl = ast;
            if (ast != null) {
                this.isInterface = ast.isInterface;
                this.interfaceNames.addAll(ast.interfaces);
            }
        }

        /** Busca un miembro de instancia subiendo por la cadena de herencia (V8 H7.3: en una interfaz, por las
         *  interfaces de las que hereda, y después por Object). */
        public Symbol lookupInstance(String n) {
            for (ClassSymbol c : dondeBuscar()) {
                Symbol s = c.instanceMembers.tryLookup(n);
                if (s != null) return s;
            }
            return null;
        }

        /** V8 H7.3 — dónde se buscan los miembros de instancia, en orden: en una clase, ella y sus padres; en una
         *  interfaz, ella y las interfaces de las que hereda (en el orden en que las declara, en profundidad y sin
         *  repetir), y después la cadena de su `baseClass` (Object). */
        public java.util.List<ClassSymbol> dondeBuscar() {
            java.util.List<ClassSymbol> out = new java.util.ArrayList<>();
            if (isInterface) {
                interfacesAplanadas(out);
                for (ClassSymbol c = baseClass; c != null; c = c.baseClass) out.add(c);
            } else {
                for (ClassSymbol c = this; c != null; c = c.baseClass) out.add(c);
            }
            return out;
        }

        /** V8 H7.3 — una interfaz y todas aquellas de las que hereda, sin repetir: ella primero, y después cada
         *  padre en el orden en que los declara, en profundidad. */
        public void interfacesAplanadas(java.util.List<ClassSymbol> out) {
            for (ClassSymbol c : out) if (mismaClase(c, this)) return;
            out.add(this);
            for (ClassSymbol p : interfaces) p.interfacesAplanadas(out);
        }

        /** V8 H7.3 — todas las interfaces que implementa una clase: las de su cabecera, las que éstas heredan y las
         *  de sus padres (las del padre primero). En una interfaz, ella y las de las que hereda. */
        public java.util.List<ClassSymbol> todasLasInterfaces() {
            java.util.List<ClassSymbol> out = new java.util.ArrayList<>();
            if (isInterface) { interfacesAplanadas(out); return out; }
            java.util.List<ClassSymbol> cadena = new java.util.ArrayList<>();
            for (ClassSymbol c = this; c != null; c = c.baseClass) cadena.add(0, c);
            for (ClassSymbol c : cadena) for (ClassSymbol i : c.interfaces) i.interfacesAplanadas(out);
            return out;
        }

        /** V8 H7.3 — ¿un valor de este tipo es una `iface`? Una clase, si la implementa ella o un padre (directamente
         *  o por una interfaz que hereda de ella); una interfaz, si es ella o hereda de ella. */
        public boolean implementa(ClassSymbol iface) {
            for (ClassSymbol i : todasLasInterfaces()) if (mismaClase(i, iface)) return true;
            return false;
        }

        /** V8 H7.3 — el mismo tipo: el mismo símbolo, o, de otro módulo, el mismo nombre en el mismo módulo (cada
         *  `import` hace su propio símbolo de lo que importa, como con Object). */
        public static boolean mismaClase(ClassSymbol a, ClassSymbol b) {
            if (a == b) return true;
            if (a == null || b == null || !a.name.equals(b.name)) return false;
            if (a.isExternal && b.isExternal) return a.externalModule.equals(b.externalModule)
                    && a.externalLibrary.equals(b.externalLibrary);
            return a.esDelLenguaje() && b.esDelLenguaje();
        }

        /** Busca un miembro estático subiendo por la cadena de herencia. */
        public Symbol lookupStatic(String n) {
            ClassSymbol c = this;
            while (c != null) {
                Symbol s = c.staticMembers.tryLookup(n);
                if (s != null) return s;
                c = c.baseClass;
            }
            return null;
        }

        /** true si this hereda (transitivamente) de other. */
        public boolean isSubclassOf(ClassSymbol other) {
            ClassSymbol c = baseClass;
            while (c != null) {
                if (c == other) return true;
                c = c.baseClass;
            }
            return false;
        }

        /** Busca el constructor más cercano subiendo por la jerarquía. */
        public FunctionSymbol findConstructor() {
            ClassSymbol c = this;
            while (c != null) {
                if (c.constructor != null) return c.constructor;
                c = c.baseClass;
            }
            return null;
        }

        /** #174b — slot de vtable por nombre de método/accesor PÚBLICO (o -1).
         *  Memoizado en {@link #methodSlots}. AotCEmitter lo lee para emitir
         *  call_method_i32; MivmEmitter lo verifica contra ModWriter (cross-check
         *  en emitInvokeVirtualSmart) — si divergen, error ruidoso de compilación. */
        public java.util.Map<String, Integer> methodSlots;   // null = no computado

        public int slotOf(String methodName) {
            ensureMethodSlots();
            Integer s = methodSlots.get(methodName);
            return s == null ? -1 : s;
        }

        /** Puebla el mapa nombre→slot PREGUNTÁNDOSELO A LA FUNCIÓN ÚNICA
         *  ({@link ModuleInterface#computeClassLayout}), que es la que reproduce
         *  fielmente al ModWriter y la que publica el layout en la interfaz.
         *
         *  Norma de #299: **dos funciones calculando lo mismo = bug casi seguro**.
         *  Aquí vivía un cálculo GEMELO (`ensureMethodSlots`) que se desviaba en
         *  dos casos REALES, confirmados con repro (el cross-check #174b gritaba):
         *    · property PRIVADA antes de un método público → el gemelo filtraba
         *      `!isPublic` y la función única NO ⇒ slots 2 menos (Box.saluda 2 vs 4);
         *    · subclase de `Thread` → el gemelo numeraba desde Object
         *      {toString,compareTo} y la base real es {run,start,join} (Worker.extra
         *      2 vs 3).
         *  El #174b solo cubre el camino del INTÉRPRETE; `slotOf()` lo consumen SIN
         *  verificación el `.slots` (que el IDE publica) y el AOT
         *  → ahí la desviación era silenciosa. Preguntando a la función única el
         *  modo de fallo desaparece por construcción. */
        private void ensureMethodSlots() {
            if (methodSlots != null) return;
            if (isExternal) { methodSlots = externalMethodSlots; return; }
            java.util.Map<String, Integer> m = new java.util.LinkedHashMap<>();
            ModuleInterface.LayoutAndNames lay =
                    ModuleInterface.computeClassLayout(this, new java.util.HashMap<>());
            for (int slot = 0; slot < lay.methodNames.size(); slot++) {
                String nm = lay.methodNames.get(slot);
                if (nm != null) m.putIfAbsent(nm, slot);   // 1er slot con ese nombre
            }
            methodSlots = m;
        }
    }

    // ============================================================
    // Funciones
    // ============================================================
    public static final class FunctionSymbol extends Symbol {
        public final boolean isPublic;
        public boolean isFinal;
        public final boolean isStatic;
        public final ClassSymbol ownerClass;        // null = función a nivel módulo
        public final List<ParamSymbol> params = new ArrayList<>();
        public BpType returnType;                    // null = void
        public boolean isConstructor;
        /** H5.c — `function event f()`: necesita slot de vtable aunque sea
         *  privada, porque el destino de un evento ES un slot. */
        public boolean isEventHandler = false;
        /** V7/H1 (21-sep) — el DESTRUCTOR (`function ~Clase()`): nombre interno `__dtor`, público y
         *  virtual (clave `__dtor()V` en toda la jerarquía: Core.Object lo declara vacío), sin
         *  parámetros ni retorno. Lo llama el lenguaje, no el usuario. */
        public boolean isDestructor = false;
        public static final String DTOR = "__dtor";
        /** #390 — `private` EXPLÍCITO: sólo la ve su propia clase. Sin marcar,
         *  un método es *protected* (la clase y sus descendientes). */
        public boolean isPrivate = false;

        /** #390 — **¿este método es VIRTUAL?**, o sea: ¿va en la tabla de métodos?
         *
         *  Regla de hoy: **lo es todo MENOS lo `private`**. Los manejadores de
         *  evento son virtuales siempre, incluso privados, porque el destino de un
         *  evento ES un slot.
         *
         *  Existe como función explícita a petición de Eduardo, y el nombre es el
         *  suyo: pregunta por el CONCEPTO del lenguaje, no por su consecuencia en
         *  la vtable. Así los sitios de llamada siguen leyéndose bien el día que la
         *  regla cambie — que era el motivo: *«si mañana se cambia la lógica, es
         *  más sencillo de mantener»*.
         *
         *  Y hacía falta: la pregunta estaba escrita A MANO en SIETE sitios, cada
         *  uno con su `isPublic`, dos de ellos con un comentario que ya decía
         *  «privado ⇒ no está en la vtable» sin poder decirlo con código. Eso es
         *  justo lo que la norma de #299 prohíbe («dos funciones calculando lo
         *  mismo = bug casi seguro»): cambiar la regla obligaba a acertar en las
         *  siete, y en la primera pasada fallé la tercera. */
        public boolean esVirtual() { return !isPrivate || isEventHandler; }
        /** Auto-super (modelo Java): este constructor NO escribió super() y el
         *  padre tiene un constructor invocable sin argumentos → el emisor inyecta
         *  la llamada implícita al __init del padre. Lo marca el SemanticAnalyzer. */
        public boolean implicitSuper;
        /** V7/H2 (#509) — QUÉ constructor del padre llama el super implícito: con
         *  sobrecarga, el que no lleva parámetros puede no ser la cabeza. */
        public FunctionSymbol implicitSuperCtor;
        public boolean isModuleInitializer;
        /** Función intrínseca: declarada sólo como signature; el emisor inlinea
         *  opcodes en cada call-site en lugar de generar CALL / CALL_EXT. El
         *  cuerpo (si existe en el AST) se ignora; el .mod no contiene código
         *  para esta función. */
        public boolean isIntrinsic;
        /** H1 (20-sep) — el bytecode de una intrínseca: su cuerpo `inline` (local: del AST;
         *  importada: de la interfaz `func … inline …`). */
        public List<Ast.InlineStmt> inlineOps;
        public final Ast.FuncDef astNode;

        // ----- Función externa (importada vía .bpi) -----
        /** true si esta función vive en otro módulo. astNode será null en ese caso. */
        public boolean isExternal;
        /** Library del módulo dueño (puede ser "" si el dueño no declara library). */
        public String externalLibrary = "";
        /** Nombre lógico del módulo dueño, e.g. "Util". */
        public String externalModule = "";
        /** fromPath declarado en el import; "" si no hay (usa convención por defecto). */
        public String externalFromPath = "";

        // ----- H5.a — sobrecarga de funciones (overloading) -----
        /** Cadena de sobrecargas con el MISMO nombre en el mismo scope. El
         *  símbolo que vive en el Scope es la CABEZA; las demás cuelgan aquí.
         *  null = sin más sobrecargas (caso común). La resolución por firma en
         *  la llamada recorre la cadena desde la cabeza. */
        public FunctionSymbol nextOverload;
        /** true en TODOS los miembros de un conjunto de sobrecargas (>1 función
         *  con el mismo nombre). El emisor manglea el nombre solo si es true. */
        public boolean overloaded;

        /** H5.a — CLAVE con la que esta función se registra/invoca en el backend
         *  (tablas por-nombre del ModWriter, CALL/CALL_EXT y SLOT de vtable).
         *  La calcula UNA sola pasada ({@code assignSlotKeys}) con la regla:
         *  <b>la primera firma de un grupo se queda con el nombre pelado; las
         *  siguientes se manglean</b>. Así, añadir una sobrecarga es ADITIVO (no
         *  cambia el símbolo de la que ya existía) y una clase base YA COMPILADA
         *  en otro módulo no necesita re-manglearse cuando una subclase añade una
         *  sobrecarga de ese nombre. null ⇒ aún sin asignar: vale el nombre. */
        public String slotKey;
        /** #511 (19-sep) — el constructor de la MISMA clase en el que éste delega
         *  (`MiClase(args)` como primera sentencia), o null. El que delega no hace
         *  mutex, ni super(), ni inicializadores: los hace el delegado. */
        public FunctionSymbol delegaA;

        /** Clave efectiva (ver {@link #slotKey}). Nunca null. */
        public String slotKey() { return slotKey != null ? slotKey : name; }

        /** Firma canónica (solo los tipos de los parámetros) — identifica una
         *  sobrecarga dentro de un grupo, y es lo que hace de un método un
         *  OVERRIDE de otro. Misma codificación que el mangling. */
        public String signatureKey() {
            StringBuilder sb = new StringBuilder();
            for (ParamSymbol p : params) {
                if (p.isVar) sb.append('&');                 // V8/H7.4: otra ABI, otra firma (no un override)
                sb.append(typeCode(p.type, this));
            }
            return sb.toString();
        }

        /** V8/H7.4 — ¿lleva algún parámetro `var`? */
        public boolean tieneVar() {
            for (ParamSymbol p : params) if (p.isVar) return true;
            return false;
        }

        public FunctionSymbol(String name, boolean isPublic, boolean isFinal, boolean isStatic,
                              ClassSymbol owner, Ast.FuncDef ast) {
            super(name, ast != null ? ast.line : 0, ast != null ? ast.column : 0);
            this.isPublic = isPublic;
            this.isFinal = isFinal;
            this.isStatic = isStatic;
            this.ownerClass = owner;
            this.astNode = ast;
            this.decl = ast;
        }

        /**
         * Devuelve el símbolo de import tal y como aparece en la sección imports
         * del .mod consumidor. Formato:
         *   "Module.func"            si externalLibrary == ""
         *   "lib.path.Module.func"   si externalLibrary != ""
         */
        public String externalQualifiedName() {
            StringBuilder sb = new StringBuilder();
            if (externalLibrary != null && !externalLibrary.isEmpty()) {
                sb.append(externalLibrary).append('.');
            }
            // H5.a-E2 — si es una sobrecarga, el componente final va MANGLEADO por
            // firma (igual que lo exportó el módulo dueño), para que el CALL_EXT
            // resuelva el overload correcto contra la tabla de exports.
            sb.append(externalModule).append('.').append(slotKey());
            return sb.toString();
        }

        /** Compara nombre y firma de parámetros (no el tipo de retorno). */
        public boolean hasSameSignatureAs(FunctionSymbol other) {
            if (!name.equals(other.name)) return false;
            if (params.size() != other.params.size()) return false;
            for (int i = 0; i < params.size(); i++) {
                BpType a = params.get(i).type;
                BpType b = other.params.get(i).type;
                if (a == null || b == null || !a.sameAs(b)) return false;
            }
            return true;
        }

        /** H5.a — nombre ÚNICO de ESTA sobrecarga para las tablas por-nombre del
         *  backend (ModWriter) y para el CALL/CALL_EXT. Solo debe usarse cuando
         *  {@link #overloaded} es true; si no, el nombre pelado sigue valiendo.
         *  Formato: {@code name + '$' + códigos de tipo de cada parámetro}.
         *  Requiere que los tipos de los parámetros estén resueltos. */
        public String overloadMangle() {
            StringBuilder sb = new StringBuilder(name).append('$');
            for (ParamSymbol p : params) sb.append(typeCode(p.type, this));
            return sb.toString();
        }

        /** #528 — el módulo DUEÑO de esta función (cualificado con su librería), o "" si es
         *  de este módulo. Es respecto a él como se cualifica una clase en su mangling. */
        private String moduloDueno() {
            if (externalModule != null && !externalModule.isEmpty()) return modulo(externalLibrary, externalModule);
            // un método o constructor de un stub importado no lleva externalModule: lo lleva su clase
            if (ownerClass != null && ownerClass.isExternal) return modulo(ownerClass.externalLibrary, ownerClass.externalModule);
            return "";
        }

        /** H5.a — código de tipo para el mangling de sobrecargas. Debe ser
         *  DETERMINISTA e IDÉNTICO en la definición, en la llamada y cross-module
         *  (por eso vive en UN solo sitio). Estilo descriptor de Java. */
        public static String typeCode(BpType t) { return typeCode(t, null); }

        /** #528 (19-sep) — el código de tipo RELATIVO a la función `fs`: una clase (o enum)
         *  del MISMO módulo que la función va simple (`LBase;`) y una de otro módulo va
         *  cualificada con el suyo (`LLib/Base;`, con '/' como la clave canónica). Así la
         *  definición y la llamada cross-module producen el MISMO nombre: el dueño ve su
         *  clase como local y el consumidor la ve como externa del módulo de la función.
         *  (Cualificar sólo «lo externo» rompía eso: el consumidor pedía
         *  `__cls_init_Base$LCtorBase/Base;` y el dueño exportaba `$LBase;`.) Por nombre
         *  simple a secas, `m(Lib.Base)` y `m(Otra.Base)` eran la MISMA sobrecarga. Los
         *  nombres que exporta la stdlib no se mueven (sus clases son de sus módulos). */
        public static String typeCode(BpType t, FunctionSymbol fs) {
            if (t == null) return "?";
            if (t instanceof BpType.PrimitiveType) {
                switch (((BpType.PrimitiveType) t).tag) {
                    case INTEGER: return "i";
                    case LONG:    return "j";
                    case FLOAT:   return "f";
                    case DOUBLE:  return "d";
                    case STRING:  return "s";
                    case BOOLEAN: return "z";
                    case INT8:    return "b";
                    case UINT8:   return "B";
                    case INT16:   return "h";
                    case UINT16:  return "H";
                    default:      return "?";
                }
            }
            if (t instanceof BpType.ArrayType)
                return "[" + typeCode(((BpType.ArrayType) t).element, fs);
            String dueno = (fs != null) ? fs.moduloDueno() : "";
            if (t instanceof BpType.ClassType) {
                ClassSymbol cls = ((BpType.ClassType) t).cls;
                String suyo = cls.esDelLenguaje() ? "" : modulo(cls.externalLibrary, cls.externalModule);
                return "L" + (suyo.isEmpty() || suyo.equals(dueno) ? "" : suyo.replace('.', '/') + "/") + simpleClassName(cls.name) + ";";
            }
            if (t instanceof BpType.UnresolvedClassRef) {
                // transitiva (`Lib.Base` en la interfaz de `Mid`, sin ClassSymbol): ya viene con módulo
                String n = ((BpType.UnresolvedClassRef) t).name;
                int i = n.lastIndexOf('.');
                if (i < 0) return "L" + n + ";";
                String suyo = n.substring(0, i);
                // H1 P3 (21-sep): `Core.Object` sin resolver (el ns de Core llega el ÚLTIMO: el import
                // implícito va al final) va SIMPLE, como la clase resuelta — si no, el consumidor
                // mangleaba `put$sLCore/Object;` y el dueño `put$sLObject;` (MOD8, MapTipado).
                if ("Core".equals(suyo) && ClassSymbol.nombreDelLenguaje(n.substring(i + 1))) return "L" + n.substring(i + 1) + ";";
                return "L" + (suyo.equals(dueno) ? "" : suyo.replace('.', '/') + "/") + n.substring(i + 1) + ";";
            }
            if (t instanceof BpType.EnumType) {
                EnumSymbol en = ((BpType.EnumType) t).en;
                String suyo = modulo(en.externalLibrary, en.externalModule);
                return "E" + (suyo.isEmpty() || suyo.equals(dueno) ? "" : suyo.replace('.', '/') + "/") + en.name + ";";
            }
            if (t instanceof BpType.AnyType) return "a";
            if (t instanceof BpType.TupleType) {
                StringBuilder sb = new StringBuilder("T");
                for (BpType e : ((BpType.TupleType) t).elements) sb.append(typeCode(e, fs));
                return sb.append(";").toString();
            }
            return "?";
        }

        /** H5.a-E2 — nombre simple (último segmento). La interfaz serializa las
         *  clases con nombre cualificado (Lib.Mod.Cls) pero el módulo dueño manglea
         *  con el nombre simple; normalizamos a simple para que la firma del def y
         *  la de la llamada cross-module coincidan. */
        private static String simpleClassName(String n) {
            int i = n.lastIndexOf('.');
            return (i < 0) ? n : n.substring(i + 1);
        }
        /** #528 — el módulo cualificado (`lib.Mod`) de una clase o enum EXTERNO, o "" si es local. */
        private static String modulo(String lib, String mod) {
            if (mod == null || mod.isEmpty()) return "";
            return (lib != null && !lib.isEmpty()) ? lib + "." + mod : mod;
        }
    }

    // ============================================================
    // Parámetros
    // ============================================================
    public static final class ParamSymbol extends Symbol {
        public BpType type;
        /** H8.1 — valor por defecto (expresión literal constante normalizada), o null. */
        public Ast.IExpr defaultExpr;
        /** V8/H7.4 — `var`: se le pasa `objeto::property` y viajan el objeto y las ranuras de su getter y su setter. */
        public boolean isVar;
        public ParamSymbol(String name, int line, int column) { super(name, line, column); }
    }

    // ============================================================
    // Variables, constantes, propiedades
    // ============================================================
    /**
     * H5.c — un EVENTO declarado en una clase.
     *
     * No lleva `isPublic` porque su acceso no se elige: asignarlo (suscribirse)
     * es público SIEMPRE, y leerlo/dispararlo es de la clase y sus descendientes
     * SIEMPRE. Es lo que define a un evento, no un modificador.
     *
     * En memoria son dos campos, `<name>$recv` y `<name>$dest`; el símbolo sólo
     * guarda el nombre lógico y quién lo declaró.
     */
    public static final class EventSymbol extends Symbol {
        public final ClassSymbol ownerClass;
        /** H5.c (revisión) — la FIRMA declarada. El handler suscrito tiene que
         *  casar con ella; es lo que sustituye al `kind` y lo que resuelve la
         *  sobrecarga en el `::`. */
        public final List<ParamSymbol> params = new ArrayList<>();
        // OJO: NO declarar aquí un `decl` propio — Symbol ya lo tiene, y uno
        // nuevo lo SOMBREARÍA: quien escriba por la variable base y lea por la
        // derivada (o al revés) vería null sin que nada falle.
        /** H5.c-E2 — evento de una clase IMPORTADA: slot absoluto de su campo
         *  `recv` (el `dest` va en recv+2), tal y como lo publicó su módulo.
         *  -1 = evento local, el emisor resuelve los campos por nombre. */
        public int externalRecvSlot = -1;
        /** H5.c-E2 — importado sin firma (algún param no era exportable): ocupa
         *  sus slots, pero suscribirse desde fuera no es posible. */
        public boolean externalOpaque = false;

        public EventSymbol(String name, ClassSymbol owner, int line, int column) {
            super(name, line, column);
            this.ownerClass = owner;
        }
        /** Nombre del campo que guarda el receptor (handle de 8B, raíz de GC). */
        public String recvField() { return name + "$recv"; }
        /** Nombre del campo que guarda el destino (slot de vtable, 4B, NO ref). */
        public String destField() { return name + "$dest"; }
    }

    public static final class VarSymbol extends Symbol {
        public final boolean isPublic;
        /** #508 — `private var`: de su clase y de nadie más (checkVisibility). */
        public boolean isPrivate = false;
        /** V7/H2 (#492-B) — campo de una clase de OTRO módulo: su ranura en el
         *  objeto, tal como la publicó la interfaz del dueño (`field … slot n`).
         *  -1 = campo local (la ranura la conoce el ModWriter por nombre). El
         *  emisor accede por ranura cuando el dueño es externo. */
        public int externalSlot = -1;
        public final boolean isStatic;
        public final ClassSymbol ownerClass;
        public final boolean isLocal;
        /** Si esta variable es un "owner" (var owner): destruye el objeto al
         *  salir de scope o al reasignarse. Solo válido si type es ClassType. */
        public boolean isOwner;
        public BpType type;
        /** L8 v2 — var de módulo `tipo[N]`: array de tamaño fijo HORNEADO en
         *  el data block ([len][elems], binding fija — leer la var es LEA de
         *  la dirección del símbolo; no hay slot de ref y no es reasignable).
         *  -1 = var normal. */
        public int fixedArraySize = -1;

        public VarSymbol(String name, boolean isPublic, boolean isStatic,
                         ClassSymbol owner, boolean isLocal, int line, int column) {
            super(name, line, column);
            this.isPublic = isPublic;
            this.isStatic = isStatic;
            this.ownerClass = owner;
            this.isLocal = isLocal;
        }
    }

    public static final class ConstSymbol extends Symbol {
        public final boolean isPublic;
        public final boolean isStatic;
        public final ClassSymbol ownerClass;
        public BpType type;
        /** L8 v2 — true si está declarada en un cuerpo de función: cuando su
         *  init no es literal vive en un slot LOCAL (no hay símbolo global). */
        public boolean isLocal;
        /**
         * Valor literal compilado cuando se conoce en tiempo de compilación
         * (típicamente importadas desde un .bpi). Tipos: Long, Double, String,
         * Boolean. null ⇒ no inlinable, hay que resolver vía global/getter.
         */
        public Object literalValue;

        public ConstSymbol(String name, boolean isPublic, boolean isStatic,
                           ClassSymbol owner, int line, int column) {
            super(name, line, column);
            this.isPublic = isPublic;
            this.isStatic = isStatic;
            this.ownerClass = owner;
        }
    }

    public static final class PropertySymbol extends Symbol {
        public final boolean isPublic;
        /** #508 — `private property`. */
        public boolean isPrivate = false;
        public boolean isFinal;
        public final boolean isStatic;
        public final boolean isOwner;
        public final boolean isSync;       // `public sync property` — getter/setter envueltos con un Mutex compartido por instancia.
        public final ClassSymbol ownerClass;
        public BpType type;
        public final boolean hasCustomGetter;
        public final boolean hasCustomSetter;
        public final boolean isShortForm;
        public final Ast.PropertyDef astNode;

        // ----- Property externa (importada vía .bpi, property a nivel módulo) -----
        /** true si esta property vive en otro módulo (sólo aplica a properties de módulo). */
        public boolean isExternal;
        /** Library del módulo dueño (puede ser ""). */
        public String externalLibrary = "";
        /** Nombre lógico del módulo dueño. */
        public String externalModule = "";
        /** fromPath declarado en el import; "" si no hay. */
        public String externalFromPath = "";

        public PropertySymbol(String name, boolean isPublic, boolean isFinal, boolean isStatic,
                              ClassSymbol owner, Ast.PropertyDef ast) {
            super(name, ast != null ? ast.line : 0, ast != null ? ast.column : 0);
            this.isPublic = isPublic;
            this.isFinal = isFinal;
            this.isStatic = isStatic;
            this.isOwner = ast != null && ast.isOwner;
            this.isSync  = ast != null && ast.isSync;
            this.ownerClass = owner;
            this.hasCustomGetter = ast != null && ast.getter != null;
            this.hasCustomSetter = ast != null && ast.setter != null;
            this.isShortForm = ast != null && ast.isShortForm;
            this.astNode = ast;
            this.decl = ast;
        }
    }

    // ============================================================
    // Namespace de un módulo importado (cargado desde un .bpi)
    // ============================================================
    /**
     * Símbolo que vive en el scope del módulo bajo el alias usado por el
     * import (e.g. "Util" para `import Util` o `import lib.Util`). Expone las
     * funciones públicas del módulo importado para que el analizador resuelva
     * `Util.foo(args)` en tiempo de compilación. Sólo se crea por el driver
     * que carga el .bpi correspondiente, no por el parser.
     */
    public static final class ImportedNamespaceSymbol extends Symbol {
        public final String library;        // "" si el módulo importado no declara library
        public final String moduleName;     // nombre lógico, e.g. "Util"
        public final String fromPath;       // "" si no se declaró `from "..."`
        public final Map<String, FunctionSymbol> functions  = new HashMap<>();
        public final Map<String, ConstSymbol>    consts     = new HashMap<>();
        public final Map<String, EnumSymbol>     enums      = new HashMap<>();
        public final Map<String, PropertySymbol> properties = new HashMap<>();
        public final Map<String, ClassSymbol>    classes    = new HashMap<>();
        public ImportedNamespaceSymbol(String alias, String library, String moduleName, String fromPath) {
            super(alias, 0, 0);
            this.library    = (library == null) ? "" : library;
            this.moduleName = moduleName;
            this.fromPath   = (fromPath == null) ? "" : fromPath;
        }
    }

    // ============================================================
    // Enums
    // ============================================================
    public static final class EnumSymbol extends Symbol {
        public final boolean isPublic;
        public final Map<String, Long> values = new java.util.LinkedHashMap<>();   // #522: orden de declaración (la interfaz lo vuelca y el ifaceCrc lo sella)
        /** V7/H2 (#523) — de qué módulo es un enum IMPORTADO ("" = declarado aquí).
         *  Sin esto el enum no podía viajar en una firma (#516): la clave canónica
         *  lo cualifica como a las clases. Lo rellena el importador. */
        public String externalLibrary = "";
        public String externalModule  = "";

        public EnumSymbol(String name, boolean isPublic, int line, int column) {
            super(name, line, column);
            this.isPublic = isPublic;
        }
    }

    // ============================================================
    // Scope: tabla mutable name -> symbol con padre opcional
    // ============================================================
    public static final class Scope {
        public final String tag;
        public final Scope parent;
        private final Map<String, Symbol> symbols = new HashMap<>();

        public Scope(String tag, Scope parent) {
            this.tag = tag;
            this.parent = parent;
        }

        public Collection<Symbol> getSymbols() { return symbols.values(); }

        /** Define un símbolo en este scope. Devuelve false si ya existía. */
        public boolean tryDefine(Symbol s) {
            if (symbols.containsKey(s.name)) return false;
            symbols.put(s.name, s);
            return true;
        }

        /** Busca un símbolo solo en este scope. Devuelve null si no está. */
        public Symbol tryLookup(String n) { return symbols.get(n); }

        /** Busca subiendo por la cadena de scopes. Null si no está. */
        public Symbol resolve(String n) {
            Scope s = this;
            while (s != null) {
                Symbol sym = s.symbols.get(n);
                if (sym != null) return sym;
                s = s.parent;
            }
            return null;
        }
    }

    // ============================================================
    // V7/H2 (#523, regla R3) — LA CLAVE CANÓNICA
    // ============================================================
    /**
     * El nombre con el que un miembro CRUZA la frontera de un módulo, calculado en UN
     * sitio y de una sola manera, para que el dueño que lo exporta y el consumidor
     * que lo pide lleguen a la misma cadena sin ponerse de acuerdo:
     *
     *   método / accesor / función / ctor   {@code nombre(códigos)ret}   ({@code ret} = {@code V} si no
     *                                        devuelve; el ctor no lleva retorno)
     *   campo / property                    {@code nombre:código}
     *   evento                              {@code nombre(códigos)}
     *
     * Códigos: los de {@link FunctionSymbol#typeCode} con tres cambios que son la razón
     * de ser de esta clase — (1) una clase DECLARADA va CUALIFICADA ({@code L<Mod/Cls>;},
     * con library si la hay, y con '/' como en los descriptores de la JVM porque los dos
     * cargadores partan los nombres de import por sus puntos), así dos clases homónimas de
     * módulos distintos no se confunden; (2) una clase del LENGUAJE (las que sintetiza el semántico: Object,
     * Thread, Mutex, StringBuilder — {@code esDelLenguaje()}; H1 P3: viven en Core.bp y la
     * clave siguió simple) va SIMPLE; (3) un enum va {@code E<Mod.Enum>;}, cualificado igual. Y la
     * cuarta: un tipo SIN RESOLVER es un error de compilación, nunca un {@code ?}: el
     * {@code L} contra {@code E} de un stub mangleado antes de resolver tipos fue lo
     * que hizo imposible reconstruir claves en el importador (#514, medido el 15-sep).
     *
     * A diferencia de {@code overloadMangle}, aquí SIEMPRE va la firma completa, también
     * en la cabeza de una cadena de sobrecargas y también el tipo de retorno: cambiar
     * cualquiera de los dos en el dueño tiene que dejar de casar (V7_IDEAS §H2.7, D1).
     *
     * {@code moduloLocal} es el nombre cualificado (library incluida) del módulo que
     * está compilando: es lo que cualifica a sus propias clases y enums.
     */
    public static final class ClaveCanonica {
        private ClaveCanonica() {}

        /** Un tipo que no se ha resuelto no puede cruzar: se aborta con nombre. */
        public static final class TipoSinResolver extends RuntimeException {
            public TipoSinResolver(String que) { super("clave canonica: tipo sin resolver: " + que); }
        }

        public static String tipo(BpType t, String moduloLocal) {
            if (t == null || t instanceof BpType.VoidType) return "V";
            if (t instanceof BpType.PrimitiveType) {
                String c = FunctionSymbol.typeCode(t);
                if ("?".equals(c)) throw new TipoSinResolver(t.display());
                return c;
            }
            if (t instanceof BpType.ArrayType) return "[" + tipo(((BpType.ArrayType) t).element, moduloLocal);
            if (t instanceof BpType.AnyType)   return "a";
            // Dentro de un código el cualificador va con '/' (como los descriptores de la JVM): los dos
            // cargadores partan un nombre de import por sus PUNTOS (`Mod.f(LLib/Base;)s` → módulo `Mod`).
            if (t instanceof BpType.ClassType) {
                ClassSymbol cls = ((BpType.ClassType) t).cls;
                if (cls.esDelLenguaje()) return "L" + cls.name + ";";   // H1 P3: del lenguaje, SIMPLE (la clave no cambió al vivir en Core)
                return "L" + clase(cls, moduloLocal).replace('.', '/') + ";";
            }
            if (t instanceof BpType.EnumType) {
                EnumSymbol en = ((BpType.EnumType) t).en;
                StringBuilder sb = new StringBuilder("E");
                if (en.externalModule != null && !en.externalModule.isEmpty()) {
                    if (en.externalLibrary != null && !en.externalLibrary.isEmpty()) sb.append(en.externalLibrary).append('.');
                    sb.append(en.externalModule);
                } else {
                    if (moduloLocal == null || moduloLocal.isEmpty()) throw new TipoSinResolver("modulo local del enum '" + en.name + "'");
                    sb.append(moduloLocal);
                }
                return "E" + sb.substring(1).replace('.', '/') + "/" + en.name + ";";
            }
            if (t instanceof BpType.TupleType) {
                StringBuilder sb = new StringBuilder("T");
                for (BpType e : ((BpType.TupleType) t).elements) sb.append(tipo(e, moduloLocal));
                return sb.append(';').toString();
            }
            if (t instanceof BpType.UnresolvedClassRef) {
                String un = ((BpType.UnresolvedClassRef) t).name;   // H1 P3: `Core.Object` sin resolver, simple
                if (un.startsWith("Core.") && ClassSymbol.nombreDelLenguaje(un.substring(5))) return "L" + un.substring(5) + ";";
                // V10 (16-sep, H1): una clase de una dependencia TRANSITIVA que este módulo no importa
                // (`Lib.Base` en la interfaz de `Mid`) no tiene ClassSymbol, pero su nombre ya viene
                // cualificado: la clave sólo necesita eso. Sin punto no hay de dónde sacar el módulo.
                String n = ((BpType.UnresolvedClassRef) t).name;
                if (n.indexOf('.') > 0) return "L" + n.replace('.', '/') + ";";
                throw new TipoSinResolver(n);
            }
            throw new TipoSinResolver(t.display());
        }

        /** El nombre cualificado de una clase tal como va dentro de {@code L…;}. */
        public static String clase(ClassSymbol cls, String moduloLocal) {
            StringBuilder sb = new StringBuilder();
            if (cls.isExternal) {
                if (cls.externalLibrary != null && !cls.externalLibrary.isEmpty()) sb.append(cls.externalLibrary).append('.');
                sb.append(cls.externalModule);
            } else {
                if (moduloLocal == null || moduloLocal.isEmpty()) throw new TipoSinResolver("modulo local de '" + cls.name + "'");
                sb.append(moduloLocal);
            }
            return sb.append('.').append(cls.name).toString();
        }

        /** {@code nombre(códigos)ret}; un constructor no lleva retorno. */
        public static String metodo(FunctionSymbol f, String moduloLocal) {
            StringBuilder sb = new StringBuilder(f.name).append('(');
            for (ParamSymbol p : f.params) {
                if (p.type == null) throw new TipoSinResolver("parametro '" + p.name + "' de '" + f.name + "'");
                if (p.isVar) sb.append('&');                 // V8/H7.4: el que no lo sepa no enlaza, en vez de descuadrar la pila
                sb.append(tipo(p.type, moduloLocal));
            }
            sb.append(')');
            if (!f.isConstructor) sb.append(tipo(f.returnType, moduloLocal));
            return sb.toString();
        }

        /** {@code nombre:código} — campos y properties (el accesor de una property es un método: {@code getX()t}). */
        public static String campo(String nombre, BpType t, String moduloLocal) {
            if (t == null) throw new TipoSinResolver("campo '" + nombre + "'");
            return nombre + ":" + tipo(t, moduloLocal);
        }

        /** {@code nombre(códigos)} — la firma que el handler tiene que casar. */
        public static String evento(EventSymbol e, String moduloLocal) {
            StringBuilder sb = new StringBuilder(e.name).append('(');
            for (ParamSymbol p : e.params) {
                if (p.type == null) throw new TipoSinResolver("parametro '" + p.name + "' del evento '" + e.name + "'");
                sb.append(tipo(p.type, moduloLocal));
            }
            return sb.append(')').toString();
        }
    }
}
