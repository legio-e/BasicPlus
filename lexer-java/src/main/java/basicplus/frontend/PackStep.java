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

        // 2) resources del proyecto (cualquier extensión), si hay carpeta
        Path resDir = Paths.get(proj.projectDir, "resources");
        if (Files.isDirectory(resDir)) collectDir(resDir, entries, null);

        if (entries.isEmpty())
            throw new IOException("out:pack — no hay .mod/.mdn en " + outDir + " que empaquetar");

        // 3) manifest → pack ejecutable (modelo jar). Reusa el `main` del .bpbuild.
        String manifest = "main=" + proj.main + "\n";
        entries.add(new PackEntry(PackFormat.TYPE_MANIFEST, PackFormat.MANIFEST_NAME,
                manifest.getBytes(StandardCharsets.UTF_8)));

        // 4) empaquetar (Pack.jar = librería). fecha = ahora (la no-determinación
        //    intencionada del formato). bloque por defecto 4 KB.
        Path out = outDir.resolve(proj.main + ".pack");
        try {
            byte[] img = PackWriter.build(proj.main, "", System.currentTimeMillis() / 1000L,
                    entries, PackFormat.DEFAULT_BLOCK);
            Files.write(out, img);
        } catch (PackException pe) {
            throw new IOException("out:pack — empaquetando: " + pe.getMessage(), pe);
        }
        return out;
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
