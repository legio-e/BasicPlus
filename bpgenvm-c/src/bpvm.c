/*
 * bpvm.c — implementación de la API pública.
 *
 * En F1 mantenemos malloc/free para la estructura de control bpvm_t.
 * El memory[] sigue siendo del caller. F2+ podría ofrecer una variante
 * "todo-en-buffer-del-caller" para targets sin libc.
 */

#include "bpvm_out.h"
#include "bpvm_internal.h"
#include "bpvm_aot_helpers.h"   /* H3 #158: tabla helpers para AOT */
#include "bpvm_pack.h"          /* H3.c: resolución de imports contra la zona de packs */
#include "bpvm_fs.h"            /* #310: un pack ejecutable vive en /app del FS */
#include "crc32.h"              /* V7 #650: el CRC del módulo de la zona */
#include "bpvm_gpio.h"    /* V7/H4: el estado de los pines se limpia al fin de RUN */
#include "bpvm_adc.h"     /* V7 #659: y el del ADC */
#include "bpvm_pwm.h"     /* V7 #667: y lo que el programa encendió y nadie soltó */
#include "bpvm_pulse.h"
#include "bpvm_neopixel.h"
#include "bpvm_uart.h"
#include "bpvm_net.h"     /* V8/H3: y lo que siga abierto en la red */
#include "bpvm_wifi.h"    /* V8/H3: y la WiFi, si el programa la tocó */
#include "bpvm_bt.h"      /* V8/H2: y el Bluetooth, igual */
#include "bpvm_teclado.h" /* V8/H2, paso 2: y el teclado del SO, sin lo del programa que acaba */
#include "bpvm_raton.h"   /* V8/H2, paso 3: y el ratón del SO, igual */
#include "bpvm_sonido.h"  /* V8/H4 (#708): y el sonido que siga sonando */
#include "bpvm_bus.h"     /* V7/H6.c G4.a1: y los buses del programa se sueltan */
#include "bpvm_spi.h"
#include "bpvm_i2c.h"
#include "bpvm_alloc.h"         /* #339: guardián de fin de RUN */
#include "bpvm_nativa.h"        /* V7/H5 P2b.3: los ejecutores de natives se van con la VM */
#include "bpvm_entry.h"         /* #344: el RUN, escrito UNA vez */
#ifdef BPVM_GUI
#include "bpvm_gui.h"          /* #352: el modelo del GUI se va con la VM */
#endif
#include "bpvm_platform.h"      /* #339: candado de hoja de la lista de bloques */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>             /* #353: bpvm_diag es variádica */

/* ==========================================================================
 * #339 — GUARDIÁN DE FIN DE RUN (idea de Eduardo, 28-jul)
 *
 * "No preguntes cuánta memoria libre queda: pon un contador. Desde la marca de
 *  arranque del programa, todo bloque posterior que siga vivo es memoria que se
 *  quedó sin limpiar."
 *
 * Eso es literalmente lo que hay aquí: cada reserva del núcleo lleva delante
 * una cabecera con su NÚMERO DE SECUENCIA, su tamaño y dónde se pidió, y va
 * encadenada en una lista de bloques vivos. bpvm_init anota la secuencia del
 * momento; bpvm_destroy recorre la lista y canta lo que tenga secuencia >= esa
 * marca.
 *
 * La alternativa (preguntarle al sistema la memoria libre antes y después) se
 * DESCARTÓ: da un número que ensucian otras tareas y la fragmentación, obliga a
 * una cintura por micro —justo lo que la arquitectura BP evita— y sólo sabe
 * decir cuánto falta, no quién se lo quedó.
 *
 * Coste: la cabecera por bloque. Las reservas C del núcleo son POCAS y GRANDES
 * (tabla de handles, símbolos, imports, EH stacks), así que es despreciable;
 * la memoria del programa BP no pasa por aquí (vive en memory[], el bloque del
 * llamante, que el siguiente RUN reinicia entero).
 * ========================================================================== */

#define BPVM_BLK_MAGIC 0x424C4B21u   /* "BLK!" — cazar free() de puntero ajeno */

/* Las DOS bases de contador (idea de Eduardo). Muy separadas a propósito: el
 * rango de la secuencia ES la etiqueta de familia, sin campo extra. Con 2^62 de
 * hueco no hay forma de que una alcance a la otra ni en una eternidad de RUNs. */
#define BPVM_SEQ_OS_BASE  ((uint64_t) 1)
#define BPVM_SEQ_VM_BASE  ((uint64_t) 1 << 62)

typedef struct bpvm_blk {
    struct bpvm_blk* prev;
    struct bpvm_blk* next;
    uint64_t         seq;      /* número de secuencia: EL flag de Eduardo.
                                * >= BPVM_SEQ_VM_BASE → bloque de la VM. */
    size_t           size;     /* bytes útiles (sin cabecera) */
    const char*      file;     /* dónde se pidió... */
    int              line;     /* ...para poder señalar con el dedo */
    uint32_t         magic;
} bpvm_blk_t;

#define BLK_IS_VM(b) ((b)->seq >= BPVM_SEQ_VM_BASE)

/* Alineación: la cabecera se redondea para que el payload salga alineado como
 * lo estaría un malloc normal (double/uint64 en todas las familias). */
#define BPVM_BLK_HDR ((sizeof(bpvm_blk_t) + 15u) & ~(size_t)15u)

static bpvm_blk_t* g_blk_head = NULL;
static uint64_t    g_seq_os   = BPVM_SEQ_OS_BASE;
static uint64_t    g_seq_vm   = BPVM_SEQ_VM_BASE;
static uint32_t    g_live[2]  = { 0, 0 };   /* [VM], [OS] */
static uint64_t    g_bytes[2] = { 0, 0 };

/* Candado de HOJA: sólo protege el empalme en la lista, nunca se tiene cogido
 * mientras se hace otra cosa. Por eso no puede interbloquear con vm_lock (que
 * no es recursivo).
 *
 * El huevo y la gallina: crear el candado necesita un mutex de plataforma, que
 * RESERVA memoria, que querría el candado. Se corta con un pestillo de
 * reentrada — durante esa única reserva se pasa sin candado, que es correcto
 * porque ocurre en la primera reserva del proceso, cuando aún no hay más de un
 * hilo. Y como esa reserva es de familia OS, tampoco ensucia nunca el veredicto
 * sobre la memoria del programa. */
static bpvm_platform_mutex_handle_t g_blk_mtx = NULL;
static int g_blk_mtx_ready = 0;
static int g_blk_mtx_busy  = 0;

static int blk_lock(void) {
    if (!g_blk_mtx_ready) {
        if (g_blk_mtx_busy) return 0;          /* reentrada: aún single-thread */
        g_blk_mtx_busy = 1;
        int ok = (bpvm_platform_mutex_init(&g_blk_mtx) == 0);
        g_blk_mtx_busy = 0;
        if (!ok) return 0;   /* sin mutex: seguimos; el guardián no tumba la VM */
        g_blk_mtx_ready = 1;
    }
    bpvm_platform_mutex_lock(&g_blk_mtx);
    return 1;
}
static void blk_unlock(int locked) {
    if (locked) bpvm_platform_mutex_unlock(&g_blk_mtx);
}

static void blk_link(bpvm_blk_t* b, bpvm_alloc_kind_t k) {
    int lk = blk_lock();
    b->seq  = (k == BPVM_ALLOC_VM) ? g_seq_vm++ : g_seq_os++;
    b->prev = NULL;
    b->next = g_blk_head;
    if (g_blk_head) g_blk_head->prev = b;
    g_blk_head = b;
    g_live[k]++;
    g_bytes[k] += (uint64_t) b->size;
    blk_unlock(lk);
}

static void blk_unlink(bpvm_blk_t* b) {
    int k = BLK_IS_VM(b) ? BPVM_ALLOC_VM : BPVM_ALLOC_OS;
    int lk = blk_lock();
    if (b->prev) b->prev->next = b->next; else g_blk_head = b->next;
    if (b->next) b->next->prev = b->prev;
    g_live[k]--;
    g_bytes[k] -= (uint64_t) b->size;
    blk_unlock(lk);
}

/* V7/H7 #598 — la inyección de fallos de las pruebas (bpvm_alloc.h). */
static const char* volatile g_fallar_en = NULL;
static volatile int g_fallar_veces = 0;
void bpvm_alloc_prueba_fallar(const char* fichero) { bpvm_alloc_prueba_fallar_veces(fichero, 1); }
void bpvm_alloc_prueba_fallar_veces(const char* fichero, int veces) { g_fallar_veces = veces; g_fallar_en = fichero; }
static int termina_en(const char* s, const char* fin) {
    size_t a = strlen(s), b = strlen(fin);
    return a >= b && strcmp(s + a - b, fin) == 0;
}

/* #639 (3-oct) — el TOPE de las pruebas (bpvm_alloc.h): de `fichero`, nada de mas de `tope` bytes de un bloque. */
static const char* volatile g_tope_en = NULL;
static size_t g_tope = 0;
void bpvm_alloc_prueba_tope(const char* fichero, size_t tope) { g_tope = tope; g_tope_en = fichero; }
static int pasa_del_tope(size_t n, const char* file) {
    return g_tope_en != NULL && file != NULL && n > g_tope && termina_en(file, g_tope_en);
}
/* #639 (3) — y el PRESUPUESTO: de `fichero`, nada más que `bytes` en total (la memoria que se ACABA). */
static const char* volatile g_presu_en = NULL;
static long g_presu = 0, g_gastado = 0;
void bpvm_alloc_prueba_presupuesto(const char* fichero, long bytes) { g_presu = bytes; g_gastado = 0; g_presu_en = fichero; }
long bpvm_alloc_prueba_gastado(void) { return g_gastado; }
static int pasa_del_presupuesto(size_t n, const char* file) {
    if (g_presu_en == NULL || file == NULL || !termina_en(file, g_presu_en)) return 0;
    if (g_gastado + (long) n > g_presu) return 1;
    g_gastado += (long) n;
    return 0;
}

void* bpvm_alloc_raw(size_t n, int zero, bpvm_alloc_kind_t k,
                     const char* file, int line) {
    if (g_fallar_en != NULL && file != NULL && termina_en(file, g_fallar_en)) {
        if (--g_fallar_veces <= 0) g_fallar_en = NULL;
        return NULL;
    }
    if (pasa_del_tope(n, file) || pasa_del_presupuesto(n, file)) return NULL;
    uint8_t* raw = (uint8_t*) malloc(BPVM_BLK_HDR + n);
    if (!raw) return NULL;
    bpvm_blk_t* b = (bpvm_blk_t*) raw;
    b->size = n; b->file = file; b->line = line; b->magic = BPVM_BLK_MAGIC;
    blk_link(b, k);
    if (zero) memset(raw + BPVM_BLK_HDR, 0, n);
    return raw + BPVM_BLK_HDR;
}

void bpvm_free(void* p) {
    if (!p) return;
    bpvm_blk_t* b = (bpvm_blk_t*) ((uint8_t*) p - BPVM_BLK_HDR);
    if (b->magic != BPVM_BLK_MAGIC) {
        /* Puntero que no salió de aquí. GRITA en vez de corromper la lista:
         * es un error de programación (reservado con malloc y liberado con
         * bpvm_free, o doble free), y en silencio sería indetectable. */
        bpvm_diag("[bpvm] bpvm_free: puntero que no reservó el núcleo (%p)", p);
        return;
    }
    blk_unlink(b);
    b->magic = 0;
    free(b);
}

void* bpvm_realloc_at(void* p, size_t n, const char* file, int line) {
    if (!p) return bpvm_alloc_raw(n, 0, BPVM_ALLOC_VM, file, line);
    bpvm_blk_t* old = (bpvm_blk_t*) ((uint8_t*) p - BPVM_BLK_HDR);
    if (old->magic != BPVM_BLK_MAGIC) {
        bpvm_diag("[bpvm] bpvm_realloc: puntero que no reservó el núcleo (%p)", p);
        return NULL;
    }
    if (pasa_del_tope(n, file) || pasa_del_presupuesto(n, file)) return NULL;   /* #639: el bloque de antes, intacto */
    bpvm_alloc_kind_t k = BLK_IS_VM(old) ? BPVM_ALLOC_VM : BPVM_ALLOC_OS;
    /* Se desencadena ANTES de realloc: puede mover el bloque, y los vecinos
     * apuntarían a memoria liberada. Al volver se re-encadena (secuencia NUEVA
     * de su misma familia: un realloc es un bloque nuevo para el guardián). */
    size_t keep_size = old->size;
    blk_unlink(old);
    uint8_t* raw = (uint8_t*) realloc((void*) old, BPVM_BLK_HDR + n);
    if (!raw) {
        /* realloc falló: el bloque original SIGUE vivo. Re-encadenarlo. */
        old->size = keep_size;
        blk_link(old, k);
        return NULL;
    }
    bpvm_blk_t* b = (bpvm_blk_t*) raw;
    b->size = n; b->file = file; b->line = line; b->magic = BPVM_BLK_MAGIC;
    blk_link(b, k);
    return raw + BPVM_BLK_HDR;
}

char* bpvm_strdup_at(const char* s, const char* file, int line) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* d = (char*) bpvm_alloc_raw(n, 0, BPVM_ALLOC_VM, file, line);
    if (d) memcpy(d, s, n);
    return d;
}

uint32_t bpvm_alloc_live_blocks(bpvm_alloc_kind_t k) { return g_live[k]; }
uint64_t bpvm_alloc_live_bytes (bpvm_alloc_kind_t k) { return g_bytes[k]; }

uint64_t bpvm_alloc_mark(void) { return g_seq_vm; }

/* #353 (canal de diagnóstico) y #338 (zona de rascar compartida) vivían aquí.
 * Se mudaron a src/bpvm_util.c: los usan el gestor de placa y las cinturas del
 * FS, que no tienen por qué enlazar la VM entera para pedir un buffer o soltar
 * un aviso. Las declaraciones siguen en bpvm.h. */

/* El guardián de #339 mantiene su propio sink porque el test lo CAPTURA para
 * comprobar el veredicto palabra por palabra; si compartiera el de diag, el
 * test se comería también los mensajes del loader. Su default, eso sí, es el
 * canal común: así un firmware que instale el sink de diag se lleva el
 * veredicto del guardián al log sin tener que acordarse de dos cosas. */
/* "%s" y no `linea` directa: el veredicto lleva rutas y nombres de fichero, y
 * un '%' suelto ahí convertiría el aviso en lectura de basura de la pila. */
static void report_via_diag(const char* linea) { bpvm_diag("%s", linea); }
static bpvm_alloc_report_fn g_report = report_via_diag;

void bpvm_alloc_set_report(bpvm_alloc_report_fn fn) {
    g_report = fn ? fn : report_via_diag;
}

uint64_t bpvm_alloc_sweep(uint64_t mark) {
    char linea[224];
    uint32_t n = 0;
    uint64_t bytes = 0;
    const bpvm_blk_t* peor = NULL;

    int lk = blk_lock();
    for (const bpvm_blk_t* b = g_blk_head; b; b = b->next) {
        if (!BLK_IS_VM(b)) continue;              /* plataforma: no es del programa */
        if (b->seq < mark) continue;              /* de antes del RUN: tampoco */
        n++;
        bytes += (uint64_t) b->size;
        if (!peor || b->size > peor->size) peor = b;
    }
    uint32_t os_n = g_live[BPVM_ALLOC_OS];
    blk_unlock(lk);

    /* Habla SIEMPRE. Un guardián que sólo abre la boca cuando hay problema no
     * se distingue de uno averiado — la lección de #326 (todo instrumento
     * necesita su control). Los bloques de plataforma van como DATO al final,
     * nunca mezclados con el veredicto. */
    if (n == 0) {
        snprintf(linea, sizeof linea,
                 "[bpvm] fin de RUN: la memoria del programa vuelve a su sitio "
                 "(0 bloques sin liberar; plataforma: %lu vivos)",
                 (unsigned long) os_n);
        g_report(linea);
        return 0;
    }
    snprintf(linea, sizeof linea,
             "[bpvm] fin de RUN: FUGA — %lu bloque(s) del programa sin liberar, %lu B; "
             "el mayor (%lu B) se pidio en %s linea %d (plataforma: %lu vivos)",
             (unsigned long) n, (unsigned long) bytes,
             (unsigned long) (peor ? peor->size : 0),
             peor && peor->file ? peor->file : "?",
             peor ? peor->line : 0,
             (unsigned long) os_n);
    g_report(linea);
    return bytes;
}

/* V7 #651 (8-oct) — EL CENSO (ver bpvm_alloc.h). Los orígenes, en una tabla fija: contar no puede pedir memoria al
 * guardián que cuenta. Los que no caben en ella van al resto. */
#define CENSO_MAX 128
static struct { const char* file; int line; uint32_t n; uint64_t bytes; } g_censo[CENSO_MAX];

uint32_t bpvm_alloc_censo(uint64_t mark, int max, bpvm_alloc_report_fn fn) {
    char linea[200];
    bpvm_alloc_report_fn out = fn ? fn : g_report;
    int no = 0;
    uint32_t total = 0, resto_n = 0;
    uint64_t total_b = 0, resto_b = 0;
    int lk = blk_lock();
    for (const bpvm_blk_t* b = g_blk_head; b; b = b->next) {
        if (!BLK_IS_VM(b) || b->seq < mark) continue;
        total++;
        total_b += (uint64_t) b->size;
        int k = 0;
        while (k < no && !(g_censo[k].file == b->file && g_censo[k].line == b->line)) k++;
        if (k == no) {
            if (no == CENSO_MAX) { resto_n++; resto_b += (uint64_t) b->size; continue; }
            g_censo[no].file = b->file; g_censo[no].line = b->line; g_censo[no].n = 0; g_censo[no].bytes = 0;
            no++;
        }
        g_censo[k].n++;
        g_censo[k].bytes += (uint64_t) b->size;
    }
    /* los que más bloques, primero (pocos: una inserción basta) */
    for (int i = 1; i < no; i++) {
        int j = i;
        while (j > 0 && g_censo[j - 1].n < g_censo[j].n) {
            const char* f = g_censo[j].file; int l = g_censo[j].line; uint32_t n = g_censo[j].n; uint64_t by = g_censo[j].bytes;
            g_censo[j] = g_censo[j - 1];
            g_censo[j - 1].file = f; g_censo[j - 1].line = l; g_censo[j - 1].n = n; g_censo[j - 1].bytes = by;
            j--;
        }
    }
    blk_unlock(lk);
    snprintf(linea, sizeof linea, "[censo] %lu bloque(s) de la VM vivos, %lu B, desde %d sitio(s)",
             (unsigned long) total, (unsigned long) total_b, no + (resto_n ? 1 : 0));
    out(linea);
    for (int i = 0; i < no; i++) {
        if (i >= max) { resto_n += g_censo[i].n; resto_b += g_censo[i].bytes; continue; }
        const char* f = g_censo[i].file ? g_censo[i].file : "?";
        const char* barra = strrchr(f, '/');
        if (barra) f = barra + 1;
        snprintf(linea, sizeof linea, "[censo] %5lu bloque(s) %7lu B  %s:%d",
                 (unsigned long) g_censo[i].n, (unsigned long) g_censo[i].bytes, f, g_censo[i].line);
        out(linea);
    }
    if (resto_n) {
        snprintf(linea, sizeof linea, "[censo] %5lu bloque(s) %7lu B  (el resto)", (unsigned long) resto_n, (unsigned long) resto_b);
        out(linea);
    }
    return total;
}

/* ENV `stack` — el usuario fija el tamaño de la región de pilas, en KB.
 *
 * 0 (o la clave ausente) = la regla de siempre, que es lo que estaba antes de
 * esto: nadie que no ponga la variable nota ningún cambio.
 *
 * POR QUE SE PUEDE QUERER TOCAR: el reparto por defecto era 25% pilas / 75% heap,
 * y esa proporción es una apuesta sobre el programa. Un programa con muchos
 * hilos se queda sin pilas teniendo heap de sobra; uno de un solo hilo que
 * mueve datos desperdicia el 25%. En la Pico 2 son 89 KB de los 358 del bloque,
 * y no había forma de moverlos sin recompilar.
 *
 * V8 #740 (29-oct): esa apuesta ya no existe —la región es la pila del main y las
 * de los hilos salen del heap—, así que lo que pide la clave es la pila del main.
 * Eduardo: «la mantenemos de momento, ya veremos mas adelante que hacemos».
 *
 * La regla sigue viviendo AQUI y en un solo sitio: el ENV la alimenta, no la
 * duplica. Las familias sólo leen la clave y la pasan (ver bpvm_set_stack_kb). */
static unsigned long g_stack_kb = 0;    /* 0 = regla por defecto */
static int           g_stack_avisado = 0;

void bpvm_set_stack_kb(unsigned long kb) {
    g_stack_kb = kb;
    g_stack_avisado = 0;
}

/* #462 — QUANTUM AJUSTABLE, y de momento es un INSTRUMENTO más que un ajuste.
 *
 * El quantum de la VM-C se mide en OPCODES (1024), no en tiempo como el de la
 * VM-Java — está anotado desde hace tiempo en `bpvm_internal.h`. Eso es barato y
 * predecible mientras cada opcode cueste lo mismo… y deja de serlo cuando UNO
 * cuesta milisegundos: `Gui.run()` es `while __guiRunOnce() do endwh`, unos 3-4
 * opcodes por vuelta de los que uno bombea LVGL entero. Con 1024 opcodes por
 * quantum salen ~290 vueltas de bombeo antes de que el planificador drene los
 * eventos encolados — y el handler de un clic espera todo eso.
 *
 * Medido en la STM32 Discovery el 31-ago con `samples/GuiEvLat.bp`: **393, 393,
 * 420, 416, 393 ms** de latencia upcall→handler. Tan constante que no puede ser
 * carga: es un número FIJO de vueltas.
 *
 * Este mando existe para PROBAR esa explicación en placa en vez de creérsela: si
 * la latencia baja proporcionalmente al bajar el quantum, la causa es ésta. Si no
 * baja, la explicación es falsa y hay que buscar en otro sitio. */
static long g_quantum_ops = 0;          /* 0 = por defecto (1024) */

void bpvm_set_quantum_ops(long ops) {
    g_quantum_ops = ops;
}

/* V7/H12, etapa 2b (5-oct; Eduardo: «ponemos el 2º run, conseguimos que funcione con lo mínimo y vamos ampliando la
 * cobertura») — los runs permanentes: `vm.runs` del ENV. Dos, como mucho: uno por núcleo en las placas de dos (la
 * etapa 3), y en la 2b, los dos en uno. */
#define BPVM_RUNS_MAX 2
static int g_runs = 1;

void bpvm_set_runs(long n) {
    if (n == 0) return;                                  /* sin la clave: los de siempre */
    if (n < 1 || n > BPVM_RUNS_MAX) {
        bpvm_diag("[bpvm] vm.runs=%ld fuera de rango (1..%d): se queda en %d", n, BPVM_RUNS_MAX, g_runs);
        return;
    }
    g_runs = (int) n;
}

int bpvm_runs(void) { return g_runs; }

/* V7/H12 — `vm.cores=1` en el ENV: la vuelta atrás de los dos núcleos sin regrabar (Eduardo, 5-oct, para la Pico; 6-oct,
 * al encender los dos en la S3 y la P4): las tareas de la VM, todas en el núcleo 0. 0 = sin la clave (los que haya). La
 * Pico lo lee en su arranque (pico/main.c); la familia ESP32, de aquí, al crear cada tarea (su FreeRTOS no cambia la
 * afinidad de una tarea ya creada). */
static int g_cores = 0;
void bpvm_set_cores(long n) {
    if (n == 0) return;
    if (n != 1 && n != 2) {
        bpvm_diag("[bpvm] vm.cores=%ld fuera de rango (1 o 2): se queda en %d", n, g_cores);
        return;
    }
    g_cores = (int) n;
}
int bpvm_cores(void) { return g_cores; }

size_t bpvm_stack_region_bytes(size_t total_bytes) {
    if (g_stack_kb) {
        size_t pedido = (size_t) g_stack_kb * 1024u;
        /* Los topes NO son paternalismo: por debajo del stack del main la VM ni
         * arranca (bpvm_init devuelve NULL), y por encima de la mitad el heap se
         * queda sin nada y no arranca el programa. Un valor del env no puede
         * dejar la placa inutil — se ajusta y SE DICE. */
        size_t suelo = (size_t) BPVM_MAIN_STACK_BYTES;
        size_t techo = total_bytes / 2u;
        size_t r = pedido;
        if (r < suelo) r = suelo;
        if (r > techo) r = techo;
        if (r != pedido && !g_stack_avisado) {
            g_stack_avisado = 1;      /* una vez, no en cada INFO */
            bpvm_diag("[bpvm] env stack=%lu KB fuera de rango: se usan %u KB "
                      "(permitido %u..%u KB con un bloque de %u KB)",
                      g_stack_kb, (unsigned)(r >> 10), (unsigned)(suelo >> 10),
                      (unsigned)(techo >> 10), (unsigned)(total_bytes >> 10));
        }
        return r;
    }
    /* V8 #740 (29-oct) — LA DEL MAIN, Y NADA MÁS (Eduardo: «Los hilos BP no podrian tomar la RAM del heap? […] 64K en
     * el C3 y C6 que van muy justas de RAM es muchísimo»). Era el 25 % del bloque, entre 64 y 512 KB, para el main y
     * las pilas de los hilos, se crearan o no; ahora cada hilo pide la suya al heap (bpvm_thread_spawn), y la RAM que
     * gastan es proporcional a los hilos. Lo mismo con PSRAM (Eduardo: «en la práctica no habría límite en el número de
     * hilos BP»). */
    size_t r = (size_t) BPVM_MAIN_STACK_BYTES;
    if (r >= total_bytes) r = total_bytes / 2u;     /* bloque diminuto: mitad */
    return r;
}

bpvm_t* bpvm_init(uint8_t* memory, size_t memory_size, size_t stack_base) {
    if (memory == NULL || memory_size < 4096) return NULL;
    if (stack_base == 0) stack_base = memory_size / 2;
    if (stack_base >= memory_size) return NULL;
    if (stack_base + BPVM_MAIN_STACK_BYTES > memory_size) return NULL;
    /* V7/H12, etapa 2a.2 — LA COLA, permanente: en una placa ya la creó el arranque; en el PC, la primera VM. Sin ella
     * no hay dónde colgar el programa. (Su cerrojo y su condición son de la plataforma: el guardián de #339 no los
     * cuenta como del programa.) */
    if (bpvm_cola_asegurar() != 0) return NULL;

    /* #339 — la marca se toma AQUÍ, antes de reservar nada nuestro, para que la
     * propia estructura del vm quede DENTRO de la ventana: si algún día se
     * escapara, el guardián la vería como cualquier otro bloque. */
    uint64_t run_mark = bpvm_alloc_mark();

    bpvm_t* vm = (bpvm_t*) bpvm_calloc(1, sizeof(bpvm_t));
    if (!vm) return NULL;
    vm->run_mark = run_mark;
    /* H3.c — ventana XIP vacía (lo>hi imposible con lo=MAX): el guardián de PC
     * rechaza todo fuera de memory_size hasta que un loader XIP la abra. */
    vm->xip_lo = 0xFFFFFFFFu;
    vm->xip_hi = 0;

    vm->memory = memory;
    vm->memory_size = memory_size;
    vm->stack_base = (uint32_t) stack_base;
    /* #451 — EL TECHO DEL HEAP SE SEPARA DE `stack_base`. Hasta hoy eran el
     * mismo número y por eso 27 sitios preguntaban por `stack_base` queriendo
     * decir «dónde acaba el heap». Son dos conceptos distintos: `stack_base` es
     * dónde empieza la pila del main; `heap_top` es hasta dónde puede crecer el
     * heap. Coinciden AHORA (este paso no cambia ningún comportamiento), y dejan
     * de coincidir en cuanto la tabla de handles se aloje entre los dos. */
    vm->heap_top = (uint32_t) stack_base;
    vm->next_free_address = BPVM_INITIAL_FREE_ADDR;
    vm->heap_start = (uint32_t) stack_base;   /* sin módulos cargados aún */
    vm->heap_next  = vm->heap_start;
    vm->heap_bloques = 0;                     /* V7/H7 #593: el heap, vacío */
    /* H3 (V2): el loader fijará el umbral real con el heap_start tras cargar. */
    bpvm_huecos_vaciar(vm);   /* V8 #718: las listas de huecos, vacías */
    vm->last_gc_heap_next = vm->heap_next;
    vm->alloc_since_gc    = 0u;   /* #357 */
    vm->gc_bump_threshold = 0;   /* off hasta entonces */
    vm->heap_reserve = 0;        /* #355: se arma al fijar el heap real, no aquí */
    vm->overlay_cs = 0xFFFFFFFFu;   /* V8 H9 (#786): sin overlay, LEA_STR no llega */
    vm->main_absolute_address = 0;
    /* V4 — tabla de handles (lazy; V6/U5: en su módulo, bpvm_handles.c) + GC
     * suspendido durante la migración. */
    bpvm_handles_init(vm);                      /* #430: tope del puerto, la marca sin cruzar, tabla vacía */
    vm->oom_exc.v = 0u;                         /* #430: la prefabricada, aun no */
    vm->heap_exc.v = 0u;                        /* #494: idem, la del heap descarrilado */
    vm->gc_suspended = 0;        /* paso 6: GC reactivado (handle-aware: mark + barrido de tabla) */

    /* Pone los bytes sentinela en la región reservada:
     *   memory[0] = THREAD_EXIT (fin de Thread.run / hilo)
     *   memory[1] = NATIVE_RETURN (retorno del puente native→BP)
     *   memory[2] = EVENT_RETURN  (vuelta de un handler de evento, H5.c) */
    memory[0] = BPVM_SENTINEL_THREAD_EXIT;
    memory[BPVM_SENTINEL_NATIVE_RETURN_ADDR] = BPVM_SENTINEL_NATIVE_RETURN;
    memory[BPVM_SENTINEL_EVENT_RETURN_ADDR]  = BPVM_SENTINEL_EVENT_RETURN;
    bpvm_event_queue_init(vm);
    /* V7 #592 — y la MARCA del 2º temporizador, a 0 y dada de alta (el reloj de la plataforma la sube cada ~5 ms). */
    bpvm_marca_tiempo_alta(vm);

    /* Thread main: región fija MAIN_STACK_BYTES a partir de stack_base. V7/H12, etapa 2: su struct, con malloc (a la
     * cola entra al colgar el programa: threading.c); sin memoria para ella no hay VM. */
    if (bpvm_hilos_crear_main(vm, (uint32_t) stack_base) != 0) {
        bpvm_free(vm);
        return NULL;
    }

    /* (V8 #740: aquí arrancaba el alocador de las pilas de los hilos, tras la del main; ahora cada Thread.start() pide la
     * suya al heap: bpvm_thread_spawn.) */

    /* H3 #158 — apuntar a la tabla global de helpers para AOT. El
     * código AOT C-emitido la usa vía vm->aot_helpers->func(...). */
    vm->aot_helpers = &bpvm_aot_helpers_v2;

    /* V6/#478 — la traza de las fachadas sale por el camino del `print`. */
    bpvm_out_set_vm(vm);
    return vm;
}

/* Extrae el dirname de un path. dst debe tener al menos 256 bytes. */
static void path_dirname(const char* path, char* dst, size_t dst_size) {
    const char* last_sep = NULL;
    for (const char* p = path; *p; p++) {
        if (*p == '/' || *p == '\\') last_sep = p;
    }
    if (!last_sep) { dst[0] = '\0'; return; }
    size_t n = (size_t)(last_sep - path);
    if (n >= dst_size) n = dst_size - 1;
    memcpy(dst, path, n);
    dst[n] = '\0';
}

static void load_err(bpvm_t* vm, const char* fmt, ...);   /* definida más abajo (#421) */

/* Índice del módulo cargado con ese (library, name), o -1. */
static int module_index(const bpvm_t* vm, const char* library, const char* name) {
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* m = &vm->modules[i];
        if (strcmp(m->name, name) == 0
                && strcmp(m->library, library ? library : "") == 0) {
            return i;
        }
    }
    return -1;
}

/* Comprueba si un módulo con nombre dado ya está cargado. */
static int module_loaded(const bpvm_t* vm, const char* library, const char* name) {
    return module_index(vm, library, name) >= 0;
}

/* V7/H2 (#500) — LA NORMA de versiones de las dependencias, la misma que miVM y
 * con el MISMO texto por OUTPUT (ASCII, sin rutas):
 *   1. dep.gen < requerida → rechazo («mas antiguo»);
 *   2. misma gen y otra ifaceCrc → rechazo («no es la interfaz con la que se compilo»);
 *   3. gen mayor → se acepta si enlaza (los marcadores deciden);
 *   una dependencia MOD6/MOD7 no lleva datos: cuenta como gen 0 y CRC desconocido —
 *   si el importador exige gen > 0 es «mas antiguo»; si exige 0, se anota «sin
 *   datos de version» por el diag y no se comprueba la interfaz.
 * `canon` es el nombre lógico cualificado (`Core`, `acme.LibHdr`), la clave con
 * la que el compilador selló la subtabla `deps` del importador. */
static bpvm_status_t comprobar_norma(bpvm_t* vm, bpvm_module_t* imp, const char* canon,
                                     const bpvm_module_t* dep) {
    if (imp->fmt < 8) return BPVM_OK;          /* el importador no exige nada */
    int k = -1;
    for (int i = 0; i < imp->dep_count; i++) {
        if (strcmp(imp->dep_names[i], canon) == 0) { k = i; break; }
    }
    if (k < 0) return BPVM_OK;                 /* no lo selló (intrínsecos, o import sin dueño) */
    uint32_t req_gen = imp->dep_gen[k], req_crc = imp->dep_crc[k];
    int extiende = (imp->dep_flags[k] & 1u) != 0;
    int con_datos = (dep->fmt >= 8);
    uint32_t dep_gen = con_datos ? dep->gen : 0u;
    char msg[320];   /* la linea de OUTPUT entera; load_error recorta a lo suyo */
    msg[0] = '\0';
    if (dep_gen < req_gen) {
        snprintf(msg, sizeof msg,
                 "'%s' (gen %u) es mas antiguo que el que '%s' necesita (gen %u): actualiza /lib o recompila",
                 canon, (unsigned) dep_gen, imp->name, (unsigned) req_gen);
    } else if (!con_datos) {
        if (!imp->dep_avisado[k]) {
            imp->dep_avisado[k] = 1;
            bpvm_diag("[bpvm-c] dep '%s' sin datos de version (MOD%u): no se comprueba su interfaz",
                      canon, (unsigned) dep->fmt);
        }
    } else if (dep->fmt >= 9) {
        /* V7/H2 (#523, D9) — con ENLACE TARDIO la verdad la dice el enlace, simbolo a
         * simbolo, contra la clase que hay: ni la interfaz distinta (regla 2) ni el
         * padre mas nuevo (regla 3) se rechazan aqui. Queda la regla 1 y, como pista,
         * el diag de que se compilo contra otra interfaz. */
        if (dep->iface_crc != req_crc && !imp->dep_avisado[k]) {
            imp->dep_avisado[k] = 1;
            imp->enlazado_tarde = 1;
            bpvm_diag("[bpvm-c] dep '%s' (gen %u): compilado contra otra interfaz (%08x != %08x); enlace tardio",
                      canon, (unsigned) dep_gen, (unsigned) req_crc, (unsigned) dep->iface_crc);
        }
    } else if (dep_gen > req_gen && extiende) {
        /* Norma de Eduardo (15-sep): un padre nunca más reciente que sus hijos. */
        snprintf(msg, sizeof msg,
                 "'%s' (gen %u) es mas reciente que '%s' (compilado contra gen %u) y '%s' extiende una clase suya: recompila '%s'",
                 canon, (unsigned) dep_gen, imp->name, (unsigned) req_gen, imp->name, imp->name);
    } else if (dep_gen == req_gen && dep->iface_crc != req_crc) {
        snprintf(msg, sizeof msg,
                 "'%s' (gen %u, interfaz %08x) no es la interfaz con la que se compilo '%s' (interfaz %08x): recompila",
                 canon, (unsigned) dep_gen, (unsigned) dep->iface_crc, imp->name, (unsigned) req_crc);
    }
    if (msg[0]) {
        bpvm_out("%s\n", msg);
        load_err(vm, "%s", msg);
        return BPVM_ERR_DEP_VERSION;
    }
    return BPVM_OK;
}

/* Para un import qualified (e.g. "L2Lib.Counter.__init"), deriva el
 * (library, module) que apunta. Convención del frontend:
 *   - "Mod.sym"             → library="",  module="Mod"
 *   - "Lib.Mod.sym"         → library="Lib", module="Mod"
 * El SÍMBOLO es siempre el último componente y NUNCA lleva puntos: el
 * penúltimo es el módulo. Por eso un miembro de clase no viaja como
 * "Mod.Cls.sym" (se leería library=Mod, módulo=Cls) sino con un nombre
 * plano que el dueño exporta: "__cls_new_Cls", "__cls_m_Cls_m",
 * "__cls_s_Cls_m" (V7/H2, estáticos), "Cls#m#slot" (marcadores).
 * (Hasta el 14-sep-2026 este comentario prometía un caso "Lib.Mod.Cls.sym"
 * que el código de abajo nunca implementó.) */
static void derive_owner(const char* qualified, char* lib, size_t lib_size,
                         char* mod, size_t mod_size) {
    /* Split por '.'. parts[length-1] = símbolo, parts[length-2] = módulo,
     * el resto = library (juntado con '.'). */
    int dot_positions[16];
    int ndots = 0;
    for (const char* p = qualified; *p && ndots < 16; p++) {
        if (*p == '.') dot_positions[ndots++] = (int)(p - qualified);
    }
    lib[0] = '\0'; mod[0] = '\0';
    if (ndots < 1) return;   /* sin separador: no hay módulo. */
    int last = dot_positions[ndots - 1];
    int second_last = ndots >= 2 ? dot_positions[ndots - 2] : -1;
    /* Modulo = parts[ndots-2..ndots-1) = subcadena (second_last+1, last). */
    int mod_start = second_last + 1;
    int mod_len = last - mod_start;
    size_t mn = (size_t) mod_len;
    if (mn >= mod_size) mn = mod_size - 1;
    memcpy(mod, qualified + mod_start, mn);
    mod[mn] = '\0';
    if (ndots >= 2) {
        /* Library = subcadena [0, second_last). */
        int ll = second_last;
        size_t llz = (size_t) ll;
        if (llz >= lib_size) llz = lib_size - 1;
        memcpy(lib, qualified, llz);
        lib[llz] = '\0';
    }
}

/* #310 — carga UN módulo desde una fuente de pack. Definida más abajo, junto
 * al resto de lo de packs; se declara aquí porque discover_deps la usa para el
 * pack en ejecución y bpvm_load_pack para el módulo principal: UNA función
 * para los dos, que es lo que garantiza que se carguen igual. */
static bpvm_status_t load_from_pack(bpvm_t* vm, const bpvm_pack_src_t* src,
                                    const bpvm_pack_entry_t* e, const char* name);

/* Resuelve las dependencias del módulo `raiz` y las de todo lo que va cargando
 * (el cierre transitivo; #649: sin recursión, ver discover_deps), cargándolas
 * desde `search_dir` por convención de naming (<lib>.<mod>.mod o <mod>.mod). */
/* #421 — el cargador que DEJA DICHO por qué falló (en `vm->load_error`). Se
 * declara aquí porque `discover_deps`, de abajo, lo usa para las dependencias:
 * el fallo que de verdad le pasa a la gente no es el del módulo principal. */
static bpvm_status_t load_entry_file_det(bpvm_t* vm, const char* resolved_path);

/* [V7/H2, #493] ¿Dos ficheros con los mismos bytes? Tamaño y CRC32 por la
 * fachada: es lo que el IDE compara para decidir si sube, y lo que miVM hace
 * para el mismo aviso (Files.size + CRC32). */
static int mismo_contenido(const char* a, uint32_t asz, const char* b, uint32_t bsz) {
    if (asz != bsz) return 0;
    uint32_t ca = 0, cb = 0;
    if (bpvm_fs_crc32(a, &ca) != 0 || bpvm_fs_crc32(b, &cb) != 0) return 0;
    return ca == cb;
}

/* ═══ #649 (4-oct) — LAS DEPENDENCIAS, SIN RECURSIÓN ════════════════════════════════════════════════════════════════
 *
 * `discover_deps` se llamaba a sí misma por cada módulo que cargaba, con ~1,7 KB de marco (las rutas de abajo, en la
 * pila): en la Pico 2, la tarea que carga llegaba a ~12 KB de sus 16 con `TimeDemo`, y una cadena de 16 módulos (el
 * tope, BPVM_MAX_MODULES) pedía ~27 —más que su pila— (medido el 4-oct, en la puerta de H12). Ahora, el MISMO recorrido
 * —el mismo orden de cargas, de avisos y de comprobaciones de la norma— con una pila de marcos explícita y pequeña. Un
 * marco son los imports de un módulo (el `k` de su `n`), o la RISTRA de módulos que acaba de traer una carga: desde
 * `idx` hasta el module_count de CADA momento, como el `for` de antes, que volvía a mirar el tope en cada vuelta. Por el
 * camino los índices sólo crecen —un marco de imports es siempre de un módulo cargado después que el de debajo—, así que
 * no hay más de BPVM_MAX_MODULES de cada clase. */
typedef struct {
    uint8_t ristra;   /* 0: los imports del módulo `idx`; 1: los recién cargados, desde `idx` */
    uint8_t idx;      /* < BPVM_MAX_MODULES */
    uint8_t core;     /* #652: el del módulo raíz de un programa —acabados sus imports, `Core` si aún no está— */
    int     k, n;     /* los imports: el siguiente y cuántos (`n`, al empezar el marco: si cargamos un dep,
                       * vm->modules crece pero el mod actual no muta) */
} dd_marco_t;
#define DD_MARCOS (2 * BPVM_MAX_MODULES + 1)

static int dd_empuja(bpvm_t* vm, dd_marco_t* pila, int* sp, int ristra, int idx) {
    if (*sp >= DD_MARCOS) {   /* no pasa (los índices crecen por el camino); si pasara, que se diga */
        load_err(vm, "dependencias: el recorrido pasa de %d marcos", DD_MARCOS);
        return -1;
    }
    dd_marco_t* f = &pila[(*sp)++];
    f->ristra = (uint8_t) ristra;
    f->idx    = (uint8_t) idx;
    f->core   = 0u;
    f->k      = 0;
    f->n      = ristra ? 0 : vm->modules[idx].import_count;
    return 0;
}

/* #652 (7-oct) — «NO PUEDE HABER PROGRAMAS SIN CORE» (Eduardo, 4-oct). Un módulo de hoy lo arrastra siempre (el
 * compilador pone `import Core` en todo módulo desde H1, y su __init llama al de Core), pero uno de antes no: los de la
 * cápsula de V6 que no usan nada de él —`Hello.mod`— se cargaban SIN Core (las dos VMs: «enlace: 1 modulos»), sin
 * RuntimeError con el que construir una excepción («OOM sin prefabricar»). Con `con_core`, el marco del módulo raíz de
 * un programa, acabados sus imports, carga `Core` si aún no está: por el MISMO camino que un import (el pack en
 * ejecución, /app, /lib, la zona), y sin la norma de #500 —el raíz no se compiló contra él—. No se llama a su __init:
 * Core no tiene globales que inicializar (si algún día las tiene, habrá que llamarlo aquí). Espejo de asegurarCore
 * (miVM). La IMPORTACIÓN no se añade a la tabla del módulo: esa lista ES su tabla externa, y el enlace escribe una
 * dirección por entrada. */
static bpvm_status_t discover_deps(bpvm_t* vm, int raiz, const char* search_dir, int con_core) {
    dd_marco_t pila[DD_MARCOS];
    int sp = 0;
    if (dd_empuja(vm, pila, &sp, 0, raiz) != 0) return BPVM_ERR_OOM;
    pila[0].core = (uint8_t) (con_core ? 1 : 0);
    while (sp > 0) {
        dd_marco_t* t = &pila[sp - 1];
        if (t->ristra) {   /* el `for (j = desde; j < vm->module_count; j++)` de antes: el tope, en CADA vuelta */
            if (t->idx >= vm->module_count) { sp--; continue; }
            int j = t->idx++;
            if (dd_empuja(vm, pila, &sp, 0, j) != 0) return BPVM_ERR_OOM;
            continue;
        }
        int forzado = 0;                        /* #652: el Core que el programa no trae */
        const char* imp;
        if (t->k < t->n) {
            imp = vm->modules[t->idx].imports[t->k++];
        } else if (t->core) {
            t->core = 0u;
            if (module_index(vm, "", "Core") >= 0) continue;
            imp = "Core.__init";
            forzado = 1;
        } else {
            sp--;
            continue;
        }
        int mod_idx = t->idx;                   /* el importador: el parámetro de la versión recursiva */
        if (!imp || !imp[0]) continue;
        char lib[64], mod[64];
        derive_owner(imp, lib, sizeof(lib), mod, sizeof(mod));
        if (!mod[0]) continue;
        char canon[132];
        if (lib[0]) snprintf(canon, sizeof canon, "%s.%s", lib, mod);
        else        snprintf(canon, sizeof canon, "%s", mod);
        {
            int ya = module_index(vm, lib, mod);
            if (ya >= 0) {
                /* V7/H2 (#500) — también cuando ya estaba cargada (otro importador
                 * la trajo): cada importador exige lo suyo. */
                bpvm_status_t nr = forzado ? BPVM_OK : comprobar_norma(vm, &vm->modules[mod_idx], canon, &vm->modules[ya]);
                if (nr != BPVM_OK) return nr;
                continue;
            }
        }
        /* Derivar filename + nombre de entrada en pack (mismo base-name).
         * `pname_file` = el nombre de fichero SIN directorio, que es lo que
         * come la regla de búsqueda común cuando search_dir no acierta. */
        char filename[512], pname[160], pname_file[168];
        if (lib[0]) {
            snprintf(filename, sizeof(filename), "%s%s%s.%s.mod",
                     search_dir, search_dir[0] ? "/" : "", lib, mod);
            snprintf(pname, sizeof(pname), "%s.%s", lib, mod);
        } else {
            snprintf(filename, sizeof(filename), "%s%s%s.mod",
                     search_dir, search_dir[0] ? "/" : "", mod);
            snprintf(pname, sizeof(pname), "%s", mod);
        }
        snprintf(pname_file, sizeof(pname_file), "%s.mod", pname);
        /* ═══ [V7/H2, #493] EL ORDEN DE BÚSQUEDA, UNO SOLO Y ESCRITO ═══════════
         *
         * Decisión de Eduardo (15-sep): «El orden siempre ha sido /app → /lib →
         * packs. Si el módulo principal está en un pack: pack del principal →
         * /app → /lib → packs. /sys es para ficheros de configuración, no hace
         * falta incluirlo en el path.»
         *
         *   0) el PACK EN EJECUCIÓN, si lo hay — sus módulos primero: un pack es
         *      como un docker, autocontenido, y nada de fuera le cambia las
         *      tripas (#310);
         *   1) /app — el directorio del módulo principal (`search_dir`), el
         *      base-dir del proyecto si es otro, y `/app` a secas;
         *   2) /lib — la stdlib (en el PC, `--libdir`; en miVM, `stdlibDir`);
         *   3) la ZONA de packs grabados, XIP: el código se queda en flash.
         *
         * Lo que YA NO está, a propósito: el cwd («tal cual») — con un `.mod`
         * homónimo en el directorio actual miVM lo cogía y la VM-C cogía el de
         * la carpeta del programa: mismo `.mod`, mismo cwd, stdout distinto—;
         * `/sys` (#418, era el último candidato); y `fromPath`, que es la pista
         * del COMPILADOR (`import X from "..."`) y no de la VM: miVM lo honraba
         * relativo al cwd de la JVM y la VM-C lo tiraba. Y la zona ya no se
         * disfraza de `/app`: contesta sólo a `pack:<Pack>/x`, así que el paso
         * 1 dice la verdad y el 3 vuelve a ser alcanzable (llevaba muerto
         * desde el 13-ago, #362: todo módulo de pack se copiaba a RAM). Es el
         * MISMO orden que `ModuleManager.resolveModulePath` en miVM. */
        if (vm->run_pack_on) {
            bpvm_pack_entry_t pe;
            int hay = vm->run_pack_only[0]
                ? bpvm_pack_find_in_src(&vm->run_pack_src, vm->run_pack_only, "mod", pname, &pe)
                : bpvm_pack_find_src(&vm->run_pack_src, "mod", pname, &pe);
            if (hay) {
                int idx0 = vm->module_count;
                bpvm_status_t rs = load_from_pack(vm, &vm->run_pack_src, &pe, pname);
                if (rs != BPVM_OK) return rs;
                bpvm_diag("[bpvm-c] dep '%s' -> pack EN EJECUCION", pname);
                /* V7/H2 (#500) — la norma también aquí: el pack lleva su versión
                 * y el importador exige la suya (antes este paso se la saltaba). */
                {
                    int ya = module_index(vm, lib, mod);
                    if (ya >= 0) {
                        bpvm_status_t nr = forzado ? BPVM_OK : comprobar_norma(vm, &vm->modules[mod_idx], canon, &vm->modules[ya]);
                        if (nr != BPVM_OK) return nr;
                    }
                }
                /* #649 — sus dependencias, antes que el siguiente import: un marco, no una llamada. */
                if (dd_empuja(vm, pila, &sp, 1, idx0) != 0) return BPVM_ERR_OOM;
                continue;                       /* resuelto dentro del pack */
            }
        }
        /* 3) la zona: se pregunta SIEMPRE (barato: recorrer un índice) para
         * poder decir «eclipsa» cuando el FS gana, que es el aviso de
         * desarrollo de siempre — y ahora es verdad, porque la zona ya no
         * contesta al stat de /app. */
        uint32_t pk_region_size = 0;
        const uint8_t* pk_region = bpvm_pack_mounted(&pk_region_size);
        uint32_t pk_len = 0;
        const uint8_t* pk_mod = pk_region
            ? bpvm_pack_find(pk_region, pk_region_size, "mod", pname, &pk_len)
            : NULL;

        /* 1) /app y 2) /lib, por la FACHADA (#344: así las cinco familias corren
         * este mismo código). `en_lib` recuerda si ganó /lib, para el aviso. */
        char found[BPVM_FS_PATH_MAX];   /* #520 (e): era 192 y truncaba una ruta de proyecto de 200+ («IO error») */
        uint32_t fsz = 0;
        int have_fs = 0, en_lib = 0;
        /* La precisión explícita no es cosmética: sin ella gcc avisa de posible
         * truncado en CADA build, y un build ruidoso es exactamente donde se
         * escondió la rotura de la Pico en #339. Truncar aquí es lo correcto —
         * `filename` ya pasó el stat, así que cabe salvo ruta absurda. */
        if (bpvm_fs_stat(filename, &fsz) == 0) {
            snprintf(found, sizeof found, "%.*s", (int)(sizeof found - 1), filename);
            have_fs = 1;
        }
        if (!have_fs) {
            const char* bd = bpvm_fs_basedir();
            if (bd && bd[0] && strcmp(bd, search_dir) != 0) {
                snprintf(found, sizeof found, "%.*s/%s", (int)(sizeof found - 2 - strlen(pname_file)), bd, pname_file);
                have_fs = (bpvm_fs_stat(found, &fsz) == 0);
            }
        }
        if (!have_fs) {
            snprintf(found, sizeof found, "/app/%s", pname_file);
            have_fs = (bpvm_fs_stat(found, &fsz) == 0);
        }
        if (!have_fs) {
            snprintf(found, sizeof found, "/lib/%s", pname_file);
            have_fs = en_lib = (bpvm_fs_stat(found, &fsz) == 0);
        }

        if (!have_fs && !pk_mod) {
            bpvm_diag("[bpvm-c] dep '%s' (%s) NO ENCONTRADO: %s",
                    imp, mod, filename);
            continue;   /* dejamos que linkAll dispare el error si falta. */
        }
        /* EL AVISO DE #493, por OUTPUT y con el MISMO texto en miVM: un módulo
         * que se resuelve fuera de /lib teniendo un GEMELO en /lib está tapando
         * a la stdlib — el caso real de la ficha (una `Collections` vieja en
         * /app y una tarde mirando otro sitio). Criterio «gemelo tapado» y no
         * una lista de nombres: mudo donde no hay /lib (el host sin --libdir,
         * el arnés de paridad, el sim), y suena justo donde tapa. Va por
         * OUTPUT (stdout / la consola del IDE), no por el diag: es para el
         * usuario. ASCII y sin la ruta resuelta (cada VM resuelve una distinta;
         * el texto tiene que ser byte-idéntico). */
        if (have_fs && !en_lib) {
            char gemelo[BPVM_FS_PATH_MAX]; uint32_t gsz = 0;   /* #520 (e) */
            snprintf(gemelo, sizeof gemelo, "/lib/%s", pname_file);
            /* «Tapa» = hay OTRO fichero en /lib con ese nombre y es DISTINTO
             * (tamaño o CRC). Una copia idéntica no tapa nada (y puede ser el
             * mismo fichero visto por dos rutas: el principal que vive en /lib,
             * o el /lib del PC montado con --libdir — la revisión lo cazó
             * avisando de `Core` al ejecutar un módulo desde /lib). */
            if (strcmp(gemelo, found) != 0 && bpvm_fs_stat(gemelo, &gsz) == 0
                    && !mismo_contenido(found, fsz, gemelo, gsz))
                bpvm_out("aviso: '%s' cargado desde /app, tapa al de /lib\n", pname);
        }
        /* RASTRO — una línea por dependencia resuelta, diciendo DE DÓNDE.
         *
         * Hasta ahora esto sólo hablaba cuando fallaba o cuando el FS eclipsaba
         * al pack. O sea que en un arranque normal no se veía QUÉ módulos
         * entraron ni por dónde, y cuando algo se tuerce entre la carga y el
         * primer `print` no hay nada que mirar. Eduardo, cazándome
         * especulando: *«hay que empezar viendo el error y acotando... lo que
         * necesitamos son más mensajes en el log que nos permitan ver un
         * rastro»*. Tenía razón: esto es barato y se queda. */
        /* V7 #650 (8-oct) — el del FS y el de la zona, IGUALES (Eduardo: «Misma versión, de momento mismo tamaño y
         * CRC»): el del PACK, que corre en su sitio (XIP); el del FS se copiaría entero a la RAM. El tamaño primero,
         * que es gratis; el CRC, sólo si casa. Si difieren en algo, el del FS, como siempre: está ahí a propósito, y
         * el diag dice «eclipsa». */
        if (have_fs && pk_mod && fsz == pk_len) {
            uint32_t crc_fs = 0;
            if (bpvm_fs_crc32(found, &crc_fs) == 0 && crc_fs == bpvm_crc32(pk_mod, pk_len)) {
                bpvm_diag("[bpvm-c] '%s' del FS es igual al del pack: se carga del pack (XIP, sin copiarlo a la RAM)",
                          pname);
                have_fs = 0;
            }
        }
        bpvm_diag("[bpvm-c] dep '%s' -> %s", pname,
                  have_fs ? found : "PACK (XIP)");
        int idx_before = vm->module_count;
        bpvm_status_t s;
        if (have_fs) {
            if (pk_mod) {
                bpvm_diag("[bpvm-c] aviso: '%s' del FS eclipsa al del pack",
                        pname);
            }
            s = load_entry_file_det(vm, found);   /* #421: deja el porqué en la VM */
        } else {
            /* H3.c tanda 2 — carga XIP: el código se queda EN LA REGIÓN (flash
             * en placa); a RAM solo van ext-table + data block. */
            s = bpvm_loader_load_xip(vm, pk_mod, pk_len, pname);
            if (s == BPVM_OK) {
                bpvm_diag("[bpvm-c] '%s' cargado XIP desde pack (codigo en sitio)",
                        pname);
            }
        }
        if (s != BPVM_OK) return s;
        /* V7/H2 (#500) — la norma, con la cabecera de la dependencia recién leída
         * (antes de recurrir a sus imports, como miVM). */
        {
            int ya = module_index(vm, lib, mod);
            if (ya >= 0) {
                bpvm_status_t nr = forzado ? BPVM_OK : comprobar_norma(vm, &vm->modules[mod_idx], canon, &vm->modules[ya]);
                if (nr != BPVM_OK) return nr;
            }
        }
        /* Las deps de esta nueva carga, antes que el siguiente import (#649: un marco, no una llamada). */
        if (dd_empuja(vm, pila, &sp, 1, idx_before) != 0) return BPVM_ERR_OOM;
    }
    return BPVM_OK;
}

/* ── #310: ejecutar un pack ──────────────────────────────────────────────── */

/* Lectura por trozos de un pack que vive en el FS. La fuente pide offsets
 * dentro del PACK; aquí se traducen a offsets del fichero, que son los mismos. */
static long pack_fs_read_at(void* user, uint32_t off, uint8_t* dst, uint32_t n) {
    const bpvm_pack_fs_t* st = (const bpvm_pack_fs_t*) user;
    return bpvm_fs_read_at(st->path, off, dst, n);
}

int bpvm_pack_open_fs(bpvm_pack_src_t* src, bpvm_pack_fs_t* st, const char* path) {
    if (!src || !st || !path) return -1;
    size_t n = strlen(path);
    if (n + 1 > sizeof st->path) return -1;      /* ruta larga: mejor decirlo */
    memcpy(st->path, path, n + 1);
    uint32_t size = 0;
    if (bpvm_fs_stat(path, &size) != 0 || size == 0) return -1;
    bpvm_pack_src_stream(src, pack_fs_read_at, st, size);
    return 0;
}

/* Lectura de UNA entrada del pack: el loader pide offsets desde 0 (el .mod
 * empieza donde empieza), así que aquí se le suma dónde vive esa entrada. */
typedef struct { const bpvm_pack_src_t* src; uint32_t base; } pack_entry_rd_t;

static long pack_entry_read_at(void* user, uint32_t off, uint8_t* dst, uint32_t n) {
    pack_entry_rd_t* e = (pack_entry_rd_t*) user;
    return e->src->read_at(e->src->user, e->base + off, dst, n);
}

/* ── #310 paso 4: los RECURSOS del pack en ejecución también van primero ──
 * Los recursos no se resuelven por donde los módulos (readFile y compañía van
 * por bpvm_fs), así que se engancha ahí: un overlay que la fachada consulta
 * ANTES del backend. Un sitio, y todos los lectores de recursos lo heredan
 * sin tocar ni uno.
 *
 * Del path sólo importa el BASENAME, porque dentro del pack una entrada es
 * (tipo, nombre) = (extensión, nombre sin extensión) — así lo empaqueta
 * PackStep. `/app/x/main.win` → ("win", "main"). */

static int pack_res_entry(bpvm_t* vm, const char* path, bpvm_pack_entry_t* out) {
    if (!vm->run_pack_on || !path) return 0;
    /* Nunca reclamar el fichero del PROPIO pack: se lee por el FS de verdad y
     * reclamarlo aquí sería morderse la cola. */
    if (strcmp(path, vm->run_pack_st.path) == 0) return 0;
    char tipo[BPVM_PACK_TYPE_LEN + 1], nombre[BPVM_PACK_NAME_LEN + 1];
    char pack[BPVM_PACK_NAME_LEN + 1];
    if (!bpvm_pack_key_de_path(path, tipo, nombre, pack)) return 0;
    /* Cualificado: sólo es mío si el pack en ejecución es ÉSE. Si se pide otro,
     * aquí se dice que no y el recurso sigue su camino (FS → zona). */
    if (pack[0])
        return (vm->run_pack_only[0] && strcmp(pack, vm->run_pack_only) != 0) ? 0
             : bpvm_pack_find_in_src(&vm->run_pack_src, pack, tipo, nombre, out);
    /* V7/H2 (#493): si el pack en ejecución es uno DE LA ZONA, sólo el suyo. */
    if (vm->run_pack_only[0])
        return bpvm_pack_find_in_src(&vm->run_pack_src, vm->run_pack_only, tipo, nombre, out);
    return bpvm_pack_find_src(&vm->run_pack_src, tipo, nombre, out);
}

static int pack_res_stat(void* user, const char* path, uint32_t* size) {
    bpvm_pack_entry_t e;
    if (!pack_res_entry((bpvm_t*) user, path, &e)) return -1;   /* no es mío */
    if (size) *size = e.len;
    return 0;
}

static long pack_res_read(void* user, const char* path, uint32_t off,
                          uint8_t* dst, uint32_t cap) {
    bpvm_t* vm = (bpvm_t*) user;
    bpvm_pack_entry_t e;
    if (!pack_res_entry(vm, path, &e)) return -1;
    if (off >= e.len) return 0;
    uint32_t n = e.len - off;
    if (n > cap) n = cap;
    const uint8_t* p = bpvm_pack_src_ptr(&vm->run_pack_src, e.data_off + off, n);
    if (p) { memcpy(dst, p, n); return (long) n; }       /* mapeado: en sitio */
    return vm->run_pack_src.read_at(vm->run_pack_src.user,
                                    e.data_off + off, dst, n);
}

/* La regla de con/sin código vive AQUÍ y en un solo sitio: no se mira de dónde
 * viene el pack, se le PREGUNTA a la fuente si da puntero. */
static bpvm_status_t load_from_pack(bpvm_t* vm, const bpvm_pack_src_t* src,
                                    const bpvm_pack_entry_t* e, const char* name) {
    const uint8_t* xip = bpvm_pack_src_ptr(src, e->data_off, e->len);
    if (xip) return bpvm_loader_load_xip(vm, xip, e->len, name);
    pack_entry_rd_t rd = { src, e->data_off };
    return bpvm_load_mod_stream(vm, pack_entry_read_at, &rd, e->len, name);
}

bpvm_status_t bpvm_load_pack(bpvm_t* vm, const char* pack_path,
                             char* main_out, int main_cap) {
    if (!vm || !pack_path) return BPVM_ERR_IO;
    /* La fuente vive en la VM, no en la pila: el pack sigue en ejecución
     * después de esta función y su ruta tiene que seguir viva. */
    bpvm_pack_src_t* src = &vm->run_pack_src;
    bpvm_pack_fs_t*  st  = &vm->run_pack_st;
    vm->run_pack_on = 0;
    vm->run_pack_only[0] = '\0';           /* V7/H2 (#493): un pack del FS, sin filtro */
    if (bpvm_pack_open_fs(src, st, pack_path) != 0) {
        bpvm_diag("[bpvm-c] pack '%s' no se puede abrir", pack_path);
        return BPVM_ERR_IO;
    }
    /* Antes de hablar del manifest, comprobar que esto es un pack: si no,
     * el mensaje del manifest MIENTE (culpa al manifest de un fichero que ni
     * siquiera es un pack) y manda a buscar donde no es. */
    if (!bpvm_pack_src_is_pack(src)) {
        bpvm_diag("[bpvm-c] '%s' no es un pack (cabecera invalida)", pack_path);
        return BPVM_ERR_IO;
    }
    char mainmod[BPVM_PACK_NAME_LEN + 1];
    if (!bpvm_pack_manifest_get(src, "main", mainmod, (int) sizeof mainmod)) {
        /* Un pack SIN manifest es perfectamente válido — es una librería. Lo
         * que no es válido es pedirle que se ejecute, y hay que decirlo. */
        bpvm_diag("[bpvm-c] '%s' no es ejecutable: su manifest no declara 'main'",
                pack_path);
        return BPVM_ERR_IO;
    }
    bpvm_pack_entry_t e;
    if (!bpvm_pack_find_src(src, "mod", mainmod, &e)) {
        bpvm_diag("[bpvm-c] '%s' declara main='%s' pero no lleva ese modulo",
                pack_path, mainmod);
        return BPVM_ERR_IO;
    }
    int idx_before = vm->module_count;
    bpvm_status_t s = load_from_pack(vm, src, &e, mainmod);
    if (s != BPVM_OK) return s;
    /* A partir de aquí HAY pack en ejecución: sus imports se buscan DENTRO
     * antes que en ningún otro sitio. Se enciende DESPUÉS de cargar el main
     * para que un pack roto no deje la resolución tocada. */
    vm->run_pack_on = 1;
    /* Y los RECURSOS: el overlay hace que readFile y compañía miren dentro del
     * pack antes que en el FS, sin que ninguno de ellos se entere. */
    bpvm_fs_set_overlay(pack_res_stat, pack_res_read, vm);
    /* Y las dependencias, como en bpvm_load_mod: primero dentro del pack (lo
     * de arriba), y lo que no esté, en el directorio del propio pack. */
    char dir[256];
    path_dirname(pack_path, dir, sizeof dir);
    for (int j = idx_before; j < vm->module_count; j++) {
        bpvm_status_t r = discover_deps(vm, j, dir, j == idx_before);
        if (r != BPVM_OK) return r;
    }
    if (main_out && main_cap > 0) {
        size_t n = strlen(mainmod);
        if (n + 1 > (size_t) main_cap) return BPVM_ERR_IO;
        memcpy(main_out, mainmod, n + 1);
    }
    return BPVM_OK;
}

bpvm_status_t bpvm_load_mod_buffer(bpvm_t* vm, const uint8_t* data,
                                    size_t size, const char* name_hint) {
    if (!vm || !data || size == 0) return BPVM_ERR_IO;
    /* En target embebido NO descubrimos deps. El caller las carga manualmente. */
    return bpvm_loader_load_buffer(vm, data, size, name_hint);
}

/* ==========================================================================
 * #344 — EL RUN, ESCRITO UNA VEZ. Ver include/bpvm_entry.h.
 *
 * Había CINCO cargas de "lo que se va a ejecutar", y no eran cinco variantes:
 * eran la misma regla copiada (las 15 líneas de v1_resolve_path están palabra
 * por palabra en Pico, ESP32 y STM32). Peor: las cuatro de los REPL eran
 * versiones POBRES de la del núcleo — cortaban el import por el primer punto
 * (así que un módulo con nombre compuesto no resolvía) y no sabían nada del
 * pack en ejecución, que es justo lo que impedía llevar #310 a la placa.
 *
 * Así que esto no escribe un sexto resolvedor: hace PORTABLE el bueno. Lo
 * único que ataba `discover_deps` al host era un fopen; con la fachada del FS
 * (bpvm_fs_stat/read_at, que ya existía) desaparece y las 5 familias corren
 * exactamente el mismo código.
 * ========================================================================== */

/* [V7/H2, #493] ¿Algún pack grabado lleva este `.mod`? El último de la cadena que
 * lo tenga, la regla de siempre. Sólo nombres pelados de módulo: con directorio o
 * esquema no es cosa de la zona.
 * V7/H6 (#468) — expuesta: el instalador de la stdlib pregunta con ESTA MISMA
 * búsqueda si la zona ya sirve un módulo (y con qué bytes), en vez de llevar una
 * copia del orden de la cadena. */
int bpvm_entry_zona(const char* name, const uint8_t** data, uint32_t* len, char* pack_out) {
    uint32_t zsz = 0;
    const uint8_t* zona = bpvm_pack_mounted(&zsz);
    if (!zona || !zsz || !name) return -1;
    size_t n = strlen(name);
    if (n <= 4 || strcmp(name + n - 4, ".mod") != 0 || n - 4 > BPVM_PACK_NAME_LEN) return -1;
    for (size_t i = 0; i < n; i++) if (name[i] == '/' || name[i] == '\\' || name[i] == ':') return -1;
    char base[BPVM_PACK_NAME_LEN + 1];
    memcpy(base, name, n - 4); base[n - 4] = '\0';
    bpvm_pack_src_t src; bpvm_pack_entry_t e; char pk[BPVM_PACK_NAME_LEN + 1];
    bpvm_pack_src_mem(&src, zona, zsz);
    if (!bpvm_pack_find_src_pack(&src, "mod", base, &e, pk)) return -1;
    if (e.data_off > zsz || e.len > zsz - e.data_off) return -1;   /* una entrada que se sale de la zona no sirve */
    if (data) *data = zona + e.data_off;
    if (len) *len = e.len;
    if (pack_out) memcpy(pack_out, pk, sizeof pk);
    return 0;
}

/* → `pack:<P>/X.mod`, el nombre que contesta el respaldo de la fachada. */
static int zona_resolve(const char* name, char* out, size_t cap, uint32_t* size) {
    char pk[BPVM_PACK_NAME_LEN + 1];
    uint32_t len = 0;
    if (bpvm_entry_zona(name, NULL, &len, pk) != 0) return -1;
    int w = snprintf(out, cap, "%s%s/%s", BPVM_PACK_URI, pk, name);
    if (w <= 0 || (size_t) w >= cap) return -1;
    if (size) *size = len;
    return 0;
}

int bpvm_entry_resolve(const char* name, char* out, size_t out_cap,
                       uint32_t* size_out) {
    return bpvm_entry_resolve_en(bpvm_fs_basedir(), name, out, out_cap, size_out);
}

int bpvm_entry_resolve_en(const char* base, const char* name, char* out, size_t out_cap,
                          uint32_t* size_out) {
    if (!name || !out || out_cap == 0) return -1;
    uint32_t dummy; if (!size_out) size_out = &dummy;
    /* H19 — el base-dir del proyecto PRIMERO (la carpeta del módulo principal):
     * un import resuelve contra /app/<proj>/ antes que contra nada más. Plano
     * (basedir="") o ruta absoluta → se salta este candidato. V7 #654: la
     * carpeta, la DADA (bpvm_entry_resolve da la global). */
    const char* bd = base;
    if (bd && bd[0] && name[0] != '/') {
        snprintf(out, out_cap, "%s/%s", bd, name);
        if (bpvm_fs_stat(out, size_out) == 0) return 0;
    }
    snprintf(out, out_cap, "%s", name);
    if (bpvm_fs_stat(out, size_out) == 0) return 0;
    snprintf(out, out_cap, "/app/%s", name);
    if (bpvm_fs_stat(out, size_out) == 0) return 0;
    snprintf(out, out_cap, "/lib/%s", name);
    if (bpvm_fs_stat(out, size_out) == 0) return 0;
    /* [V7/H2, #493] `/sys` YA NO ESTÁ. Entró el 14-ago (#418) como último
     * candidato, con `FontLoadDemo` en el P4; el 15-sep Eduardo lo quita del
     * path: «/sys es más bien para ficheros de configuración». Los RECURSOS
     * (fuentes) siguen resolviéndose por ruta absoluta o por bpvm_fs_resolve.
     *
     * Y EL ÚLTIMO ES LA ZONA DE PACKS: un módulo que sólo vive en un pack
     * grabado resuelve a `pack:<Pack>/X.mod`. Aquí y no sólo en
     * bpvm_load_entry, porque los cuatro REPL (sim, Pico, ESP32, STM32)
     * SONDEAN con esta función antes de cargar y contestaban NOT_FOUND al RUN
     * de ese módulo (lo cazó la revisión adversarial: por el CLI del host iba,
     * por el wire no). Así RUN, STAT por nombre y autorun ven lo mismo. */
    if (zona_resolve(name, out, out_cap, size_out) == 0) return 0;
    out[0] = '\0';
    return -1;
}

/* Adaptador de la fachada al loader por trozos: el .mod se queda en el FS y
 * sólo se traen los pedazos que hacen falta. Igual en host y en placa. */
static long entry_fs_read_at(void* user, uint32_t off, uint8_t* dst, uint32_t n) {
    return bpvm_fs_read_at((const char*) user, off, dst, n);
}

/* #421 — el mismo cargador, pero DEJANDO DICHO por qué falló.
 *
 * El rastro se acababa justo aquí, que es donde falla. Un `IO error` significaba
 * a la vez «no existe», «mide cero» y «no se pudo leer», sin decir NUNCA la ruta
 * — que está en la mano. Costó una mañana de hipótesis (15-ago) con el firmware
 * sabiendo la respuesta y contándosela a nadie: al log iba (`18effeb`), pero por
 * el wire salía «IO error» a secas y el log de la P4 ni siquiera existía.
 *
 * Así que el motivo se escribe además en `det`, que el llamante manda al IDE.
 * `det` puede ser NULL: entonces esto se comporta como siempre. */
static void load_err(bpvm_t* vm, const char* fmt, ...) {
    if (!vm) return;
    va_list ap; va_start(ap, fmt);
    vsnprintf(vm->load_error, sizeof vm->load_error, fmt, ap);
    va_end(ap);
}

/* Pasa el detalle de la VM a la estructura que el REPL manda por el wire. Son
 * dos sitios porque son dos vidas distintas: el de la VM lo escribe quien falla
 * (a veces a varios imports de distancia, cargando una dependencia) y el de `bpvm_entry_t` es
 * lo que ve el IDE. */
static void recoge_fallo(bpvm_t* vm, bpvm_entry_t* e) {
    if (!vm || !e || e->fallo[0] || !vm->load_error[0]) return;
    snprintf(e->fallo, sizeof e->fallo, "%s", vm->load_error);
}

const char* bpvm_load_error(const bpvm_t* vm) {
    return vm ? vm->load_error : "";
}

static bpvm_status_t load_entry_file_det(bpvm_t* vm, const char* resolved_path) {
    uint32_t size = 0;
    if (bpvm_fs_stat(resolved_path, &size) != 0) {
        bpvm_diag("[bpvm-c] cargar '%s': NO se puede leer (stat falla)", resolved_path);
        load_err(vm, "no se puede leer '%s'", resolved_path);
        return BPVM_ERR_IO;
    }
    if (size == 0) {
        bpvm_diag("[bpvm-c] cargar '%s': mide 0 bytes (subida a medias?)", resolved_path);
        load_err(vm, "'%s' mide 0 bytes (subida a medias?)", resolved_path);
        return BPVM_ERR_IO;
    }
    /* #639 — lo que diga el cargador de ESTE modulo (la tabla de simbolos sin memoria, la subtabla CLASES rota) es el
     * motivo; antes se pisaba con el generico del estado («memoria del buffer insuficiente»). */
    vm->load_error[0] = '\0';
    bpvm_status_t s = bpvm_load_mod_stream(vm, entry_fs_read_at, (void*) resolved_path,
                                           (size_t) size, resolved_path);
    if (s != BPVM_OK) {
        bpvm_diag("[bpvm-c] cargar '%s' (%u B): %s", resolved_path,
                  (unsigned) size, bpvm_status_str(s));
        /* El caso que costó la mañana: el fichero está y se lee, pero su
         * contenido no cuadra con su cabecera. El gate de ABI (#284) valida la
         * VERSIÓN —un `.mod` v5 grita con su error propio—, no la INTEGRIDAD; un
         * v6 truncado pasa el control y muere aquí. Decirlo con el tamaño es lo
         * que separa «está rancio» de «se subió a medias». */
        if (s == BPVM_ERR_IO)
            load_err(vm, "'%s' (%u B) se lee pero no cuadra con su cabecera: "
                         "truncado o de otra version", resolved_path, (unsigned) size);
        else {
            char det[sizeof vm->load_error];
            snprintf(det, sizeof det, "%s", vm->load_error[0] ? vm->load_error : bpvm_status_str(s));
            load_err(vm, "'%s' (%u B): %s", resolved_path, (unsigned) size, det);
        }
    }
    return s;
}

bpvm_status_t bpvm_load_entry_file(bpvm_t* vm, const char* resolved_path) {
    return load_entry_file_det(vm, resolved_path);
}

/* #345 — la primera línea de /sys/auto.txt, limpia. Ver bpvm_entry.h. */
int bpvm_autorun_entry(char* out, size_t out_cap) {
    if (!out || out_cap == 0) return 0;
    out[0] = '\0';
    /* Sólo la CABEZA del fichero: lo que se busca es un renglón, no un
     * fichero. En un micro traerse el fichero entero para esto sería el mismo
     * error que ya costó el espejo de 128 KB en #305. */
    uint8_t head[160];
    long got = bpvm_fs_read_at("/sys/auto.txt", 0, head,
                               (uint32_t) sizeof head);
    if (got <= 0) return 0;                       /* no hay autorun: normal */

    size_t size = (size_t) got, i = 0, n = 0;
    while (i < size && (head[i] == ' ' || head[i] == '\t')) i++;
    while (i < size && head[i] != '\n' && head[i] != '\r' && n + 1 < out_cap)
        out[n++] = (char) head[i++];
    while (n > 0 && (out[n - 1] == ' ' || out[n - 1] == '\t')) n--;
    out[n] = '\0';
    return n > 0;
}

/* #345 paso 2 — la ventana de rescate. Ver bpvm_entry.h para el porqué. */
int bpvm_autorun_gate(const bpvm_autorun_wire_t* w, const char* path,
                      int saludo_ms, int ventana_ms) {
    /* Sin cintura no hay a quién preguntar → se arranca, que es lo que hacía
     * antes de todo esto. Nunca se queda una placa sin arrancar por un fallo
     * de este mecanismo: el Auto es lo que el usuario pidió. */
    if (!w || !w->escucha || !w->ahora_ms) return 1;

    if (w->anuncia) w->anuncia(path, ventana_ms, w->user);

    /* Fase 1: ¿hay alguien? El IDE manda HELLO al conectar — ya lo hacía. Corta
     * a propósito: si está conectado, su HELLO ya llegó; si no contesta es que
     * no está, y entonces NO se paga ninguna espera (una placa suelta, sin
     * nadie delante, arranca a su ritmo como siempre). */
    uint32_t t0 = w->ahora_ms(w->user);
    int hay_alguien = 0;
    while ((uint32_t)(w->ahora_ms(w->user) - t0) < (uint32_t) saludo_ms) {
        int r = w->escucha(w->user);
        if (r == 2) return 0;               /* KILL de entrada: ni se arranca */
        if (r == 1) { hay_alguien = 1; break; }
        if (w->espera_ms) w->espera_ms(5, w->user);
    }
    if (!hay_alguien) return 1;

    /* Fase 2: la espera de verdad. Es del usuario, no de la placa: le da tiempo
     * a ver el aviso y darle a Stop, que manda el KILL de siempre (#257). */
    t0 = w->ahora_ms(w->user);
    while ((uint32_t)(w->ahora_ms(w->user) - t0) < (uint32_t) ventana_ms) {
        if (w->escucha(w->user) == 2) return 0;
        if (w->espera_ms) w->espera_ms(10, w->user);
    }
    return 1;
}

/* ¿Queda algún import sin dueño cargado? Devuelve 1 y deja el nombre en `out`.
 * Mejor decir "falta 'Gui'" que dejar que el link reviente doscientas líneas
 * más tarde con un símbolo que no le dice nada a nadie. */
static int first_missing(const bpvm_t* vm, char* out, size_t cap) {
    for (int mi = 0; mi < vm->module_count; mi++) {
        const bpvm_module_t* m = &vm->modules[mi];
        for (int k = 0; k < m->import_count; k++) {
            const char* imp = m->imports[k];
            if (!imp || !imp[0]) continue;
            char lib[64], mod[64];
            derive_owner(imp, lib, sizeof lib, mod, sizeof mod);
            if (!mod[0] || module_loaded(vm, lib, mod)) continue;
            /* Precisión explícita: el nombre es para un mensaje de error, así
             * que truncarlo es aceptable — pero hay que DECIRLO, o gcc avisa en
             * cada build y el aviso de verdad se pierde entre el ruido. */
            if (lib[0]) snprintf(out, cap, "%.*s.%.*s",
                                 (int)(cap / 2 - 1), lib, (int)(cap / 2 - 1), mod);
            else        snprintf(out, cap, "%.*s", (int)(cap - 1), mod);
            return 1;
        }
    }
    out[0] = '\0';
    return 0;
}

/* [V7/H2, #493] El principal que SÓLO está en la zona de packs: `X.mod` (sin
 * directorio) que ningún candidato del FS tiene. Se carga XIP desde el pack
 * que lo lleve (el último grabado, la regla de siempre) y ese pack pasa a ser
 * el «pack en ejecución» —la zona entera como fuente, filtrada por su nombre—
 * para que sus módulos y recursos vayan primero. Devuelve BPVM_ERR_IO con
 * e->fallo vacío si no está en la zona (el llamante pone el mensaje). */
static void apagar_pack_en_ejecucion(bpvm_t* vm);

static bpvm_status_t cargar_principal_de_la_zona(bpvm_t* vm, bpvm_entry_t* e) {
    /* e->resolved = "pack:<P>/X.mod", lo que devolvió bpvm_entry_resolve (o lo
     * que el usuario dio tal cual: `RUN pack:P/X.mod` tiene que dar lo mismo
     * que `RUN X.mod` — la revisión lo cazó cargándolo por stream y con un
     * aviso falso). */
    uint32_t zsz = 0;
    const uint8_t* zona = bpvm_pack_mounted(&zsz);
    if (!zona || !zsz) { snprintf(e->fallo, sizeof e->fallo, "'%.100s': no hay zona de packs montada", e->resolved); return BPVM_ERR_IO; }
    char pk[BPVM_PACK_NAME_LEN + 1], tipo[BPVM_PACK_TYPE_LEN + 1], base[BPVM_PACK_NAME_LEN + 1];
    if (!bpvm_pack_key_de_path(e->resolved, tipo, base, pk) || !pk[0] || strcmp(tipo, "mod") != 0) {
        snprintf(e->fallo, sizeof e->fallo, "'%.100s' no es un modulo de un pack (pack:<Pack>/X.mod)", e->resolved);
        return BPVM_ERR_IO;
    }
    bpvm_pack_src_t zsrc; bpvm_pack_entry_t pe;
    bpvm_pack_src_mem(&zsrc, zona, zsz);
    if (!bpvm_pack_find_in_src(&zsrc, pk, "mod", base, &pe)) {
        snprintf(e->fallo, sizeof e->fallo, "el pack '%s' de la zona no lleva '%s.mod'", pk, base);
        return BPVM_ERR_IO;
    }
    int idx_before = vm->module_count;
    bpvm_status_t s = load_from_pack(vm, &zsrc, &pe, base);
    if (s != BPVM_OK) {
        snprintf(e->fallo, sizeof e->fallo, "'%s.mod' esta en el pack '%s' de la zona pero no carga", base, pk);
        return s;
    }
    bpvm_diag("[bpvm-c] '%s.mod' cargado XIP desde el pack '%s' de la zona", base, pk);
    /* Pack en ejecución = la zona filtrada por ESTE pack. Como en bpvm_load_pack,
     * se enciende después de cargar el main para que un pack roto no deje la
     * resolución tocada. `run_pack_st.path` vacío: no hay fichero que proteger. */
    vm->run_pack_src = zsrc;
    vm->run_pack_st.path[0] = '\0';
    snprintf(vm->run_pack_only, sizeof vm->run_pack_only, "%s", pk);
    vm->run_pack_on = 1;
    bpvm_fs_set_overlay(pack_res_stat, pack_res_read, vm);
    e->from_pack = 1;
    snprintf(e->main_module, sizeof e->main_module, "%.*s", (int)(sizeof e->main_module - 1), base);
    for (int j = idx_before; j < vm->module_count; j++) {
        s = discover_deps(vm, j, "/app", j == idx_before);
        if (s != BPVM_OK) { recoge_fallo(vm, e); apagar_pack_en_ejecucion(vm); return s; }
    }
    return BPVM_OK;
}

/* [V7/H2, #493] Un pack en ejecución que no llega a arrancar NO deja la
 * resolución tocada: ni el overlay de recursos (que apuntaba a un `vm` que a
 * lo mejor se libera: la revisión midió un STAT contestando con la Str de un
 * pack muerto), ni el filtro de la zona. Lo mismo que hacen bpvm_run y
 * bpvm_run_smp al terminar, aquí para los caminos de error y para destroy. */
static void apagar_pack_en_ejecucion(bpvm_t* vm) {
    vm->run_pack_on = 0;
    vm->run_pack_only[0] = '\0';
    bpvm_fs_set_overlay(NULL, NULL, NULL);
}

/* El cierre de bpvm_load_entry, para los dos caminos (FS/pack del FS, y la
 * zona): avisar de cada módulo cargado y, el guardián, nombrar lo que se quedó
 * sin dueño. */
static bpvm_status_t cerrar_entry(bpvm_t* vm, bpvm_entry_t* e, int idx_before) {
    if (e->on_module) {
        for (int j = idx_before; j < vm->module_count; j++)
            e->on_module(vm->modules[j].name, e->from_pack ? "pack" : "fs", 0, e->user);
    }
    /* 3. El guardián: si algo se quedó sin dueño, se NOMBRA. */
    if (first_missing(vm, e->missing, sizeof e->missing)) return BPVM_ERR_IO;
    return BPVM_OK;
}

bpvm_status_t bpvm_load_entry(bpvm_t* vm, const char* path, bpvm_entry_t* e) {
    bpvm_entry_t local;
    if (!e) { memset(&local, 0, sizeof local); e = &local; }
    e->missing[0] = e->resolved[0] = e->main_module[0] = '\0';
    e->fallo[0] = '\0';                       /* #421 */
    e->from_pack = 0;
    if (!vm || !path || !path[0]) {
        snprintf(e->fallo, sizeof e->fallo, "no se dijo que ejecutar (ruta vacia)");
        return BPVM_ERR_IO;
    }

    /* 1. La regla de búsqueda, una sola (…, /lib y, la última, la zona de packs). */
    uint32_t size = 0;
    if (bpvm_entry_resolve(path, e->resolved, sizeof e->resolved, &size) != 0) {
        /* #421 — «no aparece» y «aparece pero no se puede leer» son dos ratos de
         * búsqueda distintos, y hasta ahora los dos decían `IO error`. Decir
         * DÓNDE se ha buscado ahorra la mitad: si el fichero está donde el
         * usuario cree, entonces el problema es el nombre. */
        snprintf(e->fallo, sizeof e->fallo,
                 "no encuentro '%s' (buscado en el proyecto, tal cual, en /app, /lib y en la zona de packs)",
                 path);
        return BPVM_ERR_IO;
    }
    if (vm->module_count == 0 && vm->raiz_dir == NULL) {   /* V8 H9 (#786): lo que un programa carga, primero a su lado */
        char dir[256];
        path_dirname(e->resolved, dir, sizeof dir);
        vm->raiz_dir = bpvm_strdup(dir);
    }
    /* [V7/H2, #493] UN MÓDULO DE LA ZONA (`pack:<P>/X.mod`, lo devuelva el
     * resolvedor o lo dé el usuario tal cual): se ejecuta XIP desde ahí, y sus
     * dependencias se buscan PRIMERO en su propio pack (el orden de Eduardo:
     * pack del principal → /app → /lib → packs). Nunca por el camino del FS:
     * por ahí se copiaría a RAM y el aviso de la sombra saldría falso. */
    if (strncmp(e->resolved, BPVM_PACK_URI, sizeof(BPVM_PACK_URI) - 1) == 0) {
        int idx0 = vm->module_count;
        bpvm_status_t zs = cargar_principal_de_la_zona(vm, e);
        if (zs != BPVM_OK) return zs;
        return cerrar_entry(vm, e, idx0);
    }
    /* 2. EL `if` DE .mod/.pack, y vive AQUÍ. Antes sólo el CLI del host sabía
     *    despachar packs; por eso llevarlos a la placa era copiar la regla en
     *    cuatro sitios más. */
    size_t n = strlen(e->resolved);
    e->from_pack = (n > 5 && strcmp(e->resolved + n - 5, ".pack") == 0);

    bpvm_status_t s;
    int idx_before = vm->module_count;
    if (e->from_pack) {
        s = bpvm_load_pack(vm, e->resolved, e->main_module, (int) sizeof e->main_module);
        if (s != BPVM_OK) { recoge_fallo(vm, e); apagar_pack_en_ejecucion(vm); return s; }   /* #421 */
        /* bpvm_load_pack ya descubre las deps del main (dentro del pack primero,
         * y luego junto al propio pack); aquí sólo queda el barrido final. */
    } else {
        s = load_entry_file_det(vm, e->resolved);
        if (s != BPVM_OK) { recoge_fallo(vm, e); return s; }
        if (vm->module_count > idx_before)
            /* Precisión explícita otra vez: main_module tiene el ancho de un
             * nombre de pack (33) y un nombre de módulo puede ser mayor. Lo
             * cazó el gcc de ARM, no el del host — otra razón para compilar la
             * placa en cada tanda, no sólo el host. */
            snprintf(e->main_module, sizeof e->main_module, "%.*s",
                     (int)(sizeof e->main_module - 1), vm->modules[idx_before].name);
        /* El directorio del PROPIO módulo es el primer sitio donde buscar sus
         * dependencias — un proyecto se lleva sus .mod al lado. (Se me olvidó
         * en el primer intento y la batería lo cazó a la primera: seis targets
         * con "falta la lib 'X'".) */
        char dir[256];
        path_dirname(e->resolved, dir, sizeof dir);
        for (int j = idx_before; j < vm->module_count; j++) {
            s = discover_deps(vm, j, dir, j == idx_before);
            if (s != BPVM_OK) { recoge_fallo(vm, e); return s; }   /* #421 */
        }
    }

    return cerrar_entry(vm, e, idx_before);
}

uint8_t* bpvm_arena_reserve(bpvm_t* vm, uint32_t n, uint32_t align) {
    if (!vm || n == 0) return NULL;
    uint32_t base = vm->next_free_address;
    if (align > 1) base = (base + (align - 1)) & ~(align - 1);
    if (base + n > vm->heap_top) return NULL;      /* no cabe: que lo sepa el caller */
    vm->next_free_address = base + n;
    /* El heap empieza tras lo último reservado — MISMO invariante (y mismos
     * campos) que fija el loader al terminar de cargar un módulo, incluido el
     * umbral del GC, que se recalcula porque la arena disponible ha encogido. */
    vm->heap_start        = vm->next_free_address;
    vm->heap_next         = vm->heap_start;
    vm->heap_bloques      = 0;   /* V7/H7 #593: el heap, vacío */
    bpvm_huecos_vaciar(vm);   /* V8 #718: las listas de huecos, vacías */
    vm->last_gc_heap_next = vm->heap_next;
    vm->alloc_since_gc    = 0u;   /* #357 */
    vm->gc_bump_threshold = (vm->heap_top - vm->heap_start) / 8;
    if (vm->gc_bump_threshold < 4096) vm->gc_bump_threshold = 4096;
    /* #355 — ARMA LA RESERVA DE EMERGENCIA (idea de Eduardo). 1 KB del final del
     * heap que el programa no puede tocar; se suelta cuando una reserva ya ha
     * fallado de verdad, para que quede sitio con el que CONSTRUIR el error
     * (cadena del mensaje + objeto RuntimeError). Sin esto, quedarse sin memoria
     * significaba quedarse tambien sin poder avisar, y el programa seguia con
     * refs nulas — que es como #355 se volvio invisible durante una manana.
     *
     * Se arma en cada carga de modulo (o sea, en cada RUN) y se gasta UNA vez.
     * No se rearma sola a mitad: si lo hiciera, un programa que ignore el OOM
     * la gastaria en bucle y volveriamos al fallo mudo.
     *
     * 1 KB da de sobra: el mensaje mas largo no llega a 100 bytes y el objeto
     * RuntimeError son unas pocas decenas. Y si el heap es tan pequeno que 1 KB
     * es una porcion notable, no se arma: mas vale ese K para el programa. */
    vm->heap_reserve = 1024u;
    if ((vm->heap_top - vm->heap_start) < 16u * 1024u) vm->heap_reserve = 0u;
    return vm->memory + base;
}

/* Lo que un módulo cargado tiene fuera del bloque de la VM. */
static void modulo_suelta(bpvm_module_t* m) {
    bpvm_free(m->imports);     /* V7 #651: sus nombres viven en m->nombres */
    bpvm_free(m->dep_names);   /* V7/H2 (MOD8) */
    bpvm_free(m->dep_gen);
    bpvm_free(m->dep_crc);
    bpvm_free(m->dep_flags);
    bpvm_free(m->dep_avisado);
    bpvm_free(m->class_fixups);
    bpvm_free(m->eh_class_fixups);
    bpvm_free(m->refs);        /* V7/H2 (#523) — V10 (R4/J-2): la subtabla refs se quedaba viva → «FUGA» en todo RUN MOD9 */
    bpvm_free(m->nombres);     /* V7 #651: los nombres de imports, dep_names y refs, en un bloque */
}

/* ═══════════════════════════════════════════════════════════════════════════
 * V8 H9 (#786) — LOS OVERLAYS: un programa que carga a otro, lo ejecuta y lo descarga (Overlays.bp, V8_IDEAS §E13). Espejo
 * de cargaOverlay y descargaOverlay (ModuleManager, miVM). Lo del heap, en heap.c; lo del enlace, en link.c.
 *
 * Una carga a la vez, y lo que entró se va entero: descargar es volver a como estaba. Lo ya cargado no se vuelve a
 * cargar (discover_deps lo ve en vm->modules, y sólo pasa la norma).
 * ═══════════════════════════════════════════════════════════════════════════ */

int bpvm_sabotaje_overlay = 0;   /* SÓLO PRUEBAS: bpvm_internal.h */

/* Vuelve todo a como estaba, salvo la memoria del heap, que devuelve quien llama. */
static void overlay_deshaz(bpvm_t* vm, const bpvm_overlay_t* o) {
    for (int i = o->modulos; i < vm->module_count; i++) modulo_suelta(&vm->modules[i]);
    vm->module_count = o->modulos;
    if (bpvm_sabotaje_overlay != 6) {
        vm->symbol_count = o->simbolos;
        while (vm->sym_ntrozos > o->sym_ntrozos) bpvm_free(vm->sym_trozos[--vm->sym_ntrozos]);
        vm->sym_pool_len    = o->sym_pool_len;
        vm->sym_trozo_usado = o->sym_trozo_usado;
    }
    vm->next_free_address = o->libre;
    vm->xip_lo = o->xip_lo;
    vm->xip_hi = o->xip_hi;
    if (bpvm_sabotaje_overlay != 3) {
        vm->ifaz_tabla = o->ifaz_tabla;
        vm->ifaz_n     = o->ifaz_n;
    }
    vm->run_pack_on  = o->run_pack_on;
    vm->run_pack_src = o->run_pack_src;
    vm->run_pack_st  = o->run_pack_st;
    memcpy(vm->run_pack_only, o->run_pack_only, sizeof vm->run_pack_only);
    if (vm->run_pack_on) bpvm_fs_set_overlay(pack_res_stat, pack_res_read, vm);
    else                 bpvm_fs_set_overlay(NULL, NULL, NULL);
    vm->link_error[0] = '\0';
    vm->load_error[0] = '\0';
    vm->gui_disp_click = vm->gui_disp_change = 0;   /* #486: se resuelven una vez, y lo que hay ha cambiado */
}

int bpvm_overlay_cargar(bpvm_t* vm, const bpvm_thread_t* tc, const char* ruta, char* fallo, size_t cap) {
    if (vm->overlay != NULL) { snprintf(fallo, cap, "%s", BPVM_MSG_OVERLAY_YA); return 1; }
    bpvm_overlay_t* o = (bpvm_overlay_t*) bpvm_calloc(1, sizeof *o);
    if (o == NULL) { snprintf(fallo, cap, "Overlay.load: %s", BPVM_MSG_OVERLAY_NO_CABE); return 1; }
    o->modulos = vm->module_count;
    o->simbolos = vm->symbol_count;
    o->sym_ntrozos = vm->sym_ntrozos;
    o->sym_pool_len = vm->sym_pool_len;
    o->sym_trozo_usado = vm->sym_trozo_usado;
    o->libre = vm->next_free_address;
    o->heap_antes = vm->heap_start;
    o->xip_lo = vm->xip_lo;
    o->xip_hi = vm->xip_hi;
    o->ifaz_tabla = vm->ifaz_tabla;
    o->ifaz_n = vm->ifaz_n;
    o->run_pack_on = vm->run_pack_on;
    o->run_pack_src = vm->run_pack_src;
    o->run_pack_st = vm->run_pack_st;
    memcpy(o->run_pack_only, vm->run_pack_only, sizeof o->run_pack_only);
    if (bpvm_overlay_apartar(vm, tc, o, fallo, cap) != 0) { bpvm_free(o); return 1; }
    /* Con el heap apartado y el mundo parado, como al arrancar. */
    o->cargando = 1;
    vm->overlay = o;
    char real[BPVM_FS_PATH_MAX];
    uint32_t tam = 0;
    const char* dada = ruta;   /* la carpeta del programa principal, lo primero: la misma búsqueda que miVM */
    if (vm->raiz_dir != NULL && ruta[0] != '/' && bpvm_entry_resolve_en(vm->raiz_dir, ruta, real, sizeof real, &tam) == 0)
        dada = real;
    bpvm_entry_t e;
    memset(&e, 0, sizeof e);
    bpvm_status_t s = bpvm_load_entry(vm, dada, &e);
    char motivo[sizeof e.fallo + 64];
    motivo[0] = '\0';
    if (s == BPVM_OK) {
        const bpvm_module_t* m = &vm->modules[o->modulos];
        for (int i = 0; i < o->modulos; i++)
            if (strcmp(vm->modules[i].name, m->name) == 0 && strcmp(vm->modules[i].library, m->library) == 0) {
                snprintf(motivo, sizeof motivo, "ya hay un modulo '%s' cargado", m->name);
                s = BPVM_ERR_RUNTIME;
            }
        if (s == BPVM_OK && m->main_offset < 0) {
            snprintf(motivo, sizeof motivo, "'%s' no tiene punto de entrada", m->name);
            s = BPVM_ERR_RUNTIME;
        }
    }
    if (s == BPVM_OK) s = bpvm_link_desde(vm, o->modulos);
    if (s != BPVM_OK) {
        if (motivo[0])            { /* ya está */ }
        else if (s == BPVM_ERR_OOM) snprintf(motivo, sizeof motivo, "%s", BPVM_MSG_OVERLAY_NO_CABE);
        else if (e.missing[0])    snprintf(motivo, sizeof motivo, "falta el modulo '%s'", e.missing);
        else if (e.fallo[0])      snprintf(motivo, sizeof motivo, "%s", e.fallo);
        else if (vm->link_error[0]) snprintf(motivo, sizeof motivo, "%.300s", vm->link_error);
        else                      snprintf(motivo, sizeof motivo, "%s", bpvm_status_str(s));
        overlay_deshaz(vm, o);
        vm->overlay = NULL;
        bpvm_overlay_devolver(vm, o, o->heap_antes);
        bpvm_free(o);
        snprintf(fallo, cap, "Overlay.load: %s", motivo);
        return 1;
    }
    o->cargando = 0;
    /* Los bloques del heap van de 4 en 4 desde heap_start, y al descargar el hueco se pone delante: de `heap_antes` a
     * donde empiecen los objetos, un múltiplo de 4. */
    uint32_t nuevo = vm->next_free_address + ((o->heap_antes - vm->next_free_address) & 3u);
    vm->next_free_address = nuevo;
    const bpvm_module_t* m = &vm->modules[o->modulos];
    o->entrada = m->cb + (uint32_t) m->main_offset;
    vm->overlay_cs = m->code_start;
    vm->gui_disp_click = vm->gui_disp_change = 0;
    bpvm_overlay_devolver(vm, o, nuevo);
    return 0;
}

/* Overlay.unload, lo primero: sus globales dejan de ser raíces. Lo que sólo ellas alcanzan se va en las colectas que
 * vienen, con sus destructores, y el código del programa sigue en memoria para correrlos. */
int bpvm_overlay_cerrar(bpvm_t* vm, const bpvm_thread_t* tc, char* fallo, size_t cap) {
    bpvm_smp_lock(vm);
    int otros = bpvm_overlay_otros_hilos(vm, tc);
    if (!otros && vm->overlay != NULL && bpvm_sabotaje_overlay != 5) vm->overlay->cerrado = 1;
    bpvm_smp_unlock(vm);
    if (otros) snprintf(fallo, cap, "%s", BPVM_MSG_OVERLAY_SUELTA_HILOS);
    return otros;
}

/* Overlay.unload, lo último: que no quede nada que apunte a lo que se va, y soltarlo. */
int bpvm_overlay_soltar(bpvm_t* vm, const bpvm_thread_t* tc, char* fallo, size_t cap) {
    bpvm_overlay_t* o = vm->overlay;
    bpvm_smp_lock(vm);
    int r = 0;
    if (bpvm_overlay_otros_hilos(vm, tc)) { snprintf(fallo, cap, "%s", BPVM_MSG_OVERLAY_SUELTA_HILOS); r = 1; }
    else r = bpvm_overlay_lo_que_queda(vm, o, fallo, cap);
    if (r == 0) {
        overlay_deshaz(vm, o);
        bpvm_overlay_heap_suelta(vm, o);
        vm->overlay = NULL;
        vm->overlay_cs = 0xFFFFFFFFu;
        /* Lo que guarda direcciones: otra carga en la misma dirección heredaría datos de ésta. */
        vm->str_epoca++;
        for (bpvm_thread_t* h = bpvm_hilos_primero(vm); h; h = bpvm_hilos_siguiente(vm, h)) h->cb_cache_cs = 0u;
        bpvm_destruir_olvida_clases(vm);
        bpvm_free(o);
    }
    bpvm_smp_unlock(vm);
    return r;
}

bpvm_status_t bpvm_load_mod_stream(bpvm_t* vm, bpvm_read_at_fn rd, void* user,
                                    size_t size, const char* name_hint) {
    if (!vm || !rd || size == 0) return BPVM_ERR_IO;
    /* Igual que load_mod_buffer: en target embebido NO descubrimos deps, las
     * carga el caller. */
    return bpvm_loader_load_stream(vm, rd, user, size, name_hint);
}

bpvm_status_t bpvm_load_mod(bpvm_t* vm, const char* path) {
    if (!vm || !path) return BPVM_ERR_IO;
    int idx_before = vm->module_count;
    bpvm_status_t s = bpvm_loader_load(vm, path);
    if (s != BPVM_OK) return s;
    /* Descubrir y cargar las dependencias del módulo, y las de las suyas,
     * buscándolas en el mismo directorio que el .mod cargado. */
    char dir[256];
    path_dirname(path, dir, sizeof(dir));
    for (int j = idx_before; j < vm->module_count; j++) {
        bpvm_status_t r = discover_deps(vm, j, dir, j == idx_before);
        if (r != BPVM_OK) return r;
    }
    return BPVM_OK;
}

/* Gancho post-enlace (ver bpvm.h). Fuera del bpvm_t a propósito: es andamio de
 * diagnóstico del puerto, no estado del programa. */
static bpvm_after_link_fn g_after_link = NULL;

void bpvm_set_after_link_hook(bpvm_after_link_fn fn) { g_after_link = fn; }

/* V7/H7 #600 — al acabar el RUN, lo que algún hilo dejara a mitad de un `print` (el programa parado con un KILL,
 * o muerto por un error de la VM, en medio de una línea): sale tal cual, sin salto, como salía antes. El hilo que
 * acaba solo ya lo ha vaciado al terminar (interp.c, `done`). */
static void lineas_vaciar(bpvm_t* vm) {
    for (bpvm_thread_t* h = bpvm_hilos_primero(vm); h; h = bpvm_hilos_siguiente(vm, h)) bpvm_linea_vaciar(vm, h);
}

/* V7/H7 #606 (29-sep) — LA PUESTA A PUNTO de un RUN, UNA para los dos planificadores. bpvm_run y bpvm_run_smp la
 * llevaban copiada y la copia se quedó atrás: el cs del `main` se buscaba en bpvm_run_smp por el rango viejo
 * [code_start, end_addr), que en un módulo XIP (de un pack) está vacío —end_addr == code_start—, y un `main` de la
 * zona de packs con --smp acababa en «PC fuera de rango»; bpvm_run se había arreglado en su día (H3.c) y su gemelo
 * no. Enlazar, el hilo `main` desde el punto de entrada, el cuanto y las dos excepciones prefabricadas. V7/H12, etapa
 * 2a.2: la llama la cola al COLGAR el programa, con su main ya en la cola y antes de publicarlo (scheduler.c). */
bpvm_status_t bpvm_programa_preparar(bpvm_t* vm) {
    bpvm_kill_marcar(vm, 0);   /* P-run-stop: los re-runs no nacen muertos */
    /* F3 — resolver imports y aplicar class fixups antes de ejecutar. */
    bpvm_status_t ls = bpvm_link_all(vm);
    if (ls != BPVM_OK) return ls;
    if (g_after_link) g_after_link(vm);   /* el código ya no debe cambiar */

    /* Inicializar thread main desde el entry-point antes de entrar al
     * scheduler. */
    if (vm->main_absolute_address == 0) return BPVM_ERR_BAD_PC;
    bpvm_thread_t* main_tc = vm->hilo_main;   /* V7/H12, etapa 2a.2: ya en la cola (bpvm_cola_colgar) */
    vm->pila_peor_usada = vm->pila_peor_total = 0;   /* #605: el peor de ESTE RUN */
    vm->hilos_max = bpvm_la_cola.en_cola;            /* H12: el máximo de ESTE RUN (el main) */
    main_tc->pc = vm->main_absolute_address;
    /* cs del módulo del entry-point (H3.c: por rango de CÓDIGO [cb, cb+size)). */
    {
        const bpvm_module_t* m = bpvm_module_for_code_addr(vm, vm->main_absolute_address);
        if (m) main_tc->cs = m->code_start;
    }
    if (main_tc->cs == 0) return BPVM_ERR_BAD_PC;
    main_tc->sp = main_tc->stack_base;
    main_tc->bp = main_tc->stack_base;
    main_tc->pila_max = main_tc->stack_base;   /* V7/H7 #605: la marca, de cero en cada RUN */
    vm->hilo_sin_atrapar = 0;                  /* V7/H7 #608: ningún hilo ha muerto sin atrapar */
    main_tc->status = BPVM_THREAD_RUNNABLE;

    /* Default quantum si no se ajustó; el ENV (`quantum=N`) manda si trae valor (#462, también con SMP). */
    if (vm->quantum_ops == 0)
        vm->quantum_ops = (g_quantum_ops > 0) ? (int) g_quantum_ops : 1024;

    /* #430 — LA EXCEPCION PREFABRICADA (idea de Eduardo): el OOM se fabrica
     * AQUI, antes de arrancar el programa, cuando fabricarlo es gratis — y
     * cuando de verdad no quede memoria (de heap o de tabla), lanzar esta no
     * aloja NADA. Es raiz propia del GC (gc_mark_phase 2a-bis) y vive toda la
     * ejecucion. Se pone a 0 antes de refabricar: en modo daemon el RUN
     * anterior dejo una que apunta a un heap ya reseteado. */
    vm->oom_exc.v = 0u;
    {
        vm->prefabricando_oom = 1;   /* V6/P1.C3.3: para que el aviso no mienta */
        bpref_t pre = bpvm_throw_runtime_error(vm, main_tc, "No space in heap");
        vm->prefabricando_oom = 0;
        vm->oom_exc = pre;                 /* .v == 0 si ni esto se pudo */
        vm->runtime_error[0] = 0;          /* el prologo no es un error */
        vm->runtime_traza[0] = 0;
        main_tc->alloc_anchor = 0;         /* el ancla era transitoria */
    }
    /* V7 #494 — y la del HEAP DESCARRILADO, igual y por lo mismo: cuando el recorrido del heap se
     * rompe, reservar ya no es seguro (ver bpvm_internal.h). Después de la de #430, en el mismo orden
     * que miVM: las dos VMs dejan el heap igual. */
    vm->heap_exc.v = 0u;
    vm->gc_descarrilo = 0;
    bpvm_aot_callctx()->reserva_descarrilada = 0;   /* #659: la de esta tarea (la de las demás, sin el heap descarrilado, no vale) */
    vm->gc_colectas = 0;           /* V7 (28-sep): Heap.getCollections cuenta las de ESTE RUN */
    {
        vm->prefabricando_oom = 1;
        bpref_t pre = bpvm_throw_runtime_error(vm, main_tc, BPVM_MSG_HEAP_DESCARRILADO);
        vm->prefabricando_oom = 0;
        vm->heap_exc = pre;
        vm->runtime_error[0] = 0;
        vm->runtime_traza[0] = 0;
        main_tc->alloc_anchor = 0;
    }
    return BPVM_OK;
}

/* #310 (Eduardo) — al TERMINAR se vuelve al camino estándar: el del pack
 * desaparece. Si no, en un daemon (que es como lo usa el IDE) el Run
 * siguiente heredaría el pack del anterior y resolvería imports contra un
 * pack que ya no se está ejecutando: la peor clase de fallo, porque
 * funciona hasta que alguien encadena dos ejecuciones. */
static void cerrar_run(bpvm_t* vm) {
    vm->run_pack_on = 0;
    vm->run_pack_only[0] = '\0';
    bpvm_fs_set_overlay(NULL, NULL, NULL);   /* y los recursos, al camino normal */
    /* V7/H7 #605 — CUÁNTA PILA USÓ (la marca de ENTER: con los locales, sin los operandos): el main, y el hilo que más
     * cerca estuvo de su tope. El instrumento para saber si bastan los 16 KB del main y lo que pide cada hilo; habla
     * siempre, como el guardián de la memoria. */
    const bpvm_thread_t* m = vm->hilo_main;
    unsigned mu = (m->pila_max > m->stack_base) ? (unsigned) (m->pila_max - m->stack_base) : 0u;
    /* V7/H12, etapa 2 — los que ya salieron de la cola se apuntaron al salir; los que quedan, ahora. */
    for (bpvm_thread_t* t = bpvm_hilos_primero(vm); t; t = bpvm_hilos_siguiente(vm, t)) bpvm_hilo_apuntar_pila(vm, t);
    unsigned pu = vm->pila_peor_usada, pt = vm->pila_peor_total;
    if (pt > 0)
        bpvm_diag("[bpvm] pila: el main llegó a %u de %u B; el hilo más lleno, a %u de %u B", mu,
                  (unsigned) (m->stack_top - m->stack_base), pu, pt);
    else
        bpvm_diag("[bpvm] pila: el main llegó a %u de %u B", mu, (unsigned) (m->stack_top - m->stack_base));
    /* V7/H12, etapa 2 — que la memoria de los hilos DECRECE: un hilo que acaba sale de la cola y devuelve su struct,
     * así que «a la vez» es lo que hubo vivo, no lo que se arrancó (`make test-hilos`). */
    bpvm_diag("[bpvm] hilos: %ld arrancados, como mucho %ld a la vez (el main incluido)",
              (long) (vm->sig_tid - 1), (long) vm->hilos_max);
}

/* V7/H12, etapa 2a.2 — EL CIERRE del programa, cuando acaba: lo que algún hilo dejó a mitad de un `print` (#600) y
 * las líneas `pila:` e `hilos:` (#605, H12). Lo hace el último run que sale (scheduler.c), antes de descolgarlo. */
void bpvm_programa_cerrar(bpvm_t* vm) {
    lineas_vaciar(vm);   /* V7/H7 #600 */
    cerrar_run(vm);
}

/* V7/H12, etapa 2a.2 — UN PROGRAMA HASTA EL FINAL, en el hilo que llama: el PC, la STM32 y las pruebas de C. Se cuelga
 * en la cola (bpvm_smp.h: el main a la cola, prepararlo, publicarlo) y el que llama hace de run hasta que acaba —el
 * mismo planificador que los runs permanentes de una placa—. Lo que devuelve, el del último run al descolgarlo: KILLED
 * si se pidió parar, aunque acabase solo (#462, #640: el kill de una native que corre SIN EJECUTOR la para LANZANDO, y
 * sin atrapar salía RUNTIME_ERROR por un programa que se mandó parar; `test-aotkill`, la pasada sin ejecutor); si no,
 * el primer fallo, o RUNTIME_ERROR si un hilo murió sin atrapar (#608). */
bpvm_status_t bpvm_run(bpvm_t* vm) {
    return bpvm_run_smp(vm, 1);
}

/* H2 — con `n_workers` runs (el que llama y n-1 hilos que acaban con el programa; scheduler_smp.c). Con uno, como
 * bpvm_run. */
#include "bpvm_smp.h"
bpvm_status_t bpvm_run_smp(bpvm_t* vm, int n_workers) {
    if (!vm) return BPVM_ERR_BAD_PC;
    if (n_workers < 1) n_workers = 1;
    int ps = bpvm_cola_colgar(vm, n_workers, 0, NULL, NULL);
    if (ps != BPVM_OK) return (bpvm_status_t) ps;   /* no se publicó: un fallo de enlace, la cola ocupada… */
    bpvm_cola_correr_n(n_workers);
    return (bpvm_status_t) bpvm_la_cola.rs;
}

void bpvm_set_output(bpvm_t* vm, bpvm_output_cb cb, void* user) {
    if (!vm) return;
    vm->output_cb = cb;
    vm->output_user = user;
}

void bpvm_set_tracing(bpvm_t* vm, int enabled) {
    if (!vm) return;
    vm->tracing = enabled ? true : false;
}

void bpvm_set_debug_hook(bpvm_t* vm,
                          bpvm_debug_hook_t hook,
                          bpvm_pc_to_line_t pc_to_line,
                          void* user) {
    if (!vm) return;
    /* Setear todos juntos. El inner loop lee debug_hook primero — si
     * es NULL no toca los otros dos campos. */
    vm->debug_hook        = hook;
    vm->debug_pc_to_line  = pc_to_line;
    vm->debug_user        = user;
}

int bpvm_thread_id(const bpvm_thread_t* tc) {
    return tc ? (int) tc->id : -1;
}

/* ============================================================ */
/*  H6.b — Debugger del device: API de breakpoints + pausa.     */
/* ============================================================ */

/* P-run-stop (#257) — KILL cooperativo. */
void bpvm_set_poll(bpvm_t* vm, bpvm_poll_cb_t cb, void* user) {
    if (!vm) return;
    vm->poll_cb   = cb;
    vm->poll_user = user;
}

void bpvm_request_kill(bpvm_t* vm) {
    if (!vm) return;
    /* V8 #695 — el poll «pegajoso» pide el KILL en cada vuelta (con `io`, cada 5 ms). La fase del final lo limpia para que
     * corran los destructores: repetido aquí, los cortaba a los pocos milisegundos. El que la abrió ya está apuntado
     * (d->matado manda en el resultado), y a la fase la vigila su tope. */
    if (bpvm_destruir_abierta_por_kill(vm)) return;
    int antes = bpvm_kill_pedido(vm);
    bpvm_kill_marcar(vm, 1);
    /* V7/H12, etapa 2a.2 — EL KILL DESPIERTA: un run que duerme en la cola (todos los hilos durmiendo, o esperando un
     * hilo de otro run) lo ve ya, no al vencer su espera. Una vez: el poll «pegajoso» lo pide en cada vuelta. El run
     * mira la bandera con el cerrojo cogido antes de dormirse, así que el aviso no se pierde. */
    if (!antes && !bpvm_la_cola.sabotaje_despertar) bpvm_cola_despertar();   /* (el sabotaje: SÓLO PRUEBAS, test-kill) */
}

int bpvm_kill_requested(const bpvm_t* vm) {
    return vm ? bpvm_kill_pedido(vm) : 0;
}

void bpvm_set_pause_cb(bpvm_t* vm, bpvm_pause_cb_t cb, void* user) {
    if (!vm) return;
    vm->pause_cb   = cb;
    vm->pause_user = user;
}

int bpvm_debug_add_breakpoint(bpvm_t* vm, uint32_t pc) {
    if (!vm) return -1;
    /* Idempotente por pc: si ya existe, devuelve su id. */
    for (int i = 0; i < BPVM_MAX_BREAKPOINTS; i++)
        if (vm->breakpoints[i].id != 0 && vm->breakpoints[i].pc == pc)
            return vm->breakpoints[i].id;
    /* Buscar slot libre (id==0). */
    for (int i = 0; i < BPVM_MAX_BREAKPOINTS; i++) {
        if (vm->breakpoints[i].id == 0) {
            int id = ++vm->bp_next_id;
            vm->breakpoints[i].pc = pc;
            vm->breakpoints[i].id = id;
            vm->bp_active++;
            return id;
        }
    }
    return -1;   /* tabla llena */
}

bool bpvm_debug_clear_breakpoint(bpvm_t* vm, int bp_id) {
    if (!vm || bp_id <= 0) return false;
    for (int i = 0; i < BPVM_MAX_BREAKPOINTS; i++) {
        if (vm->breakpoints[i].id == bp_id) {
            vm->breakpoints[i].id = 0;
            vm->breakpoints[i].pc = 0;
            vm->bp_active--;
            return true;
        }
    }
    return false;
}

void bpvm_debug_clear_breakpoints(bpvm_t* vm) {
    if (!vm) return;
    for (int i = 0; i < BPVM_MAX_BREAKPOINTS; i++) {
        vm->breakpoints[i].id = 0;
        vm->breakpoints[i].pc = 0;
    }
    vm->bp_active = 0;
}

int bpvm_debug_list_breakpoints(bpvm_t* vm, uint32_t* out_pcs, int* out_ids, int max) {
    if (!vm) return 0;
    int n = 0;
    for (int i = 0; i < BPVM_MAX_BREAKPOINTS && n < max; i++) {
        if (vm->breakpoints[i].id != 0) {
            if (out_pcs) out_pcs[n] = vm->breakpoints[i].pc;
            if (out_ids) out_ids[n] = vm->breakpoints[i].id;
            n++;
        }
    }
    return n;
}

void bpvm_debug_request_pause(bpvm_t* vm) {
    if (!vm) return;
    vm->pause_requested = 1;
}

uint32_t bpvm_thread_pc(const bpvm_thread_t* tc) { return tc ? tc->pc : 0; }
uint32_t bpvm_thread_sp(const bpvm_thread_t* tc) { return tc ? tc->sp : 0; }
uint32_t bpvm_thread_bp(const bpvm_thread_t* tc) { return tc ? tc->bp : 0; }
uint32_t bpvm_thread_cs(const bpvm_thread_t* tc) { return tc ? tc->cs : 0; }

/* V6/#412 — el argumento de ejecucion. Lo fija quien lanza el programa (el
 * CLI o el verbo RUN del wire) ANTES de bpvm_run; lo lee el builtin __runArg.
 * NULL o "" = no se dio ninguno y manda el valor por defecto del fuente. */
int bpvm_set_run_arg(bpvm_t* vm, const char* arg) {
    if (!vm) return -1;
    vm->run_arg[0] = 0;
    if (!arg) return 0;
    if (strlen(arg) > BPVM_RUN_ARG_MAX) return -1;   /* V8 #764: no se corta; lo dice quien lo recibió */
    memcpy(vm->run_arg, arg, strlen(arg) + 1u);
    return 0;
}

/* V7/H6.c G4.a1 — lo que el fin de RUN cierra en la cintura: los buses que sólo
 * usaba el programa (los de un pack, la TF o el ENV se quedan). */
static void cerrar_bus(int tipo, int id) {
    if (tipo == BPVM_BUS_SPI) bpvm_spi_cerrar(id); else bpvm_i2c_cerrar(id);
}

void bpvm_destroy(bpvm_t* vm) {
    bpvm_out_set_vm(NULL);   /* #478 */

    if (!vm) return;
    /* V7/H5 P2b.3 — los ejecutores de natives, lo PRIMERO: un ejecutor parado a mitad de una native tiene marcos
     * que apuntan al programa; se le da la orden de terminar (salta a su fondo, abandonándolos) antes de tocar nada. */
    bpvm_nativa_liberar_todas(vm);
    /* V7 #592 — la marca del 2º temporizador, de baja: en el PC quien creó la VM libera su memoria después, y el reloj
     * no puede escribir en ella. */
    bpvm_marca_tiempo_baja(vm);
    bpvm_files_fin_de_run(vm);      /* V7/H3 (#491): lo primero, con la memoria y la tabla de handles vivas */
    /* V7 #667 — lo que el programa encendió y nadie soltó (sus destructores, en la fase del final de #668, ya lo han
     * hecho; esto es la red): el PWM, los contadores, las tiras y las UART. Antes que los pines. */
    bpvm_net_fin_de_run();          /* V8/H3: lo que el programa dejó abierto en la red */
    bpvm_wifi_fin_de_run();         /* V8/H3: y la WiFi, si la tocó, como diga la configuración */
    bpvm_bt_fin_de_run();           /* V8/H2: y el Bluetooth, igual */
    bpvm_teclado_fin_de_run();      /* V8/H2, paso 2: y del teclado del SO, lo del programa (sus teclados, lo no leído) */
    bpvm_raton_fin_de_run();        /* V8/H2, paso 3: y del ratón del SO, sus ratones y lo no avisado */
    bpvm_pwm_fin_de_run();
    bpvm_pulse_fin_de_run();
    bpvm_neopixel_fin_de_run();
    bpvm_uart_fin_de_run();
    bpvm_sonido_fin_de_run();       /* V8/H4 (#708): lo que suene se para (la tarea suelta el camino) y el volumen, a 85 */
    bpvm_gpio_estado_reset();       /* V7/H4 (#479): los pines vuelven a libres para el siguiente RUN; V7 #667: y al reset */
    bpvm_adc_fin_de_run();          /* V7 #659: y el ADC del stub, a cero (la rampa, el aviso de «sin backend») */
    bpvm_bus_fin_de_run(cerrar_bus);   /* V7/H6.c G4.a1: y los buses que sólo usaba el programa, se cierran */
    apagar_pack_en_ejecucion(vm);   /* V7/H2 (#493): el overlay no puede apuntar a un vm muerto */
    /* #339 — a una local: la estructura del vm se libera aquí abajo y el barrido
     * va DESPUÉS, cuando ya no queda nada del programa a lo que preguntar. */
    uint64_t run_mark = vm->run_mark;

    /* #451 — `handle_addr` y `handle_gen` YA NO SE LIBERAN: desde que la tabla
     * vive dentro del bloque de la VM, esos punteros apuntan a `vm->memory` y
     * pasarlos por `bpvm_free` seria liberar memoria que no es nuestra. Se van
     * con el bloque. La free-list si sigue saliendo del malloc de plataforma. */
    bpvm_handles_destroy(vm);          /* V4/paso 4c: la free-list (la tabla se va con el bloque) */
    bpvm_destruir_liberar(vm);         /* V7 #668: el registro, la cola y la marca por clase */
    /* Liberar módulos cargados. */
    for (int i = 0; i < vm->module_count; i++) modulo_suelta(&vm->modules[i]);
    bpvm_free(vm->overlay);            /* V8 H9 (#786): si el programa acabó con uno cargado */
    bpvm_free(vm->raiz_dir);
    /* Liberar los hilos —sus structs, sus EH stacks y la línea de su `print` (#600): toda la cola (H12)— y los
     * mutex waiters. */
    bpvm_hilos_liberar(vm);
    for (int i = 0; i < vm->mutex_count; i++) {
        bpvm_free(vm->mutexes[i].waiters);
    }
    bpvm_free(vm->mutexes);
    bpvm_free(vm->symbols);
    for (int i = 0; i < vm->sym_ntrozos; i++) bpvm_free(vm->sym_trozos[i]);   /* #449/#639: los nombres, en trozos */
    bpvm_free(vm->sym_trozos);
    bpvm_free(vm->scratch);
    bpvm_free(vm->gc_valid_map);
    /* #352 — el modelo del GUI es de la VM: se va con ella. Antes vivía en
     * globales de fichero y sobrevivía al programa (ver bpvm_gui_reset). */
#ifdef BPVM_GUI
    bpvm_gui_reset();
#endif
    bpvm_free(vm);

    /* Y ahora el juicio: lo que quede vivo con secuencia >= la marca es memoria
     * del programa que nadie limpió. Habla siempre (control). */
    bpvm_alloc_sweep(run_mark);
}


/* #481 - EL CODIGO DE SALIDA, en UN solo sitio.
 *
 * Antes cada consumidor devolvia `(int) status`, o sea el ORDINAL DEL ENUM de
 * aqui arriba: un `exit 11` que no significa nada, y que cambiaria solo con
 * reordenar la lista. Y estaba escrito cinco veces (el CLI del host y los
 * `map_vm_status` de Pico, ESP32, STM32 y el sim), todas igual de crudas.
 *
 * Criterio de Eduardo (9-sep): «hay que distinguir de un exit diferente de 0, de
 * una excepcion no atrapada: 2 casos» y «un exit(11) no me dice nada». Asi que
 * el numero contesta QUE PASO, en una tabla corta que se pueda explicar, y el
 * DETALLE viaja aparte -por stderr en el PC, en el `errorMessage` del EXITED en
 * placa-. El IDE la recita con `help error`, y por eso vive AQUI: para que la
 * ayuda no pueda desfasarse del codigo.
 *
 *    0   termino bien
 *    1   excepcion BP no atrapada
 *    2   no se pudo cargar el modulo
 *    3   fallo interno de la VM
 *    4   sin memoria
 *  130   parado con Stop/KILL   (convencion 128+SIGINT, ya existia)
 *  131   parado por el depurador
 */
int bpvm_exit_code(bpvm_status_t s) {
    switch (s) {
        case BPVM_OK:                 return 0;

        /* El programa hizo algo mal. Los tres acaban en lo mismo para quien
         * mira desde fuera: el programa se murio por su culpa. */
        case BPVM_ERR_RUNTIME:
        case BPVM_ERR_DIV_BY_ZERO:
        case BPVM_ERR_NULL_RECEIVER:  return 1;

        /* Ni siquiera llego a arrancar: el .mod no se pudo leer, no es un .mod,
         * o es de una ABI que esta VM ya no entiende (#284). */
        case BPVM_ERR_IO:
        case BPVM_ERR_BAD_MAGIC:
        case BPVM_ERR_ABI_MOD_V5:
        case BPVM_ERR_BAD_HEADER:
        case BPVM_ERR_DEP_VERSION:    return 2;   /* V7/H2 (#500): «no se pudo cargar» */

        /* Esto NO es culpa del programa: es un bug nuestro o un .mod corrupto
         * que paso los chequeos. Merece numero propio para no confundirlo con
         * el 1, que es del programa. */
        case BPVM_ERR_BAD_OPCODE:
        case BPVM_ERR_BAD_PC:         return 3;

        case BPVM_ERR_OOM:
        case BPVM_ERR_STACK_OVERFLOW: return 4;

        case BPVM_KILLED:             return 130;
        case BPVM_DBG_STOPPED:        return 131;

        /* Sentinela interno del puente native->BP: nunca escapa a bpvm_run, asi
         * que llegar aqui ya seria un bug. Se trata como fallo interno. */
        case BPVM_NATIVE_RETURN:      return 3;
    }
    return 3;   /* estado nuevo sin clasificar: fallo interno, no silencio */
}

/* #481 - El texto de cada codigo, para que el IDE (`help error`) y cualquier
 * script recite LA MISMA tabla que se acaba de aplicar. */
const char* bpvm_exit_code_str(int code) {
    switch (code) {
        case 0:   return "termino bien";
        case 1:   return "excepcion BP no atrapada";
        case 2:   return "no se pudo cargar el modulo";
        case 3:   return "fallo interno de la VM";
        case 4:   return "sin memoria";
        case 130: return "parado con Stop/KILL";
        case 131: return "parado por el depurador";
        default:  return "codigo desconocido";
    }
}

const char* bpvm_status_str(bpvm_status_t s) {
    switch (s) {
    case BPVM_OK:                 return "OK";
    case BPVM_ERR_IO:             return "IO error";
    case BPVM_ERR_BAD_MAGIC:      return "MAGIC inválido (no es un .mod; se esperaba \"MOD6\", \"MOD7\" o \"MOD8\")";
    case BPVM_ERR_DEP_VERSION:    return "dependencia de otra version (ver el detalle en la salida)";
    case BPVM_ERR_ABI_MOD_V5:     return ".mod v5: es anterior al ensanchado de refs 4->8B, "
                                         "su ABI no se puede garantizar (si es de la era 4B "
                                         "corrompería memoria). Recompílalo con el compilador actual";
    case BPVM_ERR_BAD_HEADER:     return "header inconsistente";
    case BPVM_ERR_OOM:            return "memoria del buffer insuficiente";
    case BPVM_ERR_BAD_OPCODE:     return "opcode desconocido o no soportado";
    case BPVM_ERR_BAD_PC:         return "PC fuera de rango";
    case BPVM_ERR_STACK_OVERFLOW: return "stack overflow";
    case BPVM_ERR_DIV_BY_ZERO:    return "división por cero";
    case BPVM_ERR_NULL_RECEIVER:  return "INVOKE_VIRTUAL sobre null";
    case BPVM_ERR_RUNTIME:        return "RuntimeError BP no atrapado";
    case BPVM_NATIVE_RETURN:      return "native-return (interno)";
    case BPVM_DBG_STOPPED:        return "detenido por el debugger";
    case BPVM_KILLED:             return "terminado por KILL";
    default:                       return "?";
    }
}
