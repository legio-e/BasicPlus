// ============================================================
// SemanticAnalyzer.cs
// Análisis semántico de BASICPLUS en tres pases:
//
//   1) Pase de DECLARACIONES
//      - Construye ModuleSymbol y los símbolos top-level.
//      - Para cada clase, construye sus miembros (instancia y estáticos).
//      - Identifica constructores (función con el nombre de la clase),
//        inicializador de módulo y la función Main.
//      - Detecta nombres duplicados, prefijo estático mal puesto.
//
//   2) Pase de RESOLUCIÓN DE TIPOS
//      - Resuelve cada TypeRef a un BpType concreto.
//      - Resuelve 'extends' a un ClassSymbol.
//      - Detecta ciclos de herencia.
//      - Asigna tipo a parámetros, vars, consts, propiedades.
//
//   3) Pase de CUERPOS
//      - Para cada función, recorre su cuerpo.
//      - Resuelve identificadores con un scope.
//      - Verifica tipos en asignaciones, operadores, llamadas.
//      - Aplica reglas contextuales: this, super, field, final,
//        visibilidad, miembros estáticos, constructor, catch order,
//        super() como primera sentencia, exhaustividad de switch
//        sobre enum, etc.
//      - Llena SemanticInfo.ExprTypes y ExprSymbols.
// ============================================================

namespace BasicPlus.Lexer;

public sealed class SemanticAnalyzer
{
    private readonly SemanticInfo _info = new();
    private ModuleSymbol _module = null!;

    // ---------- Estado del recorrido (pase 3) ----------
    private ClassSymbol? _currentClass;
    private FunctionSymbol? _currentFunction;
    private bool _insideGetter;
    private bool _insideSetter;
    private string? _setterParamName;
    private int _loopDepth;
    private int _switchDepth;
    private bool _seenSuperCallInCtor;
    private bool _isFirstStmtOfCtor;

    public SemanticInfo Analyze(ModuleNode module)
    {
        DeclarePass(module);
        ResolveTypesPass(module);
        BodyPass(module);
        ValidateModuleEntryPoints(module);
        return _info;
    }

    // ============================================================
    // PASE 1 — Declaraciones
    // ============================================================
    private void DeclarePass(ModuleNode mod)
    {
        _module = new ModuleSymbol(mod.Name, mod.Line, mod.Column) { Decl = mod };
        _info.Module = _module;
        _info.DeclSymbols[mod] = _module;

        foreach (var def in mod.Defs)
            DeclareTopLevel(def, ownerClass: null, scope: _module.Members);

        // ¿inicializador de módulo? — función con el mismo nombre del módulo
        if (_module.Members.TryLookup(_module.Name, out var maybeInit) && maybeInit is FunctionSymbol fInit)
        {
            _module.Initializer = fInit;
            fInit.IsModuleInitializer = true;
            if (fInit.IsStatic)
                Err(fInit.Line, fInit.Column, "el inicializador del módulo no puede ser estático");
        }

        if (_module.Members.TryLookup("Main", out var maybeMain) && maybeMain is FunctionSymbol fMain)
            _module.MainFunction = fMain;
    }

    private void DeclareTopLevel(ITopLevelDecl decl, ClassSymbol? ownerClass, Scope scope)
    {
        switch (decl)
        {
            case ConstDecl c:    DeclareConst(c, ownerClass, scope);    break;
            case VarDecl v:      DeclareVar(v, ownerClass, scope);      break;
            case FuncDef f:      DeclareFunc(f, ownerClass, scope);     break;
            case PropertyDef p:  DeclareProperty(p, ownerClass, scope); break;
            case ClassDef cls:   DeclareClass(cls);                     break;
            case EnumDef en:     DeclareEnum(en);                        break;
        }
    }

    private void DeclareClass(ClassDef cd)
    {
        if (_currentClass is not null)
        {
            Err(cd.Line, cd.Column, "no se permiten clases anidadas");
            return;
        }
        var cls = new ClassSymbol(cd.Name, cd.IsPublic, cd.BaseClass, cd, cd.Line, cd.Column);
        if (!_module.Members.TryDefine(cls))
            Err(cd.Line, cd.Column, $"nombre duplicado: '{cd.Name}'");
        _info.DeclSymbols[cd] = cls;

        // Recorre miembros como en módulo, con ownerClass = cls.
        var save = _currentClass;
        _currentClass = cls;
        try
        {
            foreach (var m in cd.Members)
            {
                if (m is ClassDef || m is EnumDef)
                {
                    Err(((AstNode)m).Line, ((AstNode)m).Column, "no se permiten clases ni enums dentro de una clase");
                    continue;
                }
                DeclareTopLevel(m, ownerClass: cls, scope: m.IsStaticDecl(out _) ? cls.StaticMembers : cls.InstanceMembers);
            }
        }
        finally { _currentClass = save; }

        // Identificar constructor: función con el nombre de la clase.
        if (cls.InstanceMembers.TryLookup(cls.Name, out var maybeCtor) && maybeCtor is FunctionSymbol fCtor)
        {
            cls.Constructor = fCtor;
            fCtor.IsConstructor = true;
            if (fCtor.AstNode.ReturnType is not null)
                Err(fCtor.Line, fCtor.Column, "el constructor no puede declarar tipo de retorno");
        }
    }

    private void DeclareEnum(EnumDef ed)
    {
        var en = new EnumSymbol(ed.Name, ed.IsPublic, ed.Line, ed.Column) { Decl = ed };
        if (!_module.Members.TryDefine(en))
            Err(ed.Line, ed.Column, $"nombre duplicado: '{ed.Name}'");
        _info.DeclSymbols[ed] = en;

        long next = 0;
        var seen = new HashSet<string>(StringComparer.Ordinal);
        foreach (var v in ed.Values)
        {
            if (!seen.Add(v.Name))
                Err(v.Line, v.Column, $"valor duplicado en enum '{ed.Name}': '{v.Name}'");
            long val = v.ExplicitValue ?? next;
            en.Values[v.Name] = val;
            next = val + 1;
        }
    }

    private void DeclareConst(ConstDecl c, ClassSymbol? owner, Scope scope)
    {
        var (qual, simple) = SplitDeclName(c.Name, owner);
        bool isStatic = qual is not null;
        var sym = new ConstSymbol(simple, c.IsPublic, isStatic, owner, c.Line, c.Column) { Decl = c };
        if (!scope.TryDefine(sym))
            Err(c.Line, c.Column, $"nombre duplicado: '{simple}' en {scope.Tag}");
        _info.DeclSymbols[c] = sym;
    }

    private void DeclareVar(VarDecl v, ClassSymbol? owner, Scope scope)
    {
        foreach (var n in v.Names)
        {
            var (qual, simple) = SplitDeclName(n, owner);
            bool isStatic = qual is not null;
            var sym = new VarSymbol(simple, v.IsPublic, isStatic, owner, isLocal: false, n.Line, n.Column) { Decl = v };
            if (!scope.TryDefine(sym))
                Err(n.Line, n.Column, $"nombre duplicado: '{simple}' en {scope.Tag}");
            // Solo asociamos el primer nombre con el AstNode
            if (!_info.DeclSymbols.ContainsKey(v))
                _info.DeclSymbols[v] = sym;
        }
    }

    private void DeclareFunc(FuncDef f, ClassSymbol? owner, Scope scope)
    {
        var (qual, simple) = SplitDeclName(f.Name, owner);
        bool isStatic = qual is not null;
        if (f.IsFinal && owner is null)
            Err(f.Line, f.Column, "'final' solo es válido en métodos de clase");
        var sym = new FunctionSymbol(simple, f.IsPublic, f.IsFinal, isStatic, owner, f);
        foreach (var p in f.Params)
            sym.Params.Add(new ParamSymbol(p.Name, p.Line, p.Column));
        if (!scope.TryDefine(sym))
            Err(f.Line, f.Column, $"nombre duplicado: '{simple}' en {scope.Tag}");
        _info.DeclSymbols[f] = sym;
    }

    private void DeclareProperty(PropertyDef p, ClassSymbol? owner, Scope scope)
    {
        var (qual, simple) = SplitDeclName(p.Name, owner);
        bool isStatic = qual is not null;
        if (p.IsFinal && owner is null)
            Err(p.Line, p.Column, "'final' solo es válido en propiedades de clase");
        var sym = new PropertySymbol(simple, p.IsPublic, p.IsFinal, isStatic, owner, p);
        if (!scope.TryDefine(sym))
            Err(p.Line, p.Column, $"nombre duplicado: '{simple}' en {scope.Tag}");
        _info.DeclSymbols[p] = sym;
    }

    private (string? qualifier, string simple) SplitDeclName(DeclName n, ClassSymbol? owner)
    {
        if (n.ClassQualifier is null) return (null, n.Name);
        if (owner is null)
        {
            Err(n.Line, n.Column, $"prefijo de clase '{n.ClassQualifier}.' no válido a nivel módulo");
            return (n.ClassQualifier, n.Name);
        }
        if (n.ClassQualifier != owner.Name)
            Err(n.Line, n.Column, $"prefijo '{n.ClassQualifier}.' no coincide con la clase '{owner.Name}'");
        return (n.ClassQualifier, n.Name);
    }

    // ============================================================
    // PASE 2 — Resolución de tipos
    // ============================================================
    private void ResolveTypesPass(ModuleNode mod)
    {
        // 1) extends de cada clase + ciclos.
        foreach (var s in _module.Members.Symbols)
        {
            if (s is ClassSymbol cls && cls.BaseClassName is not null)
            {
                var baseSym = _module.Members.Resolve(cls.BaseClassName);
                if (baseSym is not ClassSymbol baseCls)
                {
                    Err(cls.Line, cls.Column, $"clase base '{cls.BaseClassName}' no existe");
                }
                else
                {
                    cls.BaseClass = baseCls;
                }
            }
        }
        // Ciclos
        foreach (var s in _module.Members.Symbols)
        {
            if (s is ClassSymbol cls)
            {
                var visited = new HashSet<ClassSymbol>();
                var c = cls;
                while (c is not null)
                {
                    if (!visited.Add(c))
                    {
                        Err(cls.Line, cls.Column, $"ciclo de herencia detectado en '{cls.Name}'");
                        cls.BaseClass = null;
                        break;
                    }
                    c = c.BaseClass;
                }
            }
        }

        // 2) tipos de vars/consts/props/parámetros + retorno de funciones a nivel módulo
        foreach (var def in mod.Defs) ResolveDefTypes(def, owner: null);
        foreach (var s in _module.Members.Symbols)
            if (s is ClassSymbol cls)
                foreach (var m in cls.AstNode.Members)
                    ResolveDefTypes(m, owner: cls);
    }

    private void ResolveDefTypes(ITopLevelDecl def, ClassSymbol? owner)
    {
        switch (def)
        {
            case ConstDecl c when _info.DeclSymbols.TryGetValue(c, out var s) && s is ConstSymbol cs:
                if (c.Type is not null) cs.Type = ResolveType(c.Type);
                // si no hay tipo explícito, se infiere en pase 3 al evaluar c.Value
                break;
            case VarDecl v:
                {
                    BpType t = ResolveType(v.Type);
                    foreach (var n in v.Names)
                    {
                        // Buscar el símbolo correspondiente en el scope adecuado.
                        Symbol? sym = owner is null
                            ? _module.Members.Resolve(n.Name)
                            : (n.IsStatic ? owner.StaticMembers.Resolve(n.Name) : owner.InstanceMembers.Resolve(n.Name));
                        if (sym is VarSymbol vs) vs.Type = t;
                    }
                    break;
                }
            case FuncDef f when _info.DeclSymbols.TryGetValue(f, out var s) && s is FunctionSymbol fs:
                for (int i = 0; i < f.Params.Count; i++)
                    fs.Params[i].Type = ResolveType(f.Params[i].Type);
                if (f.ReturnType is not null) fs.ReturnType = ResolveType(f.ReturnType);
                break;
            case PropertyDef p when _info.DeclSymbols.TryGetValue(p, out var s) && s is PropertySymbol ps:
                ps.Type = ResolveType(p.Type);
                break;
        }
    }

    private BpType ResolveType(TypeRef t)
    {
        switch (t)
        {
            case SimpleTypeRef st:
                return st.Name switch
                {
                    "integer" => PrimitiveType.Integer,
                    "float"   => PrimitiveType.Float,
                    "string"  => PrimitiveType.String,
                    "boolean" => PrimitiveType.Boolean,
                    _         => ResolveNamedType(st)
                };
            case ArrayTypeRef at:
                return new ArrayType(ResolveType(at.Element));
            default:
                return ErrorType.Instance;
        }
    }

    private BpType ResolveNamedType(SimpleTypeRef st)
    {
        var sym = _module.Members.Resolve(st.Name);
        if (sym is ClassSymbol cls) return new ClassType(cls);
        if (sym is EnumSymbol en)   return new EnumType(en);
        Err(st.Line, st.Column, $"tipo '{st.Name}' no encontrado");
        return ErrorType.Instance;
    }

    // ============================================================
    // PASE 3 — Cuerpos
    // ============================================================
    private void BodyPass(ModuleNode mod)
    {
        // Funciones a nivel módulo
        foreach (var def in mod.Defs)
            BodyForDef(def, owner: null);

        // Métodos y propiedades de clases
        foreach (var s in _module.Members.Symbols)
            if (s is ClassSymbol cls)
            {
                CheckOverridesAndFinal(cls);
                foreach (var m in cls.AstNode.Members)
                    BodyForDef(m, owner: cls);
            }
    }

    private void BodyForDef(ITopLevelDecl def, ClassSymbol? owner)
    {
        var save = _currentClass;
        _currentClass = owner;
        try
        {
            switch (def)
            {
                case ConstDecl c:    AnalyzeConstInit(c);   break;
                case VarDecl v:      AnalyzeVarInit(v);     break;
                case FuncDef f:      AnalyzeFunction(f);    break;
                case PropertyDef p:  AnalyzeProperty(p);    break;
            }
        }
        finally { _currentClass = save; }
    }

    private void AnalyzeConstInit(ConstDecl c)
    {
        if (_info.DeclSymbols[c] is not ConstSymbol cs) return;
        var t = AnalyzeExpr(c.Value, expected: cs.Type);
        if (cs.Type is null) cs.Type = t;
        else if (!cs.Type.IsAssignableFrom(t))
            Err(c.Line, c.Column, $"valor de tipo '{t.Display}' no asignable a const de tipo '{cs.Type.Display}'");
    }

    private void AnalyzeVarInit(VarDecl v)
    {
        if (v.Init is null) return;
        // Tomamos el tipo desde el primer símbolo declarado.
        Symbol? sym = _info.DeclSymbols.TryGetValue(v, out var x) ? x : null;
        BpType? t = sym is VarSymbol vs ? vs.Type : null;
        var got = AnalyzeExpr(v.Init, expected: t);
        if (t is not null && !t.IsAssignableFrom(got))
            Err(v.Line, v.Column, $"valor de tipo '{got.Display}' no asignable a var de tipo '{t.Display}'");
        if (t is { IsScalar: true } && got is NullType)
            Err(v.Line, v.Column, $"no se puede asignar 'null' a un tipo escalar '{t.Display}'");
    }

    private void AnalyzeFunction(FuncDef f)
    {
        if (_info.DeclSymbols[f] is not FunctionSymbol fs) return;

        var save = _currentFunction;
        _currentFunction = fs;
        var localScope = new Scope("locals", _currentClass is null ? _module.Members : null);

        // Si es método de instancia, las búsquedas tienen acceso a los miembros
        // de la clase implícitamente (vía 'this'); lo modelamos en LookupName.
        // Aquí solo registramos parámetros como locales.
        foreach (var p in fs.Params)
        {
            if (!localScope.TryDefine(new VarSymbol(p.Name, false, false, null, isLocal: true, p.Line, p.Column) { Type = p.Type }))
                Err(p.Line, p.Column, $"parámetro duplicado: '{p.Name}'");
        }

        // Reglas del constructor: marcar contexto inicial.
        _isFirstStmtOfCtor = fs.IsConstructor;
        _seenSuperCallInCtor = false;

        AnalyzeBody(f.Body, localScope);

        _currentFunction = save;
    }

    private void AnalyzeProperty(PropertyDef p)
    {
        if (_info.DeclSymbols[p] is not PropertySymbol ps) return;

        if (p.Init is not null)
        {
            var t = AnalyzeExpr(p.Init, expected: ps.Type);
            if (ps.Type is not null && !ps.Type.IsAssignableFrom(t))
                Err(p.Line, p.Column, $"valor inicial de tipo '{t.Display}' no asignable a propiedad '{ps.Type.Display}'");
        }
        if (p.Getter is not null)
        {
            _insideGetter = true;
            var localScope = new Scope("getter", null);
            AnalyzeBody(p.Getter.Body, localScope);
            _insideGetter = false;
        }
        if (p.Setter is not null)
        {
            _insideSetter = true;
            _setterParamName = p.Setter.ParamName;
            var localScope = new Scope("setter", null);
            // Parámetro del setter
            localScope.TryDefine(new VarSymbol(p.Setter.ParamName, false, false, null, true, p.Setter.Line, p.Setter.Column) { Type = ps.Type });
            AnalyzeBody(p.Setter.Body, localScope);
            _insideSetter = false;
            _setterParamName = null;
        }
    }

    // ============================================================
    // STATEMENTS
    // ============================================================
    private void AnalyzeBody(List<IStmt> body, Scope scope)
    {
        bool returnSeen = false;
        foreach (var s in body)
        {
            if (returnSeen)
                Warn(((AstNode)s).Line, ((AstNode)s).Column, "código inalcanzable después de 'return'");
            AnalyzeStmt(s, scope);
            if (s is ReturnStmt) returnSeen = true;
        }
    }

    private void AnalyzeStmt(IStmt s, Scope scope)
    {
        // Solo super(...) como sentencia preserva 'soy la primera del ctor'.
        bool isSuperCallStmt = s is ExprStmt es0 && es0.Expr is SuperCallExpr;
        if (!isSuperCallStmt) _isFirstStmtOfCtor = false;

        switch (s)
        {
            case VarDecl v:        AnalyzeLocalVarDecl(v, scope); break;
            case ConstDecl c:      AnalyzeLocalConstDecl(c, scope); break;
            case AssignStmt a:     AnalyzeAssign(a, scope); break;
            case IfStmt iff:       AnalyzeIf(iff, scope); break;
            case SwitchStmt sw:    AnalyzeSwitch(sw, scope); break;
            case WhileStmt w:      AnalyzeWhile(w, scope); break;
            case DoLoopStmt dl:    AnalyzeDoLoop(dl, scope); break;
            case ForStmt fs:       AnalyzeFor(fs, scope); break;
            case TryStmt tr:       AnalyzeTry(tr, scope); break;
            case ReturnStmt r:     AnalyzeReturn(r, scope); break;
            case ThrowStmt th:     AnalyzeExpr(th.Value, scope: scope); break;
            case PrintStmt pr:
                foreach (var it in pr.Items)
                    if (it.Expr is not null) AnalyzeExpr(it.Expr, scope: scope);
                break;
            case BreakStmt br:
                if (_loopDepth == 0 && _switchDepth == 0)
                    Err(br.Line, br.Column, "'break' fuera de bucle o switch");
                break;
            case ContinueStmt cs:
                if (_loopDepth == 0)
                    Err(cs.Line, cs.Column, "'continue' fuera de bucle");
                break;
            case ExprStmt es:
                AnalyzeExpr(es.Expr, scope: scope);
                if (es.Expr is not (CallExpr or SuperCallExpr))
                    Err(es.Line, es.Column, "se esperaba una llamada como sentencia");
                break;
        }
    }

    private void AnalyzeLocalVarDecl(VarDecl v, Scope scope)
    {
        var t = ResolveType(v.Type);
        foreach (var n in v.Names)
        {
            if (n.IsStatic)
                Err(n.Line, n.Column, "no se puede declarar miembro estático en un cuerpo de función");
            var sym = new VarSymbol(n.Name, false, false, null, true, n.Line, n.Column) { Type = t };
            if (!scope.TryDefine(sym))
                Err(n.Line, n.Column, $"variable duplicada en este scope: '{n.Name}'");
            if (!_info.DeclSymbols.ContainsKey(v)) _info.DeclSymbols[v] = sym;
        }
        if (v.Init is not null)
        {
            var got = AnalyzeExpr(v.Init, scope: scope, expected: t);
            if (!t.IsAssignableFrom(got))
                Err(v.Line, v.Column, $"valor de tipo '{got.Display}' no asignable a variable de tipo '{t.Display}'");
            if (t.IsScalar && got is NullType)
                Err(v.Line, v.Column, $"no se puede asignar 'null' a un tipo escalar '{t.Display}'");
        }
    }

    private void AnalyzeLocalConstDecl(ConstDecl c, Scope scope)
    {
        if (c.Name.IsStatic)
            Err(c.Line, c.Column, "no se puede declarar const estática en un cuerpo de función");
        BpType? declared = c.Type is null ? null : ResolveType(c.Type);
        var got = AnalyzeExpr(c.Value, scope: scope, expected: declared);
        var t = declared ?? got;
        var sym = new ConstSymbol(c.Name.Name, false, false, null, c.Line, c.Column) { Type = t };
        if (!scope.TryDefine(sym))
            Err(c.Line, c.Column, $"const duplicada: '{c.Name.Name}'");
        if (declared is not null && !declared.IsAssignableFrom(got))
            Err(c.Line, c.Column, $"valor de tipo '{got.Display}' no asignable a const de tipo '{declared.Display}'");
        _info.DeclSymbols[c] = sym;
    }

    private void AnalyzeAssign(AssignStmt a, Scope scope)
    {
        var lhsT = AnalyzeExpr(a.Target, scope: scope);
        var rhsT = AnalyzeExpr(a.Value,  scope: scope, expected: lhsT);

        // Target debe ser asignable: identificador, member access, index o 'field'.
        if (a.Target is not (IdentifierExpr or MemberAccessExpr or IndexExpr or FieldExpr))
            Err(a.Line, a.Column, "el operando izquierdo no es asignable");

        // 'field' como destino es escribible solo dentro de un setter.
        if (a.Target is FieldExpr && !_insideSetter)
            Err(a.Line, a.Column, "'field' solo puede usarse dentro de get/set");

        if (a.Op == AssignOpKind.PlusAssign || a.Op == AssignOpKind.MinusAssign)
        {
            bool ok = (lhsT.IsNumeric && rhsT.IsNumeric)
                   || (a.Op == AssignOpKind.PlusAssign && lhsT is PrimitiveType { Tag: PrimitiveType.Kind.String } && rhsT is PrimitiveType { Tag: PrimitiveType.Kind.String });
            if (!ok)
                Err(a.Line, a.Column, $"operandos incompatibles para '{(a.Op == AssignOpKind.PlusAssign ? "+=" : "-=")}': '{lhsT.Display}' y '{rhsT.Display}'");
        }
        else // Assign
        {
            if (rhsT is NullType && lhsT.IsScalar)
                Err(a.Line, a.Column, $"no se puede asignar 'null' a un tipo escalar '{lhsT.Display}'");
            else if (!lhsT.IsAssignableFrom(rhsT))
                Err(a.Line, a.Column, $"no se puede asignar '{rhsT.Display}' a '{lhsT.Display}'");
        }
    }

    private void AnalyzeIf(IfStmt iff, Scope scope)
    {
        AnalyzeBoolCondition(iff.Then.Condition, scope);
        AnalyzeBody(iff.Then.Body, new Scope("if-then", scope));
        foreach (var ei in iff.ElseIfs)
        {
            AnalyzeBoolCondition(ei.Condition, scope);
            AnalyzeBody(ei.Body, new Scope("elseif", scope));
        }
        if (iff.Else is not null) AnalyzeBody(iff.Else, new Scope("else", scope));
    }

    private void AnalyzeSwitch(SwitchStmt sw, Scope scope)
    {
        var subjT = AnalyzeExpr(sw.Subject, scope: scope);
        _switchDepth++;
        try
        {
            var seen = new HashSet<string>();
            foreach (var cc in sw.Cases)
            {
                foreach (var v in cc.Values)
                {
                    var t = AnalyzeExpr(v, scope: scope, expected: subjT);
                    if (!subjT.IsAssignableFrom(t) && !t.IsAssignableFrom(subjT))
                        Err((v as AstNode)!.Line, (v as AstNode)!.Column,
                            $"valor de 'case' tipo '{t.Display}' incompatible con switch tipo '{subjT.Display}'");
                    string key = ExprKey(v);
                    if (key is not null && !seen.Add(key))
                        Err((v as AstNode)!.Line, (v as AstNode)!.Column, $"valor 'case' duplicado: {key}");
                }
                AnalyzeBody(cc.Body, new Scope("case", scope));
            }
            if (sw.Default is not null) AnalyzeBody(sw.Default, new Scope("default", scope));

            // Exhaustividad sobre enum (solo si no hay default).
            if (sw.Default is null && subjT is EnumType et)
            {
                var missing = et.Enum.Values.Keys.Where(n => !seen.Contains($"{et.Enum.Name}.{n}")).ToList();
                if (missing.Count > 0)
                    Warn(sw.Line, sw.Column,
                        $"switch sobre enum '{et.Enum.Name}' no exhaustivo, faltan: {string.Join(", ", missing)}");
            }
        }
        finally { _switchDepth--; }
    }

    private string? ExprKey(IExpr e) => e switch
    {
        IntLitExpr i    => $"int:{i.Value}",
        FloatLitExpr f  => $"float:{f.Value}",
        StringLitExpr s => $"str:{s.Value}",
        BoolLitExpr b   => $"bool:{b.Value}",
        MemberAccessExpr m when m.Target is IdentifierExpr id => $"{id.Name}.{m.Member}",
        IdentifierExpr id => id.Name,
        _ => null
    };

    private void AnalyzeWhile(WhileStmt w, Scope scope)
    {
        AnalyzeBoolCondition(w.Condition, scope);
        _loopDepth++;
        try { AnalyzeBody(w.Body, new Scope("while", scope)); }
        finally { _loopDepth--; }
    }

    private void AnalyzeDoLoop(DoLoopStmt dl, Scope scope)
    {
        _loopDepth++;
        try
        {
            AnalyzeBody(dl.Body, new Scope("do", scope));
            if (dl.Condition is not null) AnalyzeBoolCondition(dl.Condition, scope);
        }
        finally { _loopDepth--; }
    }

    private void AnalyzeFor(ForStmt f, Scope scope)
    {
        var inner = new Scope("for", scope);
        BpType iterT;
        switch (f.Range)
        {
            case ForNumericRange nr:
                {
                    var fromT = AnalyzeExpr(nr.From, scope: scope, expected: PrimitiveType.Integer);
                    var toT   = AnalyzeExpr(nr.To,   scope: scope, expected: PrimitiveType.Integer);
                    if (nr.Step is not null) AnalyzeExpr(nr.Step, scope: scope, expected: PrimitiveType.Integer);
                    if (!PrimitiveType.Integer.IsAssignableFrom(fromT) || !PrimitiveType.Integer.IsAssignableFrom(toT))
                        Err(f.Line, f.Column, "los límites del 'for' numérico deben ser integer");
                    iterT = PrimitiveType.Integer;
                    break;
                }
            case ForInRange inr:
                {
                    var collT = AnalyzeExpr(inr.Iterable, scope: scope);
                    if (collT is ArrayType at) iterT = at.Element;
                    else
                    {
                        Err(f.Line, f.Column, $"se esperaba array en 'for in', encontrado '{collT.Display}'");
                        iterT = ErrorType.Instance;
                    }
                    break;
                }
            default: iterT = ErrorType.Instance; break;
        }
        inner.TryDefine(new VarSymbol(f.IteratorName, false, false, null, true, f.Line, f.Column) { Type = iterT });
        _loopDepth++;
        try { AnalyzeBody(f.Body, inner); }
        finally { _loopDepth--; }
    }

    private void AnalyzeTry(TryStmt tr, Scope scope)
    {
        AnalyzeBody(tr.Body, new Scope("try", scope));
        bool seenCatchAll = false;
        foreach (var cl in tr.Catches)
        {
            if (seenCatchAll)
                Err(cl.Line, cl.Column, "catch inalcanzable: hay un 'catch' sin tipo previo");
            if (cl.ExceptionType is null) seenCatchAll = true;
            var inner = new Scope("catch", scope);
            if (cl.VarName is not null)
            {
                BpType? excT = null;
                if (cl.ExceptionType is not null)
                {
                    var sym = _module.Members.Resolve(cl.ExceptionType);
                    if (sym is ClassSymbol cs) excT = new ClassType(cs);
                    else
                    {
                        Err(cl.Line, cl.Column, $"tipo de excepción '{cl.ExceptionType}' no es una clase");
                        excT = ErrorType.Instance;
                    }
                }
                inner.TryDefine(new VarSymbol(cl.VarName, false, false, null, true, cl.Line, cl.Column) { Type = excT });
            }
            AnalyzeBody(cl.Body, inner);
        }
        if (tr.Finally is not null) AnalyzeBody(tr.Finally, new Scope("finally", scope));
    }

    private void AnalyzeReturn(ReturnStmt r, Scope scope)
    {
        if (_currentFunction is null) { Err(r.Line, r.Column, "'return' fuera de función"); return; }
        if (_currentFunction.IsConstructor && r.Value is not null)
            Err(r.Line, r.Column, "el constructor no puede retornar un valor");
        if (r.Value is null)
        {
            if (_currentFunction.ReturnType is not null && !_currentFunction.IsConstructor)
                Err(r.Line, r.Column, $"falta el valor de retorno (tipo '{_currentFunction.ReturnType.Display}')");
            return;
        }
        var t = AnalyzeExpr(r.Value, scope: scope, expected: _currentFunction.ReturnType);
        if (_currentFunction.ReturnType is not null && !_currentFunction.ReturnType.IsAssignableFrom(t))
            Err(r.Line, r.Column, $"return tipo '{t.Display}' incompatible con declarado '{_currentFunction.ReturnType.Display}'");
    }

    private void AnalyzeBoolCondition(IExpr e, Scope scope)
    {
        var t = AnalyzeExpr(e, scope: scope, expected: PrimitiveType.Boolean);
        if (!PrimitiveType.Boolean.IsAssignableFrom(t) && t is not ErrorType)
            Err((e as AstNode)!.Line, (e as AstNode)!.Column, $"se esperaba boolean, encontrado '{t.Display}'");
    }

    // ============================================================
    // EXPRESIONES — devuelven su tipo
    // ============================================================
    private BpType AnalyzeExpr(IExpr e, Scope? scope = null, BpType? expected = null)
    {
        BpType t = e switch
        {
            IntLitExpr     => PrimitiveType.Integer,
            FloatLitExpr   => PrimitiveType.Float,
            StringLitExpr  => PrimitiveType.String,
            BoolLitExpr    => PrimitiveType.Boolean,
            NullLitExpr    => NullType.Instance,
            ThisExpr th    => AnalyzeThis(th),
            SuperExpr sup  => AnalyzeSuperRef(sup),
            SuperCallExpr sc => AnalyzeSuperCall(sc, scope),
            FieldExpr fe   => AnalyzeField(fe),
            ParenExpr pe   => AnalyzeExpr(pe.Inner, scope, expected),
            ArrayLitExpr al => AnalyzeArrayLit(al, scope, expected),
            IdentifierExpr id => AnalyzeIdentifier(id, scope),
            MemberAccessExpr ma => AnalyzeMemberAccess(ma, scope),
            IndexExpr ix   => AnalyzeIndex(ix, scope),
            CallExpr ce    => AnalyzeCall(ce, scope),
            UnaryExpr u    => AnalyzeUnary(u, scope),
            BinaryExpr b   => AnalyzeBinary(b, scope),
            _ => ErrorType.Instance
        };
        _info.ExprTypes[e] = t;
        return t;
    }

    private BpType AnalyzeThis(ThisExpr th)
    {
        if (_currentClass is null || _currentFunction is null || _currentFunction.IsStatic)
        {
            Err(th.Line, th.Column, "'this' solo es válido en métodos de instancia");
            return ErrorType.Instance;
        }
        return new ClassType(_currentClass);
    }

    private BpType AnalyzeSuperRef(SuperExpr sup)
    {
        if (_currentClass is null || _currentClass.BaseClass is null)
        {
            Err(sup.Line, sup.Column, "'super' solo es válido en una subclase");
            return ErrorType.Instance;
        }
        if (_currentFunction is null || _currentFunction.IsStatic)
        {
            Err(sup.Line, sup.Column, "'super' solo es válido en métodos de instancia");
            return ErrorType.Instance;
        }
        return new ClassType(_currentClass.BaseClass);
    }

    private BpType AnalyzeSuperCall(SuperCallExpr sc, Scope? scope)
    {
        if (_currentFunction is null || !_currentFunction.IsConstructor)
        {
            Err(sc.Line, sc.Column, "'super(...)' solo es válido dentro de un constructor");
            return ErrorType.Instance;
        }
        if (!_isFirstStmtOfCtor)
            Err(sc.Line, sc.Column, "'super(...)' debe ser la primera sentencia del constructor");
        if (_currentClass?.BaseClass is null)
        {
            Err(sc.Line, sc.Column, "la clase no tiene clase base");
            return ErrorType.Instance;
        }
        _seenSuperCallInCtor = true;
        var baseCtor = _currentClass.BaseClass.FindConstructor();
        if (baseCtor is null)
        {
            // El padre tiene constructor por defecto (sin args).
            if (sc.Args.Count > 0)
                Err(sc.Line, sc.Column, $"la clase base '{_currentClass.BaseClass.Name}' no define constructor; super() no puede llevar argumentos");
        }
        else
        {
            CheckArgs(sc.Line, sc.Column, baseCtor.Params, sc.Args, scope);
        }
        return VoidType.Instance;
    }

    private BpType AnalyzeField(FieldExpr fe)
    {
        if (!_insideGetter && !_insideSetter)
        {
            Err(fe.Line, fe.Column, "'field' solo es válido dentro de get/set");
            return ErrorType.Instance;
        }
        return ErrorType.Instance; // tipo lo determina la propiedad; aquí no nos hace falta.
    }

    private BpType AnalyzeArrayLit(ArrayLitExpr al, Scope? scope, BpType? expected)
    {
        BpType? elemHint = expected is ArrayType at ? at.Element : null;
        BpType? inferred = null;
        foreach (var e in al.Elements)
        {
            var t = AnalyzeExpr(e, scope: scope, expected: elemHint);
            if (inferred is null) inferred = t;
            else if (!inferred.SameAs(t))
            {
                if (inferred.IsAssignableFrom(t)) { /* OK */ }
                else if (t.IsAssignableFrom(inferred)) inferred = t;
                else
                {
                    Err(al.Line, al.Column, $"elementos del array de tipos incompatibles: '{inferred.Display}' y '{t.Display}'");
                    inferred = ErrorType.Instance;
                }
            }
        }
        return new ArrayType(inferred ?? (elemHint ?? ErrorType.Instance));
    }

    private BpType AnalyzeIdentifier(IdentifierExpr id, Scope? scope)
    {
        // 1) scope local de función / for / etc.
        Symbol? sym = scope?.Resolve(id.Name);
        // 2) miembros de instancia accesibles via 'this' implícito
        if (sym is null && _currentClass is not null && _currentFunction is { IsStatic: false })
            sym = _currentClass.LookupInstance(id.Name);
        // 3) miembros del módulo
        sym ??= _module.Members.Resolve(id.Name);
        if (sym is null)
        {
            Err(id.Line, id.Column, $"identificador no resuelto: '{id.Name}'");
            return ErrorType.Instance;
        }
        _info.ExprSymbols[id] = sym;
        return TypeOfSymbol(sym);
    }

    private BpType AnalyzeMemberAccess(MemberAccessExpr ma, Scope? scope)
    {
        var tgtT = AnalyzeExpr(ma.Target, scope: scope);

        // Acceso estático: ClassName.member o EnumName.MEMBER
        if (ma.Target is IdentifierExpr id && _info.ExprSymbols.TryGetValue(id, out var idSym))
        {
            if (idSym is ClassSymbol cs)
            {
                var sub = cs.LookupStatic(ma.Member);
                if (sub is null) { Err(ma.Line, ma.Column, $"'{cs.Name}' no tiene miembro estático '{ma.Member}'"); return ErrorType.Instance; }
                CheckVisibility(ma.Line, ma.Column, sub, cs);
                _info.ExprSymbols[ma] = sub;
                return TypeOfSymbol(sub);
            }
            if (idSym is EnumSymbol es)
            {
                if (!es.Values.ContainsKey(ma.Member))
                { Err(ma.Line, ma.Column, $"el enum '{es.Name}' no tiene valor '{ma.Member}'"); return ErrorType.Instance; }
                return new EnumType(es);
            }
        }

        // Acceso de instancia
        if (tgtT is ClassType ct)
        {
            var sub = ct.Class.LookupInstance(ma.Member);
            if (sub is null) { Err(ma.Line, ma.Column, $"'{ct.Class.Name}' no tiene miembro de instancia '{ma.Member}'"); return ErrorType.Instance; }
            CheckVisibility(ma.Line, ma.Column, sub, ct.Class);
            _info.ExprSymbols[ma] = sub;
            return TypeOfSymbol(sub);
        }

        if (tgtT is ErrorType) return ErrorType.Instance;

        Err(ma.Line, ma.Column, $"el tipo '{tgtT.Display}' no tiene miembros");
        return ErrorType.Instance;
    }

    private BpType AnalyzeIndex(IndexExpr ix, Scope? scope)
    {
        var t = AnalyzeExpr(ix.Target, scope: scope);
        var idxT = AnalyzeExpr(ix.Index, scope: scope, expected: PrimitiveType.Integer);
        if (!PrimitiveType.Integer.IsAssignableFrom(idxT))
            Err(ix.Line, ix.Column, $"índice debe ser integer, no '{idxT.Display}'");
        if (t is ArrayType at) return at.Element;
        if (t is ErrorType) return ErrorType.Instance;
        Err(ix.Line, ix.Column, $"el tipo '{t.Display}' no es indexable");
        return ErrorType.Instance;
    }

    private BpType AnalyzeCall(CallExpr ce, Scope? scope)
    {
        // Identificar la función o constructor invocado.
        Symbol? target = null;
        BpType? selfType = null;
        if (ce.Callee is IdentifierExpr id)
        {
            // Resolución especial: prioridad a símbolo "callable" en el módulo (función o clase=constructor).
            target = _module.Members.Resolve(id.Name)
                  ?? scope?.Resolve(id.Name)
                  ?? (_currentClass is { } cc && _currentFunction is { IsStatic: false } ? cc.LookupInstance(id.Name) : null);
            if (target is not null) _info.ExprSymbols[id] = target;
        }
        else if (ce.Callee is MemberAccessExpr ma)
        {
            // Evalúa el target para obtener tipo y descubrir el miembro.
            AnalyzeExpr(ma, scope: scope);
            target = _info.ExprSymbols.TryGetValue(ma, out var s) ? s : null;
            selfType = _info.ExprTypes.TryGetValue(ma.Target, out var tt) ? tt : null;
        }

        // Llamada a clase ⇒ construcción.
        if (target is ClassSymbol cls)
        {
            // Constructor heredado del padre si la clase no define uno propio.
            var ctor = cls.FindConstructor();
            if (ctor is null)
            {
                if (ce.Args.Count > 0)
                    Err(ce.Line, ce.Column, $"la clase '{cls.Name}' no define constructor; no admite argumentos");
                foreach (var a in ce.Args) AnalyzeExpr(a, scope: scope);
            }
            else
            {
                CheckArgs(ce.Line, ce.Column, ctor.Params, ce.Args, scope);
            }
            return new ClassType(cls);
        }

        if (target is FunctionSymbol fn)
        {
            CheckArgs(ce.Line, ce.Column, fn.Params, ce.Args, scope);
            return fn.ReturnType ?? VoidType.Instance;
        }

        // Si no se identificó, evaluamos args para no perder anotaciones.
        foreach (var a in ce.Args) AnalyzeExpr(a, scope: scope);
        if (target is null && ce.Callee is IdentifierExpr id2)
            Err(ce.Line, ce.Column, $"no se puede llamar a '{id2.Name}'");
        return ErrorType.Instance;
    }

    private void CheckArgs(int line, int col, List<ParamSymbol> ps, List<IExpr> args, Scope? scope)
    {
        if (ps.Count != args.Count)
        {
            Err(line, col, $"número de argumentos incorrecto: se esperaban {ps.Count}, se pasaron {args.Count}");
        }
        int n = Math.Min(ps.Count, args.Count);
        for (int i = 0; i < n; i++)
        {
            var t = AnalyzeExpr(args[i], scope: scope, expected: ps[i].Type);
            if (ps[i].Type is not null && !ps[i].Type!.IsAssignableFrom(t))
                Err((args[i] as AstNode)!.Line, (args[i] as AstNode)!.Column,
                    $"argumento {i + 1}: '{t.Display}' no asignable a '{ps[i].Type!.Display}'");
        }
        // Procesar argumentos restantes para anotación.
        for (int i = n; i < args.Count; i++) AnalyzeExpr(args[i], scope: scope);
    }

    private BpType AnalyzeUnary(UnaryExpr u, Scope? scope)
    {
        var t = AnalyzeExpr(u.Operand, scope: scope);
        if (u.Op == "-")
        {
            if (!t.IsNumeric && t is not ErrorType)
                Err(u.Line, u.Column, $"'-' unario requiere numérico, no '{t.Display}'");
            return t.IsNumeric ? t : ErrorType.Instance;
        }
        if (u.Op == "not")
        {
            if (!PrimitiveType.Boolean.IsAssignableFrom(t) && t is not ErrorType)
                Err(u.Line, u.Column, $"'not' requiere boolean, no '{t.Display}'");
            return PrimitiveType.Boolean;
        }
        return ErrorType.Instance;
    }

    private BpType AnalyzeBinary(BinaryExpr b, Scope? scope)
    {
        var lt = AnalyzeExpr(b.Left,  scope: scope);
        var rt = AnalyzeExpr(b.Right, scope: scope);
        switch (b.Op)
        {
            case "+":
                if (lt is PrimitiveType { Tag: PrimitiveType.Kind.String } || rt is PrimitiveType { Tag: PrimitiveType.Kind.String })
                    return PrimitiveType.String; // concat
                if (lt.IsNumeric && rt.IsNumeric)
                    return Promote(lt, rt);
                Err(b.Line, b.Column, $"'+' incompatible: '{lt.Display}' y '{rt.Display}'");
                return ErrorType.Instance;
            case "-": case "*": case "/":
                if (lt.IsNumeric && rt.IsNumeric) return Promote(lt, rt);
                Err(b.Line, b.Column, $"'{b.Op}' requiere numéricos, encontrados '{lt.Display}' y '{rt.Display}'");
                return ErrorType.Instance;
            case "mod":
                if (lt.SameAs(PrimitiveType.Integer) && rt.SameAs(PrimitiveType.Integer)) return PrimitiveType.Integer;
                Err(b.Line, b.Column, "'mod' requiere integer");
                return ErrorType.Instance;
            case "&": case "|": case "xor": case "shl": case "shr":
                if (lt.SameAs(PrimitiveType.Integer) && rt.SameAs(PrimitiveType.Integer)) return PrimitiveType.Integer;
                Err(b.Line, b.Column, $"'{b.Op}' requiere integer");
                return ErrorType.Instance;
            case "and": case "or":
                if (PrimitiveType.Boolean.IsAssignableFrom(lt) && PrimitiveType.Boolean.IsAssignableFrom(rt))
                    return PrimitiveType.Boolean;
                Err(b.Line, b.Column, $"'{b.Op}' requiere boolean");
                return ErrorType.Instance;
            case "==": case "!=":
                if (!ComparableForEquality(lt, rt))
                    Err(b.Line, b.Column, $"'{b.Op}' incompatible: '{lt.Display}' vs '{rt.Display}'");
                return PrimitiveType.Boolean;
            case "<": case ">": case "<=": case ">=":
                if (lt.IsNumeric && rt.IsNumeric) return PrimitiveType.Boolean;
                if (lt is PrimitiveType { Tag: PrimitiveType.Kind.String } && rt is PrimitiveType { Tag: PrimitiveType.Kind.String }) return PrimitiveType.Boolean;
                Err(b.Line, b.Column, $"'{b.Op}' requiere numéricos o strings");
                return ErrorType.Instance;
        }
        return ErrorType.Instance;
    }

    private static BpType Promote(BpType a, BpType b)
    {
        if (a is PrimitiveType { Tag: PrimitiveType.Kind.Float } || b is PrimitiveType { Tag: PrimitiveType.Kind.Float })
            return PrimitiveType.Float;
        return PrimitiveType.Integer;
    }

    private static bool ComparableForEquality(BpType a, BpType b)
    {
        if (a is ErrorType || b is ErrorType) return true;
        if (a.SameAs(b)) return true;
        if (a is NullType && b.IsReference) return true;
        if (b is NullType && a.IsReference) return true;
        if (a.IsNumeric && b.IsNumeric) return true;
        return false;
    }

    private BpType TypeOfSymbol(Symbol s) => s switch
    {
        VarSymbol v       => v.Type ?? ErrorType.Instance,
        ConstSymbol c     => c.Type ?? ErrorType.Instance,
        ParamSymbol p     => p.Type ?? ErrorType.Instance,
        PropertySymbol pr => pr.Type ?? ErrorType.Instance,
        FunctionSymbol f  => f.ReturnType ?? VoidType.Instance,
        ClassSymbol cls   => new ClassType(cls),     // referencia a la clase como entidad (constructor/staticos)
        EnumSymbol en     => new EnumType(en),
        _ => ErrorType.Instance
    };

    // ============================================================
    // OVERRIDES Y FINAL (chequeo entre clases)
    // ============================================================
    private void CheckOverridesAndFinal(ClassSymbol cls)
    {
        if (cls.BaseClass is null) return;
        foreach (var s in cls.InstanceMembers.Symbols)
        {
            if (s is FunctionSymbol fn)
            {
                var baseSym = cls.BaseClass.LookupInstance(fn.Name);
                if (baseSym is FunctionSymbol baseFn)
                {
                    if (baseFn.IsFinal)
                        Err(fn.Line, fn.Column, $"'{fn.Name}' está marcada 'final' en la clase base, no se puede sobreescribir");
                    if (!fn.HasSameSignatureAs(baseFn))
                        Err(fn.Line, fn.Column, $"sobreescritura de '{fn.Name}' con firma incompatible");
                    var bret = baseFn.ReturnType?.Display ?? "void";
                    var fret = fn.ReturnType?.Display ?? "void";
                    if (bret != fret)
                        Err(fn.Line, fn.Column, $"sobreescritura de '{fn.Name}' cambia el tipo de retorno ('{bret}' vs '{fret}')");
                }
            }
            else if (s is PropertySymbol pp)
            {
                var baseSym = cls.BaseClass.LookupInstance(pp.Name);
                if (baseSym is PropertySymbol basePp && basePp.IsFinal)
                    Err(pp.Line, pp.Column, $"propiedad '{pp.Name}' es final en la base, no se puede sobreescribir");
            }
        }
    }

    // ============================================================
    // VISIBILIDAD
    // ============================================================
    private void CheckVisibility(int line, int col, Symbol sub, ClassSymbol? owner)
    {
        bool isPublic = sub switch
        {
            FunctionSymbol f => f.IsPublic,
            VarSymbol v      => v.IsPublic,
            ConstSymbol c    => c.IsPublic,
            PropertySymbol p => p.IsPublic,
            _ => true
        };
        // Dentro de la propia clase o subclase: siempre permitido.
        if (_currentClass is not null && owner is not null
            && (ReferenceEquals(_currentClass, owner) || _currentClass.IsSubclassOf(owner)))
            return;
        if (!isPublic)
            Err(line, col, $"miembro privado '{sub.Name}' inaccesible aquí");
    }

    // ============================================================
    // ENTRY POINTS
    // ============================================================
    private void ValidateModuleEntryPoints(ModuleNode mod)
    {
        if (_module.MainFunction is null)
        {
            Warn(mod.Line, mod.Column, $"el módulo '{mod.Name}' no define 'Main' (no podrá ser módulo principal)");
            return;
        }
        var m = _module.MainFunction;
        if (m.Params.Count != 0)
            Err(m.Line, m.Column, "'Main' no puede tener parámetros");
        if (m.ReturnType is not null && m.ReturnType is not VoidType)
            Err(m.Line, m.Column, "'Main' no puede declarar tipo de retorno");
    }

    // ============================================================
    // Diagnostics helpers
    // ============================================================
    private void Err(int line, int col, string msg) =>
        _info.Diagnostics.Add(new SemanticDiagnostic(DiagnosticKind.Error, msg, line, col));

    private void Warn(int line, int col, string msg) =>
        _info.Diagnostics.Add(new SemanticDiagnostic(DiagnosticKind.Warning, msg, line, col));
}

// ============================================================
// Extension auxiliar para detectar si una declaración es estática
// ============================================================
internal static class DeclExtensions
{
    public static bool IsStaticDecl(this ITopLevelDecl d, out string? qualifier)
    {
        qualifier = d switch
        {
            ConstDecl c when c.Name.IsStatic    => c.Name.ClassQualifier,
            VarDecl v when v.Names.Count > 0
                && v.Names[0].IsStatic           => v.Names[0].ClassQualifier,
            FuncDef f when f.Name.IsStatic      => f.Name.ClassQualifier,
            PropertyDef p when p.Name.IsStatic  => p.Name.ClassQualifier,
            _ => null
        };
        return qualifier is not null;
    }
}
