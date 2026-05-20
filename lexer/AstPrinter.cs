// ============================================================
// AstPrinter.cs
// Vuelca un AST de BASICPLUS en formato indentado tipo árbol,
// útil para depurar el parser y comprobar visualmente que la
// estructura de un programa es la esperada.
// ============================================================

using System.Text;

namespace BasicPlus.Lexer;

public static class AstPrinter
{
    public static string Print(AstNode node)
    {
        var sb = new StringBuilder();
        Write(node, sb, 0);
        return sb.ToString();
    }

    private static void Write(object? node, StringBuilder sb, int depth)
    {
        string ind = new string(' ', depth * 2);
        switch (node)
        {
            case null:
                sb.AppendLine($"{ind}<null>");
                break;

            // -------- Módulo / imports --------
            case ModuleNode m:
                sb.AppendLine($"{ind}Module '{m.Name}'  @ {m.Line}:{m.Column}");
                foreach (var imp in m.Imports) Write(imp, sb, depth + 1);
                foreach (var d in m.Defs) Write(d, sb, depth + 1);
                break;
            case ImportNode imp:
                sb.AppendLine($"{ind}Import {string.Join(".", imp.Path)}");
                break;

            // -------- Tipos --------
            case SimpleTypeRef st:
                sb.AppendLine($"{ind}Type {st.Name}");
                break;
            case ArrayTypeRef at:
                sb.AppendLine($"{ind}ArrayType");
                Write(at.Element, sb, depth + 1);
                if (at.Size is not null)
                {
                    sb.AppendLine($"{ind}  size:");
                    Write(at.Size, sb, depth + 2);
                }
                break;

            // -------- Helpers --------
            case DeclName dn:
                sb.AppendLine($"{ind}DeclName {(dn.IsStatic ? dn.ClassQualifier + "." : "")}{dn.Name}");
                break;
            case Param p:
                sb.AppendLine($"{ind}Param {p.Name}");
                Write(p.Type, sb, depth + 1);
                break;
            case EnumValue ev:
                string val = ev.ExplicitValue is { } v ? $" := {v}" : "";
                sb.AppendLine($"{ind}EnumValue {ev.Name}{val}");
                break;
            case GetterDef g:
                sb.AppendLine($"{ind}Getter");
                foreach (var s in g.Body) Write(s, sb, depth + 1);
                break;
            case SetterDef s:
                sb.AppendLine($"{ind}Setter (param={s.ParamName})");
                foreach (var st in s.Body) Write(st, sb, depth + 1);
                break;

            // -------- Declaraciones --------
            case ConstDecl c:
                sb.AppendLine($"{ind}ConstDecl{Mods(c.IsPublic, false)}");
                Write(c.Name, sb, depth + 1);
                if (c.Type is not null) Write(c.Type, sb, depth + 1);
                sb.AppendLine($"{ind}  value:");
                Write(c.Value, sb, depth + 2);
                break;
            case VarDecl v:
                sb.AppendLine($"{ind}VarDecl{Mods(v.IsPublic, false)}");
                foreach (var n in v.Names) Write(n, sb, depth + 1);
                Write(v.Type, sb, depth + 1);
                if (v.Init is not null)
                {
                    sb.AppendLine($"{ind}  init:");
                    Write(v.Init, sb, depth + 2);
                }
                break;
            case FuncDef f:
                sb.AppendLine($"{ind}FuncDef{Mods(f.IsPublic, f.IsFinal)}");
                Write(f.Name, sb, depth + 1);
                foreach (var p in f.Params) Write(p, sb, depth + 1);
                if (f.ReturnType is not null)
                {
                    sb.AppendLine($"{ind}  returns:");
                    Write(f.ReturnType, sb, depth + 2);
                }
                sb.AppendLine($"{ind}  body:");
                foreach (var st in f.Body) Write(st, sb, depth + 2);
                break;
            case PropertyDef pr:
                sb.AppendLine($"{ind}PropertyDef{Mods(pr.IsPublic, pr.IsFinal)}{(pr.IsShortForm ? " [short]" : "")}");
                Write(pr.Name, sb, depth + 1);
                Write(pr.Type, sb, depth + 1);
                if (pr.Init is not null)
                {
                    sb.AppendLine($"{ind}  init:");
                    Write(pr.Init, sb, depth + 2);
                }
                if (pr.Getter is not null) Write(pr.Getter, sb, depth + 1);
                if (pr.Setter is not null) Write(pr.Setter, sb, depth + 1);
                break;
            case ClassDef cd:
                sb.AppendLine($"{ind}ClassDef '{cd.Name}'{(cd.BaseClass is null ? "" : $" extends {cd.BaseClass}")}{Mods(cd.IsPublic, false)}");
                foreach (var m in cd.Members) Write(m, sb, depth + 1);
                break;
            case EnumDef en:
                sb.AppendLine($"{ind}EnumDef '{en.Name}'{Mods(en.IsPublic, false)}");
                foreach (var ev in en.Values) Write(ev, sb, depth + 1);
                break;

            // -------- Sentencias --------
            case AssignStmt a:
                sb.AppendLine($"{ind}AssignStmt op={a.Op}");
                sb.AppendLine($"{ind}  target:");
                Write(a.Target, sb, depth + 2);
                sb.AppendLine($"{ind}  value:");
                Write(a.Value, sb, depth + 2);
                break;
            case IfStmt iff:
                sb.AppendLine($"{ind}IfStmt{(iff.IsSingleLine ? " [single-line]" : "")}");
                sb.AppendLine($"{ind}  then:");
                Write(iff.Then, sb, depth + 2);
                foreach (var ei in iff.ElseIfs)
                {
                    sb.AppendLine($"{ind}  elseif:");
                    Write(ei, sb, depth + 2);
                }
                if (iff.Else is not null)
                {
                    sb.AppendLine($"{ind}  else:");
                    foreach (var st in iff.Else) Write(st, sb, depth + 2);
                }
                break;
            case IfClause ic:
                sb.AppendLine($"{ind}IfClause");
                sb.AppendLine($"{ind}  condition:");
                Write(ic.Condition, sb, depth + 2);
                sb.AppendLine($"{ind}  body:");
                foreach (var st in ic.Body) Write(st, sb, depth + 2);
                break;
            case SwitchStmt sw:
                sb.AppendLine($"{ind}SwitchStmt");
                sb.AppendLine($"{ind}  subject:");
                Write(sw.Subject, sb, depth + 2);
                foreach (var cc in sw.Cases) Write(cc, sb, depth + 1);
                if (sw.Default is not null)
                {
                    sb.AppendLine($"{ind}  default:");
                    foreach (var st in sw.Default) Write(st, sb, depth + 2);
                }
                break;
            case CaseClause cc:
                sb.AppendLine($"{ind}CaseClause");
                sb.AppendLine($"{ind}  values:");
                foreach (var e in cc.Values) Write(e, sb, depth + 2);
                sb.AppendLine($"{ind}  body:");
                foreach (var st in cc.Body) Write(st, sb, depth + 2);
                break;
            case WhileStmt w:
                sb.AppendLine($"{ind}WhileStmt{(w.IsSingleLine ? " [single-line]" : "")}");
                sb.AppendLine($"{ind}  condition:");
                Write(w.Condition, sb, depth + 2);
                sb.AppendLine($"{ind}  body:");
                foreach (var st in w.Body) Write(st, sb, depth + 2);
                break;
            case DoLoopStmt dl:
                sb.AppendLine($"{ind}DoLoopStmt{(dl.Condition is null ? " [infinite]" : "")}");
                sb.AppendLine($"{ind}  body:");
                foreach (var st in dl.Body) Write(st, sb, depth + 2);
                if (dl.Condition is not null)
                {
                    sb.AppendLine($"{ind}  while:");
                    Write(dl.Condition, sb, depth + 2);
                }
                break;
            case ForStmt fs:
                sb.AppendLine($"{ind}ForStmt iter={fs.IteratorName}{(fs.IsSingleLine ? " [single-line]" : "")}");
                Write(fs.Range, sb, depth + 1);
                sb.AppendLine($"{ind}  body:");
                foreach (var st in fs.Body) Write(st, sb, depth + 2);
                break;
            case ForNumericRange fnr:
                sb.AppendLine($"{ind}NumericRange");
                sb.AppendLine($"{ind}  from:"); Write(fnr.From, sb, depth + 2);
                sb.AppendLine($"{ind}  to:");   Write(fnr.To,   sb, depth + 2);
                if (fnr.Step is not null) { sb.AppendLine($"{ind}  step:"); Write(fnr.Step, sb, depth + 2); }
                break;
            case ForInRange fir:
                sb.AppendLine($"{ind}InRange");
                Write(fir.Iterable, sb, depth + 1);
                break;
            case TryStmt tr:
                sb.AppendLine($"{ind}TryStmt");
                sb.AppendLine($"{ind}  body:");
                foreach (var st in tr.Body) Write(st, sb, depth + 2);
                foreach (var cl in tr.Catches) Write(cl, sb, depth + 1);
                if (tr.Finally is not null)
                {
                    sb.AppendLine($"{ind}  finally:");
                    foreach (var st in tr.Finally) Write(st, sb, depth + 2);
                }
                break;
            case CatchClause cl:
                string vn = cl.VarName ?? "_";
                string tn = cl.ExceptionType is null ? "*" : cl.ExceptionType;
                sb.AppendLine($"{ind}Catch (var={vn}, type={tn})");
                foreach (var st in cl.Body) Write(st, sb, depth + 1);
                break;
            case ReturnStmt r:
                sb.AppendLine($"{ind}Return{(r.Value is null ? " (void)" : "")}");
                if (r.Value is not null) Write(r.Value, sb, depth + 1);
                break;
            case ThrowStmt th:
                sb.AppendLine($"{ind}Throw");
                Write(th.Value, sb, depth + 1);
                break;
            case PrintStmt pr2:
                sb.AppendLine($"{ind}Print");
                foreach (var it in pr2.Items)
                {
                    sb.AppendLine($"{ind}  item sep={it.LeadingSep}");
                    if (it.Expr is not null) Write(it.Expr, sb, depth + 2);
                }
                break;
            case BreakStmt:    sb.AppendLine($"{ind}Break"); break;
            case ContinueStmt: sb.AppendLine($"{ind}Continue"); break;
            case ExprStmt es:
                sb.AppendLine($"{ind}ExprStmt");
                Write(es.Expr, sb, depth + 1);
                break;

            // -------- Expresiones --------
            case IntLitExpr i:    sb.AppendLine($"{ind}Int    {i.Value}"); break;
            case FloatLitExpr fl: sb.AppendLine($"{ind}Float  {fl.Value}"); break;
            case StringLitExpr s2:sb.AppendLine($"{ind}String \"{Escape(s2.Value)}\""); break;
            case BoolLitExpr b:   sb.AppendLine($"{ind}Bool   {b.Value}"); break;
            case NullLitExpr:     sb.AppendLine($"{ind}Null"); break;
            case ThisExpr:        sb.AppendLine($"{ind}This"); break;
            case SuperExpr:       sb.AppendLine($"{ind}Super"); break;
            case FieldExpr:       sb.AppendLine($"{ind}Field"); break;
            case SuperCallExpr sc:
                sb.AppendLine($"{ind}SuperCall");
                foreach (var a in sc.Args) Write(a, sb, depth + 1);
                break;
            case IdentifierExpr id:
                sb.AppendLine($"{ind}Ident  {id.Name}");
                break;
            case ArrayLitExpr al:
                sb.AppendLine($"{ind}ArrayLit ({al.Elements.Count})");
                foreach (var e in al.Elements) Write(e, sb, depth + 1);
                break;
            case ParenExpr pe:
                sb.AppendLine($"{ind}Paren");
                Write(pe.Inner, sb, depth + 1);
                break;
            case MemberAccessExpr ma:
                sb.AppendLine($"{ind}Member .{ma.Member}");
                Write(ma.Target, sb, depth + 1);
                break;
            case IndexExpr ix:
                sb.AppendLine($"{ind}Index");
                sb.AppendLine($"{ind}  target:");
                Write(ix.Target, sb, depth + 2);
                sb.AppendLine($"{ind}  index:");
                Write(ix.Index, sb, depth + 2);
                break;
            case CallExpr ce:
                sb.AppendLine($"{ind}Call");
                sb.AppendLine($"{ind}  callee:");
                Write(ce.Callee, sb, depth + 2);
                if (ce.Args.Count > 0)
                {
                    sb.AppendLine($"{ind}  args:");
                    foreach (var a in ce.Args) Write(a, sb, depth + 2);
                }
                break;
            case UnaryExpr u:
                sb.AppendLine($"{ind}Unary {u.Op}");
                Write(u.Operand, sb, depth + 1);
                break;
            case BinaryExpr bin:
                sb.AppendLine($"{ind}Binary {bin.Op}");
                Write(bin.Left, sb, depth + 1);
                Write(bin.Right, sb, depth + 1);
                break;

            default:
                sb.AppendLine($"{ind}<{node?.GetType().Name ?? "?"}>");
                break;
        }
    }

    private static string Mods(bool isPublic, bool isFinal)
    {
        var sb = new StringBuilder();
        if (isPublic) sb.Append(" [public]");
        if (isFinal)  sb.Append(" [final]");
        return sb.ToString();
    }

    private static string Escape(string s) =>
        s.Replace("\\", "\\\\")
         .Replace("\"", "\\\"")
         .Replace("\n", "\\n")
         .Replace("\t", "\\t")
         .Replace("\r", "\\r");
}
