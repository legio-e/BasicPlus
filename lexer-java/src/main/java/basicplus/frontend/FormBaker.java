// ============================================================
// FormBaker.java — H13.1 (V3) paso 4.1: horneado del .win (Forms, Camino A).
//
// El usuario diseña el formulario como un .win (JSON) con los eventos por NOMBRE
// ("clic":"onOk"). En el despliegue/preview, el IDE compila el módulo de la
// ventana —que produce <Módulo>.slots (método→slot, ver Main.writeSlotsFile)— y
// llama aquí para HORNEAR el .win: resuelve cada evento nombre→slot de la clase
// ventana y escribe "clicSlot"/"changeSlot". El loader en placa despacha por ese
// slot (__guiInvokeBySlot) sin tocar el .mod ni necesitar resolución por nombre.
//
// Es una herramienta de build (como AotMain/MdnPack): vive en el frontend para
// poder probarse por CLI, y el IDE la invoca (empaqueta el frontend).
//
//   java basicplus.frontend.FormBaker <in.win> <Módulo.slots> <out.win>
// ============================================================
package basicplus.frontend;

import edu.bpgenvm.util.Json;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class FormBaker {
    private FormBaker() {}

    /**
     * Hornea un .win: por cada nodo con evento `clic`/`change` (nombre del método
     * handler), resuelve su slot de vtable en la clase ventana (vía el .slots) y
     * escribe `clicSlot`/`changeSlot`. La clase ventana se toma del campo `class`
     * de la raíz del .win.
     *
     * Robusto (decisión de Eduardo: un form a medias avisa, no peta): si el .win
     * no declara `class`, la clase no está en el .slots, o un handler no existe,
     * se acumula un aviso en {@code warnings} y ese evento se hornea a slot -1
     * (el runtime lo IGNORA). Devuelve el JSON horneado (compacto).
     *
     * @param winJson    contenido del .win autoría (eventos por nombre)
     * @param slotsJson  contenido de &lt;Módulo&gt;.slots (puede ser null/vacío)
     * @param warnings   salida: avisos legibles (no fatales)
     */
    @SuppressWarnings("unchecked")
    public static String bake(String winJson, String slotsJson, List<String> warnings) {
        Object winRoot = Json.parse(winJson);
        if (!(winRoot instanceof Map))
            throw new IllegalArgumentException(".win: la raíz no es un objeto JSON");
        Map<String, Object> root = (Map<String, Object>) winRoot;

        Map<String, Object> slots = new LinkedHashMap<>();
        if (slotsJson != null && !slotsJson.trim().isEmpty()) {
            Object s = Json.parse(slotsJson);
            if (s instanceof Map) slots = (Map<String, Object>) s;
        }

        String winClass = Json.getString(root, "class", null);
        Map<String, Object> classSlots = null;
        if (winClass == null) {
            warnings.add(".win sin campo \"class\": no se hornea ningún evento");
        } else if (slots.get(winClass) instanceof Map) {
            classSlots = (Map<String, Object>) slots.get(winClass);
        } else {
            warnings.add("la clase ventana '" + winClass
                    + "' no está en el .slots (¿es public? ¿es el módulo principal?): eventos sin hornear");
        }

        bakeNode(root, classSlots, warnings);
        return Json.write(root);
    }

    @SuppressWarnings("unchecked")
    private static void bakeNode(Map<String, Object> node, Map<String, Object> classSlots,
                                 List<String> warnings) {
        bakeEvent(node, "clic",   "clicSlot",   classSlots, warnings);
        bakeEvent(node, "change", "changeSlot", classSlots, warnings);
        Object children = node.get("children");
        if (children instanceof List) {
            for (Object ch : (List<Object>) children)
                if (ch instanceof Map) bakeNode((Map<String, Object>) ch, classSlots, warnings);
        }
    }

    private static void bakeEvent(Map<String, Object> node, String evKey, String slotKey,
                                  Map<String, Object> classSlots, List<String> warnings) {
        Object ev = node.get(evKey);
        if (!(ev instanceof String)) return;       // sin ese evento → nada que hornear
        String handler = (String) ev;
        long slot = -1;
        if (classSlots != null && classSlots.get(handler) instanceof Long) {
            slot = (Long) classSlots.get(handler);
        } else if (classSlots != null) {
            warnings.add("handler '" + handler + "' (evento '" + evKey
                    + "') no existe en la clase ventana: el evento quedará inactivo");
        }
        node.put(slotKey, slot);                    // -1 = sin resolver → el runtime lo ignora
    }

    // ---- CLI (para pruebas sin el IDE) -------------------------------------
    public static void main(String[] args) throws Exception {
        if (args.length < 3) {
            System.err.println("uso: FormBaker <in.win> <Módulo.slots> <out.win>");
            System.exit(1);
            return;
        }
        String win = new String(Files.readAllBytes(Paths.get(args[0])), StandardCharsets.UTF_8);
        Path slotsPath = Paths.get(args[1]);
        String slots = Files.exists(slotsPath)
                ? new String(Files.readAllBytes(slotsPath), StandardCharsets.UTF_8) : null;
        List<String> warnings = new ArrayList<>();
        String baked = bake(win, slots, warnings);
        Files.write(Paths.get(args[2]), baked.getBytes(StandardCharsets.UTF_8));
        for (String w : warnings) System.err.println("[bake] aviso: " + w);
        System.out.println("horneado: " + args[2]);
    }
}
