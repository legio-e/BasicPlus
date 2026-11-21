package edu.bpgenvm.vm;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.SourceDataLine;

/**
 * V8/H4 (27-oct, #708) — EL SONIDO en la VM-Java: el espejo de {@code bpvm_sonido.c} (lo común de la HAL BP) y de
 * {@code sonido_host.c} (el audio del PC). Eduardo: «la reproducción de sonidos debería ser siempre en paralelo al
 * resto» y «Que en el PC suene, si».
 *
 * <p>Lo mismo que la VM-C en el QUÉ: la cabecera del WAV con las mismas reglas y en el mismo orden (los textos de los
 * errores salen iguales), una tarea que toca a trozos de unos 50 ms mientras la VM sigue, uno a la vez (la GENERACIÓN:
 * cada tocar y cada parar la suben), {@code sonando} a 1 desde el propio tocar, el progreso en bytes del sonido, el
 * volumen de 0 a 100 como en la P4 (de -50 a 0 dB, medio dB por paso) y el fin de RUN. En el CÓMO, lo de Java: el
 * fichero entero en memoria y {@code javax.sound}, que viene con Java. Con {@code BPVM_SONIDO=mudo} en el entorno, en
 * silencio y al mismo ritmo (las pruebas de la red).
 */
final class Sonido {

    static final int OK = 0, NO_ABRE = 2, NO_ES_WAV = 3, FORMATO = 4;   /* los de bpvm_sonido.h */
    static final int VOLUMEN_DEFECTO = 85;

    private static final int TROZO_MAX = 4096;
    private static final int VUELTAS_MAX = 64;
    private static final int ESPERA_FIN_MS = 1000;
    private static final int NADA = 0, TOCAR = 1, PARAR = 2;

    private static final class Wav {
        byte[] f;
        String ruta;
        long hz;
        int canales, bits;
        long off, len;
    }

    private static final Object cerrojo = new Object();
    private static Thread hilo;
    private static int orden = NADA;
    private static long gen;
    private static boolean sonando, tocando;
    private static volatile int volumen = VOLUMEN_DEFECTO;
    private static Wav pedido;
    private static long pos, largo;

    private Sonido() { }

    private static boolean mudo() { return "mudo".equals(System.getenv("BPVM_SONIDO")); }

    private static int disponibles(byte[] f, long off, int n) {
        if (off < 0 || off >= f.length) return 0;
        return (int) Math.min(n, f.length - off);
    }
    private static long le32(byte[] f, long o) {
        int i = (int) o;
        return (f[i] & 0xFFL) | ((f[i + 1] & 0xFFL) << 8) | ((f[i + 2] & 0xFFL) << 16) | ((f[i + 3] & 0xFFL) << 24);
    }
    private static int le16(byte[] f, long o) {
        int i = (int) o;
        return (f[i] & 0xFF) | ((f[i + 1] & 0xFF) << 8);
    }
    private static boolean es(byte[] f, long o, String s) {
        for (int k = 0; k < 4; k++) if (f[(int) o + k] != (byte) s.charAt(k)) return false;
        return true;
    }

    /** La cabecera: leer_cabecera de la VM-C, regla a regla y en el mismo orden. */
    private static int leerCabecera(Wav w, StringBuilder det) {
        byte[] f = w.f;
        long off = 12;
        boolean fmtVisto = false, dataVisto = false;
        int formato = 0;
        if (f.length < 12 || !es(f, 0, "RIFF") || !es(f, 8, "WAVE")) return NO_ES_WAV;
        for (int vuelta = 0; vuelta < VUELTAS_MAX && !dataVisto; vuelta++) {
            if (disponibles(f, off, 8) < 8) return NO_ES_WAV;
            long tam = le32(f, off + 4);
            if (es(f, off, "fmt ")) {
                if (tam < 16 || disponibles(f, off + 8, 16) < 16) return NO_ES_WAV;
                formato   = le16(f, off + 8);
                w.canales = le16(f, off + 10);
                w.hz      = le32(f, off + 12);
                w.bits    = le16(f, off + 22);
                fmtVisto  = true;
            } else if (es(f, off, "data")) {
                if (!fmtVisto) return NO_ES_WAV;
                w.off = off + 8;
                w.len = tam;
                dataVisto = true;
                break;
            }
            long sig = (off + 8 + tam + (tam & 1)) & 0xFFFFFFFFL;   /* en 32 bits, como la VM-C */
            if (sig <= off) return NO_ES_WAV;
            off = sig;
        }
        if (!dataVisto) return NO_ES_WAV;
        if (formato != 1)                   { det.append("formato ").append(formato).append(", no es PCM"); return FORMATO; }
        if (w.bits != 8 && w.bits != 16)    { det.append(w.bits).append(" bits"); return FORMATO; }
        if (w.canales < 1 || w.canales > 2) { det.append(w.canales).append(" canales"); return FORMATO; }
        if (w.hz < 8000 || w.hz > 48000)    { det.append(w.hz).append(" Hz"); return FORMATO; }
        return OK;
    }

    /** Toca el WAV que trae {@code f} y vuelve en seguida (bpvm_sonido_tocar). */
    static int tocar(byte[] f, String ruta, StringBuilder det) {
        Wav w = new Wav();
        w.f = f;
        w.ruta = ruta;
        int r = leerCabecera(w, det);
        if (r != OK) return r;
        long marco = (long) w.canales * (w.bits / 8);
        long tam = f.length;   /* lo que de verdad trae, como el stat de la VM-C */
        w.len = (tam > w.off) ? Math.min(tam - w.off, w.len) : 0;
        if (w.len > 0x7FFFFFFFL) w.len = 0x7FFFFFFFL;
        w.len -= w.len % marco;
        synchronized (cerrojo) {
            if (hilo == null) {
                hilo = new Thread(Sonido::tarea, "bpvm-sonido");
                hilo.setDaemon(true);
                hilo.start();
            }
            gen++;
            pedido = w;
            orden = TOCAR;
            sonando = true;
            pos = 0;
            largo = w.len;
            cerrojo.notifyAll();
        }
        return OK;
    }

    static void parar() {
        synchronized (cerrojo) {
            if (hilo == null) return;
            gen++;
            orden = PARAR;
            sonando = false;
            cerrojo.notifyAll();
        }
    }

    static boolean sonando() { synchronized (cerrojo) { return hilo != null && sonando; } }
    static int posicion()    { synchronized (cerrojo) { return (int) pos; } }
    static int largo()       { synchronized (cerrojo) { return (int) largo; } }
    static void ponerVolumen(int v) { volumen = v; }
    static int volumen()     { return volumen; }
    static boolean hay()     { return true; }   /* en el PC hay sonido (o el de en silencio) */

    /** El fin de RUN (bpvm_sonido_fin_de_run): lo que suene se para, el progreso a 0 y el volumen a 85. */
    static void finDeRun() {
        volumen = VOLUMEN_DEFECTO;
        synchronized (cerrojo) {
            if (hilo == null) return;
            gen++;
            orden = PARAR;
            sonando = false;
            pos = 0;
            largo = 0;
            cerrojo.notifyAll();
            long t0 = System.currentTimeMillis();
            while ((tocando || orden != NADA) && System.currentTimeMillis() - t0 < ESPERA_FIN_MS) {
                try { cerrojo.wait(50); } catch (InterruptedException e) { break; }
            }
        }
    }

    private static void tarea() {
        for (;;) {
            Wav w;
            long g;
            int o;
            synchronized (cerrojo) {
                while (orden == NADA) {
                    try { cerrojo.wait(); } catch (InterruptedException e) { return; }
                }
                o = orden;
                orden = NADA;
                g = gen;
                w = pedido;
                tocando = (o == TOCAR);
            }
            if (o == TOCAR) tocarWav(w, g);
            synchronized (cerrojo) {
                tocando = false;
                if (o == TOCAR && gen == g) sonando = false;   /* acabó, y nadie ha pedido otro entretanto */
                cerrojo.notifyAll();
            }
        }
    }

    /** El PC, MÁS BAJO que la P4 (Eduardo: «Si reproduces un sonido en el PC al 85% vas a dejar sordo a más de uno»): la
     *  misma escala, desplazada hacia abajo 20 dB, ELEGIDOS DE OÍDO por Eduardo el 27-oct (el canon a 85 con 40, 30 y 20 dB
     *  menos: «la 3»). El espejo, W_ATENUACION_DB (sonido_host.c). */
    private static final double ATENUACION_PC_DB = 20.0;

    private static int ganancia(int v) {   /* Q15, de -50 a 0 dB como en la P4, y lo del PC; 0, nada */
        return (v <= 0) ? 0 : (int) (32768.0 * Math.pow(10.0, ((v - 100) / 2.0 - ATENUACION_PC_DB) / 20.0));
    }

    private static void tocarWav(Wav w, long g) {
        int marco = w.canales * (w.bits / 8);
        int trozo = (int) (w.hz / 20) * marco;   /* unos 50 ms */
        if (trozo > TROZO_MAX) trozo = TROZO_MAX - (TROZO_MAX % marco);
        boolean mudo = mudo();
        SourceDataLine linea = null;
        if (!mudo) {
            try {
                AudioFormat af = new AudioFormat((float) w.hz, 16, w.canales, true, false);
                linea = AudioSystem.getSourceDataLine(af);
                linea.open(af, (int) (w.hz * w.canales * 2 / 5));   /* unos 200 ms en cola, como los cuatro búferes de winmm */
                linea.start();
            } catch (Exception e) {
                System.err.println("[sonido] '" + w.ruta + "': javax.sound no abre (" + e.getMessage() + ")");
                if (linea != null) linea.close();
                return;
            }
        }
        long t0 = System.currentTimeMillis(), marcos = 0, hechos = 0;
        int vol = volumen, gan = ganancia(vol);
        boolean drenar = true;
        byte[] out = new byte[TROZO_MAX * 2];
        while (hechos < w.len) {
            int v;
            boolean otra;
            synchronized (cerrojo) { otra = (gen != g); v = volumen; }
            if (otra) { drenar = false; break; }
            if (v != vol) { vol = v; gan = ganancia(vol); }
            int r = disponibles(w.f, w.off + hechos, (int) Math.min(w.len - hechos, trozo));
            r -= r % marco;
            if (r <= 0) break;
            int n = 0, base = (int) (w.off + hechos);
            for (int i = 0; i < r; i += w.bits / 8) {
                int s = (w.bits == 8) ? ((w.f[base + i] & 0xFF) - 128) * 256
                                      : (short) ((w.f[base + i] & 0xFF) | (w.f[base + i + 1] << 8));
                s = (s * gan) >> 15;
                out[n++] = (byte) s;
                out[n++] = (byte) (s >> 8);
            }
            if (mudo) {
                marcos += r / marco;
                long objetivo = t0 + marcos * 1000 / w.hz, ahora = System.currentTimeMillis();
                if (objetivo > ahora) {
                    try { Thread.sleep(objetivo - ahora); } catch (InterruptedException e) { drenar = false; break; }
                }
            } else {
                linea.write(out, 0, n);
            }
            hechos += r;
            synchronized (cerrojo) { if (gen == g) pos = hechos; }
        }
        if (linea != null) {
            if (drenar) linea.drain(); else linea.flush();
            linea.stop();
            linea.close();
        }
    }
}
