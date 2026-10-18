// ============================================================
// Traductor.java
// V8 #698 — traducir entre BP y JC desde fuera del compilador: el IDE.
// ============================================================
package basicplus.frontend;

import java.io.IOException;
import java.nio.file.Path;

/**
 * V8 #698 — la traducción BP ↔ JC para quien la llama desde dentro, el IDE: devuelve el texto, o por qué no, sin salir
 * del proceso ni escribir por la salida estándar. Las opciones `--escribir-jc` y `--escribir-bp` hacen lo mismo, pero
 * acaban en `System.exit` y escriben por el `stdout` del proceso.
 *
 * <p>Los comentarios no viajan: el lexer de los dos lenguajes los tira.
 */
public final class Traductor {

    /** La traducción, o el error: uno de los dos es null. */
    public static final class Resultado {
        public final String texto;
        public final String error;
        Resultado(String texto, String error) {
            this.texto = texto;
            this.error = error;
        }
    }

    private Traductor() { }

    /** A JC, un `.bp` (o un `.jc`, que se reescribe): desde el árbol analizado, como `--escribir-jc`. */
    public static Resultado aJc(Path fuente) {
        EscritorJc.Resultado r = EscritorJc.traducir(fuente);
        if (r.jc != null) return new Resultado(r.jc, null);
        return new Resultado(null, r.clase + ": " + r.detalle);
    }

    /** A BP, un `.jc` (o un `.bp`, que se reescribe): desde el árbol tal cual, como `--escribir-bp`. */
    public static Resultado aBp(Path fuente) {
        try {
            return EscritorBp.traducir(fuente);
        } catch (IOException e) {
            return new Resultado(null, "no se lee " + fuente + ": " + e.getMessage());
        }
    }
}
