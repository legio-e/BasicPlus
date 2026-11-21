/*
 * TeclasSwing.java — V8/H2, paso 2 (9-nov): las teclas de la ventana de Swing, en el idioma del teclado del SO
 * (Teclado.java, en la VM-C bpvm_teclado.h): el código del estándar HID de cada VK_ (el que mandaría un teclado
 * Bluetooth o USB; SDL ya los da así), con su lado y el teclado numérico; los modificadores, en el byte de
 * modificadores HID y los bloqueos; y un aviso por pulsación: la repetición del sistema no cuenta.
 *
 * El carácter no se traduce: es el que da el sistema (KEY_TYPED), con la distribución del teclado del PC (Eduardo: en
 * el PC, el del sistema). Una tecla sin VK_ propio en esa distribución (la ñ de un teclado español) sale con el código
 * 0 y su carácter. Va en el EDT.
 */
package edu.bpgenvm.gui;

import java.awt.event.KeyEvent;

final class TeclasSwing {
    // Los bits de estado (los de Teclado.java y bpvm_teclado.h).
    static final int CTRL_IZQ = 1 << 16, SHIFT_IZQ = 1 << 17, ALT = 1 << 18, WIN_IZQ = 1 << 19;
    static final int CTRL_DER = 1 << 20, SHIFT_DER = 1 << 21, ALTGR = 1 << 22, WIN_DER = 1 << 23;
    static final int BLOQ_MAYUS = 1 << 24, BLOQ_NUM = 1 << 25, BLOQ_DESPL = 1 << 26;

    private int mods;   // los modificadores pulsados, con su lado: Swing dice el lado sólo al pulsarlos
    private final java.util.Set<Integer> abajo = new java.util.HashSet<>();   // las pulsadas, para no repetir

    /** El código HID de una tecla de Swing; 0 si no tiene (una tecla propia de otra distribución). */
    static int hid(int vk, int loc) {
        boolean num = loc == KeyEvent.KEY_LOCATION_NUMPAD;
        if (vk >= KeyEvent.VK_A && vk <= KeyEvent.VK_Z) return 0x04 + (vk - KeyEvent.VK_A);
        if (vk >= KeyEvent.VK_1 && vk <= KeyEvent.VK_9) return 0x1E + (vk - KeyEvent.VK_1);
        if (vk >= KeyEvent.VK_NUMPAD1 && vk <= KeyEvent.VK_NUMPAD9) return 0x59 + (vk - KeyEvent.VK_NUMPAD1);
        if (vk >= KeyEvent.VK_F1 && vk <= KeyEvent.VK_F12) return 0x3A + (vk - KeyEvent.VK_F1);
        if (vk >= KeyEvent.VK_F13 && vk <= KeyEvent.VK_F24) return 0x68 + (vk - KeyEvent.VK_F13);
        switch (vk) {
            case KeyEvent.VK_0:             return 0x27;
            case KeyEvent.VK_ENTER:         return num ? 0x58 : 0x28;
            case KeyEvent.VK_ESCAPE:        return 0x29;
            case KeyEvent.VK_BACK_SPACE:    return 0x2A;
            case KeyEvent.VK_TAB:           return 0x2B;
            case KeyEvent.VK_SPACE:         return 0x2C;
            case KeyEvent.VK_MINUS:         return 0x2D;
            case KeyEvent.VK_EQUALS:        return 0x2E;
            case KeyEvent.VK_OPEN_BRACKET:  return 0x2F;
            case KeyEvent.VK_CLOSE_BRACKET: return 0x30;
            case KeyEvent.VK_BACK_SLASH:    return 0x31;
            case KeyEvent.VK_SEMICOLON:     return 0x33;
            case KeyEvent.VK_QUOTE:         return 0x34;
            case KeyEvent.VK_BACK_QUOTE:    return 0x35;
            case KeyEvent.VK_COMMA:         return 0x36;
            case KeyEvent.VK_PERIOD:        return 0x37;
            case KeyEvent.VK_SLASH:         return 0x38;
            case KeyEvent.VK_CAPS_LOCK:     return 0x39;
            case KeyEvent.VK_PRINTSCREEN:   return 0x46;
            case KeyEvent.VK_SCROLL_LOCK:   return 0x47;
            case KeyEvent.VK_PAUSE:         return 0x48;
            // las de moverse: del teclado numérico (sin Bloq Num), su tecla de allí, como las manda un teclado HID
            case KeyEvent.VK_INSERT:        return num ? 0x62 : 0x49;
            case KeyEvent.VK_HOME:          return num ? 0x5F : 0x4A;
            case KeyEvent.VK_PAGE_UP:       return num ? 0x61 : 0x4B;
            case KeyEvent.VK_DELETE:        return num ? 0x63 : 0x4C;
            case KeyEvent.VK_END:           return num ? 0x59 : 0x4D;
            case KeyEvent.VK_PAGE_DOWN:     return num ? 0x5B : 0x4E;
            case KeyEvent.VK_RIGHT:         return num ? 0x5E : 0x4F;
            case KeyEvent.VK_LEFT:          return num ? 0x5C : 0x50;
            case KeyEvent.VK_DOWN:          return num ? 0x5A : 0x51;
            case KeyEvent.VK_UP:            return num ? 0x60 : 0x52;
            case KeyEvent.VK_KP_RIGHT:      return 0x5E;
            case KeyEvent.VK_KP_LEFT:       return 0x5C;
            case KeyEvent.VK_KP_DOWN:       return 0x5A;
            case KeyEvent.VK_KP_UP:         return 0x60;
            case KeyEvent.VK_CLEAR:         return 0x5D;   // el 5 del teclado numérico, sin Bloq Num
            case KeyEvent.VK_NUM_LOCK:      return 0x53;
            case KeyEvent.VK_DIVIDE:        return 0x54;
            case KeyEvent.VK_MULTIPLY:      return 0x55;
            case KeyEvent.VK_SUBTRACT:      return 0x56;
            case KeyEvent.VK_ADD:           return 0x57;
            case KeyEvent.VK_NUMPAD0:       return 0x62;
            case KeyEvent.VK_DECIMAL:       return 0x63;
            case KeyEvent.VK_LESS:          return 0x64;   // la tecla de más de los teclados europeos (< >)
            case KeyEvent.VK_CONTEXT_MENU:  return 0x65;
            default:                        return 0;
        }
    }

    /** El bit de un modificador, con su lado; 0 si no lo es. */
    private static int modificador(int vk, int loc) {
        boolean der = loc == KeyEvent.KEY_LOCATION_RIGHT;
        switch (vk) {
            case KeyEvent.VK_SHIFT:     return der ? SHIFT_DER : SHIFT_IZQ;
            case KeyEvent.VK_CONTROL:   return der ? CTRL_DER : CTRL_IZQ;
            case KeyEvent.VK_ALT:       return der ? ALTGR : ALT;
            case KeyEvent.VK_ALT_GRAPH: return ALTGR;
            case KeyEvent.VK_WINDOWS:   return der ? WIN_DER : WIN_IZQ;
            case KeyEvent.VK_META:      return der ? WIN_DER : WIN_IZQ;
            default:                    return 0;
        }
    }

    /** Una tecla de Swing, al teclado del SO (por `t`, que mira si hay un teclado conectado). */
    void evento(KeyEvent e, GuiBackend.TecladoPC t) {
        int vk = e.getKeyCode(), loc = e.getKeyLocation();
        switch (e.getID()) {
            case KeyEvent.KEY_PRESSED: {
                int m = modificador(vk, loc);
                if (m != 0) { mods |= m; return; }   // los modificadores, solos, no son teclas
                if (!abajo.add(vk)) return;           // la repetición del sistema: un aviso por pulsación
                t.pulsada(hid(vk, loc), estado(e));
                return;
            }
            case KeyEvent.KEY_RELEASED: {
                int m = modificador(vk, loc);
                if (m != 0) { mods &= ~m; return; }
                abajo.remove(vk);
                t.soltada(hid(vk, loc));
                return;
            }
            case KeyEvent.KEY_TYPED: {
                char ch = e.getKeyChar();
                if (ch == KeyEvent.CHAR_UNDEFINED || ch < 0x20 || ch == 0x7F) return;   // Enter, Tab...: ya salieron
                if (Character.isSurrogate(ch)) return;   // media letra de UTF-16 (un emoji): no es un código Unicode
                t.texto(ch, estado(e));
                return;
            }
            default:
        }
    }

    /** La ventana pierde el foco: lo que se suelte fuera no llega, así que se olvida. */
    void olvida() {
        mods = 0;
        abajo.clear();
    }

    private int estado(KeyEvent e) {
        int m = mods;
        if (!e.isShiftDown()) m &= ~(SHIFT_IZQ | SHIFT_DER);   // lo que diga el evento manda
        if (!e.isControlDown()) m &= ~(CTRL_IZQ | CTRL_DER);
        if (!e.isAltDown()) m &= ~ALT;
        if (e.isAltGraphDown()) m |= ALTGR;
        try {
            java.awt.Toolkit tk = java.awt.Toolkit.getDefaultToolkit();
            if (tk.getLockingKeyState(KeyEvent.VK_CAPS_LOCK)) m |= BLOQ_MAYUS;
            if (tk.getLockingKeyState(KeyEvent.VK_NUM_LOCK)) m |= BLOQ_NUM;
            if (tk.getLockingKeyState(KeyEvent.VK_SCROLL_LOCK)) m |= BLOQ_DESPL;
        } catch (UnsupportedOperationException ex) {
            // un sistema que no lo dice: sin bloqueos
        }
        return m;
    }
}
