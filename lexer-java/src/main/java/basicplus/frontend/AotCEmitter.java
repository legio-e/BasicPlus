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
import java.util.HashSet;
import java.util.List;
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

    /** #211 — nombre de la función AOT que se está emitiendo (para mensajes
     *  de aviso). Fijado al inicio de cada emitFunction. */
    private String currentFuncName = "?";

    /** Contador para nombres únicos en for-loops (avoid colisión de
     *  __end/__step entre fors anidados). */
    private int forCounter = 0;

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
    private final java.util.Map<String, Integer> moduleVarOffsets =
            new java.util.LinkedHashMap<>();

    /** #172 — true si alguna función emitida hasta ahora consume globals.
     *  Cuando es true, el header del .c incluye:
     *    - static uint32_t s_module_cs = 0;
     *    - static inline uint32_t aot_<Mod>_cs(struct bpvm* vm) { ... }
     *  Y cada función AOT empieza con: `uint32_t cs = aot_<Mod>_cs(vm);`. */
    private boolean usesModuleGlobals = false;

    /** H3 #158 — si es true, NO emite aot_<Mod>_register (esa función
     *  tiene relocs externas a bpvm_aot_register_by_name + string
     *  literal, que rompen la position-independence del .o standalone).
     *  El loader del .mdn registra automáticamente desde el symtab. */
    private boolean omitRegisterFunc = false;

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

    /** #173 — ¿el tipo resuelto de `e` es integer? */
    private boolean isIntExpr(Ast.IExpr e) {
        if (semInfo == null) return false;
        BpType t = semInfo.exprTypes.get(e);
        return (t instanceof PrimitiveType)
            && ((PrimitiveType) t).tag == PrimitiveType.Kind.INTEGER;
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
                    if (c < 0x20 || c > 0x7e) sb.append(String.format("\\x%02x", c));
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
        if (nativeFuncs.isEmpty()) return "";

        // #172 — Pre-pass: asignar offsets CS-relativos a vars/consts
        // nivel-módulo recorriendo en orden de declaración. MivmEmitter
        // sigue la misma convención (cf. ModWriter.registerSymbol):
        // primer decl encontrado → -4, segundo → -8, etc.
        precomputeModuleVarOffsets(module);

        // Header del archivo.
        emitHeader();

        // Una función + thunk por cada native.
        for (Ast.FuncDef f : nativeFuncs) {
            emitFunction(f);
            emitThunk(f);
        }

        // Función de registro — solo si NO estamos en modo .mdn.
        // En .mdn las relocs de bpvm_aot_register_by_name + string
        // literal contaminan el code section. El loader del firmware
        // registra automáticamente a partir del symtab.
        if (!omitRegisterFunc) {
            emitRegisterFunc(nativeFuncs);
        }
        return out.toString();
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
        w.println("/* Forward decls de las funciones AOT de este módulo. */");
        for (Ast.FuncDef f : nativeFuncDefs.values()) {
            w.print("static " + cType(f.returnType) + " aot_"
                + moduleName + "_" + f.name.name + "(struct bpvm* vm");
            for (Ast.Param p : f.params) {
                w.print(", " + cType(p.type) + " " + p.name);
            }
            w.println(");");
        }
        w.println();

        /* #172 — Si el módulo declara vars/consts a nivel-módulo, emitimos
         * el cache de CS por-módulo. CS es runtime (depende de en qué
         * orden cargue el loader los módulos), así que usamos lazy init.
         * Las accesos a globales se compilan como:
         *     read_i32_be(mem + cs + OFFSET_LITERAL)
         * donde `cs` = aot_<Mod>_cs(vm) cacheado en la static. */
        if (!moduleVarOffsets.isEmpty()) {
            w.println("/* #172 — cache de CS del módulo. Lazy init en la primera");
            w.println(" * invocación de cualquier thunk AOT de este .c. No hace");
            w.println(" * falta sincronización: en runtime single-thread la");
            w.println(" * primera escritura es idempotente; en multi-thread");
            w.println(" * cualquier carrera escribiría el mismo valor (data race");
            w.println(" * benigno tipo \"singly assigned\"). */");
            w.println("static uint32_t s_aot_" + moduleName + "_cs = 0;");
            w.println();
            w.println("static inline uint32_t aot_" + moduleName + "_cs(struct bpvm* vm) {");
            w.println("    uint32_t cs = s_aot_" + moduleName + "_cs;");
            w.println("    if (!cs) {");
            w.println("        cs = vm->aot_helpers->find_module_cs(vm, \""
                    + moduleName + "\");");
            w.println("        if (!cs) {");
            w.println("            vm->aot_helpers->throw_runtime(vm,");
            w.println("                \"AOT: módulo '" + moduleName + "' no encontrado\");");
            w.println("            return 0;");
            w.println("        }");
            w.println("        s_aot_" + moduleName + "_cs = cs;");
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
        String fname = "aot_" + moduleName + "_" + f.name.name;
        String cRet  = cType(f.returnType);
        w.print("static " + cRet + " " + fname + "(struct bpvm* vm");
        for (Ast.Param p : f.params) {
            w.print(", " + cType(p.type) + " " + p.name);
        }
        w.println(") {");
        w.println("    (void) vm;   /* puede no usarse si la función no toca");
        w.println("                  *  globals/arrays/builtins. */");

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
            w.println("    (void) " + p.name + ";");
        }

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

    private boolean endsWithReturn(List<Ast.IStmt> body) {
        if (body.isEmpty()) return false;
        Ast.IStmt last = body.get(body.size() - 1);
        return last instanceof Ast.ReturnStmt;
    }

    // ==================== Thunk ====================

    private void emitThunk(Ast.FuncDef f) {
        String fname = "aot_" + moduleName + "_" + f.name.name;
        String tname = "thunk_" + moduleName + "_" + f.name.name;
        // En modo .mdn el thunk debe ser visible al linker — MdnPack lo
        // busca por nombre en el symtab del .o. Además marcamos `used`
        // para que -Os con -ffunction-sections no lo deade-code-stripe
        // por no tener llamadores en este TU. En modo "linked-in" se
        // mantiene como antes: static, registrado via aot_<Mod>_register.
        if (omitRegisterFunc) {
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
        w.println("    const struct aot_helpers_v1* H = vm->aot_helpers;");
        w.println("    uint8_t* mem = vm->memory;");
        w.println("    uint32_t sp = *sp_p;");
        // Pop args en orden inverso (último pusheado, primero popeado).
        // Cada arg usa el helper de lectura adecuado a su tipo (int/float).
        int n = f.params.size();
        for (int i = n - 1; i >= 0; i--) {
            Ast.Param p = f.params.get(i);
            w.println("    " + cType(p.type) + " a" + i + " = H->"
                + readHelper(p.type) + "(mem + sp - 4); sp -= 4;");
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
        call.append(");");
        w.println(call);
        if (!isVoid) {
            w.println("    H->" + writeHelper(f.returnType)
                + "(mem + sp, r); sp += 4;");
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

    private void emitRegisterFunc(List<Ast.FuncDef> nativeFuncs) {
        w.println("/* Registra todas las funciones AOT de este módulo en el AOT");
        w.println(" * registry. Llamar tras link, antes de bpvm_run. Tolerante a");
        w.println(" * símbolos ausentes (skip silente si el .mod no está cargado). */");
        w.println("void aot_" + moduleName + "_register(struct bpvm* vm) {");
        for (Ast.FuncDef f : nativeFuncs) {
            String qualified = moduleName + "." + f.name.name;
            String tname = "thunk_" + moduleName + "_" + f.name.name;
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
                emitExpr(r.value);
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
                w.print(cVarType + " " + dn.name);
                if (v.init != null) {
                    w.print(" = ");
                    emitExpr(v.init);
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
                if (localNames.contains(tname)) {
                    /* Local — emisión directa estilo C. */
                    w.print(tname);
                    switch (a.op) {
                        case ASSIGN:       w.print(" = ");  break;
                        case PLUS_ASSIGN:  w.print(" += "); break;
                        case MINUS_ASSIGN: w.print(" -= "); break;
                    }
                    emitExpr(a.value);
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
                    if (a.op == Ast.AssignOpKind.ASSIGN) {
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
                indent();
                if (a.op == Ast.AssignOpKind.ASSIGN) {
                    w.print("vm->aot_helpers->array_store_i32(vm, ");
                    emitExpr(ix.target);
                    w.print(", ");
                    emitExpr(ix.index);
                    w.print(", ");
                    emitExpr(a.value);
                    w.println(");");
                } else {
                    /* a[i] += v  →  store(a, i, load(a, i) + v) */
                    String binOp = (a.op == Ast.AssignOpKind.PLUS_ASSIGN) ? "+" : "-";
                    w.print("vm->aot_helpers->array_store_i32(vm, ");
                    emitExpr(ix.target);
                    w.print(", ");
                    emitExpr(ix.index);
                    w.print(", vm->aot_helpers->array_load_i32(vm, ");
                    emitExpr(ix.target);
                    w.print(", ");
                    emitExpr(ix.index);
                    w.print(") " + binOp + " (");
                    emitExpr(a.value);
                    w.println("));");
                }
                return;
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
            for (Ast.IStmt st : wl.body) emitStmt(st);
            indentLevel--;
            indent();
            w.println("}");
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
            /* BP switch → C switch. BP no tiene fall-through implícito
             * (cada case acaba al final del bloque), así que añadimos
             * `break;` automático tras cada case. */
            Ast.SwitchStmt sw = (Ast.SwitchStmt) s;
            indent();
            w.print("switch (");
            emitExpr(sw.subject);
            w.println(") {");
            indentLevel++;
            for (Ast.CaseClause cc : sw.cases) {
                for (Ast.IExpr v : cc.values) {
                    indent();
                    w.print("case ");
                    emitExpr(v);
                    w.println(":");
                }
                indentLevel++;
                for (Ast.IStmt st : cc.body) emitStmt(st);
                indent(); w.println("break;");
                indentLevel--;
            }
            if (sw.defaultBody != null) {
                indent(); w.println("default:");
                indentLevel++;
                for (Ast.IStmt st : sw.defaultBody) emitStmt(st);
                indent(); w.println("break;");
                indentLevel--;
            }
            indentLevel--;
            indent(); w.println("}");
            return;
        }
        if (s instanceof Ast.ThrowStmt) {
            emitThrowStmt((Ast.ThrowStmt) s);
            return;
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
     *  v1 SOLO acepta `throw RuntimeError(<string literal>)`. Un mensaje
     *  no-literal (string runtime) o una clase de usuario != RuntimeError
     *  exigiría construir un objeto en native = frontera native↔intérprete
     *  (v2, junto a #174). En ese caso lanzamos UnsupportedAotException y
     *  el módulo cae a bytecode interpretado (donde sí funciona). */
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
        throw new UnsupportedAotException(
            "AOT: en native `throw` solo soporta RuntimeError(string) "
            + "(clase de usuario y try/catch dentro de native = pendiente #175b). line "
            + ts.line);
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
    private void emitForStmt(Ast.ForStmt fs) {
        if (!(fs.range instanceof Ast.ForNumericRange)) {
            throw new UnsupportedAotException(
                "AOT: for-range no numérico no soportado (line " + fs.line + ")");
        }
        Ast.ForNumericRange r = (Ast.ForNumericRange) fs.range;
        int tag = ++forCounter;
        String it   = fs.iteratorName;
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
        localNames.add(it);

        // while ((step > 0) ? (it <= end) : (it >= end)) { body; it += step; }
        indent();
        w.println("while ((" + step + " > 0) ? (" + it + " <= " + end + ") : ("
                  + it + " >= " + end + ")) {");
        indentLevel++;
        for (Ast.IStmt st : fs.body) emitStmt(st);
        indent(); w.println(it + " += " + step + ";");
        indentLevel--;
        indent(); w.println("}");

        indentLevel--;
        indent();
        w.println("}");
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
        if (e instanceof Ast.FloatLitExpr) {
            /* Emitimos con sufijo 'f' para que gcc no promocione a double.
             * Java Float.toString puede dar "1.0E10" — eso es válido como
             * literal C. */
            double v = ((Ast.FloatLitExpr) e).value;
            w.print(Float.toString((float) v) + "f");
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
                w.print(name);                          // param o local
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
            w.print("vm->aot_helpers->read_i32_be(mem + (uint32_t)((int32_t)cs + ("
                    + off + ")))");
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
            w.print("vm->aot_helpers->array_load_i32(vm, ");
            emitExpr(ix.target);
            w.print(", ");
            emitExpr(ix.index);
            w.print(")");
            return;
        }
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
                        throw new UnsupportedAotException(
                            "AOT: intrínseco cross-module '" + fs.externalQualifiedName()
                            + "' (line " + c.line + ") no soportado en native todavía.");
                    }
                    /* #174b — método PÚBLICO de instancia (virtual) sobre un
                     * receptor: despacho por vtable vía call_method_i32 con el
                     * slot que computa el frontend (ClassSymbol.slotOf, decisión
                     * B). Privado/super/estático van por el throw de abajo (no
                     * virtuales → sin asidero, pendientes). */
                    if (fs.ownerClass != null && !fs.isStatic && fs.isPublic
                            && !(ma.target instanceof Ast.SuperExpr)) {
                        emitVirtualMethodCall(fs, ma, c);
                        return;
                    }
                }
                throw new UnsupportedAotException(
                    "AOT: method call '" + ma.member + "' no soportado todavía (line "
                    + c.line + ") — solo método público (virtual); privado/super/"
                    + "estático/construcción pendientes.");
            }

            if (!(c.callee instanceof Ast.IdentifierExpr)) {
                throw new UnsupportedAotException(
                    "AOT: call con callee no-identifier no soportado (line " + c.line + ")");
            }
            String name = ((Ast.IdentifierExpr) c.callee).name;

            /* Builtins (H3 #168) — antes que native funcs por si hubiera
             * colisión de nombres. Lista hardcoded por ahora; expansible. */
            if (emitBuiltinCall(name, c.args)) return;

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
                throw new UnsupportedAotException(
                    "AOT: call a función desconocida '" + name + "' (line " + c.line + "). "
                    + "Debe ser native o BP del mismo módulo, o un builtin AOT-soportado "
                    + "(now, charAt, charCodeAt, substring, intToString). "
                    + "Cross-module sigue pendiente (#169).");
            }

            /* C call directo a otra función native del mismo módulo.
             * BL PC-relative dentro del mismo blob — gcc lo resuelve
             * sin relocations porque ambas son static en el mismo .o. */
            w.print("aot_" + moduleName + "_" + name + "(vm");
            for (Ast.IExpr arg : c.args) {
                w.print(", ");
                emitExpr(arg);
            }
            w.print(")");
            return;
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
            + "declara '" + name + "' también como native");
    }

    /** #169 — emite una llamada cross-module native→BP (`Mod.func(args)`) por
     *  el puente. La firma viene de la FunctionSymbol externa (.bpi del módulo
     *  dueño). El runtime resuelve la dirección y el CS del módulo destino. */
    private void emitCrossModuleBridgeCall(Symbol.FunctionSymbol fs, Ast.CallExpr c) {
        String qn = fs.externalQualifiedName();
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
        emitCallBpEmission(qn, c.args, c.line, "la función BP cross-module '" + qn + "'");
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
        int slot = (fs.ownerClass != null) ? fs.ownerClass.slotOf(fs.name) : -1;
        if (slot < 0) {
            throw new UnsupportedAotException(
                "AOT: no se pudo resolver el slot de vtable de '" + fs.name + "' (line "
                + c.line + ").");
        }
        if (c.args.size() != fs.params.size()) {
            throw new UnsupportedAotException(
                "AOT: método '" + fs.name + "' (line " + c.line + "): " + c.args.size()
                + " args pero espera " + fs.params.size() + ".");
        }
        warnings.add("la función native '" + currentFuncName + "' invoca el método público '"
            + fs.name + "' (línea " + c.line + "). Esa llamada cruza al intérprete por el "
            + "puente native→BP (dispatch virtual) y NO se acelera por AOT.");
        /* call_method_i32(vm, <this>, slot, (int32_t[]){args...}, n). */
        w.print("vm->aot_helpers->call_method_i32(vm, ");
        emitExpr(ma.target);   // receptor (this)
        w.print(", " + slot + ", ");
        int n = c.args.size();
        if (n == 0) {
            w.print("(const int32_t*) 0, 0)");
            return;
        }
        w.print("(int32_t[]){ ");
        for (int i = 0; i < n; i++) {
            if (i > 0) w.print(", ");
            emitExpr(c.args.get(i));
        }
        w.print(" }, " + n + ")");
    }

    /** Emisión común del puente: aviso + call_bp_i32(vm, find_function(qn),
     *  (int32_t[]){args...}, n). El compound literal C99 vive en el bloque
     *  envolvente — válido como argumento. find_function resuelve el nombre
     *  cada vez (scan barato; cachear en static es mejora futura, pero el coste
     *  del puente domina). */
    private void emitCallBpEmission(String qualified, List<Ast.IExpr> args, int line, String targetDesc) {
        warnings.add("la función native '" + currentFuncName + "' llama a " + targetDesc
            + " (línea " + line + "). Esa llamada cruza al intérprete por el puente "
            + "native→BP y NO se acelera por AOT.");
        w.print("vm->aot_helpers->call_bp_i32(vm, vm->aot_helpers->find_function(vm, \""
            + qualified + "\"), ");
        int n = args.size();
        if (n == 0) {
            w.print("(const int32_t*) 0, 0)");
            return;
        }
        w.print("(int32_t[]){ ");
        for (int i = 0; i < n; i++) {
            if (i > 0) w.print(", ");
            emitExpr(args.get(i));
        }
        w.print(" }, " + n + ")");
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
            default:
                return false;
        }
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
                case "long": case "double":
                    /* 8 bytes: no soportado en signature de native v1. */
                    throw new UnsupportedAotException(
                        "AOT: tipo '" + n + "' (8 bytes) no soportado en signature de native");
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
        throw new UnsupportedAotException(
            "AOT: TypeRef no soportado: " + t.getClass().getSimpleName());
    }

    /** Para el thunk: nombre del helper que lee un valor del tipo
     *  indicado desde el stack BP. Cada slot ocupa 4 bytes BE. */
    private String readHelper(Ast.TypeRef t) {
        if (t instanceof Ast.SimpleTypeRef) {
            String n = ((Ast.SimpleTypeRef) t).name;
            switch (n) {
                case "integer": case "boolean": case "string":
                    return "read_i32_be";
                case "float":
                    return "read_f32_be";
                case "long": case "double":
                    throw new UnsupportedAotException(
                        "AOT: tipo '" + n + "' (8 bytes) no soportado en thunk native");
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
                case "long": case "double":
                    throw new UnsupportedAotException(
                        "AOT: tipo '" + n + "' (8 bytes) no soportado en thunk native");
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

    /** Recorre `module.defs` en orden y asigna offsets CS-relativos a
     *  cada VarDecl / ConstDecl con tipo primitivo. Convención cuadrada
     *  con MivmEmitter / ModWriter.registerSymbol: el primer decl
     *  encontrado obtiene offset -4, el siguiente -8, etc.
     *
     *  v1: solo cuenta integer/float/bool (4 bytes); otros tipos (string,
     *  array, ref) no entran en este path. Limitación documentada — si
     *  un .bp más complejo tiene class descriptors o synthetics
     *  intercalados, los offsets podrían descuadrarse y la AOT cascarse
     *  con datos incoherentes. La convención está fijada en MivmEmitter:
     *  user-vars/consts van primero, __initialized y class descriptors
     *  después (verificado en GlobalsAot.mod). */
    private void precomputeModuleVarOffsets(Ast.ModuleNode module) {
        int nextOffset = -4;
        for (Ast.ITopLevelDecl d : module.defs) {
            if (d instanceof Ast.VarDecl) {
                Ast.VarDecl vd = (Ast.VarDecl) d;
                if (!isPrimitiveTypeName(vd.type)) continue;
                for (Ast.DeclName dn : vd.names) {
                    moduleVarOffsets.put(dn.name, nextOffset);
                    nextOffset -= 4;
                }
            } else if (d instanceof Ast.ConstDecl) {
                Ast.ConstDecl cd = (Ast.ConstDecl) d;
                if (cd.value instanceof Ast.IntLitExpr
                        || cd.value instanceof Ast.FloatLitExpr
                        || cd.value instanceof Ast.BoolLitExpr) {
                    moduleVarOffsets.put(cd.name.name, nextOffset);
                    nextOffset -= 4;
                }
            }
        }
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
