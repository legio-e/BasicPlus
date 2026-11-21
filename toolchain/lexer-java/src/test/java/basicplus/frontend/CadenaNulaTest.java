package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * V8 #774, #776 y #777 — el null de las cadenas en el semántico. Una variable string puede valer null; una cadena, y
 * una constante, no. Un campo, una property o una variable de módulo string sin valor nace "" (un inicializador
 * implícito, que los escritores no escriben y el emisor pone antes del super(): #776, eso lo prueba
 * bpgenvm-c/samples/CadenaNace.bp en compat.sh); una interfaz no guarda nada, así que a la suya no se le pone.
 */
class CadenaNulaTest {

    private static Ast.ModuleNode analiza(String src, List<String> errores) {
        Parser p = new Parser(new Lexer(src).tokenize());
        Ast.ModuleNode m = p.parseModule();
        assertTrue(p.getErrors().isEmpty(), "el fuente del test no parsea: " + p.getErrors() + "\n" + src);
        SemanticInfo info = new SemanticAnalyzer().analyze(m);
        // sin Core.mod a mano, una clase no encuentra su base (Object): lo que se prueba aquí (el "" implícito) se
        // decide antes, en cadenasNacenVacias, y no depende de ella
        for (SemanticDiagnostic d : info.diagnostics)
            if (d.kind == SemanticDiagnostic.Kind.ERROR && !d.message.startsWith("no encuentro la clase Object"))
                errores.add(d.line + ": " + d.message);
        return m;
    }

    @Test @DisplayName("una variable string puede valer null: local, de módulo, asignada, devuelta y comparada")
    void variableNula() {
        List<String> e = new ArrayList<>();
        analiza("module T\n  var g: string := null\n  function nada(): string\n    return null\n  end nada\n"
              + "  function Main()\n    var s: string := null\n    s := nada()\n    print s == null\n  end Main\nend T\n", e);
        assertTrue(e.isEmpty(), e.toString());
    }

    @Test @DisplayName("#777: una constante string no puede ser null (de módulo y local)")
    void constanteNula() {
        List<String> e = new ArrayList<>();
        analiza("module T\n  const X: string := null\n  function Main()\n    const Y: string := null\n    print 1\n"
              + "  end Main\nend T\n", e);
        assertEquals(2, e.size(), e.toString());
        assertTrue(e.get(0).startsWith("2: la constante 'X' no puede ser null: una constante es un valor"), e.toString());
        assertTrue(e.get(1).startsWith("4: la constante 'Y' no puede ser null"), e.toString());
    }

    @Test @DisplayName("#774: un campo y una property string sin valor llevan el \"\" implícito; un integer, no")
    void campoNaceVacio() {
        List<String> e = new ArrayList<>();
        Ast.ModuleNode m = analiza("module T\n  public class C\n    var s: string\n    var n: integer\n"
              + "    public property p: string\n  end C\n  function Main()\n    print 1\n  end Main\nend T\n", e);
        assertTrue(e.isEmpty(), e.toString());
        Ast.ClassDef c = (Ast.ClassDef) m.defs.get(0);
        Ast.VarDecl s = (Ast.VarDecl) c.members.get(0), n = (Ast.VarDecl) c.members.get(1);
        Ast.PropertyDef p = (Ast.PropertyDef) c.members.get(2);
        assertTrue(Ast.implicito(s.init), "el campo string, con su \"\" implícito");
        assertTrue(Ast.implicito(p.init), "la property string, con su \"\" implícito");
        assertNull(n.init, "un integer sin valor no lleva nada (nace a 0)");
    }

    @Test @DisplayName("#774: la property string de una interfaz no lleva el \"\" (una interfaz no guarda nada)")
    void interfazSinValor() {
        List<String> e = new ArrayList<>();
        Ast.ModuleNode m = analiza("module T\n  public interface Etiquetado\n    property etiqueta: string\n"
              + "  end Etiquetado\n  function Main()\n    print 1\n  end Main\nend T\n", e);
        assertTrue(e.isEmpty(), e.toString());
        Ast.ClassDef i = (Ast.ClassDef) m.defs.get(0);
        assertNull(((Ast.PropertyDef) i.members.get(0)).init);
    }
}
