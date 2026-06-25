// ============================================================
// Intrinsics.java
// Registro de funciones `intrinsic` exportadas por módulos stdlib.
//
// Una función intrinsic vive en un .bpi como signature, pero NO tiene
// código en el .mod dueño. Cuando otro módulo la llama, el emisor mete
// los opcodes inline en el call-site (en lugar de generar CALL_EXT). El
// resultado es que `Math.sign(x)` se compila a una secuencia corta de
// opcodes — efectivamente la misma forma que tendrían los builtins
// implícitos como `abs(x)`, pero con namespace.
//
// El frontend invoca `Intrinsics.lookup("Math.sign")` en el call-site
// (cuando detecta que la FunctionSymbol resuelta tiene isIntrinsic=true).
// Los lambdas asumen que los argumentos YA están en pila en el orden de
// declaración (último arg en top), igual que para CALL/CALL_EXT.
//
// Para añadir uno nuevo:
//   1. Añade la signature en el .bp del módulo dueño con `public intrinsic
//      function nombre(...): tipo`.
//   2. Si necesita un opcode VM nuevo, añádelo a Builtin.java y
//      VirtualMachine.dispatchBuiltin().
//   3. Registra un emitter aquí: `register("Modulo.nombre", w -> {...})`.
// ============================================================
package basicplus.frontend;

import edu.bpgenvm.bytecode.Builtin;
import edu.bpgenvm.bytecode.OpCode;
import edu.bpgenvm.generador.ModWriter;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

public final class Intrinsics {
    private Intrinsics() {}

    @FunctionalInterface
    public interface IntrinsicEmitter {
        /** Args ya están en pila (orden de declaración, último en top).
         *  La implementación debe dejar exactamente UN valor en pila —
         *  el return value (dummy 0 si la función es void), igual que
         *  CALL_BUILTIN. El emitter del statement se encarga del POP. */
        void emit(ModWriter w) throws IOException;
    }

    private static final Map<String, IntrinsicEmitter> REGISTRY = new HashMap<>();

    /** Helper: emit CALL_BUILTIN <id>. */
    private static void emitBuiltin(ModWriter w, Builtin b) throws IOException {
        w.emit(OpCode.CALL_BUILTIN);
        w.emitShort((short) b.id);
    }

    private static void register(String qualifiedName, IntrinsicEmitter em) {
        if (REGISTRY.containsKey(qualifiedName))
            throw new IllegalStateException("intrinsic duplicado: " + qualifiedName);
        REGISTRY.put(qualifiedName, em);
    }

    public static boolean isKnown(String qualifiedName) {
        return REGISTRY.containsKey(qualifiedName);
    }

    public static IntrinsicEmitter lookup(String qualifiedName) {
        return REGISTRY.get(qualifiedName);
    }

    // ============================================================
    // Registro
    // ============================================================
    static {
        // ---- Math ----
        // Constantes (sin args): cada llamada empuja el valor.
        register("Math.pi",       w -> emitBuiltin(w, Builtin.PI));
        register("Math.e",        w -> emitBuiltin(w, Builtin.E));
        // Trig directa.
        register("Math.sin",      w -> emitBuiltin(w, Builtin.SIN));
        register("Math.cos",      w -> emitBuiltin(w, Builtin.COS));
        register("Math.tan",      w -> emitBuiltin(w, Builtin.TAN));
        // Logaritmos. ln(x) reusa el builtin LOG (que ya es log natural en la VM).
        register("Math.ln",       w -> emitBuiltin(w, Builtin.LOG));
        register("Math.log10",    w -> emitBuiltin(w, Builtin.LOG10));
        // sign(x: integer): integer  →  builtin SIGN_I
        register("Math.sign",     w -> emitBuiltin(w, Builtin.SIGN_I));
        // signF(x: float): integer →  builtin SIGN_F
        register("Math.signF",    w -> emitBuiltin(w, Builtin.SIGN_F));
        // asin / acos / atan / atan2
        register("Math.asin",     w -> emitBuiltin(w, Builtin.ASIN));
        register("Math.acos",     w -> emitBuiltin(w, Builtin.ACOS));
        register("Math.atan",     w -> emitBuiltin(w, Builtin.ATAN));
        register("Math.atan2",    w -> emitBuiltin(w, Builtin.ATAN2));
        // factorial(n: integer): integer  → producto exacto. Error en runtime si n<0
        //   o si el resultado se desborda i32 (n>12).
        register("Math.factorial",w -> emitBuiltin(w, Builtin.FACTORIAL_I));
        // gamma(x: float): float — factorial real (Lanczos approximation).
        register("Math.gamma",    w -> emitBuiltin(w, Builtin.GAMMA_F));

        // ---- IO ----
        register("IO.pathJoin",     w -> emitBuiltin(w, Builtin.PATH_JOIN));
        register("IO.pathParent",   w -> emitBuiltin(w, Builtin.PATH_PARENT));
        register("IO.pathBasename", w -> emitBuiltin(w, Builtin.PATH_BASENAME));
        register("IO.pathExtension",w -> emitBuiltin(w, Builtin.PATH_EXTENSION));
        register("IO.pathAbsolute", w -> emitBuiltin(w, Builtin.PATH_ABSOLUTE));
        register("IO.mkdir",        w -> emitBuiltin(w, Builtin.MKDIR));
        register("IO.rmdir",        w -> emitBuiltin(w, Builtin.RMDIR));
        register("IO.removeFile",   w -> emitBuiltin(w, Builtin.REMOVE_FILE));
        register("IO.rename",       w -> emitBuiltin(w, Builtin.RENAME));
        register("IO.copyFile",     w -> emitBuiltin(w, Builtin.COPY_FILE));
        register("IO.fileSize",     w -> emitBuiltin(w, Builtin.FILE_SIZE));
        register("IO.isDirectory",  w -> emitBuiltin(w, Builtin.IS_DIRECTORY));
        register("IO.lastModified", w -> emitBuiltin(w, Builtin.LAST_MODIFIED));
        // N20 — UI vía IDE conectado.
        register("IO.prompt",       w -> emitBuiltin(w, Builtin.PROMPT));

        // ---- Gpio (control de pines en dispositivos embebidos) ----
        register("Gpio.init",       w -> emitBuiltin(w, Builtin.GPIO_INIT));
        register("Gpio.pull",       w -> emitBuiltin(w, Builtin.GPIO_PULL));
        register("Gpio.write",      w -> emitBuiltin(w, Builtin.GPIO_WRITE));
        register("Gpio.read",       w -> emitBuiltin(w, Builtin.GPIO_READ));

        // ---- Net (H11 #241 — cliente TCP simple) ----
        // Nombres tcp* para no colisionar con los métodos connect/send/
        // recv/close de la clase Net.Tcp (misma convención que I2c.Bus).
        register("Net.tcpConnect",  w -> emitBuiltin(w, Builtin.TCP_CONNECT));
        register("Net.tcpSend",     w -> emitBuiltin(w, Builtin.TCP_SEND));
        register("Net.tcpRecv",     w -> emitBuiltin(w, Builtin.TCP_RECV));
        register("Net.tcpClose",    w -> emitBuiltin(w, Builtin.TCP_CLOSE));

        // ---- I2C ----
        // Intrínsecos I2c — nombres internos sufijados con "Bus" para
        // no colisionar con los métodos init/write/read de I2c.Bus.
        // Las funciones top-level I2c.init/write/read son wrappers BP
        // que delegan en estos (forma de compatibilidad).
        register("I2c.initBus",     w -> emitBuiltin(w, Builtin.I2C_INIT));
        register("I2c.writeBus",    w -> emitBuiltin(w, Builtin.I2C_WRITE));
        register("I2c.readBus",     w -> emitBuiltin(w, Builtin.I2C_READ));

        // ---- SPI ----
        // Spi — nombres internos sufijados con "Bus" para no
        // colisionar con los métodos de Spi.Bus. Las funciones
        // top-level Spi.init/write/read/transfer son wrappers BP.
        register("Spi.initBus",     w -> emitBuiltin(w, Builtin.SPI_INIT));
        register("Spi.writeBus",    w -> emitBuiltin(w, Builtin.SPI_WRITE));
        register("Spi.readBus",     w -> emitBuiltin(w, Builtin.SPI_READ));
        register("Spi.transferBus", w -> emitBuiltin(w, Builtin.SPI_TRANSFER));

        // ---- UART ----
        // Uart — nombres internos sufijados con "Port" para no
        // colisionar con los métodos de Uart.Port. Las funciones
        // top-level Uart.init/write/read/available son wrappers BP.
        register("Uart.initPort",      w -> emitBuiltin(w, Builtin.UART_INIT));
        register("Uart.writePort",     w -> emitBuiltin(w, Builtin.UART_WRITE));
        register("Uart.readPort",      w -> emitBuiltin(w, Builtin.UART_READ));
        register("Uart.availablePort", w -> emitBuiltin(w, Builtin.UART_AVAILABLE));

        // ---- Pulse (contador de pulsos hardware) ----
        // Los nombres llevan sufijo "Slice" para no colisionar con los
        // métodos start/stop/value/reset de la clase Pulse.Counter.
        register("Pulse.initSlice",  w -> emitBuiltin(w, Builtin.PULSE_INIT));
        register("Pulse.startSlice", w -> emitBuiltin(w, Builtin.PULSE_START));
        register("Pulse.stopSlice",  w -> emitBuiltin(w, Builtin.PULSE_STOP));
        register("Pulse.readSlice",  w -> emitBuiltin(w, Builtin.PULSE_VALUE));
        register("Pulse.resetSlice", w -> emitBuiltin(w, Builtin.PULSE_RESET));

        // ---- PWM (generación de señal hardware) ----
        // Los nombres del módulo llevan sufijos para no chocar con los
        // métodos start/stop/setFreq/setDuty de la clase Pwm.Slice.
        register("Pwm.initSlice",  w -> emitBuiltin(w, Builtin.PWM_INIT));
        register("Pwm.setFreqHz",  w -> emitBuiltin(w, Builtin.PWM_SET_FREQ));
        register("Pwm.setDutyPct", w -> emitBuiltin(w, Builtin.PWM_SET_DUTY));
        register("Pwm.startSlice", w -> emitBuiltin(w, Builtin.PWM_START));
        register("Pwm.stopSlice",  w -> emitBuiltin(w, Builtin.PWM_STOP));

        // ---- Pico (info del MCU) ----
        register("Pico.uniqueId",  w -> emitBuiltin(w, Builtin.PICO_UNIQUE_ID));
        register("Pico.boardName", w -> emitBuiltin(w, Builtin.PICO_BOARD_NAME));
        register("Pico.tempC",     w -> emitBuiltin(w, Builtin.PICO_TEMP_C));
        register("Pico.cpuFreqHz", w -> emitBuiltin(w, Builtin.PICO_CPU_FREQ_HZ));
        register("Pico.uptimeMs",  w -> emitBuiltin(w, Builtin.PICO_UPTIME_MS));
        register("Pico.gpioCount", w -> emitBuiltin(w, Builtin.PICO_GPIO_COUNT));
        register("Pico.resetCause", w -> emitBuiltin(w, Builtin.PICO_RESET_CAUSE));  // H10
        register("Pico.setMark",   w -> emitBuiltin(w, Builtin.PICO_SET_MARK));      // H10 breadcrumb
        register("Pico.markCount", w -> emitBuiltin(w, Builtin.PICO_MARK_COUNT));
        register("Pico.markAt",    w -> emitBuiltin(w, Builtin.PICO_MARK_AT));
        register("Pico.bootCount", w -> emitBuiltin(w, Builtin.PICO_BOOT_COUNT));
        // H7.4 — NeoPixel WS2812 (internos de la clase Neopixel.Strip)
        register("Neopixel.__npInit", w -> emitBuiltin(w, Builtin.NEOPIXEL_INIT));
        register("Neopixel.__npShow", w -> emitBuiltin(w, Builtin.NEOPIXEL_SHOW));
        register("Pico.setCpuFreqMHzRaw",
                                   w -> emitBuiltin(w, Builtin.PICO_SET_CPU_FREQ_MHZ));

        // ---- Rtc (wall clock) ----
        register("Rtc.nowSec",     w -> emitBuiltin(w, Builtin.RTC_NOW_SEC));
        register("Rtc.setNowSec",  w -> emitBuiltin(w, Builtin.RTC_SET_NOW_SEC));

        // ---- Adc (4 canales del RP2350) ----
        register("Adc.initChannel", w -> emitBuiltin(w, Builtin.ADC_INIT_CHANNEL));
        register("Adc.readChannel", w -> emitBuiltin(w, Builtin.ADC_READ_CHANNEL));

        // ---- Wdt (watchdog singleton) ----
        register("Wdt.enableRaw", w -> emitBuiltin(w, Builtin.WDT_ENABLE));
        register("Wdt.feedRaw",   w -> emitBuiltin(w, Builtin.WDT_FEED));
        register("Wdt.disableRaw", w -> emitBuiltin(w, Builtin.WDT_DISABLE));

        // ---- Gui (V3 H3) — armazón gráfico. Internos __gui* llamados por los
        //      métodos de las clases Gui.* (Obj/Screen/Panel/Label/Button). ----
        register("Gui.__guiScreenActive", w -> emitBuiltin(w, Builtin.GUI_SCREEN_ACTIVE));
        register("Gui.__guiCreateObj",    w -> emitBuiltin(w, Builtin.GUI_CREATE_OBJ));
        register("Gui.__guiCreateLabel",  w -> emitBuiltin(w, Builtin.GUI_CREATE_LABEL));
        register("Gui.__guiCreateButton", w -> emitBuiltin(w, Builtin.GUI_CREATE_BUTTON));
        register("Gui.__guiSetText",      w -> emitBuiltin(w, Builtin.GUI_SET_TEXT));
        register("Gui.__guiSetWidth",     w -> emitBuiltin(w, Builtin.GUI_SET_WIDTH));
        register("Gui.__guiSetHeight",    w -> emitBuiltin(w, Builtin.GUI_SET_HEIGHT));
        register("Gui.__guiAlign",        w -> emitBuiltin(w, Builtin.GUI_ALIGN));
        register("Gui.__guiSetBgColor",   w -> emitBuiltin(w, Builtin.GUI_SET_BG_COLOR));
        register("Gui.__guiSetTextColor", w -> emitBuiltin(w, Builtin.GUI_SET_TEXT_COLOR));
        register("Gui.__guiSetFont",      w -> emitBuiltin(w, Builtin.GUI_SET_FONT));
        register("Gui.__guiClean",        w -> emitBuiltin(w, Builtin.GUI_CLEAN));
        register("Gui.__guiDelete",       w -> emitBuiltin(w, Builtin.GUI_DELETE));
        register("Gui.__guiScreenLoad",   w -> emitBuiltin(w, Builtin.GUI_SCREEN_LOAD));
        register("Gui.__guiRun",          w -> emitBuiltin(w, Builtin.GUI_RUN));
        register("Gui.__guiDumpTree",     w -> emitBuiltin(w, Builtin.GUI_DUMP_TREE));
        // H3.4 — eventos. __guiDispatch NO va aquí: es función BP normal (cuerpo
        // self.onClick()); la VM la llama por nombre al pulsar.
        register("Gui.__guiBindClick",    w -> emitBuiltin(w, Builtin.GUI_BIND_CLICK));
        register("Gui.__guiClick",        w -> emitBuiltin(w, Builtin.GUI_CLICK));
        // H13 — Forms: call-by-name del handler (owner, name: string, sender) → void.
        register("Gui.__guiInvokeByName", w -> emitBuiltin(w, Builtin.GUI_INVOKE_BY_NAME));
        // H13.1 — Forms Camino A: dispatch por slot de vtable (win, slot: integer, sender) → void.
        register("Gui.__guiInvokeBySlot", w -> emitBuiltin(w, Builtin.GUI_INVOKE_BY_SLOT));
        // H6 — geometría (backend = verdad) + scroll (opt-in) + refresh.
        register("Gui.__guiSetX",         w -> emitBuiltin(w, Builtin.GUI_SET_X));
        register("Gui.__guiGetX",         w -> emitBuiltin(w, Builtin.GUI_GET_X));
        register("Gui.__guiSetY",         w -> emitBuiltin(w, Builtin.GUI_SET_Y));
        register("Gui.__guiGetY",         w -> emitBuiltin(w, Builtin.GUI_GET_Y));
        register("Gui.__guiGetWidth",     w -> emitBuiltin(w, Builtin.GUI_GET_WIDTH));
        register("Gui.__guiGetHeight",    w -> emitBuiltin(w, Builtin.GUI_GET_HEIGHT));
        register("Gui.__guiSetScrollDir", w -> emitBuiltin(w, Builtin.GUI_SET_SCROLL_DIR));
        register("Gui.__guiGetScrollDir", w -> emitBuiltin(w, Builtin.GUI_GET_SCROLL_DIR));
        register("Gui.__guiRefresh",      w -> emitBuiltin(w, Builtin.GUI_REFRESH));
        // H6 widgets — checkbox.
        register("Gui.__guiCreateCheckbox", w -> emitBuiltin(w, Builtin.GUI_CREATE_CHECKBOX));
        register("Gui.__guiSetChecked",     w -> emitBuiltin(w, Builtin.GUI_SET_CHECKED));
        register("Gui.__guiGetChecked",     w -> emitBuiltin(w, Builtin.GUI_GET_CHECKED));
        register("Gui.__guiChange",         w -> emitBuiltin(w, Builtin.GUI_CHANGE));
        // H6 widgets — switch + slider + bar (value-widgets enteros).
        register("Gui.__guiCreateSwitch",   w -> emitBuiltin(w, Builtin.GUI_CREATE_SWITCH));
        register("Gui.__guiCreateSlider",   w -> emitBuiltin(w, Builtin.GUI_CREATE_SLIDER));
        register("Gui.__guiCreateBar",      w -> emitBuiltin(w, Builtin.GUI_CREATE_BAR));
        register("Gui.__guiSetValue",       w -> emitBuiltin(w, Builtin.GUI_SET_VALUE));
        register("Gui.__guiGetValue",       w -> emitBuiltin(w, Builtin.GUI_GET_VALUE));
        register("Gui.__guiSetRange",       w -> emitBuiltin(w, Builtin.GUI_SET_RANGE));
        // H6 widgets — spinbox + led.
        register("Gui.__guiCreateSpinbox",  w -> emitBuiltin(w, Builtin.GUI_CREATE_SPINBOX));
        register("Gui.__guiCreateLed",      w -> emitBuiltin(w, Builtin.GUI_CREATE_LED));
        // H6 widgets — dropdown + textarea.
        register("Gui.__guiCreateDropdown", w -> emitBuiltin(w, Builtin.GUI_CREATE_DROPDOWN));
        register("Gui.__guiSetOptions",     w -> emitBuiltin(w, Builtin.GUI_SET_OPTIONS));
        register("Gui.__guiCreateTextarea", w -> emitBuiltin(w, Builtin.GUI_CREATE_TEXTAREA));
        register("Gui.__guiGetText",        w -> emitBuiltin(w, Builtin.GUI_GET_TEXT));
        // H6 widgets — list + keyboard.
        register("Gui.__guiCreateList",     w -> emitBuiltin(w, Builtin.GUI_CREATE_LIST));
        register("Gui.__guiCreateKeyboard", w -> emitBuiltin(w, Builtin.GUI_CREATE_KEYBOARD));
        register("Gui.__guiKeyboardSetTextarea", w -> emitBuiltin(w, Builtin.GUI_KEYBOARD_SET_TEXTAREA));
        // H6 widgets — msgbox.
        register("Gui.__guiCreateMsgbox",   w -> emitBuiltin(w, Builtin.GUI_CREATE_MSGBOX));
        register("Gui.__guiSetButtons",     w -> emitBuiltin(w, Builtin.GUI_SET_BUTTONS));
        // H6 widgets — tabview.
        register("Gui.__guiCreateTabview",  w -> emitBuiltin(w, Builtin.GUI_CREATE_TABVIEW));
        register("Gui.__guiTabviewAddTab",  w -> emitBuiltin(w, Builtin.GUI_TABVIEW_ADD_TAB));
        // H6 widgets — table.
        register("Gui.__guiCreateTable",    w -> emitBuiltin(w, Builtin.GUI_CREATE_TABLE));
        register("Gui.__guiTableSetGrid",   w -> emitBuiltin(w, Builtin.GUI_TABLE_SET_GRID));
        register("Gui.__guiTableSetCell",   w -> emitBuiltin(w, Builtin.GUI_TABLE_SET_CELL));
        register("Gui.__guiTableGetCell",   w -> emitBuiltin(w, Builtin.GUI_TABLE_GET_CELL));
        // H6 widgets — image (asset + ImageView).
        register("Gui.__guiImageNew",          w -> emitBuiltin(w, Builtin.GUI_IMAGE_NEW));
        register("Gui.__guiImageLoadFile",     w -> emitBuiltin(w, Builtin.GUI_IMAGE_LOAD_FILE));
        register("Gui.__guiImageWidth",        w -> emitBuiltin(w, Builtin.GUI_IMAGE_WIDTH));
        register("Gui.__guiImageHeight",       w -> emitBuiltin(w, Builtin.GUI_IMAGE_HEIGHT));
        register("Gui.__guiCreateImageView",   w -> emitBuiltin(w, Builtin.GUI_CREATE_IMAGEVIEW));
        register("Gui.__guiImageViewSetImage", w -> emitBuiltin(w, Builtin.GUI_IMAGEVIEW_SET_IMAGE));
        register("Gui.__guiImageViewRefresh",  w -> emitBuiltin(w, Builtin.GUI_IMAGEVIEW_REFRESH));
        // H6 — fuente (catálogo de tamaños).
        register("Gui.__guiSetFontSize",       w -> emitBuiltin(w, Builtin.GUI_SET_FONT_SIZE));
        register("Gui.__guiGetFontSize",       w -> emitBuiltin(w, Builtin.GUI_GET_FONT_SIZE));
        // H6 — textarea read-only.
        register("Gui.__guiTextareaSetReadonly", w -> emitBuiltin(w, Builtin.GUI_TEXTAREA_SET_READONLY));
        register("Gui.__guiTextareaGetReadonly", w -> emitBuiltin(w, Builtin.GUI_TEXTAREA_GET_READONLY));
    }
}
