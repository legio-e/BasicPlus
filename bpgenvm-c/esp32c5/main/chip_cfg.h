/*
 * chip_cfg.h — LO PARTICULAR DE ESTE SILICIO (ESP32-C5).
 *
 * El `main.c` es el de `esp32/common/`, compartido con el S3, el C3, el C6 y el P4. Aquí
 * sólo lo que de verdad cambia: el nombre, los tamaños del bloque de la VM y la identidad
 * de placa. V7/H11 (21-sep-2026), 1.ª etapa: entra por el camino del C3/C6 — un USB nativo
 * (USB-Serial-JTAG) para el wire, consola a UART0, RISC-V a 240 MHz, 384 KB de SRAM,
 * 8 MB de PSRAM y 32 MB de flash (esptool: «ESP32-C5 (revision v1.2)», flash 32MB).
 * La PSRAM y los 32 MB se activan DESPUÉS, cada uno por separado (una variable cada vez).
 */
#ifndef BPVM_CHIP_CFG_H
#define BPVM_CHIP_CFG_H

#define CHIP_NOMBRE  "ESP32-C5 (ensayo)"

/* El ID canónico del chip: lo que contesta `Machine.getMicro()` y el `INFO` del wire. */
#define CHIP_MICRO   "esp32c5"

/* ─── EL BLOQUE DE LA VM: objetivo, margen y suelo ───────────────────────────
 *
 * ARRANQUE (21-sep): los números del C3 (heap 64 + pilas 64 = 128 KB, el reparto más
 * conservador de la familia), con el margen del C3. La SRAM del C5 es de 384 KB (menos
 * que los 512 del C6, más que los 400 del C3), y este silicio trae Wi-Fi 6 en dos bandas,
 * BLE y 802.15.4: cuando entren pedirán su parte. Se MIDE con el protocolo de U6.4 en
 * esta placa (foto del heap en el log de arranque + mínimo histórico tras PUT/RUN/LIST)
 * y entonces se escribe aquí el número de verdad, como se hizo en el C3 y el C6. */
#define CHIP_VM_OBJETIVO      (128u * 1024u)
#define CHIP_MARGEN_SISTEMA   17588u          /* el del C3, hasta medir aquí */
#define CHIP_VM_MIN           (64u * 1024u)    /* el suelo de las cinco placas */

/* Identidad de placa: sin esto el C5 saludaría como `bpvm-esp32` (el S3). */
#include "c5_board_id.h"
#define CHIP_INSTALAR_BOARD_ID()  c5_install_board_id()

/* V7/H3 SD.3 (24-sep) — la TF de la placa, por SPI: va en el SPI 2 de la pantalla (`sd=bus:2,cs:23` en el ENV,
 * con los pines del bus en su línea `spi2=`), y la línea `sd` decide si se usa. El registro común la monta al
 * arrancar y atiende SD_INFO/SD_MOUNT; en sus esperas suelta el bus (SD.2), así que convive con la pantalla del
 * pack. Sin pin de detección: no hay vigía en caliente —se monta al arrancar o con SD_MOUNT—. */
#include "bpvm_sd_blk.h"
#include "board_mgr_esp32.h"   /* board_mgr_env() */
#define CHIP_SD_REGISTRAR()  ((void) bpvm_sd_blk_registrar_desde_env(board_mgr_env()))

/* #581 (7-oct) — EL PAD VDD_SPI: la ALIMENTACIÓN de la flash, que en el C5 es el GPIO19 (el IDF lo nombra así:
 * `IO_MUX_GPIO19_REG` es el registro del pad VDD_SPI, en soc/io_mux_reg.h). Es un GPIO SÓLO con el eFuse
 * VDD_SPI_AS_GPIO quemado; si no, para el programa no existe. El IDF no lo reserva —no es un pin de datos de la
 * flash—: `esp32_caps_init` (gpio_esp32.c) lee el eFuse. */
#define CHIP_GPIO_VDD_SPI  19

#endif /* BPVM_CHIP_CFG_H */
