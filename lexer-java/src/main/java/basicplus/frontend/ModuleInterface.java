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
    public static final int CURRENT_VERSION = 5;
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
        public ParamSig(String name, BpType type) { this.name = name; this.type = type; }
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
                for (ParamSymbol p : fs.params) ps.add(new ParamSig(p.name, p.type));
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
            for (ParamSymbol p : ctor.params) ctorParams.add(new ParamSig(p.name, p.type));
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
                    for (ParamSymbol p : fsym.params) ps.add(new ParamSig(p.name, p.type));
                    methods.add(new FuncSig(fsym.name, true, false, ps, fsym.returnType));
                }
            }
        }
        // baseClassName: hoy informativo; el importador no usa el campo para
        // reconstruir vtables porque v1 no soporta herencia cross-module.
        String parent = (cls.baseClass != null) ? cls.baseClass.name : null;
        return new ClassSig(cls.name, true, parent, ctorParams, properties, methods);
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
                    ParamSig p = f.params.get(i);
                    sb.append(p.name).append(':').append(typeToString(p.type));
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

                if (c.ctorParams != null) {
                    StringBuilder cs = new StringBuilder();
                    cs.append("  ctor(");
                    for (int i = 0; i < c.ctorParams.size(); i++) {
                        if (i > 0) cs.append(',');
                        ParamSig p = c.ctorParams.get(i);
                        cs.append(p.name).append(':').append(typeToString(p.type));
                    }
                    cs.append(')');
                    pw.println(cs.toString());
                }
                for (PropSig p : c.properties) {
                    pw.printf("  prop %s:%s public%s%n",
                            p.name, typeToString(p.type), p.isSync ? " sync" : "");
                }
                for (FuncSig m : c.methods) {
                    StringBuilder sb = new StringBuilder();
                    sb.append("  method ").append(m.name).append('(');
                    for (int i = 0; i < m.params.size(); i++) {
                        if (i > 0) sb.append(',');
                        ParamSig p = m.params.get(i);
                        sb.append(p.name).append(':').append(typeToString(p.type));
                    }
                    sb.append("):").append(typeToString(m.returnType));
                    sb.append(" public");
                    pw.println(sb.toString());
                }
                pw.println("end class");
            }
        }
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
                case FLOAT:
                    try { return Double.parseDouble(s); }
                    catch (NumberFormatException ex) {
                        throw new IOException(file + ":" + lineNo + ": float inválido: " + s);
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

    private static String typeToString(BpType t) {
        if (t == null) return "void";
        if (t instanceof VoidType) return "void";
        if (t instanceof PrimitiveType) return ((PrimitiveType) t).tag.name().toLowerCase();
        // L2: clase del mismo módulo o stub no resuelto — serializamos por nombre.
        if (t instanceof BpType.ClassType) return ((BpType.ClassType) t).cls.name;
        if (t instanceof BpType.UnresolvedClassRef) return ((BpType.UnresolvedClassRef) t).name;
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
                        iface.classes.add(new ClassSig(clsName, true, clsParent,
                                clsCtorParams, clsProps, clsMethods));
                        clsName = null; clsParent = null;
                        clsCtorParams = null; clsProps = null; clsMethods = null;
                        continue;
                    }
                    if (trimmed.startsWith("ctor(") || trimmed.startsWith("ctor (")) {
                        String body = trimmed.substring(trimmed.indexOf('(') + 1);
                        int rp = body.indexOf(')');
                        if (rp < 0) throw new IOException(file + ":" + lineNo + ": ctor sin ')'");
                        clsCtorParams = parseParamList(file, lineNo, body.substring(0, rp));
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
        for (String pTok : paramsRaw.split(",")) {
            pTok = pTok.trim();
            int colon = pTok.indexOf(':');
            if (colon < 0)
                throw new IOException(file + ":" + lineNo + ": param mal formado: '" + pTok + "'");
            String pname = pTok.substring(0, colon).trim();
            String ptype = pTok.substring(colon + 1).trim();
            params.add(new ParamSig(pname, parseType(file, lineNo, ptype)));
        }
        return params;
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
        int rp = body.indexOf(')', lp + 1);
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

        List<ParamSig> params = new ArrayList<>();
        if (!paramsRaw.isEmpty()) {
            for (String pTok : paramsRaw.split(",")) {
                pTok = pTok.trim();
                int colon = pTok.indexOf(':');
                if (colon < 0)
                    throw new IOException(file + ":" + lineNo + ": param mal formado: '" + pTok + "'");
                String pname = pTok.substring(0, colon).trim();
                String ptype = pTok.substring(colon + 1).trim();
                params.add(new ParamSig(pname, parseType(file, lineNo, ptype)));
            }
        }

        return new FuncSig(name, isPublic, isStatic, isIntrinsic, params, ret);
    }

    private static BpType parseType(Path file, int lineNo, String s) throws IOException {
        switch (s) {
            case "void":    return null;
            case "integer": return PrimitiveType.INTEGER;
            case "float":   return PrimitiveType.FLOAT;
            case "string":  return PrimitiveType.STRING;
            case "boolean": return PrimitiveType.BOOLEAN;
            case "any":     return BpType.AnyType.INSTANCE;
            default:
                // L2: identificador no-primitivo → asumimos clase del mismo
                // módulo (o jerarquía de stdlib). El consumidor del .bpi
                // (Main.java loader) resolverá UnresolvedClassRef a un
                // ClassType una vez tenga todos los stubs construidos.
                // Validamos que sea un identificador legal para evitar basura.
                if (!isValidIdentifier(s))
                    throw new IOException(file + ":" + lineNo + ": tipo no soportado en .bpi: " + s);
                return new BpType.UnresolvedClassRef(s);
        }
    }

    private static boolean isValidIdentifier(String s) {
        if (s == null || s.isEmpty()) return false;
        if (!Character.isJavaIdentifierStart(s.charAt(0))) return false;
        for (int i = 1; i < s.length(); i++) {
            if (!Character.isJavaIdentifierPart(s.charAt(i))) return false;
        }
        return true;
    }
}
