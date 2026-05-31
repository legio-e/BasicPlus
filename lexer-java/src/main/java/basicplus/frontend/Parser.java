// ============================================================
// Parser.java
// Analizador sintáctico de BASICPLUS (port a Java 8 del Parser.cs).
//
// Recursive-descent: cada regla EBNF se implementa en su propio método.
// Precedencia de operadores codificada en la cascada
//   or → and → not → cmp → add → mul → unary → primary → atom.
//
// Recuperación de errores: en un fallo grave, synchronize() descarta
// tokens hasta el siguiente NEWLINE y reintenta.
// ============================================================
package basicplus.frontend;

import basicplus.frontend.Ast.*;

import java.util.ArrayList;
import java.util.List;

public final class Parser {

    private final List<Token> tokens;
    private final List<ParserError> errors = new ArrayList<>();
    private int pos = 0;

    /**
     * True mientras estamos parseando un `module interface X ... end X`.
     * En este modo, las funciones se declaran sólo con su firma — sin cuerpo
     * y sin `end <name>`. Las clases/enums/consts siguen el formato normal.
     */
    private boolean inInterface = false;

    public Parser(List<Token> tokens) {
        if (tokens == null) throw new IllegalArgumentException("tokens no puede ser null");
        this.tokens = tokens;
    }

    public List<ParserError> getErrors() { return errors; }

    // ============================================================
    // PUNTO DE ENTRADA
    // ============================================================
    public ModuleNode parseModule() {
        skipNewlines();
        Token startTok = current();

        // Cabecera opcional `library "..."` antes de `module`.
        String library = "";
        if (match(TokenType.LIBRARY)) {
            Token litTok = current();
            if (!check(TokenType.STRING_LIT)) {
                error("se esperaba un string literal tras 'library'");
            } else {
                Object val = litTok.value;
                library = (val instanceof String) ? (String) val : "";
                advance();
            }
            consumeStmtTerminator("se esperaba salto de línea tras la declaración de library");
            skipNewlines();
        }

        if (!match(TokenType.MODULE)) {
            error("se esperaba 'module' al inicio del fichero");
            return null;
        }

        // Variante: `module interface NAME` — declaración de interfaz (contrato
        // sin código). El módulo se compila como .bpi y nunca como .mod.
        boolean isInterface = false;
        if (match(TokenType.INTERFACE)) isInterface = true;

        String name = consumeIdentifier("nombre del módulo");

        // Variante: `module NAME implements LIB.IFACE` — declara conformidad
        // con una interfaz. El analizador verificará la compatibilidad.
        String implementsName = null;
        if (!isInterface && match(TokenType.IMPLEMENTS)) {
            StringBuilder sb = new StringBuilder();
            sb.append(consumeIdentifier("nombre de la interfaz tras 'implements'"));
            while (match(TokenType.DOT)) {
                sb.append('.').append(consumeIdentifier("componente del path tras '.'"));
            }
            implementsName = sb.toString();
        }

        // Variante: `module interface NAME extends LIB.PARENT` — la interfaz
        // hereda todas las declaraciones de PARENT. Una interfaz hija puede
        // añadir más; el conjunto efectivo es la unión transitiva.
        if (isInterface && match(TokenType.EXTENDS)) {
            StringBuilder sb = new StringBuilder();
            sb.append(consumeIdentifier("nombre de la interfaz padre tras 'extends'"));
            while (match(TokenType.DOT)) {
                sb.append('.').append(consumeIdentifier("componente del path tras '.'"));
            }
            implementsName = sb.toString();  // reutilizamos el slot: la
            // semántica de "extends" para interfaces es exactamente la misma
            // que "implements" para módulos concretos — registra el contrato
            // padre. La distinción es por isInterface.
        }

        // Un `module interface` no admite `implements` (las interfaces no
        // implementan, son implementadas).
        if (isInterface && check(TokenType.IMPLEMENTS)) {
            error("'module interface' no puede llevar 'implements' (usa 'extends')");
            advance();
        }
        // Cabecera-final
        // Track interface mode for body parsing (functions sin cuerpo).
        boolean prevInIface = this.inInterface;
        this.inInterface = isInterface;

        consumeStmtTerminator("se esperaba salto de línea tras la cabecera del módulo");

        List<ImportNode> imports = new ArrayList<>();
        skipNewlines();
        while (check(TokenType.IMPORT)) {
            imports.add(parseImport());
            skipNewlines();
        }

        List<ITopLevelDecl> defs = new ArrayList<>();
        while (!isAtEnd() && !check(TokenType.END)) {
            skipNewlines();
            if (isAtEnd() || check(TokenType.END)) break;
            ITopLevelDecl def = parseDefStmt();
            if (def != null) defs.add(def);
            skipNewlines();
        }

        if (!match(TokenType.END)) {
            error("se esperaba 'end' para cerrar el módulo");
        } else {
            if (check(TokenType.IDENTIFIER)) advance();
            consumeStmtTerminator("se esperaba salto de línea tras 'end'");
        }

        this.inInterface = prevInIface;
        return new ModuleNode(library, name, isInterface, implementsName,
                              imports, defs, startTok.line, startTok.column);
    }

    // ============================================================
    // IMPORTS
    // ============================================================
    private ImportNode parseImport() {
        Token tok = current();
        match(TokenType.IMPORT);
        List<String> path = new ArrayList<>();
        path.add(consumeIdentifier("nombre de librería"));
        while (match(TokenType.DOT)) {
            path.add(consumeIdentifier("componente del path tras '.'"));
        }
        // Variante: `import Iface : Impl`. El path identifica la interfaz
        // (compile-time type-check); el módulo tras ':' es la implementación
        // concreta que la VM cargará y a la que apuntan los CALL_EXT.
        String boundImpl = null;
        if (match(TokenType.COLON)) {
            StringBuilder sb = new StringBuilder();
            sb.append(consumeIdentifier("nombre del módulo concreto tras ':'"));
            while (match(TokenType.DOT)) {
                sb.append('.').append(consumeIdentifier("componente del path del impl"));
            }
            boundImpl = sb.toString();
        }
        String fromPath = null;
        if (match(TokenType.FROM)) {
            Token litTok = current();
            if (!check(TokenType.STRING_LIT)) {
                error("se esperaba un string literal tras 'from'");
            } else {
                Object val = litTok.value;
                fromPath = (val instanceof String) ? (String) val : null;
                advance();
            }
        }
        consumeStmtTerminator("se esperaba salto de línea tras 'import'");
        return new ImportNode(path, fromPath, boundImpl, tok.line, tok.column);
    }

    // ============================================================
    // DEFINICIONES (top-level y miembros de clase)
    // ============================================================
    private ITopLevelDecl parseDefStmt() {
        boolean isPublic = match(TokenType.PUBLIC);
        boolean isFinal  = false;

        if (check(TokenType.FINAL)) {
            isFinal = true;
            advance();
        }

        boolean isSync = false;
        if (check(TokenType.SYNC)) {
            isSync = true;
            advance();
        }

        boolean isIntrinsic = false;
        if (check(TokenType.INTRINSIC)) {
            isIntrinsic = true;
            advance();
        }

        // H3 #162 — modificador `native`. Marca la función como AOT'd.
        // El bytecode .mod se emite IGUAL (sin cambios). El flag solo
        // se propaga al .bpi (futuro) y al emisor AOT (#157).
        boolean isNative = false;
        if (check(TokenType.NATIVE)) {
            isNative = true;
            advance();
        }

        Token tok = current();
        if (isSync && tok.type != TokenType.PROPERTY) {
            error("'sync' sólo se aplica a 'property'");
            // continuamos sin sync para no cascadear errores
            isSync = false;
        }
        if (isIntrinsic && tok.type != TokenType.FUNCTION) {
            error("'intrinsic' sólo se aplica a 'function'");
            isIntrinsic = false;
        }
        if (isNative && tok.type != TokenType.FUNCTION) {
            error("'native' sólo se aplica a 'function'");
            isNative = false;
        }
        if (isIntrinsic && isNative) {
            error("una función no puede ser 'intrinsic' y 'native' a la vez");
            isNative = false;
        }
        switch (tok.type) {
            case CONST:    return parseConstDecl(isPublic);
            case VAR:      return parseVarDecl(isPublic);
            case FUNCTION:
                // Recovery específico para función rota: en lugar del
                // sync genérico que pararía en el primer NEWLINE o
                // keyword de bloque (y dejaría el resto del cuerpo
                // como ruido), saltamos hasta el `end` que cierra
                // esta función. Así un error en cabecera o body
                // no descarrila el parseo del resto del módulo.
                try {
                    return parseFuncDef(isPublic, isFinal, isIntrinsic, isNative);
                } catch (RuntimeException ex) {
                    if (ex.getMessage() != null) error(ex.getMessage());
                    synchronizeToFunctionEnd();
                    return null;
                }
            case PROPERTY: return parsePropertyDef(isPublic, isFinal, isSync);
            case CLASS:    return parseClassDef(isPublic);
            case ENUM:     return parseEnumDef(isPublic);
            default:
                error("se esperaba const/var/function/property/class/enum, encontrado '" + tok.lexeme + "'");
                synchronize();
                return null;
        }
    }

    // ============================================================
    // CONST / VAR
    // ============================================================
    private ConstDecl parseConstDecl(boolean isPublic) {
        Token tok = current();
        match(TokenType.CONST);
        DeclName name = parseDeclName();
        TypeRef type = null;
        if (match(TokenType.COLON)) type = parseType();
        consume(TokenType.ASSIGN, "se esperaba ':=' en la declaración de la constante");
        IExpr value = parseExpr();
        consumeStmtTerminator("se esperaba ';' o salto de línea tras la constante");
        return new ConstDecl(isPublic, name, type, value, tok.line, tok.column);
    }

    private VarDecl parseVarDecl(boolean isPublic) {
        Token tok = current();
        match(TokenType.VAR);
        boolean isOwner = match(TokenType.OWNER);   // `var owner x: T := ...`
        List<DeclName> names = new ArrayList<>();
        names.add(parseDeclName());
        while (match(TokenType.COMMA)) names.add(parseDeclName());
        consume(TokenType.COLON, "se esperaba ':' tras el nombre de la variable");
        TypeRef type = parseType();
        IExpr init = null;
        if (match(TokenType.ASSIGN)) init = parseExpr();
        consumeStmtTerminator("se esperaba ';' o salto de línea tras la declaración");
        return new VarDecl(isPublic, isOwner, names, type, init, tok.line, tok.column);
    }

    /** decl_name ::= name [ '.' name ]   — soporta miembros estáticos.
     *
     *  L4: `get` y `set` son keywords contextuales — sólo reservadas dentro
     *  de un bloque `property { get ... set ... }`. Como nombres de método
     *  o función a nivel de declaración son legítimos (List.get(i),
     *  Map.set(k,v), etc.), así que los aceptamos aquí. */
    private DeclName parseDeclName() {
        Token tok = current();
        String first = consumeIdentifierOrContextualKw("nombre");
        if (match(TokenType.DOT)) {
            String second = consumeIdentifierOrContextualKw("miembro tras 'NombreClase.'");
            return new DeclName(first, second, tok.line, tok.column);
        }
        return new DeclName(null, first, tok.line, tok.column);
    }

    /** L4 — acepta IDENTIFIER o keywords contextuales (GET, SET) en
     *  declaraciones, sin afectar el parsing de `property { get/set }`
     *  (esos casos llaman a match(TokenType.GET/SET) directamente). */
    private String consumeIdentifierOrContextualKw(String what) {
        Token t = current();
        if (t.type == TokenType.IDENTIFIER
                || t.type == TokenType.GET
                || t.type == TokenType.SET) {
            advance();
            return t.lexeme;
        }
        error("se esperaba " + what + ", encontrado '" + t.lexeme + "'");
        return "?";
    }

    // ============================================================
    // TYPE
    // ============================================================
    private TypeRef parseType() {
        Token tok = current();
        TypeRef baseType;
        switch (tok.type) {
            case INTEGER:    baseType = new SimpleTypeRef("integer", tok.line, tok.column); advance(); break;
            case FLOAT:      baseType = new SimpleTypeRef("float",   tok.line, tok.column); advance(); break;
            case STRING:     baseType = new SimpleTypeRef("string",  tok.line, tok.column); advance(); break;
            case BOOLEAN:    baseType = new SimpleTypeRef("boolean", tok.line, tok.column); advance(); break;
            // L10 — tipos enteros estrechos. `short` es alias de `int16`.
            case BYTE:       baseType = new SimpleTypeRef("byte",    tok.line, tok.column); advance(); break;
            case INT8:       baseType = new SimpleTypeRef("int8",    tok.line, tok.column); advance(); break;
            case WORD:       baseType = new SimpleTypeRef("word",    tok.line, tok.column); advance(); break;
            case INT16:      baseType = new SimpleTypeRef("int16",   tok.line, tok.column); advance(); break;
            case SHORT:      baseType = new SimpleTypeRef("int16",   tok.line, tok.column); advance(); break;
            case LONG:       baseType = new SimpleTypeRef("long",    tok.line, tok.column); advance(); break;   // H1.2 (V2)
            case IDENTIFIER: {
                String first = tok.lexeme;
                advance();
                // L2 v2 — acepta tipos cualificados `Mod.Class`.
                if (match(TokenType.DOT)) {
                    String second = consumeIdentifier("nombre tras '" + first + ".'");
                    baseType = new SimpleTypeRef(first + "." + second, tok.line, tok.column);
                } else {
                    baseType = new SimpleTypeRef(first, tok.line, tok.column);
                }
                break;
            }
            default:
                error("se esperaba un tipo, encontrado '" + tok.lexeme + "'");
                advance();
                baseType = new SimpleTypeRef("?", tok.line, tok.column);
                break;
        }
        if (match(TokenType.LBRACKET)) {
            IExpr size = null;
            if (!check(TokenType.RBRACKET)) size = parseExpr();
            consume(TokenType.RBRACKET, "se esperaba ']' en el tipo array");
            return new ArrayTypeRef(baseType, size, tok.line, tok.column);
        }
        return baseType;
    }

    // ============================================================
    // FUNCIONES
    // ============================================================
    private FuncDef parseFuncDef(boolean isPublic, boolean isFinal) {
        return parseFuncDef(isPublic, isFinal, false, false);
    }

    private FuncDef parseFuncDef(boolean isPublic, boolean isFinal, boolean isIntrinsic) {
        return parseFuncDef(isPublic, isFinal, isIntrinsic, false);
    }

    private FuncDef parseFuncDef(boolean isPublic, boolean isFinal,
                                  boolean isIntrinsic, boolean isNative) {
        Token tok = current();
        match(TokenType.FUNCTION);
        DeclName name = parseDeclName();

        consume(TokenType.LPAREN, "se esperaba '(' tras el nombre de la función");
        List<Param> paramList = new ArrayList<>();
        if (!check(TokenType.RPAREN)) {
            paramList.add(parseParam());
            while (match(TokenType.COMMA)) paramList.add(parseParam());
        }
        consume(TokenType.RPAREN, "se esperaba ')' al final de los parámetros");

        TypeRef retType = null;
        if (match(TokenType.COLON)) retType = parseType();
        consumeStmtTerminator("se esperaba salto de línea tras la cabecera de la función");

        // Funciones intrinsic e interfaces sólo declaran signature (sin body / sin `end`).
        // El emisor reemplaza las llamadas a intrínsecos por opcodes inline; sus
        // bodies no se emiten al .mod.
        if (isIntrinsic || inInterface) {
            return new FuncDef(isPublic, isFinal, isIntrinsic, false, name, paramList, retType,
                               new ArrayList<>(), tok.line, tok.column);
        }

        List<IStmt> body = parseBody(TokenType.END);
        consume(TokenType.END, "se esperaba 'end' al final de la función");
        // L4 — `end get` y `end set` también deben aceptarse (get/set como
        // keywords contextuales se permiten como nombre de método).
        if (check(TokenType.IDENTIFIER)
                || check(TokenType.GET)
                || check(TokenType.SET)) {
            parseDeclName();
        }
        consumeStmtTerminator("se esperaba salto de línea tras 'end'");
        return new FuncDef(isPublic, isFinal, false, isNative, name, paramList, retType, body, tok.line, tok.column);
    }

    private Param parseParam() {
        Token tok = current();
        String n = consumeIdentifier("nombre del parámetro");
        consume(TokenType.COLON, "se esperaba ':' tras el nombre del parámetro");
        TypeRef t = parseType();
        return new Param(n, t, tok.line, tok.column);
    }

    // ============================================================
    // PROPIEDADES
    // ============================================================
    private PropertyDef parsePropertyDef(boolean isPublic, boolean isFinal, boolean isSync) {
        Token tok = current();
        match(TokenType.PROPERTY);
        boolean isOwner = match(TokenType.OWNER);   // `property owner foo: T := ...`
        DeclName name = parseDeclName();
        consume(TokenType.COLON, "se esperaba ':' tras el nombre de la propiedad");
        TypeRef type = parseType();
        IExpr init = null;
        if (match(TokenType.ASSIGN)) init = parseExpr();

        // Forma corta: ';' | NEWLINE   -> get/set implícitos
        if (match(TokenType.SEMICOLON))
            return new PropertyDef(isPublic, isFinal, isOwner, isSync, name, type, init, null, null, true, tok.line, tok.column);

        consumeStmtTerminator("se esperaba salto de línea tras la declaración de la propiedad");

        skipNewlines();
        if (!check(TokenType.GET) && !check(TokenType.SET))
            return new PropertyDef(isPublic, isFinal, isOwner, isSync, name, type, init, null, null, true, tok.line, tok.column);

        GetterDef getter = null;
        SetterDef setter = null;
        if (check(TokenType.GET)) { getter = parseGetter(); skipNewlines(); }
        if (check(TokenType.SET)) { setter = parseSetter(); skipNewlines(); }
        consume(TokenType.ENDPROP, "se esperaba 'endprop' al final de la propiedad");
        consumeStmtTerminator("se esperaba salto de línea tras 'endprop'");
        return new PropertyDef(isPublic, isFinal, isOwner, isSync, name, type, init, getter, setter, false, tok.line, tok.column);
    }

    private GetterDef parseGetter() {
        Token tok = current();
        match(TokenType.GET);
        consumeStmtTerminator("se esperaba salto de línea tras 'get'");
        List<IStmt> body = parseBody(TokenType.ENDGET);
        consume(TokenType.ENDGET, "se esperaba 'endget' al final del getter");
        consumeStmtTerminator("se esperaba salto de línea tras 'endget'");
        return new GetterDef(body, tok.line, tok.column);
    }

    private SetterDef parseSetter() {
        Token tok = current();
        match(TokenType.SET);
        consume(TokenType.LPAREN, "se esperaba '(' tras 'set'");
        String param = consumeIdentifier("nombre del parámetro del setter");
        consume(TokenType.RPAREN, "se esperaba ')' tras el parámetro del setter");
        consumeStmtTerminator("se esperaba salto de línea tras 'set(...)'");
        List<IStmt> body = parseBody(TokenType.ENDSET);
        consume(TokenType.ENDSET, "se esperaba 'endset' al final del setter");
        consumeStmtTerminator("se esperaba salto de línea tras 'endset'");
        return new SetterDef(param, body, tok.line, tok.column);
    }

    // ============================================================
    // CLASES
    // ============================================================
    private ClassDef parseClassDef(boolean isPublic) {
        Token tok = current();
        match(TokenType.CLASS);
        String name = consumeIdentifier("nombre de la clase");
        String baseClass = null;
        if (match(TokenType.EXTENDS)) {
            // L2 v2 — acepta `extends Foo` (local) o `extends Mod.Foo`
            // (cross-module: alias del namespace importado + clase).
            baseClass = consumeIdentifier("clase base");
            if (match(TokenType.DOT)) {
                String memberName = consumeIdentifier("clase tras '" + baseClass + ".'");
                baseClass = baseClass + "." + memberName;
            }
        }
        consumeStmtTerminator("se esperaba salto de línea tras la cabecera de la clase");

        List<ITopLevelDecl> members = new ArrayList<>();
        while (!isAtEnd() && !check(TokenType.END)) {
            skipNewlines();
            if (isAtEnd() || check(TokenType.END)) break;
            ITopLevelDecl m = parseDefStmt();
            if (m != null) members.add(m);
            skipNewlines();
        }
        consume(TokenType.END, "se esperaba 'end' al final de la clase");
        if (check(TokenType.IDENTIFIER)) advance();
        consumeStmtTerminator("se esperaba salto de línea tras 'end'");
        return new ClassDef(isPublic, name, baseClass, members, tok.line, tok.column);
    }

    // ============================================================
    // ENUMS
    // ============================================================
    private EnumDef parseEnumDef(boolean isPublic) {
        Token tok = current();
        match(TokenType.ENUM);
        String name = consumeIdentifier("nombre del enum");
        consumeStmtTerminator("se esperaba salto de línea tras el nombre del enum");
        List<EnumValue> values = new ArrayList<>();
        while (!isAtEnd() && !check(TokenType.END)) {
            skipNewlines();
            if (isAtEnd() || check(TokenType.END)) break;
            values.add(parseEnumValue());
        }
        consume(TokenType.END, "se esperaba 'end' al final del enum");
        if (check(TokenType.IDENTIFIER)) advance();
        consumeStmtTerminator("se esperaba salto de línea tras 'end'");
        return new EnumDef(isPublic, name, values, tok.line, tok.column);
    }

    private EnumValue parseEnumValue() {
        Token tok = current();
        String n = consumeIdentifier("nombre del miembro del enum");
        Long explicitVal = null;
        if (match(TokenType.ASSIGN)) {
            Token lit = current();
            if (lit.type == TokenType.INTEGER_LIT && lit.value instanceof Long) {
                explicitVal = (Long) lit.value;
                advance();
            } else {
                error("se esperaba un literal entero tras ':='");
            }
        }
        consumeStmtTerminator("se esperaba salto de línea tras el miembro del enum");
        return new EnumValue(n, explicitVal, tok.line, tok.column);
    }

    // ============================================================
    // CUERPO Y SENTENCIAS
    // ============================================================
    private List<IStmt> parseBody(TokenType... stopAt) {
        List<IStmt> stmts = new ArrayList<>();
        while (!isAtEnd()) {
            skipNewlines();
            if (isAtEnd()) break;
            if (containsType(stopAt, current().type)) break;
            if (isBlockTerminator(current().type)) break;
            IStmt s = parseStmt();
            if (s != null) stmts.add(s);
        }
        return stmts;
    }

    private static boolean containsType(TokenType[] arr, TokenType t) {
        for (TokenType x : arr) if (x == t) return true;
        return false;
    }

    private static boolean isBlockTerminator(TokenType t) {
        switch (t) {
            case END: case ENDIF: case ENDWH: case ENDSW:
            case ENDTRY: case ENDPROP: case ENDGET: case ENDSET:
            case ELSEIF: case ELSE:
            case CASE: case DEFAULT:
            case CATCH: case FINALLY:
            case NEXT: case LOOP:
                return true;
            default:
                return false;
        }
    }

    private IStmt parseStmt() {
        TokenType t = current().type;
        switch (t) {
            case VAR:      return parseVarDecl(false);
            case CONST:    return parseConstDecl(false);
            case IF:       return parseIfStmt();
            case SWITCH:   return parseSwitchStmt();
            case WHILE:    return parseWhileStmt();
            case DO:       return parseDoLoopStmt();
            case FOR:      return parseForStmt();
            case TRY:      return parseTryStmt();
            case RETURN:   return parseReturnStmt();
            case THROW:    return parseThrowStmt();
            case PRINT:    return parsePrintStmt();
            case PARALLEL: return parseParallelStmt();
            case BREAK: {
                Token tok = current(); advance();
                consumeStmtTerminator("se esperaba salto de línea tras 'break'");
                return new BreakStmt(tok.line, tok.column);
            }
            case CONTINUE: {
                Token tok = current(); advance();
                consumeStmtTerminator("se esperaba salto de línea tras 'continue'");
                return new ContinueStmt(tok.line, tok.column);
            }
            default:
                return parseAssignOrCallStmt();
        }
    }

    private IStmt parseAssignOrCallStmt() {
        Token startTok = current();
        IExpr lhs;
        try { lhs = parsePrimary(); }
        catch (RuntimeException ex) { synchronize(); return null; }

        if (check(TokenType.ASSIGN) || check(TokenType.PLUS_ASSIGN) || check(TokenType.MINUS_ASSIGN)) {
            AssignOpKind op;
            switch (current().type) {
                case PLUS_ASSIGN:  op = AssignOpKind.PLUS_ASSIGN; break;
                case MINUS_ASSIGN: op = AssignOpKind.MINUS_ASSIGN; break;
                default:           op = AssignOpKind.ASSIGN; break;
            }
            advance();
            IExpr rhs = parseExpr();
            consumeStmtTerminator("se esperaba ';' o salto de línea tras la asignación");
            return new AssignStmt(lhs, op, rhs, startTok.line, startTok.column);
        }

        if (lhs instanceof CallExpr || lhs instanceof SuperCallExpr) {
            consumeStmtTerminator("se esperaba salto de línea tras la llamada");
            return new ExprStmt(lhs, startTok.line, startTok.column);
        }

        error("se esperaba una asignación o una llamada como sentencia");
        synchronize();
        return null;
    }

    // ---------- IF ----------
    private IfStmt parseIfStmt() {
        Token tok = current();
        match(TokenType.IF);
        IExpr cond = parseExpr();
        consume(TokenType.THEN, "se esperaba 'then' tras la condición del 'if'");

        // Forma de una línea
        if (!check(TokenType.NEWLINE)) {
            IStmt stmt = parseStmt();
            List<IStmt> body = new ArrayList<>();
            if (stmt != null) body.add(stmt);
            IfClause thenClause = new IfClause(cond, body, tok.line, tok.column);
            return new IfStmt(thenClause, new ArrayList<IfClause>(), null, true, tok.line, tok.column);
        }

        consumeStmtTerminator("se esperaba salto de línea tras 'then'");
        List<IStmt> thenBody = parseBody(TokenType.ELSEIF, TokenType.ELSE, TokenType.ENDIF);
        IfClause thenC = new IfClause(cond, thenBody, tok.line, tok.column);

        List<IfClause> elseifs = new ArrayList<>();
        while (check(TokenType.ELSEIF)) {
            Token elseifTok = current();
            advance();
            IExpr ec = parseExpr();
            consume(TokenType.THEN, "se esperaba 'then' tras la condición del 'elseif'");
            consumeStmtTerminator("se esperaba salto de línea tras 'then'");
            List<IStmt> eb = parseBody(TokenType.ELSEIF, TokenType.ELSE, TokenType.ENDIF);
            elseifs.add(new IfClause(ec, eb, elseifTok.line, elseifTok.column));
        }

        List<IStmt> elseBody = null;
        if (match(TokenType.ELSE)) {
            consumeStmtTerminator("se esperaba salto de línea tras 'else'");
            elseBody = parseBody(TokenType.ENDIF);
        }
        consume(TokenType.ENDIF, "se esperaba 'endif'");
        consumeStmtTerminator("se esperaba salto de línea tras 'endif'");
        return new IfStmt(thenC, elseifs, elseBody, false, tok.line, tok.column);
    }

    // ---------- SWITCH ----------
    private SwitchStmt parseSwitchStmt() {
        Token tok = current();
        match(TokenType.SWITCH);
        IExpr subject = parseExpr();
        consumeStmtTerminator("se esperaba salto de línea tras la expresión del 'switch'");
        skipNewlines();

        List<CaseClause> cases = new ArrayList<>();
        List<IStmt> def = null;
        while (check(TokenType.CASE)) {
            Token caseTok = current();
            advance();
            List<IExpr> values = new ArrayList<>();
            values.add(parseExpr());
            while (match(TokenType.COMMA)) values.add(parseExpr());
            consumeStmtTerminator("se esperaba salto de línea tras la lista del 'case'");
            List<IStmt> body = parseBody(TokenType.CASE, TokenType.DEFAULT, TokenType.ENDSW);
            cases.add(new CaseClause(values, body, caseTok.line, caseTok.column));
            skipNewlines();
        }
        if (match(TokenType.DEFAULT)) {
            consumeStmtTerminator("se esperaba salto de línea tras 'default'");
            def = parseBody(TokenType.ENDSW);
        }
        consume(TokenType.ENDSW, "se esperaba 'endsw'");
        consumeStmtTerminator("se esperaba salto de línea tras 'endsw'");
        return new SwitchStmt(subject, cases, def, tok.line, tok.column);
    }

    // ---------- PARALLEL (multitarea sintáctica) ----------
    // parallel
    //     case t1 := Thread(STACKSIZE)
    //        ... cuerpo del run()
    //     case t2 := Thread(STACKSIZE)
    //        ...
    //     default
    //        ... bloque que corre en el thread llamador entre start() y join()
    // endpar
    private ParallelStmt parseParallelStmt() {
        Token tok = current();
        match(TokenType.PARALLEL);
        consumeStmtTerminator("se esperaba salto de línea tras 'parallel'");
        skipNewlines();

        List<ParallelBranch> branches = new ArrayList<>();
        List<IStmt> def = null;
        while (check(TokenType.CASE)) {
            Token caseTok = current();
            advance();
            if (!check(TokenType.IDENTIFIER)) {
                error("se esperaba nombre de variable tras 'case' en parallel");
                synchronize();
                continue;
            }
            String varName = current().lexeme;
            advance();
            consume(TokenType.ASSIGN, "se esperaba ':=' tras nombre de variable en 'case'");
            // Constructor: literalmente "Thread(<expr>)". Restringimos a Thread
            // directamente; si el usuario quiere su propia subclase con state,
            // que use el patrón clásico (class X extends Thread).
            if (!check(TokenType.IDENTIFIER) || !"Thread".equals(current().lexeme)) {
                error("se esperaba 'Thread' tras ':=' en case de parallel");
                synchronize();
                continue;
            }
            advance();
            consume(TokenType.LPAREN, "se esperaba '(' tras 'Thread'");
            IExpr stackSize = parseExpr();
            consume(TokenType.RPAREN, "se esperaba ')' tras el tamaño de stack");
            consumeStmtTerminator("se esperaba salto de línea tras 'case Thread(...)'");
            List<IStmt> body = parseBody(TokenType.CASE, TokenType.DEFAULT, TokenType.ENDPAR);
            branches.add(new ParallelBranch(varName, stackSize, body, caseTok.line, caseTok.column));
            skipNewlines();
        }
        if (match(TokenType.DEFAULT)) {
            consumeStmtTerminator("se esperaba salto de línea tras 'default'");
            def = parseBody(TokenType.ENDPAR);
        }
        consume(TokenType.ENDPAR, "se esperaba 'endpar'");
        consumeStmtTerminator("se esperaba salto de línea tras 'endpar'");
        return new ParallelStmt(branches, def, tok.line, tok.column);
    }

    // ---------- WHILE ----------
    private WhileStmt parseWhileStmt() {
        Token tok = current();
        match(TokenType.WHILE);
        IExpr cond = parseExpr();
        consume(TokenType.DO, "se esperaba 'do' tras la condición del 'while'");

        if (!check(TokenType.NEWLINE)) {
            IStmt stmt = parseStmt();
            List<IStmt> body = new ArrayList<>();
            if (stmt != null) body.add(stmt);
            return new WhileStmt(cond, body, true, tok.line, tok.column);
        }

        consumeStmtTerminator("se esperaba salto de línea tras 'do'");
        List<IStmt> body = parseBody(TokenType.ENDWH);
        consume(TokenType.ENDWH, "se esperaba 'endwh'");
        consumeStmtTerminator("se esperaba salto de línea tras 'endwh'");
        return new WhileStmt(cond, body, false, tok.line, tok.column);
    }

    // ---------- DO ... LOOP [cond] ----------
    private DoLoopStmt parseDoLoopStmt() {
        Token tok = current();
        match(TokenType.DO);
        consumeStmtTerminator("se esperaba salto de línea tras 'do'");
        List<IStmt> body = parseBody(TokenType.LOOP);
        consume(TokenType.LOOP, "se esperaba 'loop'");
        IExpr cond = null;
        if (!check(TokenType.NEWLINE) && !check(TokenType.SEMICOLON) && !isAtEnd())
            cond = parseExpr();
        consumeStmtTerminator("se esperaba salto de línea tras 'loop'");
        return new DoLoopStmt(body, cond, tok.line, tok.column);
    }

    // ---------- FOR ----------
    private ForStmt parseForStmt() {
        Token tok = current();
        match(TokenType.FOR);
        String iter = consumeIdentifier("nombre del iterador del 'for'");

        ForRange range;
        if (match(TokenType.ASSIGN)) {
            IExpr from = parseExpr();
            consume(TokenType.TO, "se esperaba 'to' en el 'for' numérico");
            IExpr to = parseExpr();
            IExpr step = null;
            if (match(TokenType.STEP)) step = parseExpr();
            range = new ForNumericRange(from, to, step, tok.line, tok.column);
        } else if (match(TokenType.IN)) {
            IExpr iterable = parseExpr();
            range = new ForInRange(iterable, tok.line, tok.column);
        } else {
            error("se esperaba ':=' (numérico) o 'in' (foreach) en el 'for'");
            range = new ForInRange(new NullLitExpr(tok.line, tok.column), tok.line, tok.column);
        }
        consume(TokenType.DO, "se esperaba 'do' tras la cabecera del 'for'");

        if (!check(TokenType.NEWLINE)) {
            IStmt stmt = parseStmt();
            List<IStmt> body = new ArrayList<>();
            if (stmt != null) body.add(stmt);
            return new ForStmt(iter, range, body, true, tok.line, tok.column);
        }

        consumeStmtTerminator("se esperaba salto de línea tras 'do'");
        List<IStmt> body = parseBody(TokenType.NEXT);
        consume(TokenType.NEXT, "se esperaba 'next' al final del 'for'");
        if (check(TokenType.IDENTIFIER)) advance();
        consumeStmtTerminator("se esperaba salto de línea tras 'next'");
        return new ForStmt(iter, range, body, false, tok.line, tok.column);
    }

    // ---------- TRY ----------
    private TryStmt parseTryStmt() {
        Token tok = current();
        match(TokenType.TRY);
        consumeStmtTerminator("se esperaba salto de línea tras 'try'");
        List<IStmt> body = parseBody(TokenType.CATCH, TokenType.FINALLY, TokenType.ENDTRY);

        List<CatchClause> catches = new ArrayList<>();
        while (check(TokenType.CATCH)) {
            Token cTok = current();
            advance();
            String varName = null;
            String excType = null;
            if (check(TokenType.IDENTIFIER)) {
                varName = current().lexeme;
                advance();
                if (match(TokenType.COLON))
                    excType = consumeIdentifier("tipo de la excepción tras ':'");
            }
            consumeStmtTerminator("se esperaba salto de línea tras la cabecera del 'catch'");
            List<IStmt> cb = parseBody(TokenType.CATCH, TokenType.FINALLY, TokenType.ENDTRY);
            catches.add(new CatchClause(varName, excType, cb, cTok.line, cTok.column));
        }
        List<IStmt> fin = null;
        if (match(TokenType.FINALLY)) {
            consumeStmtTerminator("se esperaba salto de línea tras 'finally'");
            fin = parseBody(TokenType.ENDTRY);
        }
        consume(TokenType.ENDTRY, "se esperaba 'endtry'");
        consumeStmtTerminator("se esperaba salto de línea tras 'endtry'");
        return new TryStmt(body, catches, fin, tok.line, tok.column);
    }

    // ---------- RETURN / THROW ----------
    private ReturnStmt parseReturnStmt() {
        Token tok = current();
        match(TokenType.RETURN);
        IExpr val = null;
        if (!check(TokenType.NEWLINE) && !check(TokenType.SEMICOLON) && !isAtEnd())
            val = parseExpr();
        consumeStmtTerminator("se esperaba salto de línea tras 'return'");
        return new ReturnStmt(val, tok.line, tok.column);
    }

    private ThrowStmt parseThrowStmt() {
        Token tok = current();
        match(TokenType.THROW);
        IExpr v = parseExpr();
        consumeStmtTerminator("se esperaba salto de línea tras 'throw'");
        return new ThrowStmt(v, tok.line, tok.column);
    }

    // ---------- PRINT ----------
    private PrintStmt parsePrintStmt() {
        Token tok = current();
        match(TokenType.PRINT);
        List<PrintItem> items = new ArrayList<>();

        if (!check(TokenType.NEWLINE) && !isAtEnd() && !check(TokenType.SEMICOLON)) {
            items.add(new PrintItem(PrintSep.NONE, parseExpr()));
            while (check(TokenType.COMMA) || check(TokenType.SEMICOLON)) {
                PrintSep sep = current().type == TokenType.COMMA ? PrintSep.COMMA : PrintSep.SEMICOLON;
                advance();
                IExpr e = null;
                if (!check(TokenType.COMMA) && !check(TokenType.SEMICOLON)
                        && !check(TokenType.NEWLINE) && !isAtEnd())
                    e = parseExpr();
                items.add(new PrintItem(sep, e));
            }
        }
        consumeStmtTerminator("se esperaba salto de línea tras 'print'");
        return new PrintStmt(items, tok.line, tok.column);
    }

    // ============================================================
    // EXPRESIONES (precedencia)
    // ============================================================
    private IExpr parseExpr() { return parseOr(); }

    private IExpr parseOr() {
        IExpr left = parseAnd();
        while (check(TokenType.OR)) {
            Token op = current(); advance();
            IExpr right = parseAnd();
            left = new BinaryExpr("or", left, right, op.line, op.column);
        }
        return left;
    }

    private IExpr parseAnd() {
        IExpr left = parseNot();
        while (check(TokenType.AND)) {
            Token op = current(); advance();
            IExpr right = parseNot();
            left = new BinaryExpr("and", left, right, op.line, op.column);
        }
        return left;
    }

    private IExpr parseNot() {
        if (check(TokenType.NOT)) {
            Token op = current(); advance();
            IExpr inner = parseCmp();
            return new UnaryExpr("not", inner, op.line, op.column);
        }
        return parseCmp();
    }

    private IExpr parseCmp() {
        IExpr left = parseAdd();
        if (check(TokenType.EQ) || check(TokenType.NEQ)
                || check(TokenType.LT) || check(TokenType.GT)
                || check(TokenType.LE) || check(TokenType.GE)) {
            Token op = current(); advance();
            IExpr right = parseAdd();
            return new BinaryExpr(op.lexeme, left, right, op.line, op.column);
        }
        if (check(TokenType.INSTANCEOF)) {
            Token op = current(); advance();
            // Tras instanceof esperamos un identificador (nombre de clase).
            if (!check(TokenType.IDENTIFIER)) {
                error("se esperaba un nombre de clase tras 'instanceof'");
                return left;
            }
            Token typeTok = current(); advance();
            return new InstanceOfExpr(left, typeTok.lexeme, op.line, op.column);
        }
        return left;
    }

    private IExpr parseAdd() {
        IExpr left = parseMul();
        while (check(TokenType.PLUS) || check(TokenType.MINUS)
                || check(TokenType.BAR) || check(TokenType.XOR)) {
            Token op = current(); advance();
            IExpr right = parseMul();
            left = new BinaryExpr(op.lexeme, left, right, op.line, op.column);
        }
        return left;
    }

    private IExpr parseMul() {
        IExpr left = parseUnary();
        while (check(TokenType.STAR) || check(TokenType.SLASH)
                || check(TokenType.MOD) || check(TokenType.AMP)
                || check(TokenType.SHL) || check(TokenType.SHR)) {
            Token op = current(); advance();
            IExpr right = parseUnary();
            left = new BinaryExpr(op.lexeme, left, right, op.line, op.column);
        }
        return left;
    }

    private IExpr parseUnary() {
        if (check(TokenType.MINUS)) {
            Token op = current(); advance();
            IExpr inner = parsePrimary();
            return new UnaryExpr("-", inner, op.line, op.column);
        }
        return parsePrimary();
    }

    /** primary ::= atom { '.' (call | name) | '[' expr ']' } */
    private IExpr parsePrimary() {
        IExpr node = parseAtom();
        while (true) {
            if (check(TokenType.DOT)) {
                Token op = current(); advance();
                String member = consumeMemberName();
                node = new MemberAccessExpr(node, member, op.line, op.column);
                if (check(TokenType.LPAREN)) {
                    Token lp = current(); advance();
                    List<IExpr> args = parseArgList();
                    consume(TokenType.RPAREN, "se esperaba ')'");
                    node = new CallExpr(node, args, lp.line, lp.column);
                }
            } else if (check(TokenType.LBRACKET)) {
                Token op = current(); advance();
                IExpr idx = parseExpr();
                consume(TokenType.RBRACKET, "se esperaba ']'");
                node = new IndexExpr(node, idx, op.line, op.column);
            } else {
                break;
            }
        }
        return node;
    }

    private List<IExpr> parseArgList() {
        List<IExpr> args = new ArrayList<>();
        if (!check(TokenType.RPAREN)) {
            args.add(parseExpr());
            while (match(TokenType.COMMA)) args.add(parseExpr());
        }
        return args;
    }

    private IExpr parseAtom() {
        Token tok = current();
        switch (tok.type) {
            case INTEGER_LIT: {
                advance();
                long v = (tok.value instanceof Long) ? (Long) tok.value : 0L;
                return new IntLitExpr(v, tok.line, tok.column);
            }
            case FLOAT_LIT: {
                advance();
                double v = (tok.value instanceof Double) ? (Double) tok.value : 0.0;
                return new FloatLitExpr(v, tok.line, tok.column);
            }
            case LONG_LIT: {
                advance();
                long v = (tok.value instanceof Long) ? (Long) tok.value : 0L;
                return new LongLitExpr(v, tok.line, tok.column);
            }
            case STRING_LIT: {
                advance();
                String v = (tok.value instanceof String) ? (String) tok.value : "";
                return new StringLitExpr(v, tok.line, tok.column);
            }
            case TRUE:  advance(); return new BoolLitExpr(true,  tok.line, tok.column);
            case FALSE: advance(); return new BoolLitExpr(false, tok.line, tok.column);
            case NULL:  advance(); return new NullLitExpr(tok.line, tok.column);
            case THIS:  advance(); return new ThisExpr(tok.line, tok.column);
            case FIELD: advance(); return new FieldExpr(tok.line, tok.column);
            case SUPER: {
                advance();
                if (check(TokenType.LPAREN)) {
                    advance();
                    List<IExpr> args = parseArgList();
                    consume(TokenType.RPAREN, "se esperaba ')' tras 'super('");
                    return new SuperCallExpr(args, tok.line, tok.column);
                }
                return new SuperExpr(tok.line, tok.column);
            }
            case LPAREN: {
                advance();
                IExpr inner = parseExpr();
                consume(TokenType.RPAREN, "se esperaba ')'");
                return new ParenExpr(inner, tok.line, tok.column);
            }
            case LBRACKET: return parseArrayLiteral();
            case IDENTIFIER: {
                advance();
                if (check(TokenType.LPAREN)) {
                    Token lp = current(); advance();
                    List<IExpr> args = parseArgList();
                    consume(TokenType.RPAREN, "se esperaba ')'");
                    return new CallExpr(new IdentifierExpr(tok.lexeme, tok.line, tok.column),
                            args, lp.line, lp.column);
                }
                return new IdentifierExpr(tok.lexeme, tok.line, tok.column);
            }
            // L10 — tipos estrechos como casts en posición de expresión:
            //   byte(x), int8(x), word(x), int16(x), short(x).
            // Se parsean como CallExpr con el nombre canónico del tipo;
            // SemanticAnalyzer + MivmEmitter saben que son casts a tipo
            // estrecho y emiten I32_TO_{U8,I8,U16,I16} en su lugar.
            // `short(x)` se normaliza a `int16(x)` (mismo Kind).
            case BYTE:
            case INT8:
            case WORD:
            case INT16:
            case SHORT: {
                String castName;
                switch (tok.type) {
                    case BYTE:  castName = "byte";  break;
                    case INT8:  castName = "int8";  break;
                    case WORD:  castName = "word";  break;
                    case SHORT: castName = "int16"; break;
                    default:    castName = "int16"; break;   // INT16
                }
                advance();
                if (!check(TokenType.LPAREN)) {
                    error("se esperaba '(' tras tipo '" + castName + "' en expresión (cast)");
                    return new NullLitExpr(tok.line, tok.column);
                }
                Token lp = current(); advance();
                List<IExpr> args = parseArgList();
                consume(TokenType.RPAREN, "se esperaba ')'");
                return new CallExpr(new IdentifierExpr(castName, tok.line, tok.column),
                        args, lp.line, lp.column);
            }
            default:
                error("se esperaba una expresión, encontrado '" + tok.lexeme + "'");
                advance();
                return new NullLitExpr(tok.line, tok.column);
        }
    }

    private IExpr parseArrayLiteral() {
        Token tok = current();
        match(TokenType.LBRACKET);
        List<IExpr> elems = new ArrayList<>();
        if (!check(TokenType.RBRACKET)) {
            elems.add(parseExpr());
            while (match(TokenType.COMMA)) elems.add(parseExpr());
        }
        consume(TokenType.RBRACKET, "se esperaba ']' al final del array literal");
        return new ArrayLitExpr(elems, tok.line, tok.column);
    }

    // ============================================================
    // HELPERS DE CURSOR
    // ============================================================
    private Token current() { return tokens.get(pos); }

    private boolean isAtEnd() { return current().type == TokenType.EOF; }

    private boolean check(TokenType t) { return current().type == t; }

    private boolean match(TokenType t) {
        if (!check(t)) return false;
        advance();
        return true;
    }

    private Token advance() {
        Token t = current();
        if (!isAtEnd()) pos++;
        return t;
    }

    private Token consume(TokenType t, String msg) {
        if (check(t)) return advance();
        error(msg);
        return current();
    }

    private String consumeIdentifier(String what) {
        if (check(TokenType.IDENTIFIER)) {
            String name = current().lexeme;
            advance();
            return name;
        }
        error("se esperaba " + what + ", encontrado '" + current().lexeme + "'");
        return "?";
    }

    /**
     * Acepta como nombre de miembro un IDENTIFIER o algunos keywords que en
     * BP son contextuales (`get` y `set` se usan en `property`, pero como
     * nombres de método tras `.` son perfectamente legítimos: list.get(i),
     * map.set(k,v), etc.). Permitirlos aquí evita renombrar APIs útiles.
     */
    private String consumeMemberName() {
        Token t = current();
        if (t.type == TokenType.IDENTIFIER || t.type == TokenType.GET || t.type == TokenType.SET) {
            advance();
            return t.lexeme;
        }
        error("se esperaba nombre tras '.', encontrado '" + t.lexeme + "'");
        return "?";
    }

    /** Acepta NEWLINE, ';' o EOF como terminador de sentencia. */
    private void consumeStmtTerminator(String msg) {
        if (match(TokenType.NEWLINE)) return;
        if (match(TokenType.SEMICOLON)) return;
        if (isAtEnd()) return;
        error(msg);
    }

    private void skipNewlines() {
        while (check(TokenType.NEWLINE)) advance();
    }

    private void error(String message) {
        Token t = current();
        errors.add(new ParserError(message, t.line, t.column));
    }

    /**
     * Tokens de sincronización para recuperación tras error de expresión
     * o sentencia. Cuando el parser se topa con tokens que no encajan
     * sintácticamente, salta hasta el próximo punto reconocible para
     * intentar continuar parseando el resto del fichero sin cascadear
     * errores en el resto del fichero.
     *
     * La lista es deliberadamente amplia para que el parser nunca se
     * "trague" más de una sentencia ante un error puntual:
     *
     *   - Terminadores de sentencia (`;`, NEWLINE).
     *   - Keywords que abren o cierran bloques (`if`, `while`, `for`,
     *     `next`, `return`, `break`, `continue`, `endif`, `endwh`, `end`).
     *   - Keywords intermedias de cabeceras (`then`, `do`, `to`).
     *
     * El criterio común: cualquiera de estos tokens marca un sitio donde
     * un parser razonable puede volver a engancharse y seguir leyendo.
     */
    private static final java.util.Set<TokenType> EXPR_SYNC_TOKENS =
            java.util.EnumSet.of(
                    TokenType.SEMICOLON,    // ;
                    TokenType.NEWLINE,      // CR
                    TokenType.THEN,         // then
                    TokenType.DO,           // do
                    TokenType.TO,           // to
                    TokenType.IF,           // if
                    TokenType.WHILE,        // while
                    TokenType.FOR,          // for
                    TokenType.NEXT,         // next
                    TokenType.RETURN,       // return
                    TokenType.BREAK,        // break
                    TokenType.CONTINUE,     // continue
                    TokenType.ENDIF,        // endif
                    TokenType.ENDWH,        // endwh
                    TokenType.END           // end
            );

    /**
     * Recuperación tras error de expresión/sentencia: avanza tokens
     * hasta encontrar uno de los {@link #EXPR_SYNC_TOKENS}, SIN
     * consumirlo (excepto NEWLINEs, que se saltan para arrancar limpio
     * la sentencia siguiente).
     *
     * Diseño: dejamos el sync token en el stream para que el parser de
     * más arriba lo reconozca como cierre de la sentencia que estaba
     * intentando parsear (p. ej. `then` cerrando una condición rota de
     * un if, `endwh` cerrando un body de while), o como apertura de la
     * sentencia siguiente (p. ej. `return`, `if`).
     */
    private void synchronize() {
        while (!isAtEnd() && !EXPR_SYNC_TOKENS.contains(current().type)) {
            advance();
        }
        // Si paramos en NEWLINE, lo consumimos: la sentencia ya terminó.
        // Los demás sync tokens los dejamos en el stream.
        skipNewlines();
    }

    /**
     * Recuperación para una función rota: avanza tokens hasta encontrar
     * un `end`, lo consume (junto con el nombre opcional que le siga
     * — estilo `end miFunc`) y deja el parser listo para la siguiente
     * declaración top-level. Esto es más agresivo que {@link #synchronize}
     * y se usa cuando un error en la cabecera o cuerpo de la función
     * dejaría todo descolocado.
     *
     * No intenta balancear `if`/`endif`, `while`/`endwh`, etc. dentro
     * del cuerpo: salta al primer `end` que vea. En código BP normal el
     * primer `end` que aparece tras una cabecera de función rota es el
     * cierre de la propia función (las construcciones internas usan sus
     * propios cierres específicos: `endif`, `endwh`, `next`, `endsw`,
     * `endtry`, `endprop`). Esa heurística es suficiente para casi todos
     * los casos prácticos y evita el coste de mantener un parser de
     * balance ad-hoc en modo pánico.
     */
    private void synchronizeToFunctionEnd() {
        while (!isAtEnd() && !check(TokenType.END)) {
            advance();
        }
        if (check(TokenType.END)) {
            advance();
            // Aceptamos opcionalmente el nombre tras 'end' (estilo
            // `end miFunc`) o `end get` / `end set` para que el siguiente
            // ciclo del parser no lo malinterprete como una declaración.
            if (check(TokenType.IDENTIFIER)
                    || check(TokenType.GET)
                    || check(TokenType.SET)) {
                advance();
            }
        }
        skipNewlines();
    }
}
