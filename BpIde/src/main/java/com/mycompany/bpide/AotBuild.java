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
import java.util.List;
import java.util.function.Consumer;

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
        Result res = new Result();

        if (target != null && !target.equalsIgnoreCase("arm")) {
            String w = "AOT target '" + target + "' no soportado en V3 (sólo 'arm' = "
                + "Cortex-M33 para RP2350/STM32). Los módulos se ejecutarán interpretados.";
            res.warnings.add(w);
            log.accept("[aot] " + w);
            return res;
        }

        String gcc     = resolveGcc(prefs);
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

        List<Path> bps = listBpFiles(sourceDir);
        if (bps.isEmpty()) {
            log.accept("[aot] no hay .bp bajo " + sourceDir);
            return res;
        }

        for (Path bp : bps) {
            try {
                Path mdn = buildOne(bp, outDir, work, gcc, bpgenvm, res, log);
                if (mdn != null) res.mdnFiles.add(mdn);
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

    /** AOT de un único .bp. Devuelve el `.mdn` generado, o null si el módulo no
     *  tiene funciones `native` (lo normal para la mayoría de módulos). */
    private static Path buildOne(Path bp, Path outDir, Path work, String gcc,
                                 String bpgenvm, Result res, Consumer<String> log)
            throws Exception {
        // 1) Emitir aot_<Mod>.c (modo --mdn). cFile==null → sin native.
        AotMain.AotResult ar;
        try {
            ar = AotMain.emitAotC(bp, work, /*mdnMode=*/true);
        } catch (AotCEmitter.UnsupportedAotException ux) {
            String w = bp.getFileName() + ": native no AOT-able — " + ux.getMessage()
                + " (se ejecutará interpretado)";
            res.warnings.add(w);
            log.accept("[aot] " + w);
            return null;
        }
        for (String w : ar.warnings) log.accept("[aot] aviso " + ar.moduleName + ": " + w);
        if (ar.cFile == null) return null;   // sin funciones native — nada que compilar

        String mod   = ar.moduleName;
        Path oFile   = work.resolve("aot_" + mod + ".o");
        Path mdnFile = outDir.resolve(mod + ".mdn");

        // 2) arm-none-eabi-gcc → .o (PIC Thumb-2).
        List<String> cmd = new ArrayList<>();
        cmd.add(gcc);
        cmd.addAll(Arrays.asList(ARM_M33_FLAGS));
        cmd.add("-I" + Paths.get(bpgenvm, "include"));
        cmd.add("-I" + Paths.get(bpgenvm, "src"));
        cmd.add("-c"); cmd.add(ar.cFile.toString());
        cmd.add("-o"); cmd.add(oFile.toString());
        runGcc(cmd, mod, log);

        // 3) MdnPack → .mdn (alongside del .mod en outDir; PicoExplorer lo sube).
        MdnPack.PackResult pr = MdnPack.pack(oFile, mdnFile, mod);
        log.accept("[aot] " + mod + ".mdn ✓ (" + pr.symbols + " thunk(s), "
            + pr.codeBytes + " B nativo)");
        return mdnFile;
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
            throw new ToolchainMissing("No se pudo lanzar arm-none-eabi-gcc ('"
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
