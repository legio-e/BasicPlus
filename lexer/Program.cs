// ============================================================
// Program.cs
// Demo del frontend BASICPLUS:
//   lex → parse → analyze (semántico) → muestra tokens / AST / diagnósticos.
//
// Uso:
//   dotnet run                           (procesa samples/hello.bp)
//   dotnet run -- ruta/al/archivo.bp
//   dotnet run -- archivo.bp --tokens    (muestra solo los tokens)
//   dotnet run -- archivo.bp --ast       (muestra solo el AST)
//   dotnet run -- archivo.bp --quiet     (solo diagnósticos)
// ============================================================

using BasicPlus.Lexer;

string? path = null;
string? compileOut = null;
bool showTokens = true;
bool showAst    = true;

for (int i = 0; i < args.Length; i++)
{
    var a = args[i];
    switch (a)
    {
        case "--tokens": showAst = false; break;
        case "--ast":    showTokens = false; break;
        case "--quiet":  showAst = false; showTokens = false; break;
        case "--compile":
            showAst = false; showTokens = false;
            if (i + 1 < args.Length) { compileOut = args[++i]; }
            else { Console.Error.WriteLine("--compile requiere ruta de salida"); return 1; }
            break;
        default:
            if (path is null) path = a;
            else Console.Error.WriteLine($"argumento extra ignorado: {a}");
            break;
    }
}

string sourcePath = path ?? Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "samples", "hello.bp");
if (!File.Exists(sourcePath))
{
    Console.Error.WriteLine($"No se encuentra el archivo: {sourcePath}");
    return 1;
}

string source = File.ReadAllText(sourcePath);
Console.WriteLine($"=== Procesando: {Path.GetFullPath(sourcePath)} ({source.Length} chars) ===");
Console.WriteLine();

// 1) LEXER
var lexer = new Lexer(source);
var tokens = lexer.Tokenize();

if (showTokens)
{
    Console.WriteLine($"-- Tokens ({tokens.Count}) --");
    foreach (var t in tokens) Console.WriteLine(t);
    Console.WriteLine();
}
if (lexer.Errors.Count > 0)
{
    Console.WriteLine($"-- Errores léxicos ({lexer.Errors.Count}) --");
    foreach (var e in lexer.Errors) Console.WriteLine(e);
    Console.WriteLine();
}

// 2) PARSER
var parser = new Parser(tokens);
var module = parser.ParseModule();

if (showAst && module is not null)
{
    Console.WriteLine("-- AST --");
    Console.Write(AstPrinter.Print(module));
    Console.WriteLine();
}
if (parser.Errors.Count > 0)
{
    Console.WriteLine($"-- Errores sintácticos ({parser.Errors.Count}) --");
    foreach (var e in parser.Errors) Console.WriteLine(e);
    Console.WriteLine();
}

// 3) ANÁLISIS SEMÁNTICO
SemanticInfo? info = null;
if (module is not null && parser.Errors.Count == 0)
{
    var analyzer = new SemanticAnalyzer();
    info = analyzer.Analyze(module);

    int errs = info.Diagnostics.Count(d => d.Kind == DiagnosticKind.Error);
    int warns = info.Diagnostics.Count(d => d.Kind == DiagnosticKind.Warning);

    if (info.Diagnostics.Count == 0)
        Console.WriteLine("-- Sin diagnósticos semánticos --");
    else
    {
        Console.WriteLine($"-- Diagnósticos semánticos ({errs} errores, {warns} avisos) --");
        foreach (var d in info.Diagnostics) Console.WriteLine(d);
    }
    Console.WriteLine();

    // Resumen rápido del módulo procesado
    if (info.Module is { } m)
    {
        int classes = m.Members.Symbols.OfType<ClassSymbol>().Count();
        int enums   = m.Members.Symbols.OfType<EnumSymbol>().Count();
        int funcs   = m.Members.Symbols.OfType<FunctionSymbol>().Count();
        int vars    = m.Members.Symbols.OfType<VarSymbol>().Count();
        int consts  = m.Members.Symbols.OfType<ConstSymbol>().Count();
        int props   = m.Members.Symbols.OfType<PropertySymbol>().Count();
        Console.WriteLine($"-- Resumen módulo '{m.Name}' --");
        Console.WriteLine($"  classes={classes}  enums={enums}  funcs={funcs}  vars={vars}  consts={consts