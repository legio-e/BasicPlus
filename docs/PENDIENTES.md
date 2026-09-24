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
> - **`V6_BACKLOG.md`** / **`V5_BACKLOG.md`** / **`V4_BACKLOG.md`** / **`HECHO_V*.md`** —
>   snapshots inmutables de versiones cerradas.
> - **`V7_BACKLOG.md`** / **`V7_IDEAS.md`** — la versión EN CURSO: lo aplazado y su diseño.
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

  Con un programa en ejecución la placa atiende `HELLO`, `KILL` y —desde V6
  (`#452`)— también `RESET`: el `reset` del IDE llega con un RUN vivo en las cinco
  familias, mata el programa y reinicia después. `kill` + `reset` sigue valiendo. El
  `kill` llega aunque el programa esté colgado —está probado sobre un bucle cerrado
  que no cede el turno— y no borra el log, así que la autopsia sale entera igual.

- **L17 — un `catch` sin tipo ES `catch e: Exception`** (V6, 30-ago). No es un caso
  aparte: el compilador le pone ese tipo al AST, así que `catch e` y
  `catch e: Core.Exception` compilan a lo mismo. Consecuencias prácticas:
  - `e.msg` funciona sin escribir el tipo;
  - **imprimir la excepción muestra su mensaje** (`Exception.toString()` devuelve `msg`),
    no `object@1234`;
  - y como `Exception` es un tipo de `Core`, un módulo que use `catch` necesita
    `import Core` — ver **L16**.

  No se pierde ninguna captura: `throw` sólo acepta instancias de `Exception`, y el
  compilador lo rechaza si no lo es.

- **L16 — si usas un tipo de `Core`, tienes que escribir `import Core`** (V6, 30-ago).
  Vale para `List`, los envoltorios de los primitivos (`Integer`, `Long`, `Float`,
  `Double`, `Boolean`), `Comparable`, `Exception` y `RuntimeError` — o sea que **atrapar
  un error también pide el import**:

  ```basicplus
  module L16
    import Core          // <- hace falta para el catch de abajo y para List

    function leer(): List
      var l: List := List()
      try
        l.add("uno")
        throw RuntimeError("fallo de prueba")
      catch e: RuntimeError
        l.add(e.msg)
      endtry
      return l
    end leer

    public function Main()
      var l: List := leer()
      print l.length()
    end Main
  end L16
  ```

  (Compila y ejecuta tal cual: imprime `2` en las dos VMs.)

  **Antes no hacía falta**: el compilador inyectaba el `import` por su cuenta. Se quitó
  porque lo inyectaba en una de sus dos pasadas y no en la otra, y eso hacía que un módulo
  que expusiera un tipo de `Core` en una firma pública **perdiera ese miembro** al ser
  importado por otro — con el error apareciendo en el consumidor, lejos de la causa.
  Norma de Eduardo: *«la norma tiene que ser sencilla: si se utiliza un tipo de Core, se
  ha de importar Core»*.

  **`Object` NO lo necesita**: es un tipo real, con `toString()` y `compareTo()`, pero
  vive en el lenguaje y no en `Core` — como `integer` o `string`.

  Si se te olvida, lo que dice el compilador depende de por dónde tropiece:
  - usas `List` sin el import → `tipo 'List' no encontrado` (y detrás
    `no se puede llamar a 'List'`);
  - `catch e: RuntimeError` sin el import → `tipo de excepción 'RuntimeError' no es
    una clase`.

  **La regla es transitiva, y los imports no lo son** (`#492`, `#498`). Si importas un
  módulo cuyas clases extienden un tipo de `Core` —`Collections`, cuyas `SyncList`,
  `OwnerList`… extienden `Core.List`— tienes que importar `Core` también, aunque tu
  código no nombre `List`. Aquí el mensaje sí señala el arreglo:

  ```
  la clase 'SyncList' extiende 'Core.List' y este modulo no importa 'Core': anade `import Core` (los imports no son transitivos)
  ```

- **L18 — lo que cruza la frontera de un módulo, y lo que no** (V6, `#492` → ✅ V7,
  14-sep-2026). Desde V7 la interfaz de un módulo exporta **todo lo que un descendiente
  puede tocar**: métodos, propiedades, **los campos protegidos**, **todas las
  sobrecargas del constructor**, **los métodos y properties estáticos** y las
  constantes — con `final` y `private` respetados. Los dos primeros límites de V6 ya
  no existen (`class Gato extends Base.Animal` en otro módulo ve `var nombre`;
  `Base.Animal()` y `Base.Animal("Rex")` valen las dos, y `super(...)` elige por
  firma). Lo que sigue dentro:
  - **una `native` no lee campos ni llama a estáticos** (ni propios ni heredados): el
    compilador lo rechaza con un error claro, en una native de módulo y en un método
    `native` de clase (`#519`, V7). Rodeo: un getter.
  - **una `var` estática no cruza** (ni de módulo ni de clase): vive en el segmento
    de datos del módulo, que sólo vale para su propio código. El estado sale del
    módulo por getters y setters: declárala `property` (protegida) y cruza.
  - **`Clase(o)` con `o: Object` decide cast-o-construcción mirando sólo la primera
    sobrecarga del constructor** (`#389`): si hay un `Animal(o: Object)` que no es la
    cabeza, `Animal(x)` se toma como cast.
  - **los imports no son transitivos.** Si `Mid` importa `Base` y te devuelve un
    `Base.Animal`, para nombrar ese tipo —o llamar a sus métodos— tu módulo tiene que
    escribir `import Base` él mismo; si no, `tipo cualificado 'Base.Animal' no
    encontrado` (o `el tipo 'Base.Animal' no tiene miembros`). Es la misma regla de
    L16 con `Core`.

- **L7 — `owner`/`final` no aplican a property de módulo.** Por diseño: `owner`
  pide FREE_REF en cascada (solo campos de instancia); `final` aplica a herencia
  (los módulos no la tienen). Reabrible si surge caso de uso.
- **L9 — `Mutex` no reentrante.** Por diseño (documentado en el manual). Para
  re-entrada, usar otro patrón (flag + condvar).


### `input()` sólo funciona en el PC, no en el micro (decidido el 26-ago; `listDir` YA está en las dos desde V7/H3)

`input()` / `IO.prompt()` abre su ventana en el IDE cuando el programa corre en la **VM
Java**. Es un builtin del compilador (se llama sin `import`) y **compila igual para todo
el mundo**, pero en la **VM-C —o sea en cualquier placa y en el micro simulado— lanza un
`RuntimeError` atrapable**:

```
builtin 37 no soportado en esta VM (subconjunto C)      // input()
```

Si no lo atrapas, el programa **termina con código de salida 1** (`RuntimeError BP no
atrapado`), igual que cualquier otra excepción sin `catch`. `IO.prompt()` en la VM-C lanza
`prompt: no hay IDE conectado`. Es el único builtin del lenguaje que no está en la VM-C
(además de `heapFrag`/`heapMap`, que son diagnóstico de la VM Java): el resto — ficheros,
rutas, matemáticas, cadenas — corre igual en las dos. `listDir(path)` **también** desde
V7/H3 (21-sep): en las dos VMs devuelve los nombres **ordenados por bytes** (el orden de un
listado es del FS, y el mismo programa tiene que imprimir lo mismo en el PC y en la placa)
y lanza `RuntimeError` si el directorio no se puede abrir (antes miVM devolvía una lista
vacía en silencio).

**Se queda así a propósito.** El razonamiento de Eduardo: `print` e `input` no son entrada
y salida de verdad en un micro —son **herramientas de depuración**, anteriores al
debugger—, y un `input()` era el «breakpoint del pobre» para parar un programa. Con el
debugger de verdad funcionando, eso ya está mejor resuelto.

⚠️ **Lo que hay que saber al escribir un programa**: si va a correr en placa, no uses
`input()` para pedir datos. Para interactuar con el usuario en un micro está la GUI
(formularios), y para pausar y mirar, el debugger. Un programa que quiera correr en los dos
sitios puede envolver la llamada en `try … catch e: RuntimeError`.

📌 El coste de igualarlo no era pequeño: `input()` bloquea el hilo BP (estado
`BLOCKED_PROMPT` y salida de la cola de ejecución hasta que llega la respuesta), así que en
la VM-C tocaría el planificador, no la tabla de builtins.

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

var lst: List := List()
lst.add(Par(42, "hi"))
var p: Par := Par(lst.get(0))
```

```basic
var m: Collections.Map := Collections.Map()   // 2) un Map, si lo que quieres es buscar por clave
m.put("hi", Integer(42))                      //    (import Collections, y Core por L16)
```

El ejemplo `samples/TupleFirstClass.bp` lo enseña de las dos maneras.

📌 **Lo mismo vale para los arrays**: un `byte[]` tampoco es un `Object`, así que
`lst.add(newByteArray(n))` no compila. Se envuelve igual, en una clase de una sola
propiedad — es lo que hace `samples/MemInfo.bp`, donde la lista sólo sirve de ancla
contra el recolector.

## ⏭️ Aplazado a V7 — lo que conviene saber al programar hoy

Decisiones de V6 que no son bugs sino piezas que no están. El estado de cada una vive en
su ficha (`docs/FICHAS.md`); aquí, lo que cambia al escribir un programa.

- **No hay bajo consumo** (`#490`). El micro nunca duerme: sin nada que hacer sigue
  girando (y en el STM32 la tarea ociosa ni siquiera llega a correr). No existe un
  `sleep` de placa ni fuentes de despertar en BP: un programa a batería no puede
  ahorrar hoy. Es de **V8**, con la WiFi y el Bluetooth.
- **`GET` y `LS` contestan `BUSY` mientras corre un programa** (`#495`). El
  explorador de ficheros del IDE no lista ni baja nada durante un RUN; `HELLO`, `KILL` y
  `RESET` sí llegan. Los ficheros que escribe el programa (un `.shot`, un log) se bajan
  al terminar o tras `kill`.
- **Una dependencia más antigua que el módulo principal pasa en silencio** (`#500`).
  El cargador rechaza un `.mod` de formato incompatible y el enlazador grita si un slot
  no cuadra, pero no mira la **versión** de la stdlib con la que compilaste: un `Core`
  del mismo formato pero más viejo —el de un pack, o una copia olvidada en `/app`, que
  tapa a `/lib` (`#493`)— se carga sin aviso. Mientras llega la norma: no dejes módulos
  de stdlib en `/app` ni packs con stdlib dentro en la placa; el log del RUN dice de
  dónde carga cada dependencia (`dep 'Str' -> /app/Str.mod`).
- **Una `native` larga no se puede parar**: la VM atiende el Stop (`KILL`) entre dos
  instrucciones del intérprete, y mientras corre código nativo no hay dónde mirarlo. Una
  `native` que tarde segundos deja el Stop esperando hasta que vuelva (`#379`: medido en el
  P4 con tarjeta, 8 de 8 Stop limpios con código interpretado). Si necesitas parar un cálculo
  largo, hazlo interpretado o pártelo en llamadas cortas.
- **En el ESP32-S3 todo se ejecuta interpretado** (`#488`, fuera del plan). El AOT
  genera ARM Thumb-2 y RISC-V; **no genera Xtensa**, y de momento no se va a hacer. Las
  `native function` compilan y corren, pero a la velocidad del intérprete. **El ESP32-C3
  y el C6 también corren interpretados** (`#502`): son RISC-V **sin FPU** y el único
  destino RISC-V del IDE es el del P4 (con FPU, `ilp32f`); su firmware (`ilp32`) no carga
  ningún bloque nativo, así que un blob del P4 se ignora. Dar de alta su destino (`ilp32`)
  quedó fuera de V6.
  Aceleran hoy: RP2350, STM32 y ESP32-P4 (en el PC —VM Java y micro simulado— todo es
  interpretado, y así se comparan los tiempos).

## 🟢 Pulido (no urgente)

*(El aviso del «pwm» y el truncado mudo del árbol se subieron a `docs/FICHAS.md`
el 17-ago: tienen trabajo pendiente, así que su sitio es el registro.)*

- **M2 — auto-unbox `any → primitive` con check en runtime** (variante "segura"
  de L1; coste: tag de tipo en cada `any`; discutible si compensa).
- **M5 — debugger: inspección de properties heredadas** (verificar que se recorre
  la cadena de herencia al inspeccionar; relacionado con N11).

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
