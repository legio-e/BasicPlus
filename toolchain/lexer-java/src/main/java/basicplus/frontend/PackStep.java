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
        /** [V7/H2, #493] Dónde buscar los módulos que faltan para cerrar el pack
         *  (la stdlib, los `dependencies` del proyecto): los mismos sitios en los
         *  que busca el compilador. Vacío = no se completa el cierre. */
        public final List<Path> dondeBuscar;
        /** [V7 #623] La carpeta de la STDLIB (la de BpVM.cfg), o null: lo que hay en ella no entra en un pack
         *  LIBRERÍA. Null = no se sabe, y entonces como antes. */
        public final Path stdlibDir;

        public Cierre(Map<String, Path> mods, Set<String> packs, boolean ejec) {
            this(mods, packs, ejec, "", java.util.Collections.emptyList());
        }
        public Cierre(Map<String, Path> mods, Set<String> packs, boolean ejec, String mainEntry) {
            this(mods, packs, ejec, mainEntry, java.util.Collections.emptyList());
        }
        public Cierre(Map<String, Path> mods, Set<String> packs, boolean ejec, String mainEntry,
                      List<Path> dondeBuscar) {
            this(mods, packs, ejec, mainEntry, dondeBuscar, null);
        }
        public Cierre(Map<String, Path> mods, Set<String> packs, boolean ejec, String mainEntry,
                      List<Path> dondeBuscar, Path stdlibDir) {
            this.modsExternos = mods; this.packsRequeridos = packs; this.ejecutable = ejec;
            this.mainEntry = (mainEntry == null) ? "" : mainEntry;
            this.dondeBuscar = (dondeBuscar == null) ? java.util.Collections.<Path>emptyList() : dondeBuscar;
            this.stdlibDir = stdlibDir;
        }
        /** Para los llamantes que aún no lo calculan: se comporta como antes. */
        public static Cierre desconocido() {
            return new Cierre(java.util.Collections.emptyMap(),
                              java.util.Collections.emptySet(), true);
        }
    }

    /** [V7 #623] Los módulos de la STDLIB que los de un pack importan y el pack NO lleva: los que tiene que poner
     *  la placa, del Stdlib.pack (XIP) o de /lib (a la RAM). Es para quien graba (el panel de Packs del IDE;
     *  `wire_serie.py pack` hace la misma cuenta): desde #623 un pack LIBRERÍA no lleva la stdlib, y en una placa
     *  sin el Stdlib.pack eso es RAM —en la C6 no cabe la GUI—. «De la stdlib» = lo hay en `stdlibDir`; con
     *  `stdlibDir` null (no se sabe) cuenta todo lo que importa y no lleva. Ordenados. Vacío para el Stdlib.pack
     *  y para un pack PROGRAMA, que lo llevan todo. */
    public static List<String> stdlibQueImporta(List<PackEntry> entries, Path stdlibDir) {
        List<String> r = new ArrayList<>();
        for (String d : duenosQueFaltan(entries))
            if (stdlibDir == null || esDeLaStdlib(stdlibDir, d)) r.add(d);
        java.util.Collections.sort(r);
        return r;
    }

    /** [V7 #623] ¿`nombre` es un módulo de la stdlib? Lo es si la carpeta de la stdlib lo tiene. */
    static boolean esDeLaStdlib(Path stdlibDir, String nombre) {
        return stdlibDir != null && Files.isRegularFile(stdlibDir.resolve(nombre + ".mod"));
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

        /* 1a) [V7 #623, 8-oct] UN PACK LIBRERÍA NO LLEVA LA STDLIB.
         *
         * Eduardo: el pack PROGRAMA es «una especie de contenedor que nos garantice lo más posible que el programa y
         * los módulos son autocontenidos», y lleva lo que su programa usa, la stdlib incluida (abajo, 1b y 1c). Un
         * pack LIBRERÍA (su raíz no tiene `main`) no: sus copias de la stdlib se quedaban rancias al cambiar ésta y,
         * como entre packs gana el último grabado, tapaban las del Stdlib.pack para TODOS los programas —en la C6, a
         * la RAM y sin memoria (#623)—. Así que aquí no entra ningún módulo de la stdlib —ni del outDir (la receta de
         * la GUI los copiaba a mano), ni de las dependencias, ni del cierre—: la placa los saca del Stdlib.pack (XIP)
         * o de /lib. Y se dice cuáles. La excepción, declarada: el proyecto que ES la stdlib (`"stdlib": true`). */
        boolean sinStdlib = !cierre.ejecutable && cierre.stdlibDir != null && !proj.esLaStdlib;
        java.util.TreeSet<String> fueraStdlib = new java.util.TreeSet<>();
        if (sinStdlib) {
            for (java.util.Iterator<PackEntry> it = entries.iterator(); it.hasNext(); ) {
                PackEntry e = it.next();
                if (("mod".equals(e.tipo) || "mdn".equals(e.tipo)) && esDeLaStdlib(cierre.stdlibDir, e.nombre)) {
                    fueraStdlib.add(e.nombre);
                    it.remove();
                }
            }
        }
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
            if (sinStdlib && esDeLaStdlib(cierre.stdlibDir, nombre)) { fueraStdlib.add(nombre); continue; }   // #623
            if (yaEsta(entries, "mod", nombre)) continue;      // el del outDir manda
            if (!Files.isRegularFile(mod)) continue;
            entries.add(new PackEntry("mod", nombre, Files.readAllBytes(mod)));
            modulos.add(entries.get(entries.size() - 1));
            externos++;
        }
        if (externos > 0)
            System.out.println("pack: " + externos
                + " modulo(s) de dependencia incluidos (los MISMOS contra los que se compilo)");

        /* 1c) [V7/H2, #493] EL CIERRE TRANSITIVO — un pack es como un docker.
         *
         * Decisión de Eduardo (15-sep): «Un pack debería ser como un docker,
         * autocontenido y así asegurar que no haya nada externo que le afecte
         * (salvo SQLite u otros similares)». Hasta hoy entraban sólo los imports
         * DIRECTOS que resolvió el compilador: `AppPack` llevaba `Str` y `Json` y
         * no `Core` (que Str importa), y al ejecutarlo: «falta el modulo 'Core'».
         * Ni autocontenido ni lo contrario.
         *
         * Aquí se recorren los imports de cada `.mod` que ya va en el pack —los
         * dueños de sus símbolos importados— y lo que falte se trae de donde el
         * compilador lo habría buscado (la stdlib, `dependencies`, el outDir),
         * hasta que no falte nada. Y como en el RUN el pack en ejecución va
         * PRIMERO en la resolución, lo que lleva dentro es lo que se ejecuta. Lo
         * que no se encuentre se dice: es una dependencia que el pack necesitará
         * de fuera (un pack nativo grabado aparte, o el /lib de la placa). */
        int cerrados = 0;
        for (;;) {
            Set<String> faltan = duenosQueFaltan(entries);
            if (faltan.isEmpty()) break;
            int antes = entries.size();
            for (String canon : faltan) {
                if (sinStdlib && esDeLaStdlib(cierre.stdlibDir, canon)) { fueraStdlib.add(canon); continue; }   // #623
                Path mod = localizarModulo(canon + ".mod", cierre.dondeBuscar, outDir);
                if (mod == null) {
                    System.out.println("pack: aviso — '" + canon + "' lo importa un modulo del pack y no se "
                        + "encuentra en la stdlib ni en las dependencias: el pack lo necesitara de fuera");
                    continue;
                }
                entries.add(new PackEntry("mod", canon, Files.readAllBytes(mod)));
                modulos.add(entries.get(entries.size() - 1));
                cerrados++;
            }
            if (entries.size() == antes) break;              /* nada nuevo: no insistir */
        }
        if (cerrados > 0)
            System.out.println("pack: " + cerrados + " modulo(s) mas para cerrar las dependencias "
                + "(un pack es autocontenido: lo que importa va dentro)");
        if (!fueraStdlib.isEmpty())
            System.out.println("pack: LIBRERIA sin la stdlib (#623): " + String.join(", ", fueraStdlib)
                + (fueraStdlib.size() == 1 ? " no va dentro; la placa lo saca" : " no van dentro; la placa los saca")
                + " del Stdlib.pack (o de /lib)");

        /* 1d) [V7/H6, #563] UN MÓDULO DE LA STDLIB SIN SELLAR NO ENTRA EN UN PACK.
         *
         * El pack de la GUI llevaba un `Core.mod` de GENERACIÓN 0: su proyecto tiene por
         * sourceDir `bpstdlib/`, así que el build compilaba `Core.bp` como fuente suya —sin
         * el sello que la stdlib saca de BpVM.cfg— y el paso 1 lo metía en el pack. La receta
         * copiaba los canónicos de Gui, Json, Collections y Str y se olvidó de Core. En la
         * placa no mordía porque el `/lib/Core.mod` de la imagen lo tapaba; pero borrado ése,
         * todo lo compilado contra la stdlib dejaba de enlazar («'Core' (gen 0) es más
         * antiguo…»), y con #468 una copia distinta de la de la imagen ya no se puede quitar
         * de /lib. La regla es exacta: un `.mod` con versión y gen 0 cuyo nombre la stdlib
         * (los mismos sitios que el cierre) tiene SELLADO no entra; se dice con los dos datos
         * y qué hacer. Un módulo que la stdlib no tiene no se mira. */
        for (PackEntry e : modulos) {
            if (!"mod".equals(e.tipo) || genDe(e.data) != 0 || !conVersion(e.data)) continue;
            Path canon = localizarModulo(e.nombre + ".mod", cierre.dondeBuscar, null);
            if (canon == null) continue;
            int gCanon = genDe(Files.readAllBytes(canon));
            if (gCanon > 0) {
                throw new IOException("pack: '" + e.nombre + ".mod' va SIN SELLAR (generación 0: se compiló "
                    + "como fuente de este proyecto) y la stdlib lo tiene sellado (generación " + gCanon
                    + ", " + canon + "). En la placa, lo compilado contra la stdlib no enlazaría con él. "
                    + "Copia el de la stdlib al outDir antes de construir el pack (como hace la receta de "
                    + "bpstdlib/gui/Gui.bpbuild con Gui, Json, Collections, Str y Core) — #563.");
            }
        }

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
    /* ── [V7/H2, #493] el cierre: los dueños de los imports de cada .mod ─────── */

    /** Los módulos (nombre canónico `[lib.]Mod`) que algún `.mod` del pack importa y
     *  que el pack no lleva. Lee la sección `imports` de cada uno (MOD6/7/8). */
    static Set<String> duenosQueFaltan(List<PackEntry> entries) {
        Set<String> hay = new java.util.HashSet<>();
        for (PackEntry e : entries) if ("mod".equals(e.tipo)) hay.add(e.nombre);
        Set<String> faltan = new java.util.LinkedHashSet<>();
        for (PackEntry e : entries) {
            if (!"mod".equals(e.tipo)) continue;
            for (String d : duenosDeImports(e.data)) if (!hay.contains(d)) faltan.add(d);
        }
        return faltan;
    }

    /** Los dueños de los símbolos importados por un `.mod`: `Str.toHex` → `Str`,
     *  `acme.LibHdr.__init` → `acme.LibHdr`. Un `.mod` que no se pueda leer no
     *  aporta nada (el pack se valida en otro sitio). */
    static java.util.List<String> duenosDeImports(byte[] mod) {
        java.util.List<String> out = new ArrayList<>();
        try {
            java.io.DataInputStream in = new java.io.DataInputStream(new java.io.ByteArrayInputStream(mod));
            int magic = in.readInt();
            /* [V7/H6, #564] Y MOD9. Este lector se quedó en MOD8 (tenía los magics escritos a
             * mano) y en cuanto MOD9 fue el formato por defecto el cierre dejó de ver UN SOLO
             * import: un pack no traía nada de lo que sus módulos importan de segunda mano. No
             * se notaba porque las recetas copian módulos a mano y el /lib de la placa pone el
             * resto — el mismo agujero que #555 en conBloqueNativo. Ahora pregunta a ModFormat,
             * que es quien sabe qué formatos llevan la cabecera con versión (la de MOD8 y MOD9
             * es la misma, 44 B): el día de MOD10 se entera sin tocar esto. */
            boolean v8 = edu.bpgenvm.bytecode.ModFormat.tieneVersion(magic);
            boolean v7 = magic == edu.bpgenvm.bytecode.ModFormat.MAGIC_NUMBER_V7 || v8;
            boolean v6 = magic != edu.bpgenvm.bytecode.ModFormat.MAGIC_NUMBER;   /* todos menos MOD5 */
            if (magic != edu.bpgenvm.bytecode.ModFormat.MAGIC_NUMBER_V6 && !v7) return out;   /* MOD6 en adelante */
            in.readInt(); in.readInt();                      /* dataSize, mainOffset */
            int importSize = in.readInt();
            in.readInt(); in.readInt();                      /* exportSize, codeSize */
            int librarySize = in.readInt();
            if (v6) in.readInt();                            /* interfaceSize */
            if (v7) in.readInt();                            /* nativeSize */
            if (v8) { in.readInt(); in.readInt(); }          /* gen, ifaceCrc */
            if (librarySize < 0 || importSize < 0 || (long) librarySize + importSize > mod.length) return out;
            in.skipBytes(librarySize);
            byte[] imports = new byte[importSize];
            in.readFully(imports);
            java.io.DataInputStream ii = new java.io.DataInputStream(new java.io.ByteArrayInputStream(imports));
            int n = ii.readInt();
            for (int i = 0; i < n; i++) {
                String name = ii.readUTF(); ii.readUTF();    /* fromPath: no cuenta (#493) */
                int p = name.lastIndexOf('.');
                if (p <= 0) continue;
                String dueno = name.substring(0, p);
                if (!out.contains(dueno)) out.add(dueno);
            }
        } catch (IOException | RuntimeException ignore) { /* no legible o corrupto: nada */ }
        return out;
    }

    /** [V7/H6, #563] ¿Lleva el `.mod` la cabecera con versión (MOD8 en adelante)? */
    static boolean conVersion(byte[] mod) {
        if (mod == null || mod.length < 4) return false;
        int magic = ((mod[0] & 0xFF) << 24) | ((mod[1] & 0xFF) << 16) | ((mod[2] & 0xFF) << 8) | (mod[3] & 0xFF);
        return edu.bpgenvm.bytecode.ModFormat.tieneVersion(magic);
    }

    /** [V7/H6, #563] La GENERACIÓN del `.mod` (be32 en el offset 36 de la cabecera con
     *  versión); 0 si no la lleva o no cabe — que es también lo que escribe un build que
     *  compila un módulo de la stdlib como fuente propia. */
    static int genDe(byte[] mod) {
        if (!conVersion(mod) || mod.length < 40) return 0;
        return ((mod[36] & 0xFF) << 24) | ((mod[37] & 0xFF) << 16) | ((mod[38] & 0xFF) << 8) | (mod[39] & 0xFF);
    }

    /** Dónde busca el compilador: los `dondeBuscar` del cierre (stdlibDir,
     *  dependencies), y el outDir. Un `.mod` sin sección de interfaz (v5) no vale. */
    private static Path localizarModulo(String fichero, List<Path> dondeBuscar, Path outDir) {
        java.util.List<Path> sitios = new ArrayList<>(dondeBuscar);
        sitios.add(outDir);
        for (Path d : sitios) {
            if (d == null) continue;
            Path cand = Files.isDirectory(d) ? d.resolve(fichero)
                      : (d.getFileName() != null && d.getFileName().toString().equals(fichero) ? d : null);
            if (cand != null && Files.isRegularFile(cand)) return cand;
        }
        return null;
    }

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
