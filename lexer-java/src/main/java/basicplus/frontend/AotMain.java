// ============================================================
// AotMain.java
// CLI standalone para emisor AOT (H3 #157 fase 1).
//
//   java basicplus.frontend.AotMain <file.bp> [<outDir>] [--mdn] [--formato=9|8] [--stdlibDir=DIR]
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
//   --stdlibDir=DIR  La stdlib contra la que se traduce (como el compilador). Sin
//           ella, la del BpVM.cfg que se encuentre subiendo desde el .bp.
//
// #542 (V7/H5, 25-sep) — LOS IMPORTS, COMO EL COMPILADOR. Hasta hoy el AOT sólo
// miraba el BpVM.cfg del cwd y, si no encontraba la interfaz de un import, avisaba
// por stdout y SEGUÍA: sin la de `Core`, `NatNew` salía con `getX`/`getY` por
// ranuras fijas y `areaDe(6,7)` daba 0 con status=OK. Eduardo: «hoy en día Core se
// importa en todos los módulos; si no está, es un error de carga de módulo». Ahora
// busca la stdlib igual que Main (autodiscoverFromSource desde el fuente, y
// --stdlibDir) y un import sin interfaz es ImportsFaltanException, con su nombre.
//
// Errores de tipo o constructos no soportados se reportan via
// UnsupportedAotException.
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
            System.err.println("Uso: AotMain <file.bp> [<outDir>] [--mdn] [--formato=9|8] [--stdlibDir=DIR]  (9 por defecto)");
            System.exit(2);
        }
        // Separar flags y posicionales.
        boolean mdnMode = false;
        java.util.List<String> positional = new java.util.ArrayList<>();
        for (String a : args) {
            if ("--mdn".equals(a)) {
                mdnMode = true;
            } else if (a.startsWith("--formato=")) {
                // V7/H2 (#523) — el `.mdn` se construye con el MISMO formato que el
                // `.mod` al que acompaña: en MOD9 el AOT no hornea ranuras ajenas.
                Main.FORMATO = Integer.parseInt(a.substring("--formato=".length()));
            } else if (a.startsWith("--stdlibDir=")) {   // #542: la stdlib dicha a mano, como en Main
                Main.STDLIB_DIR = a.substring("--stdlibDir=".length());
                if (!Files.isDirectory(Paths.get(Main.STDLIB_DIR))) {
                    System.err.println("--stdlibDir: no es un directorio: " + Main.STDLIB_DIR);
                    System.exit(2);
                }
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
        } catch (ImportsFaltanException ex) {             // #542
            System.err.println("AOT: " + ex.getMessage());
            System.exit(4);
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

    /** #847 — ¿tiene el módulo alguna función que el AOT traduzca? Las mismas que recoge
     *  {@link AotCEmitter#emitModule} en su primera pasada: las funciones del módulo y los métodos
     *  de sus clases, `native` y no `intrinsic`. Si las dos preguntas divergieran, un módulo con
     *  natives se quedaría sin traducir: por eso es la misma, y la prueban AotImportsTest y el
     *  `.mdn` de cada pack en la red. */
    static boolean tieneNative(Ast.ModuleNode module) {
        for (Ast.ITopLevelDecl d : module.defs) {
            if (d instanceof Ast.FuncDef) {
                Ast.FuncDef f = (Ast.FuncDef) d;
                if (f.isNative && !f.isIntrinsic) return true;
            } else if (d instanceof Ast.ClassDef) {
                for (Ast.ITopLevelDecl m : ((Ast.ClassDef) d).members) {
                    if (!(m instanceof Ast.FuncDef)) continue;
                    Ast.FuncDef f = (Ast.FuncDef) m;
                    if (f.isNative && !f.isIntrinsic) return true;
                }
            }
        }
        return false;
    }

    /** El `.bp` no parsea (no debería ocurrir si el `.mod` ya compiló). */
    public static final class ParseFailedException extends Exception {
        ParseFailedException(String m) { super(m); }
    }

    /** #542 — un import del `.bp` no se pudo cargar (su interfaz no está, o no se lee): el AOT
     *  no traduce a ciegas. El mensaje lleva el nombre de cada módulo y dónde se buscó. */
    public static final class ImportsFaltanException extends Exception {
        ImportsFaltanException(String m) { super(m); }
    }

    /**
     * Lexa + parsea + analiza un `.bp` y emite `aot_&lt;Module&gt;.c` en outDir.
     * REUTILIZABLE desde el CLI (main) y desde el IDE (compilación AOT automática,
     * H12) — sin System.exit, devuelve/lanza para que el caller decida.
     *
     * @return AotResult con cFile=null si el módulo no tiene funciones `native`.
     * @throws AotCEmitter.UnsupportedAotException si alguna native no es AOT-able.
     * @throws ParseFailedException               si el `.bp` no parsea.
     * @throws ImportsFaltanException             si un import no carga (#542).
     * @throws IOException                        error de lectura/escritura.
     */
    public static AotResult emitAotC(Path src, Path outDir, boolean mdnMode)
            throws IOException, AotCEmitter.UnsupportedAotException, ParseFailedException, ImportsFaltanException {
        return emitAotC(src, outDir, mdnMode, outDir, java.util.Collections.<String>emptyList());
    }

    /**
     * #542 — lo mismo, diciendo DÓNDE dejó el build los módulos, para buscar las interfaces de los
     * imports donde las busca el compilador. En la línea de órdenes es el mismo {@code outDir}; el IDE
     * escribe el C en {@code <proyecto>/target} y los {@code .mod} están en el {@code outDir} del
     * proyecto: sin decírselo, un {@code import Y} de la propia app «faltaba» y el AOT del módulo se
     * rechazaba (antes de #542 sólo avisaba y seguía).
     *
     * @param modsDir      donde están los {@code .mod} del build: se mira ANTES que junto al fuente,
     *                     como en {@code Main.buildProject} (su {@code ctx.outDir}).
     * @param dependencias las {@code dependencies} del proyecto (ya absolutas, {@link BpBuild}): el
     *                     compilador las mira, y el AOT también (un pack de librería: {@code sqlite/out}).
     */
    public static AotResult emitAotC(Path src, Path outDir, boolean mdnMode, Path modsDir, List<String> dependencias)
            throws IOException, AotCEmitter.UnsupportedAotException, ParseFailedException, ImportsFaltanException {
        String source = new String(Files.readAllBytes(src), StandardCharsets.UTF_8);

        // 1. Lex + 2. Parse. V8 #698: un `.jc`, por el parser de JC, al mismo árbol.
        Ast.ModuleNode module;
        if (Main.esJc(src)) {
            try {
                module = JcParser.leer(source);
            } catch (JcLexer.Error e) {
                throw new ParseFailedException("Errores de parseo en " + src + ":\n  " + e.getMessage());
            }
        } else {
            Lexer lex = new Lexer(source);
            List<Token> tokens = lex.tokenize();
            Parser parser = new Parser(tokens);
            module = parser.parseModule();
            if (!parser.getErrors().isEmpty() || module == null) {
                StringBuilder sb = new StringBuilder("Errores de parseo en " + src + ":");
                for (ParserError e : parser.getErrors()) sb.append("\n  ").append(e);
                throw new ParseFailedException(sb.toString());
            }
        }

        // #847 (20-nov) — un módulo sin `native` no necesita sus imports: no hay C que generar.
        // Hasta hoy los resolvía igual, y uno que sólo viaja en un pack —SQLite, que el AOT no
        // mira (el compilador sí)— daba «falta el modulo 'SQLite'» en el Run de SqlDemo, que no
        // tiene ninguna native y corría bien. Lo que sale es lo de siempre para un módulo sin
        // native: sin C y sin avisos (abajo, `csrc.isEmpty()`). Un módulo CON native sigue
        // resolviendo sus imports, y uno que falta sigue siendo un error con su nombre (#542).
        if (!tieneNative(module)) {
            return new AotResult(null, module.name, new java.util.ArrayList<>());
        }

        // 2.5 Semántico + resolución de imports (igual que Main.compileFull): sin
        //     esto una llamada `Mod.func()` desde native no resuelve a símbolo
        //     external y AotCEmitter la rechazaría. Las interfaces de las deps deben
        //     existir (ya compiladas).
        //     #542 — y se buscan COMO EL COMPILADOR: el BpVM.cfg subiendo desde el
        //     fuente (no sólo el del cwd) y --stdlibDir; los mismos imports implícitos;
        //     y uno que no carga es un ERROR con su nombre, no un aviso que se ignora.
        SemanticAnalyzer analyzer = new SemanticAnalyzer();
        Main.Ctx ctx = new Main.Ctx();
        ctx.outDir = modsDir;                        // donde están los .mod (el C va a outDir, abajo)
        ctx.faltaImportEsError = true;
        ctx.autodiscoverFromSource(src.toAbsolutePath());
        for (String d : dependencias) ctx.dependencyPaths.add(Paths.get(d));   // como buildProject
        Main.injectImplicitCoreImport(module);
        Main.injectImplicitCollectionsImport(module);
        try {
            Main.loadImportsForAnalyzer(module, src, ctx, analyzer, 0);
        } catch (Exception impEx) {
            throw new ImportsFaltanException("los imports de " + src.getFileName() + " no cargan: " + impEx.getMessage());
        }
        if (ctx.totalErrors > 0) {
            String cuales = ctx.importsQueFaltan.isEmpty()
                    ? "un import incompatible (ver el error de arriba)"
                    : String.join("; ", ctx.importsQueFaltan);
            throw new ImportsFaltanException("no se traduce " + src.getFileName() + ": " + cuales);
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
