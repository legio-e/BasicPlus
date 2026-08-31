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

/* ⚠️ ENSAYO — ESTE NÚMERO ESTÁ SIN MEDIR.
 *
 * El del S3 (160 KB) sale de medir la DRAM libre y el consumo en marcha (`#336`).
 * El C3 tiene MENOS SRAM (~400 KB de chip contra 512 del S3) y un solo núcleo,
 * así que copiar el número sería justo lo que este proyecto no hace: ponerlo a
 * ojo. Se arranca a 96 KB —por debajo del respaldo known-good del S3— sólo para
 * que el ensayo enlace y se pueda MEDIR en placa con `heap_caps_get_free_size`.
 *
 * El definitivo sale de repetir aquí la medida de `#336`, no de esta línea.
 * Está fichado como `P1.C3.3`. */
#define VM_BUFFER_SIZE     (96 * 1024)
#define VM_BUFFER_FALLBACK (64 * 1024)    /* ensayo: mejor una VM pequeña que ninguna */

#endif /* BPVM_CHIP_CFG_H */
