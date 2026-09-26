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

    /* ═══ [V7/H2, #441] LOS FLAGS YA NO VIVEN AQUÍ ═══════════════════════════
     *
     * Aquí había dos arrays (`ARM_M33_FLAGS`, `RISCV_P4_FLAGS`) con un aviso:
     * «DEBEN coincidir con build_mdn.sh y con el loader del firmware». Tres
     * sitios para el mismo dato y NADIE que los comparara: el 17-ago añadir
     * `-mcmodel=medany` dejó inservibles todos los `.mdn` RISC-V ya generados
     * sin que ni el gate de `arch` ni las fechas lo vieran.
     *
     * Ahora la ÚNICA fuente es `bpgenvm-c/include/bpvm_aot_recetas.h`: una
     * receta por juego de instrucciones y ABI (`ARMV8M_MAIN_SOFTFP`,
     * `RV32IMAFC_ILP32F`), que el IDE parsea aquí ({@link basicplus.frontend.AotRecetas})
     * y el firmware hashea con las macros de su ISA. El `.mdn` (v2) lleva el CRC32
     * del literal; la placa rechaza el que no coincide y lo publica en el INFO
     * (`aotReceta`/`aotFlagsCrc`) para que el IDE no lo suba.
     *
     * Qué receta usa cada destino lo dice `NpackReloc.Destino.receta`. Lo que
     * sigue es la HISTORIA de por qué cada flag está en la receta de RISC-V —
     * se queda, porque cada uno costó un cuelgue mudo.
     *
     *  ⚠️ Este comentario decía que NO hacía falta fijar `-march`/`-mabi` porque
     *  «casan por construcción». Era falso y costó un cuelgue — ver la nota larga
     *  de abajo (25-ago). A diferencia de ARM, el .text
     *  RISC-V NO sale autocontenido (la llamada recursiva, refs PC-relativas dejan
     *  relocalizaciones que MdnPack no resuelve) → hace falta el paso de enlace
     *  (LINK_FLAGS_COMUNES + `--no-relax`). -mno-relax evita las relocs de relajación.
     *
     *  #440 — `-mcmodel=medany` NO es opcional: es lo que hace el .mdn cargable
     *  fuera de la dirección 0. Enlazar a -Ttext=0 deja relativos los SALTOS,
     *  pero con el modelo por defecto (medlow) los DATOS se direccionan en
     *  absoluto. Un literal en el offset 0x288 salía así:
     *
     *      lui  a1,0x0  /  add a1,a1,648      ← a1 = 648, dirección absoluta
     *
     *  o sea que en la placa `string_from_cstr` recibía el puntero 648 y se
     *  ponía a leer memoria salvaje: cuelgue. ARM no lo sufre porque va con
     *  -fpic y remata con `add r1, pc`. Con medany sale el equivalente:
     *
     *      auipc a1,0x0 /  add a1,a1,528      ← a1 = PC + offset
     *
     *  Medido desensamblando las dos familias: 3 refs absolutas → 0. Y hace
     *  falta el -mno-relax de arriba, que la relajación deshace los auipc. */
    /* ⚠️ `-march`/`-mabi` EXPLICITOS, y son obligatorios — 25-ago-2026.
     *
     * Aqui ponia que no hacia falta fijarlos: *«el toolchain esp trae el del P4
     * por DEFECTO y el firmware tampoco los pasa -> casan por construccion»*.
     * MEDIDO, es falso, y por los dos lados:
     *
     *              arch                         ABI
     *   firmware   rv32i m a f c  (SIN `d`)     single-float (ilp32f)
     *   .mdn       rv32i m a f D c  (CON `d`)   soft-float   (ilp32)
     *
     * El defecto del toolchain incluye la extension `d` — coma flotante de
     * DOBLE en hardware, que el ESP32-P4 NO TIENE. Con doubles vivos gcc los
     * derrama a la pila con `fld`/`fsd`, y eso en el P4 es instruccion ilegal:
     * la placa se CUELGA. Se vio con `NatNew.mediaDobles` (un `double[]`), y el
     * desensamblado lo dejo exacto: 6 instrucciones de doble, TODAS en el thunk
     * de esa funcion. Lo demas del sample habia pasado.
     *
     * Y el ABI tambien difiere: soft-float pasa los `float` en registros
     * enteros y single-float en registros de coma flotante. Los `double` van en
     * pares de enteros con los dos, y por eso `mediaPor2` (V6/NatV7) funciono y
     * escondio el problema: el primer sintoma necesito un `float`/`double`
     * DERRAMADO, no solo pasado.
     *
     * 📌 No se copia la cadena entera del firmware: lleva extensiones propias de
     * Espressif (`xesploop`, `xespv`) que el `.mdn` no necesita. Lo que tiene que
     * casar es lo que decide la ABI y el repertorio: `imafc` + `ilp32f`. */

    /** Para `AotRiscvPicSmoke`, que compila el mismo `.c` con y sin `medany`:
     *  los flags salen de la receta REAL del `.h` (la del destino RISC-V), no de
     *  una copia que se quede rancia. Falla si no se encuentra bpgenvm-c. */
    static String[] flagsRiscvParaPruebas() throws IOException {
        String bpgenvm = resolveBpgenvm(null, null);
        if (bpgenvm == null) throw new IOException("no encuentro la raíz de bpgenvm-c (las recetas del .mdn)");
        basicplus.frontend.AotRecetas r = basicplus.frontend.AotRecetas.deBpgenvm(Paths.get(bpgenvm));
        return basicplus.frontend.AotRecetas.flags(
                r.exigir(basicplus.frontend.NpackReloc.RISCV32_ESP_P4.receta));
    }

    /** Paso EXTRA de RISC-V (bifurcación A, decidida con Eduardo): enlazar el .o a
     *  -Ttext=0 resolviendo las relocalizaciones internas PC-relativas → el .text
     *  queda autocontenido y position-independent (todo relativo al PC). MdnPack lee
     *  el .elf enlazado igual que un .o; con -Ttext=0, valor de símbolo == offset. */
    /** #428 — lo que el enlace del `.mdn` necesita en CUALQUIER familia. La
     *  dirección la fija el guión (`bpgenvm-c/aot/mdn.ld`), que además mete los
     *  literales dentro de `.text` — sin eso, una `native` con una cadena
     *  produce un `.rodata` que el `.mdn` no puede llevarse.
     *
     *  [V7/H5 #559, 25-sep] Ya SIN `--unresolved-symbols=ignore-all`. Estaba por el
     *  `aot_<Mod>_register` del `.c`, que llama al registro del firmware — pero en
     *  modo `.mdn` esa función ya no se emite, y lo único que dejaba pasar el flag era
     *  lo que NO debía: una llamada de gcc a libgcc (`__addsf3`) quedaba en la
     *  dirección 0, que es la primera función del propio `.mdn`, y la C5 se reiniciaba
     *  al sumar dos `float`. Ahora eso lo para antes `exigirSinSimbolosExternos`, con
     *  nombre; y si algo se le escapara, el enlace falla en vez de callar. */
    private static final String[] LINK_FLAGS_COMUNES = {
        "-nostdlib", "-nostartfiles",
        "-Wl,-e,0",
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
        final String   receta;       /* [V7/H2, #441] el literal del .h: se hashea en la cabecera */
        final String[] cflags;       /* la receta partida, para gcc */
        final boolean  enlazar;      /* #428 — hoy SIEMPRE: ver LINK_FLAGS_COMUNES */
        Familia(basicplus.frontend.NpackReloc.Destino d, String gcc,
                String receta, boolean enlazar) {
            this.destino = d; this.gcc = gcc; this.receta = receta;
            this.cflags = basicplus.frontend.AotRecetas.flags(receta);
            this.enlazar = enlazar;
        }
        String target() { return destino.targetAot; }
        String sufijo() { return destino.sufijo; }
        /** RISC-V necesita además `--no-relax` al enlazar: la relajación genera
         *  relocalizaciones que `MdnPack` no resuelve. */
        boolean enlazarRiscv() {
            return destino.esRiscv();   /* V7/H6: los dos destinos RISC-V, no sólo el del P4 */
        }
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
                                      List<String> dependencias,
                                      String target, IdePrefs prefs, Consumer<String> log) {
        return build(sourceDir, (List<Path>) null, outDir, projectDir, dependencias,
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
        return buildPackTargets(sourceDir, null, outDir, projectDir, targets, prefs, log);
    }

    /**
     * V5/H8 — igual, pero con los `.bp` DICHOS uno a uno (`sources` del
     * proyecto). Si `soloEstos` es null se barre `sourceDir` como siempre.
     *
     * <p>No es sólo comodidad: barrer una carpeta que resulta ser la stdlib
     * entera lanza el emisor por cada `.bp` que hay dentro. Con la lista se
     * compila lo que se ha pedido y nada más.
     */
    public static Result buildPackTargets(Path sourceDir, List<Path> soloEstos,
                                          Path outDir, Path projectDir,
                                          List<String> targets, IdePrefs prefs,
                                          Consumer<String> log) {
        return buildPackTargets(sourceDir, soloEstos, outDir, projectDir,
                java.util.Collections.<String>emptyList(), targets, prefs, log);
    }

    /** #542 — con las {@code dependencies} del proyecto: el AOT resuelve los imports con ellas, como
     *  el compilador (el paso AOT del pack, {@code FrmMain.pasoAotDelPack}). */
    public static Result buildPackTargets(Path sourceDir, List<Path> soloEstos,
                                          Path outDir, Path projectDir,
                                          List<String> dependencias,
                                          List<String> targets, IdePrefs prefs,
                                          Consumer<String> log) {
        return build(sourceDir, soloEstos, outDir, projectDir, dependencias, targets, true, prefs, log);
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
        return build(bpFile.getParent(), java.util.Collections.singletonList(bpFile),
                     outDir, workRoot, java.util.Collections.<String>emptyList(),
                     java.util.Collections.singletonList(target), false, prefs, log);
    }

    /**
     * Núcleo común: si `only` != null se compila SOLO ese fichero.
     *
     * @param dependencias las {@code dependencies} del proyecto (#542): con ellas y con outDir,
     *                 el AOT busca los imports donde los buscó el compilador.
     * @param targets  las familias para las que compilar (una o varias).
     * @param sufijar  true = el `.mdn` sale con su doble extensión (pack);
     *                 false = nombre pelado `<Mod>.mdn`, que es lo que busca el
     *                 dispositivo en su FS al subirlo en un Run.
     */
    private static Result build(Path sourceDir, List<Path> soloEstos, Path outDir,
                                Path projectDir, List<String> dependencias,
                                List<String> targets, boolean sufijar,
                                IdePrefs prefs, Consumer<String> log) {
        Result res = new Result();

        /* La raíz de bpgenvm-c va PRIMERO: de ahí salen los includes nativos y,
         * desde V7/H2 (#441), las recetas de compilación de cada destino. */
        String bpgenvm = resolveBpgenvm(prefs, outDir);
        if (bpgenvm == null) {
            res.toolchainMissing = true;
            String w = "AOT: no encuentro la raíz de bpgenvm-c (includes nativos). "
                + "Configúrala en Ajustes → AOT. El proyecto se ejecutará interpretado.";
            res.warnings.add(w);
            log.accept("[aot] " + w);
            return res;
        }
        basicplus.frontend.AotRecetas recetas;
        try {
            recetas = basicplus.frontend.AotRecetas.deBpgenvm(Paths.get(bpgenvm));
        } catch (IOException e) {
            res.toolchainMissing = true;
            String w = "AOT: " + e.getMessage() + ". Los módulos se ejecutarán interpretados.";
            res.warnings.add(w);
            log.accept("[aot] " + w);
            return res;
        }

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
            boolean riscv = d.esRiscv();   /* V7/H6: P4 (ilp32f) y C3/C6/C5 (ilp32) */
            /* #428 — ENLAZAN LAS DOS. RISC-V ya lo hacía (su `.text` no sale
             * autocontenido); ARM no, y por eso una `native` no podía llevar ni
             * un literal de cadena: el literal se queda en `.rodata`, fuera de
             * lo único que un `.mdn` se lleva. El guión de enlace lo mete
             * DENTRO de `.text`, y eso hace falta en las dos familias. */
            String receta;
            try {
                receta = recetas.exigir(d.receta);
            } catch (IOException e) {
                String w = "AOT: " + e.getMessage() + ". Los módulos se ejecutarán interpretados.";
                res.warnings.add(w);
                log.accept("[aot] " + w);
                return res;
            }
            log.accept(String.format("[aot] %s: receta %s (huella %08x) de %s", d.targetAot,
                    d.receta, basicplus.frontend.AotRecetas.crc(receta), recetas.origen().getFileName()));
            familias.add(new Familia(d,
                    riscv ? resolveRiscvGcc(prefs) : resolveGcc(prefs),
                    receta,
                    /*enlazar*/ true));
        }
        if (familias.isEmpty()) {
            log.accept("[aot] no se ha dicho para qué familia compilar");
            return res;
        }

        // Intermedios (.c / .o) en <projectDir>/target/ — carpeta de trabajo del
        // proyecto, fuera de outDir, así que nunca se suben al device. Sólo el
        // .mdn final va a outDir (alongside del .mod) para que PicoExplorer lo coja.
        Path work = (projectDir != null ? projectDir : outDir).resolve("target");
        try { Files.createDirectories(work); } catch (IOException ignore) {}

        List<Path> bps = (soloEstos != null && !soloEstos.isEmpty())
                ? soloEstos
                : listBpFiles(sourceDir);
        if (bps.isEmpty()) {
            log.accept("[aot] no hay .bp bajo " + sourceDir);
            return res;
        }

        for (Path bp : bps) {
            try {
                res.mdnFiles.addAll(
                        buildOne(bp, outDir, work, dependencias, familias, sufijar, bpgenvm, res, log));
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
                                       List<String> dependencias,
                                       List<Familia> familias, boolean sufijar,
                                       String bpgenvm, Result res, Consumer<String> log)
            throws Exception {
        // 1) UNA VEZ: emitir aot_<Mod>.c (modo --mdn). cFile==null → sin native.
        AotMain.AotResult ar;
        try {
            /* #542 — el C a `work`, pero los imports se buscan en outDir (donde el build dejó los
             * .mod de la app) y en las dependencies: si no, `import Y` de la propia app «falta». */
            ar = AotMain.emitAotC(bp, work, /*mdnMode=*/true, outDir, dependencias);
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

            /* #440 — el .o TIENE que direccionar sus datos relativo al PC. Se
             * comprueba AQUÍ y no después porque al enlazar las relocs se
             * consumen y ya no se distingue una de otra: en el .elf las dos
             * variantes son bytes plausibles. Y se mira el .o, no el .c, que
             * esto lo decide el compilador, no lo que escribimos.
             *
             * El .mdn se carga donde caiga, así que una dirección absoluta es
             * un puntero salvaje en la placa — y no revienta: se CUELGA. */
            exigirDireccionamientoRelativo(oFile, f, mod);

            /* [V7/H5 #559] — y que no llame a NADA que el `.mdn` no se lleve. Se mira
             * también aquí, en el `.o`: enlazado, una llamada a un símbolo que falta es
             * un salto como otro cualquiera. */
            exigirSinSimbolosExternos(oFile, f, mod);

            // 2b) RISC-V: paso EXTRA de enlace. El .text RISC-V (a diferencia de ARM)
            // NO sale autocontenido — lleva relocalizaciones internas PC-relativas (la
            // recursión, etc.) que MdnPack no resuelve. Enlazamos a -Ttext=0 para
            // resolverlas dejando el código position-independent. Empaquetamos el .elf.
            Path packInput = oFile;
            if (f.enlazar) {
                Path elf = work.resolve("aot_" + mod + "." + f.target() + ".elf");
                List<String> lk = new ArrayList<>();
                lk.add(f.gcc);
                lk.addAll(Arrays.asList(LINK_FLAGS_COMUNES));
                /* RISC-V necesita además desactivar la relajación, que genera
                 * relocs que MdnPack no resuelve. */
                if (f.enlazarRiscv()) lk.add("-Wl,--no-relax");
                lk.add("-Wl,-T," + Paths.get(bpgenvm, "aot", "mdn.ld"));
                lk.add(oFile.toString());
                lk.add("-o"); lk.add(elf.toString());
                runGcc(lk, mod, log);
                packInput = elf;
            }

            // 2c) MdnPack → .mdn.
            /* [V6/N1.4] El empaquetado se hace EN MEMORIA y de ahí salen las DOS
             * cosas, sin releer nada del disco. Idea de Eduardo. */
            MdnPack.Empaquetado emp = MdnPack.empaquetar(packInput, mod, f.receta);
            MdnPack.PackResult pr = emp.resultado;

            /* [25-ago-2026] AQUEL «se invierte hasta que el .mdn suelto
             * desaparezca» era por esto, y ese día llegó: las TRES familias
             * leen ya la sección del propio `.mod`, verificado en placa
             * (Pico 23-ago, P4 y STM32 25-ago). El `.mdn` suelto de la APP no
             * se escribe más — no hay dos ficheros que puedan desparejarse,
             * que era la clase entera de fallo. */

            /* (a) El blob, fundido en el `.mod` (sección `native`, v7).
             *     Acumulativo: con varias familias, cada vuelta añade el suyo y
             *     el módulo acaba llevándolas todas, cada una con su `arch`.
             *     Tolerante: si el `.mod` no está, se avisa y se sigue — el `.mdn`
             *     de (b) mantiene el AOT funcionando igual. */
            java.nio.file.Path modFile = outDir.resolve(mod + ".mod");
            if (java.nio.file.Files.exists(modFile)) {
                byte[] antes = java.nio.file.Files.readAllBytes(modFile);
                byte[] fundido = edu.bpgenvm.bytecode.ModFormat.conBloqueNativo(antes, emp.bytes);
                java.nio.file.Files.write(modFile, fundido);
                log.accept("[aot] " + modFile.getFileName() + " lleva dentro el nativo de "
                    + f.target() + " (" + emp.bytes.length + " B, sección native — "
                    + antes.length + " -> " + fundido.length + " B)");
            } else {
                log.accept("[aot] aviso: no encuentro " + modFile.getFileName()
                    + " para fundir el nativo; queda el .mdn suelto");
            }

            /* (b) El `.mdn` SUELTO sólo se escribe cuando hace falta de verdad:
             *
             *     - `sufijar` (pipeline de PACKS): el `SQLite.mdn.RISCV` y
             *       hermanos son artefactos con nombre propio, siguen igual.
             *     - la fusión FALLÓ (no había `.mod`): el aviso de arriba ya
             *       dijo «queda el .mdn suelto», y es el paracaídas.
             *
             *     Para la APP con fusión hecha, NO se escribe — petición de
             *     Eduardo (25-ago): «si quitas que genere el .mdn me haces un
             *     favor, lo que tengo que borrar en cada prueba». Y de paso se
             *     RETIRA el rancio que quedara de antes en outDir: si se queda,
             *     cada compilación repite el aviso de «más viejo que su .mod»
             *     para un fichero que ya no pinta nada. */
            boolean fundido = !sufijar && java.nio.file.Files.exists(modFile);
            if (fundido) {
                if (java.nio.file.Files.deleteIfExists(mdnFile)) {
                    log.accept("[aot] retirado " + mdnFile.getFileName()
                        + " (el nativo va DENTRO del .mod desde N1.4)");
                }
                log.accept("[aot] " + mod + ": " + pr.symbols + " thunk(s), "
                    + pr.codeBytes + " B nativo — dentro del .mod");
            } else {
                java.nio.file.Files.write(mdnFile, emp.bytes);
                salidas.add(mdnFile);
                log.accept("[aot] " + mdnFile.getFileName() + " ✓ (" + pr.symbols
                    + " thunk(s), " + pr.codeBytes + " B nativo)");
            }
        }
        return salidas;
    }

    /* #440 — RISC-V: relocalizaciones de direccionamiento ABSOLUTO. Son las que
     * emite el modelo `medlow` (el de por defecto) para llegar a un símbolo:
     * `lui`+`addi` con la dirección de enlace metida como constante. Un `lui` de
     * constante grande NO lleva relocalización, así que ver una de éstas en el
     * `.text` significa siempre lo mismo: el código sólo vale cargado en la
     * dirección de enlace. Y el `.mdn` se carga donde caiga.
     *
     * Medido con `NatEsc` (3 literales) en las dos direcciones:
     *    sin `-mcmodel=medany`  →  {26×3, 27×3}   ← absoluto, cuelga en placa
     *    con `-mcmodel=medany`  →  {23×3, 24×3}   ← PCREL_HI20/LO12_I, correcto
     * y el control de la familia que SÍ funciona, ARM: {3×3} = R_ARM_REL32. */
    private static final int R_RISCV_HI20 = 26, R_RISCV_LO12_I = 27, R_RISCV_LO12_S = 28;

    /** Comprueba que el `.o` de RISC-V direcciona sus datos relativo al PC.
     *  Se mira el `.o` y no el `.elf` porque al enlazar las relocalizaciones se
     *  consumen: en el enlazado las dos variantes son bytes igual de plausibles
     *  y ya no hay nada que distinguir. Sólo RISC-V — ARM va con `-fpic`, que no
     *  tiene este modo de fallo (y está verificado en placa). */
    private static void exigirDireccionamientoRelativo(Path oFile, Familia f, String mod)
            throws IOException {
        if (!f.enlazarRiscv()) return;
        int abs = relocsAbsolutasRiscv(oFile);
        if (abs > 0) {
            throw new IOException(
                "AOT " + mod + " (" + f.target() + "): el .o direcciona datos en ABSOLUTO ("
                + abs + " relocalización(es) R_RISCV_HI20/LO12). El .mdn se carga en"
                + " cualquier dirección, así que en la placa saldría un puntero salvaje"
                + " y un cuelgue mudo. Falta '-mcmodel=medany' en la receta RISC-V de bpvm_aot_recetas.h (#440).");
        }
    }

    /** [V7/H5 #559, 25-sep] El `.o` no puede usar NINGÚN símbolo que no defina él.
     *
     *  Un `.mdn` se carga sin enlazador: todo lo que el código nativo necesita del
     *  firmware lo alcanza por la tabla `vm->aot_helpers`, nunca por nombre. Así que un
     *  símbolo sin definir en el `.o` es siempre un fallo — casi siempre una rutina de
     *  libgcc que gcc llama por su cuenta cuando el micro no tiene la instrucción
     *  (`__addsf3` sin FPU, `__aeabi_i2d` en cualquier Cortex-M, `memset` para poner a
     *  cero un bloque). El guardián de `MdnPack` lo sabía decir, pero miraba el `.elf`
     *  ENLAZADO, donde las relocalizaciones ya se han consumido: nunca saltaba, y el
     *  enlace (con `--unresolved-symbols=ignore-all`) dejaba la llamada en la dirección
     *  0. La C5 se reiniciaba al sumar dos `float`, sin un mensaje (lo cazó la tanda).
     *
     *  El error dice qué `native` lo pide, que es lo que el usuario puede tocar. */
    private static void exigirSinSimbolosExternos(Path oFile, Familia f, String mod)
            throws IOException {
        java.util.Map<String, java.util.Set<String>> fuera = simbolosSinResolver(oFile, mod);
        if (fuera.isEmpty()) return;
        StringBuilder sb = new StringBuilder();
        sb.append("AOT ").append(mod).append(" (").append(f.target())
          .append("): el código nativo llama a ").append(fuera.size())
          .append(" rutina(s) que un .mdn no se lleva:\n");
        for (java.util.Map.Entry<String, java.util.Set<String>> e : fuera.entrySet()) {
            sb.append("    - ").append(e.getKey());
            if (!e.getValue().isEmpty()) sb.append("   (la usa: ").append(String.join(", ", e.getValue())).append(")");
            sb.append("\n");
        }
        sb.append("  En la placa, cada llamada saltaría a la dirección 0: un cuelgue o un reinicio, sin mensaje (#559).\n")
          .append("  Es un fallo del EMISOR del AOT, no del programa: lo que el micro no hace en hardware\n")
          .append("  (la coma flotante sin FPU, el double, dividir un long...) tiene que ir por un helper\n")
          .append("  de la tabla, y esto se le ha escapado. Mientras se arregla, quítale 'native' a esa\n")
          .append("  función: corre interpretada y el resto del módulo sigue igual.");
        throw new IOException(sb.toString());
    }

    /** Los símbolos que el `.o` usa sin definirlos, cada uno con las `native` que lo
     *  piden (la función que contiene la relocalización: el `thunk_`/`aot_` de mayor
     *  dirección que no pasa de ella). Separado de la guarda para poder medirlo. */
    static java.util.Map<String, java.util.Set<String>> simbolosSinResolver(Path oFile, String mod)
            throws IOException {
        basicplus.frontend.Elf32 elf =
                basicplus.frontend.Elf32.parse(Files.readAllBytes(oFile));
        int text = elf.findSectionIndex(".text");
        /* Las funciones del `.text`, por dirección (en Thumb el bit 0 es el modo, no
         * la dirección). Las `aot_` se suelen quedar dentro de su thunk (static e
         * inline): el nombre que sale es entonces el del thunk, que es la misma native. */
        java.util.TreeMap<Integer, String> funciones = new java.util.TreeMap<>();
        String pa = "aot_" + mod + "_", pt = "thunk_" + mod + "_";
        for (basicplus.frontend.Elf32.Symbol sym : elf.symbols()) {
            if (sym.shndx != text) continue;
            String n = sym.name;
            int k = n.indexOf('.');                        /* clones de gcc: `.constprop.0`... */
            String base = (k > 0) ? n.substring(0, k) : n;
            if (base.startsWith(pt)) funciones.put(sym.value & ~1, base.substring(pt.length()));
            else if (base.startsWith(pa)) funciones.put(sym.value & ~1, base.substring(pa.length()));
        }
        java.util.Map<String, java.util.Set<String>> fuera = new java.util.TreeMap<>();
        for (basicplus.frontend.Elf32.Reloc r : elf.relocs(".text")) {
            basicplus.frontend.Elf32.Symbol sym = elf.symbol(r.symIdx);
            if (sym == null || sym.shndx != 0 || sym.name.isEmpty()) continue;   /* SHN_UNDEF, con nombre */
            java.util.Map.Entry<Integer, String> fn = funciones.floorEntry(r.offset);
            fuera.computeIfAbsent(sym.name, x -> new java.util.TreeSet<>());
            if (fn != null) fuera.get(sym.name).add(fn.getValue());
        }
        /* Y los que no toca ninguna reloc del `.text` (otra sección con código, o una
         * referencia que el ensamblador resolvió a medias): también son un fallo. */
        for (basicplus.frontend.Elf32.Symbol sym : elf.symbols()) {
            if (sym.shndx == 0 && !sym.name.isEmpty())
                fuera.computeIfAbsent(sym.name, x -> new java.util.TreeSet<>());
        }
        return fuera;
    }

    /** Cuántas relocalizaciones de direccionamiento ABSOLUTO hay en el `.text`
     *  de un `.o` de RISC-V. Separado de la guarda para que `AotRiscvPicSmoke`
     *  pueda medir las dos variantes y comprobar que esto DISTINGUE — una
     *  guarda sin control es un instrumento mudo. */
    static int relocsAbsolutasRiscv(Path oFile) throws IOException {
        basicplus.frontend.Elf32 elf =
                basicplus.frontend.Elf32.parse(Files.readAllBytes(oFile));
        int abs = 0;
        for (basicplus.frontend.Elf32.Reloc r : elf.relocs(".text")) {
            if (r.type == R_RISCV_HI20 || r.type == R_RISCV_LO12_I
                    || r.type == R_RISCV_LO12_S) abs++;
        }
        return abs;
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
