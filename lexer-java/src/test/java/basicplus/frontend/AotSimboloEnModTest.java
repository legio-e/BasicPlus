package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * El nombre con el que el AOT registra un thunk tiene que EXISTIR en el `.mod`.
 *
 * <h3>Por qué hace falta una prueba para esto</h3>
 *
 * El secuestro AOT se instala por nombre: {@code bpvm_aot_register_by_name(vm,
 * "Mod.func", thunk)} busca ese símbolo en el módulo cargado para sacar su
 * dirección. Si el nombre no casa, <b>no pasa nada malo</b>: el registro se
 * salta, la función corre interpretada y el programa da <b>el resultado
 * correcto</b>. Silencio absoluto — que es justamente el contrato del degrade.
 *
 * <h3>Lo que pasó el 24-ago-2026</h3>
 *
 * Un método {@code native} nunca llegó a acelerarse. El `.mod` lo exporta como
 * {@code Caja.doble}, y {@code MdnPack} reconstruía el nombre partiendo el
 * identificador de C ({@code thunk_NatV7_Caja_doble}) por el prefijo — o sea que
 * pedía {@code NatV7.Caja_doble}, con guion bajo. De {@code Caja_doble} no se
 * puede recuperar {@code Caja.doble}: un identificador BP también puede llevar
 * guiones bajos.
 *
 * <p>Sólo se vio <b>en placa y con el log encendido</b>:
 * <pre>MDN: skip 'NatV7.Caja_doble' rc=-2 (symbol no en .mod?)
 * MDN: 3/4 thunks registrados</pre>
 * La salida del programa era correcta, y el test de emisión pasaba: el emisor
 * generaba C impecable con el nombre equivocado.
 *
 * <h3>Por qué esta prueba SÍ lo caza</h3>
 *
 * Porque compara <b>dos artefactos</b>, y el fallo era un desacuerdo entre dos.
 * Los nombres salen de {@code AotCEmitter} y los símbolos del `.mod` los escribe
 * {@code MivmEmitter}: son dos caminos distintos que tienen que coincidir. Una
 * prueba que mirara sólo uno no puede ver nada — es la lección de
 * [[n-casos-del-mismo-sample-no-son-n-casos]] aplicada a artefactos.
 */
class AotSimboloEnModTest {

    /** Un módulo con las DOS formas: función de módulo y método de clase. La de
     *  módulo acertaba (sin punto que perder) y por eso el fallo pasó: 3 de 4. */
    private static final String SRC =
          "module AotSim\n"
        + "  native function libre(n: integer): integer\n"
        + "    return n + 1\n"
        + "  end libre\n"
        + "  public class Caja\n"
        + "    public property n: integer\n"
        + "    public function Caja(v: integer)\n"
        + "      this.n := v\n"
        + "    end Caja\n"
        + "    public native function doble(): integer\n"
        + "      return this.n * 2\n"
        + "    end doble\n"
        + "  end Caja\n"
        + "  function Main()\n"
        + "    print libre(1)\n"
        + "  end Main\n"
        + "end AotSim\n";

    /** Los nombres que el AOT va a pedirle al `.mod`, tal cual los emite el
     *  emisor en el registro / en el símbolo del `.mdn`. */
    private static List<String> nombresQueElAotRegistra(String csrc, String modulo) {
        List<String> out = new ArrayList<>();
        // En modo .mdn el nombre viaja en el __asm__ del thunk; en modo enlazado,
        // en la llamada a register_by_name. Se leen los dos por si divergen.
        for (String linea : csrc.split("\n")) {
            String t = linea.trim();
            if (t.startsWith("__asm__(\"thunk_" + modulo + "_")) {
                int a = t.indexOf('"') + 1, b = t.lastIndexOf('"');
                out.add(modulo + "." + t.substring(a + ("thunk_" + modulo + "_").length(), b));
            } else if (t.startsWith("bpvm_aot_register_by_name(vm, \"")) {
                int a = t.indexOf('"') + 1, b = t.indexOf('"', a);
                out.add(t.substring(a, b));
            }
        }
        return out;
    }

    private static String emitirC(Path dir, boolean modoMdn) throws Exception {
        Path bp = dir.resolve("AotSim.bp");
        Files.write(bp, SRC.getBytes(StandardCharsets.UTF_8));
        AotMain.AotResult r = AotMain.emitAotC(bp, dir, modoMdn);
        assertTrue(r.cFile != null, "el módulo tiene native: debe emitir C");
        return new String(Files.readAllBytes(r.cFile), StandardCharsets.UTF_8);
    }

    @Test
    @DisplayName("cada nombre que el AOT registra existe en el .mod (si no, corre interpretado en silencio)")
    void losNombresCasan(@TempDir Path tmp) throws Exception {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve("AotSim.bp"), SRC.getBytes(StandardCharsets.UTF_8));

        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = "AotSim";
        assertTrue(Main.buildProject(b, "mivm", false), "el módulo de la prueba debe compilar");

        Set<String> enElMod = ModSimbolos.conjunto(out.resolve("AotSim.mod"));

        for (boolean modoMdn : new boolean[]{true, false}) {
            Path d = Files.createDirectories(tmp.resolve("c" + modoMdn));
            List<String> pedidos = nombresQueElAotRegistra(emitirC(d, modoMdn), "AotSim");
            assertTrue(pedidos.size() >= 2,
                    "se esperaban al menos 2 nombres (la función de módulo y el método), "
                    + "salieron " + pedidos + ". Si sale sólo uno, el barrido ha dejado de "
                    + "descender a las clases.");
            for (String q : pedidos) {
                // El .mod guarda el símbolo SIN el prefijo de módulo.
                String pelado = q.startsWith("AotSim.") ? q.substring("AotSim.".length()) : q;
                assertTrue(enElMod.contains(pelado),
                        "el AOT va a pedir '" + q + "' y el .mod NO exporta '" + pelado + "'."
                        + "\n  Eso NO da error en ejecución: el thunk no se registra, la"
                        + " función corre interpretada y el resultado sale bien. Sólo se ve"
                        + " en el log de la placa (`MDN: skip ... rc=-2`)."
                        + "\n  modo " + (modoMdn ? ".mdn" : "enlazado")
                        + "\n  pedidos: " + pedidos
                        + "\n  candidatos en el .mod: "
                        + ModSimbolos.conPrefijo(enElMod, "Caja"));
            }
        }
    }
}
