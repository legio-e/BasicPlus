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
#define CHIP_VM_OBJETIVO      (192u * 1024u)
#define CHIP_MARGEN_SISTEMA   16932u
#define CHIP_VM_MIN           (64u * 1024u)    /* el suelo de las cinco placas */

/* Identidad de placa: sin esto el C6 saludaría como `bpvm-esp32` (el S3). */
#include "c6_board_id.h"
#define CHIP_INSTALAR_BOARD_ID()  c6_install_board_id()

#endif /* BPVM_CHIP_CFG_H */
