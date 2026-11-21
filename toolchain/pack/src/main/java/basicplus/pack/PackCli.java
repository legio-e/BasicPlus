package basicplus.pack;

import java.io.IOException;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.InvalidPathException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * CLI de Pack.jar — el "main fino" sobre el núcleo ({@link PackWriter}/{@link
 * PackReader}).
 *
 * <p><b>Pack.jar NO conoce proyectos.</b> Recibe una <b>lista plana de
 * ficheros</b> a empaquetar (como argumentos o vía {@code --files lista.txt});
 * el compilador es quien lee el descriptor del proyecto ({@code .bpbuild}),
 * construye, escribe el manifest como un fichero más, y le pasa aquí la lista
 * pelada. tipo = la extensión del fichero; nombre = el basename sin extensión.
 *
 * <pre>
 *   pack build   --out &lt;f.pack&gt; [--name &lt;n&gt;] [--version &lt;s&gt;] [--block &lt;n&gt;]
 *                [--date &lt;unix&gt;] [--files &lt;lst&gt;] [&lt;fichero&gt;...]
 *   pack list    &lt;f.pack&gt;
 *   pack extract &lt;f.pack&gt; &lt;dir-destino&gt;
 * </pre>
 */
public final class PackCli {

    private PackCli() {}

    public static void main(String[] args) {
        int code = run(args, System.out, System.err);
        if (code != 0) System.exit(code);
    }

    /** Punto de entrada testeable: devuelve el código de salida, sin System.exit. */
    public static int run(String[] args, PrintStream out, PrintStream err) {
        if (args.length == 0) { usage(err); return 2; }
        String[] rest = Arrays.copyOfRange(args, 1, args.length);
        try {
            switch (args[0]) {
                case "build":   return cmdBuild(rest, out, err);
                case "list":    return cmdList(rest, out);
                case "extract": return cmdExtract(rest, out);
                default:
                    err.println("pack: subcomando desconocido: " + args[0]);
                    usage(err);
                    return 2;
            }
        } catch (PackException e) {
            err.println("pack: " + e.getMessage());
            return 1;
        } catch (NumberFormatException e) {
            err.println("pack: número inválido: " + e.getMessage());
            return 2;
        } catch (InvalidPathException e) {
            err.println("pack: ruta inválida: " + e.getMessage());
            return 1;
        } catch (IOException e) {
            err.println("pack: E/S: " + e.getMessage());
            return 1;
        }
    }

    // ── build ──
    private static int cmdBuild(String[] args, PrintStream out, PrintStream err)
            throws PackException, IOException {
        String outPath = null, name = null, version = "", listFile = null;
        Long date = null;
        int block = PackFormat.DEFAULT_BLOCK;
        List<String> files = new ArrayList<>();

        for (int i = 0; i < args.length; i++) {
            String a = args[i];
            switch (a) {
                case "--out":     outPath  = need(args, ++i, a); break;
                case "--name":    name     = need(args, ++i, a); break;
                case "--version": version  = need(args, ++i, a); break;
                case "--files":   listFile = need(args, ++i, a); break;
                case "--block":   block    = Integer.parseInt(need(args, ++i, a)); break;
                case "--date":    date     = Long.parseLong(need(args, ++i, a)); break;
                default:
                    if (a.startsWith("--")) throw new PackException("opción desconocida: " + a);
                    files.add(a);
            }
        }
        if (outPath == null) throw new PackException("falta --out <fichero.pack>");
        if (listFile != null) {
            for (String line : Files.readAllLines(Paths.get(listFile), StandardCharsets.UTF_8)) {
                String t = line.trim();
                if (!t.isEmpty() && !t.startsWith("#")) files.add(t);
            }
        }
        if (files.isEmpty()) throw new PackException("no hay ficheros que empaquetar");
        if (name == null) name = stripExt(Paths.get(outPath).getFileName().toString());
        long fecha = (date != null) ? date : (System.currentTimeMillis() / 1000L);

        List<PackEntry> entries = new ArrayList<>();
        for (String f : files) {
            Path p = Paths.get(f);
            byte[] data = Files.readAllBytes(p);
            String base = p.getFileName().toString();
            String tipo = ext(base);
            if (tipo.isEmpty())
                throw new PackException("fichero sin extensión, no sé el tipo: " + f);
            entries.add(new PackEntry(tipo.toLowerCase(), stripExt(base), data));
        }

        byte[] img = PackWriter.build(name, version, fecha, entries, block);
        Files.write(Paths.get(outPath), img);
        out.println("pack '" + name + "': " + entries.size() + " ficheros, " + img.length + " B → " + outPath);
        return 0;
    }

    // ── list ──
    private static int cmdList(String[] args, PrintStream out) throws PackException, IOException {
        if (args.length < 1) throw new PackException("uso: list <fichero.pack>");
        byte[] img = Files.readAllBytes(Paths.get(args[0]));
        PackReader.Pack p = PackReader.read(img);
        out.printf("pack     : %s%n", p.nombre);
        out.printf("estado   : %s%n", p.active ? "activo" : "tombstoned");
        out.printf("version  : formato %d, contenido '%s'%n", p.versionFormato, p.versionContenido);
        out.printf("fecha    : %d (unix)%n", p.fechaUnix);
        out.printf("tamaño   : %d B%n", p.sizeTotal);
        out.printf("ficheros : %d%n", p.entries.size());
        for (PackEntry e : p.entries)
            out.printf("  %-4s  %-24s  %9d B%n", e.tipo, e.nombre, e.data.length);
        return 0;
    }

    // ── extract ──
    private static int cmdExtract(String[] args, PrintStream out) throws PackException, IOException {
        if (args.length < 2) throw new PackException("uso: extract <fichero.pack> <dir-destino>");
        byte[] img = Files.readAllBytes(Paths.get(args[0]));
        Path dir = Paths.get(args[1]);
        Files.createDirectories(dir);
        PackReader.Pack p = PackReader.read(img);
        for (PackEntry e : p.entries)
            Files.write(dir.resolve(e.nombre + "." + e.tipo), e.data);
        out.println("extraídos " + p.entries.size() + " ficheros de '" + p.nombre + "' → " + dir);
        return 0;
    }

    // ── helpers ──
    private static String need(String[] a, int i, String opt) throws PackException {
        if (i >= a.length) throw new PackException("falta valor para " + opt);
        return a[i];
    }

    private static String ext(String name) {
        int d = name.lastIndexOf('.');
        return (d < 0) ? "" : name.substring(d + 1);
    }

    private static String stripExt(String name) {
        int d = name.lastIndexOf('.');
        return (d < 0) ? name : name.substring(0, d);
    }

    private static void usage(PrintStream o) {
        o.println("Uso:");
        o.println("  pack build   --out <f.pack> [--name <n>] [--version <s>] [--block <n>] [--date <unix>] [--files <lst>] [<fichero>...]");
        o.println("  pack list    <f.pack>");
        o.println("  pack extract <f.pack> <dir-destino>");
    }
}
