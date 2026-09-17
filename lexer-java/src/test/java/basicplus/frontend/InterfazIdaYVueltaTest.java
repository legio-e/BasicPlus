package basicplus.frontend;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.stream.Stream;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * V7/H2 — la INTERFAZ embebida en el `.mod` hace la ida y vuelta byte a byte:
 * bytes del `.mod` → {@code ModuleInterface.fromBytes} → {@code toBytes} → los mismos bytes.
 *
 * <p>Es la línea base ANTES de `bpi 8` (`field`, `final`, estáticos, `ctor`×N): cualquier
 * directiva que se añada tiene que sobrevivir a leer-y-escribir, y cualquier cosa que el
 * lector tolere pero el escritor no reproduzca (o al revés) sale aquí. Se prueba sobre
 * la stdlib entera compilada (28 módulos, con clases, sobrecargas, properties, eventos,
 * constantes estáticas y layouts), no sobre un módulo de juguete.
 *
 * <p>Y sobre UN módulo de juguete además: la stdlib no tiene ni un método ni una property
 * estáticos de clase (sólo `const`) ni una clase con dos constructores, así que sin él las directivas
 * `staticmethod`/`staticprop`/`ctor+` (V7/H2, #492-B y #509) pasarían por aquí sin ejercitarse — verde falso.
 */
class InterfazIdaYVueltaTest {

    static Stream<Path> modulos() {
        File std = new File("../bpstdlib");
        File[] mods = std.listFiles((d, n) -> n.endsWith(".mod"));
        Assumptions.assumeTrue(mods != null && mods.length > 0, "hace falta ../bpstdlib/*.mod (compila la stdlib)");
        return Stream.of(mods).map(File::toPath).sorted();
    }

    /** La sección interface tal como la saca el compilador de un .mod (método privado de Main:
     *  se llama por reflexión para no abrir la API sólo por el test). */
    private static byte[] seccionInterface(Path mod) throws Exception {
        Method m = Main.class.getDeclaredMethod("extractInterfaceSection", Path.class);
        m.setAccessible(true);
        return (byte[]) m.invoke(null, mod);
    }

    @ParameterizedTest(name = "{0}")
    @MethodSource("modulos")
    @DisplayName("leer la interfaz de un .mod y volver a escribirla da los mismos bytes")
    void idaYVuelta(Path mod) throws Exception {
        idaYVueltaDe(mod);
    }

    /** V7/H2 (#492-B) — lo que la stdlib no tiene: método estático, sobrecarga estática,
     *  estático protegido, property estática (pública y protegida), y un estático que
     *  devuelve la propia clase. */
    private static final String ESTATICOS =
        "module Juguete\n"
      + "  public class Base\n"
      + "    var n: integer := 0\n"
      + "    public const Base.MAX: integer := 3\n"
      + "    public property Base.total: integer := 0\n"
      + "    property Base.prot: long := 0L\n"
      + "    public function Base()\n"
      + "      this.n := 1\n"
      + "    end Base\n"
      + "    public function Base(v: integer)\n"
      + "      this.n := v\n"
      + "    end Base\n"
      + "    public function Base(o: Base)\n"
      + "      this.n := o.n\n"
      + "    end Base\n"
      + "    public function Base.cuantos(): integer\n"
      + "      return 7\n"
      + "    end Base.cuantos\n"
      + "    public function Base.cuantos(v: integer): integer\n"
      + "      return v\n"
      + "    end Base.cuantos\n"
      + "    function Base.protegido(): integer\n"
      + "      return 42\n"
      + "    end Base.protegido\n"
      + "    public function Base.crea(): Base\n"
      + "      return Base()\n"
      + "    end Base.crea\n"
      + "    public function valor(): integer\n"
      + "      return this.n\n"
      + "    end valor\n"
      + "  end Base\n"
      + "  public function main()\n"
      + "    print Base.cuantos()\n"
      + "  end main\n"
      + "end Juguete\n";

    @Test
    @DisplayName("staticmethod/staticprop (que la stdlib no tiene) hacen la ida y vuelta")
    void estaticosIdaYVuelta(@TempDir Path tmp) throws Exception {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve("Juguete.bp"), ESTATICOS.getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = "Juguete";
        b.dependencies.add(new File("../bpstdlib").getAbsolutePath());
        assertTrue(Main.buildProject(b, "mivm", false), "el módulo de juguete debe compilar");
        Path mod = out.resolve("Juguete.mod");
        String texto = new String(seccionInterface(mod), StandardCharsets.UTF_8);
        // Que de verdad estén (si no, la ida y vuelta de abajo no prueba nada):
        for (String linea : new String[]{
                "staticmethod cuantos():integer public",
                "staticmethod cuantos(v:integer):integer public",
                "staticmethod protegido():integer",
                "staticmethod crea():Base public",
                "staticprop total:integer public",
                "staticprop prot:long",
                "staticconst MAX:integer=3 public",
                "ctor()",
                "ctor+(v:integer)",
                "ctor+(o:Base)",
                "method valor():integer public"}) {
            assertTrue(texto.contains(linea), "falta «" + linea + "» en la interfaz:\n" + texto);
        }
        idaYVueltaDe(mod);
    }

    /** V7/H2 (#523) — bpi 9: con `--formato=9` la interfaz NO publica ningún número de forma
     *  (`layout`, `field … slot n`, `event … recv n`): las ranuras son del cargador. Se afirma el texto
     *  (campo, evento, clase abierta) y que hace la ida y vuelta; y que el lector deja -1 donde no hay número. */
    private static final String CON_EVENTO =
        "module Juguete\n"
      + "  import Core\n"
      + "  public class Base\n"
      + "    var n: integer\n"
      + "    var owner hijo: Base\n"
      + "    event cambio(x: integer)\n"
      + "    public function Base(v: integer)\n"
      + "      this.n := v\n"
      + "    end Base\n"
      + "    public function valor(): integer\n"
      + "      return this.n\n"
      + "    end valor\n"
      + "  end Base\n"
      + "  public class Hija extends Core.Exception\n"
      + "    var q: integer\n"
      + "    public function Hija()\n"
      + "      super(\"h\")\n"
      + "    end Hija\n"
      + "  end Hija\n"
      + "  public function main()\n"
      + "    print Base(1).valor()\n"
      + "  end main\n"
      + "end Juguete\n";

    @Test
    @DisplayName("bpi 9 (--formato=9): sin layout, sin slot, sin recv — y la ida y vuelta; el lector deja -1")
    void bpi9SinNumeros(@TempDir Path tmp) throws Exception {
        int antes = Main.FORMATO; Main.FORMATO = 9;
        try {
            Path src = Files.createDirectories(tmp.resolve("src"));
            Path out = Files.createDirectories(tmp.resolve("out"));
            Files.write(src.resolve("Juguete.bp"), CON_EVENTO.getBytes(StandardCharsets.UTF_8));
            BpBuild b = new BpBuild();
            b.projectDir = tmp.toString(); b.sourceDir = src.toString(); b.outDir = out.toString(); b.main = "Juguete";
            b.version = 7;
            b.dependencies.add(new File("../bpstdlib").getAbsolutePath());
            assertTrue(Main.buildProject(b, "mivm", false), "el módulo de juguete debe compilar en MOD9");
            Path mod = out.resolve("Juguete.mod");
            String texto = new String(seccionInterface(mod), StandardCharsets.UTF_8).replace("\r\n", "\n");
            assertTrue(texto.startsWith("bpi 9\n"), texto);
            for (String linea : new String[]{ "field n:integer\n", "field hijo:Base owner\n", "event cambio(x:integer)\n",
                                              "class Hija public extends Core.Exception\n", "field q:integer\n" })
                assertTrue(texto.contains(linea), "falta «" + linea.trim() + "» en:\n" + texto);
            for (String no : new String[]{ "layout", " slot ", " recv " })
                assertTrue(!texto.contains(no), "bpi 9 no lleva «" + no.trim() + "»:\n" + texto);
            idaYVueltaDe(mod);
            ModuleInterface iface = ModuleInterface.fromBytes(seccionInterface(mod), mod.toString());
            ModuleInterface.ClassSig base = null;
            for (ModuleInterface.ClassSig c : iface.classes) if (c.name.equals("Base")) base = c;
            assertTrue(base != null, "Base en la interfaz");
            assertEquals(-1, base.binaryNumFields, "sin layout");
            assertEquals(-1, base.fields.get(0).slot, "sin slot");
            assertEquals(-1, base.events.get(0).recvSlot, "sin recv");
        } finally { Main.FORMATO = antes; }
    }

    private static void idaYVueltaDe(Path mod) throws Exception {
        byte[] original = seccionInterface(mod);
        assertTrue(original != null && original.length > 0, mod + ": el .mod lleva sección interface");
        ModuleInterface iface = ModuleInterface.fromBytes(original, mod.toString());
        // V7/H2 (#523, V11) — el escritor emite la version de Main.FORMATO (9 por defecto desde el
        // 17-sep); la ida y vuelta se hace en la version que el .mod TRAE (bpi 8 en un MOD7/8,
        // bpi 9 en un MOD9), no en la del compilador de hoy.
        int formatoAntes = Main.FORMATO;
        Main.FORMATO = new String(original, 0, Math.min(6, original.length), StandardCharsets.UTF_8).startsWith("bpi 9") ? 9 : 8;
        byte[] reescrita;
        try { reescrita = iface.toBytes(); } finally { Main.FORMATO = formatoAntes; }
        // V7/H2 (MOD8) - el escritor emite LF fijo desde el 15-sep; un .mod escrito antes
        // lleva CRLF (Windows). La ida y vuelta se compara sin el separador de linea.
        String a = new String(original, StandardCharsets.UTF_8).replace("\r\n", "\n");
        String b = new String(reescrita, StandardCharsets.UTF_8);
        assertEquals(a, b, mod.getFileName() + ": la interfaz reescrita difiere de la embebida (¿una directiva que el lector"
                + " acepta y el escritor no reproduce, o al revés?)");
    }
}
