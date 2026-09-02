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

/* ─── EL BLOQUE DE LA VM: objetivo, margen y suelo (V6/U6.7, 2-sep-2026) ───────
 *
 * Todo lo de aquí está MEDIDO EN ESTA PLACA (P1.C3.3 y U6.4), no heredado del
 * S3. El arranque (esp32/common/main.c, vm_buffer_init) mide DRAM libre y bloque
 * contiguo, comprueba las dos restricciones y baja si no cabe, diciéndolo.
 *
 * ─── 1. El techo NO es la RAM total, es el BLOQUE CONTIGUO ──────────────────
 *
 *   heap: libre 280032 | mayor 139264 | bloques: 7 libres, 37 usados | usado 13424
 *
 * Hay 280 KB libres y el mayor bloque son 139264 B (136 KB): el espacio libre
 * está en SIETE trozos, partidos por 37 reservas del propio IDF (tareas, drivers,
 * el timer) hechas antes de que arranque app_main. Los 160 KB del S3 aquí no
 * caben aunque sobre RAM. Es la restricción que el comentario original de
 * vm_buffer_init predecía: «se puede tener RAM de sobra y aun así no caber».
 *
 * ─── 2. Por qué 128 y no otro ───────────────────────────────────────────────
 *
 * El reparto (bpvm_stack_region_bytes) da 25 % a pilas pero nunca menos de
 * 64 KB; con un bloque pequeño ese suelo manda:
 *
 *   bloque  96 KB -> pilas 64 KB + heap 32 KB   (24 hilos)  ← el ensayo
 *   bloque 128 KB -> pilas 64 KB + heap 64 KB   (24 hilos)  ← éste
 *
 * El heap se dobla sin tocar los hilos, y quedan 148956 B libres con el peor
 * momento medido en 131368: margen de sobra. 128 deja 8 KB de holgura sobre el
 * bloque contiguo; si un día no cabe, el arranque baja de 4 en 4 KB y lo dice.
 *
 * ⚠️ Lo que NO justifica este número: al medir vi «throw ... No space in heap»
 * con el bloque de 96 y lo leí como que el heap se agotaba. Era FALSO — es la
 * prefabricación del OOM (#430) en un módulo que no importa Core, un no-evento
 * que sonaba igual que un fallo. El aviso ya distingue las dos cosas. */
#define CHIP_VM_OBJETIVO      (128u * 1024u)

/* Lo que el SISTEMA consume en marcha fuera de la VM. MEDIDO con
 * tools/medir_margen.ps1 (U6.4), y reproducible al 0,16 %:
 *   libre tras reservar 148956 − mínimo histórico 131368 = 17588 B
 * Una parte de lo del S3 (26564): un solo núcleo, menos tareas, sin WiFi. */
#define CHIP_MARGEN_SISTEMA   17588u

/* Suelo: mismo concepto y mismo valor que la Pico (VM_SRAM_MIN) y el S3. */
#define CHIP_VM_MIN           (64u * 1024u)

/* Identidad de placa: sin esto el C3 saluda como `bpvm-esp32` y anuncia los
 * GPIOs, el ADC y la SRAM del S3. Ver `c3_board_id.c`. */
#include "c3_board_id.h"
#define CHIP_INSTALAR_BOARD_ID()  c3_install_board_id()

#endif /* BPVM_CHIP_CFG_H */
