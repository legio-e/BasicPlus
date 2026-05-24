# Política HW = Clase OO

**TL;DR**: Todo periférico hardware nuevo se modela como **clase OO** dentro de un módulo BP que la encapsula. Excepción explícita: información-del-MCU (uno y único, sin recursos asociados), que se modela como funciones + constantes.

---

## Por qué

Los periféricos físicos tienen identidad, estado y recursos:

- Un pin GPIO tiene un número físico y un modo configurado.
- Un slice PWM tiene un id de slice + freq + duty actual.
- Un contador de pulsos tiene un id de counterSlice asignado.
- Un bus I2C/SPI/UART tiene un id de bus + pines + config (baudrate, paridad, modo).

Modelar eso como una clase tiene tres ventajas:

1. **Encapsulación**: el id opaco del backend se guarda en la instancia, el usuario no lo arrastra entre llamadas.
2. **Tipado**: `var p: Pwm.Slice` documenta qué representa esa variable. El compilador BP detecta errores como pasar un `Pulse.Counter` donde se espera un `Pwm.Slice`.
3. **Liberación de recursos**: cuando lleguen finalizers, el destructor del objeto liberará el slice/bus automáticamente.

Pequeño coste: 1 indirección en la llamada (`p.start()` en vez de `Pwm.startSlice(id)`). En el contexto de hardware (operaciones del orden de µs hacia abajo), invisible.

---

## El patrón canónico

```basicplus
// ============================================================
// Foo.bp — control del periférico Foo en MCU.
// ============================================================
module Foo

  // ---- Intrínsecos de bajo nivel ----
  //
  // Reciben un handle opaco entero (sliceId / counterId / busId)
  // como primer parámetro. El nombre lleva sufijo del tipo de
  // handle ("Slice", "Bus") para no colisionar con métodos de la
  // clase envolvente — dentro de un método `start()`, llamar a
  // `start(id)` se resolvería al propio método (recursión infinita).
  public intrinsic function initBus(...): integer    // devuelve id, -1 si error
  public intrinsic function startBus(id: integer)
  public intrinsic function stopBus(id: integer)
  public intrinsic function readBus(id: integer, ...): integer

  // ---- Constantes top-level (para uso sin instanciar) ----
  //
  // Funciones nullary que devuelven el valor. Cuando la
  // infraestructura `public const` esté lista a nivel módulo,
  // migrar a esa forma.
  public function MODE_X(): integer
    return 0
  end MODE_X

  // ============================================================
  // Clase Foo.Bar — wrapper OO sobre los intrínsecos.
  //
  // Doc-comment explicando para qué sirve la clase, qué encapsula,
  // y un ejemplo mínimo de uso.
  // ============================================================
  public class Bar

    // Static consts dentro de la clase para enumeraciones del
    // dominio (modos, flags, paridades). Accesibles como
    // `Foo.Bar.MODE_X`.
    public const Bar.MODE_X: integer := 0
    public const Bar.MODE_Y: integer := 1

    // Propiedades públicas: los parámetros físicos que el
    // constructor recibió + el handle opaco devuelto por el
    // backend. Públicas para inspección en el debugger.
    public var pin:  integer
    public var mode: integer
    public var id:   integer    // handle opaco del backend

    // Constructor: recibe los parámetros físicos, llama al
    // intrínseco de inicialización, guarda el handle y los
    // parámetros. Si el backend devuelve un id inválido (< 0),
    // lanza RuntimeError con un mensaje legible.
    public function Bar(pin: integer, mode: integer)
      this.pin  := pin
      this.mode := mode
      this.id   := initBus(pin, mode)
      if this.id < 0 then
        var msg: string := "Foo.Bar: pin " + intToString(pin) + " inválido"
        throw RuntimeError(msg)
      endif
    end Bar

    // Métodos: la instancia ya tiene `this.id`, no hace falta
    // arrastrarlo. Nombres cortos, en imperativo o presente
    // ("start", "stop", "value", "setX", "isX").
    public function start()
      startBus(this.id)
    end start

    public function stop()
      stopBus(this.id)
    end stop

    public function value(): integer
      return readBus(this.id)
    end value

  end Bar

end Foo
```

### Convenciones de nombres

| Concepto | Convención | Ejemplo |
|---|---|---|
| Módulo | Sustantivo singular o nombre técnico | `Gpio`, `Spi`, `Pulse`, `Pwm`, `Rtc` |
| Clase principal | Sustantivo describiendo la instancia | `Gpio.Pin`, `Spi.Bus`, `Pulse.Counter`, `Pwm.Slice`, `Rtc.Clock` |
| Intrínseco | Verbo + sufijo del recurso | `initBus`, `startSlice`, `readPort` |
| Método de la clase | Verbo corto, sin sufijo | `start()`, `stop()`, `value()`, `setFreq(f)` |
| Constante de modo | UPPER_SNAKE_CASE | `Bar.MODE_X`, `Pin.PULL_UP`, `Spi.Bus.MODE0` |

### Estado del constructor

El constructor SIEMPRE:
1. Guarda los parámetros físicos en propiedades (`this.pin`, `this.mode`).
2. Llama al intrínseco de inicialización del backend.
3. Verifica el id devuelto.
4. Lanza `RuntimeError` con un mensaje legible si la inicialización falla.

El constructor NO arranca el periférico — eso lo hace `start()` explícitamente. Razón: permite construir el objeto, configurarlo más (duty, paridad, etc.) y luego arrancar.

### Liberación

Hoy BP no tiene finalizers. Mientras tanto, el usuario llama `stop()` explícitamente para liberar el recurso. El siguiente `Foo.Bar(...)` sobre el mismo pin reabre limpio. Cuando se añadan finalizers, el destructor llamará `stop()` automáticamente.

---

## Excepción documentada: módulo `Pico`

El módulo `Pico.bp` expone información del microcontrolador (uniqueId, boardName, tempC, cpuFreqHz, uptimeMs, setCpuFreqMHz) **como funciones + constantes, sin clase**. Razón:

- La info del chip **no tiene "instancia"**: es uno y único.
- **No hay recursos que liberar**: `uniqueId` es estático, `tempC` se lee on-demand, `setCpuFreqMHz` afecta a un singleton global (el clk_sys).
- Una clase `Pico.Mcu` con métodos `mcu.uniqueId()` añade ceremonia sin ganar nada — no hay un caso de uso donde tengas DOS instancias.

Misma lógica se aplicará a otros módulos de información global cuando los haya (p.ej. un futuro `Power` para sleep modes globales).

**Heurística**: si tu módulo tendría como mucho UNA instancia de la clase a la vez y la clase no encapsula un recurso liberable, plantéate si no es mejor un módulo de funciones puro.

---

## Casos canónicos ya implementados

| Módulo | Clase | Recurso encapsulado | Status |
|---|---|---|---|
| `Gpio` | `Gpio.Pin` | Número de pin + modo | ✅ #103 |
| `Pulse` | `Pulse.Counter` | counterSlice del PWM HW en modo input-gate | ✅ #122 |
| `Pwm` | `Pwm.Slice` | sliceId del PWM HW + pin físico (canal A/B) | ✅ #131 |
| `Spi` | (`Spi.Bus`) | busId + pines + baudrate + mode | 📋 #124 pendiente refactor |
| `Uart` | (`Uart.Port`) | busId + pines + baudrate + format | 📋 #125 pendiente refactor |
| `I2c` | (`I2c.Bus`) | busId + pines + baudrate | 📋 #126 pendiente refactor |
| `Rtc` | (`Rtc.Clock`) | RTC HW del MCU (singleton, pero con setTime/getTime útiles) | 📋 #127 pendiente |
| `Adc` | (`Adc.Channel`) | channel del ADC HW + pin | 📋 #128 pendiente |
| `Wdt` | (`Wdt.Timer`) | watchdog HW (singleton) | 📋 #129 pendiente |
| `Timer`| (`Timer.Alarm`) | timer HW + callback | 📋 #130 pendiente |

`Pico` rompe la regla intencionadamente (ver "Excepción documentada" arriba).

---

## Compatibilidad hacia atrás durante refactors

Cuando refactorizamos un módulo existente (Spi/Uart/I2c) para añadir la clase:

1. **Mantener los intrínsecos top-level con su signatura actual** — código BP que ya usaba `Spi.write(0, data, n)` sigue compilando.
2. **Añadir la clase como segunda forma de uso** — código nuevo puede usar `var bus: Spi.Bus := Spi.Bus(0, ...)` y luego `bus.write(data, n)`.
3. **Documentar en el doc-comment** que la forma OO es la preferida para código nuevo.
4. **No marcar los intrínsecos como deprecated** — son la API de bajo nivel que la propia clase usa internamente; siempre van a existir.

Eso permite migrar samples y drivers gradualmente sin churn de PR.
