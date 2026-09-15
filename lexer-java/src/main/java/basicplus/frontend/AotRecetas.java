package basicplus.frontend;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.CRC32;

/**
 * V7/H2 (#441) — LAS RECETAS del `.mdn`, leídas de su única fuente:
 * {@code bpgenvm-c/include/bpvm_aot_recetas.h}.
 *
 * <p>Una receta es el literal de flags con que se compila el código nativo de una
 * ISA/ABI (`ARMV8M_MAIN_SOFTFP`, `RV32IMAFC_ILP32F`…). El IDE compila con ella y
 * estampa en la cabecera del `.mdn` (v2, `flags_crc`) el CRC32 de sus BYTES; el
 * firmware hashea el mismo literal del mismo `.h` y rechaza lo que no coincide.
 * Por eso aquí no hay copia de los flags: se parsea el `.h` en el momento, y si
 * alguien cambia un flag allí, cambia a la vez lo que el IDE aplica y lo que la
 * placa exige. Una copia en Java es exactamente el desfase que #441 persigue.
 *
 * <p>Lo que se lee: líneas {@code #define BPVM_AOT_RECETA_<NOMBRE> "<flags>"}.
 * El literal no admite escapes ({@code \}): el CRC es el de los caracteres tal
 * cual, y un escape haría que Java y C vieran bytes distintos. Se rechaza.
 */
public final class AotRecetas {

    /** El fichero, relativo a la raíz de bpgenvm-c: {@code include/bpvm_aot_recetas.h}. */
    public static final String FICHERO = "bpvm_aot_recetas.h";

    private static final Pattern DEF = Pattern.compile(
            "^[ \\t]*#define[ \\t]+BPVM_AOT_RECETA_([A-Z0-9_]+)[ \\t]+\"((?:[^\"\\\\]|\\\\.)*)\"",
            Pattern.MULTILINE);

    private final Map<String, String> recetas;   /* nombre → literal, en orden del .h */
    private final Path origen;

    private AotRecetas(Map<String, String> r, Path origen) {
        this.recetas = Collections.unmodifiableMap(r);
        this.origen = origen;
    }

    /** Lee las recetas de {@code <bpgenvm-c>/include/bpvm_aot_recetas.h}. */
    public static AotRecetas deBpgenvm(Path raizBpgenvm) throws IOException {
        return leer(raizBpgenvm.resolve("include").resolve(FICHERO));
    }

    /** Parsea el `.h`. Falla con un mensaje claro si no está, si no trae ninguna
     *  receta (un checkout de antes de V7/H2) o si un literal lleva escapes. */
    public static AotRecetas leer(Path h) throws IOException {
        if (!Files.isRegularFile(h)) {
            throw new IOException("no encuentro las recetas del .mdn: " + h
                    + " (bpgenvm-c de antes de V7/H2, #441)");
        }
        String texto = new String(Files.readAllBytes(h), StandardCharsets.UTF_8);
        Map<String, String> r = new LinkedHashMap<>();
        Matcher m = DEF.matcher(texto);
        while (m.find()) {
            String nombre = m.group(1), literal = m.group(2);
            if (literal.indexOf('\\') >= 0) {
                throw new IOException(h.getFileName() + ": la receta " + nombre
                        + " lleva un escape (\\): el CRC no sería el que calcula el firmware");
            }
            if (literal.trim().isEmpty()) {
                throw new IOException(h.getFileName() + ": la receta " + nombre + " está vacía");
            }
            r.put(nombre, literal);
        }
        if (r.isEmpty()) {
            throw new IOException(h + ": no trae ninguna receta BPVM_AOT_RECETA_*");
        }
        return new AotRecetas(r, h);
    }

    /** El literal de la receta, o null si el `.h` no la tiene. */
    public String literal(String nombre) { return recetas.get(nombre); }

    /** El literal, o una excepción que dice qué recetas hay. */
    public String exigir(String nombre) throws IOException {
        String l = recetas.get(nombre);
        if (l == null) {
            throw new IOException(origen.getFileName() + ": no tiene la receta " + nombre
                    + " (tiene: " + String.join(", ", recetas.keySet()) + ")");
        }
        return l;
    }

    /** Los flags de una receta, uno a uno, para pasárselos a gcc. */
    public static String[] flags(String literal) { return literal.trim().split("[ \\t]+"); }

    /** El nombre de la receta con esa huella, o null. Para decir «la placa exige
     *  RV32IMAFC_ILP32F» en vez de un número. */
    public String nombreDe(int crc) {
        for (Map.Entry<String, String> e : recetas.entrySet())
            if (crc(e.getValue()) == crc) return e.getKey();
        return null;
    }

    public Set<String> nombres() { return recetas.keySet(); }

    public Path origen() { return origen; }

    /** LA HUELLA: CRC32 (IEEE, el de `java.util.zip` y el de `bpvm_crc32`) de los
     *  bytes del literal. Como `int`, que es como viaja en la cabecera (u32 LE). */
    public static int crc(String literal) {
        CRC32 c = new CRC32();
        c.update(literal.getBytes(StandardCharsets.US_ASCII));
        return (int) c.getValue();
    }
}
