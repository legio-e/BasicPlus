/*
 * wire_v1.c (RP2350) — EL CABLE del wire v1: USB-CDC. Y nada más.
 *
 * [V6/U2.1, 24-ago-2026] Aquí vivían además los quince constructores de JSON,
 * y con ellos otras dos copias idénticas en el S3 y en la P4. Se midieron
 * palabra por palabra —idénticas— y se fueron a `src/wire_v1_proto.c`. Lo que
 * queda es lo que de verdad es de esta placa: cuatro funciones que mueven bytes
 * por USB-CDC, más el mutex de transmisión.
 *
 * El contrato de las dos capas está en `include/bpvm_wire_v1.h`.
 */

#include "wire_v1.h"   /* la API completa: cable + protocolo comun */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico/stdlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===================== Lectura ===================== */

static int try_get_char_v1(void) {
    return getchar_timeout_us(0);
}

int wire_v1_recv_line(int first_char_already_read,
                       char* buf, size_t buf_max) {
    size_t n = 0;
    /* Sembrado opcional con el char ya consumido por el dispatcher. */
    if (first_char_already_read >= 0) {
        if (n + 1 >= buf_max) return -1;
        buf[n++] = (char) first_char_already_read;
    }
    for (;;) {
        int c = try_get_char_v1();
        if (c < 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }
        if (c == '\n') return (int) n;
        if (c == '\r') continue;             /* tolerante: CR ignorado */
        if (n + 1 >= buf_max) return -1;     /* overflow */
        buf[n++] = (char) c;
    }
}

int wire_v1_recv_bulk(uint8_t* buf, size_t n, size_t buf_max) {
    if (n > buf_max) return -1;
    size_t got = 0;
    while (got < n) {
        int c = try_get_char_v1();
        if (c < 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }
        buf[got++] = (uint8_t) c;
    }
    return (int) n;
}

/* ===================== Escritura ===================== */

/* P-autorun (#256) — mutex de línea del wire.
 *
 * Hasta #256 el wire tenía un único escritor en cada momento (REPL task
 * en idle; comm task durante un run SMP, con los acks del poll DIFERIDOS
 * a después del run precisamente para no entrelazarse). Para poder
 * CONECTARSE a la placa con un autorun corriendo, el poll necesita
 * contestar HELLO/BUSY en caliente — es decir, dos escritores
 * concurrentes (comm task con OUTPUTs + poll con replies). FreeRTOS es
 * single-core (configNUMBER_OF_CORES=1) pero con preemption: un mutex
 * por LÍNEA mantiene la atomicidad. Coste en el caso común: un take/give
 * sin contención por línea.
 *
 * Lazy init seguro: la primera escritura ocurre en vm_task (boot, sin
 * comm task viva todavía) — no hay carrera de creación. */
static SemaphoreHandle_t s_wire_tx_mutex = NULL;

void wire_v1_tx_lock(void) {
    if (s_wire_tx_mutex == NULL) s_wire_tx_mutex = xSemaphoreCreateMutex();
    if (s_wire_tx_mutex != NULL) xSemaphoreTake(s_wire_tx_mutex, portMAX_DELAY);
}

void wire_v1_tx_unlock(void) {
    if (s_wire_tx_mutex != NULL) xSemaphoreGive(s_wire_tx_mutex);
}

void wire_v1_send_line(const char* data, size_t len) {
    wire_v1_tx_lock();
    fwrite(data, 1, len, stdout);
    fputc('\n', stdout);
    fflush(stdout);
    wire_v1_tx_unlock();
}

void wire_v1_send_bulk(const uint8_t* data, size_t n) {
    /* Nota #256: el par línea-de-tamaño + bulk NO es atómico bajo el
     * mutex, pero los comandos FILES nunca conviven con un run (BUSY),
     * así que no hay con quién entrelazarse. El lock es por higiene. */
    wire_v1_tx_lock();
    fwrite(data, 1, n, stdout);
    fflush(stdout);
    wire_v1_tx_unlock();
}
