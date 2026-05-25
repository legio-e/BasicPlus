// ============================================================
// WireFraming.java
// Primitivos de framing del wire BPVM v1: líneas JSON UTF-8
// terminadas en `\n` + bulk binario raw inline.
//
// La capa que está justo encima (DebugServer / BpvmClient) lee así:
//   1) recvLine(in) → String JSON (sin `\n`).
//   2) Si el JSON tiene `"bulk":N`, recvBulk(in, N) → N bytes raw.
//   3) Vuelve a 1.
//
// Y escribe así:
//   1) sendLine(out, jsonLine) — escribe jsonLine + `\n`.
//   2) Si el mensaje lleva bulk, sendBulk(out, bytes) inmediatamente
//      detrás. flush() en el out lo asegura atómico para el peer.
//
// Sin nada de smart parsing — son funciones puras sobre streams.
// El control de concurrencia (sincronización de escrituras, dueño
// del read loop) es responsabilidad del caller.
// ============================================================
package edu.bpgenvm.util;

import java.io.ByteArrayOutputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

public final class WireFraming {
    private WireFraming() {}

    /**
     * Lee bytes hasta encontrar `\n` (0x0A). Devuelve los bytes intermedios
     * decodificados como UTF-8 (sin el `\n`). Los `\r` (0x0D) se descartan
     * silenciosamente para tolerar peers que escriben CRLF (algunos
     * PrintWriter en Windows). Devuelve null si el stream cierra antes
     * de cualquier byte.
     *
     * Los `\r` dentro de strings JSON ya van escapados como `\\r` (dos
     * bytes: 0x5C 0x72), así que el filtro de 0x0D no toca contenido.
     */
    public static String recvLine(InputStream in) throws IOException {
        ByteArrayOutputStream buf = new ByteArrayOutputStream(256);
        int b;
        boolean sawAny = false;
        while ((b = in.read()) != -1) {
            sawAny = true;
            if (b == '\n') {
                return new String(buf.toByteArray(), StandardCharsets.UTF_8);
            }
            if (b == '\r') continue;   // tolerar CRLF
            buf.write(b);
        }
        if (!sawAny) return null;
        // EOF sin newline final pero con datos: devolvemos lo que hay
        // (los protocolos line-delimited reales no deberían dejar
        // un sufijo sin terminar, pero ser tolerante no cuesta).
        return new String(buf.toByteArray(), StandardCharsets.UTF_8);
    }

    /**
     * Lee EXACTAMENTE `n` bytes del stream. Si el stream se cierra antes
     * de leer todos, lanza EOFException. n debe ser >= 0; si es 0
     * devuelve array vacío sin leer nada.
     */
    public static byte[] recvBulk(InputStream in, int n) throws IOException {
        if (n < 0) throw new IllegalArgumentException("bulk size negativo: " + n);
        if (n == 0) return new byte[0];
        byte[] buf = new byte[n];
        int got = 0;
        while (got < n) {
            int r = in.read(buf, got, n - got);
            if (r < 0) {
                throw new EOFException("bulk EOF: leídos " + got + " de " + n + " bytes esperados");
            }
            got += r;
        }
        return buf;
    }

    /**
     * Escribe `jsonLine` (UTF-8) + `\n`. NO flush — el caller lo hace
     * tras enviar también el bulk si toca (ver sendBulk).
     */
    public static void sendLine(OutputStream out, String jsonLine) throws IOException {
        out.write(jsonLine.getBytes(StandardCharsets.UTF_8));
        out.write('\n');
    }

    /**
     * Escribe `bytes` raw al stream. NO flush — el caller lo hace tras
     * el último write del mensaje.
     */
    public static void sendBulk(OutputStream out, byte[] bytes) throws IOException {
        if (bytes != null && bytes.length > 0) {
            out.write(bytes);
        }
    }
}
