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
        public final String baseClassName; // nombre crudo de 'extends X' (null si no hay)
        public ClassSymbol baseClass;      // resuelto en pase 2
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

        public ClassSymbol(String name, boolean isPublic, String baseClassName,
                           Ast.ClassDef ast, int line, int column) {
            super(name, line, column);
            this.isPublic = isPublic;
            this.baseClassName = baseClassName;
            this.astNode = ast;
            this.decl = ast;
        }

        /** Busca un miembro de instancia subiendo por la cadena de herencia. */
        public Symbol lookupInstance(String n) {
            ClassSymbol c = this;
            while (c != null) {
                Symbol s = c.instanceMembers.tryLookup(n);
                if (s != null) return s;
                c = c.baseClass;
            }
            return null;
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

        /** Computa el mapa nombre→slot replicando FIELMENTE el orden de
         *  ModWriter/emitClassDef: Object root (toString=0, compareTo=1) →
         *  heredados del padre → accesores get/set de properties públicas de
         *  instancia → métodos públicos de instancia (no ctor, no static), todo
         *  en orden de declaración del AST; un override reusa el slot heredado.
         *  Privados/estáticos NO entran (no virtuales). Clase externa: usa
         *  externalMethodSlots (sembrado desde el .bpi). */
        private void ensureMethodSlots() {
            if (methodSlots != null) return;
            if (isExternal) { methodSlots = externalMethodSlots; return; }
            java.util.Map<String, Integer> m = new java.util.LinkedHashMap<>();
            int nextSlot;
            if (baseClass != null) {
                baseClass.ensureMethodSlots();
                m.putAll(baseClass.methodSlots);
                nextSlot = (baseClass.isExternal && baseClass.binaryLayout != null)
                         ? baseClass.binaryLayout.numMethods
                         : m.size();
            } else {
                // Raíz implícita Object: slots 0/1 = toString/compareTo.
                m.put("toString", 0);
                m.put("compareTo", 1);
                nextSlot = 2;
            }
            if (astNode != null && astNode.members != null) {
                // Properties públicas de instancia PRIMERO (get/set), orden de decl.
                for (Ast.ITopLevelDecl d : astNode.members) {
                    if (!(d instanceof Ast.PropertyDef)) continue;
                    Ast.PropertyDef pd = (Ast.PropertyDef) d;
                    if (!pd.isPublic) continue;
                    if (pd.name != null && pd.name.isStatic()) continue;
                    String nm = pd.name.name;
                    String cap = Character.toUpperCase(nm.charAt(0)) + nm.substring(1);
                    m.put("get" + cap, nextSlot++);
                    m.put("set" + cap, nextSlot++);
                }
                // Métodos públicos de instancia (no ctor, no static), orden de decl.
                for (Ast.ITopLevelDecl d : astNode.members) {
                    if (!(d instanceof Ast.FuncDef)) continue;
                    Ast.FuncDef fn = (Ast.FuncDef) d;
                    Symbol s = instanceMembers.tryLookup(fn.name.name);
                    if (!(s instanceof FunctionSymbol)) continue;
                    FunctionSymbol fs = (FunctionSymbol) s;
                    if (!fs.isPublic || fs.isStatic || fs.isConstructor) continue;
                    if (!m.containsKey(fn.name.name)) m.put(fn.name.name, nextSlot++);
                }
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
        public boolean isModuleInitializer;
        /** Función intrínseca: declarada sólo como signature; el emisor inlinea
         *  opcodes en cada call-site en lugar de generar CALL / CALL_EXT. El
         *  cuerpo (si existe en el AST) se ignora; el .mod no contiene código
         *  para esta función. */
        public boolean isIntrinsic;
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
            sb.append(externalModule).append('.').append(name);
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
    }

    // ============================================================
    // Parámetros
    // ============================================================
    public static final class ParamSymbol extends Symbol {
        public BpType type;
        public ParamSymbol(String name, int line, int column) { super(name, line, column); }
    }

    // ============================================================
    // Variables, constantes, propiedades
    // ============================================================
    public static final class VarSymbol extends Symbol {
        public final boolean isPublic;
        public final boolean isStatic;
        public final ClassSymbol ownerClass;
        public final boolean isLocal;
        /** Si esta variable es un "owner" (var owner): destruye el objeto al
         *  salir de scope o al reasignarse. Solo válido si type es ClassType. */
        public boolean isOwner;
        public BpType type;

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
        public final Map<String, Long> values = new HashMap<>();

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
}
