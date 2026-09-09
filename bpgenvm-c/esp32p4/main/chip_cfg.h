/*
 * chip_cfg.h — la entrada de ESTE micro (ESP32-P4).
 *
 * V6 (9-sep). El S3, el C3 y el C6 tienen aqui tambien sus numeros de memoria
 * (CHIP_VM_OBJETIVO, CHIP_MARGEN_SISTEMA, CHIP_VM_MIN) porque comparten el
 * arranque de `esp32/common/main.c`. El P4 NO: tiene su propio `main.c` con su
 * rama de PSRAM, asi que aqui solo esta lo que el codigo COMPARTIDO necesita.
 *
 * Si algun dia el P4 pasa a usar el arranque comun, sus numeros vienen a este
 * fichero y no a un `#ifdef` en el comun.
 */
#ifndef BPVM_CHIP_CFG_H
#define BPVM_CHIP_CFG_H

/* EL ID CANONICO DEL CHIP: lo que contesta `Machine.getMicro()` y el `INFO` del
 * wire. En minusculas y sin adornos, igual que sus hermanos. */
#define CHIP_MICRO   "esp32p4"

#endif /* BPVM_CHIP_CFG_H */
