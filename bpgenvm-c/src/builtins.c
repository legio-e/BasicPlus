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
#include <inttypes.h>
#include <string.h>
#include <math.h>       /* H7 — NAN para eval() */
#include <stdlib.h>

#include "bpvm_gpio.h"
#include "bpvm_net.h"    /* H11 (#241) — cliente TCP */
#include "bpvm_i2c.h"
#include "bpvm_spi.h"
#include "bpvm_pulse.h"
#include "bpvm_pwm.h"
#include "bpvm_pico.h"
#include "bpvm_neopixel.h"
#include "bpvm_rtc.h"
#include "bpvm_adc.h"
#include "bpvm_wdt.h"
#include "bpvm_uart.h"
#include "bpvm_fs.h"
#ifdef BPVM_GUI
#include "bpvm_gui.h"   /* V3 / H4 — backend GUI (modelo). Sólo en el build con GUI. */
#endif

/* IDs estables (= ordinal del enum Builtin Java). Sólo los que F2
 * implementa; los demás devuelven BAD_OPCODE para que el caller sepa
 * que falta. */
enum {
    BUILTIN_STRLEN          = 0,
    BUILTIN_PARSE_INT       = 1,
    BUILTIN_PARSE_FLOAT     = 2,   /* string -> float (f32); strtod, parity-safe */
    BUILTIN_INT_TO_STRING   = 3,
    BUILTIN_BOOL_TO_STRING  = 5,
    BUILTIN_SUBSTRING       = 9,   /* #173: substring(s, start, end) */
    BUILTIN_CHAR_AT         = 14,
    /* Numéricas enteras (V2/GAP-1) — byte-exactas, sin riesgo de paridad float. */
    BUILTIN_ABS             = 16,
    BUILTIN_MIN             = 17,
    BUILTIN_MAX             = 18,
    /* File I/O (V2/H10/#247) — paridad con enum Builtin Java (ids 38..41). */
    BUILTIN_READ_FILE       = 38,
    BUILTIN_WRITE_FILE      = 39,
    BUILTIN_APPEND_FILE     = 40,
    BUILTIN_FILE_EXISTS     = 41,
    /* #240 (logger) — gestión de ficheros via IO.bp. */
    BUILTIN_REMOVE_FILE     = 71,
    BUILTIN_RENAME          = 72,
    BUILTIN_FILE_SIZE       = 74,
    /* #240 (2ª pasada) — resto de IO.bp. */
    BUILTIN_MKDIR           = 69,
    BUILTIN_RMDIR           = 70,
    BUILTIN_COPY_FILE       = 73,
    BUILTIN_IS_DIRECTORY    = 75,
    BUILTIN_LAST_MODIFIED   = 76,
    /* L13 — concat string + float/long/double (formateo canónico GAP-4). */
    BUILTIN_FLOAT_TO_STRING  = 4,
    BUILTIN_LONG_TO_STRING   = 129,
    BUILTIN_DOUBLE_TO_STRING = 130,
    /* H11 (#241) — cliente TCP simple via fachada bpvm_net. */
    BUILTIN_TCP_CONNECT      = 131,
    BUILTIN_TCP_SEND         = 132,
    BUILTIN_TCP_RECV         = 133,
    BUILTIN_TCP_CLOSE        = 134,
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
    BUILTIN_WDT_DISABLE      = 118,
    /* H2 (V2) — conversión string <-> byte[] (ambos TYPE_ARRAY_I8). */
    BUILTIN_TO_BYTES         = 119,
    BUILTIN_FROM_BYTES       = 120,
    /* 121/122 = HEAP_FRAG/HEAP_MAP: diagnósticos SÓLO de la VM-Java; la VM C
     * no los implementa. Hueco intencional para mantener el id alineado al
     * ordinal del enum Builtin.java. */
    /* H7.3 — board-aware (RP2350A/B): GPIO de la variante desde board_desc. */
    BUILTIN_PICO_GPIO_COUNT  = 123,
    /* H7.4 — NeoPixel WS2812 (device-only; no-op en host). */
    BUILTIN_NEOPIXEL_INIT    = 124,
    BUILTIN_NEOPIXEL_SHOW    = 125,
    /* H10/#247 — file I/O binario (byte[]). Mismo cuerpo que READ_FILE/WRITE_FILE
     * (heap TYPE_ARRAY_I8, bytes crudos sin pasar por string); el frontend los
     * tipa byte[]. Ids al final, alineados con el enum Builtin.java (126/127). */
    BUILTIN_READ_FILE_BYTES  = 126,
    BUILTIN_WRITE_FILE_BYTES = 127,
    BUILTIN_THROW_RTE        = 128,  /* #248: lanza RuntimeError nativo (msg) */
    /* V3 / H4 — GUI (modelo de comportamiento + dumpTree). Ids 135..152 =
     * ordinal del enum Builtin Java. Sólo se despachan en el build con BPVM_GUI;
     * sin él caen al default (= micro sin-GUI). */
    BUILTIN_GUI_SCREEN_ACTIVE = 135,
    BUILTIN_GUI_CREATE_OBJ    = 136,
    BUILTIN_GUI_CREATE_LABEL  = 137,
    BUILTIN_GUI_CREATE_BUTTON = 138,
    BUILTIN_GUI_SET_TEXT      = 139,
    BUILTIN_GUI_SET_WIDTH     = 140,
    BUILTIN_GUI_SET_HEIGHT    = 141,
    BUILTIN_GUI_ALIGN         = 142,
    BUILTIN_GUI_SET_BG_COLOR  = 143,
    BUILTIN_GUI_SET_TEXT_COLOR = 144,
    BUILTIN_GUI_SET_FONT      = 145,
    BUILTIN_GUI_CLEAN         = 146,
    BUILTIN_GUI_DELETE        = 147,
    BUILTIN_GUI_SCREEN_LOAD   = 148,
    BUILTIN_GUI_RUN           = 149,
    BUILTIN_GUI_DUMP_TREE     = 150,
    BUILTIN_GUI_BIND_CLICK    = 151,
    BUILTIN_GUI_CLICK         = 152,
    /* H6 — geometría (backend=verdad) + scroll (opt-in) + refresh. Ids = ordinal
     * del enum Builtin Java (idénticos en ambas VMs; aditivo). */
    BUILTIN_GUI_SET_X          = 153,
    BUILTIN_GUI_GET_X          = 154,
    BUILTIN_GUI_SET_Y          = 155,
    BUILTIN_GUI_GET_Y          = 156,
    BUILTIN_GUI_GET_WIDTH      = 157,
    BUILTIN_GUI_GET_HEIGHT     = 158,
    BUILTIN_GUI_SET_SCROLL_DIR = 159,
    BUILTIN_GUI_GET_SCROLL_DIR = 160,
    BUILTIN_GUI_REFRESH        = 161,
    /* H6 — checkbox (1er value-widget): create + checked + el inyector __guiChange
     * que cierra el camino onChange (kind=CHANGE). Ids = ordinal del enum Java. */
    BUILTIN_GUI_CREATE_CHECKBOX = 162,
    BUILTIN_GUI_SET_CHECKED     = 163,
    BUILTIN_GUI_GET_CHECKED     = 164,
    BUILTIN_GUI_CHANGE          = 165,
    /* H6 widgets — switch + slider + bar (value-widgets enteros). */
    BUILTIN_GUI_CREATE_SWITCH   = 166,
    BUILTIN_GUI_CREATE_SLIDER   = 167,
    BUILTIN_GUI_CREATE_BAR      = 168,
    BUILTIN_GUI_SET_VALUE       = 169,
    BUILTIN_GUI_GET_VALUE       = 170,
    BUILTIN_GUI_SET_RANGE       = 171,
    /* H6 widgets — spinbox (entero+rango) + led (indicador on/off). */
    BUILTIN_GUI_CREATE_SPINBOX  = 172,
    BUILTIN_GUI_CREATE_LED      = 173,
    /* H6 widgets — dropdown (opciones + índice) + textarea (texto editable). */
    BUILTIN_GUI_CREATE_DROPDOWN = 174,
    BUILTIN_GUI_SET_OPTIONS     = 175,
    BUILTIN_GUI_CREATE_TEXTAREA = 176,
    BUILTIN_GUI_GET_TEXT        = 177,
    /* H6 widgets — list (ítems + índice) + keyboard. */
    BUILTIN_GUI_CREATE_LIST     = 178,
    BUILTIN_GUI_CREATE_KEYBOARD = 179,
    BUILTIN_GUI_KEYBOARD_SET_TEXTAREA = 180,
    /* H6 widgets — msgbox (aviso async: mensaje + botones). */
    BUILTIN_GUI_CREATE_MSGBOX   = 181,
    BUILTIN_GUI_SET_BUTTONS     = 182,
    /* H6 widgets — tabview (pestañas; addTab devuelve el handle de la página). */
    BUILTIN_GUI_CREATE_TABVIEW  = 183,
    BUILTIN_GUI_TABVIEW_ADD_TAB = 184,
    /* H6 widgets — table (rejilla de celdas filas×columnas). */
    BUILTIN_GUI_CREATE_TABLE    = 185,
    BUILTIN_GUI_TABLE_SET_GRID  = 186,
    BUILTIN_GUI_TABLE_SET_CELL  = 187,
    BUILTIN_GUI_TABLE_GET_CELL  = 188,
    /* H6 widgets — image (asset separado del control que lo muestra). */
    BUILTIN_GUI_IMAGE_NEW        = 189,
    BUILTIN_GUI_IMAGE_LOAD_FILE  = 190,
    BUILTIN_GUI_IMAGE_WIDTH      = 191,
    BUILTIN_GUI_IMAGE_HEIGHT     = 192,
    BUILTIN_GUI_CREATE_IMAGEVIEW = 193,
    BUILTIN_GUI_IMAGEVIEW_SET_IMAGE = 194,
    BUILTIN_GUI_IMAGEVIEW_REFRESH = 195,
    /* H6 — fuente: tamaño de texto por componente (catálogo). */
    BUILTIN_GUI_SET_FONT_SIZE = 196,
    BUILTIN_GUI_GET_FONT_SIZE = 197,
    /* H6 — textarea read-only (sin cursor, no editable). */
    BUILTIN_GUI_TEXTAREA_SET_READONLY = 198,
    BUILTIN_GUI_TEXTAREA_GET_READONLY = 199,
    /* H7 — eval("expr"): calculadora de constantes (id 200). */
    BUILTIN_EVAL = 200,
    /* H10 — Pico.resetCause(): causa del último reset como string (id 201). */
    BUILTIN_PICO_RESET_CAUSE = 201,
    /* H10 — breadcrumb en RAM retenida: migas que sobreviven al reset (202-205). */
    BUILTIN_PICO_SET_MARK   = 202,   /* setMark(code): deja una miga */
    BUILTIN_PICO_MARK_COUNT = 203,   /* markCount(): nº de migas del trail previo */
    BUILTIN_PICO_MARK_AT    = 204,   /* markAt(i): i-ésima (0=origen pegajoso) */
    BUILTIN_PICO_BOOT_COUNT = 205,   /* bootCount(): arranques desde power-on */
    /* H13 (V3) — Forms: call-by-name del handler (host, name, sender) → void. */
    BUILTIN_GUI_INVOKE_BY_NAME = 206,
    /* H13.1 (V3) — Forms Camino A: dispatch del handler por SLOT de vtable
     * (win, slot, sender) → void. El handler es un MÉTODO de la ventana cuyo slot
     * horneó el IDE en el .win (resuelto vía .bpi/slotOf). */
    BUILTIN_GUI_INVOKE_BY_SLOT = 207
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
    /* H2 (V2): strings son byte[] UTF-8 → copiamos los bytes tal cual. */
    if (ref == 0) { if (dst_size) dst[0] = '\0'; return 0; }
    uint32_t nbytes = bpvm_read_u32_be(vm->memory + ref);
    size_t out = 0;
    for (uint32_t i = 0; i < nbytes && out + 1 < dst_size; i++) {
        dst[out++] = (char) vm->memory[ref + 4 + i];
    }
    if (out < dst_size) dst[out] = '\0';
    return out;
}

/* Helpers UTF-8 (utf8_cp_count / utf8_byte_offset / utf8_decode / utf8_encode)
 * viven en bpvm_internal.h (fuente única compartida con el AOT). */

/* BUG-7b — Lanza un RuntimeError BP atrapable desde un builtin nativo. Como el
 * dispatcher de OP_CALL_BUILTIN re-sincroniza pc/sp/bp/cs desde `tc` al volver:
 *   - ATRAPADO: eh_unwind dejó tc->pc en el handler (+empujó el ref), thread
 *     sigue RUNNING ⇒ devolvemos BPVM_OK y el intérprete continúa en el catch.
 *   - NO atrapado: eh_unwind dejó el thread TERMINATED ⇒ devolvemos
 *     BPVM_ERR_RUNTIME y el dispatcher termina el quantum con error.
 * tc->cs ya está fijado por el dispatcher (lo necesita throw_runtime_error). */
static bpvm_status_t builtin_throw(bpvm_t* vm, bpvm_thread_t* tc, const char* msg) {
    uint32_t ref = bpvm_throw_runtime_error(vm, tc, msg);
    return (ref && bpvm_eh_unwind(vm, tc, ref)) ? BPVM_OK : BPVM_ERR_RUNTIME;
}

/* H13 (V3) — Forms: resuelve una función PÚBLICA por nombre SIMPLE en el módulo
 * dueño del objeto `host` (su class_ptr vive en el data block de ese módulo).
 * Espejo de invokeHandlerByName de miVM (getCSForDataAddr + resolveExportInModule).
 * Devuelve la dirección absoluta o 0 si no existe. */
static uint32_t bpvm_resolve_handler(bpvm_t* vm, uint32_t host, const char* simple) {
    if (host == 0) return 0;
    uint32_t class_ptr = (uint32_t) bpvm_read_i32_be(vm->memory + host);
    const bpvm_module_t* m = NULL;
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* mm = &vm->modules[i];
        if (class_ptr >= mm->data_start && class_ptr < mm->data_start + mm->data_size) {
            m = mm; break;
        }
    }
    if (m == NULL) return 0;
    char qual[200];
    if (m->library[0] != '\0')
        snprintf(qual, sizeof(qual), "%s.%s.%s", m->library, m->name, simple);
    else
        snprintf(qual, sizeof(qual), "%s.%s", m->name, simple);
    for (int i = 0; i < vm->symbol_count; i++)
        if (strcmp(vm->symbols[i].name, qual) == 0) return vm->symbols[i].abs_addr;
    if (m->library[0] != '\0') {   /* fallback: clave corta name.simple */
        snprintf(qual, sizeof(qual), "%s.%s", m->name, simple);
        for (int i = 0; i < vm->symbol_count; i++)
            if (strcmp(vm->symbols[i].name, qual) == 0) return vm->symbols[i].abs_addr;
    }
    return 0;
}

/* H7 — calculadora de constantes para eval(). Descenso recursivo que evalúa
 * SOBRE LA MARCHA (sin AST): + - * / paréntesis y unario sobre literales. Réplica
 * byte-a-byte del EvalCalc de miVM (VirtualMachine.java): mismas operaciones double
 * + parseo numérico MANUAL (no strtod) para garantizar paridad. Error -> NaN. */
typedef struct { const char* s; int pos; int len; int err; } evalcalc_t;
static double ec_expr(evalcalc_t* c);
static void ec_ws(evalcalc_t* c) {
    while (c->pos < c->len && (c->s[c->pos] == ' ' || c->s[c->pos] == '\t')) c->pos++;
}
static double ec_number(evalcalc_t* c) {
    ec_ws(c);
    double v = 0; int any = 0;
    while (c->pos < c->len && c->s[c->pos] >= '0' && c->s[c->pos] <= '9') { v = v * 10 + (c->s[c->pos] - '0'); c->pos++; any = 1; }
    if (c->pos < c->len && c->s[c->pos] == '.') {
        c->pos++; double sc = 1;
        while (c->pos < c->len && c->s[c->pos] >= '0' && c->s[c->pos] <= '9') { v = v * 10 + (c->s[c->pos] - '0'); sc *= 10; c->pos++; any = 1; }
        v = v / sc;
    }
    if (!any) c->err = 1;
    return v;
}
static double ec_factor(evalcalc_t* c) {
    ec_ws(c);
    if (c->pos >= c->len) { c->err = 1; return 0; }
    char ch = c->s[c->pos];
    if (ch == '-') { c->pos++; return -ec_factor(c); }
    if (ch == '+') { c->pos++; return ec_factor(c); }
    if (ch == '(') {
        c->pos++; double v = ec_expr(c); ec_ws(c);
        if (c->pos < c->len && c->s[c->pos] == ')') c->pos++; else c->err = 1;
        return v;
    }
    return ec_number(c);
}
static double ec_term(evalcalc_t* c) {
    double v = ec_factor(c); ec_ws(c);
    while (c->pos < c->len && (c->s[c->pos] == '*' || c->s[c->pos] == '/')) {
        char op = c->s[c->pos++]; double r = ec_factor(c); v = (op == '*') ? v * r : v / r; ec_ws(c);
    }
    return v;
}
static double ec_expr(evalcalc_t* c) {
    double v = ec_term(c); ec_ws(c);
    while (c->pos < c->len && (c->s[c->pos] == '+' || c->s[c->pos] == '-')) {
        char op = c->s[c->pos++]; double r = ec_term(c); v = (op == '+') ? v + r : v - r; ec_ws(c);
    }
    return v;
}
static double bpvm_eval_calc(const char* s, int len) {
    evalcalc_t c; c.s = s; c.pos = 0; c.len = len; c.err = 0;
    double v = ec_expr(&c); ec_ws(&c);
    if (c.pos != c.len) c.err = 1;
    return c.err ? NAN : v;
}

bpvm_status_t bpvm_call_builtin(bpvm_t* vm, bpvm_thread_t* tc, int id) {
    switch (id) {

    case BUILTIN_STRLEN: {
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        uint32_t nbytes = (ref == 0) ? 0 : bpvm_read_u32_be(vm->memory + ref);
        uint32_t ncp = (ref == 0) ? 0 : utf8_cp_count(vm->memory + ref + 4, nbytes);
        push_i32(vm, tc, (int32_t) ncp);   /* H2: longitud en codepoints */
        return BPVM_OK;
    }

    case BUILTIN_EVAL: {   /* H7 — eval("expr") -> float (calc. de constantes) */
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        char buf[256];
        size_t n = read_bp_string(vm, ref, buf, sizeof(buf));
        double v = bpvm_eval_calc(buf, (int) n);
        union { float f; int32_t i; } u; u.f = (float) v;
        push_i32(vm, tc, u.i);
        return BPVM_OK;
    }

    /* GAP-1 — Numéricas enteras (ids 16..18). Byte-exactas con la VM-Java:
     * sólo aritmética int32, sin libm, así que la paridad es por construcción
     * (a diferencia de sqrt/sin/cos, donde Java Math vs C libm podrían diferir
     * en el último ULP y romper el stdout byte-idéntico). */
    case BUILTIN_ABS: {
        int32_t x = pop_i32(vm, tc);
        /* Math.abs(INT_MIN) = INT_MIN (overflow). Vía unsigned para evitar el
         * UB de negar INT_MIN y casar exactamente con la VM-Java. */
        int32_t r = (x < 0) ? (int32_t) (0u - (uint32_t) x) : x;
        push_i32(vm, tc, r);
        return BPVM_OK;
    }
    case BUILTIN_MIN: {
        int32_t b = pop_i32(vm, tc);   /* pop b primero, luego a (orden VM-Java) */
        int32_t a = pop_i32(vm, tc);
        push_i32(vm, tc, (a < b) ? a : b);
        return BPVM_OK;
    }
    case BUILTIN_MAX: {
        int32_t b = pop_i32(vm, tc);
        int32_t a = pop_i32(vm, tc);
        push_i32(vm, tc, (a > b) ? a : b);
        return BPVM_OK;
    }

    case BUILTIN_INT_TO_STRING: {
        int32_t v = pop_i32(vm, tc);
        char buf[32];
        int n = snprintf(buf, sizeof(buf), "%" PRId32, v);
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, (size_t)(n > 0 ? n : 0));
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }

    /* L13 — coerciones a string del concat. float/double usan el formateo
     * canónico GAP-4 (bpvm_format_double, el mismo de FPRINT/DPRINT) →
     * byte-idéntico a la VM-Java, y `"" + x` == `print x` siempre. */
    case BUILTIN_FLOAT_TO_STRING: {
        int32_t bits = pop_i32(vm, tc);
        float x;
        memcpy(&x, &bits, 4);
        char buf[64];
        int n = bpvm_format_double(buf, (double) x);
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, (size_t)(n > 0 ? n : 0));
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }
    case BUILTIN_LONG_TO_STRING: {
        uint32_t lo = (uint32_t) pop_i32(vm, tc);
        uint32_t hi = (uint32_t) pop_i32(vm, tc);
        int64_t v = (int64_t) (((uint64_t) hi << 32) | lo);
        char buf[32];
        int n = snprintf(buf, sizeof(buf), "%" PRId64, v);
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, (size_t)(n > 0 ? n : 0));
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }
    case BUILTIN_DOUBLE_TO_STRING: {
        uint32_t lo = (uint32_t) pop_i32(vm, tc);
        uint32_t hi = (uint32_t) pop_i32(vm, tc);
        uint64_t bits = ((uint64_t) hi << 32) | lo;
        double v;
        memcpy(&v, &bits, 8);
        char buf[64];
        int n = bpvm_format_double(buf, v);
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

    /* H11 (#241) — cliente TCP simple. Puente sobre la fachada bpvm_net
     * (backend sockets del SO en host; NULL en firmwares hasta H11.b/c).
     * Sin backend → RuntimeError ATRAPABLE (paridad con la VM-Java, que
     * siempre tiene java.net). El fallo de CONEXIÓN es un resultado
     * normal (handle 0 → boolean false en Net.Tcp.connect); los errores
     * de send/recv sobre una conexión establecida sí son RuntimeError. */
    case BUILTIN_TCP_CONNECT: {
        int32_t timeout_ms = pop_i32(vm, tc);
        int32_t port       = pop_i32(vm, tc);
        uint32_t href      = (uint32_t) pop_i32(vm, tc);
        if (!bpvm_net_available()) {
            return builtin_throw(vm, tc, "Net: sin red en esta plataforma");
        }
        char host[256];
        read_bp_string(vm, href, host, sizeof(host));
        int h = bpvm_net_connect(host, (int) port, (int) timeout_ms);
        push_i32(vm, tc, (int32_t) h);          /* 0 = no conectado */
        return BPVM_OK;
    }
    case BUILTIN_TCP_SEND: {
        uint32_t dref = (uint32_t) pop_i32(vm, tc);   /* data: byte[] */
        int32_t  h    = pop_i32(vm, tc);
        if (!bpvm_net_available()) {
            return builtin_throw(vm, tc, "Net: sin red en esta plataforma");
        }
        uint32_t len = (dref == 0) ? 0 : bpvm_read_u32_be(vm->memory + dref);
        const uint8_t* data = (dref == 0) ? NULL : (vm->memory + dref + 4);
        int n = (len == 0) ? 0 : bpvm_net_send((int) h, data, (int) len);
        if (n < 0) return builtin_throw(vm, tc, "Net.send: conexión cerrada o inválida");
        push_i32(vm, tc, (int32_t) n);
        return BPVM_OK;
    }
    case BUILTIN_TCP_RECV: {
        int32_t timeout_ms = pop_i32(vm, tc);
        int32_t max        = pop_i32(vm, tc);
        int32_t h          = pop_i32(vm, tc);
        if (!bpvm_net_available()) {
            return builtin_throw(vm, tc, "Net: sin red en esta plataforma");
        }
        if (max < 0) max = 0;
        if (max > 65536) max = 65536;            /* tope sano por llamada */
        uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) max, BPVM_TYPE_ARRAY_I8);
        if (ref == 0) return builtin_throw(vm, tc, "Net.recv: sin memoria");
        int n = (max == 0) ? 0
              : bpvm_net_recv((int) h, vm->memory + ref + 4, (int) max,
                               (int) timeout_ms);
        if (n == BPVM_NET_CLOSED) {
            return builtin_throw(vm, tc, "Net.recv: conexión cerrada por el peer");
        }
        if (n < 0) return builtin_throw(vm, tc, "Net.recv: error de red");
        /* n==0 (timeout) → byte[] vacío. La longitud del array puede ser
         * menor que el payload alocado (mismo patrón que READ_FILE). */
        bpvm_write_u32_be(vm->memory + ref, (uint32_t) n);
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }
    case BUILTIN_TCP_CLOSE: {
        int32_t h = pop_i32(vm, tc);
        bpvm_net_close((int) h);                 /* tolerante, sin backend = no-op */
        return BPVM_OK;
    }

#ifdef BPVM_GUI
    /* ---- V3 / H4.1 — GUI (modelo de comportamiento; paridad por dumpTree).
     * Sólo en el build con GUI. Convención (confirmada por disasm): todo builtin
     * GUI deja UN valor en pila — void → 0 (el emisor lo guarda), valor → result.
     * Orden de pop: último arg en top (igual que el resto). Color/fuente son
     * render-only → no afectan al dump (no-op aquí; LVGL los honrará en H4.2). */
    case BUILTIN_GUI_SCREEN_ACTIVE: { push_i32(vm, tc, bpvm_gui_screen_active()); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_OBJ:    { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_obj(p));    return BPVM_OK; }
    case BUILTIN_GUI_CREATE_LABEL:  { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_label(p));  return BPVM_OK; }
    case BUILTIN_GUI_CREATE_BUTTON: { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_button(p)); return BPVM_OK; }
    case BUILTIN_GUI_SET_TEXT: {
        uint32_t tref = (uint32_t) pop_i32(vm, tc);
        int handle = pop_i32(vm, tc);
        char buf[1024];
        read_bp_string(vm, tref, buf, sizeof(buf));
        bpvm_gui_set_text(handle, buf);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_SET_WIDTH:  { int w = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_width(h, w);  push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_SET_HEIGHT: { int v = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_height(h, v); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_ALIGN: {
        int dy = pop_i32(vm, tc); int dx = pop_i32(vm, tc); int a = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_align(h, a, dx, dy); push_i32(vm, tc, 0); return BPVM_OK;
    }
    /* Color/fuente: render-only. En modelo-only son no-op (no tocan el dump);
     * bajo BPVM_LVGL aplican estilo al lv_obj. Se enrutan a gui.c en ambos casos. */
    case BUILTIN_GUI_SET_BG_COLOR:   { uint32_t rgb = (uint32_t) pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_bg_color(h, rgb);   push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_SET_TEXT_COLOR: { uint32_t rgb = (uint32_t) pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_text_color(h, rgb); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_SET_FONT:       { int f = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_font(h, f); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_CLEAN:       { int h = pop_i32(vm, tc); bpvm_gui_clean(h);  push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_DELETE:      { int h = pop_i32(vm, tc); bpvm_gui_delete(h); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_SCREEN_LOAD: { pop_i32(vm, tc); push_i32(vm, tc, 0); return BPVM_OK; }   /* una sola pantalla por ahora */
    case BUILTIN_GUI_RUN: {
        /* Lazo de eventos. Resuelve por NOMBRE los dos dispatchers BP:
         * Gui.__guiDispatch (click→onClick) y Gui.__guiDispatchChange (change→
         * onChange). Cada evento drenado lleva su `kind` y elige el dispatcher
         * (simétrico a invokeGuiDispatch de miVM). Con LVGL: bombea la ventana SDL
         * hasta cerrarla, drenando eventos reales + sintéticos. Sin LVGL
         * (modelo-only/arnés): drena los inyectados y vuelve. */
        uint32_t disp_click = 0, disp_change = 0;
        for (int i = 0; i < vm->symbol_count; i++) {
            if      (strcmp(vm->symbols[i].name, "Gui.__guiDispatch")       == 0) disp_click  = vm->symbols[i].abs_addr;
            else if (strcmp(vm->symbols[i].name, "Gui.__guiDispatchChange") == 0) disp_change = vm->symbols[i].abs_addr;
        }
#ifdef BPVM_LVGL
        for (;;) {
            uint32_t objptr; int kind;
            while ((objptr = bpvm_gui_next_event(&kind)) != 0) {
                uint32_t d = (kind == 1) ? disp_change : disp_click;
                if (d) { int32_t a = (int32_t) objptr; bpvm_call_bp_from_builtin(vm, tc, d, &a, 1); }
            }
            /* P-run-stop (#257) — KILL durante Gui.run(): el scheduler no corre
             * quanta mientras este builtin bombea, así que poleamos el wire aquí
             * mismo (el MISMO poll_cb que el scheduler usa entre quanta). Al romper
             * caemos al push+return de abajo → el quantum termina → el scheduler ve
             * kill_requested y devuelve BPVM_KILLED (parada limpia entre opcodes). */
            if (vm->poll_cb != NULL && vm->poll_cb(vm, vm->poll_user) != 0)
                vm->kill_requested = 1;
            if (vm->kill_requested) break;
            if (!bpvm_gui_lvgl_window_open()) break;
            bpvm_gui_lvgl_pump();
        }
#else
        /* Modelo-only (headless): drena los eventos inyectados y vuelve (paridad). */
        uint32_t objptr; int kind;
        while ((objptr = bpvm_gui_next_event(&kind)) != 0) {
            uint32_t d = (kind == 1) ? disp_change : disp_click;
            if (d) { int32_t a = (int32_t) objptr; bpvm_call_bp_from_builtin(vm, tc, d, &a, 1); }
        }
#endif
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_DUMP_TREE: {
        char* buf = NULL;
        size_t n = bpvm_gui_dump_tree(&buf);
        uint32_t ref = bpvm_heap_alloc_string(vm, buf ? buf : "", n);
        free(buf);
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }
    case BUILTIN_GUI_INVOKE_BY_NAME: {
        /* H13 — Forms: resuelve `name` como función pública del módulo de `host`
         * (la ventana) y la invoca con `sender` como arg0. Args: host, name, sender.
         * Espejo de GUI_INVOKE_BY_NAME en miVM. */
        uint32_t sender   = (uint32_t) pop_i32(vm, tc);
        uint32_t name_ref = (uint32_t) pop_i32(vm, tc);
        uint32_t host     = (uint32_t) pop_i32(vm, tc);
        char nm[128];
        read_bp_string(vm, name_ref, nm, sizeof(nm));
        uint32_t target = bpvm_resolve_handler(vm, host, nm);
        if (target == 0) {
            /* H13 (decisión de Eduardo): handler no implementado → IGNORAR (sin
             * excepción). El aviso se da una vez al cargar el form (Gui.Window). */
            push_i32(vm, tc, 0);
            return BPVM_OK;
        }
        int32_t a = (int32_t) sender;
        bpvm_call_bp_from_builtin(vm, tc, target, &a, 1);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_INVOKE_BY_SLOT: {
        /* H13.1 — Forms Camino A: invoca el MÉTODO de la ventana en vtable[slot]
         * (slot horneado por el IDE). Args: win, slot, sender. Espejo de
         * invokeHandlerBySlot en miVM: paseo de vtable con fallback al padre
         * (idéntico a OP_INVOKE_VIRTUAL) → bridge con [this=win, sender] como
         * locals 0/1. slot < 0 o no resoluble → IGNORA (sin excepción). */
        uint32_t sender = (uint32_t) pop_i32(vm, tc);
        int32_t  slot   = pop_i32(vm, tc);
        uint32_t win    = (uint32_t) pop_i32(vm, tc);
        if (win == 0 || slot < 0) { push_i32(vm, tc, 0); return BPVM_OK; }
        uint8_t* mem = vm->memory;
        uint32_t desc = (uint32_t) bpvm_read_i32_be(mem + win);
        int32_t  method_off = -1;
        uint32_t target_cs  = 0;
        for (;;) {
            uint16_t bw    = bpvm_read_u16_be(mem + desc + BPVM_CLS_OFF_BITMAP_WORDS);
            uint16_t nmeth = bpvm_read_u16_be(mem + desc + BPVM_CLS_OFF_NUM_METHODS);
            uint32_t vt_base = desc + BPVM_CLS_OFF_FIELD_BITMAP + 2u * (uint32_t) bw * 4u;
            if ((uint32_t) slot < nmeth) {
                int32_t off = bpvm_read_i32_be(mem + vt_base + (uint32_t) slot * 4);
                if (off != -1) { method_off = off; target_cs = bpvm_get_cs_for_data_addr(vm, desc); break; }
            }
            int32_t parent_off = bpvm_read_i32_be(mem + desc + BPVM_CLS_OFF_PARENT_OFF);
            if (parent_off == 0) { push_i32(vm, tc, 0); return BPVM_OK; }  /* no resoluble → ignora */
            uint32_t cur_cs = bpvm_get_cs_for_data_addr(vm, desc);
            desc = (uint32_t) ((int32_t) cur_cs + parent_off);
        }
        uint32_t target_abs = target_cs + (uint32_t) method_off;
        int32_t a2[2]; a2[0] = (int32_t) win; a2[1] = (int32_t) sender;
        bpvm_call_bp_from_builtin(vm, tc, target_abs, a2, 2);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_BIND_CLICK: {
        uint32_t self = (uint32_t) pop_i32(vm, tc);
        int handle = pop_i32(vm, tc);
        bpvm_gui_bind_click(handle, self);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_CLICK: {
        uint32_t obj = (uint32_t) pop_i32(vm, tc);
        bpvm_gui_inject_click(obj);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    /* H6 — geometría (backend = verdad) + scroll (opt-in) + refresh. */
    case BUILTIN_GUI_SET_X:  { int v = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_x(h, v); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_GET_X:  { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_x(h)); return BPVM_OK; }
    case BUILTIN_GUI_SET_Y:  { int v = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_y(h, v); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_GET_Y:  { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_y(h)); return BPVM_OK; }
    case BUILTIN_GUI_GET_WIDTH:  { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_width(h));  return BPVM_OK; }
    case BUILTIN_GUI_GET_HEIGHT: { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_height(h)); return BPVM_OK; }
    case BUILTIN_GUI_SET_SCROLL_DIR: { int d = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_scroll_dir(h, d); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_GET_SCROLL_DIR: { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_scroll_dir(h)); return BPVM_OK; }
    case BUILTIN_GUI_REFRESH: { int h = pop_i32(vm, tc); bpvm_gui_refresh(h); push_i32(vm, tc, 0); return BPVM_OK; }
    /* H6 — checkbox (1er value-widget). set_checked es programático (no emite
     * onChange); __guiChange inyecta un CHANGE sintético (= toggle del usuario). */
    case BUILTIN_GUI_CREATE_CHECKBOX: { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_checkbox(p)); return BPVM_OK; }
    case BUILTIN_GUI_SET_CHECKED:     { int v = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_checked(h, v); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_GET_CHECKED:     { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_checked(h)); return BPVM_OK; }
    case BUILTIN_GUI_CHANGE:          { uint32_t obj = (uint32_t) pop_i32(vm, tc); bpvm_gui_inject_change(obj); push_i32(vm, tc, 0); return BPVM_OK; }
    /* H6 — switch + slider + bar (value-widgets enteros; el backend clampa al rango). */
    case BUILTIN_GUI_CREATE_SWITCH: { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_switch(p)); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_SLIDER: { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_slider(p)); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_BAR:    { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_bar(p)); return BPVM_OK; }
    case BUILTIN_GUI_SET_VALUE:     { int v = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_value(h, v); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_GET_VALUE:     { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_value(h)); return BPVM_OK; }
    case BUILTIN_GUI_SET_RANGE:     { int mx = pop_i32(vm, tc); int mn = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_range(h, mn, mx); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_SPINBOX: { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_spinbox(p)); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_LED:     { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_led(p)); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_DROPDOWN: { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_dropdown(p)); return BPVM_OK; }
    case BUILTIN_GUI_SET_OPTIONS: {
        uint32_t ref = (uint32_t) pop_i32(vm, tc); int h = pop_i32(vm, tc);
        char buf[512]; read_bp_string(vm, ref, buf, sizeof(buf));
        bpvm_gui_set_options(h, buf); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CREATE_TEXTAREA: { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_textarea(p)); return BPVM_OK; }
    case BUILTIN_GUI_GET_TEXT: {
        int h = pop_i32(vm, tc);
        const char* s = bpvm_gui_get_text(h);
        uint32_t ref = bpvm_heap_alloc_string(vm, s, strlen(s));
        push_i32(vm, tc, (int32_t) ref); return BPVM_OK;
    }
    case BUILTIN_GUI_CREATE_LIST:     { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_list(p)); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_KEYBOARD: { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_keyboard(p)); return BPVM_OK; }
    case BUILTIN_GUI_KEYBOARD_SET_TEXTAREA: { int ta = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_keyboard_set_textarea(h, ta); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_MSGBOX: { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_msgbox(p)); return BPVM_OK; }
    case BUILTIN_GUI_SET_BUTTONS: {
        uint32_t ref = (uint32_t) pop_i32(vm, tc); int h = pop_i32(vm, tc);
        char buf[256]; read_bp_string(vm, ref, buf, sizeof(buf));
        bpvm_gui_set_buttons(h, buf); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CREATE_TABVIEW: { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_tabview(p)); return BPVM_OK; }
    case BUILTIN_GUI_TABVIEW_ADD_TAB: {
        uint32_t ref = (uint32_t) pop_i32(vm, tc); int h = pop_i32(vm, tc);
        char buf[128]; read_bp_string(vm, ref, buf, sizeof(buf));
        push_i32(vm, tc, bpvm_gui_tabview_add_tab(h, buf)); return BPVM_OK;
    }
    case BUILTIN_GUI_CREATE_TABLE: { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_table(p)); return BPVM_OK; }
    case BUILTIN_GUI_TABLE_SET_GRID: {
        int cols = pop_i32(vm, tc); int rows = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_table_set_grid(h, rows, cols); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_TABLE_SET_CELL: {
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        int col = pop_i32(vm, tc); int row = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        char buf[256]; read_bp_string(vm, ref, buf, sizeof(buf));
        bpvm_gui_table_set_cell(h, row, col, buf); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_TABLE_GET_CELL: {
        int col = pop_i32(vm, tc); int row = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        const char* s = bpvm_gui_table_get_cell(h, row, col);
        uint32_t ref = bpvm_heap_alloc_string(vm, s, strlen(s));
        push_i32(vm, tc, (int32_t) ref); return BPVM_OK;
    }
    case BUILTIN_GUI_IMAGE_NEW: { push_i32(vm, tc, bpvm_gui_image_new()); return BPVM_OK; }
    case BUILTIN_GUI_IMAGE_LOAD_FILE: {
        uint32_t ref = (uint32_t) pop_i32(vm, tc); int id = pop_i32(vm, tc);
        char buf[512]; read_bp_string(vm, ref, buf, sizeof(buf));
        push_i32(vm, tc, bpvm_gui_image_load_file(id, buf)); return BPVM_OK;
    }
    case BUILTIN_GUI_IMAGE_WIDTH:  { int id = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_image_width(id));  return BPVM_OK; }
    case BUILTIN_GUI_IMAGE_HEIGHT: { int id = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_image_height(id)); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_IMAGEVIEW: { int p = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_create_imageview(p)); return BPVM_OK; }
    case BUILTIN_GUI_IMAGEVIEW_SET_IMAGE: {
        int img = pop_i32(vm, tc); int view = pop_i32(vm, tc);
        bpvm_gui_imageview_set_image(view, img); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_IMAGEVIEW_REFRESH: {
        int view = pop_i32(vm, tc);
        bpvm_gui_imageview_refresh(view); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_SET_FONT_SIZE: {
        int px = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_set_font_size(h, px); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_GET_FONT_SIZE: {
        int h = pop_i32(vm, tc);
        push_i32(vm, tc, bpvm_gui_get_font_size(h)); return BPVM_OK;
    }
    case BUILTIN_GUI_TEXTAREA_SET_READONLY: {
        int ro = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_textarea_set_readonly(h, ro); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_TEXTAREA_GET_READONLY: {
        int h = pop_i32(vm, tc);
        push_i32(vm, tc, bpvm_gui_textarea_get_readonly(h)); return BPVM_OK;
    }
#endif /* BPVM_GUI */

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

    case BUILTIN_PARSE_FLOAT: {   /* string -> float (f32). Espejo de parseInt pero con
                                   * strtod: (float)strtod == (float)Double.parseDouble
                                   * (f32 correctamente redondeado) -> paridad con miVM. */
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        char buf[64];
        read_bp_string(vm, ref, buf, sizeof(buf));
        /* trim simple (igual que parseInt) */
        char* p = buf; while (*p == ' ' || *p == '\t') p++;
        char* end = p + strlen(p);
        while (end > p && (end[-1] == ' ' || end[-1] == '\n' || end[-1] == '\r' || end[-1] == '\t')) {
            *--end = '\0';
        }
        union { float f; int32_t i; } u;
        u.f = (float) strtod(p, NULL);
        push_i32(vm, tc, u.i);
        return BPVM_OK;
    }

    /* File I/O (H10/#247) — puente sobre la fachada bpvm_fs (backend libc en
     * host, fs_get/fs_put en device). Paridad con la VM-Java: lee/escribe los
     * bytes del string BP (UTF-8) tal cual; errores → RuntimeError atrapable.
     * El path se lee a un buffer C; el contenido se escribe/lee directo desde
     * el heap (cualquier tamaño). */
    case BUILTIN_READ_FILE:
    case BUILTIN_READ_FILE_BYTES: {   /* #247: idéntico — los bytes ya son crudos en heap */
        uint32_t pref = (uint32_t) pop_i32(vm, tc);
        char path[512];
        read_bp_string(vm, pref, path, sizeof(path));
        char alt[600];
        const char* rpath = path;        /* path efectivo tras el fallback /app */
        uint32_t size = 0;
        if (bpvm_fs_stat(rpath, &size) != 0) {
            /* H13.1 (B) — fallback /app para paths RELATIVOS (nombre simple): el IDE
             * sube los resources/ del proyecto a /app/, así readFile("main.win") los
             * encuentra (mismo criterio que el cargador de imágenes en gui.c). Sólo
             * si el path no es absoluto. (Mejora pendiente: carpeta por proyecto
             * /app/<proj>/ + dir-base relativo — ver V3_BACKLOG/PENDIENTES.) */
            int resolved = 0;
            if (path[0] != '/') {
                snprintf(alt, sizeof(alt), "/app/%s", path);
                if (bpvm_fs_stat(alt, &size) == 0) { rpath = alt; resolved = 1; }
            }
            if (!resolved) {
                char em[576];
                snprintf(em, sizeof(em), "readFile('%s'): no se pudo abrir", path);
                return builtin_throw(vm, tc, em);
            }
        }
        uint32_t ref = bpvm_heap_alloc(vm, size, BPVM_TYPE_ARRAY_I8);
        if (ref == 0) return builtin_throw(vm, tc, "readFile: sin memoria");
        bpvm_write_u32_be(vm->memory + ref, size);
        if (size > 0) {
            long n = bpvm_fs_read(rpath, vm->memory + ref + 4, size);
            if (n < 0 || (uint32_t) n != size) {
                char em[576];
                snprintf(em, sizeof(em), "readFile('%s'): error de lectura", path);
                return builtin_throw(vm, tc, em);
            }
        }
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }
    case BUILTIN_WRITE_FILE:
    case BUILTIN_WRITE_FILE_BYTES:    /* #247: idéntico — escribe los bytes crudos del array */
    case BUILTIN_APPEND_FILE: {
        int append = (id == BUILTIN_APPEND_FILE);
        uint32_t cref = (uint32_t) pop_i32(vm, tc);   /* content (empujado el último) */
        uint32_t pref = (uint32_t) pop_i32(vm, tc);   /* path */
        char path[512];
        read_bp_string(vm, pref, path, sizeof(path));
        uint32_t clen = (cref == 0) ? 0 : bpvm_read_u32_be(vm->memory + cref);
        const uint8_t* cdata = (cref == 0) ? NULL : (vm->memory + cref + 4);
        if (bpvm_fs_write(path, cdata, clen, append) != 0) {
            char em[576];
            snprintf(em, sizeof(em), "%s('%s'): error de escritura",
                     append ? "appendFile" : "writeFile", path);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);   /* void → dummy */
        return BPVM_OK;
    }
    case BUILTIN_FILE_EXISTS: {
        uint32_t pref = (uint32_t) pop_i32(vm, tc);
        char path[512];
        read_bp_string(vm, pref, path, sizeof(path));
        push_i32(vm, tc, bpvm_fs_exists(path) ? 1 : 0);
        return BPVM_OK;
    }

    /* #240 (logger) — gestión de ficheros (IO.removeFile/rename/fileSize).
     * Mismos contratos que la VM-Java; backend sin la op → RuntimeError
     * atrapable (los firmwares la añaden en su próximo build). */
    case BUILTIN_REMOVE_FILE: {
        uint32_t pref = (uint32_t) pop_i32(vm, tc);
        char path[512];
        read_bp_string(vm, pref, path, sizeof(path));
        if (bpvm_fs_remove(path) != 0) {
            char em[576];
            snprintf(em, sizeof(em), "removeFile('%s'): error al borrar", path);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_RENAME: {
        uint32_t tref = (uint32_t) pop_i32(vm, tc);   /* to (empujado el último) */
        uint32_t fref = (uint32_t) pop_i32(vm, tc);   /* from */
        char from[512], to[512];
        read_bp_string(vm, fref, from, sizeof(from));
        read_bp_string(vm, tref, to, sizeof(to));
        if (bpvm_fs_rename(from, to) != 0) {
            char em[1100];
            snprintf(em, sizeof(em), "rename('%s' -> '%s'): error al renombrar", from, to);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_FILE_SIZE: {
        uint32_t pref = (uint32_t) pop_i32(vm, tc);
        char path[512];
        read_bp_string(vm, pref, path, sizeof(path));
        uint32_t sz = 0;
        if (bpvm_fs_stat(path, &sz) != 0) {
            char em[576];
            snprintf(em, sizeof(em), "fileSize('%s'): no se pudo leer el tamaño", path);
            return builtin_throw(vm, tc, em);
        }
        if (sz > 0x7FFFFFFFu) {
            char em[576];
            snprintf(em, sizeof(em), "fileSize('%s'): tamaño > 2GB no representable en integer", path);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, (int32_t) sz);
        return BPVM_OK;
    }

    /* #240 (2ª pasada) — resto de IO.bp. En device los backends pueden no
     * implementarlos (FS plano sin directorios) → RuntimeError atrapable. */
    case BUILTIN_MKDIR: {
        uint32_t pref = (uint32_t) pop_i32(vm, tc);
        char path[512];
        read_bp_string(vm, pref, path, sizeof(path));
        if (bpvm_fs_mkdir(path) != 0) {
            char em[576];
            snprintf(em, sizeof(em), "mkdir('%s'): no se pudo crear", path);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_RMDIR: {
        uint32_t pref = (uint32_t) pop_i32(vm, tc);
        char path[512];
        read_bp_string(vm, pref, path, sizeof(path));
        if (bpvm_fs_rmdir(path) != 0) {
            char em[576];
            snprintf(em, sizeof(em), "rmdir('%s'): no se pudo borrar (¿no vacío?)", path);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_COPY_FILE: {
        uint32_t tref = (uint32_t) pop_i32(vm, tc);   /* to (empujado el último) */
        uint32_t fref = (uint32_t) pop_i32(vm, tc);   /* from */
        char from[512], to[512];
        read_bp_string(vm, fref, from, sizeof(from));
        read_bp_string(vm, tref, to, sizeof(to));
        if (bpvm_fs_copy(from, to) != 0) {
            char em[1100];
            snprintf(em, sizeof(em), "copyFile('%s' -> '%s'): error al copiar", from, to);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_IS_DIRECTORY: {
        uint32_t pref = (uint32_t) pop_i32(vm, tc);
        char path[512];
        read_bp_string(vm, pref, path, sizeof(path));
        push_i32(vm, tc, bpvm_fs_isdir(path) ? 1 : 0);   /* sin throw, como Java */
        return BPVM_OK;
    }
    case BUILTIN_LAST_MODIFIED: {
        uint32_t pref = (uint32_t) pop_i32(vm, tc);
        char path[512];
        read_bp_string(vm, pref, path, sizeof(path));
        long long ms = bpvm_fs_mtime_ms(path);
        if (ms < 0) {
            char em[576];
            snprintf(em, sizeof(em), "lastModified('%s'): no se pudo leer", path);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, (int32_t) (ms & 0x7FFFFFFFLL));   /* truncado como Java */
        return BPVM_OK;
    }

    case BUILTIN_THROW_RTE: {
        /* #248 — lanza el RuntimeError NATIVO de la VM con el mensaje dado
         * (mismo path que div0/null deref → atrapable con try/catch BP).
         * Lo usa el compareTo por defecto de Object para no depender del
         * descriptor local de RuntimeError. No retorna valor. */
        uint32_t mref = (uint32_t) pop_i32(vm, tc);
        char msg[256];
        read_bp_string(vm, mref, msg, sizeof(msg));
        return builtin_throw(vm, tc, msg);
    }

    case BUILTIN_CHAR_CODE_AT: {
        int32_t i   = pop_i32(vm, tc);
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        if (ref == 0) { push_i32(vm, tc, 0); return BPVM_OK; }
        uint32_t nbytes = bpvm_read_u32_be(vm->memory + ref);
        const uint8_t* p = vm->memory + ref + 4;
        uint32_t ncp = utf8_cp_count(p, nbytes);
        if (i < 0 || (uint32_t) i >= ncp) { push_i32(vm, tc, 0); return BPVM_OK; }
        uint32_t off = utf8_byte_offset(p, nbytes, (uint32_t) i);
        uint32_t adv; uint32_t cp = utf8_decode(p + off, nbytes - off, &adv);
        push_i32(vm, tc, (int32_t) cp);    /* H2: codepoint en índice de codepoint */
        return BPVM_OK;
    }

    case BUILTIN_CHAR_AT: {
        /* charAt(str, idx): string  — devuelve un string de 1 char con
         * el codepoint en esa posición. Si idx fuera de rango, devuelve
         * string vacía. */
        int32_t i    = pop_i32(vm, tc);
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        uint8_t enc[4]; uint32_t enc_len = 0;
        if (ref != 0) {
            uint32_t nbytes = bpvm_read_u32_be(vm->memory + ref);
            const uint8_t* p = vm->memory + ref + 4;
            uint32_t ncp = utf8_cp_count(p, nbytes);
            if (i < 0 || (uint32_t) i >= ncp) {
                /* BUG-7b — idx fuera de rango → RuntimeError atrapable (paridad VM-Java). */
                char em[96];
                snprintf(em, sizeof em, "charAt: idx fuera de rango %" PRId32 " (len=%d)", i, (int) ncp);
                return builtin_throw(vm, tc, em);
            }
            uint32_t off = utf8_byte_offset(p, nbytes, (uint32_t) i);
            uint32_t adv; uint32_t cp = utf8_decode(p + off, nbytes - off, &adv);
            enc_len = utf8_encode(cp, enc);   /* re-codifica a UTF-8 (antes de alocar) */
        }
        /* Alocamos un string UTF-8 de 1 codepoint (enc_len bytes). */
        uint32_t out = bpvm_heap_alloc(vm, enc_len, BPVM_TYPE_ARRAY_I8);
        if (out) {
            bpvm_write_u32_be(vm->memory + out, enc_len);
            for (uint32_t k = 0; k < enc_len; k++) vm->memory[out + 4 + k] = enc[k];
        }
        push_i32(vm, tc, (int32_t) out);
        return BPVM_OK;
    }

    case BUILTIN_SUBSTRING: {
        /* substring(s, start, end): string — copia [start, end) con
         * clamp estilo BP. Idéntico a aot_helpers h_string_substring
         * para que intérprete y AOT den el mismo resultado (#173). */
        int32_t end   = pop_i32(vm, tc);
        int32_t start = pop_i32(vm, tc);
        uint32_t ref  = (uint32_t) pop_i32(vm, tc);
        uint32_t nbytes = (ref == 0) ? 0 : bpvm_read_u32_be(vm->memory + ref);
        const uint8_t* p = vm->memory + ref + 4;
        uint32_t ncp = (ref == 0) ? 0 : utf8_cp_count(p, nbytes);   /* H2: índices en codepoints */
        if (start < 0) start = 0;
        if (end   < 0) end = 0;
        if ((uint32_t) end > ncp) end = (int32_t) ncp;
        if (start > end) start = end;
        uint32_t boff = utf8_byte_offset(p, nbytes, (uint32_t) start);
        uint32_t eoff = utf8_byte_offset(p, nbytes, (uint32_t) end);
        uint32_t n = eoff - boff;                                   /* nº de bytes del rango */
        uint32_t out = bpvm_heap_alloc(vm, n, BPVM_TYPE_ARRAY_I8);
        if (out) {
            bpvm_write_u32_be(vm->memory + out, n);
            for (uint32_t i = 0; i < n; i++)
                vm->memory[out + 4 + i] = vm->memory[ref + 4 + boff + i];
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
        /* H2: input = array i32 de codepoints; output = string byte[] UTF-8. */
        uint32_t total = 0;
        for (uint32_t i = 0; i < (uint32_t) len; i++) {
            uint32_t cp = bpvm_read_u32_be(vm->memory + chars_ref + 4 + i * 4);
            uint8_t tmp[4]; total += utf8_encode(cp, tmp);
        }
        uint32_t new_ref = bpvm_heap_alloc(vm, total, BPVM_TYPE_ARRAY_I8);
        if (new_ref == 0) return BPVM_ERR_OOM;
        bpvm_write_u32_be(vm->memory + new_ref, total);
        uint32_t w = 0;
        for (uint32_t i = 0; i < (uint32_t) len; i++) {
            uint32_t cp = bpvm_read_u32_be(vm->memory + chars_ref + 4 + i * 4);
            uint8_t enc[4]; uint32_t el = utf8_encode(cp, enc);
            for (uint32_t k = 0; k < el; k++) vm->memory[new_ref + 4 + w++] = enc[k];
        }
        push_i32(vm, tc, (int32_t) new_ref);
        return BPVM_OK;
    }

    case BUILTIN_TO_BYTES:
    case BUILTIN_FROM_BYTES: {
        /* H2 (V2): string y byte[] comparten layout (TYPE_ARRAY_I8). La
         * conversión es una copia defensiva (string inmutable / byte[]
         * mutable): mismos bytes, objeto nuevo. */
        uint32_t ref = (uint32_t) pop_i32(vm, tc);
        uint32_t n = (ref == 0) ? 0 : bpvm_read_u32_be(vm->memory + ref);
        uint32_t out = bpvm_heap_alloc(vm, n, BPVM_TYPE_ARRAY_I8);
        if (out == 0) return BPVM_ERR_OOM;
        bpvm_write_u32_be(vm->memory + out, n);
        for (uint32_t i = 0; i < n; i++) vm->memory[out + 4 + i] = vm->memory[ref + 4 + i];
        push_i32(vm, tc, (int32_t) out);
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
            fprintf(stderr, "[bpvm-c] mutex_lock: mid inválido %" PRId32 "\n", mid);
            push_i32(vm, tc, 0);
            return BPVM_ERR_RUNTIME;
        }
        bpvm_bp_mutex_t* m = &vm->mutexes[mid];
        if (m->owner_tid == tc->id) {
            /* BUG-7b — reentrada → RuntimeError atrapable (paridad VM-Java). */
            char em[96];
            snprintf(em, sizeof em, "mutex.lock: re-entrada por mismo thread tid=%d (los Mutex no son reentrantes)",
                     (int) tc->id);
            return builtin_throw(vm, tc, em);
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
            fprintf(stderr, "[bpvm-c] mutex_unlock: mid inválido %" PRId32 "\n", mid);
            push_i32(vm, tc, 0);
            return BPVM_ERR_RUNTIME;
        }
        bpvm_bp_mutex_t* m = &vm->mutexes[mid];
        if (m->owner_tid != tc->id) {
            /* BUG-7b — unlock por no-propietario → RuntimeError atrapable (paridad VM-Java). */
            char em[96];
            snprintf(em, sizeof em, "mutex.unlock: thread %d no es propietario (owner=%d)",
                     (int) tc->id, (int) m->owner_tid);
            return builtin_throw(vm, tc, em);
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
    case BUILTIN_PICO_RESET_CAUSE: {   /* H10 — causa del último reset (string) */
        const char* s = bpvm_pico_reset_cause();
        uint32_t ref = bpvm_heap_alloc_string(vm, s, strlen(s));
        push_i32(vm, tc, (int32_t) ref);
        return BPVM_OK;
    }
    case BUILTIN_PICO_SET_MARK: {      /* H10 — breadcrumb: deja una miga */
        int32_t code = pop_i32(vm, tc);
        bpvm_pico_set_mark((int) code);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_PICO_MARK_COUNT: {    /* H10 — nº de migas del trail previo */
        push_i32(vm, tc, (int32_t) bpvm_pico_mark_count());
        return BPVM_OK;
    }
    case BUILTIN_PICO_MARK_AT: {       /* H10 — i-ésima miga (0 = origen) */
        int32_t i = pop_i32(vm, tc);
        push_i32(vm, tc, (int32_t) bpvm_pico_mark_at((int) i));
        return BPVM_OK;
    }
    case BUILTIN_PICO_BOOT_COUNT: {    /* H10 — arranques desde power-on */
        push_i32(vm, tc, (int32_t) bpvm_pico_boot_count());
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
    case BUILTIN_PICO_GPIO_COUNT: {
        push_i32(vm, tc, (int32_t) bpvm_pico_gpio_count());
        return BPVM_OK;
    }
    case BUILTIN_NEOPIXEL_INIT: {
        /* __npInit(pin): void */
        int pin = pop_i32(vm, tc);
        bpvm_neopixel_init(pin);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_NEOPIXEL_SHOW: {
        /* __npShow(pin, grb: integer[], count): void — empuja count palabras
         * GRB del array (mismo layout que I2C/SPI: ref+4 = primer i32 BE). */
        int count = pop_i32(vm, tc);
        uint32_t grbRef = (uint32_t) pop_i32(vm, tc);
        int pin = pop_i32(vm, tc);
        static uint32_t s_npbuf[256];           /* single-worker: estático OK */
        int n = count < 0 ? 0 : (count > 256 ? 256 : count);
        for (int i = 0; i < n; i++) {
            s_npbuf[i] = (uint32_t) bpvm_read_i32_be(vm->memory + grbRef + 4 + i * 4);
        }
        bpvm_neopixel_show(pin, s_npbuf, n);
        push_i32(vm, tc, 0);
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

    default: {
        /* GAP-1: builtin fuera del subconjunto de esta VM-C. En vez de abortar
         * la VM con BAD_OPCODE (crash duro, no diagnosticable desde BP), lanzamos
         * un RuntimeError BP *atrapable*: el programa puede capturarlo con
         * try/catch o terminar con un mensaje claro. Fallo limpio. */
        char msg[96];
        snprintf(msg, sizeof(msg),
                 "builtin %d no soportado en esta VM (subconjunto C)", id);
        return builtin_throw(vm, tc, msg);
    }
    }
}
