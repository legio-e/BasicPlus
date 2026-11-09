/*
 * RatonSwing.java — V8/H2, paso 3 (9-nov): EL RATÓN DE LA VENTANA DE SWING, para el ratón del SO de la VM (Raton.java;
 * V8_IDEAS §E1, «El paso 3»). Lo mismo que hace la VM-C con los eventos de ratón de SDL (gui_display_sdl.c):
 * - el punto, en la pantalla (el panel raíz: en la VM-Java la pantalla no gira, cambia de aspecto) y dentro de ella:
 *   arrastrando fuera de la ventana, se queda en el borde;
 * - los botones, con los bits del informe HID (1 izquierdo, 2 derecho, 4 central, 8 atrás, 16 adelante), de los
 *   modificadores extendidos del evento, que son los botones tal como quedan después de él;
 * - la rueda, en muescas, +1 hacia arriba (Swing la da al revés), sumando las fracciones de las ruedas finas.
 * Quien lo recibe (la VM) mira si hay un ratón conectado.
 */
package edu.bpgenvm.gui;

import java.awt.Point;
import java.awt.event.InputEvent;
import java.awt.event.MouseEvent;
import java.awt.event.MouseWheelEvent;
import javax.swing.JComponent;
import javax.swing.SwingUtilities;

final class RatonSwing {
    private static final int B4 = mascara(4), B5 = mascara(5);   // los laterales (atrás y adelante), si los hay
    private double rueda;   // lo que queda de las fracciones de muesca

    private static int mascara(int boton) {
        try { return InputEvent.getMaskForButton(boton); }
        catch (IllegalArgumentException e) { return 0; }
    }

    static int botones(int ex) {
        int b = 0;
        if ((ex & InputEvent.BUTTON1_DOWN_MASK) != 0) b |= 1;
        if ((ex & InputEvent.BUTTON3_DOWN_MASK) != 0) b |= 2;
        if ((ex & InputEvent.BUTTON2_DOWN_MASK) != 0) b |= 4;
        if (B4 != 0 && (ex & B4) != 0) b |= 8;
        if (B5 != 0 && (ex & B5) != 0) b |= 16;
        return b;
    }

    private static int dentro(int v, int max) { return v < 0 ? 0 : (v >= max ? max - 1 : v); }

    /** Un evento de ratón de la ventana, en el EDT: moverse, arrastrar, apretar, soltar y la rueda (entrar, salir y el
     *  clic de Swing no dicen nada nuevo). */
    void evento(MouseEvent e, JComponent pantalla, int w, int h, GuiBackend.RatonPC r) {
        int id = e.getID();
        if (id == MouseEvent.MOUSE_ENTERED || id == MouseEvent.MOUSE_EXITED || id == MouseEvent.MOUSE_CLICKED) return;
        if (w <= 0 || h <= 0) return;
        Point p = SwingUtilities.convertPoint(e.getComponent(), e.getPoint(), pantalla);
        int d = 0;
        if (e instanceof MouseWheelEvent) {
            rueda -= ((MouseWheelEvent) e).getPreciseWheelRotation();
            d = (int) rueda;   // las muescas enteras; la fracción espera a la siguiente
            rueda -= d;
            if (d == 0) return;
        }
        r.raton(dentro(p.x, w), dentro(p.y, h), botones(e.getModifiersEx()), d);
    }

    /** Al perder el foco o cerrar: las fracciones de la rueda, fuera. */
    void olvida() { rueda = 0; }
}
