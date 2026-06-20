/*
 * Catálogo de funciones built-in invocables por bytecode con CALL_BUILTIN id.
 * El ID es el `ordinal()` del enum, así que el ORDEN ES ESTABLE: añadir nuevos
 * built-ins al final.
 */
package edu.bpgenvm.bytecode;

import java.util.HashMap;
import java.util.Map;

public enum Builtin {
    // Strings utilitarios
    STRLEN("strlen"),
    PARSE_INT("parseInt"),
    PARSE_FLOAT("parseFloat"),
    INT_TO_STRING("intToString"),
    FLOAT_TO_STRING("floatToString"),
    BOOL_TO_STRING("boolToString"),
    UPPER("upper"),
    LOWER("lower"),
    TRIM("trim"),
    SUBSTRING("substring"),
    INDEX_OF("indexOf"),
    STARTS_WITH("startsWith"),
    ENDS_WITH("endsWith"),
    CONTAINS("contains"),
    CHAR_AT("charAt"),
    REPLACE("replace"),

    // Numéricas enteras
    ABS("abs"),
    MIN("min"),
    MAX("max"),

    // Numéricas float
    SQRT("sqrt"),
    POW("pow"),
    LOG("log"),
    LOG10("log10"),
    EXP("exp"),
    SIN("sin"),
    COS("cos"),
    TAN("tan"),
    RANDOM("random"),
    RANDOM_INT("randomInt"),

    // Constantes
    PI("pi"),
    E("e"),

    // Conversión float→int
    FLOOR("floor"),
    CEIL("ceil"),
    ROUND("round"),

    // Tiempo
    NOW("now"),
    SLEEP("sleep"),

    // Strings que devuelven arrays
    SPLIT("split"),

    // I/O
    INPUT("input"),
    READ_FILE("readFile"),
    WRITE_FILE("writeFile"),
    APPEND_FILE("appendFile"),
    FILE_EXISTS("fileExists"),
    LIST_DIR("listDir"),

    // Debug
    GC("gc"),

    // ---- Soporte para clases stdlib (List, StringBuilder). Los nombres
    //      empiezan por __ para señalar uso interno (aunque cualquier código
    //      BP puede llamarlos también). ----
    NEW_REF_ARRAY("__newRefArray"),      // (cap)        → array TYPE_ARRAY_REF (slots a 0)
    GROW_REF_ARRAY("__growRefArray"),    // (old, newCap)→ array TYPE_ARRAY_REF copiado
    GROW_INT_ARRAY("__growIntArray"),    // (old, newCap)→ array TYPE_ARRAY_I32 copiado
    CHARS_TO_STRING("__charsToString"),  // (chars, len) → string heap-allocado
    CHAR_CODE_AT("charCodeAt"),          // (s, i)       → integer (code point del char i)

    // ---- Threading ----
    THREAD_START("__threadStart"),       // (threadRef)  → void; spawnea + arranca run() del thread
    THREAD_JOIN("__threadJoin"),         // (threadRef)  → void; bloquea hasta que termine
    YIELD("yield"),                      // ()           → void; cede CPU al scheduler

    // ---- Sync ----
    MUTEX_CREATE("__mutexCreate"),       // ()           → integer (id de mutex en VM)
    MUTEX_LOCK("__mutexLock"),           // (mutexRef)   → void; bloquea si está tomado
    MUTEX_UNLOCK("__mutexUnlock"),       // (mutexRef)   → void; libera; despierta primer waiter

    // ---- Arrays ----
    MOVE("move"),                        // (src,dst,srcStart,dstStart,count) → void
                                         //   copia `count` elementos. Soporta overlapping
                                         //   en el mismo array. RuntimeError si:
                                         //   - src/dst null o no son arrays;
                                         //   - tipos de array distintos (i8/i16/i32/ref);
                                         //   - count negativo o rango fuera de los arrays.

    // ---- Math (intrínsecos accesibles vía `import Math`) ----
    SIGN_I("__sign_i"),                  // (i: int)   → int   (-1/0/1)
    SIGN_F("__sign_f"),                  // (f: float) → int   (-1/0/1)
    ASIN("__asin"),                      // (f: float) → float
    ACOS("__acos"),                      // (f: float) → float
    ATAN("__atan"),                      // (f: float) → float
    ATAN2("__atan2"),                    // (y: float, x: float) → float
    FACTORIAL_I("__factorial_i"),        // (n: int)   → int    (n>=0 y resultado i32)
    GAMMA_F("__gamma_f"),                // (x: float) → float  (Lanczos)

    // ---- IO (intrínsecos accesibles vía `import IO`) ----
    PATH_JOIN("__pathJoin"),             // (a, b)        → string
    PATH_PARENT("__pathParent"),         // (p: string)   → string (puede ser "")
    PATH_BASENAME("__pathBasename"),     // (p: string)   → string
    PATH_EXTENSION("__pathExtension"),   // (p: string)   → string (sin punto, "" si no hay)
    PATH_ABSOLUTE("__pathAbsolute"),     // (p: string)   → string
    MKDIR("__mkdir"),                    // (p: string)   → void  (crea dirs intermedios)
    RMDIR("__rmdir"),                    // (p: string)   → void  (sólo si vacío)
    REMOVE_FILE("__removeFile"),         // (p: string)   → void
    RENAME("__rename"),                  // (from, to)    → void
    COPY_FILE("__copyFile"),             // (from, to)    → void
    FILE_SIZE("__fileSize"),             // (p: string)   → int   (bytes)
    IS_DIRECTORY("__isDirectory"),       // (p: string)   → boolean
    LAST_MODIFIED("__lastModified"),     // (p: string)   → int   (epoch ms truncado i32)

    // ---- N20 — UI via IDE conectado ----
    PROMPT("__prompt"),                  // (spec: string) → string
                                         //   Envía formRequest al IDE, bloquea el
                                         //   thread BP hasta respuesta. Devuelve
                                         //   el JSON con los valores. Si no hay
                                         //   IDE, RuntimeError BP atrapable.

    // ---- Gpio — control de pines ----
    GPIO_INIT("__gpioInit"),             // (pin: int, mode: int) → void
                                         //   mode: 0=INPUT, 1=OUTPUT
    GPIO_WRITE("__gpioWrite"),           // (pin: int, value: int) → void
                                         //   value: 0 (LOW) o !=0 (HIGH)
    GPIO_READ("__gpioRead"),             // (pin: int) → int   (0 o 1)
    GPIO_PULL("__gpioPull"),             // (pin: int, pull: int) → void
                                         //   pull: 0=none, 1=up, 2=down

    // ---- I2C — bus serial síncrono multi-slave ----
    I2C_INIT("__i2cInit"),               // (bus, sda, scl, baudrate) → void
                                         //   bus: 0 o 1
    I2C_WRITE("__i2cWrite"),             // (bus, addr, data: int[], n) → int
                                         //   bytes escritos; -1 en error
    I2C_READ("__i2cRead"),               // (bus, addr, data: int[], n) → int
                                         //   bytes leídos; -1 en error

    // ---- Allocators dinámicos accesibles desde user BP ----
    NEW_INT_ARRAY("newIntArray"),        // (size: int) → integer[] (zeros)

    // ---- SPI — bus serial síncrono full-duplex master ----
    SPI_INIT("__spiInit"),               // (bus, sck, mosi, miso, baudrate, mode) → void
    SPI_WRITE("__spiWrite"),             // (bus, data: int[], n) → int (bytes escritos)
    SPI_READ("__spiRead"),               // (bus, data: int[], n) → int (bytes leídos; envía 0xFF dummy)
    SPI_TRANSFER("__spiTransfer"),       // (bus, tx: int[], rx: int[], n) → int (bytes intercambiados)

    // ---- UART — bus serial asíncrono punto a punto ----
    UART_INIT("__uartInit"),             // (bus, tx, rx, baud, dataBits, stopBits, parity) → void
                                         //   bus: 0=uart0, 1=uart1
                                         //   dataBits: 5..8 (típico 8)
                                         //   stopBits: 1 ó 2
                                         //   parity:   0=NONE, 1=ODD, 2=EVEN
    UART_WRITE("__uartWrite"),           // (bus, data: int[], n) → int (bytes escritos; bloquea hasta vaciar TX)
    UART_READ("__uartRead"),             // (bus, data: int[], n, timeoutMs) → int
                                         //   lee hasta n bytes; devuelve los bytes recibidos
                                         //   (puede ser menos que n si expira el timeout).
                                         //   timeoutMs<=0 bloquea hasta tener todos.
    UART_AVAILABLE("__uartAvailable"),   // (bus) → int (bytes disponibles para leer sin bloquear)

    // ---- Pulse counter — contador de pulsos hardware en un GPIO ----
    PULSE_INIT("__pulseInit"),           // (pin, edgeKind) → int counterId, o -1 si pin inválido.
                                         //   edgeKind: 0=RISING, 1=FALLING, 2=BOTH.
                                         //   En RP2350 usa el slice PWM correspondiente al pin
                                         //   en modo "input gate edge counting" — pin debe ser
                                         //   un canal B de slice (impares: GP1,3,5,7,9,11,...).
    PULSE_START("__pulseStart"),         // (counterId) → void
    PULSE_STOP("__pulseStop"),           // (counterId) → void
    PULSE_VALUE("__pulseValue"),         // (counterId) → int (cuenta actual, 0..65535 en RP2350)
    PULSE_RESET("__pulseReset"),         // (counterId) → void

    // ---- PWM — generación de señal hardware ----
    PWM_INIT("__pwmInit"),               // (pin, freqHz) → int sliceId, o -1 si pin inválido.
                                         //   El pin determina el slice (cualquier canal A/B sirve).
                                         //   freqHz: objetivo. El backend calcula clkdiv+wrap.
    PWM_SET_FREQ("__pwmSetFreq"),        // (sliceId, freqHz) → void
    PWM_SET_DUTY("__pwmSetDuty"),        // (sliceId, pin, dutyPct) → void
                                         //   pin para escoger canal A o B del slice.
                                         //   dutyPct: 0..100.
    PWM_START("__pwmStart"),             // (sliceId) → void
    PWM_STOP("__pwmStop"),               // (sliceId) → void

    // ---- Pico (info del microcontrolador) ----
    PICO_UNIQUE_ID("__picoUniqueId"),    // () → string (16 hex chars del flash chip ID)
    PICO_BOARD_NAME("__picoBoardName"),  // () → string ("pico2" / "host" / ...)
    PICO_TEMP_C("__picoTempC"),          // () → float  (°C del sensor interno)
    PICO_CPU_FREQ_HZ("__picoCpuFreqHz"), // () → integer (Hz del clk_sys)
    PICO_UPTIME_MS("__picoUptimeMs"),    // () → integer (ms desde boot, 32-bit wrap)

    // ---- Time (variantes de sleep) ----
    SLEEP_SEC("sleepSec"),               // (s: integer) → void
                                         //   Cede el thread BP al scheduler durante s segundos.
                                         //   Equivalente a sleep(s * 1000) pero más legible
                                         //   para pausas largas.
    SLEEP_US("sleepUs"),                 // (us: integer) → void
                                         //   Busy-wait que NO cede el thread BP. Para pausas
                                         //   tan cortas (< 1 ms) el coste de un context switch
                                         //   supera la pausa misma. Útil para timing crítico
                                         //   (setup/hold de chips, bit-banging de protocolos
                                         //   rápidos, etc.). En multi-thread el resto de
                                         //   threads BP NO corren durante el wait.

    PICO_SET_CPU_FREQ_MHZ("__picoSetCpuFreqMHz"), // (mhz: int) → boolean
                                         //   Cambia el clk_sys del RP2350 a la frecuencia
                                         //   pedida en MHz. Si mhz > MAX_CPU_MHZ se aplica
                                         //   MAX_CPU_MHZ (clamp). Devuelve true si la PLL
                                         //   pudo configurarse, false en caso contrario.
                                         //   Afecta a periféricos derivados de clk_peri
                                         //   (UART/SPI/I2C/PWM): llamar ANTES de
                                         //   configurarlos. Las funciones sleep* NO se ven
                                         //   afectadas (timer HW corre a 1 MHz independiente).

    // ---- Rtc — wall clock con sincronización IDE→Pico ----
    RTC_NOW_SEC("__rtcNowSec"),          // () → integer
                                         //   Segundos desde epoch Unix (1970-01-01 UTC).
                                         //   Si no se ha sincronizado, devuelve segundos
                                         //   desde el boot del firmware/JVM.
    RTC_SET_NOW_SEC("__rtcSetNowSec"),   // (sec: integer) → void
                                         //   Calibra el reloj: a partir de aquí
                                         //   nowSec() devuelve sec + tiempo transcurrido
                                         //   desde esta llamada.

    // ---- Adc — 4 canales ADC del RP2350 (CH0..CH3 → GP26..GP29) ----
    ADC_INIT_CHANNEL("__adcInitChannel"),// (ch: int) → int (pin físico, -1 si error)
    ADC_READ_CHANNEL("__adcReadChannel"),// (ch: int) → int (raw 0..4095)

    // ---- Wdt — watchdog timer singleton del MCU ----
    WDT_ENABLE("__wdtEnable"),           // (timeoutMs: int) → void
    WDT_FEED("__wdtFeed"),               // () → void
    WDT_DISABLE("__wdtDisable"),         // () → void
                                         //   En RP2350 "disable" se aproxima con un
                                         //   timeout muy grande (no hay way real de
                                         //   deshabilitar el watchdog una vez activado).
                                         //   Cambia el clk_sys del RP2350 a la frecuencia
                                         //   pedida en MHz. Si mhz > MAX_CPU_MHZ se aplica
                                         //   MAX_CPU_MHZ (clamp). Devuelve true si la PLL
                                         //   pudo configurarse, false en caso contrario.
                                         //   Afecta a periféricos derivados de clk_peri
                                         //   (UART/SPI/I2C/PWM): llamar ANTES de
                                         //   configurarlos. Las funciones sleep* NO se ven
                                         //   afectadas (timer HW corre a 1 MHz independiente).

    // ---- H2 (V2) — conversión string <-> byte[] (ambos TYPE_ARRAY_I8;
    //      copia defensiva por la inmutabilidad del string) ----
    TO_BYTES("toBytes"),                 // (s: string) → byte[]
    FROM_BYTES("fromBytes"),             // (b: byte[]) → string

    // ---- H3 — diagnóstico de heap (SOLO VM-Java; la VM C no las implementa) ----
    HEAP_FRAG("heapFrag"),               // () → string  (resumen de fragmentación)
    HEAP_MAP("heapMap"),                 // (cols: int) → string (mapa ASCII)

    // ---- H7.3 — board-aware (RP2350A/B). El device lo resuelve desde el
    //      board_desc (variante/board.json); host = perfil RP2350A. ----
    PICO_GPIO_COUNT("__picoGpioCount"),  // () → integer (GPIO de la variante)

    // ---- H7.4 — NeoPixel WS2812 (device-only vía PIO; no-op en host) ----
    NEOPIXEL_INIT("__npInit"),           // (pin) → void
    NEOPIXEL_SHOW("__npShow"),            // (pin, grb: int[], count) → void

    // ---- H10/#247 — file I/O BINARIO (byte[]). Mismos bytes en heap que
    //      readFile/writeFile (TYPE_ARRAY_I8), pero el frontend los tipa byte[]
    //      y la VM-Java NO los pasa por String → binario sin pérdidas (NUL, >127,
    //      UTF-8 inválido). Es lo que necesita la descompresión (#240). Ids al
    //      FINAL (126/127) para no desplazar ordinales existentes. ----
    READ_FILE_BYTES("readFileBytes"),    // (path: string)              → byte[]
    WRITE_FILE_BYTES("writeFileBytes"),  // (path: string, data: byte[]) → void

    // ---- #248 — id 128, al FINAL. ----
    THROW_RTE("__throwRte"),             // (msg: string) → no retorna; lanza el
                                         //   RuntimeError NATIVO de la VM (mismo
                                         //   path que div0/null deref). Lo usa el
                                         //   compareTo por defecto de Object para
                                         //   no depender del descriptor local de
                                         //   RuntimeError (que #248 retira).

    // ---- L13 — ids 129/130: concat string + long/double. ----
    LONG_TO_STRING("__longToString"),    // (v: long)   → string (decimal)
    DOUBLE_TO_STRING("__doubleToString"),// (v: double) → string (formateo
                                         //   canónico GAP-4 — el mismo de
                                         //   DPRINT, byte-idéntico en C)

    // ---- H11 (#241) — ids 131-134: cliente TCP simple (módulo Net). ----
    TCP_CONNECT("__tcpConnect"),         // (host, port, timeoutMs) → handle (0 = no conectado)
    TCP_SEND("__tcpSend"),               // (h, data: byte[]) → n enviados; error → RuntimeError
    TCP_RECV("__tcpRecv"),               // (h, max, timeoutMs) → byte[] (vacío = timeout)
    TCP_CLOSE("__tcpClose"),             // (h) → void, tolerante

    // ---- H3 (V3) — GUI (módulo Gui). Las clases Gui.* son bindings sobre un
    //      `handle` (id del lv_obj). El render/transporte vive en el backend
    //      (Swing en miVM, LVGL en VM-C); estos builtins son la frontera
    //      BP↔backend. Ids al FINAL (aditivo, principio 7 — no mueven los V2). ----
    GUI_SCREEN_ACTIVE("__guiScreenActive"),  // () → id de la pantalla raíz
    GUI_CREATE_OBJ("__guiCreateObj"),        // (parent) → id  (Panel/contenedor)
    GUI_CREATE_LABEL("__guiCreateLabel"),    // (parent) → id
    GUI_CREATE_BUTTON("__guiCreateButton"),  // (parent) → id
    GUI_SET_TEXT("__guiSetText"),            // (id, s: string) → void
    GUI_SET_WIDTH("__guiSetWidth"),          // (id, w) → void
    GUI_SET_HEIGHT("__guiSetHeight"),        // (id, h) → void
    GUI_ALIGN("__guiAlign"),                 // (id, align, dx, dy) → void
    GUI_SET_BG_COLOR("__guiSetBgColor"),     // (id, rgb) → void
    GUI_SET_TEXT_COLOR("__guiSetTextColor"), // (id, rgb) → void
    GUI_SET_FONT("__guiSetFont"),            // (id, fontId) → void
    GUI_CLEAN("__guiClean"),                 // (id) → void
    GUI_DELETE("__guiDelete"),               // (id) → void
    GUI_SCREEN_LOAD("__guiScreenLoad"),      // (id) → void
    GUI_RUN("__guiRun"),                     // () → void; lazo de eventos (bloquea)
    GUI_DUMP_TREE("__guiDumpTree"),          // () → string; volcado del árbol (paridad)

    // H3.4 — eventos (upcall). BIND_CLICK registra handle→objptr; CLICK inyecta
    // un clic sintetico (diagnostico/pruebas). El dispatch onClick lo hace la VM
    // llamando por NOMBRE a la funcion BP Gui.__guiDispatch (no es builtin).
    GUI_BIND_CLICK("__guiBindClick"),        // (handle, self) → void
    GUI_CLICK("__guiClick"),                 // (handle) → void; clic sintetico

    // V3/H6 — geometría (backend = la verdad: get/set → intrínsecos), scroll
    // (opt-in, default NONE) y refresh (invalidar). Ids al final (aditivo).
    GUI_SET_X("__guiSetX"),                  // (id, x) → void
    GUI_GET_X("__guiGetX"),                  // (id) → x
    GUI_SET_Y("__guiSetY"),                  // (id, y) → void
    GUI_GET_Y("__guiGetY"),                  // (id) → y
    GUI_GET_WIDTH("__guiGetWidth"),          // (id) → w
    GUI_GET_HEIGHT("__guiGetHeight"),        // (id) → h
    GUI_SET_SCROLL_DIR("__guiSetScrollDir"), // (id, dir) → void
    GUI_GET_SCROLL_DIR("__guiGetScrollDir"), // (id) → dir
    GUI_REFRESH("__guiRefresh"),             // (id) → void

    // H6 widgets — checkbox (1er value-widget): create + checked + change.
    GUI_CREATE_CHECKBOX("__guiCreateCheckbox"), // (parent) → id
    GUI_SET_CHECKED("__guiSetChecked"),         // (id, bool) → void
    GUI_GET_CHECKED("__guiGetChecked"),         // (id) → bool
    GUI_CHANGE("__guiChange"),                  // (id) → void; cambio sintético

    // H6 widgets — value-widgets enteros (switch booleano + slider/bar enteros).
    GUI_CREATE_SWITCH("__guiCreateSwitch"),     // (parent) → id
    GUI_CREATE_SLIDER("__guiCreateSlider"),     // (parent) → id
    GUI_CREATE_BAR("__guiCreateBar"),           // (parent) → id
    GUI_SET_VALUE("__guiSetValue"),             // (id, int) → void (clampa al rango)
    GUI_GET_VALUE("__guiGetValue"),             // (id) → int
    GUI_SET_RANGE("__guiSetRange"),             // (id, min, max) → void

    // H6 widgets — spinbox (entero+rango como slider) + led (indicador on/off).
    GUI_CREATE_SPINBOX("__guiCreateSpinbox"),   // (parent) → id
    GUI_CREATE_LED("__guiCreateLed"),           // (parent) → id

    // H6 widgets — dropdown (opciones + índice sel.) + textarea (texto editable).
    GUI_CREATE_DROPDOWN("__guiCreateDropdown"), // (parent) → id
    GUI_SET_OPTIONS("__guiSetOptions"),         // (id, "a\nb\nc") → void
    GUI_CREATE_TEXTAREA("__guiCreateTextarea"), // (parent) → id
    GUI_GET_TEXT("__guiGetText"),               // (id) → string

    // H6 widgets — list (ítems + índice, reusa options/value) + keyboard.
    GUI_CREATE_LIST("__guiCreateList"),         // (parent) → id
    GUI_CREATE_KEYBOARD("__guiCreateKeyboard"), // (parent) → id
    GUI_KEYBOARD_SET_TEXTAREA("__guiKeyboardSetTextarea"), // (id, taId) → void

    // H6 widgets — msgbox (aviso async: mensaje + botones, resultado por onChange).
    GUI_CREATE_MSGBOX("__guiCreateMsgbox"),     // (parent) → id
    GUI_SET_BUTTONS("__guiSetButtons"),         // (id, "Si\nNo") → void

    // H6 widgets — tabview (pestañas; addTab devuelve el handle de la página).
    GUI_CREATE_TABVIEW("__guiCreateTabview"),   // (parent) → id
    GUI_TABVIEW_ADD_TAB("__guiTabviewAddTab");  // (id, name) → pageId

    public final String bpName;
    public final int id;

    Builtin(String bpName) { this.bpName = bpName; this.id = ordinal(); }

    private static final Map<String, Builtin> BY_NAME = new HashMap<>();
    static { for (Builtin b : values()) BY_NAME.put(b.bpName, b); }

    public static Builtin byName(String name) { return BY_NAME.get(name); }
    public static Builtin byId(int id) { return values()[id]; }
}
