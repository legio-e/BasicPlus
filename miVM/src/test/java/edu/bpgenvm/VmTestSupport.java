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
        /* SE CAPTURAN LOS DOS CANALES, y el porqué importa (22-ago):
         *
         * El diagnóstico de GC (`VM [GC]: ...`) salía por **stdout**, y de ahí lo
         * contaba `parse`. En V5 se movió a **stderr**, y con razón: stdout tiene
         * que ser BYTE-IDÉNTICO entre miVM y la VM-C —es el invariante del
         * proyecto— y cada una escribe su traza de GC de forma distinta. Dejarla
         * en stdout rompía la paridad.
         *
         * Nadie actualizó estos tests, así que llevaban rojos contando 0 GCs
         * mientras el GC funcionaba: `GcTest` y `OopTest` fallaban SÓLO en el
         * recuento —sus aserciones de salida pasaban— que es la firma de un test
         * rancio y no de un producto roto.
         *
         * Se capturan los dos y se concatenan: las líneas de programa siguen
         * viniendo de stdout y las de diagnóstico de stderr, y `parse` las
         * distingue por su prefijo como siempre. */
        PrintStream originalOut = System.out;
        PrintStream originalErr = System.err;
        ByteArrayOutputStream bOut = new ByteArrayOutputStream();
        ByteArrayOutputStream bErr = new ByteArrayOutputStream();
        PrintStream capOut, capErr;
        try {
            capOut = new PrintStream(bOut, true, "UTF-8");
            capErr = new PrintStream(bErr, true, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            throw new AssertionError(e);
        }
        System.setOut(capOut);
        System.setErr(capErr);
        try {
            mainFn.run();
        } catch (Exception e) {
            System.setOut(originalOut);
            System.setErr(originalErr);
            e.printStackTrace(originalOut);
            throw new AssertionError("main lanzó excepción", e);
        } finally {
            System.setOut(originalOut);
            System.setErr(originalErr);
        }
        String full = new String(bOut.toByteArray(), StandardCharsets.UTF_8)
                    + new String(bErr.toByteArray(), StandardCharsets.UTF_8);
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
            /* El diagnóstico de GC va por stderr y se concatena DESPUÉS de stdout,
             * o sea que llega cuando `inVm` ya es false. Se cuenta aparte, antes
             * del filtro: no está dentro de la ventana de ejecución y no tiene por
             * qué estarlo — es diagnóstico, no salida de programa. */
            if (raw.trim().startsWith("VM [GC]:")) { r.gcCount++; continue; }
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
            } else {
                // Línea de PRINT_STRING (o PRINT_CHAR concatenado): la VM la imprime "limpia".
                r.stringPrints.add(line);
            }
        }
        return r;
    }

    private VmTestSupport() {}
}
