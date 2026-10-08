/*
 * bpvm_bmgr_wire.h — H9: núcleo ÚNICO del adaptador de wire de gestión de placa.
 *
 * Despacha UN comando (STATE, ENV_x, PART_x) contra el bpvm_bmgr y construye la línea
 * JSON de reply. Lo comparten el boardsim (TCP, host) y el firmware (USB) → las
 * replies son BYTE-IDÉNTICAS por construcción (misma filosofía dual-VM: una sola
 * fuente de verdad, y el firmware hereda replies ya host-verificadas por el smoke
 * del sim). El llamador solo pone: parsear su JSON al `req`, enviar `out`, y —si
 * hubo escritura— volcar el sector A/B indicado a su "flash" (fichero en host,
 * flash_range_* en el device). Ver docs/H9_KERNEL_CAPAS.md.
 */
#ifndef BPVM_BMGR_WIRE_H
#define BPVM_BMGR_WIRE_H

#include <stddef.h>
#include "bpvm_bmgr.h"
#include "bpvm_part.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Request ya EXTRAÍDO del JSON por el llamador (cada lado con su json_min). Campos
 * ausentes: has_key/has_value/has_off = 0; part_sizes[i] = -1. */
typedef struct {
    char type[32];                      /* "STATE", "ENV_SET", "PART_APPLY", … */
    long id;                            /* id de correlación del wire */
    char key[64];                       /* ENV_GET/SET/DEL */
    int  has_key;
    char value[192];                    /* ENV_SET (un valor de env: nombre/número/path) */
    int  has_value;
    long part_sizes[BPVM_PART_COUNT];   /* PART_APPLY (por partición, en bytes); -1 si falta */
    long off;                           /* H3: offset de pack en la región (PACK_ENTRIES; */
    int  has_off;                       /*     lo usarán también PACK_DEL/PACK_READ) */
    long size;                          /* H3: PACK_BURN_BEGIN (tamaño total anunciado) */
    int  has_size;
    const unsigned char* bulk;          /* H3: PACK_BURN_DATA — el chunk ya RECIBIDO por */
    long bulk_len;                      /*     el transporte del llamador (NULL si no hay) */
    int  confirm_yes;                   /* H3: PACK_FORMAT — "confirm":"YES" (como el FORMAT del FS) */
} bpvm_bmgr_req_t;

/* #338 — ¿este verbo necesita las copias A/B del env en RAM?
 *
 * Los comandos del gestor se parten en DOS grupos que NUNCA se solapan: los del
 * entorno (STATE, ENV_x, PART_x) leen y reescriben el bloque A/B, y los de packs
 * (PACK_*) no lo tocan para nada —van contra la zona de packs—. Esa frontera es
 * lo que permite que los dos sectores del env y los buffers de trabajo de los
 * PACK_* COMPARTAN la misma memoria (la zona de rascar), en vez de sumarse: en
 * un micro sin PSRAM eso son 8 KB de .bss que vuelven al heap de la VM.
 *
 * Vive aquí, en el núcleo, y no en cada cintura, porque es UNA regla: si un
 * verbo nuevo lee el env y alguien se olvida de añadirlo, el fallo tiene que ser
 * el mismo en las cuatro familias, no cuatro fallos distintos.
 *
 * Devuelve 1 si hace falta el env (bm->a y bm->b NO pueden ser NULL), 0 si no. */
int bpvm_bmgr_needs_env(const char* type);

/* Despacha el comando → escribe la línea JSON de reply en `out` (SIN '\n'; el
 * llamador enmarca y envía). Devuelve la longitud (>0), o -1 si no cabe en `out`.
 * Si el verbo necesita el env (ver arriba) y el llamador no puso `bm->a`/`bm->b`,
 * responde ERROR en vez de leer un puntero nulo: un descuido de la cintura sale
 * por el wire con su nombre, no como un reset sin explicación.
 * En *wrote_slot deja el sector A/B modificado (0=A, 1=B) que el llamador debe
 * volcar a flash, o -1 si no hubo escritura. Un verbo desconocido produce un reply
 * ERROR UNSUPPORTED (longitud válida, wrote_slot=-1). */
int bpvm_bmgr_wire_dispatch(bpvm_bmgr_t* bm, const bpvm_bmgr_req_t* req,
                            char* out, size_t out_cap, int* wrote_slot);

/* V7 #671 (8-oct) — un PACK_BURN_END, PACK_DEL o PACK_FORMAT que salió bien con motores YA cargados deja la placa
 * pendiente de REINICIAR (su respuesta lo dice: "reinicia":true): las tablas de los motores apuntan a la zona de
 * antes. Lo hace quien manda la respuesta (bpvm_repl.c, tras mandarla) con el `reiniciar` de la familia. 1 = sí, y se
 * consume (la siguiente pregunta, 0). */
int bpvm_bmgr_wire_reinicio_pendiente(void);

/* Estado de arranque derivado del bmgr (para STATE y para el "escrutinio" del IDE):
 * virgen (sin env) → 0 (kernel); env pero sin particiones → 1; completo → 3. */
int bpvm_bmgr_wire_state(const bpvm_bmgr_t* bm);

/* ── V7/H10, EL BOARD_MGR COMÚN (3-oct; V7_IDEAS §H10 «el board_mgr común», las decisiones de Eduardo) ──────────
 *
 * LA CINTURA DE UNA FAMILIA: lo poco que el común no puede hacer por ella. Los tres `handle` de las placas eran el
 * mismo código salvo esto. Es también la «cara común delgada» de la flash (D3/D5): sólo lo que necesita el gestor —el
 * ENV y la zona de packs, que ya tiene su cintura en `bm->packs_flash`—; el log y el FS, como están. */
typedef struct bpvm_bmgr_familia {
    size_t sector;                                      /* el sector del ENV: 4 KB (RP2350, ESP32, el sim) u 8 KB (U5) */
    /* Dónde se LEEN las dos copias del ENV (0 = A, 1 = B). O un puntero a la copia, en su sitio —la flash mapeada: la
     * Pico y la STM32—, o, si no se puede (la ESP32 no mapea `bpenv`: gasta espacio virtual, y en la C5 no queda),
     * `copia` NULL y `leer` la copia a un sector de `espejo` (2 sectores que pone la familia). */
    const uint8_t* (*copia)(int slot);
    int  (*leer)(int slot, uint8_t* dst);               /* 0 leída; si no, la vista la da por borrada (0xFF) */
    uint8_t* espejo;
    int  (*escribir)(int slot, const uint8_t* src);     /* borrar y programar un sector entero; 0 escrito, -1 no */
    /* Lo de cada placa en el gestor: todo menos `a`, `b`, `scratch` y `sector`, que pone el común (la base y el tamaño
     * de las particiones, el FS arriba, el estado REAL del arranque, la zona de packs montada y su cintura, el bloque de
     * los motores). */
    void (*rellenar)(bpvm_bmgr_t* bm);
    const char* (*no_vale)(void);                       /* NULL = adelante; si no, por qué no se toca nada */
} bpvm_bmgr_familia_t;

/* La familia la registra al arrancar, antes de leer el ENV. */
void bpvm_bmgr_familia(const bpvm_bmgr_familia_t* f);
const bpvm_bmgr_familia_t* bpvm_bmgr_familia_actual(void);

/* D1 (Eduardo, 3-oct: *«En realidad deberían de leerse siempre de la flash»*) — LA VISTA DEL ENV: la copia buena de las
 * que hay en la flash. Toda escritura del ENV pasa por `bpvm_bmgr_escribir` y sube una generación; la vista vuelve a
 * elegir (y la ESP32 vuelve a leer su espejo) en cuanto cambia: es siempre lo que tiene la flash, sin leerla en cada
 * consulta. CUÁNDO entra en efecto un valor lo decide quien lo lee (al arrancar, en cada RUN…); lo EN VIGOR es otra
 * cosa. Sin familia registrada, un ENV vacío (`valid` = 0). El puntero vale hasta la próxima escritura. */
const bpvm_env_t* bpvm_bmgr_vista(void);

/* Escribe un sector del ENV por la cintura de la familia y renueva la vista. 0 escrito; -1 no (y la vista sigue a lo
 * que haya quedado en la flash: la otra copia, si ésta quedó a medias). */
int bpvm_bmgr_escribir(int slot, const uint8_t* src);

/* #626 — el arranque común AÑADE una clave que falta (la fecha de la imagen, una vez): el bloque nuevo, en un sector
 * de la zona de rascar, desde las copias de la flash, y por `bpvm_bmgr_escribir`. 0 escrita; 1 ya estaba (nada que
 * escribir); -1 no (no cabe, la zona no está libre, la flash no escribe o la familia dice que no). */
int bpvm_bmgr_anadir(const char* clave, const char* valor);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_BMGR_WIRE_H */
