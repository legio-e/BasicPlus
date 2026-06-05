package edu.bpgenvm.vm.debug;

import edu.bpgenvm.vm.ModuleManager;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.FileInputStream;
import java.nio.charset.StandardCharsets;
import java.util.Map;
import java.util.TreeMap;

/**
 * H6.b.3 — Lector STANDALONE del fichero `.dbg` para el host/IDE, desacoplado
 * de la {@link ModuleManager} (que vive dentro de una VM y mapea por dirección
 * absoluta tras cargar el `.mod`).
 *
 * <p>Lo usa el IDE cuando depura una VM <b>device-role</b> (el server wire de la
 * VM-C, o la Pico): el device reporta pc/sp/bp CRUDOS y locales por índice; el
 * IDE tiene el `.dbg` y resuelve aquí los símbolos (relPc→función/línea,
 * slot→nombre/tipo). Es la pieza "host hace la traducción simbólica" de la regla
 * de oro de H6.</p>
 *
 * <p>Trabaja en <b>relPc</b> (relativo al inicio de código del módulo). El device
 * envía pc absoluto + cs (= code start) en BP_HIT, así que el IDE calcula
 * relPc = pc - cs y pregunta aquí. Reusa {@link ModuleManager.FunctionVars} /
 * {@link ModuleManager.LocalVarDescriptor} para no duplicar structs.</p>
 *
 * <p>NOTA: el parser del formato `.dbg` está duplicado a propósito respecto a
 * {@code ModuleManager.loadDbgFor} — preferimos un lector read-only separado a
 * refactorizar la ModuleManager (que es load-bearing del debugger Java). El
 * formato `.dbg` está documentado y es estable (ver MivmEmitter.writeDbgFile).</p>
 */
public final class DbgFile {

    private String module;
    private String source;
    private final TreeMap<Integer, Integer> lineMap = new TreeMap<>();              // relPc → línea
    private final TreeMap<Integer, ModuleManager.FunctionVars> funcs = new TreeMap<>(); // startRelPc → vars

    private DbgFile() {}

    public String getModule() { return module; }
    public String getSource() { return source; }

    /** Línea origen 1-based para un relPc, o -1 si no hay info. */
    public int lineForRelPc(int relPc) {
        Map.Entry<Integer, Integer> e = lineMap.floorEntry(relPc);
        return (e != null) ? e.getValue() : -1;
    }

    /** Función (con sus vars) que contiene el relPc, o null. */
    public ModuleManager.FunctionVars functionForRelPc(int relPc) {
        Map.Entry<Integer, ModuleManager.FunctionVars> e = funcs.floorEntry(relPc);
        return (e != null) ? e.getValue() : null;
    }

    /** Parsea un `.dbg`. Devuelve null si no existe o el header no es válido. */
    public static DbgFile load(String path) {
        java.io.File f = new java.io.File(path);
        if (!f.isFile()) return null;
        DbgFile d = new DbgFile();
        try (BufferedReader br = new BufferedReader(
                new InputStreamReader(new FileInputStream(f), StandardCharsets.UTF_8))) {
            String header = br.readLine();
            if (header == null || !header.startsWith("dbg ")) return null;
            String line;
            boolean inLines = false, inVars = false;
            ModuleManager.FunctionVars cur = null;
            while ((line = br.readLine()) != null) {
                if (line.isEmpty()) continue;
                if (line.startsWith("module ")) { d.module = line.substring(7).trim(); inLines = inVars = false; continue; }
                if (line.startsWith("source ")) { d.source = line.substring(7).trim(); inLines = inVars = false; continue; }
                if (line.equals("lines"))      { inLines = true;  inVars = false; continue; }
                if (line.equals("properties")) { inLines = false; inVars = false; continue; }   // ignoradas aquí
                if (line.equals("vars"))       { inLines = false; inVars = true;  cur = null; continue; }
                if (inLines) {
                    int sp = line.indexOf(' ');
                    if (sp <= 0) continue;
                    try {
                        d.lineMap.put(Integer.parseInt(line.substring(0, sp).trim()),
                                      Integer.parseInt(line.substring(sp + 1).trim()));
                    } catch (NumberFormatException ignored) {}
                    continue;
                }
                if (inVars) {
                    if (line.startsWith("func ")) {
                        String[] t = line.substring(5).trim().split("\\s+");
                        cur = null;
                        if (t.length >= 2) {
                            try {
                                int start = Integer.parseInt(t[1]);
                                cur = new ModuleManager.FunctionVars(t[0], start);
                                d.funcs.put(start, cur);
                            } catch (NumberFormatException ignored) {}
                        }
                        continue;
                    }
                    if (cur != null) {
                        // "<name> <offset> <size> <isArray> [<type>]"
                        String[] t = line.trim().split("\\s+");
                        if (t.length >= 4) {
                            try {
                                String type = (t.length >= 5) ? t[4] : "?";
                                cur.vars.add(new ModuleManager.LocalVarDescriptor(
                                        t[0], Integer.parseInt(t[1]),
                                        Integer.parseInt(t[2]), t[3].equals("1"), type));
                            } catch (NumberFormatException ignored) {}
                        }
                    }
                }
            }
        } catch (IOException e) {
            return null;
        }
        return d;
    }
}
