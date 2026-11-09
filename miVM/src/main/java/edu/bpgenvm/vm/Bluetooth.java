/*
 * Bluetooth.java — V8/H2 (8-nov): EL BLUETOOTH DEL PC, simulado, para la clase Air.Bluetooth (V8_IDEAS §E1, «Lo que ve el
 * programa»; Eduardo: «Funcionalidades, encender y apagar, scan, conectar y desconectar»).
 *
 * El espejo de la VM-C: de bpvm_bt.c, lo común (el estado, lo encontrado en la última búsqueda, lo conectado y lo recordado
 * de /sys/bluetooth.json, que escribe la clase en BP y aquí sólo se lee al encender); y de bt_host.c, la radio del PC, con
 * dos aparatos de mentira, fijos: «Teclado simulado» y «Ratón simulado». Las dos VMs tienen que escribir lo mismo.
 */
package edu.bpgenvm.vm;

import edu.bpgenvm.util.Json;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Function;

final class Bluetooth {
    static final int OFF = 0, STARTING = 1, ON = 2, SCANNING = 3;
    static final String FICHERO = "/sys/bluetooth.json";
    private static final int MAX_ENCONTRADOS = 16, MAX_CONECTADOS = 8;   // los de bpvm_bt.h

    /** Un aparato visto en una búsqueda. */
    static final class Aparato {
        final String nombre, dir, tipo, clase;
        final int rssi;
        Aparato(String nombre, String dir, String tipo, String clase, int rssi) {
            this.nombre = nombre; this.dir = dir; this.tipo = tipo; this.clase = clase; this.rssi = rssi;
        }
    }

    // La radio del PC (bt_host.c): el teclado, como el de verdad, no dice qué es en la búsqueda; lo dice al conectarlo.
    private static final Aparato[] SIMULADOS = {
        new Aparato("Teclado simulado", "02:00:00:00:00:01", "random", "unknown", -40),
        new Aparato("Ratón simulado",   "02:00:00:00:00:02", "random", "mouse",   -55),
    };
    private static final String[] CLASES = { "keyboard", "mouse" };

    /** Lee un fichero de la placa (su ruta, "/sys/…"): sus bytes, o null si no está. */
    private final Function<String, byte[]> lector;
    private int estado = OFF;
    private long buscaHasta;
    private final List<Aparato> vistos = new ArrayList<>();
    private final LinkedHashMap<String, String> conectados = new LinkedHashMap<>();   // dirección → clase
    private boolean tocado;   // un programa lo ha encendido o apagado: al acabar, vuelve a lo del fichero
    private String motivo = "";

    Bluetooth(Function<String, byte[]> lector) { this.lector = lector; }

    synchronized int estado() {
        if (estado == SCANNING && ahora() >= buscaHasta) estado = ON;
        return estado;
    }

    /** null, o el motivo por el que no se enciende. */
    synchronized String programaEnciende() { tocado = true; return encender(); }
    synchronized void programaApaga() { tocado = true; apagar(); }

    private String encender() {
        if (estado != OFF) return null;   // ya está, o encendiéndose
        estado = STARTING;
        estado = ON;                      // la radio del PC avisa en seguida: lista
        reconectarRecordados();
        return null;
    }

    private void apagar() {
        estado = OFF;
        conectados.clear();
        vistos.clear();
    }

    /** null, o el motivo por el que no busca. */
    synchronized String buscar(int ms) {
        if (estado() < ON) return "Bluetooth.scan: está apagado (turnOn primero)";
        if (ms < 0) ms = 0;
        vistos.clear();
        estado = SCANNING;
        buscaHasta = ahora() + ms;
        for (Aparato a : SIMULADOS) visto(a);   // están ahí desde el principio; la búsqueda dura lo pedido
        return null;
    }

    private void visto(Aparato a) {
        if (estado != SCANNING || conectados.containsKey(a.dir) || vistos.size() >= MAX_ENCONTRADOS) return;
        for (Aparato v : vistos) if (v.dir.equals(a.dir)) return;   // el mismo, otra vez: no se repite
        vistos.add(a);
    }

    synchronized int encontrados() { return vistos.size(); }
    synchronized Aparato encontrado(int i) { return (i >= 0 && i < vistos.size()) ? vistos.get(i) : null; }

    /** 1 conectado, 0 no (el programa recibe false), -1 apagado (con el motivo en motivo()). */
    synchronized int conectar(String dir, String tipo) {
        if (estado() < ON) { motivo = "Bluetooth.connect: está apagado (turnOn primero)"; return -1; }
        if (conectados.containsKey(dir)) return 1;
        String clase = radioConectar(dir);
        if (clase == null) return 0;
        if (conectados.size() < MAX_CONECTADOS) conectados.put(dir, clase);
        return 1;
    }

    /** La radio del PC: la clase del aparato, o null si no lo encuentra. */
    private static String radioConectar(String dir) {
        for (int i = 0; i < SIMULADOS.length; i++) if (SIMULADOS[i].dir.equals(dir)) return CLASES[i];
        return null;
    }

    synchronized void desconectar(String dir) { conectados.remove(dir); }
    synchronized void olvidar(String dir) { conectados.remove(dir); }
    synchronized boolean conectado(String dir) { return conectados.containsKey(dir); }
    synchronized String claseDe(String dir) { String c = conectados.get(dir); return c == null ? "" : c; }
    /** V8/H2, paso 2 — ¿hay un teclado conectado? En el PC, sin él, la ventana de la GUI no teclea (bpvm_bt_hay_teclado). */
    synchronized boolean hayTeclado() { return conectados.containsValue("keyboard"); }
    String motivo() { return motivo; }

    /** Como una placa al arrancar: si el fichero dice "autoStart", se enciende y espera a los recordados. */
    synchronized void arranque() { if (ficheroAutoStart()) encender(); }

    /** Al acabar un programa que lo tocó: lo que diga el fichero. */
    synchronized void finDeRun() {
        if (!tocado) return;
        tocado = false;
        if (ficheroAutoStart()) { if (estado() == OFF) encender(); }
        else apagar();
    }

    // ── Lo recordado: /sys/bluetooth.json ────────────────────────────────────────────────────────────────────────────

    @SuppressWarnings("unchecked")
    private Map<String, Object> leerFichero() {
        byte[] b = lector.apply(FICHERO);
        if (b == null) return null;
        try {
            Object o = Json.parse(new String(b, StandardCharsets.UTF_8).trim());
            return (o instanceof Map) ? (Map<String, Object>) o : null;
        } catch (RuntimeException e) {
            return null;
        }
    }

    /** ¿Arranca encendido? "autoStart", como en /sys/wifi.json (Eduardo: «como en la wifi»). */
    private boolean ficheroAutoStart() {
        Map<String, Object> m = leerFichero();
        return m != null && Boolean.TRUE.equals(m.get("autoStart"));
    }

    /** Cada recordado, por su dirección y en el orden del fichero: la radio lo conecta (en el PC, lo encuentra). Si
     *  alguno no está o no conecta, no es un error (Eduardo: «al menos que lo intenten pero si no están o no conectan que
     *  no se produzca un error»). */
    @SuppressWarnings("unchecked")
    private void reconectarRecordados() {
        Map<String, Object> m = leerFichero();
        if (m == null || !(m.get("devices") instanceof Map)) return;
        for (Map.Entry<String, Object> e : ((Map<String, Object>) m.get("devices")).entrySet()) {
            String dir = e.getKey();
            if (dir.length() > 17) dir = dir.substring(0, 17);   // el tope de la VM-C (BPVM_BT_DIR)
            if (!(e.getValue() instanceof Map)) continue;
            String clase = radioConectar(dir);
            if (clase == null) continue;
            if (!conectados.containsKey(dir) && conectados.size() < MAX_CONECTADOS) conectados.put(dir, clase);
        }
    }

    private static long ahora() { return System.nanoTime() / 1_000_000L; }
}
