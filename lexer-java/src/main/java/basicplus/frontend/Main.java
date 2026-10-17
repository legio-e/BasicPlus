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
    /** V7/H2 (#523) — ver Ctx.formato. MOD9 por defecto desde el 17-sep (V11, decisión de
     *  Eduardo: «no se puede mantener la mezcla de versiones»); `--formato=8` sigue existiendo
     *  como salida de emergencia hasta cerrar V7 (la foto «antes» de compat/mezcla.py). */
    public static int FORMATO = 9;
    /** V7/H2 (#523, V11, 17-sep) — `--stdlibDir=DIR`: la stdlib contra la que se compila, dicha
     *  desde la línea de órdenes (como el `--stdlibDir` de la miVM). Manda sobre el `stdlibDir`
     *  del BpVM.cfg autodescubierto (#467: la stdlib PRIMERO, y ésta es «la más específica»).
     *  Lo necesita cualquier arnés que compile un .bp del repo contra una stdlib que no es la
     *  de bpstdlib/ (la MOD8 del temporal, la que se sube a una placa): sin esto el sello
     *  `ifaceCrc` del consumidor era el de OTRA interfaz y la norma #500 rechazaba con razón. */
    public static String STDLIB_DIR = null;
    /** V7/H2 (#523, V1) — el guardián del compilador: permutar las ranuras propias de cada clase
     *  (semilla; 0 = identidad) y meter n miembros privados de relleno. Ver {@link Barajar}. */
    public static int BARAJAR = 0, RELLENO = 0;

    static final class Ctx {
        /** #542 (V7/H5) — un import cuya interfaz no aparece, ¿aviso o error? El compilador avisa
         *  y sigue (el fallo sale luego como nombres que no resuelven, y el modo interfaz es
         *  tolerante a propósito); el AOT lo pone a true: sin la interfaz de `Core` generaba código
         *  EQUIVOCADO sin un error (Eduardo, 25-sep: «si no está es un error de carga de módulo»). */
        boolean faltaImportEsError = false;
        /** #542 — los imports que faltaron con {@link #faltaImportEsError}, con su nombre. */
        final List<String> importsQueFaltan = new ArrayList<>();
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
        /** V7/H2 (MOD8, #500) — la generación de ESTE build (la `version` del proyecto;
         *  0 en fichero suelto) y, por nombre canónico (`Core`, `acme.LibHdr`), el
         *  (gen, ifaceCrc) de cada módulo cuya interfaz se ha leído o producido aquí:
         *  es lo que cada importador sella en su subtabla `deps`. */
        int gen = 0;
        final Map<String, int[]> versionesDeps = new java.util.LinkedHashMap<>();
        /** V7/H2 (#523) — el FORMATO del .mod que se emite: 9 (enlace tardío, el de hoy) u 8
         *  (el anterior, sólo con `--formato=8`). Estático a propósito: vale para los tres
         *  caminos de compilación. */
        int formato = FORMATO;

        /** Mapa de "<library>.<Module>" → ruta del .bp. Construido lazy. */
        Map<String, Path> bpSources;
        Path rootSrcDir;

        /* ── V5/H8 — LO QUE EL BUILD SABE Y ANTES NO CONTABA ────────────────
         *
         * Pregunta de Eduardo: «si un usuario construye un pack, ¿cómo sabe qué
         * dependencias va a necesitar?». La respuesta es que el compilador YA lo
         * sabe —recorre el cierre transitivo de imports para compilar— y
         * simplemente no lo decía. Esto lo apunta.
         *
         * Lo que el PC NO puede saber es qué hay en UNA PLACA: cada firmware
         * embebe un conjunto distinto y encima puede haber otros packs grabados.
         * Por eso esto no decide nada, sólo DECLARA: el pack dice lo que
         * necesita, y quien lo carga —el único que sabe lo que tiene— compara. */

        /** Nombre canónico del módulo → el `.mod` PRECOMPILADO del que salió su
         *  interfaz. Sólo los que NO compila este build (la stdlib y demás
         *  dependencias); los propios se emiten al outDir y ya están ahí.
         *
         *  <p>Es la ruta REAL que resolvió {@link #locateImportMod}, no una
         *  reconstruida después — así el pack se lleva exactamente aquello
         *  contra lo que se compiló, y un desfase dentro del pack es imposible. */
        final Map<String, Path> modsUsados = new java.util.LinkedHashMap<>();

        /** Los packs de los que este build importa algo (`import v1 from pack
         *  "SQLI"`). Son las dependencias OPCIONALES: no están en el micro y hay
         *  que grabarlas aparte. Justo lo que el usuario no puede adivinar hoy. */
        final Set<String> packsRequeridos = new LinkedHashSet<>();

        /** ¿El módulo raíz tiene función `main`? Decide si el pack es EJECUTABLE.
         *  Una librería no tiene por qué llevar main (criterio de Eduardo), y esto
         *  se SABE, no se declara: preguntarlo otra vez en el `.bpbuild` sería un
         *  segundo sitio para la misma verdad, y los dos sitios se separan. */
        boolean raizTieneMain = false;

        /** V5/H11 (#365) — CÓMO SE LLAMA la entrada del módulo de arranque dentro
         *  del pack: su nombre canónico, cualificado si declara `library`
         *  (`com.example.Demo`). No coincide con `proj.main`, que nombra el
         *  FICHERO FUENTE (`Demo.bp`) — y ésa era justo la razón por la que un
         *  módulo con `library` no podía arrancar un pack: el manifest decía
         *  `Demo` y dentro la entrada se llamaba `com.example.Demo`.
         *
         *  Se toma de la raíz que TIENE main, no de la última compilada, y el
         *  primero gana (con varias raíces, sólo una nombra el arranque). */
        String raizEntry = "";

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
        /** #467 — el stdlibDir del BpVM.cfg que mande (el del fuente pisa al del
         *  cwd). Se consulta ANTES que outDir y que el dir del importer: un módulo
         *  que vive aquí ES la stdlib, y una copia rancia por el camino no vale. */
        Path stdlibDir = null;

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
                if (cfg.stdlibDir != null && !cfg.stdlibDir.isEmpty()
                        && Files.isDirectory(Paths.get(cfg.stdlibDir))) {
                    stdlibDir = Paths.get(cfg.stdlibDir);   // #467: el último en llamar, el más específico
                }
            } catch (Throwable ignored) {
                // Sin config; sigue funcionando para módulos que no importan stdlib.
            }
            // V11 — la de la línea de órdenes gana siempre (se aplica en cada llamada, que es
            // justo lo que hace «más específico» al último en llamar).
            if (STDLIB_DIR != null) {
                addDepDirIfPresent(STDLIB_DIR);
                if (Files.isDirectory(Paths.get(STDLIB_DIR))) stdlibDir = Paths.get(STDLIB_DIR);
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
        return buildProject(proj, backend, pruneBpi, null);
    }

    /**
     * Lo que hay que hacer DESPUÉS de compilar y ANTES de empaquetar.
     *
     * <p>Existe por el AOT: los `.mdn` tienen que estar escritos en el `outDir`
     * cuando {@code PackStep} lo barre, o el pack sale sin ellos. Y el AOT no
     * puede vivir aquí —necesita las rutas de los toolchain, que son de cada
     * máquina y las guarda el IDE—, así que el frontend deja el hueco y quien
     * sabe compilar lo rellena. Sin esto habría que partir el build en dos y
     * duplicar el paso de empaquetado en los dos llamantes.
     */
    public interface PasoAntesDelPack {
        /** @return false si el paso falló de forma que NO se debe empaquetar. */
        boolean ejecutar(BpBuild proj) throws IOException;
    }

    /** Igual, con un paso intermedio (ver {@link PasoAntesDelPack}). */
    public static boolean buildProject(BpBuild proj, String backend, boolean pruneBpi,
                                       PasoAntesDelPack antesDelPack)
            throws IOException {
        System.out.println("project: " + proj.sourcePath
                + " (main=" + proj.main
                + ", sourceDir=" + proj.sourceDir
                + ", outDir=" + proj.outDir
                + ", dependencies=" + proj.dependencies + ")");
        // V5/H5 — el compilador CONTRASTA las entidades con la base real. Solo
        // avisos, nunca errores: una base se puede disenar entera antes que el
        // programa, y con 40 tablas por delante nadie escribe las 40 entidades
        // de un tiron. Bloquear el build por eso convertiria la herramienta en
        // un estorbo (criterio de Eduardo).
        DaoGen.Resultado ver = DaoGen.verificar(proj);
        for (String w : ver.avisos) System.out.println("aviso BD: " + w);
        if (ver.resumen != null)    System.out.println("BD: " + ver.resumen);

        /* QUÉ SE COMPILA (V5/H8). Con `sources` lo dice el proyecto, uno a uno;
         * sin ella, el de siempre: `sourceDir/main.bp`.
         *
         * Una librería no tiene UNA raíz de la que cuelgue todo — tiene varias,
         * y cuáles publica lo sabe sólo quien la escribe (idea de Eduardo). */
        List<Path> raices = new ArrayList<>();
        if (!proj.sources.isEmpty()) {
            for (String s : proj.sources) raices.add(Paths.get(s));
            System.out.println("sources: " + raices.size() + " modulo(s) — "
                    + proj.sources.stream().map(s -> Paths.get(s).getFileName().toString())
                        .collect(java.util.stream.Collectors.joining(", ")));
        } else {
            Path mainBp = Paths.get(proj.sourceDir, proj.main + ".bp")
                    .toAbsolutePath().normalize();
            if (!Files.exists(mainBp)) {
                System.err.println("no se encuentra el fichero del main: " + mainBp);
                return false;
            }
            raices.add(mainBp);
        }
        Ctx ctx = new Ctx();
        ctx.backend    = backend;
        ctx.showTokens = false;
        ctx.showAst    = false;
        ctx.rootSrcDir = Paths.get(proj.sourceDir);
        ctx.outDir     = Paths.get(proj.outDir);
        ctx.gen        = proj.version;   // V7/H2 (MOD8): la generación del proyecto
        Files.createDirectories(ctx.outDir);
        for (String d : proj.dependencies) ctx.dependencyPaths.add(Paths.get(d));
        ctx.pruneBpi = pruneBpi;
        boolean ok = true;
        for (Path raiz : raices) {
            /* `raizTieneMain` se pone a depth 0 en cada raíz. Con varias, lo que
             * decide si el pack es ejecutable es si ALGUNA la tiene — y en la
             * práctica sólo puede tenerla la que nombra `main`. */
            boolean teniaMain = ctx.raizTieneMain;
            if (!compileFull(raiz, ctx, /*depth*/0)) ok = false;
            ctx.raizTieneMain = teniaMain || ctx.raizTieneMain;
        }
        boolean success = ok && ctx.totalErrors == 0;
        if (success && ctx.pruneBpi) pruneWrittenBpi(ctx);
        // H3 Packs: tras el build correcto, si out:pack empaqueta el proyecto
        // (el manifest reusa `main` → pack ejecutable). IOException la ve el caller.
        if (success && "pack".equals(proj.out)) {
            // V5/H8 — el AOT va AQUÍ, entre compilar y empaquetar: los `.mdn` de
            // cada familia tienen que existir cuando PackStep barre el outDir.
            if (antesDelPack != null && !antesDelPack.ejecutar(proj)) {
                System.err.println("no se empaqueta: el paso previo al pack fallo");
                return false;
            }
            /* El pack se lleva lo que el build SUPO: los `.mod` que resolvió,
             * los packs de los que importa, y si la raíz tiene `main`. Nada de
             * esto se declara aparte — se sabe. */
            /* V7/H2 (#493) — y dónde buscar lo que falte para CERRAR el pack (un
             * pack es autocontenido): la stdlib y las dependencias del proyecto. */
            java.util.List<Path> dondeBuscar = new java.util.ArrayList<>();
            if (ctx.stdlibDir != null) dondeBuscar.add(ctx.stdlibDir);
            dondeBuscar.addAll(ctx.dependencyPaths);
            Path packPath = PackStep.buildPack(proj, new PackStep.Cierre(
                    ctx.modsUsados, ctx.packsRequeridos, ctx.raizTieneMain,
                    ctx.raizEntry, dondeBuscar, ctx.stdlibDir));   /* V7 #623: la stdlib no entra en un pack librería */
            System.out.println("pack generado: " + packPath);
        }
        return success;
    }

    /**
     * EL NOMBRE CANÓNICO de un módulo: `Demo` a secas, o `com.example.Demo` si
     * declara `library`. Es el nombre con el que se escribe su `.mod`, con el que
     * lo busca un `import` y con el que entra en un pack — o sea, el nombre por
     * el que se le conoce en todas partes menos en el fichero fuente.
     *
     * <p>La concatenación está escrita a mano en una veintena de sitios de este
     * fichero (el resolutor de imports, sobre todo). Este helper NO viene a
     * unificarlos —eso es un refactor aparte, y del camino más delicado que hay
     * aquí—: viene a que el sitio donde el nombre decide el CONTENIDO DE UN
     * ARTEFACTO tenga una sola forma de calcularlo. Es lo que añade #365: si el
     * manifest del pack compusiera el nombre de una manera y el emisor del `.mod`
     * de otra, el pack no arrancaría y el error no diría por qué.
     */
    static String nombreCanonico(String library, String name) {
        return (library == null || library.isEmpty()) ? name : library + "." + name;
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
        boolean daoBuild       = false;
        // El default de la CLI es `mivm`, que es lo que emite el producto y lo que
        // pide el IDE en sus cuatro llamadas (FrmMain: buildProject/compileFile con
        // "mivm" a pelo). Estuvo en "jvm" por historia —contradiciendo al propio
        // Ctx.backend, que ya era "mivm"— y eso mandaba a quien compilaba a mano
        // con el jar a una RAMA OBSOLETA (ver la cabecera de JvmEmitter): salían
        // errores raros de una clase importada, y parecían un bug del compilador.
        // Eso fue #403. `--backend=jvm` sigue disponible para quien lo quiera.
        String backend = "mivm";
        boolean showTokens = true;
        boolean showAst    = true;
        boolean pruneBpi   = false;   // borra los .bpi del build al terminar (opt-in)
        // V8 #698 (JC, paso 1) — escribir BP desde el árbol, y la ida y vuelta (EscritorBp).
        boolean escribirBp  = false;
        boolean comprobarBp = false;
        boolean escribirJc  = false;   // V8 #698 (JC, paso 2) — AST → JC (EscritorJc), desde el árbol analizado

        for (int i = 0; i < args.length; i++) {
            String a = args[i];
            if ("--tokens".equals(a))      showAst    = false;
            else if ("--escribir-bp".equals(a))  { escribirBp = true;  showAst = false; showTokens = false; }
            else if ("--comprobar-bp".equals(a)) { comprobarBp = true; showAst = false; showTokens = false; }
            else if ("--escribir-jc".equals(a))  { escribirJc = true;  showAst = false; showTokens = false; }
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
            else if ("--dao".equals(a)) {
                // V5/H5 — `DAO build`: genera los DAOs de las entidades y PARA.
                // No compila. Es un acto aparte a propósito: el usuario hereda
                // del DAO generado, y regenerarlo dentro del build le
                // reescribiría por debajo la clase de la que cuelga su código.
                showAst = false; showTokens = false; daoBuild = true;
            }
            else if (a.startsWith("--stdlibDir=")) {   // V7/H2 (#523, V11): la stdlib dicha a mano
                STDLIB_DIR = a.substring("--stdlibDir=".length());
                if (!Files.isDirectory(Paths.get(STDLIB_DIR))) { System.err.println("--stdlibDir: no es un directorio: " + STDLIB_DIR); System.exit(1); return; }
            }
            else if (a.startsWith("--formato=")) {   // V7/H2 (#523): 8 | 9
                String f = a.substring("--formato=".length());
                if (!f.equals("8") && !f.equals("9")) { System.err.println("--formato desconocido: " + f + " (usa 8|9)"); System.exit(1); return; }
                FORMATO = Integer.parseInt(f);
            }
            else if (a.startsWith("--barajar=")) {   // V7/H2 (#523, V1): el guardián
                try { BARAJAR = Integer.parseInt(a.substring("--barajar=".length())); }
                catch (NumberFormatException ex) { System.err.println("--barajar necesita una semilla entera: " + a); System.exit(1); return; }
            }
            else if (a.startsWith("--relleno=")) {
                try { RELLENO = Integer.parseInt(a.substring("--relleno=".length())); }
                catch (NumberFormatException ex) { System.err.println("--relleno necesita un entero: " + a); System.exit(1); return; }
                if (RELLENO < 0) { System.err.println("--relleno no puede ser negativo"); System.exit(1); return; }
            }
            else if (a.startsWith("--backend=")) {
                backend = a.substring("--backend=".length()).toLowerCase();
                if (!backend.equals("jvm") && !backend.equals("mivm")) {
                    System.err.println("--backend desconocido: " + backend + " (usa jvm|mivm)");
                    System.exit(1); return;
                }
                // Que lo diga ANTES de compilar, no después con 20 errores raros.
                if (backend.equals("jvm")) {
                    System.err.println("-- aviso: el backend 'jvm' (.class) es una RAMA OBSOLETA y no se"
                            + " mantiene. No soporta clases de OTRO módulo: si tu programa usa una,"
                            + " saldrán errores del tipo \"no se puede llamar a 'X' sobre '<error>'\"."
                            + " El backend del producto es 'mivm' (.mod), que es el que ejecutan las"
                            + " dos VMs. --");
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
                // V5/H5 — avisos del proyecto (p.ej. `database` que no está).
                // No impiden compilar, pero no pueden pasar callados.
                for (String w : proj.warnings)
                    System.out.println("aviso proyecto: " + w);
                if (daoBuild) { System.exit(generarDaos(proj) ? 0 : 2); return; }
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
            System.err.println("     java -jar basicplus-frontend.jar <archivo.bp> --escribir-bp");
            System.err.println("     java -jar basicplus-frontend.jar <archivo.bp|carpeta> --comprobar-bp");
            System.err.println("     java -jar basicplus-frontend.jar <archivo.bp|carpeta> --escribir-jc");
            System.exit(1);
            return;
        }

        Path p = Paths.get(path).toAbsolutePath().normalize();
        if (!Files.exists(p)) {
            System.err.println("No se encuentra el archivo: " + p);
            System.exit(1);
            return;
        }

        // V8 #698 — sólo leer y escribir: ni semántico ni emisión.
        if (escribirBp || comprobarBp) {
            try {
                System.exit(comprobarBp ? EscritorBp.comprobar(p) : EscritorBp.escribirFichero(p));
            } catch (IOException ex) {
                System.err.println("error de I/O: " + ex.getMessage());
                System.exit(2);
            }
            return;
        }

        // V8 #698 (JC, paso 2) — el árbol analizado, escrito en JC: nada se emite.
        if (escribirJc) {
            try {
                System.exit(EscritorJc.escribirJc(p));
            } catch (IOException ex) {
                System.err.println("error de I/O: " + ex.getMessage());
                System.exit(2);
            }
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
        // 2a raiz de build. ESTE es el modo del incidente del 12-ago: 17 errores
        // de sintaxis y ni una pista de que el compilador era de otra epoca.
        Version.banner();
        Parsed parsed = parseAndPrint(p, ctx, /*isRoot*/true);
        if (parsed == null || parsed.module == null) return;
        if (!parsed.parser.getErrors().isEmpty()) return;

        SemanticAnalyzer analyzer = new SemanticAnalyzer();
        SemanticInfo info = analyzer.analyze(parsed.module);
        printSemantics(info, parsed.module);
        ctx.totalErrors += countSemErrors(info);
    }

    // ============================================================
    // V8 #698 (JC) — EL ÁRBOL ANALIZADO, PARA TRADUCIRLO
    // ============================================================
    /** Un fuente leído y analizado como para compilarlo, o el primer error. */
    static final class Analizado {
        Ast.ModuleNode modulo;
        SemanticInfo info;
        String error;
    }

    /**
     * El árbol de {@code src} y su análisis semántico, hechos con los mismos pasos que {@link #compileFull} antes de
     * emitir: las interfaces de los imports, los imports implícitos y el análisis. No emite nada. Lo que el compilador
     * va contando por la salida se tira, porque por la salida va el JC; los ficheros que haga falta escribir (las
     * interfaces de los imports que se compilan de su fuente) van a un temporal que se borra al acabar.
     *
     * <p>Lo usa el traductor a JC (`EscritorJc`): en BP crear, llamar y convertir se escriben igual, `X(…)`, y sólo el
     * análisis sabe qué es cada uno; y el tipo de una constante que no lo escribe también sale de aquí (V8_IDEAS §E2,
     * puntos 22, 23 y 29). Los imports que ponga el análisis (el de `Collections`, #450) se quitan después: el JC
     * lleva los del fuente.
     */
    static Analizado analizarParaTraducir(Path src) {
        Analizado r = new Analizado();
        java.io.PrintStream out = System.out, err = System.err;
        java.io.PrintStream mudo = new java.io.PrintStream(new java.io.ByteArrayOutputStream(), true);
        Path tmp = null;
        try {
            System.setOut(mudo);
            System.setErr(mudo);
            Ctx ctx = new Ctx();
            ctx.autodiscoverFromSource(src.toAbsolutePath());
            ctx.verbose = false;
            tmp = Files.createTempDirectory("bp-jc-");
            ctx.outDir = tmp;
            Parsed parsed = parseAndPrint(src, ctx, true);
            if (parsed == null || parsed.module == null) { r.error = "el fuente no se lee"; return r; }
            if (!parsed.lexer.getErrors().isEmpty())  { r.error = parsed.lexer.getErrors().get(0).toString(); return r; }
            if (!parsed.parser.getErrors().isEmpty()) { r.error = parsed.parser.getErrors().get(0).toString(); return r; }
            Ast.ModuleNode m = parsed.module;
            List<Ast.ImportNode> delFuente = new ArrayList<>(m.imports);
            for (Ast.ImportNode imp : delFuente) {
                ensureInterfaceForImport(imp, src, ctx, 1);
                if (imp.boundImpl != null) ensureInterfaceForBoundImpl(imp.boundImpl, imp.fromPath, m, src, ctx, 1);
            }
            SemanticAnalyzer analyzer = new SemanticAnalyzer();
            injectImplicitCoreImport(m);
            injectImplicitCollectionsImport(m);
            loadImportsForAnalyzer(m, src, ctx, analyzer, 1);
            if (ctx.totalErrors > 0) { r.error = "los imports no se cargan"; return r; }
            SemanticInfo info = analyzer.analyze(m);
            m.imports.clear();
            m.imports.addAll(delFuente);
            for (SemanticDiagnostic d : info.diagnostics)
                if (d.kind == SemanticDiagnostic.Kind.ERROR) { r.error = d.toString(); return r; }
            r.modulo = m;
            r.info = info;
            return r;
        } catch (IOException | RuntimeException ex) {
            r.error = "el análisis falló: " + ex;
            return r;
        } finally {
            System.setOut(out);
            System.setErr(err);
            borrarTemporal(tmp);
        }
    }

    private static void borrarTemporal(Path dir) {
        if (dir == null) return;
        try (java.util.stream.Stream<Path> w = Files.walk(dir)) {
            w.sorted(java.util.Comparator.reverseOrder()).forEach(f -> { try { Files.deleteIfExists(f); } catch (IOException ignored) { } });
        } catch (IOException ignored) {
            // un temporal que no se borra no es un error del análisis
        }
    }

    // ============================================================
    // COMPILACIÓN FULL: produce .mod + .bpi del módulo. Recursivamente
    // garantiza .bpi de cada import (modo INTERFACE) antes de analizar
    // y, tras emitir, recursivamente garantiza .mod de cada import.
    // ============================================================
    private static boolean compileFull(Path src, Ctx ctx, int depth) throws IOException {
        // QUIEN esta compilando (Eduardo, 12-ago). depth 0 = raiz del build, asi
        // que sale UNA vez y no una por modulo. Ver Version.java para el caso
        // que lo motiva; y ojo, hay otras DOS raices que tambien lo llaman
        // (compileInterface y runDiagnosticsOnly): si algun dia se anade un
        // cuarto modo de arranque, tiene que llevarlo tambien.
        if (depth == 0) Version.banner();
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
                        injectImplicitCollectionsImport(parsed.module);   // #450
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
            injectImplicitCoreImport(module);          // #248/#450 — ahora SIEMPRE
            injectImplicitCollectionsImport(module);   // #450 — solo si nombra SyncList/OwnerList
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
            if (depth == 0) {
                printSemantics(info, module);
                /* V5/H8 — ¿es EJECUTABLE lo que estamos construyendo? Lo dice
                 * tener `main`, no un campo del proyecto. Una librería no tiene
                 * por qué llevarlo (criterio de Eduardo), y así el manifest del
                 * pack no declara una entrada que no existe. */
                ctx.raizTieneMain = info.module != null && info.module.mainFunction != null;
                /* #365 — y CÓMO SE LLAMA esa raíz dentro del pack. Sólo de la que
                 * tiene main, y el primero gana: con varias raíces sólo una es el
                 * arranque, y machacarlo con la última compilada haría que el
                 * manifest apuntara a un módulo que no arranca nada. */
                if (ctx.raizTieneMain && ctx.raizEntry.isEmpty())
                    ctx.raizEntry = nombreCanonico(module.library, module.name);
            }
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
                try {
                    if (!emitMivmMod(module, info, ctx, depth, srcAbs)) return false;   // #521: sin .mod no hay build
                } catch (RuntimeException ex) {
                    // V10 (16-sep, E-10): lo que el emisor o la interfaz no pueden hacer se DICE
                    // («la base cross-module … no publica layout (bpi 9) — compila con --formato=9»),
                    // sin traza de Java, y cuenta como error del build.
                    indent(depth); System.err.println("error: " + ex.getMessage());
                    ctx.totalErrors++;
                    return false;
                }
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
            // V7/H2 (#523, V11) — en MOD9 no hay ranuras que hornear (Gui resuelve el
            // handler por nombre al cargar con __guiSlotOf, y el IDE sólo abría el
            // fichero como JSON): sólo se escribe con --formato=8.
            if (depth == 0 && FORMATO != 9) writeSlotsFile(module, info, ctx, depth);

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
                    // #390 — se pregunta por la RANURA, así que la condición es
                    // «¿es virtual?», no «¿es público?».
                    if (!fsym.esVirtual() || fsym.isStatic || fsym.isConstructor) continue;
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
        // 3a raiz de build (--interface). Mismo criterio que compileFull.
        if (depth == 0) Version.banner();
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
     *
     * De paquete, no privada: la llama {@code AotNativeEnMetodoTest} (#519).
     */
    static String validateNativeFunctionsAreAotable(Ast.ModuleNode module, SemanticInfo info) {
        // Ningún native → no hay nada que validar. #519 (V7): también los MÉTODOS native de
        // una clase, que el emisor aplana a funciones (V6/N1.1). Sin mirarlos, un módulo cuyas
        // únicas native eran métodos no se validaba, y un método que no se podía traducir
        // compilaba sin decir nada y corría interpretado.
        boolean hasNative = false;
        if (module.defs != null) {
            for (Ast.ITopLevelDecl d : module.defs) {
                if (d instanceof Ast.FuncDef) {
                    Ast.FuncDef f = (Ast.FuncDef) d;
                    if (f.isNative && !f.isIntrinsic) hasNative = true;
                } else if (d instanceof Ast.ClassDef) {
                    for (Ast.ITopLevelDecl m : ((Ast.ClassDef) d).members) {
                        if (!(m instanceof Ast.FuncDef)) continue;
                        Ast.FuncDef f = (Ast.FuncDef) m;
                        if (f.isNative && !f.isIntrinsic) { hasNative = true; break; }
                    }
                }
                if (hasNative) break;
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

    /** @return false si el emisor contó errores (#521: entonces NO hay .mod). */
    private static boolean emitMivmMod(Ast.ModuleNode module, SemanticInfo info, Ctx ctx, int depth,
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
        // V7/H2 (MOD8, #500) — la generación de este módulo y el sello de cada
        // dependencia contra la que se compiló (leído del .mod de disco, del pack, o
        // de la pasada de interfaz de un módulo de este mismo build).
        emitter.setGeneracion(ctx.gen);
        emitter.setFormato(ctx.formato);   // V7/H2 (#523)
        emitter.setVersionesDeps(ctx.versionesDeps);
        try {
            emitter.emitTo(ctx.outDir);
        } catch (RuntimeException ex) {
            // V10 (E-10): si el emisor reventó, lo que ya había anotado en `errors` es la causa
            // legible («parent cross-module sin layout (bpi 9) — compila con --formato=9»); la
            // excepción («Clase no declarada») es la consecuencia. Se dicen las dos.
            for (String e : emitter.errors) { indent(depth); System.err.println("  " + e); }
            throw ex;
        }
        String modFilename = (module.library != null && !module.library.isEmpty())
                ? module.library + "." + module.name + ".mod"
                : module.name + ".mod";
        // #521 (16-sep) — errores del emisor: NO hay .mod (emitTo no lo escribe y borra el
        // rancio), se dice «NO compilado», y cuentan en totalErrors (código de salida).
        if (!emitter.errors.isEmpty()) {
            indent(depth); System.err.printf("-- Errores del emisor mivm (%d) --%n", emitter.errors.size());
            for (String e : emitter.errors) { indent(depth); System.err.println("  " + e); }
            ctx.totalErrors += emitter.errors.size();
            indent(depth); System.err.println("NO compilado: " + ctx.outDir.toAbsolutePath().resolve(modFilename)
                    + " (" + emitter.errors.size() + " errores del emisor: no se escribe el .mod)");
            return false;
        }
        // El sello propio: el CRC que el ModWriter ESCRIBIÓ. Si un consumidor de este
        // build ya selló este módulo (pasada INTERFACE previa), tienen que coincidir;
        // si no, la interfaz cambió entre el análisis y la emisión — bug del
        // compilador, y se dice aquí, no en el cargador tres meses después.
        {
            String cacheKey = ifaceLib.isEmpty() ? module.name : ifaceLib + "." + module.name;
            int crcReal = emitter.getInterfaceCrc();
            int[] previo = ctx.versionesDeps.get(cacheKey);
            if (previo != null && previo[1] != crcReal) {
                // La evidencia al lado del .mod: las dos serializaciones, para diff.
                try {
                    ModuleInterface cachedIf = ctx.interfaceCache.get(cacheKey);
                    if (cachedIf != null) Files.write(ctx.outDir.resolve(cacheKey + ".iface-pasada.txt"), cachedIf.toBytes());
                    byte[] emitida = ModuleInterface.extractFrom(ifaceLib, module.name, module.isInterface,
                            module.implementsName, info.module, new ArrayList<>()).toBytes();
                    Files.write(ctx.outDir.resolve(cacheKey + ".iface-emision.txt"), emitida);
                } catch (IOException ignorada) { /* diagnóstico: si no se puede escribir, el error de abajo basta */ }
                indent(depth); System.err.printf(
                    "error: la interfaz de '%s' cambió entre la pasada de interfaz (crc %08x) y la emisión (crc %08x):"
                    + " bug del compilador — los módulos que la importan llevarían un sello equivocado%n",
                    cacheKey, previo[1], crcReal);
                ctx.totalErrors++;
            }
            ctx.versionesDeps.put(cacheKey, new int[]{ctx.gen, crcReal});
        }
        indent(depth); System.out.println("compilado: " + ctx.outDir.toAbsolutePath().resolve(modFilename));
        return true;
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
        byte[] ifaceBytes = iface.toBytes();
        ctx.interfaceCache.put(cacheKey, ModuleInterface.fromBytes(ifaceBytes, cacheKey));
        // V7/H2 (MOD8) — el sello de este módulo para quien lo importe en este build
        // (pasada INTERFACE: aún no hay .mod). Si la emisión ya lo selló, gana el suyo
        // (es el CRC de los bytes que de verdad están en el fichero).
        if (!ctx.versionesDeps.containsKey(cacheKey))
            ctx.versionesDeps.put(cacheKey, new int[]{ctx.gen, crc32De(ifaceBytes)});
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
        /* V6/#458 — UNA OMISIÓN POR TIPO SIN RESOLVER NO ES UNA OMISIÓN: ES UN FALLO.
         *
         * Aquí estaba la mitad de verdad del bug, y no la vi hasta reproducirlo por
         * tercera vez. Quitar el `import Core` implícito hace que el fallo salte en
         * la pasada COMPLETA... pero un módulo que se consume como dependencia se
         * compila antes en modo INTERFAZ, que es TOLERANTE a propósito: no reporta
         * errores. Así que seguía tirando el miembro en silencio y el consumidor
         * seguía diciendo «no tiene miembro de instancia 'todos'», lejos de la causa.
         *
         * La distinción que faltaba: omitir un miembro porque su tipo NO DEBE
         * exportarse es correcto; omitirlo porque su tipo NO RESOLVIÓ (`<error>`) es
         * un fallo, y encima uno que sólo se manifiesta en OTRO fichero. Se separan
         * por ahí, y el que no resuelve pasa a contar como error de verdad. */
        for (String s : skipped) {
            if (!s.contains("<error>")) continue;
            ctx.totalErrors++;
            indent(depth);
            System.err.printf("error: la interfaz de '%s' PIERDE '%s' porque su tipo no "
                    + "resuelve. El módulo compila, pero quien lo importe no verá ese "
                    + "miembro. Si el tipo es de `Core`, este módulo necesita "
                    + "`import Core`.%n", module.name, s);
        }
    }

    // ============================================================
    // RESOLUCIÓN DE DEPS (.bpi y .bp)
    // ============================================================
    private static void ensureInterfaceForImport(Ast.ImportNode imp, Path importerSrc, Ctx ctx, int depth) throws IOException {
        /* V7 #623 (8-oct) — `import v1 from pack "LVGL"` NO es un módulo: es la versión de la API de un pack nativo
         * (Ast.ImportNode#packNativoMarca), y no tiene interfaz que asegurar. Se buscaba como un módulo llamado `v1` y
         * salía «no se localizó .bp ni .bpi para import 'v1'; se omitirá», que el barrido de samples cuenta como un
         * import omitido (Gui2, al pasar a samples/guipack/; y SQLite). El recorrido del AOT ya lo saltaba. */
        if (imp.packNativoMarca != null) return;
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
        // V10 (16-sep) — un .mod que NO está en el outDir del build y cuyo FUENTE sí está en el
        // proyecto es un resto de otra compilación, no una entrada: su interfaz puede ser la de
        // otro compilador (la stdlib versionada, con CRLF de antes del 15-sep), y el módulo se
        // va a recompilar a outDir de todos modos. Sellar a los consumidores con el CRC del
        // resto y emitir luego otro era el «la interfaz cambió entre la pasada y la emisión»
        // en los 26 módulos de la stdlib. La interfaz sale de la fuente (pasada INTERFACE).
        if (preMod != null && bpSource != null && ctx.outDir != null && ctx.rootSrcDir != null
                && bpSource.toAbsolutePath().getParent().equals(ctx.rootSrcDir.toAbsolutePath())   // del PROYECTO (no una dep con fuente a mano)
                && !preMod.toAbsolutePath().getParent().equals(ctx.outDir.toAbsolutePath())) {
            indent(depth); System.out.printf("-- %s es un resto fuera de outDir: la interfaz de '%s' sale de su fuente --%n",
                    preMod.getFileName(), qualifiedName);
            preMod = null;
        }
        if (preMod != null && !isStale(preMod, bpSource)
                && readInterfaceCached(preMod, ctx) != null) {
            indent(depth); System.out.printf("-- interfaz de '%s' desde %s (embebida) --%n",
                    qualifiedName, preMod.getFileName());
            avisarSiTapaStdlib(qualifiedName, modName, preMod, ctx, depth);   /* V7/H2 (#493) */
            /* V5/H8 — apuntar de DÓNDE salió. Si este build acaba en un pack,
             * el pack se lleva ESTE fichero: el mismo contra el que se ha
             * compilado, no uno que se parezca. */
            anotarDependencia(ctx, qualifiedName, preMod, imp);
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

        // 2b) V5 — NADA SUELTO: ¿lo trae algún pack INSTALADO?
        //
        // El runtime, al cargar, mira el FS *y* la zona de packs. El compilador
        // sólo miraba los packs si el import decía cuál (`from pack X`). Esa
        // asimetría es la que obliga a dejar copias sueltas de un `.mod` que ya
        // viaja dentro del pack — y copiar a mano un artefacto es exactamente
        // como se llega a compilar contra uno y ejecutar otro.
        //
        // No adivina: si lo traen DOS, no elige — los nombra y pide `from pack`.
        //
        // Sólo para imports sin librería: `Lib.Modulo` tiene su propio esquema de
        // nombres y hoy ningún pack guarda entradas así. Cuando lo hagan, aquí es
        // donde se amplía, no en otro sitio.
        // `bpSource == null` no es un detalle: si el módulo tiene FUENTE aquí, es
        // que lo estás desarrollando, y entonces manda la tuya. El pack es el
        // último recurso, no el primero — al revés, tocar el .bp no cambiaría
        // nada y no habría forma de entender por qué.
        if (bpSource == null && library.isEmpty()) {
            java.util.List<Path> traen = packsQueTraen(moduleName, ctx);
            if (traen.size() == 1) {
                cargarInterfazDelPack(traen.get(0), traen.get(0).getFileName().toString(),
                                      moduleName, ifaceCacheKey, ctx, depth);
                return;
            }
            if (traen.size() > 1) {
                /* Con la RUTA de cada uno: el nombre a secas no basta para
                 * decidir, y si además coincidieran, el mensaje parecería roto. */
                StringBuilder cuales = new StringBuilder();
                for (Path p : traen) cuales.append("\n    ").append(p);
                throw new IOException("el módulo '" + moduleName + "' lo traen " + traen.size()
                        + " packs instalados:" + cuales
                        + "\n  Di cuál quieres: `import " + moduleName
                        + " from pack <NombreDelPack>`.");
            }
        }

        // 3) Compila la interfaz desde el .bp source.
        if (bpSource == null) {
            indent(depth); System.out.printf("-- no se localizó .bp ni .bpi para import '%s'; se omitirá --%n", qualifiedName);
            return;
        }
        avisarSiTapaStdlib(qualifiedName, modName, bpSource, ctx, depth);   /* V7/H2 (#493) */
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
        if (ctx.stdlibDir != null && Files.exists(ctx.stdlibDir.resolve(bpiName)))
            candidate = ctx.stdlibDir.resolve(bpiName);   // #467: la stdlib PRIMERO
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

    /**
     * V5/H8 — apunta una dependencia resuelta: de qué `.mod` salió, y si venía
     * de un pack, de cuál.
     *
     * <p>Se llama desde donde se RESUELVE, no desde un recorrido aparte: un
     * segundo recorrido puede no coincidir con el que compiló de verdad, y
     * entonces el pack declararía una cosa y llevaría otra.
     */
    private static void anotarDependencia(Ctx ctx, String nombre, Path mod,
                                          Ast.ImportNode imp) {
        if (mod != null) ctx.modsUsados.put(nombre, mod.toAbsolutePath().normalize());
        if (imp != null && imp.packName != null && !imp.packName.isEmpty())
            ctx.packsRequeridos.add(imp.packName);
    }

    /** [V7/H2, #493] EL COMPILADOR TAMBIÉN FABRICA LA SOMBRA, y hasta hoy en
     *  silencio: una `Str.bp` junto al programa más nueva que `bpstdlib/Str.mod`
     *  gana a la stdlib y deja `out/Str.mod` —justo lo que el IDE sube a /app—;
     *  un `import Str from "Str.mod"` gana a stdlibDir; una copia en outDir gana
     *  si no hay BpVM.cfg. Y la línea de log era la misma en todos los casos
     *  («desde Str.mod (embebida)»). No cambia lo que se elige —eso lo decide el
     *  orden de arriba—: lo DICE cuando lo elegido no es la stdlib y la stdlib
     *  tiene ese módulo, con la ruta real. Mismo criterio que el aviso de las
     *  VMs («gemelo tapado»). */
    private static void avisarSiTapaStdlib(String qualifiedName, String modName, Path elegido, Ctx ctx, int depth) {
        if (ctx.stdlibDir == null || elegido == null) return;
        Path enStdlib = ctx.stdlibDir.resolve(modName).toAbsolutePath().normalize();
        Path real = elegido.toAbsolutePath().normalize();
        if (real.equals(enStdlib) || real.startsWith(ctx.stdlibDir.toAbsolutePath().normalize())) return;
        if (!Files.isRegularFile(enStdlib)) return;
        indent(depth); System.out.printf("-- aviso: '%s' se toma de %s y NO de la stdlib (%s); si no es a propósito, quita esa copia --%n",
                qualifiedName, real, enStdlib);
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
        /* #467 — la stdlib PRIMERO: un módulo que vive en stdlibDir ES la stdlib, y
         * una copia rancia en outDir o junto al fuente no debe cortar la búsqueda.
         * samples/out tenía los 26 módulos de V5 (MOD6, del 21-ago) tapando a los
         * MOD7 de bpstdlib: «el módulo importado 'Math' no expone 'clamp'» × 25, con
         * el compilador y la stdlib al día. carriesInterface() sólo descarta los v5
         * SIN interfaz; un v6 rancio la lleva y pasaba. Es la misma regla que el IDE
         * aplica a las deps del device desde el 26-jun; aquí faltaba. */
        if (ctx.stdlibDir != null) {
            Path enStdlib = ctx.stdlibDir.resolve(modName);
            if (carriesInterface(enStdlib)) return enStdlib;
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
        ModLeido ml = leerMod(src);
        return (ml == null) ? null : ml.iface;
    }

    /** V7/H2 — lo que el compilador lee de la cabecera de un `.mod` ajeno: el magic,
     *  su generación y el CRC de su interfaz (MOD8; 0/0 en MOD6/MOD7) y los bytes de
     *  la sección interface (null si no la lleva). */
    static final class ModLeido {
        int magic, gen, ifaceCrc;
        byte[] iface;
    }

    static int crc32De(byte[] b) {
        java.util.zip.CRC32 c = new java.util.zip.CRC32(); c.update(b); return (int) c.getValue();
    }

    private static ModLeido leerMod(java.io.InputStream src) throws IOException {
        {
            java.io.DataInputStream in = new java.io.DataInputStream(src);
            int magic = in.readInt();
            // #457 — TAMBIÉN v7. Este `if` decía sólo V6 y era un bug MUDO: desde
            // que `N1.4` subió el formato a MOD7 (sección `native` embebida), el
            // compilador ESCRIBE v7 y aquí lo rechazaba devolviendo `null`, o sea
            // que cualquier módulo recompilado hoy se volvía invisible como
            // dependencia. El error no salía aquí sino lejísimos, en el consumidor,
            // como «identificador no resuelto: 'Math'».
            //
            // No mordía porque los 27 `.mod` de `bpstdlib/` seguían siendo v6 (se
            // generaron antes del salto) y porque los módulos que se compilan
            // JUNTOS resuelven su interfaz en memoria, sin pasar por disco. Muerde
            // en cuanto se regenera uno.
            // V7/H2 — y TAMBIÉN v8 (MOD8: gen + ifaceCrc detrás de nativeSize). Es el
            // mismo bug de #457 esperando a repetirse con cada subida: el lector de
            // cabecera del compilador tiene que aprenderla el mismo día que el escritor.
            // V7/H2 (#523) — y v9 (MOD9, enlace tardío): misma cabecera que v8.
            boolean v8 = (magic == edu.bpgenvm.bytecode.ModFormat.MAGIC_NUMBER_V8)
                      || (magic == edu.bpgenvm.bytecode.ModFormat.MAGIC_NUMBER_V9);
            boolean v7 = (magic == edu.bpgenvm.bytecode.ModFormat.MAGIC_NUMBER_V7) || v8;
            if (!v7 && magic != edu.bpgenvm.bytecode.ModFormat.MAGIC_NUMBER_V6) return null;
            ModLeido ml = new ModLeido();
            ml.magic = magic;
            in.readInt();                 // dataSize
            in.readInt();                 // mainOffset
            int importsSize = in.readInt();
            int exportsSize = in.readInt();
            in.readInt();                 // codeSize
            int librarySize = in.readInt();
            int interfaceSize = in.readInt();
            // El header v7 tiene UN entero más, `nativeSize`, justo detrás de
            // `interfaceSize` (ver ModFormat: HEADER_SIZE_V6=32 → V7=36). Hay que
            // consumirlo o las secciones se leen 4 bytes corridas. El v8, dos más.
            if (v7) in.readInt();         // nativeSize
            if (v8) { ml.gen = in.readInt(); ml.ifaceCrc = in.readInt(); }
            if (interfaceSize <= 0) return ml;
            // Orden de secciones: library, imports, exports, [interface], data,
            // code. La interfaz va tras exports → saltamos library+imports+exports.
            byte[] skip = new byte[librarySize + importsSize + exportsSize];
            in.readFully(skip);
            byte[] iface = new byte[interfaceSize];
            in.readFully(iface);
            ml.iface = iface;
            return ml;
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
        cargarInterfazDelPack(packFile, imp.packName, moduleName, ifaceCacheKey, ctx, depth);
    }

    /**
     * Saca la interfaz del módulo `moduleName` de un `.pack` YA LOCALIZADO.
     *
     * <p>Separado de {@link #loadInterfaceFromPack} porque hay dos maneras de
     * llegar aquí y sólo una de ellas sabe el nombre del pack por adelantado:
     * el `from pack X` lo trae escrito, y la búsqueda por módulo lo descubre.
     * De la localización en adelante el trabajo es idéntico —abrir, encontrar la
     * entrada, extraer la interfaz—, así que vive una vez. Si se hubiera copiado,
     * el día que cambie el formato del pack una de las dos vías se quedaría atrás
     * y sólo se notaría en la que menos se usa.
     */
    private static void cargarInterfazDelPack(Path packFile, String etiquetaPack,
                                              String moduleName, String ifaceCacheKey,
                                              Ctx ctx, int depth) throws IOException {
        basicplus.pack.PackReader.Pack pk;
        try {
            pk = basicplus.pack.PackReader.read(Files.readAllBytes(packFile));
        } catch (basicplus.pack.PackException e) {
            throw new IOException("pack '" + etiquetaPack + "' (" + packFile + ") ilegible: "
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
            throw new IOException("el pack '" + etiquetaPack + "' (" + packFile.getFileName()
                    + ") no contiene el módulo '" + want + "'. Módulos del pack: ["
                    + hay + "]");
        }
        ModLeido ml = leerMod(new java.io.ByteArrayInputStream(modBytes));
        byte[] ifaceBytes = (ml == null) ? null : ml.iface;
        if (ifaceBytes == null) {
            throw new IOException("el módulo '" + want + "' dentro del pack '" + etiquetaPack
                    + "' no lleva interfaz embebida (¿.mod de una versión anterior?); recompílalo");
        }
        ModuleInterface iface = ModuleInterface.fromBytes(ifaceBytes,
                packFile.getFileName() + "!" + want);
        ctx.interfaceCache.put(ifaceCacheKey, iface);
        ctx.versionesDeps.put(ifaceCacheKey, new int[]{ml.gen, crc32De(ifaceBytes)});   // V7/H2
        /* V5/H8 — el pack del que salió queda apuntado, venga de `from pack` o de
         * la búsqueda. Antes sólo se anotaba la vía del `.mod` suelto, así que un
         * proyecto que importara de un pack construía su manifest SIN el
         * `requires-pack=` correspondiente: el pack decía no necesitar nada y sí
         * lo necesitaba. Se apunta el nombre REAL del fichero, no el que escribió
         * el usuario, que puede diferir en mayúsculas o traer la extensión. */
        String nombrePack = packFile.getFileName().toString();
        if (nombrePack.endsWith(".pack")) nombrePack = nombrePack.substring(0, nombrePack.length() - 5);
        ctx.packsRequeridos.add(nombrePack);
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

    /** Las carpetas donde vive un pack, en orden: primero la BIBLIOTECA DE PACKS
     *  (la que fija el IDE, o `packsDir` del BpVM.cfg — la carpeta que
     *  distribuimos: stdlib, GUI, …), luego el `outDir` del proyecto (packs
     *  recién construidos) y los dirs de dependencias.
     *
     *  <p>UNA sola lista: la usan tanto
     *  el `from pack <nombre>` (que sabe cuál quiere) como la búsqueda por
     *  módulo (que no lo sabe). Si divergieran, un import encontraría un pack
     *  que el otro no, y eso es de los fallos que no se entienden mirando. */
    private static java.util.List<Path> dirsDePacks(Ctx ctx) {
        java.util.List<Path> dirs = new java.util.ArrayList<>();
        if (packsDirOverride != null) dirs.add(Paths.get(packsDirOverride));
        try {
            edu.bpgenvm.config.VmConfig cfg = edu.bpgenvm.config.VmConfig.loadDefaultFor(null);
            if (cfg.packsDir != null && !cfg.packsDir.isEmpty()) dirs.add(Paths.get(cfg.packsDir));
        } catch (Throwable ignored) { /* sin config: solo dirs del proyecto */ }
        if (ctx.outDir != null) dirs.add(ctx.outDir);
        dirs.addAll(ctx.dependencyPaths);
        return dirs;
    }

    /**
     * V5 — qué packs INSTALADOS traen el módulo `moduleName`. Devuelve TODOS los
     * que lo traen, no el primero.
     *
     * <p>Devolver todos y no el primero es la diferencia entre preguntar y
     * adivinar: con dos packs que traigan el mismo módulo, quedarse con el
     * primero elige por orden de carpeta —que el usuario no ve y no controla— y
     * el día que instale otro pack le cambia el programa sin tocar una línea.
     * Con la lista, el llamante puede decir «hay dos, dime cuál» y el usuario
     * desempata con `from pack`.
     *
     * <p>Recorre el índice de cada `.pack`, que es barato: la cabecera y la
     * tabla de entradas, sin tocar el contenido.
     */
    private static java.util.List<Path> packsQueTraen(String moduleName, Ctx ctx) {
        java.util.List<Path> hallados = new java.util.ArrayList<>();
        /* Por NOMBRE de fichero, no por ruta: el mismo pack puede estar copiado
         * en la biblioteca y en el outDir, y eso NO es una ambigüedad — es el
         * mismo pack encontrado dos veces. La lista de carpetas es un orden de
         * prioridad, así que gana la primera, igual que hace `locatePackFile`
         * cuando el import nombra el pack. Ambiguo es que lo traigan packs
         * DISTINTOS, que es lo único que el usuario tiene que desempatar.
         *
         * En minúsculas porque en Windows `SQLite.pack` y `sqlite.pack` son el
         * mismo fichero, y decir que hay dos sería mentira en esta máquina. */
        java.util.Set<String> vistos = new LinkedHashSet<>();
        for (Path d : dirsDePacks(ctx)) {
            if (d == null || !Files.isDirectory(d)) continue;
            java.util.List<Path> packs = new java.util.ArrayList<>();
            try (java.util.stream.Stream<Path> s = Files.list(d)) {
                s.filter(p -> p.getFileName().toString().endsWith(".pack"))
                 .forEach(packs::add);
            } catch (IOException ignored) { continue; }   /* carpeta ilegible: no es un error del usuario */
            java.util.Collections.sort(packs);            /* orden estable entre máquinas */
            for (Path p : packs) {
                Path abs = p.toAbsolutePath().normalize();
                if (!vistos.add(abs.getFileName().toString().toLowerCase())) continue;
                try {
                    basicplus.pack.PackReader.Pack pk =
                            basicplus.pack.PackReader.read(Files.readAllBytes(abs));
                    for (basicplus.pack.PackEntry e : pk.entries) {
                        if (!"mod".equals(e.tipo)) continue;
                        if (moduleName.equals(e.nombre) || (moduleName + ".mod").equals(e.nombre)) {
                            hallados.add(abs); break;
                        }
                    }
                } catch (Exception ignored) {
                    /* Un .pack ilegible NO puede tumbar la compilación de un
                     * programa que no lo usa: aquí sólo estamos mirando a ver
                     * quién trae el módulo. Si el usuario lo nombra a mano con
                     * `from pack`, ahí sí revienta con su motivo. */
                }
            }
        }
        return hallados;
    }

    private static Path locatePackFile(String packName, Ctx ctx, java.util.List<Path> tried) {
        String fname = packName.endsWith(".pack") ? packName : packName + ".pack";
        for (Path d : dirsDePacks(ctx)) {
            if (d == null) continue;
            Path p = d.resolve(fname);
            tried.add(p);
            if (Files.isRegularFile(p)) return p.toAbsolutePath().normalize();
        }
        return null;
    }

    /**
     * #387 — TODAS las firmas de un grupo de sobrecargas, empezando por la cabeza.
     *
     * <p>Las 2ª y siguientes NO están en el mapa del scope: cuelgan de la primera
     * por {@code nextOverload}. O sea que un bucle sobre {@code getSymbols()} —o
     * sobre {@code ns.functions.values()}— ve UNA firma por nombre y se salta las
     * demás sin decir nada. Quien recorra símbolos importados para TOCARLOS (no
     * sólo para mirarlos) tiene que pasar por aquí.
     *
     * <p>Lo que costó: los pases que resuelven los tipos de un import se saltaban
     * las sobrecargas, así que sus parámetros de clase se quedaban en
     * {@code UnresolvedClassRef} — que es «asignable desde todo» A PROPÓSITO, como
     * red para no soltar errores confusos si algo no se resolvió. Resultado:
     * {@code dao.load(9999)} con {@code load(long)} y {@code load(Medida)}
     * empataba a coste 1 y salía «llamada ambigua», cuando sólo una de las dos era
     * viable. Y sólo pasaba CROSS-MODULE: dentro del módulo los tipos ya vienen
     * resueltos del análisis.
     */
    private static java.util.List<Symbol.FunctionSymbol> firmasDe(Symbol.FunctionSymbol head) {
        java.util.List<Symbol.FunctionSymbol> out = new java.util.ArrayList<>();
        for (Symbol.FunctionSymbol f = head; f != null; f = f.nextOverload) out.add(f);
        return out;
    }

    /**
     * #392 — reparte los slots de vtable de una clase IMPORTADA. UNA sola
     * función, porque hay dos momentos en que se puede sembrar (al leer la
     * interfaz, o en el post-pass si la base vive en otro módulo) y antes eran
     * dos repartos distintos.
     *
     * <p>Precondición: los slots HEREDADOS ya están dentro de
     * {@code stub.externalMethodSlots} —los pone el llamante, que es quien sabe
     * de dónde vienen (otra clase del mismo .mod, los builtins de Object o de
     * Thread, o una base cross-module ya resuelta)— y ocupan 0..n-1. De ahí que
     * la siguiente ranura libre sea sencillamente el tamaño del mapa.
     *
     * <p>Las claves propias llegan EN ORDEN DE DECLARACIÓN y ya calculadas: una
     * clave que ya venía de la base es un OVERRIDE y conserva su ranura; una
     * nueva consume la siguiente. Y son claves, no nombres: una sobrecarga llega
     * MANGLEADA (regla H5.a: pelada la 1ª firma del grupo, mangleadas las
     * demás), que es justo lo que el reparto viejo del post-pass no sabía. Con
     * tres `buscar` en una clase hija, las dos mangleadas colisionaban en la
     * misma clave, no gastaban ranura, y todo lo declarado detrás quedaba
     * corrido: el dueño exportaba `despues` en el slot 10 y el importador lo
     * pedía en el 8 → fallo al ENLAZAR, con el .mod correcto en las dos manos.
     *
     * <p>Por eso recibe una lista de claves y no la {@code ClassSig}: quién es
     * la primera firma de cada grupo sólo lo sabe el bucle que construye los
     * símbolos, y esa cuenta no se puede rehacer aquí sin repetirla.
     */
    /** V7/H3 (21-sep) — la clave de ranura de cada método PROPIO de una clase importada se decide
     *  contra su BASE, que aquí ya está a mano (los dos llamantes acaban de enlazarla): la misma
     *  regla que el dueño aplica al compilar (`SemanticAnalyzer.assignKeysInChain`): misma firma
     *  heredada → override (su misma clave, su misma ranura); mismo nombre con OTRA firma →
     *  sobrecarga (clave mangleada, ranura NUEVA). Hasta hoy se decidía sólo contra los hermanos
     *  del stub, así que `TextFile.write(string)` sobre `File.write(byte[])` salía como override
     *  y todo lo declarado detrás quedaba corrido: el dueño exportaba `writeLine#21` y el
     *  consumidor pedía `#20` (MOD8, el corpus en el arnés; en MOD9 el cargador lo resuelve por
     *  nombre y no se veía). Reescribe `clavesPropias` con las claves definitivas. */
    private static void reclavarContraLaBase(Symbol.ClassSymbol stub, java.util.List<String> clavesPropias) {
        if (stub.baseClass == null) return;
        java.util.Map<String, String> cambio = new java.util.HashMap<>();
        for (Symbol s : new java.util.ArrayList<>(stub.instanceMembers.getSymbols())) {
            if (!(s instanceof Symbol.FunctionSymbol)) continue;
            Symbol.FunctionSymbol head = (Symbol.FunctionSymbol) s;
            java.util.Set<String> taken = new java.util.HashSet<>();
            java.util.Map<String, String> inheritedBySig = new java.util.LinkedHashMap<>();
            for (Symbol.ClassSymbol a = stub.baseClass; a != null; a = a.baseClass) {
                Symbol as = a.instanceMembers.tryLookup(head.name);
                if (!(as instanceof Symbol.FunctionSymbol)) continue;
                for (Symbol.FunctionSymbol o = (Symbol.FunctionSymbol) as; o != null; o = o.nextOverload) {
                    taken.add(o.slotKey());
                    inheritedBySig.putIfAbsent(o.signatureKey(), o.slotKey());
                }
            }
            if (taken.isEmpty()) continue;   // nada heredado con ese nombre: la regla local ya era la buena
            for (Symbol.FunctionSymbol f = head; f != null; f = f.nextOverload) {
                if (f.isStatic || f.isConstructor) continue;
                String vieja = f.slotKey();
                String inherited = inheritedBySig.get(f.signatureKey());
                if (inherited != null) f.slotKey = inherited;
                else if (!taken.contains(f.name)) f.slotKey = f.name;
                else f.slotKey = f.overloadMangle();
                taken.add(f.slotKey);
                if (!vieja.equals(f.slotKey())) cambio.put(vieja, f.slotKey());
            }
        }
        if (cambio.isEmpty()) return;
        for (int i = 0; i < clavesPropias.size(); i++) {
            String nueva = cambio.get(clavesPropias.get(i));
            if (nueva != null) clavesPropias.set(i, nueva);
        }
    }

    private static int sembrarSlotsDeVtable(Symbol.ClassSymbol stub,
                                            java.util.List<String> clavesPropias,
                                            String etiqueta) {
        reclavarContraLaBase(stub, clavesPropias);   // V7/H3
        int next = stub.externalMethodSlots.size();
        for (String clave : clavesPropias) {
            if (!stub.externalMethodSlots.containsKey(clave)) {
                stub.externalMethodSlots.put(clave, next++);
            }
        }
        return cruzarSlots(stub, clavesPropias, etiqueta);
    }

    /**
     * #393 — CROSS-CHECK DEL IMPORTADOR. El dueño ya tiene el suyo (#299, «no se
     * publica un layout que miente»); el importador reconstruía los slots por su
     * cuenta y NADIE comprobaba el resultado.
     *
     * <p>La invariante es «lo que calculé es lo que sembré»: cada clave de la
     * lista tiene que haber acabado en el mapa, y dos claves distintas no pueden
     * compartir ranura. Eso es EXACTAMENTE lo que fallaba en #392: las claves
     * mangleadas se calculaban bien, pero el reparto las ignoraba y nunca las
     * metía — el llamante pedía el slot 8 y el dueño lo tenía en el 10, y no se
     * sabía hasta ENLAZAR, con un mensaje que sugería un `.mod` rancio que no
     * existía.
     *
     * <p><b>Lo que este guardián NO puede comprobar</b>, y conviene tenerlo
     * escrito: que el NÚMERO coincida con el del dueño. Comparar contra
     * {@code binaryNumMethods} sería lo natural y está MAL: un {@code function
     * event} PRIVADO ocupa ranura (la regla del emisor es
     * {@code isPublic || isEventHandler}) pero no se exporta, así que el
     * importador cuenta legítimamente menos. Medido: una clase con un manejador
     * privado publica 4 y el importador reconstruye 3. Con esa comparación el
     * guardián saltaría en toda clase que maneje eventos — o sea, en todas las de
     * GUI. Para comprobar el número haría falta que la interfaz publicase el slot
     * de cada método, que es cambio de formato (ver la ficha).
     *
     * @return número de incoherencias encontradas (0 = todo bien).
     */
    static int cruzarSlots(Symbol.ClassSymbol stub,
                                   java.util.List<String> clavesPropias,
                                   String etiqueta) {
        int fallos = 0;
        for (String clave : clavesPropias) {
            if (!stub.externalMethodSlots.containsKey(clave)) {
                System.err.printf(
                        "-- error interno del compilador: al leer la clase importada '%s' no"
                      + " se ha sabido colocar el método '%s' en la tabla de métodos de la"
                      + " clase.%n"
                      + "   No es un fallo de tu programa, ni un fichero desactualizado. Si"
                      + " la compilación siguiera, esa llamada acabaría en el método"
                      + " equivocado. --%n",
                        etiqueta, clave);
                fallos++;
            }
        }
        // Dos claves distintas en la misma ranura = una tapa a la otra.
        java.util.Map<Integer, String> porSlot = new java.util.HashMap<>();
        for (java.util.Map.Entry<String, Integer> e : stub.externalMethodSlots.entrySet()) {
            String otra = porSlot.put(e.getValue(), e.getKey());
            if (otra != null) {
                System.err.printf(
                        "-- error interno del compilador: al leer la clase importada '%s',"
                      + " los métodos '%s' y '%s' han quedado en la MISMA posición (%d) de"
                      + " la tabla de métodos.%n"
                      + "   No es un fallo de tu programa. Uno de los dos taparía al otro. --%n",
                        etiqueta, otra, e.getKey(), e.getValue());
                fallos++;
            }
        }
        return fallos;
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
            ModLeido ml;
            try (java.io.InputStream raw = new java.io.BufferedInputStream(Files.newInputStream(path))) {
                ml = leerMod(raw);
            }
            if (ml == null || ml.iface == null) return null;   // .mod v5 sin interfaz
            iface = ModuleInterface.fromBytes(ml.iface, path.toString());
            ctx.versionesDeps.put(key, new int[]{ml.gen, crc32De(ml.iface)});   // V7/H2: el sello de la dependencia
        } else {
            iface = ModuleInterface.readFrom(path);
            // Un .bpi suelto (vía muerta desde H6.a) no tiene cabecera: sin sello.
        }
        ctx.interfaceCache.put(key, iface);
        return iface;
    }

    // Package-private (#212): AotMain lo reutiliza para resolver imports antes de
    // emitir AOT, igual que compileFull aquí. Carga las .bpi disponibles en el
    // analizador (no compila deps; deben existir ya).
    /** #542 — un import que falta, contado como ERROR (sólo con {@link Ctx#faltaImportEsError}). */
    private static void faltaImport(Ctx ctx, int depth, String msg) {
        ctx.totalErrors++;
        ctx.importsQueFaltan.add(msg);
        indent(depth); System.err.println("error: " + msg);
    }

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
            // V7/H6 G5.b0 (26-sep, el resto de #542) — `import v1 from pack "SQLI"` NO es un módulo:
            // es la versión de la API que la placa publica con esa marca, y sus funciones son las
            // `native` externas de ESTE módulo (el AOT les emite el puente). No hay interfaz que
            // buscar. Hasta #542 se buscaba igual y sólo avisaba («sin interfaz para import 'v1'»);
            // desde #542, en el AOT, era un error — y SQLite.bp y Gui2.bp se quedaban sin .mdn.
            if (imp.packNativoMarca != null) continue;
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
                    if (ctx.faltaImportEsError) {   // #542: en el AOT, error de carga con su nombre
                        faltaImport(ctx, depth, String.format(
                                "falta el modulo '%s': ni '%s' ni su .mod en la stdlib (%s) ni junto al fuente (%s)"
                                + " — ¿BpVM.cfg o --stdlibDir?, ¿compilado antes?",
                                alias, bpiName, ctx.stdlibDir, importerDir));
                        continue;
                    }
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
                    if (ctx.faltaImportEsError) {   // #542: en el AOT, error de carga con su nombre
                        faltaImport(ctx, depth, String.format(
                                "el modulo '%s' no expone una interfaz legible ('%s')", alias, bpi));
                        continue;
                    }
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
                    if (fs.inlineText != null) f.inlineOps = Ast.InlineStmt.deTexto(fs.inlineText);   // H1 (20-sep)
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
                    // V10 (16-sep, H6) — de qué módulo es: sin esto la clave canónica lo cualificaba
                    // con el módulo LOCAL (`EMid/Color;` por `ELib/Color;`) y un override con un enum
                    // ajeno en la firma no casaba nunca (ranura nueva, en silencio).
                    e.externalLibrary = ns.library;
                    e.externalModule  = ns.moduleName;
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
                    // #392 — claves de vtable PROPIAS de esta clase, en orden de
                    // declaración: get/set de cada property, y luego cada método
                    // con su slotKey (pelada la 1ª firma de un grupo, MANGLEADA
                    // las siguientes). Se acumulan aquí porque hay DOS momentos
                    // en que se puede sembrar la vtable —ahora, o en el post-pass
                    // si la base vive en otro módulo— y las reparte UNA función.
                    java.util.List<String> clavesPropias = new java.util.ArrayList<>();
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
                    boolean basePendiente = false;
                    if (baseStub != null) for (Object[] pe : pendingCrossBase) if (pe[1] == baseStub) { basePendiente = true; break; }
                    if (baseStub != null && basePendiente) {
                        // H1 P3 (20-sep) — la base es de ESTE módulo pero está DIFERIDA (su propia base
                        // aún no ha pasado por aquí: la interfaz no va en orden de declaración, y desde
                        // que toda cadena acaba en Core.Object el caso se da): copiar ahora sus ranuras
                        // sería copiar una tabla vacía y numerar las propias desde 0 («no exporta
                        // 'Core.Integer#value#0'», MOD8). Se difiere también; el post-pass las encadena.
                        deferSlots = true;
                    } else if (baseStub != null) {
                        // #248 — enlazar la base también para el LOOKUP de
                        // miembros heredados (e.msg sobre el stub hijo).
                        stub.baseClass = baseStub;
                        stub.externalMethodSlots.putAll(baseStub.externalMethodSlots);
                    } else if (cs.baseClassName == null) {
                        // H1 P3 (20-sep) — sin base en la interfaz = `root class` (Object, Thread,
                        // en Core): no hereda nada. Las demás traen SIEMPRE su base, cualificada
                        // si es de otro módulo (`Core.Object`), y van por el defer de abajo.
                    } else {
                        // #248 (cerraba el gap anotado de #44): base en OTRO
                        // módulo (`extends Core.Exception`). Aquí su ns puede no
                        // estar cargado aún → difiere baseClass + siembra de
                        // slots al post-pass L2 v3.e, con todos los ns presentes.
                        // Sin esto los métodos propios numeraban desde 0 →
                        // INVOKE_VIRTUAL a la ranura equivocada (getCode → toString).
                        deferSlots = true;
                    }
                    // V7/H2 (#492-B) — CAMPOS de instancia, con su ranura. Un hijo de
                    // otro módulo los ve como si estuvieran en el suyo: checkVisibility
                    // aplica protegido/privado, y el emisor accede por externalSlot.
                    for (ModuleInterface.FieldSig f : cs.fields) {
                        Symbol.VarSymbol vsym = new Symbol.VarSymbol(f.name, false, false, stub, false, 0, 0);
                        vsym.type = f.type;
                        vsym.isPrivate = f.isPrivate;
                        vsym.isOwner = f.isOwner;
                        vsym.externalSlot = f.slot;
                        stub.instanceMembers.tryDefine(vsym);
                    }
                    // Properties → getter (slot N) + setter (slot N+1)
                    for (ModuleInterface.PropSig p : cs.properties) {
                        // #316 — respeta la visibilidad declarada: una property NO
                        // pública se importa igualmente (sus accesores ocupan slots
                        // y hay que contarlos), pero marcada como no-pública ⇒
                        // checkVisibility sigue prohibiendo el acceso desde fuera y
                        // permitiéndoselo a las subclases (que es su razón de ser).
                        Symbol.PropertySymbol psym = new Symbol.PropertySymbol(
                                p.name, p.isPublic, p.isFinal, false, stub, null);   // V7/H2: final viaja
                        psym.type = p.type;
                        psym.isPrivate = p.isPrivate;   // V7/H2 (bpi 8)
                        stub.instanceMembers.tryDefine(psym);
                        String capName = Character.toUpperCase(p.name.charAt(0)) + p.name.substring(1);
                        clavesPropias.add("get" + capName);
                        clavesPropias.add("set" + capName);
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
                        // #390 — la visibilidad DECLARADA, no un `true` a pelo: un
                        // método no público se importa igual (la hija tiene que
                        // saber que existe) y es checkVisibility quien decide quién
                        // puede llamarlo, exactamente como con las properties.
                        Symbol.FunctionSymbol fsym = new Symbol.FunctionSymbol(
                                m.name, m.isPublic, false, false, stub, null);
                        fsym.returnType = m.returnType;
                        fsym.isFinal = m.isFinal;   // V7/H2 (bpi 8): el hijo de otro módulo tampoco lo sobreescribe
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
                        clavesPropias.add(fsym.slotKey());
                    }
                    // La vtable se siembra AHORA si la base ya está a mano; si vive
                    // en otro módulo, en el post-pass —que necesita todos los
                    // namespaces cargados—, pero con la MISMA función y la MISMA
                    // lista de claves. Ahí estaba #392: eran dos repartos distintos
                    // y el del post-pass numeraba por el nombre pelado, así que las
                    // sobrecargas de una clase HIJA no gastaban ranura y todo lo
                    // declarado detrás quedaba corrido.
                    if (deferSlots) pendingCrossBase.add(new Object[]{ns, stub, cs, clavesPropias});
                    else ctx.totalErrors += sembrarSlotsDeVtable(stub, clavesPropias,
                                                ns.moduleName + "." + cs.name);
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
                    // V7/H2 (#492-B) — MÉTODOS y PROPERTIES ESTÁTICOS del stub. Van a
                    // staticMembers (el semántico resuelve `Mod.Cls.x` con lookupStatic)
                    // y NO gastan ranura de vtable: no entran en clavesPropias. Sin
                    // isExternal: la rama de funciones de módulo del emisor ganaría y
                    // llamaría a `Mod.cuantos` (sin clase); el emisor decide por
                    // ownerClass.isExternal, como el `super` cross-module (#451), y
                    // llama al alias plano `__cls_s_<Cls>_<clave>`. La clave de
                    // sobrecarga se calcula como en el dueño (overloadMangle); no se
                    // guarda ningún alias en slotKey porque assignSlotKeys lo recalcula.
                    for (ModuleInterface.FuncSig m : cs.staticMethods) {
                        Symbol.FunctionSymbol fsym = new Symbol.FunctionSymbol(
                                m.name, m.isPublic, m.isFinal, true, stub, null);
                        fsym.returnType = m.returnType;
                        for (ModuleInterface.ParamSig pp : m.params) {
                            Symbol.ParamSymbol psm = new Symbol.ParamSymbol(pp.name, 0, 0);
                            psm.type = pp.type;
                            if (pp.defaultValue != null)
                                psm.defaultExpr = SemanticAnalyzer.literalExprFromValue(pp.defaultValue, pp.type, 0, 0);
                            fsym.params.add(psm);
                        }
                        Symbol prevS = stub.staticMembers.tryLookup(m.name);
                        if (prevS instanceof Symbol.FunctionSymbol) {
                            Symbol.FunctionSymbol headS = (Symbol.FunctionSymbol) prevS;
                            Symbol.FunctionSymbol tailS = headS;
                            while (tailS.nextOverload != null) tailS = tailS.nextOverload;
                            tailS.nextOverload = fsym;
                            headS.overloaded = true;
                            fsym.overloaded  = true;
                            fsym.slotKey = fsym.overloadMangle();
                        } else {
                            stub.staticMembers.tryDefine(fsym);
                            fsym.slotKey = fsym.name;
                        }
                    }
                    for (ModuleInterface.PropSig p : cs.staticProps) {
                        Symbol.PropertySymbol psym = new Symbol.PropertySymbol(
                                p.name, p.isPublic, p.isFinal, true, stub, null);   // isStatic=true
                        psym.type = p.type;
                        psym.isPrivate = p.isPrivate;
                        stub.staticMembers.tryDefine(psym);
                    }

                    // Constructor: como FunctionSymbol cross-module. Lo expone-
                    // mos en realidad vía la factoría `__cls_new_<Cls>`, pero
                    // para el typecheck necesitamos el FunctionSymbol del ctor
                    // con los params correctos. El emisor cuando ve construc-
                    // ción de clase externa cambia el CALL al de la factoría.
                    if (cs.ctorParams != null) {
                        Symbol.FunctionSymbol ctor = new Symbol.FunctionSymbol(
                                cs.name, "public".equals(cs.ctorVis), false, false, stub, null);
                        ctor.isConstructor = true;
                        ctor.isPrivate = "private".equals(cs.ctorVis);   // #515: la marca viaja en `ctor(...) private|protected`
                        for (ModuleInterface.ParamSig pp : cs.ctorParams) {
                            Symbol.ParamSymbol psm = new Symbol.ParamSymbol(pp.name, 0, 0);
                            psm.type = pp.type;
                            if (pp.defaultValue != null)   // H8.1
                                psm.defaultExpr = SemanticAnalyzer.literalExprFromValue(pp.defaultValue, pp.type, 0, 0);
                            ctor.params.add(psm);
                        }
                        stub.constructor = ctor;
                        stub.instanceMembers.tryDefine(ctor);
                        ctor.slotKey = ctor.name;
                        // V7/H2 (#509) — las demás sobrecargas, ENCADENADAS como los
                        // métodos (H5.a-E5): la clave mangleada sale del mismo
                        // overloadMangle que en el dueño, así `__cls_new_<Cls>$clave`
                        // y `__cls_init_<Cls>$clave` casan a los dos lados.
                        Symbol.FunctionSymbol tailC = ctor;
                        for (int k = 0; k < cs.ctorOverloads.size(); k++) {
                            List<ModuleInterface.ParamSig> ov = cs.ctorOverloads.get(k);
                            String visK = (k < cs.ctorOverloadsVis.size()) ? cs.ctorOverloadsVis.get(k) : "public";
                            Symbol.FunctionSymbol oc = new Symbol.FunctionSymbol(
                                    cs.name, "public".equals(visK), false, false, stub, null);
                            oc.isConstructor = true;
                            oc.isPrivate = "private".equals(visK);   // #515
                            for (ModuleInterface.ParamSig pp : ov) {
                                Symbol.ParamSymbol psm = new Symbol.ParamSymbol(pp.name, 0, 0);
                                psm.type = pp.type;
                                if (pp.defaultValue != null)
                                    psm.defaultExpr = SemanticAnalyzer.literalExprFromValue(pp.defaultValue, pp.type, 0, 0);
                                oc.params.add(psm);
                            }
                            tailC.nextOverload = oc;
                            tailC = oc;
                            ctor.overloaded = true;
                            oc.overloaded = true;
                            oc.slotKey = oc.overloadMangle();
                        }
                    }
                    // Resolver UnresolvedClassRef en params/return de métodos
                    // y del ctor, contra el propio stub o contra otros stubs
                    // ya añadidos al ns. Lo hacemos AL FINAL del bucle de
                    // clases para que todos los stubs estén creados.
                    ns.classes.put(cs.name, stub);
                }
                // 2nd pass: resolver UnresolvedClassRef contra stubs creados.
                for (Symbol.ClassSymbol stub : ns.classes.values()) {
                    // V7/H2 (#492-B) — los estáticos también: `function Base.crea(): Base`
                    // o `property Base.actual: Otra` llegan como UnresolvedClassRef.
                    List<Symbol> miembros = new ArrayList<>(stub.instanceMembers.getSymbols());
                    miembros.addAll(stub.staticMembers.getSymbols());
                    for (Symbol mem : miembros) {
                        if (mem instanceof Symbol.FunctionSymbol) {
                            // #387 — la cadena ENTERA: las sobrecargas no están en el mapa.
                            for (Symbol.FunctionSymbol f : firmasDe((Symbol.FunctionSymbol) mem)) {
                                f.returnType = resolveTypeAgainst(f.returnType, ns);
                                for (Symbol.ParamSymbol p : f.params)
                                    p.type = resolveTypeAgainst(p.type, ns);
                            }
                        } else if (mem instanceof Symbol.PropertySymbol) {
                            Symbol.PropertySymbol p = (Symbol.PropertySymbol) mem;
                            p.type = resolveTypeAgainst(p.type, ns);
                        } else if (mem instanceof Symbol.VarSymbol) {
                            // V7/H2 (#492-B) — los campos importados también: `var otro: Base`
                            // llega como UnresolvedClassRef y sin esto «el tipo 'Base' no tiene miembros».
                            Symbol.VarSymbol vv = (Symbol.VarSymbol) mem;
                            vv.type = resolveTypeAgainst(vv.type, ns);
                        } else if (mem instanceof Symbol.EventSymbol) {
                            // V10 (16-sep, H2/V10-EV-1) — los parámetros de un EVENTO también: con una
                            // clase en la firma, la clave canónica no resolvía y ningún consumidor MOD9
                            // podía suscribirse (la GUI entera: onClick(Component)).
                            for (Symbol.ParamSymbol p : ((Symbol.EventSymbol) mem).params)
                                p.type = resolveTypeAgainst(p.type, ns);
                        }
                    }
                }
                analyzer.registerImport(ns);
                loadedNs.add(ns);
                // V6/#458 — si el que se acaba de cargar es Core, apuntamos sus
                // nombres de clase para que el mensaje de un nombre sin resolver
                // pueda decir «`List` vive en `Core`». Se cogen de aquí y no de una
                // lista escrita a mano: una copia se queda rancia. El conjunto es
                // estático y dura toda la compilación, así que basta con que UN
                // módulo del proyecto importe Core — y tras la migración lo hacen
                // casi todos los de la stdlib.
                if ("Core".equals(ns.moduleName))
                    SemanticAnalyzer.registrarNombresDeCore(ns.classes.keySet());
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
                    if (base == null) continue;   // se avisa al salir del bucle (abajo)
                    // Si la base es a su vez pendiente y aún no sembrada, espera
                    // a la siguiente ronda (sus slots todavía no están).
                    boolean baseStillPending = false;
                    for (Object[] p2 : pend) {
                        if (p2[1] == base) { baseStillPending = true; break; }
                    }
                    if (baseStillPending) continue;
                    stub.baseClass = base;
                    stub.externalMethodSlots.putAll(base.externalMethodSlots);
                    @SuppressWarnings("unchecked")
                    java.util.List<String> clavesPropias = (java.util.List<String>) e[3];
                    ctx.totalErrors += sembrarSlotsDeVtable(stub, clavesPropias,
                                            ownNs.moduleName + "." + cs.name);
                    it.remove();
                    progress = true;
                }
                if (!progress) break;
            }
            // 14-sep — el aviso estaba dentro del bucle condicionado a `round == 7`, y
            // sin progreso el bucle salía en la ronda 0: nunca se imprimía. Lo que
            // quedaba era una clase con base desconocida, sin ctor ni métodos
            // heredados, y errores que no decían por qué («no define constructor»).
            for (Object[] e : pend) {
                Symbol.ImportedNamespaceSymbol ownNs = (Symbol.ImportedNamespaceSymbol) e[0];
                ModuleInterface.ClassSig cs = (ModuleInterface.ClassSig) e[2];
                indent(depth); System.err.printf(
                    "-- aviso: base cross-module '%s' de la clase importada '%s.%s' no resuelta"
                    + " (¿falta el import de su módulo?) — sin sus miembros heredados --%n",
                    cs.baseClassName, ownNs.moduleName, cs.name);
            }
        }

        // L2 v3.e — post-pass: resuelve UnresolvedClassRef con nombre dotted
        // (e.g. `L2Lib.Counter`) contra otros namespaces ya cargados. Hace
        // esta pasada DESPUÉS del loop principal para que todos los ns estén
        // disponibles. Sólo toca refs que no se resolvieron contra el propio
        // ns en el primer pass.
        for (Symbol.ImportedNamespaceSymbol ns : loadedNs) {
            for (Symbol.FunctionSymbol head : ns.functions.values()) {
                // #387 — idem con las funciones LIBRES: el mapa guarda la 1ª firma.
                // V10 (H7): primero contra su PROPIO ns (`load(Medida)` nombra la clase del módulo sin
                // cualificar), luego los demás por nombre cualificado; nunca «el primer ns que la tenga».
                for (Symbol.FunctionSymbol fn : firmasDe(head)) {
                    fn.returnType = resolveCrossModuleType(resolveTypeAgainst(fn.returnType, ns), loadedNs);
                    for (Symbol.ParamSymbol p : fn.params) {
                        p.type = resolveCrossModuleType(resolveTypeAgainst(p.type, ns), loadedNs);
                    }
                    exigirEnumsResueltos(ns, fn.name, fn.params, fn.returnType, analyzer);   // #516
                }
            }
            for (Symbol.ClassSymbol stub : ns.classes.values()) {
                List<Symbol> miembros = new ArrayList<>(stub.instanceMembers.getSymbols());
                miembros.addAll(stub.staticMembers.getSymbols());   // V7/H2 (#492-B)
                for (Symbol mem : miembros) {
                    if (mem instanceof Symbol.FunctionSymbol) {
                        for (Symbol.FunctionSymbol f : firmasDe((Symbol.FunctionSymbol) mem)) {
                            f.returnType = resolveCrossModuleType(f.returnType, loadedNs);
                            for (Symbol.ParamSymbol p : f.params)
                                p.type = resolveCrossModuleType(p.type, loadedNs);
                            exigirEnumsResueltos(ns, stub.name + "." + f.name, f.params, f.returnType, analyzer);   // #516
                        }
                    } else if (mem instanceof Symbol.PropertySymbol) {
                        Symbol.PropertySymbol p = (Symbol.PropertySymbol) mem;
                        p.type = resolveCrossModuleType(p.type, loadedNs);
                    } else if (mem instanceof Symbol.VarSymbol) {
                        // V7/H2 — un campo con tipo de OTRO namespace (`var x: OtraLib.Foo`)
                        // sólo lo resolvía la 1ª pasada, que no ve otros namespaces.
                        Symbol.VarSymbol vv = (Symbol.VarSymbol) mem;
                        vv.type = resolveCrossModuleType(vv.type, loadedNs);
                    } else if (mem instanceof Symbol.EventSymbol) {   // V10 (H2/H3)
                        for (Symbol.ParamSymbol p : ((Symbol.EventSymbol) mem).params)
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
    /* H1 P2/P3 — `cargarRaiz` y `tipoDeRaiz` ya no existen: el parser pone `import Core` en todo
     * módulo y el semántico registra las funciones públicas de ese namespace sin cualificar
     * (SemanticAnalyzer.registrarRaiz), con los stubs de ESTE módulo — así `Object[]` de la
     * firma y `Object[]` del programa son el mismo tipo sin resolver nada aparte. */

    static void injectImplicitCoreImport(Ast.ModuleNode module) {
        if (module == null || "Core".equals(module.name)) return;
        if (module.imports != null) {
            for (Ast.ImportNode imp : module.imports) {
                if (imp.path != null && !imp.path.isEmpty()
                        && "Core".equals(imp.path.get(imp.path.size() - 1))) return;
            }
        }
        if (module.imports == null) return;   // defensivo: el parser siempre crea la lista

        /* V6/#458 — YA NO SE INYECTA NADA. Norma de Eduardo (30-ago):
         *
         *     «la norma tiene que ser sencilla: si se utiliza un tipo de Core, se
         *      ha de importar Core».
         *
         * Por qué se quita, y no es cosmética: el `import Core` sintético lo metía
         * la pasada COMPLETA y no la de INTERFAZ, así que las dos veían cosas
         * distintas del mismo fichero. Un módulo que exponía un tipo de Core en una
         * firma pública compilaba entero y, al ser importado por otro, PERDÍA ese
         * miembro de su interfaz (`retorno tipo no exportable: <error>`) — y el
         * error salía en el consumidor, lejísimos de la causa.
         *
         * Se podía haber enseñado a la pasada de interfaz a inyectarlo también. La
         * norma es mejor: en vez de que las dos pasadas adivinen igual, no hay nada
         * que adivinar. Y de paso un módulo que no usa Core deja de arrastrarlo.
         *
         * ⚠️ El método se queda (no se borra la llamada) porque sigue haciendo lo
         * único que importa ahora: NO tocar el AST. Dejar aquí el porqué vale más
         * que el hueco que dejaría quitarlo. */
    }

    /** #450 — `import Collections` implicito si el modulo NOMBRA `SyncList` u
     *  `OwnerList` como tipo. Aqui si es una condicion y no un «siempre»: son
     *  clases de coleccion, no del nucleo, y quien no las usa no debe cargarlas.
     *  Un falso negativo no corrompe nada — da un «identificador no resuelto» en
     *  compilacion, que se ve. */
    static void injectImplicitCollectionsImport(Ast.ModuleNode module) {
        if (module == null || "Collections".equals(module.name)) return;
        if (module.imports == null) return;
        for (Ast.ImportNode imp : module.imports) {
            if (imp.path != null && !imp.path.isEmpty()
                    && "Collections".equals(imp.path.get(imp.path.size() - 1))) return;
        }
        if (!usaListasDeColeccion(module)) return;
        java.util.List<String> p = new java.util.ArrayList<>();
        p.add("Collections");
        module.imports.add(new Ast.ImportNode(p, null, module.line, module.column));
    }

    private static boolean usaListasDeColeccion(Ast.ModuleNode module) {
        for (Ast.ITopLevelDecl d : module.defs) {
            if (declNombraTipo(d)) return true;
        }
        return false;
    }

    private static boolean declNombraTipo(Ast.ITopLevelDecl d) {
        if (d instanceof Ast.FuncDef) {
            Ast.FuncDef f = (Ast.FuncDef) d;
            if (tipoEsListaDeColeccion(f.returnType)) return true;
            for (Ast.Param p : f.params) if (tipoEsListaDeColeccion(p.type)) return true;
            return stmtsNombranTipo(f.body);
        }
        if (d instanceof Ast.ClassDef) {
            for (Ast.ITopLevelDecl m : ((Ast.ClassDef) d).members) {
                if (declNombraTipo(m)) return true;
            }
            return false;
        }
        if (d instanceof Ast.VarDecl) return tipoEsListaDeColeccion(((Ast.VarDecl) d).type);
        return false;
    }

    private static boolean stmtsNombranTipo(java.util.List<Ast.IStmt> body) {
        if (body == null) return false;
        for (Ast.IStmt s : body) {
            if (s instanceof Ast.VarDecl && tipoEsListaDeColeccion(((Ast.VarDecl) s).type)) return true;
        }
        return false;
    }

    private static boolean tipoEsListaDeColeccion(Ast.TypeRef t) {
        if (!(t instanceof Ast.SimpleTypeRef)) return false;
        String n = ((Ast.SimpleTypeRef) t).name;
        return "SyncList".equals(n) || "OwnerList".equals(n);
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
        // V7/H2 (#523) — también DENTRO de un array o una tupla: `backing(): Object[]` llegaba
        // con su elemento sin resolver y la clave canónica no se podía calcular.
        if (t instanceof basicplus.frontend.BpType.ArrayType) {
            basicplus.frontend.BpType el = ((basicplus.frontend.BpType.ArrayType) t).element;
            basicplus.frontend.BpType r = resolveCrossModuleType(el, allNs);
            return (r == el) ? t : new basicplus.frontend.BpType.ArrayType(r);
        }
        if (t instanceof basicplus.frontend.BpType.TupleType) {
            java.util.List<basicplus.frontend.BpType> els = new java.util.ArrayList<>();
            boolean cambio = false;
            for (basicplus.frontend.BpType e : ((basicplus.frontend.BpType.TupleType) t).elements) {
                basicplus.frontend.BpType r = resolveCrossModuleType(e, allNs); cambio |= (r != e); els.add(r);
            }
            return cambio ? new basicplus.frontend.BpType.TupleType(els) : t;
        }
        if (t instanceof basicplus.frontend.BpType.UnresolvedEnumRef) {   // #516 — `enum:Lib.Color`
            String name = ((basicplus.frontend.BpType.UnresolvedEnumRef) t).name;
            int lastDot = name.lastIndexOf('.');
            if (lastDot > 0) {
                String modPart = name.substring(0, lastDot), enPart = name.substring(lastDot + 1);
                for (Symbol.ImportedNamespaceSymbol candidate : allNs) {
                    String full = candidate.library.isEmpty() ? candidate.moduleName : candidate.library + "." + candidate.moduleName;
                    if (full.equals(modPart) || candidate.moduleName.equals(modPart)) {
                        Symbol.EnumSymbol es = candidate.enums.get(enPart);
                        if (es != null) return new basicplus.frontend.BpType.EnumType(es);
                    }
                }
            }
            return t;   // sin resolver: exigirEnumsResueltos lo convierte en error
        }
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
        // Sin punto: la interfaz publica sin cualificar SOLO las clases de su propio módulo (ya
        // resueltas en la 1ª pasada contra su ns) y las del lenguaje. Buscarlo «en cualquier ns»
        // cogía un HOMÓNIMO de otro módulo según el orden de los imports (V10, 16-sep, H7).
        return t;   // H1 P3: ya no hay clases «de nadie»; las del lenguaje son de Core y vienen cualificadas
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
        if (ctx.stdlibDir != null) {                       // #467: la stdlib PRIMERO
            Path enStdlib = ctx.stdlibDir.resolve(bpiName);
            if (Files.exists(enStdlib)) return enStdlib;
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
        if (t instanceof basicplus.frontend.BpType.ArrayType) {   // V7/H2 (#523): recursivo
            basicplus.frontend.BpType el = ((basicplus.frontend.BpType.ArrayType) t).element;
            basicplus.frontend.BpType r = resolveTypeAgainst(el, ns);
            return (r == el) ? t : new basicplus.frontend.BpType.ArrayType(r);
        }
        if (t instanceof basicplus.frontend.BpType.TupleType) {
            java.util.List<basicplus.frontend.BpType> els = new java.util.ArrayList<>();
            boolean cambio = false;
            for (basicplus.frontend.BpType e : ((basicplus.frontend.BpType.TupleType) t).elements) {
                basicplus.frontend.BpType r = resolveTypeAgainst(e, ns); cambio |= (r != e); els.add(r);
            }
            return cambio ? new basicplus.frontend.BpType.TupleType(els) : t;
        }
        if (t instanceof basicplus.frontend.BpType.UnresolvedEnumRef) {   // #516
            String en = ((basicplus.frontend.BpType.UnresolvedEnumRef) t).name;
            if (en.indexOf('.') < 0) {
                Symbol.EnumSymbol es = ns.enums.get(en);
                if (es != null) return new basicplus.frontend.BpType.EnumType(es);
            }
            return t;   // de otro módulo (con punto) o desconocido: lo resuelve la pasada 2 o es error
        }
        if (!(t instanceof basicplus.frontend.BpType.UnresolvedClassRef)) return t;
        String name = ((basicplus.frontend.BpType.UnresolvedClassRef) t).name;
        Symbol.ClassSymbol cls = ns.classes.get(name);
        if (cls != null) return new basicplus.frontend.BpType.ClassType(cls);
        return t;
    }

    /** #516 — un enum que sigue sin resolver tras las dos pasadas (o dentro de un array/tupla):
     *  su nombre, o null. Un enum sin resolver NO puede cruzar: el dueño lee 1 slot (escalar de
     *  4 B) y una referencia serían 2. */
    private static String enumSinResolver(basicplus.frontend.BpType t) {
        if (t instanceof basicplus.frontend.BpType.UnresolvedEnumRef) return ((basicplus.frontend.BpType.UnresolvedEnumRef) t).name;
        if (t instanceof basicplus.frontend.BpType.ArrayType) return enumSinResolver(((basicplus.frontend.BpType.ArrayType) t).element);
        if (t instanceof basicplus.frontend.BpType.TupleType)
            for (basicplus.frontend.BpType e : ((basicplus.frontend.BpType.TupleType) t).elements) { String r = enumSinResolver(e); if (r != null) return r; }
        return null;
    }
    private static void exigirEnumsResueltos(Symbol.ImportedNamespaceSymbol ns, String donde, java.util.List<Symbol.ParamSymbol> params,
                                             basicplus.frontend.BpType retorno, SemanticAnalyzer analyzer) {
        String en = enumSinResolver(retorno);
        if (en == null) for (Symbol.ParamSymbol p : params) { en = enumSinResolver(p.type); if (en != null) break; }
        if (en == null) return;
        int punto = en.lastIndexOf('.');
        String mod = punto > 0 ? en.substring(0, punto) : ns.moduleName;
        analyzer.errorDeImport("la firma de '" + ns.moduleName + "." + donde + "' usa el enum '" + en
            + "' y este módulo no importa '" + mod + "' (un enum no puede cruzar sin resolver): añade `import " + mod + "`");
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
    /**
     * V5/H5 — `DAO build`. Genera los DAOs y se detiene: no compila.
     *
     * Verbo aparte y no un paso del build, por dos motivos:
     *
     *   · el usuario HEREDA del DAO generado, y regenerarlo dentro del build le
     *     reescribiría por debajo la clase de la que cuelga su código —la
     *     regeneración tiene que ser un ACTO, no un efecto secundario;
     *   · el compilador va dirigido por demanda, así que al resolver
     *     `import MedidasDao` se encontraría con que ese fichero todavía no
     *     existe. Generando ANTES, cuando arranca el build ya es un .bp más.
     *
     * Es la misma idea que el `.win` del diseñador de ventanas: se genera
     * fuera y entra como fuente.
     */
    private static boolean generarDaos(BpBuild proj) {
        DaoGen.Resultado r = DaoGen.generar(proj, true);
        for (String a : r.avisos)  System.out.println("aviso: " + a);
        for (String e : r.errores) System.err.println("error: " + e);
        for (String f : r.escritos)   System.out.println("  generado : " + f);
        for (String f : r.sinCambios) System.out.println("  al dia   : " + f);
        if (!r.ok()) {
            System.err.println("DAO build: NO se ha generado nada (" + r.errores.size() + " errores)");
            return false;
        }
        System.out.println("DAO build: " + r.entidades + " entidad(es), "
                + r.escritos.size() + " fichero(s) escrito(s), "
                + r.sinCambios.size() + " ya al dia");
        return true;
    }

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
        Barajar.aplicar(r.module);   // V7/H2 (#523, V1): --barajar / --relleno, si se pidieron

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
