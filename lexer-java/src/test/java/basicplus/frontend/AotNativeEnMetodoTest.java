package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Un método {@code native} de una clase NO se compila a código nativo — pero
 * tiene que DECIRLO.
 *
 * <h3>Qué pasaba</h3>
 *
 * El pre-pass de {@link AotCEmitter#emitModule} que recolecta las {@code native}
 * recorre {@code module.defs} y sólo abre los {@code Ast.FuncDef} de nivel
 * módulo. Un {@code ClassDef} <b>no entra</b>, así que sus métodos ni se miran.
 * No se rechazaban: no se veían.
 *
 * <p>Y por eso el fallo era <b>mudo</b>, que es lo grave: el compilador no daba
 * error, el AOT decía «este módulo no tiene funciones native — sin emisión», y el
 * método corría <b>interpretado</b> mientras el programador creía tener velocidad
 * AOT. Pedir velocidad y que te la nieguen sin avisar.
 *
 * <p>Lo destapó Eduardo el 21-ago-2026 dudando de un diagnóstico mío: <i>«¿ningún
 * método de clase puede ser native? Me parece una limitación tonta, teniendo en
 * cuenta que miObjeto.miMetodo(...) en realidad internamente es
 * miMetodo(miObjeto, ...)»</i>. Tenía razón, y de paso corrigió mi explicación:
 * la barrera no era «no hay {@code this}» —el emisor ya sabe emitir
 * {@code MemberAccessExpr}— sino el barrido.
 *
 * <h3>Qué fija este test, y qué NO</h3>
 *
 * Fija <b>el aviso</b>, que es la mitad barata del arreglo (V6/N1.1) y sigue el
 * criterio que el proyecto ya aplica en el gate del {@code .mod}, el
 * {@code magic} de la BIOS y el aviso de {@code /lib} rancio: un desfase mudo es
 * peor que uno ruidoso.
 *
 * <p><b>No</b> fija que el método se emita. Cuando se abra el barrido —la otra
 * mitad— este test tendrá que cambiar, y eso es correcto: entonces el aviso
 * sobrará porque ya no habrá nada que avisar.
 *
 * <h3>Por qué DOS casos y no uno</h3>
 *
 * Porque hay dos caminos distintos dentro de {@code emitModule} y el aviso tiene
 * que salir por los dos:
 *
 * <ul>
 *   <li><b>Sólo métodos native</b>: {@code nativeFuncs} queda vacío y la función
 *       sale por un {@code return ""} temprano. Es el caso más peligroso —el
 *       módulo entero parece no tener nada nativo— y es justo el que se colaba.</li>
 *   <li><b>Mezcla</b>: hay una {@code native} de nivel módulo que sí se emite. El
 *       aviso tiene que convivir con una emisión normal, sin romperla.</li>
 * </ul>
 */
class AotNativeEnMetodoTest {

    /** Módulo cuyas ÚNICAS native son métodos: el camino del `return ""`. */
    private static final String SOLO_METODO =
          "module NatMet\n"
        + "  class Caja\n"
        + "    var n: integer\n"
        + "    public function Caja(v: integer)\n"
        + "      this.n := v\n"
        + "    end Caja\n"
        + "    public native function doble(): integer\n"
        + "      return this.n * 2\n"
        + "    end doble\n"
        + "  end Caja\n"
        + "  function Main()\n"
        + "    var c: Caja := Caja(21)\n"
        + "    print c.doble()\n"
        + "  end Main\n"
        + "end NatMet\n";

    /** Una native de módulo (que SÍ se emite) y otra en una clase (que no). */
    private static final String MEZCLA =
          "module NatMix\n"
        + "  native function suma(a: integer, b: integer): integer\n"
        + "    return a + b\n"
        + "  end suma\n"
        + "  class Caja\n"
        + "    var n: integer\n"
        + "    public function Caja(v: integer)\n"
        + "      this.n := v\n"
        + "    end Caja\n"
        + "    public native function doble(): integer\n"
        + "      return this.n * 2\n"
        + "    end doble\n"
        + "  end Caja\n"
        + "  function Main()\n"
        + "    print suma(1, 2)\n"
        + "  end Main\n"
        + "end NatMix\n";

    /** Lex + parse + semántico + emisión. Devuelve el emisor, ya con sus avisos. */
    private static AotCEmitter emitir(String fuente) {
        Parser p = new Parser(new Lexer(fuente).tokenize());
        Ast.ModuleNode module = p.parseModule();
        assertTrue(p.getErrors().isEmpty(),
                "el fuente del test no compila, así que el test no prueba nada: " + p.getErrors());
        SemanticInfo info = new SemanticAnalyzer().analyze(module);
        AotCEmitter emitter = new AotCEmitter(module.name);
        emitter.setSemanticInfo(info);
        emitter.emitModule(module);
        return emitter;
    }

    private static boolean avisaDe(List<String> avisos, String metodo) {
        return avisos.stream().anyMatch(a -> a.contains(metodo));
    }

    @Test
    @DisplayName("un módulo cuyas únicas native son métodos AVISA (antes callaba)")
    void soloMetodoAvisa() {
        List<String> avisos = emitir(SOLO_METODO).getWarnings();
        assertFalse(avisos.isEmpty(),
                "el módulo tiene un método native y el AOT no lo emite: tiene que decirlo."
                + " Éste es el caso mudo que se colaba.");
        assertTrue(avisaDe(avisos, "Caja.doble"),
                "el aviso no NOMBRA el método, así que no dice qué mirar. Avisos: " + avisos);
    }

    @Test
    @DisplayName("con una native de módulo al lado, el aviso sale igual y la emisión no se rompe")
    void mezclaAvisaYSigueEmitiendo() {
        AotCEmitter em = emitir(MEZCLA);
        assertTrue(avisaDe(em.getWarnings(), "Caja.doble"),
                "el aviso se pierde cuando hay una native de módulo. Avisos: " + em.getWarnings());
    }

    @Test
    @DisplayName("una clase SIN native no genera ruido")
    void claseNormalNoAvisa() {
        String limpio =
              "module NatLimpio\n"
            + "  class Caja\n"
            + "    var n: integer\n"
            + "    public function Caja(v: integer)\n"
            + "      this.n := v\n"
            + "    end Caja\n"
            + "    public function doble(): integer\n"
            + "      return this.n * 2\n"
            + "    end doble\n"
            + "  end Caja\n"
            + "  function Main()\n"
            + "    print Caja(21).doble()\n"
            + "  end Main\n"
            + "end NatLimpio\n";
        // Un aviso que salta cuando no toca se aprende a ignorar, y entonces deja
        // de servir para cuando sí toca.
        assertFalse(avisaDe(emitir(limpio).getWarnings(), "Caja.doble"),
                "una clase sin `native` no debe generar aviso: un aviso que grita siempre"
                + " se vuelve ruido y se ignora justo el día que importa.");
    }
}
