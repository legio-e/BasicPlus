// ============================================================
// Main.java
// Demo CLI: lex → parse → muestra tokens y/o AST + diagnósticos.
//
// Uso (tras 'mvn package'):
//     java -jar target/basicplus-frontend.jar samples/hello.bp
//     java -jar target/basicplus-frontend.jar samples/hello.bp --tokens
//     java -jar target/basicplus-frontend.jar samples/hello.bp --ast
//     java -jar target/basicplus-frontend.jar samples/hello.bp --quiet
//     java -jar target/basicplus-frontend.jar samples/app.bp --compile out --backend=mivm
//     java -jar target/basicplus-frontend.jar samples/dep.bp --interface out
//
// Driver de compilación (modo `--compile --backend=mivm`):
//   1) Parsea el .bp raíz.
//   2) Para cada import declarado, busca la .bpi en outDir. Si falta y se
//      localiza la .bp fuente, la compila recursivamente en modo
//      INTERFACE_ONLY (genera la .bpi sin código y sin resolver los
//      imports de esa dep — modelo Modula-2 DEFINITION MODULE).
//   3) Analiza el .bp raíz con todos los namespaces importados cargados.
//   4) Emite .mod + .bpi del raíz.
//   5) Para cada import cuyo .mod siga faltando, recurre en modo FULL.
//
// Detección de ciclos: dos sets (compilingFull, compilingInterface) en
// el contexto cortan reentradas; combinadas con el caching en disco
// (skip si .bpi ya existe) garantizan terminación.
//
// Compatible con JDK 8.
// ============================================================
package basicplus.frontend;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public final class Main {

    enum Mode { FULL, INTERFACE_ONLY }

    /** Contexto de compilación que viaja por la recursión. */
    static final class Ctx {
        Path outDir;                                // donde se emiten .mod y .bpi
        String backend = "mivm";                    // sólo afecta al modo FULL
        boolean showTokens = false;
        boolean showAst    = false;
        boolean verbose    = true;                  // imprimir progreso humano

        /** En el call stack actual. Detección de ciclos. */
        final Set<Path> compilingFull      = new LinkedHashSet<>();
        final Set<Path> compilingInterface = new LinkedHashSet<>();

        /** Mapa de "<library>.<Module>" → ruta del .bp. Construido lazy. */
        Map<String, Path> bpSources;
        Path rootSrcDir;

        /**
         * Paths donde buscar dependencias precompiladas (.bpi y .mod) cuando
         * no se encuentran en outDir ni en el directorio del importer.
         * Viene del .bpbuild si se compila en modo proyecto. Cada entry es
         * un directorio o un fichero .mod/.bpi específico.
         *
         * Cuando se construye, el ctor de Ctx añade el `stdlibDir` del
         * BpVM.cfg autodetectado (cwd) — así `import Math` resuelve sin que
         * el usuario tenga que declarar Math.mod como dependency en cada
         * .bpbuild. Si no hay BpVM.cfg, la lista queda vacía.
         */
        final List<Path> dependencyPaths = new ArrayList<>();

        int totalErrors = 0;

        Ctx() {
            // Auto-discover BpVM.cfg/stdlibDir desde cwd. Útil para single-file
            // (CLI sin --project) y para que `import Math` funcione sin que el
            // usuario configure dependencias. Si el caller tiene un path fuente
            // (CLI o IDE), debería llamar adicionalmente a autodiscoverFromSource
            // — esto cubre el caso del IDE arrancado con cwd != raíz del repo.
            autodiscoverFromSource(null);
        }

        /** Intenta localizar BpVM.cfg caminando hacia arriba desde {@code source}
         *  (o sólo en cwd si source==null). Añade stdlibDir y devicesDir a
         *  {@link #dependencyPaths} si están definidos y son dirs. No falla. */
        void autodiscoverFromSource(Path source) {
            try {
                edu.bpgenvm.config.VmConfig cfg = edu.bpgenvm.config.VmConfig.loadDefaultFor(source);
                addDepDirIfPresent(cfg.stdlibDir);
                addDepDirIfPresent(cfg.devicesDir);
            } catch (Throwable ignored) {
                // Sin config; sigue funcionando para módulos que no importan stdlib.
            }
        }

        private void addDepDirIfPresent(String dirStr) {
            if (dirStr == null || dirStr.isEmpty()) return;
            Path d = Paths.get(dirStr);
            if (Files.isDirectory(d) && !dependencyPaths.contains(d)) {
                dependencyPaths.add(d);
            }
        }
    }

    /**
     * API pública in-process para callers externos (typical: un IDE).
     * Compila {@code source} a un .mod en {@code outDir} con el backend
     * indicado ("mivm" por defecto). No llama a {@code System.exit}; los
     * diagnósticos salen por {@link System#out}/{@link System#err} y el
     * caller puede redirigirlos. El número de errores semánticos queda
     * en {@code ctxTotalErrors} del valor devuelto.
     *
     * @return true si la compilación produjo el .mod sin errores semánticos.
     */
    public static boolean compileFile(Path source, Path outDir, String backend) {
        Ctx ctx = new Ctx();
        ctx.outDir = outDir;
        ctx.backend = (backend != null) ? backend : "mivm";
        // Si el cwd no es el del repo (típico cuando un IDE invoca el compile)
        // el ctor de Ctx no habrá localizado BpVM.cfg. Re-intentamos caminando
        // hacia arriba desde el .bp para que `import IO` y resto de stdlib
        // resuelvan sin tener que configurar dependencyPaths a mano.
        if (source != null) ctx.autodiscoverFromSource(source.toAbsolutePath());
        try {
            boolean ok = compileFull(source, ctx, 0);
            return ok && ctx.totalErrors == 0;
        } catch (IOException ex) {
            System.err.println("compileFile: " + ex.getClass().getSimpleName() + ": " + ex.getMessage());
            return false;
        }
    }

    public static void main(String[] args)
    {
        String path = null;
        String compileOutDir   = null;
        String interfaceOutDir = null;
        String projectFile     = null;
        String backend = "jvm";
        boolean showTokens = true;
        boolean showAst    = true;

        for (int i = 0; i < args.length; i++) {
            String a = args[i];
            if ("--tokens".equals(a))      showAst    = false;
            else if ("--ast".equals(a))    showTokens = false;
            else if ("--quiet".equals(a)) { showAst = false; showTokens = false; }
            else if ("--compile".equals(a)) {
                showAst = false; showTokens = false;
                if (i + 1 < args.length) compileOutDir = args[++i];
                else { System.err.println("--compile requiere ruta de salida"); System.exit(1); return; }
            }
            else if ("--interface".equals(a)) {
                showAst = false; showTokens = false;
                if (i + 1 < args.length) interfaceOutDir = args[++i];
                else { System.err.println("--interface requiere ruta de salida"); System.exit(1); return; }
            }
            else if ("--project".equals(a)) {
                showAst = false; showTokens = false;
                if (i + 1 < args.length) projectFile = args[++i];
                else { System.err.println("--project requiere ruta a un .bpbuild"); System.exit(1); return; }
            }
            else if (a.startsWith("--backend=")) {
                backend = a.substring("--backend=".length()).toLowerCase();
                if (!backend.equals("jvm") && !backend.equals("mivm")) {
                    System.err.println("--backend desconocido: " + backend + " (usa jvm|mivm)");
                    System.exit(1); return;
                }
            }
            else if (path == null)         path = a;
            else System.err.println("argumento extra ignorado: " + a);
        }

        // ============================================================
        // Modo proyecto (.bpbuild)
        // ============================================================
        if (projectFile != null) {
            try {
                BpBuild proj = BpBuild.load(Paths.get(projectFile));
                System.out.println("project: " + proj.sourcePath
                        + " (main=" + proj.main
                        + ", sourceDir=" + proj.sourceDir
                        + ", outDir=" + proj.outDir
                        + ", dependencies=" + proj.dependencies + ")");
                Path mainBp = Paths.get(proj.sourceDir, proj.main + ".bp")
                        .toAbsolutePath().normalize();
                if (!Files.exists(mainBp)) {
                    System.err.println("no se encuentra el fichero del main: " + mainBp);
                    System.exit(1); return;
                }
                Ctx ctx = new Ctx();
                ctx.backend    = backend;
                ctx.showTokens = false;
                ctx.showAst    = false;
                ctx.rootSrcDir = Paths.get(proj.sourceDir);
                ctx.outDir     = Paths.get(proj.outDir);
                Files.createDirectories(ctx.outDir);
                for (String d : proj.dependencies) ctx.dependencyPaths.add(Paths.get(d));
                boolean ok = compileFull(mainBp, ctx, /*depth*/0);
                System.exit(ok && ctx.totalErrors == 0 ? 0 : 2);
            } catch (IOException ex) {
                System.err.println("error proyecto: " + ex.getMessage());
                System.exit(2); return;
            }
        }

        if (path == null) {
            System.err.println("Uso: java -jar basicplus-frontend.jar <archivo.bp> [--tokens|--ast|--quiet]");
            System.err.println("     java -jar basicplus-frontend.jar --project <archivo.bpbuild>");
            System.exit(1);
            return;
        }

        Path p = Paths.get(path).toAbsolutePath().normalize();
        if (!Files.exists(p)) {
            System.err.println("No se encuentra el archivo: " + p);
            System.exit(1);
            return;
        }

        Ctx ctx = new Ctx();
        ctx.backend = backend;
        ctx.showTokens = showTokens;
        ctx.showAst    = showAst;
        ctx.rootSrcDir = p.getParent();
        // Re-intenta localizar BpVM.cfg caminando desde el .bp. Si el usuario
        // lanza el comando desde un cwd que no contiene BpVM.cfg, así seguimos
        // resolviendo stdlib.
        ctx.autodiscoverFromSource(p);

        try {
            if (interfaceOutDir != null) {
                ctx.outDir = Paths.get(interfaceOutDir);
                Files.createDirectories(ctx.outDir);
                boolean ok = compileInterface(p, ctx, /*depth*/0);
                System.exit(ok && ctx.totalErrors == 0 ? 0 : 2);
            } else if (compileOutDir != null) {
                ctx.outDir = Paths.get(compileOutDir);
                Files.createDirectories(ctx.outDir);
                boolean ok = compileFull(p, ctx, /*depth*/0);
                System.exit(ok && ctx.totalErrors == 0 ? 0 : 2);
            } else {
                // Sin --compile ni --interface: sólo diagnósticos del raíz.
                ctx.outDir = null;
                runDiagnosticsOnly(p, ctx);
                System.exit(ctx.totalErrors == 0 ? 0 : 2);
            }
        } catch (IOException ex) {
            System.err.println("error de I/O: " + ex.getMessage());
            System.exit(2);
        }
    }

    // ============================================================
    // MODO DIAGNÓSTICOS (sin emisión, comportamiento original)
    // ============================================================
    private static void runDiagnosticsOnly(Path p, Ctx ctx) {
        Parsed parsed = parseAndPrint(p, ctx, /*isRoot*/true);
        if (parsed == null || parsed.module == null) return;
        if (!parsed.parser.getErrors().isEmpty()) return;

        SemanticAnalyzer analyzer = new SemanticAnalyzer();
        SemanticInfo info = analyzer.analyze(parsed.module);
        printSemantics(info, parsed.module);
        ctx.totalErrors += countSemErrors(info);
    }

    // ============================================================
    // COMPILACIÓN FULL: produce .mod + .bpi del módulo. Recursivamente
    // garantiza .bpi de cada import (modo INTERFACE) antes de analizar
    // y, tras emitir, recursivamente garantiza .mod de cada import.
    // ============================================================
    private static boolean compileFull(Path src, Ctx ctx, int depth) throws IOException {
        Path srcAbs = src.toAbsolutePath().normalize();
        if (ctx.compilingFull.contains(srcAbs)) {
            indent(depth); System.out.println("(ciclo full detectado, skip: " + srcAbs.getFileName() + ")");
            return true;
        }
        // Fast-path: si el .mod ya existe y NO está obsoleto respecto al
        // .bp fuente, asumimos que el .bpi también está al día y saltamos.
        // Sólo aplicable a compilaciones transitivas (depth > 0); la raíz
        // siempre re-compila por petición explícita del usuario.
        if (depth > 0) {
            String modName = peekArtifactName(src, ".mod");
            if (modName != null) {
                Path modPath = ctx.outDir.resolve(modName);
                if (Files.exists(modPath) && !isStale(modPath, src)) {
                    indent(depth); System.out.println("(.mod fresco, skip: " + modPath.getFileName() + ")");
                    return true;
                }
            }
        }
        ctx.compilingFull.add(srcAbs);
        try {
            indent(depth); System.out.println(">>> compile " + srcAbs.getFileName() + " (mode=FULL)");

            Parsed parsed = parseAndPrint(src, ctx, /*isRoot*/depth == 0);
            if (parsed == null) return false;
            if (!parsed.lexer.getErrors().isEmpty() || !parsed.parser.getErrors().isEmpty()) {
                ctx.totalErrors += parsed.lexer.getErrors().size() + parsed.parser.getErrors().size();
                indent(depth); System.err.println("compilación abortada por errores de parseo en " + srcAbs.getFileName());
                return false;
            }
            Ast.ModuleNode module = parsed.module;
            if (module == null) return false;

            // 1) Garantizar .bpi de cada import (recursión INTERFACE_ONLY si falta).
            //    Para imports bindeados (`import Iface:Impl`), garantizamos
            //    ambas: la interfaz para typecheck y la impl para extraer su
            //    qualified name de runtime.
            if (module.imports != null) {
                for (Ast.ImportNode imp : module.imports) {
                    ensureInterfaceForImport(imp, src, ctx, depth + 1);
                    if (imp.boundImpl != null) {
                        ensureInterfaceForBoundImpl(imp.boundImpl, imp.fromPath, module, src, ctx, depth + 1);
                    }
                }
            }

            // 2) Cargar .bpi disponibles en el analizador.
            SemanticAnalyzer analyzer = new SemanticAnalyzer();
            loadImportsForAnalyzer(module, src, ctx, analyzer, depth + 1);

            // 2b) Si `module X implements Lib.Iface` (módulo concreto) o
            //     `module interface X extends Lib.Parent` (interfaz hija):
            //     garantizar la cadena completa de contratos y construir
            //     la unión plana para verificación de conformidad.
            //
            //     Para interfaces hijas no hace falta verificar — heredan
            //     pero no implementan — así que se omite el setContract.
            if (module.implementsName != null) {
                ensureContractChain(module.implementsName, src, ctx, depth + 1, new java.util.HashSet<>());
                if (!module.isInterface) {
                    ModuleInterface contract = flattenContract(module.implementsName, src, ctx, depth + 1);
                    if (contract != null) {
                        indent(depth + 1); System.out.printf(
                            "-- contrato plano '%s' resuelto (funcs=%d, consts=%d, enums=%d) --%n",
                            module.implementsName,
                            contract.functions.size(), contract.consts.size(), contract.enums.size());
                        analyzer.setImplementsContract(contract);
                    }
                }
            }

            // 3) Análisis completo.
            SemanticInfo info = analyzer.analyze(module);
            if (depth == 0) printSemantics(info, module);
            int errs = countSemErrors(info);
            ctx.totalErrors += errs;
            if (errs > 0) {
                indent(depth); System.err.println("compilación abortada por errores semánticos en " + srcAbs.getFileName());
                return false;
            }

            // 3.5) Validación AOT para funciones marcadas `function native`.
            //      Una función nativa promete que su cuerpo es expresable
            //      como C (subset soportado por AotCEmitter). Si el cuerpo
            //      usa constructos fuera de ese subset (cross-module call,
            //      tipos no soportados, statements como TRY/THROW, etc.),
            //      el error debe saltar AQUÍ — en el compile normal, no
            //      sólo al lanzar el AOT pipeline (build_mdn.sh / AotMain).
            //      Es un error duro: aborta la compilación.
            String aotMsg = validateNativeFunctionsAreAotable(module, info);
            if (aotMsg != null) {
                indent(depth);
                System.err.println("error AOT en función native:");
                indent(depth);
                System.err.println("  " + aotMsg);
                indent(depth);
                System.err.println("compilación abortada por error AOT en " + srcAbs.getFileName());
                ctx.totalErrors++;
                return false;
            }

            // 4) Emisión:
            //    - Si es `module interface X`, sólo escribimos la .bpi.
            //    - En otro caso, .mod (según backend) y la .bpi como subproducto.
            if (module.isInterface) {
                writeInterfaceFile(module, info, ctx, depth);
                indent(depth); System.out.println("interface-only: no se emite .mod");
                return true;
            }
            if ("mivm".equals(ctx.backend)) {
                emitMivmMod(module, info, ctx, depth, srcAbs);
            } else {
                emitJvmClass(module, info, ctx, depth);
                return true;
            }
            writeInterfaceFile(module, info, ctx, depth);

            // 5) Pasada recursiva FULL para deps con .bpi pero sin .mod.
            //    Para imports bindeados, queremos el .mod del IMPL, no de la
            //    interfaz (las interfaces no tienen .mod). Si no hay binding,
            //    el import apunta a un módulo concreto: ensure su .mod.
            if (module.imports != null) {
                for (Ast.ImportNode imp : module.imports) {
                    if (imp.boundImpl != null) {
                        ensureFullModForBoundImpl(imp.boundImpl, imp.fromPath, module, src, ctx, depth + 1);
                    } else {
                        ensureFullModForImport(imp, src, ctx, depth + 1);
                    }
                }
            }
            return true;
        } finally {
            ctx.compilingFull.remove(srcAbs);
        }
    }

    // ============================================================
    // COMPILACIÓN INTERFACE_ONLY: parsea + analiza solo decls + escribe .bpi.
    // NO procesa imports (modelo Modula-2 DEFINITION: no hace falta resolverlos
    // para extraer la firma).
    // ============================================================
    private static boolean compileInterface(Path src, Ctx ctx, int depth) throws IOException {
        Path srcAbs = src.toAbsolutePath().normalize();
        if (ctx.compilingInterface.contains(srcAbs)) {
            indent(depth); System.out.println("(ciclo interface detectado, skip: " + srcAbs.getFileName() + ")");
            return true;
        }
        // Fast-path: .bpi fresca → skip.
        if (depth > 0) {
            String bpiName = peekArtifactName(src, ".bpi");
            if (bpiName != null) {
                Path bpiPath = ctx.outDir.resolve(bpiName);
                if (Files.exists(bpiPath) && !isBpiOutdated(bpiPath, src)) {
                    indent(depth); System.out.println("(.bpi fresca, skip: " + bpiPath.getFileName() + ")");
                    return true;
                }
            }
        }
        ctx.compilingInterface.add(srcAbs);
        try {
            indent(depth); System.out.println(">>> compile " + srcAbs.getFileName() + " (mode=INTERFACE)");

            Parsed parsed = parseAndPrint(src, ctx, /*isRoot*/depth == 0);
            if (parsed == null) return false;
            if (!parsed.lexer.getErrors().isEmpty() || !parsed.parser.getErrors().isEmpty()) {
                ctx.totalErrors += parsed.lexer.getErrors().size() + parsed.parser.getErrors().size();
                indent(depth); System.err.println("interface abortada por errores de parseo en " + srcAbs.getFileName());
                return false;
            }
            Ast.ModuleNode module = parsed.module;
            if (module == null) return false;

            SemanticAnalyzer analyzer = new SemanticAnalyzer();
            SemanticInfo info = analyzer.analyzeInterface(module);
            // No abortamos ante errores: en modo interface puede haber refs no
            // resueltas (a tipos importados). Sólo contamos diagnósticos para
            // información. La extracción saltará firmas no exportables.
            if (depth == 0) printSemantics(info, module);

            if (info.module == null) {
                indent(depth); System.err.println("interface sin ModuleSymbol en " + srcAbs.getFileName());
                return false;
            }
            writeInterfaceFile(module, info, ctx, depth);
            return true;
        } finally {
            ctx.compilingInterface.remove(srcAbs);
        }
    }

    // ============================================================
    // HELPERS DE EMISIÓN
    // ============================================================
    /**
     * Valida que cada función `native` del módulo sea AOT-able. Re-usa el
     * AotCEmitter como oracle: lo invoca en modo dry-run (descartamos el
     * .c emitido) y captura {@link AotCEmitter.UnsupportedAotException}.
     *
     * Devuelve `null` si todas las funciones nativas son válidas, o un
     * mensaje de error legible (con número de línea cuando AotCEmitter lo
     * incluye en su mensaje). El caller aborta la compilación.
     *
     * Diseño: la palabra clave `function native` es una PROMESA del autor
     * de que el cuerpo encaja en el subset AOT. Aceptar esa promesa sin
     * verificar permite que el bytecode .mod se emita OK pero el AOT
     * pipeline falle aguas abajo (build_mdn.sh, AotMain) — el usuario lo
     * descubre tarde, sin contexto. Validar aquí da el error en la fase
     * que el usuario asocia con "el compilador" y con la línea concreta.
     */
    private static String validateNativeFunctionsAreAotable(Ast.ModuleNode module, SemanticInfo info) {
        // Ningún native → no hay nada que validar.
        boolean hasNative = false;
        if (module.defs != null) {
            for (Ast.ITopLevelDecl d : module.defs) {
                if (d instanceof Ast.FuncDef) {
                    Ast.FuncDef f = (Ast.FuncDef) d;
                    if (f.isNative && !f.isIntrinsic) { hasNative = true; break; }
                }
            }
        }
        if (!hasNative) return null;

        try {
            AotCEmitter emitter = new AotCEmitter(module.name);
            // #173 — info semántica para que el validador conozca tipos
            // y acepte ops de string (concat/==) sin falsos negativos.
            emitter.setSemanticInfo(info);
            // Modo .mdn-friendly: omitimos la función register para no
            // disparar errores espúreos sobre relocs a símbolos del
            // runtime que la validación no necesita verificar.
            emitter.setOmitRegisterFunc(true);
            String csrc = emitter.emitModule(module);
            // csrc puede estar vacío si no hay nativas — ya filtrado arriba,
            // así que aquí esperamos algo. Si no, raro pero no error.
            if (csrc == null) return null;
            return null;
        } catch (AotCEmitter.UnsupportedAotException ex) {
            return ex.getMessage();
        } catch (RuntimeException ex) {
            // Errores inesperados del emisor — mejor surfaciarlos como
            // bug interno que ocultarlos.
            return "fallo interno en validación AOT: " + ex.getMessage();
        }
    }

    private static void emitMivmMod(Ast.ModuleNode module, SemanticInfo info, Ctx ctx, int depth,
                                    java.nio.file.Path sourcePath) throws IOException {
        MivmEmitter emitter = new MivmEmitter(module, info);
        if (sourcePath != null) {
            emitter.setSourcePath(sourcePath.toString());
        }
        emitter.emitTo(ctx.outDir);
        if (!emitter.errors.isEmpty()) {
            indent(depth); System.out.printf("-- Errores del emisor mivm (%d) --%n", emitter.errors.size());
            for (String e : emitter.errors) System.out.println("  " + e);
            ctx.totalErrors += emitter.errors.size();
        }
        String modFilename = (module.library != null && !module.library.isEmpty())
                ? module.library + "." + module.name + ".mod"
                : module.name + ".mod";
        indent(depth); System.out.println("compilado: " + ctx.outDir.toAbsolutePath().resolve(modFilename));
    }

    private static void emitJvmClass(Ast.ModuleNode module, SemanticInfo info, Ctx ctx, int depth) throws IOException {
        JvmEmitter emitter = new JvmEmitter(module, info);
        emitter.emitTo(ctx.outDir);
        if (!emitter.errors.isEmpty()) {
            indent(depth); System.out.printf("-- Errores del emisor JVM (%d) --%n", emitter.errors.size());
            for (String e : emitter.errors) System.out.println("  " + e);
            ctx.totalErrors += emitter.errors.size();
        }
        indent(depth); System.out.println("compilado: " + ctx.outDir.toAbsolutePath().resolve(module.name + ".class"));
    }

    private static void writeInterfaceFile(Ast.ModuleNode module, SemanticInfo info, Ctx ctx, int depth) throws IOException {
        String lib = (module.library == null) ? "" : module.library;
        String bpiName = lib.isEmpty() ? module.name + ".bpi" : lib + "." + module.name + ".bpi";
        List<String> skipped = new ArrayList<>();
        ModuleInterface iface = ModuleInterface.extractFrom(
                lib, module.name, module.isInterface, module.implementsName,
                info.module, skipped);
        Path bpiPath = ctx.outDir.resolve(bpiName);
        iface.writeTo(bpiPath);
        indent(depth); System.out.printf("interfaz : %s (funcs=%d%s%s)%n",
                bpiPath.toAbsolutePath(), iface.functions.size(),
                module.isInterface ? ", interface=true" : "",
                module.implementsName == null ? ""
                        : (module.isInterface
                                ? ", extends=" + module.implementsName
                                : ", implements=" + module.implementsName));
        if (!skipped.isEmpty()) {
            indent(depth); System.out.printf("  -- omitidas en interfaz (%d): %s%n",
                    skipped.size(), String.join("; ", skipped));
        }
    }

    // ============================================================
    // RESOLUCIÓN DE DEPS (.bpi y .bp)
    // ============================================================
    private static void ensureInterfaceForImport(Ast.ImportNode imp, Path importerSrc, Ctx ctx, int depth) throws IOException {
        String qualifiedName = joinPath(imp.path);
        String library = libraryFromImportPath(imp);
        String moduleName = imp.path.get(imp.path.size() - 1);
        String bpiName = library.isEmpty() ? moduleName + ".bpi" : library + "." + moduleName + ".bpi";
        Path importerDir = importerSrc.toAbsolutePath().getParent();

        // En `import Iface:Impl from "..."`, el fromPath pertenece al impl, no
        // a la interfaz. Para resolver la interfaz no debemos usarlo (lo hace
        // ensureInterfaceForBoundImpl). Sólo en `import Module from "..."`
        // (sin binding) el fromPath aplica al propio módulo.
        String effectiveFromPath = (imp.boundImpl != null) ? null : imp.fromPath;

        Path bpSource = locateBpSource(qualifiedName, library, moduleName, effectiveFromPath, importerSrc, ctx);

        // 1) Si hay fromPath aplicable, busca la .bpi directamente allí.
        if (effectiveFromPath != null && !effectiveFromPath.isEmpty()) {
            String fp = effectiveFromPath;
            if (fp.endsWith(".mod")) fp = fp.substring(0, fp.length() - 4) + ".bpi";
            Path fromBpi = importerDir.resolve(fp).toAbsolutePath().normalize();
            if (Files.exists(fromBpi)) {
                if (!isBpiOutdated(fromBpi, bpSource)) return;
                indent(depth); System.out.printf("-- .bpi obsoleta (%s); regenerando --%n", fromBpi.getFileName());
            }
        }
        // 2) Convención: .bpi en outDir.
        Path bpiInOut = ctx.outDir.resolve(bpiName);
        if (Files.exists(bpiInOut)) {
            if (!isBpiOutdated(bpiInOut, bpSource)) return;
            indent(depth); System.out.printf("-- .bpi obsoleta (%s); regenerando --%n", bpiInOut.getFileName());
        }

        // 3) Compila la interfaz desde el .bp source.
        if (bpSource == null) {
            indent(depth); System.out.printf("-- no se localizó .bp ni .bpi para import '%s'; se omitirá --%n", qualifiedName);
            return;
        }
        compileInterface(bpSource, ctx, depth);
    }

    private static void ensureFullModForImport(Ast.ImportNode imp, Path importerSrc, Ctx ctx, int depth) throws IOException {
        String library = libraryFromImportPath(imp);
        String moduleName = imp.path.get(imp.path.size() - 1);
        String modName = library.isEmpty() ? moduleName + ".mod" : library + "." + moduleName + ".mod";
        Path importerDir = importerSrc.toAbsolutePath().getParent();
        Path bpSource = locateBpSource(joinPath(imp.path), library, moduleName, imp.fromPath, importerSrc, ctx);

        // Si el BP declaró `from "<path>"`, busca el .mod directamente allí.
        if (imp.fromPath != null && !imp.fromPath.isEmpty()) {
            Path fromMod = importerDir.resolve(imp.fromPath).toAbsolutePath().normalize();
            if (Files.exists(fromMod)) {
                if (!isStale(fromMod, bpSource)) return;
                indent(depth); System.out.printf("-- .mod obsoleto (%s); regenerando --%n", fromMod.getFileName());
            }
        }
        Path modInOut = ctx.outDir.resolve(modName);
        if (Files.exists(modInOut)) {
            if (!isStale(modInOut, bpSource)) return;
            indent(depth); System.out.printf("-- .mod obsoleto (%s); regenerando --%n", modInOut.getFileName());
        }

        if (bpSource == null) return;
        compileFull(bpSource, ctx, depth);
    }

    /**
     * Peek rápido a un .bp para construir el nombre canónico de su artefacto
     * "<library>.<module><ext>" sin parsear el cuerpo. Devuelve null si no
     * puede determinar el module.
     */
    private static String peekArtifactName(Path bpSrc, String ext) {
        try {
            String lib = "";
            String mod = "";
            for (String line : Files.readAllLines(bpSrc, StandardCharsets.UTF_8)) {
                String t = line.trim();
                if (t.isEmpty() || t.startsWith("//")) continue;
                if (t.startsWith("library ")) {
                    int q1 = t.indexOf('"');
                    int q2 = (q1 >= 0) ? t.indexOf('"', q1 + 1) : -1;
                    if (q1 >= 0 && q2 > q1) lib = t.substring(q1 + 1, q2);
                } else if (t.startsWith("module ")) {
                    String[] parts = t.split("\\s+");
                    if (parts.length >= 2) mod = parts[1];
                    break;
                }
            }
            if (mod.isEmpty()) return null;
            return lib.isEmpty() ? (mod + ext) : (lib + "." + mod + ext);
        } catch (IOException ex) {
            return null;
        }
    }

    /**
     * Devuelve true si el artefacto está más viejo que el fuente. Si el fuente
     * no existe (null o no encontrado), se considera no-stale (no podemos
     * regenerar de todos modos). Se compara mtime del filesystem.
     */
    private static boolean isStale(Path artifact, Path source) {
        if (source == null) return false;
        try {
            if (!Files.exists(source)) return false;
            long aT = Files.getLastModifiedTime(artifact).toMillis();
            long sT = Files.getLastModifiedTime(source).toMillis();
            return aT < sT;
        } catch (IOException ex) {
            return false;
        }
    }

    /**
     * Lee la primera línea no-vacía de un .bpi y extrae su versión.
     * Devuelve -1 si no hay header válido o hay error de lectura.
     * Sólo se usa para decidir si un .bpi cacheado debe regenerarse tras
     * un bump del formato (E2): si version &lt; CURRENT_VERSION, lo tratamos
     * como obsoleto aunque su mtime sea fresco respecto al .bp.
     */
    private static int readBpiVersion(Path bpiPath) {
        try (java.io.BufferedReader br = Files.newBufferedReader(bpiPath,
                java.nio.charset.StandardCharsets.UTF_8)) {
            String line;
            while ((line = br.readLine()) != null) {
                String t = line.trim();
                if (t.isEmpty() || t.startsWith("#")) continue;
                if (t.startsWith("bpi ")) {
                    try { return Integer.parseInt(t.substring(4).trim()); }
                    catch (NumberFormatException ex) { return -1; }
                }
                return -1;   // primera línea útil no es header válido
            }
        } catch (IOException ex) {
            return -1;
        }
        return -1;
    }

    /**
     * True si el .bpi en disco es estale por mtime O su formato es anterior
     * al actual (CURRENT_VERSION del ModuleInterface). Sustituye al
     * `isStale` plano en todos los chequeos sobre .bpi.
     */
    private static boolean isBpiOutdated(Path bpiPath, Path source) {
        if (isStale(bpiPath, source)) return true;
        int ver = readBpiVersion(bpiPath);
        return ver < ModuleInterface.CURRENT_VERSION;
    }

    /**
     * Garantiza recursivamente que la .bpi del contrato y todos sus padres
     * (cadena de `extends`) está disponible. Para `module X implements C`
     * donde C `extends P`, asegura tanto C.bpi como P.bpi (y la cadena que
     * P pueda tener encima).
     */
    private static void ensureContractChain(String qualifiedName, Path importerSrc,
                                            Ctx ctx, int depth, java.util.Set<String> visited) throws IOException {
        if (qualifiedName == null || qualifiedName.isEmpty() || visited.contains(qualifiedName)) return;
        visited.add(qualifiedName);
        ensureContractAtName(qualifiedName, importerSrc, ctx, depth);
        ModuleInterface bpi = loadContractBpiByName(qualifiedName, importerSrc, ctx, depth);
        if (bpi != null && bpi.extendsName != null && !bpi.extendsName.isEmpty()) {
            ensureContractChain(bpi.extendsName, importerSrc, ctx, depth, visited);
        }
    }

    /**
     * Como ensureContractForImplements pero a partir de un qualified name puro
     * (sin AST). Reutilizable para padres dentro de la cadena.
     */
    private static void ensureContractAtName(String qualifiedName, Path importerSrc, Ctx ctx, int depth) throws IOException {
        String[] segs = qualifiedName.split("\\.");
        String moduleName = segs[segs.length - 1];
        String library;
        if (segs.length >= 2) {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < segs.length - 1; i++) {
                if (i > 0) sb.append('.');
                sb.append(segs[i]);
            }
            library = sb.toString();
        } else library = "";
        String bpiName = library.isEmpty() ? moduleName + ".bpi" : library + "." + moduleName + ".bpi";
        Path bpiInOut = ctx.outDir.resolve(bpiName);
        Path bpSource = locateBpSource(qualifiedName, library, moduleName, null, importerSrc, ctx);
        if (Files.exists(bpiInOut)) {
            if (!isBpiOutdated(bpiInOut, bpSource)) return;
            indent(depth); System.out.printf("-- .bpi de contrato obsoleta (%s); regenerando --%n", bpiInOut.getFileName());
        }
        if (bpSource == null) {
            indent(depth); System.out.printf("-- no se localizó .bp ni .bpi para contrato '%s' --%n", qualifiedName);
            return;
        }
        compileInterface(bpSource, ctx, depth);
    }

    /** Lee y devuelve la .bpi del contrato dado (sin flatten). null si no la encuentra. */
    private static ModuleInterface loadContractBpiByName(String qualifiedName, Path importerSrc,
                                                         Ctx ctx, int depth) throws IOException {
        String[] segs = qualifiedName.split("\\.");
        String moduleName = segs[segs.length - 1];
        String library;
        if (segs.length >= 2) {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < segs.length - 1; i++) {
                if (i > 0) sb.append('.');
                sb.append(segs[i]);
            }
            library = sb.toString();
        } else library = "";
        String bpiName = library.isEmpty() ? moduleName + ".bpi" : library + "." + moduleName + ".bpi";
        Path p = ctx.outDir.resolve(bpiName);
        if (!Files.exists(p)) {
            Path sib = importerSrc.toAbsolutePath().getParent().resolve(bpiName);
            if (Files.exists(sib)) p = sib;
            else return null;
        }
        return ModuleInterface.readFrom(p);
    }

    /**
     * Aplana la cadena de `extends` de un contrato a una sola
     * ModuleInterface con la unión de funciones / consts / enums. Las
     * declaraciones de la hija prevalecen sobre las del padre cuando hay
     * choque de nombres.
     */
    private static ModuleInterface flattenContract(String qualifiedName, Path importerSrc,
                                                   Ctx ctx, int depth) throws IOException {
        java.util.Set<String> visited = new java.util.HashSet<>();
        return flattenContract0(qualifiedName, importerSrc, ctx, depth, visited);
    }

    private static ModuleInterface flattenContract0(String qualifiedName, Path importerSrc,
                                                    Ctx ctx, int depth,
                                                    java.util.Set<String> visited) throws IOException {
        if (qualifiedName == null || qualifiedName.isEmpty()) return null;
        if (visited.contains(qualifiedName)) return null;
        visited.add(qualifiedName);
        ModuleInterface mine = loadContractBpiByName(qualifiedName, importerSrc, ctx, depth);
        if (mine == null) return null;
        if (mine.extendsName == null || mine.extendsName.isEmpty()) return mine;
        ModuleInterface parent = flattenContract0(mine.extendsName, importerSrc, ctx, depth, visited);
        if (parent == null) return mine;
        // Construir la unión: hija prevalece sobre padre.
        ModuleInterface flat = new ModuleInterface();
        flat.library = mine.library;
        flat.moduleName = mine.moduleName;
        flat.isInterface = mine.isInterface;
        flat.implementsName = mine.implementsName;
        flat.extendsName = null;
        java.util.Set<String> seenF = new java.util.HashSet<>();
        java.util.Set<String> seenC = new java.util.HashSet<>();
        java.util.Set<String> seenE = new java.util.HashSet<>();
        for (ModuleInterface.FuncSig f : mine.functions) { flat.functions.add(f); seenF.add(f.name); }
        for (ModuleInterface.ConstSig c : mine.consts)   { flat.consts.add(c); seenC.add(c.name); }
        for (ModuleInterface.EnumSig e : mine.enums)     { flat.enums.add(e); seenE.add(e.name); }
        for (ModuleInterface.FuncSig f : parent.functions) if (!seenF.contains(f.name)) flat.functions.add(f);
        for (ModuleInterface.ConstSig c : parent.consts)   if (!seenC.contains(c.name)) flat.consts.add(c);
        for (ModuleInterface.EnumSig e : parent.enums)     if (!seenE.contains(e.name)) flat.enums.add(e);
        return flat;
    }

    /**
     * Devuelve true si el impl declarado implementa, directa o
     * transitivamente, el contrato pedido. Walks la cadena
     * implBpi.implementsName → su .bpi → su extendsName → … hasta encontrar
     * wantedQualified o agotarse.
     */
    private static boolean implSatisfies(ModuleInterface implBpi, String wantedQualified,
                                         Path importerSrc, Ctx ctx, int depth) throws IOException {
        if (implBpi.implementsName == null || implBpi.implementsName.isEmpty()) return false;
        // Aseguramos que toda la cadena .bpi esté disponible antes de
        // recorrerla. Sin esto, un eslabón intermedio podría faltar y
        // cortaríamos la búsqueda dando un falso negativo.
        ensureContractChain(implBpi.implementsName, importerSrc, ctx, depth, new java.util.HashSet<>());
        String cur = implBpi.implementsName;
        java.util.Set<String> seen = new java.util.HashSet<>();
        while (cur != null && !cur.isEmpty()) {
            if (seen.contains(cur)) return false;
            seen.add(cur);
            if (cur.equals(wantedQualified)) return true;
            ModuleInterface c = loadContractBpiByName(cur, importerSrc, ctx, depth);
            if (c == null) return false;
            cur = c.extendsName;
        }
        return false;
    }

    /**
     * Garantiza que la .bpi de la interfaz que un módulo dice implementar
     * está disponible: si falta, localiza su .bp fuente y recompila en modo
     * INTERFACE_ONLY.
     */
    private static void ensureContractForImplements(Ast.ModuleNode module, Path importerSrc,
                                                    Ctx ctx, int depth) throws IOException {
        String[] segs = module.implementsName.split("\\.");
        String moduleName = segs[segs.length - 1];
        String library;
        if (segs.length >= 2) {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < segs.length - 1; i++) {
                if (i > 0) sb.append('.');
                sb.append(segs[i]);
            }
            library = sb.toString();
        } else {
            library = (module.library == null) ? "" : module.library;
        }
        String bpiName = library.isEmpty() ? moduleName + ".bpi" : library + "." + moduleName + ".bpi";
        Path bpiInOut = ctx.outDir.resolve(bpiName);
        Path bpSource = locateBpSource(module.implementsName, library, moduleName, null, importerSrc, ctx);
        if (Files.exists(bpiInOut)) {
            if (!isBpiOutdated(bpiInOut, bpSource)) return;
            indent(depth); System.out.printf("-- .bpi de interfaz obsoleta (%s); regenerando --%n", bpiInOut.getFileName());
        }
        if (bpSource == null) {
            indent(depth); System.out.printf("-- no se localizó .bp ni .bpi para interfaz '%s' --%n", module.implementsName);
            return;
        }
        compileInterface(bpSource, ctx, depth);
    }

    private static ModuleInterface loadContractInterface(String qualifiedName, Path importerSrc,
                                                         Ctx ctx, int depth) throws IOException {
        String[] segs = qualifiedName.split("\\.");
        String moduleName = segs[segs.length - 1];
        String library;
        if (segs.length >= 2) {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < segs.length - 1; i++) {
                if (i > 0) sb.append('.');
                sb.append(segs[i]);
            }
            library = sb.toString();
        } else library = "";
        String bpiName = library.isEmpty() ? moduleName + ".bpi" : library + "." + moduleName + ".bpi";
        Path candidate = ctx.outDir.resolve(bpiName);
        if (!Files.exists(candidate)) {
            Path sib = importerSrc.toAbsolutePath().getParent().resolve(bpiName);
            if (Files.exists(sib)) candidate = sib;
            else {
                indent(depth); System.out.printf("-- aviso: no se encontró .bpi para interfaz '%s' --%n", qualifiedName);
                return null;
            }
        }
        try {
            ModuleInterface iface = ModuleInterface.readFrom(candidate);
            indent(depth); System.out.printf("-- cargado contrato '%s' desde %s (funcs=%d, consts=%d, enums=%d, interface=%s) --%n",
                    qualifiedName, candidate.getFileName(),
                    iface.functions.size(), iface.consts.size(), iface.enums.size(),
                    iface.isInterface);
            return iface;
        } catch (IOException ex) {
            System.err.println("error leyendo " + candidate + ": " + ex.getMessage());
            return null;
        }
    }

    private static void loadImportsForAnalyzer(Ast.ModuleNode module, Path importerSrc, Ctx ctx,
                                               SemanticAnalyzer analyzer, int depth) throws IOException {
        if (module.imports == null || module.imports.isEmpty()) return;
        Path importerDir = importerSrc.toAbsolutePath().getParent();
        // L2 v3.e — recolectamos todos los namespaces que se cargan para
        // poder resolver tipos cross-module (`L2Lib.Counter`) en un segundo
        // pass tras crearlos todos. La resolución intra-ns se sigue haciendo
        // inline en el primer pass.
        java.util.List<Symbol.ImportedNamespaceSymbol> loadedNs = new java.util.ArrayList<>();
        for (Ast.ImportNode imp : module.imports) {
            String alias = imp.path.get(imp.path.size() - 1);
            String library = libraryFromImportPath(imp);
            String bpiName = library.isEmpty() ? alias + ".bpi" : library + "." + alias + ".bpi";

            // ---- 1) Localizar la .bpi del path (interfaz o módulo concreto) ----
            Path bpi = locateImportBpi(imp, bpiName, library, alias, importerDir, ctx);
            if (bpi == null) {
                indent(depth); System.out.printf("-- aviso: sin interfaz '%s' para import '%s' --%n",
                        bpiName, alias);
                continue;
            }

            try {
                ModuleInterface ifaceBpi = ModuleInterface.readFrom(bpi);
                // Si la interfaz extiende otra, garantizamos la cadena y la
                // aplanamos para que el namespace exponga también los símbolos
                // heredados.
                if (ifaceBpi.isInterface && ifaceBpi.extendsName != null && !ifaceBpi.extendsName.isEmpty()) {
                    ensureContractChain(ifaceBpi.extendsName, importerSrc, ctx, depth, new java.util.HashSet<>());
                    ModuleInterface flat = flattenContract(joinPath(imp.path), importerSrc, ctx, depth);
                    if (flat != null) ifaceBpi = flat;
                }

                // ---- 2) Resolver el módulo concreto contra el que generar CALL_EXT ----
                //
                //   Si imp.boundImpl != null: el usuario escribió `import Iface:Impl`,
                //   así que cargamos también la .bpi del Impl y la usamos para
                //   los nombres "externalLibrary" / "externalModule". La interfaz
                //   manda en la firma; el impl manda en la dirección runtime.
                //
                //   Si imp.boundImpl == null: import directo a un módulo concreto
                //   (compatibilidad). Usamos el propio bpi como impl.
                String implLibrary   = ifaceBpi.library;
                String implModule    = ifaceBpi.moduleName;
                if (imp.boundImpl != null) {
                    if (!ifaceBpi.isInterface) {
                        indent(depth); System.err.printf(
                            "-- error: '%s' no es una interfaz; no admite ':%s' --%n",
                            joinPath(imp.path), imp.boundImpl);
                        continue;
                    }
                    Path implBpiPath = resolveImplBpi(imp.boundImpl, imp.fromPath, module, importerDir, ctx);
                    if (implBpiPath == null) {
                        indent(depth); System.err.printf(
                            "-- error: no se encuentra .bpi del impl '%s' --%n",
                            imp.boundImpl);
                        continue;
                    }
                    ModuleInterface implBpi = ModuleInterface.readFrom(implBpiPath);
                    if (implBpi.isInterface) {
                        indent(depth); System.err.printf(
                            "-- error: '%s' es una interfaz, no puede ser impl bindeado --%n",
                            imp.boundImpl);
                        continue;
                    }
                    // El impl puede implementar la interfaz pedida directamente
                    // o cualquier descendiente de ella (subinterfaz). Esto
                    // permite `import LogApi:RichLoggerV2` donde RichLoggerV2
                    // implementa LogApiV2 que extiende LogApi.
                    if (!implSatisfies(implBpi, joinPath(imp.path), importerSrc, ctx, depth)) {
                        indent(depth); System.err.printf(
                            "-- error: '%s' no implementa '%s' (directa o transitivamente; declara %s) --%n",
                            imp.boundImpl, joinPath(imp.path), implBpi.implementsName);
                        continue;
                    }
                    implLibrary = implBpi.library;
                    implModule  = implBpi.moduleName;
                    indent(depth); System.out.printf(
                            "-- vínculo: interfaz '%s' → impl '%s.%s' --%n",
                            joinPath(imp.path), implLibrary, implModule);
                } else if (ifaceBpi.isInterface) {
                    indent(depth); System.err.printf(
                        "-- error: '%s' es una interfaz; requiere `import %s:<Impl>` --%n",
                        joinPath(imp.path), alias);
                    continue;
                }

                // ---- 3) Construir el ImportedNamespaceSymbol ----
                String fromPath = (imp.fromPath == null) ? "" : imp.fromPath;
                Symbol.ImportedNamespaceSymbol ns =
                        new Symbol.ImportedNamespaceSymbol(alias, implLibrary, implModule, fromPath);
                for (ModuleInterface.FuncSig fs : ifaceBpi.functions) {
                    Symbol.FunctionSymbol f =
                            new Symbol.FunctionSymbol(fs.name, true, false, fs.isStatic, null, null);
                    f.returnType = fs.returnType;
                    f.isExternal = true;
                    f.isIntrinsic = fs.isIntrinsic;
                    f.externalLibrary = implLibrary;
                    f.externalModule  = implModule;
                    f.externalFromPath = fromPath;
                    for (ModuleInterface.ParamSig ps : fs.params) {
                        Symbol.ParamSymbol psym = new Symbol.ParamSymbol(ps.name, 0, 0);
                        psym.type = ps.type;
                        f.params.add(psym);
                    }
                    ns.functions.put(fs.name, f);
                }
                for (ModuleInterface.ConstSig cs : ifaceBpi.consts) {
                    Symbol.ConstSymbol c =
                            new Symbol.ConstSymbol(cs.name, true, false, null, 0, 0);
                    c.type = cs.type;
                    c.literalValue = cs.value;
                    ns.consts.put(cs.name, c);
                }
                for (ModuleInterface.EnumSig es : ifaceBpi.enums) {
                    Symbol.EnumSymbol e =
                            new Symbol.EnumSymbol(es.name, true, 0, 0);
                    e.values.putAll(es.values);
                    ns.enums.put(es.name, e);
                }
                for (ModuleInterface.PropSig ps : ifaceBpi.properties) {
                    // Construimos un PropertySymbol "shell" sin AST. Lo marcamos
                    // external para que el emitter sepa que el accesor está en
                    // otro módulo y emita CALL_EXT en vez de CALL local.
                    Symbol.PropertySymbol p =
                            new Symbol.PropertySymbol(ps.name, true, false, false, null, null);
                    p.type = ps.type;
                    p.isExternal = true;
                    p.externalLibrary = implLibrary;
                    p.externalModule  = implModule;
                    p.externalFromPath = fromPath;
                    ns.properties.put(ps.name, p);
                }
                // L2: clases públicas exportadas. Construimos ClassSymbol stub
                // con methods/properties marcados isExternal. Slots precalculados
                // según el ORDEN de declaración en el .bpi (properties primero —
                // cada una añade getter+setter a la vtable — luego métodos).
                for (ModuleInterface.ClassSig cs : ifaceBpi.classes) {
                    Symbol.ClassSymbol stub = new Symbol.ClassSymbol(
                            cs.name, true, cs.baseClassName, null, 0, 0);
                    stub.isExternal = true;
                    stub.externalLibrary = implLibrary;
                    stub.externalModule  = implModule;
                    stub.externalFromPath = fromPath;
                    // L2 v3 — propaga layout binario para que subclasses
                    // cross-module puedan reservar slots/fields correctamente.
                    if (cs.binaryNumFields >= 0) {
                        stub.binaryLayout = new SemanticInfo.ClassBinaryLayout(
                                cs.binaryNumFields, cs.binaryNumMethods,
                                cs.binaryFieldBitmap, cs.binaryOwnerBitmap);
                    }
                    int nextSlot = 0;
                    // Properties → getter (slot N) + setter (slot N+1)
                    for (ModuleInterface.PropSig p : cs.properties) {
                        Symbol.PropertySymbol psym = new Symbol.PropertySymbol(
                                p.name, true, false, false, stub, null);
                        psym.type = p.type;
                        stub.instanceMembers.tryDefine(psym);
                        String capName = Character.toUpperCase(p.name.charAt(0)) + p.name.substring(1);
                        stub.externalMethodSlots.put("get" + capName, nextSlot++);
                        stub.externalMethodSlots.put("set" + capName, nextSlot++);
                    }
                    // Methods en orden de declaración
                    for (ModuleInterface.FuncSig m : cs.methods) {
                        Symbol.FunctionSymbol fsym = new Symbol.FunctionSymbol(
                                m.name, true, false, false, stub, null);
                        fsym.returnType = m.returnType;
                        for (ModuleInterface.ParamSig pp : m.params) {
                            Symbol.ParamSymbol psm = new Symbol.ParamSymbol(pp.name, 0, 0);
                            psm.type = pp.type;
                            fsym.params.add(psm);
                        }
                        // isExternal queda false porque INVOKE_VIRTUAL despacha
                        // via vtable, no via CALL_EXT. Lo que sí necesita el
                        // emisor es el slot — está en externalMethodSlots.
                        stub.instanceMembers.tryDefine(fsym);
                        stub.externalMethodSlots.put(m.name, nextSlot++);
                    }
                    // L2 v3.d — static consts públicos del .bpi. Se añaden al
                    // staticMembers del stub con literalValue para que el
                    // emisor los inlinee en el call-site (mismo path que
                    // consts cross-module a nivel módulo).
                    for (ModuleInterface.ConstSig sc : cs.staticConsts) {
                        Symbol.ConstSymbol cst = new Symbol.ConstSymbol(
                                sc.name, true, true, stub, 0, 0);   // isStatic=true
                        cst.type = sc.type;
                        cst.literalValue = sc.value;
                        stub.staticMembers.tryDefine(cst);
                    }

                    // Constructor: como FunctionSymbol cross-module. Lo expone-
                    // mos en realidad vía la factoría `__cls_new_<Cls>`, pero
                    // para el typecheck necesitamos el FunctionSymbol del ctor
                    // con los params correctos. El emisor cuando ve construc-
                    // ción de clase externa cambia el CALL al de la factoría.
                    if (cs.ctorParams != null) {
                        Symbol.FunctionSymbol ctor = new Symbol.FunctionSymbol(
                                cs.name, true, false, false, stub, null);
                        ctor.isConstructor = true;
                        for (ModuleInterface.ParamSig pp : cs.ctorParams) {
                            Symbol.ParamSymbol psm = new Symbol.ParamSymbol(pp.name, 0, 0);
                            psm.type = pp.type;
                            ctor.params.add(psm);
                        }
                        stub.constructor = ctor;
                        stub.instanceMembers.tryDefine(ctor);
                    }
                    // Resolver UnresolvedClassRef en params/return de métodos
                    // y del ctor, contra el propio stub o contra otros stubs
                    // ya añadidos al ns. Lo hacemos AL FINAL del bucle de
                    // clases para que todos los stubs estén creados.
                    ns.classes.put(cs.name, stub);
                }
                // 2nd pass: resolver UnresolvedClassRef contra stubs creados.
                for (Symbol.ClassSymbol stub : ns.classes.values()) {
                    for (Symbol mem : stub.instanceMembers.getSymbols()) {
                        if (mem instanceof Symbol.FunctionSymbol) {
                            Symbol.FunctionSymbol f = (Symbol.FunctionSymbol) mem;
                            f.returnType = resolveTypeAgainst(f.returnType, ns);
                            for (Symbol.ParamSymbol p : f.params)
                                p.type = resolveTypeAgainst(p.type, ns);
                        } else if (mem instanceof Symbol.PropertySymbol) {
                            Symbol.PropertySymbol p = (Symbol.PropertySymbol) mem;
                            p.type = resolveTypeAgainst(p.type, ns);
                        }
                    }
                }
                analyzer.registerImport(ns);
                loadedNs.add(ns);
                indent(depth); System.out.printf("-- cargado import '%s' desde %s (funcs=%d, consts=%d, enums=%d, props=%d, classes=%d) --%n",
                        alias, bpi.getFileName(),
                        ns.functions.size(), ns.consts.size(), ns.enums.size(),
                        ns.properties.size(), ns.classes.size());
            } catch (IOException ex) {
                System.err.println("error leyendo " + bpi + ": " + ex.getMessage());
            }
        }

        // L2 v3.e — post-pass: resuelve UnresolvedClassRef con nombre dotted
        // (e.g. `L2Lib.Counter`) contra otros namespaces ya cargados. Hace
        // esta pasada DESPUÉS del loop principal para que todos los ns estén
        // disponibles. Sólo toca refs que no se resolvieron contra el propio
        // ns en el primer pass.
        for (Symbol.ImportedNamespaceSymbol ns : loadedNs) {
            for (Symbol.FunctionSymbol fn : ns.functions.values()) {
                fn.returnType = resolveCrossModuleType(fn.returnType, loadedNs);
                for (Symbol.ParamSymbol p : fn.params) {
                    p.type = resolveCrossModuleType(p.type, loadedNs);
                }
            }
            for (Symbol.ClassSymbol stub : ns.classes.values()) {
                for (Symbol mem : stub.instanceMembers.getSymbols()) {
                    if (mem instanceof Symbol.FunctionSymbol) {
                        Symbol.FunctionSymbol f = (Symbol.FunctionSymbol) mem;
                        f.returnType = resolveCrossModuleType(f.returnType, loadedNs);
                        for (Symbol.ParamSymbol p : f.params)
                            p.type = resolveCrossModuleType(p.type, loadedNs);
                    } else if (mem instanceof Symbol.PropertySymbol) {
                        Symbol.PropertySymbol p = (Symbol.PropertySymbol) mem;
                        p.type = resolveCrossModuleType(p.type, loadedNs);
                    }
                }
            }
        }
    }

    /** L2 v3.e — resuelve un UnresolvedClassRef contra TODOS los namespaces
     *  importados. Si el nombre tiene puntos (`L2Lib.Counter`), separa
     *  módulo + clase y busca en `ns.classes`. Devuelve el tipo original si
     *  no se encuentra (deja que el typecheck lo marque). */
    private static basicplus.frontend.BpType resolveCrossModuleType(
            basicplus.frontend.BpType t,
            java.util.List<Symbol.ImportedNamespaceSymbol> allNs) {
        if (!(t instanceof basicplus.frontend.BpType.UnresolvedClassRef)) return t;
        String name = ((basicplus.frontend.BpType.UnresolvedClassRef) t).name;
        int lastDot = name.lastIndexOf('.');
        if (lastDot > 0) {
            String modPart = name.substring(0, lastDot);
            String clsPart = name.substring(lastDot + 1);
            for (Symbol.ImportedNamespaceSymbol candidate : allNs) {
                String full = candidate.library.isEmpty()
                        ? candidate.moduleName
                        : candidate.library + "." + candidate.moduleName;
                if (full.equals(modPart) || candidate.moduleName.equals(modPart)) {
                    Symbol.ClassSymbol cls = candidate.classes.get(clsPart);
                    if (cls != null) return new basicplus.frontend.BpType.ClassType(cls);
                }
            }
            return t;   // no se encontró
        }
        // Sin punto: busca en cualquier ns que exponga esa clase.
        for (Symbol.ImportedNamespaceSymbol candidate : allNs) {
            Symbol.ClassSymbol cls = candidate.classes.get(name);
            if (cls != null) return new basicplus.frontend.BpType.ClassType(cls);
        }
        return t;
    }

    private static Path locateImportBpi(Ast.ImportNode imp, String bpiName, String library, String alias,
                                        Path importerDir, Ctx ctx) {
        // En `import Iface:Impl from "..."` el fromPath es del impl, no de
        // la interfaz: lo ignoramos al resolver la .bpi de la interfaz.
        String effectiveFromPath = (imp.boundImpl != null) ? null : imp.fromPath;
        if (effectiveFromPath != null && !effectiveFromPath.isEmpty()) {
            String fp = effectiveFromPath;
            if (fp.endsWith(".mod")) fp = fp.substring(0, fp.length() - 4) + ".bpi";
            Path direct = importerDir.resolve(fp).toAbsolutePath().normalize();
            if (Files.exists(direct)) return direct;
        }
        Path candidate = ctx.outDir.resolve(bpiName);
        if (Files.exists(candidate)) return candidate;
        Path sib = importerDir.resolve(bpiName);
        if (Files.exists(sib)) return sib;
        // N10.build: si estamos en modo proyecto, probar las dependencies.
        //   - dir: dir/<bpiName>
        //   - fichero .bpi exacto: usar tal cual si su basename coincide.
        //   - fichero .mod: el .bpi asociado al lado.
        for (Path dep : ctx.dependencyPaths) {
            if (Files.isDirectory(dep)) {
                Path inDir = dep.resolve(bpiName);
                if (Files.exists(inDir)) return inDir;
            } else if (Files.isRegularFile(dep)) {
                String depName = dep.getFileName().toString();
                if (depName.equals(bpiName)) return dep;
                // ¿Una entry .mod que se corresponde con este bpi?
                if (depName.endsWith(".mod")) {
                    String stem = depName.substring(0, depName.length() - 4);
                    if ((stem + ".bpi").equals(bpiName)) {
                        Path bpiNext = dep.resolveSibling(bpiName);
                        if (Files.exists(bpiNext)) return bpiNext;
                    }
                }
            }
        }
        return null;
    }

    /**
     * Si {@code t} es UnresolvedClassRef, intenta resolverlo contra los
     * stubs de clase del namespace. Devuelve el tipo resuelto o el
     * original si no se encuentra (deja que el typecheck lo marque).
     */
    private static basicplus.frontend.BpType resolveTypeAgainst(
            basicplus.frontend.BpType t, Symbol.ImportedNamespaceSymbol ns) {
        if (!(t instanceof basicplus.frontend.BpType.UnresolvedClassRef)) return t;
        String name = ((basicplus.frontend.BpType.UnresolvedClassRef) t).name;
        Symbol.ClassSymbol cls = ns.classes.get(name);
        if (cls != null) return new basicplus.frontend.BpType.ClassType(cls);
        return t;
    }

    private static Path findExistingBpi(String bpiName, Path importerDir, Path outDir) {
        Path inOut = outDir.resolve(bpiName);
        if (Files.exists(inOut)) return inOut;
        Path sib = importerDir.resolve(bpiName);
        if (Files.exists(sib)) return sib;
        return null;
    }

    /**
     * Garantiza que la .bpi del impl bindeado existe. Si no, intenta
     * localizar su .bp fuente y compilar en INTERFACE_ONLY. Acepta un
     * fromPath opcional (el del `import I:M from "..."`): si está, se
     * derivan rutas relativas a él para .bpi y .bp.
     */
    private static void ensureInterfaceForBoundImpl(String boundImpl, String fromPath,
                                                    Ast.ModuleNode importerModule,
                                                    Path importerSrc, Ctx ctx, int depth) throws IOException {
        Path importerDir = importerSrc.toAbsolutePath().getParent();
        Path bpiPath = resolveImplBpi(boundImpl, fromPath, importerModule, importerDir, ctx);
        if (bpiPath != null) return;
        Path bpSrc = locateImplBpSource(boundImpl, fromPath, importerDir, ctx);
        if (bpSrc == null) {
            indent(depth); System.out.printf("-- no se localizó .bp ni .bpi para impl '%s' --%n", boundImpl);
            return;
        }
        compileInterface(bpSrc, ctx, depth);
    }

    /**
     * Garantiza que el .mod del impl bindeado existe. Si no, localiza su .bp
     * y compila en FULL. fromPath, si está, ayuda a localizar .bpi/.bp.
     */
    private static void ensureFullModForBoundImpl(String boundImpl, String fromPath,
                                                  Ast.ModuleNode importerModule,
                                                  Path importerSrc, Ctx ctx, int depth) throws IOException {
        Path importerDir = importerSrc.toAbsolutePath().getParent();
        Path bpiPath = resolveImplBpi(boundImpl, fromPath, importerModule, importerDir, ctx);
        String modName;
        if (bpiPath != null) {
            ModuleInterface bpi = ModuleInterface.readFrom(bpiPath);
            modName = bpi.library.isEmpty()
                    ? bpi.moduleName + ".mod"
                    : bpi.library + "." + bpi.moduleName + ".mod";
        } else {
            modName = boundImpl + ".mod";
        }

        Path bpSrc = locateImplBpSource(boundImpl, fromPath, importerDir, ctx);

        // .mod en outDir (donde el compilador siempre escribe).
        Path modPath = ctx.outDir.resolve(modName);
        if (Files.exists(modPath)) {
            if (!isStale(modPath, bpSrc)) return;
            indent(depth); System.out.printf("-- .mod del impl obsoleto (%s); regenerando --%n", modPath.getFileName());
        }
        // Si el fromPath apunta a un .mod en otra ubicación que ya exista y esté
        // fresco, también lo aceptamos (el usuario lo gestiona manualmente).
        if (fromPath != null && !fromPath.isEmpty()) {
            Path runtimeMod = importerDir.resolve(fromPath).toAbsolutePath().normalize();
            if (Files.exists(runtimeMod) && !isStale(runtimeMod, bpSrc)) return;
        }

        if (bpSrc == null) {
            indent(depth); System.out.printf("-- no se localizó .bp para impl '%s' --%n", boundImpl);
            return;
        }
        compileFull(bpSrc, ctx, depth);
    }

    /**
     * Busca la .bp fuente del impl bindeado. Prioridad:
     *   0) fromPath con .mod → .bp, relativo al importer.
     *   1) Mapa escaneado en ctx.rootSrcDir (cualified o simple).
     */
    private static Path locateImplBpSource(String boundImpl, String fromPath,
                                           Path importerDir, Ctx ctx) throws IOException {
        if (fromPath != null && !fromPath.isEmpty()) {
            String fp = fromPath;
            if (fp.endsWith(".mod")) fp = fp.substring(0, fp.length() - 4) + ".bp";
            Path direct = importerDir.resolve(fp).toAbsolutePath().normalize();
            if (Files.exists(direct)) return direct;
        }
        String[] segs = boundImpl.split("\\.");
        String simple = segs[segs.length - 1];
        if (ctx.bpSources == null) ctx.bpSources = scanBpSources(ctx.rootSrcDir);
        Path bp = ctx.bpSources.get(boundImpl);
        if (bp == null) bp = ctx.bpSources.get(simple);
        return bp;
    }

    /**
     * Resuelve la ruta de la .bpi del módulo impl para un `import Iface:Impl`.
     * El boundImpl puede ser un nombre simple ("ConsoleLogger") o un path
     * cualificado ("com.example.ConsoleLogger"). Estrategia:
     *   0) Si hay `fromPath` (ruta al .mod del impl en runtime), buscamos la
     *      .bpi adyacente (cambiando .mod → .bpi) relativa al importer.
     *   1) Si boundImpl es cualificado: derivar el nombre canónico del .bpi.
     *   2) Si es simple: probar con la library del importer y, si no, escanear
     *      outDir buscando un .bpi cuyo módulo coincida.
     */
    private static Path resolveImplBpi(String boundImpl, String fromPath,
                                       Ast.ModuleNode importerModule,
                                       Path importerDir, Ctx ctx) throws IOException {
        // (0) fromPath con extensión .mod → derivamos .bpi
        if (fromPath != null && !fromPath.isEmpty()) {
            String fp = fromPath;
            if (fp.endsWith(".mod")) fp = fp.substring(0, fp.length() - 4) + ".bpi";
            Path direct = importerDir.resolve(fp).toAbsolutePath().normalize();
            if (Files.exists(direct)) return direct;
        }
        if (boundImpl.contains(".")) {
            int lastDot = boundImpl.lastIndexOf('.');
            String lib = boundImpl.substring(0, lastDot);
            String mod = boundImpl.substring(lastDot + 1);
            String bpiName = lib + "." + mod + ".bpi";
            return findExistingBpi(bpiName, importerDir, ctx.outDir);
        }
        // Simple name: prueba con la library del importer.
        String importerLib = (importerModule.library == null) ? "" : importerModule.library;
        String firstTry = importerLib.isEmpty() ? boundImpl + ".bpi" : importerLib + "." + boundImpl + ".bpi";
        Path direct = findExistingBpi(firstTry, importerDir, ctx.outDir);
        if (direct != null) return direct;
        // Búsqueda amplia: cualquier <something>.<boundImpl>.bpi o <boundImpl>.bpi en outDir.
        try (DirectoryStream<Path> ds = Files.newDirectoryStream(ctx.outDir, "*.bpi")) {
            for (Path p : ds) {
                String fn = p.getFileName().toString();
                if (fn.equals(boundImpl + ".bpi")
                        || fn.endsWith("." + boundImpl + ".bpi")) {
                    return p;
                }
            }
        }
        return null;
    }

    /**
     * Localiza el .bp fuente de un import. Prioridad:
     *   1) `imp.fromPath` con .mod reemplazado por .bp (relativo al directorio del importer).
     *   2) Mismo directorio del importer: `<lib>.<module>.bp` o `<module>.bp` (case-sensitive).
     *   3) Escaneo del directorio raíz del compilado: parsea cabeceras y busca match
     *      por qualified name "<library>.<Module>" o por "<Module>" suelto.
     */
    private static Path locateBpSource(String qualifiedName, String library, String moduleName,
                                       String fromPath, Path importerSrc, Ctx ctx) throws IOException {
        Path importerDir = importerSrc.toAbsolutePath().getParent();

        if (fromPath != null && !fromPath.isEmpty()) {
            String fp = fromPath;
            if (fp.endsWith(".mod")) fp = fp.substring(0, fp.length() - 4) + ".bp";
            Path direct = importerDir.resolve(fp);
            if (Files.exists(direct)) return direct;
        }

        if (importerDir != null) {
            Path[] tries = new Path[] {
                    importerDir.resolve((library.isEmpty() ? moduleName : library + "." + moduleName) + ".bp"),
                    importerDir.resolve(moduleName + ".bp"),
                    importerDir.resolve(moduleName.toLowerCase() + ".bp")
            };
            for (Path c : tries) if (Files.exists(c)) return c;
        }

        if (ctx.bpSources == null) {
            ctx.bpSources = scanBpSources(ctx.rootSrcDir);
        }
        Path byQualified = ctx.bpSources.get(qualifiedName);
        if (byQualified != null) return byQualified;
        return ctx.bpSources.get(moduleName);  // fallback por nombre simple
    }

    /**
     * Escanea el directorio buscando *.bp y peek a su cabecera para extraer
     * library/module. Devuelve un mapa "<library>.<Module>" → Path (más una
     * entrada por nombre simple "<Module>" como fallback).
     */
    private static Map<String, Path> scanBpSources(Path dir) throws IOException {
        Map<String, Path> result = new HashMap<>();
        if (dir == null || !Files.isDirectory(dir)) return result;
        try (DirectoryStream<Path> ds = Files.newDirectoryStream(dir, "*.bp")) {
            for (Path p : ds) {
                try {
                    String lib = "";
                    String mod = "";
                    for (String line : Files.readAllLines(p, StandardCharsets.UTF_8)) {
                        String t = line.trim();
                        if (t.isEmpty() || t.startsWith("//")) continue;
                        if (t.startsWith("library ")) {
                            int q1 = t.indexOf('"');
                            int q2 = (q1 >= 0) ? t.indexOf('"', q1 + 1) : -1;
                            if (q1 >= 0 && q2 > q1) lib = t.substring(q1 + 1, q2);
                        } else if (t.startsWith("module ")) {
                            String[] parts = t.split("\\s+");
                            if (parts.length >= 2) mod = parts[1];
                            break;  // módulo declarado: terminar peek
                        }
                    }
                    if (!mod.isEmpty()) {
                        String key = lib.isEmpty() ? mod : lib + "." + mod;
                        result.putIfAbsent(key, p);
                        result.putIfAbsent(mod, p);
                    }
                } catch (IOException ignored) { }
            }
        }
        return result;
    }

    // ============================================================
    // LEX + PARSE + PRINTS
    // ============================================================
    private static final class Parsed {
        Lexer lexer;
        Parser parser;
        Ast.ModuleNode module;
    }

    private static Parsed parseAndPrint(Path src, Ctx ctx, boolean isRoot) {
        String source;
        try {
            source = new String(Files.readAllBytes(src), StandardCharsets.UTF_8);
        } catch (IOException ex) {
            System.err.println("Error leyendo el archivo: " + ex.getMessage());
            return null;
        }
        if (isRoot && ctx.verbose) {
            System.out.printf("=== Procesando: %s (%d caracteres) ===%n", src, source.length());
            System.out.println();
        }

        Parsed r = new Parsed();
        r.lexer = new Lexer(source);
        List<Token> tokens = r.lexer.tokenize();

        if (isRoot && ctx.showTokens) {
            System.out.printf("-- Tokens (%d) --%n", tokens.size());
            System.out.printf("%-9s  %-12s  lexema  =>  valor%n", "line:col", "TYPE");
            System.out.println(repeat('-', 70));
            for (Token t : tokens) System.out.println(t);
            System.out.println();
        }
        if (!r.lexer.getErrors().isEmpty()) {
            System.out.printf("-- Errores léxicos en %s (%d) --%n", src.getFileName(), r.lexer.getErrors().size());
            for (LexerError e : r.lexer.getErrors()) System.out.println(e);
        }

        r.parser = new Parser(tokens);
        r.module = r.parser.parseModule();

        if (isRoot && ctx.showAst) {
            if (r.module != null) {
                System.out.println("-- AST --");
                System.out.print(AstPrinter.print(r.module));
                System.out.println();
            }
        }
        if (!r.parser.getErrors().isEmpty()) {
            System.out.printf("-- Errores sintácticos en %s (%d) --%n", src.getFileName(), r.parser.getErrors().size());
            for (ParserError e : r.parser.getErrors()) System.out.println(e);
        }
        return r;
    }

    private static void printSemantics(SemanticInfo info, Ast.ModuleNode module) {
        if (info == null) return;
        int errs = 0, warns = 0;
        for (SemanticDiagnostic d : info.diagnostics) {
            if (d.kind == SemanticDiagnostic.Kind.ERROR) errs++; else warns++;
        }
        if (info.diagnostics.isEmpty()) {
            System.out.println("-- Sin diagnósticos semánticos --");
        } else {
            System.out.printf("-- Diagnósticos semánticos (%d errores, %d avisos) --%n", errs, warns);
            for (SemanticDiagnostic d : info.diagnostics) System.out.println(d);
        }
        if (info.module != null) {
            Symbol.ModuleSymbol m = info.module;
            int classes = 0, enums = 0, funcs = 0, vars = 0, consts = 0, props = 0;
            for (Symbol s : m.members.getSymbols()) {
                if      (s instanceof Symbol.ClassSymbol)    classes++;
                else if (s instanceof Symbol.EnumSymbol)     enums++;
                else if (s instanceof Symbol.FunctionSymbol) funcs++;
                else if (s instanceof Symbol.VarSymbol)      vars++;
                else if (s instanceof Symbol.ConstSymbol)    consts++;
                else if (s instanceof Symbol.PropertySymbol) props++;
            }
            System.out.printf("-- Resumen módulo '%s' --%n", m.name);
            System.out.printf("  classes=%d  enums=%d  funcs=%d  vars=%d  consts=%d  props=%d%n",
                    classes, enums, funcs, vars, consts, props);
            System.out.printf("  initializer=%s  Main=%s%n",
                    m.initializer == null ? "no" : "sí",
                    m.mainFunction == null ? "no" : "sí");
        }
    }

    private static int countSemErrors(SemanticInfo info) {
        if (info == null) return 0;
        int n = 0;
        for (SemanticDiagnostic d : info.diagnostics)
            if (d.kind == SemanticDiagnostic.Kind.ERROR) n++;
        return n;
    }

    // ============================================================
    // UTILS
    // ============================================================
    private static String joinPath(List<String> path) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < path.size(); i++) {
            if (i > 0) sb.append('.');
            sb.append(path.get(i));
        }
        return sb.toString();
    }

    private static String libraryFromImportPath(Ast.ImportNode imp) {
        if (imp.path.size() < 2) return "";
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < imp.path.size() - 1; i++) {
            if (i > 0) sb.append('.');
            sb.append(imp.path.get(i));
        }
        return sb.toString();
    }

    private static void indent(int depth) {
        for (int i = 0; i < depth; i++) System.out.print("  ");
    }

    private static String repeat(char c, int n) {
        char[] buf = new char[n];
        for (int i = 0; i < n; i++) buf[i] = c;
        return new String(buf);
    }
}
