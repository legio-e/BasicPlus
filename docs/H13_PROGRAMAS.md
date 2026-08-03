# H13 — los programas de prueba

Complemento de [H13_PRUEBAS.md](H13_PRUEBAS.md): **qué se ejecuta** en cada bloque. El
guión dice el procedimiento; esto dice el material.

**El criterio de V4**: se prueba a fondo lo que hemos cambiado —**memoria, sistema de
ficheros y gráficos**— más lo que es nuevo del lenguaje (eventos, tuplas, sobrecarga,
async). Los **dispositivos** (GPIO, I²C, SPI, UART, ADC, PWM…) NO se barren: no se han
tocado en V4 y son la parte más pesada de probar. Se deja **un** GPIO como testigo.

**Todos los que se ejecutan en el PC están verificados** con `scripts/h13-lista.sh`
(compila con el frontend de hoy y corre en la VM-C del PC). Una lista con programas que
ya no compilan es peor que no tener lista: gasta tiempo de placa depurando el sample.

```bash
bash scripts/h13-lista.sh
```

---

## M · Memoria — el cambio más grande de V4 (H1 handles + #355 + #357)

El tren de memoria son cinco escalones: cada uno añade UNA cosa al anterior, así que si
falla el 4 y va el 3, el problema está en lo que añade el 4. Correrlos **en orden**.

| Programa | Qué añade sobre el anterior |
|---|---|
| `MemT1_Oo` | objetos y campos |
| `MemT2_StrField` | …un campo `string` (referencia) dentro del objeto |
| `MemT3_Inherit` | …herencia, `super()` y despacho virtual |
| `MemT4_Concat` | …concatenación en bucle (realloc de string) |
| `MemT5_Gc` | …churn de GC (~440 KB alocados y tirados) |
| `MemStress` | los cuatro a la vez: el aislante completo |
| `StrGcTest` | regresión #1 del censo (`StringBuilder.chars` era ref de 4 B) |
| `AnyGcHard` | #14: el objeto vive SOLO dentro de un campo `Object` |
| `ConcatObjTest` | auto-`toString` de objetos al concatenar |
| `smp_heap_stress_pico` | estrés dimensionado a los ~64 KB de la Pico |

**Lo que hay que mirar**, y no es la salida del programa: el `INFO` de la placa
**antes y después**. El heap tiene que volver a su sitio (#357: 10.000 vueltas en la
Pico sin crecer). Si sube y no baja, ahí hay algo — y el guardián de fin de RUN (#339)
dice *quién* se quedó la memoria, con fichero y línea.

## F · Sistema de ficheros — littlefs en las 3 familias (H2)

| Programa | Qué prueba |
|---|---|
| `FileTest` | leer / escribir / añadir |
| `FileOpsTest` | `mkdir`, `rmdir`, `copyFile`, `isDirectory` |
| `FileBytesTest` | I/O binario con `byte[]` |
| `FsPowerCut` | **el torturador**: simula cortes de corriente a mitad de escritura |
| `FsImportTest` | importar un módulo que se subió al FS |
| `JsonDemo` | la librería Json (parse + serialize) sobre ficheros |
| `LogTest` | el módulo Log escribiendo a fichero |
| `CompressFileTest` | descomprimir desde fichero |

`FsPowerCut` es el que más vale de los ocho y el más lento (~1 min en el PC). Va el
primero de la tanda de FS, no el último: si el FS está mal, lo demás miente.

## G · Gráficos — H7 + eventos (#324) + panel por ENV (#311)

Sólo en las placas con pantalla (**b2**, **b3**, **c2**) y en el **micro simulado**. Se
compilan todos en el PC; ejecutarlos aquí abriría ventana.

| Programa | Widget o rasgo |
|---|---|
| `GuiDemo` | el primero: ventana + etiqueta |
| `GuiGeomDemo` | geometría explícita x/y/ancho/alto |
| `GuiColorDemo` | color de fondo y de texto — *el que dio guerra en el P4* |
| `GuiValueDemo` | switch, slider, bar |
| `GuiCheckDemo` | checkbox + `onChange` |
| `GuiInputDemo` | dropdown + textarea |
| `GuiListKbd` | list + teclado en pantalla |
| `GuiTabDemo` | pestañas |
| `GuiMsgDemo` | msgbox modal |
| `GuiTableDemo` | tabla de celdas |
| `GuiImageDemo` | imagen (asset + control) |
| `GuiFontDemo` | catálogo de tamaños de fuente |
| `GuiRotDemo` | rotación del display |
| `GuiClickDemo` | eventos de clic |
| `GuiAsyncDemo` | trabajo largo desde un handler **sin congelar la UI** |
| `ChartDemo` | **el widget nuevo de V4** (gráfica de series) |

⚠️ `ChartDemo.bp` vive en `bpgenvm-c/samples/`, no en `samples/` — hoy **no va en el
ZIP**. Es la novedad gráfica de V4 y debería ir con los demás.

## E · Eventos — H5.c, nuevo en V4

| Programa | Qué prueba |
|---|---|
| `EvFull` | el ciclo completo: declarar, suscribir, disparar, atender |
| `EvOrder` | se atienden en el orden en que se dispararon |
| `EvNest` | un handler dispara otro evento (reentrada) |
| `EvThrow` | un handler que lanza y no atrapa — **termina mal a propósito** |
| `EvFin` | #342: el evento del thread que muere no se pierde |
| `h5cevuse` | eventos **cruzando módulo** |

## T · Threads y async

| Programa | Qué prueba |
|---|---|
| `AsyncDemo` | #325: `Thread(obj::metodo(args))`, sin palabras nuevas |
| `ThreadTrasMain` | #346: acaba cuando acaban TODOS los threads, no cuando acaba `main` |
| `mutextest` | mutex bajo contención (transferencias entre cuentas) |
| `synclisttest` | `SyncList` productor/consumidor — *destapó un crash del compilador, 9dd7d10* |
| `preempttest` | worker CPU-bound: la consola sigue viva |

## L · Lo nuevo del lenguaje en V4

`TupleFirstClass` · `TupCrossTest` (tuplas) · `DefaultParams` (parámetros por defecto) ·
`StaticPropTest` · `narrowtypes` (enteros estrechos) · `Field8Test` · `Wrap8Test` ·
`PropLongTest` (los 8 bytes: long y double en campos, envoltorios y properties).

## X · Base — que no se haya roto lo de siempre

`ExcCatchTest` (try/catch y jerarquía) · `OoSmoke` (humo de OO) · `StrOps348` ·
`MathOps348` · `PathOps348` (los builtins repasados en #348).

## D · Dispositivos — sólo el testigo

`blink` (GPIO) y `BoardTest` (identificación de la placa). **Nada más**: los
periféricos no se han tocado en V4 y barrerlos cuesta más que todo lo anterior junto.
Si algo de HW falla aquí, se mira; si no, no se abre esa puerta.

---

## Proyectos — lo que un `.bp` suelto no prueba

Un proyecto ejercita el ciclo del IDE: `.bpbuild`, `sourceDir`/`outDir`, dependencias,
`resources/` que suben a la placa, y en dos casos el **pack** y el **AOT**.

| Proyecto | Qué prueba | Dónde |
|---|---|---|
| `samples/sampleproject` | `out: pack` + `aot: arm` — **#310 packs ejecutables**, lo más nuevo | RP2350, STM32 |
| `samples/aottest` | `aot: riscv` — el .mdn dinámico de H4 (Fibo 58 s → 0,5 s) | ESP32-P4 |
| `samples/formdemo` | Forms: `.win` con slots horneados por el IDE | con pantalla |
| `samples/formev` | Forms **sin slots**: el `.win` lleva sólo nombres (#324) | con pantalla |
| `samples/formmin` | el `.win` mínimo — es la bisección si Forms cuelga | con pantalla |
| `samples/imageproject` | `resources/testimg.png` sube a la placa y se dibuja | con pantalla |

⚠️ `formev`, `formmin` y `aottest` **no van hoy en el ZIP** (`scripts/montar-zip.sh`
copia sólo `sampleproject`, `formdemo` e `imageproject`). `formev` es el que prueba el
despacho de Forms de verdad y `aottest` el AOT: deberían ir.

---

## Reparto por placa

| Bloque | a1 Pico | a2 Metro | b1 S3 | b2 P4 Kit | b3 P4 WS | c1 Nucleo | c2 DK2 |
|---|---|---|---|---|---|---|---|
| M memoria | **sí** | sí | sí | sí | — | sí | — |
| F ficheros | **sí** | sí | sí | sí | — | sí | sí |
| G gráficos | — | — | — | **sí** | sí | — | sí |
| E eventos | sí | — | sí | sí | — | sí | — |
| T threads | **sí** | — | sí | sí | — | sí | — |
| L lenguaje | sí | — | sí | — | — | sí | — |
| X base | sí | sí | sí | sí | sí | sí | sí |
| D testigo | sí | sí | sí | — | — | sí | — |

En **negrita**, dónde duele más si falla: la Pico es la de menos memoria (el que aprieta
M y T) y la primera que se rompió con el heap (#357).
