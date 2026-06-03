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
            String source = new String(Files.readAllBytes(src), StandardCharsets.UTF_8);

            // 1. Lex
            Lexer lex = new Lexer(source);
            List<Token> tokens = lex.tokenize();

            // 2. Parse
            Parser parser = new Parser(tokens);
            Ast.ModuleNode module = parser.parseModule();
            if (!parser.getErrors().isEmpty()) {
                System.err.println("Errores de parseo:");
                for (ParserError e : parser.getErrors()) {
                    System.err.println("  " + e);
                }
                System.exit(1);
            }
            if (module == null) {
                System.err.println("parseModule devolvió null");
                System.exit(1);
            }

            // 2.5 Análisis semántico (#173): el emisor necesita
            //     info.exprTypes para saber el tipo de cada expresión y
            //     distinguir ops de string (concat `+`, `==`) de las
            //     numéricas. No abortamos por errores semánticos — AotMain
            //     solo emite; los exprTypes de las expresiones locales del
            //     cuerpo native se rellenan igualmente.
            SemanticAnalyzer analyzer = new SemanticAnalyzer();
            SemanticInfo info = analyzer.analyze(module);

            // 3. Emit AOT
            AotCEmitter emitter = new AotCEmitter(module.name);
            emitter.setSemanticInfo(info);
            emitter.setOmitRegisterFunc(mdnMode);
            String csrc;
            try {
                csrc = emitter.emitModule(module);
            } catch (AotCEmitter.UnsupportedAotException ex) {
                System.err.println("AOT no soportado para " + module.name + ":");
                System.err.println("  " + ex.getMessage());
                System.exit(3);
                return;
            }

            // #211 — avisos no fatales (llamadas native→BP que pierden AOT).
            for (String wmsg : emitter.getWarnings()) {
                System.out.println("-- aviso AOT: " + wmsg);
            }

            if (csrc.isEmpty()) {
                System.out.println("(módulo " + module.name + " no tiene funciones `native` — sin emisión)");
                return;
            }

            // 4. Escribir
            Files.createDirectories(outDir);
            Path outFile = outDir.resolve("aot_" + module.name + ".c");
            Files.write(outFile, csrc.getBytes(StandardCharsets.UTF_8));
            System.out.println("emitido: " + outFile);
            System.out.println(csrc.length() + " bytes, "
                + csrc.split("\n").length + " líneas");
        } catch (IOException e) {
            System.err.println("Error de E/S: " + e.getMessage());
            System.exit(1);
        }
    }
}
