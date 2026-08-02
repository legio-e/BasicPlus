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
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public final class PackStep {

    private PackStep() {}

    /** Extensiones del outDir que van al pack (ejecutables del build). */
    private static final Set<String> OUTDIR_TYPES =
            new HashSet<>(Arrays.asList("mod", "mdn"));

    /**
     * Empaqueta el resultado del build en {@code <outDir>/<main>.pack}.
     * @return el path del pack generado.
     * @throws IOException si no hay nada que empaquetar o falla la E/S / el formato.
     */
    public static Path buildPack(BpBuild proj) throws IOException {
        Path outDir = Paths.get(proj.outDir);
        List<PackEntry> entries = new ArrayList<>();

        // 1) módulos + nativo del outDir (filtrados a .mod/.mdn: nunca .bpi/.slots)
        collectDir(outDir, entries, OUTDIR_TYPES);
        // Lo que ha aportado el outDir, ANTES de mezclar los resources. El pack se
        // valida contra esto y no contra el total: un proyecto con resources/ y
        // CERO módulos dejaba la lista no-vacía y se colaba (ver abajo).
        List<PackEntry> modulos = new ArrayList<>(entries);

        // 2) resources del proyecto (cualquier extensión), si hay carpeta
        Path resDir = Paths.get(proj.projectDir, "resources");
        if (Files.isDirectory(resDir)) collectDir(resDir, entries, null);

        // El pack tiene que poder EJECUTARSE, y eso son dos condiciones. Las dos se
        // comprueban aquí, al construir, y no en la placa: la VM las detecta y lo
        // dice bien, pero para entonces ya has grabado un pack inservible.
        if (modulos.isEmpty())
            throw new IOException("out:pack — no hay .mod/.mdn en " + outDir + " que empaquetar");

        if (!contieneModulo(modulos, proj.main)) {
            throw new IOException("out:pack — el manifest declara main='" + proj.main
                    + "' pero ese módulo no está en el pack. Módulos encontrados en "
                    + outDir + ": [" + nombresDeModulos(modulos) + "]."
                    + pistaLibrary(modulos, proj.main));
        }

        // 3) manifest → pack ejecutable (modelo jar). Reusa el `main` del .bpbuild.
        String manifest = "main=" + proj.main + "\n";
        entries.add(new PackEntry(PackFormat.TYPE_MANIFEST, PackFormat.MANIFEST_NAME,
                manifest.getBytes(StandardCharsets.UTF_8)));

        // 4) empaquetar (Pack.jar = librería). fecha = ahora (la no-determinación
        //    intencionada del formato). Bloque 8 KB = el bloque de borrado MÁS
        //    GRANDE de las 3 familias (STM32 U5 página 8K; múltiplo del 4K de
        //    Pico/ESP32) → el MISMO pack se puede grabar en cualquiera (regla de
        //    portabilidad de la spec §2.1; el BURN valida la alineación).
        final int PORTABLE_BLOCK = 8192;
        Path out = outDir.resolve(proj.main + ".pack");
        try {
            byte[] img = PackWriter.build(proj.main, "", System.currentTimeMillis() / 1000L,
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
     *  (`com.example.Demo.mod` cuando el main es `Demo`), decirlo — porque el
     *  usuario NO tiene salida buscándola solo: poner el nombre cualificado en
     *  `main` tampoco vale, ahí se busca el FICHERO FUENTE, que se llama
     *  `Demo.bp`. No adivinamos por él (no arreglamos el manifest a su espalda):
     *  le decimos qué pasa y qué puede hacer. */
    private static String pistaLibrary(List<PackEntry> mods, String main) {
        String sufijo = "." + main;
        for (PackEntry e : mods) {
            if (!"mod".equals(e.tipo) || !e.nombre.endsWith(sufijo)) continue;
            return " El módulo SÍ está, pero con su nombre de librería ('"
                 + e.nombre + "'), porque declara `library`. Hoy un módulo con"
                 + " `library` no puede ser el main de un pack ejecutable: quítale"
                 + " el `library` al módulo principal, o publica el pack como"
                 + " biblioteca (sin main).";
        }
        return "";
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
            if (onlyTypes != null && !onlyTypes.contains(tipo)) continue;
            String nombre = base.substring(0, dot);
            entries.add(new PackEntry(tipo, nombre, Files.readAllBytes(f)));
        }
    }
}
