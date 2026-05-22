// ============================================================
// IdePrefs.java
// Preferencias persistentes del IDE (A2.6+). Hoy contiene sólo el
// endpoint de la VM (host:port) para targeting remoto, pero está
// pensado para crecer: layout de paneles, último proyecto abierto,
// etc. Se almacena en JSON sencillo en el cwd del IDE: `.bpide-prefs`.
// Errores de lectura/escritura se loggean a stderr y no rompen el flujo.
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.util.Json;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.LinkedHashMap;
import java.util.Map;

/** Preferencias persistentes del IDE. */
public final class IdePrefs {

    /** Endpoint de la VM remota. Null/empty = spawn local (default histórico).
     *  Si está fijado, doRun/doDebug se conectan directamente a host:port en
     *  vez de lanzar un subproceso local. */
    public String vmHost;
    public int    vmPort;

    private static final String FILENAME = ".bpide-prefs";

    /** Lee las prefs del cwd. Devuelve una instancia vacía si no existe el
     *  fichero o si hay error de lectura — nunca lanza. */
    public static IdePrefs load() {
        IdePrefs p = new IdePrefs();
        Path f = Paths.get(FILENAME);
        if (!Files.isRegularFile(f)) return p;
        try {
            String raw = new String(Files.readAllBytes(f), StandardCharsets.UTF_8);
            Map<String, Object> m = Json.parseFlatObject(raw);
            p.vmHost = Json.getString(m, "vmHost", null);
            p.vmPort = (int) Json.getLong(m, "vmPort", 0);
            if (p.vmHost != null && p.vmHost.isEmpty()) p.vmHost = null;
        } catch (Throwable t) {
            System.err.println("[IdePrefs] no se pudo leer " + FILENAME + ": " + t.getMessage());
        }
        return p;
    }

    /** Persiste al cwd. Errores se loggean. */
    public void save() {
        Map<String, Object> m = new LinkedHashMap<>();
        m.put("vmHost", vmHost == null ? "" : vmHost);
        m.put("vmPort", (long) vmPort);
        StringBuilder sb = new StringBuilder();
        sb.append('{');
        boolean first = true;
        for (Map.Entry<String, Object> e : m.entrySet()) {
            if (!first) sb.append(',');
            first = false;
            sb.append('"').append(Json.escape(e.getKey())).append("\":");
            Object v = e.getValue();
            if (v instanceof String) sb.append('"').append(Json.escape((String) v)).append('"');
            else                     sb.append(v.toString());
        }
        sb.append('}');
        try {
            Files.write(Paths.get(FILENAME), sb.toString().getBytes(StandardCharsets.UTF_8));
        } catch (IOException ioe) {
            System.err.println("[IdePrefs] no se pudo escribir " + FILENAME + ": " + ioe.getMessage());
        }
    }
}
