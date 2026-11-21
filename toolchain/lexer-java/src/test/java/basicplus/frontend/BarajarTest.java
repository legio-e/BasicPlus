package basicplus.frontend;

import edu.bpgenvm.bytecode.ModFormat;
import edu.bpgenvm.bytecode.TablasMod9;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V7/H2 (#523, V1) — EL GUARDIÁN: `--barajar=<semilla>` permuta las ranuras propias de cada
 * clase de la librería y `--relleno=<n>` mete n properties privadas por clase. Un consumidor
 * MOD9 no puede llevar horneado ningún número que dependa de la forma de una clase ajena, así
 * que su `.mod` compilado contra Lib(semilla k) tiene que ser BYTE-IDÉNTICO al compilado
 * contra Lib(0) en todas las secciones menos la de imports (que lleva el sello de la
 * dependencia). La prueba dinámica (ejecutar App(0) contra Lib(k) en las dos VMs) es el
 * bloque B de `compat/mezcla.py`; ésta es la estática, y comprueba el ARTEFACTO: que las
 * libs barajadas de verdad difieren (un flag que no hace nada daría un verde falso).
 */
class BarajarTest {

    private static final String LIB = String.join("\n",
        "module Lib", "import Core",
        "public class Base",
        "  var n: integer", "  private var s: string", "  public property p: integer",
        "  public function Base(n: integer)", "    this.n := n", "    this.s := \"s\"", "    this.p := n * 10", "  end Base",
        "  public function k(): string", "    return \"k\" + this.n", "  end k",
        "  public function m(v: integer): integer", "    return v + 1", "  end m",
        "  public function suma(o: Base): integer", "    return this.n + o.n", "  end suma",
        "  public function z(): integer", "    return 99", "  end z",
        "end Base",
        "public class Derived extends Base",
        "  var d: integer",
        "  public function Derived(n: integer)", "    super(n)", "    this.d := n + 100", "  end Derived",
        "  public function dd(): integer", "    return this.d", "  end dd",
        "end Derived",
        "end Lib", "");
    private static final String USA = String.join("\n",
        "module Usa", "import Core", "import Lib",
        "public class Hija extends Lib.Base",
        "  var q: integer",
        "  public function Hija(n: integer)", "    super(n)", "    this.q := n * 2", "  end Hija",
        "  public function k(): string", "    return \"hija/\" + super.k() + \"/\" + this.q", "  end k",
        "end Hija",
        "public function main()",
        "  var b: Lib.Base := Lib.Base(3)", "  print b.k()", "  print b.m(4)", "  b.p := 8", "  print b.p",
        "  var d: Lib.Derived := Lib.Derived(1)", "  print d.dd()", "  var bd: Lib.Base := d", "  print bd.k()",
        "  var h: Hija := Hija(2)", "  print h.k()", "  print h.z()",
        "end main", "end Usa", "");

    private int formatoAntes, barajarAntes, rellenoAntes;
    @BeforeEach void mod9() { formatoAntes = Main.FORMATO; barajarAntes = Main.BARAJAR; rellenoAntes = Main.RELLENO; Main.FORMATO = 9; }
    @AfterEach  void restaurar() { Main.FORMATO = formatoAntes; Main.BARAJAR = barajarAntes; Main.RELLENO = rellenoAntes; }

    /** Compila Lib con (semilla, relleno) y Usa contra ella — Usa SIEMPRE con semilla 0 y sin relleno. */
    private static Path compila(Path tmp, int semilla, int relleno) throws IOException {
        Path lib = Files.createDirectories(tmp.resolve("lib_" + semilla + "_" + relleno));
        Main.BARAJAR = semilla; Main.RELLENO = relleno;
        construye(lib, "Lib", new String[]{"Lib", LIB});
        Main.BARAJAR = 0; Main.RELLENO = 0;
        Path usa = Files.createDirectories(tmp.resolve("usa_" + semilla + "_" + relleno));
        Files.copy(lib.resolve("out").resolve("Lib.mod"), Files.createDirectories(usa.resolve("out")).resolve("Lib.mod"));
        construye(usa, "Usa", new String[]{"Usa", USA});
        return tmp;
    }
    private static void construye(Path dir, String main, String[]... fuentes) throws IOException {
        Path src = Files.createDirectories(dir.resolve("src"));
        Path out = Files.createDirectories(dir.resolve("out"));
        for (String[] f : fuentes) Files.write(src.resolve(f[0] + ".bp"), f[1].getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = dir.toString(); b.sourceDir = src.toString(); b.outDir = out.toString(); b.main = main;
        b.version = 7;
        File std = new File("../../bpstdlib");
        Assumptions.assumeTrue(new File(std, "Core.mod").isFile(), "hace falta ../../bpstdlib/Core.mod");
        b.dependencies.add(std.getAbsolutePath());
        b.dependencies.add(out.toString());   // la Lib copiada al out de Usa
        assertTrue(Main.buildProject(b, "mivm", false), main + " tiene que compilar (semilla " + Main.BARAJAR + ", relleno " + Main.RELLENO + ")");
    }

    /** Orden de secciones en el fichero: 0 lib, 1 imports, 2 exports (con la 4.5 CLASES), 3 iface, 4 native, 5 data, 6 code.
     *  La 3 (la INTERFAZ embebida de Usa) llevaba en bpi 8 `layout 5 10 …` y `field q slot 4` de la clase abierta Hija:
     *  números que dependían de la forma de Lib.Base — este test los cazó el 16-sep, y la «bpi 9» dejó de publicarlos en MOD9.
     *  Se compara también (-1 = ninguna excluida). */
    static final int SECCION_IFACE = -1;   // bpi 9 hecha (16-sep): la interfaz ya no lleva números; se compara también

    private static byte[][] secciones(byte[] mod) {
        ByteBuffer b = ByteBuffer.wrap(mod);
        assertEquals(ModFormat.MAGIC_NUMBER_V9, b.getInt(0));
        int[] h = new int[11];
        for (int i = 0; i < 11; i++) h[i] = b.getInt();
        int off = ModFormat.HEADER_SIZE_V9;
        int[] tam = { h[6], h[3], h[4], h[7], h[8], h[1], h[5] };
        byte[][] out = new byte[7][];
        for (int i = 0; i < 7; i++) { out[i] = new byte[tam[i]]; System.arraycopy(mod, off, out[i], 0, tam[i]); off += tam[i]; }
        return out;
    }
    private static TablasMod9.Clase clase(byte[] lib, String n) throws IOException {
        for (TablasMod9.Clase c : TablasMod9.leerClases(secciones(lib)[2])) if (c.name.equals(n)) return c;
        fail("no esta " + n); return null;
    }
    private static List<Integer> slots(TablasMod9.Clase c) { List<Integer> l = new ArrayList<>(); for (TablasMod9.Metodo m : c.metodos) l.add(m.slot); return l; }
    private static List<String> claves(TablasMod9.Clase c) { List<String> l = new ArrayList<>(); for (TablasMod9.Metodo m : c.metodos) l.add(m.clave); return l; }

    @Test @DisplayName("barajar: las libs de semillas distintas DIFIEREN en ranuras (mismas claves), y el Usa compilado contra cada una es byte-identico salvo imports")
    void semillas(@TempDir Path tmp) throws IOException {
        byte[][] libs = new byte[4][], usas = new byte[4][];
        for (int k = 0; k < 4; k++) {
            compila(tmp, k, 0);
            libs[k] = Files.readAllBytes(tmp.resolve("lib_" + k + "_0/out/Lib.mod"));
            usas[k] = Files.readAllBytes(tmp.resolve("usa_" + k + "_0/out/Usa.mod"));
        }
        TablasMod9.Clase base0 = clase(libs[0], "Base");
        int distintas = 0;
        for (int k = 1; k < 4; k++) {
            TablasMod9.Clase basek = clase(libs[k], "Base");
            assertEquals(new java.util.TreeSet<>(claves(base0)), new java.util.TreeSet<>(claves(basek)), "las mismas claves");
            java.util.Map<String, Integer> s0 = new java.util.HashMap<>(), sk = new java.util.HashMap<>();
            for (TablasMod9.Metodo m : base0.metodos) s0.put(m.clave, m.slot);
            for (TablasMod9.Metodo m : basek.metodos) sk.put(m.clave, m.slot);
            if (!s0.equals(sk)) distintas++;
            assertFalse(Arrays.equals(libs[0], libs[k]), "Lib(" + k + ") identica a Lib(0): el flag no ha hecho nada");
        }
        assertTrue(distintas >= 2, "al menos dos de tres semillas mueven alguna ranura de Base: " + distintas);
        byte[][] u0 = secciones(usas[0]);
        for (int k = 1; k < 4; k++) {
            byte[][] uk = secciones(usas[k]);
            for (int s = 0; s < 7; s++) {
                if (s == 1) continue;   // imports: lleva el sello (gen, ifaceCrc) de Lib, que cambia con la forma
                if (s == SECCION_IFACE) continue;   // ver SECCION_IFACE
                assertArrayEquals(u0[s], uk[s], "Usa contra Lib(" + k + "): la seccion " + s + " difiere de Usa contra Lib(0) — hay un numero horneado");
            }
        }
    }

    @Test @DisplayName("relleno=3: tres properties privadas por clase = 3 campos y 6 ranuras de vtable mas en Base; Usa sigue byte-identico salvo imports")
    void relleno(@TempDir Path tmp) throws IOException {
        compila(tmp, 0, 0); compila(tmp, 0, 3);
        TablasMod9.Clase sin = clase(Files.readAllBytes(tmp.resolve("lib_0_0/out/Lib.mod")), "Base");
        TablasMod9.Clase con = clase(Files.readAllBytes(tmp.resolve("lib_0_3/out/Lib.mod")), "Base");
        assertEquals(sin.campos.size() + 3, con.campos.size(), "tres backings");
        assertEquals(sin.metodos.size() + 6, con.metodos.size(), "tres getters y tres setters");
        assertTrue(claves(con).contains("getRelleno__0Base()i") && claves(con).contains("setRelleno__2Base(i)V"), claves(con).toString());
        for (TablasMod9.Campo c : con.campos) if (c.clave.startsWith("relleno__")) assertTrue(c.privado, "el relleno es privado: " + c.clave);
        assertTrue(slots(con).get(slots(con).size() - 1) > slots(sin).get(slots(sin).size() - 1), "las ranuras de lo de siempre se han corrido");
        byte[][] u0 = secciones(Files.readAllBytes(tmp.resolve("usa_0_0/out/Usa.mod")));
        byte[][] u3 = secciones(Files.readAllBytes(tmp.resolve("usa_0_3/out/Usa.mod")));
        for (int s = 0; s < 7; s++) if (s != 1 && s != SECCION_IFACE) assertArrayEquals(u0[s], u3[s], "seccion " + s + " de Usa difiere con el relleno");
    }

    @Test @DisplayName("semilla 0 = identidad: el mismo fuente da el mismo .mod que sin flag")
    void identidad(@TempDir Path tmp) throws IOException {
        Path a = Files.createDirectories(tmp.resolve("a")), b = Files.createDirectories(tmp.resolve("b"));
        Main.BARAJAR = 0; Main.RELLENO = 0; construye(a, "Lib", new String[]{"Lib", LIB});
        construye(b, "Lib", new String[]{"Lib", LIB});
        assertArrayEquals(Files.readAllBytes(a.resolve("out/Lib.mod")), Files.readAllBytes(b.resolve("out/Lib.mod")));
    }
}
