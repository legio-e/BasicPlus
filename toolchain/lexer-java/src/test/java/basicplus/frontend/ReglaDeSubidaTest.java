package basicplus.frontend;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * V7/H9 #624 (2-oct) — si se sube una librería a la placa, con la regla de Eduardo: formato compatible y versión igual o
 * superior a la mínima (la de compilación). El CRC sólo dice «idéntico».
 */
class ReglaDeSubidaTest {

    private static final long CRC = 0x12345678L;

    private static ReglaDeSubida.Decision d(String fPlaca, long vPlaca, long crcPlaca) {
        return ReglaDeSubida.decidir(fPlaca, vPlaca, crcPlaca, "MOD9", 7, CRC);
    }

    @Test
    @DisplayName("no la tiene → se sube")
    void noLaTiene() {
        assertTrue(d(null, -1, -1).subir);
    }

    @Test
    @DisplayName("idéntica → no se sube")
    void identica() {
        ReglaDeSubida.Decision x = d("MOD9", 7, CRC);
        assertFalse(x.subir);
        assertTrue(x.motivo.contains("idéntico"), x.motivo);
    }

    @Test
    @DisplayName("otro formato → incompatible: se sube")
    void otroFormato() {
        assertTrue(d("MOD8", 7, 1).subir);
    }

    @Test
    @DisplayName("la placa no dice la versión (firmware anterior) y el CRC es otro → se sube (la regla de antes)")
    void sinVersion() {
        assertTrue(d("MOD9", -1, 1).subir);
    }

    @Test
    @DisplayName("versión anterior a la mínima → se sube")
    void anterior() {
        assertTrue(d("MOD9", 6, 1).subir);
    }

    @Test
    @DisplayName("la misma versión, otra compilación → VALE: no se sube (#623: era lo que subía a /lib)")
    void mismaVersionOtroCrc() {
        ReglaDeSubida.Decision x = d("MOD9", 7, 1);
        assertFalse(x.subir, x.motivo);
        assertTrue(x.motivo.contains("vale"), x.motivo);
    }

    @Test
    @DisplayName("versión posterior a la mínima → vale: no se sube")
    void posterior() {
        assertFalse(d("MOD9", 8, 1).subir);
    }

    @Test
    @DisplayName("formato y versión, de la cabecera de un .mod (MAGIC, gen en +36)")
    void cabecera() {
        java.nio.ByteBuffer b = java.nio.ByteBuffer.allocate(48);
        b.put(new byte[] {'M', 'O', 'D', '9'}).position(36);
        b.putInt(7);
        byte[] mod9 = b.array();
        assertEquals("MOD9", ReglaDeSubida.formatoDe(mod9));
        assertEquals(7, ReglaDeSubida.versionDe(mod9));
        byte[] mod7 = mod9.clone();
        mod7[3] = '7';
        assertEquals("MOD7", ReglaDeSubida.formatoDe(mod7));
        assertEquals(-1, ReglaDeSubida.versionDe(mod7), "MOD7 no lleva versión");
        assertNull(ReglaDeSubida.formatoDe(new byte[] {1, 2, 3, 4}));
    }
}
