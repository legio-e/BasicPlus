# BasicPlus — Limitaciones y pulido conocidos

> **Qué es esto, y qué NO.** Aquí van las **limitaciones y decisiones del lenguaje**
> que alguien que USA BasicPlus debe conocer: cosas que son así y de momento se
> quedan así. Es documentación, y por eso la citan `QUICKSTART`, `PHILOSOPHY` y los
> README.
>
> **Lo que ya NO va aquí: el estado del trabajo.** Qué ficha está abierta, cerrada
> o en curso vive **sólo** en `docs/FICHAS.md`. Decisión de Eduardo (17-ago): *«Estado
> y pendientes son ficheros de trabajo tuyos. Pero el que dice realmente cuál es la
> situación es Fichas.»* **Si este fichero contradice a `FICHAS.md`, manda `FICHAS.md`**
> y esto se corrige.
>
> El motivo es concreto: el 17-ago este fichero daba por abiertos dos bugs cerrados
> ese mismo día, y de las 51 fichas que citaba `ESTADO`, 49 eran una segunda copia de
> las de `FICHAS`. Una lista de bugs más larga que la real es justo lo que este
> documento venía a evitar.
>
> **Mapa de docs:**
> - **`docs/FICHAS.md`** — el registro de fichas. **La fuente de verdad.**
> - **`docs/ESTADO.md`** — el traspaso entre sesiones (diario, por fechas).
> - **`V4_BACKLOG.md`** / **`HECHO_V*.md`** — snapshots inmutables de versiones cerradas.
>
> Convención: L=limitación · N=hallazgo · M=mejora. (Los bugs, **B**, se fueron a
> `FICHAS.md`: tienen estado.)

---

## 🟡 Limitaciones / decisiones documentadas del lenguaje

- **L14 — en el RP2350 (Pico 2 / Metro), los `double` SUBNORMALES se aplastan a
  cero.** Es decir: por debajo de `2.2250738585072014e-308`, la Metro da `0`
  donde el PC, el ESP32-P4 y el STM32 dan el valor. Todo lo que esté por encima
  de esa frontera es exacto y byte-idéntico en las cuatro plataformas.

  **De dónde sale.** El SDK de la Pico sustituye las rutinas de `double` de
  libgcc por las suyas optimizadas, y ésas descartan los subnormales *a
  propósito* — está escrito en su ensamblador: `double_sci_m33.S:121`,
  `movs r0,#0  @ flush denormal`. No es un fallo nuestro ni del SDK: es su
  compromiso de velocidad, que heredamos al enlazar.

  **Medido, no supuesto** (17-ago, `samples/SubNorm.bp` y `samples/DblBench.bp`,
  los dos en el repo):
  - la frontera es EXACTAMENTE la del formato: `n1..n4` (normales, incluido el
    menor normal) salen bien; `s1..s5` (subnormales) y las operaciones que caen
    ahí, todos a `0`; el control en magnitudes normales, exacto;
  - el P4 y el host dan las 16 líneas correctas — **la Metro es la excepción, no
    la regla**;
  - cambiarlo se puede (`pico_set_double_implementation(bpvm_pico compiler)`) y
    cuesta **+23 KB de flash y un 24 % de tiempo** en el banco, que es **1,8×**
    en la aritmética de coma flotante una vez descontada la sobrecarga del
    intérprete (control entero idéntico al milisegundo en las dos corridas).

  **Decisión de Eduardo (17-ago): NO se cambia.** *«Prefiero un 25 % más de
  velocidad y perder un poco de compatibilidad que afecta al 0,01 % de los casos,
  en los extremos, no con valores normales. `double` se va a utilizar en la toma
  de medidas que requieran precisión, pero estamos hablando de instrumentación
  donde tenemos 6 u 8 dígitos significativos como mucho.»* Aplica su propio
  criterio: esto lo pagaría TODO programa que use `double` en la Metro, siempre,
  para proteger un rango que no usa nadie.

  ⚠️ **El caso a vigilar no es escribir `5e-324` a mano** —eso no pasa— sino que
  un cálculo DESBORDE POR ABAJO: en la Metro daría `0` y en el P4 un número
  diminuto, en silencio. Si algún día alguien tropieza con eso, la palanca está
  identificada y medida aquí mismo.

- **L15 — el log post-mortem sobrevive a unos resets y a otros no, y depende de la
  familia.** Desde el 18-ago el log vive en una región de RAM que el arranque no
  borra, así que una autopsia enseña las líneas de ANTES del reinicio — incluidas
  las de un cuelgue, que es para lo que se hizo. Pero esa RAM sólo sobrevive si el
  reset **no corta la alimentación del núcleo**, y ahí las placas no se comportan
  igual:

  | | conserva el log | lo pierde |
  |---|---|---|
  | **ESP32-S3 / P4** | reset por software (el `reset` del IDE), panic/excepción, watchdog | **el botón RST de la placa** y desenchufar |
  | **RP2350 (Pico 2 / Metro)** | el pin de RUN — o sea el botón de reset de la Metro | desenchufar |
  | **STM32** | sin comprobar | desenchufar |

  Lo que sorprende es el ESP32: su botón RST tira del pin EN y corta el dominio
  digital entero, así que cuenta como arranque en frío. **Si persigues un cuelgue en
  un P4 o un S3, reinicia desde el IDE, no con el botón.** El arranque dice siempre
  de cuándo es lo que ha cargado (`RAM SUPERVIVIENTE` frente a `arranque en frío`),
  así que no hay que adivinarlo; y el `Reset` del diálogo de Info dice qué tipo de
  reinicio hubo. El volcado a flash sigue existiendo como red para los cortes de
  corriente, pero sólo llega hasta el último punto de guardado.

  ⚠️ **Y el orden importa**: con un programa en ejecución la placa sólo atiende
  `HELLO` y `KILL`, así que el `reset` del IDE no llega. **Primero `kill`, luego
  `reset`.** El `kill` sí llega aunque el programa esté colgado —está probado sobre
  un bucle cerrado que no cede el turno— y no borra el log, así que la autopsia
  sale entera igual. Que `RESET` se pueda mandar directo queda para V6.

- **L7 — `owner`/`final` no aplican a property de módulo.** Por diseño: `owner`
  pide FREE_REF en cascada (solo campos de instancia); `final` aplica a herencia
  (los módulos no la tienen). Reabrible si surge caso de uso.
- **L9 — `Mutex` no reentrante.** Por diseño (documentado en el manual). Para
  re-entrada, usar otro patrón (flag + condvar).
- **N2 — convención de acceso a `mem[]` / `JavaMutex`.** Cualquier acceso a
  `JavaMutex.{ownerTid, waiters}` debe ir bajo `vmLock` (o acquire/release
  explícito). Hoy se cumple; conviene documentar la regla.
- **N9 — clase sintetizada declarada parcialmente por el usuario.** Si el usuario
  declara `class SyncList` con solo `add`, la suya gana e incompleta; diagnosticar
  la incompatibilidad de firma sería útil.


### Una tupla no entra en una colección (decidido el 21-ago)

`lst.add(pair())` **no compila**. Hasta V4 colaba porque `any` era asignable en los
dos sentidos; desde `#389` la raíz del modelo de objetos es `Object`, y una tupla no
es un `Object`.

**Es una decisión, no un olvido.** La tupla está para **devolver varios valores de
una función**, que es donde gana:

```basic
{ n, s } := pair()          // esto sigue igual de bien
```

Para **guardar** pares hay dos formas mejores, y las dos le ponen nombre a los campos:

```basic
class Par                    // 1) una clase con sus dos propiedades
  public property numero: integer
  public property texto: string
  public function Par(n: integer, t: string)
    this.numero := n
    this.texto  := t
  end Par
end Par

lst.add(Par(42, "hi"))
var p: Par := Par(lst.get(0))
```

```basic
var m: Map := Map()          // 2) un Map, si lo que quieres es buscar por clave
m.put("hi", 42)
```

El ejemplo `samples/TupleFirstClass.bp` lo enseña de las dos maneras.

📌 **Lo mismo vale para los arrays**: un `byte[]` tampoco es un `Object`, así que
`lst.add(newByteArray(n))` no compila. Se envuelve igual, en una clase de una sola
propiedad — es lo que hace `samples/MemInfo.bp`, donde la lista sólo sirve de ancla
contra el recolector.

## 🟢 Pulido (no urgente)

*(El aviso del «pwm» y el truncado mudo del árbol se subieron a `docs/FICHAS.md`
el 17-ago: tienen trabajo pendiente, así que su sitio es el registro.)*

- **M2 — auto-unbox `any → primitive` con check en runtime** (variante "segura"
  de L1; coste: tag de tipo en cada `any`; discutible si compensa).
- **M4 — namespace separado para identificadores sintéticos** (`__prop_get_X`,
  `__strconcat`…; el prefijo `__` ya está reservado — sería un check explícito).
- **M5 — debugger: inspección de properties heredadas** (verificar que se recorre
  la cadena de herencia al inspeccionar; relacionado con N11).
- **M6 — `const` con valor de enum (`const C := Color.RED`)** hoy da "requiere
  literal" (el valor de enum es conocido en compilación). Mejora natural: tratarlo
  como literal e inlinarlo desde `EnumSymbol.values`. (Sale de N17, resuelto: una
  const de clase no-literal ya da diagnóstico limpio en vez de tumbar el emisor.)

## 🔬 Prueba de resistencia larga (idea de Eduardo, 3-ago — post-V4)

Dejar una placa corriendo **días** y ver hasta dónde aguanta. No entra en V4: en
plena campaña de publicación no es razonable, y una prueba así no se improvisa.
Anotado para hacerla con calma.

Lo que la haría útil, y no sólo larga:

- **Que la muerte deje rastro.** Si se cuelga a las 30 horas y no queda registro, se
  han perdido 30 horas. El log post-mortem en flash ya es un anillo que sobrevive al
  reset — hay que apoyarse en él y grabar una marca periódica (vuelta, heap, RTOS
  libre, marca de agua de pila), no sólo los errores.
- **Carga VARIADA, no un bucle.** Repetir una sola operación ejercita un único patrón
  de asignación. Lo que caza fugas y fragmentación es alternar: cadenas, objetos,
  ficheros, hilos, eventos. La batería de H13 ya es ese repertorio.
- **RUN largo Y muchos RUN.** Son dos fallos distintos: lo que se acumula dentro de
  una ejecución y lo que no se devuelve entre ejecuciones. El guardián de fin de RUN
  (#339) sólo ve el segundo.
- **Antes, la versión barata en el PC.** El mismo repertorio en host corre órdenes de
  magnitud más rápido y encuentra gratis lo que sea de memoria pura. No sustituye a
  la placa —no hay flash real, ni dos núcleos de verdad, ni IRQs— pero se paga solo.

Precedente de que el método funciona: #357 se cerró con 10.000 vueltas en la Pico, y
el bug que quedaba sólo se manifestaba al estrechar el heap.
