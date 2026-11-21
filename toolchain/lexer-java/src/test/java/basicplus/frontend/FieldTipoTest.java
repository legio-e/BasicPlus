package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * #547 (V7) — dentro de un get/set, `field` tiene el tipo de su property. Era ErrorType, que por el
 * anti-cascada lo admite todo: ni chequeo al asignarle (`field := "hola"` en una integer compilaba) ni la
 * conversión del emisor, que convierte al tipo del destino (`field := 7` en una long apilaba 4 B para un
 * store de 8). La conversión en sí se prueba ejecutando: {@code samples/PropField.bp} en compat.sh.
 * Aquí, con una property de MÓDULO (no hace falta Core para analizarla); la de clase sale por el mismo
 * sitio (analyzeProperty) y la cubren PropField y samples/errores/PropFieldTipo.bp.
 */
class FieldTipoTest {

    private static SemanticInfo analiza(String src, List<String> errores) {
        Parser p = new Parser(new Lexer(src).tokenize());
        Ast.ModuleNode m = p.parseModule();
        assertTrue(p.getErrors().isEmpty(), "el fuente del test no parsea: " + p.getErrors() + "\n" + src);
        SemanticInfo info = new SemanticAnalyzer().analyze(m);
        for (SemanticDiagnostic d : info.diagnostics)
            if (d.kind == SemanticDiagnostic.Kind.ERROR) errores.add(d.line + ": " + d.message);
        return info;
    }

    private static String modulo(String tipo, String accesores) {
        return "module T\n  public property p: " + tipo + "\n" + accesores + "  endprop\n"
             + "  function Main()\n    print 1\n  end Main\nend T\n";
    }

    @Test @DisplayName("field := un string en una property integer: error, como en cualquier variable")
    void tipoIncompatible() {
        List<String> e = new ArrayList<>();
        analiza(modulo("integer", "    set(v)\n      field := \"hola\"\n    endset\n"), e);
        assertEquals(1, e.size(), e.toString());
        assertEquals("4: no se puede asignar 'string' a 'integer'", e.get(0));
    }

    @Test @DisplayName("field := 7 en una long, field += 1 y return field * 2: sin error")
    void conversionesValidas() {
        List<String> e = new ArrayList<>();
        analiza(modulo("long", "    get\n      return field * 2\n    endget\n    set(v)\n      field := 7\n      field += 1\n    endset\n"), e);
        assertTrue(e.isEmpty(), e.toString());
    }

    @Test @DisplayName("el tipo de `field` en el AST anotado es el de su property (el que usa el emisor para convertir)")
    void tipoAnotado() {
        List<String> e = new ArrayList<>();
        String src = modulo("long", "    set(v)\n      field := 7\n    endset\n");
        Parser p = new Parser(new Lexer(src).tokenize());
        Ast.ModuleNode m = p.parseModule();
        SemanticInfo info = new SemanticAnalyzer().analyze(m);
        Ast.PropertyDef pd = null;
        for (Ast.ITopLevelDecl d : m.defs) if (d instanceof Ast.PropertyDef) pd = (Ast.PropertyDef) d;
        Ast.AssignStmt a = (Ast.AssignStmt) pd.setter.body.get(0);
        BpType t = info.exprTypes.get(a.target);
        assertTrue(t != null && t.sameAs(BpType.PrimitiveType.LONG), "field es " + t + ", no long");
    }
}
