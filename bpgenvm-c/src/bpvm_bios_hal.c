/* bpvm_bios_hal.c — las ranuras de HARDWARE de la tabla BIOS (V7/H6, v5).
 *
 * Hasta la v4 la tabla prestaba memoria, cadenas, ficheros y el punto de
 * encuentro: lo que una BASE DE DATOS necesita. Un driver de pantalla en un
 * pack necesita otra cosa —un pin, un bus SPI, un bus I2C, un reloj— y la
 * tabla no prestaba ni un pin. Estas ranuras son eso, y son PORTABLES a
 * propósito: no envuelven el SDK de ninguna familia sino las fachadas que la
 * VM ya tiene (`bpvm_gpio`, `bpvm_spi`, `bpvm_i2c`, `bpvm_platform`), que son
 * las mismas en las cinco familias. Un driver escrito contra esto corre donde
 * corra la VM, y añadir una familia no toca esta tabla.
 *
 * Lo que NO se presta: `esp_lcd`, DMA, LEDC… Una ranura que sólo existe en una
 * familia rompería la propiedad que hace útil la tabla (26 de 29 ranuras eran
 * idénticas en cualquier placa cuando se montó la segunda).
 *
 * ─── EL PIN DE UN PACK SOBREVIVE AL FIN DE RUN ───
 *
 * El pack se carga UNA vez y vive mientras viva la placa; el programa BP no.
 * Al acabar un RUN la VM libera todos los pines (`bpvm_gpio_estado_reset`), y
 * si el pin DC del panel quedara libre, el siguiente programa podría tomarlo
 * con `Pin(9, OUTPUT)` y pelearse con la pantalla en silencio. Por eso
 * `gpio_init` reclama el pin como DE PACK: el fin de RUN no lo toca y un
 * programa que lo pida recibe «el pin 9 lo usa un código nativo», con nombre.
 */
#include "bpvm_bios.h"
#include "bpvm_gpio.h"
#include "bpvm_spi.h"
#include "bpvm_i2c.h"
#include "bpvm_platform.h"
#include "bpvm.h"          /* bpvm_diag: el porqué de un -1 va al log de la VM */

int bpvm_bios_hal_gpio_init(int pin, int mode) {
    /* mode: 0 = INPUT, 1 = OUTPUT, 2 = colector abierto — la numeración de la
     * fachada y de `Gpio.Pin`. La validación es la de H4: contra la tabla de
     * capacidades del micro y el estado de los pines, con el mismo texto. */
    int uso = (mode == 0) ? BPVM_USO_INPUT : (mode == 2) ? BPVM_USO_OD : BPVM_USO_OUTPUT;
    char err[96];
    if (bpvm_gpio_reclamar_pack(pin, uso, err, sizeof err) != 0) {
        bpvm_diag("BIOS: gpio_init(%d, %d) denegado: %s", pin, mode, err);
        return -1;
    }
    bpvm_gpio_init(pin, mode);
    return 0;
}
void bpvm_bios_hal_gpio_write(int pin, int value) { bpvm_gpio_write(pin, value); }
int  bpvm_bios_hal_gpio_read (int pin)            { return bpvm_gpio_read(pin); }

int bpvm_bios_hal_spi_init(int bus, int sck, int mosi, int miso, int baudrate, int mode) {
    char err[96];
    if (bpvm_gpio_instancia_ok(2, bus, err, sizeof err) != 0) {   /* 2 = SPI (bpvm_gpio.h) */
        bpvm_diag("BIOS: spi_init(%d) denegado: %s", bus, err);
        return -1;
    }
    bpvm_spi_init(bus, sck, mosi, miso, baudrate, mode);
    return 0;
}
int bpvm_bios_hal_spi_write(int bus, const uint8_t* data, size_t n) { return bpvm_spi_write(bus, data, n); }

int bpvm_bios_hal_i2c_init(int bus, int sda, int scl, int baudrate) {
    char err[96];
    if (bpvm_gpio_instancia_ok(1, bus, err, sizeof err) != 0) {   /* 1 = I2C */
        bpvm_diag("BIOS: i2c_init(%d) denegado: %s", bus, err);
        return -1;
    }
    bpvm_i2c_init(bus, sda, scl, baudrate);
    return 0;
}
int bpvm_bios_hal_i2c_write(int bus, int addr, const uint8_t* data, size_t n) { return bpvm_i2c_write(bus, addr, data, n); }
int bpvm_bios_hal_i2c_read (int bus, int addr, uint8_t* data, size_t n)       { return bpvm_i2c_read(bus, addr, data, n); }

uint32_t bpvm_bios_hal_now_ms(void)   { return (uint32_t) bpvm_platform_now_ms(); }
void     bpvm_bios_hal_sleep_ms(int ms) { bpvm_platform_thread_sleep_ms(ms); }
