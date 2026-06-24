// ============================================================
// AotMain.java
// CLI standalone para emisor AOT (H3 #157 fase 1).
//
//   java basicplus.frontend.AotMain <file.bp> [<outDir>] [--mdn]
//
// Toma un .bp, lo lexa + parsea, busca funciones `function native`,
// y emite aot_<Module>.c en outDir (defecto: cwd).
//
// Flags:
//   --mdn   No emite la función aot_<Mod>_register (que mete relocs
//           a bpvm_aot_register_by_name + string literal y rompe
//           la position-independence del .o cuando lo empaquetamos
//           a .mdn). El loader del firmware registra a partir del
//           symtab del .mdn — H3 #158.
//
// Sin semantic analysis para mantenerlo simple — el emisor solo
// asume integer i32 por ahora. Errores de tipo o constructos no
// soportados se reportan via UnsupportedAotException.
// ============================================================
package basicplus.frontend;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.List;

public final class AotMain {

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Uso: AotMain <file.bp> [<outDir>] [--mdn]");
            System.exit(2);
        }
        // Separar flags y posicionales.
        boolean mdnMode = false;
        java.util.List<String> positional = new java.util.ArrayList<>();
        for (String a : args) {
            if ("--mdn".equals(a)) {
                mdnMode = true;
            } else {
                positional.add(a);
            }
        }
        Path src = Paths.get(positional.get(0));
        Path outDir = (positional.size() >= 2) ? Paths.get(positional.get(1)) : Paths.get(".");
        try {
            AotResult r = emitAotC(src, outDir, mdnMode);
            // #211 — avisos no fatales (llamadas native→BP que pierden AOT).
            for (String wmsg : r.warnings) {
                System.out.println("-- aviso AOT: " + wmsg);
            }
            if (r.cFile == null) {
                System.out.println("(módulo " + r.moduleName + " no tiene funciones `native` — sin emisión)");
                return;
            }
            System.out.println("emitido: " + r.cFile);
        } catch (AotCEmitter.UnsupportedAotException ex) {
            System.err.println("AOT no soportado: " + ex.getMessage());
            System.exit(3);
        } catch (ParseFailedException ex) {
            System.err.println(ex.getMessage());
            System.exit(1);
        } catch (IOException e) {
            System.err.println("Error de E/S: " + e.getMessage());
            System.exit(1);
        }
    }

    /** Resultado de la emisión AOT: el `.c` generado (null si el módulo no tiene
     *  funciones `native`), el nombre del módulo y los avisos no fatales. */
    public static final class AotResult {
        public final Path cFile;
        public final String moduleName;
        public final List<String> warnings;
        AotResult(Path cFile, String moduleName, List<String> warnings) {
            this.cFile = cFile; this.moduleName = moduleName; this.warnings = warnings;
        }
    }

    /** El `.bp` no parsea (no debería ocurrir si el `.mod` ya compiló). */
    public static final class ParseFailedException extends Exception {
        ParseFailedException(String m) { super(m); }
    }

    /**
     * Lexa + parsea + analiza un `.bp` y emite `aot_&lt;Module&gt;.c` en outDir.
     * REUTILIZABLE desde el CLI (main) y desde el IDE (compilación AOT automática,
     * H12) — sin System.exit, devuelve/lanza para que el caller decida.
     *
     * @return AotResult con cFile=null si el módulo no tiene funciones `native`.
     * @throws AotCEmitter.UnsupportedAotException si alguna native no es AOT-able.
     * @throws ParseFailedException               si el `.bp` no parsea.
     * @throws IOException                        error de lectura/escritura.
     */
    public static AotResult emitAotC(Path src, Path outDir, boolean mdnMode)
            throws IOException, AotCEmitter.UnsupportedAotException, ParseFailedException {
        String source = new String(Files.readAllBytes(src), StandardCharsets.UTF_8);

        // 1. Lex + 2. Parse.
        Lexer lex = new Lexer(source);
        List<Token> tokens = lex.tokenize();
        Parser parser = new Parser(tokens);
        Ast.ModuleNode module = parser.parseModule();
        if (!parser.getErrors().isEmpty() || module == null) {
            StringBuilder sb = new StringBuilder("Errores de parseo en " + src + ":");
            for (ParserError e : parser.getErrors()) sb.append("\n  ").append(e);
            throw new ParseFailedException(sb.toString());
        }

        // 2.5 Semántico + resolución de imports (igual que Main.compileFull): sin
        //     esto una llamada `Mod.func()` desde native no resuelve a símbolo
        //     external y AotCEmitter la rechazaría. Las .bpi de las deps deben
        //     existir (ya compiladas); las que falten se avisan y se omiten (#212).
        SemanticAnalyzer analyzer = new SemanticAnalyzer();
        try {
            Main.Ctx ctx = new Main.Ctx();
            ctx.outDir = outDir;
            Main.loadImportsForAnalyzer(module, src, ctx, analyzer, 0);
        } catch (Exception impEx) {
            System.err.println("aviso: imports no resueltos del todo: " + impEx.getMessage());
        }
        SemanticInfo info = analyzer.analyze(module);

        // 3. Emit AOT (lanza UnsupportedAotException si alguna native no es AOT-able).
        AotCEmitter emitter = new AotCEmitter(module.name);
        emitter.setSemanticInfo(info);
        emitter.setOmitRegisterFunc(mdnMode);
        String csrc = emitter.emitModule(module);

        if (csrc.isEmpty()) {
            return new AotResult(null, module.name, emitter.getWarnings());  // sin funciones native
        }
        // 4. Escribir aot_<Module>.c.
        Files.createDirectories(outDir);
        Path outFile = outDir.resolve("aot_" + module.name + ".c");
        Files.write(outFile, csrc.getBytes(StandardCharsets.UTF_8));
        return new AotResult(outFile, module.name, emitter.getWarnings());
    }
}
