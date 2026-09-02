/*
 * chip_cfg.h — LO PARTICULAR DE ESTE SILICIO (ESP32-S3).
 *
 * ─── Por qué existe (V6/P1.C3.2, 31-ago-2026) ───────────────────────────────
 *
 * El `main.c` del C3 era una COPIA del de este proyecto que se diferenciaba en
 * TRES sitios: dos números y un nombre. 176 líneas duplicadas para eso. Es la
 * trampa de siempre —«el común crece y la copia privada no»— y ya había mordido
 * dos veces esta misma semana: el `if` sin llaves (`#464`) y las puertas por
 * capacidad del silicio (`#465`) se arreglaron en uno y no en el otro.
 *
 * Así que `main.c` subió a `esp32/common/` y lo que de verdad cambia por chip
 * bajó aquí. Cada proyecto (`esp32/`, `esp32c3/`, y el P4 si algún día comparte
 * arranque) tiene SU copia de este fichero en su componente `main/`, y el común
 * lo incluye por nombre: como en `esp32/common/` NO hay ningún `chip_cfg.h`, el
 * `#include "chip_cfg.h"` no puede resolver al del vecino por accidente —
 * siempre cae en el del proyecto que se está compilando.
 *
 * ⚠️ Lo que entra aquí es lo que NO se puede preguntar en marcha. El tamaño del
 * heap de la VM es el caso: `vm_buffer_init()` corre ANTES de
 * `board_mgr_esp32_boot()` (layer_app comprueba `s_vm_buffer`), o sea antes de
 * que exista el ENV. Todo lo demás que dependa de la placa va al ENV, no aquí.
 */
#ifndef BPVM_CHIP_CFG_H
#define BPVM_CHIP_CFG_H

/* El nombre con el que este silicio se presenta en la consola y en el log. */
#define CHIP_NOMBRE  "ESP32-S3"

/* ─── EL BLOQUE DE LA VM: objetivo, margen y suelo (V6/U6.7, 2-sep-2026) ───────
 *
 * Aquí había VM_BUFFER_SIZE (160 KB) y VM_BUFFER_FALLBACK (128): dos números a
 * mano que el arranque pedía A CIEGAS. Ahora el arranque MIDE —DRAM libre y
 * bloque contiguo, con heap_caps— y comprueba que el objetivo cabe dejando el
 * margen del sistema; si no, baja y LO DICE. La regla vive en
 * esp32/common/main.c (vm_buffer_init); aquí sólo lo que es de este silicio.
 *
 * Las dos restricciones salieron de medir el C3 (P1.C3.3): el techo real no es
 * la RAM libre sino el BLOQUE CONTIGUO, y además hay que dejarle al sistema lo
 * que consume en marcha. */

/* Cuánto QUIERE la VM. Es política, no medida, y se queda en los 160 KB de
 * siempre A PROPÓSITO — Eduardo (31-ago): «no tocaría el heap del RTOS, la idea
 * es explotarlo más si es necesario en el futuro; no tiene sentido reducirlo
 * para volver a agrandarlo». El objetivo protege ese heap: aunque quepa más, no
 * se coge más. Lo que cambia es que ya no se pide sin mirar. */
#define CHIP_VM_OBJETIVO      (160u * 1024u)

/* Lo que el SISTEMA consume en marcha fuera de la VM: tareas, buffers, FS,
 * wire. MEDIDO con tools/medir_margen.ps1 (U6.4), secuencia escrita:
 *   libre tras reservar 174524 − mínimo histórico 147960 = 26564 B
 * ⚠️ Aquí decía 86256 (#336). Era 3,3× el valor real, medido con una carga que
 * nadie anotó; sirvió para elegir los 160 KB y por eso salieron conservadores.
 * Con el número bueno los 160 dejan ~178 KB libres, y el peor momento medido
 * deja ~148: margen de sobra, que es lo que Eduardo quiere para el RTOS. */
#define CHIP_MARGEN_SISTEMA   26564u

/* Por debajo de esto la VM no da para nada útil (el suelo de pilas de 64 KB se
 * lo comería entero): no se arranca a medias, se dice. Mismo valor que
 * VM_SRAM_MIN de la Pico, a propósito: es el mismo concepto. */
#define CHIP_VM_MIN           (64u * 1024u)

/* Identidad de placa: el S3 ES el defecto de `repl_esp32.c`, asi que no hay
 * nada que pisar. Los silicios que no son el defecto instalan la suya. */
#define CHIP_INSTALAR_BOARD_ID()  ((void) 0)

#endif /* BPVM_CHIP_CFG_H */
