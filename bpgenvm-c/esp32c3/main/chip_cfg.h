/*
 * chip_cfg.h — LO PARTICULAR DE ESTE SILICIO (ESP32-C3).
 *
 * El `main.c` es el de `esp32/common/`, compartido con el S3. Aquí solo está lo
 * que de verdad cambia: el nombre y los dos tamaños del heap de la VM — que son
 * lo único que no se puede preguntar en marcha, porque `vm_buffer_init()` corre
 * antes de que exista el ENV. El porqué completo, en el `chip_cfg.h` del S3.
 */
#ifndef BPVM_CHIP_CFG_H
#define BPVM_CHIP_CFG_H

#define CHIP_NOMBRE  "ESP32-C3 (ensayo)"

/* EL BLOQUE DE LA VM — MEDIDO EN PLACA (V6/P1.C3.3, 31-ago-2026).
 *
 * Nació a 96 KB «para que el ensayo enlace» y con la advertencia de que estaba
 * sin medir. Ya está medido, y las dos cosas que salieron cambian el número.
 *
 * ─── 1. El techo NO es la RAM total, es el BLOQUE CONTIGUO ──────────────────
 *
 *   [boot] vm: ... DRAM interna libre 280032->181724 B (bloque mayor 139264->114688 B)
 *
 * Hay 280 KB libres y el mayor bloque son **139264 B (136 KB)**: la DRAM del C3
 * sale troceada en regiones (157 KiB + 113 de retención + 10 + 7) y este bloque
 * tiene que ser UNO. O sea que los 160 KB del S3 aquí no caben aunque sobre RAM
 * — es el caso que el comentario de `vm_buffer_init` predecía: «se puede tener
 * RAM de sobra y aun así no caber».
 *
 * ─── 2. Lo que el sistema consume EN MARCHA: ~17 KB ─────────────────────────
 *
 * Marca de agua tras dos `Run` de `Bench.mod` (`heap_caps_get_minimum_free_size`,
 * el mismo instrumento de `#336`):
 *
 *   libre tras reservar ......... 181724 B
 *   MINIMO HISTORICO ............ 164648 B
 *   ⇒ consumo en marcha .........  17076 B
 *
 * Una quinta parte de los 86 KB del S3, y tiene sentido: un solo núcleo, menos
 * tareas del IDF y sin WiFi levantado.
 *
 * ─── 3. Por qué 128 y no otro ───────────────────────────────────────────────
 *
 * El reparto lo decide `bpvm_stack_region_bytes`: 25 % para pilas **pero nunca
 * menos de 64 KB**. Con un bloque pequeño ese suelo manda, y eso es lo que hace
 * cara la diferencia:
 *
 *   bloque  96 KB -> pilas 64 KB + heap **32 KB**   (24 hilos)
 *   bloque 128 KB -> pilas 64 KB + heap **64 KB**   (24 hilos)
 *
 * El heap se DOBLA y las pilas no cambian. Y sale gratis: con 128 KB reservados
 * quedan 148956 B libres, y el peor momento medido deja 131396 — o sea 131 KB de
 * margen sobre un consumo en marcha de 17,5 KB. La otra forma de llegar a 64 KB
 * de heap era bajar el suelo de pilas a 32 KB, pero eso deja la placa en OCHO
 * hilos (main 16 KB + 2 KB por hilo) y ademas toca una regla que comparten las
 * cinco plataformas. Esto solo toca este fichero.
 *
 * ⚠️ Lo que NO justifica este numero: al medir vi en el log del arranque un
 * `throw ... No space in heap` con el bloque de 96 y lo lei como que el heap se
 * habia agotado. **Era falso.** Es la PREFABRICACION del OOM (`#430`), que corre
 * en todo arranque y que en un modulo que no importa `Core` no puede hacerse —
 * un no-evento que sonaba igual que un fallo. El aviso se arreglo para que
 * distinga las dos cosas. Con 96 KB el `Bench` tambien terminaba bien.
 *
 * 128 KB deja 8 KB de holgura sobre el bloque contiguo (139264 B). Es poco, pero
 * es holgura de MOMENTO DE ARRANQUE, no de ejecucion: si un dia no cabe, entra
 * el respaldo y LO DICE en el log. Por eso el respaldo no baja a 64 sino que se
 * queda en los 96 que ya estan probados en placa. */
#define VM_BUFFER_SIZE     (128 * 1024)
#define VM_BUFFER_FALLBACK (96 * 1024)    /* el del ensayo: probado en placa */

/* Identidad de placa: sin esto el C3 saluda como `bpvm-esp32` y anuncia los
 * GPIOs, el ADC y la SRAM del S3. Ver `c3_board_id.c`. */
#include "c3_board_id.h"
#define CHIP_INSTALAR_BOARD_ID()  c3_install_board_id()

#endif /* BPVM_CHIP_CFG_H */
