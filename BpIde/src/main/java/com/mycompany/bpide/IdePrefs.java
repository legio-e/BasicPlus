// ============================================================
// IdePrefs.java
// Preferencias persistentes del IDE. Hoy contiene endpoint de la
// VM (host:port) y última carpeta abierta; está pensado para
// crecer: layout de paneles, último proyecto, etc.
//
// El fichero se guarda en el HOME del usuario (System property
// user.home) como `.bpide-prefs`. En Windows típicamente
// C:\Users\<nombre>\.bpide-prefs; en Linux/Mac ~/.bpide-prefs.
// Antes vivía en el CWD del proceso, lo cual fallaba si se
// lanzaba el jar desde un directorio sin permisos de escritura
// (Program Files, etc.). Si existe el legacy en CWD, lo migra
// transparente al HOME y elimina el viejo.
//
// Errores de lectura/escritura se loggean a stderr con la ruta
// absoluta para diagnóstico, y nunca rompen el flujo del IDE.
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

    /** Última carpeta donde se abrió o guardó un fichero .bp.
     *  El JFileChooser arranca aquí en vez de en Documents para que el
     *  usuario no tenga que navegar de nuevo cada vez. */
    public String lastDir;

    /** IDE-4 — Última carpeta usada en el "Upload…" del PicoExplorer. Separada
     *  de lastDir (Load/Save de .bp) porque suelen ser carpetas distintas: los
     *  .mod compilados vs los .bp fuente. */
    public String lastUploadDir;

    /** AOT (H12) — ruta del compilador ARM (arm-none-eabi-gcc) en ESTA máquina.
     *  Null/empty = autodetect (PATH + ubicación estándar del instalador Arm).
     *  Per-máquina: el toolchain se instala en sitios distintos en cada PC, por
     *  eso va en prefs y no en el .bpbuild del proyecto (que es portable). */
    public String aotGccPath;
    /** AOT (H12) — raíz de bpgenvm-c en ESTA máquina, para los includes que gcc
     *  necesita al compilar aot_&lt;Mod&gt;.c (-I &lt;dir&gt;/include -I &lt;dir&gt;/src).
     *  Null/empty = autodetect. */
    public String aotBpgenvmDir;

    /** IDE-3 — Recientes: ficheros .bp abiertos y proyectos .bpbuild abiertos.
     *  Más-reciente-primero, sin duplicados, cap MAX_RECENT. Se persisten como
     *  un string con entradas separadas por '\n' (los paths no llevan newline). */
    public java.util.List<String> recentFiles    = new java.util.ArrayList<>();
    public java.util.List<String> recentProjects = new java.util.ArrayList<>();
    public static final int MAX_RECENT = 8;

    /** Inserta `path` al frente de `list` (dedup + cap a `max`). */
    public static void pushRecent(java.util.List<String> list, String path, int max) {
        if (path == null || path.isEmpty()) return;
        list.remove(path);
        list.add(0, path);
        while (list.size() > max) list.remove(list.size() - 1);
    }

    /** Parte un string "a\nb\nc" en lista (ignora vacíos). Inverso de String.join. */
    private static java.util.List<String> splitRecent(String s) {
        java.util.List<String> out = new java.util.ArrayList<>();
        if (s == null || s.isEmpty()) return out;
        for (String p : s.split("\n")) if (!p.isEmpty()) out.add(p);
        return out;
    }

    private static final String FILENAME = ".bpide-prefs";

    /** Ruta canónica del fichero de prefs: $HOME/.bpide-prefs.
     *  Si user.home no está disponible (improbable en JVM normal),
     *  fallback al cwd para no romper en entornos exóticos. */
    private static Path prefsPath() {
        String home = System.getProperty("user.home");
        if (home != null && !home.isEmpty()) {
            return Paths.get(home, FILENAME);
        }
        return Paths.get(FILENAME);   /* fallback legacy */
    }

    /** Si existe un .bpide-prefs en CWD (legacy de versiones
     *  anteriores), lo migra al HOME y borra el original. Llamado
     *  desde load() para que la primera vez tras el upgrade el
     *  usuario no pierda sus prefs. */
    private static void migrateLegacyIfAny() {
        Path legacy = Paths.get(FILENAME);
        Path canonical = prefsPath();
        if (legacy.equals(canonical)) return;     /* mismo sitio, nada que hacer */
        if (!Files.isRegularFile(legacy)) return;
        if (Files.isRegularFile(canonical)) {
            /* Ambos existen: el canónico gana, borramos el legacy. */
            try { Files.delete(legacy); } catch (IOException ignored) {}
            return;
        }
        try {
            Files.copy(legacy, canonical);
            Files.delete(legacy);
        } catch (IOException ignored) {
            /* Si no podemos mover, dejamos las prefs viejas donde estaban
             * — el load del cwd las recuperará como fallback. */
        }
    }

    /** Lee las prefs. Devuelve una instancia vacía si no existe el
     *  fichero o si hay error de lectura — nunca lanza. */
    public static IdePrefs load() {
        migrateLegacyIfAny();
        IdePrefs p = new IdePrefs();
        Path f = prefsPath();
        /* Fallback: si no hay en HOME pero sí en CWD (migración
         * fallida o entorno raro), leer del CWD. */
        if (!Files.isRegularFile(f)) f = Paths.get(FILENAME);
        if (!Files.isRegularFile(f)) return p;
        try {
            String raw = new String(Files.readAllBytes(f), StandardCharsets.UTF_8);
            Map<String, Object> m = Json.parseFlatObject(raw);
            p.vmHost  = Json.getString(m, "vmHost", null);
            p.vmPort  = (int) Json.getLong(m, "vmPort", 0);
            p.lastDir = Json.getString(m, "lastDir", null);
            p.lastUploadDir = Json.getString(m, "lastUploadDir", null);
            p.aotGccPath = Json.getString(m, "aotGccPath", null);
            p.aotBpgenvmDir = Json.getString(m, "aotBpgenvmDir", null);
            if (p.vmHost  != null && p.vmHost.isEmpty())  p.vmHost  = null;
            if (p.lastDir != null && p.lastDir.isEmpty()) p.lastDir = null;
            if (p.lastUploadDir != null && p.lastUploadDir.isEmpty()) p.lastUploadDir = null;
            if (p.aotGccPath != null && p.aotGccPath.isEmpty()) p.aotGccPath = null;
            if (p.aotBpgenvmDir != null && p.aotBpgenvmDir.isEmpty()) p.aotBpgenvmDir = null;
            p.recentFiles    = splitRecent(Json.getString(m, "recentFiles", null));
            p.recentProjects = splitRecent(Json.getString(m, "recentProjects", null));
        } catch (Throwable t) {
            System.err.println("[IdePrefs] no se pudo leer " + f.toAbsolutePath()
                    + ": " + t.getMessage());
        }
        return p;
    }

    /** Persiste al HOME del usuario. Errores se loggean con la ruta
     *  absoluta para diagnóstico — nunca lanzan. */
    public void save() {
        Map<String, Object> m = new LinkedHashMap<>();
        m.put("vmHost",  vmHost  == null ? "" : vmHost);
        m.put("vmPort",  (long) vmPort);
        m.put("lastDir", lastDir == null ? "" : lastDir);
        m.put("lastUploadDir", lastUploadDir == null ? "" : lastUploadDir);
        m.put("aotGccPath", aotGccPath == null ? "" : aotGccPath);
        m.put("aotBpgenvmDir", aotBpgenvmDir == null ? "" : aotBpgenvmDir);
        m.put("recentFiles",    String.join("\n", recentFiles));
        m.put("recentProjects", String.join("\n", recentProjects));
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
        Path f = prefsPath();
        try {
            Files.write(f, sb.toString().getBytes(StandardCharsets.UTF_8));
        } catch (IOException ioe) {
            System.err.println("[IdePrefs] no se pudo escribir " + f.toAbsolutePath()
                    + ": " + ioe.getMessage());
        }
    }
}
