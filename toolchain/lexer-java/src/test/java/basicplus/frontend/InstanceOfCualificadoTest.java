package basicplus.frontend;

import edu.bpgenvm.bytecode.ModFormat;
import edu.bpgenvm.bytecode.TablasMod9;
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
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V7/H2 (#523, D7) — `instanceof Mod.Cls`: el nombre cualificado que ya aceptaba un tipo
 * (`var b: Lib.Base`) vale también tras `instanceof`, y se resuelve por el mismo camino.
 * En MOD9 la clase ajena va por referencia (`INSTANCEOF_REF` + ref CLASE). Se afirma que
 * compila, que la ref está, y que un nombre que no existe lo dice.
 */
class InstanceOfCualificadoTest {

    private static final String LIB = String.join("\n",
        "module Lib", "import Core",
        "public class Base", "  var n: integer",
        "  public function Base(n: integer)", "    this.n := n", "  end Base",
        "end Base",
        "public class Derived extends Base",
        "  public function Derived(n: integer)", "    super(n)", "  end Derived",
        "end Derived",
        "end Lib", "");
    private static final String USA = String.join("\n",
        "module Usa", "import Core", "import Lib",
        "public function main()",
        "  var o: Object := Lib.Derived(1)",
        "  print o instanceof Lib.Base",
        "  print o instanceof Lib.Derived",
        "  var s: Object := \"x\"",
        "  print s instanceof Lib.Base",
        "end main", "end Usa", "");

    private int formatoAntes;
    @BeforeEach void mod9() { formatoAntes = Main.FORMATO; Main.FORMATO = 9; }
    @AfterEach  void restaurar() { Main.FORMATO = formatoAntes; }

    private static Object[] compila(Path tmp, String main, String[]... fuentes) throws IOException {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        for (String[] f : fuentes) Files.write(src.resolve(f[0] + ".bp"), f[1].getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString(); b.sourceDir = src.toString(); b.outDir = out.toString(); b.main = main;
        b.version = 7;
        File std = new File("../../bpstdlib");
        Assumptions.assumeTrue(new File(std, "Core.mod").isFile(), "hace falta ../../bpstdlib/Core.mod");
        b.dependencies.add(std.getAbsolutePath());
        PrintStream out0 = System.out, err0 = System.err;
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(buf, true, "UTF-8");
        boolean ok;
        try { System.setOut(ps); System.setErr(ps); ok = Main.buildProject(b, "mivm", false); }
        finally { System.setOut(out0); System.setErr(err0); }
        return new Object[]{ ok, new String(buf.toByteArray(), StandardCharsets.UTF_8), out };
    }

    private static byte[] seccionRefs(byte[] mod) {
        ByteBuffer b = ByteBuffer.wrap(mod);
        int[] h = new int[11];
        for (int i = 0; i < 11; i++) h[i] = b.getInt();
        int off = ModFormat.HEADER_SIZE_V9 + h[6];   // tras la primera sección (imports va la segunda)
        byte[] out = new byte[h[3]];
        System.arraycopy(mod, off, out, 0, h[3]);
        return out;
    }

    @Test @DisplayName("`o instanceof Lib.Base` compila y en MOD9 pide la clase por ref (CLASE Lib.Base [dep1])")
    void cualificado(@TempDir Path tmp) throws IOException {
        Object[] r = compila(tmp, "Usa", new String[]{"Lib", LIB}, new String[]{"Usa", USA});
        assertTrue((Boolean) r[0], (String) r[1]);
        byte[] usa = Files.readAllBytes(((Path) r[2]).resolve("Usa.mod"));
        assertEquals(ModFormat.MAGIC_NUMBER_V9, ByteBuffer.wrap(usa).getInt());
        List<String> refs = new ArrayList<>();
        for (TablasMod9.Ref x : TablasMod9.leerRefs(seccionRefs(usa))) refs.add(x.toString());
        assertTrue(refs.contains("CLASE Lib.Base [dep1]") && refs.contains("CLASE Lib.Derived [dep1]"), refs.toString());
    }

    @Test @DisplayName("un nombre cualificado que no existe lo dice, y el build falla")
    void noExiste(@TempDir Path tmp) throws IOException {
        String usa = USA.replace("o instanceof Lib.Base", "o instanceof Lib.Nadie");
        Object[] r = compila(tmp, "Usa", new String[]{"Lib", LIB}, new String[]{"Usa", usa});
        assertFalse((Boolean) r[0], (String) r[1]);
        assertTrue(((String) r[1]).contains("'Lib.Nadie' no encontrado"), (String) r[1]);
    }
}
