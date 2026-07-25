// ============================================================
// SimIcons.java
// H10 — iconos del micro simulado, DIBUJADOS (Java2D), no cargados
// de fichero: así el jar sigue siendo autocontenido, no hay recursos
// que empaquetar ni rutas que se pierdan, y se adaptan al color del
// tema (usan el foreground del componente).
//
//   chip(size)  → un micro visto desde arriba, con sus patillas.
//   gear(size)  → el engranaje de "configuración" de toda la vida.
// ============================================================
package com.mycompany.bpide;

import javax.swing.Icon;
import java.awt.*;
import java.awt.geom.*;

public final class SimIcons {

    private SimIcons() { }

    public static Icon chip(int size)      { return new ChipIcon(size, null); }
    /** Variante coloreada (p.ej. verde = simulador en marcha). */
    public static Icon chip(int size, Color c) { return new ChipIcon(size, c); }
    public static Icon gear(int size)      { return new GearIcon(size); }

    /** Base común: antialias + color del componente si no se fuerza otro. */
    private abstract static class Base implements Icon {
        final int size; final Color forced;
        Base(int size, Color forced) { this.size = size; this.forced = forced; }
        @Override public int getIconWidth()  { return size; }
        @Override public int getIconHeight() { return size; }

        @Override public void paintIcon(Component c, Graphics g, int x, int y) {
            Graphics2D g2 = (Graphics2D) g.create();
            try {
                g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
                                    RenderingHints.VALUE_ANTIALIAS_ON);
                g2.setRenderingHint(RenderingHints.KEY_STROKE_CONTROL,
                                    RenderingHints.VALUE_STROKE_PURE);
                Color col = forced != null ? forced
                          : (c != null && c.getForeground() != null ? c.getForeground() : Color.DARK_GRAY);
                if (c != null && !c.isEnabled()) col = new Color(col.getRed(), col.getGreen(),
                                                                 col.getBlue(), 90);
                g2.setColor(col);
                g2.translate(x, y);
                draw(g2);
            } finally {
                g2.dispose();
            }
        }
        abstract void draw(Graphics2D g2);
    }

    /** Micro visto desde arriba: cuerpo cuadrado con esquinas redondeadas, patillas
     *  a los cuatro lados y la marca del pin 1 (el puntito). */
    private static final class ChipIcon extends Base {
        ChipIcon(int size, Color forced) { super(size, forced); }

        @Override void draw(Graphics2D g2) {
            float s   = size;
            float pin = Math.max(1.5f, s * 0.14f);          // longitud de la patilla
            float pad = pin + s * 0.06f;                    // margen del cuerpo
            float body = s - 2 * pad;
            float stroke = Math.max(1f, s / 14f);
            g2.setStroke(new BasicStroke(stroke, BasicStroke.CAP_BUTT, BasicStroke.JOIN_ROUND));

            /* Patillas: 3 por lado, repartidas sobre el borde del cuerpo. */
            for (int i = 0; i < 3; i++) {
                float t = pad + body * (0.25f + 0.25f * i);
                g2.draw(new Line2D.Float(pad - pin, t, pad, t));                 // izquierda
                g2.draw(new Line2D.Float(pad + body, t, pad + body + pin, t));   // derecha
                g2.draw(new Line2D.Float(t, pad - pin, t, pad));                 // arriba
                g2.draw(new Line2D.Float(t, pad + body, t, pad + body + pin));   // abajo
            }
            /* Cuerpo. */
            g2.draw(new RoundRectangle2D.Float(pad, pad, body, body, s * 0.12f, s * 0.12f));
            /* Marca del pin 1. */
            float d = Math.max(1.5f, s * 0.13f);
            g2.fill(new Ellipse2D.Float(pad + body * 0.18f, pad + body * 0.18f, d, d));
        }
    }

    /** Engranaje: corona de dientes + eje hueco. */
    private static final class GearIcon extends Base {
        GearIcon(int size) { super(size, null); }

        @Override void draw(Graphics2D g2) {
            float c = size / 2f;
            float rOut = size * 0.46f;      // punta del diente
            float rIn  = size * 0.33f;      // valle
            float rHub = size * 0.13f;      // agujero del eje
            int teeth = 8;

            Path2D.Float p = new Path2D.Float();
            int steps = teeth * 4;          // diente = punta-punta-valle-valle
            for (int i = 0; i < steps; i++) {
                double a = (Math.PI * 2 * i) / steps - Math.PI / 2;
                /* patrón 0,1 = radio exterior; 2,3 = interior → dientes cuadrados */
                float r = (i % 4 < 2) ? rOut : rIn;
                float px = c + (float) (Math.cos(a) * r);
                float py = c + (float) (Math.sin(a) * r);
                if (i == 0) p.moveTo(px, py); else p.lineTo(px, py);
            }
            p.closePath();

            Area gear = new Area(p);
            gear.subtract(new Area(new Ellipse2D.Float(c - rHub, c - rHub, rHub * 2, rHub * 2)));
            g2.fill(gear);
        }
    }
}
