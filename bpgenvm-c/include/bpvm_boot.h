/*
 * bpvm_boot.h — H9: máquina de estados del arranque escalonado + STATE.
 *
 * El kernel SUBE por capas recuperables: 0 KERNEL → 1 PARTICIONES → 2 FS →
 * 3 APP. Sube mientras cada capa arranca; a la PRIMERA que falla, se queda en el
 * último estado bueno y guarda el MOTIVO (fail-fast al nivel del boot). Un fallo
 * en runtime (la app peta) BAJA a un estado inferior REPORTANDO. `STATE` es el
 * latido de feedback: el host siempre sabe en qué estado está y por qué.
 *
 * Es PORTABLE y host-testable: la máquina conduce las capas como CALLBACKS
 * falibles. En host se inyectan fallos (sin particiones / FS no monta / heap no
 * arranca) para probar la subida, la retirada y el reporte SIN placa. En device,
 * los callbacks son el mount real (leen lo de abajo: bpvm_env / bpvm_part / FS /
 * heap). Ver docs/H9_KERNEL_CAPAS.md §El modelo + §Flujo de bring-up.
 */
#ifndef BPVM_BOOT_H
#define BPVM_BOOT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_BOOT_REASON_MAX  64

typedef enum {
    BPVM_BOOT_KERNEL     = 0,   /* comms mínima (kernel-comm); sin heap/FS/particiones */
    BPVM_BOOT_PARTITIONS = 1,   /* tabla de particiones válida */
    BPVM_BOOT_FS         = 2,   /* FS montado */
    BPVM_BOOT_APP        = 3    /* VM/app corriendo (full-comm) */
} bpvm_boot_state_t;

/* Resultado de intentar subir UNA capa. */
typedef struct {
    int  ok;                             /* 1 si la capa arrancó */
    char reason[BPVM_BOOT_REASON_MAX];   /* si !ok, por qué (para STATE / el asistente) */
} bpvm_boot_step_t;

/* Callback de subida de capa: intenta llevar el sistema al estado que representa,
 * leyendo lo de abajo. `user` = contexto del firmware (o del test). */
typedef bpvm_boot_step_t (*bpvm_boot_layer_fn)(void* user);

typedef struct {
    bpvm_boot_layer_fn to_partitions;    /* 0→1 */
    bpvm_boot_layer_fn to_fs;            /* 1→2 */
    bpvm_boot_layer_fn to_app;           /* 2→3 */
    void*             user;
    /* Tope (MODO SEGURO): no subir por encima de este estado. Poner BPVM_BOOT_APP
     * para "sin tope". Alcanzar el tope NO es degradación (es intencional). */
    bpvm_boot_state_t max_state;
} bpvm_boot_layers_t;

typedef struct {
    bpvm_boot_state_t state;                  /* estado ACTUAL (último bueno) */
    int               degraded;               /* 1 si un FALLO cortó la subida (o un fault) */
    char              reason[BPVM_BOOT_REASON_MAX];  /* motivo (vacío si sana) */
} bpvm_boot_status_t;

/* Sube 0→…→max_state mientras cada capa arranca; para en el 1er fallo (se queda
 * en el último estado bueno, degraded=1, con el motivo). Alcanzar el tope sin
 * fallo → degraded=0. Rellena `out`. Un callback NULL detiene la subida ahí
 * (sin marcar degradación: capa no provista, no fallida). */
void bpvm_boot_climb(const bpvm_boot_layers_t* layers, bpvm_boot_status_t* out);

/* Fallo en RUNTIME (p.ej. la app petó): baja a `drop_to` (si es menor que el
 * estado actual) y marca degraded con el motivo. El canal kernel-comm sigue vivo,
 * así que el host lo ve por STATE y puede conducir la recuperación. */
void bpvm_boot_fault(bpvm_boot_status_t* st, bpvm_boot_state_t drop_to, const char* reason);

/* Nombre legible de un estado (para STATE / logs). */
const char* bpvm_boot_state_name(bpvm_boot_state_t s);

/* Serializa el STATE a texto para el wire (kernel-comm), formato clave=valor:
 *   "state=<n> name=<...> degraded=<0|1> reason=<...>"
 * Lo lee el IDE para decidir auto-abrir/proponer FrmBoard. Devuelve los bytes
 * escritos (sin NUL), o -1 si no cabe. */
int bpvm_boot_state_report(const bpvm_boot_status_t* st, char* buf, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_BOOT_H */
