/*
 * platform_freertos.c — V6/A1.5: el contrato de plataforma SOBRE FreeRTOS, común.
 *
 * ─── POR QUÉ ESTE FICHERO NACE EN `src/` Y NO EN UNA FAMILIA ─────────────────
 *
 * Al llevar A1 al STM32 hizo falta lo mismo que la Pico ya tenía: mutex,
 * condvar y hilos sobre FreeRTOS. Y al mirar `pico/platform_freertos.c` resultó
 * que de sus 363 líneas **sólo cuatro** son de la Pico: el reloj (`time_us_64`),
 * la espera activa y el azar (`pico/time.h`, `pico/rand.h`) y la variante con
 * afinidad de núcleo. Todo lo demás es FreeRTOS puro, es decir: común.
 *
 * Copiarlo habría sido repetir el fallo que este proyecto ya tiene documentado
 * tres veces — «un arreglo que no viaja entre familias»: el común crece y la
 * copia privada no, y el día que se arregla un cerrojo aquí, la otra placa se
 * queda con el viejo sin que nadie lo diga. Así que lo que hay aquí es LA
 * implementación, y cada familia pone sólo lo suyo.
 *
 * ⚠️ Hoy lo compila SÓLO el STM32, y la Pico NO — pero ya no por precaución:
 * SE INTENTÓ Y SE MIDIÓ (5-sep). Con este fichero, la Pico hacía las 2 000 líneas
 * de `PrintBench` en **5 330 ms** en vez de **4 040** con el suyo: un 32 % peor,
 * repetible a ±10 ms en cuatro pasadas y aislado por bisección (se volvió a su
 * fichero y el tiempo volvió). Curiosamente el KILL MEJORÓ, de 33 ms a 2, o sea
 * que `io` se despierta antes — la diferencia está en cómo se reparte el turno,
 * no en trabajo de más, y ésa es la pista para quien lo retome.
 *
 * No se subió porque no se supo explicar, que es la regla de esta casa: no se
 * publica lo que no se entiende, aunque compile y funcione. La migración sigue
 * siendo lo correcto —evitar la copia privada que no crece con el común— pero
 * pide antes encontrar ese 32 %.
 *
 * Remedido el 30-sep (V7/H7, el paso 0 de docs/FUSION_BACKENDS_FREERTOS.md):
 * +10,8 % —4633 frente a 5132 ms—, con Bench, AllocBench, el KILL y la fuga de
 * hilos iguales. Descartados midiendo: el broadcast, el final de hilo, la
 * colocación y la prioridad de los hilos genéricos; lo que corre por línea es
 * idéntico. La Pico se prueba con esto usando -DBPVM_PICO_PLATFORM_COMUN=ON.
 *
 * ─── LO QUE CADA FAMILIA TIENE QUE PONER APARTE ──────────────────────────────
 *
 *   bpvm_platform_now_ms()        — su reloj
 *   bpvm_platform_busy_wait_us()  — su espera activa
 *   bpvm_platform_random_u32()    — su fuente de azar
 *   BPVM_FR_STACK_VM / _IO / _THR — el tamaño de pila de sus tareas (abajo)
 *
 * ─── LA REGLA DE LA PRIORIDAD, QUE ES CONTRATO Y NO AFINACIÓN ────────────────
 *
 * `io` va a la MISMA prioridad que la tarea que ejecuta la VM. Está medido en
 * dos placas (5-sep): por DEBAJO, `io` no se ejecuta mientras el programa
 * calcula y un KILL no llega nunca; por ENCIMA, cada `print` lo despierta y
 * expulsa a la VM (la C6 pasó de 486 a 848 ms y la Pico de 3 958 a 4 832). El
 * detalle largo está en `include/bpvm_platform.h`.
 */
#include "bpvm_platform.h"
#include "bpvm_alloc.h"   /* V7/H5 P2b.3: la pila del ejecutor de natives, de malloc */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* ─── Tamaños de pila, en PALABRAS (FreeRTOS vanilla; ojo: en ESP-IDF son
 * bytes, y esa diferencia ya costó un susto en este proyecto). Los pone la
 * familia con un -D si los suyos son otros. ───────────────────────────────── */
#ifndef BPVM_FR_PRIO_VM
#define BPVM_FR_PRIO_VM   (tskIDLE_PRIORITY + 2)
#endif
#ifndef BPVM_FR_STACK_IO
#define BPVM_FR_STACK_IO  1024u    /* 4 KB: el sink arma su mensaje en pila */
#endif
#ifndef BPVM_FR_STACK_THR
#define BPVM_FR_STACK_THR 1024u    /* hilos BP (Thread de BasicPlus) */
#endif

/* ============================== Mutex ================================== */

int bpvm_platform_mutex_init(bpvm_platform_mutex_handle_t* m) {
    if (!m) return -1;
    SemaphoreHandle_t s = xSemaphoreCreateMutex();
    if (!s) return -1;
    *m = (void*) s;
    return 0;
}

void bpvm_platform_mutex_destroy(bpvm_platform_mutex_handle_t* m) {
    if (!m || !*m) return;
    vSemaphoreDelete((SemaphoreHandle_t) *m);
    *m = NULL;
}

void bpvm_platform_mutex_lock(bpvm_platform_mutex_handle_t* m) {
    if (!m || !*m) return;
    xSemaphoreTake((SemaphoreHandle_t) *m, portMAX_DELAY);
}

void bpvm_platform_mutex_unlock(bpvm_platform_mutex_handle_t* m) {
    if (!m || !*m) return;
    xSemaphoreGive((SemaphoreHandle_t) *m);
}

/* ============================== Condvar ==========================
 * V7/H12, etapa 2b (5-oct): la COMÚN a las tres familias (la Pico, la ESP32 y la STM32), en vez de una copia. La de
 * antes —un semáforo binario y una cuenta de esperadores— perdía avisos con dos esperando (un `broadcast` despertaba
 * a uno) y dejaba que un tercero se llevara el aviso de otro: con dos runs, un run dormido para siempre. El porqué y
 * el cómo, allí; la prueba, test/test_frcond.c (make test-frcond), con el kernel de FreeRTOS de verdad. */
#include "platform_freertos_cond.inc"

/* ============================== Hilos ==================================
 * Cada hilo lleva su semáforo de «he terminado» para que `join` espere de
 * verdad: `vTaskDelete(NULL)` no avisa a nadie. */

typedef struct {
    TaskHandle_t        task;
    SemaphoreHandle_t   exited;
    bpvm_thread_entry_t entry;
    void*               arg;
    int                 core_id;   /* -1 = sin afinidad; lo lee el gancho de abajo */
    /* V7/H5 P2b.3 — sólo el ejecutor de una native: su TCB y su pila, de malloc (NULL en los demás). */
    void*               tcb;
    void*               pila;
} fr_thread_t;

/* ⚠️ EL HILO NO SE BORRA A SÍ MISMO, Y ESO NO ES UN DETALLE.
 *
 * `vTaskDelete(NULL)` no libera la pila ni el TCB en el acto: los deja en manos
 * de la tarea OCIOSA. Y si la tarea ociosa no llega a correr —porque `vm` está
 * a prioridad 2 sondeando el wire y ella está a 0— esa memoria NO VUELVE NUNCA.
 *
 * Medido en la Nucleo el 5-sep, y lo dijo el gancho que hay puesto para eso:
 * cada RUN creaba su hilo `io` (4 KB de pila + TCB), ninguno se reciclaba, y al
 * quinto la placa se paró con «RTOS: sin heap de FreeRTOS». No fue un cuelgue
 * mudo: fue un mensaje en el log persistente, que es exactamente para lo que
 * `configUSE_MALLOC_FAILED_HOOK` está encendido.
 *
 * Así que el hilo avisa y se DUERME, y es `join` quien lo borra: borrar una
 * tarea que no es la actual sí libera su memoria en el acto, sin depender de
 * que nadie más tenga turno. */
/* #153 — la familia puede querer hacer algo EN EL CORE del hilo antes de que
 * empiece: la Pico registra ahí el core como víctima de lockout de flash, para
 * que el core que escribe pueda parquearlo. Por defecto no hace nada, y esa es
 * la forma de que el común no sepa de flash ni de cores. */
void bpvm_platform_thread_arranca(int core_id);
__attribute__((weak)) void bpvm_platform_thread_arranca(int core_id) { (void) core_id; }

static void fr_thread_trampoline(void* pv) {
    fr_thread_t* ft = (fr_thread_t*) pv;
    bpvm_platform_thread_arranca(ft->core_id);
    ft->entry(ft->arg);
    if (ft->exited) xSemaphoreGive(ft->exited);
    vTaskSuspend(NULL);
    for (;;) { vTaskDelay(portMAX_DELAY); }   /* por si acaso alguien la reanuda */
}

static int fr_spawn(bpvm_platform_thread_handle_t* t, bpvm_thread_entry_t entry,
                    void* arg, const char* nombre, uint16_t pila, UBaseType_t prio,
                    int core_id) {
    if (!t || !entry) return -1;
    fr_thread_t* ft = (fr_thread_t*) pvPortMalloc(sizeof(fr_thread_t));
    if (!ft) return -1;
    ft->exited = xSemaphoreCreateBinary();
    if (!ft->exited) { vPortFree(ft); return -1; }
    ft->entry   = entry;
    ft->arg     = arg;
    ft->core_id = core_id;
    ft->tcb     = NULL;
    ft->pila    = NULL;
#if (configNUMBER_OF_CORES > 1) && (configUSE_CORE_AFFINITY == 1)
    /* Con SMP y un core pedido, se fija la afinidad al crear. Hoy ninguna familia
     * lo activa (la Pico va con configNUMBER_OF_CORES=1 hasta que cierre #153),
     * pero el camino vive aquí y no en una copia de familia. */
    if (core_id >= 0 && core_id < configNUMBER_OF_CORES) {
        UBaseType_t mascara = (UBaseType_t) 1U << core_id;
        if (xTaskCreateAffinitySet(fr_thread_trampoline, nombre, pila, ft, prio,
                                    mascara, &ft->task) != pdPASS) {
            vSemaphoreDelete(ft->exited);
            vPortFree(ft);
            return -1;
        }
        *t = (void*) ft;
        return 0;
    }
#endif
    if (xTaskCreate(fr_thread_trampoline, nombre, pila, ft, prio, &ft->task) != pdPASS) {
        vSemaphoreDelete(ft->exited);
        vPortFree(ft);
        return -1;
    }
    *t = (void*) ft;
    return 0;
}

int bpvm_platform_thread_create(bpvm_platform_thread_handle_t* t,
                                 bpvm_thread_entry_t entry, void* arg) {
    return fr_spawn(t, entry, arg, "bpvm-thr", BPVM_FR_STACK_THR, BPVM_FR_PRIO_VM, -1);
}

/* El hilo `io`: MISMA prioridad que la VM (ver la cabecera). */
int bpvm_platform_thread_create_io(bpvm_platform_thread_handle_t* t,
                                    bpvm_thread_entry_t entry, void* arg) {
    /* #485 - `io` NO LLEVA NUMERO: hereda el de quien lo arranca.
     *
     * El contrato dice «io a la MISMA prioridad que la tarea que ejecuta la VM,
     * nunca por debajo» (bpvm_platform.h), y hasta hoy eso se VIGILABA: habia
     * un literal aqui y otro donde cada familia crea su tarea de VM, y tenian
     * que coincidir a mano. En el P4 dejaron de coincidir -su VM corre en
     * wire_task a prioridad 5 y esto ponia io a 1- sin que nada lo dijera.
     *
     * bpvm_io_start() lo llama SIEMPRE la tarea que ejecuta la VM (los cuatro
     * repl lo hacen desde el camino del RUN), asi que uxTaskPriorityGet(NULL)
     * ES la prioridad de la VM. Con esto el contrato pasa de vigilado a
     * CONSTRUIDO: no hay dos numeros que puedan divergir. */
    return fr_spawn(t, entry, arg, "bpvm-io", BPVM_FR_STACK_IO,
                    (int) uxTaskPriorityGet(NULL), -1);
}

/* Sin SMP la afinidad no significa nada: se acepta y se ignora, que es lo que
 * dice el contrato. */
int bpvm_platform_thread_create_pinned(bpvm_platform_thread_handle_t* t,
                                        bpvm_thread_entry_t entry, void* arg,
                                        int core_id) {
    /* Sin SMP la afinidad no significa nada: `fr_spawn` la ignora y crea el hilo
     * corriente, que es lo que dice el contrato. */
    return fr_spawn(t, entry, arg, "bpvm-thr-pin", BPVM_FR_STACK_THR,
                    BPVM_FR_PRIO_VM, core_id);
}

void bpvm_platform_thread_join(bpvm_platform_thread_handle_t* t) {
    if (!t || !*t) return;
    fr_thread_t* ft = (fr_thread_t*) *t;
    if (ft->exited) {
        xSemaphoreTake(ft->exited, portMAX_DELAY);
        vSemaphoreDelete(ft->exited);
    }
    /* Aquí es donde se recicla de verdad la pila y el TCB (ver el trampolín). */
    if (ft->task) vTaskDelete(ft->task);
    /* P2b.3 — el ejecutor de una native: su memoria es nuestra, y se devuelve DESPUÉS de borrar la tarea (borrar
     * OTRA tarea la saca de las listas del núcleo en el acto). */
    if (ft->tcb)  bpvm_free(ft->tcb);
    if (ft->pila) bpvm_free(ft->pila);
    vPortFree(ft);
    *t = NULL;
}

/* ─── V7/H5 P2b.3 — EL TESTIGO Y EL HILO DEL EJECUTOR DE NATIVES ───────────────────────────────────────────── */

int bpvm_platform_sem_init(bpvm_platform_sem_handle_t* s) {
    if (!s) return -1;
    SemaphoreHandle_t x = xSemaphoreCreateBinary();   /* nace a 0 */
    if (!x) return -1;
    *s = (void*) x;
    return 0;
}

void bpvm_platform_sem_destroy(bpvm_platform_sem_handle_t* s) {
    if (!s || !*s) return;
    vSemaphoreDelete((SemaphoreHandle_t) *s);
    *s = NULL;
}

void bpvm_platform_sem_give(bpvm_platform_sem_handle_t* s) {
    xSemaphoreGive((SemaphoreHandle_t) *s);   /* ya dado: se queda dado (binario) */
}

void bpvm_platform_sem_take(bpvm_platform_sem_handle_t* s) {
    xSemaphoreTake((SemaphoreHandle_t) *s, portMAX_DELAY);
}

/* El ejecutor es una tarea ESTÁTICA con su TCB y su pila de malloc: el heap de FreeRTOS está dimensionado para
 * `vm` e `io` (24 KB en la STM32) y una pila de ejecutor no cabe en él; así sólo cuesta mientras existe. Nace de
 * la tarea de la VM y a su prioridad, como `io`. Termina como los demás hilos de aquí: avisa y se SUSPENDE, y
 * `join` la borra y devuelve su memoria (la propia tarea no podría: su TCB seguiría en la lista de terminadas
 * hasta que corriera la ociosa). */
int bpvm_platform_thread_create_nativa(bpvm_platform_thread_handle_t* t,
                                        bpvm_thread_entry_t entry, void* arg,
                                        uint32_t pila_bytes) {
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    if (!t || !entry || pila_bytes < configMINIMAL_STACK_SIZE * sizeof(StackType_t)) return -1;
    fr_thread_t* ft = (fr_thread_t*) pvPortMalloc(sizeof(fr_thread_t));
    if (!ft) return -1;
    ft->exited  = xSemaphoreCreateBinary();
    ft->entry   = entry;
    ft->arg     = arg;
    ft->core_id = -1;
    ft->tcb     = bpvm_malloc_os(sizeof(StaticTask_t));
    ft->pila    = bpvm_malloc_os(pila_bytes);
    ft->task    = NULL;
    if (ft->exited && ft->tcb && ft->pila)
        ft->task = xTaskCreateStatic(fr_thread_trampoline, "bpvm-nat",
                                     (configSTACK_DEPTH_TYPE) (pila_bytes / sizeof(StackType_t)), ft,
                                     uxTaskPriorityGet(NULL), (StackType_t*) ft->pila, (StaticTask_t*) ft->tcb);
    if (ft->task == NULL) {
        if (ft->exited) vSemaphoreDelete(ft->exited);
        if (ft->tcb)  bpvm_free(ft->tcb);
        if (ft->pila) bpvm_free(ft->pila);
        vPortFree(ft);
        return -1;
    }
    *t = (void*) ft;
    return 0;
#else
    (void) t; (void) entry; (void) arg; (void) pila_bytes;
    return -1;   /* sin configSUPPORT_STATIC_ALLOCATION no hay ejecutor: las natives corren en la tarea de la VM */
#endif
}

int32_t bpvm_platform_pila_libre_actual(void) {
    return (int32_t) (uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t));
}

void bpvm_platform_thread_yield(void) { taskYIELD(); }

void bpvm_platform_thread_sleep_ms(int ms) {
    if (ms <= 0) { taskYIELD(); return; }
    TickType_t ticks = pdMS_TO_TICKS(ms);
    if (ticks == 0) ticks = 1;     /* mismo motivo que arriba: dormir 0 no es dormir */
    vTaskDelay(ticks);
}
