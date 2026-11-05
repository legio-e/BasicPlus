// ============================================================
// Ast.java
// Nodos del AST de BASICPLUS (port a Java 8 del Ast.cs en C#).
//
// Organización:
//   - Una clase contenedor 'Ast' (no instanciable).
//   - Clase abstracta 'Node' con line/column.
//   - Interfaces marcador: ITopLevelDecl, IStmt, IExpr.
//   - Clases concretas como nested static classes.
//
// Java 8 no tiene records: cada nodo es una clase final con
// campos public final inicializados por constructor.
// ============================================================
package basicplus.frontend;

import java.util.ArrayList;
import java.util.List;

public final class Ast {

    private Ast() {}

    // ============================================================
    // RAÍZ
    // ============================================================
    public static abstract class Node {
        public final int line;
        public final int column;
        /** V5/H5 — las anotaciones que preceden a esta declaración, en su orden; vacía si no lleva ninguna. Desde V8
         *  H7.3 (`#756`), varias, como en Java (Eduardo: «Si, varias notaciones»): `@BD{ pk }` y `@implement{ I }`
         *  sobre la misma property.
         *
         *  Viven en `Node` (y no sólo en ClassDef/PropertyDef) a propósito: el
         *  PARSER las acepta delante de cualquier declaración y es el SEMÁNTICO
         *  quien decide dónde es legal. Así un `@BD` en el sitio equivocado da
         *  un diagnóstico que explica el problema, en vez de un error de
         *  sintaxis que no dice nada.
         *
         *  NO se persisten: no tocan el .mod ni el .bpi. Son para el compilador y
         *  el generador de DAOs; `@implement` y `@override` (H7.3) las comprueba
         *  el semántico, y lo que el `.mod` lleva de una implementación lo saca
         *  él de los tipos, no de la anotación. */
        public final List<Annotation> annotations = new ArrayList<>();
        /** La anotación con ese nombre, o null. */
        public Annotation anotacion(String nombre) {
            for (Annotation a : annotations) if (a.name.equals(nombre)) return a;
            return null;
        }
        protected Node(int line, int column) {
            this.line = line;
            this.column = column;
        }
    }

    // ---- Anotaciones (V5/H5) ----

    /** Una entrada `clave = valor` dentro de `@Nombre{ ... }`.
     *
     *  Guarda su propia línea/columna para poder señalar LA CLAVE concreta en
     *  un diagnóstico, y no la anotación entera. */
    public static final class AnnEntry {
        public final String key;
        public final Object value;      // hoy siempre un literal de cadena
        public final int line;
        public final int column;
        public AnnEntry(String key, Object value, int line, int column) {
            this.key = key;
            this.value = value;
            this.line = line;
            this.column = column;
        }
    }

    /** `@BD{ tabla = "medidas" }` — anotación colgada de la declaración que la sigue.
     *
     *  El parser NO interpreta las claves: recoge pares `clave = valor` y ya.
     *  Qué claves son válidas y en qué declaración lo decide el semántico, de
     *  forma que añadir una clave nueva no toca la gramática. */
    public static final class Annotation {
        public final String name;              // "BD"
        public final List<AnnEntry> entries;
        public final int line;
        public final int column;
        public Annotation(String name, List<AnnEntry> entries, int line, int column) {
            this.name = name;
            this.entries = entries;
            this.line = line;
            this.column = column;
        }
        /** La entrada con esa clave, o null. */
        public AnnEntry get(String key) {
            for (AnnEntry e : entries) if (e.key.equals(key)) return e;
            return null;
        }
    }

    // ---- Marcadores ----
    public interface ITopLevelDecl { }
    public interface IStmt         { }
    public interface IExpr         { }

    // ---- Enumeraciones auxiliares ----
    /** Las asignaciones. Las compuestas (V8/H7.1 #747: `*=` y los demás, en BP `mod=`, `xor=`, `shl=` y `shr=`) son
     *  `x := x op e` con `x` calculado una vez; {@link #binario()} dice su operador de BP (el `op` de un BinaryExpr). */
    public enum AssignOpKind {
        ASSIGN(null), PLUS_ASSIGN("+"), MINUS_ASSIGN("-"),
        MUL_ASSIGN("*"), DIV_ASSIGN("/"), MOD_ASSIGN("mod"), AND_ASSIGN("&"), OR_ASSIGN("|"),
        XOR_ASSIGN("xor"), SHL_ASSIGN("shl"), SHR_ASSIGN("shr");
        private final String binario;
        AssignOpKind(String binario) { this.binario = binario; }
        /** El operador de BP con el que opera el compuesto; null en `:=`. */
        public String binario() { return binario; }
        /** Cómo se escribe en BP: `:=`, `+=`, `mod=`… */
        public String enBp() { return binario == null ? ":=" : binario + "="; }
    }

    /** V8/H7.1 (#747) — el ternario: `iif(c, a, b)` en BP, `c ? a : b` en JC. Tiene forma de llamada, pero sólo se
     *  calcula el valor que toca (lo despliega el compilador, como `integer(x)`). */
    public static final String TERNARIO = "iif";
    public enum PrintSep     { NONE, COMMA, SEMICOLON }

    // ============================================================
    // MÓDULO
    // ============================================================
    public static final class ModuleNode extends Node {
        public final String library;              // "" si no se declara library
        public final String name;
        public final boolean isInterface;          // `module interface X ...`
        /**
         * Nombre cualificado del contrato padre.
         *   - Si !isInterface  → `module X implements <implementsName>`.
         *   - Si isInterface   → `module interface X extends <implementsName>` (interface inheritance).
         * Null en ambas variantes si no se declara nada.
         */
        public final String implementsName;
        public final List<ImportNode> imports;
        public final List<ITopLevelDecl> defs;
        /** V5/H4 — `import v1 from pack "SQLI"`: marca (4 chars) y versión de la
         *  API del pack NATIVO del que este módulo toma sus `native` externas.
         *  null = módulo normal. Es propiedad del MÓDULO, no de cada función:
         *  un módulo habla con UN pack, igual que un `.mod` declara sus imports.
         *  Mutable a propósito — el parser las descubre al leer los imports, que
         *  van antes que las funciones, y el ModuleNode se construye al final. */
        public String packNativoMarca   = null;
        public int    packNativoVersion = 0;
        /** V8 #698 — leído de un `.jc`. El análisis mira entonces que el `new` y el `(T) x` escritos casan con lo
         *  que cada llamada es (las marcas `conNew` y `esConversion` de CallExpr); en BP, eso lo deduce de los tipos. */
        public boolean escritoEnJc = false;
        public ModuleNode(String library, String name, List<ImportNode> imports, List<ITopLevelDecl> defs, int line, int column) {
            this(library, name, false, null, imports, defs, line, column);
        }
        public ModuleNode(String library, String name, boolean isInterface, String implementsName,
                          List<ImportNode> imports, List<ITopLevelDecl> defs, int line, int column) {
            super(line, column);
            this.library = library;
            this.name = name;
            this.isInterface = isInterface;
            this.implementsName = implementsName;
            this.imports = imports;
            this.defs = defs;
        }
    }

    public static final class ImportNode extends Node {
        public final List<String> path;           // segmentos: último = nombre de la interfaz (o módulo, si no hay binding)
        public final String fromPath;             // null si no hay 'from "..."'
        public final String boundImpl;            // nombre simple del módulo concreto en `import Iface:Impl`; null si import directo
        /** H5.b — `import Modulo from pack NombrePack`: el módulo vive DENTRO de
         *  ese pack (no como .mod suelto). null = import normal. Se eligió esta
         *  forma porque `from` ya significa "de dónde sale este módulo" y el ':'
         *  estaba ocupado por `import Iface:Impl`. */
        public final String packName;
        /** H1 P3 (20-sep) — `import Core` lo pone el PARSER en todo módulo que no lo escriba
         *  (salvo en el propio Core): la raíz se importa siempre. Marcado para que quien liste
         *  los imports del fuente (IDE, mensajes) sepa que no está escrito. */
        public boolean implicito = false;
        /** V7/H6 G5.b0 (26-sep, el resto de #542) — `import v1 from pack "SQLI"`: la marca, si ESTE
         *  import es el de la API de un pack NATIVO. No es un módulo BP —no tiene interfaz que cargar—:
         *  es la versión de la tabla que la placa publica con esa marca, y sus funciones son las
         *  `native` externas del propio módulo. null = un import de verdad. Va en el import y no sólo
         *  en el módulo ({@link ModuleNode#packNativoMarca}) porque quien recorre los imports tiene
         *  que poder saltarlo SIN adivinar por el nombre (un módulo puede llamarse `v1`). */
        public String packNativoMarca = null;
        public ImportNode(List<String> path, String fromPath, int line, int column) {
            this(path, fromPath, null, null, line, column);
        }
        public ImportNode(List<String> path, String fromPath, String boundImpl, int line, int column) {
            this(path, fromPath, boundImpl, null, line, column);
        }
        public ImportNode(List<String> path, String fromPath, String boundImpl, String packName,
                          int line, int column) {
            super(line, column);
            this.path = path;
            this.fromPath = fromPath;
            this.boundImpl = boundImpl;
            this.packName = packName;
        }
    }

    // ============================================================
    // TIPOS
    // ============================================================
    public static abstract class TypeRef extends Node {
        protected TypeRef(int line, int column) { super(line, column); }
    }

    public static final class SimpleTypeRef extends TypeRef {
        public final String name;   // 'integer' | 'float' | ... | nombre de clase/enum
        /** V8/H7.2 (#749) — los argumentos de tipo, `List<Object>`, `Collections.Map<integer, Object>`; null si no los
         *  lleva. De momento, sólo esos dos (lo comprueba el análisis), y no cambian el bytecode: son la List y el Map
         *  de siempre, con la llave del Map comprobada al compilar. */
        public List<TypeRef> argumentos;
        public SimpleTypeRef(String name, int line, int column) {
            super(line, column);
            this.name = name;
        }
    }

    public static final class ArrayTypeRef extends TypeRef {
        public final TypeRef element;
        public final IExpr size;    // null si no se especifica tamaño
        public ArrayTypeRef(TypeRef element, IExpr size, int line, int column) {
            super(line, column);
            this.element = element;
            this.size = size;
        }
    }

    // Tipo tupla `(T1, T2, ...)` — solo como tipo de retorno (tuplas para
    // retorno múltiple; no son valores de primera clase). >= 2 elementos.
    public static final class TupleTypeRef extends TypeRef {
        public final java.util.List<TypeRef> elements;
        public TupleTypeRef(java.util.List<TypeRef> elements, int line, int column) {
            super(line, column);
            this.elements = elements;
        }
    }

    // ============================================================
    // HELPERS COMPARTIDOS
    // ============================================================
    public static final class DeclName extends Node {
        public final String classQualifier;  // null para no estático
        public final String name;
        public DeclName(String classQualifier, String name, int line, int column) {
            super(line, column);
            this.classQualifier = classQualifier;
            this.name = name;
        }
        public boolean isStatic() { return classQualifier != null; }
    }

    public static final class Param extends Node {
        public final String name;
        public final TypeRef type;
        public final IExpr defaultValue;   // H8.1: valor por defecto (literal const) o null
        public Param(String name, TypeRef type, int line, int column) {
            this(name, type, null, line, column);
        }
        public Param(String name, TypeRef type, IExpr defaultValue, int line, int column) {
            super(line, column);
            this.name = name;
            this.type = type;
            this.defaultValue = defaultValue;
        }
    }

    public static final class EnumValue extends Node {
        public final String name;
        public final Long explicitValue;   // null si va por defecto
        public EnumValue(String name, Long explicitValue, int line, int column) {
            super(line, column);
            this.name = name;
            this.explicitValue = explicitValue;
        }
    }

    public static final class GetterDef extends Node {
        public final List<IStmt> body;
        public GetterDef(List<IStmt> body, int line, int column) {
            super(line, column);
            this.body = body;
        }
    }

    public static final class SetterDef extends Node {
        public final String paramName;     // el set requiere parámetro explícito
        public final List<IStmt> body;
        public SetterDef(String paramName, List<IStmt> body, int line, int column) {
            super(line, column);
            this.paramName = paramName;
            this.body = body;
        }
    }

    // ============================================================
    // DECLARACIONES (top-level y miembros)
    // ============================================================
    public static final class ConstDecl extends Node implements ITopLevelDecl, IStmt {
        public final boolean isPublic;
        public final DeclName name;
        public final TypeRef type;     // null si se infiere
        public final IExpr value;
        public ConstDecl(boolean isPublic, DeclName name, TypeRef type, IExpr value, int line, int column) {
            super(line, column);
            this.isPublic = isPublic;
            this.name = name;
            this.type = type;
            this.value = value;
        }
    }

    public static final class VarDecl extends Node implements ITopLevelDecl, IStmt {
        public final boolean isPublic;
        /** #508 — `private var`: sólo lo ve su propia clase, ni sus descendientes. Se pega
         *  tras construir el nodo (como en FuncDef); hasta el 14-sep el parser lo tiraba. */
        public boolean isPrivate = false;
        public final boolean isOwner;        // `var owner x: T := ...`
        public final List<DeclName> names;   // soporta 'var x, y, z: integer'
        public final TypeRef type;
        public IExpr init;                   // null si no hay inicializador (V8 #774: una cadena de módulo o de clase sin él, "")
        public VarDecl(boolean isPublic, boolean isOwner, List<DeclName> names, TypeRef type, IExpr init, int line, int column) {
            super(line, column);
            this.isPublic = isPublic;
            this.isOwner = isOwner;
            this.names = names;
            this.type = type;
            this.init = init;
        }
    }

    /**
     * H5.c — `raise onClick(args)`: DISPARA un evento del propio objeto.
     *
     * Verbo propio, y no una llamada, porque las tres cosas que hace son
     * distintas de llamar: puede no hacer NADA (si nadie se suscribió), no es
     * inmediato (se encola y lo ejecuta el thread dueño) y nunca devuelve valor.
     * Escribirlo `this.onClick(args)` haría que la sintaxis mintiera sobre las
     * tres. La simetría es con `throw` (Eduardo): throw dispara un error, raise
     * dispara un evento.
     */
    public static final class RaiseStmt extends Node implements IStmt {
        public final String event;        // nombre del evento (del propio objeto)
        /** Argumentos EFECTIVOS del disparo. Pueden ser menos que los declarados:
         *  los que falten los rellena el emisor con su valor por defecto (H8.1,
         *  sustitución en el llamante) — así un evento puede GANAR parámetros
         *  después sin romper los `raise` ya escritos. */
        public final List<IExpr> args;
        public RaiseStmt(String event, List<IExpr> args, int line, int column) {
            super(line, column);
            this.event = event;
            this.args = args;
        }
    }

    /**
     * H5.c — REFERENCIA a un método: `obj::onTick`, `this::pulsar`.
     *
     * No es una llamada ni un valor de primera clase: sólo vale a la derecha de
     * una asignación a un evento. El semántico lo comprueba y lo dice claro si
     * aparece en otro sitio — hacerlo un valor suelto obligaría a inventar un
     * tipo "referencia a función" en el sistema de tipos, y el diseño no lo
     * necesita (un suscriptor, asignación y ya).
     *
     * Se escribe `::` y no `.` por una razón de BP, no por imitar a Java: aquí
     * `obj.algo` sin paréntesis ya significa "lee la property", y una property
     * EJECUTA código. Con el mismo símbolo, "dame el valor" y "dame la función"
     * serían indistinguibles.
     */
    public static final class MethodRefExpr extends Node implements IExpr {
        public final IExpr target;      // el receptor: this, una var, ...
        public final String method;     // nombre del método
        public MethodRefExpr(IExpr target, String method, int line, int column) {
            super(line, column);
            this.target = target;
            this.method = method;
        }
    }

    /**
     * #325 — LLAMADA PREPARADA: {@code obj::metodo(args)}.
     *
     * Es una {@link MethodRefExpr} con los argumentos ya puestos: receptor,
     * método y valores, pero SIN llamar. Los argumentos se evalúan donde está
     * escrita la expresión (no donde se ejecute luego), que es lo que la
     * distingue de una closure — BP no tiene, y por eso los valores viajan
     * como campos de la clase que sintetiza el emisor.
     *
     * En V4 el único sitio donde vale es {@code Thread(obj::metodo(args))}: el
     * semántico la rechaza en cualquier otro, para no prometer un valor de
     * "llamada diferida" de primera clase que no está diseñado. La sintaxis ya
     * está puesta si algún día hace falta (un timer, un callback).
     */
    public static final class BoundCallExpr extends Node implements IExpr {
        public final IExpr target;        // el receptor
        public final String method;       // nombre del método
        public final List<IExpr> args;    // argumentos, ya evaluados en el llamante
        /** Nombre de la subclase de Thread que sintetiza el emisor (lo pone él). */
        public String synthesizedClassName;
        public BoundCallExpr(IExpr target, String method, List<IExpr> args, int line, int column) {
            super(line, column);
            this.target = target;
            this.method = method;
            this.args = args;
        }
    }

    /**
     * H5.c — declaración de un EVENTO: `event onClick`.
     *
     * Declara su FIRMA y nunca devuelve valor. El handler al que se suscriba
     * tiene que casar con ella exactamente; la comprobación reusa el mangling
     * de H5.a, que además resuelve la sobrecarga (ver docs/V4_IDEAS.md §H5.c).
     *
     * No lleva `isPublic`: el acceso de un evento es ASIMÉTRICO por definición —
     * asignarlo (suscribirse) es público, leerlo y dispararlo es de la clase y
     * sus descendientes. No es una visibilidad que se elija, es lo que ES un
     * evento; por eso tiene palabra propia en vez de ser una `property` con
     * modificadores.
     *
     * En memoria son DOS campos consecutivos (receptor de 8B + destino de 4B),
     * no un campo de 3 slots: así reusa `declareField` tal cual y el bit de GC
     * cae solo en el slot del receptor. Ver computeClassLayout.
     */
    public static final class EventDef extends Node implements ITopLevelDecl {
        public final DeclName name;
        /** H5.c (revisión) — el evento DECLARA su firma. Con eso el `kind`
         *  desaparece para los eventos con nombre: el discriminante deja de ser
         *  un dato en ejecución y pasa a ser la SUSCRIPCIÓN. Y el sender va
         *  tipado, así que no hay casteo en el handler.
         *  Admite valores por defecto (H8.1) para que un evento pueda GANAR
         *  parámetros después sin romper los `raise` ya escritos — importante
         *  en una librería que se publica. */
        public final List<Param> params;
        public EventDef(DeclName name, List<Param> params, int line, int column) {
            super(line, column);
            this.name = name;
            this.params = params;
        }
    }

    public static final class FuncDef extends Node implements ITopLevelDecl {
        public final boolean isPublic;
        public final boolean isFinal;
        /** Función `intrinsic`: sólo signature; el compilador inlinea opcodes en el call site. */
        public final boolean isIntrinsic;
        /** H3 #162 — función `native`: candidata a tener versión AOT C.
         *  El bytecode .mod se emite igual; el flag se propaga al .bpi
         *  y al emisor AOT (#157). El runtime ignora el flag — quien
         *  decide ejecutar AOT vs bytecode es el AOT registry tras link. */
        public final boolean isNative;

        /** V5/H4 — `native` EXTERNA: sin cuerpo, resuelta al campo homónimo de
         *  la tabla que publica un pack (`import v1 from pack "SQLI"`). Mutable
         *  como `isEventHandler`: la marca el parser tras construir el nodo. */
        public boolean isPackExtern = false;
        /** #390 — `private` EXPLÍCITO. Mutable por el mismo motivo que los dos de
         *  arriba: lo pega el parser sobre el nodo ya construido. Sin marcar, un
         *  miembro de clase es *protected* (clase + descendientes). */
        public boolean isPrivate = false;
        public final DeclName name;
        public final List<Param> params;
        public final TypeRef returnType;     // null si void
        public final List<IStmt> body;
        /**
         * H5.c — `function event pulsado()`: esta función puede colgar de un
         * evento, así que necesita SLOT DE VTABLE aunque sea privada (el destino
         * de un evento es un slot; sin él no hay a dónde apuntar).
         *
         * Es un modificador DECLARATIVO a propósito. Dárselo en su lugar a "toda
         * privada referenciada con `::`" haría que el layout de la clase
         * dependiese de los CUERPOS, y la pasada interfaz-only no los analiza:
         * el layout publicado saldría distinto del emitido y el desajuste
         * aparecería cross-module y en silencio. Declarándolo, el layout sale de
         * la declaración — igual que con las properties.
         *
         * Mutable y no en el constructor porque FuncDef ya tiene tres
         * constructores telescópicos; una cuarta bandera los multiplicaría.
         */
        public boolean isEventHandler = false;
        /** V7/H1 (21-sep) — `function ~Clase()`: el DESTRUCTOR, el contrario del constructor (diseño
         *  de V5_IDEAS, 9-ago). Su nombre interno es `__dtor`; el usuario escribe `~Clase` y
         *  `end ~Clase`. Lo llama el lenguaje —al morir un `var owner` local, al salir de un
         *  `try var`— nunca el usuario; al acabar su cuerpo, corre el del padre. */
        public boolean isDestructor = false;
        /** El nombre que el usuario escribió tras `~` (tiene que ser el de su clase); `name` es `__dtor`. */
        public String dtorEscrito = null;
        public FuncDef(boolean isPublic, boolean isFinal, DeclName name, List<Param> params,
                       TypeRef returnType, List<IStmt> body, int line, int column) {
            this(isPublic, isFinal, false, false, name, params, returnType, body, line, column);
        }
        public FuncDef(boolean isPublic, boolean isFinal, boolean isIntrinsic, DeclName name,
                       List<Param> params, TypeRef returnType, List<IStmt> body, int line, int column) {
            this(isPublic, isFinal, isIntrinsic, false, name, params, returnType, body, line, column);
        }
        public FuncDef(boolean isPublic, boolean isFinal, boolean isIntrinsic, boolean isNative,
                       DeclName name, List<Param> params, TypeRef returnType, List<IStmt> body,
                       int line, int column) {
            super(line, column);
            this.isPublic = isPublic;
            this.isFinal = isFinal;
            this.isIntrinsic = isIntrinsic;
            this.isNative = isNative;
            this.name = name;
            this.params = params;
            this.returnType = returnType;
            this.body = body;
        }
    }

    public static final class PropertyDef extends Node implements ITopLevelDecl {
        public final boolean isPublic;
        /** #508 — `private property`: como en VarDecl. */
        public boolean isPrivate = false;
        public final boolean isFinal;
        public final boolean isOwner;        // `public property owner foo: T := ...`
        public final boolean isSync;         // `public sync property foo: T` — getter/setter envuelven con un Mutex compartido por instancia.
        public final DeclName name;
        public final TypeRef type;
        public IExpr init;                   // null si no hay inicializador (V8 #774: una cadena de módulo o de clase sin él, "")
        public final GetterDef getter;       // null en forma corta o si se omite
        public final SetterDef setter;       // null en forma corta o si se omite
        public final boolean isShortForm;    // true => get/set implícitos
        public PropertyDef(boolean isPublic, boolean isFinal, boolean isOwner, boolean isSync, DeclName name, TypeRef type,
                           IExpr init, GetterDef getter, SetterDef setter, boolean isShortForm,
                           int line, int column) {
            super(line, column);
            this.isPublic = isPublic;
            this.isFinal = isFinal;
            this.isOwner = isOwner;
            this.isSync = isSync;
            this.name = name;
            this.type = type;
            this.init = init;
            this.getter = getter;
            this.setter = setter;
            this.isShortForm = isShortForm;
        }
    }

    public static final class ClassDef extends Node implements ITopLevelDecl {
        public final boolean isPublic;
        public final String name;
        public final String baseClass;        // null si no hay 'extends'
        /** H1 P3 (20-sep) — `public root class Object`: una clase que NO desciende de nada (Eduardo:
         *  «'root' o algo similar para indicar que no desciende de nada»). Sólo la raíz (Core) puede
         *  escribirlo: `Object` y `Thread`. Sin `root` y sin `extends`, la base es Object. */
        public boolean isRoot = false;
        /** V8 H7.3 (`#756`) — una INTERFAZ (`interface Forma … end Forma`; en JC, `interface IForma { … }`): se
         *  guarda como una clase con su marca (Eduardo: «una clase y una interfaz se guardan igual o casi igual»).
         *  Sus miembros, métodos sin cuerpo y properties en forma corta; `baseClass`, null; y `interfaces`, las
         *  interfaces de las que hereda. */
        public boolean isInterface = false;
        /** V8 H7.3 — las interfaces que la clase implementa, como se escriben (`implements Forma, Lib.Medible`); en
         *  una interfaz, las de las que hereda. Vacía si no hay. */
        public final List<String> interfaces = new ArrayList<>();
        /** V8 H7.3 — JC escribe el padre y las interfaces en la misma lista (`class C : A, B`), como C#, y el parser
         *  no sabe si `A` es una clase o una interfaz: lo pone en {@link #baseClass} y lo marca aquí, y el semántico
         *  lo pasa a las interfaces si lo es. En BP (`extends`/`implements`), false. Es una marca de JC: la
         *  comparación de árboles no la cuenta (ComparaAst). */
        public boolean primeroPuedeSerInterfaz = false;
        public final List<ITopLevelDecl> members;
        public ClassDef(boolean isPublic, String name, String baseClass, List<ITopLevelDecl> members,
                        int line, int column) {
            super(line, column);
            this.isPublic = isPublic;
            this.name = name;
            this.baseClass = baseClass;
            this.members = members;
        }
    }

    public static final class EnumDef extends Node implements ITopLevelDecl {
        public final boolean isPublic;
        public final String name;
        public final List<EnumValue> values;
        public EnumDef(boolean isPublic, String name, List<EnumValue> values, int line, int column) {
            super(line, column);
            this.isPublic = isPublic;
            this.name = name;
            this.values = values;
        }
    }

    // ============================================================
    // SENTENCIAS
    // ============================================================
    public static final class AssignStmt extends Node implements IStmt {
        public final IExpr target;     // primary
        public final AssignOpKind op;
        public final IExpr value;
        public AssignStmt(IExpr target, AssignOpKind op, IExpr value, int line, int column) {
            super(line, column);
            this.target = target;
            this.op = op;
            this.value = value;
        }
    }

    // Desempaquetado de tupla: `{ t1, t2, ... } := call()` sobre lvalues YA
    // declarados. Un target puede ser IdentifierExpr "_" (descartar). El value
    // es una llamada cuyo tipo de retorno es una tupla de la misma aridad.
    public static final class DestructAssignStmt extends Node implements IStmt {
        public final List<IExpr> targets;
        public final IExpr value;
        public DestructAssignStmt(List<IExpr> targets, IExpr value, int line, int column) {
            super(line, column);
            this.targets = targets;
            this.value = value;
        }
    }

    public static final class IfClause extends Node {
        public final IExpr condition;
        public final List<IStmt> body;
        public IfClause(IExpr condition, List<IStmt> body, int line, int column) {
            super(line, column);
            this.condition = condition;
            this.body = body;
        }
    }

    public static final class IfStmt extends Node implements IStmt {
        public final IfClause then_;
        public final List<IfClause> elseIfs;
        public final List<IStmt> else_;       // null si no hay 'else'
        public final boolean isSingleLine;
        public IfStmt(IfClause then_, List<IfClause> elseIfs, List<IStmt> else_, boolean isSingleLine,
                      int line, int column) {
            super(line, column);
            this.then_ = then_;
            this.elseIfs = elseIfs;
            this.else_ = else_;
            this.isSingleLine = isSingleLine;
        }
    }

    public static final class CaseClause extends Node {
        public final List<IExpr> values;
        public final List<IStmt> body;
        public CaseClause(List<IExpr> values, List<IStmt> body, int line, int column) {
            super(line, column);
            this.values = values;
            this.body = body;
        }
    }

    public static final class SwitchStmt extends Node implements IStmt {
        public final IExpr subject;
        public final List<CaseClause> cases;
        public final List<IStmt> defaultBody;   // null si no hay 'default'
        public SwitchStmt(IExpr subject, List<CaseClause> cases, List<IStmt> defaultBody,
                          int line, int column) {
            super(line, column);
            this.subject = subject;
            this.cases = cases;
            this.defaultBody = defaultBody;
        }
    }

    /**
     * Rama de un statement {@code parallel}: {@code case <var> := Thread(<stackSize>)}
     * seguido de un cuerpo. Cada rama se desazucara en una subclase anónima de
     * Thread cuyo run() ES el {@code body}.
     */
    public static final class ParallelBranch extends Node {
        public final String varName;        // ej. "t1"
        public final IExpr  stackSizeExpr;  // expresión int para super(stackSize)
        public final List<IStmt> body;      // cuerpo del run()
        /** Nombre de la clase sintetizada; lo fija el semántico. */
        public String synthesizedClassName;
        public ParallelBranch(String varName, IExpr stackSizeExpr, List<IStmt> body, int line, int column) {
            super(line, column);
            this.varName = varName;
            this.stackSizeExpr = stackSizeExpr;
            this.body = body;
        }
    }

    /**
     * Statement {@code parallel ... endpar}: arranca N threads, ejecuta el bloque
     * default (si existe) en el thread llamador, y joinea todos al salir. Es
     * azúcar sintáctico: cada rama → subclase anónima de Thread.
     */
    public static final class ParallelStmt extends Node implements IStmt {
        public final List<ParallelBranch> branches;
        public final List<IStmt> defaultBody;   // null si no hay 'default'
        public ParallelStmt(List<ParallelBranch> branches, List<IStmt> defaultBody, int line, int column) {
            super(line, column);
            this.branches = branches;
            this.defaultBody = defaultBody;
        }
    }

    public static final class WhileStmt extends Node implements IStmt {
        public final IExpr condition;
        public final List<IStmt> body;
        public final boolean isSingleLine;
        public String nombre;               // V8/H8: `while c do miLoop`, como el de DoLoopStmt
        public WhileStmt(IExpr condition, List<IStmt> body, boolean isSingleLine, int line, int column) {
            super(line, column);
            this.condition = condition;
            this.body = body;
            this.isSingleLine = isSingleLine;
        }
    }

    public static final class DoLoopStmt extends Node implements IStmt {
        public final List<IStmt> body;
        public final IExpr condition;       // null => bucle infinito
        public final String nombre;         // V8/H8: `do miLoop`, al que van `break miLoop` y `continue miLoop`; null si no lleva
        public DoLoopStmt(List<IStmt> body, IExpr condition, String nombre, int line, int column) {
            super(line, column);
            this.body = body;
            this.condition = condition;
            this.nombre = nombre;
        }
    }

    public static abstract class ForRange extends Node {
        protected ForRange(int line, int column) { super(line, column); }
    }

    public static final class ForNumericRange extends ForRange {
        public final IExpr from;
        public final IExpr to;
        public final IExpr step;            // null si no hay 'step'
        public ForNumericRange(IExpr from, IExpr to, IExpr step, int line, int column) {
            super(line, column);
            this.from = from;
            this.to = to;
            this.step = step;
        }
    }

    public static final class ForInRange extends ForRange {
        public final IExpr iterable;
        public ForInRange(IExpr iterable, int line, int column) {
            super(line, column);
            this.iterable = iterable;
        }
    }

    public static final class ForStmt extends Node implements IStmt {
        public final String iteratorName;
        public final ForRange range;
        public final List<IStmt> body;
        public final boolean isSingleLine;
        public String nombre;               // V8/H8: `for i := 1 to 3 do miLoop`, como el de DoLoopStmt
        public ForStmt(String iteratorName, ForRange range, List<IStmt> body, boolean isSingleLine,
                       int line, int column) {
            super(line, column);
            this.iteratorName = iteratorName;
            this.range = range;
            this.body = body;
            this.isSingleLine = isSingleLine;
        }
    }

    public static final class CatchClause extends Node {
        public final String varName;        // null si 'catch' a secas
        public final String exceptionType;  // null si 'catch err' (sin tipo)
        public final List<IStmt> body;
        public CatchClause(String varName, String exceptionType, List<IStmt> body, int line, int column) {
            super(line, column);
            this.varName = varName;
            this.exceptionType = exceptionType;
            this.body = body;
        }
    }

    public static final class TryStmt extends Node implements IStmt {
        public final List<IStmt> body;
        public final List<CatchClause> catches;
        public final List<IStmt> finallyBody;   // null si no hay 'finally'
        /** V7/H1 (21-sep) — `try var q: T := expr` (idea de Eduardo, 9-ago: el try-with-resources
         *  de Java sin gastar palabras): el recurso es un `var owner` local declarado ANTES del
         *  try, y el finally (creado por el parser) lo destruye, lo libera y lo pone a null al
         *  salir — salgas por donde salgas. Azúcar del parser: cero cambios en las VMs. */
        public VarDecl recurso = null;
        public TryStmt(List<IStmt> body, List<CatchClause> catches, List<IStmt> finallyBody,
                       int line, int column) {
            super(line, column);
            this.body = body;
            this.catches = catches;
            this.finallyBody = finallyBody;
        }
    }

    public static final class ReturnStmt extends Node implements IStmt {
        public final IExpr value;       // null si 'return' a secas
        public ReturnStmt(IExpr value, int line, int column) {
            super(line, column);
            this.value = value;
        }
    }

    public static final class ThrowStmt extends Node implements IStmt {
        public final IExpr value;
        public ThrowStmt(IExpr value, int line, int column) {
            super(line, column);
            this.value = value;
        }
    }

    public static final class PrintItem {
        public final PrintSep leadingSep;   // NONE solo en el primer item
        public final IExpr expr;            // null permitido tras separador
        public PrintItem(PrintSep leadingSep, IExpr expr) {
            this.leadingSep = leadingSep;
            this.expr = expr;
        }
    }

    public static final class PrintStmt extends Node implements IStmt {
        public final List<PrintItem> items;
        public PrintStmt(List<PrintItem> items, int line, int column) {
            super(line, column);
            this.items = items;
        }
        /** V8 #700 — un separador al final (`print a; b;`) no salta de línea, como en el BASIC clásico. Sus piezas
         *  esperan en la línea del hilo y salen con el siguiente salto, o al acabar el hilo (#600). Es el `print` de
         *  JC, que no salta solo (#698). */
        public boolean sinSalto() {
            return !items.isEmpty() && items.get(items.size() - 1).expr == null;
        }
    }

    public static final class BreakStmt extends Node implements IStmt {
        public final String nombre;         // V8/H8: `break miLoop`; null, el bucle o el switch más cercano
        public BreakStmt(String nombre, int line, int column) { super(line, column); this.nombre = nombre; }
    }

    public static final class ContinueStmt extends Node implements IStmt {
        public final String nombre;         // V8/H8: `continue miLoop`; null, el bucle más cercano
        public ContinueStmt(String nombre, int line, int column) { super(line, column); this.nombre = nombre; }
    }

    /** H1 (20-sep) — `inline <MNEMÓNICO> [operandos]`: una línea de bytecode escrita en el
     *  cuerpo de una función. Una función cuyo cuerpo son SÓLO líneas `inline` es una
     *  intrínseca (Eduardo, de Turbo Pascal: «una función normal; la diferencia es el
     *  cuerpo»): no se emite en el .mod y cada llamada la inlina. El id de un builtin es el
     *  número escrito: `inline CALL_BUILTIN 23`. */
    public static final class InlineStmt extends Node implements IStmt {
        public final String mnemonic;
        public final List<Long> operands;
        public InlineStmt(String mnemonic, List<Long> operands, int line, int column) {
            super(line, column);
            this.mnemonic = mnemonic;
            this.operands = operands;
        }
        /** El texto canónico, el que viaja en la interfaz: `CALL_BUILTIN 23`. */
        public String texto() {
            StringBuilder sb = new StringBuilder(mnemonic);
            for (Long o : operands) sb.append(' ').append(o);
            return sb.toString();
        }
        /** La lectura del texto de la interfaz: `CALL_BUILTIN 23; DUP` → lista. */
        public static List<InlineStmt> deTexto(String texto) {
            List<InlineStmt> out = new ArrayList<>();
            for (String linea : texto.split(";")) {
                String[] t = linea.trim().split("\\s+");
                if (t.length == 0 || t[0].isEmpty()) continue;
                List<Long> ops = new ArrayList<>();
                for (int i = 1; i < t.length; i++) ops.add(Long.parseLong(t[i]));
                out.add(new InlineStmt(t[0], ops, 0, 0));
            }
            return out;
        }
        public static String textoDe(List<InlineStmt> ops) {
            StringBuilder sb = new StringBuilder();
            for (InlineStmt s : ops) { if (sb.length() > 0) sb.append("; "); sb.append(s.texto()); }
            return sb.toString();
        }
    }

    /** V7/H1 — «destruye, libera y pon a null este owner local»: lo pone el parser en el finally de
     *  un `try var` y lo emite el emisor (el semántico no tiene nada que decir). */
    public static final class ReleaseOwnerStmt extends Node implements IStmt {
        public final String name;
        public ReleaseOwnerStmt(String name, int line, int column) { super(line, column); this.name = name; }
    }

    public static final class ExprStmt extends Node implements IStmt {
        public final IExpr expr;
        public ExprStmt(IExpr expr, int line, int column) {
            super(line, column);
            this.expr = expr;
        }
    }

    // ============================================================
    // EXPRESIONES
    // ============================================================
    public static final class IntLitExpr extends Node implements IExpr {
        public final long value;
        public IntLitExpr(long value, int line, int column) { super(line, column); this.value = value; }
    }

    public static final class FloatLitExpr extends Node implements IExpr {
        public final double value;
        public FloatLitExpr(double value, int line, int column) { super(line, column); this.value = value; }
    }

    // H1.2 (V2): literal long (i64), p.ej. 5L / 3000000000L.
    public static final class LongLitExpr extends Node implements IExpr {
        public final long value;
        public LongLitExpr(long value, int line, int column) { super(line, column); this.value = value; }
    }

    // H1.3 (V2): literal double (f64), p.ej. 1.5d / 3.14159265358979d.
    public static final class DoubleLitExpr extends Node implements IExpr {
        public final double value;
        public DoubleLitExpr(double value, int line, int column) { super(line, column); this.value = value; }
    }

    public static final class StringLitExpr extends Node implements IExpr {
        public final String value;
        /** V8 #774 — el `""` que el compilador pone a una cadena de módulo o de clase sin valor: no lo escribió el
         *  programa, así que los escritores BP ↔ JC no lo escriben (lo vuelve a poner el compilador al otro lado). */
        public boolean implicito;
        public StringLitExpr(String value, int line, int column) { super(line, column); this.value = value; }
    }

    /** V8 #774 — ¿es éste un valor inicial que puso el compilador (el `""` de una cadena sin valor)? */
    public static boolean implicito(IExpr e) {
        return e instanceof StringLitExpr && ((StringLitExpr) e).implicito;
    }

    public static final class BoolLitExpr extends Node implements IExpr {
        public final boolean value;
        public BoolLitExpr(boolean value, int line, int column) { super(line, column); this.value = value; }
    }

    public static final class NullLitExpr extends Node implements IExpr {
        public NullLitExpr(int line, int column) { super(line, column); }
    }

    public static final class ThisExpr extends Node implements IExpr {
        public ThisExpr(int line, int column) { super(line, column); }
    }

    public static final class SuperExpr extends Node implements IExpr {     // para super.foo
        public SuperExpr(int line, int column) { super(line, column); }
    }

    public static final class FieldExpr extends Node implements IExpr {     // contextual
        public FieldExpr(int line, int column) { super(line, column); }
    }

    public static final class SuperCallExpr extends Node implements IExpr {
        public final List<IExpr> args;
        public SuperCallExpr(List<IExpr> args, int line, int column) {
            super(line, column);
            this.args = args;
        }
    }

    public static final class IdentifierExpr extends Node implements IExpr {
        public final String name;
        public IdentifierExpr(String name, int line, int column) {
            super(line, column);
            this.name = name;
        }
    }

    public static final class ArrayLitExpr extends Node implements IExpr {
        public final List<IExpr> elements;
        public ArrayLitExpr(List<IExpr> elements, int line, int column) {
            super(line, column);
            this.elements = elements;
        }
    }

    public static final class ParenExpr extends Node implements IExpr {
        public final IExpr inner;
        public ParenExpr(IExpr inner, int line, int column) {
            super(line, column);
            this.inner = inner;
        }
    }

    public static final class MemberAccessExpr extends Node implements IExpr {
        public final IExpr target;
        public final String member;
        public MemberAccessExpr(IExpr target, String member, int line, int column) {
            super(line, column);
            this.target = target;
            this.member = member;
        }
    }

    public static final class IndexExpr extends Node implements IExpr {
        public final IExpr target;
        public final IExpr index;
        /** V8/H8 — JC: se escribió `new A[n]`. El árbol es el `A[n]` de BP; la marca, cómo se escribió (como la de CallExpr). */
        public boolean conNew;
        public IndexExpr(IExpr target, IExpr index, int line, int column) {
            super(line, column);
            this.target = target;
            this.index = index;
        }
    }

    public static final class CallExpr extends Node implements IExpr {
        public final IExpr callee;
        public final List<IExpr> args;
        /** V8 #698 (JC) — en BP crear, llamar y convertir se escriben igual, `X(…)`; en JC, `new X(…)` y `(X) x`. El
         *  parser de JC lo marca aquí para que el análisis pueda comprobarlo; el de BP las deja a false. No son parte
         *  del programa: la comparación de árboles no las cuenta (ComparaAst). */
        public boolean conNew;
        public boolean esConversion;
        /** V8/H7.1 (#747) — `c ? a : b` en JC: el `iif(c, a, b)` de BP. Tampoco cuenta en la comparación. */
        public boolean esTernario;
        /** V8/H7.2 (#749) — `new List<Object>()` o `new Collections.Map<int, Object>()` en JC: los argumentos de tipo
         *  escritos, y {@link #claseNew} la clase escrita cuando la llamada de BP es otra (`Collections.numericMap()`
         *  para un Map de llave entera). Tampoco cuentan en la comparación. */
        public List<TypeRef> argumentosNew;
        public String claseNew;
        public CallExpr(IExpr callee, List<IExpr> args, int line, int column) {
            super(line, column);
            this.callee = callee;
            this.args = args;
        }
    }

    // Tupla literal `(e1, e2, ...)` — SOLO válida como valor de `return` en una
    // función con tipo de retorno tupla. >= 2 elementos. No es valor de primera
    // clase: el semántico rechaza usarla en cualquier otro contexto.
    public static final class TupleExpr extends Node implements IExpr {
        public final List<IExpr> elements;
        public TupleExpr(List<IExpr> elements, int line, int column) {
            super(line, column);
            this.elements = elements;
        }
    }

    public static final class UnaryExpr extends Node implements IExpr {
        public final String op;       // "-" o "not"
        public final IExpr operand;
        public UnaryExpr(String op, IExpr operand, int line, int column) {
            super(line, column);
            this.op = op;
            this.operand = operand;
        }
    }

    public static final class BinaryExpr extends Node implements IExpr {
        public final String op;       // "+", "-", "*", "/", "mod", ..., "and", "or", "==", ...
        public final IExpr left;
        public final IExpr right;
        public BinaryExpr(String op, IExpr left, IExpr right, int line, int column) {
            super(line, column);
            this.op = op;
            this.left = left;
            this.right = right;
        }
    }

    /** Operador 'instanceof': la parte derecha NO es una expresión sino el
     *  nombre de una clase (resuelto en análisis semántico). Resultado boolean. */
    public static final class InstanceOfExpr extends Node implements IExpr {
        public final IExpr target;
        public final String typeName;
        public InstanceOfExpr(IExpr target, String typeName, int line, int column) {
            super(line, column);
            this.target = target;
            this.typeName = typeName;
        }
    }
}
