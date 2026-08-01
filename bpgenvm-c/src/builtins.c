/*
 * builtins.c — dispatch de CALL_BUILTIN para F2.
 *
 * IDs estables del enum Builtin de la VM Java (docs/BUILTINS.md).
 * F2 cubre el subset mínimo para programas que usan strings y arrays
 * comunes; el resto se va añadiendo según haga falta.
 */

#include "bpvm_internal.h"
#include "bpvm_alloc.h"   /* #339: reservas del nucleo con guardian */
#include "bpvm.h"       /* #338: la zona de rascar compartida */
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
    /* #348 — cadenas de cómputo puro. Estaban SÓLO en miVM: un programa que
     * las usara pasaba en el host y moría en la placa. Ids del enum Builtin. */
    BUILTIN_UPPER           = 6,
    BUILTIN_LOWER           = 7,
    BUILTIN_TRIM            = 8,
    BUILTIN_INDEX_OF        = 10,
    BUILTIN_STARTS_WITH     = 11,
    BUILTIN_ENDS_WITH       = 12,
    BUILTIN_CONTAINS        = 13,
    BUILTIN_REPLACE         = 15,
    BUILTIN_SPLIT           = 36,
    /* #348 tanda 2 — matemáticas de cómputo puro. Todas calculan en DOUBLE y
     * estrechan a float AL FINAL, igual que miVM (Math.* de Java toma double y
     * el resultado se castea a (float)). Usar las variantes 'f' de libm daría
     * otro último bit y rompería la paridad. */
    BUILTIN_SQRT            = 19,
    BUILTIN_POW             = 20,
    BUILTIN_LOG             = 21,
    BUILTIN_LOG10           = 22,
    BUILTIN_EXP             = 23,
    BUILTIN_SIN             = 24,
    BUILTIN_COS             = 25,
    BUILTIN_TAN             = 26,
    BUILTIN_PI              = 29,
    BUILTIN_E               = 30,
    BUILTIN_FLOOR           = 31,
    BUILTIN_CEIL            = 32,
    BUILTIN_ROUND           = 33,
    BUILTIN_SIGN_I          = 56,
    BUILTIN_SIGN_F          = 57,
    BUILTIN_ASIN            = 58,
    BUILTIN_ACOS            = 59,
    BUILTIN_ATAN            = 60,
    BUILTIN_ATAN2           = 61,
    BUILTIN_FACTORIAL_I     = 62,
    BUILTIN_GAMMA_F         = 63,
    /* #348 tanda 3 — rutas. SIEMPRE con '/' (decisión de Eduardo): el FS del
     * dispositivo es '/', y la semántica la definimos nosotros en vez de
     * heredar la de java.nio.file, que es dependiente de plataforma. */
    BUILTIN_PATH_JOIN       = 64,
    BUILTIN_PATH_PARENT     = 65,
    BUILTIN_PATH_BASENAME   = 66,
    BUILTIN_PATH_EXTENSION  = 67,
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
    BUILTIN_GROW_REF_ARRAY  = 45,   /* V4: faltaba (paridad con miVM) */
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
    BUILTIN_GUI_INVOKE_BY_SLOT = 207,
    /* H14 — counts de periféricos board-aware (del board_desc / board.json). */
    BUILTIN_PICO_ADC_CHANNELS  = 208,   /* () → canales ADC de la placa */
    BUILTIN_PICO_PWM_SLICES    = 209,   /* () → slices PWM de la placa */

    BUILTIN_GUI_LOAD_FONT      = 210,   /* (path: string) → id de fuente (1-based); setFont(id) la aplica */

    /* H19 — introspección del proyecto en ejecución (módulo App). */
    BUILTIN_APP_MAIN_MODULE      = 211, /* () → string: nombre del módulo principal ("Main") */
    BUILTIN_APP_MAIN_MODULE_PATH = 212, /* () → string: ruta completa del entry ("/app/<proj>/Main.mod") */
    BUILTIN_APP_PROJECT_PATH     = 213, /* () → string: carpeta del proyecto ("/app/<proj>"); "" si plano */

    /* Orientación del display en runtime (0/90/180/270 grados). */
    BUILTIN_GUI_SET_ROTATION     = 214, /* (deg: integer) → void */

    /* H7 — Chart. Los ids DEBEN coincidir con el ordinal del enum Builtin.java
     * (allí id = ordinal(), por eso las entradas nuevas van al final). El eje Y
     * y el repintado reusan los genéricos SET_RANGE / REFRESH. */
    BUILTIN_GUI_CREATE_CHART     = 215, /* (parent) → id */
    BUILTIN_GUI_CHART_SET_POINTS = 216, /* (h, n) → void */
    BUILTIN_GUI_CHART_ADD_SERIES = 217, /* (h, rgb) → índice de serie */
    BUILTIN_GUI_CHART_PUSH       = 218, /* (h, serie, v) → void */
    BUILTIN_GUI_CHART_SET_VALUE  = 219, /* (h, serie, idx, v) → void */
    BUILTIN_GUI_CHART_SET_TYPE   = 220, /* (h, tipo) → void: 0=línea, 1=barras */

    /* H5.c — `raise ev(args)`. Encola; NO llama al handler (lo inyecta el
     * scheduler entre quanta). Ver docs/V4_IDEAS.md §contrato de __eventRaise. */
    BUILTIN_EVENT_RAISE          = 221,

    /* #324 — UNA PASADA del bombeo del GUI, en vez del lazo entero.
     *   devuelve 1 = "queda trabajo, vuelve a llamarme"
     *            0 = "no queda nada, puedes salir"
     *
     * POR QUÉ SE PARTIÓ. __guiRun bloqueaba hasta cerrar la ventana, y mientras
     * bombeaba la VM ENTERA estaba parada: ni avanzaban los threads ni se drenaba
     * la cola de eventos. Con el lazo en BP (Gui.run) hay frontera de instrucción
     * entre pasadas, que es el ÚNICO sitio donde el scheduler puede inyectar el
     * frame de un handler — events.c lo exige: "El thread NO puede estar
     * corriendo". Medido en samples/GuiEvSpike.bp: sin esto, un evento levantado
     * desde el upcall del GUI no se drena JAMÁS.
     *
     * En headless devuelve 1 si esta pasada drenó algo: así el lazo da UNA VUELTA
     * MÁS y el scheduler tiene dónde correr los handlers antes de salir. */
    BUILTIN_GUI_RUN_ONCE         = 222,
    BUILTIN_GUI_SLOT_OF          = 223
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
/* H1.2a (V4): refs = 8 bytes (plano, low32 = dirección). */
/* V4: adaptadores finos sobre la abstracción bpref_* (bpvm_internal.h). Mantienen
 * la firma uint32_t para no tocar aún los ~99 callers; el ancho/codificación ya
 * pasa por BPVM_REF_SIZE + bpref_load/store. Migración capa a capa después. */
static uint32_t pop_ref(bpvm_t* vm, bpvm_thread_t* tc) {
    return bpref_addr(bpref_pop(vm, tc));
}
/* V4 (tanda 2): dirección FÍSICA del objeto tras un pop_ref — deref del handle
 * por la tabla (identidad para direcciones directas/constantes). Obligatorio
 * antes de tocar vm->memory: el valor de pop_ref es un handle (idx|TAG), no
 * una dirección. */
static uint32_t ref_addr(bpvm_t* vm, uint32_t ref) {
    return (ref == 0) ? 0 : bpref_deref(vm, bpref_from_addr(ref));
}
static void push_ref(bpvm_t* vm, bpvm_thread_t* tc, uint32_t ref) {
    /* V4: `ref` puede ser un handle tageado de 32b (idx|TAG) cuyo llamante PERDIÓ
     * la generación al truncar el bpref_t de 64b a uint32_t (p.ej. el retorno de
     * bpvm_heap_alloc_string). bpref_regen reconstruye la gen VIVA del slot (aquí
     * la correcta: el objeto se acaba de registrar) → el handle en pila lleva su
     * gen real. Mismo patrón, misma implementación que el guardado del msg en
     * exceptions.c (bpvm_internal.h). */
    bpref_push(vm, tc, bpref_regen(vm, ref));
}

/* #348 — lee un argumento ref SIN sacarlo de la pila (0 = el de más arriba).
 *
 * POR QUÉ EXISTE: el GC marca escaneando [stack_base, sp) — EXCLUYE lo que ya
 * se ha sacado. Y bpvm_heap_alloc PUEDE colectar (umbral de bump, y el
 * reintento tras OOM). Así que el patrón «pop del origen → alocar el resultado
 * → seguir leyendo del origen» deja la cadena origen SIN RAÍZ justo cuando
 * puede pasar el GC: si el llamante no la tenía además en un local, se la
 * lleva. Con handles no corrompe en silencio —la gen no cuadra y grita— pero
 * el programa falla igual.
 *
 * Con peek el argumento sigue por debajo de sp, o sea que sigue siendo raíz.
 * Se saca AL FINAL, justo antes de empujar el resultado. Todo builtin que
 * alogue y siga leyendo de un argumento tiene que usar esto. */
static bpref_t peek_ref(bpvm_t* vm, bpvm_thread_t* tc, int depth) {
    return bpref_load(vm, tc->sp - (uint32_t)(depth + 1) * BPVM_REF_SIZE);
}

/* #348 — mayúsculas/minúsculas LATIN-1 (decisión de Eduardo 31-jul).
 *
 * miVM usaba toUpperCase()/toLowerCase() de Java: Unicode completo y —peor—
 * DEPENDIENTE DEL LOCALE de la máquina (en un locale turco, 'i' sube a 'İ').
 * Reproducir eso en un micro pide tablas de kilobytes, así que se acordó
 * LATIN-1 en las DOS VMs: ASCII + el bloque U+00C0..U+00FF, que es lo que un
 * proyecto en español necesita, y sale algorítmico sin ninguna tabla.
 *
 * Propiedad que aprovecha el llamante: en UTF-8 la longitud en BYTES no cambia
 * (á U+00E1 y Á U+00C1 son 2 bytes los dos; ÿ U+00FF → Ÿ U+0178 también), así
 * que el resultado se puede alocar del tamaño exacto del origen.
 *
 * Lo que NO hace, a propósito: 'ß' (U+00DF) se queda como está — su mayúscula
 * Unicode es "SS", dos caracteres, y eso rompería la propiedad de la longitud.
 * Java sí lo convierte; es la divergencia que se aceptó al elegir Latin-1. */
static uint32_t latin1_upper_cp(uint32_t c) {
    if (c >= 'a' && c <= 'z')            return c - 32;
    if (c >= 0x00E0 && c <= 0x00FE && c != 0x00F7) return c - 32;  /* ÷ no es letra */
    if (c == 0x00FF)                     return 0x0178;            /* ÿ → Ÿ */
    return c;
}
static uint32_t latin1_lower_cp(uint32_t c) {
    if (c >= 'A' && c <= 'Z')            return c + 32;
    if (c >= 0x00C0 && c <= 0x00DE && c != 0x00D7) return c + 32;  /* × no es letra */
    if (c == 0x0178)                     return 0x00FF;            /* Ÿ → ÿ */
    return c;
}

/* #348 tanda 2 — bits de un float BP (f32) y vuelta. El stack de BP guarda los
 * floats como sus bits en un i32, igual que miVM (Float.intBitsToFloat). */
static float bits_to_f32(int32_t bits) { float f; memcpy(&f, &bits, 4); return f; }
static int32_t f32_to_bits(float f)    { int32_t b; memcpy(&b, &f, 4); return b; }

/* #348 tanda 2 — conversión double→int32 con la semántica de JAVA, que NO es la
 * de C. En C, convertir a int un double fuera de rango o NaN es COMPORTAMIENTO
 * INDEFINIDO (en x86 suele salir 0x80000000, pero no hay promesa y en ARM/RISC-V
 * puede ser otra cosa). Java lo define: NaN → 0, y saturación a MIN/MAX.
 * floor/ceil/round tienen que coincidir en las dos VMs TAMBIÉN en los extremos,
 * que es justo donde nadie mira. */
static int32_t java_d2i(double d) {
    if (d != d) return 0;                                  /* NaN */
    if (d >=  2147483647.0) return  2147483647;
    if (d <= -2147483648.0) return -2147483647 - 1;
    return (int32_t) d;                                    /* trunca hacia cero */
}

/* #348 tanda 2 — Lanczos g=7, n=9. RÉPLICA EXACTA de lanczosGamma de miVM:
 * mismos coeficientes, mismo orden de operaciones y misma reflexión. Cualquier
 * reordenación cambia el último bit y rompe la paridad. */
static double lanczos_gamma(double x) {
    static const double p[9] = {
        0.99999999999980993,
        676.5203681218851,
        -1259.1392167224028,
        771.32342877765313,
        -176.61502916214059,
        12.507343278686905,
        -0.13857109526572012,
        9.9843695780195716e-6,
        1.5056327351493116e-7
    };
    const double PI_D = 3.14159265358979323846;
    if (x < 0.5) return PI_D / (sin(PI_D * x) * lanczos_gamma(1.0 - x));
    x -= 1.0;
    double a = p[0];
    double t = x + 7.5;
    for (int i = 1; i < 9; i++) a += p[i] / (x + i);
    return sqrt(2.0 * PI_D) * pow(t, x + 0.5) * exp(-t) * a;
}

/* #348 — busca `q` (qn bytes) dentro de `p` (n bytes) desde el byte `from`.
 * Devuelve el OFFSET EN BYTES o -1. A nivel de byte y no de codepoint a
 * propósito: UTF-8 es auto-sincronizante, o sea que una secuencia válida no
 * puede casar a mitad de otra. El resultado es el mismo que buscar por
 * caracteres, y sin decodificar nada. Aguja vacía → casa en `from` (como Java). */
static long bp_find_bytes(const uint8_t* p, uint32_t n,
                          const uint8_t* q, uint32_t qn, uint32_t from) {
    if (qn == 0) return (long) (from <= n ? from : n);
    if (qn > n) return -1;
    for (uint32_t i = from; i + qn <= n; i++) {
        if (memcmp(p + i, q, qn) == 0) return (long) i;
    }
    return -1;
}

/* Lee un string BP (TYPE_ARRAY_I32 con codepoints) a un buffer C UTF-8.
 * Devuelve el número de bytes escritos (sin null terminator). Si el
 * codepoint > 127 lo escribe como '?' (F2 v1 sólo ASCII). */
static size_t read_bp_string(const bpvm_t* vm, uint32_t ref, char* dst, size_t dst_size) {
    /* H2 (V2): strings son byte[] UTF-8 → copiamos los bytes tal cual.
     * V4: la resolución ref→bytes pasa por la abstracción (bpref_deref/arr_len);
     * la firma sigue tomando uint32_t (boundary a migrar con el modelo handle). */
    if (ref == 0) { if (dst_size) dst[0] = '\0'; return 0; }
    bpref_t s = bpref_from_addr(ref);
    uint32_t nbytes = bpref_arr_len(vm, s);
    uint32_t base   = bpref_deref(vm, s) + BPVM_ARR_DATA_OFF;
    size_t out = 0;
    for (uint32_t i = 0; i < nbytes && out + 1 < dst_size; i++) {
        dst[out++] = (char) vm->memory[base + i];
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
    bpref_t ref = bpvm_throw_runtime_error(vm, tc, msg);
    return (!bpref_is_null(ref) && bpvm_eh_unwind(vm, tc, ref)) ? BPVM_OK : BPVM_ERR_RUNTIME;
}

#ifdef BPVM_GUI
/* V3 Forms — Crea un widget hijo: saca `parent` de la pila, valida que es un
 * contenedor vivo (si no, lanza RuntimeError "widget sin contenedor") y empuja el
 * handle del nuevo nodo. `mk` = la bpvm_gui_create_* del modelo. La regla y el
 * mensaje se replican en miVM (GuiBackend) -> paridad de comportamiento. */
static bpvm_status_t gui_make_child(bpvm_t* vm, bpvm_thread_t* tc, int (*mk)(int)) {
    int parent = pop_i32(vm, tc);
    if (!bpvm_gui_parent_alive(parent))
        return builtin_throw(vm, tc,
            "Gui: no se puede crear un widget sin un contenedor valido; crea Gui.Screen() o Gui.Window() primero");
    push_i32(vm, tc, mk(parent));
    return BPVM_OK;
}
#endif /* BPVM_GUI */

/* H13 (V3) — Forms: resuelve una función PÚBLICA por nombre SIMPLE en el módulo
 * dueño del objeto `host` (su class_ptr vive en el data block de ese módulo).
 * Espejo de invokeHandlerByName de miVM (getCSForDataAddr + resolveExportInModule).
 * Devuelve la dirección absoluta o 0 si no existe. */
static uint32_t bpvm_resolve_handler(bpvm_t* vm, uint32_t host, const char* simple) {
    if (host == 0) return 0;
    uint32_t ha = bpref_deref(vm, bpref_from_addr(host));   /* V4: handle→addr */
    uint32_t class_ptr = (uint32_t) bpvm_read_i32_be(vm->memory + ha);
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
        bpref_t s = bpref_pop(vm, tc);   /* string ref */
        uint32_t nbytes = bpref_arr_len(vm, s);
        uint32_t ncp = bpref_is_null(s) ? 0 : utf8_cp_count(bpref_arr_elem(vm, s, 0, 1), nbytes);
        push_i32(vm, tc, (int32_t) ncp);   /* H2: longitud en codepoints */
        return BPVM_OK;
    }

    case BUILTIN_EVAL: {   /* H7 — eval("expr") -> float (calc. de constantes) */
        uint32_t ref = pop_ref(vm, tc);   /* H1.2a: string ref = 8 bytes */
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
        push_ref(vm, tc, ref);
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
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }
    case BUILTIN_LONG_TO_STRING: {
        uint32_t lo = (uint32_t) pop_i32(vm, tc);
        uint32_t hi = (uint32_t) pop_i32(vm, tc);
        int64_t v = (int64_t) (((uint64_t) hi << 32) | lo);
        char buf[32];
        int n = bpvm_format_i64(buf, v);
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, (size_t)(n > 0 ? n : 0));
        push_ref(vm, tc, ref);
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
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }

    case BUILTIN_BOOL_TO_STRING: {
        int32_t v = pop_i32(vm, tc);
        const char* s = v ? "true" : "false";
        uint32_t ref = bpvm_heap_alloc_string(vm, s, strlen(s));
        push_ref(vm, tc, ref);
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
        uint32_t href      = pop_ref(vm, tc);
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
        uint32_t dref = pop_ref(vm, tc);   /* data: byte[] */
        int32_t  h    = pop_i32(vm, tc);
        if (!bpvm_net_available()) {
            return builtin_throw(vm, tc, "Net: sin red en esta plataforma");
        }
        uint32_t dd = (dref == 0) ? 0 : bpref_deref(vm, bpref_from_addr(dref));   /* V4: handle→addr */
        uint32_t len = (dd == 0) ? 0 : bpvm_read_u32_be(vm->memory + dd);
        const uint8_t* data = (dd == 0) ? NULL : (vm->memory + dd + 4);
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
                bpref_push(vm, tc, bpvm_handle_register(vm, ref));
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
    case BUILTIN_GUI_CREATE_OBJ: return gui_make_child(vm, tc, bpvm_gui_create_obj);
    case BUILTIN_GUI_CREATE_LABEL: return gui_make_child(vm, tc, bpvm_gui_create_label);
    case BUILTIN_GUI_CREATE_BUTTON: return gui_make_child(vm, tc, bpvm_gui_create_button);
    case BUILTIN_GUI_SET_TEXT: {
        uint32_t tref = pop_ref(vm, tc);
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
    case BUILTIN_GUI_LOAD_FONT:      { uint32_t ref = pop_ref(vm, tc); char path[256]; read_bp_string(vm, ref, path, sizeof(path)); push_i32(vm, tc, bpvm_gui_load_font(path)); return BPVM_OK; }
    case BUILTIN_GUI_SET_ROTATION:   { int deg = pop_i32(vm, tc); bpvm_gui_set_rotation(deg); push_i32(vm, tc, 0); return BPVM_OK; }

    /* H19 — App.* introspección del proyecto en ejecución (id 211-213). */
    case BUILTIN_APP_MAIN_MODULE: {        /* nombre del entry: basename sin ".mod" */
        const char* p = bpvm_fs_main_module_path();
        const char* base = strrchr(p, '/');
        base = base ? base + 1 : p;
        char nm[64]; size_t i = 0;
        while (base[i] && base[i] != '.' && i + 1 < sizeof(nm)) { nm[i] = base[i]; i++; }
        nm[i] = '\0';
        uint32_t r = bpvm_heap_alloc_string(vm, nm, strlen(nm));
        push_ref(vm, tc, r);   /* V4 #8: string = ref 8B (era push_i32 4B → drift + gen=0) */
        return BPVM_OK;
    }
    case BUILTIN_APP_MAIN_MODULE_PATH: {
        const char* s = bpvm_fs_main_module_path();
        uint32_t r = bpvm_heap_alloc_string(vm, s, strlen(s));
        push_ref(vm, tc, r);   /* V4 #8: string = ref 8B (era push_i32 4B → drift + gen=0) */
        return BPVM_OK;
    }
    case BUILTIN_APP_PROJECT_PATH: {
        const char* s = bpvm_fs_basedir();
        uint32_t r = bpvm_heap_alloc_string(vm, s, strlen(s));
        push_ref(vm, tc, r);   /* V4 #8: string = ref 8B (era push_i32 4B → drift + gen=0) */
        return BPVM_OK;
    }
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
                if (d) { int32_t a = (int32_t) objptr; bpvm_call_bp_from_builtin(vm, tc, d, &a, 1, 1u); /* #302: arg0 es ref */ }
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
            if (d) { int32_t a = (int32_t) objptr; bpvm_call_bp_from_builtin(vm, tc, d, &a, 1, 1u); /* #302: arg0 es ref */ }
        }
#endif
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_RUN_ONCE: {
        /* #324 — UNA pasada del bombeo. Mismo trabajo que GUI_RUN por dentro,
         * pero SIN el lazo: el lazo vive ahora en Gui.run() (BP), que es lo que
         * da la frontera de instrucción donde el scheduler drena los eventos.
         * Devuelve 1 = "vuelve a llamarme" / 0 = "no queda nada". */
        uint32_t disp_click = 0, disp_change = 0;
        for (int i = 0; i < vm->symbol_count; i++) {
            if      (strcmp(vm->symbols[i].name, "Gui.__guiDispatch")       == 0) disp_click  = vm->symbols[i].abs_addr;
            else if (strcmp(vm->symbols[i].name, "Gui.__guiDispatchChange") == 0) disp_change = vm->symbols[i].abs_addr;
        }
        int drained = 0;
        uint32_t objptr; int kind;
        while ((objptr = bpvm_gui_next_event(&kind)) != 0) {
            uint32_t d = (kind == 1) ? disp_change : disp_click;
            if (d) { int32_t a = (int32_t) objptr; bpvm_call_bp_from_builtin(vm, tc, d, &a, 1, 1u); /* #302: arg0 es ref */ }
            drained++;
        }
        /* P-run-stop (#257): el KILL se sigue poleando aquí. Ahora además el
         * scheduler ve el kill entre pasadas, así que la parada es más fina que
         * con el lazo dentro del builtin. */
        if (vm->poll_cb != NULL && vm->poll_cb(vm, vm->poll_user) != 0)
            vm->kill_requested = 1;
        if (vm->kill_requested) { push_i32(vm, tc, 0); return BPVM_OK; }
#ifdef BPVM_LVGL
        if (!bpvm_gui_lvgl_window_open()) { push_i32(vm, tc, 0); return BPVM_OK; }
        bpvm_gui_lvgl_pump();
        push_i32(vm, tc, 1);                 /* ventana viva → sigue habiendo trabajo */
#else
        /* Headless: "queda trabajo" = clics drenados en ESTA pasada O eventos BP
         * ENCOLADOS sin entregar. Lo segundo es lo que hace que el lazo sirva de
         * algo: `raise` encola, y el scheduler sólo inyecta el frame del handler
         * ENTRE quanta (events.c: "el thread NO puede estar corriendo"). Salir en
         * cuanto no hay clics deja el evento en la cola para siempre — que es
         * exactamente lo que midió samples/GuiEvSpike.bp. Girando mientras
         * ev_count>0, el lazo alcanza la frontera de quantum donde se drena. */
        push_i32(vm, tc, (drained > 0 || vm->ev_count > 0) ? 1 : 0);
#endif
        return BPVM_OK;
    }
    case BUILTIN_GUI_SLOT_OF: {
        /* #324 tanda 2b — REFLEXION MINIMA: nombre de metodo -> slot de vtable,
         * en EJECUCION. Idea de Eduardo, y no hace falta dato nuevo: el .mod ya
         * exporta un simbolo `Clase#metodo#slot` por cada entrada de la vtable
         * (exportVtableMarkers), apuntando al descriptor de la clase. O sea que
         * el mapa nombre->slot YA viaja; solo faltaba preguntarle.
         *
         * Con esto el .win deja de necesitar slots horneados: se acabo el
         * numero congelado en un fichero que miente cuando la clase cambia.
         *
         * Devuelve -1 si no existe (el llamante decide; aqui no se lanza nada:
         * "no hay handler con ese nombre" es normal, no un error).
         *
         * Coste: un barrido lineal de la tabla, en CARGA del form y una vez por
         * nombre. No esta en ningun camino caliente. */
        uint32_t name_ref = pop_ref(vm, tc);
        uint32_t obj      = pop_ref(vm, tc);
        if (obj == 0) { push_i32(vm, tc, -1); return BPVM_OK; }
        char nm[96];
        read_bp_string(vm, name_ref, nm, sizeof(nm));
        uint32_t oa = ref_addr(vm, obj);
        uint32_t class_ptr = (uint32_t) bpvm_read_i32_be(vm->memory + oa);
        size_t nlen = strlen(nm);
        for (int i = 0; i < vm->symbol_count; i++) {
            if (vm->symbols[i].abs_addr != class_ptr) continue;
            /* Formato: <Clase>#<metodo>#<slot>. Se busca "#<metodo>#" y se lee
             * el numero que va detras. */
            const char* s = vm->symbols[i].name;
            const char* h = strchr(s, '#');
            if (h == NULL) continue;
            if (strncmp(h + 1, nm, nlen) != 0 || h[1 + nlen] != '#') continue;
            push_i32(vm, tc, atoi(h + 2 + nlen));
            return BPVM_OK;
        }
        push_i32(vm, tc, -1);
        return BPVM_OK;
    }
    case BUILTIN_GUI_DUMP_TREE: {
        char* buf = NULL;
        size_t n = bpvm_gui_dump_tree(&buf);
        uint32_t ref = bpvm_heap_alloc_string(vm, buf ? buf : "", n);
        bpvm_free(buf);
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }
    case BUILTIN_GUI_INVOKE_BY_NAME: {
        /* H13 — Forms: resuelve `name` como función pública del módulo de `host`
         * (la ventana) y la invoca con `sender` como arg0. Args: host, name, sender.
         * Espejo de GUI_INVOKE_BY_NAME en miVM. */
        uint32_t sender   = pop_ref(vm, tc);
        uint32_t name_ref = pop_ref(vm, tc);
        uint32_t host     = pop_ref(vm, tc);
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
        bpvm_call_bp_from_builtin(vm, tc, target, &a, 1, 1u); /* #302: sender es ref */
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_INVOKE_BY_SLOT: {
        /* H13.1 — Forms Camino A: invoca el MÉTODO de la ventana en vtable[slot]
         * (slot horneado por el IDE). Args: win, slot, sender. Espejo de
         * invokeHandlerBySlot en miVM: paseo de vtable con fallback al padre
         * (idéntico a OP_INVOKE_VIRTUAL) → bridge con [this=win, sender] como
         * locals 0/1. slot < 0 o no resoluble → IGNORA (sin excepción). */
        uint32_t sender = pop_ref(vm, tc);
        int32_t  slot   = pop_i32(vm, tc);
        uint32_t win    = pop_ref(vm, tc);
        if (win == 0 || slot < 0) { push_i32(vm, tc, 0); return BPVM_OK; }
        uint8_t* mem = vm->memory;
        uint32_t wa = ref_addr(vm, win);   /* V4: handle→addr (el `this` que se pasa abajo sigue siendo el REF `win`) */
        uint32_t desc = (uint32_t) bpvm_read_i32_be(mem + wa);
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
        bpvm_call_bp_from_builtin(vm, tc, target_abs, a2, 2, 3u); /* #302: win+sender son refs */
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_BIND_CLICK: {
        uint32_t self = pop_ref(vm, tc);
        int handle = pop_i32(vm, tc);
        bpvm_gui_bind_click(handle, self);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_CLICK: {
        uint32_t obj = pop_ref(vm, tc);
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
    case BUILTIN_GUI_CREATE_CHECKBOX: return gui_make_child(vm, tc, bpvm_gui_create_checkbox);
    case BUILTIN_GUI_SET_CHECKED:     { int v = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_checked(h, v); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_GET_CHECKED:     { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_checked(h)); return BPVM_OK; }
    case BUILTIN_GUI_CHANGE:          { uint32_t obj = pop_ref(vm, tc); bpvm_gui_inject_change(obj); push_i32(vm, tc, 0); return BPVM_OK; }
    /* H6 — switch + slider + bar (value-widgets enteros; el backend clampa al rango). */
    case BUILTIN_GUI_CREATE_SWITCH: return gui_make_child(vm, tc, bpvm_gui_create_switch);
    case BUILTIN_GUI_CREATE_SLIDER: return gui_make_child(vm, tc, bpvm_gui_create_slider);
    case BUILTIN_GUI_CREATE_BAR: return gui_make_child(vm, tc, bpvm_gui_create_bar);
    case BUILTIN_GUI_SET_VALUE:     { int v = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_value(h, v); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_GET_VALUE:     { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_value(h)); return BPVM_OK; }
    case BUILTIN_GUI_SET_RANGE:     { int mx = pop_i32(vm, tc); int mn = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_range(h, mn, mx); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_SPINBOX: return gui_make_child(vm, tc, bpvm_gui_create_spinbox);
    case BUILTIN_GUI_CREATE_LED: return gui_make_child(vm, tc, bpvm_gui_create_led);
    case BUILTIN_GUI_CREATE_DROPDOWN: return gui_make_child(vm, tc, bpvm_gui_create_dropdown);
    case BUILTIN_GUI_SET_OPTIONS: {
        uint32_t ref = pop_ref(vm, tc); int h = pop_i32(vm, tc);
        char buf[512]; read_bp_string(vm, ref, buf, sizeof(buf));
        bpvm_gui_set_options(h, buf); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CREATE_TEXTAREA: return gui_make_child(vm, tc, bpvm_gui_create_textarea);
    case BUILTIN_GUI_GET_TEXT: {
        int h = pop_i32(vm, tc);
        const char* s = bpvm_gui_get_text(h);
        uint32_t ref = bpvm_heap_alloc_string(vm, s, strlen(s));
        push_ref(vm, tc, ref); return BPVM_OK;
    }
    case BUILTIN_GUI_CREATE_LIST: return gui_make_child(vm, tc, bpvm_gui_create_list);
    case BUILTIN_GUI_CREATE_KEYBOARD: return gui_make_child(vm, tc, bpvm_gui_create_keyboard);
    case BUILTIN_GUI_KEYBOARD_SET_TEXTAREA: { int ta = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_keyboard_set_textarea(h, ta); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_MSGBOX: return gui_make_child(vm, tc, bpvm_gui_create_msgbox);
    case BUILTIN_GUI_SET_BUTTONS: {
        uint32_t ref = pop_ref(vm, tc); int h = pop_i32(vm, tc);
        char buf[256]; read_bp_string(vm, ref, buf, sizeof(buf));
        bpvm_gui_set_buttons(h, buf); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CREATE_TABVIEW: return gui_make_child(vm, tc, bpvm_gui_create_tabview);
    case BUILTIN_GUI_TABVIEW_ADD_TAB: {
        uint32_t ref = pop_ref(vm, tc); int h = pop_i32(vm, tc);
        char buf[128]; read_bp_string(vm, ref, buf, sizeof(buf));
        push_i32(vm, tc, bpvm_gui_tabview_add_tab(h, buf)); return BPVM_OK;
    }
    /* H7 — Chart */
    case BUILTIN_GUI_CREATE_CHART: return gui_make_child(vm, tc, bpvm_gui_create_chart);
    case BUILTIN_GUI_CHART_SET_POINTS: {
        int n = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_chart_set_points(h, n); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CHART_ADD_SERIES: {
        int rgb = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        push_i32(vm, tc, bpvm_gui_chart_add_series(h, rgb)); return BPVM_OK;
    }
    case BUILTIN_GUI_CHART_PUSH: {
        int v = pop_i32(vm, tc); int s = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_chart_push(h, s, v); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CHART_SET_VALUE: {
        int v = pop_i32(vm, tc); int i = pop_i32(vm, tc); int s = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_chart_set_value(h, s, i, v); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CHART_SET_TYPE: {
        int t = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_chart_set_type(h, t); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CREATE_TABLE: return gui_make_child(vm, tc, bpvm_gui_create_table);
    case BUILTIN_GUI_TABLE_SET_GRID: {
        int cols = pop_i32(vm, tc); int rows = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_table_set_grid(h, rows, cols); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_TABLE_SET_CELL: {
        uint32_t ref = pop_ref(vm, tc);
        int col = pop_i32(vm, tc); int row = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        char buf[256]; read_bp_string(vm, ref, buf, sizeof(buf));
        bpvm_gui_table_set_cell(h, row, col, buf); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_TABLE_GET_CELL: {
        int col = pop_i32(vm, tc); int row = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        const char* s = bpvm_gui_table_get_cell(h, row, col);
        uint32_t ref = bpvm_heap_alloc_string(vm, s, strlen(s));
        push_ref(vm, tc, ref); return BPVM_OK;
    }
    case BUILTIN_GUI_IMAGE_NEW: { push_i32(vm, tc, bpvm_gui_image_new()); return BPVM_OK; }
    case BUILTIN_GUI_IMAGE_LOAD_FILE: {
        uint32_t ref = pop_ref(vm, tc); int id = pop_i32(vm, tc);
        char buf[512]; read_bp_string(vm, ref, buf, sizeof(buf));
        push_i32(vm, tc, bpvm_gui_image_load_file(id, buf)); return BPVM_OK;
    }
    case BUILTIN_GUI_IMAGE_WIDTH:  { int id = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_image_width(id));  return BPVM_OK; }
    case BUILTIN_GUI_IMAGE_HEIGHT: { int id = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_image_height(id)); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_IMAGEVIEW: return gui_make_child(vm, tc, bpvm_gui_create_imageview);
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
        uint32_t ref = pop_ref(vm, tc);   /* H1.2a: string ref = 8 bytes */
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
        uint32_t ref = pop_ref(vm, tc);   /* H1.2a: string ref = 8 bytes */
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
        uint32_t pref = pop_ref(vm, tc);
        char path[512];
        read_bp_string(vm, pref, path, sizeof(path));
        /* H19-F1 — resuelve relativo al base-dir del proyecto (si lo hay), luego
         * cwd/literal, luego /app (modo plano). Los absolutos no se tocan. */
        char eff[600];
        const char* rpath = bpvm_fs_resolve(path, eff, sizeof(eff));
        uint32_t size = 0;
        if (bpvm_fs_stat(rpath, &size) != 0) {
            char em[576];
            snprintf(em, sizeof(em), "readFile('%s'): no se pudo abrir", path);
            return builtin_throw(vm, tc, em);
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
                bpref_push(vm, tc, bpvm_handle_register(vm, ref));
        return BPVM_OK;
    }
    case BUILTIN_WRITE_FILE:
    case BUILTIN_WRITE_FILE_BYTES:    /* #247: idéntico — escribe los bytes crudos del array */
    case BUILTIN_APPEND_FILE: {
        int append = (id == BUILTIN_APPEND_FILE);
        uint32_t cref = pop_ref(vm, tc);   /* content (empujado el último) */
        uint32_t pref = pop_ref(vm, tc);   /* path */
        char path[512];
        read_bp_string(vm, pref, path, sizeof(path));
        uint32_t cad = (cref == 0) ? 0 : bpref_deref(vm, bpref_from_addr(cref));   /* V4: handle→addr */
        uint32_t clen = (cad == 0) ? 0 : bpvm_read_u32_be(vm->memory + cad);
        const uint8_t* cdata = (cad == 0) ? NULL : (vm->memory + cad + 4);
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
        uint32_t pref = pop_ref(vm, tc);
        char path[512];
        read_bp_string(vm, pref, path, sizeof(path));
        push_i32(vm, tc, bpvm_fs_exists(path) ? 1 : 0);
        return BPVM_OK;
    }

    /* #240 (logger) — gestión de ficheros (IO.removeFile/rename/fileSize).
     * Mismos contratos que la VM-Java; backend sin la op → RuntimeError
     * atrapable (los firmwares la añaden en su próximo build). */
    case BUILTIN_REMOVE_FILE: {
        uint32_t pref = pop_ref(vm, tc);
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
        uint32_t tref = pop_ref(vm, tc);   /* to (empujado el último) */
        uint32_t fref = pop_ref(vm, tc);   /* from */
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
        uint32_t pref = pop_ref(vm, tc);
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
        uint32_t pref = pop_ref(vm, tc);
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
        uint32_t pref = pop_ref(vm, tc);
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
        uint32_t tref = pop_ref(vm, tc);   /* to (empujado el último) */
        uint32_t fref = pop_ref(vm, tc);   /* from */
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
        uint32_t pref = pop_ref(vm, tc);
        char path[512];
        read_bp_string(vm, pref, path, sizeof(path));
        push_i32(vm, tc, bpvm_fs_isdir(path) ? 1 : 0);   /* sin throw, como Java */
        return BPVM_OK;
    }
    case BUILTIN_LAST_MODIFIED: {
        uint32_t pref = pop_ref(vm, tc);
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
        uint32_t mref = pop_ref(vm, tc);
        char msg[256];
        read_bp_string(vm, mref, msg, sizeof(msg));
        return builtin_throw(vm, tc, msg);
    }

    case BUILTIN_CHAR_CODE_AT: {
        int32_t i   = pop_i32(vm, tc);
        bpref_t s = bpref_pop(vm, tc);   /* string ref */
        if (bpref_is_null(s)) { push_i32(vm, tc, 0); return BPVM_OK; }
        uint32_t nbytes = bpref_arr_len(vm, s);
        const uint8_t* p = bpref_arr_elem(vm, s, 0, 1);
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
        bpref_t s = bpref_pop(vm, tc);   /* string ref */
        uint8_t enc[4]; uint32_t enc_len = 0;
        if (!bpref_is_null(s)) {
            uint32_t nbytes = bpref_arr_len(vm, s);
            const uint8_t* p = bpref_arr_elem(vm, s, 0, 1);
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
            out = (uint32_t) bpvm_handle_register(vm, out).v;   /* V4: addr → handle */
        }
        push_ref(vm, tc, out);   /* H1.2a: string ref result = 8 bytes */
        return BPVM_OK;
    }

    case BUILTIN_SUBSTRING: {
        /* substring(s, start, end): string — copia [start, end) con
         * clamp estilo BP. Idéntico a aot_helpers h_string_substring
         * para que intérprete y AOT den el mismo resultado (#173). */
        int32_t end   = pop_i32(vm, tc);
        int32_t start = pop_i32(vm, tc);
        /* #350 — PEEK, no pop: el bucle de copia de abajo sigue leyendo de `s`
         * DESPUÉS de alocar el resultado, y una reserva puede colectar. Con pop,
         * `s` queda por encima de sp y el marcado conservador no la ve: si el
         * llamante no la tenía además en un local (p.ej. substring(a+b, ...)),
         * el GC se la lleva EN VIVO. Medido: 3.000 vueltas con el heap estrecho
         * daban entre 3 y 6 resultados malos, y subían al encoger el heap. */
        bpref_t s = peek_ref(vm, tc, 0);
        uint32_t nbytes = bpref_arr_len(vm, s);
        const uint8_t* p = bpref_arr_elem(vm, s, 0, 1);
        uint32_t ncp = bpref_is_null(s) ? 0 : utf8_cp_count(p, nbytes);   /* H2: índices en codepoints */
        if (start < 0) start = 0;
        if (end   < 0) end = 0;
        if ((uint32_t) end > ncp) end = (int32_t) ncp;
        if (start > end) start = end;
        uint32_t boff = utf8_byte_offset(p, nbytes, (uint32_t) start);
        uint32_t eoff = utf8_byte_offset(p, nbytes, (uint32_t) end);
        uint32_t n = eoff - boff;                                   /* nº de bytes del rango */
        uint32_t out = bpvm_heap_alloc(vm, n, BPVM_TYPE_ARRAY_I8);
        if (out) {
            bpvm_write_u32_be(vm->memory + out, n);   /* out: alloc fresca (dirección física) */
            for (uint32_t i = 0; i < n; i++)
                vm->memory[out + 4 + i] = *bpref_arr_elem(vm, s, boff + i, 1);
            out = (uint32_t) bpvm_handle_register(vm, out).v;   /* V4: addr → handle */
        }
        tc->sp -= BPVM_REF_SIZE;   /* #350: AHORA se saca el origen, ya copiado */
        push_ref(vm, tc, out);   /* string ref result (push_ref ya es genérico) */
        return BPVM_OK;
    }

    /* ================================================================
     * #348 — CADENAS DE CÓMPUTO PURO. Estaban sólo en miVM.
     *
     * Semánticas MEDIDAS contra Java (sonda con codepoints y longitudes
     * reales), no deducidas — los tres corners que importan:
     *   split(",a,,b,", ",") -> 5 partes, conserva vacíos delante y detrás
     *   split("abc", "")     -> 4: "a" "b" "c" ""
     *   split("", cualquiera)-> 1: ""
     *   replace("ab", "", "-") -> "-a-b-"
     *   trim quita <= U+0020 (NO quita U+00A0)
     *
     * TODAS leen sus argumentos con peek_ref y los sacan AL FINAL: ver el
     * comentario de peek_ref sobre por qué un pop antes de alocar es un
     * agujero de GC.
     * ================================================================ */

    case BUILTIN_UPPER:
    case BUILTIN_LOWER: {
        /* Latin-1 (ver latin1_upper_cp): la longitud en BYTES no cambia, así
         * que el resultado se aloca del tamaño exacto del origen. */
        int up = (id == BUILTIN_UPPER);
        bpref_t s = peek_ref(vm, tc, 0);
        uint32_t n = bpref_is_null(s) ? 0 : bpref_arr_len(vm, s);
        uint32_t out = bpvm_heap_alloc(vm, n, BPVM_TYPE_ARRAY_I8);
        if (out) {
            bpvm_write_u32_be(vm->memory + out, n);
            const uint8_t* p = (n > 0) ? bpref_arr_elem(vm, s, 0, 1) : NULL;
            uint32_t i = 0, o = 0;
            while (i < n) {
                uint32_t adv = 0;
                uint32_t cp  = utf8_decode(p + i, n - i, &adv);
                uint32_t m   = up ? latin1_upper_cp(cp) : latin1_lower_cp(cp);
                uint8_t enc[4];
                uint32_t el = utf8_encode(m, enc);
                for (uint32_t k = 0; k < el && o < n; k++) vm->memory[out + 4 + o++] = enc[k];
                i += (adv > 0) ? adv : 1;
            }
            out = (uint32_t) bpvm_handle_register(vm, out).v;
        }
        tc->sp -= BPVM_REF_SIZE;
        push_ref(vm, tc, out);
        return BPVM_OK;
    }

    case BUILTIN_TRIM: {
        /* Java String.trim(): quita los caracteres <= U+0020 de los dos
         * extremos. A nivel de byte es exacto — ningún byte de continuación
         * UTF-8 es <= 0x20, así que no se puede cortar un carácter por medio. */
        bpref_t s = peek_ref(vm, tc, 0);
        uint32_t n = bpref_is_null(s) ? 0 : bpref_arr_len(vm, s);
        uint32_t b = 0, e = n;
        if (n > 0) {
            const uint8_t* p = bpref_arr_elem(vm, s, 0, 1);
            while (b < e && p[b] <= 0x20) b++;
            while (e > b && p[e - 1] <= 0x20) e--;
        }
        uint32_t len = e - b;
        uint32_t out = bpvm_heap_alloc(vm, len, BPVM_TYPE_ARRAY_I8);
        if (out) {
            bpvm_write_u32_be(vm->memory + out, len);
            for (uint32_t i = 0; i < len; i++)
                vm->memory[out + 4 + i] = *bpref_arr_elem(vm, s, b + i, 1);
            out = (uint32_t) bpvm_handle_register(vm, out).v;
        }
        tc->sp -= BPVM_REF_SIZE;
        push_ref(vm, tc, out);
        return BPVM_OK;
    }

    case BUILTIN_INDEX_OF: {
        /* Devuelve el índice en CODEPOINTS (no en bytes), para ser coherente
         * con charAt/substring. Se busca por bytes y se convierte al final. */
        bpref_t q = peek_ref(vm, tc, 0);          /* aguja */
        bpref_t s = peek_ref(vm, tc, 1);          /* pajar */
        uint32_t sn = bpref_is_null(s) ? 0 : bpref_arr_len(vm, s);
        uint32_t qn = bpref_is_null(q) ? 0 : bpref_arr_len(vm, q);
        const uint8_t* sp_ = (sn > 0) ? bpref_arr_elem(vm, s, 0, 1) : (const uint8_t*) "";
        const uint8_t* qp_ = (qn > 0) ? bpref_arr_elem(vm, q, 0, 1) : (const uint8_t*) "";
        long off = bp_find_bytes(sp_, sn, qp_, qn, 0);
        int32_t res = (off < 0) ? -1 : (int32_t) utf8_cp_count(sp_, (uint32_t) off);
        tc->sp -= 2 * BPVM_REF_SIZE;
        push_i32(vm, tc, res);
        return BPVM_OK;
    }

    case BUILTIN_STARTS_WITH:
    case BUILTIN_ENDS_WITH:
    case BUILTIN_CONTAINS: {
        bpref_t q = peek_ref(vm, tc, 0);
        bpref_t s = peek_ref(vm, tc, 1);
        uint32_t sn = bpref_is_null(s) ? 0 : bpref_arr_len(vm, s);
        uint32_t qn = bpref_is_null(q) ? 0 : bpref_arr_len(vm, q);
        const uint8_t* sp_ = (sn > 0) ? bpref_arr_elem(vm, s, 0, 1) : (const uint8_t*) "";
        const uint8_t* qp_ = (qn > 0) ? bpref_arr_elem(vm, q, 0, 1) : (const uint8_t*) "";
        int r;
        if (id == BUILTIN_STARTS_WITH)      r = (qn <= sn) && memcmp(sp_, qp_, qn) == 0;
        else if (id == BUILTIN_ENDS_WITH)   r = (qn <= sn) && memcmp(sp_ + (sn - qn), qp_, qn) == 0;
        else                                r = bp_find_bytes(sp_, sn, qp_, qn, 0) >= 0;
        tc->sp -= 2 * BPVM_REF_SIZE;
        push_i32(vm, tc, r ? 1 : 0);
        return BPVM_OK;
    }

    case BUILTIN_REPLACE: {
        /* replace(s, target, rep): literal, TODAS las ocurrencias.
         * Target vacío = el corner de Java: "ab" -> "-a-b-", o sea el
         * reemplazo se mete en cada frontera de carácter Y al final. */
        bpref_t rp = peek_ref(vm, tc, 0);
        bpref_t tg = peek_ref(vm, tc, 1);
        bpref_t s  = peek_ref(vm, tc, 2);
        uint32_t sn = bpref_is_null(s)  ? 0 : bpref_arr_len(vm, s);
        uint32_t tn = bpref_is_null(tg) ? 0 : bpref_arr_len(vm, tg);
        uint32_t rn = bpref_is_null(rp) ? 0 : bpref_arr_len(vm, rp);
        const uint8_t* sb = (sn > 0) ? bpref_arr_elem(vm, s,  0, 1) : (const uint8_t*) "";
        const uint8_t* tb = (tn > 0) ? bpref_arr_elem(vm, tg, 0, 1) : (const uint8_t*) "";

        /* 1ª pasada: contar ocurrencias para saber el tamaño exacto. */
        uint32_t hits = 0;
        if (tn == 0) {
            hits = utf8_cp_count(sb, sn) + 1;          /* una por frontera + la final */
        } else {
            for (uint32_t i = 0; i + tn <= sn; ) {
                if (memcmp(sb + i, tb, tn) == 0) { hits++; i += tn; } else i++;
            }
        }
        uint32_t outn = sn - hits * tn + hits * rn;
        uint32_t out  = bpvm_heap_alloc(vm, outn, BPVM_TYPE_ARRAY_I8);
        if (out) {
            bpvm_write_u32_be(vm->memory + out, outn);
            /* 2ª pasada: copiar. Los punteros se re-piden tras la alocación. */
            sb = (sn > 0) ? bpref_arr_elem(vm, s,  0, 1) : (const uint8_t*) "";
            tb = (tn > 0) ? bpref_arr_elem(vm, tg, 0, 1) : (const uint8_t*) "";
            const uint8_t* rb = (rn > 0) ? bpref_arr_elem(vm, rp, 0, 1) : (const uint8_t*) "";
            uint32_t o = 0, i = 0;
            if (tn == 0) {
                for (uint32_t k = 0; k < rn; k++) vm->memory[out + 4 + o++] = rb[k];
                while (i < sn) {
                    uint32_t adv = 0;
                    (void) utf8_decode(sb + i, sn - i, &adv);
                    if (adv == 0) adv = 1;
                    for (uint32_t k = 0; k < adv; k++) vm->memory[out + 4 + o++] = sb[i + k];
                    for (uint32_t k = 0; k < rn; k++) vm->memory[out + 4 + o++] = rb[k];
                    i += adv;
                }
            } else {
                while (i < sn) {
                    if (i + tn <= sn && memcmp(sb + i, tb, tn) == 0) {
                        for (uint32_t k = 0; k < rn; k++) vm->memory[out + 4 + o++] = rb[k];
                        i += tn;
                    } else {
                        vm->memory[out + 4 + o++] = sb[i++];
                    }
                }
            }
            out = (uint32_t) bpvm_handle_register(vm, out).v;
        }
        tc->sp -= 3 * BPVM_REF_SIZE;
        push_ref(vm, tc, out);
        return BPVM_OK;
    }

    case BUILTIN_SPLIT: {
        /* split(s, sep) -> string[]. Semántica de Java con límite -1: conserva
         * los vacíos de los dos extremos. Casos medidos arriba.
         *
         * ORDEN DE ALOCACIÓN: primero el ARRAY y se EMPUJA como raíz; después
         * cada trozo, guardándolo en el array nada más crearlo. Así, si una
         * alocación dispara GC, los trozos ya creados están dentro de un array
         * que sí es raíz. (miVM lo hace al revés — sus refs viven en un array
         * Java que el GC de BP no escanea. Mismo QUÉ, distinto CÓMO.) */
        bpref_t sep = peek_ref(vm, tc, 0);
        bpref_t s   = peek_ref(vm, tc, 1);
        uint32_t sn = bpref_is_null(s)   ? 0 : bpref_arr_len(vm, s);
        uint32_t pn = bpref_is_null(sep) ? 0 : bpref_arr_len(vm, sep);
        const uint8_t* sb = (sn > 0) ? bpref_arr_elem(vm, s,   0, 1) : (const uint8_t*) "";
        const uint8_t* pb = (pn > 0) ? bpref_arr_elem(vm, sep, 0, 1) : (const uint8_t*) "";

        /* Contar partes (y de paso decidir la forma). */
        uint32_t nparts;
        if (sn == 0)      nparts = 1;                        /* "" -> [""] siempre */
        else if (pn == 0) nparts = utf8_cp_count(sb, sn) + 1;/* "abc" -> a,b,c,"" */
        else {
            nparts = 1;
            for (uint32_t i = 0; i + pn <= sn; ) {
                if (memcmp(sb + i, pb, pn) == 0) { nparts++; i += pn; } else i++;
            }
        }

        uint32_t arr = bpvm_heap_alloc(vm, nparts * BPVM_REF_SIZE, BPVM_TYPE_ARRAY_REF);
        if (arr == 0) return builtin_throw(vm, tc, "No space in heap");
        bpvm_write_u32_be(vm->memory + arr, nparts);
        bpref_t arrh = bpvm_handle_register(vm, arr);
        bpref_push(vm, tc, arrh);          /* raíz temporal: protege los trozos */

        uint32_t done = 0;
        uint32_t i = 0;
        while (done < nparts) {
            /* Recalcular punteros: entre trozo y trozo hay alocaciones. */
            sb = (sn > 0) ? bpref_arr_elem(vm, s,   0, 1) : (const uint8_t*) "";
            pb = (pn > 0) ? bpref_arr_elem(vm, sep, 0, 1) : (const uint8_t*) "";
            uint32_t b = i, e;
            if (sn == 0)      { e = 0; i = 0; }
            else if (pn == 0) {
                uint32_t adv = 0;
                if (i < sn) { (void) utf8_decode(sb + i, sn - i, &adv); if (adv == 0) adv = 1; }
                e = (i < sn) ? i + adv : i;
                i = e;
            } else {
                long off = bp_find_bytes(sb, sn, pb, pn, i);
                if (off < 0 || done == nparts - 1) { e = sn; i = sn; }
                else { e = (uint32_t) off; i = e + pn; }
            }
            uint32_t plen = e - b;
            uint32_t ps = bpvm_heap_alloc(vm, plen, BPVM_TYPE_ARRAY_I8);
            if (ps == 0) { tc->sp -= BPVM_REF_SIZE; return builtin_throw(vm, tc, "No space in heap"); }
            bpvm_write_u32_be(vm->memory + ps, plen);
            for (uint32_t k = 0; k < plen; k++)
                vm->memory[ps + 4 + k] = *bpref_arr_elem(vm, s, b + k, 1);
            bpref_t psh = bpvm_handle_register(vm, ps);
            bpref_store(vm, bpref_deref(vm, arrh) + BPVM_ARR_DATA_OFF + done * BPVM_REF_SIZE, psh);
            done++;
        }

        tc->sp -= BPVM_REF_SIZE;           /* fuera la raíz temporal */
        tc->sp -= 2 * BPVM_REF_SIZE;       /* fuera los dos argumentos */
        bpref_push(vm, tc, arrh);
        return BPVM_OK;
    }

    /* ================================================================
     * #348 tanda 2 — MATEMÁTICAS DE CÓMPUTO PURO.
     *
     * REGLA que las gobierna todas: calcular en DOUBLE y estrechar a float AL
     * FINAL. miVM lo hace asi porque los Math.* de Java toman double y el
     * resultado se castea a (float). Usar sqrtf/sinf/... daria otro ultimo bit
     * en algunos valores y la paridad se rompe donde nadie mira.
     * ================================================================ */

    case BUILTIN_SQRT: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) sqrt((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_POW: {
        float e = bits_to_f32(pop_i32(vm, tc));     /* exponente arriba */
        float b = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) pow((double) b, (double) e)));
        return BPVM_OK;
    }
    case BUILTIN_LOG: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) log((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_LOG10: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) log10((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_EXP: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) exp((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_SIN: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) sin((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_COS: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) cos((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_TAN: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) tan((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_ASIN: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) asin((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_ACOS: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) acos((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_ATAN: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) atan((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_ATAN2: {
        /* pila (abajo→arriba): y, x. El pop devuelve primero el de arriba. */
        float x = bits_to_f32(pop_i32(vm, tc));
        float y = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) atan2((double) y, (double) x)));
        return BPVM_OK;
    }
    case BUILTIN_PI: {
        /* El double más cercano a pi, luego a f32 — igual que (float) Math.PI. */
        push_i32(vm, tc, f32_to_bits((float) 3.14159265358979323846));
        return BPVM_OK;
    }
    case BUILTIN_E: {
        push_i32(vm, tc, f32_to_bits((float) 2.7182818284590452354));
        return BPVM_OK;
    }
    case BUILTIN_FLOOR: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, java_d2i(floor((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_CEIL: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, java_d2i(ceil((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_ROUND: {
        /* Math.round(float) de Java: el int más cercano, con los empates hacia
         * +infinito — o sea floor(x + 0.5f). NaN → 0 y saturación las pone
         * java_d2i. Ojo: el +0.5 va en FLOAT, como en Java, no en double. */
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, java_d2i(floor((double)(x + 0.5f))));
        return BPVM_OK;
    }
    case BUILTIN_SIGN_I: {
        int32_t x = pop_i32(vm, tc);
        push_i32(vm, tc, (x > 0) - (x < 0));       /* Integer.compare(x,0) */
        return BPVM_OK;
    }
    case BUILTIN_SIGN_F: {
        float x = bits_to_f32(pop_i32(vm, tc));
        int32_t r;
        if (x != x)      r = 0;                    /* convención de miVM: NaN → 0 */
        else if (x > 0)  r = 1;
        else if (x < 0)  r = -1;
        else             r = 0;                    /* ±0 */
        push_i32(vm, tc, r);
        return BPVM_OK;
    }
    case BUILTIN_FACTORIAL_I: {
        int32_t n = pop_i32(vm, tc);
        char msg[80];
        if (n < 0) {
            snprintf(msg, sizeof msg, "factorial: argumento negativo (%d)", (int) n);
            return builtin_throw(vm, tc, msg);
        }
        if (n > 12) {   /* 13! desborda i32 con signo */
            snprintf(msg, sizeof msg, "factorial: %d desborda integer (max 12)", (int) n);
            return builtin_throw(vm, tc, msg);
        }
        int32_t r = 1;
        for (int32_t i = 2; i <= n; i++) r *= i;
        push_i32(vm, tc, r);
        return BPVM_OK;
    }
    case BUILTIN_GAMMA_F: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) lanczos_gamma((double) x)));
        return BPVM_OK;
    }

    /* ================================================================
     * #348 tanda 3 — RUTAS. Siempre con '/'.
     *
     * miVM las hacía con java.nio.file.Paths, que es DEPENDIENTE DE
     * PLATAFORMA: en Windows pathJoin("a","b") daba "a\b" — o sea rutas que el
     * FS del dispositivo no entiende, y ademas distintas segun el host donde
     * corriera. Mismo patron que el locale de upper/lower.
     *
     * La semantica se DEFINE aqui (y en miVM igual), no se hereda:
     *   join(a,b)  a vacio -> b; b vacio -> a; si no, a sin '/' finales + '/' +
     *              b sin '/' iniciales.  ("/" + "x" -> "/x")
     *   parent(p)  se ignoran los '/' finales; luego hasta el ultimo '/' sin
     *              incluirlo. Sin '/' -> "". Si el ultimo esta en 0 -> "/".
     *   basename(p) se ignoran los '/' finales; luego desde el ultimo '/'.
     *   extension(p) sobre el basename: ultimo '.'; si esta en 0 o al final -> "".
     * ================================================================ */

    case BUILTIN_PATH_JOIN: {
        bpref_t b = peek_ref(vm, tc, 0);
        bpref_t a = peek_ref(vm, tc, 1);
        uint32_t an = bpref_is_null(a) ? 0 : bpref_arr_len(vm, a);
        uint32_t bn = bpref_is_null(b) ? 0 : bpref_arr_len(vm, b);
        const uint8_t* ap = (an > 0) ? bpref_arr_elem(vm, a, 0, 1) : (const uint8_t*) "";
        const uint8_t* bp = (bn > 0) ? bpref_arr_elem(vm, b, 0, 1) : (const uint8_t*) "";
        uint32_t ae = an;                       while (ae > 0 && ap[ae-1] == '/') ae--;
        uint32_t bs = 0;                        while (bs < bn && bp[bs] == '/') bs++;
        uint32_t out_n;
        if (an == 0)      out_n = bn;           /* a vacio -> b tal cual */
        else if (bn == 0) out_n = an;           /* b vacio -> a tal cual */
        else              out_n = ae + 1 + (bn - bs);
        uint32_t out = bpvm_heap_alloc(vm, out_n, BPVM_TYPE_ARRAY_I8);
        if (out) {
            bpvm_write_u32_be(vm->memory + out, out_n);
            ap = (an > 0) ? bpref_arr_elem(vm, a, 0, 1) : (const uint8_t*) "";
            bp = (bn > 0) ? bpref_arr_elem(vm, b, 0, 1) : (const uint8_t*) "";
            uint32_t o = 0;
            if (an == 0)      { for (uint32_t i = 0; i < bn; i++) vm->memory[out+4+o++] = bp[i]; }
            else if (bn == 0) { for (uint32_t i = 0; i < an; i++) vm->memory[out+4+o++] = ap[i]; }
            else {
                for (uint32_t i = 0; i < ae; i++) vm->memory[out+4+o++] = ap[i];
                vm->memory[out+4+o++] = '/';
                for (uint32_t i = bs; i < bn; i++) vm->memory[out+4+o++] = bp[i];
            }
            out = (uint32_t) bpvm_handle_register(vm, out).v;
        }
        tc->sp -= 2 * BPVM_REF_SIZE;
        push_ref(vm, tc, out);
        return BPVM_OK;
    }

    case BUILTIN_PATH_PARENT:
    case BUILTIN_PATH_BASENAME:
    case BUILTIN_PATH_EXTENSION: {
        bpref_t s = peek_ref(vm, tc, 0);
        uint32_t n = bpref_is_null(s) ? 0 : bpref_arr_len(vm, s);
        const uint8_t* p = (n > 0) ? bpref_arr_elem(vm, s, 0, 1) : (const uint8_t*) "";
        uint32_t e = n;  while (e > 0 && p[e-1] == '/') e--;   /* '/' finales fuera */
        long slash = -1;
        for (uint32_t i = 0; i < e; i++) if (p[i] == '/') slash = (long) i;
        uint32_t b0, b1;                                        /* rango a copiar */
        if (id == BUILTIN_PATH_PARENT) {
            if (slash < 0)      { b0 = 0; b1 = 0; }             /* sin '/' -> "" */
            else if (slash == 0){ b0 = 0; b1 = 1; }             /* raiz -> "/"   */
            else                { b0 = 0; b1 = (uint32_t) slash; }
        } else {                                                /* basename y ext */
            b0 = (slash < 0) ? 0 : (uint32_t) slash + 1;
            b1 = e;
            if (id == BUILTIN_PATH_EXTENSION) {
                long dot = -1;
                for (uint32_t i = b0; i < b1; i++) if (p[i] == '.') dot = (long) i;
                /* dot en la 1a posicion del nombre (".oculto") o al final: sin ext */
                if (dot < 0 || (uint32_t) dot == b0 || (uint32_t) dot == b1 - 1) b0 = b1 = 0;
                else b0 = (uint32_t) dot + 1;
            }
        }
        uint32_t len = b1 - b0;
        uint32_t out = bpvm_heap_alloc(vm, len, BPVM_TYPE_ARRAY_I8);
        if (out) {
            bpvm_write_u32_be(vm->memory + out, len);
            for (uint32_t i = 0; i < len; i++)
                vm->memory[out + 4 + i] = *bpref_arr_elem(vm, s, b0 + i, 1);
            out = (uint32_t) bpvm_handle_register(vm, out).v;
        }
        tc->sp -= BPVM_REF_SIZE;
        push_ref(vm, tc, out);
        return BPVM_OK;
    }

    case BUILTIN_NEW_REF_ARRAY: {
        int32_t cap = pop_i32(vm, tc);
        if (cap < 0) return BPVM_ERR_RUNTIME;
        /* V4: ref plana = 8 bytes/elem (era *4 → array a media asignación →
         * corrupción al almacenar el elemento cap/2 en adelante). heap_alloc
         * zero-inicializa el payload, así que los slots quedan nulos. */
        uint32_t ref = bpvm_heap_alloc(vm, (uint32_t) cap * BPVM_REF_SIZE, BPVM_TYPE_ARRAY_REF);
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");   /* H1: OOM atrapable */
        bpvm_write_u32_be(vm->memory + ref, (uint32_t) cap);
                bpref_push(vm, tc, bpvm_handle_register(vm, ref));
        return BPVM_OK;
    }

    case BUILTIN_GROW_REF_ARRAY: {
        /* V4/paridad miVM: crecer el array de refs (backing de List/any[]).
         * Antes ausente en la VM-C (enum saltaba 44→46) → cualquier List que
         * creciera más allá de su capacidad inicial petaba 'builtin desconocido'. */
        int32_t new_cap = pop_i32(vm, tc);
        /* #350 — PEEK por HIGIENE, no por un fallo demostrado. El bucle de
         * copia lee del array VIEJO tras alocar el nuevo, o sea que viola el
         * mismo contrato que substring. Pero HOY NO MUERDE, y conviene que
         * quede escrito por qué: el emisor genera
         *     this ; this.items ; this.cap ; CALL grow ; setField items
         * así que `this.items` sigue apuntando al array viejo hasta DESPUÉS de
         * la llamada — el GC lo ve y no se lo lleva. Lo que protege es el
         * llamante, no el builtin; el día que alguien llame a esto desde otro
         * sitio, lo único que queda de pie es el contrato. */
        bpref_t oldr = peek_ref(vm, tc, 0);   /* array ref = 8 bytes (como miVM popTcRef) */
        if (new_cap < 0) { tc->sp -= BPVM_REF_SIZE; return BPVM_ERR_RUNTIME; }
        uint32_t od = bpref_is_null(oldr) ? 0 : bpref_deref(vm, oldr);   /* V4: fuente handle→addr */
        uint32_t old_len = (od == 0) ? 0 : bpvm_read_u32_be(vm->memory + od);
        uint32_t new_ref = bpvm_heap_alloc(vm, (uint32_t) new_cap * BPVM_REF_SIZE, BPVM_TYPE_ARRAY_REF);
        if (new_ref == 0) {
            tc->sp -= BPVM_REF_SIZE;
            return builtin_throw(vm, tc, "No space in heap");   /* H1: OOM atrapable */
        }
        od = bpref_is_null(oldr) ? 0 : bpref_deref(vm, oldr);   /* re-derivar tras la reserva */
        bpvm_write_u32_be(vm->memory + new_ref, (uint32_t) new_cap);
        uint32_t copy = (old_len < (uint32_t) new_cap) ? old_len : (uint32_t) new_cap;
        for (uint32_t i = 0; i < copy; i++) {   /* ref plana 8B/elem, vía la frontera de codificación */
            bpref_t e = bpref_load(vm, od + BPVM_ARR_DATA_OFF + i * BPVM_REF_SIZE);
            bpref_store(vm, new_ref + BPVM_ARR_DATA_OFF + i * BPVM_REF_SIZE, e);
        }
        tc->sp -= BPVM_REF_SIZE;   /* #350: ya copiado */
                bpref_push(vm, tc, bpvm_handle_register(vm, new_ref));
        return BPVM_OK;
    }

    case BUILTIN_GROW_INT_ARRAY: {
        int32_t new_cap = pop_i32(vm, tc);
        /* #350 — PEEK por higiene; ver el porqué completo en GROW_REF_ARRAY. */
        bpref_t oldr = peek_ref(vm, tc, 0);   /* V4: array ref = 8 bytes (era pop_i32 4B → drift vs miVM popTcRef) */
        if (new_cap < 0) { tc->sp -= BPVM_REF_SIZE; return BPVM_ERR_RUNTIME; }
        uint32_t od = bpref_is_null(oldr) ? 0 : bpref_deref(vm, oldr);   /* V4: fuente handle→addr */
        uint32_t old_len = (od == 0) ? 0 : bpvm_read_u32_be(vm->memory + od);
        uint32_t new_ref = bpvm_heap_alloc(vm, (uint32_t) new_cap * 4, BPVM_TYPE_ARRAY_I32);
        if (new_ref == 0) {
            tc->sp -= BPVM_REF_SIZE;
            return builtin_throw(vm, tc, "No space in heap");   /* H1: OOM atrapable */
        }
        od = bpref_is_null(oldr) ? 0 : bpref_deref(vm, oldr);   /* re-derivar tras la reserva */
        bpvm_write_u32_be(vm->memory + new_ref, (uint32_t) new_cap);
        uint32_t copy = (old_len < (uint32_t) new_cap) ? old_len : (uint32_t) new_cap;
        for (uint32_t i = 0; i < copy; i++) {
            uint32_t v = bpvm_read_u32_be(vm->memory + od + 4 + i * 4);
            bpvm_write_u32_be(vm->memory + new_ref + 4 + i * 4, v);
        }
        tc->sp -= BPVM_REF_SIZE;   /* #350: ya copiado */
                bpref_push(vm, tc, bpvm_handle_register(vm, new_ref));
        return BPVM_OK;
    }

    case BUILTIN_CHARS_TO_STRING: {
        int32_t len = pop_i32(vm, tc);
        /* #350 — PEEK: el SEGUNDO bucle vuelve a leer de `ca` tras alocar.
         * Como en fromBytes, sin repro que lo demuestre pero con la misma
         * violación de contrato. Las salidas de error sacan la fuente a mano
         * para no dejar la pila descuadrada. */
        bpref_t ca = peek_ref(vm, tc, 0);                  /* array i32 de codepoints (entrada) */
        if (len < 0) { tc->sp -= BPVM_REF_SIZE; return BPVM_ERR_RUNTIME; }
        uint32_t avail = bpref_arr_len(vm, ca);
        if ((uint32_t) len > avail) { tc->sp -= BPVM_REF_SIZE; return BPVM_ERR_RUNTIME; }
        /* H2: input = array i32 de codepoints; output = string byte[] UTF-8. */
        uint32_t total = 0;
        for (uint32_t i = 0; i < (uint32_t) len; i++) {
            uint32_t cp = bpvm_read_u32_be(bpref_arr_elem(vm, ca, i, 4));
            uint8_t tmp[4]; total += utf8_encode(cp, tmp);
        }
        uint32_t new_ref = bpvm_heap_alloc(vm, total, BPVM_TYPE_ARRAY_I8);
        if (new_ref == 0) {
            tc->sp -= BPVM_REF_SIZE;
            return builtin_throw(vm, tc, "No space in heap");   /* H1: OOM atrapable */
        }
        bpvm_write_u32_be(vm->memory + new_ref, total);   /* new_ref: alloc fresca (dirección física) */
        uint32_t w = 0;
        for (uint32_t i = 0; i < (uint32_t) len; i++) {
            uint32_t cp = bpvm_read_u32_be(bpref_arr_elem(vm, ca, i, 4));
            uint8_t enc[4]; uint32_t el = utf8_encode(cp, enc);
            for (uint32_t k = 0; k < el; k++) vm->memory[new_ref + 4 + w++] = enc[k];
        }
        tc->sp -= BPVM_REF_SIZE;   /* #350: ya copiado */
                bpref_push(vm, tc, bpvm_handle_register(vm, new_ref));
        return BPVM_OK;
    }

    case BUILTIN_TO_BYTES:
    case BUILTIN_FROM_BYTES: {
        /* H2 (V2): string y byte[] comparten layout (TYPE_ARRAY_I8). La
         * conversión es una copia defensiva (string inmutable / byte[]
         * mutable): mismos bytes, objeto nuevo. */
        /* #350 — PEEK: el bucle de copia lee de la fuente DESPUÉS de alocar.
         * Mismo contrato que substring (ver allí). Aquí NO está demostrado con
         * un repro —el que escribí no lo saca— pero la violación del contrato
         * es idéntica, así que se cierra igual. */
        bpref_t bref = peek_ref(vm, tc, 0);
        uint32_t rd = bpref_is_null(bref) ? 0 : bpref_deref(vm, bref);
        uint32_t n = (rd == 0) ? 0 : bpvm_read_u32_be(vm->memory + rd);
        uint32_t out = bpvm_heap_alloc(vm, n, BPVM_TYPE_ARRAY_I8);
        if (out == 0) {
            tc->sp -= BPVM_REF_SIZE;                 /* saca la fuente también al fallar */
            return builtin_throw(vm, tc, "No space in heap");   /* H1: OOM atrapable */
        }
        /* Re-derivar TRAS la reserva: hoy el mark-sweep no mueve objetos, pero
         * depender de eso es una trampa para el día que compacte. */
        rd = bpref_is_null(bref) ? 0 : bpref_deref(vm, bref);
        bpvm_write_u32_be(vm->memory + out, n);
        for (uint32_t i = 0; i < n; i++) vm->memory[out + 4 + i] = vm->memory[rd + 4 + i];
        tc->sp -= BPVM_REF_SIZE;   /* #350: ya copiado, ahora sí se saca */
                bpref_push(vm, tc, bpvm_handle_register(vm, out));   /* H1.2a: string ref result = 8 bytes */
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

    case BUILTIN_EVENT_RAISE: {
        /* Pila (de arriba abajo): recv(8B) dest(4B) nargs(4B) masks(4B) argN-1..arg0.
         * Los anchos de los argumentos los DICE el compilador en `masks`
         * (bits 8-11); la VM no los adivina. */
        bpref_t recv  = bpref_pop(vm, tc);
        int32_t dest  = pop_i32(vm, tc);
        int32_t nargs = pop_i32(vm, tc);
        uint32_t masks = (uint32_t) pop_i32(vm, tc);
        if (nargs < 0 || nargs > BPVM_EVENT_MAX_ARGS) {
            fprintf(stderr, "[bpvm] __eventRaise: aridad %d fuera de rango\n", (int) nargs);
            push_i32(vm, tc, 0);
            return BPVM_ERR_RUNTIME;
        }
        int64_t args[BPVM_EVENT_MAX_ARGS];
        for (int i = nargs - 1; i >= 0; i--) {           /* se desapilan al revés */
            if (masks & (1u << (8 + i))) { tc->sp -= 8; args[i] = bpvm_read_i64_be(vm->memory + tc->sp); }
            else                         { tc->sp -= 4; args[i] = (int64_t) bpvm_read_i32_be(vm->memory + tc->sp); }
        }
        /* Sin suscriptor no pasa nada: un evento es una NOTIFICACIÓN, y si
         * nadie escucha no hay error (decisión de diseño, modelo Swing). */
        if (!bpref_is_null(recv) && dest != 0)
            bpvm_event_enqueue(vm, tc->id, recv.v, dest, nargs, masks, args);
        push_i32(vm, tc, 0);   /* void */
        return BPVM_OK;
    }

    case BUILTIN_THREAD_START: {
        uint32_t thread_ref = pop_ref(vm, tc);
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
        uint32_t thread_ref = pop_ref(vm, tc);
        if (thread_ref == 0) {
            push_i32(vm, tc, 0); return BPVM_OK;
        }
        /* Convención: field[0] del Thread BP guarda el tid (escrito
         * por __threadStart). 0 = no spawneado todavía. */
        int32_t target_tid = bpvm_read_i32_be(vm->memory + ref_addr(vm, thread_ref) + 4 + 0 * 4);
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
        uint32_t mref = pop_ref(vm, tc);
        if (mref == 0) {
            push_i32(vm, tc, 0); return BPVM_ERR_NULL_RECEIVER;
        }
        int32_t mid = bpvm_read_i32_be(vm->memory + ref_addr(vm, mref) + 4 + 0 * 4);
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
        uint32_t mref = pop_ref(vm, tc);
        if (mref == 0) {
            push_i32(vm, tc, 0); return BPVM_ERR_NULL_RECEIVER;
        }
        int32_t mid = bpvm_read_i32_be(vm->memory + ref_addr(vm, mref) + 4 + 0 * 4);
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
        uint32_t dataRef = pop_ref(vm, tc);
        uint32_t dataAddr = ref_addr(vm, dataRef);   /* V4: handle→addr */
        int addr = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        uint8_t buf[64];
        int n = count > (int) sizeof(buf) ? (int) sizeof(buf) : count;
        for (int i = 0; i < n; i++) {
            int32_t v = bpvm_read_i32_be(vm->memory + dataAddr + 4 + i * 4);
            buf[i] = (uint8_t)(v & 0xFF);
        }
        int wrote = bpvm_i2c_write(bus, addr, buf, (size_t) n);
        push_i32(vm, tc, (int32_t) wrote);
        return BPVM_OK;
    }
    case BUILTIN_I2C_READ: {
        /* read(bus, addr, data: integer[], count): integer */
        int count = pop_i32(vm, tc);
        uint32_t dataRef = pop_ref(vm, tc);
        uint32_t dataAddr = ref_addr(vm, dataRef);   /* V4: handle→addr */
        int addr = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        uint8_t buf[64];
        int n = count > (int) sizeof(buf) ? (int) sizeof(buf) : count;
        int got = bpvm_i2c_read(bus, addr, buf, (size_t) n);
        if (got > 0) {
            for (int i = 0; i < got; i++) {
                bpvm_write_i32_be(vm->memory + dataAddr + 4 + i * 4,
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
        uint32_t dstRef  = pop_ref(vm, tc);
        uint32_t srcRef  = pop_ref(vm, tc);
        if (count <= 0 || srcRef == 0 || dstRef == 0) {
            push_i32(vm, tc, 0);
            return BPVM_OK;
        }
        /* Tamaño de slot por tipo. Para v1 asumimos I32 (caso común
         * con integer[] y string). Para int8/int16 habría que mirar
         * el tag — F3 lo añade. */
        uint32_t slot = 4;
        uint32_t dd = bpref_deref(vm, bpref_from_addr(dstRef));   /* V4: handle→addr */
        uint32_t sd = bpref_deref(vm, bpref_from_addr(srcRef));
        memmove(vm->memory + dd + 4 + (uint32_t) dstStart * slot,
                vm->memory + sd + 4 + (uint32_t) srcStart * slot,
                (size_t) count * slot);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    case BUILTIN_NEW_INT_ARRAY: {
        int32_t size = pop_i32(vm, tc);
        if (size < 0) return BPVM_ERR_RUNTIME;
        uint32_t bytes = (uint32_t) size * 4u;
        uint32_t ref = bpvm_heap_alloc(vm, bytes, BPVM_TYPE_ARRAY_I32);
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");   /* H1: OOM atrapable */
        bpvm_write_u32_be(vm->memory + ref, (uint32_t) size);
        /* bpvm_heap_alloc ya zero-init (memset en heap.c). */
                bpref_push(vm, tc, bpvm_handle_register(vm, ref));
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
        uint32_t dataRef = pop_ref(vm, tc);
        uint32_t dataAddr = ref_addr(vm, dataRef);   /* V4: handle→addr */
        int bus = pop_i32(vm, tc);
        uint8_t buf[256];
        int n = count > (int) sizeof(buf) ? (int) sizeof(buf) : count;
        for (int i = 0; i < n; i++) {
            int32_t v = bpvm_read_i32_be(vm->memory + dataAddr + 4 + i * 4);
            buf[i] = (uint8_t)(v & 0xFF);
        }
        int wrote = bpvm_spi_write(bus, buf, (size_t) n);
        push_i32(vm, tc, (int32_t) wrote);
        return BPVM_OK;
    }
    case BUILTIN_SPI_READ: {
        int count = pop_i32(vm, tc);
        uint32_t dataRef = pop_ref(vm, tc);
        uint32_t dataAddr = ref_addr(vm, dataRef);   /* V4: handle→addr */
        int bus = pop_i32(vm, tc);
        uint8_t buf[256];
        int n = count > (int) sizeof(buf) ? (int) sizeof(buf) : count;
        int got = bpvm_spi_read(bus, buf, (size_t) n);
        if (got > 0) {
            for (int i = 0; i < got; i++) {
                bpvm_write_i32_be(vm->memory + dataAddr + 4 + i * 4, (int32_t) buf[i]);
            }
        }
        push_i32(vm, tc, (int32_t) got);
        return BPVM_OK;
    }
    case BUILTIN_SPI_TRANSFER: {
        int count = pop_i32(vm, tc);
        uint32_t rxRef = pop_ref(vm, tc);
        uint32_t rxAddr = ref_addr(vm, rxRef);   /* V4: handle→addr */
        uint32_t txRef = pop_ref(vm, tc);
        uint32_t txAddr = ref_addr(vm, txRef);   /* V4: handle→addr */
        int bus = pop_i32(vm, tc);
        uint8_t txBuf[256], rxBuf[256];
        int n = count > (int) sizeof(txBuf) ? (int) sizeof(txBuf) : count;
        for (int i = 0; i < n; i++) {
            int32_t v = bpvm_read_i32_be(vm->memory + txAddr + 4 + i * 4);
            txBuf[i] = (uint8_t)(v & 0xFF);
        }
        int xchg = bpvm_spi_transfer(bus, txBuf, rxBuf, (size_t) n);
        if (xchg > 0) {
            for (int i = 0; i < xchg; i++) {
                bpvm_write_i32_be(vm->memory + rxAddr + 4 + i * 4, (int32_t) rxBuf[i]);
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
        uint32_t dataRef = pop_ref(vm, tc);
        uint32_t dataAddr = ref_addr(vm, dataRef);   /* V4: handle→addr */
        int bus = pop_i32(vm, tc);
        uint8_t buf[256];
        int n = count > (int) sizeof(buf) ? (int) sizeof(buf) : count;
        for (int i = 0; i < n; i++) {
            int32_t v = bpvm_read_i32_be(vm->memory + dataAddr + 4 + i * 4);
            buf[i] = (uint8_t)(v & 0xFF);
        }
        int wrote = bpvm_uart_write(bus, buf, (size_t) n);
        push_i32(vm, tc, (int32_t) wrote);
        return BPVM_OK;
    }
    case BUILTIN_UART_READ: {
        int timeout = pop_i32(vm, tc);
        int count   = pop_i32(vm, tc);
        uint32_t dataRef = pop_ref(vm, tc);
        uint32_t dataAddr = ref_addr(vm, dataRef);   /* V4: handle→addr */
        int bus = pop_i32(vm, tc);
        uint8_t buf[256];
        int n = count > (int) sizeof(buf) ? (int) sizeof(buf) : count;
        int got = bpvm_uart_read(bus, buf, (size_t) n, timeout);
        if (got > 0) {
            for (int i = 0; i < got; i++) {
                bpvm_write_i32_be(vm->memory + dataAddr + 4 + i * 4, (int32_t) buf[i]);
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
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }
    case BUILTIN_PICO_BOARD_NAME: {
        char buf[32];
        bpvm_pico_board_name(buf, sizeof(buf));
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, strlen(buf));
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }
    case BUILTIN_PICO_RESET_CAUSE: {   /* H10 — causa del último reset (string) */
        const char* s = bpvm_pico_reset_cause();
        uint32_t ref = bpvm_heap_alloc_string(vm, s, strlen(s));
        push_ref(vm, tc, ref);
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
    case BUILTIN_PICO_ADC_CHANNELS: {
        push_i32(vm, tc, (int32_t) bpvm_pico_adc_channels());
        return BPVM_OK;
    }
    case BUILTIN_PICO_PWM_SLICES: {
        push_i32(vm, tc, (int32_t) bpvm_pico_pwm_slices());
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
        uint32_t grbRef = pop_ref(vm, tc);
        uint32_t grbAddr = ref_addr(vm, grbRef);   /* V4: handle→addr */
        int pin = pop_i32(vm, tc);
        /* #338 — el buffer de transcodificación (BE del heap BP → nativo para
         * el driver) sale de la zona compartida: 1 KB que sólo hace falta
         * mientras se empuja una trama, no toda la vida del firmware.
         *
         * Soltarlo aquí mismo es SEGURO porque el driver es BLOQUEANTE: el
         * backend del Pico mete las palabras con pio_sm_put_blocking y espera
         * los 60 µs de latch del WS2812, así que al volver el buffer ya está
         * consumido. Si algún día un backend lo hiciera por DMA habría que
         * retener la zona hasta que termine — está dicho aquí para que no se
         * descubra por una tira parpadeando en colores raros.
         *
         * De propina se cierra un latente: esto era `static` con el comentario
         * "single-worker: estático OK". Bajo SMP dos workers llamando a la vez
         * se pisaban EN SILENCIO; ahora el segundo se encuentra la zona cogida
         * y el guardián lo dice con nombres. */
        enum { NP_MAX = 256 };
        uint32_t* npbuf = (uint32_t*) bpvm_scratch_take(NP_MAX * sizeof(uint32_t),
                                                        "__npShow");
        if (!npbuf) {          /* el guardián ya ha dicho por qué */
            push_i32(vm, tc, 0);
            return BPVM_OK;
        }
        int n = count < 0 ? 0 : (count > NP_MAX ? NP_MAX : count);
        for (int i = 0; i < n; i++) {
            npbuf[i] = (uint32_t) bpvm_read_i32_be(vm->memory + grbAddr + 4 + i * 4);
        }
        bpvm_neopixel_show(pin, npbuf, n);
        bpvm_scratch_give("__npShow");
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
