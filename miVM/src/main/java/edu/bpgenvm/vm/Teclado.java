/*
 * Teclado.java — V8/H2, paso 2 (9-nov): EL TECLADO DEL SO, para la clase Keyboards.Keyboard (V8_IDEAS §E1, «El paso 2»).
 * Eduardo: «Un teclado a nivel de SO. Un teclado a nivel de programa ( un objeto ). El teclado a nivel de SO es único,
 * con su buffer. El de programa puede ser mas de uno, y se relaciona con el del SO».
 *
 * El espejo de bpvm_teclado.c (allí, el porqué entero): una tecla es un int32 (abajo el código HID, arriba los bits de
 * estado) y su carácter, como código Unicode (0 en una especial). Preguntando, el búfer del SO (el primero que lee se la
 * lleva); con eventos, cada teclado apuntado recibe cada tecla nueva en el hilo que lo creó: el reparto lo hace la VM
 * entre turno y turno (VirtualMachine.repartirTeclas), uno en la cola por teclado como mucho. Las dos VMs tienen que
 * repartir lo mismo y en el mismo orden.
 */
package edu.bpgenvm.vm;

final class Teclado {
    // Los bits de estado, la parte alta del int32: el byte de modificadores del estándar HID y los bloqueos.
    static final int CODIGO = 0x0000FFFF;
    static final int CTRL_IZQ = 1 << 16, SHIFT_IZQ = 1 << 17, ALT = 1 << 18, WIN_IZQ = 1 << 19;
    static final int CTRL_DER = 1 << 20, SHIFT_DER = 1 << 21, ALTGR = 1 << 22, WIN_DER = 1 << 23;
    static final int BLOQ_MAYUS = 1 << 24, BLOQ_NUM = 1 << 25, BLOQ_DESPL = 1 << 26, FN = 1 << 27;

    static final int BUFER = 32, AVISOS = 16, OBJETOS = 8, HILOS = 8;   // los de bpvm_teclado.h

    /** Un Keyboards.Keyboard apuntado: su referencia entera (la generación dice si sigue vivo; no lo mantiene vivo), el
     *  hilo que lo creó, la ranura de su __despacha y el número de la siguiente tecla que le toca. */
    static final class Apuntado {
        long ref;
        int tid, slot;
        long sig;
        boolean enUso;
    }

    private final int[] buferK = new int[BUFER], buferCp = new int[BUFER];
    private int buferIni, buferN;
    final int[] avisosK = new int[AVISOS], avisosCp = new int[AVISOS];
    long avisosFin;   // el número de la siguiente que llegue
    final Apuntado[] ap = new Apuntado[OBJETOS];
    private final int[] ultTid = new int[HILOS], ultCp = new int[HILOS];
    private final boolean[] ultUsado = new boolean[HILOS];
    private int ultSig;
    private boolean pend;   // el PC: una pulsación que espera su texto
    private int pendK, pendCodigo;

    Teclado() {
        for (int i = 0; i < OBJETOS; i++) ap[i] = new Apuntado();
    }

    private void ponEn(int k, int cp) {
        if (buferN < BUFER) {
            buferK[(buferIni + buferN) % BUFER] = k;
            buferCp[(buferIni + buferN) % BUFER] = cp;
            buferN++;
        }   // lleno: la que llega se pierde, como en un BIOS
        avisosK[(int) (avisosFin % AVISOS)] = k;
        avisosCp[(int) (avisosFin % AVISOS)] = cp;
        avisosFin++;
    }

    synchronized void pon(int k, int cp) { ponEn(k, cp); }

    // ── Preguntando ─────────────────────────────────────────────────────────────────────────────────────────────────

    synchronized boolean hay() { return buferN > 0; }

    /** La saca (0 si no hay); su carácter se queda para el hilo `tid` (getChar). */
    synchronized int lee(int tid) {
        int k = 0, cp = 0, j = -1;
        if (buferN > 0) {
            k = buferK[buferIni];
            cp = buferCp[buferIni];
            buferIni = (buferIni + 1) % BUFER;
            buferN--;
        }
        for (int i = 0; i < HILOS; i++) if (ultUsado[i] && ultTid[i] == tid) { j = i; break; }
        if (j < 0) { j = ultSig; ultSig = (ultSig + 1) % HILOS; }
        ultTid[j] = tid;
        ultCp[j] = cp;
        ultUsado[j] = true;
        return k;
    }

    synchronized int caracter(int tid) {
        for (int i = 0; i < HILOS; i++) if (ultUsado[i] && ultTid[i] == tid) return ultCp[i];
        return 0;
    }

    // ── Los teclados del programa ──────────────────────────────────────────────────────────────────────────────────

    /** true, o false si no caben más. */
    synchronized boolean apunta(long ref, int tid, int slot) {
        int libre = -1;
        for (int i = 0; i < OBJETOS; i++) {
            if (ap[i].enUso && ap[i].ref == ref) { libre = i; break; }   // el mismo, otra vez
            if (!ap[i].enUso && libre < 0) libre = i;
        }
        if (libre < 0) return false;
        Apuntado a = ap[libre];
        a.ref = ref;
        a.tid = tid;
        a.slot = slot;
        a.sig = avisosFin;   // las teclas que lleguen desde ahora
        a.enUso = true;
        return true;
    }

    synchronized void borra(long ref) {
        for (Apuntado a : ap) if (a.enUso && a.ref == ref) a.enUso = false;
    }

    /** Al acabar un programa: fuera lo suyo (sus teclados, lo que no leyó y lo que no se avisó). */
    synchronized void finDeRun() {
        buferIni = 0;
        buferN = 0;
        java.util.Arrays.fill(avisosK, 0);
        java.util.Arrays.fill(avisosCp, 0);
        avisosFin = 0;
        for (Apuntado a : ap) { a.ref = 0; a.tid = 0; a.slot = 0; a.sig = 0; a.enUso = false; }
        java.util.Arrays.fill(ultTid, 0);
        java.util.Arrays.fill(ultCp, 0);
        java.util.Arrays.fill(ultUsado, false);
        ultSig = 0;
        pend = false;
        pendK = 0;
        pendCodigo = 0;
    }

    // ── El PC: las teclas de la ventana de la GUI (las de Swing), sólo con un teclado conectado ─────────────────────
    //    Lo mismo que bpvm_teclado_pc_*: una tecla que puede dar carácter espera a su texto; si no llega, sale sin él en
    //    la siguiente pulsación o al soltarla. Con Ctrl, Alt o Windows (no AltGr), sale en seguida.

    private static boolean esModificador(int c) { return c >= 0xE0 && c <= 0xE7; }

    private static boolean puedeDarTexto(int c, int estado) {
        if (c >= 0x59 && c <= 0x63) return (estado & BLOQ_NUM) != 0;
        return (c >= 0x04 && c <= 0x27) || c == 0x2C || (c >= 0x2D && c <= 0x38) || c == 0x64
            || (c >= 0x54 && c <= 0x57) || c == 0x67;
    }

    private static boolean conAtajo(int e) { return (e & (CTRL_IZQ | CTRL_DER | ALT | WIN_IZQ | WIN_DER)) != 0; }

    /** El AltGr de Windows trae un Ctrl izquierdo de más: un teclado de verdad no lo manda. */
    private static int limpia(int e) {
        e &= ~CODIGO;
        if ((e & ALTGR) != 0) e &= ~CTRL_IZQ;
        return e;
    }

    synchronized void pcPulsada(int codigo, int estado) {
        if (codigo <= 0 || codigo > 0xFFFF || esModificador(codigo)) return;
        estado = limpia(estado);
        int k = codigo | estado;
        if (pend) { ponEn(pendK, 0); pend = false; }
        if (puedeDarTexto(codigo, estado) && !conAtajo(estado)) { pend = true; pendK = k; pendCodigo = codigo; }
        else ponEn(k, 0);
    }

    synchronized void pcTexto(int cp, int estado) {
        if (cp <= 0) return;
        if (pend) { ponEn(pendK, cp); pend = false; }
        else ponEn(limpia(estado), cp);   // un carácter sin tecla (lo compone el sistema): código 0
    }

    synchronized void pcSoltada(int codigo) {
        if (pend && pendCodigo == codigo) { ponEn(pendK, 0); pend = false; }
    }
}
