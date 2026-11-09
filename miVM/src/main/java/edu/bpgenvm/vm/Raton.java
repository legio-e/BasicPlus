/*
 * Raton.java — V8/H2, paso 3 (9-nov): EL RATÓN DEL SO, para la clase Mice.Mouse (V8_IDEAS §E1, «El paso 3»). Como el
 * teclado, en dos niveles (Eduardo, 8-nov): uno del SO, con su posición y sus botones, que junta todos los ratones
 * conectados, y los del programa, uno o varios.
 *
 * El espejo de bpvm_raton.c (allí, el porqué entero): la posición, en los puntos de la pantalla, y los botones, con los
 * bits del informe HID, tal como están (lo que dan getX, getY e isPressed); y los avisos de los botones y de la rueda que
 * faltan por repartir, cada uno con el punto donde pasó. Moverse no deja aviso: cada ratón apuntado recuerda la última
 * posición que se le avisó, y si la del SO es otra, le toca un onMove (sólo la última: un ratón Bluetooth manda unos 100
 * informes por segundo). El reparto lo hace la VM entre turno y turno (VirtualMachine.repartirRaton), después del de las
 * teclas: las dos VMs tienen que repartir lo mismo y en el mismo orden.
 */
package edu.bpgenvm.vm;

final class Raton {
    // Los botones: los bits del informe de un ratón HID (los valores del enum Mice.Button).
    static final int IZQ = 1, DER = 2, CENTRAL = 4, ATRAS = 8, ADELANTE = 16, BOTONES = 31;
    // El tipo de cada aviso: el primer argumento de __despacha.
    static final int PULSA = 1, MUEVE = 2, RUEDA = 3;

    static final int AVISOS = 16, OBJETOS = 8;   // los de bpvm_raton.h

    /** Un Mice.Mouse apuntado: su referencia entera (la generación dice si sigue vivo; no lo mantiene vivo), el hilo que
     *  lo creó, la ranura de su __despacha, el número del siguiente aviso que le toca y la última posición que se le
     *  avisó. */
    static final class Apuntado {
        long ref;
        int tid, slot;
        long sig;
        int ux, uy;
        boolean enUso;
    }

    int x, y, botones;   // tal como están
    final int[] avTipo = new int[AVISOS], avDato = new int[AVISOS], avX = new int[AVISOS], avY = new int[AVISOS];
    long avisosFin;      // el número del siguiente aviso que llegue
    final Apuntado[] ap = new Apuntado[OBJETOS];

    Raton() {
        for (int i = 0; i < OBJETOS; i++) ap[i] = new Apuntado();
    }

    private void aviso(int tipo, int dato) {
        int i = (int) (avisosFin % AVISOS);
        avTipo[i] = tipo;
        avDato[i] = dato;
        avX[i] = x;
        avY[i] = y;
        avisosFin++;
    }

    /** Lo que manda un ratón, con el punto en vez del desplazamiento (el PC y las pruebas): la posición; un aviso por cada
     *  botón que se aprieta ahora, del bit bajo al alto (soltar no avisa, de momento); y la rueda, en muescas. */
    synchronized void pon(int nx, int ny, int nb, int rueda) {
        x = nx;
        y = ny;
        nb &= BOTONES;
        int nuevos = nb & ~botones;
        for (int b = IZQ; b <= ADELANTE; b <<= 1) if ((nuevos & b) != 0) aviso(PULSA, b);
        botones = nb;
        if (rueda != 0) aviso(RUEDA, rueda);
    }

    // ── Preguntando: tal como está ──────────────────────────────────────────────────────────────────────────────────

    synchronized int x() { return x; }
    synchronized int y() { return y; }
    synchronized int botones() { return botones; }

    // ── Los ratones del programa ───────────────────────────────────────────────────────────────────────────────────

    /** true, o false si no caben más. Desde ahora le llegan los avisos nuevos, y está donde está el del SO. */
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
        a.sig = avisosFin;
        a.ux = x;
        a.uy = y;
        a.enUso = true;
        return true;
    }

    synchronized void borra(long ref) {
        for (Apuntado a : ap) if (a.enUso && a.ref == ref) a.enUso = false;
    }

    /** Al acabar un programa: fuera lo suyo (sus ratones y lo que no se avisó). La posición y los botones son del ratón,
     *  no del programa: se quedan. */
    synchronized void finDeRun() {
        java.util.Arrays.fill(avTipo, 0);
        java.util.Arrays.fill(avDato, 0);
        java.util.Arrays.fill(avX, 0);
        java.util.Arrays.fill(avY, 0);
        avisosFin = 0;
        for (Apuntado a : ap) { a.ref = 0; a.tid = 0; a.slot = 0; a.sig = 0; a.ux = 0; a.uy = 0; a.enUso = false; }
    }
}
