/*
 * board_mgr_pico.h — H9: adaptador del firmware RP2350 para la gestión de placa.
 *
 * repl_v1 encamina aquí los verbos STATE/ENV_x/PART_x. Este módulo aporta lo ÚNICO
 * no portable: la cintura de flash del bloque de env (dos sectores A/B en sitio fijo)
 * + el arranque del bpvm_bmgr con los datos de la placa (board_desc). La lógica y las
 * replies las pone bpvm_bmgr_wire (núcleo compartido con el boardsim de host →
 * byte-idéntico). Ver docs/H9_KERNEL_CAPAS.md.
 */
#ifndef BOARD_MGR_PICO_H
#define BOARD_MGR_PICO_H

#include "json_min.h"
#include "bpvm_boot.h"

#ifdef __cplusplus
extern "C" {
#endif

/* H9 (unificación): estado REAL alcanzado por el boot (lo rellena vm_task tras
 * bpvm_boot_climb). Lo usan board_mgr (STATE live, en vez del derivado del env)
 * y repl_v1 (gating de comandos FS/RUN por estado). Implementado en main.c. */
const bpvm_boot_status_t* board_boot_status(void);

/* Atiende un comando de gestión de placa ya parseado (obj) cuyo `type` es
 * STATE, ENV_x, PART_x. Lee el env de flash, despacha al núcleo compartido, envía la
 * reply por el wire y —si hubo escritura— vuelca el sector A/B a flash.
 *
 * `scratch` (>= 4 sectores = 16 KB) lo presta el llamador —típicamente s_put_buf, que
 * está LIBRE durante un comando de gestión (nunca coincide con una subida)—. Así este
 * módulo NO tiene buffers estáticos propios: no le roba SRAM al heap de la VM, crítico
 * en la Pico (sin PSRAM). */
void board_mgr_pico_handle(long id, const json_obj_t* obj, const char* type,
                           unsigned char* scratch, unsigned long scratch_len);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_MGR_PICO_H */
