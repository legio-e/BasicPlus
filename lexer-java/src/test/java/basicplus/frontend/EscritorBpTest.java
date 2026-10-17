package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * V8 #698 (JC, paso 1) — EscritorBp: AST → BP, y la vara de medir de «equivalente» (ComparaAst).
 *
 * <h3>Qué se está protegiendo</h3>
 *
 * El barrido (`--comprobar-bp` sobre samples, la stdlib y bpdevices) dice EQUIVALENTE en todo lo que se lee. Eso sólo
 * vale si la comparación de árboles sabe ver una diferencia: una comparación que dijera siempre «iguales» daría el
 * mismo barrido verde. Por eso la primera prueba es la que TIENE que fallar —un `+` por un `-`— y la segunda, la que
 * no tiene que fallar —las líneas en blanco y los comentarios no cuentan—. Las demás recorren, en pequeño, lo que el
 * barrido recorre en grande, para que un cambio del escritor se vea aquí con nombre.
 */
class EscritorBpTest {

    private static Ast.ModuleNode leer(String fuente) {
        Lexer lx = new Lexer(fuente);
        List<Token> toks = lx.tokenize();
        assertTrue(lx.getErrors().isEmpty(), "errores léxicos: " + lx.getErrors());
        Parser p = new Parser(toks);
        Ast.ModuleNode m = p.parseModule();
        assertTrue(p.getErrors().isEmpty(), "errores de sintaxis: " + p.getErrors());
        assertNotNull(m);
        return m;
    }

    /** La ida y vuelta: el árbol del BP reescrito es el del original, y reescribirlo da el mismo texto. */
    private static String idaYVuelta(String fuente) {
        Ast.ModuleNode a = leer(fuente);
        String bp = EscritorBp.escribir(a);
        Ast.ModuleNode b = leer(bp);
        assertNull(ComparaAst.primeraDiferencia(a, b), "distinto tras reescribir:\n" + bp);
        assertEquals(bp, EscritorBp.escribir(b), "el escritor no es estable");
        return bp;
    }

    @Test
    @DisplayName("la comparación VE una diferencia: un + por un -")
    void distingueUnOperador() {
        Ast.ModuleNode a = leer("module M\n  function f()\n    var x: integer := 1 + 2\n  end f\nend M\n");
        Ast.ModuleNode b = leer("module M\n  function f()\n    var x: integer := 1 - 2\n  end f\nend M\n");
        String d = ComparaAst.primeraDiferencia(a, b);
        assertNotNull(d, "un + y un - tienen que salir distintos");
        assertTrue(d.contains(".op"), d);
        assertTrue(d.contains("línea 3"), d);
    }

    @Test
    @DisplayName("la comparación no cuenta las líneas en blanco ni los comentarios")
    void noCuentaLasPosiciones() {
        Ast.ModuleNode a = leer("module M\n  function f()\n    var x: integer := 1\n  end f\nend M\n");
        Ast.ModuleNode b = leer("// cabecera\n\nmodule M\n\n\n  function f()   // f\n\n    var x: integer := 1\n  end f\nend M\n");
        assertNull(ComparaAst.primeraDiferencia(a, b));
    }

    @Test
    @DisplayName("un programa con casi todo el lenguaje hace la ida y vuelta")
    void casiTodoElLenguaje() {
        String fuente =
            "module Todo\n" +
            "  import Collections\n" +
            "  public const MAX: integer := 10\n" +
            "  var cuenta: integer := 0\n" +
            "  public enum Color\n    Rojo\n    Verde := 5\n  end Color\n" +
            "  @BD{ tabla = \"medidas\", clave }\n" +
            "  public class Medida extends Object\n" +
            "    var valor: double\n" +
            "    private var marca: long\n" +
            "    event cambia(m: Medida)\n" +
            "    public property Valor: double\n" +
            "      get\n        return this.valor\n      endget\n" +
            "      set(v)\n        this.valor := v\n        raise cambia(this)\n      endset\n" +
            "    endprop\n" +
            "    public function Medida(v: double := 0.0d)\n      this.valor := v\n    end Medida\n" +
            "    public function ~Medida()\n      print \"fin\"\n    end ~Medida\n" +
            "    public function Medida.crear(): Medida\n      return Medida(1.5d)\n    end Medida.crear\n" +
            "  end Medida\n" +
            "  function par(): (integer, string)\n    return (1, \"uno\")\n  end par\n" +
            "  public function main()\n" +
            "    var a, b: integer := 3\n" +
            "    var owner m: Medida := Medida(2.0d)\n" +
            "    var t: integer[4]\n" +
            "    var n: integer\n    var s: string\n" +
            "    { n, s } := par()\n" +
            "    if a > b and not (b == 0) then print \"si\"\n" +
            "    if a mod 2 == 0 then\n      a += 1\n    elseif a < 0 then\n      a -= 1\n    else\n      a := -a ^ 2\n    endif\n" +
            "    while a < 100 do a := a * 2\n" +
            "    for i := 1 to 10 step 2 do\n      t[i mod 4] := i shl 1\n    next\n" +
            "    for x in [1, 2, 3] do print x; \" \"\n" +
            "    do\n      a := a - 1\n    loop a > 0\n" +
            "    switch a\n      case 1, 2\n        print \"uno o dos\"\n      default\n        print \"otro\"\n    endsw\n" +
            "    try var r: Medida := Medida(3.0d)\n      print r.Valor\n    catch e\n      throw e\n    finally\n      print \"siempre\"\n    endtry\n" +
            "    try\n      print 1\n    catch\n      print 2\n    endtry\n" +
            "    if m instanceof Medida then print \"es\"\n" +
            "    print a, b; \"x\";\n" +
            "    print\n" +
            "  end main\n" +
            "end Todo\n";
        String bp = idaYVuelta(fuente);
        assertTrue(bp.contains("try var r: Medida := Medida(3.0d)"), bp);
        assertFalse(bp.contains("ReleaseOwner"), bp);
    }

    @Test
    @DisplayName("los literales vuelven con su valor y su tipo")
    void literales() {
        idaYVuelta("module L\n  function f()\n" +
                   "    var a: float := 1.5\n    var b: double := 1.5d\n    var c: long := 5L\n" +
                   "    var d: long := 3000000000\n    var e: integer := 0xFF\n    var g: integer := 0b1010\n" +
                   "    var h: integer := 1_000_000\n    var k: float := 1.5e-3\n" +
                   "    var s: string := \"a\\\"b\\n\\t\\\\c\\0\"\n    var z: boolean := true and false\n" +
                   "  end f\nend L\n");
    }

    @Test
    @DisplayName("el import Core que pone el parser no se escribe")
    void importCoreImplicito() {
        String bp = idaYVuelta("module I\n  function f()\n    print 1\n  end f\nend I\n");
        assertFalse(bp.contains("import Core"), bp);
        String conCore = idaYVuelta("module I\n  import Core\n  function f()\n    print 1\n  end f\nend I\n");
        assertTrue(conCore.contains("import Core"), conCore);
    }

    @Test
    @DisplayName("los sitios donde fabrica el parser: la native externa de un pack y la interfaz")
    void loQueFabricaElParser() {
        String bp = idaYVuelta("module Ext\n  import v1 from pack \"SQLI\"\n" +
                               "  public native function abrir(n: string): integer\n" +
                               "  public function g(): integer\n    return abrir(\"x\")\n  end g\nend Ext\n");
        assertFalse(bp.contains("RuntimeError"), bp);
        idaYVuelta("module interface Api extends Base.Contrato\n  public function f(a: integer): string\nend Api\n");
    }
}
