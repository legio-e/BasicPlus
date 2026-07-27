/*
 * bpvm_dbg_wire.h — núcleo PORTABLE del ramo de DEPURACIÓN del wire v1
 * (breakpoints, pausa/step/stop, LOCALS/STACK/READ_*).
 *
 * Misma jugada que bpvm_bmgr_wire (STATE/ENV/PART) y bpvm_log: la lógica en un
 * sitio + una cintura fina por familia. Hasta ahora este ramo vivía SOLO dentro
 * de pico/repl_v1.c, así que ESP32 y STM32 se quedaron sin depurador — el IDE
 * ofrece "Debug on device" y el firmware contestaba UNSUPPORTED. Copiarlo a cada
 * repl sería la tercera y cuarta copia; es justo así como esta familia se quedó
 * atrás. La máquina de depurar (breakpoints por pc, pausa, step) YA está en
 * todas las placas: vive en el intérprete. Lo único que faltaba era el cableado.
 *
 * ── El núcleo NO parsea JSON ──
 * `json_min` está duplicado por familia (pico/, esp32/main/, stm32/port/), así
 * que `src/` no puede depender de él. Igual que en bpvm_bmgr_wire: el firmware
 * parsea y entrega un comando TIPADO. Para el bucle de pausa —que tiene que
 * seguir leyendo del wire mientras la VM está detenida— el llamador presta un
 * `next_cmd` que lee la siguiente línea y la deja parseada.
 *
 * Los buffers también los presta el llamador (los repl ya tienen los suyos):
 * cero .bss propio, que es la norma de los firmwares pequeños.
 */
#ifndef BPVM_DBG_WIRE_H
#define BPVM_DBG_WIRE_H

#include "bpvm.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Comandos del ramo. OTHER = no es de depuración (el repl sigue su dispatch). */
typedef enum {
    BPVM_DBGC_OTHER = 0,
    BPVM_DBGC_CONTINUE,
    BPVM_DBGC_STEP,
    BPVM_DBGC_STOP,          /* STOP y KILL */
    BPVM_DBGC_PING,
    BPVM_DBGC_PAUSE,
    BPVM_DBGC_SET_BP,
    BPVM_DBGC_CLR_BP,
    BPVM_DBGC_READ_INT,
    BPVM_DBGC_READ_STRING,
    BPVM_DBGC_LOCALS,
    BPVM_DBGC_STACK
} bpvm_dbg_cmd_kind_t;

/* Comando ya parseado por el firmware. Los campos que no apliquen al kind se
 * ignoran; usar los defaults indicados si el JSON no los trae. */
typedef struct {
    bpvm_dbg_cmd_kind_t kind;
    long id;        /* id de la request */
    long pc;        /* SET_BP    (-1 si ausente) */
    long bpId;      /* CLR_BP    (-1 = todos)    */
    long addr;      /* READ_INT  (-1 si ausente) */
    long ref;       /* READ_STRING (0 = "")      */
} bpvm_dbg_cmd_t;

/* Traduce el "type" del wire a kind (BPVM_DBGC_OTHER si no es del ramo). */
bpvm_dbg_cmd_kind_t bpvm_dbg_wire_kind(const char* type);

/* Lee el siguiente comando del wire, BLOQUEANTE. 0 = OK; !=0 = línea ilegible
 * (el bucle de pausa la ignora y reintenta, como hacía el Pico). */
typedef int (*bpvm_dbg_next_cmd_fn)(bpvm_dbg_cmd_t* out, void* user);

/* Envía una línea ya construida (el núcleo no sabe de transportes). */
typedef void (*bpvm_dbg_send_fn)(const char* line, size_t len, void* user);

/* Contexto: cintura + buffer prestado. Vive en el repl del firmware; se pasa
 * como `user` del pause_cb, así que debe sobrevivir a todo el RUN. */
typedef struct {
    bpvm_dbg_next_cmd_fn next_cmd;
    bpvm_dbg_send_fn     send;
    void*                user;      /* para next_cmd/send */
    char*                reply;     /* buffer de construcción de replies */
    size_t               reply_cap;
    long                 session;   /* id de sesión que va en BP_HIT */
} bpvm_dbg_wire_t;

/* ── Fuera de ejecución (pre-RUN) ────────────────────────────────────────── */

/* Atiende PAUSE/SET_BP/CLR_BP antes de que haya VM: acumula breakpoints
 * pendientes y la petición de pausa inicial. Devuelve 1 si ha atendido el
 * comando (y ya ha enviado su reply), 0 si no es de su ramo. */
int bpvm_dbg_wire_handle(bpvm_dbg_wire_t* w, const bpvm_dbg_cmd_t* cmd);

/* ¿Hay algo que depurar? (breakpoints pendientes o PAUSE inicial). */
int bpvm_dbg_wire_armed(void);

/* Justo antes de correr: aplica los breakpoints pendientes a la vm, registra el
 * pause_cb con `w` como user y, si se pidió PAUSE, deja la VM parada en el 1er
 * opcode. No hace nada si no hay nada armado. */
void bpvm_dbg_wire_arm(bpvm_dbg_wire_t* w, bpvm_t* vm);

/* Al terminar el RUN: olvida la vm y los pendientes (la siguiente sesión parte
 * limpia). El Pico lo hacía a mano al final de handle_run. */
void bpvm_dbg_wire_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_DBG_WIRE_H */
