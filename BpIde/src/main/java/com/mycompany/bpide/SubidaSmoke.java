// ============================================================
// SubidaSmoke.java — V7/H8 #495 (paso 3): la subida del IDE (BpvmClient.uploadFile)
// contra una placa o el micro simulado.
//
//   1. el HELLO: el trozo y la ventana que anuncia la placa;
//   2. tres subidas —de una pieza (300 B), por trozos justo pasado el umbral
//      (8 KB + 1) y una grande (KB)—, cada una bajada y comparada;
//   3. EL SEGUNDO INTENTO (Eduardo, 1-oct: «que lo haga el IDE»): la subida grande
//      otra vez con un trozo recortado 16 bytes —un cable que pierde bytes—; tiene
//      que llegar igual, y por el segundo intento;
//   4. borra lo subido.
//
// Uso:
//   java -cp BpIde/target/BpIde-6.0.jar com.mycompany.bpide.SubidaSmoke <COMx | host:puerto> [KB]
// Sale 0 si todo va, 1 si algo falla.
// ============================================================
package com.mycompany.bpide;

import java.util.Arrays;
import java.util.Random;

public final class SubidaSmoke {

    private static int fallos = 0;

    private static void mira(boolean cond, String que) {
        System.out.println((cond ? "  ok   : " : "  FALLO: ") + que);
        if (!cond) fallos++;
    }

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("uso: SubidaSmoke <COMx | host:puerto> [KB]");
            System.exit(2);
        }
        String destino = args[0];
        int kb = args.length > 1 ? Integer.parseInt(args[1]) : 64;
        final String ruta = "/app/subida_smoke.bin";
        try (BpvmClient c = new BpvmClient()) {
            c.setDiagSink(s -> System.out.println("  · " + s));
            int dos = destino.lastIndexOf(':');
            if (dos > 0 && !destino.toUpperCase().startsWith("COM")) {
                c.connectRemote(destino.substring(0, dos), Integer.parseInt(destino.substring(dos + 1)));
            } else {
                c.connectSerial(destino, 115200);
            }
            mira(c.putTrozoAnunciado() > 0 && c.putVentanaAnunciada() > 0,
                 "el HELLO anuncia el trozo y la ventana (" + c.putTrozoAnunciado() + " B, "
                 + c.putVentanaAnunciada() + ")");
            Random rnd = new Random(495);
            for (int tam : new int[] { 300, 8 * 1024 + 1, kb * 1024 }) {
                byte[] d = new byte[tam];
                rnd.nextBytes(d);
                long t0 = System.nanoTime();
                int dijo = c.uploadFile(ruta, d, 10_000);
                double s = (System.nanoTime() - t0) / 1e9;
                byte[] vuelta = c.downloadFile(ruta, 30_000);
                mira(dijo == tam && Arrays.equals(d, vuelta),
                     String.format("%d B subidos en %.2f s y bajados iguales", tam, s));
            }
            int antes = c.segundosIntentos();
            byte[] d = new byte[kb * 1024];
            rnd.nextBytes(d);
            BpvmClient.pruebaRecortarTrozo = 5;
            long t0 = System.nanoTime();
            int dijo = c.uploadFile(ruta, d, 10_000);
            double s = (System.nanoTime() - t0) / 1e9;
            byte[] vuelta = c.downloadFile(ruta, 30_000);
            mira(c.segundosIntentos() == antes + 1 && BpvmClient.pruebaRecortarTrozo == -1,
                 "un trozo recortado 16 bytes (un cable que pierde bytes): hizo falta el segundo intento");
            mira(dijo == d.length && Arrays.equals(d, vuelta),
                 String.format("... y el fichero llegó entero (%.2f s con la espera del cable)", s));
            c.deleteFile(ruta, 10_000);
        }
        System.out.println(fallos == 0 ? "[status=OK]" : "[status=FAIL]");
        System.exit(fallos == 0 ? 0 : 1);
    }
}
