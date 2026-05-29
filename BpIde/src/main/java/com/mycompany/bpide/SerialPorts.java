// ============================================================
// SerialPorts.java
// Helpers estáticos de enumeración / autodetección de puertos
// serie disponibles en la máquina. Aislado en su propio fichero
// para que ni BpvmClient ni los Backend tengan que importar la
// API de purejavacomm cuando solo necesitan listar puertos.
//
// Histórico: estos dos métodos vivían dentro de PicoClient.java
// (cliente legacy del protocolo texto-protocolo del firmware).
// Tras la migración a wire v1 (#150 firmware + #151 IDE), todo
// PicoClient quedó como código muerto excepto estos dos helpers
// estáticos. #137 los extrajo aquí y retiró el resto.
// ============================================================
package com.mycompany.bpide;

import purejavacomm.CommPortIdentifier;

import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

public final class SerialPorts {

    private SerialPorts() {}

    /** Raspberry Pi VID (informativo — purejavacomm 1.0.2 no expone
     *  VID/PID, así que no podemos filtrar por él en runtime). */
    public static final int RPI_VID = 0x2E8A;

    /** Lista los nombres de los puertos serie disponibles ahora mismo
     *  (en Windows típicamente "COM3", "COM4", ...). */
    public static List<String> listPorts() {
        List<String> out = new ArrayList<>();
        @SuppressWarnings("unchecked")
        Enumeration<CommPortIdentifier> e = CommPortIdentifier.getPortIdentifiers();
        while (e.hasMoreElements()) {
            CommPortIdentifier id = e.nextElement();
            if (id.getPortType() == CommPortIdentifier.PORT_SERIAL) {
                out.add(id.getName());
            }
        }
        return out;
    }

    /** Devuelve el primer COM cuya numeración sea &gt;= 4 (los COM1-3 de
     *  Windows suelen ser legacy / módems virtuales); si no hay
     *  ninguno con ese criterio, devuelve el primer puerto detectado;
     *  si la lista está vacía, null.
     *
     *  No es una autodetección semántica (purejavacomm no expone VID/PID)
     *  — el usuario puede sobreescribir desde el combo en la UI. */
    public static String autoDetect() {
        List<String> ports = listPorts();
        for (String p : ports) {
            if (p.startsWith("COM")) {
                try {
                    int n = Integer.parseInt(p.substring(3));
                    if (n >= 4) return p;
                } catch (NumberFormatException ignore) { }
            }
        }
        return ports.isEmpty() ? null : ports.get(0);
    }
}
