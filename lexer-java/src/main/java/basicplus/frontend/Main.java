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

        /** H6.a — caché de interfaces EN MEMORIA, clave = nombre canónico del
         *  artefacto ("lib.Module" o "Module"). Es el almacén del build
         *  recursivo: se rellena al construir cada interfaz (writeInterfaceFile)
         *  y se lee al cargar imports (readInterfaceCached), en vez de ir a
         *  disco. Sustituye al .bpi como vía de paso de interfaces DENTRO de un
         *  build. Se descarta con el Ctx (uso acotado al cierre transitivo). */
        final Map<String, ModuleInterface> interfaceCache = new java.util.LinkedHashMap<>();

        /** Mapa de "<library>.<Module>" → ruta del .bp. Construido lazy. */
        Map<String, Path> bpSources;
        Path rootSrcDir;

        /** H6.a-lite — .bpi que ESTE build ha ESCRITO (en outDir), para poder
         *  borrarlos al terminar. Sólo entra lo que escribimos: los .bpi de
         *  librerías precompiladas (stdlib) se LEEN de libPaths, no se escriben,
         *  así que nunca aparecen aquí — están a salvo. */
        final Set<Path> writtenBpi = new LinkedHashSet<>();
        /** Si true, al terminar un build se borran los writtenBpi. El IDE lo
         *  activa (un build de app: el .bpi ya cumplió su función, el .mod es el
         *  entregable). La regeneración manual de librerías lo deja en false,
         *  para no borrarse sus propios .bpi. Default OFF = comportamiento previo. */
        boolean pruneBpi = false;

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
        return compileFile(source, outDir, backend, false);
    }

    /**
     * @param pruneBpi si true, al terminar un build correcto borra los .bpi que
     *   este build ESCRIBIÓ (raíz + imports de la app). El .bpi es un artefacto
     *   de compilación: una vez emitido el .mod, ya cumplió su función (resolver
     *   imports). Borrarlos evita que queden rondando — desfases .bpi/.mod y
     *   subidas innecesarias al runtime (el IDE subía 407 ficheros para un hola
     *   mundo). Los .bpi de la stdlib NO se tocan: se leen de su libdir, no se
     *   escriben. Lo activa el IDE; la regen manual de librerías lo deja false.
     */
    public static boolean compileFile(Path source, Path outDir, String backend, boolean pruneBpi) {
        Ctx ctx = new Ctx();
        ctx.outDir = outDir;
        ctx.backend = (backend != null) ? backend : "mivm";
        ctx.pruneBpi = pruneBpi;
        // Si el cwd no es el del repo (típico cuando un IDE invoca el compile)
        // el ctor de Ctx no habrá localizado BpVM.cfg. Re-intentamos caminando
        // hacia arriba desde el .bp para que `import IO` y resto de stdlib
        // resuelvan sin tener que configurar dependencyPaths a mano.
        if (source != null) ctx.autodiscoverFromSource(source.toAbsolutePath());
        try {
            boolean ok = compileFull(source, ctx, 0);
            boolean success = ok && ctx.totalErrors == 0;
            if (success && ctx.pruneBpi) pruneWrittenBpi(ctx);
            return success;
        } catch (IOException ex) {
            System.err.println("compileFile: " + ex.getClass().getSimpleName() + ": " + ex.getMessage());
            return false;
        }
    }

    /**
     * Construye un PROYECTO entero: compila el `main` + sus dependencias
     * transitivas al `outDir` y, si el proyecto pide `out:pack`, empaqueta los
     * .mod/.mdn del outDir + resources en un pack (ejecutable si hay `main`).
     * REUTILIZADO por la CLI (--project) y por el IDE → un solo camino de build.
     * Devuelve true si compiló sin errores (y empaquetó, si tocaba). Lo impreso va
     * por stdout/stderr (el IDE lo captura a su consola). Lanza IOException si algo
     * de E/S (crear outDir, escribir el pack) falla → lo maneja el llamador.
     */
    public static boolean buildProject(BpBuild proj, String backend, boolean pruneBpi)
            throws IOException {
        System.out.println("project: " + proj.sourcePath
                + " (main=" + proj.main
                + ", sourceDir=" + proj.sourceDir
                + ", outDir=" + proj.outDir
                + ", dependencies=" + proj.dependencies + ")");
        Path mainBp = Paths.get(proj.sourceDir, proj.main + ".bp")
                .toAbsolutePath().normalize();
        if (!Files.exists(mainBp)) {
            System.err.println("no se encuentra el fichero del main: " + mainBp);
            return false;
        }
        Ctx ctx = new Ctx();
        ctx.backend    = backend;
        ctx.showTokens = false;
        ctx.showAst    = false;
        ctx.rootSrcDir = Paths.get(proj.sourceDir);
        ctx.outDir     = Paths.get(proj.outDir);
        Files.createDirectories(ctx.outDir);
        for (String d : proj.dependencies) ctx.dependencyPaths.add(Paths.get(d));
        ctx.pruneBpi = pruneBpi;
        boolean ok = compileFull(mainBp, ctx, /*depth*/0);
        boolean success = ok && ctx.totalErrors == 0;
        if (success && ctx.pruneBpi) pruneWrittenBpi(ctx);
        // H3 Packs: tras el build correcto, si out:pack empaqueta el proyecto
        // (el manifest reusa `main` → pack ejecutable). IOException la ve el caller.
        if (success && "pack".equals(proj.out)) {
            Path packPath = PackStep.buildPack(proj);
            System.out.println("pack generado: " + packPath);
        }
        return success;
    }

    /** H6.a-lite — borra los .bpi que este build escribió. Sólo los nuestros:
     *  la stdlib se lee, no se escribe, así que no está en writtenBpi. Fallar un
     *  borrado no es fatal (el .mod ya está emitido). */
    private static void pruneWrittenBpi(Ctx ctx) {
        int n = 0;
        for (Path bpi : ctx.writtenBpi) {
            try { if (Files.deleteIfExists(bpi)) n++; } catch (IOException ignore) { }
        }
        if (ctx.verbose && n > 0) System.out.println("  .bpi intermedios borrados: " + n);
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
        boolean pruneBpi   = false;   // borra los .bpi del build al terminar (opt-in)

        for (int i = 0; i < args.length; i++) {
            String a = args[i];
            if ("--tokens".equals(a))      showAst    = false;
            else if ("--prune-bpi".equals(a)) pruneBpi = true;
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
                // H3: el build de proyecto (compila + out:pack) vive en buildProject,
                // reutilizado por el IDE → un solo camino de build.
                System.exit(buildProject(proj, backend, pruneBpi) ? 0 : 2);
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
                ctx.pruneBpi = pruneBpi;
                boolean ok = compileFull(p, ctx, /*depth*/0);
                boolean success = ok && ctx.totalErrors == 0;
                if (success && ctx.pruneBpi) pruneWrittenBpi(ctx);
                System.exit(success ? 0 : 2);
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
                // N-parse-recover-semantic — el parser se recupera a nivel
                // sentencia/función (synchronize / synchronizeToFunctionEnd),
                // así que tras un error de parseo suele quedar un AST
                // utilizable. Corremos el análisis semántico sobre él PARA
                // REPORTAR MÁS ERRORES REALES (p.ej. una variable no declarada
                // en una función que sí parseó), en vez de ocultarlos
                // abortando. El error de sintaxis queda contenido en su
                // función por el recovery; el resto del módulo se analiza.
                // NUNCA generamos .mod tras un error de parseo. Protegido: un
                // AST parcial podría hacer petar al analyzer o a la carga de
                // imports → si algo falla, nos quedamos con los de parseo.
                if (parsed.module != null) {
                    try {
                        SemanticAnalyzer recAnalyzer = new SemanticAnalyzer();
                        injectImplicitCoreImport(parsed.module);
                        if (parsed.module.imports != null) {
                            for (Ast.ImportNode imp : parsed.module.imports) {
                                try { ensureInterfaceForImport(imp, src, ctx, depth + 1); }
                                catch (Exception ignored) { /* import irresoluble: lo marcará el análisis */ }
                            }
                        }
                        loadImportsForAnalyzer(parsed.module, src, ctx, recAnalyzer, depth + 1);
                        SemanticInfo recInfo = recAnalyzer.analyze(parsed.module);
                        if (depth == 0) printSemantics(recInfo, parsed.module);
                        ctx.totalErrors += countSemErrors(recInfo);
                    } catch (RuntimeException ignored) {
                        // AST demasiado roto para un análisis fiable: bastan
                        // los errores de parseo ya reportados.
                    }
                }
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
            int impErrsBefore = ctx.totalErrors;
            injectImplicitCoreImport(module);   // #248 — lazy: solo si usa excepciones
            loadImportsForAnalyzer(module, src, ctx, analyzer, depth + 1);
            // N6 — un import incompatible/erróneo (interfaz no satisfecha, impl
            // ausente, binding mal usado) es FATAL: no producimos un .mod roto.
            if (ctx.totalErrors > impErrsBefore) {
                indent(depth); System.err.println("compilación abortada por imports incompatibles en " + srcAbs.getFileName());
                return false;
            }

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
                try {
                    emitJvmClass(module, info, ctx, depth);
                } catch (RuntimeException jvmEx) {
                    // #248 — el backend JVM (legacy, el por defecto sin flag) no
                    // conoce L2 v3 (clases/super cross-module) y petaba con NPE.
                    // Error claro en vez de stack trace: el producto dual-VM es
                    // el backend mivm.
                    indent(depth); System.err.println(
                        "error: el backend 'jvm' (legacy) no soporta este módulo ("
                        + jvmEx.getClass().getSimpleName()
                        + (jvmEx.getMessage() != null ? ": " + jvmEx.getMessage() : "")
                        + ") — compila con --backend=mivm");
                    ctx.totalErrors++;
                    return false;
                }
                return true;
            }
            writeInterfaceFile(module, info, ctx, depth);

            // H13.1 (4.0) — sidecar <Módulo>.slots (JSON método→slot de cada clase
            // pública) para que el IDE HORNEE el slot del handler en el .win (Forms
            // Camino A). Sólo el módulo raíz: es el que compila el IDE.
            if (depth == 0) writeSlotsFile(module, info, ctx, depth);

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

    /**
     * H13.1 (4.0) — emite {@code <Módulo>.slots}: JSON con el método→slot de cada
     * CLASE PÚBLICA del módulo (sólo métodos de usuario públicos de instancia: los
     * handlers de Forms). El IDE lo lee para HORNEAR el slot en el .win (Camino A):
     * el .win nombra el evento ("clic":"onOk"); el IDE resuelve onOk→slot con esto
     * —autoritativo: {@link Symbol.ClassSymbol#slotOf}, el MISMO cómputo que emite
     * INVOKE_VIRTUAL— y escribe "clicSlot":N. No se emite si el módulo no tiene
     * clases públicas. Formato: {@code { "MiForm": { "onOk": 30, "onCancel": 31 } }}
     */
    private static void writeSlotsFile(Ast.ModuleNode module, SemanticInfo info, Ctx ctx, int depth) throws IOException {
        if (info.module == null) return;
        StringBuilder body = new StringBuilder();
        boolean anyClass = false;
        for (Symbol s : info.module.members.getSymbols()) {
            if (!(s instanceof Symbol.ClassSymbol)) continue;
            Symbol.ClassSymbol cls = (Symbol.ClassSymbol) s;
            if (!cls.isPublic) continue;
            StringBuilder methods = new StringBuilder();
            boolean anyM = false;
            // Mismo recorrido que ensureMethodSlots: métodos públicos de instancia
            // (no ctor, no static), en orden de declaración → su slot de vtable.
            if (cls.astNode != null && cls.astNode.members != null) {
                for (Ast.ITopLevelDecl d : cls.astNode.members) {
                    // #324 tanda 2 — PROPERTIES: su SETTER también es un slot de
                    // vtable, y es el que el IDE necesita para hornear el "name"
                    // del .win (el modelo Swing: la ventana DECLARA sus
                    // componentes y el form los construye sobre esas
                    // declaraciones). Se emite con el nombre del accesor, que es
                    // el que resuelve slotOf.
                    //
                    // Se emiten TAMBIÉN las no públicas, y es lo que se quiere:
                    // los componentes de una ventana son sus TRIPAS — si fueran
                    // públicos, cualquiera podría hacer `win.boton1 := otraCosa`
                    // y romper el form. Privadas, el compilador cierra la puerta
                    // desde fuera y el slot sigue estando para atarlas. Mismo
                    // criterio que #316 con las properties privadas en la interfaz.
                    if (d instanceof Ast.PropertyDef) {
                        Ast.PropertyDef pd = (Ast.PropertyDef) d;
                        if (pd.name == null || pd.name.isStatic()) continue;
                        Symbol ps = cls.instanceMembers.tryLookup(pd.name.name);
                        if (!(ps instanceof Symbol.PropertySymbol)) continue;
                        String setter = "set" + Character.toUpperCase(pd.name.name.charAt(0))
                                + pd.name.name.substring(1);
                        int pslot = cls.slotOf(setter);
                        if (pslot < 0) continue;
                        if (anyM) methods.append(", ");
                        anyM = true;
                        methods.append(jsonStr(setter)).append(": ").append(pslot);
                        continue;
                    }
                    if (!(d instanceof Ast.FuncDef)) continue;
                    Ast.FuncDef fn = (Ast.FuncDef) d;
                    Symbol ms = cls.instanceMembers.tryLookup(fn.name.name);
                    if (!(ms instanceof Symbol.FunctionSymbol)) continue;
                    Symbol.FunctionSymbol fsym = (Symbol.FunctionSymbol) ms;
                    if (!fsym.isPublic || fsym.isStatic || fsym.isConstructor) continue;
                    int slot = cls.slotOf(fn.name.name);
                    if (slot < 0) continue;
                    if (anyM) methods.append(", ");
                    anyM = true;
                    methods.append(jsonStr(fn.name.name)).append(": ").append(slot);
                }
            }
            if (anyClass) body.append(",");
            anyClass = true;
            body.append("\n  ").append(jsonStr(cls.name)).append(": { ").append(methods).append(" }");
        }
        if (!anyClass) return;   // módulo sin clases públicas → no emitimos sidecar
        String json = "{" + body + "\n}\n";
        Path out = ctx.outDir.resolve(module.name + ".slots");
        Files.write(out, json.getBytes(java.nio.charset.StandardCharsets.UTF_8));
        indent(depth); System.out.println("slots    : " + out.getFileName());
    }

    /** Escapa un String para JSON (comilla + barra invertida). */
    private static String jsonStr(String s) {
        return "\"" + s.replace("\\", "\\\\").replace("\"", "\\\"") + "\"";
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
            // H6.a — pasada de interfaz TRANSITIVA. Antes de extraer la interfaz
            // de este módulo, aseguramos+cargamos las interfaces de SUS imports,
            // para que las firmas públicas que referencian tipos importados
            // (p.ej. `func crear(): C.Caja`) resuelvan y NO se caigan de la .bpi.
            // Precedente Modula-2/Turbo Pascal: la interfaz de C se construye
            // antes que la de B antes que la de A. El corte de ciclos
            // (compilingInterface, en compileInterface) evita recursión infinita;
            // si un tipo público queda sin resolver por un ciclo de interfaz, lo
            // reportará el análisis como error (dependencia circular de interfaz).
            // Modo interfaz = tolerante: la resolución de imports aquí es
            // INFORMATIVA, así que preservamos el contador de errores (el chequeo
            // estricto de imports lo hace la pasada FULL); así no cambiamos el
            // comportamiento externo (la interfaz nunca aportó errores fatales).
            if (module.imports != null && !module.imports.isEmpty()) {
                for (Ast.ImportNode imp : module.imports) {
                    ensureInterfaceForImport(imp, src, ctx, depth + 1);
                }
                int errsBeforeImports = ctx.totalErrors;
                loadImportsForAnalyzer(module, src, ctx, analyzer, depth + 1);
                ctx.totalErrors = errsBeforeImports;
            }
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
            // #211 — avisos no fatales (p.ej. llamada native→BP que pierde
            // la velocidad AOT). Se imprimen como avisos del compilador.
            for (String wmsg : emitter.getWarnings()) {
                System.out.println("-- aviso AOT: " + wmsg);
            }
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
        // H6.a — construye la interfaz del módulo EN MEMORIA (el mismo modelo que
        // el .bpi) y la embebe en el .mod (sección `interface`, formato v6). Es
        // lo que permite dejar de depender del .bpi en disco.
        //
        // DIFERIDA a propósito (#299): la interfaz publica el layout binario de
        // cada clase (numFields/numMethods), y ese lo calcula el EMISOR. Si se
        // extrae aquí, `binaryLayout` es null y ModuleInterface reconstruye el
        // layout desde el AST: cuenta campos en vez de slots (una ref son 2) y
        // sólo los métodos propios (sin los heredados de Object). El .mod
        // publicaba entonces un layout MENOR que el real, y toda subclase de otro
        // módulo colocaba sus miembros ENCIMA de los heredados. El supplier se
        // invoca al final de emitTo, con el layout ya calculado.
        String ifaceLib = (module.library == null) ? "" : module.library;
        emitter.setInterfaceSupplier(() -> ModuleInterface.extractFrom(
                ifaceLib, module.name, module.isInterface, module.implementsName,
                info.module, new ArrayList<>()).toBytes());
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
        List<String> skipped = new ArrayList<>();
        ModuleInterface iface = ModuleInterface.extractFrom(
                lib, module.name, module.isInterface, module.implementsName,
                info.module, skipped);
        // H6.a — cachea la interfaz EN MEMORIA (clave = nombre canónico). La
        // round-tripeamos (toBytes→fromBytes) para que el objeto cacheado sea
        // BYTE-idéntico a lo que un lector obtendría de disco (.bpi/.mod): así el
        // caché se comporta igual que leer del fichero, sin depender de que la
        // serialización sea 100% sin pérdidas. Esto es lo que permitirá dejar de
        // escribir el .bpi (los consumidores lo encuentran en el caché).
        String cacheKey = lib.isEmpty() ? module.name : lib + "." + module.name;
        ctx.interfaceCache.put(cacheKey, ModuleInterface.fromBytes(iface.toBytes(), cacheKey));
        // H6.a — YA NO se escribe el .bpi a disco. La interfaz vive (1) en el
        // caché en memoria (arriba) para el resto de ESTE build, y (2) embebida
        // en el .mod (sección `interface`) para builds futuros. El .bpi era el
        // artefacto que se quedaba rancio y mentía; al no existir, no puede.
        // (bpiName se conserva sólo para el nombre canónico / logs.)
        indent(depth); System.out.printf("interfaz : %s (en memoria, funcs=%d%s%s)%n",
                cacheKey, iface.functions.size(),
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
        // H6.a — si la interfaz ya está en el caché en memoria (construida en
        // este build), no hay nada que asegurar: es la fuente de verdad y no
        // dependemos de que exista un .bpi en disco.
        String ifaceCacheKey = library.isEmpty() ? moduleName : library + "." + moduleName;
        if (ctx.interfaceCache.containsKey(ifaceCacheKey)) return;
        // H5.b — `import Modulo from pack NombrePack`: el módulo NO es un .mod
        // suelto, vive dentro del pack. Su interfaz sale de ahí y punto (no se
        // busca fuente ni se recompila: un pack es un artefacto ya construido).
        if (imp.packName != null) {
            loadInterfaceFromPack(imp, moduleName, ifaceCacheKey, ctx, depth);
            return;
        }
        String bpiName = library.isEmpty() ? moduleName + ".bpi" : library + "." + moduleName + ".bpi";
        Path importerDir = importerSrc.toAbsolutePath().getParent();

        // En `import Iface:Impl from "..."`, el fromPath pertenece al impl, no
        // a la interfaz. Para resolver la interfaz no debemos usarlo (lo hace
        // ensureInterfaceForBoundImpl). Sólo en `import Module from "..."`
        // (sin binding) el fromPath aplica al propio módulo.
        String effectiveFromPath = (imp.boundImpl != null) ? null : imp.fromPath;

        Path bpSource = locateBpSource(qualifiedName, library, moduleName, effectiveFromPath, importerSrc, ctx);

        // 0) H6.a — ¿hay un .mod PRECOMPILADO v6 (con la interfaz embebida) y no
        //    obsoleto respecto a su fuente? Entonces la interfaz sale de ahí y la
        //    cacheamos: es el caso de la stdlib y evita recompilar el módulo
        //    desde fuente en cada build. readInterfaceCached devuelve null si el
        //    .mod es v5 (sin sección) → caemos al camino de siempre.
        String modName = library.isEmpty() ? moduleName + ".mod" : library + "." + moduleName + ".mod";
        Path preMod = locateImportMod(imp, modName, importerDir, ctx);
        if (preMod != null && !isStale(preMod, bpSource)
                && readInterfaceCached(preMod, ctx) != null) {
            indent(depth); System.out.printf("-- interfaz de '%s' desde %s (embebida) --%n",
                    qualifiedName, preMod.getFileName());
            return;
        }

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
        return readInterfaceCached(p, ctx);   // H6.a: caché en memoria
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
            ModuleInterface iface = readInterfaceCached(candidate, ctx);   // H6.a: caché en memoria
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

    // ============================================================
    // H6.a — caché de interfaces en memoria + lectura .bpi/.mod
    // ============================================================

    /** Nombre canónico del artefacto (clave del caché): basename sin extensión
     *  .bpi/.mod. "lib.Str.bpi" → "lib.Str", "Str.mod" → "Str". Así la .bpi y el
     *  .mod del mismo módulo comparten clave. */
    private static String keyForArtifact(Path p) {
        String n = p.getFileName().toString();
        if (n.endsWith(".bpi") || n.endsWith(".mod")) return n.substring(0, n.length() - 4);
        return n;
    }

    /** H6.a — localiza el .mod PRECOMPILADO de un import. Mismo orden de
     *  búsqueda que {@link #locateImportBpi} pero con extensión .mod: outDir →
     *  dir del importer → dependencyPaths (ahí vive la stdlib). Su sección
     *  `interface` (v6) es la fuente de la interfaz para deps precompiladas, así
     *  no hay que recompilarlas desde fuente en cada build. */
    private static Path locateImportMod(Ast.ImportNode imp, String modName, Path importerDir, Ctx ctx) {
        String effectiveFromPath = (imp.boundImpl != null) ? null : imp.fromPath;
        if (effectiveFromPath != null && !effectiveFromPath.isEmpty()) {
            String fp = effectiveFromPath;
            if (fp.endsWith(".bpi")) fp = fp.substring(0, fp.length() - 4) + ".mod";
            Path direct = importerDir.resolve(fp).toAbsolutePath().normalize();
            if (carriesInterface(direct)) return direct;
        }
        Path candidate = ctx.outDir.resolve(modName);
        if (carriesInterface(candidate)) return candidate;
        Path sib = importerDir.resolve(modName);
        if (carriesInterface(sib)) return sib;
        for (Path dep : ctx.dependencyPaths) {
            if (Files.isDirectory(dep)) {
                Path inDir = dep.resolve(modName);
                if (carriesInterface(inDir)) return inDir;
            } else if (Files.isRegularFile(dep)
                    && dep.getFileName().toString().equals(modName)
                    && carriesInterface(dep)) {
                return dep;
            }
        }
        return null;
    }

    /** H6.a — ¿este .mod puede DECIR cuál es su interfaz? Sólo un v6 la lleva
     *  dentro. Un v5 rancio tirado junto al fuente (los hay: los .mod se emiten
     *  ahí) NO vale como fuente de interfaz y no debe cortar la búsqueda: hay que
     *  seguir hasta la stdlib, donde está el bueno. Devuelve false ante cualquier
     *  problema de lectura — quien no puede responder, no sirve. */
    private static boolean carriesInterface(Path modPath) {
        try { return extractInterfaceSection(modPath) != null; }
        catch (IOException e) { return false; }
    }

    /** H6.a — extrae la sección `interface` (texto del antiguo .bpi) de un .mod
     *  v6. Devuelve null si el .mod es v5 (sin sección) o su interfaz está vacía. */
    private static byte[] extractInterfaceSection(Path modPath) throws IOException {
        try (java.io.InputStream raw = new java.io.BufferedInputStream(Files.newInputStream(modPath))) {
            return extractInterfaceSection(raw);
        }
    }

    /** H5.b — misma extracción sobre los BYTES de un `.mod` (p.ej. el que vive
     *  DENTRO de un pack, que no tiene fichero propio en disco). */
    private static byte[] extractInterfaceSection(byte[] modBytes) throws IOException {
        return extractInterfaceSection(new java.io.ByteArrayInputStream(modBytes));
    }

    private static byte[] extractInterfaceSection(java.io.InputStream src) throws IOException {
        {
            java.io.DataInputStream in = new java.io.DataInputStream(src);
            int magic = in.readInt();
            if (magic != edu.bpgenvm.bytecode.ModFormat.MAGIC_NUMBER_V6) return null;
            in.readInt();                 // dataSize
            in.readInt();                 // mainOffset
            int importsSize = in.readInt();
            int exportsSize = in.readInt();
            in.readInt();                 // codeSize
            int librarySize = in.readInt();
            int interfaceSize = in.readInt();
            if (interfaceSize <= 0) return null;
            // Orden de secciones: library, imports, exports, [interface], data,
            // code. La interfaz va tras exports → saltamos library+imports+exports.
            byte[] skip = new byte[librarySize + importsSize + exportsSize];
            in.readFully(skip);
            byte[] iface = new byte[interfaceSize];
            in.readFully(iface);
            return iface;
        }
    }

    /** H5.b — resuelve `import Modulo from pack NombrePack`: localiza el `.pack`,
     *  saca de él la entrada `(mod, Modulo.mod)` y carga su interfaz embebida.
     *  Un pack es un artefacto YA CONSTRUIDO: no se busca fuente ni se recompila.
     *  Falla con un mensaje que dice exactamente qué faltó y dónde se buscó. */
    private static void loadInterfaceFromPack(Ast.ImportNode imp, String moduleName,
                                              String ifaceCacheKey, Ctx ctx, int depth)
            throws IOException {
        java.util.List<Path> tried = new java.util.ArrayList<>();
        Path packFile = locatePackFile(imp.packName, ctx, tried);
        if (packFile == null) {
            throw new IOException("no se encuentra el pack '" + imp.packName + "' (import de '"
                    + moduleName + "'). Buscado en: " + tried
                    + ". Configura la biblioteca de packs (IDE: engranaje del micro"
                    + " simulado → 'Librería de packs'; CLI: 'packsDir' en BpVM.cfg).");
        }
        basicplus.pack.PackReader.Pack pk;
        try {
            pk = basicplus.pack.PackReader.read(Files.readAllBytes(packFile));
        } catch (basicplus.pack.PackException e) {
            throw new IOException("pack '" + imp.packName + "' (" + packFile + ") ilegible: "
                    + e.getMessage(), e);
        }
        // La clave dentro del pack es (tipo, nombre) con tipo = extensión y nombre
        // = basename SIN extensión (`mod`/`Saludo`). Aceptamos también la variante
        // con `.mod` por si un pack se construyó con el nombre completo.
        String want = moduleName;
        byte[] modBytes = null;
        for (basicplus.pack.PackEntry e : pk.entries) {
            if (!"mod".equals(e.tipo)) continue;
            if (want.equals(e.nombre) || (want + ".mod").equals(e.nombre)) { modBytes = e.data; break; }
        }
        if (modBytes == null) {
            StringBuilder hay = new StringBuilder();
            for (basicplus.pack.PackEntry e : pk.entries) {
                if ("mod".equals(e.tipo)) { if (hay.length() > 0) hay.append(", "); hay.append(e.nombre); }
            }
            throw new IOException("el pack '" + imp.packName + "' (" + packFile.getFileName()
                    + ") no contiene el módulo '" + want + "'. Módulos del pack: ["
                    + hay + "]");
        }
        byte[] ifaceBytes = extractInterfaceSection(modBytes);
        if (ifaceBytes == null) {
            throw new IOException("el módulo '" + want + "' dentro del pack '" + imp.packName
                    + "' no lleva interfaz embebida (¿.mod de una versión anterior?); recompílalo");
        }
        ModuleInterface iface = ModuleInterface.fromBytes(ifaceBytes,
                packFile.getFileName() + "!" + want);
        ctx.interfaceCache.put(ifaceCacheKey, iface);
        indent(depth); System.out.printf("-- interfaz de '%s' desde el pack %s --%n",
                moduleName, packFile.getFileName());
    }

    /** H10 — biblioteca de packs fijada por el HOSPEDADOR del compilador (el IDE la
     *  pone desde sus preferencias antes de compilar). Hace falta porque el IDE
     *  llama al frontend EN PROCESO: no hay un BpVM.cfg suyo donde mirar, y obligar
     *  al usuario a mantener uno a mano sería un pie de foto para el olvido. */
    private static String packsDirOverride;

    /** Fija (o limpia, con null) la biblioteca de packs del hospedador. */
    public static void setPacksDir(String dir) {
        packsDirOverride = (dir == null || dir.isEmpty()) ? null : dir;
    }

    /** H5.b — busca `<nombre>.pack`: primero la BIBLIOTECA DE PACKS (la que fija el
     *  IDE, o packsDir del BpVM.cfg — la carpeta que distribuimos: stdlib, GUI, …),
     *  luego el outDir del proyecto (packs recién construidos) y los dirs de deps. */
    private static Path locatePackFile(String packName, Ctx ctx, java.util.List<Path> tried) {
        String fname = packName.endsWith(".pack") ? packName : packName + ".pack";
        java.util.List<Path> dirs = new java.util.ArrayList<>();
        if (packsDirOverride != null) dirs.add(Paths.get(packsDirOverride));
        try {
            edu.bpgenvm.config.VmConfig cfg = edu.bpgenvm.config.VmConfig.loadDefaultFor(null);
            if (cfg.packsDir != null && !cfg.packsDir.isEmpty()) dirs.add(Paths.get(cfg.packsDir));
        } catch (Throwable ignored) { /* sin config: solo dirs del proyecto */ }
        if (ctx.outDir != null) dirs.add(ctx.outDir);
        dirs.addAll(ctx.dependencyPaths);
        for (Path d : dirs) {
            if (d == null) continue;
            Path p = d.resolve(fname);
            tried.add(p);
            if (Files.isRegularFile(p)) return p.toAbsolutePath().normalize();
        }
        return null;
    }

    /** H6.a — lee una interfaz usando el caché EN MEMORIA como fuente primaria.
     *  Orden: (1) caché por nombre canónico; (2) si `path` es un .mod v6, su
     *  sección `interface` embebida (fromBytes); (3) fallback .bpi (readFrom).
     *  Cachea el resultado. Devuelve null si es un .mod sin interfaz (el caller
     *  cae al .bpi). Sustituye a {@code ModuleInterface.readFrom} en el resolver
     *  para no ir a disco cuando la interfaz ya se construyó en este build. */
    static ModuleInterface readInterfaceCached(Path path, Ctx ctx) throws IOException {
        String key = keyForArtifact(path);
        ModuleInterface cached = ctx.interfaceCache.get(key);
        if (cached != null) return cached;
        ModuleInterface iface;
        if (path.getFileName().toString().endsWith(".mod")) {
            byte[] bytes = extractInterfaceSection(path);
            if (bytes == null) return null;   // .mod v5 sin interfaz
            iface = ModuleInterface.fromBytes(bytes, path.toString());
        } else {
            iface = ModuleInterface.readFrom(path);
        }
        ctx.interfaceCache.put(key, iface);
        return iface;
    }

    // Package-private (#212): AotMain lo reutiliza para resolver imports antes de
    // emitir AOT, igual que compileFull aquí. Carga las .bpi disponibles en el
    // analizador (no compila deps; deben existir ya).
    static void loadImportsForAnalyzer(Ast.ModuleNode module, Path importerSrc, Ctx ctx,
                                               SemanticAnalyzer analyzer, int depth) throws IOException {
        if (module.imports == null || module.imports.isEmpty()) return;
        Path importerDir = importerSrc.toAbsolutePath().getParent();
        // L2 v3.e — recolectamos todos los namespaces que se cargan para
        // poder resolver tipos cross-module (`L2Lib.Counter`) en un segundo
        // pass tras crearlos todos. La resolución intra-ns se sigue haciendo
        // inline en el primer pass.
        java.util.List<Symbol.ImportedNamespaceSymbol> loadedNs = new java.util.ArrayList<>();
        // #248 — stubs de clase cuya base vive en OTRO módulo importado
        // (`extends Core.Exception`): {ns, stub, ClassSig}. Se resuelven en el
        // post-pass, cuando todos los namespaces están cargados.
        java.util.List<Object[]> pendingCrossBase = new java.util.ArrayList<>();
        for (Ast.ImportNode imp : module.imports) {
            String alias = imp.path.get(imp.path.size() - 1);
            String library = libraryFromImportPath(imp);
            String bpiName = library.isEmpty() ? alias + ".bpi" : library + "." + alias + ".bpi";

            // ---- 1) Resolver la interfaz: H6.a caché en memoria PRIMERO (así un
            //     módulo construido en ESTE build no necesita .bpi en disco), y
            //     en fallo localizar en disco (.mod v6 embebido o .bpi legacy).
            String cacheKey = library.isEmpty() ? alias : library + "." + alias;
            Path bpi = null;
            if (!ctx.interfaceCache.containsKey(cacheKey)) {
                bpi = locateImportBpi(imp, bpiName, library, alias, importerDir, ctx);
                if (bpi == null) {
                    // H6.a — el .bpi ya no se genera: para una librería precompilada
                    // (stdlib) la interfaz vive DENTRO de su .mod v6. Sin este
                    // fallback, `import Core` (implícito) se saltaba en silencio y
                    // RuntimeError dejaba de ser una clase → try/catch no compilaba.
                    String modName = library.isEmpty() ? alias + ".mod" : library + "." + alias + ".mod";
                    bpi = locateImportMod(imp, modName, importerDir, ctx);
                }
                if (bpi == null) {
                    indent(depth); System.out.printf("-- aviso: sin interfaz para import '%s' (ni '%s' ni su .mod) --%n",
                            alias, bpiName);
                    continue;
                }
            }

            try {
                ModuleInterface ifaceBpi = (bpi != null)
                        ? readInterfaceCached(bpi, ctx)          // disco (.mod/.bpi), cachea
                        : ctx.interfaceCache.get(cacheKey);      // ya en memoria
                if (ifaceBpi == null) {
                    // Un .mod sin interfaz legible: avisa, no revientes (antes: NPE).
                    indent(depth); System.out.printf("-- aviso: '%s' no expone interfaz legible para import '%s' --%n",
                            bpi, alias);
                    continue;
                }
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
                        ctx.totalErrors++;   // N6: fatal
                        continue;
                    }
                    // H6.a — cache-first para el impl bindeado (ya no hay .bpi en
                    // disco): buscamos su interfaz en el caché por nombre canónico
                    // (exacto o por sufijo, igual que la búsqueda amplia que hace
                    // resolveImplBpi sobre los .bpi). Fallback a disco si no está.
                    ModuleInterface implBpi = null;
                    for (Map.Entry<String, ModuleInterface> ce : ctx.interfaceCache.entrySet()) {
                        String k = ce.getKey();
                        if (k.equals(imp.boundImpl) || k.endsWith("." + imp.boundImpl)) {
                            implBpi = ce.getValue(); break;
                        }
                    }
                    if (implBpi == null) {
                        Path implBpiPath = resolveImplBpi(imp.boundImpl, imp.fromPath, module, importerDir, ctx);
                        if (implBpiPath == null) {
                            indent(depth); System.err.printf(
                                "-- error: no se encuentra interfaz del impl '%s' --%n",
                                imp.boundImpl);
                            ctx.totalErrors++;   // N6: fatal
                            continue;
                        }
                        implBpi = readInterfaceCached(implBpiPath, ctx);   // H6.a: caché en memoria
                    }
                    if (implBpi.isInterface) {
                        indent(depth); System.err.printf(
                            "-- error: '%s' es una interfaz, no puede ser impl bindeado --%n",
                            imp.boundImpl);
                        ctx.totalErrors++;   // N6: fatal
                        continue;
                    }
                    // El impl puede implementar la interfaz pedida directamente
                    // o cualquier descendiente de ella (subinterfaz). Esto
                    // permite `import LogApi:RichLoggerV2` donde RichLoggerV2
                    // implementa LogApiV2 que extiende LogApi.
                    if (!implSatisfies(implBpi, joinPath(imp.path), importerSrc, ctx, depth)) {
                        indent(depth); System.err.printf(
                            "-- error: '%s' no implementa '%s' (directa o transitivamente; declara %s). "
                            + "Necesitas un módulo igual o más nuevo que implemente '%s'. --%n",
                            imp.boundImpl, joinPath(imp.path), implBpi.implementsName, joinPath(imp.path));
                        ctx.totalErrors++;   // N6: fatal
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
                    ctx.totalErrors++;   // N6: fatal
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
                        if (ps.defaultValue != null)   // H8.1
                            psym.defaultExpr = SemanticAnalyzer.literalExprFromValue(ps.defaultValue, ps.type, 0, 0);
                        f.params.add(psym);
                    }
                    // H5.a-E2 — sobrecarga cross-module: si el nombre ya lo tiene
                    // otra firma importada, ENCADENA (la interfaz ya trae los tipos
                    // de cada params); resolveOverloadCall elige en la llamada, y el
                    // CALL_EXT nombra el target mangleado (externalQualifiedName).
                    Symbol.FunctionSymbol prevOv = ns.functions.get(fs.name);
                    if (prevOv == null) {
                        ns.functions.put(fs.name, f);
                        f.slotKey = f.name;             // 1ª firma del grupo: nombre pelado
                    } else {
                        Symbol.FunctionSymbol tail = prevOv;
                        while (tail.nextOverload != null) tail = tail.nextOverload;
                        tail.nextOverload = f;
                        prevOv.overloaded = true;
                        f.overloaded = true;
                        f.slotKey = f.overloadMangle(); // sobrecarga: clave mangleada
                    }
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
                    // Herencia cross-module: si la clase extiende una base del
                    // MISMO módulo (ya procesada antes en el .bpi — no se puede
                    // extender una clase declarada después), sembrar sus slots
                    // de vtable heredados. Así los métodos PROPIOS de la
                    // subclase numeran IGUAL que el emisor: un override reusa el
                    // slot heredado; un método nuevo se añade al final. Sin
                    // esto, un método propio declarado antes del override
                    // quedaba con slot desfasado → INVOKE_VIRTUAL cross-module
                    // a la ranura equivocada (los envoltorios Integer/etc. que
                    // extienden Comparable lo destaparon).
                    boolean deferSlots = false;   // base en OTRO módulo → post-pass
                    Symbol.ClassSymbol baseStub = (cs.baseClassName != null)
                            ? ns.classes.get(cs.baseClassName) : null;
                    if (baseStub != null) {
                        // #248 — enlazar la base también para el LOOKUP de
                        // miembros heredados (e.msg sobre el stub hijo).
                        stub.baseClass = baseStub;
                        stub.externalMethodSlots.putAll(baseStub.externalMethodSlots);
                        nextSlot = baseStub.externalMethodSlots.size();
                    } else if (cs.baseClassName == null) {
                        // H5.1.a — raíz implícita Object: TODA clase de usuario
                        // desciende de Object, cuyos 2 métodos virtuales ocupan
                        // los slots 0 (toString) y 1 (compareTo). El emisor copia
                        // Object como parent local, así que sus métodos propios
                        // numeran desde 2. Sembramos esos 2 slots aquí para que
                        // INVOKE_VIRTUAL cross-module despache a la ranura
                        // correcta (mismo riesgo BUG-5 si se omite).
                        stub.externalMethodSlots.put("toString", 0);
                        stub.externalMethodSlots.put("compareTo", 1);
                        nextSlot = 2;
                    } else if ("Object".equals(cs.baseClassName)) {
                        // #291 — base builtin EXPLÍCITA `extends Object`: misma
                        // siembra que la raíz implícita. Antes caía al defer de
                        // abajo buscando un módulo "Object" que no existe → los
                        // métodos propios numeraban desde 0 (slot de toString).
                        stub.externalMethodSlots.put("toString", 0);
                        stub.externalMethodSlots.put("compareTo", 1);
                        nextSlot = 2;
                    } else if ("Thread".equals(cs.baseClassName)) {
                        // #291 — base builtin Thread (SIN Object: run=0 lo asume
                        // __threadStart en las VMs). Antes este caso caía al
                        // defer → el stub no tenía ni los slots NI los miembros
                        // heredados → `w.start()` sobre una subclase de Thread
                        // importada era "no tiene miembro 'start'": las
                        // subclases de Thread no se podían usar cross-module.
                        // Mismos números que computeClassLayout (la función única).
                        stub.externalMethodSlots.put("run",   0);
                        stub.externalMethodSlots.put("start", 1);
                        stub.externalMethodSlots.put("join",  2);
                        nextSlot = 3;
                        for (String mn : new String[]{"run", "start", "join"}) {
                            Symbol.FunctionSymbol thf = new Symbol.FunctionSymbol(
                                    mn, true, false, false, stub, null);
                            thf.returnType = null;   // void (misma convención que FuncSig)
                            stub.instanceMembers.tryDefine(thf);
                        }
                    } else {
                        // #248 (cerraba el gap anotado de #44): base en OTRO
                        // módulo (`extends Core.Exception`). Aquí su ns puede no
                        // estar cargado aún → difiere baseClass + siembra de
                        // slots al post-pass L2 v3.e, con todos los ns presentes.
                        // Sin esto los métodos propios numeraban desde 0 →
                        // INVOKE_VIRTUAL a la ranura equivocada (getCode → toString).
                        deferSlots = true;
                        pendingCrossBase.add(new Object[]{ns, stub, cs});
                    }
                    // Properties → getter (slot N) + setter (slot N+1)
                    for (ModuleInterface.PropSig p : cs.properties) {
                        // #316 — respeta la visibilidad declarada: una property NO
                        // pública se importa igualmente (sus accesores ocupan slots
                        // y hay que contarlos), pero marcada como no-pública ⇒
                        // checkVisibility sigue prohibiendo el acceso desde fuera y
                        // permitiéndoselo a las subclases (que es su razón de ser).
                        Symbol.PropertySymbol psym = new Symbol.PropertySymbol(
                                p.name, p.isPublic, false, false, stub, null);
                        psym.type = p.type;
                        stub.instanceMembers.tryDefine(psym);
                        if (!deferSlots) {
                            String capName = Character.toUpperCase(p.name.charAt(0)) + p.name.substring(1);
                            stub.externalMethodSlots.put("get" + capName, nextSlot++);
                            stub.externalMethodSlots.put("set" + capName, nextSlot++);
                        }
                    }
                    // H5.c-E2 — EVENTOS. No tocan la vtable (viven en campos), así
                    // que no entran en el conteo de slots: se importan con el slot
                    // de su campo `recv`, que publicó el módulo dueño. Los eventos
                    // HEREDADOS no se copian — el lookup recorre baseClass, igual
                    // que con los métodos.
                    for (ModuleInterface.EventSig e : cs.events) {
                        Symbol.EventSymbol esym = new Symbol.EventSymbol(e.name, stub, 0, 0);
                        esym.externalRecvSlot = e.recvSlot;
                        esym.externalOpaque   = e.opaque;
                        for (ModuleInterface.ParamSig p : e.params) {
                            Symbol.ParamSymbol psym = new Symbol.ParamSymbol(p.name, 0, 0);
                            psym.type = p.type;
                            esym.params.add(psym);
                        }
                        stub.instanceMembers.tryDefine(esym);
                    }
                    // Methods en orden de declaración
                    for (ModuleInterface.FuncSig m : cs.methods) {
                        Symbol.FunctionSymbol fsym = new Symbol.FunctionSymbol(
                                m.name, true, false, false, stub, null);
                        fsym.returnType = m.returnType;
                        for (ModuleInterface.ParamSig pp : m.params) {
                            Symbol.ParamSymbol psm = new Symbol.ParamSymbol(pp.name, 0, 0);
                            psm.type = pp.type;
                            if (pp.defaultValue != null)   // H8.1
                                psm.defaultExpr = SemanticAnalyzer.literalExprFromValue(pp.defaultValue, pp.type, 0, 0);
                            fsym.params.add(psm);
                        }
                        // isExternal queda false porque INVOKE_VIRTUAL despacha
                        // via vtable, no via CALL_EXT. Lo que sí necesita el
                        // emisor es el slot — está en externalMethodSlots.
                        // H5.a-E5 — sobrecarga de métodos cross-module: si el nombre
                        // ya está definido en el stub, ENCADENAR (tryDefine fallaría
                        // en silencio y la 2ª firma se perdería). La CLAVE de slot
                        // sigue la misma regla aditiva: 1ª firma = nombre pelado, las
                        // siguientes mangleadas ⇒ cada una con su propio slot.
                        Symbol prevM = stub.instanceMembers.tryLookup(m.name);
                        if (prevM instanceof Symbol.FunctionSymbol) {
                            Symbol.FunctionSymbol headM = (Symbol.FunctionSymbol) prevM;
                            Symbol.FunctionSymbol tailM = headM;
                            while (tailM.nextOverload != null) tailM = tailM.nextOverload;
                            tailM.nextOverload = fsym;
                            headM.overloaded = true;
                            fsym.overloaded  = true;
                            fsym.slotKey = fsym.overloadMangle();
                        } else {
                            stub.instanceMembers.tryDefine(fsym);
                            fsym.slotKey = fsym.name;
                        }
                        // Override de un método heredado → mantiene su slot base
                        // (ya sembrado). Método nuevo/sobrecarga → siguiente slot libre.
                        if (!deferSlots && !stub.externalMethodSlots.containsKey(fsym.slotKey())) {
                            stub.externalMethodSlots.put(fsym.slotKey(), nextSlot++);
                        }
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
                            if (pp.defaultValue != null)   // H8.1
                                psm.defaultExpr = SemanticAnalyzer.literalExprFromValue(pp.defaultValue, pp.type, 0, 0);
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
                        alias, (bpi != null ? bpi.getFileName().toString() : "caché en memoria"),
                        ns.functions.size(), ns.consts.size(), ns.enums.size(),
                        ns.properties.size(), ns.classes.size());
            } catch (IOException ex) {
                System.err.println("error leyendo " + (bpi != null ? bpi : cacheKey) + ": " + ex.getMessage());
            }
        }

        // #248 — post-pass de bases cross-module: enlaza stub.baseClass y
        // siembra la vtable (slots heredados + propios) de los stubs cuya base
        // vive en otro namespace. Iterativo por si hay cadenas (A.X extends
        // B.Y extends C.Z): una pasada por nivel, hasta no progresar.
        if (!pendingCrossBase.isEmpty()) {
            java.util.List<Object[]> pend = new java.util.ArrayList<>(pendingCrossBase);
            for (int round = 0; !pend.isEmpty() && round < 8; round++) {
                boolean progress = false;
                java.util.Iterator<Object[]> it = pend.iterator();
                while (it.hasNext()) {
                    Object[] e = it.next();
                    Symbol.ImportedNamespaceSymbol ownNs = (Symbol.ImportedNamespaceSymbol) e[0];
                    Symbol.ClassSymbol stub = (Symbol.ClassSymbol) e[1];
                    ModuleInterface.ClassSig cs = (ModuleInterface.ClassSig) e[2];
                    Symbol.ClassSymbol base = findImportedClass(cs.baseClassName, ownNs, loadedNs);
                    if (base == null) {
                        if (round == 7) {
                            indent(depth); System.err.printf(
                                "-- aviso: base cross-module '%s' de la clase importada '%s.%s' no resuelta"
                                + " (¿falta el import de su módulo?) — vtable incompleta --%n",
                                cs.baseClassName, ownNs.moduleName, cs.name);
                        }
                        continue;
                    }
                    // Si la base es a su vez pendiente y aún no sembrada, espera
                    // a la siguiente ronda (sus slots todavía no están).
                    boolean baseStillPending = false;
                    for (Object[] p2 : pend) {
                        if (p2[1] == base) { baseStillPending = true; break; }
                    }
                    if (baseStillPending) continue;
                    stub.baseClass = base;
                    stub.externalMethodSlots.putAll(base.externalMethodSlots);
                    int next = base.externalMethodSlots.size();
                    for (ModuleInterface.PropSig p : cs.properties) {
                        String capName = Character.toUpperCase(p.name.charAt(0)) + p.name.substring(1);
                        stub.externalMethodSlots.put("get" + capName, next++);
                        stub.externalMethodSlots.put("set" + capName, next++);
                    }
                    for (ModuleInterface.FuncSig m : cs.methods) {
                        if (!stub.externalMethodSlots.containsKey(m.name)) {
                            stub.externalMethodSlots.put(m.name, next++);
                        }
                    }
                    it.remove();
                    progress = true;
                }
                if (!progress) break;
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

    /** #248 — import Core implícito (lazy): si el módulo usa try/throw o nombra
     *  Exception/RuntimeError en un catch, y no es Core ni ya lo importa,
     *  inyecta `import Core` sintético al AST. Así RuntimeError/Exception
     *  resuelven a las clases ÚNICAS de Core (stubs del .bpi, aliasadas sin
     *  cualificar por injectImports) y los módulos sin excepciones no ganan
     *  ninguna dependencia. */
    static void injectImplicitCoreImport(Ast.ModuleNode module) {
        if (module == null || "Core".equals(module.name)) return;
        if (module.imports != null) {
            for (Ast.ImportNode imp : module.imports) {
                if (imp.path != null && !imp.path.isEmpty()
                        && "Core".equals(imp.path.get(imp.path.size() - 1))) return;
            }
        }
        if (!usesExceptions(module)) return;
        if (module.imports == null) return;   // defensivo: el parser siempre crea la lista
        java.util.List<String> p = new java.util.ArrayList<>();
        p.add("Core");
        module.imports.add(new Ast.ImportNode(p, null, module.line, module.column));
    }

    /** #248 — ¿el módulo usa excepciones? try/throw en cualquier cuerpo
     *  (funciones libres, métodos, accesores de property), o un tipo llamado
     *  Exception/RuntimeError en firmas o declaraciones. */
    private static boolean usesExceptions(Ast.ModuleNode module) {
        for (Ast.ITopLevelDecl d : module.defs) {
            if (declUsesExceptions(d)) return true;
        }
        return false;
    }

    private static boolean declUsesExceptions(Ast.ITopLevelDecl d) {
        if (d instanceof Ast.FuncDef) {
            Ast.FuncDef f = (Ast.FuncDef) d;
            if (typeNamesException(f.returnType)) return true;
            for (Ast.Param p : f.params) if (typeNamesException(p.type)) return true;
            return stmtsUseExceptions(f.body);
        }
        if (d instanceof Ast.VarDecl)     return typeNamesException(((Ast.VarDecl) d).type);
        if (d instanceof Ast.PropertyDef) {
            Ast.PropertyDef pd = (Ast.PropertyDef) d;
            if (typeNamesException(pd.type)) return true;
            if (pd.getter != null && stmtsUseExceptions(pd.getter.body)) return true;
            if (pd.setter != null && stmtsUseExceptions(pd.setter.body)) return true;
            return false;
        }
        if (d instanceof Ast.ClassDef) {
            Ast.ClassDef cd = (Ast.ClassDef) d;
            if ("Exception".equals(cd.baseClass) || "RuntimeError".equals(cd.baseClass)) return true;
            for (Ast.ITopLevelDecl m : cd.members) if (declUsesExceptions(m)) return true;
            return false;
        }
        return false;
    }

    private static boolean typeNamesException(Ast.TypeRef t) {
        if (!(t instanceof Ast.SimpleTypeRef)) return false;
        String n = ((Ast.SimpleTypeRef) t).name;
        return "Exception".equals(n) || "RuntimeError".equals(n);
    }

    private static boolean stmtsUseExceptions(java.util.List<Ast.IStmt> body) {
        if (body == null) return false;
        for (Ast.IStmt s : body) {
            if (s instanceof Ast.ThrowStmt || s instanceof Ast.TryStmt) return true;
            if (s instanceof Ast.IfStmt) {
                Ast.IfStmt i = (Ast.IfStmt) s;
                if (stmtsUseExceptions(i.then_.body)) return true;
                for (Ast.IfClause c : i.elseIfs) if (stmtsUseExceptions(c.body)) return true;
                if (i.else_ != null && stmtsUseExceptions(i.else_)) return true;
            } else if (s instanceof Ast.WhileStmt) {
                if (stmtsUseExceptions(((Ast.WhileStmt) s).body)) return true;
            } else if (s instanceof Ast.ForStmt) {
                if (stmtsUseExceptions(((Ast.ForStmt) s).body)) return true;
            } else if (s instanceof Ast.DoLoopStmt) {
                if (stmtsUseExceptions(((Ast.DoLoopStmt) s).body)) return true;
            } else if (s instanceof Ast.SwitchStmt) {
                Ast.SwitchStmt sw = (Ast.SwitchStmt) s;
                for (Ast.CaseClause c : sw.cases) if (stmtsUseExceptions(c.body)) return true;
                if (sw.defaultBody != null && stmtsUseExceptions(sw.defaultBody)) return true;
            } else if (s instanceof Ast.ParallelStmt) {
                Ast.ParallelStmt ps = (Ast.ParallelStmt) s;
                for (Ast.ParallelBranch c : ps.branches)
                    if (stmtsUseExceptions(c.body)) return true;
                if (ps.defaultBody != null && stmtsUseExceptions(ps.defaultBody)) return true;
            } else if (s instanceof Ast.VarDecl) {
                if (typeNamesException(((Ast.VarDecl) s).type)) return true;
            }
        }
        return false;
    }

    /** #248 — localiza la ClassSymbol de una base cross-module de un stub
     *  importado. `name` puede venir sin punto (clase del PROPIO ns — raro
     *  aquí, pero seguro) o dotted (`Core.Exception`, `Lib.Mod.Cls`): el
     *  último segmento es la clase y el prefijo casa contra el alias o el
     *  qualified name de cada namespace cargado. */
    private static Symbol.ClassSymbol findImportedClass(
            String name,
            Symbol.ImportedNamespaceSymbol ownNs,
            java.util.List<Symbol.ImportedNamespaceSymbol> allNs) {
        if (name == null || name.isEmpty()) return null;
        int lastDot = name.lastIndexOf('.');
        if (lastDot < 0) {
            Symbol.ClassSymbol own = ownNs.classes.get(name);
            if (own != null) return own;
            for (Symbol.ImportedNamespaceSymbol candidate : allNs) {
                Symbol.ClassSymbol cls = candidate.classes.get(name);
                if (cls != null) return cls;
            }
            return null;
        }
        String modPart = name.substring(0, lastDot);
        String clsPart = name.substring(lastDot + 1);
        for (Symbol.ImportedNamespaceSymbol candidate : allNs) {
            String full = candidate.library.isEmpty()
                    ? candidate.moduleName
                    : candidate.library + "." + candidate.moduleName;
            if (full.equals(modPart) || candidate.moduleName.equals(modPart)
                    || candidate.name.equals(modPart)) {
                Symbol.ClassSymbol cls = candidate.classes.get(clsPart);
                if (cls != null) return cls;
            }
        }
        return null;
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
            ModuleInterface bpi = readInterfaceCached(bpiPath, ctx);   // H6.a: caché en memoria
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
