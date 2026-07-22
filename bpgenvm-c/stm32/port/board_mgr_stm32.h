/*
 * board_mgr_stm32.h — H9: adaptador de gestión de placa para el STM32 (U5).
 * Concentra lo no portable de H9 en el STM32: cintura del env A/B sobre la flash
 * interna (offsets fijos de flash_layout_stm32.h, sin tabla vendor), el arranque
 * escalonado (bpvm_boot_climb: particiones del env → FS → VM) con estado REAL, y
 * el ramo STATE/ENV_x/PART_x del wire. La lógica y las replies las pone el núcleo
 * portable bpvm_bmgr_wire (idéntico al boardsim, Pico y ESP32). Ver
 * docs/H9_KERNEL_CAPAS.md.
 */
#ifndef BOARD_MGR_STM32_H
#define BOARD_MGR_STM32_H

#include "json_min.h"
#include "bpvm_boot.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Arranque escalonado (identidad → particiones del env → FS → VM), parando en la
 * 1ª capa que falla. Lo llama el repl EN LUGAR de fs_load(). Tras esto,
 * board_boot_status() refleja el estado alcanzado. */
void board_mgr_stm32_boot(void);

/* Estado REAL del boot (para STATE del wire y el gating de FS/RUN en el repl). */
const bpvm_boot_status_t* board_boot_status(void);

/* Atiende un comando de gestión ya parseado (STATE/ENV_x/PART_x). `scratch` (>= 3
 * sectores + 512 = 24.5 KB; s_put_buf de 32 KB del repl vale) lo presta el
 * llamador → sin BSS propio grande. Idéntico shape que board_mgr_esp32/pico. */
void board_mgr_stm32_handle(long id, const json_obj_t* obj, const char* type,
                            unsigned char* scratch, unsigned long scratch_len);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_MGR_STM32_H */
