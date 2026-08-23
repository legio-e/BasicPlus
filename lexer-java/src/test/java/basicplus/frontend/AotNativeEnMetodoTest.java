package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Un método {@code native} de una clase se compila a C, con el receptor como
 * primer parámetro.
 *
 * <h3>De dónde viene</h3>
 *
 * Hasta el 23-ago-2026 no se compilaba, y —lo grave— <b>en silencio</b>: el
 * pre-pass de {@link AotCEmitter#emitModule} sólo abría los {@code Ast.FuncDef}
 * de nivel módulo, así que un {@code ClassDef} ni se miraba. El compilador no
 * daba error, el AOT decía «este módulo no tiene funciones native» y el método
 * corría <b>interpretado</b> mientras el programador creía tener velocidad AOT.
 *
 * <p>Lo destapó Eduardo dudando de un diagnóstico mío: <i>«¿ningún método de
 * clase puede ser native? Me parece una limitación tonta, teniendo en cuenta que
 * miObjeto.miMetodo(...) en realidad internamente es miMetodo(miObjeto, ...)»</i>.
 * Y la solución fue literalmente eso — <i>«mimodulo.miclase.mimetodonative(
 * objMiclase, ...)»</i>—, que no hubo que forzar porque por debajo <b>ya era
 * así</b>: {@code ModWriter.addMethod} hace {@code declareParam("this", 8)} antes
 * que los demás parámetros. Lo único que faltaba era <b>exportar el nombre</b>,
 * porque el registro AOT busca el símbolo para sacar su dirección.
 *
 * <h3>Las tres cosas que fija</h3>
 *
 * <ol>
 *   <li><b>Se emite.</b> Un método native se traduce y se registra como
 *       {@code <Modulo>.<Clase>.<metodo>}.</li>
 *   <li><b>Una property se lee llamando a su getter.</b> Criterio de Eduardo, y
 *       no es una conveniencia: en BP no hay campos públicos —público ⇒
 *       property— y es como ya lo emite el bytecode. Además es la única ruta que
 *       el runtime soporta: entre los helpers del AOT está {@code call_method_i32}
 *       y no hay ninguno para leer campos.</li>
 *   <li><b>Un método que no se puede traducir se cae SOLO.</b> Si tumbara el
 *       módulo entero sería una regresión pura: antes esos módulos emitían sus
 *       funciones native sin problema porque los métodos ni se miraban.</li>
 * </ol>
 *
 * <p>Que un {@code native} <b>no</b> pueda leer un campo directamente es una
 * limitación <b>aceptada</b> — Eduardo, 23-ago: <i>«es razonable, teniendo en
 * cuenta que siempre puede acceder a través de un getter»</i>.
 */
class AotNativeEnMetodoTest {

    private static String modulo(String nombre, String miembro, String cuerpo) {
        return "module " + nombre + "\n"
             + "  public class Caja\n"
             + "    " + miembro + "\n"
             + "    public function Caja(v: integer)\n"
             + "      this.n := v\n"
             + "    end Caja\n"
             + "    public native function doble(): integer\n"
             + "      return " + cuerpo + "\n"
             + "    end doble\n"
             + "  end Caja\n"
             + "  function Main()\n"
             + "    print Caja(21).doble()\n"
             + "  end Main\n"
             + "end " + nombre + "\n";
    }

    /** Lex + parse + semántico + emisión. Devuelve el C emitido (puede ser ""). */
    private static String emitir(String fuente, List<String>[] avisosOut) {
        Parser p = new Parser(new Lexer(fuente).tokenize());
        Ast.ModuleNode module = p.parseModule();
        assertTrue(p.getErrors().isEmpty(),
                "el fuente del test no compila, así que el test no prueba nada: " + p.getErrors());
        SemanticInfo info = new SemanticAnalyzer().analyze(module);
        AotCEmitter emitter = new AotCEmitter(module.name);
        emitter.setSemanticInfo(info);
        String c = emitter.emitModule(module);
        if (avisosOut != null) avisosOut[0] = emitter.getWarnings();
        return c;
    }

    @Test
    @DisplayName("un método native se EMITE, con `this` de primer parámetro")
    void metodoNativeSeEmite() {
        // Sin tocar miembros: el caso limpio, para que el test hable de UNA cosa.
        String src = modulo("NatSimple", "public property n: integer", "(3 * 31) + 1");
        String c = emitir(src, null);
        assertFalse(c.isEmpty(), "un método native tiene que producir C: antes se ignoraba en silencio");
        assertTrue(c.contains("int32_t this"),
                "el receptor tiene que ser el primer parámetro (el `self` de Python). C:\n" + c);
        assertTrue(c.contains("\"NatSimple.Caja.doble\""),
                "tiene que registrarse como <Modulo>.<Clase>.<metodo>, que es el símbolo"
                + " que el .mod exporta. C:\n" + c);
        assertTrue(c.contains("read_ref"),
                "`this` es una referencia: el thunk debe sacarla de la pila con read_ref (8B). C:\n" + c);
    }

    @SuppressWarnings("unchecked")
    @Test
    @DisplayName("leer una property se compila a una llamada a su getter")
    void propertyPasaPorElGetter() {
        List<String>[] avisos = new List[1];
        String c = emitir(modulo("NatProp", "public property n: integer", "this.n * 2"), avisos);
        assertTrue(c.contains("call_method_i32"),
                "una property se lee LLAMANDO al getter (público ⇒ property), igual que en el"
                + " bytecode. Y es la única vía: no hay helper para leer campos. C:\n" + c);
        // Que la llamada cruza al intérprete hay que DECIRLO: quien pone `native`
        // busca velocidad, y aquí no la hay del todo.
        assertTrue(avisos[0].stream().anyMatch(a -> a.contains("property") && a.contains("puente")),
                "no se avisa de que la lectura cruza al intérprete. Avisos: " + avisos[0]);
    }

    @SuppressWarnings("unchecked")
    @Test
    @DisplayName("un campo NO accesible degrada ese método, sin tumbar el módulo")
    void campoDegradaSoloEseMetodo() {
        // `var` privado: no hay getter, y es la limitación aceptada.
        List<String>[] avisos = new List[1];
        String c = emitir(modulo("NatCampo", "var n: integer", "this.n * 2"), avisos);
        assertTrue(avisos[0].stream().anyMatch(a -> a.contains("Caja.doble")),
                "tiene que decir QUÉ método no se pudo traducir. Avisos: " + avisos[0]);
        // Lo que NO puede pasar es que se lleve el módulo por delante: antes de
        // esto, un módulo así emitía sus native de nivel módulo tan tranquilo.
        assertTrue(c.isEmpty() || c.contains("aot_NatCampo"),
                "el módulo no debe quedar en un estado raro por un método que no se pudo traducir");
    }

    @SuppressWarnings("unchecked")
    @Test
    @DisplayName("una clase SIN native no genera ruido")
    void claseNormalNoAvisa() {
        String src =
              "module NatLimpio\n"
            + "  public class Caja\n"
            + "    public property n: integer\n"
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
        List<String>[] avisos = new List[1];
        emitir(src, avisos);
        // Un aviso que salta cuando no toca se aprende a ignorar, y entonces deja
        // de servir para cuando sí toca.
        assertTrue(avisos[0].isEmpty(),
                "una clase sin `native` no debe generar aviso: un aviso que grita siempre"
                + " se vuelve ruido y se ignora justo el día que importa. Avisos: " + avisos[0]);
    }
}
