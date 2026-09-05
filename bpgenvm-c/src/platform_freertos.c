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
 * ⚠️ Hoy lo compila SÓLO el STM32. La Pico sigue con su fichero hasta que se
 * pueda verificar EN PLACA el cambio — mover código que funciona a un sitio
 * nuevo sin poder ejecutarlo sería exactamente el «verde falso» que aquí se
 * persigue. Esa migración es una ficha propia, y es una BORRADO, no una fusión.
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

/* ============================== Condvar ================================
 * FreeRTOS no tiene condvar. La clásica: un semáforo binario y una cuenta de
 * esperadores. Quien llama trae el mutex tomado; lo soltamos, esperamos la
 * señal y lo volvemos a tomar antes de salir. */

typedef struct {
    SemaphoreHandle_t sem;     /* binario, empieza a 0 */
    SemaphoreHandle_t guard;   /* protege `waiters` */
    int               waiters;
} fr_cond_t;

int bpvm_platform_cond_init(bpvm_platform_cond_handle_t* c) {
    if (!c) return -1;
    fr_cond_t* cv = (fr_cond_t*) pvPortMalloc(sizeof(fr_cond_t));
    if (!cv) return -1;
    cv->sem   = xSemaphoreCreateBinary();
    cv->guard = xSemaphoreCreateMutex();
    cv->waiters = 0;
    if (!cv->sem || !cv->guard) {
        if (cv->sem)   vSemaphoreDelete(cv->sem);
        if (cv->guard) vSemaphoreDelete(cv->guard);
        vPortFree(cv);
        return -1;
    }
    *c = (void*) cv;
    return 0;
}

void bpvm_platform_cond_destroy(bpvm_platform_cond_handle_t* c) {
    if (!c || !*c) return;
    fr_cond_t* cv = (fr_cond_t*) *c;
    vSemaphoreDelete(cv->sem);
    vSemaphoreDelete(cv->guard);
    vPortFree(cv);
    *c = NULL;
}

void bpvm_platform_cond_wait(bpvm_platform_cond_handle_t* c,
                              bpvm_platform_mutex_handle_t* m) {
    if (!c || !*c || !m || !*m) return;
    fr_cond_t* cv = (fr_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    cv->waiters++;
    xSemaphoreGive(cv->guard);
    xSemaphoreGive((SemaphoreHandle_t) *m);
    xSemaphoreTake(cv->sem, portMAX_DELAY);
    xSemaphoreTake((SemaphoreHandle_t) *m, portMAX_DELAY);
}

int bpvm_platform_cond_timed_wait(bpvm_platform_cond_handle_t* c,
                                   bpvm_platform_mutex_handle_t* m, int ms) {
    if (!c || !*c || !m || !*m) return 1;
    fr_cond_t* cv = (fr_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    cv->waiters++;
    xSemaphoreGive(cv->guard);
    xSemaphoreGive((SemaphoreHandle_t) *m);

    /* V6/A1.2 — REDONDEAR HACIA ARRIBA, NUNCA A CERO.
     *
     * `pdMS_TO_TICKS(ms)` es una división entera: con el tick a 100 Hz,
     * cualquier espera de menos de 10 ms da 0 ticks, y `xSemaphoreTake(sem, 0)`
     * no espera — sondea. Quien pidió «hasta N ms» se encuentra con que no
     * espera nada y gira en vacío. Lo destapó el hilo `io` en la C6: pedía 5 ms,
     * se los daban a cero, y fib(28) pasó de 23 400 a 140 600 ms. Un tick es lo
     * mínimo que este reloj sabe esperar, así que es lo correcto. */
    TickType_t ticks = 0;
    if (ms > 0) {
        ticks = pdMS_TO_TICKS(ms);
        if (ticks == 0) ticks = 1;
    }
    BaseType_t r = xSemaphoreTake(cv->sem, ticks);
    xSemaphoreTake((SemaphoreHandle_t) *m, portMAX_DELAY);
    if (r != pdTRUE) {
        xSemaphoreTake(cv->guard, portMAX_DELAY);
        if (cv->waiters > 0) cv->waiters--;
        xSemaphoreGive(cv->guard);
        return 1;                     /* venció el plazo */
    }
    return 0;
}

void bpvm_platform_cond_signal(bpvm_platform_cond_handle_t* c) {
    if (!c || !*c) return;
    fr_cond_t* cv = (fr_cond_t*) *c;
    xSemaphoreTake(cv->guard, portMAX_DELAY);
    int hay = cv->waiters;
    if (hay > 0) cv->waiters--;
    xSemaphoreGive(cv->guard);
    if (hay > 0) xSemaphoreGive(cv->sem);
}

void bpvm_platform_cond_broadcast(bpvm_platform_cond_handle_t* c) {
    if (!c || !*c) return;
    fr_cond_t* cv = (fr_cond_t*) *c;
    for (;;) {
        xSemaphoreTake(cv->guard, portMAX_DELAY);
        int hay = cv->waiters;
        if (hay > 0) cv->waiters--;
        xSemaphoreGive(cv->guard);
        if (hay <= 0) break;
        xSemaphoreGive(cv->sem);
    }
}

/* ============================== Hilos ==================================
 * Cada hilo lleva su semáforo de «he terminado» para que `join` espere de
 * verdad: `vTaskDelete(NULL)` no avisa a nadie. */

typedef struct {
    TaskHandle_t        task;
    SemaphoreHandle_t   exited;
    bpvm_thread_entry_t entry;
    void*               arg;
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
static void fr_thread_trampoline(void* pv) {
    fr_thread_t* ft = (fr_thread_t*) pv;
    ft->entry(ft->arg);
    if (ft->exited) xSemaphoreGive(ft->exited);
    vTaskSuspend(NULL);
    for (;;) { vTaskDelay(portMAX_DELAY); }   /* por si acaso alguien la reanuda */
}

static int fr_spawn(bpvm_platform_thread_handle_t* t, bpvm_thread_entry_t entry,
                    void* arg, const char* nombre, uint16_t pila, UBaseType_t prio) {
    if (!t || !entry) return -1;
    fr_thread_t* ft = (fr_thread_t*) pvPortMalloc(sizeof(fr_thread_t));
    if (!ft) return -1;
    ft->exited = xSemaphoreCreateBinary();
    if (!ft->exited) { vPortFree(ft); return -1; }
    ft->entry = entry;
    ft->arg   = arg;
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
    return fr_spawn(t, entry, arg, "bpvm-thr", BPVM_FR_STACK_THR, BPVM_FR_PRIO_VM);
}

/* El hilo `io`: MISMA prioridad que la VM (ver la cabecera). */
int bpvm_platform_thread_create_io(bpvm_platform_thread_handle_t* t,
                                    bpvm_thread_entry_t entry, void* arg) {
    return fr_spawn(t, entry, arg, "bpvm-io", BPVM_FR_STACK_IO, BPVM_FR_PRIO_VM);
}

/* Sin SMP la afinidad no significa nada: se acepta y se ignora, que es lo que
 * dice el contrato. */
int bpvm_platform_thread_create_pinned(bpvm_platform_thread_handle_t* t,
                                        bpvm_thread_entry_t entry, void* arg,
                                        int core_id) {
    (void) core_id;
    return bpvm_platform_thread_create(t, entry, arg);
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
    vPortFree(ft);
    *t = NULL;
}

void bpvm_platform_thread_yield(void) { taskYIELD(); }

void bpvm_platform_thread_sleep_ms(int ms) {
    if (ms <= 0) { taskYIELD(); return; }
    TickType_t ticks = pdMS_TO_TICKS(ms);
    if (ticks == 0) ticks = 1;     /* mismo motivo que arriba: dormir 0 no es dormir */
    vTaskDelay(ticks);
}
