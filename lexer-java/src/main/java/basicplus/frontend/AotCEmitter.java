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

    private int indentLevel = 0;

    public AotCEmitter(String moduleName) {
        this.moduleName = moduleName;
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

        // Función de registro.
        emitRegisterFunc(nativeFuncs);
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
        for (String name : nativeFuncNames) {
            // Por ahora todas las funciones son (vm, i32) → i32. Más
            // adelante el emisor inferirá la firma desde los Params.
            w.println("static int32_t aot_" + moduleName + "_" + name + "(struct bpvm* vm, int32_t arg0);");
        }
        w.println();
    }

    // ==================== Función AOT ====================

    private void emitFunction(Ast.FuncDef f) {
        String fname = "aot_" + moduleName + "_" + f.name.name;
        w.print("static int32_t " + fname + "(struct bpvm* vm");
        for (Ast.Param p : f.params) {
            w.print(", int32_t " + p.name);
        }
        w.println(") {");
        w.println("    (void) vm;   /* no usado en funciones leaf */");
        indentLevel = 1;
        for (Ast.IStmt s : f.body) {
            emitStmt(s);
        }
        // Si la función no termina con return explícito, añadir return 0
        // (solo para void; para typed deberían siempre retornar).
        if (f.returnType != null && !endsWithReturn(f.body)) {
            indent(); w.println("return 0;   /* fall-through default */");
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
        w.println("static void " + tname + "(struct bpvm* vm,");
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
        int n = f.params.size();
        for (int i = n - 1; i >= 0; i--) {
            w.println("    int32_t a" + i + " = H->read_i32_be(mem + sp - 4); sp -= 4;");
        }
        // C call con args en orden original a0, a1, a2...
        StringBuilder call = new StringBuilder();
        call.append("    int32_t r = ").append(fname).append("(vm");
        for (int i = 0; i < n; i++) call.append(", a").append(i);
        call.append(");");
        w.println(call);
        w.println("    H->write_i32_be(mem + sp, r); sp += 4;");
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
            for (Ast.DeclName dn : v.names) {
                indent();
                w.print("int32_t " + dn.name);
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
            if (!(a.target instanceof Ast.IdentifierExpr)) {
                throw new UnsupportedAotException(
                    "AOT: assign target debe ser identifier simple (line " + a.line + ")");
            }
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
        throw new UnsupportedAotException(
            "AOT: statement no soportado: " + s.getClass().getSimpleName()
            + " (line " + ((Ast.Node) s).line + ")");
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
        if (e instanceof Ast.IdentifierExpr) {
            w.print(((Ast.IdentifierExpr) e).name);
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
        if (e instanceof Ast.CallExpr) {
            Ast.CallExpr c = (Ast.CallExpr) e;
            if (!(c.callee instanceof Ast.IdentifierExpr)) {
                throw new UnsupportedAotException(
                    "AOT: call con callee no-identifier no soportado (line " + c.line + ")");
            }
            String name = ((Ast.IdentifierExpr) c.callee).name;
            if (!nativeFuncNames.contains(name)) {
                throw new UnsupportedAotException(
                    "AOT: call a función no-native '" + name + "' (line " + c.line + "). "
                    + "Para AOT v1 todas las funciones llamadas deben ser native del mismo módulo.");
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

    // ==================== Helpers ====================

    private void indent() {
        for (int i = 0; i < indentLevel; i++) w.print("    ");
    }

    /** Para test unitario o uso desde IDE. */
    public static String emit(Ast.ModuleNode module) {
        AotCEmitter e = new AotCEmitter(module.name);
        return e.emitModule(module);
    }
}
