// ============================================================
// PackStep.java — H3 Packs: el paso de empaquetado del build.
//
// El COMPILADOR arma la LISTA PLANA de ficheros (lee el descriptor del
// proyecto, decide qué entra, sintetiza el manifest) y se la pasa a la librería
// del formato (Pack.jar). Pack.jar se queda tonto: solo empaqueta la lista.
//
// Qué entra en el pack (pack EJECUTABLE, modelo jar):
//   - del outDir: los .mod y .mdn (los artefactos ejecutables del build).
//     NUNCA .bpi (solo del compilador) ni .slots (debug).
//   - los resources del proyecto (projectDir/resources/*), si existen.
//   - el MANIFEST (tipo 'mft', "main=<módulo>"), que hace el pack ejecutable.
// ============================================================
package basicplus.frontend;

import basicplus.pack.PackEntry;
import basicplus.pack.PackException;
import basicplus.pack.PackFormat;
import basicplus.pack.PackWriter;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public final class PackStep {

    private PackStep() {}

    /**
     * Extensiones del outDir que van al pack (ejecutables del build).
     *
     * <p>`npk` entró en V5/H8 (D4): el motor nativo es un tipo de PRIMERA, no un
     * recurso. Entrando por aquí queda dentro de las comprobaciones de «este
     * pack se puede ejecutar» que se hacen abajo (#361), en vez de colarse por
     * `resources/`, que acepta cualquier cosa sin mirarla.
     */
    private static final Set<String> OUTDIR_TYPES =
            new HashSet<>(Arrays.asList("mod", "mdn", "npk"));

    /**
     * Los tipos que pueden venir con DOBLE EXTENSIÓN, uno por destino
     * (V5/H8, D1): `sqlite.npk.RISCV`, `SQLite.mdn.ARMV8`.
     *
     * <p>Un `.mod` NUNCA la lleva: es bytecode portable, el mismo para todas
     * las placas. Si apareciera un `Algo.mod.RISCV` sería un error de quien lo
     * construyó, no un caso a soportar.
     */
    private static final Set<String> TIPOS_POR_DESTINO =
            new HashSet<>(Arrays.asList("mdn", "npk"));

    /**
     * V5/H8 — LO QUE EL BUILD SABE Y EL PACK NECESITA SABER.
     *
     * <p>Pregunta de Eduardo: «si un usuario construye un pack, ¿cómo sabe qué
     * dependencias va a necesitar?». Esto es la respuesta: el compilador lo sabe
     * —recorre el cierre de imports para compilar— y aquí lo entrega.
     */
    public static final class Cierre {
        /** Módulo → el `.mod` PRECOMPILADO que USÓ el compilador. Los que no
         *  compila este build: la stdlib y demás. Van DENTRO del pack. */
        public final Map<String, Path> modsExternos;
        /** Fourcc de los packs de los que se importa (`from pack "SQLI"`). Son
         *  las dependencias OPCIONALES: hay que grabarlas aparte, y es lo que
         *  el usuario no puede adivinar. */
        public final Set<String> packsRequeridos;
        /** ¿El módulo raíz tiene `main`? Si no, es una LIBRERÍA. */
        public final boolean ejecutable;
        /** V5/H11 (#365) — el NOMBRE DE LA ENTRADA del módulo de arranque dentro
         *  del pack: su nombre canónico, cualificado si declara `library`
         *  (`com.example.Demo`). Vacío = no se sabe, y entonces se usa
         *  `proj.main`, que es lo de siempre y sigue valiendo sin `library`.
         *
         *  <p>Es un dato del COMPILADOR, no del proyecto: `proj.main` nombra el
         *  fichero fuente (`Demo.bp`) y no hay forma de escribir ahí el nombre
         *  cualificado — ése era exactamente el callejón sin salida de #365. */
        public final String mainEntry;

        public Cierre(Map<String, Path> mods, Set<String> packs, boolean ejec) {
            this(mods, packs, ejec, "");
        }
        public Cierre(Map<String, Path> mods, Set<String> packs, boolean ejec, String mainEntry) {
            this.modsExternos = mods; this.packsRequeridos = packs; this.ejecutable = ejec;
            this.mainEntry = (mainEntry == null) ? "" : mainEntry;
        }
        /** Para los llamantes que aún no lo calculan: se comporta como antes. */
        public static Cierre desconocido() {
            return new Cierre(java.util.Collections.emptyMap(),
                              java.util.Collections.emptySet(), true);
        }
    }

    /** Compatibilidad: sin cierre, el pack sale como salía. */
    public static Path buildPack(BpBuild proj) throws IOException {
        return buildPack(proj, Cierre.desconocido());
    }

    /**
     * Empaqueta el resultado del build en {@code <outDir>/<main>.pack}.
     * @return el path del pack generado.
     * @throws IOException si no hay nada que empaquetar o falla la E/S / el formato.
     */
    public static Path buildPack(BpBuild proj, Cierre cierre) throws IOException {
        Path outDir = Paths.get(proj.outDir);
        List<PackEntry> entries = new ArrayList<>();

        // 1) módulos + nativo del outDir (filtrados a .mod/.mdn: nunca .bpi/.slots)
        collectDir(outDir, entries, OUTDIR_TYPES);
        // Lo que ha aportado el outDir, ANTES de mezclar los resources. El pack se
        // valida contra esto y no contra el total: un proyecto con resources/ y
        // CERO módulos dejaba la lista no-vacía y se colaba (ver abajo).
        List<PackEntry> modulos = new ArrayList<>(entries);

        /* 1b) LAS DEPENDENCIAS EXTERNAS (V5/H8) — la stdlib y demás módulos que
         *     este build NO compiló pero SÍ usó.
         *
         * Decisión de Eduardo: «metemos las librerías estándar aunque no me
         * termina de gustar, pero es mejor ir sobre seguro».
         *
         * Lo que se mete es el `.mod` EXACTO que resolvió el compilador, no uno
         * equivalente. Eso convierte en imposible el riesgo que preocupaba —que
         * el pack lleve un `Str` distinto del que compiló contra él—: son el
         * mismo fichero. Lo que NO resuelve, y hay que decidir aparte, es la
         * precedencia frente al que ya trae el firmware.
         *
         * Si un módulo ya vino del outDir, gana ése: es el que acabamos de
         * construir. */
        int externos = 0;
        for (Map.Entry<String, Path> e : cierre.modsExternos.entrySet()) {
            Path mod = e.getValue();
            String base = mod.getFileName().toString();
            if (!base.endsWith(".mod")) continue;
            String nombre = base.substring(0, base.length() - 4);
            if (yaEsta(entries, "mod", nombre)) continue;      // el del outDir manda
            if (!Files.isRegularFile(mod)) continue;
            entries.add(new PackEntry("mod", nombre, Files.readAllBytes(mod)));
            modulos.add(entries.get(entries.size() - 1));
            externos++;
        }
        if (externos > 0)
            System.out.println("pack: " + externos
                + " modulo(s) de dependencia incluidos (los MISMOS contra los que se compilo)");

        // 2) resources del proyecto (cualquier extensión), si hay carpeta
        Path resDir = Paths.get(proj.projectDir, "resources");
        if (Files.isDirectory(resDir)) collectDir(resDir, entries, null);

        /* ── EL `npk` VA EL PRIMERO (V5/H8, D5) ─────────────────────────────
         *
         * No hace falta para que funcione —desde que el IDE relocaliza AL
         * GRABAR, la posición dentro del pack da igual: conoce el offset de
         * cada entrada porque él mismo lo montó—. Se hace por PREDECIBILIDAD:
         *
         *   · de primero, su offset es `128 + N*48` y sólo se mueve si cambia
         *     el NÚMERO de entradas;
         *   · en medio, se mueve si cambia el TAMAÑO de cualquier cosa que
         *     vaya antes — tocar una coma de un `.mod` lo desplaza.
         *
         * Lo segundo hace ilegible cualquier diagnóstico de direcciones. Es
         * una línea y ahorra tardes. */
        entries.sort((a, b) -> Boolean.compare(!"npk".equals(a.tipo),
                                               !"npk".equals(b.tipo)));

        // El pack tiene que poder EJECUTARSE, y eso son dos condiciones. Las dos se
        // comprueban aquí, al construir, y no en la placa: la VM las detecta y lo
        // dice bien, pero para entonces ya has grabado un pack inservible.
        if (modulos.isEmpty())
            throw new IOException("out:pack — no hay .mod/.mdn en " + outDir + " que empaquetar");

        /* ── QUÉ NOMBRE LLEVA EL ARRANQUE (V5/H11, #365) ────────────────────
         *
         * El manifest dice el nombre de la ENTRADA, no el del fichero fuente, y
         * quien arranca lo busca LITERAL — la VM-C en `bpvm.c` y miVM en
         * `ModuleManager.executeRootPack`, las dos igual. Por eso el nombre lo
         * pone aquí ya resuelto el compilador: así soportar `library` no cuesta
         * ni una línea en las dos VMs, y no hay dos sitios componiendo el mismo
         * nombre con el riesgo de que un día compongan distinto.
         *
         * Sin `library` (el caso de siempre) el canónico ES `proj.main`, así que
         * los packs que ya existen no cambian ni un byte. */
        String mainEntry = cierre.mainEntry.isEmpty() ? proj.main : cierre.mainEntry;

        /* Sólo se exige el módulo del `main` si el pack es EJECUTABLE. Una
         * librería no lleva main (criterio de Eduardo) y no tiene por qué. */
        if (cierre.ejecutable && !contieneModulo(modulos, mainEntry)) {
            throw new IOException("out:pack — el manifest declara main='" + mainEntry
                    + "' pero ese módulo no está en el pack. Módulos encontrados en "
                    + outDir + ": [" + nombresDeModulos(modulos) + "]."
                    + pistaLibrary(modulos, mainEntry));
        }

        /* ── 3) EL MANIFEST ─────────────────────────────────────────────────
         *
         * `main=` es OPCIONAL (V5/H8): sólo va si el módulo raíz tiene de verdad
         * una función `main`. Y eso se SABE —lo dice el analizador—, no se
         * declara: un campo en el `.bpbuild` sería un segundo sitio para la
         * misma verdad, y los dos sitios acaban separándose.
         *
         * `requires-pack=` es la respuesta a «¿cómo sé qué dependencias
         * necesito?». Sale de los `import ... from pack "XXXX"` que el
         * compilador ha visto. NO se comprueba aquí: el PC no sabe qué hay en
         * una placa concreta —cada firmware embebe un conjunto distinto y puede
         * haber otros packs grabados—. Sólo se DECLARA; quien carga, que es el
         * único que sabe lo que tiene, comparará. */
        StringBuilder mf = new StringBuilder();
        if (cierre.ejecutable) mf.append("main=").append(mainEntry).append('\n');
        List<String> faltan = new ArrayList<>();
        for (String p : cierre.packsRequeridos)
            if (!p.equals(proj.packProvides)) faltan.add(p);
        if (!faltan.isEmpty()) mf.append("requires-pack=").append(String.join(",", faltan)).append('\n');
        if (proj.packNotas != null && !proj.packNotas.isEmpty())
            mf.append("notas=").append(proj.packNotas.replace('\n', ' ')).append('\n');
        entries.add(new PackEntry(PackFormat.TYPE_MANIFEST, PackFormat.MANIFEST_NAME,
                mf.toString().getBytes(StandardCharsets.UTF_8)));

        System.out.println("pack: nombre '" + proj.packName + "' ("
                + (proj.packNameDeclarado ? "de pack.name" : "del fichero de proyecto")
                + ")");
        System.out.println("pack: " + (cierre.ejecutable
                ? "EJECUTABLE (main=" + mainEntry
                  + (mainEntry.equals(proj.main) ? "" : ", cualificado por su `library`") + ")"
                : "LIBRERIA (el modulo raiz no tiene `main`, y no le hace falta)"));
        if (!faltan.isEmpty())
            System.out.println("pack: NECESITA ademas estos packs grabados: " + faltan
                + " — no van dentro, se graban aparte");

        // 4) empaquetar (Pack.jar = librería). fecha = ahora (la no-determinación
        //    intencionada del formato). Bloque 8 KB = el bloque de borrado MÁS
        //    GRANDE de las 3 familias (STM32 U5 página 8K; múltiplo del 4K de
        //    Pico/ESP32) → el MISMO pack se puede grabar en cualquiera (regla de
        //    portabilidad de la spec §2.1; el BURN valida la alineación).
        final int PORTABLE_BLOCK = 8192;
        /* El nombre del pack sale de `pack.name` o del fichero de proyecto —
         * NO de `main` (V5/H8). Se DICE, con su procedencia: es un fichero que
         * se distribuye y se graba; si un dia cambia de nombre tiene que verse
         * al construirlo, no al no encontrarlo. */
        /* V5/H8 — el nombre del pack es SUYO: sale de `pack.name` o, si no, del
         * fichero de proyecto. NUNCA de `main`, que es otra cosa (cuál es el
         * módulo de entrada) y cambiaría el nombre del artefacto al cambiar de
         * punto de arranque.
         *
         * Aquí no se inventa uno por defecto: quien construye un BpBuild a mano
         * —sin fichero de proyecto del que sacarlo— tiene que decirlo. Antes
         * llegaba `null` hasta `PackWriter.build` y salía un NullPointer sin
         * pista; lo cazó `PackStepTest` al pasar la suite del frontend, un día
         * después de introducirlo. */
        if (proj.packName == null || proj.packName.trim().isEmpty()) {
            throw new IOException("out:pack — el proyecto no dice cómo se llama el pack."
                    + " Ponle `\"pack\": { \"name\": \"...\" }` en el .bpbuild, o"
                    + " construye el BpBuild con `packName` puesto (no se deduce"
                    + " de `main`: son cosas distintas).");
        }
        Path out = outDir.resolve(proj.packName + ".pack");
        try {
            /* LA VERSIÓN va en la CABECERA, no en el manifest: el formato ya
             * tiene `version_contenido` (≤16 B) y llevaba vacío desde H3. Es un
             * campo estructurado que el dispositivo lee sin parsear texto. El
             * manifest es para lo que ahí no cabe (las notas). */
            byte[] img = PackWriter.build(proj.packName,
                    proj.packVersion == null ? "" : proj.packVersion,
                    System.currentTimeMillis() / 1000L,
                    entries, PORTABLE_BLOCK);
            Files.write(out, img);
        } catch (PackException pe) {
            throw new IOException("out:pack — empaquetando: " + pe.getMessage(), pe);
        }
        return out;
    }

    /** ¿Está el módulo `main` entre las entradas de tipo 'mod'? La comparación es
     *  la MISMA que hace la VM al arrancar un pack (busca la entrada ("mod", main)
     *  tal cual): si aquí dijéramos que sí y allí que no, la comprobación no
     *  serviría de nada. */
    private static boolean contieneModulo(List<PackEntry> mods, String main) {
        if (main == null || main.isEmpty()) return false;
        for (PackEntry e : mods) {
            if ("mod".equals(e.tipo) && main.equals(e.nombre)) return true;
        }
        return false;
    }

    /** Si el módulo del main SÍ está pero bajo su nombre de librería
     *  (`com.example.Demo.mod` cuando el manifest dice `Demo`), decirlo.
     *
     *  <p>Desde #365 esto YA NO ES UNA LIMITACIÓN: un módulo con `library` puede
     *  arrancar un pack, porque el manifest lleva el nombre canónico que compone
     *  el compilador. Así que llegar aquí significa que el nombre canónico no
     *  llegó — se empaquetó por la vía corta, `buildPack(proj)` sin `Cierre`, que
     *  sólo conoce `proj.main` (el nombre del FICHERO FUENTE). Eso es lo que hay
     *  que decir: no le mandamos a cambiar su código por un fallo nuestro. */
    private static String pistaLibrary(List<PackEntry> mods, String main) {
        String sufijo = "." + main;
        for (PackEntry e : mods) {
            if (!"mod".equals(e.tipo) || !e.nombre.endsWith(sufijo)) continue;
            return " El módulo SÍ está, pero con su nombre de librería ('"
                 + e.nombre + "'), porque declara `library`. Eso está soportado"
                 + " (#365) y el manifest debería llevar ese nombre: si sale este"
                 + " error, el pack se construyó sin pasarle el cierre del"
                 + " compilador (`buildPack(proj)` a secas), que es el único que"
                 + " sabe el nombre canónico. Construye el proyecto con"
                 + " `--project`.";
        }
        return "";
    }

    /** ¿Ya hay una entrada con ese tipo y nombre? El outDir tiene prioridad
     *  sobre las dependencias externas: es lo que acabamos de construir. */
    private static boolean yaEsta(List<PackEntry> entries, String tipo, String nombre) {
        for (PackEntry e : entries)
            if (tipo.equals(e.tipo) && nombre.equals(e.nombre)) return true;
        return false;
    }

    /** Nombres de los .mod, para que el error diga qué SÍ hay (el que se equivoca
     *  de `main` suele tener el módulo delante con otro nombre). */
    private static String nombresDeModulos(List<PackEntry> mods) {
        StringBuilder sb = new StringBuilder();
        for (PackEntry e : mods) {
            if (!"mod".equals(e.tipo)) continue;
            if (sb.length() > 0) sb.append(", ");
            sb.append(e.nombre);
        }
        return sb.toString();
    }

    /**
     * Añade los ficheros de {@code dir} como entradas (tipo = extensión en
     * minúsculas, nombre = basename). Si {@code onlyTypes} != null, filtra a esas
     * extensiones. Orden determinista (alfabético). Ficheros sin extensión: skip.
     */
    private static void collectDir(Path dir, List<PackEntry> entries, Set<String> onlyTypes)
            throws IOException {
        if (!Files.isDirectory(dir)) return;
        List<Path> files;
        try (Stream<Path> s = Files.list(dir)) {
            files = s.filter(Files::isRegularFile).sorted().collect(Collectors.toList());
        }
        for (Path f : files) {
            String base = f.getFileName().toString();
            int dot = base.lastIndexOf('.');
            if (dot < 0) continue;                       // sin extensión → no sé el tipo
            String tipo = base.substring(dot + 1).toLowerCase();
            String nombre = base.substring(0, dot);

            /* ── DOBLE EXTENSIÓN (V5/H8, D1) ────────────────────────────────
             *
             * `sqlite.npk.RISCV` → tipo `npk`, nombre `sqlite.RISCV`.
             *
             * NO se puede derivar de la última extensión, que es lo que hace el
             * caso normal: daría tipo `riscv`, y el tipo de una entrada es un
             * fourcc de 4 caracteres en minúsculas (`PackFormat.TYPE_LEN`), o
             * sea que ni cabe ni vale. El destino tiene que irse al NOMBRE.
             *
             * Y el nombre es donde debe estar: el dispositivo busca por tipo y
             * nombre (`bpvm_pack_find(zona, len, "mdn", modulo, …)`), así que
             * al grabar el IDE se queda con el del destino y lo renombra
             * quitándole el sufijo — y el micro encuentra exactamente lo de
             * siempre, sin cambiar una línea de C. */
            /* ⚠️ …PERO SÓLO SI LA ÚLTIMA EXTENSIÓN NO ES YA UN TIPO (#365).
             *
             * `com.example.Npk.mod` — un módulo llamado `Npk` dentro de una
             * librería — tiene tipo `mod`, que es un tipo de pleno derecho: no
             * hay nada que interpretar. Sin esta condición, la regla de abajo
             * miraba el penúltimo componente, veía `npk`, y renombraba la
             * entrada a `com.example.mod` con tipo `npk`. Muda, y en un pack
             * grabado. Con `Mod` en vez de `Npk` daba el otro extremo: un error
             * diciendo que un `.mod` no lleva destino, que era falso.
             *
             * La doble extensión existe para `sqlite.npk.RISCV`, donde la última
             * (`riscv`) NO es un tipo — y ahí sí hay que ir a buscar el tipo al
             * penúltimo. Con esta guarda, ese caso entra igual y el de arriba se
             * queda fuera, que es justo lo que se quiere. */
            int dot2 = nombre.lastIndexOf('.');
            if (dot2 > 0 && !OUTDIR_TYPES.contains(tipo)) {
                String tipoReal = nombre.substring(dot2 + 1).toLowerCase();
                if (TIPOS_POR_DESTINO.contains(tipoReal)) {
                    String sufijo = base.substring(dot + 1);      /* SIN pasar a minúsculas */
                    nombre = nombre.substring(0, dot2) + "." + sufijo;
                    tipo   = tipoReal;
                } else if (OUTDIR_TYPES.contains(tipoReal)) {
                    /* `Algo.mod.RISCV` — un tipo que SÍ va al pack pero que NO
                     * admite destino. El `.mod` es bytecode portable: no hay
                     * uno por familia, y ponerle sufijo es un error de quien lo
                     * construyó.
                     *
                     * Sin este aviso el fichero se caía por el filtro de abajo
                     * (tipo `riscv` no está en OUTDIR_TYPES) y DESAPARECÍA en
                     * silencio: un pack sin ese módulo, y a buscar por qué. */
                    throw new IOException(base + ": un '." + tipoReal
                        + "' NO lleva destino — es portable, el mismo para todas"
                        + " las placas. Quita el sufijo '." + base.substring(dot + 1)
                        + "'. (Lo llevan sólo: " + TIPOS_POR_DESTINO + ")");
                }
            }

            if (onlyTypes != null && !onlyTypes.contains(tipo)) continue;
            entries.add(new PackEntry(tipo, nombre, Files.readAllBytes(f)));
        }
    }
}
