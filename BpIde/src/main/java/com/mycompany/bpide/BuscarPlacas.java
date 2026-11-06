// ============================================================
// BuscarPlacas.java
// V8/H3 (6-nov): encontrar las placas de la red, cuando se le pide. Un {"type":"FIND"} por UDP a toda la red, por cada interfaz
// del PC, y cada placa con red contesta quién es (esp32/common/wire_red.c; el protocolo, en docs/BPVM_WIRE_PROTOCOL.md).
// La placa no se anuncia sola: mientras nadie pregunta, no gasta nada (Eduardo: «Encontrar lo puede hacer cuando en el IDE le
// digamos que busque un dispositivo en la red local, si no se conecta con la IP que le indiquemos»).
// ============================================================
package com.mycompany.bpide;

import edu.bpgenvm.util.Json;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InterfaceAddress;
import java.net.NetworkInterface;
import java.net.SocketTimeoutException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class BuscarPlacas {

    /** El de la placa: el mismo para el wire por TCP y para buscar. */
    public static final int PUERTO = 3333;

    private static final byte[] PREGUNTA = "{\"type\":\"FIND\"}\n".getBytes(StandardCharsets.UTF_8);

    private BuscarPlacas() { }

    /** Una placa que ha contestado. */
    public static final class Placa {
        public final String nombre, micro, id, ip;
        public final int puerto;

        Placa(String nombre, String micro, String id, String ip, int puerto) {
            this.nombre = nombre;
            this.micro  = micro;
            this.id     = id;
            this.ip     = ip;
            this.puerto = puerto;
        }

        /** Lo que se escribe en el campo de la dirección. */
        public String endpoint() { return puerto == PUERTO ? ip : ip + ":" + puerto; }

        @Override public String toString() { return nombre + " (" + micro + ", " + endpoint() + ")"; }
    }

    /** A toda la red, por cada interfaz con difusión: el PC puede tener varias, y la de una máquina virtual no sirve de nada. */
    private static List<InetAddress> difusiones() throws IOException {
        List<InetAddress> r = new ArrayList<>();
        for (NetworkInterface ni : Collections.list(NetworkInterface.getNetworkInterfaces())) {
            if (!ni.isUp() || ni.isLoopback()) continue;
            for (InterfaceAddress ia : ni.getInterfaceAddresses())
                if (ia.getBroadcast() != null) r.add(ia.getBroadcast());
        }
        return r;
    }

    /** Pregunta y espera a que contesten durante {@code ms}. Una placa, una vez, aunque conteste por dos sitios. */
    public static List<Placa> buscar(int ms) throws IOException {
        Map<String, Placa> placas = new LinkedHashMap<>();
        try (DatagramSocket k = new DatagramSocket()) {
            k.setBroadcast(true);
            List<InetAddress> destinos = difusiones();
            byte[] buf = new byte[512];
            long hasta = System.currentTimeMillis() + ms, proxima = 0;
            for (;;) {
                long ahora = System.currentTimeMillis();
                if (ahora >= hasta) break;
                /* Varias veces: por la WiFi una difusión no lleva acuse, y se pierde (medido el 6-nov). */
                if (ahora >= proxima) {
                    for (InetAddress d : destinos)
                        k.send(new DatagramPacket(PREGUNTA, PREGUNTA.length, d, PUERTO));
                    proxima = ahora + 400;
                }
                k.setSoTimeout((int) Math.max(1, Math.min(hasta, proxima) - ahora));
                DatagramPacket p = new DatagramPacket(buf, buf.length);
                try { k.receive(p); }
                catch (SocketTimeoutException sigue) { continue; }
                Placa placa = lee(new String(p.getData(), 0, p.getLength(), StandardCharsets.UTF_8));
                if (placa != null) placas.put(placa.id, placa);
            }
        }
        return new ArrayList<>(placas.values());
    }

    private static Placa lee(String texto) {
        try {
            Map<String, Object> m = Json.parseFlatObject(texto.trim());
            if (!"FIND_REPLY".equals(Json.getString(m, "type", ""))) return null;
            return new Placa(Json.getString(m, "name", "?"), Json.getString(m, "micro", "?"), Json.getString(m, "uniqueId", ""),
                             Json.getString(m, "ip", ""), (int) Json.getLong(m, "port", PUERTO));
        } catch (RuntimeException ruido) {
            return null;     // lo que no se entiende no es una placa
        }
    }

    /** Para probarlo sin el IDE: las placas que contestan. */
    public static void main(String[] args) throws IOException {
        List<Placa> hay = buscar(1500);
        for (Placa p : hay) System.out.println(p + "  " + p.id);
        System.out.println(hay.size() + " placa(s)");
    }
}
