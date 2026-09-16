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
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V7/H2 (#523) — EL EMISOR EN MOD9 (F3a): lo que el compilador escribe con `--formato=9`
 * para una librería (clase cerrada con campos, property, sobrecargas y función), un
 * consumidor que la usa y una subclase de OTRO módulo (abierta). Se afirma lo que el
 * cargador va a leer — refs por (clase estática del receptor, clave con retorno),
 * la subtabla CLASES con lo propio de cada clase, la abierta sin descriptor — y que
 * el módulo NO lleva ningún marcador `Cls#clave#slot`.
 */
class Mod9EmisorTest {

    private static final String LIB = String.join("\n",
        "module Lib", "import Core",
        "public enum Color", "  ROJO", "  VERDE", "end Color",
        "public class Base",
        "  var n: integer", "  private var s: string", "  public property p: integer",
        "  public function Base(n: integer)", "    this.n := n", "    this.s := \"s\"", "    this.p := n * 10", "  end Base",
        "  public function k(): string", "    return \"k\" + this.n", "  end k",
        "  public function m(v: integer): integer", "    return v + 1", "  end m",
        "  public function m(t: string): string", "    return t + \"!\"", "  end m",
        "  public function suma(o: Base): integer", "    return this.n + o.n", "  end suma",
        "end Base",
        "public function f(): integer", "  return 7", "end f",
        "end Lib", "");
    private static final String USA = String.join("\n",
        "module Usa", "import Core", "import Lib",
        "public function main()",
        "  var b: Lib.Base := Lib.Base(3)",
        "  print b.k()", "  print b.m(4)", "  print b.m(\"x\")",
        "  var c: Lib.Base := Lib.Base(5)", "  print b.suma(c)", "  print Lib.f()",
        "  b.p := 8", "  print b.p",
        "end main", "end Usa", "");
    private static final String EXT = String.join("\n",
        "module Ext", "import Core", "import Lib",
        "public class Hija extends Lib.Base",
        "  var q: integer",
        "  public function Hija(n: integer)", "    super(n)", "    this.q := n * 2", "  end Hija",
        "  public function k(): string", "    return \"hija/\" + super.k() + \"/\" + this.n + \"/\" + this.q", "  end k",
        "  public function propio(): integer", "    return this.q + this.n", "  end propio",
        "end Hija",
        "public function main()",
        "  var h: Hija := Hija(3)", "  print h.k()", "  print h.propio()", "  print h.m(4)",
        "  var b: Lib.Base := h", "  print b.k()",
        "end main", "end Ext", "");

    private int formatoAntes;
    @BeforeEach void mod9() { formatoAntes = Main.FORMATO; Main.FORMATO = 9; }
    @AfterEach  void restaurar() { Main.FORMATO = formatoAntes; }

    private static Path compila(Path tmp, String main, String[]... fuentes) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        for (String[] f : fuentes) Files.write(src.resolve(f[0] + ".bp"), f[1].getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString(); b.sourceDir = src.toString(); b.outDir = out.toString(); b.main = main;
        b.version = 7;
        File std = new File("../bpstdlib");
        Assumptions.assumeTrue(new File(std, "Core.mod").isFile(), "hace falta ../bpstdlib/Core.mod");
        b.dependencies.add(std.getAbsolutePath());
        assertTrue(Main.buildProject(b, "mivm", false), "el proyecto tiene que compilar");
        return out;
    }

    /** Las 7 secciones tal como las corta la cabecera v9. */
    private static byte[][] secciones(byte[] mod) {
        ByteBuffer b = ByteBuffer.wrap(mod);
        int[] h = new int[11];
        for (int i = 0; i < 11; i++) h[i] = b.getInt();
        int off = ModFormat.HEADER_SIZE_V9;
        int[] tam = { h[6], h[3], h[4], h[7], h[8], h[1], h[5] };
        byte[][] out = new byte[7][];
        for (int i = 0; i < 7; i++) { out[i] = new byte[tam[i]]; System.arraycopy(mod, off, out[i], 0, tam[i]); off += tam[i]; }
        return out;
    }
    private static TablasMod9.Clase clase(List<TablasMod9.Clase> cs, String n) {
        for (TablasMod9.Clase c : cs) if (c.name.equals(n)) return c;
        fail("no esta la clase " + n); return null;
    }
    private static List<String> claves(List<TablasMod9.Metodo> ms) { List<String> l = new ArrayList<>(); for (TablasMod9.Metodo m : ms) l.add(m.clave); return l; }
    private static List<String> refs(List<TablasMod9.Ref> rs) { List<String> l = new ArrayList<>(); for (TablasMod9.Ref r : rs) l.add(r.toString()); return l; }

    @Test @DisplayName("el dueño: MOD9, la subtabla CLASES con lo propio (firma con retorno, property = accesores + backing privado) y ni un marcador")
    void dueno(@TempDir Path tmp) throws IOException {
        Path out = compila(tmp, "Usa", new String[]{"Lib", LIB}, new String[]{"Usa", USA});
        byte[] lib = Files.readAllBytes(out.resolve("Lib.mod"));
        assertEquals(ModFormat.MAGIC_NUMBER_V9, ByteBuffer.wrap(lib).getInt());
        List<TablasMod9.Clase> clases = TablasMod9.leerClases(secciones(lib)[2]);
        TablasMod9.Clase base = clase(clases, "Base");
        assertFalse(base.abierta); assertEquals(TablasMod9.DEP_LOCAL, base.parentDepIdx); assertEquals("Object", base.parentName);
        assertEquals(java.util.Arrays.asList("getP()i", "setP(i)V", "k()s", "m(i)i", "m(s)s", "suma(LLib/Base;)i"), claves(base.metodos),
                "solo lo PROPIO, en orden de ranura, con la firma completa tambien en la cabeza m(i)i");
        assertEquals(2, base.metodos.get(0).slot, "los accesores van detras de toString/compareTo");
        assertEquals("n:i", base.campos.get(0).clave); assertFalse(base.campos.get(0).privado);
        assertEquals("s:s", base.campos.get(1).clave); assertTrue(base.campos.get(1).privado);
        assertEquals("p:i", base.campos.get(2).clave); assertTrue(base.campos.get(2).privado, "el backing de la property es de la clase");
        String bytes = new String(lib, StandardCharsets.ISO_8859_1);
        assertFalse(bytes.matches("(?s).*Base#[A-Za-z_$]+#[0-9]+.*"), "sin marcadores Cls#clave#slot");
        assertTrue(TablasMod9.leerRefs(secciones(lib)[1]).isEmpty(), "Lib no pide nada por ref: sus clases son cerradas");
    }

    @Test @DisplayName("el consumidor: cada llamada a la clase ajena es una ref (clase estatica, clave con retorno); los accesores de property tambien")
    void consumidor(@TempDir Path tmp) throws IOException {
        Path out = compila(tmp, "Usa", new String[]{"Lib", LIB}, new String[]{"Usa", USA});
        byte[] usa = Files.readAllBytes(out.resolve("Usa.mod"));
        List<String> rs = refs(TablasMod9.leerRefs(secciones(usa)[1]));
        assertTrue(rs.contains("MET Lib.Base.k()s [dep1]"), rs.toString());
        assertTrue(rs.contains("MET Lib.Base.m(i)i [dep1]") && rs.contains("MET Lib.Base.m(s)s [dep1]"), "las DOS sobrecargas con su firma: " + rs);
        assertTrue(rs.contains("MET Lib.Base.suma(LLib/Base;)i [dep1]"), rs.toString());
        assertTrue(rs.contains("MET Lib.Base.getP()i [dep1]") && rs.contains("MET Lib.Base.setP(i)V [dep1]"), rs.toString());
        assertEquals(6, rs.size(), "y nada mas (dedupe): " + rs);
        String bytes = new String(usa, StandardCharsets.ISO_8859_1);
        assertFalse(bytes.contains("Base#"), "el consumidor ya no pide marcadores");
    }

    @Test @DisplayName("la subclase de otro modulo es ABIERTA: sin descriptor, lo propio relativo, el override marcado, y hasta sus propios miembros van por ref")
    void abierta(@TempDir Path tmp) throws IOException {
        Path out = compila(tmp, "Ext", new String[]{"Lib", LIB}, new String[]{"Ext", EXT});
        byte[] ext = Files.readAllBytes(out.resolve("Ext.mod"));
        byte[][] sec = secciones(ext);
        TablasMod9.Clase hija = clase(TablasMod9.leerClases(sec[2]), "Hija");
        assertTrue(hija.abierta); assertTrue(hija.publica);
        assertEquals(0, hija.csOff, "sin descriptor: lo construye el cargador contra el Lib.Base real");
        assertEquals("Lib.Base", hija.parentName); assertTrue(hija.parentDepIdx >= 0, "el padre es una dep");
        assertEquals(java.util.Arrays.asList("k()s", "propio()i"), claves(hija.metodos));
        assertTrue(hija.metodos.get(0).override, "k()s sobreescribe la de Lib.Base: el cargador la casa por clave");
        assertFalse(hija.metodos.get(1).override);
        assertEquals(0, hija.metodos.get(0).slot); assertEquals(1, hija.metodos.get(1).slot);
        assertEquals("q:i", hija.campos.get(0).clave); assertEquals(0, hija.campos.get(0).slot, "relativo: el cargador rebasa");
        List<String> rs = refs(TablasMod9.leerRefs(sec[1]));
        assertTrue(rs.contains("CAMPO Hija.q:i [local]"), "su propio campo, por ref (R2): " + rs);
        assertTrue(rs.contains("MET Hija.propio()i [local]") && rs.contains("MET Hija.k()s [local]"), "sus propios metodos, por ref: " + rs);
        assertTrue(rs.contains("CLASE Hija [local]"), "el new de una abierta, por ref: " + rs);
        assertTrue(rs.contains("CAMPO Hija.n:i [local]"), "el campo protegido heredado (this.n): la ref lleva la clase ESTATICA del receptor (V10, EC-1), el cargador sube hasta Lib.Base: " + rs);
        assertFalse(rs.contains("CAMPO Lib.Base.n:i [dep1]"), "ya no por la declarante: " + rs);
        assertTrue(rs.contains("MET Hija.m(i)i [local]"), "h.m(4): la ref lleva la clase ESTATICA del receptor (Hija); el cargador sube hasta Lib.Base: " + rs);
        assertTrue(rs.contains("MET Lib.Base.k()s [dep1]"), "b.k() con b: Lib.Base: " + rs);
        assertTrue(rs.contains("SUBTIPO Hija.Lib.Base [local]"), "A1: `var b: Lib.Base := h` ASUME Hija<:Lib.Base y lo deja escrito (E10 si ya no): " + rs);
    }

    /** V7/H2 (#523, A1) — la ref SUBTIPO: cada upcast implícito hacia una clase EXTERNA deja la
     *  asunción escrita (una entrada por par, dedupe); hacia una local o hacia `Object`, ninguna. */
    @Test @DisplayName("A1: el upcast implicito (asignacion, argumento, return, elemento, tupla) emite UNA ref SUBTIPO por par; hacia Object o hacia una clase local, ninguna")
    void subtipo(@TempDir Path tmp) throws IOException {
        String lib = LIB.replace("end Base\npublic function f()",
            "end Base\npublic class Derived extends Base\n  public function Derived(n: integer)\n    super(n)\n  end Derived\nend Derived\npublic function f()");
        String usa = String.join("\n",
            "module Usa", "import Core", "import Lib",
            "public class Local", "  public property x: integer", "end Local",
            "public class LocalHija extends Local", "  var y: integer", "end LocalHija",
            "function toma(b: Lib.Base): integer", "  return b.m(1)", "end toma",
            "function devuelve(): Lib.Base", "  return Lib.Derived(2)", "end devuelve",
            "function par(): (Lib.Derived, integer)", "  return (Lib.Derived(4), 1)", "end par",
            "public function main()",
            "  var d: Lib.Derived := Lib.Derived(1)",
            "  var b: Lib.Base := d",                       // asignacion en la declaracion
            "  b := d",                                     // asignacion suelta
            "  print toma(d)",                              // argumento
            "  print devuelve().m(2)",                      // return (dentro de devuelve)
            "  var bb: Lib.Base := Lib.Base(9)",             // S == T: NO
            "  var arr: Lib.Base[] := [bb]", "  arr[0] := d",   // elemento de array
            "  var o: Object := d",                          // hacia Object: NO
            "  var l: Local := LocalHija()",                 // hacia una local: NO
            "  var e: Lib.Base", "  var n: integer",
            "  { e, n } := par()", "  print e.m(n)",         // tupla: elemento Derived a destino Base
            "  print b.k() + l.x + o.toString() + arr[0].k()",
            "end main", "end Usa", "");
        Path out = compila(tmp, "Usa", new String[]{"Lib", lib}, new String[]{"Usa", usa});
        List<String> rs = refs(TablasMod9.leerRefs(secciones(Files.readAllBytes(out.resolve("Usa.mod")))[1]));
        List<String> sub = new ArrayList<>();
        for (String r : rs) if (r.startsWith("SUBTIPO")) sub.add(r);
        assertEquals(java.util.Arrays.asList("SUBTIPO Lib.Derived.Lib.Base [dep1]"), sub,
            "un solo par (Derived, Base) por muchos sitios que lo asuman; ni Object ni Local: " + rs);
    }

    /** Cada sitio POR SEPARADO (el dedupe del test anterior no dice cuál de ellos emitió). */
    @Test @DisplayName("A1: cada sitio de upcast, solo, emite la SUBTIPO: declaracion, asignacion, argumento, return, elemento de array, tupla")
    void subtipoCadaSitio(@TempDir Path tmp) throws IOException {
        String lib = LIB.replace("end Base\npublic function f()",
            "end Base\npublic class Derived extends Base\n  public function Derived(n: integer)\n    super(n)\n  end Derived\nend Derived\npublic function f()");
        String[][] sitios = {
            { "declaracion", "  var b: Lib.Base := d", "  print b.k()" },
            { "asignacion",  "  var b: Lib.Base := Lib.Base(1)", "  b := d", "  print b.k()" },
            { "argumento",   "  print toma(d)" },
            { "return",      "  print devuelve().k()" },
            { "elemento",    "  var arr: Lib.Base[] := [Lib.Base(1)]", "  arr[0] := d", "  print arr[0].k()" },
            { "tupla",       "  var e: Lib.Base", "  var n: integer", "  { e, n } := par()", "  print e.k() + n" },
        };
        int i = 0;
        for (String[] sitio : sitios) {
            List<String> lineas = new ArrayList<>(java.util.Arrays.asList(
                "module Usa", "import Core", "import Lib",
                "function toma(b: Lib.Base): string", "  return b.k()", "end toma",
                "function devuelve(): Lib.Base", "  return Lib.Derived(2)", "end devuelve",
                "function par(): (Lib.Derived, integer)", "  return (Lib.Derived(4), 1)", "end par",
                "public function main()", "  var d: Lib.Derived := Lib.Derived(1)"));
            for (int k = 1; k < sitio.length; k++) lineas.add(sitio[k]);
            lineas.add("end main"); lineas.add("end Usa"); lineas.add("");
            Path out = compila(tmp.resolve("s" + (i++)), "Usa", new String[]{"Lib", lib}, new String[]{"Usa", String.join("\n", lineas)});
            List<String> rs = refs(TablasMod9.leerRefs(secciones(Files.readAllBytes(out.resolve("Usa.mod")))[1]));
            assertTrue(rs.contains("SUBTIPO Lib.Derived.Lib.Base [dep1]"), sitio[0] + ": sin SUBTIPO: " + rs);
        }
    }

    @Test @DisplayName("en formato 8 (el de hoy) el mismo fuente sale MOD8 con marcadores, como siempre")
    void formato8(@TempDir Path tmp) throws IOException {
        Main.FORMATO = 8;
        Path out = compila(tmp, "Usa", new String[]{"Lib", LIB}, new String[]{"Usa", USA});
        byte[] lib = Files.readAllBytes(out.resolve("Lib.mod"));
        assertEquals(ModFormat.MAGIC_NUMBER_V8, ByteBuffer.wrap(lib).getInt());
        assertTrue(new String(lib, StandardCharsets.ISO_8859_1).contains("Base#k#"), "MOD8 sigue llevando sus marcadores");
    }
}
