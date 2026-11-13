// ============================================================
// TrazaError.java
// V8 #582 — DÓNDE falló un programa, con sus nombres.
//
// La placa no tiene el .dbg: en el `stackTrace` del EXITED da posiciones
// («Prueba+50 Prueba+72 Core+1200», de dentro afuera) y el PC pone los nombres,
// como en el depurador. Cada posición es la instrucción SIGUIENTE a la que falló
// o llamó, así que la línea es la de la posición menos uno. Lo que cae en la línea
// 0 (el __init del módulo) no es del programa, y no se cuenta.
//
// Las carpetas donde mirar los .dbg las va diciendo el IDE al compilar (conoce);
// la stdlib se busca sola. Un módulo sin .dbg sale tal cual («Core+1200»): se
// pierde la línea, no el dato. El espejo en Python, tools/traza_dbg.py.
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.vm.debug.DbgFile;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;

final class TrazaError {
    private static final List<Path> carpetas = new CopyOnWriteArrayList<>();

    private TrazaError() {}

    /** Una carpeta con los .dbg de lo que se va a ejecutar: la última que se dice, la primera que se mira. */
    static void conoce(Path carpeta) {
        if (carpeta == null) return;
        carpetas.remove(carpeta);
        carpetas.add(0, carpeta);
    }

    /** La traza del EXITED, marco a marco: «Prueba.bp:23», o «Mod+rel» si no hay .dbg de ese módulo. */
    static List<String> traduce(String traza) {
        List<Path> donde = new ArrayList<>(carpetas);
        Path stdlib = IdePrefs.installSubdir("bpstdlib");
        if (stdlib != null) donde.add(stdlib);
        return traduce(traza, donde);
    }

    static List<String> traduce(String traza, List<Path> donde) {
        List<String> salida = new ArrayList<>();
        if (traza == null || traza.trim().isEmpty()) return salida;
        Map<String, DbgFile> dbgs = new HashMap<>();
        for (String marco : traza.trim().split("\\s+")) {
            int mas = marco.lastIndexOf('+');
            if (mas <= 0) { salida.add(marco); continue; }
            String mod = marco.substring(0, mas);
            int rel;
            try { rel = Integer.parseInt(marco.substring(mas + 1)); }
            catch (NumberFormatException e) { salida.add(marco); continue; }
            if (!dbgs.containsKey(mod)) dbgs.put(mod, busca(mod, donde));
            DbgFile dbg = dbgs.get(mod);
            if (dbg == null) { salida.add(marco); continue; }
            int linea = dbg.lineForRelPc(rel - 1);
            if (linea > 0) salida.add(fichero(dbg, mod) + ":" + linea);
        }
        return salida;
    }

    /** V8 #788 — las líneas de la consola: «  en X» el primero y «  llamado desde Y» los demás. Las mismas en el
     *  camino de la placa (AbstractBpvmBackend.run) y en el de la VM local (TrazaDeLaVmLocal). */
    static List<String> lineas(List<String> donde) {
        List<String> out = new ArrayList<>(donde.size());
        for (int i = 0; i < donde.size(); i++) out.add((i == 0 ? "  en " : "  llamado desde ") + donde.get(i));
        return out;
    }

    private static DbgFile busca(String mod, List<Path> donde) {
        for (Path c : donde) {
            Path p = c.resolve(mod + ".dbg");
            if (Files.exists(p)) {
                DbgFile d = DbgFile.load(p.toString());   // null si no se lee: es como no tenerlo, sale la posición
                if (d != null) return d;
            }
        }
        return null;
    }

    private static String fichero(DbgFile dbg, String mod) {
        String fuente = dbg.getSource();
        if (fuente == null || fuente.isEmpty()) return mod + ".bp";
        return Paths.get(fuente.replace('\\', '/')).getFileName().toString();
    }
}
