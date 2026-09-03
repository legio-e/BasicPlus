/*
 * bpvm_handles.c — LA TABLA DE HANDLES, en su módulo (V6/U5). Ver bpvm_handles.h.
 *
 * Todo lo que hay aquí vivía como un tramo de heap.c (registro, baja, crecimiento,
 * barrido del GC, la presión de #430) con sus diez campos sueltos en bpvm_t. El
 * censo U5.0 contó los consumidores —heap.c 45, el header 20, bpvm.c 11— y esto es
 * todo lo que había: no era que estuviera repartida, es que no tenía nombre. El
 * código es el mismo; los comentarios de cada paso (#430, #449, #451) se quedan
 * porque son la historia de por qué la tabla está DENTRO del bloque de la VM.
 */
#include "bpvm_internal.h"
#include "bpvm_alloc.h"   /* bpvm_realloc / bpvm_free: la free-list sale del malloc de plataforma */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Tabla vacía (lazy: se aloja en el primer register), el tope del build y la
 * marca sin cruzar. Lo que hacía bpvm_init a mano. */
void bpvm_handles_init(bpvm_t* vm) {
    vm->handles.addr      = NULL;
    vm->handles.gen       = NULL;     /* paso 3: generación por índice (contrato B) */
    vm->handles.cap       = 0;
    vm->handles.next      = 1;        /* 0 = null */
    vm->handles.free_list = NULL;     /* paso 4c: free-list de slots reciclables */
    vm->handles.free_top  = 0;
    vm->handles.free_cap  = 0;
    vm->handles.cap_max   = BPVM_HANDLE_CAP_MAX;   /* #430: tope del puerto (0 = sin tope) */
    vm->handles.pressure  = 0u;                    /* #430: la marca, sin cruzar */
    vm->handles.oom_avisado = 0;
}

/* #451 — `addr` y `gen` NO SE LIBERAN: desde que la tabla vive dentro del bloque
 * de la VM apuntan a `vm->memory` y se van con él. La free-list sí sale del
 * malloc de plataforma. */
void bpvm_handles_destroy(bpvm_t* vm) {
    bpvm_free(vm->handles.free_list);
    vm->handles.free_list = NULL;
    vm->handles.free_top = vm->handles.free_cap = 0;
}

static void handle_kill_idx(bpvm_t* vm, uint32_t idx);   /* def. más abajo (junto a bpvm_handle_kill) */

/* Paso 6 — BARRIDO DE TABLA (handle-aware): un slot VIVO (addr!=0) cuyo bloque quedó
 * SIN marcar es inalcanzable → handle_kill_idx (bump gen + addr=0 + free-list) para que
 * un handle rancio a él GRITE (contrato B también para lo que recolecta el GC). Debe ir
 * ANTES del sweep de heap (que limpia el MARK_BIT). El bloque físico lo libera el sweep
 * de heap. Bajo el STW del GC → seguro reciclar ya. Espejo del barrido de tabla de miVM. */
void bpvm_handles_gc_sweep(bpvm_t* vm) {
    for (uint32_t idx = 1u; idx < vm->handles.next; idx++) {
        uint32_t a = vm->handles.addr[idx];
        if (a == 0u) continue;                                  /* slot libre */
        uint32_t hh = a - 4u;
        if (hh < vm->heap_start || hh >= vm->heap_next) continue;   /* defensivo */
        uint32_t tag = bpvm_read_u32_be(vm->memory + hh);
        if ((tag & BPVM_TAG_MARK_BIT) == 0u) {                  /* no alcanzable */
            handle_kill_idx(vm, idx);
        }
    }
}

/* #430 — tope de tabla en runtime; cliente real: el host de pruebas
 * (--handlecap), que reproduce el límite de una placa sin build especial. */
void bpvm_set_handle_cap_max(bpvm_t* vm, uint32_t slots) {
    if (vm) vm->handles.cap_max = slots;
}

/* #430 — LA MARCA: repartir un slot de los ultimos BPVM_HANDLE_MARCA arma la
 * presion (una comparacion por slot fresco). Cuando ni colectando ni creciendo
 * hay slot, el OOM se cuenta con la excepcion PREFABRICADA en el prologo del
 * RUN (idea de Eduardo, exceptions.c): construida cuando construir era gratis,
 * lanzarla no aloja nada. */
#define BPVM_HANDLE_MARCA   64u

/* #430 — crecimiento de la tabla (x2), UNICO para el gate y para register.
 * Devuelve 1 si tras la llamada hay sitio para registrar; 0 si no se puede:
 * por el TOPE del puerto (handle_cap_max, p.ej. la SRAM de la Pico no da para
 * el par de tablas de 65536) o porque el malloc de plataforma dijo que no.
 * NO grita aqui: el que llama decide si es OOM (la puerta de heap_alloc) o
 * aviso urgente (register como ultimo recurso). Se llama BAJO el vm_lock. */
/* #449 — EL TAMANO DE LA TABLA, PROPORCIONAL AL HEAP.
 *
 * Criterio de Eduardo (28-ago): *"estamos poniendo la misma tabla para un micro
 * de 520K y otro de 8M+520K. El tamano de la tabla debe ser proporcional al
 * tamano del heap."*
 *
 * Antes eran 4096 slots FIJOS de arranque, en las cuatro familias. Para la
 * Pico 2 (heap 267 KB) eso son 32 KB de tabla nada mas empezar — el 12 % del
 * heap, y salidos del malloc de plataforma, que alli son 64 KB: se los comia y
 * empujaba el heap de malloc DENTRO del bloque de la VM (#440). Para la Metro
 * (heap 8 MB) el mismo numero se queda corto y la tabla crece a las primeras de
 * cambio.
 *
 * La regla: la tabla no puede pasar del 12,5 % del heap (son 8 B por slot entre
 * las dos, o sea `heap/64` slots), y arranca en un octavo de ese tope. Con eso
 * hay UN numero que justificar en vez de dos constantes en dos ficheros.
 *
 *   Pico 2  heap  267 KB -> tope 4.272 slots (34 KB), arranque   534 (4 KB)
 *   Metro   heap    8 MB -> tope 131.072     ( 1 MB), arranque 16.384
 *
 * La cota demostrable seria `heap / BPVM_MIN_FREE_BLOCK` (nunca puede haber mas
 * objetos vivos que eso), pero para la Pico son 22.784 slots = 182 KB sobre 267
 * de heap: existe, y es inutil de tan generosa. */
/* #451 — YA SOLO DECIDE EL ARRANQUE, no el tope. El tope proporcional se retiró
 * al meter la tabla en el bloque: era una política que hacía falta mientras la
 * tabla salía de OTRA bolsa y había que adivinar cuánto podía gastar de ella.
 * Adivinaba mal, además —asumía 64 B por objeto y los reales eran 25—, así que
 * un programa de objetos pequeños se quedaba sin handles con el heap medio
 * vacío. Ahora el límite es físico: la tabla baja hasta chocar con el heap. */
static uint32_t handle_slots_por_heap(const bpvm_t* vm) {
    uint32_t heap = (vm->heap_top > vm->heap_start)
                    ? (uint32_t) (vm->heap_top - vm->heap_start) : 0u;
    uint32_t ini = heap / 512u;                 /* ~0,2 % del heap para empezar */
    if (ini < 256u) ini = 256u;                 /* piso: crecer desde 1 es tonto */
    return ini;
}

/* #451 — LA TABLA VIVE DENTRO DEL BLOQUE DE LA VM, entre el heap y las pilas:
 *
 *     [ modulos ][ heap →→→ ][ TABLA ][ pila del main ][ pilas del resto ]
 *                 heap_start  heap_top                  stack_base
 *
 * y dentro de la region, `gen` primero y `addr` despues:
 *
 *     [ gen[cap] ][ addr[cap] ]      = cap*8 bytes
 *     heap_top                       stack_base
 *
 * POR QUE SE MOVIO (29-ago). Salia del `malloc` de PLATAFORMA, que en la Pico 2
 * es un margen de 64 KB compartido con la tabla de simbolos. `synclisttest`
 * moria con `No space in heap` **teniendo el heap al 20 %**: 200 KB parados
 * mientras la tabla, que vive en otra bolsa, no podia crecer. Dos memorias
 * separadas que no se prestan nada.
 *
 * 🎁 Y ADEMAS EL CRECIMIENTO YA NO CUESTA EL DOBLE. Un `realloc` necesita el
 * array viejo y el nuevo a la vez —doblar de 256 a 512 entradas pedia 99 KB en
 * un margen de 64, que es exactamente lo que fallaba—. Aqui la tabla crece
 * HACIA ABAJO y las regiones se solapan: un `memmove` y ya. El pico de memoria
 * es el tamaño final, no el doble.
 *
 * El orden de los dos movimientos NO es libre: el sitio nuevo de `addr` es el
 * sitio viejo de `gen`, asi que `gen` se muda primero. */
int bpvm_handles_grow(bpvm_t* vm) {
    uint32_t antes    = vm->handles.cap;
    uint32_t new_cap  = antes ? antes * 2u : handle_slots_por_heap(vm);

    /* Un tope del puerto sigue mandando si lo hay (hoy ninguno lo pone). El
     * tope proporcional al heap SE RETIRA: era una politica necesaria cuando la
     * tabla salia de otra bolsa y habia que adivinar cuanto podia gastar. Ahora
     * el limite es fisico y honesto —chocar con el heap— y no hay que adivinar. */
    if (vm->handles.cap_max != 0u && new_cap > vm->handles.cap_max)
        new_cap = vm->handles.cap_max;
    if (new_cap <= antes) return 0;

    /* Dónde caeria el techo del heap con la tabla nueva. Alineado a 8 para que
     * los dos `uint32_t*` queden alineados: en ARM/RISC-V un acceso de 32 bits
     * desalineado no es "lento", es un fallo. */
    uint32_t nuevo_top = (vm->stack_base - new_cap * 8u) & ~7u;

    /* ⚠️ EL HEAP ACABA DONDE EMPIEZA LA TABLA — y un heap VACIO no tiene sitio
     * fijo, asi que su final se mueve con ella.
     *
     * `bpvm_init` deja `heap_start = heap_next = stack_base` como marcador de
     * "aun no hay modulos": el heap es un intervalo VACIO pegado al techo, y es
     * el loader quien lo baja al cargar. Mientras siga asi, cualquier tabla
     * caeria por debajo de ese marcador y se rechazaria — con eso
     * `handle_register` devolvia ref nula SIEMPRE y `test-smphandles` paso de 0
     * corrupciones a 200.000 (todas). No era una carrera: era que la tabla no
     * llegaba a existir.
     *
     * Un heap sin nada dentro se desplaza gratis. La condicion es estricta a
     * proposito —vacio Y por encima del techo nuevo—, o sea el caso del marcador
     * y ninguno mas: con modulos cargados `heap_start` esta muy por debajo. */
    if (vm->heap_next == vm->heap_start && vm->heap_start >= nuevo_top) {
        vm->heap_start = nuevo_top;
        vm->heap_next  = nuevo_top;
    }

    /* El limite real: la tabla no puede comerse el heap VIVO ni la reserva de
     * emergencia (la que permite CONSTRUIR el error de OOM, #355).
     *
     * 📌 `<` y no `<=`: que la tabla empiece EXACTAMENTE donde acaba el heap vivo
     * es el caso normal, no una colision — es lo que significa "el heap acaba
     * donde empieza la tabla". Con `<=` se rechazaba a si mismo justo despues de
     * deslizar el heap vacio, que es como se vio: la frase mal dicha y el
     * operador mal puesto eran el mismo error. */
    if (nuevo_top < vm->heap_next + vm->heap_reserve || nuevo_top >= vm->stack_base) {
        bpvm_diag("[bpvm] tabla de handles: %u -> %u slots (%u KB) NO CABE "
                  "— el heap vivo llega a %u y la tabla querria bajar a %u",
                  (unsigned) antes, (unsigned) new_cap,
                  (unsigned)((new_cap * 8u) / 1024u),
                  (unsigned) vm->heap_next, (unsigned) nuevo_top);
        return 0;
    }

    uint8_t* mem = vm->memory;
    uint32_t viejo_top = vm->heap_top;
    /* gen PRIMERO (su destino esta por debajo de todo lo vivo), addr despues
     * (su destino es justo donde estaba gen). Con `memmove` porque origen y
     * destino pueden solaparse. */
    if (antes != 0u) {
        memmove(mem + nuevo_top,                    /* gen nuevo  */
                mem + viejo_top,                    /* gen viejo  */
                (size_t) antes * 4u);
        memmove(mem + nuevo_top + new_cap * 4u,     /* addr nuevo */
                mem + viejo_top + antes * 4u,       /* addr viejo */
                (size_t) antes * 4u);
    }
    /* Los slots nuevos nacen a cero: gen 0 y addr 0 es "slot libre". */
    memset(mem + nuevo_top + antes * 4u, 0, (size_t)(new_cap - antes) * 4u);
    memset(mem + nuevo_top + new_cap * 4u + antes * 4u, 0,
           (size_t)(new_cap - antes) * 4u);

    vm->heap_top     = nuevo_top;
    vm->handles.gen   = (uint32_t*)(void*)(mem + nuevo_top);
    vm->handles.addr  = (uint32_t*)(void*)(mem + nuevo_top + new_cap * 4u);
    vm->handles.cap   = new_cap;

    /* ⚠️ Esta linea se emitia ANTES de saber si habia funcionado, y en el log de
     * la Pico se leia como un crecimiento consumado mientras la tabla seguia
     * igual. Ahora sale cuando ya es un hecho. */
    bpvm_diag("[bpvm] tabla de handles: %u -> %u slots (%u KB dentro del heap) OK "
              "— techo del heap %u, libre %u KB",
              (unsigned) antes, (unsigned) new_cap,
              (unsigned)((new_cap * 8u) / 1024u), (unsigned) nuevo_top,
              (unsigned)((nuevo_top - vm->heap_next) / 1024u));
    return 1;
}

/* V4/paso4c — registra un objeto de HEAP y devuelve su HANDLE 64b (gen(slot)<<32 |
 * idx|TAG). Reusa un slot de la free-list si lo hay (con su gen ya bumpeada); si no,
 * crece la tabla. Devuelve bpref_t: al asignarlo a un uint32_t da error de compilación
 * → el compilador caza cada sitio que perdería la generación. Si no puede crecer,
 * devuelve ref NULA (0) → viaja por el contrato "ref 0 = OOM" (#430; antes
 * devolvía la dirección cruda y las refs MENTIAN). */
bpref_t bpvm_handle_register(bpvm_t* vm, uint32_t addr) {
    /* Paso 7 — la tabla (free-list/handle_next) es estado COMPARTIDO: bajo SMP dos
     * workers registran a la vez (handle_register corre FUERA del vm_lock, heap_alloc
     * ya lo soltó) → carrera en free_top → roban el mismo idx. Serializamos con el
     * vm_lock (no-op en single-worker → coste cero en el default de envío). El GC usa
     * handle_kill_idx (sin lock) porque corre bajo STW (todos parados). */
    bpref_t r;
    bpvm_smp_lock(vm);
    uint32_t idx;
    if (vm->handles.free_top > 0u) {
        idx = vm->handles.free_list[--vm->handles.free_top];   /* REUSO: slot reciclado, gen ya bumpeada */
    } else {
        if (vm->handles.next >= vm->handles.cap) {
            /* #430 — con la presion de tabla en la puerta de heap_alloc este
             * crecimiento es el ULTIMO RECURSO (registros en vuelo mas alla del
             * margen). Si no se puede, el "paso siguiente" que prometia #355:
             * ref NULA → el llamador la trata como el OOM que ya sabe tratar.
             * Nada de direcciones crudas — las refs no mienten. */
            if (!bpvm_handles_grow(vm)) {
                bpvm_diag_urgente("[bpvm] tabla de handles AGOTADA (%u slots, tope %u): "
                          "ref nula -> OOM atrapable",
                          (unsigned) vm->handles.cap, (unsigned) vm->handles.cap_max);
                r.v = 0u; bpvm_smp_unlock(vm); return r;
            }
        }
        idx = vm->handles.next++;
        vm->handles.gen[idx] = 0u;   /* slot fresco */
        /* #430 — LA MARCA (idea de Eduardo): repartir un slot de la zona final
         * anuncia la frontera — arma el flag que la puerta de heap_alloc
         * consulta para colectar (o crecer) ANTES de que la tabla se llene.
         * Una comparacion por slot FRESCO (el reuso de free-list ni pasa por
         * aqui: si hay reciclados, no hay presion). El margen cubre los
         * registros en vuelo alloc→register. */
        if (idx + BPVM_HANDLE_MARCA >= vm->handles.cap) vm->handles.pressure = 1u;
    }
    /* Paso 7c — A1: publica el slot con RELEASE → todo lo escrito ANTES (init del objeto)
     * es visible para quien lo lea con ACQUIRE (bpref_deref). Mitad ESCRITOR del apretón.
     * (El unlock de abajo ya da release; esto lo hace EXPLÍCITO y sobrevive a quitar el
     * lock en 7b.2.) Inerte en x86; barrera en ARM/RISC-V. */
    __atomic_store_n(&vm->handles.addr[idx], addr, __ATOMIC_RELEASE);
    r.v = ((uint64_t) vm->handles.gen[idx] << 32) | (uint64_t)(idx | BPVM_HANDLE_TAG);
    bpvm_smp_unlock(vm);
    return r;
}

/* Paso 4c/6 — recicla un slot de la tabla por índice: BUMP de la generación (handles
 * rancios dejan de matchear → gritan), addr=0 (slot libre: vivo ⟺ addr!=0, lo usa el
 * barrido de tabla del GC) y RECICLA el índice a la free-list para reuso. Lo comparten
 * el owner-bpvm_free(bpvm_handle_kill) y el barrido de tabla del GC (paso 6). */
static void handle_kill_idx(bpvm_t* vm, uint32_t idx) {
    if (vm->handles.gen == NULL || idx == 0u || idx >= vm->handles.next) return;
    vm->handles.gen[idx]++;                        /* gen bumpeada → handles rancios mueren */
    vm->handles.addr[idx] = 0u;                    /* slot libre en la tabla (paso 6) */
    if (vm->handles.free_top >= vm->handles.free_cap) {
        uint32_t nc = vm->handles.free_cap ? vm->handles.free_cap * 2u : 256u;
        uint32_t* nl = (uint32_t*) bpvm_realloc(vm->handles.free_list, (size_t) nc * sizeof(uint32_t));
        if (!nl) return;   /* sin free-list no reciclamos este slot (se pierde, no se corrompe) */
        vm->handles.free_list = nl;
        vm->handles.free_cap  = nc;
    }
    vm->handles.free_list[vm->handles.free_top++] = idx;   /* reciclar el slot */
}

/* Paso 4c — libera el slot de un handle (owner-free): delega en handle_kill_idx.
 * No-op para null/constantes. El TAG_FREE_BIT del bloque físico evita el doble-free real. */
void bpvm_handle_kill(bpvm_t* vm, bpref_t r) {
    if ((r.v & BPVM_HANDLE_TAG) == 0u) return;
    bpvm_smp_lock(vm);   /* paso 7: serializa la free-list contra registers/kills de otros workers */
    /* Paso 7b.1 — FREE CON GENERACIÓN VALIDADA (refuerzo de la maqueta): solo el PRIMER
     * kill de un handle vivo actúa; un kill RANCIO (slot ya reciclado, gen no matchea) es
     * NO-OP seguro — si no, bumpearía la gen del ocupante NUEVO y lo corromperría (doble
     * free-list, etc). check+kill atómicos bajo el lock = el compareAndSet(slot,g,g+1). */
    if (!bpvm_ref_dead(vm, r)) {
        handle_kill_idx(vm, (uint32_t) r.v & ~BPVM_HANDLE_TAG);
    }
    bpvm_smp_unlock(vm);
}
