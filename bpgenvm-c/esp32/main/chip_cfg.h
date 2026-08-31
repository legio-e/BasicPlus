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

/* Cuánto pide la VM. NO es un número a ojo: sale de medir (#336).
 *   DRAM libre al arrancar, antes de reservar ......... 319632 B
 *   lo que el sistema consume EN MARCHA por encima .....  86256 B  (medido con
 *     heap_caps_get_minimum_free_size tras varios RUN: 188556 justo tras
 *     reservar − 102300 en el peor momento observado)
 *   ⇒ tope seguro = 319632 − 86256 − margen
 * Con 64 KB de margen salen ~164 KB; se redondea a la baja a 160.
 * NO se sube a 192 (que daría heap 128) porque dejaría el peor caso en ~36 KB
 * libres, y con las tareas del IDF creándose EN MARCHA eso ya no es margen.
 * El reparto lo decide bpvm_stack_region_bytes: con 160 KB manda el suelo ⇒
 * stacks 64 KB (igual que antes) y heap 96 KB (+50%).
 * Si algún día no cabe, hay escalón de respaldo abajo: mejor una VM más
 * pequeña que ninguna. */
#define VM_BUFFER_SIZE     (160 * 1024)
#define VM_BUFFER_FALLBACK (128 * 1024)   /* el de siempre, known-good */

/* Identidad de placa: el S3 ES el defecto de `repl_esp32.c`, asi que no hay
 * nada que pisar. Los silicios que no son el defecto instalan la suya. */
#define CHIP_INSTALAR_BOARD_ID()  ((void) 0)

#endif /* BPVM_CHIP_CFG_H */
