/*
 * builtins.c — dispatch de CALL_BUILTIN para F2.
 *
 * IDs estables del enum Builtin de la VM Java (docs/BUILTINS.md).
 * F2 cubre el subset mínimo para programas que usan strings y arrays
 * comunes; el resto se va añadiendo según haga falta.
 */

#include "bpvm_internal.h"
#include "bpvm_platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bpvm_gpio.h"
#include "bpvm_i2c.h"
#include "bpvm_spi.h"
#include "bpvm_pulse.h"
#include "bpvm_pwm.h"
#include "bpvm_pico.h"
#include "bpvm_rtc.h"
#include "bpvm_adc.h"
#include "bpvm_wdt.h"
#include "bpvm_uart.h"

/* IDs estables (= ordinal del enum Builtin Java). Sólo los que F2
 * implementa; los demás devuelven BAD_OPCODE para que el caller sepa
 * que falta. */
enum {
    BUILTIN_STRLEN          = 0,
    BUILTIN_PARSE_INT       = 1,
    BUILTIN_INT_TO_STRING   = 3,
    BUILTIN_BOOL_TO_STRING  = 5,
    BUILTIN_CHAR_AT         = 14,
    BUILTIN_NOW             = 34,
    BUILTIN_SLEEP           = 35,
    BUILTIN_GC              = 43,
    BUILTIN_NEW_REF_ARRAY   = 44,
    BUILTIN_GROW_INT_ARRAY  = 46,
    BUILTIN_CHARS_TO_STRING = 47,
    BUILTIN_CHAR_CODE_AT    = 48,
    BUILTIN_THREAD_START    = 49,
    BUILTIN_THREAD_JOIN     = 50,
    BUILTIN_YIELD           = 51,
    BUILTIN_MUTEX_CREATE    = 52,
    BUILTIN_MUTEX_LOCK      = 53,
    BUILTIN_MUTEX_UNLOCK    = 54,
    BUILTIN_MOVE            = 55,
    /* Gpio.* — paridad con enum Builtin Java (ordinals 78..81). */
    BUILTIN_GPIO_INIT       = 78,
    BUILTIN_GPIO_WRITE      = 79,
    BUILTIN_GPIO_READ       = 80,
    BUILTIN_GPIO_PULL       = 81,
    /* I2c.* — ordinals 82..84. */
    BUILTIN_I2C_INIT        = 82,
    BUILTIN_I2C_WRITE       = 83,
    BUILTIN_I2C_READ        = 84,
    /* Allocator visible al user (ordinal 85). */
    BUILTIN_NEW_INT_ARRAY   = 85,
    /* Spi.* — ordinals 86..89. */
    BUILTIN_SPI_INIT        = 86,
    BUILTIN_SPI_WRITE       = 87,
    BUILTIN_SPI_READ        = 88,
    BUILTIN_SPI_TRANSFER    = 89,
    /* Uart.* — ordinals 90..93. */
    BUILTIN_UART_INIT       = 90,
    BUILTIN_UART_WRITE      = 91,
    BUILTIN_UART_READ       = 92,
    BUILTIN_UART_AVAILABLE  = 93,
    /* Pulse counter — ordinals 94..98. */
    BUILTIN_PULSE_INIT      = 94,
    BUILTIN_PULSE_START     = 95,
    BUILTIN_PULSE_STOP      = 96,
    BUILTIN_PULSE_VALUE     = 97,
    BUILTIN_PULSE_RESET     = 98,
    /* PWM — ordinals 99..103. */
    BUILTIN_PWM_INIT        = 99,
    BUILTIN_PWM_SET_FREQ    = 100,
    BUILTIN_PWM_SET_DUTY    = 101,
    BUILTIN_PWM_START       = 102,
    BUILTIN_PWM_STOP        = 103,
    /* Pico (info MCU) — ordinals 104..108. */
    BUILTIN_PICO_UNIQUE_ID  = 104,
    BUILTIN_PICO_BOARD_NAME = 105,
    BUILTIN_PICO_TEMP_C     = 106,
    BUILTIN_PICO_CPU_FREQ_HZ = 107,
    BUILTIN_PICO_UPTIME_MS  = 108,
    /* Time (sleep variantes) — ordinals 109..110. */
    BUILTIN_SLEEP_SEC       = 109,
    BUILTIN_SLEEP_US        = 110,
    /* Pico overclock — ordinal 111. */
    BUILTIN_PICO_SET_CPU_FREQ_MHZ = 111,
    /* Rtc — ordinales 112..113. */
    BUILTIN_RTC_NOW_SEC     = 112,
    BUILTIN_RTC_SET_NOW_SEC = 113,
    /* Adc — ordinales 114..115. */
    BUILTIN_ADC_INIT_CHANNEL = 114,
    BUILTIN_ADC_READ_CHANNEL = 115,
    /* Wdt — ordinales 116..118. */
    BUILTIN_WDT_ENABLE       = 116,
    BUILTIN_WDT_FEED         = 117,
    BUILTIN_WDT_DISABLE      = 118
};

/* Helpers: pop / push del thread actual. */
static int32_t pop_i32(bpvm_t* vm, bpvm_thread_t* tc) {
    tc->sp -= 4;
    return bpvm_read_i32_be(vm->memory + tc->sp);
}
static void push_i32(bpvm_t* vm, bpvm_thread_t* tc, int32_t v) {
    bpvm_write_i32_be(vm->memory + tc->sp, v);
    tc->sp += 4;
}

/* Lee un string BP (TYPE_ARRAY_I32 con codepoints) a un buffer C UTF-8.
 * Devuelve el número de bytes escritos (sin null terminator). Si el
 * codepoint > 127 lo escribe como '?' (F2 v1 sólo ASCII). */
static size_t read_bp_string(const bpvm_t* vm, uint32_t ref, char* dst, size_t dst_size) {
    if (ref == 0) return 0;
    uint32_t len = bpvm_read_u32_be(vm->memory + ref);
    size_t out = 0;
    for (uint32_t i = 0; i < len && out + 1 < dst_size; i++) {
        uint32_t cp = bpvm_read_u32_be(vm->memory + ref + 4 + i * 4);
        dst[out++] = (cp < 128) ? (char) cp : '?';
    }
    if (out < dst_size) dst[out] = '\0';
    return out;
}

bpvm_status_t bpvm_call_builtin(bpvm_t* vm, bpvm_thread_t* tc, int id) {
    switch (id) {

    case BUILTIN_STRLEN: {
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        uint32_t len = (ref == 0) ? 0 : bpvm_read_u32_be(vm->memory + ref);
        push_i32(vm, tc, (int32_t) len);
        return BPVM_OK;
    }

    case BUILTIN_INT_TO_STRING: {
        int32_t v = pop_i32(vm, tc);
        char buf[32];
        int n = snprintf(buf, sizeof(buf), "%d", v);
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, (size_t)(n > 0 ? n : 0));
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }

    case BUILTIN_BOOL_TO_STRING: {
        int32_t v = pop_i32(vm, tc);
        const char* s = v ? "true" : "false";
        uint32_t ref = bpvm_heap_alloc_string(vm, s, strlen(s));
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }

    case BUILTIN_PARSE_INT: {
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        char buf[64];
        read_bp_string(vm, ref, buf, sizeof(buf));
        /* trim simple */
        char* p = buf; while (*p == ' ' || *p == '\t') p++;
        char* end = p + strlen(p);
        while (end > p && (end[-1] == ' ' || end[-1] == '\n' || end[-1] == '\r' || end[-1] == '\t')) {
            *--end = '\0';
        }
        long v = strtol(p, NULL, 10);
        push_i32(vm, tc, (int32_t) v);
        return BPVM_OK;
    }

    case BUILTIN_CHAR_CODE_AT: {
        int32_t i   = pop_i32(vm, tc);
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        if (ref == 0) { push_i32(vm, tc, 0); return BPVM_OK; }
        uint32_t len = bpvm_read_u32_be(vm->memory + ref);
        if (i < 0 || (uint32_t) i >= len) { push_i32(vm, tc, 0); return BPVM_OK; }
        uint32_t cp = bpvm_read_u32_be(vm->memory + ref + 4 + (uint32_t) i * 4);
        push_i32(vm, tc, (int32_t) cp);
        return BPVM_OK;
    }

    case BUILTIN_CHAR_AT: {
        /* charAt(str, idx): string  — devuelve un string de 1 char con
         * el codepoint en esa posición. Si idx fuera de rango, devuelve
         * string vacía. */
        int32_t i    = pop_i32(vm, tc);
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        uint32_t cp  = 0;
        if (ref != 0) {
            uint32_t len = bpvm_read_u32_be(vm->memory + ref);
            if (i >= 0 && (uint32_t) i < len) {
                cp = bpvm_read_u32_be(vm->memory + ref + 4 + (uint32_t) i * 4);
            }
        }
        /* Alocamos un string de 1 codepoint. */
        uint32_t out = bpvm_heap_alloc(vm, 4u, BPVM_TYPE_ARRAY_I32);
        if (out) {
            bpvm_write_u32_be(vm->memory + out, 1u);
            bpvm_write_u32_be(vm->memory + out + 4, cp);
        }
        push_i32(vm, tc, (int32_t) out);
        return BPVM_OK;
    }

    case BUILTIN_NEW_REF_ARRAY: {
        int32_t cap = pop_i32(vm, tc);
        if (cap < 0) return BPVM_ERR_RUNTIME;
        uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) cap * 4, BPVM_TYPE_ARRAY_REF);
        if (ref == 0) return BPVM_ERR_OOM;
        bpvm_write_u32_be(vm->memory + ref, (uint32_t) cap);
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }

    case BUILTIN_GROW_INT_ARRAY: {
        int32_t new_cap = pop_i32(vm, tc);
        uint32_t old_ref = (uint32_t) pop_i32(vm, tc);
        if (new_cap < 0) return BPVM_ERR_RUNTIME;
        uint32_t old_len = (old_ref == 0) ? 0 : bpvm_read_u32_be(vm->memory + old_ref);
        uint32_t new_ref = bpvm_heap_alloc(vm, (uint32_t) new_cap * 4, BPVM_TYPE_ARRAY_I32);
        if (new_ref == 0) return BPVM_ERR_OOM;
        bpvm_write_u32_be(vm->memory + new_ref, (uint32_t) new_cap);
        uint32_t copy = (old_len < (uint32_t) new_cap) ? old_len : (uint32_t) new_cap;
        for (uint32_t i = 0; i < copy; i++) {
            uint32_t v = bpvm_read_u32_be(vm->memory + old_ref + 4 + i * 4);
            bpvm_write_u32_be(vm->memory + new_ref + 4 + i * 4, v);
        }
        push_i32(vm, tc, (int32_t) new_ref);
        return BPVM_OK;
    }

    case BUILTIN_CHARS_TO_STRING: {
        int32_t len = pop_i32(vm, tc);
        uint32_t chars_ref = (uint32_t) pop_i32(vm, tc);
        if (len < 0) return BPVM_ERR_RUNTIME;
        uint32_t avail = (chars_ref == 0) ? 0 : bpvm_read_u32_be(vm->memory + chars_ref);
        if ((uint32_t) len > avail) return BPVM_ERR_RUNTIME;
        uint32_t new_ref = bpvm_heap_alloc(vm, (uint32_t) len * 4, BPVM_TYPE_ARRAY_I32);
        if (new_ref == 0) return BPVM_ERR_OOM;
        bpvm_write_u32_be(vm->memory + new_ref, (uint32_t) len);
        for (uint32_t i = 0; i < (uint32_t) len; i++) {
            uint32_t cp = bpvm_read_u32_be(vm->memory + chars_ref + 4 + i * 4);
            bpvm_write_u32_be(vm->memory + new_ref + 4 + i * 4, cp);
        }
        push_i32(vm, tc, (int32_t) new_ref);
        return BPVM_OK;
    }

    case BUILTIN_GC: {
        bpvm_heap_gc(vm);
        push_i32(vm, tc, 0);   /* void → push dummy */
        return BPVM_OK;
    }

    /* ---- F4: tiempo y threading ---- */
    case BUILTIN_NOW: {
        int64_t ms = bpvm_platform_now_ms();
        push_i32(vm, tc, (int32_t) ms);
        return BPVM_OK;
    }

    case BUILTIN_SLEEP: {
        int32_t ms = pop_i32(vm, tc);
        if (ms <= 0) { push_i32(vm, tc, 0); return BPVM_OK; }
        tc->wake_at_ms = bpvm_platform_now_ms() + ms;
        tc->status = BPVM_THREAD_BLOCKED_SLEEP;
        push_i32(vm, tc, 0);   /* void */
        return BPVM_OK;
    }

    case BUILTIN_SLEEP_SEC: {
        /* Misma semántica que SLEEP pero la entrada está en segundos. */
        int32_t s = pop_i32(vm, tc);
        if (s <= 0) { push_i32(vm, tc, 0); return BPVM_OK; }
        int64_t ms = (int64_t) s * 1000LL;
        tc->wake_at_ms = bpvm_platform_now_ms() + ms;
        tc->status = BPVM_THREAD_BLOCKED_SLEEP;
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    case BUILTIN_SLEEP_US: {
        /* Busy-wait que NO cede el thread BP. En Pico usa busy_wait_us
         * del SDK (timer HW, precisión µs); en host usa clock_gettime
         * con spin loop. */
        int32_t us = pop_i32(vm, tc);
        bpvm_platform_busy_wait_us(us);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    case BUILTIN_YIELD: {
        /* Marcar RUNNABLE — el scheduler verá el cambio al consumir
         * el quantum o al detectar el yield_requested al volver. F4 v1
         * con quantum-based scheduler: el yield se manifiesta como un
         * "saltar el resto del quantum". Forzamos terminar el quantum
         * dejando el flag — implementación simple: setear RUNNABLE
         * para que el wrapper del scheduler decida. */
        tc->status = BPVM_THREAD_RUNNABLE;
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    case BUILTIN_THREAD_START: {
        uint32_t thread_ref = (uint32_t) pop_i32(vm, tc);
        int new_tid = bpvm_thread_spawn(vm, thread_ref);
        if (new_tid < 0) {
            fprintf(stderr, "[bpvm-c] Thread.start falló\n");
            push_i32(vm, tc, 0);
            return BPVM_ERR_RUNTIME;
        }
        push_i32(vm, tc, 0);   /* void */
        return BPVM_OK;
    }

    case BUILTIN_THREAD_JOIN: {
        uint32_t thread_ref = (uint32_t) pop_i32(vm, tc);
        if (thread_ref == 0) {
            push_i32(vm, tc, 0); return BPVM_OK;
        }
        /* Convención: field[0] del Thread BP guarda el tid (escrito
         * por __threadStart). 0 = no spawneado todavía. */
        int32_t target_tid = bpvm_read_i32_be(vm->memory + thread_ref + 4 + 0 * 4);
        if (target_tid <= 0 || target_tid >= vm->thread_count) {
            push_i32(vm, tc, 0); return BPVM_OK;
        }
        if (vm->threads[target_tid].status == BPVM_THREAD_TERMINATED) {
            push_i32(vm, tc, 0); return BPVM_OK;
        }
        tc->blocked_on_join = target_tid;
        tc->status = BPVM_THREAD_BLOCKED_JOIN;
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    case BUILTIN_MUTEX_CREATE: {
        int mid = bpvm_mutex_alloc(vm);
        push_i32(vm, tc, (int32_t) mid);
        return BPVM_OK;
    }

    case BUILTIN_MUTEX_LOCK: {
        /* Recibe el ref del objeto Mutex BP. El mid (id en el pool de
         * la VM) está en field[0] del objeto, escrito por el ctor de
         * Mutex que llama a __mutexCreate. */
        uint32_t mref = (uint32_t) pop_i32(vm, tc);
        if (mref == 0) {
            push_i32(vm, tc, 0); return BPVM_ERR_NULL_RECEIVER;
        }
        int32_t mid = bpvm_read_i32_be(vm->memory + mref + 4 + 0 * 4);
        if (mid < 0 || mid >= vm->mutex_count) {
            fprintf(stderr, "[bpvm-c] mutex_lock: mid inválido %d\n", mid);
            push_i32(vm, tc, 0);
            return BPVM_ERR_RUNTIME;
        }
        bpvm_bp_mutex_t* m = &vm->mutexes[mid];
        if (m->owner_tid == tc->id) {
            fprintf(stderr, "[bpvm-c] mutex_lock: re-entrada tid=%d (no soportado)\n",
                    tc->id);
            push_i32(vm, tc, 0);
            return BPVM_ERR_RUNTIME;
        }
        if (m->owner_tid < 0) {
            m->owner_tid = tc->id;
            push_i32(vm, tc, 0);
            return BPVM_OK;
        }
        /* Contended: bloquea. */
        bpvm_mutex_add_waiter(vm, mid, tc->id);
        tc->blocked_on_mutex = mid;
        tc->status = BPVM_THREAD_BLOCKED_MUTEX;
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    case BUILTIN_MUTEX_UNLOCK: {
        uint32_t mref = (uint32_t) pop_i32(vm, tc);
        if (mref == 0) {
            push_i32(vm, tc, 0); return BPVM_ERR_NULL_RECEIVER;
        }
        int32_t mid = bpvm_read_i32_be(vm->memory + mref + 4 + 0 * 4);
        if (mid < 0 || mid >= vm->mutex_count) {
            fprintf(stderr, "[bpvm-c] mutex_unlock: mid inválido %d\n", mid);
            push_i32(vm, tc, 0);
            return BPVM_ERR_RUNTIME;
        }
        bpvm_bp_mutex_t* m = &vm->mutexes[mid];
        if (m->owner_tid != tc->id) {
            fprintf(stderr, "[bpvm-c] mutex_unlock: tid=%d intenta soltar mutex owned por %d\n",
                    tc->id, m->owner_tid);
            push_i32(vm, tc, 0);
            return BPVM_ERR_RUNTIME;
        }
        /* Si hay waiters, traspasamos la propiedad al primero. */
        int next = bpvm_mutex_pop_waiter(vm, mid);
        if (next >= 0) {
            m->owner_tid = next;
            /* Lo despertamos. */
            if (vm->threads[next].status == BPVM_THREAD_BLOCKED_MUTEX) {
                vm->threads[next].status = BPVM_THREAD_RUNNABLE;
                vm->threads[next].blocked_on_mutex = -1;
            }
        } else {
            m->owner_tid = -1;
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    /* ---- Gpio — todos los handlers delegan al backend de plataforma.
       En host: stubs con logging. En Pico: tabla rellenada por main.c. ---- */
    case BUILTIN_GPIO_INIT: {
        int mode = pop_i32(vm, tc);
        int pin  = pop_i32(vm, tc);
        bpvm_gpio_init(pin, mode);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GPIO_PULL: {
        int pull = pop_i32(vm, tc);
        int pin  = pop_i32(vm, tc);
        bpvm_gpio_pull(pin, pull);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GPIO_WRITE: {
        int val = pop_i32(vm, tc);
        int pin = pop_i32(vm, tc);
        bpvm_gpio_write(pin, val);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GPIO_READ: {
        int pin = pop_i32(vm, tc);
        push_i32(vm, tc, bpvm_gpio_read(pin));
        return BPVM_OK;
    }

    /* ---- I2C ----
       Layout del array TYPE_ARRAY_I32 en memory:
         [0..3] length (i32)
         [4..]  elementos (i32 cada uno)
       Cada elemento transporta un byte en el byte bajo. */
    case BUILTIN_I2C_INIT: {
        int baud = pop_i32(vm, tc);
        int scl  = pop_i32(vm, tc);
        int sda  = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        bpvm_i2c_init(bus, sda, scl, baud);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_I2C_WRITE: {
        /* write(bus, addr, data: integer[], count): integer */
        int count = pop_i32(vm, tc);
        uint32_t dataRef = (uint32_t) pop_i32(vm, tc);
        int addr = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        uint8_t buf[64];
        int n = count > (int) sizeof(buf) ? (int) sizeof(buf) : count;
        for (int i = 0; i < n; i++) {
            int32_t v = bpvm_read_i32_be(vm->memory + dataRef + 4 + i * 4);
            buf[i] = (uint8_t)(v & 0xFF);
        }
        int wrote = bpvm_i2c_write(bus, addr, buf, (size_t) n);
        push_i32(vm, tc, (int32_t) wrote);
        return BPVM_OK;
    }
    case BUILTIN_I2C_READ: {
        /* read(bus, addr, data: integer[], count): integer */
        int count = pop_i32(vm, tc);
        uint32_t dataRef = (uint32_t) pop_i32(vm, tc);
        int addr = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        uint8_t buf[64];
        int n = count > (int) sizeof(buf) ? (int) sizeof(buf) : count;
        int got = bpvm_i2c_read(bus, addr, buf, (size_t) n);
        if (got > 0) {
            for (int i = 0; i < got; i++) {
                bpvm_write_i32_be(vm->memory + dataRef + 4 + i * 4,
                                  (int32_t) buf[i]);
            }
        }
        push_i32(vm, tc, (int32_t) got);
        return BPVM_OK;
    }

    case BUILTIN_MOVE: {
        /* move(src, dst, srcStart, dstStart, count): void
         * Copia `count` slots de src[srcStart..] a dst[dstStart..].
         * Para integer[]/string (TYPE_ARRAY_I32) cada slot = 4 bytes.
         * Soporta overlapping (usa memmove). */
        int32_t count    = pop_i32(vm, tc);
        int32_t dstStart = pop_i32(vm, tc);
        int32_t srcStart = pop_i32(vm, tc);
        uint32_t dstRef  = (uint32_t) pop_i32(vm, tc);
        uint32_t srcRef  = (uint32_t) pop_i32(vm, tc);
        if (count <= 0 || srcRef == 0 || dstRef == 0) {
            push_i32(vm, tc, 0);
            return BPVM_OK;
        }
        /* Tamaño de slot por tipo. Para v1 asumimos I32 (caso común
         * con integer[] y string). Para int8/int16 habría que mirar
         * el tag — F3 lo añade. */
        uint32_t slot = 4;
        memmove(vm->memory + dstRef + 4 + (uint32_t) dstStart * slot,
                vm->memory + srcRef + 4 + (uint32_t) srcStart * slot,
                (size_t) count * slot);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    case BUILTIN_NEW_INT_ARRAY: {
        int32_t size = pop_i32(vm, tc);
        if (size < 0) return BPVM_ERR_RUNTIME;
        uint32_t bytes = (uint32_t) size * 4u;
        uint32_t ref = bpvm_heap_alloc(vm, bytes, BPVM_TYPE_ARRAY_I32);
        if (ref == 0) return BPVM_ERR_OOM;
        bpvm_write_u32_be(vm->memory + ref, (uint32_t) size);
        /* bpvm_heap_alloc ya zero-init (memset en heap.c). */
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }

    /* ---- SPI ----  igual patrón que I2C, arrays de bytes empaquetados. */
    case BUILTIN_SPI_INIT: {
        int mode = pop_i32(vm, tc);
        int baud = pop_i32(vm, tc);
        int miso = pop_i32(vm, tc);
        int mosi = pop_i32(vm, tc);
        int sck  = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        bpvm_spi_init(bus, sck, mosi, miso, baud, mode);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_SPI_WRITE: {
        int count = pop_i32(vm, tc);
        uint32_t dataRef = (uint32_t) pop_i32(vm, tc);
        int bus = pop_i32(vm, tc);
        uint8_t buf[256];
        int n = count > (int) sizeof(buf) ? (int) sizeof(buf) : count;
        for (int i = 0; i < n; i++) {
            int32_t v = bpvm_read_i32_be(vm->memory + dataRef + 4 + i * 4);
            buf[i] = (uint8_t)(v & 0xFF);
        }
        int wrote = bpvm_spi_write(bus, buf, (size_t) n);
        push_i32(vm, tc, (int32_t) wrote);
        return BPVM_OK;
    }
    case BUILTIN_SPI_READ: {
        int count = pop_i32(vm, tc);
        uint32_t dataRef = (uint32_t) pop_i32(vm, tc);
        int bus = pop_i32(vm, tc);
        uint8_t buf[256];
        int n = count > (int) sizeof(buf) ? (int) sizeof(buf) : count;
        int got = bpvm_spi_read(bus, buf, (size_t) n);
        if (got > 0) {
            for (int i = 0; i < got; i++) {
                bpvm_write_i32_be(vm->memory + dataRef + 4 + i * 4, (int32_t) buf[i]);
            }
        }
        push_i32(vm, tc, (int32_t) got);
        return BPVM_OK;
    }
    case BUILTIN_SPI_TRANSFER: {
        int count = pop_i32(vm, tc);
        uint32_t rxRef = (uint32_t) pop_i32(vm, tc);
        uint32_t txRef = (uint32_t) pop_i32(vm, tc);
        int bus = pop_i32(vm, tc);
        uint8_t txBuf[256], rxBuf[256];
        int n = count > (int) sizeof(txBuf) ? (int) sizeof(txBuf) : count;
        for (int i = 0; i < n; i++) {
            int32_t v = bpvm_read_i32_be(vm->memory + txRef + 4 + i * 4);
            txBuf[i] = (uint8_t)(v & 0xFF);
        }
        int xchg = bpvm_spi_transfer(bus, txBuf, rxBuf, (size_t) n);
        if (xchg > 0) {
            for (int i = 0; i < xchg; i++) {
                bpvm_write_i32_be(vm->memory + rxRef + 4 + i * 4, (int32_t) rxBuf[i]);
            }
        }
        push_i32(vm, tc, (int32_t) xchg);
        return BPVM_OK;
    }

    /* ---- UART ---- mismo layout de buffers que I2C/SPI (TYPE_ARRAY_I32,
       byte por slot). 7 argumentos en init para soportar paridad/bits. */
    case BUILTIN_UART_INIT: {
        int parity    = pop_i32(vm, tc);
        int stop_bits = pop_i32(vm, tc);
        int data_bits = pop_i32(vm, tc);
        int baud      = pop_i32(vm, tc);
        int rx        = pop_i32(vm, tc);
        int tx        = pop_i32(vm, tc);
        int bus       = pop_i32(vm, tc);
        bpvm_uart_init(bus, tx, rx, baud, data_bits, stop_bits, parity);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_UART_WRITE: {
        int count = pop_i32(vm, tc);
        uint32_t dataRef = (uint32_t) pop_i32(vm, tc);
        int bus = pop_i32(vm, tc);
        uint8_t buf[256];
        int n = count > (int) sizeof(buf) ? (int) sizeof(buf) : count;
        for (int i = 0; i < n; i++) {
            int32_t v = bpvm_read_i32_be(vm->memory + dataRef + 4 + i * 4);
            buf[i] = (uint8_t)(v & 0xFF);
        }
        int wrote = bpvm_uart_write(bus, buf, (size_t) n);
        push_i32(vm, tc, (int32_t) wrote);
        return BPVM_OK;
    }
    case BUILTIN_UART_READ: {
        int timeout = pop_i32(vm, tc);
        int count   = pop_i32(vm, tc);
        uint32_t dataRef = (uint32_t) pop_i32(vm, tc);
        int bus = pop_i32(vm, tc);
        uint8_t buf[256];
        int n = count > (int) sizeof(buf) ? (int) sizeof(buf) : count;
        int got = bpvm_uart_read(bus, buf, (size_t) n, timeout);
        if (got > 0) {
            for (int i = 0; i < got; i++) {
                bpvm_write_i32_be(vm->memory + dataRef + 4 + i * 4, (int32_t) buf[i]);
            }
        }
        push_i32(vm, tc, (int32_t) got);
        return BPVM_OK;
    }
    case BUILTIN_UART_AVAILABLE: {
        int bus = pop_i32(vm, tc);
        push_i32(vm, tc, (int32_t) bpvm_uart_available(bus));
        return BPVM_OK;
    }

    /* ---- Pulse counter ----
       init devuelve counterId (>=0) o -1 si pin inválido.
       Los demás reciben counterId y operan sobre él. */
    case BUILTIN_PULSE_INIT: {
        int edgeKind = pop_i32(vm, tc);
        int pin      = pop_i32(vm, tc);
        push_i32(vm, tc, (int32_t) bpvm_pulse_init(pin, edgeKind));
        return BPVM_OK;
    }
    case BUILTIN_PULSE_START: {
        int counterId = pop_i32(vm, tc);
        bpvm_pulse_start(counterId);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_PULSE_STOP: {
        int counterId = pop_i32(vm, tc);
        bpvm_pulse_stop(counterId);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_PULSE_VALUE: {
        int counterId = pop_i32(vm, tc);
        push_i32(vm, tc, (int32_t) bpvm_pulse_value(counterId));
        return BPVM_OK;
    }
    case BUILTIN_PULSE_RESET: {
        int counterId = pop_i32(vm, tc);
        bpvm_pulse_reset(counterId);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    /* ---- PWM ----
       init devuelve sliceId (>=0) o -1 si pin inválido. setDuty
       recibe (sliceId, pin, dutyPct) — el pin para distinguir
       canal A o B del mismo slice. */
    case BUILTIN_PWM_INIT: {
        int freqHz = pop_i32(vm, tc);
        int pin    = pop_i32(vm, tc);
        push_i32(vm, tc, (int32_t) bpvm_pwm_init(pin, freqHz));
        return BPVM_OK;
    }
    case BUILTIN_PWM_SET_FREQ: {
        int freqHz  = pop_i32(vm, tc);
        int sliceId = pop_i32(vm, tc);
        bpvm_pwm_set_freq(sliceId, freqHz);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_PWM_SET_DUTY: {
        int dutyPct = pop_i32(vm, tc);
        int pin     = pop_i32(vm, tc);
        int sliceId = pop_i32(vm, tc);
        bpvm_pwm_set_duty(sliceId, pin, dutyPct);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_PWM_START: {
        int sliceId = pop_i32(vm, tc);
        bpvm_pwm_start(sliceId);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_PWM_STOP: {
        int sliceId = pop_i32(vm, tc);
        bpvm_pwm_stop(sliceId);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    /* ---- Pico (info del MCU) ----
       uniqueId y boardName devuelven string (alocado en heap BP);
       tempC devuelve float (bit-cast a u32 para pushar al stack);
       cpuFreqHz y uptimeMs devuelven int. */
    case BUILTIN_PICO_UNIQUE_ID: {
        char buf[32];
        bpvm_pico_unique_id(buf, sizeof(buf));
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, strlen(buf));
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }
    case BUILTIN_PICO_BOARD_NAME: {
        char buf[32];
        bpvm_pico_board_name(buf, sizeof(buf));
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, strlen(buf));
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }
    case BUILTIN_PICO_TEMP_C: {
        float v = bpvm_pico_temp_c();
        /* Float en pila es bit-cast a u32 — la VM hace el mismo
         * truco para LITERAL_F32. */
        uint32_t bits;
        memcpy(&bits, &v, sizeof(bits));
        push_i32(vm, tc, (int32_t) bits);
        return BPVM_OK;
    }
    case BUILTIN_PICO_CPU_FREQ_HZ: {
        push_i32(vm, tc, (int32_t) bpvm_pico_cpu_freq_hz());
        return BPVM_OK;
    }
    case BUILTIN_PICO_UPTIME_MS: {
        push_i32(vm, tc, (int32_t) bpvm_pico_uptime_ms());
        return BPVM_OK;
    }
    case BUILTIN_PICO_SET_CPU_FREQ_MHZ: {
        /* Cambia el clk_sys del RP2350. En host es un no-op (stub
         * loguea por stdout). El clamp al máximo soportado lo hace BP
         * en Pico.bp (usando la constante MAX_CPU_MHZ) — aquí solo
         * delegamos al backend. */
        int32_t mhz = pop_i32(vm, tc);
        int ok = bpvm_pico_set_cpu_freq_mhz((int) mhz);
        push_i32(vm, tc, ok ? 1 : 0);
        return BPVM_OK;
    }
    case BUILTIN_RTC_NOW_SEC: {
        /* Wall clock en segundos. Si nadie ha llamado a setNowSec
         * desde el boot, devuelve segundos desde boot — todavía
         * útil como reloj monotónico. */
        int64_t now_ms = bpvm_rtc_now_ms();
        push_i32(vm, tc, (int32_t) (now_ms / 1000));
        return BPVM_OK;
    }
    case BUILTIN_RTC_SET_NOW_SEC: {
        int32_t sec = pop_i32(vm, tc);
        bpvm_rtc_set_now_ms((int64_t) sec * 1000LL);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_ADC_INIT_CHANNEL: {
        int32_t ch = pop_i32(vm, tc);
        push_i32(vm, tc, bpvm_adc_init_channel((int) ch));
        return BPVM_OK;
    }
    case BUILTIN_ADC_READ_CHANNEL: {
        int32_t ch = pop_i32(vm, tc);
        push_i32(vm, tc, bpvm_adc_read_channel((int) ch));
        return BPVM_OK;
    }
    case BUILTIN_WDT_ENABLE: {
        int32_t ms = pop_i32(vm, tc);
        bpvm_wdt_enable((int) ms);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_WDT_FEED: {
        bpvm_wdt_feed();
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_WDT_DISABLE: {
        bpvm_wdt_disable();
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    default:
        fprintf(stderr, "[bpvm-c] builtin id=%d no implementado (F2 subset)\n", id);
        return BPVM_ERR_BAD_OPCODE;
    }
}
