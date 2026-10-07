// ============================================================
// AotCEmitter.java
// Emisor AOT — toma funciones BP marcadas con `native` y genera
// C source equivalente para compilar en el firmware.
//
// Subset mínimo (H3 #157 fase 1):
//   - Tipos: integer (i32 únicamente). Float, string, arrays → TODO.
//   - Statements: IfStmt, ReturnStmt, VarDecl (con init), AssignStmt
//                 (a locales), ExprStmt (call con efectos).
//   - Expressions: IntLitExpr, IdentifierExpr (locales), BinaryExpr
//                  (+,-,*,/,mod,<,>,<=,>=,==,!=), UnaryExpr (-,not),
//                  CallExpr (a otras funciones AOT del mismo módulo
//                  por C call directa; a interpreted → fallback con
//                  vm_call_bp helper TODO).
//
// El emisor genera DOS funciones por cada native:
//   1) static int32_t aot_<Mod>_<func>(struct bpvm* vm, int32_t arg1, ...)
//   2) static void thunk_<Mod>_<func>(struct bpvm* vm, uint32_t* sp_p,
//                                     uint32_t* bp_p)
// más una función `aot_<Mod>_register` que registra todos los thunks
// en el AOT registry tras link.
//
// Convención: se asume que el firmware tiene bpvm_aot_register_by_name
// y el helper bpvm_read_i32_be / bpvm_write_i32_be (igual que el
// thunk manual de #160).
// ============================================================
package basicplus.frontend;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import basicplus.frontend.BpType.*;   // #173 — PrimitiveType (anidado en BpType)

public final class AotCEmitter {

    /** Excepción específica para señalar que una construcción no se
     *  puede traducir a AOT C. El caller la captura y reporta. */
    public static final class UnsupportedAotException extends RuntimeException {
        public UnsupportedAotException(String msg) { super(msg); }
    }

    private final String moduleName;
    private final StringWriter out = new StringWriter();
    private final PrintWriter w = new PrintWriter(out);

    /** Funciones nativas vistas en este módulo — para que el emisor
     *  reconozca CallExpr internos como C call directa. */
    private final Set<String> nativeFuncNames = new HashSet<>();

    /** Tabla nombre → FuncDef para resolver firma desde CallExpr. */
    private final java.util.Map<String, Ast.FuncDef> nativeFuncDefs = new java.util.HashMap<>();

    /** #211 — Tabla nombre → FuncDef de TODAS las funciones del módulo
     *  (native y BP normales). Permite que una `native` que llama a una
     *  función BP del mismo módulo resuelva su firma y emita la llamada-
     *  puente (vm->aot_helpers->call_bp_i32) en vez de abortar. */
    private final java.util.Map<String, Ast.FuncDef> allFuncDefs = new java.util.HashMap<>();

    /** #211 — Avisos no fatales recogidos durante la emisión (p.ej. una
     *  llamada native→BP que cruza al intérprete y pierde la velocidad
     *  AOT). El caller (Main/AotMain) los imprime tras emitModule. */
    private final List<String> warnings = new ArrayList<>();
    public List<String> getWarnings() { return warnings; }

    /** [V6/N1.1] Un metodo aplanado se llama `<Clase>.<metodo>`, y el punto NO
     *  vale en un identificador de C. El nombre de REGISTRO conserva el punto
     *  (es el simbolo del .mod); solo se sanean los identificadores emitidos. */
    private static String cId(String nombre) { return nombre.replace('.', '_'); }

    /** [V7/H5 #572] EL NOMBRE EN C DE UN IDENTIFICADOR DE BP —parámetro, local, iterador de un `for`, el `this` de
     *  un método aplanado—: con el prefijo `bp_`, y en ESTE sitio y en ningún otro. Hasta el 27-sep se escribían
     *  tal cual, y chocaban con lo que el emisor declara en la función (`vm`, su primer parámetro; `mem` y `cs` en un
     *  módulo con globales; `H`, `fn`, `nm`, `r`, `k`… en el puente a un pack) o con una palabra clave o una macro de
     *  C (`int`, `char`, `static`, `bool`, `INT32_MAX`…): gcc no compilaba el módulo y sus natives corrían interpretadas,
     *  con un aviso. Con el prefijo no queda nada con qué chocar: todo lo que declara el emisor empieza por otra cosa
     *  (`vm`, `mem`, `cs`, `H`, `__…`, `aot_…`), y ninguna cabecera que incluye el C generado define nada que empiece
     *  por `bp_` (mirado el 27-sep; los `bp_*` que hay son campos de struct y nombres de parámetro de un prototipo).
     *  El nombre de BP se sigue usando tal cual para BUSCAR (`localNames`) y en los mensajes. */
    private static String cVar(String nombreBp) { return "bp_" + nombreBp; }

    /** Los metodos que este modulo aplano. Si uno no se puede emitir NO tumba el
     *  modulo entero —eso seria una regresion: hoy esos modulos emiten bien— sino
     *  que se cae solo, con su aviso. */
    private final java.util.Set<String> metodosAplanados = new java.util.HashSet<>();

    /** #211 — nombre de la función AOT que se está emitiendo (para mensajes
     *  de aviso). Fijado al inicio de cada emitFunction. */
    private String currentFuncName = "?";
    /** [V7/H5 #559] El tipo que devuelve la función que se está emitiendo: el `return`
     *  convierte a él (OP_I2F y compañía), como el intérprete. */
    private BpType retActual = null;
    /** V7/H2 (#533) — el nombre canónico del módulo que se emite (`Mod` o `lib.Mod`), para las
     *  claves R4 y los nombres de clase que el `.mdn` pide al cargador en ejecución. */
    private String moduloLocal = null;

    /** Contador para nombres únicos en for-loops (avoid colisión de
     *  __end/__step entre fors anidados). */
    private int forCounter = 0;
    /** #546 — ídem para el sujeto de un switch sobre string ({@code __swN}). */
    private int switchCounter = 0;

    /** #580 (1), decidido por Eduardo el 3-oct («como un bucle») — UNA RECURSIÓN ES UN BUCLE. Una native que puede
     *  volver a entrar en sí misma por llamadas de C entre natives del módulo (`fib` → `fib`; `a` → `b` → `a`) no pasa
     *  nunca por la línea del bucle si no tiene bucles —ni cede ni ve un `kill`: `fib(40)` en la C3, 32 s sin poder
     *  pararla—, y con bucles cortos tampoco (el contador de la línea es local a cada llamada). Éstas llevan la marca a
     *  la ENTRADA: un contador que no puede ser local, así que viaja por un parámetro oculto (`CUENTA_R`) desde el thunk
     *  —o desde la native que entra en el ciclo— y cada `CUENTA_VUELTAS` entradas llama a `punto_control`. Con eso
     *  dejan de ser directas (`nativesDirectas` ve el helper) y van a un ejecutor, como una native con bucle. Lo que
     *  cruza por BP ya cede en el puente y no es una llamada de C: no cuenta. Medido en la C3 (V7_IDEAS §H5, `#580`):
     *  `fib(30)` de 262 a 398 ms (el peor caso: toda ella es llamar); el KILL de `fib(40)`, de 31,6 s a 26 ms.
     *  Las calcula una emisión EN SECO del módulo (`soloGrafo`), que apunta cada llamada de C a otra native donde la
     *  escribe (`llamadasDirectas`): el grafo es el del C que sale, como `nativesDirectas`. */
    private Set<String> recursivas = new HashSet<>();
    private boolean soloGrafo = false;
    private final Map<String, Set<String>> llamadasDirectas = new HashMap<>();
    private static final String CUENTA_R = "__bpvm_cr";

    /** #172 — nombres definidos en el ámbito de la función actual
     *  (parámetros + VarDecl + induction de for). Cualquier
     *  IdentifierExpr cuyo name NO esté aquí se asume variable
     *  nivel-módulo y se emite como acceso directo `mem + cs + OFFSET`,
     *  donde `cs` es la CS del módulo (cacheada via helper
     *  find_module_cs) y OFFSET es el literal compile-time guardado
     *  en `moduleVarOffsets`.
     *
     *  Limpieza: se vacía al inicio de cada emitFunction. */
    private final Set<String> localNames = new HashSet<>();

    /** #172 — offsets CS-relativos de variables nivel-módulo. Asignados
     *  en {@link #precomputeModuleVarOffsets} antes de emitir cualquier
     *  función, recorriendo module.defs en orden de declaración. Convención
     *  empírica del MivmEmitter: vars y consts comparten el área negativa
     *  desde CS hacia abajo, cada uno gana el siguiente slot de 4 bytes.
     *  El offset es negativo: counter → -4, segundo decl → -8, etc.
     *
     *  Si AotCEmitter ve un IdentifierExpr no-local que NO aparece aquí,
     *  asume que es un símbolo importado / cross-module y lanza
     *  UnsupportedAotException (cross-module sigue diferido — #169 / v2). */
    /** [V7/H5 #559] Las globales de `moduleVarOffsets` que son `float`: se leen y se
     *  escriben con read/write_f32_be (el patrón de bits), no con los de i32. */
    private final Set<String> moduleVarFloat = new HashSet<>();
    private final java.util.Map<String, Integer> moduleVarOffsets =
            new java.util.LinkedHashMap<>();

    /** #172 — true si alguna función emitida hasta ahora consume globals.
     *  Cuando es true, el header del .c incluye
     *    - static inline uint32_t aot_<Mod>_cs(struct bpvm* vm) { ... }
     *  y cada función AOT empieza con: `uint32_t cs = aot_<Mod>_cs(vm);`.
     *  (V7/H5 #562: ya SIN la caché `static` que tuvo — ver emitModule.) */
    private boolean usesModuleGlobals = false;

    /** H3 #158 — si es true, NO emite aot_<Mod>_register (esa función
     *  tiene relocs externas a bpvm_aot_register_by_name + string
     *  literal, que rompen la position-independence del .o standalone).
     *  El loader del .mdn registra automáticamente desde el symtab. */
    private boolean omitRegisterFunc = false;

    /* V5/H4 - `import v1 from pack "SQLI"`: identidad del pack NATIVO del que
     * este modulo toma sus `native` externas. 0 = no habla con ninguno.
     * La marca viaja como uint32 con los 4 caracteres empaquetados, que es como
     * la publica el pack y como la espera `pack_sym`. */
    private int packMarca   = 0;
    private int packVersion = 0;
    /** Una comilla simple. Aparece mucho al emitir C (nombres byte a byte,
     *  mensajes que citan un símbolo) y escaparla cada vez se lee fatal. */
    private static final String Q = "'";

    private int indentLevel = 0;

    public AotCEmitter(String moduleName) {
        this.moduleName = moduleName;
    }

    /** #173 — info semántica para conocer el tipo de cada expresión
     *  (info.exprTypes). Necesaria para distinguir ops de string de las
     *  numéricas: `a + b` es concat si algún operando es string, y
     *  `==`/`!=` de strings comparan contenido (no la ref). Si es null
     *  (emisión sin análisis), las ops string-ambiguas caen al
     *  comportamiento numérico — por eso AotMain / Main lo inyectan
     *  siempre que pueden. Los builtins de string (charAt, substring...)
     *  y los literales NO dependen de esto (se detectan por nombre/nodo). */
    private SemanticInfo semInfo = null;
    public void setSemanticInfo(SemanticInfo info) { this.semInfo = info; }

    /** #173 — ¿el tipo resuelto de `e` es string? false si no hay info. */
    private boolean isStringExpr(Ast.IExpr e) {
        if (semInfo == null) return false;
        BpType t = semInfo.exprTypes.get(e);
        return (t instanceof PrimitiveType)
            && ((PrimitiveType) t).tag == PrimitiveType.Kind.STRING;
    }

    /** #381 — ¿el tipo resuelto de `e` es long? Decide dos cosas: que su hueco
     *  en la pila BP son 8 bytes, y que su división va por helper y no con el
     *  `/` de C (que en el micro sería una llamada a libgcc, imposible en un
     *  `.mdn`). */
    private boolean isLongExpr(Ast.IExpr e) {
        if (semInfo == null) return false;
        BpType t = semInfo.exprTypes.get(e);
        return (t instanceof PrimitiveType)
            && ((PrimitiveType) t).tag == PrimitiveType.Kind.LONG;
    }

    /* ─── [V7/H5 #559, 25-sep] LAS CONVERSIONES NUMÉRICAS, CALCADAS DEL INTÉRPRETE ───
     *
     * Hasta hoy el emisor dejaba que C convirtiera por su cuenta: `float x = i;`,
     * `a + i` con `a` float, `(double)(f)` delante de un helper de double. En C eso es
     * correcto, pero gcc lo resuelve con una rutina de libgcc cuando el micro no tiene la
     * instrucción — y un `.mdn` no se lleva libgcc: la llamada quedaba en la dirección 0.
     * Medido con una sonda de catorce `native` (`Censo559`), lo que quedaba sin resolver:
     *
     *    ARM (Cortex-M33)  i2d f2d d2f d2iz d2lz f2lz l2f      (el enlace fallaba, críptico)
     *    P4 (ilp32f)       las mismas siete, con nombre de RISC-V (enlazaba y saltaba a 0)
     *    C5 (ilp32)        esas siete + TODA la aritmética y las comparaciones de float
     *
     * O sea: `double` mezclado con otro tipo y `long`↔`float` rotos EN TODAS LAS PLACAS, y
     * `float` entero roto en las que no tienen FPU. La regla ahora es la del compilador de
     * bytecode —`coerceToTarget`, `emitNumericConversion` y `promoteNumeric` de
     * MivmEmitter, la torre double > float > long > int— y cada conversión que el micro no
     * hace en hardware va por su helper, que es la misma expresión que el opcode. */

    /** El tipo que el análisis resolvió para `e`, o null. */
    private BpType tipoDe(Ast.IExpr e) {
        return (semInfo != null) ? semInfo.exprTypes.get(e) : null;
    }
    private static boolean esKind(BpType t, PrimitiveType.Kind k) {
        return t instanceof PrimitiveType && ((PrimitiveType) t).tag == k;
    }
    private static boolean esFloatT(BpType t)  { return esKind(t, PrimitiveType.Kind.FLOAT); }
    private static boolean esDoubleT(BpType t) { return esKind(t, PrimitiveType.Kind.DOUBLE); }
    private static boolean esLongT(BpType t)   { return esKind(t, PrimitiveType.Kind.LONG); }
    private static boolean esEnteroT(BpType t) {
        return t instanceof PrimitiveType && ((PrimitiveType) t).isIntegerLike();
    }

    /** El tipo numérico de una declaración (`var x: float`, un parámetro, el retorno),
     *  o null si no es uno de los que convierten. Case-sensitive, como MivmEmitter
     *  (BUG-6: una clase `Float` no es el primitivo). */
    private static BpType tipoDeclarado(Ast.TypeRef t) {
        if (!(t instanceof Ast.SimpleTypeRef)) return null;
        switch (((Ast.SimpleTypeRef) t).name) {
            case "float":   return PrimitiveType.FLOAT;
            case "double":  return PrimitiveType.DOUBLE;
            case "long":    return PrimitiveType.LONG;
            case "integer": return PrimitiveType.INTEGER;
            default:        return null;
        }
    }

    /** Emite `e` como valor de tipo `destino`: la conversión IMPLÍCITA del intérprete
     *  (MivmEmitter.coerceToTarget — asignación, parámetro, retorno, elemento de array).
     *  Sin tipo conocido, tal cual: si C tuviera que convertir con libgcc, la guarda de
     *  AotBuild lo rechaza con el nombre de la native. */
    private void emitComo(Ast.IExpr e, BpType destino) {
        String[] conv = conversionImplicita(tipoDe(e), destino);
        if (conv == null) { emitExpr(e); return; }
        w.print(conv[0]);
        emitExpr(e);
        w.print(conv[1]);
    }

    /** Lo que envuelve a un valor de tipo `src` para usarlo como `dst`, o null si no
     *  hace falta nada (mismo tipo, o `integer`→`long`, que C hace sin libgcc).
     *  Los `double` y las de 64 bits van SIEMPRE por helper: ninguna placa las tiene en
     *  hardware. `integer`→`float`, por la macro: instrucción con FPU, helper sin ella. */
    private static String[] conversionImplicita(BpType src, BpType dst) {
        if (src == null || dst == null) return null;
        if (esFloatT(dst)) {
            if (esEnteroT(src)) return new String[] { "BPVM_AOT_I2F(vm, ", ")" };
            if (esLongT(src))   return new String[] { "vm->aot_helpers->l2f(", ")" };
            return null;
        }
        if (esDoubleT(dst)) {
            if (esEnteroT(src)) return new String[] { "vm->aot_helpers->i2d(", ")" };
            if (esLongT(src))   return new String[] { "vm->aot_helpers->l2d(", ")" };
            if (esFloatT(src))  return new String[] { "vm->aot_helpers->f2d(", ")" };
            return null;
        }
        return null;
    }

    /** `integer(x)`, `long(x)`, `float(x)` y `double(x)`: la conversión EXPLÍCITA del
     *  intérprete (MivmEmitter.emitNumericConversion), opcode por opcode. */
    private void emitConversionExplicita(String destino, Ast.IExpr x) {
        BpType src = tipoDe(x);
        String pre, post = ")";
        switch (destino) {
            case "integer":
                if (esFloatT(src))       pre = "BPVM_AOT_F2I(vm, ";            /* OP_F2I */
                else if (esDoubleT(src)) pre = "vm->aot_helpers->d2i(";        /* OP_D2I */
                else { pre = "((int32_t) ("; post = "))"; }                    /* OP_I64_TO_I32, o nada */
                break;
            case "long":
                if (esFloatT(src))       pre = "vm->aot_helpers->f2l(";        /* OP_F2L */
                else if (esDoubleT(src)) pre = "vm->aot_helpers->d2l(";        /* OP_D2L */
                else { pre = "((int64_t) ("; post = "))"; }                    /* OP_I32_TO_I64, o nada */
                break;
            case "float":
                if (esEnteroT(src))      pre = "BPVM_AOT_I2F(vm, ";            /* OP_I2F */
                else if (esLongT(src))   pre = "vm->aot_helpers->l2f(";        /* OP_L2F */
                else if (esDoubleT(src)) pre = "vm->aot_helpers->d2f(";        /* OP_D2F */
                else { pre = "((float) ("; post = "))"; }
                break;
            case "double":
                if (esEnteroT(src))      pre = "vm->aot_helpers->i2d(";        /* OP_I2D */
                else if (esLongT(src))   pre = "vm->aot_helpers->l2d(";        /* OP_L2D */
                else if (esFloatT(src))  pre = "vm->aot_helpers->f2d(";        /* OP_F2D */
                else { pre = "((double) ("; post = "))"; }
                break;
            default:
                throw new IllegalStateException("conversion a " + destino);
        }
        w.print(pre);
        emitExpr(x);
        w.print(post);
    }

    /** La macro de la operación de `float` (bpvm_aot_helpers.h): la instrucción donde
     *  hay FPU, el helper donde no. `mod` no está: va siempre por `fmod` (libm). */
    private static String floatMacro(String op) {
        if (op == null) return null;
        switch (op) {
            case "+":   return "BPVM_AOT_FADD";
            case "-":   return "BPVM_AOT_FSUB";
            case "*":   return "BPVM_AOT_FMUL";
            case "/":   return "BPVM_AOT_FDIV";
            case "==":  return "BPVM_AOT_FEQ";
            case "<>": case "!=": return "BPVM_AOT_FNEQ";
            case "<":   return "BPVM_AOT_FLT";
            case "<=":  return "BPVM_AOT_FLE";
            case ">":   return "BPVM_AOT_FGT";
            case ">=":  return "BPVM_AOT_FGE";
            default:    return null;
        }
    }

    /** `l op r` hecha en el tipo `tipo` (FLOAT o DOUBLE), con los operandos ya pasados
     *  a ese tipo. Devuelve false si el operador no es aritmético ni de comparación
     *  (y entonces quien llama sigue por el camino de C). */
    private boolean emitOpFlotante(String op, BpType tipo, Ast.IExpr l, Ast.IExpr r) {
        String pre;
        if (esDoubleT(tipo)) {
            String dh = dobleHelper(op);
            if (dh == null || "dpow".equals(dh)) return false;
            pre = "vm->aot_helpers->" + dh + "(";
        } else {
            if ("mod".equals(op) || "%".equals(op)) pre = "vm->aot_helpers->fmod(";   /* OP_FMOD: fmodf */
            else {
                String m = floatMacro(op);
                if (m == null) return false;
                pre = m + "(vm, ";
            }
        }
        w.print(pre);
        emitComo(l, tipo);
        w.print(", ");
        emitComo(r, tipo);
        w.print(")");
        return true;
    }

    /** `destino op= valor` (`+=`, `-=`) sobre un `float` o un `double`: la operación
     *  del intérprete (MivmEmitter.emitCompoundOp) con el valor convertido al tipo del
     *  destino. `leer` emite el valor actual del destino. */
    private void emitCompuestoFlotante(Ast.AssignOpKind op, BpType tipo, Runnable leer, Ast.IExpr valor) {
        boolean mas = (op == Ast.AssignOpKind.PLUS_ASSIGN);
        if (esDoubleT(tipo)) w.print("vm->aot_helpers->" + (mas ? "dadd" : "dsub") + "(");
        else                 w.print((mas ? "BPVM_AOT_FADD" : "BPVM_AOT_FSUB") + "(vm, ");
        leer.run();
        w.print(", ");
        emitComo(valor, tipo);
        w.print(")");
    }

    /** #426 (V6/N1.2) — ¿el tipo resuelto de `e` es double? Decide que sus
     *  operaciones van por HELPER y no con los operadores de C: en el micro un
     *  `a * b` de double es una llamada a libgcc (`__muldf3`), y un `.mdn` no
     *  resuelve símbolos externos. Mismo motivo que la división de `long`. */
    /** #426 — el helper que corresponde a cada operador de `double`, o null si
     *  ese operador no es de coma flotante (y entonces se emite como siempre).
     *  Las seis comparaciones van por separado a proposito: con NaN NO son
     *  negaciones unas de otras, asi que un unico comparador divergiria del
     *  interprete en cuanto apareciera un NaN. */
    /** Quien ocupa los 8 bytes, para que el comentario del thunk no mienta:
     *  decia "long" tambien para un `double` (#426). */
    private static String anchoQuien(Ast.TypeRef t) {
        if (t instanceof Ast.SimpleTypeRef && "double".equals(((Ast.SimpleTypeRef) t).name))
            return "double";
        return "long";
    }

    /** [V6/N1.3] El helper de impresion que toca segun el TIPO de la expresion,
     *  o excepcion si ese tipo no tiene ninguno. Rechazar es deliberado:
     *  imprimir un `long` con print_i32 truncaria en silencio. */
    private String printHelperDe(Ast.IExpr e) {
        BpType t = (semInfo != null) ? semInfo.exprTypes.get(e) : null;
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case INTEGER: case INT8: case UINT8: case INT16: case UINT16:
                    return "print_i32";
                case FLOAT:   return "print_f32";
                case STRING:  return "print_string";
                default: break;
            }
        }
        String cual = (t == null) ? "desconocido" : t.toString();
        throw new UnsupportedAotException(
            "AOT: `print` de un valor de tipo '" + cual + "' dentro de una funcion native "
            + "todavia no va (linea " + ((Ast.Node) e).line + "). Hoy van integer, float y string. "
            + "El motivo es que el interprete los imprime con rutinas distintas y la tabla "
            + "de helpers solo trae esas tres; usar otra truncaria en silencio. Opciones: "
            + "convertir a string antes, o sacar el `print` de la native.");
    }

    private static String dobleHelper(String op) {
        if (op == null) return null;
        switch (op) {
            case "+":   return "dadd";
            case "-":   return "dsub";
            case "*":   return "dmul";
            case "/":   return "ddiv";
            case "mod": case "%": return "dmod";
            case "^":   return "dpow";
            case "==":  return "deq";
            case "<>": case "!=": return "dneq";
            case "<":   return "dlt";
            case "<=":  return "dle";
            case ">":   return "dgt";
            case ">=":  return "dge";
            default:    return null;
        }
    }

    private boolean isDoubleExpr(Ast.IExpr e) {
        if (semInfo == null) return false;
        BpType t = semInfo.exprTypes.get(e);
        return (t instanceof PrimitiveType)
            && ((PrimitiveType) t).tag == PrimitiveType.Kind.DOUBLE;
    }

    /** #173 — ¿el tipo resuelto de `e` es integer? */
    private boolean isIntExpr(Ast.IExpr e) {
        if (semInfo == null) return false;
        BpType t = semInfo.exprTypes.get(e);
        /* V7/H5 (27-sep) — y los estrechos (`byte`, `word`, `int8`, `int16`): viajan ensanchados a 32 bits y el
         * intérprete los pasa a cadena con INT_TO_STRING, como un integer (MivmEmitter.coerceToString, L8 v2). Su
         * único llamador es emitStringOperand: `"b=" + byte(x)` en una native. */
        return (t instanceof PrimitiveType) && ((PrimitiveType) t).isIntegerLike();
    }

    /** #193 — ancho del elemento del array `arr` para elegir el helper AOT.
     *
     *  <h3>[V6/N1.5] Esto devolvía "i32" para todo lo que no fuera byte</h3>
     *
     *  Y no daba error: para un {@code long[]} emitía {@code array_load_i32},
     *  que lee CUATRO bytes donde hay ocho. El resultado no era un fallo — era
     *  un número plausible y equivocado. Igual con {@code word[]} (dos bytes),
     *  {@code double[]} y {@code float[]}.
     *
     *  <p>La limitación estaba ESCRITA («v1: solo integer[]», en la emisión de
     *  IndexExpr) y no la hacía cumplir nadie. Un límite que no falla no es un
     *  límite. Lo destapó el primer {@code long[]} creado dentro de una native:
     *  {@code 3000000000L + 4000000000L} devolvió -1294967296.
     *
     *  <p>Ahora cada ancho tiene su helper, y lo que no lo tiene se RECHAZA con
     *  el motivo — que es lo que debió pasar desde el principio. */
    private String arrElemKind(Ast.IExpr arr) {
        BpType t = (semInfo != null) ? semInfo.exprTypes.get(arr) : null;
        if (!(t instanceof ArrayType)) {
            throw new UnsupportedAotException(
                "AOT: indexar algo cuyo tipo de array no está resuelto (line "
                + ((Ast.Node) arr).line + "). Sin saber el ancho del elemento no se "
                + "puede elegir el helper, y elegir mal no falla: lee de más o de menos.");
        }
        return elemKindDe(((ArrayType) t).element);
    }

    /** [V6/N1.5] El sufijo de helper que corresponde a un tipo de elemento.
     *  `null` si ese elemento no tiene helper — quien llame decide qué decir. */
    private static String elemKindDe(BpType el) {
        if (el instanceof PrimitiveType) {
            switch (((PrimitiveType) el).tag) {
                case UINT8:  return "u8";
                case INT8:   return "i8";
                case UINT16: return "u16";
                case INT16:  return "i16";
                case LONG:   return "i64";
                case DOUBLE: return "f64";
                case FLOAT:  return "f32";
                case INTEGER: case BOOLEAN: return "i32";
                /* `string` ES un PrimitiveType y a la vez una REFERENCIA de heap
                 * (lo mismo que dice isRefType en MivmEmitter). Va por ref. */
                case STRING: return "ref";
                default: return null;
            }
        }
        /* Clase, array anidado, `any`, tupla → referencia. */
        return "ref";
    }
    /** Nombre del helper de carga de elemento según el ancho del array. */
    private String arrLoadFn(Ast.IExpr arr) {
        return "array_load_" + arrElemKind(arr);
    }
    /** Nombre del helper de store de elemento. El store TRUNCA, así que signed y
     *  unsigned comparten helper: `u8`/`i8` → store_i8, `u16`/`i16` → store_i16. */
    private String arrStoreFn(Ast.IExpr arr) {
        String k = arrElemKind(arr);
        if ("u8".equals(k)  || "i8".equals(k))  return "array_store_i8";
        if ("u16".equals(k) || "i16".equals(k)) return "array_store_i16";
        return "array_store_" + k;   /* i32, i64, f64, f32, ref */
    }

    /** [V6/N1.5] Sufijo del helper de reserva segun el tipo de elemento, o null
     *  si ese tipo de array no se puede crear desde una native todavia.
     *  Espeja `newarrayOpForElement` de MivmEmitter: el AOT tiene que reservar
     *  EXACTAMENTE lo mismo que el interprete o el mismo `.bp` daria dos
     *  layouts segun llevara `.mdn`. */
    private static String newarraySufijo(BpType el) {
        String k = elemKindDe(el);
        if (k == null) return null;
        if ("u8".equals(k)  || "i8".equals(k))  return "i8";
        if ("u16".equals(k) || "i16".equals(k)) return "i16";
        if ("f64".equals(k)) return "i64";   /* `double[]` = 8 bytes opacos */
        if ("f32".equals(k)) return "i32";   /* `float[]`  = 4, como integer[] */
        return k;                            /* i32, i64, ref */
    }

    /** [V6/N1.5] Helper de store que corresponde al elemento. `double` no
     *  comparte el de `long`: el valor cruza como `double`, no como patron de
     *  bits (mismo criterio que array_load_f64 desde V5/H4). */
    private static String arrayStoreSufijo(BpType el) {
        String k = elemKindDe(el);
        if (k == null) return null;
        if ("u8".equals(k)  || "i8".equals(k))  return "i8";
        if ("u16".equals(k) || "i16".equals(k)) return "i16";
        return k;                            /* i32, i64, f64, f32, ref */
    }

    /** [V6/N1.5] ¿Esta llamada es CREAR UN OBJETO? En BP se escribe igual que
     *  una llamada a funcion (`Caja(21)`), asi que la distincion no esta en la
     *  sintaxis: esta en que el TIPO de la expresion sea la clase que se nombra.
     *  Devuelve la clase, o null si no es una construccion. */
    private Symbol.ClassSymbol claseConstruida(Ast.CallExpr c) {
        BpType t = (semInfo != null) ? semInfo.exprTypes.get(c) : null;
        if (!(t instanceof BpType.ClassType)) return null;
        Symbol.ClassSymbol cls = ((BpType.ClassType) t).cls;
        return cls.name.equals(calleeSimpleName(c.callee)) ? cls : null;
    }

    /** [V6/N1.5] `[a, b, c]` → reservar + rellenar, devolviendo el handle. */
    private void emitArrayLit(Ast.ArrayLitExpr a) {
        BpType arrT = (semInfo != null) ? semInfo.exprTypes.get(a) : null;
        BpType el   = (arrT instanceof ArrayType) ? ((ArrayType) arrT).element : null;
        if (el == null) {
            throw new UnsupportedAotException(
                "AOT: literal de array sin tipo de elemento resuelto (line " + a.line + ")");
        }
        String nuevo = newarraySufijo(el);
        String store = arrayStoreSufijo(el);
        if (nuevo == null || store == null) {
            /* ⚠️ Aqui hubo un motivo FALSO, corregido por Eduardo el 24-ago-2026:
             * que un array de refs no se podia «porque en esta ABI una ref cruza
             * con la generacion descartada». No era perdida de informacion —
             * `bpref_regen` la reconstruye— asi que se hizo (N1.5b) y este
             * rechazo dejo de aplicarles. Queda para lo que de verdad no tiene
             * helper. */
            throw new UnsupportedAotException(
                "AOT: literal de array de '" + el + "' (line " + a.line + ") todavia no "
                + "esta implementado.");
        }
        int n = a.elements.size();
        w.print("({ int32_t __arr = vm->aot_helpers->newarray_" + nuevo + "(vm, " + n + "); ");
        for (int i = 0; i < n; i++) {
            w.print("vm->aot_helpers->array_store_" + store + "(vm, (uint32_t) __arr, " + i + ", ");
            emitComo(a.elements.get(i), el);   /* #559 — `[i, j]` en un float[] */
            w.print("); ");
        }
        w.print("__arr; })");
    }

    /** #173 — emite un operando de concat como string-handle. Si ya es
     *  string, tal cual; si es integer, lo convierte con int_to_string
     *  (cubre el patrón típico `"x = " + n`). Otros tipos mixtos
     *  (float/bool) aún sin soporte → UnsupportedAotException. */
    private void emitStringOperand(Ast.IExpr e) {
        if (isStringExpr(e)) {
            emitExpr(e);
            return;
        }
        if (isIntExpr(e)) {
            w.print("vm->aot_helpers->int_to_string(vm, ");
            emitExpr(e);
            w.print(")");
            return;
        }
        throw new UnsupportedAotException(
            "AOT: concat de string solo soporta string/integer por ahora "
            + "(float/bool → pendiente). line " + ((Ast.Node) e).line);
    }

    /** #173 — escapa los BYTES UTF-8 de un literal para un string C.
     *  Trabajamos sobre bytes (no chars) para que el length que pasamos
     *  a string_from_cstr cuadre exactamente con lo escapado. */
    private static String cEscapeBytes(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            int c = b & 0xff;
            switch (c) {
                case '"':  sb.append("\\\""); break;
                case '\\': sb.append("\\\\"); break;
                case '\n': sb.append("\\n");  break;
                case '\r': sb.append("\\r");  break;
                case '\t': sb.append("\\t");  break;
                default:
                    // 16-sep — en OCTAL, no en hex: `\x` en C es GLOTÓN (coge todos los
                    // dígitos hex que sigan), así que "módulos" salía `\xc3\xb3dulos` y
                    // gcc leía `\xb3d` («hex escape sequence out of range»): un byte de
                    // menos y una letra comida, en todo literal con un acento seguido de
                    // [0-9a-f]. `\ooo` son exactamente tres dígitos, siempre.
                    if (c < 0x20 || c > 0x7e) sb.append(String.format("\\%03o", c));
                    else sb.append((char) c);
            }
        }
        return sb.toString();
    }

    /** Activa modo "para .mdn": code section 100% PIC, sin
     *  referencias a símbolos externos del runtime. */
    public void setOmitRegisterFunc(boolean omit) {
        this.omitRegisterFunc = omit;
    }

    /**
     * Emite la unidad de compilación completa para todas las funciones
     * native del módulo. Devuelve el C source listo para escribir a
     * un archivo aot_<Mod>.c.
     *
     * Lanza UnsupportedAotException si alguna función contiene
     * constructos no soportados — en ese caso el módulo entero no se
     * emite (el flag native se ignora y solo queda el bytecode normal).
     */
    public String emitModule(Ast.ModuleNode module) {
        // V7/H2 (#523) — el formato del build y el módulo local, para que los nombres
        // que el `.mdn` pide al intérprete (factorías, funciones ajenas) sean los R4
        // que el `.mod` exporta en MOD9 (y los de siempre en MOD8).
        MivmEmitter.fijarContextoEstatico(Main.FORMATO, module);
        moduloLocal = MivmEmitter.moduloLocalDe(module);
        // Pre-pass: recolectar nombres de funciones native para
        // que el emisor reconozca self-calls como C calls directas.
        List<Ast.FuncDef> nativeFuncs = new ArrayList<>();
        for (Ast.ITopLevelDecl d : module.defs) {
            if (d instanceof Ast.FuncDef) {
                Ast.FuncDef f = (Ast.FuncDef) d;
                allFuncDefs.put(f.name.name, f);   /* #211 — TODAS, para call_bp */
                if (f.isNative && !f.isIntrinsic) {
                    nativeFuncs.add(f);
                    nativeFuncNames.add(f.name.name);
                    nativeFuncDefs.put(f.name.name, f);
                }
            }
        }
        /* [V6/N1.1] Un metodo `native` de una clase NO se emite, y hasta el
         * 23-ago-2026 eso pasaba EN SILENCIO: el bucle de arriba solo abre los
         * `Ast.FuncDef` de nivel modulo, asi que un `ClassDef` ni se miraba. El
         * compilador no daba error, AotMain decia "no tiene funciones native" y
         * el metodo corria INTERPRETADO mientras el programador creia tener
         * velocidad AOT. Pedir velocidad y que te la nieguen sin avisar.
         *
         * Mientras no se abra el barrido a los metodos (el otro medio trabajo de
         * la ficha), al menos que se DIGA. Es el mismo criterio que el proyecto
         * ya aplica en el gate del `.mod`, el `magic` de la BIOS, el sello del
         * `.npk` y el aviso de `/lib` rancio: un desfase mudo es peor que uno
         * ruidoso.
         *
         * Va ANTES del `return ""` a proposito: un modulo cuyas unicas `native`
         * sean metodos sale por ahi, y es justo el caso que hay que avisar. Los
         * dos consumidores (Main y AotMain) leen getWarnings() tambien en ese
         * camino. */
        for (Ast.ITopLevelDecl d : module.defs) {
            if (!(d instanceof Ast.ClassDef)) continue;
            Ast.ClassDef c = (Ast.ClassDef) d;
            for (Ast.ITopLevelDecl m : c.members) {
                if (!(m instanceof Ast.FuncDef)) continue;
                Ast.FuncDef f = (Ast.FuncDef) m;
                if (!f.isNative || f.isIntrinsic) continue;

                /* EL APLANADO. Idea de Eduardo: «mimodulo.miclase.mimetodonative(
                 * objMiclase, ...)» — y no hay que forzar nada, porque por debajo
                 * el metodo YA ES eso: `ModWriter.addMethod` hace
                 * `declareParam("this", 8)` antes que los demas parametros. Aqui
                 * solo se hace explicito para que la maquinaria de funciones
                 * —cuerpo, thunk, refs de 8B, registro— sirva sin tocarla. */
                List<Ast.Param> ps = new ArrayList<>();
                ps.add(new Ast.Param("this",
                        new Ast.SimpleTypeRef(c.name, f.line, f.column), f.line, f.column));
                ps.addAll(f.params);
                String plano = c.name + "." + f.name.name;   /* el punto SI vale aqui:
                                                              * es el simbolo del .mod */
                Ast.FuncDef fd = new Ast.FuncDef(f.isPublic, f.isFinal, f.isIntrinsic,
                        f.isNative, new Ast.DeclName(null, plano, f.line, f.column),
                        ps, f.returnType, f.body, f.line, f.column);
                metodosAplanados.add(plano);
                nativeFuncs.add(fd);
                nativeFuncNames.add(plano);
                nativeFuncDefs.put(plano, fd);
                allFuncDefs.put(plano, fd);
            }
        }

        if (nativeFuncs.isEmpty()) return "";

        /* V5/H4 - el pack del que salen las `native` sin cuerpo. */
        if (module.packNativoMarca != null) {
            String m = module.packNativoMarca;   /* el parser ya valido 4 chars */
            packMarca = ((m.charAt(0) & 0xFF) << 24) | ((m.charAt(1) & 0xFF) << 16)
                      | ((m.charAt(2) & 0xFF) <<  8) |  (m.charAt(3) & 0xFF);
            packVersion = module.packNativoVersion;
        }

        /* #580 (1) — las natives en un ciclo, por una emisión en seco del mismo módulo (ver `recursivas`). Si la
         * de verdad no se puede traducir, ésta tampoco: lanza lo mismo, desde aquí. */
        if (!soloGrafo) {
            AotCEmitter seco = new AotCEmitter(moduleName);
            seco.semInfo = semInfo;
            seco.soloGrafo = true;
            seco.emitModule(module);
            recursivas = enCiclo(seco.llamadasDirectas);
        }

        // #172 — Pre-pass: los offsets CS-relativos de las vars/consts de
        // módulo que una native puede tocar. #588: los da el compilador
        // (MivmEmitter.offsetsDeGlobales), no una convención propia.
        precomputeModuleVarOffsets(module);

        // Header del archivo.
        emitHeader();

        // Una función por cada native; después, sus thunks.
        List<Ast.FuncDef> emitidas = new ArrayList<>();
        /* V7/H5 P2b.3 — el C de cada native, tal como sale: con él se decide cuáles son DIRECTAS. */
        Map<String, String> cuerpos = new LinkedHashMap<>();
        for (Ast.FuncDef f : nativeFuncs) {
            /* #349 — LA FUNCIÓN Y LA LÍNEA, o el aviso no sirve de nada. Los
             * throw de aquí abajo nacen en sitios que no saben en qué función
             * están (cType, readHelper...), así que el mensaje llegaba suelto:
             * "el tipo 'double' ocupa 8 bytes". En un módulo con varias native
             * eso obliga a buscarlas una a una. Se enriquece aquí, que es el
             * único punto por el que pasan todas. */
            try {
                /* V5/H4 - una `native` EXTERNA no tiene cuerpo que traducir: lo
                 * pone el pack. Lo que se emite en su lugar es el PUENTE. El
                 * thunk es el mismo de siempre: desde la pila BP, llamar a un
                 * pack o a codigo traducido se ve exactamente igual. */
                w.flush();
                int desde = out.getBuffer().length();
                if (f.isPackExtern) emitPackExtern(f); else emitFunction(f);
                w.flush();
                cuerpos.put(f.name.name, out.getBuffer().substring(desde));
                emitidas.add(f);
            } catch (UnsupportedAotException ex) {
                /* #519 (V7; decidido por Eduardo el 24-sep) — un METODO native que no se
                 * puede traducir es el MISMO error que una funcion native de modulo:
                 * `native` es una promesa, y si no se puede cumplir se dice al compilar, con
                 * el nombre y la linea, y no hay .mod (docs/AOT_LIMITES.md: «todo recorte es
                 * un error de COMPILACION»). Desde V6/N1.1 se degradaba a un aviso «corre
                 * interpretado» para no tumbar los modulos que ya existian —y el aviso solo
                 * salia si el modulo tenia ademas una native de modulo: sin ella, silencio—.
                 * En el arbol no quedaba ninguno sin traducir (NatV7, el unico, se traduce). */
                String que = metodosAplanados.contains(f.name.name)
                        ? "en el metodo native '" : "en la funcion native '";
                throw new UnsupportedAotException(
                    que + f.name.name + "' (linea " + f.line + "): " + ex.getMessage());
            }
        }

        /* V7/H5 P2b.3 — los thunks, ya sabiendo qué natives son DIRECTAS (ver nativesDirectas). */
        Set<String> directas = nativesDirectas(cuerpos);
        for (Ast.FuncDef f : emitidas) {
            try {
                emitThunk(f, directas.contains(f.name.name));
            } catch (UnsupportedAotException ex) {
                String que = metodosAplanados.contains(f.name.name)
                        ? "en el metodo native '" : "en la funcion native '";
                throw new UnsupportedAotException(
                    que + f.name.name + "' (linea " + f.line + "): " + ex.getMessage());
            }
        }

        // Función de registro — solo si NO estamos en modo .mdn.
        // En .mdn las relocs de bpvm_aot_register_by_name + string
        // literal contaminan el code section. El loader del firmware
        // registra automáticamente a partir del symtab.
        if (!omitRegisterFunc) {
            emitRegisterFunc(emitidas, directas);
        }
        return out.toString();
    }

    /** V7/H5 P2b.3 — los helpers por los que una native puede CEDER el hilo: la línea del bucle, un builtin por el
     *  despachador (puede bloquear) y el puente a BP (la función BP puede bloquear). Todo lo demás de la tabla
     *  —cadenas, arrays, reservas, `print`, lanzar— vuelve sin ceder.
     *
     *  Y LLAMAR A UN PACK TAMPOCO CEDE (Eduardo, 27-sep: «OK, adelante»): el puente de una native `from pack` usa
     *  `pack_sym` sólo para BUSCAR la función, y luego salta a código C del pack, que sólo ve la BIOS (libc, ficheros,
     *  la HAL, `log`, `env`, `sleep_ms`…): ninguna de sus ranuras ejecuta BP, así que dentro del pack no hay ningún
     *  punto de suspensión. Estaba en la lista por prudencia y mandaba a SQLite a un ejecutor sin servirle de nada
     *  (el testigo en cada llamada y su motor, 5,5 KB, en la pila pequeña del ejecutor). ⚠️ Si algún día la BIOS da a
     *  un pack una forma de llamar a BP, "->pack_sym(" tiene que VOLVER a esta lista. */
    private static final String[] CEDEN = {
        "->punto_control(", "->call_builtin(", "->call_bp_i32(", "->call_method_i32("
    };

    /** V7/H5 P2b.3 — las natives DIRECTAS: las que no pueden ceder el hilo en ningún caso, porque ni su C ni el de
     *  ninguna native del módulo a la que llamen —hasta el fondo, recursión incluida— llama a un helper de CEDEN.
     *  Una directa no necesita ejecutor: corre en la pila del worker, como todas antes de P2b, y se ahorra el testigo
     *  (dos cambios de tarea por llamada). Se mira el C que SALE, no el `.bp`: si el emisor gana otro sitio que cede,
     *  no se escapa. Una llamada que el emisor no resuelve a una native del módulo (BP, otro módulo, un pack) ya pasa
     *  por uno de esos helpers. Pregunta de Eduardo (27-sep): «la rutina en sí puede ser pequeña pero puede llamar a
     *  una mucho más grande con bucles» — por eso es transitivo. */
    private Set<String> nativesDirectas(Map<String, String> cuerpos) {
        Map<String, Boolean> cede = new HashMap<>();
        Map<String, List<String>> llama = new HashMap<>();
        for (Map.Entry<String, String> e : cuerpos.entrySet()) {
            String c = e.getValue();
            boolean si = false;
            for (String h : CEDEN) if (c.contains(h)) { si = true; break; }
            cede.put(e.getKey(), si);
            List<String> ls = new ArrayList<>();
            for (String g : cuerpos.keySet())
                if (!g.equals(e.getKey()) && c.contains("aot_" + moduleName + "_" + cId(g) + "("))
                    ls.add(g);
            llama.put(e.getKey(), ls);
        }
        boolean cambia = true;
        while (cambia) {
            cambia = false;
            for (String f : cuerpos.keySet()) {
                if (cede.get(f)) continue;
                for (String g : llama.get(f))
                    if (cede.get(g)) { cede.put(f, true); cambia = true; break; }
            }
        }
        Set<String> dir = new HashSet<>();
        for (Map.Entry<String, Boolean> e : cede.entrySet()) if (!e.getValue()) dir.add(e.getKey());
        return dir;
    }

    /** #580 (1) — las natives de un CICLO del grafo de llamadas de C entre natives del módulo: las que pueden volver
     *  a entrar en sí mismas (también directamente: `fib` → `fib`). */
    private static Set<String> enCiclo(Map<String, Set<String>> grafo) {
        Set<String> r = new HashSet<>();
        for (Map.Entry<String, Set<String>> e : grafo.entrySet()) {
            java.util.ArrayDeque<String> pila = new java.util.ArrayDeque<>(e.getValue());
            Set<String> visto = new HashSet<>();
            while (!pila.isEmpty()) {
                String g = pila.pop();
                if (g.equals(e.getKey())) { r.add(g); break; }
                Set<String> sig = grafo.get(g);
                if (visto.add(g) && sig != null) pila.addAll(sig);
            }
        }
        return r;
    }

    // ==================== Header ====================

    private void emitHeader() {
        w.println("/*");
        w.println(" * aot_" + moduleName + ".c — AUTOGENERADO por AotCEmitter (H3 #157).");
        w.println(" * NO EDITAR A MANO. Regenerar compilando " + moduleName + ".bp con --aot.");
        w.println(" *");
        w.println(" * Funciones BP marcadas con `function native ...` traducidas a C.");
        w.println(" * El bytecode .mod se sigue generando normalmente; el runtime");
        w.println(" * decide qué versión usar via aot_registry tras link.");
        w.println(" */");
        w.println();
        w.println("#include \"aot_registry.h\"");
        w.println("#include \"bpvm.h\"");
        w.println("#include \"bpvm_internal.h\"");
        w.println("#include \"bpvm_aot_helpers.h\"   /* H3 #158 — helpers indirect */");
        w.println();
        /* V7/H2 (#523) — el puente native→BP pide al intérprete por NOMBRE; en MOD9 el
         * nombre lleva la firma (`Lib.f(i)i`), así que con otra versión de la dep puede no
         * existir. `find_function` devuelve 0 y `call_bp_i32` sólo sabía decir «función
         * destino no encontrada»: este helper la nombra, y nombra a quién la pedía. */
        w.println("/* V7/H2 (#523) — busca un símbolo BP por nombre; si no está, lo DICE (no un 0 mudo). */");
        w.println("static inline uint32_t aot_" + moduleName + "_busca(struct bpvm* vm, const char* nombre, const char* aviso) {");
        w.println("    uint32_t f = vm->aot_helpers->find_function(vm, nombre);");
        w.println("    if (f == 0) vm->aot_helpers->throw_runtime(vm, aviso);   /* longjmp: no vuelve */");
        w.println("    return f;");
        w.println("}");
        /* V7/H5 (27-sep) — las conversiones ESTRECHAS (`byte(x)`…): el rango de OP_I32_TO_U8 y compañía, AQUÍ y no en
         * un helper de la VM. La opción A de Eduardo: «nos permite más flexibilidad, si se quiere cambiar algo
         * solamente hace falta modificar el compilador». Un helper nuevo habría subido la ABI del .mdn (regenerar los
         * de los packs y regrabar todas las placas). El precio: el texto está en la VM y aquí; test-aotestrecha compara
         * la native con la interpretada en los bordes de cada una. Fuera de rango arma el MISMO mensaje que el
         * intérprete (snprintf "%s%d") con los helpers que ya hay, y lo lanza como un RuntimeError atrapable. */
        w.println("/* V7/H5 — las conversiones estrechas (`byte(x)`…): el rango de OP_I32_TO_U8 y compañía, con su texto. */");
        w.println("static inline int32_t aot_" + moduleName + "_estrecha(struct bpvm* vm, int32_t v, int32_t min, int32_t max,");
        w.println("                                        const char* texto, int32_t largo) {");
        w.println("    if (v < min || v > max) {");
        w.println("        const struct aot_helpers_v2* H = vm->aot_helpers;");
        w.println("        uint32_t a = H->string_from_cstr(vm, texto, largo);");
        w.println("        uint32_t b = H->int_to_string(vm, v);");
        w.println("        H->throw_str(vm, H->string_concat(vm, a, b));   /* longjmp: no vuelve */");
        w.println("    }");
        w.println("    return v;");
        w.println("}");
        if (Main.FORMATO == 9) {
            /* V7/H2 (#533) — el AOT en MOD9, fase 2: la ranura de vtable de un método de una
             * clase EXTERNA o ABIERTA no se hornea (la fija el cargador al enlazar): el `.mdn`
             * la PIDE por clave en ejecución, como los opcodes `_REF` piden la suya a la
             * ext-table. `slot_por_clave` resuelve el descriptor de la clase ESTÁTICA por su
             * data-symbol y busca la clave por la cadena real (saltando privados ajenos), y si
             * no está, se DICE con nombre — el E3 del enlace, en nativo. */
            w.println("/* V7/H2 (#533) — la ranura de un método de clase externa/abierta, por clave y en ejecución; si no está, lo DICE. */");
            w.println("static inline int32_t aot_" + moduleName + "_ranura(struct bpvm* vm, const char* clase, const char* clave, const char* aviso) {");
            w.println("    int32_t s = vm->aot_helpers->slot_por_clave(vm, clase, clave, \"" + moduloLocal + "\");");
            w.println("    if (s < 0) vm->aot_helpers->throw_runtime(vm, aviso);   /* longjmp: no vuelve */");
            w.println("    return s;");
            w.println("}");
        }
        w.println();
        w.println("/* Forward decls de las funciones AOT de este módulo. */");
        for (Ast.FuncDef f : nativeFuncDefs.values()) {
            /* #349 — mismo motivo que en el bucle de emisión: un tipo de la
             * FIRMA revienta aquí, antes de llegar allí, así que sin este catch
             * el caso más común (un `double` como parámetro o retorno) era
             * justo el que salía sin nombre de función. */
            try {
                w.print("static " + cType(f.returnType) + " aot_"
                    + moduleName + "_" + cId(f.name.name) + "(struct bpvm* vm");
                for (Ast.Param p : f.params) {
                    w.print(", " + cType(p.type) + " " + cVar(p.name));
                }
                if (recursivas.contains(f.name.name)) w.print(", int32_t* " + CUENTA_R);   /* #580 (1) */
                w.println(");");
            } catch (UnsupportedAotException ex) {
                throw new UnsupportedAotException(
                    "en la firma de la funcion native '" + f.name.name
                    + "' (linea " + f.line + "): " + ex.getMessage());
            }
        }
        w.println();

        /* #172 — Si el módulo declara vars/consts a nivel-módulo, cada función
         * AOT resuelve el CS del módulo al empezar (una vez por llamada, no por
         * acceso). CS es runtime (depende de en qué orden cargue el loader los
         * módulos). Las accesos a globales se compilan como:
         *     read_i32_be(mem + cs + OFFSET_LITERAL)
         *
         * [V7/H5 #562, 25-sep] SIN CACHÉ `static`. La tuvo (`s_aot_<Mod>_cs`), y un
         * `.mdn` sólo se lleva `.text`: esa variable caía en `.sbss`/`.bss`, MdnPack
         * lo rechazaba —bien— y NINGÚN módulo con globales ha tenido nunca código
         * nativo en placa: «AOT omitido», interpretado. Sólo lo ejecutaban las
         * pruebas del PC, que enlazan el C en su binario. Y la caché, de haber
         * podido existir, habría guardado el CS del programa ANTERIOR en el
         * siguiente RUN. `find_module_cs` recorre los módulos cargados (unos
         * pocos) con un strcmp: se paga una vez por llamada a la native. */
        if (!moduleVarOffsets.isEmpty()) {
            w.println("/* #172 — el CS del módulo, resuelto al entrar en cada función (V7/H5 #562:");
            w.println(" * sin caché `static`, que en un .mdn no puede existir: sólo lleva .text). */");
            w.println("static inline uint32_t aot_" + moduleName + "_cs(struct bpvm* vm) {");
            w.println("    uint32_t cs = vm->aot_helpers->find_module_cs(vm, \""
                    + moduleName + "\");");
            w.println("    if (!cs) {");
            w.println("        vm->aot_helpers->throw_runtime(vm,");
            w.println("            \"AOT: módulo '" + moduleName + "' no encontrado\");");
            w.println("        return 0;");
            w.println("    }");
            w.println("    return cs;");
            w.println("}");
            w.println();
            w.println("/* Offsets CS-relativos de los module-globals (compile-time): */");
            for (java.util.Map.Entry<String, Integer> e : moduleVarOffsets.entrySet()) {
                w.println("/*   " + moduleName + "." + e.getKey() + " @ CS"
                        + (e.getValue() < 0 ? "" : "+") + e.getValue() + " */");
            }
            w.println();
        }
    }

    // ==================== Función AOT ====================

    private void emitFunction(Ast.FuncDef f) {
        currentFuncName = f.name.name;   /* #211 — para mensajes de aviso */
        retActual = tipoDeclarado(f.returnType);
        String fname = "aot_" + moduleName + "_" + cId(f.name.name);
        String cRet  = cType(f.returnType);
        w.print("static " + cRet + " " + fname + "(struct bpvm* vm");
        for (Ast.Param p : f.params) {
            w.print(", " + cType(p.type) + " " + cVar(p.name));
        }
        boolean recursiva = recursivas.contains(f.name.name);   /* #580 (1) */
        if (recursiva) w.print(", int32_t* " + CUENTA_R);
        w.println(") {");
        w.println("    (void) vm;   /* puede no usarse si la función no toca");
        w.println("                  *  globals/arrays/builtins. */");
        /* V7/H5 P2b.1.d — el contador de la LÍNEA DEL BUCLE (emitLineaDelBucle), uno por función: cuenta las
         * vueltas de cualquiera de sus bucles. En un registro: P2b.0 midió +2 ciclos por vuelta en la Pico,
         * frente a +7 de mirar un indicador en memoria en cada una. */
        w.println("    int32_t " + CUENTA + " = " + CUENTA_VUELTAS + ";   /* P2b.1.d: la línea del bucle */");
        w.println("    (void) " + CUENTA + ";");

        /* #172 — Si el módulo tiene globals declarados, exponemos `mem`
         * y `cs` al cuerpo de la función. `cs` se resuelve UNA VEZ por
         * thunk-invocation (no por acceso) vía la helper-static
         * aot_<Mod>_cs(vm). Para funciones que no tocan globals el
         * compilador C las elimina como dead-store con -O2. */
        if (!moduleVarOffsets.isEmpty()) {
            w.println("    uint8_t* mem = vm->memory;");
            w.println("    uint32_t cs = aot_" + moduleName + "_cs(vm);");
            w.println("    (void) mem; (void) cs;");
        }

        /* #171 — Silenciar warnings de params no usados (frecuente en
         * funciones de pass-through o cuando el AOT no usa todos los
         * args). El compilador C los elimina en -O2 igualmente. */
        for (Ast.Param p : f.params) {
            w.println("    (void) " + cVar(p.name) + ";");
        }

        /* #580 (1) — LA ENTRADA de una native en un ciclo cuenta como una vuelta de un bucle (ver `recursivas`). */
        if (recursiva)
            w.println("    if (__builtin_expect(--*" + CUENTA_R + " == 0, 0)) { *" + CUENTA_R + " = " + CUENTA_VUELTAS
                      + "; vm->aot_helpers->punto_control(vm); }   /* #580: la entrada de una recursión */");

        /* #172 — Inicializar ámbito local: params + reset entre funcs. */
        localNames.clear();
        for (Ast.Param p : f.params) localNames.add(p.name);

        indentLevel = 1;
        for (Ast.IStmt s : f.body) {
            emitStmt(s);
        }
        // Fall-through default: para retornos integer 0, float 0.0f.
        // Si returnType es null (void), no se hace.
        if (f.returnType != null && !endsWithReturn(f.body)) {
            indent();
            String z = "float".equals(cRet) ? "0.0f" : "0";
            w.println("return " + z + ";   /* fall-through default */");
        }
        w.println("}");
        w.println();
    }

    // ============ V5/H4: puente a un pack nativo ============

    /**
     * Emite el cuerpo de una `native` EXTERNA: la declarada sin cuerpo en un
     * modulo con `import v<N> from pack "MARCA"`. Hace dos cosas y ya - resolver
     * el simbolo por nombre y adaptar los tipos.
     *
     * NI UN LITERAL DE CADENA AQUI DENTRO. Medido con el toolchain real: un
     * literal de C se va a `.rodata.str1.1` y en `.text` queda un `R_ARM_REL32`;
     * MdnPack empaqueta `.text` y NADA MAS, sin aplicar relocs. En placa ese
     * puntero no apuntaria a ningun sitio. Por eso el nombre del simbolo se
     * materializa BYTE A BYTE en la pila (medido: cero relocs de -O0 a -Os), y
     * el texto de los errores lo redacta la VM (helper `pack_fallo`).
     *
     * Y ojo con la FORMA: `char n[] = {ESE_ARRAY}` no vale - gcc reconoce el
     * inicializador y lo vuelve a sacar a `.rodata`. Asignacion a asignacion si.
     */
    private void emitPackExtern(Ast.FuncDef f) {
        currentFuncName = f.name.name;
        String sim   = cId(f.name.name);
        String fname = "aot_" + moduleName + "_" + sim;
        String marca = String.format("0x%08Xu", packMarca);
        String ver   = packVersion + "u";
        int n = f.params.size();

        w.println("/* " + sim + " - EXTERNA: la trae el pack " + Q + marcaTexto() + Q
                  + " v" + packVersion + ".");
        w.println(" * No hay cuerpo en el .bp: lo pone el pack. Esto es el puente. */");
        w.print("static " + cType(f.returnType) + " " + fname + "(struct bpvm* vm");
        for (Ast.Param p : f.params) w.print(", " + cType(p.type) + " " + cVar(p.name));
        w.println(") {");
        w.println("    const struct aot_helpers_v2* H = vm->aot_helpers;");

        /* La firma REAL, la que el pack expone. */
        StringBuilder sig = new StringBuilder();
        for (int i = 0; i < n; i++) {
            if (i > 0) sig.append(", ");
            sig.append(cTypePack(f.params.get(i).type, sim));
        }
        if (n == 0) sig.append("void");
        w.println("    typedef " + cTypePack(f.returnType, sim) + " (*fn_t)(" + sig + ");");
        w.println("    fn_t fn;");

        /* Todas las DECLARACIONES juntas y antes de cualquier sentencia: el .c
         * generado se compila con toolchains distintos y no todos son C99. */
        w.println("    char nm[" + (sim.length() + 1) + "];");
        for (int i = 0; i < n; i++) {
            Ast.TypeRef t = f.params.get(i).type;
            if (esString(t))             w.println("    char b" + i + "[BPVM_AOT_PACK_STR];");
            else if (salida8(t) != null) w.println("    " + salida8(t) + " o" + i + " = 0;");
        }
        /* El nombre, byte a byte - ver la nota de arriba. */
        StringBuilder ln = new StringBuilder("   ");
        for (int i = 0; i < sim.length(); i++) {
            ln.append(" nm[").append(i).append("]=").append(Q).append(sim.charAt(i)).append(Q).append(";");
            if (ln.length() > 66) { w.println(ln); ln = new StringBuilder("   "); }
        }
        ln.append(" nm[").append(sim.length()).append("]=0;");
        w.println(ln);

        w.println("    fn = (fn_t) H->pack_sym(" + marca + ", " + ver + ", nm);");
        w.println("    if (fn == 0) H->pack_fallo(vm, " + marca + ", " + ver
                  + ", nm, 1);   /* no retorna */");
        for (int i = 0; i < n; i++) {
            if (!esString(f.params.get(i).type)) continue;
            String pn = cVar(f.params.get(i).name);
            w.println("    if (H->string_to_cstr(vm, (uint32_t) " + pn + ", b" + i
                      + ", (int32_t) sizeof b" + i + ") < 0)");
            w.println("        H->pack_fallo(vm, " + marca + ", " + ver
                      + ", nm, 2);   /* no retorna */");
        }

        StringBuilder args = new StringBuilder();
        for (int i = 0; i < n; i++) {
            Ast.TypeRef t = f.params.get(i).type;
            if (i > 0) args.append(", ");
            if (esString(t))             args.append("b").append(i);
            else if (salida8(t) != null) args.append("&o").append(i);
            else                         args.append(cVar(f.params.get(i).name));
        }
        boolean devuelveCadena = esString(f.returnType);
        boolean hayCajas = false;
        for (int i = 0; i < n; i++) if (salida8(f.params.get(i).type) != null) hayCajas = true;

        if (f.returnType == null) {
            w.println("    fn(" + args + ");");
        } else if (devuelveCadena) {
            /* El pack devuelve `const char*`; BP quiere una cadena del heap. La
             * longitud se cuenta a mano: `strlen` seria un simbolo externo, y en
             * el .mdn eso es otra reloc que no resuelve nadie. */
            w.println("    { const char* r = fn(" + args + "); int32_t k = 0;");
            w.println("      if (r == 0) return 0;");
            /* ACOTADO a proposito, y por dos motivos que coinciden:
             *  1. gcc reconoce `while (r[k]) k++;` y lo sustituye por una
             *     llamada a strlen -> simbolo externo -> reloc -> el .mdn no la
             *     resuelve. MEDIDO: era el UNICO reloc que quedaba.
             *  2. si el pack devuelve un buffer sin terminar, un bucle sin tope
             *     se sale de la memoria. Con tope, se corta. */
            w.println("      while (k < BPVM_AOT_PACK_STR && r[k]) k++;");
            w.println("      return (int32_t) H->string_from_cstr(vm, r, k); }");
        } else {
            w.println("    { " + cTypePack(f.returnType, sim) + " r = fn(" + args + ");");
        }

        /* Las cajas se vuelcan al elemento 0 de su array. CONVENIO: un
         * `long[]`/`double[]` en una externa es una caja de UN valor - la vuelta
         * que hay para 8 bytes mientras el AOT no los marshalle (#381).
         * `array_store_*` comprueba el indice, asi que un array vacio lo DICE. */
        for (int i = 0; i < n; i++) {
            String t8 = salida8(f.params.get(i).type);
            if (t8 == null) continue;
            String hp = t8.equals("int64_t") ? "array_store_i64" : "array_store_f64";
            w.println("    H->" + hp + "(vm, (uint32_t) " + cVar(f.params.get(i).name)
                      + ", 0, o" + i + ");   /* caja de salida */");
        }
        if (f.returnType != null && !devuelveCadena) w.println("    return r; }");
        else if (hayCajas && f.returnType == null)   { /* nada: no se abrio bloque */ }
        w.println("}");
        w.println();
    }

    /** Los 4 caracteres de la marca, para los comentarios del .c generado. */
    private String marcaTexto() {
        return "" + (char)((packMarca >> 24) & 0xFF) + (char)((packMarca >> 16) & 0xFF)
                  + (char)((packMarca >>  8) & 0xFF) + (char)( packMarca        & 0xFF);
    }

    private boolean esString(Ast.TypeRef t) {
        return (t instanceof Ast.SimpleTypeRef)
            && "string".equals(((Ast.SimpleTypeRef) t).name);
    }

    /** `long[]` / `double[]` como CAJA DE SALIDA - el pack recibe un puntero. */
    private String salida8(Ast.TypeRef t) {
        if (!(t instanceof Ast.ArrayTypeRef)) return null;
        Ast.TypeRef e = ((Ast.ArrayTypeRef) t).element;
        if (!(e instanceof Ast.SimpleTypeRef)) return null;
        String n = ((Ast.SimpleTypeRef) e).name;
        if ("long".equals(n))   return "int64_t";
        if ("double".equals(n)) return "double";
        return null;
    }

    /**
     * El tipo tal como lo ve EL PACK, que no es el que ve BP: una cadena BP es un
     * handle al heap y el pack quiere `const char*`; un `long[]` de salida es un
     * handle y el pack quiere `int64_t*`.
     *
     * Lo que NO cruza se rechaza AQUI y CON NOMBRE. Un objeto BP no puede viajar
     * a un pack: el pack no conoce el GC ni debe, y darle un puntero al heap es
     * exactamente el use-after-free que costo media V4.
     */
    private String cTypePack(Ast.TypeRef t, String sim) {
        if (t == null) return "void";
        if (esString(t)) return "const char*";
        String s8 = salida8(t);
        if (s8 != null) return s8 + "*";
        if (t instanceof Ast.SimpleTypeRef) {
            String n = ((Ast.SimpleTypeRef) t).name;
            if ("integer".equals(n) || "int".equals(n))  return "int32_t";
            if ("boolean".equals(n) || "bool".equals(n)) return "int32_t";
            if ("float".equals(n))                       return "float";
            if ("long".equals(n) || "double".equals(n)) {
                throw new UnsupportedAotException(
                    Q + sim + Q + " usa " + Q + n + Q + " (8 bytes) y el puente a un "
                    + "pack solo pasa valores de 4. Para traerse un valor de 8 bytes "
                    + "se usa un ARRAY DE SALIDA: declara el parametro como " + Q + n
                    + "[]" + Q + " y el pack escribe en su elemento 0. (El rodeo se "
                    + "quitara cuando el AOT marshalle 8 bytes - tarea #381.)");
            }
            throw new UnsupportedAotException(
                Q + sim + Q + " usa el tipo " + Q + n + Q + ", que no puede cruzar a "
                + "un pack. Un pack no conoce el GC de BP, asi que solo cruzan "
                + "VALORES: integer, boolean, float, string, y long[]/double[] como "
                + "caja de salida. Un objeto se reparte como HANDLE (un integer que "
                + "el pack valida).");
        }
        throw new UnsupportedAotException(
            Q + sim + Q + ": ese tipo de array no cruza a un pack. Solo long[] y "
            + "double[], y solo como caja de salida de un valor de 8 bytes.");
    }

    private boolean endsWithReturn(List<Ast.IStmt> body) {
        if (body.isEmpty()) return false;
        Ast.IStmt last = body.get(body.size() - 1);
        return last instanceof Ast.ReturnStmt;
    }

    // ==================== Thunk ====================

    private void emitThunk(Ast.FuncDef f, boolean directa) {
        String fname = "aot_" + moduleName + "_" + cId(f.name.name);
        String tname = "thunk_" + moduleName + "_" + cId(f.name.name);
        /* V7/H5 P2b.3 — una native DIRECTA (nativesDirectas) lo dice en el NOMBRE de su thunk, `thunkd_`: es por
         * donde MdnPack reconoce los thunks, y así la marca llega al `.mdn` sin otro canal. */
        if (directa)
            w.println("/* P2b.3: DIRECTA — sin puntos de cesión: corre en la pila del worker, sin ejecutor. */");
        // En modo .mdn el thunk debe ser visible al linker — MdnPack lo
        // busca por nombre en el symtab del .o. Además marcamos `used`
        // para que -Os con -ffunction-sections no lo deade-code-stripe
        // por no tener llamadores en este TU. En modo "linked-in" se
        // mantiene como antes: static, registrado via aot_<Mod>_register.
        if (omitRegisterFunc) {
            /* [24-ago-2026] EL NOMBRE DEL SIMBOLO ES EL DEL `.mod`, y por eso se
             * pone a mano en vez de dejar que se deduzca del identificador de C.
             *
             * `MdnPack` reconstruia el nombre BP partiendo el identificador por
             * el prefijo `thunk_<Mod>_`. Para una funcion de modulo eso acierta;
             * para un METODO no, porque `cId()` ha convertido el punto en guion
             * bajo y de `Caja_doble` no se puede recuperar `Caja.doble` — un
             * identificador BP tambien puede llevar guiones bajos.
             *
             * Consecuencia MUDA: la placa no encontraba el simbolo
             * (`MDN: skip 'NatV7.Caja_doble' rc=-2`), registraba 3 de 4 thunks y
             * el metodo corria interpretado. El programa daba el resultado
             * correcto — que es justo lo que hace invisible este fallo.
             *
             * Con el nombre puesto explicitamente no hay nada que deducir. */
            w.println("void " + tname + "(struct bpvm*, uint32_t*, uint32_t*)");
            w.println("    __asm__(\"" + (directa ? "thunkd_" : "thunk_") + moduleName + "_" + f.name.name + "\");");
            w.println("__attribute__((used))");
            w.println("void " + tname + "(struct bpvm* vm,");
        } else {
            w.println("static void " + tname + "(struct bpvm* vm,");
        }
        w.println("                              uint32_t* sp_p,");
        w.println("                              uint32_t* bp_p) {");
        w.println("    (void) bp_p;");
        w.println("    /* H3 #158 — helpers accedidos indirect via vm.");
        w.println("     * No referencia símbolos del runtime por nombre → el");
        w.println("     * .o resultante con -fpic es 100% relocatable. */");
        w.println("    const struct aot_helpers_v2* H = vm->aot_helpers;");
        w.println("    uint8_t* mem = vm->memory;");
        w.println("    uint32_t sp = *sp_p;");
        // Pop args en orden inverso (último pusheado, primero popeado).
        // #302 paso 2 — un arg REFERENCIA ocupa 8 bytes en la pila BP (handle):
        // se lee con read_ref (devuelve el handle empaquetado, que el cuerpo AOT
        // maneja como i32) y avanza sp -= 8. Los no-ref siguen a 4 bytes.
        int n = f.params.size();
        for (int i = n - 1; i >= 0; i--) {
            Ast.Param p = f.params.get(i);
            if (paramIsRef(f, i)) {
                w.println("    " + cType(p.type) + " a" + i
                    + " = (int32_t) H->read_ref(mem + sp - 8); sp -= 8;  /* ref: 8B */");
            } else {
                /* #381 — un `long` ocupa 8 bytes en la pila BP, igual que una
                 * referencia. El paso del sp lo decide el TIPO, no una constante:
                 * antes era 4 en todas partes y ahí es donde se colaba el error
                 * silencioso si se añadía un tipo ancho sin tocar el avance. */
                int w8 = slotBytes(p.type);
                w.println("    " + cType(p.type) + " a" + i + " = H->"
                    + readHelper(p.type) + "(mem + sp - " + w8 + "); sp -= " + w8 + ";"
                    + (w8 == 8 ? ("  /* " + anchoQuien(p.type) + ": 8B */") : ""));
            }
        }
        // C call con args en orden original a0, a1, a2...
        // Si la función es void, no asignamos a una variable y no
        // escribimos resultado al stack — los args ya están consumidos.
        boolean isVoid = (f.returnType == null);
        String cRet = cType(f.returnType);
        StringBuilder call = new StringBuilder();
        call.append("    ");
        if (!isVoid) call.append(cRet).append(" r = ");
        call.append(fname).append("(vm");
        for (int i = 0; i < n; i++) call.append(", a").append(i);
        if (recursivas.contains(f.name.name))   /* #580 (1): el contador de las entradas nace en el thunk */
            call.append(", &(int32_t){ ").append(CUENTA_VUELTAS).append(" }");
        call.append(");");
        w.println(call);
        if (!isVoid && returnIsRef(f)) {
            /* #302 paso 2 — retorno REFERENCIA: escribir el handle de 8 bytes con
             * la gen VIVA (write_ref regen internamente) y avanzar sp += 8. */
            w.println("    H->write_ref(vm, mem + sp, (uint32_t) r); sp += 8;  /* ref: 8B */");
        } else if (!isVoid) {
            int w8 = slotBytes(f.returnType);
            w.println("    H->" + writeHelper(f.returnType)
                + "(mem + sp, r); sp += " + w8 + ";"
                + (w8 == 8 ? ("  /* " + anchoQuien(f.returnType) + ": 8B */") : ""));
        } else {
            /* #177 FIX — Las funciones BP normal con OP_RET siempre push
             * un ret_val (incluso si son void). El compilador BP emite
             * OP_POP tras un statement-call para discardarlo. Para que el
             * thunk AOT void sea balance-equivalente al call BP, debe
             * push un dummy 0 que el OP_POP siguiente popee. Si no, sp
             * decrece en 4 bytes por cada call AOT void → frame corrupt
             * tras varias calls. */
            w.println("    H->write_i32_be(mem + sp, 0); sp += 4;  /* dummy ret para OP_POP del caller */");
        }
        w.println("    *sp_p = sp;");
        w.println("}");
        w.println();
    }

    // ==================== Register ====================

    private void emitRegisterFunc(List<Ast.FuncDef> nativeFuncs, Set<String> directas) {
        w.println("/* Registra todas las funciones AOT de este módulo en el AOT");
        w.println(" * registry. Llamar tras link, antes de bpvm_run. Tolerante a");
        w.println(" * símbolos ausentes (skip silente si el .mod no está cargado). */");
        w.println("void aot_" + moduleName + "_register(struct bpvm* vm) {");
        for (Ast.FuncDef f : nativeFuncs) {
            String qualified = moduleName + "." + f.name.name;
            String tname = "thunk_" + moduleName + "_" + cId(f.name.name);
            if (directas.contains(f.name.name))   /* V7/H5 P2b.3 */
                w.println("    bpvm_aot_register_by_name_flags(vm, \"" + qualified + "\", " + tname
                          + ", BPVM_AOT_DIRECTA);");
            else
                w.println("    bpvm_aot_register_by_name(vm, \"" + qualified + "\", " + tname + ");");
        }
        w.println("}");
        w.println();
    }

    // ==================== Statements ====================

    private void emitStmt(Ast.IStmt s) {
        if (s instanceof Ast.ReturnStmt) {
            Ast.ReturnStmt r = (Ast.ReturnStmt) s;
            indent();
            if (r.value != null) {
                w.print("return ");
                emitComo(r.value, retActual);   /* #559 — `return i` en una native float */
                w.println(";");
            } else {
                w.println("return;");
            }
            return;
        }
        if (s instanceof Ast.IfStmt) {
            emitIfStmt((Ast.IfStmt) s);
            return;
        }
        if (s instanceof Ast.VarDecl) {
            Ast.VarDecl v = (Ast.VarDecl) s;
            String cVarType = cType(v.type);
            for (Ast.DeclName dn : v.names) {
                indent();
                w.print(cVarType + " " + cVar(dn.name));
                if (v.init != null) {
                    w.print(" = ");
                    emitComo(v.init, tipoDeclarado(v.type));   /* #559 — `var x: float := i` */
                }
                w.println(";");
                localNames.add(dn.name);   /* #172 — registrar local */
            }
            return;
        }
        if (s instanceof Ast.AssignStmt) {
            Ast.AssignStmt a = (Ast.AssignStmt) s;
            /* Target IdentifierExpr: assignment a variable local/param,
             * o bien a global de módulo (#172) si no está en localNames. */
            if (a.target instanceof Ast.IdentifierExpr) {
                String tname = ((Ast.IdentifierExpr) a.target).name;
                indent();
                BpType tLocal = tipoDe(a.target);
                if (localNames.contains(tname)
                        && a.op != Ast.AssignOpKind.ASSIGN && (esFloatT(tLocal) || esDoubleT(tLocal))) {
                    /* #559 — `x += v` con x float/double: la operación del intérprete, no
                     * el `+=` de C (que sería libgcc sin FPU, y en double en toda placa). */
                    w.print(cVar(tname) + " = ");
                    emitCompuestoFlotante(a.op, tLocal, () -> w.print(cVar(tname)), a.value);
                    w.println(";");
                } else if (localNames.contains(tname)) {
                    /* Local — emisión directa estilo C. */
                    w.print(cVar(tname));
                    switch (a.op) {
                        case ASSIGN:       w.print(" = ");  break;
                        case PLUS_ASSIGN:  w.print(" += "); break;
                        case MINUS_ASSIGN: w.print(" -= "); break;
                    }
                    emitComo(a.value, tLocal);   /* #559 — `x := i` con x float */
                    w.println(";");
                } else {
                    /* #172 — Global de módulo: acceso directo mem+cs+OFFSET.
                     * - ASSIGN:        write(mem+cs+OFF, value).
                     * - PLUS/MINUS:    write(mem+cs+OFF, read(mem+cs+OFF) [+|-] value). */
                    Integer off = moduleVarOffsets.get(tname);
                    if (off == null) {
                        throw new UnsupportedAotException(
                            "AOT: assign a '" + tname + "' — no es local"
                            + " ni módulo-global conocido (cross-module o"
                            + " tipo no soportado). line " + a.line);
                    }
                    usesModuleGlobals = true;
                    String addr = "mem + (uint32_t)((int32_t)cs + (" + off + "))";
                    if (moduleVarFloat.contains(tname)) {
                        /* #559 — una global `float` guarda el PATRÓN DE BITS: se lee y se
                         * escribe con los helpers de f32, como un parámetro del thunk. Con
                         * los de i32, C convertía el número (1.5 → 1) al escribir y el
                         * patrón (0x3FC00000 → 1069547520.0) al leer. */
                        w.print("vm->aot_helpers->write_f32_be(" + addr + ", ");
                        if (a.op == Ast.AssignOpKind.ASSIGN) emitComo(a.value, PrimitiveType.FLOAT);
                        else emitCompuestoFlotante(a.op, PrimitiveType.FLOAT,
                                () -> w.print("vm->aot_helpers->read_f32_be(" + addr + ")"), a.value);
                        w.println(");");
                    } else if (a.op == Ast.AssignOpKind.ASSIGN) {
                        w.print("vm->aot_helpers->write_i32_be(" + addr + ", ");
                        emitExpr(a.value);
                        w.println(");");
                    } else {
                        String binOp = (a.op == Ast.AssignOpKind.PLUS_ASSIGN) ? "+" : "-";
                        w.print("vm->aot_helpers->write_i32_be(" + addr
                              + ", vm->aot_helpers->read_i32_be(" + addr
                              + ") " + binOp + " (");
                        emitExpr(a.value);
                        w.println("));");
                    }
                }
                return;
            }
            /* Target IndexExpr (H3 #170): a[i] := v / += / -= via helper.
             * Para += / -= cargamos primero, operamos, escribimos. */
            if (a.target instanceof Ast.IndexExpr) {
                Ast.IndexExpr ix = (Ast.IndexExpr) a.target;
                BpType tArr = tipoDe(ix.target);
                BpType tElem = (tArr instanceof ArrayType) ? ((ArrayType) tArr).element : null;
                indent();
                if (a.op == Ast.AssignOpKind.ASSIGN) {
                    w.print("vm->aot_helpers->" + arrStoreFn(ix.target) + "(vm, ");
                    emitExpr(ix.target);
                    w.print(", ");
                    emitExpr(ix.index);
                    w.print(", ");
                    emitComo(a.value, tElem);   /* #559 — `v[0] := i` con v float[] */
                    w.println(");");
                } else if (esFloatT(tElem) || esDoubleT(tElem)) {
                    /* #559 — `v[i] += x` en un float[]/double[]: load, la operación del
                     * intérprete, store. */
                    w.print("vm->aot_helpers->" + arrStoreFn(ix.target) + "(vm, ");
                    emitExpr(ix.target);
                    w.print(", ");
                    emitExpr(ix.index);
                    w.print(", ");
                    emitCompuestoFlotante(a.op, tElem, () -> {
                        w.print("vm->aot_helpers->" + arrLoadFn(ix.target) + "(vm, ");
                        emitExpr(ix.target);
                        w.print(", ");
                        emitExpr(ix.index);
                        w.print(")");
                    }, a.value);
                    w.println(");");
                } else {
                    /* a[i] += v  →  store(a, i, load(a, i) + v) */
                    String binOp = (a.op == Ast.AssignOpKind.PLUS_ASSIGN) ? "+" : "-";
                    w.print("vm->aot_helpers->" + arrStoreFn(ix.target) + "(vm, ");
                    emitExpr(ix.target);
                    w.print(", ");
                    emitExpr(ix.index);
                    w.print(", vm->aot_helpers->" + arrLoadFn(ix.target) + "(vm, ");
                    emitExpr(ix.target);
                    w.print(", ");
                    emitExpr(ix.index);
                    w.print(") " + binOp + " (");
                    emitExpr(a.value);
                    w.println("));");
                }
                return;
            }
            if (a.target instanceof Ast.MemberAccessExpr) {
                /* #519 (V7) — escribir en un objeto (campo o property) no esta en el AOT v1:
                 * una native solo LEE properties, por su getter. Dicho con el nombre (con #519
                 * un metodo native que lo haga es un error, y el motivo tiene que servir). */
                throw new UnsupportedAotException(
                    "AOT: escribe en '" + ((Ast.MemberAccessExpr) a.target).member + "' (line "
                    + a.line + "), y una native no cambia el estado de un objeto (solo lee sus "
                    + "properties, por el getter): quita 'native' para que corra interpretada.");
            }
            throw new UnsupportedAotException(
                "AOT: assign target no soportado: "
                + a.target.getClass().getSimpleName() + " (line " + a.line + ")");
        }
        if (s instanceof Ast.WhileStmt) {
            Ast.WhileStmt wl = (Ast.WhileStmt) s;
            indent();
            w.print("while (");
            emitExpr(wl.condition);
            w.println(") {");
            indentLevel++;
            emitLineaDelBucle();
            for (Ast.IStmt st : wl.body) emitStmt(st);
            indentLevel--;
            indent();
            w.println("}");
            return;
        }
        /* [V6/N1.3] `print` dentro de una `native`.
         *
         * ⚠️ NO es "una llamada al runtime que ya existe", como parecia: el
         * interprete despacha POR TIPO (`PRINT_NONL`, `LPRINT_NONL`,
         * `DPRINT_NONL`, `FPRINT_NONL`, `PRINT_STR_NONL`, el booleano via builtin
         * BOOL_TO_STRING y el objeto via toString() polimorfico), y de esos la
         * tabla de helpers solo tiene i32, f32 y string.
         *
         * Asi que se soporta lo que hay y se RECHAZA lo demas nombrando el tipo.
         * Imprimir un `long` con print_i32 truncaria en silencio, y un `print`
         * que miente es peor que un `print` que no esta — sobre todo aqui, que su
         * uso es depurar.
         *
         * El espaciado se copia del interprete: un espacio entre items separados
         * por COMA (no por punto y coma), y un salto de linea al final. */
        if (s instanceof Ast.PrintStmt) {
            Ast.PrintStmt ps = (Ast.PrintStmt) s;
            /* V7/H7 #600 — como `emitPrint` (PRINT_STRING): un `print` de UNA sola cadena sale entero, el texto
             * y su salto de una vez (`print_string` con `nl` = 1), y no pieza a pieza. */
            if (ps.items.size() == 1 && ps.items.get(0).expr != null
                    && "print_string".equals(printHelperDe(ps.items.get(0).expr))) {
                indent();
                w.print("vm->aot_helpers->print_string(vm, ");
                emitExpr(ps.items.get(0).expr);
                w.println(", 1);");
                return;
            }
            for (int i = 0; i < ps.items.size(); i++) {
                Ast.PrintItem it = ps.items.get(i);
                if (i > 0 && it.leadingSep == Ast.PrintSep.COMMA) {
                    indent();
                    w.println("vm->aot_helpers->print_char(vm, ' ');");
                }
                if (it.expr == null) continue;
                String h = printHelperDe(it.expr);
                indent();
                w.print("vm->aot_helpers->" + h + "(vm, ");
                emitExpr(it.expr);
                /* El 3er parametro es `nl`: los helpers imprimen y opcionalmente
                 * saltan de linea. Aqui va 0 SIEMPRE —son los `*_NONL` del
                 * interprete— y el salto lo pone `print_nl` al final, que es
                 * exactamente el orden de `emitPrint` en MivmEmitter. */
                w.println(", 0);");
            }
            indent();
            w.println("vm->aot_helpers->print_nl(vm);");
            return;
        }

        /* [V6/N1.3] `do ... loop [cond]` — un `while` al reves, y C lo tiene tal
         * cual: `do { ... } while (cond)`. La condicion se evalua DESPUES, asi que
         * el cuerpo corre al menos una vez; sin condicion, es un bucle infinito y
         * se emite `while (1)`, que es lo que hace el interprete. */
        if (s instanceof Ast.DoLoopStmt) {
            Ast.DoLoopStmt dl = (Ast.DoLoopStmt) s;
            indent();
            w.println("do {");
            indentLevel++;
            emitLineaDelBucle();
            for (Ast.IStmt st : dl.body) emitStmt(st);
            indentLevel--;
            indent();
            if (dl.condition == null) {
                w.println("} while (1);");
            } else {
                w.print("} while (");
                emitExpr(dl.condition);
                w.println(");");
            }
            return;
        }
        if (s instanceof Ast.ForStmt) {
            emitForStmt((Ast.ForStmt) s);
            return;
        }
        if (s instanceof Ast.BreakStmt) {
            indent(); w.println("break;");
            return;
        }
        if (s instanceof Ast.ContinueStmt) {
            indent(); w.println("continue;");
            return;
        }
        if (s instanceof Ast.ExprStmt) {
            /* Statement-form de una expresión (call con side-effects,
             * típicamente). Emitimos la expr seguida de ';'. */
            Ast.ExprStmt es = (Ast.ExprStmt) s;
            indent();
            emitExpr(es.expr);
            w.println(";");
            return;
        }
        if (s instanceof Ast.SwitchStmt) {
            emitSwitchStmt((Ast.SwitchStmt) s);
            return;
        }
        if (s instanceof Ast.ThrowStmt) {
            emitThrowStmt((Ast.ThrowStmt) s);
            return;
        }
        if (s instanceof Ast.DestructAssignStmt) {   /* V7 #577 */
            emitDestructAssign((Ast.DestructAssignStmt) s);
            return;
        }
        if (s instanceof Ast.TryStmt) {
            /* #213 parte 2 — DIFERIDO con motivo: el código .mdn no puede
             * llamar setjmp (cero relocations externas — todo acceso al
             * runtime va via vm->aot_helpers, y un jmp_buf local del native
             * no sobrevive al longjmp del helper sin un refactor de locals
             * a context-struct + helper eh_native_try). El throw HACIA fuera
             * sí funciona (#186/#175/#213): envuelve la LLAMADA al native
             * en try/catch BP. */
            throw new UnsupportedAotException(
                "AOT: try/catch DENTRO de una función native no está soportado "
                + "(#213: .mdn no puede usar setjmp). Workaround: envuelve la "
                + "LLAMADA al native en try/catch BP — los throw del native "
                + "(RuntimeError o clases de usuario) sí se cazan ahí. line "
                + ((Ast.Node) s).line);
        }
        throw new UnsupportedAotException(
            "AOT: statement no soportado: " + s.getClass().getSimpleName()
            + " (line " + ((Ast.Node) s).line + ")");
    }

    /** #186 — `throw RuntimeError("literal")` desde native.
     *
     *  El helper throw_runtime construye el RuntimeError EN LA VM
     *  (bpvm_throw_runtime_error: localiza la clase, aloca string+objeto)
     *  y hace longjmp al boundary AOT del intérprete, que lo propaga por
     *  el eh_stack hasta el try/catch BP que envuelva la llamada (o
     *  termina el thread). El native NO construye el objeto.
     *
     *  #213 — clases de usuario: `throw MyExc(args)` tampoco construye el
     *  objeto en native. Cruza el puente call_bp_i32 a la factory
     *  cross-module `Owner.__cls_new_MyExc` (la misma que usa un módulo
     *  importador para `new` — corre el ctor REAL en el intérprete) y el
     *  ref resultante viaja por throw_ref → boundary → eh_unwind, igual
     *  que un throw BP normal. Requiere clase public (la factory solo se
     *  sintetiza para public) y ctor con firma i32 (límite v1 del puente).
     *  Lo que NO soporta esta fase: try/catch DENTRO de native (.mdn no
     *  puede llamar setjmp — cero relocations externas; necesitaría
     *  helpers eh_native_try + locals en context struct, diferido). En los
     *  casos no soportados lanzamos UnsupportedAotException y el módulo
     *  cae a bytecode interpretado (donde sí funciona). */
    private void emitThrowStmt(Ast.ThrowStmt ts) {
        /* Fast path: throw RuntimeError("literal") → throw_runtime con el
         * cstring directo (sin alocar string en el heap). */
        String lit = runtimeErrorLiteral(ts.value);
        if (lit != null) {
            byte[] bytes = lit.getBytes(java.nio.charset.StandardCharsets.UTF_8);
            indent();
            w.println("vm->aot_helpers->throw_runtime(vm, \"" + cEscapeBytes(bytes) + "\");");
            return;
        }
        /* #175 — throw RuntimeError(<expr string>) con mensaje COMPUTADO
         * (p.ej. "x = " + n). Emitimos el string-handle BP y throw_str lo lee.
         * AotCEmitter ya sabe emitir concats/ops de string (#173). */
        Ast.IExpr arg = runtimeErrorArg(ts.value);
        if (arg != null && isStringExpr(arg)) {
            indent();
            w.print("vm->aot_helpers->throw_str(vm, ");
            emitExpr(arg);
            w.println(");");
            return;
        }
        /* #213 — throw de CLASE DE USUARIO. La instancia se construye en el
         * intérprete (factory cross-module via puente); aquí solo viaja el ref. */
        BpType t = (semInfo != null) ? semInfo.exprTypes.get(ts.value) : null;
        if (t instanceof BpType.ClassType) {
            Symbol.ClassSymbol cls = ((BpType.ClassType) t).cls;
            if (ts.value instanceof Ast.CallExpr
                    && cls.name.equals(calleeSimpleName(((Ast.CallExpr) ts.value).callee))) {
                emitThrowUserClass((Ast.CallExpr) ts.value, cls, ts.line);
                return;
            }
            /* Ref ya construida (param de tipo clase, llamada que devuelve la
             * excepción, ...) → throw_ref directo del handle i32. Si emitExpr
             * no sabe emitir la expresión, su UnsupportedAotException hace el
             * fallback a interpretado, como siempre. */
            indent();
            w.print("vm->aot_helpers->throw_ref(vm, (uint32_t) (");
            emitExpr(ts.value);
            w.println("));");
            return;
        }
        throw new UnsupportedAotException(
            "AOT: en native `throw` soporta RuntimeError(string) y clases de "
            + "usuario public que desciendan de Exception "
            + "(try/catch DENTRO de native = pendiente #175b). line "
            + ts.line);
    }

    /** #213 — `throw MyExc(args)` desde native. La construcción NO se hace en
     *  C: cruzamos al intérprete via call_bp_i32 → factory cross-module
     *  `Owner.__cls_new_MyExc(args)` (corre el ctor real), y throw_ref deja el
     *  ref en el fault slot + longjmp al boundary, que lo propaga por
     *  eh_unwind. GC-safe: el thunk corre dentro del dispatch del intérprete
     *  (sin safepoint intermedio), así que el ref no puede ser recolectado
     *  entre la factory y el unwind. Validamos contra la firma REAL de la
     *  factory = ctor PROPIO de la clase (igual que synthesizeCrossModuleFactory). */
    private void emitThrowUserClass(Ast.CallExpr c, Symbol.ClassSymbol cls, int line) {
        indent();
        w.print("vm->aot_helpers->throw_ref(vm, (uint32_t) ");
        emitFactoriaClase(c, cls, line, "throw " + cls.name + "(...)",
            "la instancia del throw se construye en el intérprete");
        w.println(");");
    }

    /** [V6/N1.5] CREAR UN OBJETO desde una native — `Caja(21)`.
     *
     * <p>La instancia NO se aloca en C. Un objeto BP se construye ejecutando su
     * CONSTRUCTOR, y el constructor es bytecode: alocar los campos y devolver el
     * ref daria un objeto a medio hacer. Asi que se cruza el puente a la factoria
     * `__cls_new_<Clase>` —la misma que usa un modulo importador para un `new`—,
     * que corre el ctor de verdad y devuelve la instancia ya construida.
     *
     * <p>No es una ruta nueva: es exactamente la que #213 abrio para
     * `throw MiExcepcion(...)`, y por eso los dos gestos comparten codigo aqui.
     * Lo unico que hacia falta era dejar de mirarla solo dentro de un `throw`.
     *
     * <p>🔑 Y lo que la desbloqueaba de verdad tampoco estaba en el emisor: el
     * ref que devuelve la factoria queda en un local de C, y hasta V5 el GC no
     * miraba ahi (#302 paso 3). Con el objeto invisible para el GC, crearlo era
     * crear algo que se podia recolectar en vivo.
     *
     * <p>El helper `new_object` de la tabla NO se usa —y ahora lo dice al ser
     * invocado en vez de devolver 0—: existir no es lo mismo que servir. */
    private void emitNewObject(Ast.CallExpr c, Symbol.ClassSymbol cls, int line) {
        emitFactoriaClase(c, cls, line, cls.name + "(...)",
            "la instancia se construye en el intérprete, que es quien ejecuta el ctor");
    }

    /** Emite la llamada a `__cls_new_<Clase>` con sus validaciones. `gesto` es
     *  como se escribe en el `.bp` lo que la pide, para que el motivo del
     *  rechazo nombre lo que el usuario escribio y no un detalle interno. */
    private void emitFactoriaClase(Ast.CallExpr c, Symbol.ClassSymbol cls, int line,
                                   String gesto, String porque) {
        if (!cls.isPublic) {
            throw new UnsupportedAotException(
                "AOT: `" + gesto + "` en native (line " + line + ") necesita "
                + "que la clase sea public: la instancia se construye via la factory "
                + "cross-module __cls_new_" + cls.name + ", que solo se sintetiza para "
                + "clases public. Declárala `public class` o quita native.");
        }
        // 14-sep — la sobrecarga que ELIGIÓ el semántico (anotada en el nodo), no la
        // cabeza: con `Base(v: integer)` y `Base(t: string)`, `Base("texto")` llamaba a
        // `__cls_new_Base` con el string como `v`, sin aviso. La factoría lleva el
        // mismo sufijo de clave que en el emisor mivm (ctorSuffix).
        Symbol anot = (semInfo != null) ? semInfo.exprSymbols.get(c) : null;
        Symbol.FunctionSymbol ctor = (anot instanceof Symbol.FunctionSymbol) ? (Symbol.FunctionSymbol) anot
                                                                              : cls.findConstructor();
        String factoria = MivmEmitter.crossModuleNewName(cls, ctor);
        int expected = (ctor != null) ? ctor.params.size() : 0;
        if (c.args.size() != expected) {
            throw new UnsupportedAotException(
                "AOT: `" + gesto + "` (line " + line + "): " + c.args.size()
                + " args pero la factory " + factoria + " espera " + expected
                + " (firma del constructor elegido).");
        }
        if (ctor != null) {
            for (Symbol.ParamSymbol p : ctor.params) {
                if (!isBridgeI32Type(p.type)) {
                    throw new UnsupportedAotException(
                        "AOT: `" + gesto + "` (line " + line + "): el puente v1 "
                        + "solo soporta params i32 (integer/boolean/string/ref); el param '"
                        + p.name + "' del ctor no lo es. float/long/double → pendiente.");
                }
            }
        }
        StringBuilder qn = new StringBuilder();
        if (cls.isExternal) {
            if (cls.externalLibrary != null && !cls.externalLibrary.isEmpty())
                qn.append(cls.externalLibrary).append('.');
            qn.append(cls.externalModule);
        } else {
            qn.append(moduleName);
        }
        qn.append('.').append(factoria);

        /* La factory __cls_new_* SIEMPRE devuelve la instancia (una ref) → ret_is_ref=1. */
        int ctorMask = (ctor != null) ? refMaskOfParams(ctor.params) : 0;
        emitCallBpEmission(qn.toString(), c.args, line,
            "la factory de '" + cls.name + "' (" + porque + ")", ctorMask, true);
    }

    /** Devuelve el literal del mensaje si {@code value} es exactamente
     *  {@code RuntimeError("literal")}, o null si no matchea el subset v1. */
    private static String runtimeErrorLiteral(Ast.IExpr value) {
        Ast.IExpr arg = runtimeErrorArg(value);
        if (arg instanceof Ast.StringLitExpr) return ((Ast.StringLitExpr) arg).value;
        return null;
    }

    /** Devuelve el único argumento de {@code RuntimeError(arg)} si {@code value}
     *  es exactamente esa construcción, o null en otro caso. #175. */
    private static Ast.IExpr runtimeErrorArg(Ast.IExpr value) {
        if (!(value instanceof Ast.CallExpr)) return null;
        Ast.CallExpr call = (Ast.CallExpr) value;
        if (!"RuntimeError".equals(calleeSimpleName(call.callee))) return null;
        if (call.args == null || call.args.size() != 1) return null;
        return call.args.get(0);
    }

    /** Nombre simple del callee de una llamada: el identificador, o el
     *  miembro final de un acceso {@code X.RuntimeError}. */
    private static String calleeSimpleName(Ast.IExpr callee) {
        if (callee instanceof Ast.IdentifierExpr)   return ((Ast.IdentifierExpr) callee).name;
        if (callee instanceof Ast.MemberAccessExpr) return ((Ast.MemberAccessExpr) callee).member;
        return null;
    }

    /** for-loop BP: `for i = <from> to <to> [step <step>]` con semántica
     *  inclusiva. Cuando step es literal positivo emitimos un C `for`
     *  optimizable; en otro caso una variante `while` con check dinámico
     *  de dirección. */
    /* V7/H5 P2b.1.d (26-sep) — LA LÍNEA DEL BUCLE. Al PRINCIPIO del cuerpo de cada bucle —no al final: así
     * cuenta también las vueltas que acaban en un `continue`—, un contador que cada CUENTA_VUELTAS vueltas
     * llama al helper `punto_control`, y éste decide si la native cede el hilo (su rebanada de tiempo, un
     * `kill`). Es la marca de Eduardo del 24-ago («dar paso al intérprete para que pueda hacer cambios de
     * thread»), y lo que hace que una native no sea atómica frente al planificador: se suspende en su pila
     * prestada y sigue después como si nada (la idea del 26-sep: «como una interrupción»).
     *
     * La llamada, aunque casi nunca se haga, es opaca para gcc: por eso tampoco saca del bucle las lecturas
     * de memoria (una global que otro hilo cambia se vuelve a leer tras ceder). Cuesta +2 ciclos por vuelta
     * en el bucle más apretado (P2b.0, medido en la Pico). La recursión sin bucles queda fuera: una native
     * recursiva sin ningún bucle no pasa por aquí (apuntado en V7_IDEAS §H5.P2b). */
    /* V7/H5 P2b.2 (26-sep) — UNA INTRÍNSECA CUALQUIERA desde una native: si su cuerpo es `inline CALL_BUILTIN
     * n`, el AOT llama al MISMO despachador de builtins que el intérprete (el helper `call_builtin`). La native
     * ejecuta el código del intérprete y no una copia: la paridad sale por construcción (#569 fue una copia que
     * se quedó atrás). Cada argumento, convertido al tipo del parámetro como lo haría el intérprete (emitComo) y
     * pasado por sus BITS en una palabra de 64 bits; el resultado vuelve igual. Si el builtin lanza, sale por
     * el límite de fallos; si bloquea el hilo (sleep, Mutex.lock), la native se suspende (P2b.1). */
    private int tempBuiltin = 0;

    /** El id del builtin si el cuerpo de la intrínseca es exactamente `CALL_BUILTIN n`; si no, null. */
    private static Integer idDeBuiltin(Symbol.FunctionSymbol fs) {
        if (fs == null || !fs.isIntrinsic || fs.inlineOps == null || fs.inlineOps.size() != 1) return null;
        Ast.InlineStmt op = fs.inlineOps.get(0);
        if (!"CALL_BUILTIN".equals(op.mnemonic) || op.operands == null || op.operands.size() != 1) return null;
        return op.operands.get(0).intValue();
    }

    /** Cómo cruza un valor hacia call_builtin: 'i' (integer, boolean, byte, short, enum: 4 bytes), 'l' long,
     *  'f' float, 'd' double, 'r' referencia; 0 si no se sabe. */
    private static char claseBuiltin(BpType t) {
        if (t == null) return 0;
        if (esFloatT(t))  return 'f';
        if (esDoubleT(t)) return 'd';
        if (esLongT(t))   return 'l';
        if (isRefBp(t))   return 'r';
        if (MivmEmitter.occupies8Bytes(t)) return 0;
        return 'i';
    }

    private boolean emitLlamadaABuiltin(Symbol.FunctionSymbol fs, Ast.CallExpr c) {
        Integer id = idDeBuiltin(fs);
        /* V7/H7 #595 — Heap.compact() MUEVE los objetos del heap, y una native guarda sus direcciones en variables
         * de C que nadie sabría poner al día. En miVM la native es bytecode y la VM la rechazaría por no ser Main;
         * en la VM-C la rechazaría por estar dentro de una native: el error, al compilar, igual para las dos. */
        if (id != null && id == edu.bpgenvm.bytecode.Builtin.HEAP_COMPACT.id) {
            throw new UnsupportedAotException("AOT: Heap.compact() (line " + c.line + ") no se puede llamar desde"
                + " una native: mueve los objetos del heap, y una native guarda sus direcciones en variables de C."
                + " Se llama desde Main.");
        }
        if (id == null || fs.params == null || fs.params.size() != c.args.size()) return false;
        StringBuilder tipos = new StringBuilder();
        for (Symbol.ParamSymbol p : fs.params) {
            char k = claseBuiltin(p.type);
            if (k == 0) {
                throw new UnsupportedAotException("AOT: el builtin '" + fs.name + "' (line " + c.line
                    + ") tiene un parámetro que no sé pasarle desde una native (" + p.type + ").");
            }
            tipos.append(k);
        }
        char ret = (fs.returnType == null) ? 'v' : claseBuiltin(fs.returnType);
        if (ret == 0) {
            throw new UnsupportedAotException("AOT: el builtin '" + fs.name + "' (line " + c.line
                + ") devuelve un tipo que no sé traer a una native (" + fs.returnType + ").");
        }
        int t = ++tempBuiltin;
        String a = "__bpvm_a" + t, r = "__bpvm_r" + t;
        w.print("({ int64_t " + a + "[" + Math.max(1, c.args.size()) + "]; ");
        for (int i = 0; i < c.args.size(); i++) {
            Ast.IExpr x = c.args.get(i);
            BpType pt = fs.params.get(i).type;
            w.print(a + "[" + i + "] = ");
            switch (tipos.charAt(i)) {
                case 'f': w.print("bpvm_aot_f32_bits("); emitComo(x, pt); w.print(")"); break;
                case 'd': w.print("bpvm_aot_f64_bits("); emitComo(x, pt); w.print(")"); break;
                case 'l': w.print("(int64_t) ("); emitComo(x, pt); w.print(")"); break;
                case 'r': w.print("(int64_t) (uint32_t) ("); emitExpr(x); w.print(")"); break;
                default:  w.print("(int64_t) (int32_t) ("); emitComo(x, pt); w.print(")"); break;
            }
            w.print("; ");
        }
        w.print("int64_t " + r + " = vm->aot_helpers->call_builtin(vm, " + id + ", \"" + tipos + "\", "
                + a + ", '" + ret + "'); ");
        switch (ret) {
            case 'v': w.print("(void) " + r + "; "); break;
            case 'f': w.print("bpvm_aot_bits_f32(" + r + "); "); break;
            case 'd': w.print("bpvm_aot_bits_f64(" + r + "); "); break;
            case 'l': w.print("(int64_t) " + r + "; "); break;
            case 'r': w.print("(uint32_t) " + r + "; "); break;
            default:  w.print("(int32_t) " + r + "; "); break;
        }
        w.print("})");
        return true;
    }

    private static final String CUENTA = "__bpvm_cuenta";
    private static final int CUENTA_VUELTAS = 1024;

    private void emitLineaDelBucle() {
        indent();
        w.println("if (__builtin_expect(--" + CUENTA + " == 0, 0)) { " + CUENTA + " = " + CUENTA_VUELTAS
                  + "; vm->aot_helpers->punto_control(vm); }   /* P2b.1.d */");
    }

    private void emitForStmt(Ast.ForStmt fs) {
        if (!(fs.range instanceof Ast.ForNumericRange)) {
            throw new UnsupportedAotException(
                "AOT: for-range no numérico no soportado (line " + fs.line + ")");
        }
        Ast.ForNumericRange r = (Ast.ForNumericRange) fs.range;
        int tag = ++forCounter;
        String it   = cVar(fs.iteratorName);   /* #572 — el nombre en C; localNames guarda el de BP */
        String end  = "__aot_end_"  + tag;
        String step = "__aot_step_" + tag;

        // Wrap en bloque para limitar el scope del iterator + end + step.
        indent();
        w.println("{");
        indentLevel++;

        indent(); w.print("int32_t " + it + " = ");
                  emitExpr(r.from); w.println(";");
        indent(); w.print("int32_t " + end + " = ");
                  emitExpr(r.to);   w.println(";");
        if (r.step != null) {
            indent(); w.print("int32_t " + step + " = ");
                      emitExpr(r.step); w.println(";");
        } else {
            indent(); w.println("int32_t " + step + " = 1;");
        }
        /* #172 — el iterador es local del bucle; registrarlo para que
         * emitExpr(IdentifierExpr) lo trate como tal. Lo dejamos en
         * localNames durante el ciclo del for; al cerrar el for queda
         * el scope cerrado, pero como no removemos sería ambiguo si la
         * misma variable se reutilizara en otro for hermano dentro de
         * la misma función. En la práctica el frontend BP genera
         * scoping por bloque, así que el riesgo es bajo; documentamos
         * por si surge ruido futuro. */
        localNames.add(fs.iteratorName);

        // for (; (step > 0) ? (it <= end) : (it >= end); it += step) { body }
        // #548 (24-sep) — un `for` de C y no `while (…) { body; it += step; }`: el `continue` de C
        // va al `it += step`, como el de BP; con el `while` saltaba a la condición y el bucle no
        // acababa nunca.
        indent();
        w.println("for (; (" + step + " > 0) ? (" + it + " <= " + end + ") : ("
                  + it + " >= " + end + "); " + it + " += " + step + ") {");
        indentLevel++;
        emitLineaDelBucle();
        for (Ast.IStmt st : fs.body) emitStmt(st);
        indentLevel--;
        indent(); w.println("}");

        indentLevel--;
        indent();
        w.println("}");
    }

    /** BP switch → C switch. BP no tiene fall-through implícito (cada case acaba al final del
     *  bloque), así que añadimos `break;` automático tras cada case.
     *
     *  #546 (24-sep): (1) cada case va entre llaves: en BP un case es su propio bloque, y en C una
     *  declaración no puede ir justo tras una etiqueta (`case 1: int32_t x = …;` no compila);
     *  (2) si algún case es long y el sujeto no, se conmuta en 64 bits — C convertiría la etiqueta
     *  al tipo del sujeto y `case 5000000000L` casaría con 705032704, que el intérprete no casa;
     *  (3) C no conmuta sobre cadenas: con un sujeto string se conmuta sobre el NÚMERO del primer
     *  case que casa, comparando por CONTENIDO con string_eq y en orden, como el intérprete. Así
     *  `break` sigue saliendo del switch y `continue` sigue siendo del bucle de fuera. El sujeto se
     *  evalúa una vez, a un local de C, que el escaneo de la pila de C del native ve como a
     *  cualquier otro (heap.c §2d). */
    private void emitSwitchStmt(Ast.SwitchStmt sw) {
        boolean esString = isStringExpr(sw.subject);
        String suj = null;
        if (esString) {
            suj = "__sw" + (switchCounter++);
            indent(); w.println("{");
            indentLevel++;
            indent(); w.print("int32_t " + suj + " = ");
            emitExpr(sw.subject);
            w.println(";");
        }
        indent();
        if (esString) {
            w.print("switch (");
            int k = 0;
            for (Ast.CaseClause cc : sw.cases) {
                k++;
                w.print("(");
                for (int i = 0; i < cc.values.size(); i++) {
                    if (i > 0) w.print(" || ");
                    w.print("vm->aot_helpers->string_eq(vm, " + suj + ", ");
                    emitExpr(cc.values.get(i));
                    w.print(")");
                }
                w.print(") ? " + k + " : ");
            }
            w.println("0) {");
        } else {
            boolean alLong = !isLongExpr(sw.subject) && algunCaseLong(sw);
            w.print(alLong ? "switch ((int64_t) (" : "switch (");
            emitExpr(sw.subject);
            w.println(alLong ? ")) {" : ") {");
        }
        indentLevel++;
        int k = 0;
        for (Ast.CaseClause cc : sw.cases) {
            k++;
            if (esString) {
                indent(); w.println("case " + k + ": {");
            } else {
                for (int i = 0; i < cc.values.size(); i++) {
                    indent();
                    w.print("case ");
                    emitExpr(cc.values.get(i));
                    w.println(i == cc.values.size() - 1 ? ": {" : ":");
                }
            }
            indentLevel++;
            for (Ast.IStmt st : cc.body) emitStmt(st);
            indent(); w.println("break;");
            indentLevel--;
            indent(); w.println("}");
        }
        if (sw.defaultBody != null) {
            indent(); w.println("default: {");
            indentLevel++;
            for (Ast.IStmt st : sw.defaultBody) emitStmt(st);
            indent(); w.println("break;");
            indentLevel--;
            indent(); w.println("}");
        }
        indentLevel--;
        indent(); w.println("}");
        if (esString) {
            indentLevel--;
            indent(); w.println("}");
        }
    }

    /** #546 — ¿algún valor de case es long? */
    private boolean algunCaseLong(Ast.SwitchStmt sw) {
        for (Ast.CaseClause cc : sw.cases)
            for (Ast.IExpr v : cc.values)
                if (isLongExpr(v)) return true;
        return false;
    }

    private void emitIfStmt(Ast.IfStmt iff) {
        indent();
        w.print("if (");
        emitExpr(iff.then_.condition);
        w.println(") {");
        indentLevel++;
        for (Ast.IStmt s : iff.then_.body) emitStmt(s);
        indentLevel--;
        for (Ast.IfClause eif : iff.elseIfs) {
            indent();
            w.print("} else if (");
            emitExpr(eif.condition);
            w.println(") {");
            indentLevel++;
            for (Ast.IStmt s : eif.body) emitStmt(s);
            indentLevel--;
        }
        if (iff.else_ != null) {
            indent();
            w.println("} else {");
            indentLevel++;
            for (Ast.IStmt s : iff.else_) emitStmt(s);
            indentLevel--;
        }
        indent();
        w.println("}");
    }

    // ==================== Expressions ====================

    private void emitExpr(Ast.IExpr e) {
        if (e instanceof Ast.IntLitExpr) {
            w.print(((Ast.IntLitExpr) e).value);
            return;
        }
        if (e instanceof Ast.LongLitExpr) {
            /* #381 — literal de 64 bits. El sufijo `LL` NO es decorativo: sin
             * él, `1099511627776` es un literal cuyo tipo lo elige el
             * compilador de C, y una expresión como `1 << 40` o una constante
             * que quepa justo se evaluaría en 32 bits. Es la misma trampa que
             * en BP costó la ficha #385 (un literal sin `L` daba un número
             * plausible y equivocado), y aquí sería peor: saldría bien en el
             * host de 64 bits y mal en el micro.
             *
             * `Long.MIN_VALUE` se escribe como (-9223372036854775807LL - 1):
             * su valor absoluto no es un literal válido en C. */
            long v = ((Ast.LongLitExpr) e).value;
            if (v == Long.MIN_VALUE) w.print("(-9223372036854775807LL - 1)");
            else                     w.print(v + "LL");
            return;
        }
        if (e instanceof Ast.FloatLitExpr) {
            /* Emitimos con sufijo 'f' para que gcc no promocione a double.
             * Java Float.toString puede dar "1.0E10" — eso es válido como
             * literal C. */
            double v = ((Ast.FloatLitExpr) e).value;
            w.print(Float.toString((float) v) + "f");
            return;
        }
        /* [V6/N1.3] `null` — un cero. Una referencia BP es un handle empaquetado y
         * el handle nulo ES el cero, asi que no hay nada que traducir: el mismo
         * valor que empuja el interprete con OP_NULL. */
        if (e instanceof Ast.NullLitExpr) { w.print("0"); return; }
        if (e instanceof Ast.TupleExpr) { emitTupleExpr((Ast.TupleExpr) e); return; }   /* V7 #577 */
        if (e instanceof Ast.DoubleLitExpr) {
            /* #426 — sin sufijo: en C un literal de coma flotante YA es double.
             * `Double.toString` da la representacion mas corta que redondea de
             * vuelta al mismo valor, asi que el bit-pattern se conserva; y su
             * forma ("1.0E10") es valida como literal C. */
            w.print(Double.toString(((Ast.DoubleLitExpr) e).value));
            return;
        }
        if (e instanceof Ast.BoolLitExpr) {
            /* BP boolean → C int 0/1 (matchea cType(boolean) = int32_t). */
            w.print(((Ast.BoolLitExpr) e).value ? "1" : "0");
            return;
        }
        if (e instanceof Ast.StringLitExpr) {
            /* #173 — literal string: aloca un string heap en runtime con
             * los bytes UTF-8 del literal. Devuelve el handle (i32). */
            String v = ((Ast.StringLitExpr) e).value;
            byte[] bytes = v.getBytes(java.nio.charset.StandardCharsets.UTF_8);
            w.print("vm->aot_helpers->string_from_cstr(vm, \"");
            w.print(cEscapeBytes(bytes));
            w.print("\", " + bytes.length + ")");
            return;
        }
        if (e instanceof Ast.IdentifierExpr) {
            String name = ((Ast.IdentifierExpr) e).name;
            if (localNames.contains(name)) {
                w.print(cVar(name));                    // param o local (#572: su nombre en C)
                return;
            }
            /* #172 — Variable nivel-módulo: acceso directo `mem + cs +
             * OFFSET` con OFFSET compile-time y `cs` cacheada al inicio
             * de la función. */
            Integer off = moduleVarOffsets.get(name);
            if (off == null) {
                throw new UnsupportedAotException(
                    "AOT: identificador '" + name + "' no resuelve a local"
                    + " ni a módulo-global conocido (cross-module o tipo no"
                    + " soportado — pendiente #169/#171). line "
                    + ((Ast.Node) e).line);
            }
            usesModuleGlobals = true;
            /* #559 — una global `float` se lee como float (el patrón de bits), no como
             * int32 que luego C convertiría a número. */
            w.print("vm->aot_helpers->" + (moduleVarFloat.contains(name) ? "read_f32_be" : "read_i32_be")
                    + "(mem + (uint32_t)((int32_t)cs + (" + off + ")))");
            return;
        }
        if (e instanceof Ast.ParenExpr) {
            /* Paréntesis explícitos en BP — los emitimos igual en C
             * por claridad y para preservar precedencia escrita. */
            w.print("(");
            emitExpr(((Ast.ParenExpr) e).inner);
            w.print(")");
            return;
        }
        if (e instanceof Ast.BinaryExpr) {
            Ast.BinaryExpr b = (Ast.BinaryExpr) e;
            boolean stringOp = isStringExpr(b.left) || isStringExpr(b.right);
            /* #173 — concat: `a + b` con algún operando string → helper.
             * Operando no-string se coacciona (integer → int_to_string;
             * otros tipos mixtos quedan sin soporte por ahora). */
            if (stringOp && "+".equals(b.op)) {
                w.print("vm->aot_helpers->string_concat(vm, ");
                emitStringOperand(b.left);
                w.print(", ");
                emitStringOperand(b.right);
                w.print(")");
                return;
            }
            /* #173 — igualdad de strings: compara CONTENIDO, no la ref.
             * `==` → string_eq; `!=` → !string_eq. */
            if (stringOp && ("==".equals(b.op) || "!=".equals(b.op))) {
                boolean neg = "!=".equals(b.op);
                if (neg) w.print("(!");
                w.print("vm->aot_helpers->string_eq(vm, ");
                emitExpr(b.left);
                w.print(", ");
                emitExpr(b.right);
                w.print(")");
                if (neg) w.print(")");
                return;
            }
            /* #381 — DIVISIÓN Y MÓDULO DE 64 BITS: por helper, no con `/`.
             *
             * Escribir `a / b` con dos int64_t hace que gcc emita una llamada a
             * `__aeabi_ldivmod` (el Cortex-M no divide 64 bits en hardware), y
             * eso en un `.mdn` es un símbolo sin resolver: el empaquetador lo
             * rechaza, así que la división de `long` sencillamente no se podía
             * usar en una función `native`.
             *
             * Idea de Eduardo: reemplazarla en el emisor por una llamada a una
             * función. Va a la tabla de helpers —que es como el código nativo
             * alcanza TODO lo que no puede resolver por nombre— y la división de
             * verdad la hace el runtime, que sí se enlaza con libgcc. El `.mdn`
             * queda limpio y no hay que tocar el build de ninguna arquitectura.
             *
             * Basta con que UN operando sea `long`: en C la promoción haría el
             * resto de 64 bits igualmente, y es lo que hace el intérprete. */
            if (("/".equals(b.op) || "mod".equals(b.op) || "%".equals(b.op))
                    && (isLongExpr(b.left) || isLongExpr(b.right))) {
                w.print("vm->aot_helpers->"
                        + ("/".equals(b.op) ? "idiv64" : "imod64") + "(vm, (int64_t)(");
                emitExpr(b.left);
                w.print("), (int64_t)(");
                emitExpr(b.right);
                w.print("))");
                return;
            }
            /* #426 — ARITMETICA Y COMPARACION DE DOUBLE: por helper.
             *
             * Mismo caso que la division de `long` de arriba, y por el mismo
             * motivo: ninguna familia de hoy tiene FPU de doble (el RP2350 y el
             * STM32U5 son FPv5-SP; el P4 usa ilp32f), asi que `a + b` de double
             * se convierte en una llamada a libgcc — y eso en un `.mdn` es un
             * simbolo sin resolver.
             *
             * Los helpers viven DENTRO del firmware, que si esta enlazado contra
             * libgcc. Y hacen EXACTAMENTE lo que hace el interprete: si el camino
             * compilado fuera "mas correcto" con un NaN o un -0.0, el mismo
             * programa daria dos resultados segun llevara `.mdn` o no.
             *
             * Basta con que UN operando sea double: en C la promocion haria el
             * resto en double igualmente, que es lo que hace el interprete. */
            /* [V7/H5 #559] ... Y LA DE FLOAT, con la torre del intérprete calcada
             * (MivmEmitter.emitBinary: double > float > long > int, mirando los dos
             * operandos Y el tipo del resultado). Los operandos se pasan al tipo de la
             * operación con su helper: el `(double)(x)` de antes era, con x entero o
             * float, OTRA llamada a libgcc (`__aeabi_i2d`, `__extendsfdf2`) en todas las
             * placas. Y el float va por las macros de bpvm_aot_helpers.h: la
             * instrucción donde hay FPU, el helper donde no (la C5, la C3, la C6). */
            {
                BpType tl = tipoDe(b.left), tr = tipoDe(b.right), tb = tipoDe(b);
                /* `^` con resultado de coma flotante: OP_DPOW sobre double, y OP_D2F si
                 * el resultado es float (MivmEmitter.emitPow). El de enteros sigue sin
                 * traducir (cBinaryOp lo rechaza con su motivo). */
                if ("^".equals(b.op) && (esDoubleT(tb) || esFloatT(tb))) {
                    if (esFloatT(tb)) w.print("vm->aot_helpers->d2f(");
                    w.print("vm->aot_helpers->dpow(");
                    emitComo(b.left, PrimitiveType.DOUBLE);
                    w.print(", ");
                    emitComo(b.right, PrimitiveType.DOUBLE);
                    w.print(")");
                    if (esFloatT(tb)) w.print(")");
                    return;
                }
                boolean useDouble = esDoubleT(tl) || esDoubleT(tr) || esDoubleT(tb);
                boolean useFloat  = !useDouble && (esFloatT(tl) || esFloatT(tr) || esFloatT(tb));
                if ((useDouble || useFloat) && emitOpFlotante(b.op,
                        useDouble ? PrimitiveType.DOUBLE : PrimitiveType.FLOAT, b.left, b.right)) {
                    return;
                }
            }
            /* Numérico/lógico normal. */
            w.print("(");
            emitExpr(b.left);
            w.print(" " + cBinaryOp(b.op) + " ");
            emitExpr(b.right);
            w.print(")");
            return;
        }
        if (e instanceof Ast.UnaryExpr) {
            Ast.UnaryExpr u = (Ast.UnaryExpr) e;
            w.print("(");
            if ("-".equals(u.op))      w.print("-");
            else if ("not".equals(u.op)) w.print("!");
            else throw new UnsupportedAotException("AOT: unary '" + u.op + "' no soportado");
            emitExpr(u.operand);
            w.print(")");
            return;
        }
        if (e instanceof Ast.IndexExpr) {
            /* a[i] — lectura de array via helper. v1: solo integer[]. */
            Ast.IndexExpr ix = (Ast.IndexExpr) e;
            w.print("vm->aot_helpers->" + arrLoadFn(ix.target) + "(vm, ");
            emitExpr(ix.target);
            w.print(", ");
            emitExpr(ix.index);
            w.print(")");
            return;
        }
        /* [V6/N1.5] LITERAL DE ARRAY — `[1, 2, 3]` dentro de una native.
         *
         * Se emite como una EXPRESION DE SENTENCIAS de gcc (`({ ...; v; })`), y
         * no por gusto: crear un array son N+1 operaciones (reservar y luego
         * rellenar) y el literal aparece donde se espera un valor. Con el temp
         * dentro del bloque no hay que declarar nada arriba ni llevar un
         * contador de temporales, y el anidamiento sale gratis. La extension es
         * de gcc y el `.c` generado ya usa otras (`__attribute__((used))`);
         * los tres toolchains del proyecto son gcc.
         *
         * 🔑 Y ES SEGURO FRENTE AL GC por una razon concreta: el handle recien
         * creado vive en `__arr`, un LOCAL DE C — y desde V5 (#302 paso 3, idea
         * de Eduardo) el GC escanea la pila de C del native con el volcado de
         * registros de setjmp. Antes de eso, rellenar el array habria podido
         * recolectarlo a medias: cada elemento que alocara seria una tirada de
         * dados. Esa era la verdadera razon por la que esto estaba pendiente. */
        if (e instanceof Ast.ArrayLitExpr) { emitArrayLit((Ast.ArrayLitExpr) e); return; }

        if (e instanceof Ast.CallExpr) {
            Ast.CallExpr c = (Ast.CallExpr) e;

            if (c.callee instanceof Ast.MemberAccessExpr) {
                Ast.MemberAccessExpr ma = (Ast.MemberAccessExpr) c.callee;
                /* #169 — llamada cross-module `Mod.func(args)`: el símbolo
                 * resuelto del callee es una FunctionSymbol externa a nivel
                 * módulo. La emitimos por el puente call_bp con el nombre
                 * cualificado; el runtime resuelve la dirección y el CS del
                 * módulo destino. (Intrínsecos cross-module y métodos de
                 * instancia [dispatch virtual, #174] siguen pendientes.) */
                Symbol callSym = (semInfo != null) ? semInfo.exprSymbols.get(c.callee) : null;
                if (callSym instanceof Symbol.FunctionSymbol) {
                    Symbol.FunctionSymbol fs = (Symbol.FunctionSymbol) callSym;
                    if (fs.isExternal && !fs.isIntrinsic && fs.ownerClass == null) {
                        emitCrossModuleBridgeCall(fs, c);
                        return;
                    }
                    if (fs.isExternal && fs.isIntrinsic) {
                        /* V7/H5 P2b.2 — `Math.sin(x)`: por el despachador de la VM. */
                        if (emitLlamadaABuiltin(fs, c)) return;
                        throw new UnsupportedAotException(
                            "AOT: intrínseco cross-module '" + fs.externalQualifiedName()
                            + "' (line " + c.line + ") no soportado en native: su cuerpo `inline` no es"
                            + " una llamada a un builtin (CALL_BUILTIN n).");
                    }
                    /* #174b — método PÚBLICO de instancia (virtual) sobre un
                     * receptor: despacho por vtable vía call_method_i32 con el
                     * slot que computa el frontend (ClassSymbol.slotOf, decisión
                     * B). Privado/super/estático van por el throw de abajo (no
                     * virtuales → sin asidero, pendientes). */
                    // #390 — el comentario de arriba ya decía «no virtuales → sin
                    // asidero»; ahora la condición lo pregunta de verdad.
                    if (fs.ownerClass != null && !fs.isStatic && fs.esVirtual()
                            && !(ma.target instanceof Ast.SuperExpr)) {
                        emitVirtualMethodCall(fs, ma, c);
                        return;
                    }
                }
                /* [V6/N1.5] `OtroMod.Caja(21)` — construccion cross-module. Se
                 * mira DESPUES de los metodos (un metodo que devuelva una
                 * instancia de su propia clase no es una construccion: ahi el
                 * nombre del callee es el del metodo, no el de la clase). */
                Symbol.ClassSymbol ctorCls = claseConstruida(c);
                if (ctorCls != null) { emitNewObject(c, ctorCls, c.line); return; }
                throw new UnsupportedAotException(
                    "AOT: method call '" + ma.member + "' no soportado todavía (line "
                    + c.line + ") — solo método público (virtual); privado/super/"
                    + "estático pendientes.");
            }

            if (!(c.callee instanceof Ast.IdentifierExpr)) {
                throw new UnsupportedAotException(
                    "AOT: call con callee no-identifier no soportado (line " + c.line + ")");
            }
            String name = ((Ast.IdentifierExpr) c.callee).name;

            /* #381 — CONVERSIONES NUMÉRICAS: `integer(x)`, `long(x)`, `float(x)`.
             *
             * En BP una conversión se escribe con el nombre del tipo, así que al
             * emisor le llega como una llamada — y hasta ahora moría con «call a
             * función desconocida 'integer'». Iba antes de que `long` existiera
             * en las nativas, pero con `long` dentro es lo primero que hace
             * falta: bajar a 32 bits para indexar o imprimir es el gesto normal.
             *
             * Se emite el CAST DE C, y eso no es una aproximación: los opcodes
             * del intérprete son literalmente eso — `OP_I64_TO_I32` es
             * `(int32_t) v`, `OP_I2F` es `(float) v`, `OP_F2L` es `(int64_t) f`
             * (`interp.c:1213`, `:1091`, `:1355`). El código compilado ejecuta la
             * misma conversión que el interpretado, no una equivalente.
             *
             * Va ANTES de los builtins a propósito: si alguien declarase una
             * función suya llamada `integer`, la conversión debe seguir siendo la
             * conversión — es sintaxis del lenguaje, no una llamada cualquiera. */
            /* V7/H5 (27-sep) — las conversiones ESTRECHAS (L10): `byte(x)`, `int8(x)`, `word(x)` e `int16(x)` (y
             * `short(x)`, que el parser deja en `int16`). En el intérprete no son una llamada sino un opcode
             * (OP_I32_TO_U8…) que comprueba el rango: dentro, el valor tal cual; fuera, un RuntimeError atrapable
             * con su texto. Aquí, lo mismo, por aot_<Mod>_estrecha (el prólogo). Hasta hoy caían en «función
             * desconocida» y la native entera corría interpretada. Son palabras reservadas: una función de BP no
             * puede llamarse así, así que el nombre basta. El argumento es un integer (el análisis no deja otra
             * cosa, como en MivmEmitter), y el valor sigue siendo un int32_t de C. */
            String[] estrecha = (c.args.size() == 1) ? conversionEstrecha(name) : null;
            if (estrecha != null) {
                byte[] texto = estrecha[2].getBytes(java.nio.charset.StandardCharsets.UTF_8);
                w.print("aot_" + moduleName + "_estrecha(vm, (int32_t) (");
                emitExpr(c.args.get(0));
                w.print("), " + estrecha[0] + ", " + estrecha[1] + ", \"" + cEscapeBytes(texto) + "\", "
                        + texto.length + ")");
                return;
            }

            if (c.args.size() == 1 && esCastNumerico(name)) {
                /* [V7/H5 #559] ... salvo las que el micro no hace en hardware, que van
                 * por su helper (la misma expresión de C que el opcode, compilada en el
                 * firmware). Con eso `double(x)` deja de rechazarse: era el resto de
                 * #426, y su motivo —«esas rutinas no caben en el código nativo»— es
                 * justo lo que resuelven los helpers. */
                emitConversionExplicita(name, c.args.get(0));
                return;
            }

            /* Builtins (H3 #168) — antes que native funcs por si hubiera
             * colisión de nombres. Lista hardcoded por ahora; expansible. */
            if (emitBuiltinCall(name, c.args)) return;

            /* V7/H5 P2b.2 — cualquier otra intrínseca (`inline CALL_BUILTIN n`), local o de `Core`: por el
             * despachador de la VM. La lista a mano de arriba va antes mientras se mide si compensa. */
            {
                Symbol simG = (semInfo != null) ? semInfo.exprSymbols.get(c.callee) : null;
                if (simG instanceof Symbol.FunctionSymbol && ((Symbol.FunctionSymbol) simG).isIntrinsic
                        && emitLlamadaABuiltin((Symbol.FunctionSymbol) simG, c)) return;
            }

            if (!nativeFuncNames.contains(name)) {
                /* #211 — ¿es una función BP del mismo módulo? Entonces NO
                 * abortamos: emitimos la llamada-puente native→BP
                 * (vm->aot_helpers->call_bp_i32), que ejecuta su cuerpo
                 * interpretado. Antes esto era un error duro. */
                Ast.FuncDef target = allFuncDefs.get(name);
                if (target != null) {
                    emitBridgeCall(name, target, c);
                    return;
                }
                /* [V6/N1.5] `Caja(21)` — crear un objeto. Va aqui, al final: si
                 * existiera una funcion con el nombre de la clase, la funcion
                 * manda (es lo que hace el resto del compilador). */
                Symbol.ClassSymbol nuevaCls = claseConstruida(c);
                if (nuevaCls != null) { emitNewObject(c, nuevaCls, c.line); return; }
                /* V7/H5 (27-sep) — el texto decía «un builtin AOT-soportado (now, charAt, charCodeAt, substring,
                 * intToString)» y «cross-module sigue pendiente (#169)»: desde P2b.2 vale cualquier intrínseca, y lo
                 * de otro módulo se llama cualificado (#169, `Mod.f(…)`, por emitCrossModuleBridgeCall). */
                throw new UnsupportedAotException(
                    "AOT: la native llama a '" + name + "' (line " + c.line + "), que no es una función de este "
                    + "módulo (native o BP), ni una intrínseca (Math.sin, charAt…), ni una conversión, ni la "
                    + "construcción de una clase. Una función de otro módulo se llama cualificada (Mod.f(…)); "
                    + "si no, quita 'native' para que corra interpretada.");
            }

            /* C call directo a otra función native del mismo módulo.
             * BL PC-relative dentro del mismo blob — gcc lo resuelve
             * sin relocations porque ambas son static en el mismo .o. */
            llamadasDirectas.computeIfAbsent(currentFuncName, k -> new java.util.LinkedHashSet<>()).add(name);   /* #580 */
            w.print("aot_" + moduleName + "_" + name + "(vm");
            Ast.FuncDef llamada = allFuncDefs.get(name);
            for (int k = 0; k < c.args.size(); k++) {
                w.print(", ");
                /* #559 — el argumento, al tipo del parámetro (OP_I2F y compañía). */
                BpType tp = (llamada != null && k < llamada.params.size())
                        ? tipoDeclarado(llamada.params.get(k).type) : null;
                emitComo(c.args.get(k), tp);
            }
            /* #580 (1) — a una native de un ciclo, el contador de las entradas: el mismo si se llama desde dentro de
             * otra (sigue contando), uno nuevo si se entra en el ciclo desde fuera. */
            if (recursivas.contains(name))
                w.print(recursivas.contains(currentFuncName)
                        ? ", " + CUENTA_R : ", &(int32_t){ " + CUENTA_VUELTAS + " }");
            w.print(")");
            return;
        }
        /* [V6/N1.1] `this` — en un metodo aplanado ES el primer parametro, con ese
         * mismo nombre (ver el barrido de emitModule). Asi que no hay nada que
         * resolver: se emite como el parámetro, como el `self` de Python. */
        if (e instanceof Ast.ThisExpr) { w.print(cVar("this")); return; }   /* #572: el parámetro `this` */

        /* [V6/N1.1] Lectura de una PROPERTY — `obj.x`, y tambien `this.x`.
         *
         * En BP no hay campos publicos: publico ⇒ property, y leer una property
         * es LLAMAR A SU GETTER. Criterio de Eduardo (23-ago-2026), y no es una
         * conveniencia: es como lo emite ya el bytecode (`emitInvokeVirtualSmart(
         * cls, "get"+Nombre, 0)` en MivmEmitter), asi que el AOT hace lo mismo y
         * no inventa una segunda semantica.
         *
         * Que un `native` NO pueda leer un campo directamente es una limitacion
         * ACEPTADA — Eduardo: *«es razonable, teniendo en cuenta que siempre
         * puede acceder a traves de un getter»*. Y encaja con la maquina: entre
         * los helpers del AOT esta `call_method_i32` y NO hay ninguno para leer
         * campos, asi que esta es la unica ruta que el runtime ya soporta. */
        if (e instanceof Ast.MemberAccessExpr) {
            Ast.MemberAccessExpr ma = (Ast.MemberAccessExpr) e;
            Symbol mSym = (semInfo != null) ? semInfo.exprSymbols.get(ma) : null;
            if (mSym instanceof Symbol.PropertySymbol) {
                Symbol.PropertySymbol ps = (Symbol.PropertySymbol) mSym;
                if (ps.ownerClass == null || ps.isStatic) {
                    throw new UnsupportedAotException(
                        "AOT: property de modulo o estatica '" + ma.member + "' (line "
                        + ma.line + "): el puente v1 solo cubre properties de instancia.");
                }
                String getter = "get" + Character.toUpperCase(ps.name.charAt(0))
                              + ps.name.substring(1);
                String slotExpr;
                Symbol.ClassSymbol recvCls = claseEstaticaDe(ma.target, ps.ownerClass);
                if (porRef9(recvCls) || porRef9(ps.ownerClass)) {
                    /* V7/H2 (#533) — clase externa o abierta: la ranura por clave, en ejecución. */
                    String clave = getter + "()" + tipoClave(ps.type, "la property '" + ps.name + "'", ma.line);
                    slotExpr = ranuraExpr(recvCls, clave, "la property '" + ps.name + "'", ma.line);
                } else {
                    int slot = ps.ownerClass.slotOf(getter);
                    if (slot < 0) {
                        throw new UnsupportedAotException(
                            "AOT: no se pudo resolver el slot del getter '" + getter
                            + "' de '" + ps.ownerClass.name + "' (line " + ma.line + ").");
                    }
                    slotExpr = String.valueOf(slot);
                }
                boolean retIsRef = isRefBp(ps.type);
                if (!isBridgeI32Type(ps.type) && !retIsRef) {
                    throw new UnsupportedAotException(
                        "AOT: property '" + ma.member + "' (line " + ma.line + "): el puente "
                        + "v1 solo soporta i32 y referencias; float/long/double → pendiente.");
                }
                warnings.add("la funcion native '" + currentFuncName + "' lee la property '"
                    + ma.member + "' (linea " + ma.line + "). Esa lectura cruza al interprete "
                    + "por el puente native→BP (getter virtual) y NO se acelera por AOT.");
                w.print("vm->aot_helpers->call_method_i32(vm, ");
                emitExpr(ma.target);
                w.print(", " + slotExpr + ", (const int32_t*) 0, 0, 0u, " + (retIsRef ? 1 : 0) + ")");
                return;
            }
            /* #519 (V7) — un CAMPO (var de clase): la limitacion aceptada de arriba, dicha
             * con su nombre. Con #519 un metodo native que lo haga es un error, y el motivo
             * «expression no soportada: MemberAccessExpr» no le decia nada al programador. */
            if (mSym instanceof Symbol.VarSymbol && ((Symbol.VarSymbol) mSym).ownerClass != null) {
                throw new UnsupportedAotException(
                    "AOT: lee el campo '" + ma.member + "' (line " + ma.line + "), y una native "
                    + "solo lee el estado de un objeto por una property (su getter): declara '"
                    + ma.member + "' como property, o quita 'native' para que corra interpretada.");
            }
        }

        throw new UnsupportedAotException(
            "AOT: expression no soportada: " + e.getClass().getSimpleName());
    }

    /** #211 — emite una llamada native→BP usando el puente call_bp_i32.
     *  La función BP destino corre en el INTÉRPRETE (pierde la velocidad
     *  AOT — de ahí el aviso). v1: solo firmas i32-compatibles
     *  (integer/boolean/string/array; el return también). float/long/
     *  double/void → error claro (marshalling distinto, pendiente). */
    private void emitBridgeCall(String name, Ast.FuncDef target, Ast.CallExpr c) {
        if (!isBridgeI32Type(target.returnType)) {
            throw new UnsupportedAotException(
                "AOT: call native→BP a '" + name + "' (line " + c.line + "): el puente v1 "
                + "solo soporta retorno i32 (integer/boolean/string/array); "
                + "float/long/double/void → pendiente.");
        }
        for (Ast.Param p : target.params) {
            if (!isBridgeI32Type(p.type)) {
                throw new UnsupportedAotException(
                    "AOT: call native→BP a '" + name + "' (line " + c.line + "): el puente v1 "
                    + "solo soporta params i32; el param '" + p.name + "' no lo es. "
                    + "Tipos mixtos → pendiente.");
            }
        }
        if (c.args.size() != target.params.size()) {
            throw new UnsupportedAotException(
                "AOT: call native→BP a '" + name + "' (line " + c.line + "): "
                + c.args.size() + " args pero '" + name + "' espera "
                + target.params.size() + ".");
        }

        emitCallBpEmission(moduleName + "." + name, c.args, c.line,
            "la función BP interpretada '" + name + "'. Para máximo rendimiento, "
            + "declara '" + name + "' también como native",
            refMaskOfFuncDef(target), returnIsRef(target));
    }

    /** #169 — emite una llamada cross-module native→BP (`Mod.func(args)`) por
     *  el puente. La firma viene de la FunctionSymbol externa (.bpi del módulo
     *  dueño). El runtime resuelve la dirección y el CS del módulo destino. */
    private void emitCrossModuleBridgeCall(Symbol.FunctionSymbol fs, Ast.CallExpr c) {
        // V7/H2 (#523) — en MOD9 el nombre que cruza lleva la firma (R4: `Lib.f(i)i`),
        // como en el `.mod`: si `f` cambia de firma en otra versión de `Lib`, el
        // puente no la encuentra y lo dice, en vez de llamarla con los argumentos
        // de la vieja (la fila «f_firma» de mezcla.py, ahora también para el nativo).
        String qn = (Main.FORMATO == 9)
            ? ((fs.externalLibrary == null || fs.externalLibrary.isEmpty()) ? "" : fs.externalLibrary + ".")
              + fs.externalModule + "." + Symbol.ClaveCanonica.metodo(fs, MivmEmitter.moduloLocalActual)
            : fs.externalQualifiedName();
        if (!isBridgeI32Type(fs.returnType)) {
            throw new UnsupportedAotException(
                "AOT: call native→BP cross-module a '" + qn + "' (line " + c.line + "): el puente "
                + "v1 solo soporta retorno i32; float/long/double/void → pendiente.");
        }
        for (Symbol.ParamSymbol p : fs.params) {
            if (!isBridgeI32Type(p.type)) {
                throw new UnsupportedAotException(
                    "AOT: call native→BP cross-module a '" + qn + "' (line " + c.line + "): el puente "
                    + "v1 solo soporta params i32; tipos mixtos → pendiente.");
            }
        }
        if (c.args.size() != fs.params.size()) {
            throw new UnsupportedAotException(
                "AOT: call cross-module a '" + qn + "' (line " + c.line + "): "
                + c.args.size() + " args pero espera " + fs.params.size() + ".");
        }
        emitCallBpEmission(qn, c.args, c.line, "la función BP cross-module '" + qn + "'",
            refMaskOfParams(fs.params), isRefBp(fs.returnType));
    }

    /** V7/H2 (#523 fase 1 → #533 fase 2) — las dos únicas salidas del `.mdn` que dependen
     *  de la forma de una clase son `call_method_i32(vm, this, slot, …)` (un método, o el
     *  getter de una property). Una `native` sólo HORNEA ese `slot` (`ClassSymbol.slotOf`)
     *  para una clase CERRADA de su propio módulo: con una EXTERNA el número es el de la
     *  versión contra la que se compiló, y con una ABIERTA (R1) lo asigna el CARGADOR contra
     *  el padre real — hornearlo sería la familia de #514 dentro del código nativo, en
     *  silencio. La fase 1 (16-sep) rechazaba; la fase 2 (18-sep) hace lo que hace el
     *  `.mod` con sus opcodes `_REF`: pedir la ranura por CLAVE en ejecución
     *  (`aot_<Mod>_ranura` → `slot_por_clave`, el E3 del enlace si no está). En MOD8 nada
     *  cambia: se hornea como siempre. */
    private boolean porRef9(Symbol.ClassSymbol cls) {
        return Main.FORMATO == 9 && cls != null
            && (cls.isExternal || MivmEmitter.esAbierta(cls));
    }
    /** La clase ESTÁTICA del receptor (la misma que usa el emisor mivm para la ref, R2);
     *  si el tipo del receptor no es una clase, la que declara el miembro. */
    private Symbol.ClassSymbol claseEstaticaDe(Ast.IExpr target, Symbol.ClassSymbol porDefecto) {
        BpType t = (semInfo != null) ? semInfo.exprTypes.get(target) : null;
        return (t instanceof BpType.ClassType && ((BpType.ClassType) t).cls != null) ? ((BpType.ClassType) t).cls : porDefecto;
    }
    private String tipoClave(BpType t, String que, int line) {
        try { return Symbol.ClaveCanonica.tipo(t, moduloLocal); }
        catch (Symbol.ClaveCanonica.TipoSinResolver e) {
            throw new UnsupportedAotException("AOT: " + e.getMessage() + " (" + que + ", line " + line + ")");
        }
    }
    /** `aot_<Mod>_ranura(vm, "Mod.Cls", "clave", "aviso")`: el aviso nombra la native, el
     *  método y a quién recompilar, como el E3 del enlace. */
    private String ranuraExpr(Symbol.ClassSymbol clase, String clave, String que, int line) {
        String qn;
        try { qn = Symbol.ClaveCanonica.clase(clase, moduloLocal); }
        catch (Symbol.ClaveCanonica.TipoSinResolver e) {
            throw new UnsupportedAotException("AOT: " + e.getMessage() + " (" + que + ", line " + line + ")");
        }
        String aviso = "AOT: la native '" + currentFuncName + "' pide '" + qn + "." + clave
            + "' y la clase cargada no lo tiene (¿otra versión de la dependencia?): recompila '" + moduleName + "'";
        byte[] b = aviso.getBytes(java.nio.charset.StandardCharsets.UTF_8);
        return "aot_" + moduleName + "_ranura(vm, \"" + qn + "\", \"" + clave + "\", \"" + cEscapeBytes(b) + "\")";
    }

    /** #174b — emite una llamada a método PÚBLICO de instancia (virtual) desde
     *  native vía call_method_i32. El slot lo computa el frontend
     *  (ClassSymbol.slotOf) y MivmEmitter lo verifica contra ModWriter. La VM
     *  hace el paseo vtable (obj→class→vtable[slot]) según la clase REAL del
     *  receptor → polimorfismo correcto. v1: firmas i32 (params + retorno). */
    private void emitVirtualMethodCall(Symbol.FunctionSymbol fs, Ast.MemberAccessExpr ma, Ast.CallExpr c) {
        if (!isBridgeI32Type(fs.returnType)) {
            throw new UnsupportedAotException(
                "AOT: método native→BP '" + fs.name + "' (line " + c.line + "): el puente v1 "
                + "solo soporta retorno i32; float/long/double/void → pendiente.");
        }
        for (Symbol.ParamSymbol p : fs.params) {
            if (!isBridgeI32Type(p.type)) {
                throw new UnsupportedAotException(
                    "AOT: método native→BP '" + fs.name + "' (line " + c.line + "): el puente v1 "
                    + "solo soporta params i32; tipos mixtos → pendiente.");
            }
        }
        String slotExpr;
        Symbol.ClassSymbol recvCls = claseEstaticaDe(ma.target, fs.ownerClass);
        if (porRef9(recvCls) || porRef9(fs.ownerClass)) {
            /* V7/H2 (#533) — clase externa o abierta: la ranura por clave, en ejecución. */
            String clave;
            try { clave = Symbol.ClaveCanonica.metodo(fs, moduloLocal); }
            catch (Symbol.ClaveCanonica.TipoSinResolver e) {
                throw new UnsupportedAotException("AOT: " + e.getMessage() + " (método '" + fs.name + "', line " + c.line + ")");
            }
            slotExpr = ranuraExpr(recvCls, clave, "el método '" + fs.name + "'", c.line);
        } else {
            int slot = (fs.ownerClass != null) ? fs.ownerClass.slotOf(fs.name) : -1;
            if (slot < 0) {
                throw new UnsupportedAotException(
                    "AOT: no se pudo resolver el slot de vtable de '" + fs.name + "' (line "
                    + c.line + ").");
            }
            slotExpr = String.valueOf(slot);
        }
        if (c.args.size() != fs.params.size()) {
            throw new UnsupportedAotException(
                "AOT: método '" + fs.name + "' (line " + c.line + "): " + c.args.size()
                + " args pero espera " + fs.params.size() + ".");
        }
        warnings.add("la función native '" + currentFuncName + "' invoca el método público '"
            + fs.name + "' (línea " + c.line + "). Esa llamada cruza al intérprete por el "
            + "puente native→BP (dispatch virtual) y NO se acelera por AOT.");
        /* call_method_i32(vm, <this>, slot, (int32_t[]){args...}, n, ref_mask, ret_is_ref).
         * #302 paso 2 — ref_mask marca los args del USUARIO (el `this` lo añade la
         * propia call_method_i32 como bit 0); ret_is_ref, el retorno. */
        int refMask = refMaskOfParams(fs.params);
        boolean retIsRef = isRefBp(fs.returnType);
        String tail = ", " + refMask + "u, " + (retIsRef ? 1 : 0) + ")";
        w.print("vm->aot_helpers->call_method_i32(vm, ");
        emitExpr(ma.target);   // receptor (this)
        w.print(", " + slotExpr + ", ");
        int n = c.args.size();
        if (n == 0) {
            w.print("(const int32_t*) 0, 0" + tail);
            return;
        }
        w.print("(int32_t[]){ ");
        for (int i = 0; i < n; i++) {
            if (i > 0) w.print(", ");
            emitExpr(c.args.get(i));
        }
        w.print(" }, " + n + tail);
    }

    /** Emisión común del puente: aviso + call_bp_i32(vm, find_function(qn),
     *  (int32_t[]){args...}, n). El compound literal C99 vive en el bloque
     *  envolvente — válido como argumento. find_function resuelve el nombre
     *  cada vez (scan barato; cachear en static es mejora futura, pero el coste
     *  del puente domina).
     *
     *  <h3>⚠️ El nombre va como LITERAL DE C, y así debe quedarse</h3>
     *
     *  El 24-ago-2026 lo cambié a materializarlo byte a byte en la pila, porque
     *  `MdnPack` rechazaba el `.o`: el literal vive en `.rodata` y un `.mdn` se
     *  lleva `.text`. Y concluí que el puente «nunca había llegado a una placa».
     *
     *  <p><b>Las dos cosas eran falsas, y por el mismo motivo</b>: comprobé el
     *  `.o` <b>sin el paso de ENLACE</b>, que el pipeline real hace siempre
     *  ({@code AotBuild.enlazar} = true en todas las familias). El guión
     *  {@code bpgenvm-c/aot/mdn.ld} <b>fusiona `.rodata` dentro de `.text`</b>, y
     *  con él el literal empaqueta sin problema.
     *
     *  <p>Eso es `#428`, cerrado en V5 el 16-ago con la idea de Eduardo —<i>«esos
     *  literales tienen que ir como parte del código nativo»</i>— y verificado en
     *  la Metro. No es una trampa: enlazado a dos direcciones distintas el
     *  `.text` sale byte-idéntico, o sea que sigue siendo relocatable.
     *
     *  <p>📌 Lo caro del error no fue el código de más: fue medir <b>una etapa
     *  que el producto no tiene</b> y sacar de ahí una conclusión sobre la placa.
     *  Si vuelves a ver a `MdnPack` quejarse de un `.LC0`, la pregunta es si
     *  estás enlazando, no si hay que quitar el literal. */
    /* ── V7 #577 (7-oct; Eduardo, 3-oct: «getters, sin ABI»; 7-oct: «la variante») — LAS TUPLAS DENTRO DE UNA NATIVE ──
     * Una tupla es un objeto `__Tuple_<forma>`, y una native no lee campos: la lee y la construye llamando por el puente
     * a funciones OCULTAS de su propio módulo, que sintetiza el compilador de bytecode (MivmEmitter,
     * synthesizeTupleAccessorsParaNatives): `__tup_get_<forma>_<i>(t)` y `__tup_new_<forma>(a0, …)`. Ninguna clase gana
     * ranuras, y sirven para una tupla que venga de otro módulo (la de `Str.parseInt`): el layout es el mismo por forma.
     * Sólo formas que caben en el puente (4 bytes por valor); una con float, long o double, error al compilar. Cada
     * campo, una llamada al intérprete (como una factoría). */
    private String formaPuente(BpType bt, int line, String gesto) {
        if (!(bt instanceof BpType.TupleType))
            throw new UnsupportedAotException("AOT: " + gesto + " sin el tipo de la tupla resuelto (line " + line + ")");
        String forma = MivmEmitter.formaDeTupla((BpType.TupleType) bt);
        if (!MivmEmitter.formaCabeEnElPuente(forma))
            throw new UnsupportedAotException("AOT: " + gesto + " en la native '" + currentFuncName + "' (line " + line
                + "): la tupla lleva float, long o double, y el puente native→BP sólo lleva valores de 4 bytes (integer,"
                + " boolean, string, referencias). Desestructúrala en BP y pásale los campos a la native.");
        return forma;
    }

    /** `{ a, b } := expr` dentro de una native: la ref de la tupla a un local de C (el GC escanea la pila de la native),
     *  y cada campo, por su accesor, a su variable local —del MISMO tipo que el campo: convertir aquí metería en C una
     *  conversión que el `.mdn` no puede llevar—. `_` se salta. */
    private void emitDestructAssign(Ast.DestructAssignStmt d) {
        BpType bt = tipoDe(d.value);
        String forma = formaPuente(bt, d.line, "desestructurar una tupla");
        BpType.TupleType tt = (BpType.TupleType) bt;
        String t = "__tup" + (++tempBuiltin);
        indent();
        w.print("int32_t " + t + " = ");
        emitExpr(d.value);
        w.println(";");
        int n = Math.min(d.targets.size(), tt.elements.size());
        for (int i = 0; i < n; i++) {
            Ast.IExpr tg = d.targets.get(i);
            String nombre = (tg instanceof Ast.IdentifierExpr) ? ((Ast.IdentifierExpr) tg).name : null;
            if ("_".equals(nombre)) continue;
            if (nombre == null || !localNames.contains(nombre))
                throw new UnsupportedAotException("AOT: desestructurar una tupla en la native '" + currentFuncName
                    + "' (line " + d.line + "): cada destino ha de ser una variable local de la native");
            BpType el = tt.elements.get(i);
            BpType tv = tipoDe(tg);
            if (tv != null && !tv.sameAs(el))
                throw new UnsupportedAotException("AOT: desestructurar una tupla en la native '" + currentFuncName
                    + "' (line " + d.line + "): '" + nombre + "' es " + tv.display() + " y el campo " + i + " es " + el.display()
                    + ": en una native, del mismo tipo (convierte después)");
            indent();
            w.print(cVar(nombre) + " = ");
            emitLlamadaPuente(moduleName + "." + MivmEmitter.tupGetName(forma, i), new String[] { t }, 1, isRefBp(el));
            w.println(";");
        }
        warnings.add("la función native '" + currentFuncName + "' desestructura una tupla (línea " + d.line
            + "): cada campo cruza al intérprete por el puente native→BP");
    }

    /** `(a, b, …)` dentro de una native: la construye su accesor `__tup_new_<forma>`, y deja la ref. */
    private void emitTupleExpr(Ast.TupleExpr te) {
        BpType bt = tipoDe(te);
        String forma = formaPuente(bt, te.line, "construir una tupla");
        BpType.TupleType tt = (BpType.TupleType) bt;
        int mask = 0;
        for (int i = 0; i < tt.elements.size(); i++) if (isRefBp(tt.elements.get(i))) mask |= (1 << i);
        emitCallBpEmission(moduleName + "." + MivmEmitter.tupNewName(forma), te.elements, te.line,
            "el constructor de la tupla " + forma, mask, true);
    }

    /** La llamada por el puente con argumentos que ya son C (no expresiones del `.bp`): como emitCallBpEmission. */
    private void emitLlamadaPuente(String qualified, String[] cArgs, int refMask, boolean retIsRef) {
        String aviso = "AOT: la native '" + currentFuncName + "' pide '" + qualified
            + "' y no está en los módulos cargados: recompila '" + moduleName + "'";
        w.print("vm->aot_helpers->call_bp_i32(vm, aot_" + moduleName + "_busca(vm, \"" + qualified + "\", \""
            + cEscapeBytes(aviso.getBytes(java.nio.charset.StandardCharsets.UTF_8)) + "\"), ");
        w.print("(int32_t[]){ " + String.join(", ", cArgs) + " }, " + cArgs.length + ", " + refMask + "u, "
            + (retIsRef ? 1 : 0) + ")");
    }

    private void emitCallBpEmission(String qualified, List<Ast.IExpr> args, int line,
                                    String targetDesc, int refMask, boolean retIsRef) {
        warnings.add("la función native '" + currentFuncName + "' llama a " + targetDesc
            + " (línea " + line + "). Esa llamada cruza al intérprete por el puente "
            + "native→BP y NO se acelera por AOT.");
        /* #302 paso 2 — ref_mask + ret_is_ref: los args-ref se ensanchan a 8 bytes
         * con regen en el puente y el retorno-ref popea 8 (ver bridge_run_bp_frame). */
        String tail = ", " + refMask + "u, " + (retIsRef ? 1 : 0) + ")";
        String aviso = "AOT: la native '" + currentFuncName + "' pide '" + qualified
            + "' y no está en los módulos cargados (¿otra versión de la dependencia?): recompila '"
            + moduleName + "'";
        w.print("vm->aot_helpers->call_bp_i32(vm, aot_" + moduleName + "_busca(vm, \"" + qualified + "\", \""
            + cEscapeBytes(aviso.getBytes(java.nio.charset.StandardCharsets.UTF_8)) + "\"), ");
        int n = args.size();
        if (n == 0) {
            w.print("(const int32_t*) 0, 0" + tail);
            return;
        }
        w.print("(int32_t[]){ ");
        for (int i = 0; i < n; i++) {
            if (i > 0) w.print(", ");
            emitExpr(args.get(i));
        }
        w.print(" }, " + n + tail);
    }

    /** #211 — ¿el tipo se representa como un i32 de 4 bytes que el puente
     *  call_bp_i32 puede marshallar tal cual? (integer/boolean/string/array
     *  → cType int32_t). float/long/double/void → no. */
    private boolean isBridgeI32Type(Ast.TypeRef t) {
        if (t == null) return false;   /* void */
        try { return "int32_t".equals(cType(t)); }
        catch (UnsupportedAotException e) { return false; }
    }

    /** #169 — variante BpType (firma de una FunctionSymbol cross-module).
     *  i32 = integer/boolean/string/narrow-int + refs (array/clase).
     *  float/long/double/void → no. */
    private boolean isBridgeI32Type(BpType t) {
        if (t == null) return false;   /* void */
        if (t instanceof PrimitiveType) {
            switch (((PrimitiveType) t).tag) {
                case FLOAT: case LONG: case DOUBLE: return false;
                default: return true;   /* integer/string/boolean/narrow → i32 */
            }
        }
        return true;   /* ArrayType / ClassType / ref → handle i32 de 4 bytes */
    }

    private String cBinaryOp(String bpOp) {
        switch (bpOp) {
            case "+":  return "+";
            case "-":  return "-";
            case "*":  return "*";
            case "/":  return "/";
            case "mod": case "%": return "%";
            case "==": return "==";
            case "!=": return "!=";
            case "<":  return "<";
            case ">":  return ">";
            case "<=": return "<=";
            case ">=": return ">=";
            case "and": return "&&";
            case "or":  return "||";
            case "&":  return "&";
            case "|":  return "|";
            case "xor": return "^";
            case "shl": return "<<";
            case "shr": return ">>";
            default:
                throw new UnsupportedAotException("AOT: binary op '" + bpOp + "' no soportado");
        }
    }

    /** Despacha CallExpr a un builtin si el nombre matchea uno conocido.
     *  Devuelve true si emitió código, false si no es builtin (caller
     *  prueba con native func). H3 #168.
     *
     *  NOTA sobre length(): BP usa `arr.length()` (method call), NO
     *  `len(arr)`. El AOT lo soportará cuando implementemos method
     *  dispatch en #174 — entonces detectaremos MemberAccessExpr
     *  "length" sobre target array y emitiremos array_length helper. */
    private boolean emitBuiltinCall(String name, List<Ast.IExpr> args) {
        switch (name) {
            case "now":
                if (args.size() != 0) {
                    throw new UnsupportedAotException(
                        "AOT: now() no toma argumentos");
                }
                w.print("vm->aot_helpers->now_ms(vm)");
                return true;
            /* #173 — builtins de string (funciones libres en BP). */
            case "charAt":
                requireArgc(name, args, 2);
                w.print("vm->aot_helpers->string_char_at(vm, ");
                emitExpr(args.get(0)); w.print(", "); emitExpr(args.get(1));
                w.print(")");
                return true;
            case "charCodeAt":
                requireArgc(name, args, 2);
                w.print("vm->aot_helpers->string_char_code_at(vm, ");
                emitExpr(args.get(0)); w.print(", "); emitExpr(args.get(1));
                w.print(")");
                return true;
            case "substring":
                requireArgc(name, args, 3);
                w.print("vm->aot_helpers->string_substring(vm, ");
                emitExpr(args.get(0)); w.print(", "); emitExpr(args.get(1));
                w.print(", "); emitExpr(args.get(2));
                w.print(")");
                return true;
            case "intToString":
                requireArgc(name, args, 1);
                w.print("vm->aot_helpers->int_to_string(vm, ");
                emitExpr(args.get(0));
                w.print(")");
                return true;
            /* [V6/N1.5] Crear un array DE UN TAMAÑO. El literal `[a,b,c]` sirve
             * cuando se conocen los elementos; esto es lo que hace falta cuando
             * lo que se conoce es el tamaño — que en una native es lo normal
             * (un buffer). Cada uno a su helper, por el ancho del elemento:
             * un `long[]` de 8 bytes por casilla y un `byte[]` de 1 son arrays
             * DISTINTOS, no el mismo con otro nombre. */
            case "newIntArray":    return newArrayBuiltin(name, args, "i32");
            case "newByteArray":   return newArrayBuiltin(name, args, "i8");
            case "newLongArray":   return newArrayBuiltin(name, args, "i64");
            case "newDoubleArray": return newArrayBuiltin(name, args, "i64");   /* 8 B opacos */
            /* `len(x)` son dos sobrecargas de la raiz (H1 P2c, 20-sep: `len(s: string)`
             * y `len(a: any[])`, en Core.bp); aqui se decide por el TIPO del argumento,
             * que es lo que el semantico ya resolvio. */
            case "len": {
                requireArgc(name, args, 1);
                Ast.IExpr x = args.get(0);
                BpType t = (semInfo != null) ? semInfo.exprTypes.get(x) : null;
                if (t instanceof ArrayType) {
                    w.print("vm->aot_helpers->array_length(vm, (uint32_t) (");
                    emitExpr(x); w.print("))");
                    return true;
                }
                if (isStringExpr(x)) {
                    w.print("vm->aot_helpers->string_length(vm, (uint32_t) (");
                    emitExpr(x); w.print("))");
                    return true;
                }
                throw new UnsupportedAotException(
                    "AOT: len() en native soporta arrays y strings "
                    + "(line " + ((Ast.Node) x).line + ").");
            }
            default:
                return false;
        }
    }

    /** [V6/N1.5] `newXArray(n)` → el helper de reserva del ancho que toca. */
    private boolean newArrayBuiltin(String name, List<Ast.IExpr> args, String sufijo) {
        requireArgc(name, args, 1);
        w.print("vm->aot_helpers->newarray_" + sufijo + "(vm, ");
        emitExpr(args.get(0));
        w.print(")");
        return true;
    }

    /** #173 — valida nº de args de un builtin; lanza si no cuadra. */
    private void requireArgc(String name, List<Ast.IExpr> args, int n) {
        if (args.size() != n) {
            throw new UnsupportedAotException(
                "AOT: builtin '" + name + "' espera " + n + " args, recibió "
                + args.size());
        }
    }

    // ==================== Helpers ====================

    /** #302 paso 2 — ¿este tipo cruza la frontera AOT como REFERENCIA de 8 bytes?
     *  Fuente de verdad = MivmEmitter.occupies8Bytes sobre el BpType RESUELTO: es
     *  el mismo predicado que decide cuántos bytes empuja OP_CALL, así el thunk lee
     *  exactamente lo que el intérprete escribió. El `!is8Byte` deja fuera
     *  long/double (que el AOT rechaza antes en cType). El enum NO es ref
     *  (isScalar, isReference()=false → 4 bytes), a diferencia de una clase. */
    private static boolean isRefBp(BpType t) {
        return t != null && MivmEmitter.occupies8Bytes(t) && !MivmEmitter.is8Byte(t);
    }

    /** #302 paso 2 — ref_mask de una lista de parámetros: bit i = el param i es
     *  una referencia (8 bytes). Lo consumen call_bp_i32 / call_method_i32. */
    private static int refMaskOfParams(List<Symbol.ParamSymbol> ps) {
        int m = 0;
        for (int i = 0; i < ps.size(); i++) if (isRefBp(ps.get(i).type)) m |= (1 << i);
        return m;
    }

    /** #302 paso 2 — la FunctionSymbol de un FuncDef (tipos resueltos), o null si
     *  no hay info semántica. `declSymbols` la mapea al declarar la función. */
    private Symbol.FunctionSymbol funcSym(Ast.FuncDef f) {
        if (semInfo == null) return null;
        Symbol s = semInfo.declSymbols.get(f);
        return (s instanceof Symbol.FunctionSymbol) ? (Symbol.FunctionSymbol) s : null;
    }

    /** #302 paso 2 — ¿el parámetro i-ésimo del thunk es una ref (8 bytes)? Prefiere
     *  el BpType resuelto (distingue enum de clase); sin info semántica cae al
     *  mirror por Ast.TypeRef (string/array/clase → ref; puede errar en enum, caso
     *  que no ocurre sin semInfo en la práctica). */
    private boolean paramIsRef(Ast.FuncDef f, int i) {
        Symbol.FunctionSymbol fs = funcSym(f);
        if (fs != null && i < fs.params.size()) return isRefBp(fs.params.get(i).type);
        return isRefTypeRefFallback(f.params.get(i).type);
    }

    /** #302 paso 2 — ¿el retorno del thunk es una ref (8 bytes)? */
    private boolean returnIsRef(Ast.FuncDef f) {
        if (f.returnType == null) return false;   /* void */
        Symbol.FunctionSymbol fs = funcSym(f);
        if (fs != null) return isRefBp(fs.returnType);
        return isRefTypeRefFallback(f.returnType);
    }

    /** #302 paso 2 — ref_mask de un FuncDef (para el puente call_bp_i32 a una
     *  función BP del mismo módulo). Prefiere BpType; si no, mirror por Ast. */
    private int refMaskOfFuncDef(Ast.FuncDef f) {
        Symbol.FunctionSymbol fs = funcSym(f);
        if (fs != null) return refMaskOfParams(fs.params);
        int m = 0;
        for (int i = 0; i < f.params.size(); i++)
            if (isRefTypeRefFallback(f.params.get(i).type)) m |= (1 << i);
        return m;
    }

    /** Fallback sin BpType: string/array/clase-o-any (default de cType) → ref. */
    private boolean isRefTypeRefFallback(Ast.TypeRef t) {
        if (t instanceof Ast.ArrayTypeRef) return true;
        if (t instanceof Ast.TupleTypeRef) return true;   /* V7 #577: una tupla es un objeto */
        if (t instanceof Ast.SimpleTypeRef) {
            String n = ((Ast.SimpleTypeRef) t).name;
            switch (n) {
                case "integer": case "float": case "boolean": return false;
                case "long": case "double": return false;   /* rechazados en cType */
                case "string": return true;
                default: return true;   /* clase/any (enum se maneja con BpType) */
            }
        }
        return false;
    }

    /** TypeRef BP → tipo C. null (sin tipo de retorno) → void.
     *  ArrayTypeRef se trata como handle i32 (el ref al heap donde
     *  vive el array). El acceso a elementos pasa por helpers. */
    private String cType(Ast.TypeRef t) {
        if (t == null) return "void";
        if (t instanceof Ast.SimpleTypeRef) {
            String n = ((Ast.SimpleTypeRef) t).name;
            switch (n) {
                case "integer": return "int32_t";
                case "float":   return "float";
                case "boolean": return "int32_t";   /* bool como i32 0/1 */
                case "string":  return "int32_t";   /* #171: handle al heap. */
                case "long":
                    /* #381 — `long` YA se marshalla: 8 bytes big-endian en la
                     * pila BP, los mismos que escribe el intérprete. El thunk
                     * avanza `sp` de 8 en 8 con read_i64_be/write_i64_be. */
                    return "int64_t";
                case "double":
                    /* #426 (V6/N1.2) — YA CRUZA. Lo que faltaba no era el
                     * marshalling (ese lo comparte con `long` desde #381: 8 bytes
                     * big-endian) sino su ARITMETICA: se emula por software y las
                     * rutinas de libgcc no caben en un `.mdn`, que se carga sin
                     * enlazador.
                     *
                     * La salida es la de Eduardo: que la ponga el firmware. Las 19
                     * operaciones de `double` viven en la tabla de helpers —dentro
                     * del firmware, que SI esta enlazado contra libgcc— y el emisor
                     * llama a ellas en vez de escribir `a + b`. No se reimplementa
                     * coma flotante: se le presta al `.mdn` la que ya hay. */
                    return "double";
                default:
                    /* #174b — clase/enum/any: ref u valor de 4 bytes → handle i32.
                     * (El análisis semántico ya validó el tipo, así que un nombre
                     * desconocido no llega aquí.) */
                    return "int32_t";
            }
        }
        if (t instanceof Ast.ArrayTypeRef) {
            /* Handle al heap. Element type se traduce a través de los
             * helpers array_load_<T> / array_store_<T> en el AccessExpr. */
            return "int32_t";
        }
        if (t instanceof Ast.TupleTypeRef) {
            /* V7 #577 — una tupla es un objeto: su ref, un handle i32, como una clase. Se lee y se construye por los
             * accesores ocultos del módulo (emitDestructAssign, emitTupleExpr). */
            return "int32_t";
        }
        throw new UnsupportedAotException(
            "AOT: TypeRef no soportado: " + t.getClass().getSimpleName());
    }

    /** Para el thunk: nombre del helper que lee un valor del tipo
     *  indicado desde el stack BP. Cada slot ocupa 4 bytes BE. */
    /**
     * #381 — CUÁNTO OCUPA ESTE TIPO EN LA PILA BP.
     *
     * <p>Cuatro para todo lo de siempre, ocho para `long` — y para `double` el
     * día que entre (#426). Existe como función y no como constante porque el
     * thunk tenía el 4 escrito a mano en dos sitios: ahí es exactamente donde se
     * cuela el fallo mudo si se añade un tipo ancho y se olvida el avance del
     * `sp`. Un frame que se descuadra 4 bytes por llamada no da error: da
     * basura, unas llamadas más tarde.
     *
     * <p>Las REFERENCIAS también ocupan 8, pero no pasan por aquí: el thunk las
     * trata aparte con `read_ref`/`write_ref` porque además regeneran la
     * generación del handle (#302).
     */
    private int slotBytes(Ast.TypeRef t) {
        if (t instanceof Ast.SimpleTypeRef) {
            String n = ((Ast.SimpleTypeRef) t).name;
            if ("long".equals(n) || "double".equals(n)) return 8;
        }
        return 4;
    }

    private String readHelper(Ast.TypeRef t) {
        if (t instanceof Ast.SimpleTypeRef) {
            String n = ((Ast.SimpleTypeRef) t).name;
            switch (n) {
                case "integer": case "boolean": case "string":
                    return "read_i32_be";
                case "float":
                    return "read_f32_be";
                case "long":
                    return "read_i64_be";      /* #381 — 8 bytes en la pila BP */
                case "double":
                    return "read_f64_be";     /* #426 — 8 bytes en la pila BP */
                default:
                    /* #174b — clase/enum/any: handle/valor i32. */
                    return "read_i32_be";
            }
        }
        if (t instanceof Ast.ArrayTypeRef) {
            /* El handle es i32. */
            return "read_i32_be";
        }
        throw new UnsupportedAotException(
            "AOT: no hay readHelper para tipo " + (t == null ? "null" : t.getClass().getSimpleName()));
    }

    /** Para el thunk: nombre del helper que escribe un valor del tipo
     *  indicado al stack BP. */
    private String writeHelper(Ast.TypeRef t) {
        if (t instanceof Ast.SimpleTypeRef) {
            String n = ((Ast.SimpleTypeRef) t).name;
            switch (n) {
                case "integer": case "boolean": case "string":
                    return "write_i32_be";
                case "float":
                    return "write_f32_be";
                case "long":
                    return "write_i64_be";      /* #381 — 8 bytes en la pila BP */
                case "double":
                    return "write_f64_be";    /* #426 — 8 bytes en la pila BP */
                default:
                    /* #174b — clase/enum/any: handle/valor i32. */
                    return "write_i32_be";
            }
        }
        if (t instanceof Ast.ArrayTypeRef) {
            return "write_i32_be";
        }
        throw new UnsupportedAotException(
            "AOT: no hay writeHelper para tipo " + (t == null ? "null" : t.getClass().getSimpleName()));
    }

    private void indent() {
        for (int i = 0; i < indentLevel; i++) w.print("    ");
    }

    /** Para test unitario o uso desde IDE. Corre análisis semántico
     *  best-effort para que las ops de string (#173) se resuelvan. */
    public static String emit(Ast.ModuleNode module) {
        AotCEmitter e = new AotCEmitter(module.name);
        try {
            SemanticAnalyzer analyzer = new SemanticAnalyzer();
            e.setSemanticInfo(analyzer.analyze(module));
        } catch (RuntimeException ignore) {
            /* sin info → las ops string-ambiguas caen a numérico */
        }
        return e.emitModule(module);
    }

    /* ============================================================ */
    /*  #172 helpers — layout de variables nivel-módulo              */
    /* ============================================================ */

    /** #172 — las globales de módulo que una native puede leer y escribir (integer, float, boolean y los estrechos,
     *  de 4 bytes; y las constantes integer/float, que ocupan su hueco), con su posición CS-relativa.
     *
     *  [V7/H5 #588, 27-sep] LA POSICIÓN LA DA EL COMPILADOR (MivmEmitter.offsetsDeGlobales: el mismo reparto que
     *  escribe el `.mod`), no una convención propia. Aquí se contaba -4, -8… sólo con las de 4 bytes, y una global
     *  string (8 bytes y su literal detrás), long, double o array por delante descuadraba todas las siguientes: la
     *  native leía y ESCRIBÍA otra palabra, sin un error (`NatGlobales`: `suma` daba 705033164 en vez de 23, y la
     *  escritura se comía las cadenas). Sin información semántica no hay reparto: ninguna global entra, y una native
     *  que toque una lo dice (UnsupportedAotException) en vez de adivinar. */
    private void precomputeModuleVarOffsets(Ast.ModuleNode module) {
        if (semInfo == null) return;
        java.util.Map<String, Integer> reales = MivmEmitter.offsetsDeGlobales(module, semInfo);
        for (Ast.ITopLevelDecl d : module.defs) {
            if (d instanceof Ast.VarDecl) {
                Ast.VarDecl vd = (Ast.VarDecl) d;
                if (!isPrimitiveTypeName(vd.type)) continue;
                boolean esFloat = "float".equals(((Ast.SimpleTypeRef) vd.type).name);
                for (Ast.DeclName dn : vd.names) {
                    Integer off = reales.get(dn.name);
                    if (off == null) continue;
                    moduleVarOffsets.put(dn.name, off);
                    if (esFloat) moduleVarFloat.add(dn.name);
                }
            } else if (d instanceof Ast.ConstDecl) {
                Ast.ConstDecl cd = (Ast.ConstDecl) d;
                /* L8 v2 — sólo las const integer/float literales tienen hueco; las demás se inlinan en cada uso. */
                if (cd.value instanceof Ast.IntLitExpr || cd.value instanceof Ast.FloatLitExpr) {
                    Integer off = reales.get(cd.name.name);
                    if (off == null) continue;
                    moduleVarOffsets.put(cd.name.name, off);
                    if (cd.value instanceof Ast.FloatLitExpr) moduleVarFloat.add(cd.name.name);
                }
            }
        }
    }

    /** #381 — ¿este nombre es una conversión numérica escrita como llamada?
     *  Los cuatro que el lenguaje admite; `double` entra en la lista para poder
     *  RECHAZARLO con su motivo (#426) en vez de dejarlo caer en el genérico
     *  «función desconocida», que no explicaría nada. */
    /** V7/H5 (27-sep) — `byte(x)` y compañía: {mínimo, máximo, el texto del intérprete}, o null si el nombre no es una
     *  de las cuatro. Los rangos y los textos son los de OP_I32_TO_U8/I8/U16/I16 (interp.c, y VirtualMachine en
     *  miVM): si cambian allí, cambian aquí — lo vigila test-aotestrecha. */
    private static String[] conversionEstrecha(String n) {
        switch (n) {
            case "byte":  return new String[] { "0",      "255",   "I32_TO_U8: valor fuera de rango " };
            case "int8":  return new String[] { "-128",   "127",   "I32_TO_I8: valor fuera de rango " };
            case "word":  return new String[] { "0",      "65535", "I32_TO_U16: valor fuera de rango " };
            case "int16": return new String[] { "-32768", "32767", "I32_TO_I16: valor fuera de rango " };
            default:      return null;
        }
    }

    private boolean esCastNumerico(String n) {
        return "integer".equals(n) || "long".equals(n)
            || "float".equals(n)   || "double".equals(n);
    }

    /** True si el tipo BP cabe en un slot de 4 bytes (integer/float/bool).
     *  Sólo nombres simples; tipos compuestos (array, class refs) no. */
    private boolean isPrimitiveTypeName(Ast.TypeRef t) {
        if (!(t instanceof Ast.SimpleTypeRef)) return false;
        String s = ((Ast.SimpleTypeRef) t).name;
        return "integer".equals(s) || "int".equals(s)
            || "float".equals(s)
            || "boolean".equals(s) || "bool".equals(s);
    }
}
