// ============================================================
// AstPrinter.java
// Vuelca un AST de BASICPLUS en formato indentado tipo árbol.
// Port a Java 8 del AstPrinter.cs.
// ============================================================
package basicplus.frontend;

import basicplus.frontend.Ast.*;

public final class AstPrinter {

    private AstPrinter() {}

    public static String print(Object node) {
        StringBuilder sb = new StringBuilder();
        write(node, sb, 0);
        return sb.toString();
    }

    private static String indent(int depth) {
        char[] buf = new char[depth * 2];
        for (int i = 0; i < buf.length; i++) buf[i] = ' ';
        return new String(buf);
    }

    private static String mods(boolean isPublic, boolean isFinal) {
        StringBuilder sb = new StringBuilder();
        if (isPublic) sb.append(" [public]");
        if (isFinal)  sb.append(" [final]");
        return sb.toString();
    }

    private static String escape(String s) {
        return s.replace("\\", "\\\\")
                .replace("\"", "\\\"")
                .replace("\n", "\\n")
                .replace("\t", "\\t")
                .replace("\r", "\\r");
    }

    private static void write(Object node, StringBuilder sb, int depth) {
        String ind = indent(depth);

        if (node == null) { sb.append(ind).append("<null>\n"); return; }

        // ---------- Módulo / imports ----------
        if (node instanceof ModuleNode) {
            ModuleNode m = (ModuleNode) node;
            sb.append(ind).append("Module '").append(m.name).append("'  @ ")
                    .append(m.line).append(":").append(m.column).append("\n");
            for (ImportNode imp : m.imports) write(imp, sb, depth + 1);
            for (ITopLevelDecl d : m.defs)   write(d,   sb, depth + 1);
            return;
        }
        if (node instanceof ImportNode) {
            ImportNode i = (ImportNode) node;
            sb.append(ind).append("Import ").append(String.join(".", i.path)).append("\n");
            return;
        }

        // ---------- Tipos ----------
        if (node instanceof SimpleTypeRef) {
            sb.append(ind).append("Type ").append(((SimpleTypeRef) node).name).append("\n");
            return;
        }
        if (node instanceof ArrayTypeRef) {
            ArrayTypeRef a = (ArrayTypeRef) node;
            sb.append(ind).append("ArrayType\n");
            write(a.element, sb, depth + 1);
            if (a.size != null) {
                sb.append(ind).append("  size:\n");
                write(a.size, sb, depth + 2);
            }
            return;
        }

        // ---------- Helpers ----------
        if (node instanceof DeclName) {
            DeclName dn = (DeclName) node;
            String prefix = dn.isStatic() ? dn.classQualifier + "." : "";
            sb.append(ind).append("DeclName ").append(prefix).append(dn.name).append("\n");
            return;
        }
        if (node instanceof Param) {
            Param p = (Param) node;
            sb.append(ind).append("Param ").append(p.name).append("\n");
            write(p.type, sb, depth + 1);
            return;
        }
        if (node instanceof EnumValue) {
            EnumValue ev = (EnumValue) node;
            String val = (ev.explicitValue != null) ? " := " + ev.explicitValue : "";
            sb.append(ind).append("EnumValue ").append(ev.name).append(val).append("\n");
            return;
        }
        if (node instanceof GetterDef) {
            GetterDef g = (GetterDef) node;
            sb.append(ind).append("Getter\n");
            for (IStmt s : g.body) write(s, sb, depth + 1);
            return;
        }
        if (node instanceof SetterDef) {
            SetterDef s = (SetterDef) node;
            sb.append(ind).append("Setter (param=").append(s.paramName).append(")\n");
            for (IStmt st : s.body) write(st, sb, depth + 1);
            return;
        }

        // ---------- Declaraciones ----------
        if (node instanceof ConstDecl) {
            ConstDecl c = (ConstDecl) node;
            sb.append(ind).append("ConstDecl").append(mods(c.isPublic, false)).append("\n");
            write(c.name, sb, depth + 1);
            if (c.type != null) write(c.type, sb, depth + 1);
            sb.append(ind).append("  value:\n");
            write(c.value, sb, depth + 2);
            return;
        }
        if (node instanceof VarDecl) {
            VarDecl v = (VarDecl) node;
            sb.append(ind).append("VarDecl").append(mods(v.isPublic, false)).append("\n");
            for (DeclName n : v.names) write(n, sb, depth + 1);
            write(v.type, sb, depth + 1);
            if (v.init != null) {
                sb.append(ind).append("  init:\n");
                write(v.init, sb, depth + 2);
            }
            return;
        }
        if (node instanceof FuncDef) {
            FuncDef f = (FuncDef) node;
            sb.append(ind).append("FuncDef").append(mods(f.isPublic, f.isFinal)).append("\n");
            write(f.name, sb, depth + 1);
            for (Param p : f.params) write(p, sb, depth + 1);
            if (f.returnType != null) {
                sb.append(ind).append("  returns:\n");
                write(f.returnType, sb, depth + 2);
            }
            sb.append(ind).append("  body:\n");
            for (IStmt st : f.body) write(st, sb, depth + 2);
            return;
        }
        if (node instanceof PropertyDef) {
            PropertyDef pr = (PropertyDef) node;
            sb.append(ind).append("PropertyDef")
                    .append(mods(pr.isPublic, pr.isFinal))
                    .append(pr.isShortForm ? " [short]" : "").append("\n");
            write(pr.name, sb, depth + 1);
            write(pr.type, sb, depth + 1);
            if (pr.init != null) {
                sb.append(ind).append("  init:\n");
                write(pr.init, sb, depth + 2);
            }
            if (pr.getter != null) write(pr.getter, sb, depth + 1);
            if (pr.setter != null) write(pr.setter, sb, depth + 1);
            return;
        }
        if (node instanceof ClassDef) {
            ClassDef cd = (ClassDef) node;
            sb.append(ind).append("ClassDef '").append(cd.name).append("'");
            if (cd.baseClass != null) sb.append(" extends ").append(cd.baseClass);
            sb.append(mods(cd.isPublic, false)).append("\n");
            for (ITopLevelDecl m : cd.members) write(m, sb, depth + 1);
            return;
        }
        if (node instanceof EnumDef) {
            EnumDef en = (EnumDef) node;
            sb.append(ind).append("EnumDef '").append(en.name).append("'")
                    .append(mods(en.isPublic, false)).append("\n");
            for (EnumValue ev : en.values) write(ev, sb, depth + 1);
            return;
        }

        // ---------- Sentencias ----------
        if (node instanceof AssignStmt) {
            AssignStmt a = (AssignStmt) node;
            sb.append(ind).append("AssignStmt op=").append(a.op).append("\n");
            sb.append(ind).append("  target:\n");
            write(a.target, sb, depth + 2);
            sb.append(ind).append("  value:\n");
            write(a.value, sb, depth + 2);
            return;
        }
        if (node instanceof IfStmt) {
            IfStmt iff = (IfStmt) node;
            sb.append(ind).append("IfStmt").append(iff.isSingleLine ? " [single-line]" : "").append("\n");
            sb.append(ind).append("  then:\n");
            write(iff.then_, sb, depth + 2);
            for (IfClause ei : iff.elseIfs) {
                sb.append(ind).append("  elseif:\n");
                write(ei, sb, depth + 2);
            }
            if (iff.else_ != null) {
                sb.append(ind).append("  else:\n");
                for (IStmt st : iff.else_) write(st, sb, depth + 2);
            }
            return;
        }
        if (node instanceof IfClause) {
            IfClause ic = (IfClause) node;
            sb.append(ind).append("IfClause\n");
            sb.append(ind).append("  condition:\n");
            write(ic.condition, sb, depth + 2);
            sb.append(ind).append("  body:\n");
            for (IStmt st : ic.body) write(st, sb, depth + 2);
            return;
        }
        if (node instanceof SwitchStmt) {
            SwitchStmt sw = (SwitchStmt) node;
            sb.append(ind).append("SwitchStmt\n");
            sb.append(ind).append("  subject:\n");
            write(sw.subject, sb, depth + 2);
            for (CaseClause cc : sw.cases) write(cc, sb, depth + 1);
            if (sw.defaultBody != null) {
                sb.append(ind).append("  default:\n");
                for (IStmt st : sw.defaultBody) write(st, sb, depth + 2);
            }
            return;
        }
        if (node instanceof CaseClause) {
            CaseClause cc = (CaseClause) node;
            sb.append(ind).append("CaseClause\n");
            sb.append(ind).append("  values:\n");
            for (IExpr e : cc.values) write(e, sb, depth + 2);
            sb.append(ind).append("  body:\n");
            for (IStmt st : cc.body) write(st, sb, depth + 2);
            return;
        }
        if (node instanceof WhileStmt) {
            WhileStmt w = (WhileStmt) node;
            sb.append(ind).append("WhileStmt").append(w.isSingleLine ? " [single-line]" : "").append("\n");
            sb.append(ind).append("  condition:\n");
            write(w.condition, sb, depth + 2);
            sb.append(ind).append("  body:\n");
            for (IStmt st : w.body) write(st, sb, depth + 2);
            return;
        }
        if (node instanceof DoLoopStmt) {
            DoLoopStmt dl = (DoLoopStmt) node;
            sb.append(ind).append("DoLoopStmt").append(dl.condition == null ? " [infinite]" : "").append("\n");
            sb.append(ind).append("  body:\n");
            for (IStmt st : dl.body) write(st, sb, depth + 2);
            if (dl.condition != null) {
                sb.append(ind).append("  while:\n");
                write(dl.condition, sb, depth + 2);
            }
            return;
        }
        if (node instanceof ForStmt) {
            ForStmt fs = (ForStmt) node;
            sb.append(ind).append("ForStmt iter=").append(fs.iteratorName)
                    .append(fs.isSingleLine ? " [single-line]" : "").append("\n");
            write(fs.range, sb, depth + 1);
            sb.append(ind).append("  body:\n");
            for (IStmt st : fs.body) write(st, sb, depth + 2);
            return;
        }
        if (node instanceof ForNumericRange) {
            ForNumericRange fnr = (ForNumericRange) node;
            sb.append(ind).append("NumericRange\n");
            sb.append(ind).append("  from:\n"); write(fnr.from, sb, depth + 2);
            sb.append(ind).append("  to:\n");   write(fnr.to,   sb, depth + 2);
            if (fnr.step != null) { sb.append(ind).append("  step:\n"); write(fnr.step, sb, depth + 2); }
            return;
        }
        if (node instanceof ForInRange) {
            ForInRange fir = (ForInRange) node;
            sb.append(ind).append("InRange\n");
            write(fir.iterable, sb, depth + 1);
            return;
        }
        if (node instanceof TryStmt) {
            TryStmt tr = (TryStmt) node;
            sb.append(ind).append("TryStmt\n");
            sb.append(ind).append("  body:\n");
            for (IStmt st : tr.body) write(st, sb, depth + 2);
            for (CatchClause cl : tr.catches) write(cl, sb, depth + 1);
            if (tr.finallyBody != null) {
                sb.append(ind).append("  finally:\n");
                for (IStmt st : tr.finallyBody) write(st, sb, depth + 2);
            }
            return;
        }
        if (node instanceof CatchClause) {
            CatchClause cl = (CatchClause) node;
            String vn = cl.varName == null ? "_" : cl.varName;
            String tn = cl.exceptionType == null ? "*" : cl.exceptionType;
            sb.append(ind).append("Catch (var=").append(vn).append(", type=").append(tn).append(")\n");
            for (IStmt st : cl.body) write(st, sb, depth + 1);
            return;
        }
        if (node instanceof ReturnStmt) {
            ReturnStmt r = (ReturnStmt) node;
            sb.append(ind).append("Return").append(r.value == null ? " (void)" : "").append("\n");
            if (r.value != null) write(r.value, sb, depth + 1);
            return;
        }
        if (node instanceof ThrowStmt) {
            ThrowStmt th = (ThrowStmt) node;
            sb.append(ind).append("Throw\n");
            write(th.value, sb, depth + 1);
            return;
        }
        if (node instanceof PrintStmt) {
            PrintStmt p = (PrintStmt) node;
            sb.append(ind).append("Print\n");
            for (PrintItem it : p.items) {
                sb.append(ind).append("  item sep=").append(it.leadingSep).append("\n");
                if (it.expr != null) write(it.expr, sb, depth + 2);
            }
            return;
        }
        if (node instanceof BreakStmt)    { sb.append(ind).append("Break\n");    return; }
        if (node instanceof ContinueStmt) { sb.append(ind).append("Continue\n"); return; }
        if (node instanceof ExprStmt) {
            ExprStmt es = (ExprStmt) node;
            sb.append(ind).append("ExprStmt\n");
            write(es.expr, sb, depth + 1);
            return;
        }

        // ---------- Expresiones ----------
        if (node instanceof IntLitExpr) {
            sb.append(ind).append("Int    ").append(((IntLitExpr) node).value).append("\n"); return;
        }
        if (node instanceof FloatLitExpr) {
            sb.append(ind).append("Float  ").append(((FloatLitExpr) node).value).append("\n"); return;
        }
        if (node instanceof StringLitExpr) {
            sb.append(ind).append("String \"").append(escape(((StringLitExpr) node).value)).append("\"\n"); return;
        }
        if (node instanceof BoolLitExpr) {
            sb.append(ind).append("Bool   ").append(((BoolLitExpr) node).value).append("\n"); return;
        }
        if (node instanceof NullLitExpr) { sb.append(ind).append("Null\n"); return; }
        if (node instanceof ThisExpr)    { sb.append(ind).append("This\n"); return; }
        if (node instanceof SuperExpr)   { sb.append(ind).append("Super\n"); return; }
        if (node instanceof FieldExpr)   { sb.append(ind).append("Field\n"); return; }
        if (node instanceof SuperCallExpr) {
            SuperCallExpr sc = (SuperCallExpr) node;
            sb.append(ind).append("SuperCall\n");
            for (IExpr a : sc.args) write(a, sb, depth + 1);
            return;
        }
        if (node instanceof IdentifierExpr) {
            sb.append(ind).append("Ident  ").append(((IdentifierExpr) node).name).append("\n"); return;
        }
        if (node instanceof ArrayLitExpr) {
            ArrayLitExpr al = (ArrayLitExpr) node;
            sb.append(ind).append("ArrayLit (").append(al.elements.size()).append(")\n");
            for (IExpr e : al.elements) write(e, sb, depth + 1);
            return;
        }
        if (node instanceof ParenExpr) {
            sb.append(ind).append("Paren\n");
            write(((ParenExpr) node).inner, sb, depth + 1);
            return;
        }
        if (node instanceof MemberAccessExpr) {
            MemberAccessExpr ma = (MemberAccessExpr) node;
            sb.append(ind).append("Member .").append(ma.member).append("\n");
            write(ma.target, sb, depth + 1);
            return;
        }
        if (node instanceof IndexExpr) {
            IndexExpr ix = (IndexExpr) node;
            sb.append(ind).append("Index\n");
            sb.append(ind).append("  target:\n");
            write(ix.target, sb, depth + 2);
            sb.append(ind).append("  index:\n");
            write(ix.index, sb, depth + 2);
            return;
        }
        if (node instanceof CallExpr) {
            CallExpr ce = (CallExpr) node;
            sb.append(ind).append("Call\n");
            sb.append(ind).append("  callee:\n");
            write(ce.callee, sb, depth + 2);
            if (!ce.args.isEmpty()) {
                sb.append(ind).append("  args:\n");
                for (IExpr a : ce.args) write(a, sb, depth + 2);
            }
            return;
        }
        if (node instanceof UnaryExpr) {
            UnaryExpr u = (UnaryExpr) node;
            sb.append(ind).append("Unary ").append(u.op).append("\n");
            write(u.operand, sb, depth + 1);
            return;
        }
        if (node instanceof BinaryExpr) {
            BinaryExpr b = (BinaryExpr) node;
            sb.append(ind).append("Binary ").append(b.op).append("\n");
            write(b.left, sb, depth + 1);
            write(b.right, sb, depth + 1);
            return;
        }
        if (node instanceof InstanceOfExpr) {
            InstanceOfExpr io = (InstanceOfExpr) node;
            sb.append(ind).append("InstanceOf ").append(io.typeName).append("\n");
            write(io.target, sb, depth + 1);
            return;
        }

        sb.append(ind).append("<").append(node.getClass().getSimpleName()).append(">\n");
    }
}
