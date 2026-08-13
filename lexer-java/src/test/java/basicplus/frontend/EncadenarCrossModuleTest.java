package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * #388 — lo que DEVUELVE un método importado se puede seguir usando: encadenar.
 *
 * <h3>Los dos casos, que resultaron ser dos bugs distintos</h3>
 *
 * <pre>
 *   w.eq("a", 1).gt("b", 2.0d)   // devuelve una CLASE DEL MÓDULO importado
 *   dao.list(w).length()         // devuelve un BUILT-IN (List)
 * </pre>
 *
 * El primero era #387 visto por el otro lado: a las sobrecargas que no son la 1ª
 * firma no se les resolvían los tipos —tampoco el de RETORNO—, así que el
 * resultado de `eq(col, v: long)` (la 2ª de tres) no tenía miembros. Se comprobó
 * con el código anterior a #387: daba exactamente «el tipo 'Where' no tiene
 * miembros», y sólo en esa línea.
 *
 * <p>El segundo seguía vivo y es de otra familia: `List` no es de ningún módulo
 * —la declara el analizador— así que el resolvedor de imports no la encontraba
 * por mucho que buscase bien entre las clases del módulo importado. Ver
 * {@code Main.resolverComoBuiltin}.
 *
 * <h3>Por qué los dos casos van juntos en una prueba</h3>
 *
 * Porque desde fuera son el MISMO síntoma —«el tipo X no tiene miembros» al
 * encadenar— y por eso se habían apuntado como un solo bug. Tenerlos juntos deja
 * escrito que son dos mecanismos, y que arreglar uno no cubre el otro.
 *
 * <h3>Lo que esto desbloquea</h3>
 *
 * Las interfaces fluidas: builders, y el `Where` estilo LINQ de samples/Orm.bp,
 * que ya está escrito devolviendo `this` en todos sus métodos.
 */
class EncadenarCrossModuleTest {

    private static final String LIB =
        "module CadLib\n"
      + "  public class Where\n"
      + "    public property n: integer\n"
      + "    public function Where()\n"
      + "      this.n := 0\n"
      + "    end Where\n"
      + "    public function eq(col: string, v: string): Where\n"   // 1ª firma
      + "      return this\n"
      + "    end eq\n"
      + "    public function eq(col: string, v: long): Where\n"     // 2ª — la que fallaba
      + "      return this\n"
      + "    end eq\n"
      + "    public function gt(col: string, v: double): Where\n"
      + "      return this\n"
      + "    end gt\n"
      + "  end Where\n"
      + "  public class Dao\n"
      + "    public function Dao()\n"
      + "    end Dao\n"
      // NO sobrecargado y devolviendo un BUILT-IN: el otro caso, tal cual está
      // en el Orm real (`list(w: Where, max: integer := 1000): List`).
      + "    public function list(w: Where, max: integer := 1000): List\n"
      + "      var l: List := List()\n"
      + "      l.add(w)\n"
      + "      return l\n"
      + "    end list\n"
      + "  end Dao\n"
      + "end CadLib\n";

    private static final String USO =
        "module CadUso\n"
      + "  import CadLib\n"
      + "  function Main(arg: string)\n"
      + "    var w: CadLib.Where := CadLib.Where()\n"
      + "    var d: CadLib.Dao   := CadLib.Dao()\n"
      + "    print w.eq(\"a\", 1L).gt(\"b\", 2.0d).n\n"   // clase del módulo, 2ª firma
      + "    print d.list(w).length()\n"                  // built-in devuelto
      + "  end Main\n"
      + "end CadUso\n";

    private static boolean compila(Path tmp) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve("CadLib.bp"), LIB.getBytes(StandardCharsets.UTF_8));
        Files.write(src.resolve("CadUso.bp"), USO.getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = "CadUso";
        return Main.buildProject(b, "mivm", false);
    }

    @Test
    @DisplayName("se puede encadenar sobre lo que devuelve un método importado")
    void encadenarSobreLoDevuelto(@TempDir Path tmp) throws Exception {
        assertTrue(compila(tmp),
                "encadenar sobre el resultado de un método importado debe compilar."
                + " Los dos casos del fuente son de mecanismos distintos: una clase"
                + " DEL MÓDULO devuelta por una sobrecarga que no es la 1ª firma"
                + " (#387), y un BUILT-IN devuelto por un método normal (#388).");
    }
}
