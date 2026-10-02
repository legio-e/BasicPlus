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
#include "bpvm_part.h"   /* V7/H6 G5.b0: bpvm_part_t (la partición de packs) */
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Arranque escalonado (identidad → particiones del env → FS → VM), parando en la
 * 1ª capa que falla. Lo llama el repl EN LUGAR de fs_load(). Tras esto,
 * board_boot_status() refleja el estado alcanzado. */
void board_mgr_stm32_boot(void);
/* [V7/H2, #493] Monta la zona de packs (XIP). Va APARTE del climb; sólo necesita
 * las particiones (no el FS). Idempotente. Se llama ANTES de sincronizar /lib
 * (V7/H6, #468), como en las otras familias: el instalador pregunta a la zona
 * qué sirve ya con los mismos bytes que la imagen. (En #493 iba detrás: el `stat`
 * del sync de entonces veía la zona y reponía /lib en cada arranque.) */
int  board_mgr_stm32_montar_packs(void);   /* el gancho de bpvm_pack_montar_arranque (V7/H7) */

/* Estado REAL del boot (para STATE del wire y el gating de FS/RUN en el repl). */
const bpvm_boot_status_t* board_boot_status(void);

/* #423 — una bandera del entorno (p.ej. `log`), sin sacar los slots. */
int  board_mgr_stm32_env_bool(const char* key, int def);
long board_mgr_stm32_env_long(const char* key, long def);
int  board_mgr_stm32_env_get(const char* key, char* buf, size_t cap);   /* V7/H6.c G4.a1 */

/* V7/H6 G5.b0 — el ENV leído YA, antes que el climb: el reparto de la memoria de
 * la VM (el bloque de los packs nativos, `packRam=` / `SQLite=`) se decide al abrir
 * el arranque, antes que las particiones — como la ESP32 (board_mgr_esp32_env_
 * temprano). Idempotente: el climb lo vuelve a leer. */
void board_mgr_stm32_env_temprano(void);

/* V7/H7 (Boot 2, fase 4) — el ENV entero, de sólo lectura, para el arranque común. */
const bpvm_env_t* board_mgr_stm32_env(void);

/* V7/H10 (#626) — añadir al ENV una clave que falta (bpvm_env_anadir) y volcarla a la
 * flash: lo que el arranque común anota una vez (la fecha de la imagen). 0 escrita; 1 ya
 * estaba; -1 no se pudo. */
int board_mgr_stm32_env_anadir(const char* clave, const char* valor);

/* V7/H6 G5.b0 — la partición de packs del layout del BOOT (offset relativo a
 * FLASH_BASE), o NULL si el layout no validó o no hay zona. La recorre el cargador
 * de los motores nativos (pack_stm32.c): una sola verdad con el FS y el wire. */
const bpvm_part_t* board_mgr_stm32_packs(void);

/* Atiende un comando de gestión ya parseado (STATE/ENV_x/PART_x/PACK_x). `scratch`
 * (>= 3 sectores + 512 = 24.5 KB; s_put_buf del repl vale) lo presta el llamador →
 * sin BSS propio grande. `bulk`/`bulk_len` = payload binario ya recibido por el
 * transporte (PACK_BURN_DATA; NULL si el comando no lleva). Idéntico shape que
 * board_mgr_esp32/pico. */
void board_mgr_stm32_handle(long id, const json_obj_t* obj, const char* type,
                            unsigned char* scratch, unsigned long scratch_len,
                            const unsigned char* bulk, unsigned long bulk_len);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_MGR_STM32_H */
