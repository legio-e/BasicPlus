/*
 * bpvm_handles.h — LA TABLA DE HANDLES de la VM, en su módulo (V6/U5).
 *
 * Una ref de BasicPlus no es una dirección: es un HANDLE de 64 bits
 * [gen:32 | idx|TAG:32] (V4/H1). Esta tabla es la indirección: addr[idx] dice
 * dónde vive hoy el objeto y gen[idx] cuántas veces se ha reciclado ese slot, así
 * que usar un objeto liberado GRITA en vez de corromper en silencio (contrato B).
 *
 * Dónde vive (#451): DENTRO del bloque de la VM, entre el heap y las pilas,
 * creciendo hacia abajo hasta chocar con el heap — el límite es físico, no
 * adivinado. gen[cap] primero y addr[cap] detrás, a partir de heap_top. Arranca
 * en ~0,2 % del heap (#449) y se dobla cuando hace falta (un memmove, no un
 * realloc: el pico es el tamaño final). La free-list de slots reciclables es lo
 * único que sale del malloc de plataforma.
 *
 * Quién la usa: heap.c (register al crear, la presión de #430 en la puerta de
 * heap_alloc, el barrido de tabla en la fase 6 del GC), builtins y el intérprete
 * a través de los inline de deref (bpvm_internal.h: bpref_deref, bpvm_ref_dead,
 * bpref_regen), y el host de pruebas por --handlecap. Antes de U5 era un tramo de
 * heap.c y diez campos sueltos en bpvm_t: el censo U5.0 contó los consumidores y
 * no había más.
 *
 * ABI: el código AOT de los .mdn NO lee estos campos por offset (todo va por
 * helpers->…), así que este struct puede cambiar sin romper los .mdn; lo único
 * congelado en bpvm_t sigue siendo su prefijo (memory, aot_helpers).
 */
#ifndef BPVM_HANDLES_H
#define BPVM_HANDLES_H

#include <stdint.h>

struct bpvm;

/* #430 — tope de arranque de la tabla (slots). 0 = sin tope (host). Los puertos
 * con malloc de plataforma chico lo fijaban en su build; desde #451 la tabla vive
 * en el bloque y manda el límite físico, pero el mando sigue (--handlecap). */
#ifndef BPVM_HANDLE_CAP_MAX
#define BPVM_HANDLE_CAP_MAX 0u
#endif

/* V4 — bit 30 marca "es HANDLE de heap". null (0) y las CONSTANTES del data block
 * (dirección directa, inmutable/no-heap) tienen el bit a 0 → no necesitan tabla ni
 * generación. La memoria es <256KB (0x40000) → una dirección real jamás lo tiene. */
#define BPVM_HANDLE_TAG 0x40000000u

typedef struct {
    uint32_t* addr;        /* addr[idx] = dirección física del objeto; 0 = slot libre */
    uint32_t* gen;         /* gen[idx] = generación del slot; el handle lleva la suya */
    uint32_t  cap;         /* capacidad de addr Y gen */
    uint32_t  next;        /* siguiente slot fresco; 0 reservado para null */
    uint32_t* free_list;   /* slots reciclables (pila LIFO): owner-free empuja, register reusa */
    uint32_t  free_top;
    uint32_t  free_cap;
    uint32_t  cap_max;     /* #430 — tope (slots; 0 = sin tope); runtime, por --handlecap */
    uint32_t  pressure;    /* #430 — LA MARCA cruzada: register la arma al repartir uno de los
                            * últimos 64 slots, la puerta de heap_alloc la resuelve (colecta /
                            * crece / OOM). Una colecta por cruce, cero aritmética por alloc. */
    int       oom_avisado; /* el aviso de "sin sitio para más handles", una vez POR EJECUCIÓN:
                            * un static sería una vez por ARRANQUE y en la Pico calló (29-ago) */
    /* V7/H12 (#661) — con más de un run, los números que mata un `owner` ESPERAN AL GC: su generación sube en el acto
     * (las referencias rancias mueren ya: el contrato B), pero su dirección sigue en la tabla y no vuelven a la lista
     * libre hasta que el GC, con el mundo parado y antes de marcar, los aplica (bpvm_handles_aplicar_pendientes). Así
     * un paso que esté a mitad en otro run no ve su número reciclado ni la dirección 0 (V7_IDEAS §H12 «El camino
     * fino», la pieza b). Con un run, como siempre: en el acto. */
    uint32_t* pend_list;
    uint32_t  pend_top;
    uint32_t  pend_cap;
} bpvm_handles_t;

void    bpvm_handles_init(struct bpvm* vm);     /* tabla vacía (lazy), el tope del build, la marca sin cruzar */
void    bpvm_handles_destroy(struct bpvm* vm);  /* la free-list; addr/gen se van con el bloque (#451) */

/* V4/paso4c: registra un objeto de HEAP y devuelve su HANDLE 64b (gen(slot)<<32 |
 * idx|TAG). Reusa slots de la free-list si los hay. Devuelve bpref_t (NO uint32) a
 * propósito: asignarlo a un uint32_t es error de compilación → el compilador caza
 * cada sitio que perdería la generación. Ref nula = sin slot ni sitio para crecer
 * (OOM atrapable, con la excepción prefabricada de #430). */
bpref_t bpvm_handle_register(struct bpvm* vm, uint32_t addr);
/* V7/H7 #593 — lo mismo con el vm_lock YA tomado: para bpvm_heap_nuevo, el método único de la reserva. */
bpref_t bpvm_handle_register_bajo_cerrojo(struct bpvm* vm, uint32_t addr);
/* Paso 3 — marca MUERTO el índice de un handle (owner-free): bump de gen + el slot a
 * la free-list. No-op para null y constantes. Idempotente. V7/H12, 2b: devuelve 1 si
 * lo mató ESTA llamada (estaba vivo; mirarlo y matarlo, juntos bajo el cerrojo) y 0 si
 * no: con dos runs, de dos que sueltan el mismo objeto a la vez sólo sigue el primero. */
int     bpvm_handle_kill(struct bpvm* vm, bpref_t r);
/* #430 — crecimiento (×2, hacia abajo, dentro del bloque). 1 = hay sitio para
 * registrar; 0 = no cabe (choca con el heap vivo) o lo impide el tope. NO grita:
 * quien llama decide si es OOM (la puerta de heap_alloc) o aviso urgente
 * (register, como último recurso). Bajo el vm_lock. */
int     bpvm_handles_grow(struct bpvm* vm);
/* Fase 6 del GC — un slot VIVO cuyo bloque quedó sin marcar es inalcanzable → kill
 * (bump gen + addr=0 + free-list), para que un handle rancio GRITE. ANTES del sweep
 * de heap. Bajo el STW. */
void    bpvm_handles_gc_sweep(struct bpvm* vm);
/* V7/H12 (#661) — los números que mató un `owner` con varios runs, de vuelta: su dirección a 0 y su número a la lista
 * libre. La llama el GC (bpvm_gc_cuerpo) con el mundo parado, ANTES de marcar: una referencia rancia ya no alcanza el
 * bloque, y el barrido lo libera. */
void    bpvm_handles_aplicar_pendientes(struct bpvm* vm);
/* SÓLO PRUEBAS (test/test_palabras.c): cada uno, lo de antes, para ver la prueba en rojo. */
extern int bpvm_sabotaje_reciclar;   /* #661: con varios runs, el número de un `owner` soltado, reciclado en el acto */
extern int bpvm_sabotaje_desgarro;   /* #660: una referencia sin TAG con la palabra alta distinta de 0, viva */
/* #430 — el tope en runtime; cliente real: el host de pruebas (--handlecap). */
void    bpvm_set_handle_cap_max(struct bpvm* vm, uint32_t slots);
/* H13/#17 — cuando el deref grita, DECIR QUÉ handle (bpvm_util.c): índice, la gen
 * del handle, la del slot y el next — distingue un truncamiento de un uso tras
 * liberar, que son bugs distintos con arreglos distintos. */
void    bpvm_uaf_report(uint32_t idx, uint32_t gen_handle, uint32_t gen_slot, uint32_t handle_next);
/* V7/H12 (#660) — una referencia que no puede existir (a medio escribir por otro run, o un índice fuera de la tabla):
 * decirlo, por el canal normal y las tres primeras veces (bpvm_util.c). */
void    bpvm_ref_rota_report(uint32_t alta, uint32_t baja, uint32_t handle_next);

#endif /* BPVM_HANDLES_H */
