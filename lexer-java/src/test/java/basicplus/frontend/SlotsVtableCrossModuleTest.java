package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * #392 — el slot con el que un módulo PIDE un método tiene que ser el mismo con
 * el que su dueño lo EXPORTA.
 *
 * <h3>El caso</h3>
 *
 * Tres módulos: una base, una hija que la extiende (cross-module) con VARIAS
 * SOBRECARGAS de un mismo nombre, y un tercero que la usa. El dueño exportaba
 * {@code CosaDao#despues#10} y el importador pedía {@code #despues#8} — la
 * diferencia eran exactamente las 2 sobrecargas de más. Compilaba limpio, sin un
 * solo diagnóstico, y reventaba al ENLAZAR con «lib presente pero no exporta
 * ... (¿versión vieja?)», que manda a mirar justo donde no está el problema: los
 * dos .mod eran del mismo build y del mismo minuto.
 *
 * <h3>Por qué la prueba mira los DOS .mod y no la salida del programa</h3>
 *
 * Lo que se rompió es un CONTRATO entre dos ficheros, y un contrato se comprueba
 * comparándolos. Ejecutar el programa también lo destapa, pero sólo si el camino
 * pasa por el método afectado: aquí la hija tiene seis métodos y el corrimiento
 * empieza en el cuarto, así que una prueba de salida podría ir verde con la
 * vtable ya torcida. Comparar los símbolos lo ve entero.
 *
 * <p>Y por eso la comprobación es GENERAL —«todo lo que se pide, se exporta»— en
 * vez de fijar el número 10: así cubre cualquier forma que rompa el reparto, no
 * sólo la que ya conocemos. El caso concreto se fija aparte, porque el número
 * también tiene que dejar de moverse.
 */
class SlotsVtableCrossModuleTest {

    // ── Los tres módulos. La HIJA vive en otro módulo que la BASE a propósito:
    //    es la combinación que fallaba, y es además la real (nuestra base en la
    //    stdlib, la generada en el proyecto del usuario).

    private static final String BASE =
        "module SlotBase\n"
      + "  public class Base\n"
      + "    public property n: integer\n"
      + "    public function Base()\n"
      + "      this.n := 0\n"
      + "    end Base\n"
      + "    public function read(i: integer): Object\n"
      + "      return null\n"
      + "    end read\n"
      + "    public function etiqueta(): string\n"
      + "      return \"base\"\n"
      + "    end etiqueta\n"
      + "  end Base\n"
      + "end SlotBase\n";

    private static final String HIJA =
        "module SlotDao\n"
      + "  import SlotBase\n"
      + "  public class Cosa\n"
      + "    public property v: integer\n"
      + "    public function Cosa()\n"
      + "      this.v := 0\n"
      + "    end Cosa\n"
      + "  end Cosa\n"
      + "  public class CosaDao extends SlotBase.Base\n"
      + "    public function CosaDao()\n"
      + "    end CosaDao\n"
      + "    public function etiqueta(): string\n"        // override: conserva su ranura
      + "      return \"cosa\"\n"
      + "    end etiqueta\n"
      + "    public function buscar(i: integer): integer\n"   // 1ª firma: nombre pelado
      + "      return 1\n"
      + "    end buscar\n"
      + "    public function buscar(s: string): integer\n"    // 2ª: MANGLEADA, ranura propia
      + "      return 2\n"
      + "    end buscar\n"
      + "    public function buscar(c: Cosa): integer\n"      // 3ª: idem
      + "      return 3\n"
      + "    end buscar\n"
      + "    public function despues(): string\n"             // el que quedaba corrido
      + "      return \"ok\"\n"
      + "    end despues\n"
      + "  end CosaDao\n"
      + "end SlotDao\n";

    private static final String USO =
        "module SlotUso\n"
      + "  import SlotBase\n"
      + "  import SlotDao\n"
      + "  function Main(arg: string)\n"
      + "    var dao: SlotDao.CosaDao := SlotDao.CosaDao()\n"
      + "    print dao.etiqueta()\n"
      + "    print dao.despues()\n"
      + "  end Main\n"
      + "end SlotUso\n";

    /** Compila los tres a .mod y devuelve el outDir. */
    private static Path compilarLosTres(Path tmp) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve("SlotBase.bp"), BASE.getBytes(StandardCharsets.UTF_8));
        Files.write(src.resolve("SlotDao.bp"),  HIJA.getBytes(StandardCharsets.UTF_8));
        Files.write(src.resolve("SlotUso.bp"),  USO.getBytes(StandardCharsets.UTF_8));

        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = "SlotUso";
        org.junit.jupiter.api.Assertions.assertTrue(
                Main.buildProject(b, "mivm", false),
                "el proyecto de la prueba debe compilar sin errores");
        return out;
    }

    // ── Extracción de símbolos ─────────────────────────────────────────────
    //
    // Las cadenas del .mod van con la LONGITUD delante (2 bytes big-endian), o
    // sea que se pueden sacar exactas y no «buscando texto»: sin la longitud,
    // un `#10` seguido de una cadena que empiece por dígito se leería como
    // `#103` y la prueba mentiría en la dirección peor (verde falso).

    private static List<String> cadenasDe(byte[] d) {
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

    /** `Modulo.Clase#clave#slot` — la forma con la que un módulo PIDE un método
     *  de una clase importada. La clave puede venir mangleada (`buscar$LCosa;`),
     *  así que ahí vale cualquier cosa que no sea '#'. */
    private static final Pattern PEDIDO =
            Pattern.compile("^([A-Za-z][A-Za-z0-9_]*)\\.([A-Za-z][A-Za-z0-9_]*)#([^#]+)#(\\d+)$");

    @Test
    @DisplayName("todo método que se PIDE con un slot, su dueño lo EXPORTA con ese slot")
    void loQueSePideEsLoQueSeExporta(@TempDir Path tmp) throws Exception {
        Path out = compilarLosTres(tmp);
        Set<String> exportaDao = new HashSet<>(cadenasDe(Files.readAllBytes(out.resolve("SlotDao.mod"))));
        List<String> delUso    = cadenasDe(Files.readAllBytes(out.resolve("SlotUso.mod")));

        int comprobados = 0;
        for (String s : delUso) {
            Matcher m = PEDIDO.matcher(s);
            if (!m.matches() || !"SlotDao".equals(m.group(1))) continue;
            String enElDueno = m.group(2) + "#" + m.group(3) + "#" + m.group(4);
            org.junit.jupiter.api.Assertions.assertTrue(exportaDao.contains(enElDueno),
                    "'SlotUso' pide '" + s + "' pero 'SlotDao' no lo exporta así."
                    + " Los dos .mod salen del MISMO build, o sea que no es un"
                    + " desfase de versiones: es que los dos lados numeran la"
                    + " vtable distinto. Lo que SÍ exporta de esa clase: "
                    + ordenados(exportaDao, m.group(2) + "#"));
            comprobados++;
        }
        // Sin esto, un cambio de formato que dejara de emitir estos símbolos
        // daría VERDE sin comprobar nada — el fallo silencioso de las pruebas
        // que recorren una lista que resultó estar vacía.
        org.junit.jupiter.api.Assertions.assertTrue(comprobados >= 2,
                "sólo " + comprobados + " símbolos de clase importada en SlotUso.mod;"
                + " se esperaban al menos los 2 métodos que llama. ¿Cambió el formato?");
    }

    @Test
    @DisplayName("la hija numera DETRÁS de la base, y cada sobrecarga gasta su ranura")
    void cadaSobrecargaGastaSuRanura(@TempDir Path tmp) throws Exception {
        Path out = compilarLosTres(tmp);
        Set<String> dao = new HashSet<>(cadenasDe(Files.readAllBytes(out.resolve("SlotDao.mod"))));

        // La cuenta, escrita entera para que un cambio de layout se vea aquí y
        // haya que decidirlo, en vez de descubrirlo enlazando:
        //   Base = toString 0, compareTo 1, getN 2, setN 3, read 4, etiqueta 5
        //   CosaDao sigue en la 6: buscar 6, buscar$s 7, buscar$LCosa; 8, despues 9
        // `etiqueta` es override y conserva la 5 — no gasta ranura nueva.
        for (String esperado : new String[]{
                "CosaDao#etiqueta#5",
                "CosaDao#buscar#6",
                "CosaDao#buscar$s#7",
                "CosaDao#buscar$LCosa;#8",
                "CosaDao#despues#9"}) {
            org.junit.jupiter.api.Assertions.assertTrue(dao.contains(esperado),
                    "falta '" + esperado + "' entre los símbolos de SlotDao.mod."
                    + " Exporta: " + ordenados(dao, "CosaDao#"));
        }
    }

    private static String ordenados(Set<String> todos, String prefijo) {
        List<String> l = new ArrayList<>();
        for (String s : todos) if (s.startsWith(prefijo)) l.add(s);
        l.sort(String::compareTo);
        return l.toString();
    }
}
