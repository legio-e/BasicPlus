// ============================================================
// AotBuild.java
// Pipeline de compilación AOT desde el IDE (H12 Bloque B).
//
// Por cada módulo del proyecto con funciones `function native`, genera un
// `.mdn` nativo en outDir, que PicoExplorer sube automáticamente junto al
// `.mod` (detección por sibling, ya existente). Cadena:
//
//     AotMain.emitAotC(--mdn)  →  arm-none-eabi-gcc (PIC Thumb-2)  →  MdnPack
//
// Es la versión "in-IDE" del script manual bpgenvm-c/pico/build_mdn.sh.
//
// Filosofía (decisión V3): el `.mod` SIEMPRE se genera y es ejecutable
// interpretado. El `.mdn` es una aceleración OPCIONAL. Si algo falla
// (toolchain ausente, native no AOT-able, gcc devuelve error) se AVISA por
// consola y se continúa — nunca aborta el Run. "Para eso el .mod siempre se
// genera."
//
// Target Fase 1: "arm" = Cortex-M33 → mismo `.mdn` PIC para RP2350 y STM32.
// ESP32 (Xtensa / RISC-V) queda para V4.
// ============================================================
package com.mycompany.bpide;

import basicplus.frontend.AotCEmitter;
import basicplus.frontend.AotMain;
import basicplus.frontend.MdnPack;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;
import java.util.function.Consumer;
import java.util.stream.Stream;

/** Compilación AOT de módulos `native` a `.mdn` desde el IDE. Sin estado. */
public final class AotBuild {

    private AotBuild() {}

    /** Flags fijos del target "arm" = Cortex-M33 (RP2350 + STM32: mismo .mdn
     *  PIC). DEBEN coincidir con bpgenvm-c/pico/build_mdn.sh y con el loader del
     *  firmware — si cambian aquí sin cambiar allí, el .mdn no será cargable. */
    private static final String[] ARM_M33_FLAGS = {
        "-mcpu=cortex-m33", "-mthumb", "-mfloat-abi=softfp", "-mfpu=fpv5-sp-d16",
        "-fpic", "-fno-jump-tables", "-Os",
    };

    /** Flags de compilación del target "riscv" = ESP32-P4 (RV32IMAFC). NO fijamos
     *  -march/-mabi: el toolchain esp trae el del P4 por DEFECTO y el firmware
     *  tampoco los pasa → casan por construcción. A diferencia de ARM, el .text
     *  RISC-V NO sale autocontenido (la llamada recursiva, refs PC-relativas dejan
     *  relocalizaciones que MdnPack no resuelve) → hace falta el paso de enlace
     *  (RISCV_LINK_FLAGS). -mno-relax evita las relocs de relajación. */
    private static final String[] RISCV_P4_FLAGS = {
        "-fno-pic", "-mno-relax", "-fno-jump-tables", "-Os",
    };

    /** Paso EXTRA de RISC-V (bifurcación A, decidida con Eduardo): enlazar el .o a
     *  -Ttext=0 resolviendo las relocalizaciones internas PC-relativas → el .text
     *  queda autocontenido y position-independent (todo relativo al PC). El código
     *  muerto del .mdn (aot_<Mod>_register) referencia el registry del firmware, un
     *  símbolo externo → --unresolved-symbols=ignore-all lo deja en 0 (nunca se
     *  ejecuta: el loader registra los thunks por la tabla del .mdn). MdnPack lee
     *  el .elf enlazado igual que un .o; con -Ttext=0, valor de símbolo == offset. */
    private static final String[] RISCV_LINK_FLAGS = {
        "-nostdlib", "-nostartfiles",
        "-Wl,--no-relax", "-Wl,--unresolved-symbols=ignore-all",
        "-Wl,-Ttext=0", "-Wl,-e,0",
    };

    /**
     * UNA FAMILIA: su toolchain y sus flags. Lo demás —el nombre canónico y el
     * sufijo de la doble extensión— sale del catálogo de
     * {@link basicplus.frontend.NpackReloc#DESTINOS}, que es donde se da de
     * alta una familia nueva. Aquí sólo vive lo que es propio del PC: qué gcc
     * y con qué banderas.
     */
    private static final class Familia {
        final basicplus.frontend.NpackReloc.Destino destino;
        final String   gcc;
        final String[] cflags;
        final boolean  enlazar;      /* RISC-V necesita el paso extra de enlace */
        Familia(basicplus.frontend.NpackReloc.Destino d, String gcc,
                String[] cflags, boolean enlazar) {
            this.destino = d; this.gcc = gcc; this.cflags = cflags; this.enlazar = enlazar;
        }
        String target() { return destino.targetAot; }
        String sufijo() { return destino.sufijo; }
    }

    /** Resultado global de un pase AOT sobre el proyecto. */
    public static final class Result {
        /** `.mdn` generados (alongside de sus `.mod` en outDir). */
        public final List<Path>   mdnFiles = new ArrayList<>();
        /** Avisos no fatales (ya logueados por el callback; aquí para el caller). */
        public final List<String> warnings = new ArrayList<>();
        /** true si el toolchain ARM / includes no se pudieron resolver. */
        public boolean toolchainMissing = false;
    }

    /**
     * Compila a `.mdn` todos los módulos con funciones `native` bajo sourceDir.
     * NUNCA lanza — degrada con avisos. Debe llamarse DESPUÉS de compilar el
     * proyecto a outDir (necesita las `.bpi` de las deps allí para resolver
     * imports). Llamar desde un hilo de fondo: gcc es un subproceso.
     *
     * @param sourceDir  carpeta de fuentes (.bp) del proyecto
     * @param outDir     carpeta de salida (donde están los .mod; el .mdn va aquí)
     * @param projectDir raíz del proyecto — los intermedios van a &lt;projectDir&gt;/target/
     * @param target     "arm" (único soportado en Fase 1)
     * @param prefs      prefs del IDE (rutas del toolchain per-máquina)
     * @param log        sumidero de progreso (consola del IDE), sin newline final
     */
    public static Result buildProject(Path sourceDir, Path outDir, Path projectDir,
                                      String target, IdePrefs prefs, Consumer<String> log) {
        return build(sourceDir, null, outDir, projectDir,
                     java.util.Collections.singletonList(target), false, prefs, log);
    }

    /**
     * V5/H8 — el proyecto declara VARIAS familias (`aot.targets`) y sale un
     * `.mdn` por cada una, con su doble extensión: `SQLite.mdn.RISCV`.
     *
     * <p><b>Un build, un `.bp`, un `.mod`, un C.</b> Lo único que se repite es
     * compilar ese C intermedio con cada toolchain — el `.c` se emite UNA vez y
     * lo comparten todas. No es un ahorro de tiempo: es que así los `.mdn` del
     * pack no pueden divergir entre sí ni del bytecode que llevan al lado.
     *
     * <p>El sufijo es sólo para el PC. Al grabar, {@code PackBurn} se queda con
     * el de la placa, le quita el sufijo y poda los demás — el micro encuentra
     * `SQLite.mdn` de siempre y no se entera de que hubo hermanas.
     */
    public static Result buildPackTargets(Path sourceDir, Path outDir, Path projectDir,
                                          List<String> targets, IdePrefs prefs,
                                          Consumer<String> log) {
        return build(sourceDir, null, outDir, projectDir, targets, true, prefs, log);
    }

    /**
     * H11 — AOT de UN SOLO `.bp`, para el fichero suelto (sin proyecto). Sin
     * esto habría que pasar su carpeta como sourceDir, y entonces se compilaría
     * TODO lo que hubiera al lado: abrir un sample de `benchmarks/` dispararía
     * una pasada de gcc por cada `.bp` de la carpeta. Un fichero suelto es un
     * fichero suelto.
     */
    public static Result buildSingle(Path bpFile, Path outDir, Path workRoot,
                                     String target, IdePrefs prefs, Consumer<String> log) {
        return build(bpFile.getParent(), bpFile, outDir, workRoot,
                     java.util.Collections.singletonList(target), false, prefs, log);
    }

    /**
     * Núcleo común: si `only` != null se compila SOLO ese fichero.
     *
     * @param targets  las familias para las que compilar (una o varias).
     * @param sufijar  true = el `.mdn` sale con su doble extensión (pack);
     *                 false = nombre pelado `<Mod>.mdn`, que es lo que busca el
     *                 dispositivo en su FS al subirlo en un Run.
     */
    private static Result build(Path sourceDir, Path only, Path outDir, Path projectDir,
                                List<String> targets, boolean sufijar,
                                IdePrefs prefs, Consumer<String> log) {
        Result res = new Result();

        List<Familia> familias = new ArrayList<>();
        for (String target : targets) {
            basicplus.frontend.NpackReloc.Destino d =
                    basicplus.frontend.NpackReloc.porTargetAot(target);
            if (d == null) {
                String w = "AOT target '" + target + "' no soportado (hay: "
                    + basicplus.frontend.NpackReloc.targetsConocidos()
                    + "). Los módulos se ejecutarán interpretados.";
                res.warnings.add(w);
                log.accept("[aot] " + w);
                return res;
            }
            boolean riscv = d == basicplus.frontend.NpackReloc.RISCV32_ESP_P4;
            familias.add(new Familia(d,
                    riscv ? resolveRiscvGcc(prefs) : resolveGcc(prefs),
                    riscv ? RISCV_P4_FLAGS : ARM_M33_FLAGS,
                    /*enlazar*/ riscv));
        }
        if (familias.isEmpty()) {
            log.accept("[aot] no se ha dicho para qué familia compilar");
            return res;
        }

        String bpgenvm = resolveBpgenvm(prefs, outDir);
        if (bpgenvm == null) {
            res.toolchainMissing = true;
            String w = "AOT: no encuentro la raíz de bpgenvm-c (includes nativos). "
                + "Configúrala en Ajustes → AOT. El proyecto se ejecutará interpretado.";
            res.warnings.add(w);
            log.accept("[aot] " + w);
            return res;
        }

        // Intermedios (.c / .o) en <projectDir>/target/ — carpeta de trabajo del
        // proyecto, fuera de outDir, así que nunca se suben al device. Sólo el
        // .mdn final va a outDir (alongside del .mod) para que PicoExplorer lo coja.
        Path work = (projectDir != null ? projectDir : outDir).resolve("target");
        try { Files.createDirectories(work); } catch (IOException ignore) {}

        List<Path> bps = (only != null)
                ? java.util.Collections.singletonList(only)
                : listBpFiles(sourceDir);
        if (bps.isEmpty()) {
            log.accept("[aot] no hay .bp bajo " + sourceDir);
            return res;
        }

        for (Path bp : bps) {
            try {
                res.mdnFiles.addAll(
                        buildOne(bp, outDir, work, familias, sufijar, bpgenvm, res, log));
            } catch (ToolchainMissing tm) {
                // gcc no se pudo ni lanzar — no tiene sentido reintentar el resto.
                res.toolchainMissing = true;
                res.warnings.add(tm.getMessage());
                log.accept("[aot] " + tm.getMessage());
                break;
            } catch (Throwable t) {
                // Cualquier otro fallo en un módulo: avisar y seguir con los demás.
                String w = bp.getFileName() + ": " + t.getMessage();
                res.warnings.add("AOT omitido — " + w);
                log.accept("[aot] omitido " + w + " (se ejecutará interpretado)");
            }
        }
        return res;
    }

    /** Marca que el toolchain (gcc) no se pudo invocar. */
    private static final class ToolchainMissing extends Exception {
        ToolchainMissing(String m) { super(m); }
    }

    /**
     * AOT de un único .bp. Devuelve los `.mdn` generados —uno por familia—, o la
     * lista vacía si el módulo no tiene funciones `native` (lo normal en la
     * mayoría de módulos).
     *
     * <p><b>El `.c` se emite UNA sola vez</b> y lo comparten todas las familias:
     * el código intermedio no depende de la ISA, sólo su compilación. Emitirlo
     * por familia no sería una ineficiencia sino un riesgo — dos pasadas del
     * emisor son dos oportunidades de divergir, y el resultado serían `.mdn`
     * hermanos que no lo son.
     */
    private static List<Path> buildOne(Path bp, Path outDir, Path work,
                                       List<Familia> familias, boolean sufijar,
                                       String bpgenvm, Result res, Consumer<String> log)
            throws Exception {
        // 1) UNA VEZ: emitir aot_<Mod>.c (modo --mdn). cFile==null → sin native.
        AotMain.AotResult ar;
        try {
            ar = AotMain.emitAotC(bp, work, /*mdnMode=*/true);
        } catch (AotCEmitter.UnsupportedAotException ux) {
            String w = bp.getFileName() + ": native no AOT-able — " + ux.getMessage()
                + " (se ejecutará interpretado)";
            res.warnings.add(w);
            log.accept("[aot] " + w);
            return java.util.Collections.emptyList();
        }
        for (String w : ar.warnings) log.accept("[aot] aviso " + ar.moduleName + ": " + w);
        if (ar.cFile == null)                // sin funciones native — nada que compilar
            return java.util.Collections.emptyList();

        String mod = ar.moduleName;
        List<Path> salidas = new ArrayList<>();

        // 2) POR FAMILIA: compilar ese mismo .c con su toolchain.
        for (Familia f : familias) {
            /* Los intermedios llevan la familia en el nombre SIEMPRE. Con una
             * sola da igual; con dos, el segundo gcc pisaría el .o del primero y
             * el .mdn saldría con el código de la otra ISA — un fallo que no
             * daría error aquí, sino un cuelgue en la placa. */
            Path oFile   = work.resolve("aot_" + mod + "." + f.target() + ".o");
            Path mdnFile = outDir.resolve(sufijar ? mod + ".mdn." + f.sufijo()
                                                  : mod + ".mdn");

            // 2a) gcc → .o (ARM: PIC Thumb-2 · RISC-V: -fno-pic, resuelto al enlazar).
            List<String> cmd = new ArrayList<>();
            cmd.add(f.gcc);
            cmd.addAll(Arrays.asList(f.cflags));
            cmd.add("-I" + Paths.get(bpgenvm, "include"));
            cmd.add("-I" + Paths.get(bpgenvm, "src"));
            cmd.add("-c"); cmd.add(ar.cFile.toString());
            cmd.add("-o"); cmd.add(oFile.toString());
            runGcc(cmd, mod, log);

            // 2b) RISC-V: paso EXTRA de enlace. El .text RISC-V (a diferencia de ARM)
            // NO sale autocontenido — lleva relocalizaciones internas PC-relativas (la
            // recursión, etc.) que MdnPack no resuelve. Enlazamos a -Ttext=0 para
            // resolverlas dejando el código position-independent. Empaquetamos el .elf.
            Path packInput = oFile;
            if (f.enlazar) {
                Path elf = work.resolve("aot_" + mod + "." + f.target() + ".elf");
                List<String> lk = new ArrayList<>();
                lk.add(f.gcc);
                lk.addAll(Arrays.asList(RISCV_LINK_FLAGS));
                lk.add(oFile.toString());
                lk.add("-o"); lk.add(elf.toString());
                runGcc(lk, mod, log);
                packInput = elf;
            }

            // 2c) MdnPack → .mdn.
            MdnPack.PackResult pr = MdnPack.pack(packInput, mdnFile, mod);
            log.accept("[aot] " + mdnFile.getFileName() + " ✓ (" + pr.symbols
                + " thunk(s), " + pr.codeBytes + " B nativo)");
            salidas.add(mdnFile);
        }
        return salidas;
    }

    /** Lanza gcc y espera. Captura stdout+stderr para el diagnóstico. */
    private static void runGcc(List<String> cmd, String mod, Consumer<String> log)
            throws ToolchainMissing, IOException, InterruptedException {
        ProcessBuilder pb = new ProcessBuilder(cmd);
        pb.redirectErrorStream(true);
        Process p;
        try {
            p = pb.start();
        } catch (IOException launchFail) {
            throw new ToolchainMissing("No se pudo lanzar el compilador AOT ('"
                + cmd.get(0) + "'): " + launchFail.getMessage()
                + ". Configura la ruta del toolchain en Ajustes → AOT. "
                + "Se sube sólo el .mod (interpretado).");
        }
        StringBuilder out = new StringBuilder();
        try (BufferedReader r = new BufferedReader(
                new InputStreamReader(p.getInputStream(), StandardCharsets.UTF_8))) {
            String line;
            while ((line = r.readLine()) != null) out.append(line).append('\n');
        }
        int code = p.waitFor();
        if (code != 0) {
            throw new IOException("gcc devolvió " + code + " compilando aot_" + mod
                + ".c:\n" + out);
        }
    }

    // ============================================================
    // Resolución del toolchain (prefs → autodetect).
    // ============================================================

    /** Ruta efectiva de arm-none-eabi-gcc: prefs si está fijada, si no autodetect. */
    static String resolveGcc(IdePrefs prefs) {
        if (prefs != null && prefs.aotGccPath != null && !prefs.aotGccPath.isEmpty())
            return prefs.aotGccPath;
        return autodetectArmGcc();
    }

    /** Raíz de bpgenvm-c: prefs (si válida) → autodetect. null si no se encuentra. */
    static String resolveBpgenvm(IdePrefs prefs, Path hint) {
        if (prefs != null && prefs.aotBpgenvmDir != null && !prefs.aotBpgenvmDir.isEmpty()
                && looksLikeBpgenvm(Paths.get(prefs.aotBpgenvmDir)))
            return prefs.aotBpgenvmDir;
        return autodetectBpgenvm(hint);
    }

    /** Localiza arm-none-eabi-gcc. Devuelve la ruta concreta encontrada, o el
     *  comando pelado "arm-none-eabi-gcc" (resuelto por PATH al ejecutar) si no
     *  hay candidata fija. Nunca null. */
    static String autodetectArmGcc() {
        String exe = isWindows() ? "arm-none-eabi-gcc.exe" : "arm-none-eabi-gcc";
        // 1) Ubicaciones estándar del instalador Arm GNU Toolchain.
        String[] candidates = {
            "C:\\Program Files (x86)\\Arm\\GNU Toolchain mingw-w64-i686-arm-none-eabi\\bin\\" + exe,
            "C:\\Program Files (x86)\\Arm GNU Toolchain arm-none-eabi\\bin\\" + exe,
            "/usr/bin/" + exe,
            "/usr/local/bin/" + exe,
        };
        for (String c : candidates) {
            if (Files.isRegularFile(Paths.get(c))) return c;
        }
        // 2) Buscar en PATH.
        String path = System.getenv("PATH");
        if (path != null) {
            for (String dir : path.split(File.pathSeparator)) {
                if (dir.isEmpty()) continue;
                Path cand = Paths.get(dir, exe);
                if (Files.isRegularFile(cand)) return cand.toString();
            }
        }
        // 3) Fallback: comando pelado (PATH en exec-time).
        return "arm-none-eabi-gcc";
    }

    /** Ruta efectiva del gcc RISC-V (ESP32-P4): prefs si está fijada, si no
     *  autodetect. Reutiliza aotGccPath solo si apunta a un riscv*-gcc (heurística
     *  por nombre) para no confundirlo con el ARM. */
    static String resolveRiscvGcc(IdePrefs prefs) {
        if (prefs != null && prefs.aotGccPath != null
                && prefs.aotGccPath.toLowerCase().contains("riscv"))
            return prefs.aotGccPath;
        return autodetectRiscvGcc();
    }

    /** Localiza riscv32-esp-elf-gcc. Prioridad: toolchain de ESP-IDF en
     *  ~/.espressif/tools/riscv32-esp-elf/&lt;ver&gt;/... (el MISMO que compila el
     *  firmware → ABI casada; si hay varias versiones toma la más nueva), luego
     *  PATH, y como último recurso el comando pelado. Nunca null. */
    static String autodetectRiscvGcc() {
        String exe = isWindows() ? "riscv32-esp-elf-gcc.exe" : "riscv32-esp-elf-gcc";
        // 1) Toolchain vendido con ESP-IDF (el que usa el firmware del P4).
        String home = System.getProperty("user.home");
        if (home != null) {
            Path base = Paths.get(home, ".espressif", "tools", "riscv32-esp-elf");
            if (Files.isDirectory(base)) {
                try (Stream<Path> dirs = Files.list(base)) {
                    Optional<Path> found = dirs
                        .sorted(Comparator.reverseOrder())   // esp-15.x antes que esp-13.x
                        .map(d -> d.resolve("riscv32-esp-elf").resolve("bin").resolve(exe))
                        .filter(Files::isRegularFile)
                        .findFirst();
                    if (found.isPresent()) return found.get().toString();
                } catch (IOException ignore) { /* cae a PATH */ }
            }
        }
        // 2) Buscar en PATH.
        String path = System.getenv("PATH");
        if (path != null) {
            for (String dir : path.split(File.pathSeparator)) {
                if (dir.isEmpty()) continue;
                Path cand = Paths.get(dir, exe);
                if (Files.isRegularFile(cand)) return cand.toString();
            }
        }
        // 3) Fallback: comando pelado (PATH en exec-time).
        return "riscv32-esp-elf-gcc";
    }

    /** Localiza la raíz de bpgenvm-c (la que tiene include/ y src/). Parte de
     *  `hint` (p.ej. el outDir del proyecto) caminando hacia arriba buscando un
     *  hermano "bpgenvm-c", luego de user.dir, y por último la ubicación de
     *  desarrollo conocida. null si no la encuentra (caller degrada con aviso). */
    static String autodetectBpgenvm(Path hint) {
        List<Path> starts = new ArrayList<>();
        if (hint != null) starts.add(hint);
        String userDir = System.getProperty("user.dir");
        if (userDir != null) starts.add(Paths.get(userDir));
        for (Path start : starts) {
            Path p = start.toAbsolutePath();
            for (int up = 0; up < 8 && p != null; up++, p = p.getParent()) {
                if (looksLikeBpgenvm(p)) return p.toString();          // hint ya es bpgenvm-c
                Path cand = p.resolve("bpgenvm-c");
                if (looksLikeBpgenvm(cand)) return cand.toString();
            }
        }
        // La INSTALACIÓN: en el ZIP viaja un bpgenvm-c/ con include/ y src/ (sólo
        // las cabeceras — el AOT las usa como -I, no compila esos .c). Va DESPUÉS
        // del árbol de fuentes a propósito: si estás en el repo, manda el repo.
        Path dist = IdePrefs.installSubdir("bpgenvm-c");
        if (looksLikeBpgenvm(dist)) return dist.toString();
        Path dev = Paths.get("C:\\lenguajes\\pm\\bpgenvm-c");          // último recurso (dev)
        if (looksLikeBpgenvm(dev)) return dev.toString();
        return null;
    }

    /** Un directorio "parece" bpgenvm-c si tiene include/ y src/. */
    static boolean looksLikeBpgenvm(Path dir) {
        return dir != null
            && Files.isDirectory(dir.resolve("include"))
            && Files.isDirectory(dir.resolve("src"));
    }

    private static boolean isWindows() {
        return System.getProperty("os.name", "").toLowerCase().contains("win");
    }

    /** Lista recursiva de .bp bajo dir (vacía si dir no existe). */
    private static List<Path> listBpFiles(Path dir) {
        List<Path> out = new ArrayList<>();
        if (dir == null || !Files.isDirectory(dir)) return out;
        try {
            Files.walk(dir)
                 .filter(Files::isRegularFile)
                 .filter(p -> p.getFileName().toString().endsWith(".bp"))
                 .forEach(out::add);
        } catch (IOException ignore) { /* devolvemos lo que haya */ }
        return out;
    }
}
