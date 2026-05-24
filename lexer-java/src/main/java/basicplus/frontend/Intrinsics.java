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
    }
}
