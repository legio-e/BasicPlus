// ============================================================
// ModuleInterface.java
// Representación de la "interfaz" pública de un módulo BasicPlus.
//
// Equivalente al DEFINITION MODULE de Modula-2: contiene firmas
// públicas pero ningún cuerpo de función. Sirve para que un módulo
// X que importa M pueda chequear tipos contra M sin necesidad de
// compilar el cuerpo de M.
//
// Formato textual del fichero .bpi (BasicPlus Interface):
//
//   bpi 2
//   library com.example.app
//   module Util
//   func sum(a:integer,b:integer):integer public
//   func sqrt(x:float):float public
//   func saludar(nombre:string):void public
//   const PI:float=3.14159 public
//   const VERSION:string="1.0" public
//   enum Color public
//     ROJO=0
//     VERDE=1
//     AZUL=2
//   end enum
//
// Reglas:
//   - 1ª línea: "bpi <version>"
//   - "library <name>": opcional; si se omite, library = ""
//   - "module <name>": obligatorio, una sola vez
//   - "func <name>(<params>):<ret> [public] [static]":
//       params es lista separada por comas, vacía permitida; cada uno es nombre:tipo
//       ret es un tipo primitivo o "void"
//       tipos primitivos soportados: integer, float, string, boolean
//   - "const <name>:<type>=<value> public":
//       sólo se exportan consts cuyo valor literal es conocido (int/float/string).
//   - "enum <name> public" + entradas "<MIEMBRO>=<int>" + "end enum":
//       enums públicos; los valores son los efectivos tras resolver alias.
//   - Líneas vacías y las que empiezan por "#" se ignoran.
//
// Tipos no-primitivos en signatures de func (clases, arrays) se siguen
// omitiendo en esta versión y se reportan en `skipped`. Cross-library
// con tipos referencia es una expansión posterior.
// ============================================================
package basicplus.frontend;

import basicplus.frontend.BpType.PrimitiveType;
import basicplus.frontend.BpType.VoidType;
import basicplus.frontend.Symbol.ConstSymbol;
import basicplus.frontend.Symbol.EnumSymbol;
import basicplus.frontend.Symbol.FunctionSymbol;
import basicplus.frontend.Symbol.ParamSymbol;
import basicplus.frontend.Symbol.PropertySymbol;
import basicplus.frontend.Symbol.VarSymbol;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class ModuleInterface {

    /** Versión mínima del .bpi que sabemos leer (cuando se sube, lo bumpamos). */
    public static final int CURRENT_VERSION = 7;   // v7: H8.1 valores por defecto de params
    public static final int MIN_SUPPORTED_VERSION = 1;

    public String library = "";
    public String moduleName = "";
    /** True si este .bpi describe un contrato (`module interface X`), no un módulo concreto. */
    public boolean isInterface = false;
    /** Para módulos concretos: nombre cualificado de la interfaz que declaran implementar (null si no). */
    public String implementsName = null;
    /** Para interfaces (isInterface=true): nombre cualificado de la interfaz padre que extiende (null si no). */
    public String extendsName = null;
    public final List<FuncSig> functions = new ArrayList<>();
    public final List<ConstSig> consts = new ArrayList<>();
    public final List<EnumSig> enums = new ArrayList<>();
    public final List<PropSig> properties = new ArrayList<>();
    public final List<ClassSig> classes = new ArrayList<>();

    /** Path original de donde se cargó el .bpi (informativo, puede ser null). */
    public String sourcePath;

    public static final class FuncSig {
        public final String name;
        public final boolean isPublic;
        public final boolean isStatic;
        /** Función intrínseca: el call-site emite opcodes inline en lugar de CALL_EXT.
         *  El .mod del módulo dueño NO contiene código para esta función. */
        public final boolean isIntrinsic;
        public final List<ParamSig> params;
        /** Tipo de retorno; null = void. */
        public final BpType returnType;

        public FuncSig(String name, boolean isPublic, boolean isStatic,
                       List<ParamSig> params, BpType returnType) {
            this(name, isPublic, isStatic, false, params, returnType);
        }
        public FuncSig(String name, boolean isPublic, boolean isStatic, boolean isIntrinsic,
                       List<ParamSig> params, BpType returnType) {
            this.name = name;
            this.isPublic = isPublic;
            this.isStatic = isStatic;
            this.isIntrinsic = isIntrinsic;
            this.params = params;
            this.returnType = returnType;
        }
    }

    public static final class ParamSig {
        public final String name;
        public final BpType type;
        /** H8.1 — valor por defecto (Long/Double/String/Boolean) o null si no tiene. */
        public final Object defaultValue;
        public ParamSig(String name, BpType type) { this(name, type, null); }
        public ParamSig(String name, BpType type, Object defaultValue) {
            this.name = name; this.type = type; this.defaultValue = defaultValue;
        }
    }

    /** Constante pública de tipo primitivo con valor literal conocido. */
    public static final class ConstSig {
        public final String name;
        public final BpType type;            // integer/float/string/boolean
        public final Object value;           // Long/Double/String/Boolean
        public ConstSig(String name, BpType type, Object value) {
            this.name = name; this.type = type; this.value = value;
        }
    }

    /** Enum público con miembros (nombre→valor entero). */
    public static final class EnumSig {
        public final String name;
        public final Map<String, Long> values;  // preservamos orden de declaración
        public EnumSig(String name, Map<String, Long> values) {
            this.name = name; this.values = values;
        }
    }

    /** Property pública a nivel módulo. Sólo metadata; los accesores
     *  reales viven en el .mod con nombres convenidos (__prop_get_X / __prop_set_X). */
    public static final class PropSig {
        public final String name;
        public final BpType type;
        public final boolean isSync;
        public PropSig(String name, BpType type, boolean isSync) {
            this.name = name; this.type = type; this.isSync = isSync;
        }
    }

    /**
     * Clase pública exportada al .bpi.
     *
     * Listado de miembros en ORDEN DE EMISIÓN, que es el orden de vtable real:
     * primero las properties (cada una añade un getX y un setX a la vtable),
     * luego los métodos de usuario en orden de declaración. El módulo
     * importador depende de este orden para emitir INVOKE_VIRTUAL con el
     * vtSlot correcto.
     *
     * Para construir cross-module sin opcode nuevo, el módulo dueño sintetiza
     * una factoría pública `__cls_new_&lt;Cls&gt;(args)` que el importador
     * llama vía CALL_EXT. La signature de la factoría (params) coincide con
     * el constructor.
     *
     * v1 (.bpi v4): sin herencia cross-module (baseClassName se guarda
     * pero el importador lo ignora). Sin static members. Sin fields
     * públicos (se exponen vía property).
     */
    public static final class ClassSig {
        public final String name;
        public final boolean isPublic;
        /** Padre, sólo informativo en v1 (no se usa para construir vtables). */
        public final String baseClassName;
        /** Parámetros del constructor. null = no hay constructor de usuario;
         *  la factoría no toma argumentos. */
        public final List<ParamSig> ctorParams;
        /** Properties de la clase, en orden de declaración. Cada una añade
         *  un getX y un setX a la vtable (en ese orden). */
        public final List<PropSig> properties;
        /** Métodos de usuario en orden de declaración (sin constructor,
         *  sin getX/setX sintéticos). El vtSlot del importador es:
         *  properties.size() * 2 + index del método aquí. */
        public final List<FuncSig> methods;

        // L2 v3 — layout binario completo del descriptor (incluyendo fields
        // privados y métodos sintéticos como getter/setter de props). Necesario
        // para que un módulo importador con `extends X` reserve el slot/field
        // numbering correcto. Si binaryNumFields/binaryNumMethods son -1, el
        // .bpi es de una versión vieja sin esta info (herencia cross-module no
        // disponible).
        public int binaryNumFields = -1;
        public int binaryNumMethods = -1;
        public int[] binaryFieldBitmap = new int[0];
        public int[] binaryOwnerBitmap = new int[0];

        /** L2 v3.d — static consts públicos de la clase. Cross-module se
         *  inlinean en el call-site (mismo patrón que consts de módulo).
         *  Lista vacía si la clase no expone ninguno. */
        public List<ConstSig> staticConsts = new ArrayList<>();

        public ClassSig(String name, boolean isPublic, String baseClassName,
                        List<ParamSig> ctorParams, List<PropSig> properties,
                        List<FuncSig> methods) {
            this.name = name;
            this.isPublic = isPublic;
            this.baseClassName = baseClassName;
            this.ctorParams = ctorParams;
            this.properties = properties;
            this.methods = methods;
        }
    }

    // ============================================================
    // EXTRACCIÓN: AST + ModuleSymbol → ModuleInterface
    // ============================================================

    /**
     * Construye la interfaz a partir del ModuleSymbol resultante del análisis
     * semántico. Sólo se incluyen funciones públicas con firma compuesta
     * exclusivamente por tipos primitivos (y void). Las que no califican se
     * añaden a `skipped` con una razón legible.
     */
    public static ModuleInterface extractFrom(String library, String moduleName,
                                              Symbol.ModuleSymbol modSym,
                                              List<String> skipped) {
        return extractFrom(library, moduleName, false, null, modSym, skipped);
    }

    public static ModuleInterface extractFrom(String library, String moduleName,
                                              boolean isInterface, String parentContractName,
                                              Symbol.ModuleSymbol modSym,
                                              List<String> skipped) {
        ModuleInterface iface = new ModuleInterface();
        iface.library = (library == null) ? "" : library;
        iface.moduleName = moduleName;
        iface.isInterface = isInterface;
        // Para interfaces: el contrato padre es `extends`; para módulos concretos: `implements`.
        if (isInterface) iface.extendsName = parentContractName;
        else             iface.implementsName = parentContractName;

        for (Symbol s : modSym.members.getSymbols()) {
            // ---------- Funciones públicas ----------
            if (s instanceof FunctionSymbol) {
                FunctionSymbol fs = (FunctionSymbol) s;
                if (!fs.isPublic) continue;
                if (fs.isModuleInitializer) continue;
                if (("Main".equals(fs.name) || "main".equals(fs.name)) && fs.params.size() <= 1) continue;

                String issue = signatureIssue(fs);
                if (issue != null) {
                    skipped.add("func " + fs.name + ": " + issue);
                    continue;
                }
                List<ParamSig> ps = new ArrayList<>(fs.params.size());
                for (ParamSymbol p : fs.params) ps.add(new ParamSig(p.name, p.type, paramDefault(p)));
                iface.functions.add(new FuncSig(fs.name, true, fs.isStatic, fs.isIntrinsic, ps, fs.returnType));
                continue;
            }

            // ---------- Constantes públicas (sólo si tipo+valor literal son exportables) ----------
            if (s instanceof ConstSymbol) {
                ConstSymbol cs = (ConstSymbol) s;
                if (!cs.isPublic) continue;
                if (cs.type == null || !isExportableType(cs.type)) {
                    skipped.add("const " + cs.name + ": tipo no exportable: "
                            + (cs.type == null ? "<null>" : cs.type.display()));
                    continue;
                }
                if (!(cs.decl instanceof Ast.ConstDecl)) {
                    skipped.add("const " + cs.name + ": sin AST literal accesible");
                    continue;
                }
                Ast.IExpr v = ((Ast.ConstDecl) cs.decl).value;
                Object lit = literalValueOf(v);
                if (lit == null) {
                    skipped.add("const " + cs.name + ": valor no es literal");
                    continue;
                }
                iface.consts.add(new ConstSig(cs.name, cs.type, lit));
                continue;
            }

            // ---------- Properties públicas a nivel módulo ----------
            if (s instanceof PropertySymbol) {
                PropertySymbol ps = (PropertySymbol) s;
                if (!ps.isPublic) continue;
                if (ps.ownerClass != null) continue;  // sólo módulo, no clase
                if (ps.type == null || !isExportableType(ps.type)) {
                    skipped.add("property " + ps.name + ": tipo no exportable: "
                            + (ps.type == null ? "<null>" : ps.type.display()));
                    continue;
                }
                iface.properties.add(new PropSig(ps.name, ps.type, ps.isSync));
                continue;
            }

            // ---------- Enums públicos ----------
            if (s instanceof EnumSymbol) {
                EnumSymbol es = (EnumSymbol) s;
                if (!es.isPublic) continue;
                // Preservar orden de declaración no es trivial (values es HashMap);
                // como mínimo metemos las entradas tal cual.
                Map<String, Long> ordered = new LinkedHashMap<>(es.values);
                iface.enums.add(new EnumSig(es.name, ordered));
                continue;
            }

            // ---------- Clases públicas ----------
            if (s instanceof Symbol.ClassSymbol) {
                Symbol.ClassSymbol cls = (Symbol.ClassSymbol) s;
                if (!cls.isPublic) continue;
                ClassSig cs = extractClass(cls, skipped);
                if (cs != null) iface.classes.add(cs);
                continue;
            }
        }
        return iface;
    }

    /**
     * Recorre los miembros de instancia de la clase y construye su ClassSig.
     * Devuelve null si la clase no se puede exportar por completo (e.g.
     * contiene un tipo no exportable que bloquea la exportación de un método
     * sin alternativa). Lo que falle individualmente va a {@code skipped}.
     *
     * Orden de miembros: properties primero (en orden de declaración del AST),
     * luego métodos de usuario (excl. constructor) en orden del AST. Es el
     * MISMO orden que MivmEmitter usa para construir la vtable; sin esta
     * coincidencia, los vtSlot del importador no apuntarían al método correcto.
     */
    private static ClassSig extractClass(Symbol.ClassSymbol cls, List<String> skipped) {
        // ---- Constructor (sólo el de la propia clase, no heredado) ----
        List<ParamSig> ctorParams = null;
        if (cls.constructor != null) {
            FunctionSymbol ctor = cls.constructor;
            String issue = signatureIssue(ctor);
            if (issue != null) {
                skipped.add("class " + cls.name + ": constructor: " + issue);
                // Sin ctor no podemos exportar (no se puede construir). Pero
                // permitimos clases con factoría sin args si tampoco hay
                // ctor de usuario — eso ya tira de la rama ctorParams==null.
                return null;
            }
            ctorParams = new ArrayList<>();
            for (ParamSymbol p : ctor.params) ctorParams.add(new ParamSig(p.name, p.type, paramDefault(p)));
        }

        // ---- Properties + métodos en orden de declaración del AST ----
        List<PropSig> properties = new ArrayList<>();
        List<FuncSig> methods    = new ArrayList<>();
        if (cls.astNode != null && cls.astNode.members != null) {
            for (Ast.ITopLevelDecl m : cls.astNode.members) {
                if (m instanceof Ast.PropertyDef) {
                    Ast.PropertyDef pd = (Ast.PropertyDef) m;
                    if (!pd.isPublic) continue;
                    // Property estática no soportada cross-module por ahora.
                    if (pd.name != null && pd.name.isStatic()) {
                        skipped.add("class " + cls.name + ".property " + pd.name.name
                                + ": estática (no exportable v1)");
                        continue;
                    }
                    Symbol ps = cls.instanceMembers.tryLookup(pd.name.name);
                    if (!(ps instanceof PropertySymbol)) continue;
                    PropertySymbol psym = (PropertySymbol) ps;
                    if (psym.type == null || !isExportableType(psym.type)) {
                        skipped.add("class " + cls.name + ".property " + psym.name
                                + ": tipo no exportable: "
                                + (psym.type == null ? "<null>" : psym.type.display()));
                        continue;
                    }
                    properties.add(new PropSig(psym.name, psym.type, psym.isSync));
                } else if (m instanceof Ast.FuncDef) {
                    Ast.FuncDef fn = (Ast.FuncDef) m;
                    Symbol fs = cls.instanceMembers.tryLookup(fn.name.name);
                    if (!(fs instanceof FunctionSymbol)) continue;
                    FunctionSymbol fsym = (FunctionSymbol) fs;
                    if (!fsym.isPublic) continue;
                    if (fsym.isConstructor) continue;          // se exporta vía ctorParams
                    if (fsym.isStatic) continue;               // v1: sin estáticos
                    String issue = signatureIssue(fsym);
                    if (issue != null) {
                        skipped.add("class " + cls.name + ".method " + fsym.name + ": " + issue);
                        continue;
                    }
                    List<ParamSig> ps = new ArrayList<>(fsym.params.size());
                    for (ParamSymbol p : fsym.params) ps.add(new ParamSig(p.name, p.type, paramDefault(p)));
                    methods.add(new FuncSig(fsym.name, true, false, ps, fsym.returnType));
                }
            }
        }
        // baseClassName: L2 v3 — si el parent es cross-module, cualificamos
        // con `<Mod>.<Cls>` para que el importador pueda resolver el data
        // symbol del parent en runtime (linkAll fixup). Si parent es local
        // al módulo dueño, queda sólo el simpleName (mismo comportamiento
        // que v1/v2; no causa colisión porque el importador ve el parent
        // vía import del módulo dueño).
        String parent = null;
        if (cls.baseClass != null) {
            Symbol.ClassSymbol p = cls.baseClass;
            if (p.isExternal) {
                StringBuilder sb = new StringBuilder();
                if (p.externalLibrary != null && !p.externalLibrary.isEmpty())
                    sb.append(p.externalLibrary).append('.');
                sb.append(p.externalModule).append('.').append(p.name);
                parent = sb.toString();
            } else {
                parent = p.name;
            }
        }
        ClassSig sig = new ClassSig(cls.name, true, parent, ctorParams, properties, methods);

        // L2 v3.d — static consts públicos (con literal exportable). Vars
        // estáticos quedan pendientes (requieren opcodes nuevos para
        // GET/SET global cross-module). Mismo criterio que ConstSig a nivel
        // módulo: tipo exportable + valor literal accesible.
        for (Symbol s2 : cls.staticMembers.getSymbols()) {
            if (!(s2 instanceof Symbol.ConstSymbol)) continue;
            Symbol.ConstSymbol cst = (Symbol.ConstSymbol) s2;
            if (!cst.isPublic) continue;
            if (cst.type == null || !isExportableType(cst.type)) {
                skipped.add("class " + cls.name + ".static const " + cst.name
                        + ": tipo no exportable: "
                        + (cst.type == null ? "<null>" : cst.type.display()));
                continue;
            }
            if (!(cst.decl instanceof Ast.ConstDecl)) {
                skipped.add("class " + cls.name + ".static const " + cst.name
                        + ": sin AST literal accesible");
                continue;
            }
            Object lit = literalValueOf(((Ast.ConstDecl) cst.decl).value);
            if (lit == null) {
                skipped.add("class " + cls.name + ".static const " + cst.name
                        + ": valor no es literal");
                continue;
            }
            sig.staticConsts.add(new ConstSig(cst.name, cst.type, lit));
        }
        // L2 v3 — propaga el layout binario para herencia cross-module. Si el
        // backend ya lo populates (cls.binaryLayout != null), usamos eso (la
        // fuente más fiel). Si no (típicamente porque estamos en modo
        // INTERFACE_ONLY, donde el emisor no corre), reconstruimos un layout
        // aproximado a partir del AST — replicando la lógica de MivmEmitter:
        //   * __syncMutex si la clase tiene sync property (slot 0, ref);
        //   * fields VarDecl en orden (con flags isRef/isOwner);
        //   * backing field de cada property en orden.
        //   * num_methods = 2*nProps + nMethods.
        if (cls.binaryLayout != null) {
            sig.binaryNumFields  = cls.binaryLayout.numFields;
            sig.binaryNumMethods = cls.binaryLayout.numMethods;
            sig.binaryFieldBitmap = cls.binaryLayout.fieldBitmap;
            sig.binaryOwnerBitmap = cls.binaryLayout.ownerBitmap;
        } else {
            // Reconstrucción del layout desde el AST (INTERFACE_ONLY mode).
            // Sólo necesitamos saber num_fields, num_methods y los bitmaps.
            int nFields = 0, nMethods = 0;
            java.util.List<Boolean> fieldRefFlags  = new java.util.ArrayList<>();
            java.util.List<Boolean> fieldOwnerFlags = new java.util.ArrayList<>();
            boolean classHasSyncProp = false;
            if (cls.astNode != null && cls.astNode.members != null) {
                for (Ast.ITopLevelDecl m : cls.astNode.members) {
                    if (m instanceof Ast.PropertyDef && ((Ast.PropertyDef) m).isSync) {
                        classHasSyncProp = true;
                        break;
                    }
                }
            }
            // Slot 0 = __syncMutex si la clase tiene sync property propia.
            if (classHasSyncProp) {
                fieldRefFlags.add(Boolean.TRUE);
                fieldOwnerFlags.add(Boolean.FALSE);
                nFields++;
            }
            // Fields (var/owner) y backing fields de properties, en orden de declaración.
            if (cls.astNode != null && cls.astNode.members != null) {
                for (Ast.ITopLevelDecl m : cls.astNode.members) {
                    if (m instanceof Ast.VarDecl) {
                        Ast.VarDecl vd = (Ast.VarDecl) m;
                        for (Ast.DeclName dn : vd.names) {
                            if (dn.isStatic()) continue;
                            Symbol vsym = cls.instanceMembers.tryLookup(dn.name);
                            boolean isRef = (vsym instanceof VarSymbol)
                                    && isRefTypeForBpi(((VarSymbol) vsym).type);
                            fieldRefFlags.add(isRef);
                            fieldOwnerFlags.add(vd.isOwner);
                            nFields++;
                        }
                    } else if (m instanceof Ast.PropertyDef) {
                        Ast.PropertyDef pd = (Ast.PropertyDef) m;
                        if (pd.name != null && pd.name.isStatic()) continue;
                        Symbol psym = cls.instanceMembers.tryLookup(pd.name.name);
                        boolean isRef = (psym instanceof PropertySymbol)
                                && isRefTypeForBpi(((PropertySymbol) psym).type);
                        fieldRefFlags.add(isRef);
                        fieldOwnerFlags.add(pd.isOwner);
                        nFields++;
                        // cada property → 2 slots de método (getter + setter)
                        nMethods += 2;
                    } else if (m instanceof Ast.FuncDef) {
                        Ast.FuncDef fn = (Ast.FuncDef) m;
                        Symbol fsym = cls.instanceMembers.tryLookup(fn.name.name);
                        if (fsym instanceof FunctionSymbol) {
                            FunctionSymbol f = (FunctionSymbol) fsym;
                            if (!f.isStatic && !f.isConstructor) nMethods++;
                        }
                    }
                }
            }
            int bw = (nFields + 31) >>> 5;
            int[] fieldBitmap = new int[bw];
            int[] ownerBitmap = new int[bw];
            for (int i = 0; i < nFields; i++) {
                if (fieldRefFlags.get(i))  fieldBitmap[i >>> 5] |= (1 << (i & 31));
                if (fieldOwnerFlags.get(i)) ownerBitmap[i >>> 5] |= (1 << (i & 31));
            }
            sig.binaryNumFields  = nFields;
            sig.binaryNumMethods = nMethods;
            sig.binaryFieldBitmap = fieldBitmap;
            sig.binaryOwnerBitmap = ownerBitmap;
        }
        return sig;
    }

    /** Aproximación de isRefType para reconstruir el bitmap desde el AST.
     *  Coincide con MivmEmitter.isRefType: refs = string, class, array, any. */
    private static boolean isRefTypeForBpi(BpType t) {
        if (t == null) return false;
        if (t.isReference()) return true;
        if (t instanceof BpType.PrimitiveType) {
            return ((BpType.PrimitiveType) t).tag == BpType.PrimitiveType.Kind.STRING;
        }
        return false;
    }

    private static Object literalValueOf(Ast.IExpr e) {
        if (e instanceof Ast.IntLitExpr)    return ((Ast.IntLitExpr) e).value;
        if (e instanceof Ast.FloatLitExpr)  return ((Ast.FloatLitExpr) e).value;
        if (e instanceof Ast.StringLitExpr) return ((Ast.StringLitExpr) e).value;
        if (e instanceof Ast.BoolLitExpr)   return ((Ast.BoolLitExpr) e).value;
        return null;
    }

    /** Devuelve null si la firma es serializable, o un String con la razón si no. */
    private static String signatureIssue(FunctionSymbol fs) {
        for (ParamSymbol p : fs.params) {
            if (!isExportableType(p.type))
                return "param '" + p.name + "' tipo no exportable: " +
                        (p.type == null ? "<null>" : p.type.display());
        }
        if (fs.returnType != null && !isExportableType(fs.returnType))
            return "retorno tipo no exportable: " + fs.returnType.display();
        return null;
    }

    private static boolean isExportableType(BpType t) {
        if (t == null) return true;             // void
        if (t instanceof PrimitiveType) return true;
        if (t instanceof VoidType) return true;
        // L2: tipos clase referenciables si tenemos un ClassSymbol resuelto.
        // Se serializa por nombre; el importador resuelve contra los stubs.
        if (t instanceof BpType.ClassType) return true;
        // H5 — el tipo raíz universal (`Object`/`any`) es exportable: se
        // serializa como "any" y el lector lo recupera (parseType case "any").
        // Necesario para contenedores genéricos cross-module (Map.put/get...).
        if (t instanceof BpType.AnyType) return true;
        // L-arr-export: ArrayType es exportable si su elemento lo es.
        // Permite firmas como `data: integer[]` cross-module (útil para
        // buffers de bytes en I2c/SPI, samples de ADC, etc.).
        if (t instanceof BpType.ArrayType)
            return isExportableType(((BpType.ArrayType) t).element);
        // Tuplas (retorno múltiple): exportables si todos los elementos lo son.
        if (t instanceof BpType.TupleType) {
            for (BpType e : ((BpType.TupleType) t).elements)
                if (!isExportableType(e)) return false;
            return true;
        }
        return false;
    }

    // ============================================================
    // ESCRITURA
    // ============================================================

    public void writeTo(Path file) throws IOException {
        Files.createDirectories(file.toAbsolutePath().getParent());
        try (PrintWriter pw = new PrintWriter(Files.newBufferedWriter(file, StandardCharsets.UTF_8))) {
            pw.println("bpi " + CURRENT_VERSION);
            if (library != null && !library.isEmpty()) pw.println("library " + library);
            pw.println("module " + moduleName);
            if (isInterface) pw.println("interface true");
            if (implementsName != null && !implementsName.isEmpty())
                pw.println("implements " + implementsName);
            if (extendsName != null && !extendsName.isEmpty())
                pw.println("extends " + extendsName);

            for (FuncSig f : functions) {
                StringBuilder sb = new StringBuilder();
                sb.append("func ").append(f.name).append('(');
                for (int i = 0; i < f.params.size(); i++) {
                    if (i > 0) sb.append(',');
                    appendParam(sb, f.params.get(i));
                }
                sb.append("):").append(typeToString(f.returnType));
                if (f.isPublic) sb.append(" public");
                if (f.isStatic) sb.append(" static");
                if (f.isIntrinsic) sb.append(" intrinsic");
                pw.println(sb.toString());
            }

            for (ConstSig c : consts) {
                pw.printf("const %s:%s=%s public%n",
                        c.name, typeToString(c.type), encodeLiteral(c.value));
            }

            for (EnumSig e : enums) {
                pw.println("enum " + e.name + " public");
                for (Map.Entry<String, Long> kv : e.values.entrySet()) {
                    pw.printf("  %s=%d%n", kv.getKey(), kv.getValue());
                }
                pw.println("end enum");
            }

            for (PropSig p : properties) {
                pw.printf("prop %s:%s public%s%n",
                        p.name, typeToString(p.type), p.isSync ? " sync" : "");
            }

            for (ClassSig c : classes) {
                StringBuilder hdr = new StringBuilder();
                hdr.append("class ").append(c.name).append(" public");
                if (c.baseClassName != null && !c.baseClassName.isEmpty())
                    hdr.append(" extends ").append(c.baseClassName);
                pw.println(hdr.toString());
                // L2 v3 — layout binario: línea opcional `layout=nF/nM/fbm/obm`
                // donde fbm y obm son hex (little-endian de ints). El reader
                // viejo (v5) ignora líneas desconocidas, así que es backward
                // compatible. La línea sólo se emite si binaryNumFields >= 0.
                if (c.binaryNumFields >= 0) {
                    StringBuilder lay = new StringBuilder();
                    lay.append("  layout ").append(c.binaryNumFields).append(' ').append(c.binaryNumMethods);
                    lay.append(' '); appendHexInts(lay, c.binaryFieldBitmap);
                    lay.append(' '); appendHexInts(lay, c.binaryOwnerBitmap);
                    pw.println(lay.toString());
                }

                if (c.ctorParams != null) {
                    StringBuilder cs = new StringBuilder();
                    cs.append("  ctor(");
                    for (int i = 0; i < c.ctorParams.size(); i++) {
                        if (i > 0) cs.append(',');
                        appendParam(cs, c.ctorParams.get(i));
                    }
                    cs.append(')');
                    pw.println(cs.toString());
                }
                for (PropSig p : c.properties) {
                    pw.printf("  prop %s:%s public%s%n",
                            p.name, typeToString(p.type), p.isSync ? " sync" : "");
                }
                for (ConstSig sc : c.staticConsts) {
                    pw.printf("  staticconst %s:%s=%s public%n",
                            sc.name, typeToString(sc.type), encodeLiteral(sc.value));
                }
                for (FuncSig m : c.methods) {
                    StringBuilder sb = new StringBuilder();
                    sb.append("  method ").append(m.name).append('(');
                    for (int i = 0; i < m.params.size(); i++) {
                        if (i > 0) sb.append(',');
                        appendParam(sb, m.params.get(i));
                    }
                    sb.append("):").append(typeToString(m.returnType));
                    sb.append(" public");
                    pw.println(sb.toString());
                }
                pw.println("end class");
            }
        }
    }

    /** Helper para serializar arrays de int como hex separado por '|'.
     *  "" = array vacío. Cada int se escribe como 8 hex chars sin prefijo. */
    private static void appendHexInts(StringBuilder sb, int[] arr) {
        if (arr == null || arr.length == 0) { sb.append('-'); return; }
        for (int i = 0; i < arr.length; i++) {
            if (i > 0) sb.append('|');
            sb.append(String.format("%08x", arr[i]));
        }
    }
    private static int[] parseHexInts(String s) {
        if (s == null || s.isEmpty() || "-".equals(s)) return new int[0];
        String[] parts = s.split("\\|");
        int[] out = new int[parts.length];
        for (int i = 0; i < parts.length; i++) {
            out[i] = (int) Long.parseLong(parts[i], 16);
        }
        return out;
    }

    /**
     * Codificación de literal para .bpi: int/bool en decimal, float en formato
     * Java por defecto, string entre comillas dobles con escapes mínimos.
     */
    private static String encodeLiteral(Object v) {
        if (v instanceof Long || v instanceof Integer) return v.toString();
        if (v instanceof Boolean) return ((Boolean) v) ? "true" : "false";
        if (v instanceof Double || v instanceof Float) return v.toString();
        if (v instanceof String) {
            StringBuilder sb = new StringBuilder("\"");
            for (int i = 0; i < ((String) v).length(); i++) {
                char ch = ((String) v).charAt(i);
                switch (ch) {
                    case '\\': sb.append("\\\\"); break;
                    case '"':  sb.append("\\\""); break;
                    case '\n': sb.append("\\n"); break;
                    case '\r': sb.append("\\r"); break;
                    case '\t': sb.append("\\t"); break;
                    default:   sb.append(ch);
                }
            }
            sb.append('"');
            return sb.toString();
        }
        return "0";  // safety
    }

    private static Object decodeLiteral(BpType type, String src, Path file, int lineNo) throws IOException {
        String s = src.trim();
        if (s.isEmpty()) throw new IOException(file + ":" + lineNo + ": valor literal vacío");
        if (type instanceof PrimitiveType) {
            PrimitiveType.Kind k = ((PrimitiveType) type).tag;
            switch (k) {
                case INTEGER:
                    try { return Long.parseLong(s); }
                    catch (NumberFormatException ex) {
                        throw new IOException(file + ":" + lineNo + ": entero inválido: " + s);
                    }
                case LONG:
                    try { return Long.parseLong(s); }
                    catch (NumberFormatException ex) {
                        throw new IOException(file + ":" + lineNo + ": long inválido: " + s);
                    }
                case FLOAT:
                    try { return Double.parseDouble(s); }
                    catch (NumberFormatException ex) {
                        throw new IOException(file + ":" + lineNo + ": float inválido: " + s);
                    }
                case DOUBLE:
                    try { return Double.parseDouble(s); }
                    catch (NumberFormatException ex) {
                        throw new IOException(file + ":" + lineNo + ": double inválido: " + s);
                    }
                case BOOLEAN:
                    if ("true".equals(s))  return Boolean.TRUE;
                    if ("false".equals(s)) return Boolean.FALSE;
                    throw new IOException(file + ":" + lineNo + ": booleano inválido: " + s);
                case STRING:
                    if (s.length() < 2 || s.charAt(0) != '"' || s.charAt(s.length() - 1) != '"')
                        throw new IOException(file + ":" + lineNo + ": string sin comillas: " + s);
                    StringBuilder sb = new StringBuilder();
                    for (int i = 1; i < s.length() - 1; i++) {
                        char ch = s.charAt(i);
                        if (ch == '\\' && i + 1 < s.length() - 1) {
                            char nxt = s.charAt(++i);
                            switch (nxt) {
                                case 'n': sb.append('\n'); break;
                                case 'r': sb.append('\r'); break;
                                case 't': sb.append('\t'); break;
                                case '"': sb.append('"');  break;
                                case '\\': sb.append('\\'); break;
                                default: sb.append(nxt);
                            }
                        } else sb.append(ch);
                    }
                    return sb.toString();
            }
        }
        throw new IOException(file + ":" + lineNo + ": tipo no soportado para const literal: " + type.display());
    }

    /** H8.1 — escribe "name:type" y, si tiene valor por defecto, "=<literal>". */
    private static void appendParam(StringBuilder sb, ParamSig p) {
        sb.append(p.name).append(':').append(typeToString(p.type));
        if (p.defaultValue != null) sb.append('=').append(encodeLiteral(p.defaultValue));
    }

    /** H8.1 — valor por defecto de un ParamSymbol como literal serializable, o null. */
    private static Object paramDefault(ParamSymbol p) {
        return (p.defaultExpr == null) ? null : SemanticAnalyzer.constLiteralValue(p.defaultExpr);
    }

    private static String typeToString(BpType t) {
        if (t == null) return "void";
        if (t instanceof VoidType) return "void";
        if (t instanceof PrimitiveType) return ((PrimitiveType) t).tag.name().toLowerCase();
        // L-arr-export: arrays se serializan como "<element>[]". Recursivo
        // para arrays anidados (int[][] = "integer[][]", aunque BP no los
        // usa de momento).
        if (t instanceof BpType.ArrayType)
            return typeToString(((BpType.ArrayType) t).element) + "[]";
        // L2 v3.e — para clases cross-module emitimos el nombre cualificado
        // (`<Lib>.<Mod>.<Cls>` o `<Mod>.<Cls>`). El reader del importador
        // resuelve contra los ImportedNamespaceSymbol disponibles. Para
        // clases locales del propio módulo, sigue siendo el simpleName.
        if (t instanceof BpType.ClassType) {
            Symbol.ClassSymbol cls = ((BpType.ClassType) t).cls;
            if (cls.isExternal) {
                StringBuilder sb = new StringBuilder();
                if (cls.externalLibrary != null && !cls.externalLibrary.isEmpty())
                    sb.append(cls.externalLibrary).append('.');
                sb.append(cls.externalModule).append('.').append(cls.name);
                return sb.toString();
            }
            return cls.name;
        }
        if (t instanceof BpType.UnresolvedClassRef) return ((BpType.UnresolvedClassRef) t).name;
        if (t instanceof BpType.AnyType) return "any";   // H5 — tipo raíz universal
        // Tuplas (retorno múltiple): `(t1,t2,...)`. El importador conoce así la
        // forma para emitir el desempaquetado cross-module (T3).
        if (t instanceof BpType.TupleType) {
            StringBuilder sb = new StringBuilder("(");
            java.util.List<BpType> els = ((BpType.TupleType) t).elements;
            for (int i = 0; i < els.size(); i++) {
                if (i > 0) sb.append(',');
                sb.append(typeToString(els.get(i)));
            }
            return sb.append(')').toString();
        }
        return "void"; // safety net; debería haber sido descartada por isExportableType
    }

    // ============================================================
    // LECTURA
    // ============================================================

    /** Carga una interfaz desde un .bpi. Lanza IOException con mensaje legible
     *  ante problemas de formato. */
    public static ModuleInterface readFrom(Path file) throws IOException {
        ModuleInterface iface = new ModuleInterface();
        iface.sourcePath = file.toString();
        try (BufferedReader br = Files.newBufferedReader(file, StandardCharsets.UTF_8)) {
            String line;
            int lineNo = 0;
            boolean sawHeader = false;
            EnumSig inProgressEnum = null;
            // Estado "dentro de class": acumulamos miembros hasta "end class".
            String       clsName     = null;
            String       clsParent   = null;
            List<ParamSig> clsCtorParams = null;
            List<PropSig>  clsProps   = null;
            List<FuncSig>  clsMethods = null;
            // L2 v3 — layout binario opcional (-1 = no presente).
            int clsBinNumFields = -1, clsBinNumMethods = -1;
            int[] clsBinFieldBitmap = new int[0], clsBinOwnerBitmap = new int[0];
            // L2 v3.d — static consts del bloque class.
            List<ConstSig> clsStaticConsts = new ArrayList<>();

            while ((line = br.readLine()) != null) {
                lineNo++;
                String trimmed = line.trim();
                if (trimmed.isEmpty() || trimmed.startsWith("#")) continue;

                if (!sawHeader) {
                    if (!trimmed.startsWith("bpi "))
                        throw new IOException(file + ":" + lineNo + ": se esperaba cabecera 'bpi <ver>'");
                    int ver;
                    try { ver = Integer.parseInt(trimmed.substring(4).trim()); }
                    catch (NumberFormatException ex) {
                        throw new IOException(file + ":" + lineNo + ": versión .bpi inválida"); }
                    if (ver < MIN_SUPPORTED_VERSION || ver > CURRENT_VERSION)
                        throw new IOException(file + ": versión .bpi no soportada: " + ver +
                                " (soportadas " + MIN_SUPPORTED_VERSION + ".." + CURRENT_VERSION + ")");
                    sawHeader = true;
                    continue;
                }

                // Modo "dentro de enum": parsea miembros hasta "end enum".
                if (inProgressEnum != null) {
                    if ("end enum".equals(trimmed)) {
                        iface.enums.add(inProgressEnum);
                        inProgressEnum = null;
                        continue;
                    }
                    int eq = trimmed.indexOf('=');
                    if (eq < 0)
                        throw new IOException(file + ":" + lineNo + ": miembro de enum sin '=': " + trimmed);
                    String memberName = trimmed.substring(0, eq).trim();
                    long memberValue;
                    try { memberValue = Long.parseLong(trimmed.substring(eq + 1).trim()); }
                    catch (NumberFormatException ex) {
                        throw new IOException(file + ":" + lineNo + ": valor de enum inválido"); }
                    inProgressEnum.values.put(memberName, memberValue);
                    continue;
                }

                // Modo "dentro de class": parsea miembros hasta "end class".
                if (clsName != null) {
                    if ("end class".equals(trimmed)) {
                        ClassSig cs = new ClassSig(clsName, true, clsParent,
                                clsCtorParams, clsProps, clsMethods);
                        cs.binaryNumFields  = clsBinNumFields;
                        cs.binaryNumMethods = clsBinNumMethods;
                        cs.binaryFieldBitmap = clsBinFieldBitmap;
                        cs.binaryOwnerBitmap = clsBinOwnerBitmap;
                        cs.staticConsts = clsStaticConsts;
                        iface.classes.add(cs);
                        clsName = null; clsParent = null;
                        clsCtorParams = null; clsProps = null; clsMethods = null;
                        clsBinNumFields = -1; clsBinNumMethods = -1;
                        clsBinFieldBitmap = new int[0]; clsBinOwnerBitmap = new int[0];
                        clsStaticConsts = new ArrayList<>();
                        continue;
                    }
                    if (trimmed.startsWith("ctor(") || trimmed.startsWith("ctor (")) {
                        int lp = trimmed.indexOf('(');
                        int rp = matchParen(trimmed, lp);
                        if (rp < 0) throw new IOException(file + ":" + lineNo + ": ctor sin ')'");
                        clsCtorParams = parseParamList(file, lineNo, trimmed.substring(lp + 1, rp));
                        continue;
                    }
                    if (trimmed.startsWith("prop ")) {
                        clsProps.add(parseProp(file, lineNo, trimmed.substring("prop ".length())));
                        continue;
                    }
                    if (trimmed.startsWith("method ")) {
                        clsMethods.add(parseFunc(file, lineNo, trimmed.substring("method ".length())));
                        continue;
                    }
                    if (trimmed.startsWith("staticconst ")) {
                        clsStaticConsts.add(parseConst(file, lineNo, trimmed.substring("staticconst ".length())));
                        continue;
                    }
                    if (trimmed.startsWith("layout ")) {
                        // formato: "layout <nFields> <nMethods> <fbmHex> <obmHex>"
                        String[] tok = trimmed.substring("layout ".length()).split("\\s+");
                        if (tok.length < 4)
                            throw new IOException(file + ":" + lineNo + ": layout malformed (esperado: nFields nMethods fbm obm)");
                        try {
                            clsBinNumFields  = Integer.parseInt(tok[0]);
                            clsBinNumMethods = Integer.parseInt(tok[1]);
                        } catch (NumberFormatException nfe) {
                            throw new IOException(file + ":" + lineNo + ": layout counts no enteros");
                        }
                        clsBinFieldBitmap = parseHexInts(tok[2]);
                        clsBinOwnerBitmap = parseHexInts(tok[3]);
                        continue;
                    }
                    throw new IOException(file + ":" + lineNo + ": directiva no reconocida en class: " + trimmed);
                }

                if (trimmed.startsWith("library ")) {
                    iface.library = trimmed.substring("library ".length()).trim();
                } else if (trimmed.startsWith("module ")) {
                    iface.moduleName = trimmed.substring("module ".length()).trim();
                } else if (trimmed.startsWith("interface ")) {
                    iface.isInterface = "true".equals(trimmed.substring("interface ".length()).trim());
                } else if (trimmed.startsWith("implements ")) {
                    iface.implementsName = trimmed.substring("implements ".length()).trim();
                } else if (trimmed.startsWith("extends ")) {
                    iface.extendsName = trimmed.substring("extends ".length()).trim();
                } else if (trimmed.startsWith("func ")) {
                    iface.functions.add(parseFunc(file, lineNo, trimmed.substring("func ".length())));
                } else if (trimmed.startsWith("const ")) {
                    iface.consts.add(parseConst(file, lineNo, trimmed.substring("const ".length())));
                } else if (trimmed.startsWith("enum ")) {
                    String tail = trimmed.substring("enum ".length()).trim();
                    String enumName;
                    int sp = tail.indexOf(' ');
                    enumName = (sp < 0) ? tail : tail.substring(0, sp).trim();
                    inProgressEnum = new EnumSig(enumName, new LinkedHashMap<>());
                } else if (trimmed.startsWith("prop ")) {
                    iface.properties.add(parseProp(file, lineNo, trimmed.substring("prop ".length())));
                } else if (trimmed.startsWith("class ")) {
                    String tail = trimmed.substring("class ".length()).trim();
                    // "Name public [extends Parent]"
                    String[] toks = tail.split("\\s+");
                    if (toks.length < 1) throw new IOException(file + ":" + lineNo + ": class sin nombre");
                    clsName = toks[0];
                    clsParent = null;
                    boolean sawPub = false;
                    for (int i = 1; i < toks.length; i++) {
                        if ("public".equals(toks[i])) { sawPub = true; }
                        else if ("extends".equals(toks[i]) && i + 1 < toks.length) {
                            clsParent = toks[++i];
                        } else {
                            throw new IOException(file + ":" + lineNo + ": flag desconocido en class '" + toks[i] + "'");
                        }
                    }
                    if (!sawPub) throw new IOException(file + ":" + lineNo + ": class sin 'public'");
                    clsCtorParams = null;          // null = sin ctor de usuario
                    clsProps   = new ArrayList<>();
                    clsMethods = new ArrayList<>();
                } else {
                    throw new IOException(file + ":" + lineNo + ": directiva no reconocida: " + trimmed);
                }
            }
            if (!sawHeader)
                throw new IOException(file + ": fichero vacío o sin cabecera bpi");
            if (iface.moduleName.isEmpty())
                throw new IOException(file + ": sin directiva 'module <name>'");
            if (inProgressEnum != null)
                throw new IOException(file + ": enum '" + inProgressEnum.name + "' sin 'end enum'");
            if (clsName != null)
                throw new IOException(file + ": class '" + clsName + "' sin 'end class'");
        }
        return iface;
    }

    /** Parsea una lista "a:T,b:U,..." (sin paréntesis). Vacía permitida. */
    private static List<ParamSig> parseParamList(Path file, int lineNo, String paramsRaw) throws IOException {
        List<ParamSig> params = new ArrayList<>();
        if (paramsRaw == null || paramsRaw.trim().isEmpty()) return params;
        for (String pTok : splitTopLevel(paramsRaw, ',')) {
            pTok = pTok.trim();
            if (pTok.isEmpty()) continue;
            int colon = pTok.indexOf(':');
            if (colon < 0)
                throw new IOException(file + ":" + lineNo + ": param mal formado: '" + pTok + "'");
            String pname = pTok.substring(0, colon).trim();
            String rest  = pTok.substring(colon + 1).trim();
            // H8.1 — valor por defecto opcional: `tipo=<literal>`. El '=' a nivel
            // superior (fuera de comillas/paréntesis) separa el tipo del default.
            int eq = indexOfTopLevel(rest, '=');
            String ptype = (eq < 0) ? rest : rest.substring(0, eq).trim();
            String pdef  = (eq < 0) ? null : rest.substring(eq + 1).trim();
            BpType pty = parseType(file, lineNo, ptype);
            Object dval = (pdef == null) ? null : decodeLiteral(pty, pdef, file, lineNo);
            params.add(new ParamSig(pname, pty, dval));
        }
        return params;
    }

    /** Divide 's' por 'sep' a nivel superior, respetando "..." y (...)/[...] anidados. */
    private static List<String> splitTopLevel(String s, char sep) {
        List<String> out = new ArrayList<>();
        int depth = 0; boolean inStr = false, esc = false; int start = 0;
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (inStr) {
                if (esc) esc = false;
                else if (c == '\\') esc = true;
                else if (c == '"') inStr = false;
            } else if (c == '"') inStr = true;
            else if (c == '(' || c == '[') depth++;
            else if (c == ')' || c == ']') depth--;
            else if (c == sep && depth == 0) { out.add(s.substring(start, i)); start = i + 1; }
        }
        out.add(s.substring(start));
        return out;
    }

    /** Primer índice de 'ch' a nivel superior (fuera de "..." y (...)/[...]), o -1. */
    private static int indexOfTopLevel(String s, char ch) {
        int depth = 0; boolean inStr = false, esc = false;
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (inStr) {
                if (esc) esc = false;
                else if (c == '\\') esc = true;
                else if (c == '"') inStr = false;
            } else if (c == '"') inStr = true;
            else if (c == '(' || c == '[') depth++;
            else if (c == ')' || c == ']') depth--;
            else if (c == ch && depth == 0) return i;
        }
        return -1;
    }

    /** Índice del ')' que cierra el '(' en 'open', respetando "..." y anidación. */
    private static int matchParen(String s, int open) {
        int depth = 0; boolean inStr = false, esc = false;
        for (int i = open; i < s.length(); i++) {
            char c = s.charAt(i);
            if (inStr) {
                if (esc) esc = false;
                else if (c == '\\') esc = true;
                else if (c == '"') inStr = false;
            } else if (c == '"') inStr = true;
            else if (c == '(') depth++;
            else if (c == ')') { depth--; if (depth == 0) return i; }
        }
        return -1;
    }

    private static PropSig parseProp(Path file, int lineNo, String body) throws IOException {
        // "<name>:<type> [public] [sync]"
        int colon = body.indexOf(':');
        if (colon < 0) throw new IOException(file + ":" + lineNo + ": prop mal formada (falta ':')");
        String name = body.substring(0, colon).trim();
        String rest = body.substring(colon + 1).trim();
        // partir el resto en tipo + flags
        int sp = rest.indexOf(' ');
        String typeStr = (sp < 0) ? rest : rest.substring(0, sp).trim();
        String flagsStr = (sp < 0) ? "" : rest.substring(sp + 1).trim();
        BpType type = parseType(file, lineNo, typeStr);
        boolean isSync = false;
        if (!flagsStr.isEmpty()) {
            for (String tk : flagsStr.split("\\s+")) {
                if ("public".equals(tk)) { /* sólo properties públicas se exportan */ }
                else if ("sync".equals(tk)) isSync = true;
                else throw new IOException(file + ":" + lineNo + ": flag desconocido en prop '" + tk + "'");
            }
        }
        return new PropSig(name, type, isSync);
    }

    private static ConstSig parseConst(Path file, int lineNo, String body) throws IOException {
        // "<name>:<type>=<value> [public]"
        int colon = body.indexOf(':');
        if (colon < 0) throw new IOException(file + ":" + lineNo + ": const mal formada (falta ':')");
        String name = body.substring(0, colon).trim();
        String rest = body.substring(colon + 1).trim();
        int eq = rest.indexOf('=');
        if (eq < 0) throw new IOException(file + ":" + lineNo + ": const mal formada (falta '=')");
        String typeStr = rest.substring(0, eq).trim();
        String tailVal = rest.substring(eq + 1).trim();
        // recortar flags ("public") al final
        String valueStr;
        if (typeStr.equals("string") && tailVal.startsWith("\"")) {
            // valor es string: el cierre " marca el final, lo que sigue es flags
            int closeQuote = -1;
            boolean esc = false;
            for (int i = 1; i < tailVal.length(); i++) {
                char ch = tailVal.charAt(i);
                if (esc) { esc = false; continue; }
                if (ch == '\\') { esc = true; continue; }
                if (ch == '"') { closeQuote = i; break; }
            }
            if (closeQuote < 0)
                throw new IOException(file + ":" + lineNo + ": const string sin comillas de cierre");
            valueStr = tailVal.substring(0, closeQuote + 1);
        } else {
            int sp = tailVal.indexOf(' ');
            valueStr = (sp < 0) ? tailVal : tailVal.substring(0, sp);
        }
        BpType type = parseType(file, lineNo, typeStr);
        Object lit = decodeLiteral(type, valueStr, file, lineNo);
        return new ConstSig(name, type, lit);
    }

    private static FuncSig parseFunc(Path file, int lineNo, String body) throws IOException {
        int lp = body.indexOf('(');
        int rp = (lp < 0) ? -1 : matchParen(body, lp);
        if (lp < 0 || rp < 0)
            throw new IOException(file + ":" + lineNo + ": func mal formada (faltan paréntesis)");
        String name = body.substring(0, lp).trim();
        String paramsRaw = body.substring(lp + 1, rp).trim();
        String tail = body.substring(rp + 1).trim();
        if (!tail.startsWith(":"))
            throw new IOException(file + ":" + lineNo + ": func mal formada (falta ':' tras los params)");
        tail = tail.substring(1).trim();

        String retStr;
        String flagsStr;
        int sp = tail.indexOf(' ');
        if (sp < 0) { retStr = tail; flagsStr = ""; }
        else { retStr = tail.substring(0, sp).trim(); flagsStr = tail.substring(sp + 1).trim(); }

        BpType ret = parseType(file, lineNo, retStr);

        boolean isPublic = false;
        boolean isStatic = false;
        boolean isIntrinsic = false;
        if (!flagsStr.isEmpty()) {
            for (String tk : flagsStr.split("\\s+")) {
                if ("public".equals(tk)) isPublic = true;
                else if ("static".equals(tk)) isStatic = true;
                else if ("intrinsic".equals(tk)) isIntrinsic = true;
                else throw new IOException(file + ":" + lineNo + ": flag desconocido '" + tk + "'");
            }
        }

        List<ParamSig> params = parseParamList(file, lineNo, paramsRaw);

        return new FuncSig(name, isPublic, isStatic, isIntrinsic, params, ret);
    }

    private static BpType parseType(Path file, int lineNo, String s) throws IOException {
        // L-arr-export: si termina en "[]", procesamos array recursivamente.
        if (s.endsWith("[]")) {
            BpType elem = parseType(file, lineNo, s.substring(0, s.length() - 2));
            if (elem == null) {
                throw new IOException(file + ":" + lineNo
                        + ": elemento de array no puede ser void: " + s);
            }
            return new BpType.ArrayType(elem);
        }
        // Tuplas (retorno múltiple): `(t1,t2,...)`. Split por comas de nivel
        // superior (respeta ()/[] anidados) y parsea cada elemento.
        if (s.startsWith("(") && s.endsWith(")")) {
            String inner = s.substring(1, s.length() - 1);
            java.util.List<BpType> elems = new java.util.ArrayList<>();
            int depth = 0, start = 0;
            for (int i = 0; i < inner.length(); i++) {
                char c = inner.charAt(i);
                if (c == '(' || c == '[') depth++;
                else if (c == ')' || c == ']') depth--;
                else if (c == ',' && depth == 0) {
                    BpType el = parseType(file, lineNo, inner.substring(start, i).trim());
                    if (el == null) throw new IOException(file + ":" + lineNo + ": elemento de tupla void: " + s);
                    elems.add(el); start = i + 1;
                }
            }
            BpType last = parseType(file, lineNo, inner.substring(start).trim());
            if (last == null) throw new IOException(file + ":" + lineNo + ": elemento de tupla void: " + s);
            elems.add(last);
            return new BpType.TupleType(elems);
        }
        switch (s) {
            case "void":    return null;
            case "integer": return PrimitiveType.INTEGER;
            case "float":   return PrimitiveType.FLOAT;
            case "string":  return PrimitiveType.STRING;
            case "boolean": return PrimitiveType.BOOLEAN;
            // H1.2/H1.3 (V2) — escalares de 64 bits cross-module en .bpi.
            // Se habían omitido aquí (el serializador SÍ los escribe); sin esto
            // un parámetro/retorno `long`/`double` importado se trataba como
            // UnresolvedClassRef → coerceToTarget no insertaba el widening
            // int→long → desalineación de slots. Análogo al fix de tipos
            // estrechos (H1.1). Descubierto al exportar Str (primer módulo con
            // parámetros long/double).
            case "long":    return PrimitiveType.LONG;
            case "double":  return PrimitiveType.DOUBLE;
            // H1.1 (V2) — tipos estrechos en .bpi (byte[]/word[]/int8[]/...
            // cross-module). El serializador escribe el nombre del enum en
            // minúsculas (typeToString: tag.name().toLowerCase()): UINT8→"uint8",
            // UINT16→"uint16", etc. Aquí el round-trip de vuelta al singleton.
            case "int8":    return PrimitiveType.INT8;
            case "uint8":   return PrimitiveType.UINT8;
            case "int16":   return PrimitiveType.INT16;
            case "uint16":  return PrimitiveType.UINT16;
            case "any":     return BpType.AnyType.INSTANCE;
            default:
                // L2: identificador no-primitivo → asumimos clase del mismo
                // módulo (o jerarquía de stdlib). El consumidor del .bpi
                // (Main.java loader) resolverá UnresolvedClassRef a un
                // ClassType una vez tenga todos los stubs construidos.
                // L2 v3.e — acepta nombres dotted (`L2Lib.Counter`) para
                // tipos de clase cross-module. El loader resuelve el name
                // contra los ImportedNamespaceSymbol disponibles.
                if (!isValidDottedIdentifier(s))
                    throw new IOException(file + ":" + lineNo + ": tipo no soportado en .bpi: " + s);
                return new BpType.UnresolvedClassRef(s);
        }
    }

    /** Identificador simple o dotted (e.g. `Counter` o `L2Lib.Counter`). */
    private static boolean isValidDottedIdentifier(String s) {
        if (s == null || s.isEmpty()) return false;
        if (!Character.isJavaIdentifierStart(s.charAt(0))) return false;
        for (int i = 1; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c == '.') {
                // No vacíos a izq ni der; siguiente char debe ser start.
                if (i + 1 >= s.length()
                        || !Character.isJavaIdentifierStart(s.charAt(i + 1))) return false;
            } else if (!Character.isJavaIdentifierPart(c)) {
                return false;
            }
        }
        return true;
    }
}
