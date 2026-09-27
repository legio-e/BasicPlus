package basicplus.frontend;

import edu.bpgenvm.util.Json;
import edu.bpgenvm.vm.ModuleManager;
import edu.bpgenvm.vm.VirtualMachine;
import edu.bpgenvm.vm.debug.DebugController;
import edu.bpgenvm.vm.debug.DebugServer;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.junit.jupiter.api.io.TempDir;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

import static org.junit.jupiter.api.Assertions.*;

/**
 * #589 (V7, 27-sep) — lo que el depurador de miVM ENSEÑA de una cadena, por el wire de verdad: un programa
 * compilado aquí, ejecutado en miVM con su {@link DebugServer}, parado en un breakpoint y preguntado como lo
 * pregunta el IDE (LOCALS, MODULE_PROPERTIES, READ_STRING, INSPECT).
 *
 * <h3>Lo que se protege</h3>
 *
 * Una cadena tiene dos formas —un HANDLE del heap (lo que sale de una concatenación) o la referencia CRUDA de un
 * literal del bloque de datos— y el depurador de miVM sólo sabía leer la segunda: pasaba el handle como si fuera
 * una dirección y una cadena del heap salía «@40000004». Además, los locales salían con las comillas DOS veces
 * («""literal""»), la vista de properties leía la palabra ALTA de los 8 bytes de la referencia —la generación: con
 * 0, toda property string era «null»— y READ_STRING devolvía el texto entre comillas, cuando el de la VM-C (su
 * espejo) lo devuelve tal cual. Ninguna red ejecutaba el depurador de miVM contra un programa: por eso duró.
 *
 * <h3>Los casos</h3>
 *
 * El literal y la cadena del heap, en un local y en una property; la cadena VACÍA de las dos formas (no es
 * «null» ni «@…»); y una con comillas y un salto de línea, que el panel enseña escapados.
 */
class DepuradorCadenasTest {

    /** La línea 13 es el `print`: ahí están asignadas todas. Y NO es la última: los helpers de la
     *  concatenación que el compilador sintetiza detrás de `Main` no tienen línea propia y heredan la última
     *  del mapa, así que un breakpoint en ella para también DENTRO de `__strconcat` (#591, aparte). */
    private static final String FUENTE =
          "module DbgCad\n"                                   // 1
        + "  public property nombre: string\n"                // 2
        + "  public property vacia: string\n"                 // 3
        + "  public property literal: string\n"               // 4
        + "\n"                                                // 5
        + "  function Main()\n"                               // 6
        + "    nombre := \"mun\" + \"do\"\n"                  // 7  del heap
        + "    vacia := \"\" + \"\"\n"                        // 8  del heap, vacía
        + "    literal := \"prop\"\n"                         // 9  un literal
        + "    var lit: string := \"di \\\"hola\\\"\"\n"      // 10 un literal con comillas
        + "    var heap: string := \"ho\" + \"la\\n\" + nombre\n"   // 11 del heap, con un salto
        + "    var nada: string := \"\"\n"                    // 12 un literal vacío
        + "    print heap\n"                                  // 13
        + "    print lit\n"                                   // 14
        + "  end Main\n"
        + "end DbgCad\n";
    private static final int LINEA = 13;

    private static int puertoLibre() throws Exception {
        try (ServerSocket s = new ServerSocket(0)) { return s.getLocalPort(); }
    }

    /** Lee hasta un mensaje del tipo pedido (o un ERROR con ese id); lo demás —OUTPUT, RESUMED…— se salta. */
    private static Map<String, Object> espera(BufferedReader in, String tipo, long id) throws Exception {
        while (true) {
            String l = in.readLine();
            assertNotNull(l, "el servidor cerró la conexión esperando " + tipo);
            Map<String, Object> m = Json.parseFlatObject(l);
            String t = Json.getString(m, "type", "");
            if (t.equals(tipo) && (id < 0 || Json.getLong(m, "id", -1) == id)) return m;
            if (t.equals("ERROR") && Json.getLong(m, "id", -1) == id) return m;
        }
    }

    @Test
    @Timeout(60)
    @DisplayName("el depurador de miVM enseña las cadenas del heap y los literales, en locales y properties")
    void cadenasEnElDepurador(@TempDir Path tmp) throws Exception {
        Path src = Files.createDirectories(tmp.resolve("src"));
        Path out = Files.createDirectories(tmp.resolve("out"));
        Files.write(src.resolve("DbgCad.bp"), FUENTE.getBytes(StandardCharsets.UTF_8));
        BpBuild b = new BpBuild();
        b.projectDir = tmp.toString();
        b.sourceDir  = src.toString();
        b.outDir     = out.toString();
        b.main       = "DbgCad";
        assertTrue(Main.buildProject(b, "mivm", false), "debe compilar sin errores");

        VirtualMachine vm = new VirtualMachine();
        ModuleManager mm = new ModuleManager(vm);
        mm.setStdlibDir("../bpstdlib");          // el CWD de la prueba es lexer-java/
        vm.setModuleManager(mm);
        DebugController controller = new DebugController();
        vm.setDebugHook(controller.hook());
        vm.setDebugEventListener(controller::emitEvent);
        String mod = out.resolve("DbgCad.mod").toString();
        Throwable[] fallo = new Throwable[1];
        Thread programa = new Thread(() -> {
            try { mm.executeRootModule(mod, "DbgCad"); vm.run(); }
            catch (Throwable t) { fallo[0] = t; }
        }, "dbgcad");
        programa.setDaemon(true);

        int port = puertoLibre();
        try (DebugServer server = new DebugServer(vm, controller)) {
            server.start(port);
            try (Socket s = new Socket("localhost", port)) {
                assertTrue(server.awaitClient(5, TimeUnit.SECONDS), "el servidor debió aceptar la conexión");
                BufferedReader in = new BufferedReader(new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                PrintWriter o = new PrintWriter(new OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8), true);

                o.println("{\"type\":\"HELLO\",\"id\":1,\"protoVersion\":1,\"clientName\":\"test589\"}");
                espera(in, "HELLO_REPLY", 1);
                o.println("{\"type\":\"SET_BP\",\"id\":2,\"file\":\"DbgCad.bp\",\"line\":" + LINEA + ",\"enabled\":true}");
                espera(in, "SET_BP_REPLY", 2);
                programa.start();

                // la pausa de entrada (la primera línea) y, tras CONTINUE, la del breakpoint
                long idc = 100;
                while (true) {
                    Map<String, Object> hit = espera(in, "BP_HIT", -1);
                    if (Json.getLong(Json.getMap(hit, "frame"), "line", -1) == LINEA) break;
                    o.println("{\"type\":\"CONTINUE\",\"id\":" + (idc++) + "}");
                }

                o.println("{\"type\":\"LOCALS\",\"id\":3}");
                Map<String, Object> loc = espera(in, "LOCALS_REPLY", 3);
                Map<String, String> ver = new HashMap<>();
                Map<String, Long> valor = new HashMap<>();
                @SuppressWarnings("unchecked")
                List<Object> named = (List<Object>) loc.get("named");
                assertNotNull(named, "LOCALS_REPLY sin named[]: " + loc);
                for (Object x : named) {
                    @SuppressWarnings("unchecked") Map<String, Object> v = (Map<String, Object>) x;
                    ver.put(Json.getString(v, "name", "?"), Json.getString(v, "display", null));
                    valor.put(Json.getString(v, "name", "?"), Json.getLong(v, "value", 0));
                }
                assertTrue(ver.keySet().containsAll(java.util.Arrays.asList("lit", "heap", "nada")),
                        "faltan locales en LOCALS_REPLY: " + loc);
                assertEquals("\"di \\\"hola\\\"\"", ver.get("lit"), "un literal, entre comillas UNA vez y con las suyas escapadas");
                assertEquals("\"hola\\nmundo\"", ver.get("heap"), "una cadena del HEAP (antes «\"@40000…\"»), con el salto escapado");
                assertEquals("\"\"", ver.get("nada"), "un literal vacío");

                o.println("{\"type\":\"MODULE_PROPERTIES\",\"id\":4}");
                Map<String, Object> pr = espera(in, "MODULE_PROPERTIES_REPLY", 4);
                Map<String, String> props = new HashMap<>();
                @SuppressWarnings("unchecked")
                List<Object> lista = (List<Object>) pr.get("props");
                for (Object x : lista) {
                    @SuppressWarnings("unchecked") Map<String, Object> p = (Map<String, Object>) x;
                    if ("DbgCad".equals(Json.getString(p, "module", ""))) {
                        props.put(Json.getString(p, "name", "?"), Json.getString(p, "display", null));
                    }
                }
                assertEquals("\"mundo\"", props.get("nombre"), "una property del heap (antes «null»: la palabra alta)");
                assertEquals("\"\"", props.get("vacia"), "una property vacía, del heap");
                assertEquals("\"prop\"", props.get("literal"), "una property con un literal");

                // READ_STRING, como el de la VM-C: la REFERENCIA (la palabra baja) y el texto tal cual
                long refHeap = valor.get("heap") & 0xFFFFFFFFL;
                long refLit  = valor.get("lit") & 0xFFFFFFFFL;
                o.println("{\"type\":\"READ_STRING\",\"id\":5,\"ref\":" + refHeap + "}");
                assertEquals("hola\nmundo", Json.getString(espera(in, "READ_STRING_REPLY", 5), "value", null));
                o.println("{\"type\":\"READ_STRING\",\"id\":6,\"ref\":" + refLit + "}");
                assertEquals("di \"hola\"", Json.getString(espera(in, "READ_STRING_REPLY", 6), "value", null));
                o.println("{\"type\":\"READ_STRING\",\"id\":7,\"ref\":0}");
                assertEquals("", Json.getString(espera(in, "READ_STRING_REPLY", 7), "value", null));
                o.println("{\"type\":\"READ_STRING\",\"id\":8,\"ref\":" + (0x40000000L | 0x3FFFFF) + "}");
                Map<String, Object> malo = espera(in, "READ_STRING_REPLY", 8);
                assertEquals("ERROR", Json.getString(malo, "type", ""), "un handle fuera de la tabla es BAD_REF: " + malo);
                assertEquals("BAD_REF", Json.getString(malo, "code", ""));

                o.println("{\"type\":\"INSPECT\",\"id\":9,\"ref\":" + refHeap + ",\"depth\":1}");
                Map<String, Object> ins = espera(in, "INSPECT_REPLY", 9);
                assertEquals("string", Json.getString(ins, "class", ""));
                assertEquals("hola\nmundo", Json.getString(ins, "value", null));

                o.println("{\"type\":\"CONTINUE\",\"id\":10}");
                espera(in, "CONTINUE_REPLY", 10);
                programa.join(TimeUnit.SECONDS.toMillis(20));
                assertFalse(programa.isAlive(), "el programa debió terminar tras el CONTINUE");
                assertNull(fallo[0], "el programa falló: " + fallo[0]);
            }
        }
    }
}
