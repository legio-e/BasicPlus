/*
 * bpvm_mem.h — EL PLANIFICADOR DE MEMORIA DE LA VM: una regla, cinco placas.
 *
 * ─── Por qué existe (V6/U6, 2-sep-2026) ─────────────────────────────────────
 *
 * El censo U6.0 encontró que cada puerto decidía su memoria a su manera —símbolos
 * del linker (Pico), array estático (STM32), heap_caps con constantes (S3/C3),
 * PSRAM menos el display (P4)— y que "cuánto tomar" eran dos filosofías mezcladas:
 * calcular o poner un número. U6.2 (Eduardo) enseñó que no eran dos filosofías
 * sino UNA regla aplicada a dos clases de memoria:
 *
 *     EXCLUSIVA  (PSRAM)  → todo, menos la reserva CON NOMBRE delante (SQLite) y el
 *                           margen de la región (el display del P4)
 *     COMPARTIDA (SRAM)   → todo lo contiguo, menos el margen de la región (malloc/RTOS)
 *
 * Y U6.6 fijó las tres capas: la FAMILIA enumera lo que HAY (en marcha, porque una
 * misma imagen va con PSRAM y sin ella), las CONSTANTES de la imagen dicen lo que
 * se sabe del silicio (margen medido, objetivo, suelo), y el común decide.
 *
 * Este fichero es el común. Sin dependencias a propósito (solo <stddef.h>), como
 * bpvm_sqlmem: se prueba en el host con los números MEDIDOS de cada placa
 * (test/test_mem.c), que es el arnés antes que el artefacto.
 *
 * ─── Cómo se usa ─────────────────────────────────────────────────────────────
 *
 *   bpvm_mem_region_t r[2]; int n = mi_familia_enumera(r, 2);   // qué hay
 *   bpvm_mem_cfg_t    cfg = { .objetivo=..., .vm_min=... };   // el margen va en la region
 *   bpvm_mem_plan_t   p;
 *   bpvm_mem_plan(r, n, &cfg, &p);                                // cuánto y de dónde
 *   // la familia TOMA p.bytes de r[p.idx] como sepa (puntero o malloc), y loguea
 *   // bpvm_mem_plan_str(...), que es la misma línea en las cinco placas.
 */
#ifndef BPVM_MEM_H
#define BPVM_MEM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Una pieza de memoria tal como la ve la familia AHORA. */
typedef struct {
    unsigned char* base;     /* dónde empieza; NULL si se toma pidiendo (malloc) */
    size_t         bytes;    /* lo CONTIGUO utilizable (en heap_caps: el bloque mayor) */
    size_t         libre;    /* lo libre en total en esa bolsa (>= bytes); de aquí sale el margen */
    int            exclusiva;/* 1 = nadie más asigna aquí (PSRAM); 0 = compartida con malloc/RTOS */
    size_t         margen;   /* lo que en ESTA memoria se deja a los demás inquilinos: malloc/RTOS
                              * en la compartida (MEDIDO: 26564 el S3, 17588 el C3, 64 KB la Pico),
                              * el display en la PSRAM del P4 (4 MiB para LVGL). 0 = nada. El margen
                              * es de la región y no de la configuración porque cada memoria tiene
                              * inquilinos distintos: la Metro deja 64 KB en su SRAM y nada en su PSRAM. */
    const char*    nombre;   /* "PSRAM", "SRAM interna"… para el log */
} bpvm_mem_region_t;

/* Lo que la IMAGEN sabe (chip_cfg.h) y lo que el ENV pide. */
typedef struct {
    size_t objetivo;         /* cuánto QUIERE la VM; 0 = todo lo que se pueda */
    size_t vm_min;           /* por debajo de esto no hay VM útil: no se arranca a medias */
    size_t reserva_bytes;    /* reserva CON NOMBRE delante de la memoria EXCLUSIVA (SQLite); 0 = ninguna */
    const char* reserva_nombre;
} bpvm_mem_cfg_t;

typedef enum {
    BPVM_MEM_OK = 0,
    BPVM_MEM_SIN_REGIONES,   /* la familia no ofreció nada */
    BPVM_MEM_NO_CABE         /* ni el suelo cabe: sin VM, y se dice con los números */
} bpvm_mem_res_t;

typedef struct {
    bpvm_mem_res_t res;
    int         idx;         /* región elegida (−1 si ninguna) */
    size_t      bytes;       /* para la VM (0 si NO_CABE) */
    size_t      techo;       /* lo máximo que cabía en esa región dejando lo debido */
    const char* limita;      /* qué fijó el techo: "la región" | "el bloque contiguo" | "el margen del sistema" */
    size_t      reserva_bytes;/* lo apartado delante en la exclusiva (0 si no) */
} bpvm_mem_plan_t;

/* Decide de qué región y cuánto. Preferencia: la EXCLUSIVA mayor; si no hay, la
 * compartida mayor. Nunca toma más que `objetivo` (si es >0) aunque quepa —
 * criterio de Eduardo: no se reduce el heap del RTOS para volver a agrandarlo. */
bpvm_mem_res_t bpvm_mem_plan(const bpvm_mem_region_t* r, int n,
                             const bpvm_mem_cfg_t* cfg, bpvm_mem_plan_t* out);

/* La línea del log, la MISMA en todas las placas. Devuelve chars escritos. */
int bpvm_mem_plan_str(const bpvm_mem_plan_t* p, const bpvm_mem_region_t* r,
                      const bpvm_mem_cfg_t* cfg, char* buf, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_MEM_H */
