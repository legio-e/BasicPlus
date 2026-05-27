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

    /** Contador para nombres únicos en for-loops (avoid colisión de
     *  __end/__step entre fors anidados). */
    private int forCounter = 0;

    /** H3 #158 — si es true, NO emite aot_<Mod>_register (esa función
     *  tiene relocs externas a bpvm_aot_register_by_name + string
     *  literal, que rompen la position-independence del .o standalone).
     *  El loader del .mdn registra automáticamente desde el symtab. */
    private boolean omitRegisterFunc = false;

    private int indentLevel = 0;

    public AotCEmitter(String moduleName) {
        this.moduleName = moduleName;
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
                if (f.isNative && !f.isIntrinsic) {
                    nativeFuncs.add(f);
                    nativeFuncNames.add(f.name.name);
                    nativeFuncDefs.put(f.name.name, f);
                }
            }
        }
        if (nativeFuncs.isEmpty()) return "";

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
    }

    // ==================== Función AOT ====================

    private void emitFunction(Ast.FuncDef f) {
        String fname = "aot_" + moduleName + "_" + f.name.name;
        String cRet  = cType(f.returnType);
        w.print("static " + cRet + " " + fname + "(struct bpvm* vm");
        for (Ast.Param p : f.params) {
            w.print(", " + cType(p.type) + " " + p.name);
        }
        w.println(") {");
        w.println("    (void) vm;   /* no usado en funciones leaf */");
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
            }
            return;
        }
        if (s instanceof Ast.AssignStmt) {
            Ast.AssignStmt a = (Ast.AssignStmt) s;
            /* Target IdentifierExpr: assignment a variable local/param. */
            if (a.target instanceof Ast.IdentifierExpr) {
                indent();
                w.print(((Ast.IdentifierExpr) a.target).name);
                switch (a.op) {
                    case ASSIGN:       w.print(" = ");  break;
                    case PLUS_ASSIGN:  w.print(" += "); break;
                    case MINUS_ASSIGN: w.print(" -= "); break;
                }
                emitExpr(a.value);
                w.println(";");
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
        throw new UnsupportedAotException(
            "AOT: statement no soportado: " + s.getClass().getSimpleName()
            + " (line " + ((Ast.Node) s).line + ")");
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
        if (e instanceof Ast.IdentifierExpr) {
            w.print(((Ast.IdentifierExpr) e).name);
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
            if (!(c.callee instanceof Ast.IdentifierExpr)) {
                throw new UnsupportedAotException(
                    "AOT: call con callee no-identifier no soportado (line " + c.line + ")");
            }
            String name = ((Ast.IdentifierExpr) c.callee).name;

            /* Builtins (H3 #168) — antes que native funcs por si hubiera
             * colisión de nombres. Lista hardcoded por ahora; expansible. */
            if (emitBuiltinCall(name, c.args)) return;

            if (!nativeFuncNames.contains(name)) {
                throw new UnsupportedAotException(
                    "AOT: call a función no-native '" + name + "' (line " + c.line + "). "
                    + "Para AOT v1 todas las funciones llamadas deben ser native del mismo módulo "
                    + "o un builtin soportado (now, len).");
            }
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
     *  prueba con native func). H3 #168. */
    private boolean emitBuiltinCall(String name, List<Ast.IExpr> args) {
        switch (name) {
            case "now":
                if (args.size() != 0) {
                    throw new UnsupportedAotException(
                        "AOT: now() no toma argumentos");
                }
                w.print("vm->aot_helpers->now_ms(vm)");
                return true;
            case "len":
                if (args.size() != 1) {
                    throw new UnsupportedAotException(
                        "AOT: len(arr) toma exactamente un argumento");
                }
                w.print("vm->aot_helpers->array_length(vm, ");
                emitExpr(args.get(0));
                w.print(")");
                return true;
            default:
                return false;
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
                default:
                    throw new UnsupportedAotException(
                        "AOT: tipo '" + n + "' no soportado (solo integer/float/boolean por ahora)");
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
                case "integer": case "boolean": return "read_i32_be";
                case "float":                    return "read_f32_be";
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
                case "integer": case "boolean": return "write_i32_be";
                case "float":                    return "write_f32_be";
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

    /** Para test unitario o uso desde IDE. */
    public static String emit(Ast.ModuleNode module) {
        AotCEmitter e = new AotCEmitter(module.name);
        return e.emitModule(module);
    }
}
