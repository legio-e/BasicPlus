package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * V8 #771 — `ev == null` y `ev != null` («¿tiene suscriptor?») valen con un evento dentro de su clase o de una hija, como
 * `raise`; cualquier otro uso de un evento como valor es un error en su línea (antes llegaba al emisor, que se paraba con
 * «símbolo no soportado en load: EventSymbol»). Lo que hace en ejecución, bpgenvm-c/samples/EventoNulo.bp en compat.sh.
 */
class EventoNuloTest {

    private static void analiza(String src, List<String> errores) {
        Parser p = new Parser(new Lexer(src).tokenize());
        Ast.ModuleNode m = p.parseModule();
        assertTrue(p.getErrors().isEmpty(), "el fuente del test no parsea: " + p.getErrors() + "\n" + src);
        SemanticInfo info = new SemanticAnalyzer().analyze(m);
        // sin Core.mod a mano, una clase no encuentra su base (Object): no cambia lo que se prueba aquí
        for (SemanticDiagnostic d : info.diagnostics)
            if (d.kind == SemanticDiagnostic.Kind.ERROR && !d.message.startsWith("no encuentro la clase Object"))
                errores.add(d.line + ": " + d.message);
    }

    private static String clase(String cuerpo, String main) {
        return "module T\n  public class Boton\n    event onClick(veces: integer)\n    public function Boton()\n"
             + "    end Boton\n" + cuerpo + "  end Boton\n  public function main()\n" + main + "  end main\nend T\n";
    }

    @Test @DisplayName("dentro de su clase: `onClick != null`, `this.onClick == null` y `null != onClick` valen")
    void dentroDeSuClase() {
        List<String> e = new ArrayList<>();
        analiza(clase("    public function tiene(): boolean\n      return onClick != null\n    end tiene\n"
                    + "    public function sin(): boolean\n      return this.onClick == null\n    end sin\n"
                    + "    public function otra(): boolean\n      return null != onClick\n    end otra\n",
                      "    print 1\n"), e);
        assertTrue(e.isEmpty(), e.toString());
    }

    @Test @DisplayName("desde fuera de su clase, compararlo con null es un error")
    void desdeFuera() {
        List<String> e = new ArrayList<>();
        analiza(clase("", "    var b: Boton := Boton()\n    print b.onClick != null\n"), e);
        assertEquals(1, e.size(), e.toString());
        assertTrue(e.get(0).contains("compararlo con null («¿tiene suscriptor?») sólo vale dentro de su clase"), e.toString());
    }

    @Test @DisplayName("un evento como valor (guardarlo, compararlo con otra cosa): error")
    void comoValor() {
        List<String> e = new ArrayList<>();
        analiza(clase("    public function mal()\n      var x: integer := onClick\n      print onClick == 5\n    end mal\n",
                      "    print 1\n"), e);
        assertEquals(2, e.size(), e.toString());
        for (String s : e) assertTrue(s.contains("'onClick' es un evento, no un valor"), e.toString());
    }
}
