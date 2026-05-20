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

    private final Map<String, String> discoveryQueue = new LinkedHashMap<>();
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

    /**
     * Resuelve la ruta real de un fichero .mod probando, en orden:
     *   1) `filename` tal cual (absoluto o relativo al cwd);
     *   2) cada directorio de `modulePaths` (vienen del .bpproject);
     *   3) `stdlibDir/<basename>` (del BpVM.cfg) — los módulos del lenguaje;
     *   4) si nada existe, devuelve `filename` para que el caller falle con
     *      FileNotFound y dé un mensaje claro.
     *
     * En todos los casos se usa el BASENAME del filename solicitado para
     * los pasos 2 y 3 (los imports vienen con nombres como
     * "lib.Module.mod" o "Module.mod" — no rutas relativas con subdir).
     */
    private String resolveModulePath(String filename) {
        // A2.1 — orden con workdir:
        //   1) si workdir está activo y `filename` es relativo, probamos
        //      workdir/filename (con sandbox).
        //   2) `filename` tal cual (absoluto o relativo al cwd — sólo
        //      útil cuando no hay workdir).
        //   3) modulePaths del proyecto.
        //   4) stdlibDir del BpVM.cfg.
        java.io.File asGiven = new java.io.File(filename);
        String base = asGiven.getName();
        if (workdir != null && !asGiven.isAbsolute()) {
            java.nio.file.Path safe = resolveInWorkdirOrNull(filename);
            if (safe != null && java.nio.file.Files.isRegularFile(safe)) {
                return safe.toString();
            }
            // Si el path original tenía subdir y no apareció, probamos sólo basename
            // dentro del workdir (los imports llegan típicamente como "Util.mod").
            if (!base.equals(filename)) {
                java.nio.file.Path baseInWd = resolveInWorkdirOrNull(base);
                if (baseInWd != null && java.nio.file.Files.isRegularFile(baseInWd)) {
                    return baseInWd.toString();
                }
            }
        }
        if (asGiven.isFile()) return filename;
        // 3) modulePaths del proyecto (orden de declaración).
        for (String dir : modulePaths) {
            if (dir == null || dir.isEmpty()) continue;
            java.io.File cand = new java.io.File(dir, base);
            if (cand.isFile()) return cand.getPath();
        }
        // 4) stdlibDir del BpVM.cfg.
        if (stdlibDir != null && !stdlibDir.isEmpty()) {
            java.io.File inStd = new java.io.File(stdlibDir, base);
            if (inStd.isFile()) return inStd.getPath();
        }
        return filename;
    }

    private static class ModuleMetadata {
        String name;
        String library;          // "" si el .mod no declara library
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

    private static class LinkTask {
        int extTableAddress;
        String[] requiredImports;
    }

    public ModuleManager(VirtualMachine vm) {
        this.vm = vm;
    }

    public void executeRootModule(String filename, String moduleName) throws IOException {
        discoveryQueue.clear();
        discoverDependencies(filename, moduleName);
        for (Map.Entry<String, String> entry : discoveryQueue.entrySet()) {
            loadModuleToMemory(entry.getValue(), entry.getKey());
        }
        linkAll();
        vm.setHeapStart(nextFreeAddress);
    }

    private void discoverDependencies(String filename, String moduleName) throws IOException {
        if (discoveryQueue.containsKey(moduleName)) return;
        // Resolver vía stdlib si no está en cwd — y cachear la ruta REAL en
        // la cola para que loadModuleToMemory abra exactamente el mismo .mod.
        String resolved = resolveModulePath(filename);
        discoveryQueue.put(moduleName, resolved);

        try (DataInputStream in = new DataInputStream(new FileInputStream(resolved))) {
            if (in.readInt() != ModFormat.MAGIC_NUMBER) {
                throw new RuntimeException("Firma mágica inválida en " + resolved);
            }
            in.readInt(); // dataSize
            in.readInt(); // mainOffset

            int importSize  = in.readInt();
            in.readInt();   // exportSize
            in.readInt();   // codeSize
            int librarySize = in.readInt();

            byte[] libraryBytes = new byte[librarySize];
            in.readFully(libraryBytes);
            String myLibrary = new String(libraryBytes, StandardCharsets.UTF_8);

            byte[] importBuffer = new byte[importSize];
            in.readFully(importBuffer);

            if (importSize > 0) {
                DataInputStream importIn = new DataInputStream(new java.io.ByteArrayInputStream(importBuffer));
                int extCount = importIn.readInt();
                for (int i = 0; i < extCount; i++) {
                    String imp = importIn.readUTF();
                    String fromPath = importIn.readUTF();
                    // Resolución de filename:
                    //   - Si fromPath no es vacío, lo usamos tal cual (el linker
                    //     sigue trabajando con `imp`, el nombre lógico).
                    //   - Si está vacío, derivamos por convención:
                    //     "Module.func"           → Module.mod                (misma library que el importer)
                    //     "lib.path.Module.func"  → lib.path.Module.mod
                    String[] parts = imp.split("\\.");
                    if (parts.length < 2) continue;
                    String requiredModule = parts[parts.length - 2];
                    String requiredFilename;
                    if (!fromPath.isEmpty()) {
                        requiredFilename = fromPath;
                    } else {
                        String requiredLibrary;
                        if (parts.length >= 3) {
                            StringBuilder sb = new StringBuilder();
                            for (int k = 0; k < parts.length - 2; k++) {
                                if (k > 0) sb.append('.');
                                sb.append(parts[k]);
                            }
                            requiredLibrary = sb.toString();
                        } else {
                            requiredLibrary = myLibrary;
                        }
                        requiredFilename = requiredLibrary.isEmpty()
                                ? requiredModule + ".mod"
                                : requiredLibrary + "." + requiredModule + ".mod";
                    }
                    discoverDependencies(requiredFilename, requiredModule);
                }
            }
        }
    }

    private void loadModuleToMemory(String filename, String moduleName) throws IOException {
        int moduleBase = nextFreeAddress;

        try (DataInputStream in = new DataInputStream(new FileInputStream(filename))) {
            if (in.readInt() != ModFormat.MAGIC_NUMBER) {
                throw new RuntimeException("Firma mágica inválida en " + filename);
            }
            int dataSize    = in.readInt();
            int mainOffset  = in.readInt();
            int importSize  = in.readInt();
            int exportSize  = in.readInt();
            int codeSize    = in.readInt();
            int librarySize = in.readInt();

            byte[] libraryBytes = new byte[librarySize];
            in.readFully(libraryBytes);
            String library = new String(libraryBytes, StandardCharsets.UTF_8);

            // El moduleName recibido del caller puede incluir el prefijo de library
            // (e.g., "com.example.Foo" cuando el filename es "com.example.Foo.mod"
            // y la library leída es "com.example"). Lo recortamos.
            if (!library.isEmpty() && moduleName.startsWith(library + ".")) {
                moduleName = moduleName.substring(library.length() + 1);
            }

            int extCount = in.readInt();
            String[] imports = new String[extCount];
            for (int i = 0; i < extCount; i++) {
                imports[i] = in.readUTF();
                // fromPath: solo lo usa el loader en discoverDependencies. Aquí
                // ya estamos enlazando, así que lo consumimos y lo descartamos.
                in.readUTF();
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
            int expCount = in.readInt();
            for (int i = 0; i < expCount; i++) {
                String expName = in.readUTF();
                int relativeOffset = in.readInt();
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
            meta.moduleBase = moduleBase;
            meta.extTableAddress = extTableAddress;
            meta.dataStart = dataStart;
            meta.codeStart = codeStart;
            meta.endAddress = codeStart + codeSize;

            // Intentar cargar info de depuración: <mismo nombre>.dbg al lado del .mod.
            loadDbgFor(meta, filename);

            loadedModules.add(meta);

            LinkTask task = new LinkTask();
            task.extTableAddress = extTableAddress;
            task.requiredImports = imports;
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
                    throw new RuntimeException("Símbolo no resuelto: " + symbolNeeded);
                }
                vm.writeInt32(task.extTableAddress + (i * 4), absoluteAddr);
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
            java.util.TreeMap<Integer, Integer> map = new java.util.TreeMap<>();
            java.util.List<PropertyDescriptor> props = new java.util.ArrayList<>();
            while ((line = br.readLine()) != null) {
                if (line.isEmpty()) continue;
                if (line.startsWith("module ")) { inLines = false; inProps = false; continue; }
                if (line.startsWith("source ")) {
                    meta.sourcePath = line.substring("source ".length()).trim();
                    inLines = false; inProps = false;
                    continue;
                }
                if (line.equals("lines"))      { inLines = true; inProps = false; continue; }
                if (line.equals("properties")) { inLines = false; inProps = true;  continue; }
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
