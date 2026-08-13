package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * #387 — con {@code load(clave)} y {@code load(objeto)}, un literal entero tiene
 * que elegir la de la clave. Y sola: no es un empate que haya que deshacer con un
 * cast.
 *
 * <h3>El caso</h3>
 *
 * <pre>
 *   public function load(id: long): Medida
 *   public function load(m: Medida): boolean
 *   ...
 *   dao.load(9999)   →  error semántico: llamada ambigua a 'load' con (integer)
 * </pre>
 *
 * Un {@code integer} no se convierte en una clase, así que nunca hubo dos
 * candidatas: {@code integer → long} es una promoción normal y gana sola. El
 * empate salía porque el tipo del parámetro de la SEGUNDA firma llegaba sin
 * resolver ({@code UnresolvedClassRef}), y ése es «asignable desde todo» a
 * propósito —una red para no soltar errores confusos si algo no se resolvió—, así
 * que puntuaba lo mismo que la promoción.
 *
 * <p>Y llegaba sin resolver porque los pases que resuelven los tipos de un import
 * recorrían el MAPA de símbolos, donde sólo está la primera firma de cada grupo:
 * las demás cuelgan de ella por {@code nextOverload}. Ver {@code Main.firmasDe}.
 *
 * <h3>Por qué importa más de lo que parece</h3>
 *
 * {@code load(clave)} / {@code load(objeto)} es la forma que tiene TODO DAO,
 * incluidos los que genera el compilador en el ORM. Cualquiera que escriba
 * {@code dao.load(3)} con una clave literal se lo encontraba.
 *
 * <h3>Sólo pasaba CROSS-MODULE</h3>
 *
 * Dentro del mismo módulo los tipos ya vienen resueltos del análisis, así que ahí
 * siempre funcionó. Por eso los módulos de esta prueba son DOS: con uno solo iría
 * verde sin comprobar nada.
 *
 * <h3>Qué comprueba, además de que compile</h3>
 *
 * Que compile sólo dice que ya no hay error. Lo que hay que asegurar es que
 * eligió la sobrecarga CORRECTA, y eso se ve en el símbolo que el llamante pide:
 * la 1ª firma de un grupo va con el nombre pelado y las demás mangleadas (regla
 * H5.a), así que pedir {@code find} y no {@code find$LMedida;} ES la prueba.
 */
class SobrecargaCrossModuleTest {

    /** La librería: un método y una función libre, cada uno con las dos firmas
     *  del patrón DAO. La de la clave va PRIMERA, como en el caso real. */
    private static final String LIB =
        "module OvLib\n"
      + "  public class Medida\n"
      + "    public property v: integer\n"
      + "    public function Medida()\n"
      + "      this.v := 0\n"
      + "    end Medida\n"
      + "  end Medida\n"
      + "  public class Dao\n"
      + "    public function Dao()\n"
      + "    end Dao\n"
      + "    public function load(id: long): Medida\n"
      + "      var m: Medida := Medida()\n"
      + "      m.v := 1\n"
      + "      return m\n"
      + "    end load\n"
      + "    public function load(m: Medida): boolean\n"
      + "      return true\n"
      + "    end load\n"
      + "  end Dao\n"
      + "  public function find(id: long): Medida\n"
      + "    var m: Medida := Medida()\n"
      + "    m.v := 7\n"
      + "    return m\n"
      + "  end find\n"
      + "  public function find(m: Medida): boolean\n"
      + "    return true\n"
      + "  end find\n"
      + "end OvLib\n";

    private static final String USO =
        "module OvUso\n"
      + "  import OvLib\n"
      + "  function Main(arg: string)\n"
      + "    var dao: OvLib.Dao := OvLib.Dao()\n"
      + "    var a: OvLib.Medida := dao.load(9999)      // metodo\n"
      + "    var b: OvLib.Medida := OvLib.find(9999)    // funcion libre\n"
      + "    print a.v, b.v\n"
      + "  end Main\n"
      + "end OvUso\n";

    private static Path compilar(Path tmp) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve("OvLib.bp"), LIB.getBytes(StandardCharsets.UTF_8));
        Files.write(src.resolve("OvUso.bp"), USO.getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = "OvUso";
        // Éste es EL assert del bug: antes devolvía false con dos «llamada
        // ambigua». Va aquí, y no en un test aparte, porque sin compilar no hay
        // .mod que mirar y los dos casos de abajo no podrían ni ejecutarse.
        assertTrue(Main.buildProject(b, "mivm", false),
                "con load(long)/load(Medida), un literal entero NO es ambiguo:"
                + " integer→long es una promoción y integer→clase no existe");
        return out;
    }

    @Test
    @DisplayName("un método sobrecargado importado elige la firma de la CLAVE, no la del objeto")
    void metodoImportadoEligeLaClave(@TempDir Path tmp) throws Exception {
        Path out = compilar(tmp);
        Set<String> pide = ModSimbolos.conjunto(out.resolve("OvUso.mod"));
        // El slot dice cuál eligió: load(long)=2 y load(Medida)=3 (detrás de
        // toString 0 y compareTo 1, que vienen de Object).
        assertTrue(pide.contains("OvLib.Dao#load#2"),
                "el llamante no pide la sobrecarga de 'long'. Pide: "
                + ModSimbolos.conPrefijo(pide, "OvLib.Dao#"));
        assertTrue(!pide.contains("OvLib.Dao#load$LMedida;#3"),
                "el llamante pide la sobrecarga de Medida para un literal entero");
    }

    @Test
    @DisplayName("una función libre sobrecargada importada, igual")
    void funcionLibreImportadaEligeLaClave(@TempDir Path tmp) throws Exception {
        Path out = compilar(tmp);
        Set<String> pide = ModSimbolos.conjunto(out.resolve("OvUso.mod"));
        // Nombre pelado = 1ª firma del grupo = find(long); la otra iría mangleada.
        assertTrue(pide.contains("OvLib.find"),
                "el llamante no pide 'find' pelado (la 1ª firma). Pide: "
                + ModSimbolos.conPrefijo(pide, "OvLib.find"));
        assertTrue(!pide.contains("OvLib.find$LMedida;"),
                "el llamante pide find(Medida) para un literal entero");
    }
}
