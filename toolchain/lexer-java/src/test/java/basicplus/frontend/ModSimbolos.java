package basicplus.frontend;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Lee las CADENAS de un .mod — que es donde viven los nombres de símbolo con los
 * que un módulo pide y ofrece cosas.
 *
 * <p>Sirve para comprobar contratos ENTRE dos .mod (¿lo que uno pide es lo que el
 * otro exporta? ¿qué sobrecarga eligió el llamante?), que es una pregunta que no
 * se puede contestar mirando un fichero solo.
 *
 * <h3>Por qué lee la longitud y no busca texto</h3>
 *
 * Las cadenas van con su longitud delante (2 bytes big-endian), así que se sacan
 * EXACTAS. Buscando texto a pelo, un símbolo acabado en {@code #10} seguido de
 * una cadena que empiece por dígito se leería como {@code #103}: un error que
 * cae del lado peor, el del verde falso.
 */
final class ModSimbolos {

    private ModSimbolos() {}

    /** Todas las cadenas imprimibles del .mod, en orden de aparición. */
    static List<String> de(byte[] d) {
        List<String> out = new ArrayList<>();
        for (int i = 0; i + 2 < d.length; i++) {
            int len = ((d[i] & 0xFF) << 8) | (d[i + 1] & 0xFF);
            if (len < 3 || len > 200 || i + 2 + len > d.length) continue;
            boolean imprimible = true;
            for (int k = i + 2; k < i + 2 + len; k++) {
                if (d[k] < 0x20 || d[k] > 0x7E) { imprimible = false; break; }
            }
            if (imprimible) out.add(new String(d, i + 2, len, StandardCharsets.US_ASCII));
        }
        return out;
    }

    static List<String> de(Path mod) throws IOException {
        return de(Files.readAllBytes(mod));
    }

    static Set<String> conjunto(Path mod) throws IOException {
        return new HashSet<>(de(mod));
    }

    /** Las cadenas que empiezan por {@code prefijo}, ordenadas — para que un
     *  fallo diga qué hay de verdad en vez de sólo qué falta. */
    static List<String> conPrefijo(Iterable<String> todas, String prefijo) {
        Set<String> vistos = new HashSet<>();
        List<String> l = new ArrayList<>();
        for (String s : todas) if (s.startsWith(prefijo) && vistos.add(s)) l.add(s);
        l.sort(String::compareTo);
        return l;
    }
}
