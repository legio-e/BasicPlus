package basicplus.frontend;

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.util.Arrays;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

/**
 * V8 #817 (13-nov) — LA RAM FIJA DE UN MOTOR NATIVO, POR DIRECCIÓN. El empaquetador ({@link NpackReloc#desdeElf})
 * sumaba los tamaños de las secciones que van a cero, y el enlazador las alinea: el motor de la GUI del 12-nov ocupaba
 * 4 B más de los que decía, la placa puso en ellos la RAM del pack siguiente (el driver de la pantalla de la C5), y el
 * driver pisó el final del montón de LVGL al arrancar. Los casos son las secciones de los motores de verdad, leídas con
 * {@code readelf -S} de sus {@code .elf} (que no se versionan); con la suma, el primero y el segundo dan de menos.
 */
class NpackRamTest {

    private static final int UMBRAL = 0x20000000;

    private static Elf32.Section sec(String nombre, int addr, int size) {
        Elf32.Section s = new Elf32.Section();
        s.name = nombre;
        s.addr = addr;
        s.size = size;
        return s;
    }

    @Test
    @DisplayName("el motor de la GUI del 12-nov: el hueco de 4 B entre .sbss y .bss cuenta")
    void elHuecoEntreSbssYBssCuenta() {
        /* lvgl_RV32IMC.elf: .data@0+0x50, .sdata@0x50+0x80, .sbss@0xd0+0x7c, .bss@0x150+0x5fda4 (alineada a 16). */
        int bss = NpackReloc.bytesACero(UMBRAL, 0xd0, Arrays.asList(
                sec(".data", 0x20000000, 0x50), sec(".sdata", 0x20000050, 0x80),
                sec(".sbss", 0x200000d0, 0x7c), sec(".bss", 0x20000150, 0x5fda4)));
        assertEquals(0x5fef4 - 0xd0, bss, "hasta el final de .bss, 0x2005fef4");
        assertEquals(0x7c + 0x5fda4 + 4, bss, "la suma de los tamaños se queda 4 B corta");
    }

    @Test
    @DisplayName("el de la mini (la C6): un hueco de 12 B")
    void laMiniTieneDoce() {
        /* lvgl_mini_RV32IMC.elf: .sdata acaba en 0xc0, .sbss@0xc0+0x54 (acaba en 0x114), .bss@0x120+0x204e0. */
        int bss = NpackReloc.bytesACero(UMBRAL, 0xc0, Arrays.asList(
                sec(".data", 0x20000000, 0x50), sec(".sdata", 0x20000050, 0x70),
                sec(".sbss", 0x200000c0, 0x54), sec(".bss", 0x20000120, 0x204e0)));
        assertEquals(0x54 + 0x204e0 + 12, bss);
    }

    @Test
    @DisplayName("sin hueco, como antes: el driver de la pantalla SPI")
    void sinHuecoComoAntes() {
        /* driver_spi.RV32IMC.elf: .sdata@0+4, .sbss@4+0x34, .bss@0x38+0x3a0. */
        int bss = NpackReloc.bytesACero(UMBRAL, 4, Arrays.asList(
                sec(".sdata", 0x20000000, 4), sec(".sbss", 0x20000004, 0x34), sec(".bss", 0x20000038, 0x3a0)));
        assertEquals(0x34 + 0x3a0, bss);
    }

    @Test
    @DisplayName("sin datos: lo que va a cero empieza en el umbral")
    void sinDatos() {
        /* driver_c5.RV32IMC.elf: sólo .sbss@0+8; y las secciones que no hay llegan como null. */
        int bss = NpackReloc.bytesACero(UMBRAL, 0, Arrays.asList(null, sec(".sbss", 0x20000000, 8), null));
        assertEquals(8, bss);
    }

    @Test
    @DisplayName("nada que vaya a cero: 0")
    void nadaACero() {
        int bss = NpackReloc.bytesACero(UMBRAL, 0x10, Arrays.asList(sec(".data", 0x20000000, 0x10)));
        assertEquals(0, bss);
    }
}
