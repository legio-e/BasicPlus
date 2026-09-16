package basicplus.frontend;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V7/H2 (#523) — EL AOT EN MOD9, fase 1. Una `native` llama a un método de clase por
 * la vtable con `call_method_i32(vm, this, slot, …)` y el `slot` lo hornea el
 * compilador (`ClassSymbol.slotOf`). En MOD9 ese número sólo es del compilador para
 * una clase CERRADA de su propio módulo: el de una EXTERNA es el de la versión contra
 * la que se compiló, y el de una ABIERTA lo asigna el cargador. Se afirma que el AOT
 * lo rechaza NOMBRANDO la clase (error en la función de módulo, aviso + interpretado
 * en el método aplanado), que lo cerrado local sigue acelerándose, y que en MOD8 nada
 * cambia.
 */
class AotMod9Test {

    private static final String LIB = String.join("\n",
        "module Lib", "import Core",
        "public class Base",
        "  var n: integer",
        "  public function Base(n: integer)", "    this.n := n", "  end Base",
        "  public function m(v: integer): integer", "    return v + this.n", "  end m",
        "end Base",
        "end Lib", "");
    /** La función de módulo `native` llama a `Lib.Base.m` — externa. */
    private static final String USA_EXTERNA = String.join("\n",
        "module Usa", "import Core", "import Lib",
        "native function rapida(b: Lib.Base): integer", "  return b.m(4)", "end rapida",
        "public function main()", "  print rapida(Lib.Base(3))", "end main",
        "end Usa", "");
    /** La misma `native`, pero sobre una clase CERRADA del propio módulo. */
    private static final String USA_LOCAL = String.join("\n",
        "module Usa", "import Core",
        "public class Caja",
        "  var n: integer",
        "  public function Caja(n: integer)", "    this.n := n", "  end Caja",
        "  public function m(v: integer): integer", "    return v + this.n", "  end m",
        "end Caja",
        "native function rapida(c: Caja): integer", "  return c.m(4)", "end rapida",
        "public function main()", "  print rapida(Caja(3))", "end main",
        "end Usa", "");
    /** Un método `native` APLANADO de una clase abierta que llama a otro método suyo. La
     *  `native` de módulo está para que el compilador valide el AOT (sin ninguna, los
     *  métodos no se miran hasta construir el `.mdn`). */
    private static final String EXT_ABIERTA = String.join("\n",
        "module Ext", "import Core", "import Lib",
        "public class Hija extends Lib.Base",
        "  public function Hija(n: integer)", "    super(n)", "  end Hija",
        "  public function propio(): integer", "    return this.n * 2", "  end propio",
        "  public native function doble(): integer", "    return this.propio() + 1", "  end doble",
        "end Hija",
        "native function suma(a: integer, b: integer): integer", "  return a + b", "end suma",
        "public function main()", "  print Hija(3).doble()", "  print suma(1, 2)", "end main",
        "end Ext", "");

    /** Una `native` que llama a una FUNCIÓN de otro módulo: el nombre que pide al puente. */
    private static final String LIB_F = String.join("\n",
        "module Lib", "import Core",
        "public function f(x: integer): integer", "  return x + 1", "end f",
        "end Lib", "");
    private static final String USA_F = String.join("\n",
        "module Usa", "import Core", "import Lib",
        "native function rapida(x: integer): integer", "  return Lib.f(x)", "end rapida",
        "public function main()", "  print rapida(3)", "end main",
        "end Usa", "");

    private int formatoAntes;
    @BeforeEach void guarda() { formatoAntes = Main.FORMATO; }
    @AfterEach  void restaura() { Main.FORMATO = formatoAntes; }

    /** Compila el proyecto capturando stdout+stderr; devuelve {ok, texto}. */
    private static Object[] compila(Path tmp, String main, String[]... fuentes) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        for (String[] f : fuentes) Files.write(src.resolve(f[0] + ".bp"), f[1].getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString(); b.sourceDir = src.toString(); b.outDir = out.toString(); b.main = main;
        b.version = 7;
        File std = new File("../bpstdlib");
        Assumptions.assumeTrue(new File(std, "Core.mod").isFile(), "hace falta ../bpstdlib/Core.mod");
        b.dependencies.add(std.getAbsolutePath());
        PrintStream out0 = System.out, err0 = System.err;
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(buf, true, "UTF-8");
        boolean ok;
        try { System.setOut(ps); System.setErr(ps); ok = Main.buildProject(b, "mivm", false); }
        finally { System.setOut(out0); System.setErr(err0); }
        return new Object[]{ ok, new String(buf.toByteArray(), StandardCharsets.UTF_8) };
    }

    @Test @DisplayName("MOD9: una native que llama a un método de una clase EXTERNA no compila, y el error nombra la clase")
    void externaRechazada(@TempDir Path tmp) throws IOException {
        Main.FORMATO = 9;
        Object[] r = compila(tmp, "Usa", new String[]{"Lib", LIB}, new String[]{"Usa", USA_EXTERNA});
        assertFalse((Boolean) r[0], "no debe compilar: la ranura de Lib.Base.m la fija el cargador\n" + r[1]);
        String t = (String) r[1];
        assertTrue(t.contains("AOT (MOD9)") && t.contains("EXTERNA") && t.contains("'Base'") && t.contains("'m'"), t);
    }

    @Test @DisplayName("MOD9: la misma native sobre una clase CERRADA del propio módulo sigue compilando (se acelera)")
    void localCerradaSigue(@TempDir Path tmp) throws IOException {
        Main.FORMATO = 9;
        Object[] r = compila(tmp, "Usa", new String[]{"Usa", USA_LOCAL});
        assertTrue((Boolean) r[0], (String) r[1]);
        assertFalse(((String) r[1]).contains("AOT (MOD9)"), (String) r[1]);
    }

    @Test @DisplayName("MOD9: un método native aplanado de una clase ABIERTA se degrada a aviso y el módulo compila (corre interpretado)")
    void abiertaAviso(@TempDir Path tmp) throws IOException {
        Main.FORMATO = 9;
        Object[] r = compila(tmp, "Ext", new String[]{"Lib", LIB}, new String[]{"Ext", EXT_ABIERTA});
        assertTrue((Boolean) r[0], (String) r[1]);
        String t = (String) r[1];
        assertTrue(t.contains("aviso AOT") && t.contains("AOT (MOD9)") && t.contains("ABIERTA") && t.contains("'Hija'"), t);
    }

    /** El C que AotMain emite para Usa, tras compilar el proyecto (las deps se leen del out). */
    private static String emiteC(Path tmp, String... fuentesPlanas) throws Exception {
        String[][] fuentes = new String[fuentesPlanas.length / 2][];
        for (int i = 0; i < fuentes.length; i++) fuentes[i] = new String[]{ fuentesPlanas[2 * i], fuentesPlanas[2 * i + 1] };
        Object[] r = compila(tmp, "Usa", fuentes);
        assertTrue((Boolean) r[0], (String) r[1]);
        AotMain.AotResult ar = AotMain.emitAotC(tmp.resolve("src").resolve("Usa.bp"), tmp.resolve("out"), true);
        assertNotNull(ar.cFile, "Usa tiene native: debe emitir C");
        return new String(Files.readAllBytes(ar.cFile), StandardCharsets.UTF_8);
    }

    @Test @DisplayName("MOD9: la native pide la función ajena al puente con su firma (R4: Lib.f(i)i), como el .mod")
    void funcionAjenaConFirma(@TempDir Path tmp) throws Exception {
        Main.FORMATO = 9;
        String c = emiteC(tmp, "Lib", LIB_F, "Usa", USA_F);
        assertTrue(c.contains("_busca(vm, \"Lib.f(i)i\","), c);
        assertFalse(c.contains("_busca(vm, \"Lib.f\","), "el nombre pelado casaria con otra firma en silencio");
    }

    @Test @DisplayName("MOD9: la native construye una clase ajena por su factoría con firma (R4: Lib.__cls_new_Base(i))")
    void factoriaAjenaConFirma(@TempDir Path tmp) throws Exception {
        Main.FORMATO = 9;
        String usa = String.join("\n",
            "module Usa", "import Core", "import Lib",
            "native function crea(x: integer): Lib.Base", "  return Lib.Base(x)", "end crea",
            "public function main()", "  print crea(3).m(1)", "end main",
            "end Usa", "");
        String c = emiteC(tmp, "Lib", LIB, "Usa", usa);
        assertTrue(c.contains("_busca(vm, \"Lib.__cls_new_Base(i)\","), c);
    }

    @Test @DisplayName("el aviso del puente nombra el símbolo y a quién lo pedía; los acentos van en octal (\\x en C es glotón)")
    void avisoNombraYEscapaBien(@TempDir Path tmp) throws Exception {
        Main.FORMATO = 9;
        String c = emiteC(tmp, "Lib", LIB_F, "Usa", USA_F);
        assertTrue(c.contains("la native 'rapida' pide 'Lib.f(i)i'"), c);
        assertTrue(c.contains("m\\303\\263dulos"), "«módulos» en octal: " + c);
        assertFalse(c.contains("\\xc3\\xb3d"), "`\\xb3d` es un escape hex fuera de rango para gcc (se come la d)");
    }

    @Test @DisplayName("MOD8: la native pide la función ajena con el nombre de siempre (Lib.f)")
    void funcionAjenaMod8(@TempDir Path tmp) throws Exception {
        Main.FORMATO = 8;
        String c = emiteC(tmp, "Lib", LIB_F, "Usa", USA_F);
        assertTrue(c.contains("_busca(vm, \"Lib.f\","), c);
    }

    @Test @DisplayName("MOD8: nada cambia — la native sobre la clase externa compila como hasta hoy")
    void mod8Intacto(@TempDir Path tmp) throws IOException {
        Main.FORMATO = 8;
        Object[] r = compila(tmp, "Usa", new String[]{"Lib", LIB}, new String[]{"Usa", USA_EXTERNA});
        assertTrue((Boolean) r[0], (String) r[1]);
        assertFalse(((String) r[1]).contains("AOT (MOD9)"), (String) r[1]);
    }
}
