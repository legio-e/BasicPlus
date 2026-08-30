# Repaso de V5 con la batería de V4 — 28-ago-2026

> ## ⏸️ ESTE REPASO YA NO ES UNA TAREA ABIERTA (30-ago)
>
> `#443` se cerró **por decisión de Eduardo**: la hipótesis que lo abrió está
> contestada (la batería de la Pico llegó a **48/48** y las ocho rojas eran tres
> causas, no ocho bugs), y **el cierre de V6 trae su propia tanda de pruebas**, así
> que repetir ésta ahora sería pasar dos veces la misma batería sobre un árbol que
> todavía se mueve.
>
> **Lo que este documento pasa a ser: la lista de arranque de esa tanda de cierre.**
> Las «condiciones de partida» de abajo son lo que hace el resultado *atribuible* —
> sin ellas, un rojo no dice de quién es. Se lee al empezar el cierre de V6, no antes.
>
> ⚠️ Lo que NO cubre: la **deuda de verificación del STM32** (ver la matriz en
> `ESTADO.md`). Eso no es repasar V5 — es código común que esas placas no han
> ejecutado nunca.

> **Por qué existe este documento.** El 27-ago, probando otra cosa, `JsonDemo`
> falló en la Pico con `exit 6 (opcode desconocido)`. Al buscar cuándo dejó de
> ir salió esto:
>
> | | líneas de registro | ¿`JsonDemo`? |
> |---|---|---|
> | V4 (`H13_PRUEBAS.md`) | 2413 | **sí** — tanda 3, ✅ |
> | V5 (`H13_PRUEBAS_V5.md`) | 418 | **ninguna mención** |
>
> O sea: se verificó a fondo en V4 y **no se volvió a probar en V5**. Decisión de
> Eduardo: *«yo repetiría todas las pruebas de V4 sobre IDE + Firmware + Demos de
> V5. A ver qué sale, es muy raro que solamente falle JsonDemo.»*
>
> **Esa es la hipótesis a comprobar: que `JsonDemo` no está solo.**

## Condiciones de partida — si no se cumplen, el resultado NO es atribuible

1. **Todo de V5 y nada más.** IDE, firmware y samples del mismo paquete
   (`dist/BasicPlus-5.0-win/`). El 27-ago se acabó con una mezcla (IDE de V4 +
   imagen de V5 + un FS con 35 ficheros de pruebas sueltas) y sobre eso no se
   puede atribuir nada.
2. **FS formateado antes de empezar cada placa.**
3. **Respetar el orden de las tandas.** No es ceremonia: el hallazgo 28 de V4 lo
   dejó escrito — *«no lo escondía el estado de la placa: lo escondía una prueba
   anterior»*. `JsonDemo` corría antes que las tandas gráficas y dejaba
   `Json.mod` en `/lib`, tapando un bug crítico de GUI. En una batería que
   comparte dispositivo, **cada prueba prepara el terreno de la siguiente**.
4. **Anotar el resultado aunque sea verde.** El coste de esta cacería ha sido
   justo no poder consultar un verde de hace un mes.

## Ya sabemos (27-ago, medido)

- 🔴 **C2 `JsonDemo` falla en la Pico** con la imagen de V5 y con todas las
  posteriores. Con la de **V4 va**. El hueco es 6-ago → 22-ago.
- 🟢 En la **Nucleo va**, y en el **host va**. Mismo `.mod`, mismo `interp.c`.
- Los 27 `.mod` de la stdlib son **byte a byte idénticos** en V4, V5 y hoy; el
  fuente de `JsonDemo.bp` también; y los compiladores de V4 y V5 producen el
  **mismo** `JsonDemo.mod`. La diferencia NO está en el lado del PC.
- Hay tres imágenes de bisección construidas (cuartiles del rango): 14-ago,
  17-ago y 20-ago. Están en el scratchpad de la sesión del 27-ago.

## La batería (de `H13_PRUEBAS.md`, sin tocar)

### A · Arranque y conexión
- [ ] A1 Arranca a **estado 3 (app)**; el log no trae nada raro
- [ ] A2 `INFO` con los datos correctos de ESA placa
- [ ] A3 Causa del reset correcta
- [ ] A4 Conectar y desconectar 3 veces sin colgarse
- [ ] A5 `RESET` desde el IDE → vuelve y reconecta

### B · Sistema de ficheros
- [ ] B1 `LIST` del árbol completo (`/app`, `/lib`, `/sys`)
- [ ] B2 Subir un fichero pequeño (< 8 KB, un viaje) y releerlo idéntico
- [ ] B3 Subir uno **grande** (> 8 KB → streaming) y releerlo idéntico
- [ ] B4 Sobrescribir un fichero existente
- [ ] B5 Borrar, renombrar, crear y borrar directorio
- [ ] B6 `DF` coherente
- [ ] B7 Apagar y encender: **todo sigue ahí**
- [ ] B8 Formatear y comprobar que se rehace solo

### C · Ejecución
- [ ] C1 `T` — hilos
- [ ] C2 `JsonDemo` — strings, objetos, parsing  🔴 **ROJO CONOCIDO en la Pico**
- [ ] C3 `FileTest` — FS desde BP
- [ ] C4 `AsyncDemo` — `Thread(obj::metodo(args))`
- [ ] C5 `Ev*` — `EvFull`, `EvOrder`, `EvMulti`, `EvThrow`
- [ ] C6 Excepciones: `try/catch` nativo y de usuario
- [ ] C7 **OOM atrapable**: `RuntimeError`, no un reset
- [ ] C8 El mismo programa **3 veces seguidas**: misma salida, sin arrastre
- [ ] C9 `Stop` (Ctrl+F2) corta y la placa sigue viva

### D · Gestión de placa
- [ ] D1 Panel abre y muestra el estado del arranque
- [ ] D2 Leer variables de entorno
- [ ] D3 Editar una y reiniciar: persiste
- [ ] D4 Borrar una
- [ ] D5 `Proponer defaults` de particiones
- [ ] D6 Tamaños válidos → reinicia con el reparto nuevo
- [ ] D7 Tamaño **inválido** → error claro, entorno **intacto**
- [ ] D8 A medio configurar, el IDE ofrece abrir el panel

### E · Packs
- [ ] E1 Grabar un pack de librería
- [ ] E2 Listarlo: nombre, tamaño, fecha, ficheros
- [ ] E3 `import` de un módulo del pack
- [ ] E4 Un `.mod` suelto en `/lib` **eclipsa** al del pack
- [ ] E5 Retirar un pack
- [ ] E6 Formatear la zona
- [ ] E7 Persistencia tras apagar
- [ ] E8 **Pack ejecutable**: `samples/sampleproject`, cross-module dentro
- [ ] E9 `resources/`: `leeme.txt` en `/app/…` tras el Run

### F · Depuración
- [ ] F1 Breakpoint · [ ] F2 Step · [ ] F3 Variables · [ ] F4 Call stack · [ ] F5 Continuar

### G · Autónomo
- [ ] G1 `autorun` + reinicio → arranca solo
- [ ] G2 Ventana de rescate responde a HELLO/Stop
- [ ] G3 Quitar el autorun

### H · Memoria
- [ ] H1 `INFO` recién arrancada: heap / pila / RTOS libre
- [ ] H2 Carga (T + JsonDemo + GUI si la hay) y **repetir INFO**
- [ ] H3 El heap no crece entre RUN sucesivos — 20-30 vueltas
- [ ] H4 El guardián de fin de RUN no grita
- [ ] H5 El buffer de 8 KB no estorba

### I · Recuperación
- [ ] I1 Desenchufar a media escritura y volver: el FS monta
- [ ] I2 `LOG_DUMP` tras un reset: el log sobrevive
- [ ] I3 `LOG_CLEAR` limpia

## Registro

| # | placa | bloque | qué pasó |
|---|---|---|---|
| | | | |

## ⚠️ DEUDA DE VERIFICACIÓN — el STM32, desde `U3.6` (visto por Eduardo, 27-ago)

**El matiz que se me escapó**: se probó el STM32 (`U3.5`/`U3.6`) ✅, luego se probó la Pico
(`U3.7`…`U3.12`) ✅ — pero **cinco de los seis pasos de la Pico tocaron el fichero COMÚN**,
que el STM32 también usa. Y `U3.12` tocó `stm32_repl.c` directamente. Así que el STM32
lleva cinco commits sin verificar.

Medido con `git log e8d2a129..HEAD -- src/bpvm_repl.c include/bpvm_repl.h stm32/port/stm32_repl.c`
(sólo `U3.9`, el LIST, fue exclusivo de la Pico).

**No hace falta repasarlo entero.** Lo que le cambió, y qué lo ejercita:

| paso | qué le cambió al STM32 | gesto |
|---|---|---|
| `U3.7` | el volcado de log emite `\uXXXX` en vez de un espacio | **Log** |
| `U3.8` | `DEL` distingue «no está» de «está y no se pudo borrar» | **Delete** |
| `U3.10` | `SAVE_REPLY` lleva ahora `durationMs` (campo nuevo) | **Save** |
| `U3.11` | el buffer del `INFO` sube de 900 a 1024 (pila) | **Info** |
| `U3.12` | su puerta de H9 traga el bulk; y el drenaje del PUT cambió de función | **subir un fichero pequeño y uno > 8 KB** |

⚠️ El `.bin` a usar es el de **las 16:49** o uno reconstruido — y recordar que el build
headless regenera el `.elf` pero **NO** el `.bin`: hay que pasar `arm-none-eabi-objcopy -O binary`.

**La lección, y generaliza:** ya teníamos escrito que *«un arreglo que no viaja entre
familias es medio arreglo»*. Esto es **el caso inverso y menos visible**: un cambio que SÍ
viaja, a una familia que en ese momento no se está probando. Al migrar la familia B, todo lo
que caiga en el común deja **deuda de verificación** en la familia A. La regla operativa:
**cuando un paso toca el común, la lista de placas a re-verificar son TODAS las que lo
enlazan, no sólo la que se está migrando.** Y basta con el gesto que ejercita lo que cambió.
