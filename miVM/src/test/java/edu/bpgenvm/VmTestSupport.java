package edu.bpgenvm;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.io.UnsupportedEncodingException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/**
 * Helpers para escribir tests que invocan a un Main*.main() existente,
 * capturan su stdout y parsean lo que la VM produjo entre los marcadores
 * "=== INICIANDO EJECUCIÓN DE LA VM ===" y "=== FIN DE LA EJECUCIÓN ===".
 *
 * Todo lo emitido FUERA de esa ventana (mensajes del compilador, "Esperado:"
 * y similares) se ignora a propósito.
 */
public final class VmTestSupport {

    public static final class VmResult {
        public final List<Integer> prints      = new ArrayList<>();
        public final List<Float>   fprints     = new ArrayList<>();
        public final List<String>  stringPrints = new ArrayList<>(); // PRINT_STRING outputs
        public int gcCount = 0;
        public String rawOutput = "";
    }

    @FunctionalInterface
    public interface ThrowingRunnable {
        void run() throws Exception;
    }

    /**
     * Ejecuta {@code mainFn} capturando System.out y devuelve el resultado
     * parseado. Cualquier excepción lanzada por {@code mainFn} se relanza
     * como AssertionError para que el test falle con stack trace útil.
     */
    public static VmResult runMain(ThrowingRunnable mainFn) {
        PrintStream original = System.out;
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        PrintStream capture;
        try {
            capture = new PrintStream(baos, true, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            throw new AssertionError(e);
        }
        System.setOut(capture);
        try {
            mainFn.run();
        } catch (Exception e) {
            System.setOut(original);
            e.printStackTrace(original);
            throw new AssertionError("main lanzó excepción", e);
        } finally {
            System.setOut(original);
        }
        String full = new String(baos.toByteArray(), StandardCharsets.UTF_8);
        return parse(full);
    }

    private static VmResult parse(String output) {
        VmResult r = new VmResult();
        r.rawOutput = output;
        boolean inVm = false;
        for (String raw : output.split("\\R")) {
            // Markers de la VM
            if (raw.contains("INICIANDO")) { inVm = true; continue; }
            if (raw.contains("FIN DE LA"))  { inVm = false; continue; }
            if (!inVm) continue;

            String line = raw.trim();
            if (line.isEmpty()) continue;
            // Trace per-instrucción (PC: ... | Opcode: ...): ruido del demo, descartar
            if (line.startsWith("PC: ")) continue;
            // Línea informativa que imprime la VM al inicio del run()
            if (line.startsWith("heapStart=")) continue;

            if (line.startsWith("VM [PRINT]: ")) {
                r.prints.add(Integer.parseInt(line.substring("VM [PRINT]: ".length()).trim()));
            } else if (line.startsWith("VM [FPRINT]: ")) {
                r.fprints.add(Float.parseFloat(line.substring("VM [FPRINT]: ".length()).trim()));
            } else if (line.startsWith("VM [GC]:")) {
                r.gcCount++;
            } else {
                // Línea de PRINT_STRING (o PRINT_CHAR concatenado): la VM la imprime "limpia".
                r.stringPrints.add(line);
            }
        }
        return r;
    }

    private VmTestSupport() {}
}
