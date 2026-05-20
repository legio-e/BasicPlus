// ============================================================
// Parser.cs
// Analizador sintáctico de BASICPLUS.
//
// Recursive-descent (descenso recursivo). Cada regla EBNF se
// implementa en su propio método. La precedencia de operadores
// se codifica en la cascada or → and → not → cmp → add → mul →
// unary → primary → atom.
//
// Recuperación de errores: en cuanto un método de sentencia o
// declaración encuentra un error grave, se invoca Synchronize()
// que descarta tokens hasta el siguiente NEWLINE y reintenta.
// ============================================================

namespace BasicPlus.Lexer;

public sealed class Parser
{
    private readonly List<Token> _tokens;
    private readonly List<ParserError> _errors = new();
    private int _pos = 0;

    public IReadOnlyList<ParserError> Errors => _errors;

    public Parser(List<Token> tokens)
    {
        _tokens = tokens ?? throw new ArgumentNullException(nameof(tokens));
    }

    // ============================================================
    // PUNTO DE ENTRADA
    // ============================================================
    public ModuleNode? ParseModule()
    {
        SkipNewlines();
        var startTok = Current;

        if (!Match(TokenType.Module))
        {
            Error("se esperaba 'module' al inicio del fichero");
            return null;
        }

        string name = ConsumeIdentifier("nombre del módulo");
        ConsumeStmtTerminator("se esperaba salto de línea tras el nombre del módulo");

        var imports = new List<ImportNode>();
        SkipNewlines();
        while (Check(TokenType.Import))
        {
            imports.Add(ParseImport());
            SkipNewlines();
        }

        var defs = new List<ITopLevelDecl>();
        while (!IsAtEnd() && !Check(TokenType.End))
        {
            SkipNewlines();
            if (IsAtEnd() || Check(TokenType.End)) break;
            var def = ParseDefStmt();
            if (def is not null) defs.Add(def);
            SkipNewlines();
        }

        if (!Match(TokenType.End))
        {
            Error("se esperaba 'end' para cerrar el módulo");
        }
        else
        {
            // 'end' [name] NEWLINE
            if (Check(TokenType.Identifier)) Advance();
            ConsumeStmtTerminator("se esperaba salto de línea tras 'end'");
        }

        return new ModuleNode(name, imports, defs, startTok.Line, startTok.Column);
    }

    // ============================================================
    // IMPORTS
    // ============================================================
    private ImportNode ParseImport()
    {
        var tok = Current;
        Match(TokenType.Import);

        var path = new List<string>();
        path.Add(ConsumeIdentifier("nombre de librería"));
        while (Match(TokenType.Dot))
        {
            path.Add(ConsumeIdentifier("componente del path tras '.'"));
        }
        ConsumeStmtTerminator("se esperaba salto de línea tras 'import'");
        return new ImportNode(path, tok.Line, tok.Column);
    }

    // ============================================================
    // DEFINICIONES (top-level y miembros de clase)
    // ============================================================
    private ITopLevelDecl? ParseDefStmt()
    {
        bool isPublic = Match(TokenType.Public);
        bool isFinal  = false;

        // 'final' solo aplica a function/property; lo aceptamos antes
        // de la palabra clave correspondiente.
        if (Check(TokenType.Final))
        {
            isFinal = true;
            Advance();
        }

        var tok = Current;
        switch (tok.Type)
        {
            case TokenType.Const:    return ParseConstDecl(isPublic);
            case TokenType.Var:      return ParseVarDecl(isPublic);
            case TokenType.Function: return ParseFuncDef(isPublic, isFinal);
            case TokenType.Property: return ParsePropertyDef(isPublic, isFinal);
            case TokenType.Class:    return ParseClassDef(isPublic);
            case TokenType.Enum:     return ParseEnumDef(isPublic);
            default:
                Error($"se esperaba const/var/function/property/class/enum, encontrado '{tok.Lexeme}'");
                Synchronize();
                return null;
        }
    }

    // ============================================================
    // CONST / VAR
    // ============================================================
    private ConstDecl ParseConstDecl(bool isPublic)
    {
        var tok = Current;
        Match(TokenType.Const);
        var name = ParseDeclName();
        TypeRef? type = null;
        if (Match(TokenType.Colon)) type = ParseType();
        Consume(TokenType.Assign, "se esperaba ':=' en la declaración de la constante");
        var value = ParseExpr();
        ConsumeStmtTerminator("se esperaba ';' o salto de línea tras la constante");
        return new ConstDecl(isPublic, name, type, value, tok.Line, tok.Column);
    }

    private VarDecl ParseVarDecl(bool isPublic)
    {
        var tok = Current;
        Match(TokenType.Var);
        var names = new List<DeclName> { ParseDeclName() };
        while (Match(TokenType.Comma))
            names.Add(ParseDeclName());
        Consume(TokenType.Colon, "se esperaba ':' tras el nombre de la variable");
        var type = ParseType();
        IExpr? init = null;
        if (Match(TokenType.Assign)) init = ParseExpr();
        ConsumeStmtTerminator("se esperaba ';' o salto de línea tras la declaración");
        return new VarDecl(isPublic, names, type, init, tok.Line, tok.Column);
    }

    /// <summary>
    /// decl_name ::= name [ '.' name ]   — soporta miembros estáticos.
    /// </summary>
    private DeclName ParseDeclName()
    {
        var tok = Current;
        string first = ConsumeIdentifier("nombre");
        if (Match(TokenType.Dot))
        {
            string second = ConsumeIdentifier("miembro tras 'NombreClase.'");
            return new DeclName(first, second, tok.Line, tok.Column);
        }
        return new DeclName(null, first, tok.Line, tok.Column);
    }

    // ============================================================
    // TYPE
    // ============================================================
    private TypeRef ParseType()
    {
        var tok = Current;
        TypeRef baseType;
        switch (tok.Type)
        {
            case TokenType.Integer:    baseType = new SimpleTypeRef("integer", tok.Line, tok.Column); Advance(); break;
            case TokenType.Float:      baseType = new SimpleTypeRef("float",   tok.Line, tok.Column); Advance(); break;
            case TokenType.String:     baseType = new SimpleTypeRef("string",  tok.Line, tok.Column); Advance(); break;
            case TokenType.Boolean:    baseType = new SimpleTypeRef("boolean", tok.Line, tok.Column); Advance(); break;
            case TokenType.Identifier: baseType = new SimpleTypeRef(tok.Lexeme, tok.Line, tok.Column); Advance(); break;
            default:
                Error($"se esperaba un tipo, encontrado '{tok.Lexeme}'");
                Advance();
                baseType = new SimpleTypeRef("?", tok.Line, tok.Column);
                break;
        }
        // Sufijo de array: '[' [ expr ] ']'
        if (Match(TokenType.LBracket))
        {
            IExpr? size = null;
            if (!Check(TokenType.RBracket)) size = ParseExpr();
            Consume(TokenType.RBracket, "se esperaba ']' en el tipo array");
            return new ArrayTypeRef(baseType, size, tok.Line, tok.Column);
        }
        return baseType;
    }

    // ============================================================
    // FUNCIONES
    // ============================================================
    private FuncDef ParseFuncDef(bool isPublic, bool isFinal)
    {
        var tok = Current;
        Match(TokenType.Function);
        var name = ParseDeclName();

        Consume(TokenType.LParen, "se esperaba '(' tras el nombre de la función");
        var paramList = new List<Param>();
        if (!Check(TokenType.RParen))
        {
            paramList.Add(ParseParam());
            while (Match(TokenType.Comma)) paramList.Add(ParseParam());
        }
        Consume(TokenType.RParen, "se esperaba ')' al final de los parámetros");

        TypeRef? retType = null;
        if (Match(TokenType.Colon)) retType = ParseType();
        ConsumeStmtTerminator("se esperaba salto de línea tras la cabecera de la función");

        var body = ParseBody(TokenType.End);
        Consume(TokenType.End, "se esperaba 'end' al final de la función");
        // 'end' [decl_name] NEWLINE
        if (Check(TokenType.Identifier)) ParseDeclName();
        ConsumeStmtTerminator("se esperaba salto de línea tras 'end'");
        return new FuncDef(isPublic, isFinal, name, paramList, retType, body, tok.Line, tok.Column);
    }

    private Param ParseParam()
    {
        var tok = Current;
        string n = ConsumeIdentifier("nombre del parámetro");
        Consume(TokenType.Colon, "se esperaba ':' tras el nombre del parámetro");
        var t = ParseType();
        return new Param(n, t, tok.Line, tok.Column);
    }

    // ============================================================
    // PROPIEDADES
    // ============================================================
    private PropertyDef ParsePropertyDef(bool isPublic, bool isFinal)
    {
        var tok = Current;
        Match(TokenType.Property);
        var name = ParseDeclName();
        Consume(TokenType.Colon, "se esperaba ':' tras el nombre de la propiedad");
        var type = ParseType();
        IExpr? init = null;
        if (Match(TokenType.Assign)) init = ParseExpr();

        // Forma corta: ';' | NEWLINE   -> get/set implícitos
        // Forma extendida: NEWLINE [getter] [setter] 'endprop' NEWLINE
        if (Match(TokenType.Semicolon))
            return new PropertyDef(isPublic, isFinal, name, type, init, null, null, true, tok.Line, tok.Column);

        ConsumeStmtTerminator("se esperaba salto de línea tras la declaración de la propiedad");

        // Si la siguiente línea no abre 'get'/'set', es forma corta.
        SkipNewlines();
        if (!Check(TokenType.Get) && !Check(TokenType.Set))
            return new PropertyDef(isPublic, isFinal, name, type, init, null, null, true, tok.Line, tok.Column);

        GetterDef? getter = null;
        SetterDef? setter = null;
        if (Check(TokenType.Get)) { getter = ParseGetter(); SkipNewlines(); }
        if (Check(TokenType.Set)) { setter = ParseSetter(); SkipNewlines(); }
        Consume(TokenType.EndProp, "se esperaba 'endprop' al final de la propiedad");
        ConsumeStmtTerminator("se esperaba salto de línea tras 'endprop'");
        return new PropertyDef(isPublic, isFinal, name, type, init, getter, setter, false, tok.Line, tok.Column);
    }

    private GetterDef ParseGetter()
    {
        var tok = Current;
        Match(TokenType.Get);
        ConsumeStmtTerminator("se esperaba salto de línea tras 'get'");
        var body = ParseBody(TokenType.EndGet);
        Consume(TokenType.EndGet, "se esperaba 'endget' al final del getter");
        ConsumeStmtTerminator("se esperaba salto de línea tras 'endget'");
        return new GetterDef(body, tok.Line, tok.Column);
    }

    private SetterDef ParseSetter()
    {
        var tok = Current;
        Match(TokenType.Set);
        Consume(TokenType.LParen, "se esperaba '(' tras 'set'");
        string param = ConsumeIdentifier("nombre del parámetro del setter");
        Consume(TokenType.RParen, "se esperaba ')' tras el parámetro del setter");
        ConsumeStmtTerminator("se esperaba salto de línea tras 'set(...)'");
        var body = ParseBody(TokenType.EndSet);
        Consume(TokenType.EndSet, "se esperaba 'endset' al final del setter");
        ConsumeStmtTerminator("se esperaba salto de línea tras 'endset'");
        return new SetterDef(param, body, tok.Line, tok.Column);
    }

    // ============================================================
    // CLASES
    // ============================================================
    private ClassDef ParseClassDef(bool isPublic)
    {
        var tok = Current;
        Match(TokenType.Class);
        string name = ConsumeIdentifier("nombre de la clase");
        string? baseClass = null;
        if (Match(TokenType.Extends)) baseClass = ConsumeIdentifier("clase base");
        ConsumeStmtTerminator("se esperaba salto de línea tras la cabecera de la clase");

        var members = new List<ITopLevelDecl>();
        while (!IsAtEnd() && !Check(TokenType.End))
        {
            SkipNewlines();
            if (IsAtEnd() || Check(TokenType.End)) break;
            var m = ParseDefStmt();   // mismas declaraciones que en módulo (pero sin enum/class anidados a este nivel: lo dejamos pasar y semántica lo veta)
            if (m is not null) members.Add(m);
            SkipNewlines();
        }
        Consume(TokenType.End, "se esperaba 'end' al final de la clase");
        if (Check(TokenType.Identifier)) Advance();
        ConsumeStmtTerminator("se esperaba salto de línea tras 'end'");
        return new ClassDef(isPublic, name, baseClass, members, tok.Line, tok.Column);
    }

    // ============================================================
    // ENUMS
    // ============================================================
    private EnumDef ParseEnumDef(bool isPublic)
    {
        var tok = Current;
        Match(TokenType.Enum);
        string name = ConsumeIdentifier("nombre del enum");
        ConsumeStmtTerminator("se esperaba salto de línea tras el nombre del enum");
        var values = new List<EnumValue>();
        while (!IsAtEnd() && !Check(TokenType.End))
        {
            SkipNewlines();
            if (IsAtEnd() || Check(TokenType.End)) break;
            values.Add(ParseEnumValue());
        }
        Consume(TokenType.End, "se esperaba 'end' al final del enum");
        if (Check(TokenType.Identifier)) Advance();
        ConsumeStmtTerminator("se esperaba salto de línea tras 'end'");
        return new EnumDef(isPublic, name, values, tok.Line, tok.Column);
    }

    private EnumValue ParseEnumValue()
    {
        var tok = Current;
        string n = ConsumeIdentifier("nombre del miembro del enum");
        long? explicitVal = null;
        if (Match(TokenType.Assign))
        {
            var lit = Current;
            if (lit.Type == TokenType.IntegerLit && lit.Value is long lv)
            {
                explicitVal = lv;
                Advance();
            }
            else
            {
                Error("se esperaba un literal entero tras ':='");
            }
        }
        ConsumeStmtTerminator("se esperaba salto de línea tras el miembro del enum");
        return new EnumValue(n, explicitVal, tok.Line, tok.Column);
    }

    // ============================================================
    // CUERPO Y SENTENCIAS
    // ============================================================
    private List<IStmt> ParseBody(params TokenType[] stopAt)
    {
        var stmts = new List<IStmt>();
        while (!IsAtEnd())
        {
            SkipNewlines();
            if (IsAtEnd()) break;
            if (stopAt.Contains(Current.Type)) break;
            // Otros terminadores comunes que pueden aparecer dentro de
            // construcciones anidadas (elseif/else/endif/case/default/...).
            if (IsBlockTerminator(Current.Type)) break;
            var s = ParseStmt();
            if (s is not null) stmts.Add(s);
        }
        return stmts;
    }

    private static bool IsBlockTerminator(TokenType t) => t switch
    {
        TokenType.End or TokenType.EndIf or TokenType.EndWh or TokenType.EndSw or
        TokenType.EndTry or TokenType.EndProp or TokenType.EndGet or TokenType.EndSet or
        TokenType.ElseIf or TokenType.Else or
        TokenType.Case or TokenType.Default or
        TokenType.Catch or TokenType.Finally or
        TokenType.Next or TokenType.Loop => true,
        _ => false
    };

    private IStmt? ParseStmt()
    {
        var t = Current.Type;
        switch (t)
        {
            case TokenType.Var:      return ParseVarDecl(false);
            case TokenType.Const:    return ParseConstDecl(false);
            case TokenType.If:       return ParseIfStmt();
            case TokenType.Switch:   return ParseSwitchStmt();
            case TokenType.While:    return ParseWhileStmt();
            case TokenType.Do:       return ParseDoLoopStmt();
            case TokenType.For:      return ParseForStmt();
            case TokenType.Try:      return ParseTryStmt();
            case TokenType.Return:   return ParseReturnStmt();
            case TokenType.Throw:    return ParseThrowStmt();
            case TokenType.Print:    return ParsePrintStmt();
            case TokenType.Break:
            {
                var tok = Current; Advance();
                ConsumeStmtTerminator("se esperaba salto de línea tras 'break'");
                return new BreakStmt(tok.Line, tok.Column);
            }
            case TokenType.Continue:
            {
                var tok = Current; Advance();
                ConsumeStmtTerminator("se esperaba salto de línea tras 'continue'");
                return new ContinueStmt(tok.Line, tok.Column);
            }
            default:
                return ParseAssignOrCallStmt();
        }
    }

    /// <summary>
    /// stmt en posición no-keyword: o bien es una asignación
    /// (primary assign_op expr) o bien una llamada como sentencia.
    /// </summary>
    private IStmt? ParseAssignOrCallStmt()
    {
        var startTok = Current;
        IExpr lhs;
        try { lhs = ParsePrimary(); }
        catch
        {
            Synchronize();
            return null;
        }

        // ¿Asignación?
        if (Check(TokenType.Assign) || Check(TokenType.PlusAssign) || Check(TokenType.MinusAssign))
        {
            AssignOpKind op = Current.Type switch
            {
                TokenType.Assign       => AssignOpKind.Assign,
                TokenType.PlusAssign   => AssignOpKind.PlusAssign,
                TokenType.MinusAssign  => AssignOpKind.MinusAssign,
                _ => AssignOpKind.Assign
            };
            Advance();
            var rhs = ParseExpr();
            ConsumeStmtTerminator("se esperaba ';' o salto de línea tras la asignación");
            return new AssignStmt(lhs, op, rhs, startTok.Line, startTok.Column);
        }

        // Si no es asignación, debe ser una llamada como sentencia.
        if (lhs is CallExpr || lhs is SuperCallExpr)
        {
            ConsumeStmtTerminator("se esperaba salto de línea tras la llamada");
            return new ExprStmt(lhs, startTok.Line, startTok.Column);
        }

        Error("se esperaba una asignación o una llamada como sentencia");
        Synchronize();
        return null;
    }

    // ---------- IF ----------
    private IfStmt ParseIfStmt()
    {
        var tok = Current;
        Match(TokenType.If);
        var cond = ParseExpr();
        Consume(TokenType.Then, "se esperaba 'then' tras la condición del 'if'");

        // Forma de una línea: 'if cond then stmt NEWLINE'
        if (!Check(TokenType.Newline))
        {
            var stmt = ParseStmt();
            var thenClause = new IfClause(cond, stmt is null ? new() : new() { stmt }, tok.Line, tok.Column);
            return new IfStmt(thenClause, new(), null, true, tok.Line, tok.Column);
        }

        // Multi-línea
        ConsumeStmtTerminator("se esperaba salto de línea tras 'then'");
        var thenBody = ParseBody(TokenType.ElseIf, TokenType.Else, TokenType.EndIf);
        var thenC = new IfClause(cond, thenBody, tok.Line, tok.Column);

        var elseifs = new List<IfClause>();
        while (Check(TokenType.ElseIf))
        {
            var elseifTok = Current;
            Advance();
            var ec = ParseExpr();
            Consume(TokenType.Then, "se esperaba 'then' tras la condición del 'elseif'");
            ConsumeStmtTerminator("se esperaba salto de línea tras 'then'");
            var eb = ParseBody(TokenType.ElseIf, TokenType.Else, TokenType.EndIf);
            elseifs.Add(new IfClause(ec, eb, elseifTok.Line, elseifTok.Column));
        }

        List<IStmt>? elseBody = null;
        if (Match(TokenType.Else))
        {
            ConsumeStmtTerminator("se esperaba salto de línea tras 'else'");
            elseBody = ParseBody(TokenType.EndIf);
        }
        Consume(TokenType.EndIf, "se esperaba 'endif'");
        ConsumeStmtTerminator("se esperaba salto de línea tras 'endif'");
        return new IfStmt(thenC, elseifs, elseBody, false, tok.Line, tok.Column);
    }

    // ---------- SWITCH ----------
    private SwitchStmt ParseSwitchStmt()
    {
        var tok = Current;
        Match(TokenType.Switch);
        var subject = ParseExpr();
        ConsumeStmtTerminator("se esperaba salto de línea tras la expresión del 'switch'");
        SkipNewlines();

        var cases = new List<CaseClause>();
        List<IStmt>? def = null;
        while (Check(TokenType.Case))
        {
            var caseTok = Current;
            Advance();
            var values = new List<IExpr> { ParseExpr() };
            while (Match(TokenType.Comma)) values.Add(ParseExpr());
            ConsumeStmtTerminator("se esperaba salto de línea tras la lista del 'case'");
            var body = ParseBody(TokenType.Case, TokenType.Default, TokenType.EndSw);
            cases.Add(new CaseClause(values, body, caseTok.Line, caseTok.Column));
            SkipNewlines();
        }
        if (Match(TokenType.Default))
        {
            ConsumeStmtTerminator("se esperaba salto de línea tras 'default'");
            def = ParseBody(TokenType.EndSw);
        }
        Consume(TokenType.EndSw, "se esperaba 'endsw'");
        ConsumeStmtTerminator("se esperaba salto de línea tras 'endsw'");
        return new SwitchStmt(subject, cases, def, tok.Line, tok.Column);
    }

    // ---------- WHILE ----------
    private WhileStmt ParseWhileStmt()
    {
        var tok = Current;
        Match(TokenType.While);
        var cond = ParseExpr();
        Consume(TokenType.Do, "se esperaba 'do' tras la condición del 'while'");

        if (!Check(TokenType.Newline))
        {
            var stmt = ParseStmt();
            return new WhileStmt(cond, stmt is null ? new() : new() { stmt }, true, tok.Line, tok.Column);
        }

        ConsumeStmtTerminator("se esperaba salto de línea tras 'do'");
        var body = ParseBody(TokenType.EndWh);
        Consume(TokenType.EndWh, "se esperaba 'endwh'");
        ConsumeStmtTerminator("se esperaba salto de línea tras 'endwh'");
        return new WhileStmt(cond, body, false, tok.Line, tok.Column);
    }

    // ---------- DO ... LOOP [cond] ----------
    private DoLoopStmt ParseDoLoopStmt()
    {
        var tok = Current;
        Match(TokenType.Do);
        ConsumeStmtTerminator("se esperaba salto de línea tras 'do'");
        var body = ParseBody(TokenType.Loop);
        Consume(TokenType.Loop, "se esperaba 'loop'");
        IExpr? cond = null;
        if (!Check(TokenType.Newline) && !Check(TokenType.Semicolon) && !IsAtEnd())
            cond = ParseExpr();
        ConsumeStmtTerminator("se esperaba salto de línea tras 'loop'");
        return new DoLoopStmt(body, cond, tok.Line, tok.Column);
    }

    // ---------- FOR ----------
    private ForStmt ParseForStmt()
    {
        var tok = Current;
        Match(TokenType.For);
        string iter = ConsumeIdentifier("nombre del iterador del 'for'");

        ForRange range;
        if (Match(TokenType.Assign))
        {
            var from = ParseExpr();
            Consume(TokenType.To, "se esperaba 'to' en el 'for' numérico");
            var to = ParseExpr();
            IExpr? step = null;
            if (Match(TokenType.Step)) step = ParseExpr();
            range = new ForNumericRange(from, to, step, tok.Line, tok.Column);
        }
        else if (Match(TokenType.In))
        {
            var iterable = ParseExpr();
            range = new ForInRange(iterable, tok.Line, tok.Column);
        }
        else
        {
            Error("se esperaba ':=' (numérico) o 'in' (foreach) en el 'for'");
            range = new ForInRange(new NullLitExpr(tok.Line, tok.Column), tok.Line, tok.Column);
        }
        Consume(TokenType.Do, "se esperaba 'do' tras la cabecera del 'for'");

        if (!Check(TokenType.Newline))
        {
            var stmt = ParseStmt();
            return new ForStmt(iter, range, stmt is null ? new() : new() { stmt }, true, tok.Line, tok.Column);
        }

        ConsumeStmtTerminator("se esperaba salto de línea tras 'do'");
        var body = ParseBody(TokenType.Next);
        Consume(TokenType.Next, "se esperaba 'next' al final del 'for'");
        if (Check(TokenType.Identifier)) Advance();
        ConsumeStmtTerminator("se esperaba salto de línea tras 'next'");
        return new ForStmt(iter, range, body, false, tok.Line, tok.Column);
    }

    // ---------- TRY ----------
    private TryStmt ParseTryStmt()
    {
        var tok = Current;
        Match(TokenType.Try);
        ConsumeStmtTerminator("se esperaba salto de línea tras 'try'");
        var body = ParseBody(TokenType.Catch, TokenType.Finally, TokenType.EndTry);

        var catches = new List<CatchClause>();
        while (Check(TokenType.Catch))
        {
            var cTok = Current;
            Advance();
            string? varName = null;
            string? excType = null;
            if (Check(TokenType.Identifier))
            {
                varName = Current.Lexeme;
                Advance();
                if (Match(TokenType.Colon))
                    excType = ConsumeIdentifier("tipo de la excepción tras ':'");
            }
            ConsumeStmtTerminator("se esperaba salto de línea tras la cabecera del 'catch'");
            var cb = ParseBody(TokenType.Catch, TokenType.Finally, TokenType.EndTry);
            catches.Add(new CatchClause(varName, excType, cb, cTok.Line, cTok.Column));
        }
        List<IStmt>? fin = null;
        if (Match(TokenType.Finally))
        {
            ConsumeStmtTerminator("se esperaba salto de línea tras 'finally'");
            fin = ParseBody(TokenType.EndTry);
        }
        Consume(TokenType.EndTry, "se esperaba 'endtry'");
        ConsumeStmtTerminator("se esperaba salto de línea tras 'endtry'");
        return new TryStmt(body, catches, fin, tok.Line, tok.Column);
    }

    // ---------- RETURN / THROW ----------
    private ReturnStmt ParseReturnStmt()
    {
        var tok = Current;
        Match(TokenType.Return);
        IExpr? val = null;
        if (!Check(TokenType.Newline) && !Check(TokenType.Semicolon) && !IsAtEnd())
            val = ParseExpr();
        ConsumeStmtTerminator("se esperaba salto de línea tras 'return'");
        return new ReturnStmt(val, tok.Line, tok.Column);
    }

    private ThrowStmt ParseThrowStmt()
    {
        var tok = Current;
        Match(TokenType.Throw);
        var v = ParseExpr();
        ConsumeStmtTerminator("se esperaba salto de línea tras 'throw'");
        return new ThrowStmt(v, tok.Line, tok.Column);
    }

    // ---------- PRINT ----------
    private PrintStmt ParsePrintStmt()
    {
        var tok = Current;
        Match(TokenType.Print);
        var items = new List<PrintItem>();

        if (!Check(TokenType.Newline) && !IsAtEnd() && !Check(TokenType.Semicolon))
        {
            items.Add(new PrintItem(PrintSep.None, ParseExpr()));
            while (Check(TokenType.Comma) || Check(TokenType.Semicolon))
            {
                PrintSep sep = Current.Type == TokenType.Comma ? PrintSep.Comma : PrintSep.Semicolon;
                Advance();
                IExpr? e = null;
                if (!Check(TokenType.Comma) && !Check(TokenType.Semicolon) && !Check(TokenType.Newline) && !IsAtEnd())
                    e = ParseExpr();
                items.Add(new PrintItem(sep, e));
            }
        }
        ConsumeStmtTerminator("se esperaba salto de línea tras 'print'");
        return new PrintStmt(items, tok.Line, tok.Column);
    }

    // ============================================================
    // EXPRESIONES (precedencia)
    // ============================================================
    private IExpr ParseExpr() => ParseOr();

    private IExpr ParseOr()
    {
        var left = ParseAnd();
        while (Check(TokenType.Or))
        {
            var op = Current; Advance();
            var right = ParseAnd();
            left = new BinaryExpr("or", left, right, op.Line, op.Column);
        }
        return left;
    }

    private IExpr ParseAnd()
    {
        var left = ParseNot();
        while (Check(TokenType.And))
        {
            var op = Current; Advance();
            var right = ParseNot();
            left = new BinaryExpr("and", left, right, op.Line, op.Column);
        }
        return left;
    }

    private IExpr ParseNot()
    {
        if (Check(TokenType.Not))
        {
            var op = Current; Advance();
            var inner = ParseCmp();
            return new UnaryExpr("not", inner, op.Line, op.Column);
        }
        return ParseCmp();
    }

    private IExpr ParseCmp()
    {
        var left = ParseAdd();
        if (Check(TokenType.Eq) || Check(TokenType.Neq) ||
            Check(TokenType.Lt) || Check(TokenType.Gt) ||
            Check(TokenType.Le) || Check(TokenType.Ge))
        {
            var op = Current; Advance();
            var right = ParseAdd();
            return new BinaryExpr(op.Lexeme, left, right, op.Line, op.Column);
        }
        return left;
    }

    private IExpr ParseAdd()
    {
        var left = ParseMul();
        while (Check(TokenType.Plus) || Check(TokenType.Minus) ||
               Check(TokenType.Bar)  || Check(TokenType.Xor))
        {
            var op = Current; Advance();
            var right = ParseMul();
            left = new BinaryExpr(op.Lexeme, left, right, op.Line, op.Column);
        }
        return left;
    }

    private IExpr ParseMul()
    {
        var left = ParseUnary();
        while (Check(TokenType.Star) || Check(TokenType.Slash) ||
               Check(TokenType.Mod)  || Check(TokenType.Amp)   ||
               Check(TokenType.Shl)  || Check(TokenType.Shr))
        {
            var op = Current; Advance();
            var right = ParseUnary();
            left = new BinaryExpr(op.Lexeme, left, right, op.Line, op.Column);
        }
        return left;
    }

    private IExpr ParseUnary()
    {
        if (Check(TokenType.Minus))
        {
            var op = Current; Advance();
            var inner = ParsePrimary();
            return new UnaryExpr("-", inner, op.Line, op.Column);
        }
        return ParsePrimary();
    }

    /// <summary>
    /// primary ::= atom { '.' (call | name) | '[' expr ']' }
    /// Una llamada también puede aparecer DIRECTAMENTE sobre el resultado
    /// del primary previo (no está en EBNF original pero sigue siendo
    /// determinístico y soporta 'super(...)').
    /// </summary>
    private IExpr ParsePrimary()
    {
        var node = ParseAtom();
        while (true)
        {
            if (Check(TokenType.Dot))
            {
                var op = Current; Advance();
                string member = ConsumeIdentifier("nombre tras '.'");
                node = new MemberAccessExpr(node, member, op.Line, op.Column);
                if (Check(TokenType.LParen))
                {
                    var lp = Current; Advance();
                    var args = ParseArgList();
                    Consume(TokenType.RParen, "se esperaba ')'");
                    node = new CallExpr(node, args, lp.Line, lp.Column);
                }
            }
            else if (Check(TokenType.LBracket))
            {
                var op = Current; Advance();
                var idx = ParseExpr();
                Consume(TokenType.RBracket, "se esperaba ']'");
                node = new IndexExpr(node, idx, op.Line, op.Column);
            }
            else
            {
                break;
            }
        }
        return node;
    }

    private List<IExpr> ParseArgList()
    {
        var args = new List<IExpr>();
        if (!Check(TokenType.RParen))
        {
            args.Add(ParseExpr());
            while (Match(TokenType.Comma)) args.Add(ParseExpr());
        }
        return args;
    }

    private IExpr ParseAtom()
    {
        var tok = Current;
        switch (tok.Type)
        {
            case TokenType.IntegerLit:
                Advance();
                return new IntLitExpr((long)(tok.Value ?? 0L), tok.Line, tok.Column);
            case TokenType.FloatLit:
                Advance();
                return new FloatLitExpr((double)(tok.Value ?? 0.0), tok.Line, tok.Column);
            case TokenType.StringLit:
                Advance();
                return new StringLitExpr((string)(tok.Value ?? ""), tok.Line, tok.Column);
            case TokenType.True:
                Advance();
                return new BoolLitExpr(true, tok.Line, tok.Column);
            case TokenType.False:
                Advance();
                return new BoolLitExpr(false, tok.Line, tok.Column);
            case TokenType.Null:
                Advance();
                return new NullLitExpr(tok.Line, tok.Column);
            case TokenType.This:
                Advance();
                return new ThisExpr(tok.Line, tok.Column);
            case TokenType.Field:
                Advance();
                return new FieldExpr(tok.Line, tok.Column);
            case TokenType.Super:
                Advance();
                if (Check(TokenType.LParen))
                {
                    Advance();
                    var args = ParseArgList();
                    Consume(TokenType.RParen, "se esperaba ')' tras 'super('");
                    return new SuperCallExpr(args, tok.Line, tok.Column);
                }
                return new SuperExpr(tok.Line, tok.Column);
            case TokenType.LParen:
                Advance();
                var inner = ParseExpr();
                Consume(TokenType.RParen, "se esperaba ')'");
                return new ParenExpr(inner, tok.Line, tok.Column);
            case TokenType.LBracket:
                return ParseArrayLiteral();
            case TokenType.Identifier:
                Advance();
                if (Check(TokenType.LParen))
                {
                    var lp = Current; Advance();
                    var args = ParseArgList();
                    Consume(TokenType.RParen, "se esperaba ')'");
                    return new CallExpr(new IdentifierExpr(tok.Lexeme, tok.Line, tok.Column), args, lp.Line, lp.Column);
                }
                return new IdentifierExpr(tok.Lexeme, tok.Line, tok.Column);
            default:
                Error($"se esperaba una expresión, encontrado '{tok.Lexeme}'");
                Advance();
                return new NullLitExpr(tok.Line, tok.Column);
        }
    }

    private IExpr ParseArrayLiteral()
    {
        var tok = Current;
        Match(TokenType.LBracket);
        var elems = new List<IExpr>();
        if (!Check(TokenType.RBracket))
        {
            elems.Add(ParseExpr());
            while (Match(TokenType.Comma)) elems.Add(ParseExpr());
        }
        Consume(TokenType.RBracket, "se esperaba ']' al final del array literal");
        return new ArrayLitExpr(elems, tok.Line, tok.Column);
    }

    // ============================================================
    // HELPERS DE CURSOR
    // ============================================================
    private Token Current => _tokens[_pos];

    private bool IsAtEnd() => Current.Type == TokenType.Eof;

    private bool Check(TokenType t) => Current.Type == t;

    private bool Match(TokenType t)
    {
        if (!Check(t)) return false;
        Advance();
        return true;
    }

    private Token Advance()
    {
        var t = Current;
        if (!IsAtEnd()) _pos++;
        return t;
    }

    private Token Consume(TokenType t, string msg)
    {
        if (Check(t)) return Advance();
        Error(msg);
        return Current;
    }

    private string ConsumeIdentifier(string what)
    {
        if (Check(TokenType.Identifier))
        {
            string name = Current.Lexeme;
            Advance();
            return name;
        }
        Error($"se esperaba {what}, encontrado '{Current.Lexeme}'");
        return "?";
    }

    /// <summary>
    /// Acepta NEWLINE o ';' como terminador de sentencia.
    /// Si hay EOF, también pasa (no obliga al fichero a acabar con \n).
    /// </summary>
    private void ConsumeStmtTerminator(string msg)
    {
        if (Match(TokenType.Newline)) return;
        if (Match(TokenType.Semicolon)) return;
        if (IsAtEnd()) return;
        Error(msg);
    }

    private void SkipNewlines()
    {
        while (Check(TokenType.Newline)) Advance();
    }

    private void Error(string message)
    {
        var t = Current;
        _errors.Add(new ParserError(message, t.Line, t.Column));
    }

    /// <summary>
    /// Recuperación de errores: descarta tokens hasta el próximo
    /// NEWLINE para permitir seguir parseando lo que venga después.
    /// </summary>
    private void Synchronize()
    {
        while (!IsAtEnd() && !Check(TokenType.Newline)) Advance();
        SkipNewlines();
    }
}
