/*
 * chip_cfg.h — LO PARTICULAR DE ESTE SILICIO (ESP32-C6).
 *
 * El `main.c` es el de `esp32/common/`, compartido con el S3 y el C3. Aquí sólo
 * lo que de verdad cambia: el nombre, los tamaños del bloque de la VM y la
 * identidad de placa. V6/P1.C6 (3-sep-2026): entra por el camino del C3 — un
 * USB nativo (USB-Serial-JTAG) para el wire, consola a UART0, 4 MB de flash,
 * RISC-V sin FPU (el gate del .mdn cae solo en RISC-V/softfp por `__riscv`) —
 * con más SRAM (512 KB HP + 16 KB LP) y radio Wi-Fi 6 / BLE / 802.15.4.
 */
#ifndef BPVM_CHIP_CFG_H
#define BPVM_CHIP_CFG_H

#define CHIP_NOMBRE  "ESP32-C6 (ensayo)"

/* V6 (9-sep) — EL ID CANONICO DEL CHIP. `CHIP_NOMBRE` es para leerlo un humano
 * (sale en el banner); esto es lo que contesta `Machine.getMicro()` y el `INFO`
 * del wire, y por eso va en minusculas y sin adornos. Una entrada por micro: es
 * el sitio donde ya vivian el nombre y los numeros de memoria de este chip. */
#define CHIP_MICRO   "esp32c6"

/* ─── EL BLOQUE DE LA VM: objetivo, margen y suelo (V6/P1.C6.3, 3-sep-2026) ───
 *
 * MEDIDO EN ESTA PLACA con el protocolo de U6.4, no heredado del C3:
 *
 *   heap: libre 410988 | mayor 385024 | bloques: 6 libres, 42 usados | usado 17760
 *   vm: DRAM interna libre 410988->279912 B (bloque mayor 385024->253952 B)
 *   mem: ... MINIMO HISTORICO 262980 B          (tras PUT + RUN×2 de fib(28) + LIST)
 *
 * El bloque contiguo son 376 KB (el C3 tenía 136: aquí el IDF deja la SRAM casi
 * de una pieza), así que el techo no aprieta. El OBJETIVO no es el techo: el
 * criterio de Eduardo es no vaciar el heap del RTOS para tener que devolvérselo
 * después — y este silicio trae Wi-Fi 6, BLE y 802.15.4, que cuando entren
 * pedirán su parte. 192 KB (heap 128 + pilas 64: el doble de heap que el C3) deja
 * 184 KB contiguos y ~219 KB libres en total al sistema. Si un día se quiere
 * más, 256 aún deja 120 contiguos; es un número, no una obra.
 *
 * El MARGEN es la resta de la medida: 279912 − 262980 = 16932 B. Del orden del
 * C3 (17588): un núcleo, sin radio activa. */
/* ─── Y CON LA PANTALLA (V6/P2.1, 3-sep noche) LOS NÚMEROS CAMBIAN ──────────────
 *
 * Esta imagen lleva LVGL, y LVGL pesa dos veces: ~108 KB de DRAM ESTÁTICA al arrancar
 * (heap: libre 410988 → 303164; fuentes, tablas, gui.c) y ~93 KB EN MARCHA, porque
 * LVGL va sobre el heap de C (LV_USE_STDLIB_MALLOC = CLIB: el pool LV_MEM_SIZE no
 * se usa) y ahí viven los objetos, los estilos, los dos draw buffers (23 KB, DMA) y
 * el SPI. Medido con GuiColorDemo 20 s (log=1, KILL), DOS pasadas:
 *
 *   con 192 KB de VM: libre tras reservar 106552, mínimo histórico 13624 → uso  92 928 B
 *   con 160 KB de VM: libre tras reservar 139320, mínimo histórico 35496 → uso 103 824 B
 *   con 128 KB de VM: libre tras reservar 172088, mínimo histórico 68264 → uso 103 824 B  (4-sep, confirma)
 *
 * El uso en marcha varía de pasada a pasada (LVGL sobre el heap de C, fragmentación),
 * así que el MARGEN es el máximo medido, 103 824. Y el objetivo baja a 128 (heap 64 +
 * pilas 64, el reparto del C3): el planificador deja 303164 − 131072 = 172 092 B al
 * sistema, ~68 KB de holgura sobre el peor pico medido — sin radios todavía. Con 160
 * quedaban 35 KB, y el criterio es el de Eduardo: no vaciar el heap del RTOS para
 * devolvérselo después. Aquí es la pantalla quien lo cobra. */
/* ─── Y SIN LA PANTALLA EN LA IMAGEN (V7/H6.b, 25-sep) ─────────────────────────────────
 *
 * LVGL ya no va sobre el heap de C: su pool, el modelo y la franja de dibujo son la RAM del pack
 * (el bloque `packRam=` del ENV, que se reserva ANTES que la VM). Y MEDIDO con esta imagen, el
 * margen SE QUEDA: con `packRam=136` (GuiMini + DriverSpi) y la VM en su objetivo de 128 KB,
 *
 *   heap: libre 386812 al arrancar -> 247544 tras packRam -> 116468 tras la VM
 *   mem: MINIMO HISTORICO 22360 B   (la tanda de V7 entera, SdCard, GuiLleno, GuiFlushBench, C5SdGui)
 *
 * o sea ~94 KB de pico del sistema SIN LVGL en el heap: la cifra de 103 824 de arriba no era de
 * LVGL, como decía el comentario, sino del resto (el PUT, las tareas que el IDF crea en marcha…).
 * Con `packRam=176` (GuiMini con 96 KB de pool) la VM se quedaba en 100 KB y SdCard no cabía: por
 * eso el pack de pantallas pequeñas lleva 64 KB de pool. */
#define CHIP_VM_OBJETIVO      (128u * 1024u)
#define CHIP_MARGEN_SISTEMA   103824u
#define CHIP_VM_MIN           (64u * 1024u)    /* el suelo de las cinco placas */

/* Identidad de placa: sin esto el C6 saludaría como `bpvm-esp32` (el S3). */
#include "c6_board_id.h"
#define CHIP_INSTALAR_BOARD_ID()  c6_install_board_id()

/* V7/H3 SD.3 (24-sep) y V7/H6.b (25-sep) — la TF de la placa, por SPI: va en el SPI 2 de la pantalla (MISO 5,
 * CS 4; `sd=bus:2,cs:4` en el ENV, con los pines del bus en su línea `spi2=`), y la línea `sd` decide si se usa.
 * Hasta H6.b la pantalla hablaba con el bus por `esp_lcd` directamente, sin pasar por el árbitro, y la TF a la
 * vez la habría pisado: por eso SD_INFO/SD_MOUNT decían «aún no se usa». Ahora la pantalla va por el pack y la
 * HAL BP, como en la C5, y la TF se registra igual que allí. Sin pin de detección: se monta al arrancar o con
 * SD_MOUNT. */
#include "bpvm_sd_blk.h"
#include "board_mgr_esp32.h"   /* board_mgr_env() */
#define CHIP_SD_REGISTRAR()  ((void) bpvm_sd_blk_registrar_desde_env(board_mgr_env()))

/* #581 (7-oct) — EL PAD VDD_SPI: la ALIMENTACIÓN de la flash, que en el C6 es el GPIO27 (el IDF lo nombra así:
 * `IO_MUX_GPIO27_REG` es el registro del pad VDD_SPI, en soc/io_mux_reg.h). Es un GPIO SÓLO con el eFuse
 * VDD_SPI_AS_GPIO quemado; si no, para el programa no existe. El IDF no lo reserva —no es un pin de datos de la
 * flash—: `esp32_caps_init` (gpio_esp32.c) lee el eFuse. */
#define CHIP_GPIO_VDD_SPI  27

#endif /* BPVM_CHIP_CFG_H */
