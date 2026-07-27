/*
 * board_mgr_esp32.h — H9: adaptador de gestión de placa para la familia ESP32
 * (S3 + P4). Concentra TODO lo no portable de H9 en la ESP32 en un solo sitio
 * (compartido por los dos main.c) para no duplicarlo:
 *   - cintura del bloque de env A/B sobre la partición vendor "bpenv"
 *     (esp_partition, en vez del flash_range_* del RP2350),
 *   - el arranque escalonado (bpvm_boot_climb: particiones del env → FS → VM) y
 *     board_boot_status() con el estado REAL,
 *   - el ramo STATE/ENV_x/PART_x del wire (repl_esp32 encamina aquí).
 * La lógica y las replies las pone el núcleo portable bpvm_bmgr_wire (idéntico al
 * boardsim y al Pico). Modelo de particiones (Eduardo 19-jul): la tabla vendor fija
 * la ZONA DE DATOS entera ("bpdata"); el límite FS|Packs vive en el env (un mando).
 * Ver docs/H9_KERNEL_CAPAS.md.
 */
#ifndef BOARD_MGR_ESP32_H
#define BOARD_MGR_ESP32_H

#include "json_min.h"
#include "bpvm_boot.h"
#include "bpvm_env.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Corre el arranque escalonado (identidad → particiones del env → FS → VM),
 * parando en la 1ª capa que falla. Lo llaman los dos main.c EN LUGAR de fs_init().
 * Tras esto, board_boot_status() refleja el estado alcanzado. */
void board_mgr_esp32_boot(void);

/* Estado REAL del boot (para STATE del wire y el gating de FS/RUN en repl_esp32). */
const bpvm_boot_status_t* board_boot_status(void);

/* #311 — el env del boot, para configurar HARDWARE (el panel del P4 sale de aquí).
 * Igual que el Pico con `psram`: la config de placa vive en el env, no en un
 * fichero del FS. Nunca NULL, pero puede ser inválido (sin partición bpenv o env
 * en blanco) — bpvm_env_get* lo detecta y devuelve el default, así que el
 * llamante no necesita comprobar nada. Válido desde el estado 2 (particiones). */
const bpvm_env_t* board_mgr_env(void);

/* Atiende un comando de gestión ya parseado (STATE/ENV_x/PART_x). `scratch` (>= 4
 * sectores = 16 KB) lo presta el llamador (s_put_buf de repl_esp32, libre durante
 * un comando de gestión) → sin BSS propio grande. Idéntico shape que board_mgr_pico. */
void board_mgr_esp32_handle(long id, const json_obj_t* obj, const char* type,
                            unsigned char* scratch, unsigned long scratch_len);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_MGR_ESP32_H */
