// ============================================================
// IlEmitter.cs
// Compilador de BASICPLUS a IL .NET (.dll) usando Mono.Cecil.
//
// FASE 1 — alcance:
//   - Funciones a NIVEL MÓDULO (no clases todavía).
//   - Tipos primitivos: integer (Int64), float (Double),
//     string (String), boolean (Boolean).
//   - Variables y constantes de módulo (campos estáticos).
//   - Locales y parámetros.
//   - Aritmética, comparación, lógica con corto-circuito.
//   - Asignación :=, += y -=.
//   - if/elseif/else/endif (forma corta y multilínea).
//   - while/endwh, do/loop [cond], for := to step do/next.
//   - print con ',' y ';' (',' = separador espacio, ';' = sin separador).
//   - return [expr], break, continue.
//   - Inicializador de módulo (función con el nombre del módulo) +
//     entry point que la invoca y luego llama a Main().
//
// QUEDA fuera de Fase 1:
//   - Clases, herencia, propiedades, miembros estáticos.
//   - Enums, switch, try/catch/throw, super, this.
//   - Arrays, for ... in ..., null.
//   (Se añadirá en Fase 2.)
//
// El binario producido es un .dll para .NET 8. Para ejecutarlo:
//     dotnet salida.dll
// requiere un 'salida.runtimeconfig.json' al lado.
// El emisor genera ambos archivos.
// ============================================================

using Mono.Cecil;
using Mono.Cecil.Cil;
using MC = Mono.Cecil;
using MCI = Mono.Cecil.Cil;

namespace BasicPlus.Lexer;

public sealed class IlEmitter
{
    private readonly ModuleNode _moduleAst;
    private readonly SemanticInfo _info;

    private AssemblyDefinition _assembly = null!;
    private ModuleDefinition _ilModule = null!;
    private TypeDefinition _moduleType = null!;
    private MethodDefinition _cctor = null!;

    // .NET type references (cached)
    private TypeReference _voidT   = null!;
    private TypeReference _int64T  = null!;
    private TypeReference _doubleT = null!;
    private TypeReference _stringT = null!;
    private TypeReference _boolT   = null!;
    private TypeReference _objectT = null!;

    // Console & String helpers
    private MethodReference _consoleWriteString  = null!;
    private MethodReference _consoleWriteLine    = null!;       // ()
    private MethodReference _consoleWriteLineStr = null!;       // (string)
    private MethodReference _toStringInt64 = null!;
    private MethodReference _toStringDouble = null!;
    private MethodReference _toStringBool = null!;
    private MethodReference _stringConcat2 = null!;
    private MethodReference _stringConcatN = null!;             // (string, string, string)
    private MethodReference _stringEquals = null!;
    private MethodReference _stringCompare = null!;

    // Símbolo BASICPLUS  ->  miembro IL
    private readonly Dictionary<FunctionSymbol, MethodDefinition> _funcRefs = new();
    private readonly Dictionary<VarSymbol, FieldDefinition>       _varFields = new();
    private readonly Dictionary<ConstSymbol, FieldDefinition>     _constFields = new();

    public List<string> Errors { get; } = new();

    public IlEmitter(ModuleNode moduleAst, SemanticInfo info)
    {
        _moduleAst = moduleAst;
        _info = info;
    }

    // ============================================================
    // PUNTO DE ENTRADA
    // ============================================================
    public void EmitTo(string outputDllPath)
    {
        var asmName = _moduleAst.Name;
        _assembly = AssemblyDefinition.CreateAssembly(
            new AssemblyNameDefinition(asmName, new Version(1, 0, 0, 0)),
            asmName, ModuleKind.Dll);
        _ilModule = _assembly.MainModule;
        CacheReferences();

        _moduleType = new TypeDefinition(
            "", asmName,
            TypeAttributes.Public | TypeAttributes.Abstract | TypeAttributes.Sealed
            | TypeAttributes.Class | TypeAttributes.AutoLayout | TypeAttributes.AnsiClass
            | TypeAttributes.BeforeFieldInit,
            _ilModule.TypeSystem.Object);
        _ilModule.Types.Add(_moduleType);

        // Static constructor (cctor) — para inicializar vars/consts.
        _cctor = new MethodDefinition(
            ".cctor",
            MethodAttributes.Private | MethodAttributes.HideBySig | MethodAttributes.SpecialName
            | MethodAttributes.RTSpecialName | MethodAttributes.Static,
            _voidT);
        _moduleType.Methods.Add(_cctor);
        _cctor.Body.InitLocals = true;

        DeclareMembers();
        EmitMethodBodies();
        CloseCctor();
        EmitEntryPoint();

        _assembly.Write(outputDllPath);
        WriteRuntimeConfig(outputDllPath);
    }

    // ============================================================
    // Cache de referencias a tipos y métodos del runtime
    // ============================================================
    private void CacheReferences()
    {
        var ts = _ilModule.TypeSystem;
        _voidT   = ts.Void;
        _int64T  = ts.Int64;
        _doubleT = ts.Double;
        _stringT = ts.String;
        _boolT   = ts.Boolean;
        _objectT = ts.Object;

        _consoleWriteString  = _ilModule.ImportReference(typeof(System.Console).GetMethod(nameof(System.Console.Write),     new[] { typeof(string) }));
        _consoleWriteLine    = _ilModule.ImportReference(typeof(System.Console).GetMethod(nameof(System.Console.WriteLine), Type.EmptyTypes));
        _consoleWriteLineStr = _ilModule.ImportReference(typeof(System.Console).GetMethod(nameof(System.Console.WriteLine), new[] { typeof(string) }));

        _toStringInt64  = _ilModule.ImportReference(typeof(System.Int64).GetMethod(nameof(object.ToString), Type.EmptyTypes));
        _toStringDouble = _ilModule.ImportReference(typeof(System.Double).GetMethod(nameof(object.ToString), Type.EmptyTypes));
        _toStringBool   = _ilModule.ImportReference(typeof(System.Boolean).GetMethod(nameof(object.ToString), Type.EmptyTypes));

        _stringConcat2 = _ilModule.ImportReference(typeof(System.String).GetMethod(nameof(System.String.Concat), new[] { typeof(string), typeof(string) }));
        _stringConcatN = _ilModule.ImportReference(typeof(System.String).GetMethod(nameof(System.String.Concat), new[] { typeof(string), typeof(string), typeof(string) }));

        _stringEquals  = _ilModule.ImportReference(typeof(System.String).GetMethod(nameof(System.String.Equals), new[] { typeof(string), typeof(string) }));
        _stringCompare = _ilModule.ImportReference(typeof(System.String).GetMethod(nameof(System.String.Compare), new[] { typeof(string), typeof(string) }));
    }

    // ============================================================
    // Pase 1: declarar fields y métodos
    // ============================================================
    private void DeclareMembers()
    {
        foreach (var def in _moduleAst.Defs)
        {
            switch (def)
            {
                case FuncDef f:    DeclareFunc(f);    break;
                case VarDecl v:    DeclareVarField(v); break;
                case ConstDecl c:  DeclareConstField(c); break;
                // Fase 1 ignora silenciosamente classes/enums/properties.
                case ClassDef cd:    Errors.Add($"[{cd.Line}:{cd.Column}] FASE1: clases no soportadas todavía"); break;
                case EnumDef ed:     Errors.Add($"[{ed.Line}:{ed.Column}] FASE1: enums no soportados todavía"); break;
                case PropertyDef p:  Errors.Add($"[{p.Line}:{p.Column}] FASE1: propiedades no soportadas todavía"); break;
            }
        }
    }

    private void DeclareFunc(FuncDef f)
    {
        if (!_info.DeclSymbols.TryGetValue(f, out var sym) || sym is not FunctionSymbol fs) return;
        var attrs = MethodAttributes.Public | MethodAttributes.Static | MethodAttributes.HideBySig;
        var ret   = MapType(fs.ReturnType ?? VoidType.Instance);
        var m     = new MethodDefinition(fs.Name, attrs, ret);
        m.Body.InitLocals = true;
        foreach (var p in fs.Params)
        {
            var pd = new ParameterDefinition(p.Name, ParameterAttributes.None, MapType(p.Type ?? ErrorType.Instance));
            m.Parameters.Add(pd);
        }
        _moduleType.Methods.Add(m);
        _funcRefs[fs] = m;
    }

    private void DeclareVarField(VarDecl v)
    {
        // Fase 1: solo soporta declaraciones simples (un nombre por var_decl).
        if (v.Names.Count != 1)
        {
            Errors.Add($"[{v.Line}:{v.Column}] FASE1: var con varios nombres no soportada");
            return;
        }
        var n = v.Names[0];
        if (!_info.DeclSymbols.TryGetValue(v, out var sym) || sym is not VarSymbol vs) return;
        var t = MapType(vs.Type ?? ErrorType.Instance);
        var attrs = FieldAttributes.Public | FieldAttributes.Static;
        var fd = new FieldDefinition(n.Name, attrs, t);
        _moduleType.Fields.Add(fd);
        _varFields[vs] = fd;
    }

    private void DeclareConstField(ConstDecl c)
    {
        if (!_info.DeclSymbols.TryGetValue(c, out var sym) || sym is not ConstSymbol cs) return;
        var t = MapType(cs.Type ?? ErrorType.Instance);
        // Modelamos const como static readonly (asignación en cctor).
        var attrs = FieldAttributes.Public | FieldAttributes.Static | FieldAttributes.InitOnly;
        var fd = new FieldDefinition(c.Name.Name, attrs, t);
        _moduleType.Fields.Add(fd);
        _constFields[cs] = fd;
    }

    // ============================================================
    // Pase 2: emitir cuerpos
    // ============================================================
    private void EmitMethodBodies()
    {
        foreach (var def in _moduleAst.Defs)
        {
            switch (def)
            {
                case FuncDef f:
                    if (_info.DeclSymbols[f] is FunctionSymbol fs && _funcRefs.TryGetValue(fs, out var md))
                        EmitFuncBody(f, fs, md);
                    break;
                case VarDecl v:
                    if (v.Init is not null && v.Names.Count == 1
                        && _info.DeclSymbols.TryGetValue(v, out var vsym) && vsym is VarSymbol vs
                        && _varFields.TryGetValue(vs, out var vf))
                    {
                        var ctx = new EmitContext(_cctor);
                        EmitExpr(_cctor.Body.GetILProcessor(), v.Init, ctx);
                        EmitConvertIfNeeded(_cctor.Body.GetILProcessor(), TypeOfExpr(v.Init), vs.Type);
                        _cctor.Body.GetILProcessor().Emit(OpCodes.Stsfld, vf);
                    }
                    break;
                case ConstDecl c:
                    if (_info.DeclSymbols.TryGetValue(c, out var csym) && csym is ConstSymbol cs
                        && _constFields.TryGetValue(cs, out var cf))
                    {
                        var ctx = new EmitContext(_cctor);
                        EmitExpr(_cctor.Body.GetILProcessor(), c.Value, ctx);
                        EmitConvertIfNeeded(_cctor.Body.GetILProcessor(), TypeOfExpr(c.Value), cs.Type);
                        _cctor.Body.GetILProcessor().Emit(OpCodes.Stsfld, cf);
                    }
                    break;
            }
        }
    }

    private void CloseCctor() => _cctor.Body.GetILProcessor().Emit(OpCodes.Ret);

    private void EmitEntryPoint()
    {
        var entry = new MethodDefinition(
            "__BpEntry",
            MethodAttributes.Public | MethodAttributes.Static | MethodAttributes.HideBySig,
            _voidT);
        entry.Parameters.Add(new ParameterDefinition("args", ParameterAttributes.None,
            new ArrayType(_stringT)));
        entry.Body.InitLocals = true;
        _moduleType.Methods.Add(entry);

        var il = entry.Body.GetILProcessor();

        // Llama al inicializador del módulo si existe
        if (_info.Module?.Initializer is { } init && _funcRefs.TryGetValue(init, out var initM))
            il.Emit(OpCodes.Call, initM);

        // Llama a Main si existe; si no, no hace nada
        if (_info.Module?.MainFunction is { } main && _funcRefs.TryGetValue(main, out var mainM))
            il.Emit(OpCodes.Call, mainM);
        else
            Errors.Add("módulo sin función Main; el ejecutable no hará nada útil");

        il.Emit(OpCodes.Ret);
        _assembly.EntryPoint = entry;
    }

    private void WriteRuntimeConfig(string outputDllPath)
    {
        var dir = Path.GetDirectoryName(outputDllPath) ?? ".";
        var name = Path.GetFileNameWithoutExtension(outputDllPath);
        var rc = Path.Combine(dir, name + ".runtimeconfig.json");
        File.WriteAllText(rc, """
{
  "runtimeOptions": {
    "tfm": "net8.0",
    "framework": {
      "name": "Microsoft.NETCore.App",
      "version": "8.0.0"
    }
  }
}
""");
    }

    // ============================================================
    // Cuerpo de función
    // ============================================================
    private void EmitFuncBody(FuncDef f, FunctionSymbol fs, MethodDefinition method)
    {
        var il = method.Body.GetILProcessor();
        var ctx = new EmitContext(method);

        // Mapeo parámetros: indice de IL = orden de declaración (sin 'this' porque static)
        for (int i = 0; i < fs.Params.Count; i++)
            ctx.Params[fs.Params[i]] = i;

        foreach (var s in f.Body)
            EmitStmt(il, s, ctx);

        // Return implícito si la función no devuelve nada o no terminó con ret
        if (NeedsImplicitRet(method))
        {
            if (method.ReturnType.MetadataType == MetadataType.Void)
                il.Emit(OpCodes.Ret);
            else
            {
                EmitDefault(il, method.ReturnType);
                il.Emit(OpCodes.Ret);
            }
        }
    }

    private static bool NeedsImplicitRet(MethodDefinition m)
    {
        if (m.Body.Instructions.Count == 0) return true;
        var last = m.Body.Instructions[^1];
        return last.OpCode != OpCodes.Ret;
    }

    private void EmitDefault(ILProcessor il, TypeReference t)
    {
        switch (t.MetadataType)
        {
            case MetadataType.Int64:   il.Emit(OpCodes.Ldc_I8, 0L); break;
            case MetadataType.Double:  il.Emit(OpCodes.Ldc_R8, 0.0); break;
            case MetadataType.Boolean: il.Emit(OpCodes.Ldc_I4_0); break;
            case MetadataType.String:  il.Emit(OpCodes.Ldstr, ""); break;
            default:                   il.Emit(OpCodes.Ldnull); break;
        }
    }

    // ============================================================
    // SENTENCIAS
    // ============================================================
    private void EmitStmt(ILProcessor il, IStmt s, EmitContext ctx)
    {
        switch (s)
        {
            case VarDecl v:        EmitLocalVar(il, v, ctx); break;
            case AssignStmt a:     EmitAssign(il, a, ctx); break;
            case IfStmt iff:       EmitIf(il, iff, ctx); break;
            case WhileStmt w:      EmitWhile(il, w, ctx); break;
            case DoLoopStmt dl:    EmitDoLoop(il, dl, ctx); break;
            case ForStmt fs:       EmitFor(il, fs, ctx); break;
            case ReturnStmt r:     EmitReturn(il, r, ctx); break;
            case PrintStmt p:      EmitPrint(il, p, ctx); break;
            case BreakStmt:
                if (ctx.LoopStack.Count == 0) { Errors.Add("break fuera de bucle"); break; }
                il.Emit(OpCodes.Br, ctx.LoopStack.Peek().BreakTarget);
                break;
            case ContinueStmt:
                if (ctx.LoopStack.Count == 0) { Errors.Add("continue fuera de bucle"); break; }
                il.Emit(OpCodes.Br, ctx.LoopStack.Peek().ContinueTarget);
                break;
            case ExprStmt es:
                {
                    var t = TypeOfExpr(es.Expr);
                    EmitExpr(il, es.Expr, ctx);
                    // Llamadas que devuelven valor: descartar la pila.
                    if (t is not VoidType) il.Emit(OpCodes.Pop);
                }
                break;
        }
    }

    private void EmitLocalVar(ILProcessor il, VarDecl v, EmitContext ctx)
    {
        if (v.Names.Count != 1) { Errors.Add($"[{v.Line}:{v.Column}] FASE1: var local con varios nombres"); return; }
        var n = v.Names[0];
        if (!_info.DeclSymbols.TryGetValue(v, out var sym) || sym is not VarSymbol vs) return;
        var t = MapType(vs.Type ?? ErrorType.Instance);
        var loc = new VariableDefinition(t);
        ctx.Method.Body.Variables.Add(loc);
        ctx.Locals[vs] = loc;
        if (v.Init is not null)
        {
            EmitExpr(il, v.Init, ctx);
            EmitConvertIfNeeded(il, TypeOfExpr(v.Init), vs.Type);
            il.Emit(OpCodes.Stloc, loc);
        }
    }

    private void EmitAssign(ILProcessor il, AssignStmt a, EmitContext ctx)
    {
        // a.Op puede ser :=, += o -=
        if (a.Op == AssignOpKind.Assign)
        {
            EmitAssignTarget(il, a.Target, ctx, () =>
            {
                EmitExpr(il, a.Value, ctx);
                EmitConvertIfNeeded(il, TypeOfExpr(a.Value), TypeOfExpr(a.Target));
            });
            return;
        }

        // Para += / -=: cargar valor actual, sumar/restar, almacenar.
        var lhsT = TypeOfExpr(a.Target);
        EmitAssignTarget(il, a.Target, ctx, () =>
        {
            EmitExpr(il, a.Target, ctx);
            EmitExpr(il, a.Value, ctx);
            // Promoción si aplica
            var rhsT = TypeOfExpr(a.Value);
            EmitConvertIfNeeded(il, rhsT, lhsT);
            if (lhsT is PrimitiveType { Tag: PrimitiveType.Kind.String } && a.Op == AssignOpKind.PlusAssign)
                il.Emit(OpCodes.Call, _stringConcat2);
            else if (a.Op == AssignOpKind.PlusAssign)
                il.Emit(OpCodes.Add);
            else
                il.Emit(OpCodes.Sub);
        });
    }

    /// <summary>
    /// Emite código para almacenar en el target (identificador, etc.) lo que
    /// el callback 'pushValue' deja en la pila.
    /// </summary>
    private void EmitAssignTarget(ILProcessor il, IExpr target, EmitContext ctx, Action pushValue)
    {
        switch (target)
        {
            case IdentifierExpr id:
                {
                    var sym = ResolveIdent(id, ctx);
                    if (sym is VarSymbol vs && ctx.Locals.TryGetValue(vs, out var loc))
                    {
                        pushValue();
                        il.Emit(OpCodes.Stloc, loc);
                    }
                    else if (sym is ParamSymbol ps && ctx.Params.TryGetValue(ps, out var pi))
                    {
                        pushValue();
                        il.Emit(OpCodes.Starg, pi);
                    }
                    else if (sym is VarSymbol vsg && _varFields.TryGetValue(vsg, out var vf))
                    {
                        pushValue();
                        il.Emit(OpCodes.Stsfld, vf);
                    }
                    else if (sym is ConstSymbol csg)
                    {
                        Errors.Add($"[{id.Line}:{id.Column}] no se puede asignar a una constante: '{id.Name}'");
                        pushValue(); il.Emit(OpCodes.Pop);
                    }
                    else
                    {
                        Errors.Add($"[{id.Line}:{id.Column}] identificador no asignable: '{id.Name}'");
                        pushValue(); il.Emit(OpCodes.Pop);
                    }
                    break;
                }
            default:
                Errors.Add($"FASE1: target de asignación no soportado: {target.GetType().Name}");
                pushValue();
                il.Emit(OpCodes.Pop);
                break;
        }
    }

    private void EmitIf(ILProcessor il, IfStmt iff, EmitContext ctx)
    {
        var endLbl = il.Create(OpCodes.Nop);

        // then
        var nextLbl = il.Create(OpCodes.Nop);
        EmitExpr(il, iff.Then.Condition, ctx);
        il.Emit(OpCodes.Brfalse, nextLbl);
        foreach (var s in iff.Then.Body) EmitStmt(il, s, ctx);
        il.Emit(OpCodes.Br, endLbl);

        // elseifs
        for (int i = 0; i < iff.ElseIfs.Count; i++)
        {
            il.Append(nextLbl);
            nextLbl = il.Create(OpCodes.Nop);
            var ei = iff.ElseIfs[i];
            EmitExpr(il, ei.Condition, ctx);
            il.Emit(OpCodes.Brfalse, nextLbl);
            foreach (var s in ei.Body) EmitStmt(il, s, ctx);
            il.Emit(OpCodes.Br, endLbl);
        }

        // else
        il.Append(nextLbl);
        if (iff.Else is not null)
            foreach (var s in iff.Else) EmitStmt(il, s, ctx);

        il.Append(endLbl);
    }

    private void EmitWhile(ILProcessor il, WhileStmt w, EmitContext ctx)
    {
        var loopStart = il.Create(OpCodes.Nop);
        var loopEnd   = il.Create(OpCodes.Nop);
        il.Append(loopStart);
        EmitExpr(il, w.Condition, ctx);
        il.Emit(OpCodes.Brfalse, loopEnd);
        ctx.LoopStack.Push(new LoopFrame(loopStart, loopEnd));
        foreach (var s in w.Body) EmitStmt(il, s, ctx);
        ctx.LoopStack.Pop();
        il.Emit(OpCodes.Br, loopStart);
        il.Append(loopEnd);
    }

    private void EmitDoLoop(ILProcessor il, DoLoopStmt dl, EmitContext ctx)
    {
        var loopStart = il.Create(OpCodes.Nop);
        var loopEnd   = il.Create(OpCodes.Nop);
        var continueLbl = dl.Condition is null ? loopStart : il.Create(OpCodes.Nop);

        il.Append(loopStart);
        ctx.LoopStack.Push(new LoopFrame(continueLbl, loopEnd));
        foreach (var s in dl.Body) EmitStmt(il, s, ctx);
        ctx.LoopStack.Pop();
        if (dl.Condition is null)
        {
            il.Emit(OpCodes.Br, loopStart); // bucle infinito
        }
        else
        {
            il.Append(continueLbl);
            EmitExpr(il, dl.Condition, ctx);
            il.Emit(OpCodes.Brtrue, loopStart);
        }
        il.Append(loopEnd);
    }

    private void EmitFor(ILProcessor il, ForStmt f, EmitContext ctx)
    {
        if (f.Range is not ForNumericRange nr)
        {
            Errors.Add($"[{f.Line}:{f.Column}] FASE1: 'for in' no soportado todavía");
            return;
        }
        // Iterator local
        var iterLoc = new VariableDefinition(_int64T);
        ctx.Method.Body.Variables.Add(iterLoc);
        var iterSym = new VarSymbol(f.IteratorName, false, false, null, true, f.Line, f.Column) { Type = PrimitiveType.Integer };
        ctx.Locals[iterSym] = iterLoc;
        ctx.LocalNames[f.IteratorName] = iterSym;

        // i := from
        EmitExpr(il, nr.From, ctx);
        EmitConvertIfNeeded(il, TypeOfExpr(nr.From), PrimitiveType.Integer);
        il.Emit(OpCodes.Stloc, iterLoc);

        // Pre-calculamos to en una local para no re-evaluarlo.
        var toLoc = new VariableDefinition(_int64T);
        ctx.Method.Body.Variables.Add(toLoc);
        EmitExpr(il, nr.To, ctx);
        EmitConvertIfNeeded(il, TypeOfExpr(nr.To), PrimitiveType.Integer);
        il.Emit(OpCodes.Stloc, toLoc);

        // step (por defecto 1)
        var stepLoc = new VariableDefinition(_int64T);
        ctx.Method.Body.Variables.Add(stepLoc);
        if (nr.Step is not null)
        {
            EmitExpr(il, nr.Step, ctx);
            EmitConvertIfNeeded(il, TypeOfExpr(nr.Step), PrimitiveType.Integer);
        }
        else
        {
            il.Emit(OpCodes.Ldc_I8, 1L);
        }
        il.Emit(OpCodes.Stloc, stepLoc);

        var loopStart = il.Create(OpCodes.Nop);
        var loopEnd   = il.Create(OpCodes.Nop);
        var continueLbl = il.Create(OpCodes.Nop);

        il.Append(loopStart);
        // Condición: si step >= 0, i <= to; si step < 0, i >= to.
        // Para no hacerlo dinámico, asumimos step > 0 si el literal lo es; si no, generamos el genérico.
        if (nr.Step is IntLitExpr ilit && ilit.Value < 0)
        {
            il.Emit(OpCodes.Ldloc, iterLoc);
            il.Emit(OpCodes.Ldloc, toLoc);
            il.Emit(OpCodes.Blt, loopEnd);
        }
        else
        {
            il.Emit(OpCodes.Ldloc, iterLoc);
            il.Emit(OpCodes.Ldloc, toLoc);
            il.Emit(OpCodes.Bgt, loopEnd);
        }

        ctx.LoopStack.Push(new LoopFrame(continueLbl, loopEnd));
        foreach (var s in f.Body) EmitStmt(il, s, ctx);
        ctx.LoopStack.Pop();

        il.Append(continueLbl);
        // i += step
        il.Emit(OpCodes.Ldloc, iterLoc);
        il.Emit(OpCodes.Ldloc, stepLoc);
        il.Emit(OpCodes.Add);
        il.Emit(OpCodes.Stloc, iterLoc);
        il.Emit(OpCodes.Br, loopStart);
        il.Append(loopEnd);

        ctx.LocalNames.Remove(f.IteratorName);
    }

    private void EmitReturn(ILProcessor il, ReturnStmt r, EmitContext ctx)
    {
        if (r.Value is not null)
        {
            EmitExpr(il, r.Value, ctx);
            // convertir si es necesario al tipo de retorno
            EmitConvertIfNeeded(il, TypeOfExpr(r.Value), BpFromCecil(ctx.Method.ReturnType));
        }
        il.Emit(OpCodes.Ret);
    }

    private void EmitPrint(ILProcessor il, PrintStmt p, EmitContext ctx)
    {
        // Construye una llamada Console.Write por cada item, intercalando separadores
        // ',' = " " ; ';' = "" (sin separador). Al final, Console.WriteLine() sin args.
        for (int i = 0; i < p.Items.Count; i++)
        {
            var it = p.Items[i];
            if (i > 0)
            {
                if (it.LeadingSep == PrintSep.Comma)
                {
                    il.Emit(OpCodes.Ldstr, " ");
                    il.Emit(OpCodes.Call, _consoleWriteString);
                }
            }
            if (it.Expr is null) continue;
            EmitExpr(il, it.Expr, ctx);
            EmitToStringInline(il, TypeOfExpr(it.Expr));
            il.Emit(OpCodes.Call, _consoleWriteString);
        }
        il.Emit(OpCodes.Call, _consoleWriteLine);
    }

    /// <summary>Convierte el valor en la pila a string si no lo es ya.</summary>
    private void EmitToStringInline(ILProcessor il, BpType t)
    {
        switch (t)
        {
            case PrimitiveType { Tag: PrimitiveType.Kind.String }:
                break;
            case PrimitiveType { Tag: PrimitiveType.Kind.Integer }:
                {
                    var loc = new VariableDefinition(_int64T);
                    il.Body.Variables.Add(loc);
                    il.Emit(OpCodes.Stloc, loc);
                    il.Emit(OpCodes.Ldloca, loc);
                    il.Emit(OpCodes.Call, _toStringInt64);
                    break;
                }
            case PrimitiveType { Tag: PrimitiveType.Kind.Float }:
                {
                    var loc = new VariableDefinition(_doubleT);
                    il.Body.Variables.Add(loc);
                    il.Emit(OpCodes.Stloc, loc);
                    il.Emit(OpCodes.Ldloca, loc);
                    il.Emit(OpCodes.Call, _toStringDouble);
                    break;
                }
            case PrimitiveType { Tag: PrimitiveType.Kind.Boolean }:
                {
                    var loc = new VariableDefinition(_boolT);
                    il.Body.Variables.Add(loc);
                    il.Emit(OpCodes.Stloc, loc);
                    il.Emit(OpCodes.Ldloca, loc);
                    il.Emit(OpCodes.Call, _toStringBool);
                    break;
                }
            default:
                il.Emit(OpCodes.Ldstr, "<?>");
                break;
        }
    }

    // ============================================================
    // EXPRESIONES — empujan su valor en la pila
    // ============================================================
    private void EmitExpr(ILProcessor il, IExpr e, EmitContext ctx)
    {
        switch (e)
        {
            case IntLitExpr i:    il.Emit(OpCodes.Ldc_I8, i.Value); break;
            case FloatLitExpr f:  il.Emit(OpCodes.Ldc_R8, f.Value); break;
            case StringLitExpr s: il.Emit(OpCodes.Ldstr, s.Value); break;
            case BoolLitExpr b:   il.Emit(b.Value ? OpCodes.Ldc_I4_1 : OpCodes.Ldc_I4_0); break;
            case ParenExpr pe:    EmitExpr(il, pe.Inner, ctx); break;
            case IdentifierExpr id: EmitIdentLoad(il, id, ctx); break;
            case UnaryExpr u:     EmitUnary(il, u, ctx); break;
            case BinaryExpr b:    EmitBinary(il, b, ctx); break;
            case CallExpr c:      EmitCall(il, c, ctx); break;
            default:
                Errors.Add($"FASE1: expresión no soportada: {e.GetType().Name}");
                il.Emit(OpCodes.Ldnull);
                break;
        }
    }

    private void EmitIdentLoad(ILProcessor il, IdentifierExpr id, EmitContext ctx)
    {
        var sym = ResolveIdent(id, ctx);
        if (sym is VarSymbol vs && ctx.Locals.TryGetValue(vs, out var loc))
            il.Emit(OpCodes.Ldloc, loc);
        else if (sym is ParamSymbol ps && ctx.Params.TryGetValue(ps, out var pi))
            il.Emit(OpCodes.Ldarg, pi);
        else if (sym is VarSymbol vsg && _varFields.TryGetValue(vsg, out var vf))
            il.Emit(OpCodes.Ldsfld, vf);
        else if (sym is ConstSymbol cs && _constFields.TryGetValue(cs, out var cf))
            il.Emit(OpCodes.Ldsfld, cf);
        else
        {
            Errors.Add($"[{id.Line}:{id.Column}] identificador no resuelto en emit: '{id.Name}'");
            il.Emit(OpCodes.Ldnull);
        }
    }

    private Symbol? ResolveIdent(IdentifierExpr id, EmitContext ctx)
    {
        // 1) Pista del analizador semántico (si rellenó la tabla)
        if (_info.ExprSymbols.TryGetValue(id, out var s)) return s;
        // 2) Locales por nombre
        if (ctx.LocalNames.TryGetValue(id.Name, out var ls)) return ls;
        // 3) Parámetros por nombre
        foreach (var (p, _) in ctx.Params)
            if (p.Name == id.Name) return p;
        // 4) Locales registradas
        foreach (var (vs, _) in ctx.Locals)
            if (vs.Name == id.Name) return vs;
        // 5) Campos del módulo (vars / consts)
        foreach (var (vs, _) in _varFields)
            if (vs.Name == id.Name) return vs;
        foreach (var (cs, _) in _constFields)
            if (cs.Name == id.Name) return cs;
        return null;
    }

    private void EmitUnary(ILProcessor il, UnaryExpr u, EmitContext ctx)
    {
        EmitExpr(il, u.Operand, ctx);
        switch (u.Op)
        {
            case "-":   il.Emit(OpCodes.Neg); break;
            case "not": il.Emit(OpCodes.Ldc_I4_0); il.Emit(OpCodes.Ceq); break;
            default:    Errors.Add($"unario no soportado: '{u.Op}'"); break;
        }
    }

    private void EmitBinary(ILProcessor il, BinaryExpr b, EmitContext ctx)
    {
        // Lógicos con corto-circuito
        if (b.Op == "and") { EmitShortCircuit(il, b, ctx, andSemantics: true); return; }
        if (b.Op == "or")  { EmitShortCircuit(il, b, ctx, andSemantics: false); return; }

        var lt = TypeOfExpr(b.Left);
        var rt = TypeOfExpr(b.Right);
        var common = NumericPromotion(lt, rt);

        // String concat con '+'
        if (b.Op == "+" && (lt is PrimitiveType { Tag: PrimitiveType.Kind.String } || rt is PrimitiveType { Tag: PrimitiveType.Kind.String }))
        {
            EmitExpr(il, b.Left, ctx);  EmitToStringInline(il, lt);
            EmitExpr(il, b.Right, ctx); EmitToStringInline(il, rt);
            il.Emit(OpCodes.Call, _stringConcat2);
            return;
        }

        EmitExpr(il, b.Left, ctx);
        EmitConvertIfNeeded(il, lt, common);
        EmitExpr(il, b.Right, ctx);
        EmitConvertIfNeeded(il, rt, common);

        switch (b.Op)
        {
            case "+":   il.Emit(OpCodes.Add); break;
            case "-":   il.Emit(OpCodes.Sub); break;
            case "*":   il.Emit(OpCodes.Mul); break;
            case "/":   il.Emit(OpCodes.Div); break;
            case "mod": il.Emit(OpCodes.Rem); break;
            case "&":   il.Emit(OpCodes.And); break;
            case "|":   il.Emit(OpCodes.Or);  break;
            case "xor": il.Emit(OpCodes.Xor); break;
            case "shl": il.Emit(OpCodes.Conv_I4); il.Emit(OpCodes.Shl); break;
            case "shr": il.Emit(OpCodes.Conv_I4); il.Emit(OpCodes.Shr); break;
            case "==":  EmitCompare(il, b, common, OpCodes.Ceq); break;
            case "!=":  EmitCompare(il, b, common, OpCodes.Ceq); il.Emit(OpCodes.Ldc_I4_0); il.Emit(OpCodes.Ceq); break;
            case "<":   EmitCompare(il, b, common, OpCodes.Clt); break;
            case ">":   EmitCompare(il, b, common, OpCodes.Cgt); break;
            case "<=":  EmitCompare(il, b, common, OpCodes.Cgt); il.Emit(OpCodes.Ldc_I4_0); il.Emit(OpCodes.Ceq); break;
            case ">=":  EmitCompare(il, b, common, OpCodes.Clt); il.Emit(OpCodes.Ldc_I4_0); il.Emit(OpCodes.Ceq); break;
            default:
                Errors.Add($"operador no soportado: '{b.Op}'");
                break;
        }
    }

    /// <summary>'and'/'or' con corto-circuito.</summary>
    private void EmitShortCircuit(ILProcessor il, BinaryExpr b, EmitContext ctx, bool andSemantics)
    {
        var endLbl   = il.Create(OpCodes.Nop);
        var shortLbl = il.Create(OpCodes.Nop);

        EmitExpr(il, b.Left, ctx);
        // Para 'and': si false → resultado false (0). Para 'or': si true → resultado true (1).
        il.Emit(andSemantics ? OpCodes.Brfalse : OpCodes.Brtrue, shortLbl);
        EmitExpr(il, b.Right, ctx);
        il.Emit(OpCodes.Br, endLbl);
        il.Append(shortLbl);
        il.Emit(andSemantics ? OpCodes.Ldc_I4_0 : OpCodes.Ldc_I4_1);
        il.Append(endLbl);
    }

    private void EmitCompare(ILProcessor il, BinaryExpr b, BpType common, OpCode op)
    {
        // Operandos ya están en pila (con conversión aplicada). Solo emite la comparación.
        // Para strings, esto se quedaría corto: por simplicidad, asumimos numéricos/bool aquí.
        // Igualdad/desigualdad de strings requeriría llamar a String.Equals; lo dejamos para Fase 2.
        il.Emit(op);
    }

    private void EmitCall(ILProcessor il, CallExpr ce, EmitContext ctx)
    {
        if (ce.Callee is not IdentifierExpr id)
        {
            Errors.Add("FASE1: solo se soportan llamadas por nombre simple");
            il.Emit(OpCodes.Ldnull);
            return;
        }

        // Resuelve la función por nombre.
        FunctionSymbol? target = null;
        if (_info.ExprSymbols.TryGetValue(id, out var sym) && sym is FunctionSymbol fs) target = fs;
        if (target is null)
            foreach (var (k, _) in _funcRefs)
                if (k.Name == id.Name) { target = k; break; }
        if (target is null || !_funcRefs.TryGetValue(target, out var md))
        {
            Errors.Add($"[{id.Line}:{id.Column}] función no encontrada: '{id.Name}'");
            il.Emit(OpCodes.Ldnull);
            return;
        }

        // Empuja args con conversión si hace falta.
        for (int i = 0; i < ce.Args.Count; i++)
        {
            EmitExpr(il, ce.Args[i], ctx);
            if (i < target.Params.Count)
                EmitConvertIfNeeded(il, TypeOfExpr(ce.Args[i]), target.Params[i].Type);
        }
        il.Emit(OpCodes.Call, md);
    }

    // ============================================================
    // TIPOS
    // ============================================================
    private TypeReference MapType(BpType t) => t switch
    {
        PrimitiveType { Tag: PrimitiveType.Kind.Integer } => _int64T,
        PrimitiveType { Tag: PrimitiveType.Kind.Float   } => _doubleT,
        PrimitiveType { Tag: PrimitiveType.Kind.String  } => _stringT,
        PrimitiveType { Tag: PrimitiveType.Kind.Boolean } => _boolT,
        VoidType   => _voidT,
        NullType   => _objectT,
        ErrorType  => _objectT,
        _ => _objectT
    };

    private BpType BpFromCecil(TypeReference t) => t.MetadataType switch
    {
        MetadataType.Int64   => PrimitiveType.Integer,
        MetadataType.Double  => PrimitiveType.Float,
        MetadataType.String  => PrimitiveType.String,
        MetadataType.Boolean => PrimitiveType.Boolean,
        MetadataType.Void    => VoidType.Instance,
        _ => ErrorType.Instance
    };

    private BpType TypeOfExpr(IExpr e) =>
        _info.ExprTypes.TryGetValue(e, out var t) ? t : InferLocalType(e);

    /// <summary>Fallback cuando el analizador no anotó (ej. árbol manipulado).</summary>
    private static BpType InferLocalType(IExpr e) => e switch
    {
        IntLitExpr    => PrimitiveType.Integer,
        FloatLitExpr  => PrimitiveType.Float,
        StringLitExpr => PrimitiveType.String,
        BoolLitExpr   => PrimitiveType.Boolean,
        _ => ErrorType.Instance
    };

    private static BpType NumericPromotion(BpType a, BpType b)
    {
        if (a is PrimitiveType { Tag: PrimitiveType.Kind.Float } || b is PrimitiveType { Tag: PrimitiveType.Kind.Float })
            return PrimitiveType.Float;
        if (a is PrimitiveType { Tag: PrimitiveType.Kind.Integer } && b is PrimitiveType { Tag: PrimitiveType.Kind.Integer })
            return PrimitiveType.Integer;
        return a;
    }

    /// <summary>Si 'src' ≠ 'dst' y la conversión es legal, emite la instrucción.</summary>
    private void EmitConvertIfNeeded(ILProcessor il, BpType? src, BpType? dst)
    {
        if (src is null || dst is null) return;
        if (src.SameAs(dst)) return;
        if (dst is PrimitiveType { Tag: PrimitiveType.Kind.Float } && src is PrimitiveType { Tag: PrimitiveType.Kind.Integer })
            il.Emit(OpCodes.Conv_R8);
        else if (dst is PrimitiveType { Tag: PrimitiveType.Kind.Integer } && src is PrimitiveType { Tag: PrimitiveType.Kind.Float })
            il.Emit(OpCodes.Conv_I8);
        // Otras conversiones se dejan al runtime (cast implícito en CIL no aplica para todo).
    }
}

// ============================================================
// Estado del emisor por método
// ============================================================
internal sealed class EmitContext
{
    public MethodDefinition Method { get; }
    public Dictionary<VarSymbol, VariableDefinition> Locals { get; } = new();
    public Dictionary<ParamSymbol, int> Params { get; } = new();
    public Dictionary<string, Symbol> LocalNames { get; } = new();
    public Stack<LoopFrame> LoopStack { get; } = new();
    public EmitContext(MethodDefinition method) { Method = method; }
}

internal sealed record LoopFrame(MCI.Instruction ContinueTarget, MCI.Instruction BreakTarget);
