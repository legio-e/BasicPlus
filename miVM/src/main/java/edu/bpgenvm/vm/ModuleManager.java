/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.bpgenvm.vm;

/**
 * Loader + linker dinámico de módulos .mod (formato v4).
 *
 * @author eortiz
 */
import edu.bpgenvm.bytecode.ModFormat;
import java.io.DataInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class ModuleManager {
    private final VirtualMachine vm;
    private int nextFreeAddress = 0x0100;

    private final Map<String, Integer> globalSymbolTable = new HashMap<>();
    private final List<ModuleMetadata> loadedModules = new ArrayList<>();
    private final List<LinkTask> pendingLinks = new ArrayList<>();

    private final Map<String, ModSource> discoveryQueue = new LinkedHashMap<>();
    private int mainAbsoluteAddress = 0;

    /**
     * Directorio donde se buscan los módulos stdlib cuando un import no
     * resuelve desde el cwd o desde el fromPath del .mod importador.
     * Configurable vía {@link edu.bpgenvm.config.VmConfig#stdlibDir}.
     * null = no se intenta el fallback stdlib.
     */
    private String stdlibDir = null;

    public void setStdlibDir(String dir) { this.stdlibDir = dir; }
    public String getStdlibDir() { return stdlibDir; }

    /**
     * Workdir (sandbox) de la VM. Cuando está fijado, las operaciones de
     * fichero del programa BP (readFile/writeFile/listDir/etc.) y la
     * carga de módulos por path relativo se confinan a este directorio
     * y sus descendientes. Path traversal (`..`) que intente escapar se
     * rechaza. El `stdlibDir` y los `modulePaths` (vienen de config) son
     * dirs externos "blessed" — siguen accesibles SÓLO para carga de
     * módulos, no para IO genérico.
     *
     * null = sandbox desactivado (comportamiento legacy: la VM ve todo
     * el sistema de ficheros). Se cablea desde Main con --workdir.
     */
    private java.nio.file.Path workdir = null;

    public void setWorkdir(java.nio.file.Path dir) {
        if (dir == null) { this.workdir = null; return; }
        this.workdir = dir.toAbsolutePath().normalize();
    }

    public java.nio.file.Path getWorkdir() { return workdir; }

    /**
     * Resuelve un userPath dentro del workdir y verifica que no escape.
     * Devuelve el Path absoluto (siempre dentro del workdir). Lanza si:
     *   - el workdir no está fijado (programación: no debería llamarse en
     *     ese caso);
     *   - el userPath escapa del workdir (path traversal);
     *   - el userPath es absoluto (los programas BP deben usar paths
     *     relativos al workdir; un path absoluto se rechaza).
     */
    public java.nio.file.Path resolveInWorkdir(String userPath) {
        if (workdir == null)
            throw new IllegalStateException("workdir no configurado");
        if (userPath == null)
            throw new IllegalArgumentException("path null");
        java.nio.file.Path candidate = java.nio.file.Paths.get(userPath);
        if (candidate.isAbsolute())
            throw new SecurityException("path absoluto rechazado por sandbox: " + userPath);
        java.nio.file.Path resolved = workdir.resolve(candidate).toAbsolutePath().normalize();
        if (!resolved.startsWith(workdir))
            throw new SecurityException("path escapa del workdir: " + userPath);
        return resolved;
    }

    /**
     * Variante segura para llamadas desde dispatchers: devuelve null si
     * el path es inválido o escapa, en lugar de lanzar excepción.
     */
    public java.nio.file.Path resolveInWorkdirOrNull(String userPath) {
        try { return resolveInWorkdir(userPath); }
        catch (RuntimeException ignored) { return null; }
    }

    /**
     * Directorios extra donde se buscan los módulos importados. Vienen del
     * .bpproject que arrancó la ejecución (si lo hay). Se prueban DESPUÉS
     * del path tal cual y ANTES de stdlibDir. Lista vacía = no aplica.
     */
    private final java.util.List<String> modulePaths = new java.util.ArrayList<>();

    public void setModulePaths(java.util.List<String> paths) {
        modulePaths.clear();
        if (paths != null) modulePaths.addAll(paths);
    }

    public java.util.List<String> getModulePaths() {
        return java.util.Collections.unmodifiableList(modulePaths);
    }

    /** B3 v2 — Devuelve la dirección absoluta de un símbolo exportado, o
     *  null si no existe. Usado por builtins nativos para instanciar
     *  RuntimeError. Acepta tanto la forma cualificada (`Mod.symName`)
     *  como un símbolo dentro del módulo cuyo `cs` se pasa. */
    /** #248 — lookup directo en la tabla global de símbolos por nombre ya
     *  cualificado (p.ej. "Core.RuntimeError"). null si no está. */
    public Integer resolveGlobal(String qualifiedName) {
        return globalSymbolTable.get(qualifiedName);
    }

    public Integer resolveExportInModule(int cs, String simpleName) {
        // Buscar el módulo cuyo codeStart == cs.
        for (ModuleMetadata m : loadedModules) {
            if (m.codeStart == cs) {
                String qualified = (m.library == null || m.library.isEmpty())
                        ? (m.name + "." + simpleName)
                        : (m.library + "." + m.name + "." + simpleName);
                Integer addr = globalSymbolTable.get(qualified);
                if (addr != null) return addr;
                // Fallback: clave corta (sin prefijo de library).
                return globalSymbolTable.get(m.name + "." + simpleName);
            }
        }
        return null;
    }

    /**
     * Resuelve la ruta real de un fichero .mod.
     *
     * <p><b>La raíz</b> (lo que se manda ejecutar) es una RUTA: se toma tal cual
     * (absoluta o relativa al cwd), o dentro del workdir si lo hay.
     *
     * <p><b>Una dependencia</b> es un NOMBRE (`Str.mod`, `acme.LibHdr.mod`), y
     * se busca por el orden único de las dos VMs — [V7/H2, #493] decisión de
     * Eduardo (15-sep): <i>«el orden siempre ha sido /app → /lib → packs»</i>,
     * el mismo que `discover_deps` en la VM-C:
     * <ol>
     *   <li>el pack en ejecución (lo mira {@link #resolveModuleSource} antes);</li>
     *   <li><b>/app</b>: el workdir si lo hay (sandbox), la carpeta del módulo
     *       raíz (#431) y los `modulePaths` del .bpproject;</li>
     *   <li><b>/lib</b>: `stdlibDir` del BpVM.cfg.</li>
     * </ol>
     * Lo que YA NO se prueba para una dependencia: el nombre «tal cual» contra
     * el cwd de la JVM — con un `.mod` homónimo en el directorio actual miVM lo
     * cogía y la VM-C cogía el de la carpeta del programa: mismo `.mod`, mismo
     * cwd, stdout distinto —; y el `fromPath` del import, que es la pista del
     * COMPILADOR (`import X from "..."`), no de la VM (miVM lo honraba relativo
     * al cwd de la JVM, y se saltaba el sandbox con ello). Si nada existe,
     * devuelve el nombre para que el caller falle con «falta el modulo 'X'».
     */
    private String resolveModulePath(String filename, boolean esRaiz) {
        java.io.File asGiven = new java.io.File(filename);
        String base = asGiven.getName();
        if (esRaiz) {
            if (workdir != null && !asGiven.isAbsolute()) {
                java.nio.file.Path safe = resolveInWorkdirOrNull(filename);
                if (safe != null && java.nio.file.Files.isRegularFile(safe)) return safe.toString();
            }
            return filename;
        }
        // 1) /app: el workdir (sandbox), la carpeta de la raíz, los modulePaths.
        if (workdir != null) {
            java.nio.file.Path baseInWd = resolveInWorkdirOrNull(base);
            if (baseInWd != null && java.nio.file.Files.isRegularFile(baseInWd)) return baseInWd.toString();
        }
        if (rootModuleDir != null) {
            java.io.File junto = new java.io.File(rootModuleDir, base);
            if (junto.isFile()) return junto.getPath();
        }
        for (String dir : modulePaths) {
            if (dir == null || dir.isEmpty()) continue;
            java.io.File cand = new java.io.File(dir, base);
            if (cand.isFile()) return cand.getPath();
        }
        // 2) /lib: stdlibDir del BpVM.cfg.
        java.io.File inStd = enStdlib(base);
        if (inStd != null) return inStd.getPath();
        // Nada: una ruta que NO existe, junto a la raíz. El nombre pelado no vale
        // aquí — el cwd de la JVM lo abriría si por casualidad lo tuviera, que
        // es justo la puerta que se cierra.
        return rootModuleDir != null ? new java.io.File(rootModuleDir, base).getPath() : base;
    }

    /** `stdlibDir/<base>` si existe; null si no hay stdlibDir o no está. */
    private java.io.File enStdlib(String base) {
        if (stdlibDir == null || stdlibDir.isEmpty()) return null;
        java.io.File f = new java.io.File(stdlibDir, base);
        return f.isFile() ? f : null;
    }

    /** [V7/H2, #493] EL AVISO DE LA SOMBRA, por OUTPUT y con el mismo texto que la
     *  VM-C: una dependencia resuelta fuera de /lib que tiene un GEMELO en /lib
     *  está tapando a la stdlib — el caso real de la ficha (una `Collections`
     *  vieja en /app y una tarde mirando otro sitio). Criterio «gemelo tapado»:
     *  mudo sin stdlibDir (el arnés de paridad copia la stdlib al lado del
     *  sample y no fija ninguno), y suena justo donde tapa. Una vez por
     *  dependencia. ASCII, sin la ruta resuelta (cada VM resuelve una distinta). */
    private final java.util.Set<String> sombrasAvisadas = new java.util.HashSet<>();

    private void avisarSiTapa(String canon, String resuelto) {
        java.io.File gemelo = enStdlib(new java.io.File(resuelto).getName());
        if (gemelo == null) return;
        try {
            if (java.nio.file.Files.isSameFile(gemelo.toPath(), java.nio.file.Paths.get(resuelto))) return;
        } catch (IOException e) {
            return;   // uno de los dos no se puede abrir: que lo diga la carga
        }
        if (!sombrasAvisadas.add(canon)) return;
        vm.getProgramOut().writeText("aviso: '" + canon + "' cargado desde /app, tapa al de /lib");
        vm.getProgramOut().newline();
    }

    // ============================================================
    // #310 — PACKS EJECUTABLES
    //
    // Espejo del lado VM-C (bpvm.c). Dos ideas, las mismas que allí:
    //
    //   1) De dónde salen los bytes de un módulo es UNA costura (ModSource):
    //      el resto del loader no sabe si vienen del FS o de dentro del pack.
    //   2) Con un pack en ejecución, sus módulos y sus recursos se buscan
    //      DENTRO antes que en ningún otro sitio (spec §4 — ADITIVO: el resto
    //      del orden de búsqueda no se toca).
    //
    // Diferencia HONESTA con la VM-C: aquí el pack se lee entero a memoria
    // (PackReader trabaja sobre byte[]). En un PC eso da igual, pero NO es un
    // espejo de la restricción del micro, que lo lee por trozos. Lo que sí
    // tiene que ser idéntico es el COMPORTAMIENTO: qué módulo gana y qué
    // recurso gana.
    // ============================================================

    /**
     * De dónde salen los bytes de un módulo: un fichero del FS o una entrada
     * del pack en ejecución. Es la costura ÚNICA — quien carga módulos abre
     * esto y no pregunta de dónde viene (espejo de {@code load_from_pack}).
     */
    private static final class ModSource {
        final String path;    // != null → fichero del FS (y sitio donde buscar el .dbg)
        final byte[] bytes;   // != null → entrada del pack en ejecución
        final String label;   // para los mensajes de error
        /** V7/H2 (MOD8) — lo que dice su cabecera: magic, generación y CRC de su
         *  interfaz (0/0 si es MOD6/MOD7). Se rellenan en discoverDependencies al
         *  leer la cabecera, ANTES de recurrir a sus imports. */
        int magic, gen, ifaceCrc;

        private ModSource(String path, byte[] bytes, String label) {
            this.path = path; this.bytes = bytes; this.label = label;
        }
        static ModSource ofFile(String path) {
            return new ModSource(path, null, path);
        }
        static ModSource ofPack(byte[] bytes, String name, String packPath) {
            return new ModSource(null, bytes, name + " (dentro de " + packPath + ")");
        }
        DataInputStream open() throws IOException {
            /* #431 — que FALTE un módulo es un caso previsto, no un accidente:
             * se dice con el mismo texto que la VM-C («falta el modulo 'X'»),
             * no con un FileNotFoundException y su volcado de pila. Un stack
             * trace de Java delante de quien escribe BP no informa de nada y
             * parece que la VM se ha roto. */
            if (bytes == null) {
                java.io.File f = new java.io.File(path);
                if (!f.isFile()) {
                    String base = f.getName();
                    if (base.endsWith(".mod")) base = base.substring(0, base.length() - 4);
                    throw new IOException("falta el modulo '" + base + "' (buscado como "
                            + path + ")");
                }
            }
            return new DataInputStream(bytes != null
                    ? new java.io.ByteArrayInputStream(bytes)
                    : new FileInputStream(path));
        }
    }

    /** V7/H2 — un símbolo cross-module que no resuelve al enlazar. Lleva el MISMO
     *  texto que la VM-C (link.c: «lib 'X' presente pero no exporta 'X.sym' (la usa
     *  'Y'; version vieja?)») para que el host lo imprima igual. */
    public static final class LinkException extends RuntimeException {
        public LinkException(String msg) { super(msg); }
    }

    /** V7/H2 (#500) — el rechazo de la norma de versiones: el texto ya salió por el
     *  OUTPUT del programa (mismo texto en las dos VMs); el mensaje de la excepción
     *  es ese mismo texto, y el host lo repite por stderr como «load X: …». */
    public static final class VersionRechazada extends IOException {
        public VersionRechazada(String msg) { super(msg); }
    }

    /** Ruta del pack en ejecución, o null si se ejecuta un .mod suelto. */
    private String runPackPath = null;

    /** Índice {@code tipo:nombre} → entrada del pack en ejecución. Se construye
     *  en orden, así que el ÚLTIMO gana — igual que {@code bpvm_pack_find_src}. */
    private final Map<String, basicplus.pack.PackEntry> runPackIndex = new HashMap<>();

    /** true mientras hay un pack en ejecución. */
    public boolean isRunningPack() { return runPackPath != null; }

    /**
     * Nombre de una entrada dentro del pack: el BASENAME sin extensión. Así lo
     * empaqueta PackStep y así lo busca la VM-C ({@code lib.Mod.mod} → "lib.Mod").
     */
    private static String packEntryName(String filename) {
        String base = new java.io.File(filename).getName();
        int dot = base.lastIndexOf('.');
        return (dot >= 0) ? base.substring(0, dot) : base;
    }

    /** Extensión en minúsculas = el `tipo` de la entrada; "" si no tiene. */
    private static String packEntryType(String filename) {
        String base = new java.io.File(filename).getName();
        int dot = base.lastIndexOf('.');
        return (dot >= 0) ? base.substring(dot + 1).toLowerCase() : "";
    }

    /**
     * Lee el manifest buscando una línea {@code clave=valor}. Espejo de
     * {@code bpvm_pack_manifest_get}: primera línea que casa, \n o \r\n.
     */
    private static String manifestGet(byte[] manifest, String key) {
        String txt = new String(manifest, StandardCharsets.UTF_8);
        for (String raw : txt.split("\n", -1)) {
            String line = raw.endsWith("\r") ? raw.substring(0, raw.length() - 1) : raw;
            if (line.length() <= key.length()) continue;
            if (!line.startsWith(key) || line.charAt(key.length()) != '=') continue;
            return line.substring(key.length() + 1);
        }
        return null;
    }

    /**
     * Ejecuta un pack: lo abre, lee el manifest, localiza el módulo principal,
     * lo carga DESDE DENTRO y resuelve sus dependencias con el pack por delante.
     * Espejo de {@code bpvm_load_pack}.
     *
     * @return el nombre del módulo principal (el del manifest).
     */
    public String executeRootPack(String packPath) throws IOException {
        String main = openRunPack(packPath);
        boolean ok = false;
        try {
            discoveryQueue.clear();
            // El main se busca por su nombre de entrada: ya está DENTRO, así que
            // resolveModuleSource lo encuentra en el pack sin tocar el FS.
            discoverDependencies(main + ".mod", main, false);
            for (Map.Entry<String, ModSource> entry : discoveryQueue.entrySet()) {
                loadModuleToMemory(entry.getValue(), entry.getKey());
            }
            linkAll();
            vm.setHeapStart(nextFreeAddress);
            ok = true;
        } finally {
            // Un pack que no llega a arrancar no deja la resolución tocada
            // (mismo criterio que la VM-C, que enciende el pack al final).
            if (!ok) closeRunPack();
        }
        return main;
    }

    /**
     * Abre y VALIDA el pack (magic, versión de formato, los dos CRC) y deja su
     * índice montado. Devuelve el módulo principal del manifest. Si algo no
     * cuadra —incluido que no lleve manifest o que el main no esté dentro— no
     * deja el pack montado: falla con un mensaje que dice qué pasa.
     */
    private String openRunPack(String packPath) throws IOException {
        byte[] img = java.nio.file.Files.readAllBytes(java.nio.file.Paths.get(packPath));
        basicplus.pack.PackReader.Pack p;
        try {
            p = basicplus.pack.PackReader.read(img);
        } catch (basicplus.pack.PackException pe) {
            throw new IOException("pack '" + packPath + "': " + pe.getMessage(), pe);
        }
        if (!p.active)
            throw new IOException("pack '" + packPath + "': está marcado como borrado (tombstone)");

        Map<String, basicplus.pack.PackEntry> idx = new HashMap<>();
        for (basicplus.pack.PackEntry e : p.entries) {
            idx.put(e.tipo + ":" + e.nombre, e);   // el último de la cadena gana
        }
        basicplus.pack.PackEntry mf = idx.get(
                basicplus.pack.PackFormat.TYPE_MANIFEST + ":" + basicplus.pack.PackFormat.MANIFEST_NAME);
        if (mf == null)
            throw new IOException("pack '" + packPath + "': no lleva manifest — no es ejecutable");
        String main = manifestGet(mf.data, "main");
        if (main == null || main.isEmpty())
            throw new IOException("pack '" + packPath + "': el manifest no declara main=");
        if (!idx.containsKey("mod:" + main))
            throw new IOException("pack '" + packPath + "' declara main='" + main
                    + "' pero no lleva ese módulo");

        runPackIndex.clear();
        runPackIndex.putAll(idx);
        runPackPath = packPath;
        return main;
    }

    /**
     * Cierra el pack en ejecución: se acaba la ejecución, se acaba el camino
     * del pack. Sin esto, una segunda ejecución en el mismo proceso heredaría
     * el pack de la anterior — la peor clase de fallo, porque funciona hasta
     * que alguien encadena dos.
     */
    public void closeRunPack() {
        runPackPath = null;
        runPackIndex.clear();
    }

    /**
     * #310 — recurso del pack en ejecución, o null si no lo lleva (o no hay
     * pack). Espejo de {@code pack_res_entry}: del path sólo importa el
     * BASENAME, porque dentro del pack la clave es (tipo, nombre) =
     * (extensión, nombre sin extensión). `/app/x/main.win` → ("win", "main").
     *
     * Lo llaman los builtins de LECTURA de fichero (readFile, readFileBytes,
     * fileExists, fileSize) — nunca los de escritura: para el programa y para
     * la VM, el pack es de sólo lectura.
     */
    public byte[] packResource(String path) {
        if (runPackPath == null || path == null) return null;
        // Nunca reclamar el fichero del PROPIO pack: se lee por el FS de
        // verdad, y reclamarlo aquí sería morderse la cola.
        if (path.equals(runPackPath)) return null;
        String tipo = packEntryType(path);
        if (tipo.isEmpty()) return null;
        basicplus.pack.PackEntry e = runPackIndex.get(tipo + ":" + packEntryName(path));
        return (e != null) ? e.data : null;
    }

    /**
     * Resuelve de dónde salen los bytes de un módulo. ORDEN (spec §4, ADITIVO):
     *   0) el PACK EN EJECUCIÓN, si lo hay  ← lo único nuevo
     *   1..n) exactamente lo de siempre (workdir, cwd, modulePaths, stdlibDir).
     *
     * Un pack que se ejecuta se lleva sus módulos dentro: buscarlos fuera
     * primero sería dejar que el entorno le cambie las tripas a una app que
     * viene cerrada.
     */
    private ModSource resolveModuleSource(String filename, boolean esRaiz) {
        if (runPackPath != null) {
            basicplus.pack.PackEntry e = runPackIndex.get("mod:" + packEntryName(filename));
            if (e != null) return ModSource.ofPack(e.data, packEntryName(filename), runPackPath);
        }
        return ModSource.ofFile(resolveModulePath(filename, esRaiz));
    }

    private static class ModuleMetadata {
        String name;
        String library;          // "" si el .mod no declara library
        int magic, gen, ifaceCrc;   // V7/H2 (MOD8): lo que dice su cabecera (0/0 en MOD6/MOD7)
        int moduleBase;
        int extTableAddress;
        int dataStart;
        int codeStart;
        int endAddress;
        // Información de depuración (opcional, cargada del .dbg si existe):
        String sourcePath;       // path al .bp original; null si no hay .dbg
        // Mapeo relPc → línea. Usamos TreeMap para búsqueda por "el mayor
        // relPc <= valor dado" (cada entry corresponde al inicio de una
        // sentencia BP; cualquier PC entre dos entries pertenece a la
        // sentencia anterior).
        java.util.TreeMap<Integer, Integer> lineMap;
        // Properties públicas del módulo (.dbg v2). Cada entry: name, type,
        // csOff (offset relativo a CS del backing global). El debugger las
        // usa para mostrarlas en el panel de variables.
        java.util.List<PropertyDescriptor> properties;
        // Vars por función (.dbg v3 — H6.a.1). TreeMap<startRelPc, FunctionVars>
        // para localizar, vía floorEntry, la función que contiene un relPc dado.
        // El debugger lo usa para mostrar locales por nombre. null si el .dbg
        // no trae sección `vars` (formato < v3).
        java.util.TreeMap<Integer, FunctionVars> functionVars;
    }

    /** Descriptor de una variable local/param (.dbg v3) para el debugger. */
    public static final class LocalVarDescriptor {
        public final String  name;
        public final int     offset;     // con signo, relativo a bp: <0 param, >=0 local
        public final int     sizeBytes;  // 4 (i32/ref) · 8 (long/double) · 4+len*4 (array)
        public final boolean isArray;
        public final String  type;       // H6.a.2: tag de tipo BP (integer/long/float/double/string/boolean/ref/"?")
        public LocalVarDescriptor(String name, int offset, int sizeBytes, boolean isArray, String type) {
            this.name = name; this.offset = offset; this.sizeBytes = sizeBytes;
            this.isArray = isArray; this.type = (type != null ? type : "?");
        }
    }

    /** Vars (params + locales) de una función (.dbg v3) para el debugger. */
    public static final class FunctionVars {
        public final String name;
        public final int    startRelPc;
        public final java.util.List<LocalVarDescriptor> vars = new java.util.ArrayList<>();
        public FunctionVars(String name, int startRelPc) {
            this.name = name; this.startRelPc = startRelPc;
        }
    }

    /** Info de una property pública de módulo, leída del .dbg. */
    public static final class PropertyDescriptor {
        public final String name;
        public final String type;     // "integer", "float", "string", "boolean", "ref"
        public final int    csOff;    // offset (relativo a CS del módulo) del backing global
        public PropertyDescriptor(String name, String type, int csOff) {
            this.name = name; this.type = type; this.csOff = csOff;
        }
    }

    /** V7/H2 — el texto de un símbolo no resuelto, calcado de link.c (bpvm_link_set_error):
     *  la lib es lo que va antes del PRIMER punto; «presente» si hay un módulo cargado
     *  con ese nombre. Y una pista cuando es un marcador de LAYOUT: el padre cambió de
     *  forma y la clase hija hay que recompilarla. */
    private String textoDeEnlace(String needed, String who) {
        int p = needed.indexOf('.');
        String lib = (p < 0) ? "" : needed.substring(0, p);
        boolean loaded = false;
        for (ModuleMetadata m : loadedModules) if (m.name.equals(lib)) { loaded = true; break; }
        String msg;
        if (!lib.isEmpty() && !loaded)
            msg = "falta la lib '" + lib + "' (la usa '" + who + "'; simbolo '" + needed + "')";
        else if (!lib.isEmpty())
            msg = "lib '" + lib + "' presente pero no exporta '" + needed + "' (la usa '" + who + "'; version vieja?)";
        else
            msg = "simbolo no resuelto '" + needed + "' (la usa '" + who + "')";
        if (needed.contains("#layout#")) msg += " [el padre cambio de forma: recompila la clase hija]";
        return msg;
    }

    private static class LinkTask {
        String moduleName;            // V7/H2: quién pide (para el texto de enlace)
        int extTableAddress;
        int moduleCodeStart;          // CS del módulo dueño (= codeStart)
        String[] requiredImports;
        // L2 v3 — fixups de parentOff para clases con padre cross-module.
        // En linkAll, después de resolver imports, recorremos esta lista
        // y parcheamos parentOff del descriptor del child con
        // (parentAbs - childAbs), siguiendo la convención CS-relative de
        // la VM. Si está vacía, no hay clases con padre cross-module.
        java.util.List<ClassFixupEntry> classFixups = new java.util.ArrayList<>();
        // BUG-2 — fixups de clsOff de TRY_BEGIN_EXT (catch cross-module).
        java.util.List<EhClassFixupEntry> ehClassFixups = new java.util.ArrayList<>();
    }

    /** Fixup de parentOff: el child está en `moduleCodeStart + childCsOff` (en
     *  el módulo del LinkTask); el parent es el data symbol `parentQualified`
     *  exportado por otro módulo (resuelto vía globalSymbolTable). */
    private static class ClassFixupEntry {
        String childClassName;
        int childCsOff;           // offset del child descriptor relativo al CS de su módulo (negativo)
        String parentQualified;   // p.ej. "L2Lib.Counter"
    }

    /** BUG-2 — fixup del operando clsOff de un TRY_BEGIN_EXT. El operando i32
     *  vive en `moduleCodeStart + codeOff`; se parcha con (parentAbs - CS) para
     *  que `cs + clsOff` apunte al descriptor de la clase de excepción del otro
     *  módulo (resuelto vía globalSymbolTable por nombre cualificado). */
    private static class EhClassFixupEntry {
        int codeOff;              // offset del operando i32 relativo al code block del módulo
        String parentQualified;   // p.ej. "HoleLibA.MyError"
    }

    public ModuleManager(VirtualMachine vm) {
        this.vm = vm;
    }

    /** #431 — la carpeta del `.mod` que se está ejecutando, para buscar ahí sus
     *  dependencias. La VM-C lo hace desde siempre (`bpvm_load_mod` saca el
     *  dirname del path y descubre las deps al lado); miVM sólo miraba el
     *  directorio ACTUAL, así que `java -jar miVM.jar samples/X.mod` reventaba
     *  y `cd samples && java ... X.mod` funcionaba. Dos VMs con la misma orden
     *  y distinto resultado, que es lo que no puede pasar. */
    private String rootModuleDir;

    public void executeRootModule(String filename, String moduleName) throws IOException {
        {   // #431: dónde vive la raíz, para resolver sus deps al lado
            java.io.File raiz = new java.io.File(filename);
            java.io.File dir = raiz.getAbsoluteFile().getParentFile();
            rootModuleDir = (dir != null && dir.isDirectory()) ? dir.getPath() : null;
        }
        discoveryQueue.clear();
        sombrasAvisadas.clear();
        discoverDependencies(filename, moduleName, true);
        for (Map.Entry<String, ModSource> entry : discoveryQueue.entrySet()) {
            loadModuleToMemory(entry.getValue(), entry.getKey());
        }
        linkAll();
        vm.setHeapStart(nextFreeAddress);
    }

    /** La cabecera de un módulo recién resuelto: lo que hace falta para aplicar la
     *  norma (#500) y para recorrer sus imports. Se lee ANTES de recurrir. */
    private static final class Cabecera {
        ModSource src; String myLibrary; byte[] importBuffer; boolean v8a;
    }

    /** Resuelve la fuente, la mete en la cola y lee su cabecera. NO recurre. */
    private Cabecera abrirCabecera(String filename, String moduleName, boolean esRaiz) throws IOException {
        // Resolver (#310: pack en ejecución primero; si no, /app → /lib) — y
        // cachear la FUENTE en la cola para que loadModuleToMemory abra
        // exactamente los mismos bytes.
        ModSource src = resolveModuleSource(filename, esRaiz);
        discoveryQueue.put(moduleName, src);
        Cabecera c = new Cabecera();
        c.src = src;
        try (DataInputStream in = src.open()) {
            int magic0 = in.readInt();
            if (!ModFormat.isAbiSupported(magic0)) {   // #284 — gate de ABI
                throw new RuntimeException("no se puede cargar " + src.label + ": "
                        + ModFormat.abiRejectReason(magic0));
            }
            boolean v6a = (magic0 != ModFormat.MAGIC_NUMBER);   // v6 o v7: llevan interfaceSize
            boolean v8a = (magic0 == ModFormat.MAGIC_NUMBER_V8);
            boolean v7a = (magic0 == ModFormat.MAGIC_NUMBER_V7) || v8a;
            in.readInt(); // dataSize
            in.readInt(); // mainOffset

            int importSize  = in.readInt();
            in.readInt();   // exportSize
            in.readInt();   // codeSize
            int librarySize = in.readInt();
            if (v6a) in.readInt();   // interfaceSize (v6): irrelevante para el discovery de imports
            if (v7a) in.readInt();   // nativeSize (v7): idem — aqui solo se buscan imports
            src.magic = magic0;
            if (v8a) { src.gen = in.readInt(); src.ifaceCrc = in.readInt(); }   // V7/H2 (MOD8)

            byte[] libraryBytes = new byte[librarySize];
            in.readFully(libraryBytes);
            c.myLibrary = new String(libraryBytes, StandardCharsets.UTF_8);

            c.importBuffer = new byte[importSize];
            in.readFully(c.importBuffer);
            c.v8a = v8a;
        }
        return c;
    }

    /** [V7/H2, #493/#500] El descubrimiento, en el MISMO orden que `discover_deps`
     *  de la VM-C: se resuelve la dependencia, se lee su cabecera, se aplica la
     *  norma, y sólo entonces se recurre a sus imports. miVM recurría antes de
     *  comprobar, así que con dos incumplimientos en cadena nombraba el más
     *  profundo y la VM-C el primero: el texto por OUTPUT no era byte-idéntico. */
    private void discoverDependencies(String filename, String moduleName, boolean esRaiz) throws IOException {
        if (discoveryQueue.containsKey(moduleName)) return;
        Cabecera c = abrirCabecera(filename, moduleName, esRaiz);
        descubrirImports(c, moduleName);
    }

    private void descubrirImports(Cabecera c, String moduleName) throws IOException {
        ModSource src = c.src;
        String myLibrary = c.myLibrary;
        byte[] importBuffer = c.importBuffer;
        int importSize = importBuffer.length;
        boolean v8a = c.v8a;
        {
            if (importSize > 0) {
                DataInputStream importIn = new DataInputStream(new java.io.ByteArrayInputStream(importBuffer));
                int extCount = importIn.readInt();
                // V7/H2 (MOD8, #500) — la subtabla `deps` va DETRÁS de las entradas de
                // import: count + (modulo, fromPath, gen, ifaceCrc)*. Se lee primero
                // (saltando las entradas) para tenerla a mano al resolver cada import.
                java.util.Map<String, int[]> requeridos = new java.util.HashMap<>();
                if (v8a) {
                    DataInputStream depIn = new DataInputStream(new java.io.ByteArrayInputStream(importBuffer));
                    int n = depIn.readInt();
                    for (int i = 0; i < n; i++) { depIn.readUTF(); depIn.readUTF(); }
                    if (depIn.available() >= 4) {
                        int nd = depIn.readInt();
                        for (int i = 0; i < nd; i++) {
                            String mod = depIn.readUTF(); depIn.readUTF();
                            requeridos.put(mod, new int[]{depIn.readInt(), depIn.readInt(), depIn.readInt()});
                        }
                    }
                }
                for (int i = 0; i < extCount; i++) {
                    String imp = importIn.readUTF();
                    String fromPath = importIn.readUTF();
                    // [V7/H2, #493] `fromPath` NO se usa para resolver: es la pista
                    // del compilador (`import X from "..."`, una ruta del árbol de
                    // FUENTES relativa al .bp), no de la VM. Se deriva por convención:
                    //     "Module.func"           → Module.mod                (misma library que el importer)
                    //     "lib.path.Module.func"  → lib.path.Module.mod
                    String[] parts = imp.split("\\.");
                    if (parts.length < 2) continue;
                    String requiredModule = parts[parts.length - 2];
                    String canon = requiredLibraryDe(imp, parts, myLibrary, fromPath, requiredModule);
                    String requiredFilename = canon + ".mod";
                    ModSource dep = discoveryQueue.get(requiredModule);
                    Cabecera cabDep = null;
                    if (dep == null) {
                        cabDep = abrirCabecera(requiredFilename, requiredModule, false);
                        dep = cabDep.src;
                        if (dep.path != null) avisarSiTapa(canon, dep.path);   /* #493 */
                    }
                    // V7/H2 (#500) — LA NORMA, con la cabecera de la dependencia recién
                    // leída y ANTES de recurrir a sus imports (como la VM-C). También
                    // cuando ya estaba en la cola: cada importador exige lo suyo.
                    if (v8a) {
                        int[] req = requeridos.get(canon);
                        if (req != null) comprobarNorma(moduleName, canon, req, dep);
                    }
                    if (cabDep != null) descubrirImports(cabDep, requiredModule);
                }
            }
        }
    }

    /** Nombre canónico `[lib.]Mod` de la dependencia de un import, con la misma regla
     *  que el compilador usa para sellarla (ns.library + moduleName). */
    private static String requiredLibraryDe(String imp, String[] parts, String myLibrary, String fromPath, String requiredModule) {
        String lib;
        if (parts.length >= 3) {
            StringBuilder sb = new StringBuilder();
            for (int k = 0; k < parts.length - 2; k++) { if (k > 0) sb.append('.'); sb.append(parts[k]); }
            lib = sb.toString();
        } else {
            lib = myLibrary;
        }
        return lib.isEmpty() ? requiredModule : lib + "." + requiredModule;
    }

    /** V7/H2 (#500) — la norma de versiones de las dependencias, la misma en las dos
     *  VMs y con el MISMO texto por OUTPUT (ASCII, sin rutas):
     *   1. dep.gen < requerida → rechazo («mas antiguo»);
     *   2. misma gen y otra ifaceCrc → rechazo («no es la interfaz con la que se compilo»);
     *   3. gen mayor → se acepta si enlaza (los marcadores deciden);
     *   una dependencia MOD6/MOD7 no lleva datos: cuenta como gen 0 y CRC desconocido —
     *   si el importador exige gen > 0 es «mas antiguo» (regla 1); si exige 0, se anota
     *   «sin datos de version» por el log y no se comprueba la interfaz. */
    private final java.util.Set<String> sinDatosAvisados = new java.util.HashSet<>();

    private void comprobarNorma(String importador, String canon, int[] req, ModSource dep) throws IOException {
        int reqGen = req[0], reqCrc = req[1], flags = req[2];
        boolean extiende = (flags & 1) != 0;
        boolean depConDatos = ModFormat.tieneVersion(dep.magic);
        int depGen = depConDatos ? dep.gen : 0;
        String msg = null;
        if (depGen < reqGen) {
            msg = String.format("'%s' (gen %d) es mas antiguo que el que '%s' necesita (gen %d): actualiza /lib o recompila",
                    canon, depGen, importador, reqGen);
        } else if (!depConDatos) {
            if (sinDatosAvisados.add(importador + "->" + canon))
                System.err.printf("[miVM] dep '%s' sin datos de version (%s): no se comprueba su interfaz%n",
                        canon, magicTexto(dep.magic));
        } else if (depGen == reqGen && dep.ifaceCrc != reqCrc) {
            msg = String.format("'%s' (gen %d, interfaz %08x) no es la interfaz con la que se compilo '%s' (interfaz %08x): recompila",
                    canon, depGen, dep.ifaceCrc, importador, reqCrc);
        } else if (depGen > reqGen && extiende) {
            // Norma de Eduardo (15-sep): un padre nunca más reciente que sus hijos.
            msg = String.format("'%s' (gen %d) es mas reciente que '%s' (compilado contra gen %d) y '%s' extiende una clase suya: recompila '%s'",
                    canon, depGen, importador, reqGen, importador, importador);
        }
        if (msg != null) {
            vm.getProgramOut().writeText(msg);
            vm.getProgramOut().newline();
            throw new VersionRechazada(msg);
        }
    }

    private static String magicTexto(int magic) {
        return new String(new byte[]{(byte) (magic >> 24), (byte) (magic >> 16), (byte) (magic >> 8), (byte) magic},
                          StandardCharsets.US_ASCII);
    }

    private void loadModuleToMemory(ModSource source, String moduleName) throws IOException {
        int moduleBase = nextFreeAddress;

        try (DataInputStream in = source.open()) {
            int magic1 = in.readInt();
            if (!ModFormat.isAbiSupported(magic1)) {   // #284 — gate de ABI
                throw new RuntimeException("no se puede cargar " + source.label + ": "
                        + ModFormat.abiRejectReason(magic1));
            }
            boolean v6 = (magic1 != ModFormat.MAGIC_NUMBER);   // v6 o v7 llevan interfaceSize
            boolean v8 = (magic1 == ModFormat.MAGIC_NUMBER_V8);
            boolean v7 = (magic1 == ModFormat.MAGIC_NUMBER_V7) || v8;
            int dataSize    = in.readInt();
            int mainOffset  = in.readInt();
            int importSize  = in.readInt();
            int exportSize  = in.readInt();
            int codeSize    = in.readInt();
            int librarySize = in.readInt();
            int interfaceSize = v6 ? in.readInt() : 0;   // H6.a: sección interface (v6)
            /* [V6/N1.4] v7 — sección `native`: N blobs `.mdn` concatenados, cada
             * uno autodelimitado (cabecera + sym_count*sizeof(sym) + code_size) y
             * autoidentificado por su `arch`. La VM-Java NO ejecuta código nativo
             * —sus `native` corren interpretadas— asi que la SALTA; pero tiene que
             * saltarla BIEN, o todo lo que viene detras se desplaza. */
            int nativeSize = v7 ? in.readInt() : 0;
            int gen = 0, ifaceCrc = 0;
            if (v8) { gen = in.readInt(); ifaceCrc = in.readInt(); }   // V7/H2 (MOD8)

            byte[] libraryBytes = new byte[librarySize];
            in.readFully(libraryBytes);
            String library = new String(libraryBytes, StandardCharsets.UTF_8);

            // El moduleName recibido del caller puede incluir el prefijo de library
            // (e.g., "com.example.Foo" cuando el filename es "com.example.Foo.mod"
            // y la library leída es "com.example"). Lo recortamos.
            if (!library.isEmpty() && moduleName.startsWith(library + ".")) {
                moduleName = moduleName.substring(library.length() + 1);
            }

            // V7/H2 — la sección imports ENTERA a un buffer y se parsea de ahí: así la
            // subtabla `deps` de MOD8 (detrás de las entradas) queda consumida por
            // tamaño, y ningún byte de más o de menos corre la sección exports.
            byte[] importBuf = new byte[importSize];
            in.readFully(importBuf);
            DataInputStream impIn = new DataInputStream(new java.io.ByteArrayInputStream(importBuf));
            int extCount = impIn.readInt();
            String[] imports = new String[extCount];
            for (int i = 0; i < extCount; i++) {
                imports[i] = impIn.readUTF();
                // fromPath: solo lo usa el loader en discoverDependencies. Aquí
                // ya estamos enlazando, así que lo consumimos y lo descartamos.
                impIn.readUTF();
            }

            int extTableSize = extCount * 4;
            int extTableAddress = moduleBase;
            int dataStart = moduleBase + extTableSize;
            int codeStart = dataStart + dataSize;

            if (extTableSize > 0) {
                vm.injectMemory(extTableAddress, new byte[extTableSize]);
            }

            // Las claves de exports se prefijan con la library cuando ésta no
            // está vacía, así dos módulos con el mismo nombre en distintas
            // librerías no colisionan.
            String exportPrefix = library.isEmpty()
                    ? (moduleName + ".")
                    : (library + "." + moduleName + ".");
            // Leemos la sección exports COMPLETA a un buffer para tener
            // boundaries precisos: así podemos detectar la subsección
            // opcional de data exports (B3 v2) que sólo está presente en
            // .mods nuevos sin romper compat con los viejos.
            byte[] expBuf = new byte[exportSize];
            in.readFully(expBuf);
            // H6.a — la sección `interface` va entre exports y data. La VM
            // ejecuta sin ella; la saltamos íntegra (readFully garantiza el
            // consumo, a diferencia de skipBytes).
            if (interfaceSize > 0) {
                byte[] ifaceSkip = new byte[interfaceSize];
                in.readFully(ifaceSkip);
            }
            // [V6/N1.4] Y la sección `native` va justo después, por el mismo
            // motivo y con el mismo gesto: la VM-Java no ejecuta código nativo
            // (sus `native` corren interpretadas), pero saltarla MAL desplazaría
            // data y code — que es como se corrompe un módulo en silencio.
            if (nativeSize > 0) {
                byte[] natSkip = new byte[nativeSize];
                in.readFully(natSkip);
            }
            java.io.DataInputStream expIn = new java.io.DataInputStream(
                    new java.io.ByteArrayInputStream(expBuf));

            int expCount = expIn.readInt();
            for (int i = 0; i < expCount; i++) {
                String expName = expIn.readUTF();
                int relativeOffset = expIn.readInt();
                int absoluteAddress = codeStart + relativeOffset;
                globalSymbolTable.put(exportPrefix + expName, absoluteAddress);
                // Para compatibilidad con módulos library-less que importan sin
                // prefijo, también registramos sin prefijo de library si la hay,
                // SOLO si no colisiona (primer módulo gana).
                if (!library.isEmpty()) {
                    String shortKey = moduleName + "." + expName;
                    globalSymbolTable.putIfAbsent(shortKey, absoluteAddress);
                }
            }
            // B3 v2 — subsección opcional de data symbol exports.
            java.util.List<ClassFixupEntry> moduleClassFixups = new java.util.ArrayList<>();
            java.util.List<EhClassFixupEntry> moduleEhClassFixups = new java.util.ArrayList<>();
            if (expIn.available() > 0) {
                int dataExpCount = expIn.readInt();
                for (int i = 0; i < dataExpCount; i++) {
                    String name = expIn.readUTF();
                    int csOffset = expIn.readInt();
                    // csOffset es CS-relative (CS == codeStart), siguiendo
                    // la convención de NEW_OBJECT que también añade csOff
                    // a cs (= codeStart). csOffset suele ser NEGATIVO
                    // (el data block precede al code).
                    int absoluteAddress = codeStart + csOffset;
                    globalSymbolTable.put(exportPrefix + name, absoluteAddress);
                    if (!library.isEmpty()) {
                        String shortKey = moduleName + "." + name;
                        globalSymbolTable.putIfAbsent(shortKey, absoluteAddress);
                    }
                }
                // L2 v3 — subsección opcional de class fixups (parejas
                // childClassName + childCsOff + parentQualified). Sólo
                // presente si el módulo declara clases con padre cross-module.
                if (expIn.available() > 0) {
                    int fxCount = expIn.readInt();
                    for (int i = 0; i < fxCount; i++) {
                        ClassFixupEntry fx = new ClassFixupEntry();
                        fx.childClassName = expIn.readUTF();
                        fx.childCsOff = expIn.readInt();
                        fx.parentQualified = expIn.readUTF();
                        moduleClassFixups.add(fx);
                    }
                    // BUG-2 — §4.4 eh-class fixups (catch cross-module).
                    if (expIn.available() > 0) {
                        int ehCount = expIn.readInt();
                        for (int i = 0; i < ehCount; i++) {
                            EhClassFixupEntry fx = new EhClassFixupEntry();
                            fx.codeOff = expIn.readInt();
                            fx.parentQualified = expIn.readUTF();
                            moduleEhClassFixups.add(fx);
                        }
                    }
                }
            }

            // Sólo el módulo raíz (primero cargado) aporta la dirección de
            // entrada del programa. Los módulos importados pueden tener su
            // propio "main" en el .mod (es siempre emitido por el frontend),
            // pero NO debe sobrescribir el del root.
            if (mainOffset != -1 && this.mainAbsoluteAddress == 0) {
                this.mainAbsoluteAddress = codeStart + mainOffset;
            }

            if (dataSize > 0) {
                byte[] dataBlock = new byte[dataSize];
                in.readFully(dataBlock);
                vm.injectMemory(dataStart, dataBlock);
            }

            byte[] bytecode = new byte[codeSize];
            in.readFully(bytecode);
            vm.injectMemory(codeStart, bytecode);

            ModuleMetadata meta = new ModuleMetadata();
            meta.name = moduleName;
            meta.library = library;
            meta.magic = magic1; meta.gen = gen; meta.ifaceCrc = ifaceCrc;   // V7/H2
            meta.moduleBase = moduleBase;
            meta.extTableAddress = extTableAddress;
            meta.dataStart = dataStart;
            meta.codeStart = codeStart;
            meta.endAddress = codeStart + codeSize;

            // Intentar cargar info de depuración: <mismo nombre>.dbg al lado del .mod.
            // Un módulo que viene de un pack no tiene .dbg al lado (PackStep no
            // los empaqueta): source.path es null y loadDbgFor no hace nada —
            // mismo comportamiento que la VM-C.
            loadDbgFor(meta, source.path);

            loadedModules.add(meta);

            LinkTask task = new LinkTask();
            task.moduleName = moduleName;
            task.extTableAddress = extTableAddress;
            task.moduleCodeStart = codeStart;
            task.requiredImports = imports;
            task.classFixups = moduleClassFixups;
            task.ehClassFixups = moduleEhClassFixups;
            pendingLinks.add(task);

            nextFreeAddress = meta.endAddress + 64;

            if (exportSize < 0) {
                throw new RuntimeException("Tamaño de exports negativo");
            }
        }
    }

    public void linkAll() {
        for (LinkTask task : pendingLinks) {
            for (int i = 0; i < task.requiredImports.length; i++) {
                String symbolNeeded = task.requiredImports[i];
                Integer absoluteAddr = globalSymbolTable.get(symbolNeeded);
                if (absoluteAddr == null) {
                    throw new LinkException(textoDeEnlace(symbolNeeded, task.moduleName));
                }
                vm.writeInt32(task.extTableAddress + (i * 4), absoluteAddr);
            }
            // L2 v3 — parcha parentOff de clases con padre cross-module. El
            // descriptor del child quedó con parentOff=0 al emitir; aquí
            // resolvemos el parent vía globalSymbolTable y escribimos el offset
            // CS-relative al CS del módulo del child (siguiendo la convención
            // que usa isDescendantOf y INVOKE_VIRTUAL para navegar la cadena).
            for (ClassFixupEntry fx : task.classFixups) {
                Integer parentAbs = globalSymbolTable.get(fx.parentQualified);
                if (parentAbs == null) {
                    throw new RuntimeException("L2 v3 fixup: parent no resuelto '"
                            + fx.parentQualified + "' para clase '"
                            + fx.childClassName + "'");
                }
                int childAbs = task.moduleCodeStart + fx.childCsOff;
                int parentOff = parentAbs - task.moduleCodeStart;  // CS-relative al CS del child
                // CLS_OFF_PARENT_OFF = 8 (layout del descriptor; ver ModWriter.endClass)
                vm.writeInt32(childAbs + 8, parentOff);
            }
            // BUG-2 — parcha el clsOff i32 de cada TRY_BEGIN_EXT (catch cross-module)
            // con la dirección CS-relative del descriptor de la clase de excepción.
            for (EhClassFixupEntry fx : task.ehClassFixups) {
                Integer parentAbs = globalSymbolTable.get(fx.parentQualified);
                if (parentAbs == null) {
                    throw new RuntimeException("BUG-2 eh-fixup: clase de excepción no resuelta '"
                            + fx.parentQualified + "'");
                }
                int clsOff = parentAbs - task.moduleCodeStart;  // cs + clsOff = descriptor
                vm.writeInt32(task.moduleCodeStart + fx.codeOff, clsOff);
            }
        }
        vm.setPC(mainAbsoluteAddress);
        vm.setCS(getModuleBaseFromPC(mainAbsoluteAddress));
    }

    public int getModuleBaseFromPC(int pc) {
        for (ModuleMetadata meta : loadedModules) {
            if (pc >= meta.codeStart && pc <= meta.endAddress) return meta.codeStart;
        }
        return 0;
    }

    /**
     * Descripción humana de un PC: "ModuleName.exportedFunc+offset" si cae en
     * el rango de una función exportada conocida, o "ModuleName+offset" si solo
     * conocemos el módulo, o "<unknown PC>" si no cae en ningún módulo cargado.
     * Si está cargado un .dbg para el módulo, añade " (file:line)" al final.
     */
    public String describePc(int pc) {
        ModuleMetadata mod = null;
        for (ModuleMetadata meta : loadedModules) {
            if (pc >= meta.codeStart && pc <= meta.endAddress) { mod = meta; break; }
        }
        if (mod == null) return "<unknown PC=" + pc + ">";
        // Busca la función exportada cuya dirección es la mayor ≤ pc DENTRO del módulo.
        String bestName = null;
        int bestAddr = -1;
        String prefix = mod.name + ".";
        for (Map.Entry<String, Integer> e : globalSymbolTable.entrySet()) {
            String key = e.getKey();
            if (!key.startsWith(prefix)) continue;
            int addr = e.getValue();
            if (addr <= pc && addr > bestAddr) {
                bestAddr = addr;
                bestName = key;
            } else if (addr == bestAddr && bestName != null && bestName.contains(".__cls_")
                       && !key.contains(".__cls_")) {
                bestName = key;   // V7/H2: a igual dirección, el nombre real antes que su alias `__cls_*`
            }
        }
        String base;
        if (bestName != null) {
            base = bestName + "+" + (pc - bestAddr) + " (PC=" + pc + ")";
        } else {
            base = mod.name + "+" + (pc - mod.codeStart) + " (PC=" + pc + ")";
        }
        // Anexar info del .dbg si tenemos.
        String src = sourceLocation(mod, pc);
        return (src != null) ? base + " " + src : base;
    }

    /**
     * Devuelve la línea origen 1-based para un PC absoluto, o -1 si el
     * módulo no tiene .dbg cargado o el PC no cae en ningún módulo. Usa
     * el lineMap del módulo (floor lookup: el inicio de la sentencia que
     * contiene este PC).
     */
    public int getLineForPc(int pc) {
        for (ModuleMetadata meta : loadedModules) {
            if (pc >= meta.codeStart && pc < meta.endAddress) {
                if (meta.lineMap == null) return -1;
                int relPc = pc - meta.codeStart;
                Map.Entry<Integer, Integer> e = meta.lineMap.floorEntry(relPc);
                return (e != null) ? e.getValue() : -1;
            }
        }
        return -1;
    }

    /**
     * H6.a.1 — Devuelve la {@link FunctionVars} (vars del .dbg v3) cuya función
     * contiene el PC absoluto dado, o null si el módulo no trae sección `vars`
     * o el PC no cae en ninguna función conocida. Usa la misma lógica de
     * pertenencia a módulo que {@link #getLineForPc} y un floor lookup por
     * relPc de inicio de función (las funciones son contiguas en el código).
     */
    public FunctionVars functionForPc(int pc) {
        for (ModuleMetadata meta : loadedModules) {
            if (pc >= meta.codeStart && pc < meta.endAddress) {
                if (meta.functionVars == null) return null;
                int relPc = pc - meta.codeStart;
                Map.Entry<Integer, FunctionVars> e = meta.functionVars.floorEntry(relPc);
                return (e != null) ? e.getValue() : null;
            }
        }
        return null;
    }

    /**
     * Devuelve el path al .bp original para un PC absoluto, o null si el
     * módulo no tiene .dbg cargado o el PC no cae en ningún módulo.
     */
    public String getSourceForPc(int pc) {
        for (ModuleMetadata meta : loadedModules) {
            if (pc >= meta.codeStart && pc < meta.endAddress) {
                return meta.sourcePath;
            }
        }
        return null;
    }

    /**
     * Devuelve "(file:line)" para un PC absoluto si el módulo tiene .dbg
     * cargado. Si no, devuelve null. Busca el mayor relPc ≤ (pc - codeStart)
     * en el lineMap — esa entry indica el inicio de la sentencia donde cae el PC.
     */
    private String sourceLocation(ModuleMetadata mod, int pc) {
        if (mod.lineMap == null || mod.lineMap.isEmpty()) return null;
        int relPc = pc - mod.codeStart;
        Map.Entry<Integer, Integer> e = mod.lineMap.floorEntry(relPc);
        if (e == null) return null;
        String src = (mod.sourcePath != null) ? mod.sourcePath : mod.name + ".bp";
        // Si sourcePath es una ruta absoluta larga, dejamos solo el basename
        // para que el trace sea legible. Conservamos el separador final.
        int sep = Math.max(src.lastIndexOf('/'), src.lastIndexOf('\\'));
        String shortSrc = (sep >= 0) ? src.substring(sep + 1) : src;
        return "[" + shortSrc + ":" + e.getValue() + "]";
    }

    /**
     * Intenta cargar el fichero .dbg correspondiente al .mod que se está
     * cargando. Si no existe o tiene formato inválido, no es error: el módulo
     * sigue cargado sin info de depuración.
     */
    private void loadDbgFor(ModuleMetadata meta, String modFilename) {
        if (modFilename == null) return;   // #310 — módulo cargado desde un pack: no hay .dbg al lado
        String dbgFilename;
        if (modFilename.endsWith(".mod")) {
            dbgFilename = modFilename.substring(0, modFilename.length() - 4) + ".dbg";
        } else {
            dbgFilename = modFilename + ".dbg";
        }
        java.io.File dbgFile = new java.io.File(dbgFilename);
        if (!dbgFile.isFile()) return;
        try (java.io.BufferedReader br = new java.io.BufferedReader(
                new java.io.InputStreamReader(new FileInputStream(dbgFile), StandardCharsets.UTF_8))) {
            String header = br.readLine();
            if (header == null || !header.startsWith("dbg ")) return;
            String line;
            boolean inLines = false;
            boolean inProps = false;
            boolean inVars  = false;
            java.util.TreeMap<Integer, Integer> map = new java.util.TreeMap<>();
            java.util.List<PropertyDescriptor> props = new java.util.ArrayList<>();
            java.util.TreeMap<Integer, FunctionVars> fvMap = new java.util.TreeMap<>();
            FunctionVars curFv = null;
            while ((line = br.readLine()) != null) {
                if (line.isEmpty()) continue;
                if (line.startsWith("module ")) { inLines = false; inProps = false; inVars = false; continue; }
                if (line.startsWith("source ")) {
                    meta.sourcePath = line.substring("source ".length()).trim();
                    inLines = false; inProps = false; inVars = false;
                    continue;
                }
                if (line.equals("lines"))      { inLines = true;  inProps = false; inVars = false; continue; }
                if (line.equals("properties")) { inLines = false; inProps = true;  inVars = false; continue; }
                if (line.equals("vars"))       { inLines = false; inProps = false; inVars = true;  curFv = null; continue; }
                if (inVars) {
                    if (line.startsWith("func ")) {
                        // "func <qualifiedName> <startRelPc>"
                        String[] t = line.substring("func ".length()).trim().split("\\s+");
                        curFv = null;
                        if (t.length >= 2) {
                            try {
                                int start = Integer.parseInt(t[1]);
                                curFv = new FunctionVars(t[0], start);
                                fvMap.put(start, curFv);
                            } catch (NumberFormatException ignored) { }
                        }
                        continue;
                    }
                    if (curFv != null) {
                        // "<varName> <signedOffset> <sizeBytes> <isArray:0|1> [<typeTag>]"
                        // El 5º campo (typeTag) es opcional: .dbg v3 pre-H6.a.2 trae 4.
                        String[] t = line.trim().split("\\s+");
                        if (t.length >= 4) {
                            try {
                                String typeTag = (t.length >= 5) ? t[4] : "?";
                                curFv.vars.add(new LocalVarDescriptor(
                                        t[0], Integer.parseInt(t[1]),
                                        Integer.parseInt(t[2]), t[3].equals("1"), typeTag));
                            } catch (NumberFormatException ignored) { }
                        }
                    }
                    continue;
                }
                if (inLines) {
                    int sp = line.indexOf(' ');
                    if (sp <= 0) continue;
                    try {
                        int relPc = Integer.parseInt(line.substring(0, sp).trim());
                        int ln    = Integer.parseInt(line.substring(sp + 1).trim());
                        map.put(relPc, ln);
                    } catch (NumberFormatException ignored) { }
                    continue;
                }
                if (inProps) {
                    // "<name> <type> <csOff>"
                    String[] toks = line.trim().split("\\s+");
                    if (toks.length != 3) continue;
                    try {
                        int csOff = Integer.parseInt(toks[2]);
                        props.add(new PropertyDescriptor(toks[0], toks[1], csOff));
                    } catch (NumberFormatException ignored) { }
                }
            }
            if (!map.isEmpty()) meta.lineMap = map;
            if (!props.isEmpty()) meta.properties = props;
            if (!fvMap.isEmpty()) meta.functionVars = fvMap;
        } catch (IOException ignored) {
            // .dbg corrupto o ilegible: silencioso, sin info de depuración.
        }
    }

    // =================================================================
    // API para el debugger: enumerar properties públicas y leer su valor
    // del data block sin ejecutar BP code.
    // =================================================================

    /**
     * Vista pública (read-only) de una property con su valor actual,
     * destinada al debugger del IDE.
     */
    public static final class PropertyView {
        public final String module;
        public final String name;
        public final String type;     // "integer", "float", "string", "boolean", "ref"
        public final int    rawValue; // bytes interpretados según tipo
        public final String display;  // representación lista para mostrar
        public PropertyView(String module, String name, String type, int rawValue, String display) {
            this.module = module; this.name = name; this.type = type;
            this.rawValue = rawValue; this.display = display;
        }
    }

    /**
     * Snapshot de TODAS las properties públicas de todos los módulos
     * cargados. Lectura directa del data block — no ejecuta bytecode.
     * Llamable de forma segura desde el DebugHook con la VM pausada.
     */
    public java.util.List<PropertyView> snapshotAllProperties() {
        java.util.List<PropertyView> out = new java.util.ArrayList<>();
        for (ModuleMetadata m : loadedModules) {
            if (m.properties == null) continue;
            for (PropertyDescriptor pd : m.properties) {
                int absAddr = m.codeStart + pd.csOff;  // CS = codeStart; csOff es relativo a CS
                int raw = vm.readMemoryInt(absAddr);
                String display = formatPropertyValue(pd.type, raw);
                out.add(new PropertyView(m.name, pd.name, pd.type, raw, display));
            }
        }
        return out;
    }

    private String formatPropertyValue(String type, int raw) {
        switch (type) {
            case "integer": return Integer.toString(raw);
            case "boolean": return raw != 0 ? "true" : "false";
            case "float":   return Float.toString(Float.intBitsToFloat(raw));
            case "string":  return raw == 0 ? "null" : vm.readStringIfPossible(raw);
            case "ref":     return raw == 0 ? "null" : ("@" + Integer.toHexString(raw));
            default:        return "?";
        }
    }

    /**
     * Dado un puntero al data block de algún módulo (típicamente un class_ptr),
     * devuelve el codeStart de ese módulo. INVOKE_VIRTUAL lo usa para fijar CS
     * al módulo donde está definida la clase, independientemente del CS del caller.
     */
    public int getCSForDataAddr(int addr) {
        for (ModuleMetadata meta : loadedModules) {
            if (addr >= meta.dataStart && addr < meta.codeStart) return meta.codeStart;
        }
        throw new RuntimeException("Dirección " + addr + " no cae en el data block de ningún módulo cargado");
    }

    /**
     * #324 tanda 2b — REFLEXIÓN MÍNIMA: nombre de método → slot de vtable, en
     * ejecución. Idea de Eduardo, y sin dato nuevo: el .mod ya exporta un
     * símbolo {@code Clase#metodo#slot} por cada entrada de la vtable
     * (ModWriter.exportVtableMarkers), apuntando al descriptor de la clase. El
     * mapa nombre→slot YA viajaba; sólo faltaba preguntarle.
     *
     * Gracias a esto el .win no necesita slots horneados: se acabó el número
     * congelado en un fichero, que miente en cuanto la clase cambia.
     *
     * @param classPtr dirección del descriptor (la primera palabra del objeto)
     * @param simple   nombre del método, sin clase
     * @return el slot, o -1 si esa clase no tiene un método así.
     */
    public int slotOfMarker(int classPtr, String simple) {
        String needle = "#" + simple + "#";
        for (Map.Entry<String, Integer> e : globalSymbolTable.entrySet()) {
            if (e.getValue() != classPtr) continue;
            String k = e.getKey();
            int at = k.indexOf(needle);
            if (at < 0) continue;
            try { return Integer.parseInt(k.substring(at + needle.length())); }
            catch (NumberFormatException nfe) { /* no era un marcador: sigue */ }
        }
        return -1;
    }

    public int getExternalTableAddressForCS(int cs) {
        for (ModuleMetadata meta : loadedModules) {
            if (meta.codeStart == cs) return meta.extTableAddress;
        }
        return 0;
    }

    /**
     * Lista de regiones de data block de todos los módulos cargados.
     * Cada entrada es {dataStart, dataSize}. Lo usa el GC para hacer
     * scan conservativo en busca de posibles refs al heap.
     */
    public List<int[]> getDataRegions() {
        List<int[]> result = new ArrayList<>();
        for (ModuleMetadata meta : loadedModules) {
            int dataSize = meta.codeStart - meta.dataStart;
            if (dataSize > 0) {
                result.add(new int[]{meta.dataStart, dataSize});
            }
        }
        return result;
    }
}
