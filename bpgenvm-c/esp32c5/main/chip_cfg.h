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

#endif /* BPVM_CHIP_CFG_H */
