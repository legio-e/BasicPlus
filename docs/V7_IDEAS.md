# V7 — ideas y diseño

> 🚀 **Desde el 13-sep-2026, V7 es la versión en curso** (V6 se publicó el 12-sep, tag
> `v6.0`). Éste es el cuaderno de diseño vivo del hito, hermano de `V6_IDEAS.md`: las
> charlas de diseño van aquí **antes** de escribir código. El estado vive en `FICHAS.md`
> §ABIERTAS (bloque V7) y el índice de lo aplazado en `V7_BACKLOG.md`.
>
> Documento abierto el 21-ago con la visión de Eduardo sobre el código nativo en packs.
> **Va en `docs/` y no en `notas/` a propósito**: `notas/` está en el `.gitignore` y por
> esa vía ya se perdió un fichero (el `SqlDemo.bpbuild`, reconstruido el 19-ago).
>
> ℹ️ La última sección («Sistematizar las pruebas EN PLACA», 29-ago) **se hizo en V6**: es
> el hito `T1` (`tools/tanda.py`, 11-sep). Se deja como está porque es el diseño del que salió.

---

## Código nativo empotrable: cuántos, cómo publican su API, y si hablan entre ellos

### La visión, en palabras de Eduardo (21-ago)

> *«En Linux el núcleo del sistema te lo puedes hacer un poco a tu gusto, pero obliga una
> vez decidido qué va dentro a compilarlo todo. **Nosotros vamos en otra dirección:
> código independiente que ya sabemos recolocar en una dirección concreta.** Lo que hay
> que mejorar es **cómo se publica su API** y **cómo interactúa con el resto del
> sistema**.»*

Y el encargo: primer diseño en V7, **poniéndolo en marcha con LVGL**.

📌 **Precisión suya que cambia la unidad de cuenta**: la memoria y la identidad no son
*por pack* sino **por código nativo**. Un pack puede llevar más de uno.

### Lo que YA está construido (V5/H4) — y es más de lo que parece

El **punto de encuentro** de la tabla BIOS, dos ranuras:

```c
int         (*publica)(uint32_t marca, const void* tabla);
const void* (*busca)  (uint32_t marca);
```

Y su comentario ya declara la propiedad que Eduardo persigue, con LVGL nombrado:

> *«son LO ÚNICO que la VM sabe de todo esto. No conoce SQLite, ni LVGL, ni ningún driver
> de pantalla: sólo sabe guardar un puntero bajo una marca y devolverlo. **Por eso añadir
> un pack nuevo NO toca la VM**»*

- **Marca de 4 bytes** (`'SQLI'`, `'LVGL'`, `'PANT'`), comparada con `==`. Se eligió
  frente a un nombre para no depender de una convención que alguien escribiría mal.
- **Se ve funcionando hoy**: `pack: sqlite: API publicada como 'SQLI' — 17 simbolos, v1`.
- **`busca()` ES el mecanismo de que hablen entre sí.** La pregunta *«¿se podrían hablar
  unos con otros?»* está contestada en el mecanismo; lo que falta es la disciplina
  alrededor (ver abajo).
- **¿Cuántos? Hoy CUATRO**: `#define BPVM_PACK_MAX 4`, un array fijo de (marca, tabla).

### Lo que hay que mejorar, que es justo lo que dice Eduardo

**1. La API se publica como `const void*` — la VM no puede comprobar NADA.**
El contrato dice que la tabla *«DEBE empezar por su propia marca y versión»*, pero es una
**convención, no un chequeo**: `publica` acepta cualquier puntero. Si un pack publica una
tabla con otro layout, el que la consume salta a donde no debe.
⏭️ Lo barato y de mucho valor: que `publica` **lea esos primeros bytes y verifique** que
la marca declarada coincide con la que se pasa, y que la versión es la esperada. Convierte
la convención en reja, y encaja con el criterio que el proyecto ya aplica en tres sitios
(el gate del `.mod` `#284`, el `magic`/`version` de la propia BIOS, y el sello del `.npk`).

**2. Nadie ordena la carga.** Si LVGL necesita el driver de pantalla, hoy no hay nada que
garantice que el driver publicó antes.
⏭️ **Propuesta: resolver PEREZOSAMENTE, no al cargar.** Que cada uno haga `busca()` en su
primer uso y no en su arranque. Así el orden deja de importar y desaparece la necesidad de
declarar dependencias entre packs — que sería el camino de Linux, el que Eduardo
descarta. Es también más honesto: el fallo aparece cuando de verdad hace falta la pieza.

**3. Cuatro ranuras.** Subir el número es una línea, pero la pregunta buena no es cuántas
sino **si el array fijo es la forma correcta** cuando el catálogo crezca (SQLite, LVGL, el
panel, red…). Con 4 y comparación por `==` el coste es cero; conviene medir antes de
cambiarlo por algo con más ceremonia.

**4. La RAM, y la corrección de Eduardo.** Ver la nota de `docs/V6_IDEAS.md` sobre la
arena: la tercera vía ya está elegida y corriendo, pero **la unidad debe ser el código
nativo, no el pack**, y hoy la arena es singular y se llama `SQLite`.

### Por qué la dirección es distinta a la de Linux, y qué implica

Linux te deja elegir qué entra en el núcleo, pero **una vez elegido hay que compilarlo
todo junto**. Aquí el código nativo se compila aparte, se graba, y se **recoloca** — eso
ya funciona y está probado en placa (el `.npk` de SQLite, ARM y RISC-V).

⚠️ **Lo que esa elección hace caro, y hay que asumir**: sin enlazado global no hay quien
compruebe que las dos partes encajan. Todo lo que un enlazador te da gratis —firmas,
símbolos, versiones— aquí hay que **declararlo y comprobarlo en runtime**. De ahí que el
proyecto ya tenga cuatro rejas distintas (ABI del `.mod`, `magic`/`version` de la BIOS,
sello del `.npk` con `float_abi`, y la marca del punto de encuentro). El diseño de V7 es,
en el fondo, **decidir cuánta reja más hace falta** para que la API publicada sea tan
segura como una llamada enlazada — sin volver a compilarlo todo junto.

### Empezar por LVGL: por qué es un buen primer cliente

- Es **grande y ajeno**, como SQLite, así que vuelve a probar el camino completo.
- Tiene **dos** interlocutores naturales: la VM (que hoy le llama por builtins) y el
  driver de pantalla. O sea que ejercita `busca()` de verdad, que SQLite no hizo.
- Y ya hay una ficha que empuja en la misma dirección: **`#434`, desacoplar los eventos
  del lazo de LVGL**. Conviene mirarlas juntas.

---

## Los cabos sueltos del AOT: errores, líneas, parar, y el debugger (Eduardo, 21-ago)

> *«Según vayamos avanzando, con cada vez más código AOT hay cabos que quedan sueltos:
> qué pasa cuando hay un error, cómo se puede localizar en qué línea ocurrió, ¿podemos
> parar un código AOT?, perdemos debugger.»*

Planteado **fuera de V6**, como idea. Y es la pregunta correcta en el momento correcto:
hoy el AOT cubre funciones sueltas de cálculo y estos cabos casi no se notan; el hito de
V6 amplía la cobertura, y **cuanto más código sea nativo, más pesan**. Aquí queda lo que
ya está resuelto y lo que no, **medido el 21-ago**, para no volver a suponerlo.

### ✅ «Qué pasa cuando hay un error» — esto YA está resuelto

`#186` arma un **boundary de fault** con `setjmp` alrededor de cada llamada a un thunk
(`interp.c:74`). Un `throw` desde nativo hace `longjmp` a ese boundary y se reconvierte en
excepción de BP, que se propaga al `try/catch` que envuelva la llamada. De ahí que
*«dividir por cero desde código nativo lance un error de BP atrapable en vez de reiniciar
la placa»*. Y `#213` añadió que pueda propagarse una excepción **ya construida**
(`pending_ref`), no sólo un `RuntimeError` genérico.

📌 Detalle de diseño que merece no perderse: el `setjmp` vive en el *wrapper* y no en el
intérprete **a propósito**, para que los registros calientes del hot loop no haya que
marcarlos `volatile`.

### ❌ «En qué línea ocurrió» — NO, y se ve en la struct

```c
typedef struct {
    jmp_buf      buf;
    char         msg[128];      /* el mensaje… */
    volatile int armed;
    volatile uint32_t pending_ref;
} bpvm_aot_fault_t;             /* …y ni un campo de LÍNEA */
```

El fault viaja con mensaje pero **sin origen**. En bytecode la línea la da el `.dbg`; el
código nativo no es bytecode, así que ese mapa no aplica. ⏭️ Lo que habría que decidir:
si el `.mdn` (o el `.mod` fundido, ya que V6 los une) lleva un **mapa dirección→línea**
del estilo del `.dbg`, aunque sea grueso — por función y no por instrucción.

### ❌ «¿Podemos parar un código AOT?» — NO, y esto es lo más gordo

El thunk se llama directo y **corre hasta terminar**. No hay frontera de quantum dentro,
con tres consecuencias que hoy apenas se notan y mañana sí:

1. **`kill` no lo alcanza.** Se aborta entre instrucciones de bytecode; dentro de un
   nativo no hay dónde.
2. **El planificador no puede expropiarlo**: mientras corre, los demás hilos esperan. Un
   nativo largo de cálculo se come el reparto.
3. Y por tanto **un bucle infinito en nativo no se recupera** salvo desenchufando.

📌 **Matiz importante y medido**: esto vale para el nativo *puro de cálculo*. Si el nativo
**llama de vuelta a BP**, el intérprete corre ANIDADO (`interp.c:188`) y ahí sí hay
quanta. O sea que la ventana ciega es exactamente la del código que no vuelve a BP — que
es justo el que más interesa acelerar.

⏭️ Ideas a sopesar, ninguna gratis: un **chequeo periódico** que el emisor inserte en los
bucles (cuesta velocidad, que es lo que se venía a comprar); un **límite de tiempo** con
watchdog que sólo sirve para abortar, no para parar limpio; o **asumirlo y decirlo** —
documentar que una `native` es atómica frente al planificador, que es una promesa clara
aunque incómoda.

### ❌ «Perdemos debugger» — sí, y hay que decidir qué se ofrece a cambio

Los breakpoints son de bytecode: la VM para en una instrucción y el host traduce con el
`.dbg`. Dentro de un thunk no hay instrucciones que parar. Con el AOT limitado a
funciones de cálculo se nota poco; con métodos y statements normales dentro (hito de V6)
pasa a ser **la diferencia entre depurar tu programa y no poder**.

⏭️ Lo que hay que decidir, y conviene decidirlo ANTES de ampliar mucho la cobertura:
- ¿Se puede poner un breakpoint **en la frontera** (entrada y salida de la nativa), aunque
  no dentro? Es barato y ya cubre bastante.
- ¿Debería el IDE **avisar** de que una función marcada `native` no es depurable, igual
  que avisa de otras cosas?
- ¿O `native` debería **desactivarse en modo depuración** y compilar interpretado, de modo
  que depures lo mismo que ejecutas… salvo la velocidad? Es lo que hacen otros entornos, y
  choca de frente con *«el mismo binario en el PC y en el micro»*, que es un invariante
  del proyecto. **Es una decisión de fondo, no un detalle.**

📌 **La observación que enmarca todo esto**: el AOT no es sólo una optimización, es una
**región del programa donde las garantías del runtime cambian** — no se para, no se
depura, y hasta hace poco tampoco se sabía dónde falló. Mientras sea un rincón para `fib`,
da igual. El hito de V6 lo convierte en territorio, y entonces cada uno de estos cabos
pasa de curiosidad a requisito.

### ⏭️ La propuesta de Eduardo (24-ago): UNA marca que sirve para tres cosas

> *«Si tenemos funciones native 100×100 compatibles, ¿cuál es el siguiente paso?
> Tendríamos un híbrido de funciones native envueltas en un sistema BP. Ganamos velocidad,
> pero perdemos el debug y la traza, y el poder detener una función native, y el cambio de
> contexto en un sistema multi-thread BP. Perdemos demasiadas cosas.*
>
> *Un pequeño parche que se me ocurre es, con el modo debug activado, insertar pequeñas
> entradas para indicar número de línea del source, para poder localizar los errores pero
> también para dar paso al intérprete para que pueda hacer cambios de thread. A ver,
> penalizará el rendimiento, habrá que medirlo o estudiar una solución mejor.»*

**Lo que aporta sobre lo de arriba**: los cuatro cabos estaban listados por separado, cada
uno con su idea. Esto los ata: **una sola marca** —un punto seguro que el emisor inserta—
resuelve tres a la vez. Sabe la línea (traza y errores), y al pasar por ella el intérprete
puede tomar el control (cambio de hilo, `kill`, breakpoint).

#### 🟢 Y una pieza que el 21-ago no estaba: la SALIDA existe

Aquel día la lista de ideas no tenía cómo *salir* del nativo una vez detectado que hay que
parar. **Medido el 24-ago con los dos toolchains reales**: `__builtin_setjmp` /
`__builtin_longjmp` de gcc se expanden **inline** — cero símbolos indefinidos, y el `.o`
empaqueta (ARM 52 B, RISC-V 60 B). O sea que el salto no-local dentro de un `.mdn` es
posible, y es la misma pieza que hace falta para el `try`/`catch` de dentro de una native.
**Dos cabos, un mecanismo.**

⚠️ Sin dar por bueno más de lo medido: el experimento salta **dentro de la misma función**.
Lo que aquí hace falta es saltar desde OTRO marco, y `__builtin_setjmp` está pensado para
uso interno de gcc y no guarda los callee-saved como el `setjmp` de verdad.

#### 📐 Un matiz que cambia el coste: NO todo necesita la misma densidad

La propuesta habla de una marca por línea. Pero los tres usos no piden lo mismo, y
mezclarlos hace parecer caro algo que puede no serlo:

| para qué | dónde hace falta la marca | coste |
|---|---|---|
| **expropiar / `kill`** | sólo en los **saltos hacia atrás** (un bucle es lo único que puede correr sin fin) | una comprobación por vuelta; el código en línea recto no paga nada |
| **traza y línea del error** | por sentencia | ahí sí se nota |
| **breakpoint dentro** | por sentencia | idem |

📌 O sea que **lo que hace falta siempre —no colgar el planificador— es lo barato**, y lo
caro es lo que sólo se quiere al depurar. Es lo que hacen las máquinas virtuales con
*safepoints*: sondeo en los back-edges y en los retornos, no en cada instrucción.

#### ⚠️ Y el riesgo de fondo: depurar un artefacto que no es el que se ejecuta

Si el modo depuración cambia el código generado, **el `.mdn` que depuras no es el que
envías** — y el bug que persigues puede no existir en la variante con marcas. Choca con el
instinto del proyecto (el invariante dual-VM, los gates de ABI: *lo que pruebas es lo que
publicas*).

🟢 **Pero aquí hay suerte, y viene de V6/N1.4**: la sección `native` del `.mod` **ya es
multi-bloque y autodescriptiva** — hoy un bloque por familia, y el cargador se queda con el
suyo mirando su campo `arch`. Llevar **las dos variantes** (con marcas y sin ellas) no pide
rediseñar nada de la sección: pide un campo más en la cabecera del `.mdn` para
distinguirlas. El mismo `.mod` podría depurarse y ejecutarse a toda velocidad sin
recompilar.

#### ⏭️ El primer paso cuando se retome, y es barato

**Medirlo**, que es lo que pide Eduardo. El banco ya existe: `samples/NatV7.bp` cronometra
`sumaHasta(20000)` y en la Pico dio **1 ms** (24-ago). Añadir la comprobación en el
back-edge y volver a medir el mismo sample da el número real en una tarde — y **el número
es el que decide** si esto es un parche o hay que estudiar una solución mejor. Suponer el
coste antes de medirlo sería repetir el error que este documento lleva anotado tres veces.

*(Sin fecha, decisión de Eduardo: «todo eso para el futuro».)*

➡️ **26-sep**: el cambio de hilo y el `kill` dentro de una native, **decididos** en §H5.P2b —la native se suspende como
una interrupción, en una pila que le presta el intérprete; idea de Eduardo—. La línea del error y el breakpoint quedan
con la base puesta: es la misma marca.

---

## El lazo de LVGL, y por qué es la MISMA enfermedad que el AOT (Eduardo, 21-ago)

> *«El bucle principal de LVGL, una vez que entramos en él como que no hay manera de
> salir. Creo que debemos crear nuestro propio bucle, que haga todo lo que hace el de
> LVGL pero que nosotros podamos interrumpir, parar, suspender y reanudar a voluntad. De
> esa manera el stop de la línea de comandos debería funcionar, pero si hay más de un
> Thread, que los otros hilos puedan trabajar correctamente.»*

### La mitad que YA está hecha (y conviene saberlo antes de rehacerla)

**No entramos en el lazo de LVGL: ya tenemos el nuestro.** `BUILTIN_GUI_RUN`
(`builtins.c:921`) drena eventos, polea el wire y llama a `bpvm_gui_lvgl_pump()` **una
iteración cada vez**. LVGL no manda: se le pide una vuelta.

Y el `stop` **ya funciona**, por `#257`, con el porqué escrito en el propio código:

> *«KILL durante `Gui.run()`: el scheduler no corre quanta mientras este builtin bombea,
> así que poleamos el wire aquí mismo (el MISMO `poll_cb` que el scheduler usa entre
> quanta). Al romper caemos al push+return → el quantum termina → el scheduler ve
> `kill_requested` y devuelve `BPVM_KILLED` (parada limpia entre opcodes).»*

### La mitad que NO está, y está nombrada en esa misma frase

**«El scheduler no corre quanta mientras este builtin bombea.»** Ése es exactamente el
segundo requisito de Eduardo, y hoy no se cumple: con la GUI viva, **los demás hilos BP
no avanzan**. El `poll_cb` se metió para poder ABORTAR, no para REPARTIR.

📌 **Y ahí está la lección que vale para los dos casos**: polear el wire dentro de un lazo
resuelve *parar*; **no resuelve *compartir***. Son dos problemas distintos con la misma
apariencia, y el arreglo de uno no da el otro gratis.

### Por qué es la misma enfermedad que el AOT

| | `Gui.run()` | thunk AOT |
|---|---|---|
| ¿se puede abortar? | ✅ sí, `poll_cb` en cada vuelta (`#257`) | ❌ no hay dónde preguntar |
| ¿corren otros hilos? | ❌ no | ❌ no |
| forma del problema | un builtin que no vuelve | una función que no vuelve |

Las dos son **código que se queda con la CPU y deja al planificador sin sitio donde
entrar**. Y el remedio de fondo es el mismo en los dos: que el lazo **ceda al
planificador**, no sólo que pregunte si debe morir.

### Qué habría que hacer, tal como se ve hoy

⏭️ Que la vuelta del lazo de la GUI **corra quanta de los demás hilos** entre bombeo y
bombeo, en vez de sólo preguntar por el KILL. La pieza existe —el scheduler ya sabe correr
un quantum— y el lazo ya está en nuestras manos; lo que falta es llamarlo desde ahí.
⚠️ **La trampa a medir antes**: LVGL no es reentrante, y hoy hay un comentario que lo dice
(*«el pump (hilo del `lv_timer_handler`) para no reentrar LVGL»*). Si un quantum de otro
hilo ejecuta código BP que toca la GUI, se reentra por la puerta de atrás. O sea que
repartir el tiempo exige decidir **qué pueden hacer los otros hilos mientras la GUI vive**
— y eso es diseño, no una llamada más.

📌 **Emparenta directamente con `#434`** (desacoplar los eventos del lazo de LVGL), que ya
está en V6: quien toque uno va a estar mirando el otro. Conviene abrirlos juntos.

🔴 **Y la prioridad, en palabras de Eduardo**: de todos los cabos, *«quizás el más
importante sea que al menos podamos detener un bucle, porque si no eso se puede convertir
en un bucle infinito y el usuario lo va a traducir en un cuelgue»*. Con razón: un cuelgue
no se distingue de una avería, y en la GUI ya está resuelto — **el que queda descubierto
es el AOT**.

### 💡 El modelo de Swing: `invokeLater`, y por qué encaja (Eduardo, 21-ago)

> *«Si no recuerdo mal, en el Swing de Java tienen un "ejecutar más adelante"… utilizan
> una cola de mensajes donde se pueden acumular tareas que se despachan cuando el Swing
> tiene tiempo.»*

**Y media pieza ya está construida.** La cola de eventos de la GUI (`gui.c:1096-1112`) es
un anillo `(objptr, kind)` con `ev_push` de un lado y `bpvm_gui_next_event` del otro, que
**drena `Gui.run()` en cada vuelta**. Esa es exactamente la forma del EDT: un hilo de UI
que vacía una cola cuando tiene tiempo.

**Lo que falta es la otra dirección**, que es justo lo que aporta `invokeLater`:

| | hoy | lo que añadiría Swing |
|---|---|---|
| sentido | eventos que **entran** (un clic → `onClick`) | trabajo que **cualquier hilo encola** para el hilo de la UI |
| quién produce | el backend (LVGL/SDL) | cualquier hilo BP |
| qué se encola | `(objptr, kind)` | una tarea: algo que ejecutar |

🔑 **Y con eso el problema de reentrada se disuelve, en vez de resolverse.** Antes había
que decidir *«qué pueden hacer los otros hilos mientras la GUI vive»*, que es diseño
delicado porque LVGL no es reentrante. Con el modelo de Swing la regla cabe en una línea:
**los otros hilos pueden hacer todo menos tocar la GUI; para eso, encolan.** Un hilo sólo
toca LVGL: el que bombea.

### Dos detalles concretos, sacados de la cola que ya existe

⚠️ **1. La cola de hoy DESCARTA cuando se llena**: *«cola llena: descarta (como `offer()`
de miVM)»*. Para eventos de interfaz es defendible —un clic perdido con la cola a tope es
mejor que bloquear el backend—. **Para tareas de `invokeLater` NO lo es**: perder trabajo
en silencio es un bug, no una política. Si se generaliza la cola, ese comportamiento hay
que decidirlo aparte para cada uso.

✅ **2. La parte difícil ya está resuelta, y conviene saberlo**: `bpvm_gui_visit_roots`
(`gui.c:1114`) ya enumera **los eventos pendientes en la cola como raíces del GC**, bajo
stop-the-world. O sea que el patrón *«lo encolado mantiene vivo lo que referencia»* está
hecho y probado. Una cola de tareas que lleve referencias entra por el mismo sitio.

⏭️ **Lo que quedaría por decidir**: qué es «una tarea» en BP. Swing encola un `Runnable`;
aquí lo natural sería una referencia a método o un objeto con un método conocido —y ahí
entra `MethodRefExpr`, que hoy tampoco soporta el AOT (ver el censo de `AOT_LIMITES.md`).
Otra vez la misma pieza asomando por dos sitios distintos.

### 💡💡 «Comandos» como solución intermedia (Eduardo, 21-ago)

> *«Encolar métodos diferentes con diferentes parámetros es difícil. Como solución
> intermedia podemos utilizar "comandos": tienen una interfaz definida, se pueden
> activar/desactivar, y si los hacemos asíncronos se podrían encolar.»*

**Resuelve exactamente la parte difícil.** En vez de encolar `(método, parámetros)`
heterogéneos —que obliga a marshallar firmas distintas—, se encolan **objetos de un tipo
conocido que llevan sus parámetros dentro**. El despacho pasa a ser **una sola llamada
virtual**, que es algo que la VM ya hace en cada método.

📌 **Y Swing hizo literalmente esto.** Encima de los listeners crudos añadió
`javax.swing.Action`: un comando con `setEnabled(boolean)`. El «activar/desactivar» de
Eduardo es el mismo que Swing acabó necesitando, por el mismo motivo — un botón y una
entrada de menú que hacen lo mismo comparten el comando, y se habilitan a la vez.

#### La forma que encaja HOY, sin tocar el lenguaje

⚠️ **BP no tiene interfaces de clase ni clases abstractas**: la gramática las lista como
*«IDEAS para v3 (no en el lenguaje)»*. Las `module interface` son de módulo, no de clase.

✅ **Pero la vía de la clase base funciona, y hay precedente en la propia stdlib**:
`Core.Comparable` es una base cuyos métodos lanzan por defecto y se sobrescriben en los
cinco envoltorios (`toInteger`, `toLong`, `toDouble`, `toBoolean`, `toString`, añadidos el
20-ago). Un `Command` con `ejecutar()` y `habilitado` es la misma forma exacta.

```basic
class Command
  public property habilitado: boolean
  public function ejecutar()        // la base lanza; cada comando la sobrescribe
end Command
```

#### ⚠️ El aviso que sale de la experiencia de ayer, y no es menor

**Meter una clase base nueva en la stdlib es un EVENTO DE ABI.** El 20-ago, añadir cinco
métodos a `Comparable` **corrió las ranuras de su vtable** y con ellas las de todo lo que
la extiende: `DaoDemo` y `GenDemo` compilaban y **fallaban en ejecución** hasta reconstruir
la librería de SQLite contra el `Core` nuevo. Y en placa el desfase sobrevive a lo que
tengas grabado.

⏭️ **Consecuencia práctica**: si entra un `Command`, que entre **completo de una vez** —con
los métodos y propiedades que se le prevean— y no creciendo de uno en uno. Cada método
añadido después es otra corrida de ranuras y otra tanda de reconstrucciones.

#### Lo que queda por decidir

- **¿La base lanza o no hace nada?** `Comparable` lanza, y para conversiones es correcto
  (fallar es informativo). Para un comando, un `ejecutar()` que no hace nada puede ser un
  fallo mudo; conviene que lance.
- **Qué pasa con la cola llena.** Ya apuntado arriba: descartar un clic se defiende;
  descartar un comando, no.
- **Y una pregunta de alcance**: ¿el comando es sólo para la GUI, o es el mecanismo general
  de *«ejecutar esto en aquel hilo»*? Si es lo segundo, deja de ser una pieza de la GUI y
  pasa a ser del lenguaje — que es más potente, y también más caro de equivocarse.

---

## Sistematizar las pruebas EN PLACA — un arnés que recorra la matriz (Eduardo, 29-ago) — ✅ HECHO EN V6 como `T1` (`tools/tanda.py`, 11-sep)

### El problema, y cómo se manifestó

Balance de Eduardo tras la campaña de memoria del 27-29 de agosto:

> *«Este problema no apareció en V5 porque estábamos con SQLite y utilizamos la Metro
> para las pruebas, y nos confiamos en que la Metro y la Pico tienen la misma imagen.
> OK, un error nuestro. Pero hacer test sobre todas las placas es mucho trabajo. Lo que
> hace falta es una herramienta de test que nos permita sistematizar las pruebas.»*

Y el matiz que lo hace peligroso: **la imagen SÍ era la misma**. Lo que cambia no es el
binario, es la *realidad de memoria* — la Metro lleva el heap a la PSRAM y deja la SRAM
entera para `malloc`; la Pico 2 lo comparte todo en 357 KB. `#430` se diagnosticó y se
arregló en la Metro, donde el consumidor nuevo no cuesta nada, y viajó a la Pico 2 en la
MISMA imagen. Es la contrapartida de la imagen única: **la placa más estricta es la que
menos se prueba**.

Lo que falló no fue el criterio, fue que **nadie llevaba la cuenta de qué celdas de la
matriz (sample × placa) estaban sin correr**. Una celda vacía y una celda verde se ven
igual cuando no hay tabla.

### Lo que YA está construido — y es más de la mitad

Antes de diseñar nada, el censo, porque la pieza cara ya existe:

| pieza | dónde | qué hace ya |
|---|---|---|
| **protocolo wire v1** | `docs/BPVM_WIRE_PROTOCOL.md` | `PUT`/`RUN`/`OUTPUT`/`EXITED` — cargar, ejecutar y recoger, ya está especificado y en las 5 familias |
| **cliente wire en Python** | `bpgenvm-c/tools/sim_smoke.py` | clase `Wire` con `call`, `call_bulk`, `get`, y **`drain_run()` que junta los OUTPUT hasta el EXITED**. Es literalmente «cargar un programa, atrapar los resultados» |
| **el simulador** | `tools/bpvm_sim.c` (H10) | habla wire v1 **completo** en el PC: un destino más de la matriz, gratis y sin cable |
| **arnés de paridad** | `compat/compat.sh` | ya hace Java-vs-C sobre 38 samples en el host |
| **la referencia** | miVM | el INVARIANTE SAGRADO da el *golden* sin inventarlo: el `stdout` de la VM-Java |

⏩ **Lo que falta es sorprendentemente poco**: `Wire.__init__` recibe un **socket**. Una
placa habla por **serie**. Abstraer ese transporte (socket ↔ `pyserial`) es el 80 % del
camino, porque `drain_run()` ya hace el resto.

### La forma propuesta

**1. El golden sale de la VM-Java, no se escribe a mano.** Es el invariante del proyecto
aplicado al device: para el mismo `.mod`, el `stdout` de la placa debe ser byte-idéntico
al de miVM. Eso convierte los **48 samples que ya existen** en una batería de placa **sin
tocar ni uno**. Es la diferencia entre «escribir tests» (caro, y nunca se hace) y
«cosechar los que ya hay».

**2. Un manifiesto con lo que cada sample NECESITA**, que es lo que habría cazado este
bug:

```
synclisttest:   corre_en: todas        memoria: APRETADA
GuiColorDemo:   corre_en: p4, disco    necesita: pantalla
SdPaso:         corre_en: metro, p4    necesita: sd
JsonDemo:       corre_en: todas
```

Un sample marcado `memoria: APRETADA` **no puede darse por verde hasta haber corrido en
la placa más pequeña**. La regla que faltaba, escrita donde la máquina la puede exigir.

**3. El informe es una MATRIZ, y las celdas sin correr se ven.** No un «38 PASS»: una
tabla sample × placa donde `—` (sin correr) es visualmente distinto de ✅. El fallo de V5
fue exactamente una celda que nadie miró. Un total agregado la habría escondido otra vez.

**4. El flasheo sigue siendo manual** (es de Eduardo). El arnés asume placa flasheada en
un puerto. Pero **debe verificar contra qué imagen habla** —por `INFO`, antes de empezar—
porque «correr la batería contra un binario de hace tres semanas» ya ha pasado
([[el-sello-de-build-no-identifica-la-imagen]]).

### Lo que queda por decidir

- **¿Tests que se auto-verifican, o comparación con golden?** Hay samples que ya imprimen
  su propio veredicto (`=== ThreadFieldTest: 3 de 3 ===`) — es un test de estilo Java sin
  formalizar. El golden cubre los 48 hoy y sin trabajo; lo auto-verificado es mejor para
  los nuevos. Probablemente los dos, pero **el golden primero**: da cobertura inmediata.
- **Qué hacer con lo que legítimamente difiere por placa** (GPIO, temperatura, serial).
  Un sample así o se marca o normaliza esas líneas. Conviene que sean pocos y explícitos.
- **El tiempo de la vuelta completa.** 48 samples × 5 placas por serie no es gratis. Se
  puede paralelizar por placa (puertos distintos, procesos distintos) y correr el
  simulador siempre. Medir antes de optimizar.
- **Dónde vive.** `compat/` ya es el sitio de los arneses; esto es su hermano de device.

📌 **Y el criterio que ordena todo esto**: la herramienta no tiene que *diagnosticar*.
Eduardo lo dijo explícito — *«luego, investigar si un test no pasa ya es cosa nuestra»*.
Su único trabajo es **decir qué celda está roja y cuál está sin correr**. Todo lo que se
le añada por encima de eso es la trampa de siempre: construir el instrumento en vez de
usar los que ya hay.

### El mecanismo, dicho por Eduardo (29-ago)

> *«Si un test no pasa, aparte de registrarlo, lo ideal sería registrar la salida de la
> consola y la captura del log. Así que el mecanismo sería: pasar una lista de test, para
> cada entrada en la lista secuencia: borrar log, borrar consola, compilar, cargar,
> ejecutar; si OK borrar programa del micro, si no OK, guardar consola y log, y borrar
> programa.»*

Traducido a verbos, **y todos existen ya** — ninguno hay que inventarlo:

| paso | verbo | notas |
|---|---|---|
| borrar log | `LOG_CLEAR` | |
| borrar consola | *(del lado del PC)* | el buffer del runner, no de la placa |
| compilar | `basicplus-frontend.jar` | en el PC |
| cargar | `PUT` / `PUT_BEGIN`+`DATA`+`END` | el streaming, por encima de 40 K |
| ejecutar | `RUN` → `OUTPUT`* → `EXITED` | `drain_run()` ya junta esto |
| si OK | `DEL` | |
| si NO OK | `LOG_DUMP` + volcar la consola a fichero, luego `DEL` | |

🔗 **Y esto sólo es fiable gracias a `U3`.** Al unificar 26 de los 29 verbos en
`src/bpvm_repl.c` se encontraron **cinco divergencias reales** entre familias —y la que
estaba «bien» no siempre era la misma placa. Un arnés escrito antes de `U3` habría hablado
un dialecto distinto con cada micro y sus rojos no serían comparables. La fontanería de
estos días es lo que hace posible la herramienta.

#### Los dos detalles del mecanismo que no son obvios, y los dos salen de hoy

**Borrar el log ANTES, no leerlo después.** El log post-mortem es un **anillo**: el 29-ago
llegó con la vuelta dada (`se tiraron lineas ANTIGUAS`) y se perdió el principio de la
ejecución justo cuando hacía falta. Limpiándolo antes de cada test, lo que se captura es
**la historia completa de ese test y de nada más**.

**Borrar el programa SIEMPRE, pase o falle.** No es sólo higiene del FS: hoy se vio
`/app/SyncListTest.mod ya en FS (contenido idéntico), salto PUT`. Ese atajo es correcto
para el IDE y **veneno para un arnés** — bastaría con que un `.mod` no se regenerara para
estar ejecutando el de la vuelta anterior y no enterarse. Borrando siempre, lo que corre
es siempre lo que se acaba de compilar.

#### El caso que el mecanismo todavía no cubre: que se cuelgue

La secuencia asume que `RUN` termina. Si el micro se queda colgado no hay `EXITED`, y
tampoco se puede `DEL` — es exactamente lo que Eduardo describió el 28-ago:
*«cuando se cuelga desde el IDE no tengo ningún control, y si reseteo no hay nada
registrado en el log»*.

Hoy eso es mucho menos probable (`#447` hace hablar a los fallos, `#448` quitó el `panic`
del `malloc`), pero **el arnés no puede depender de que no pase**. Hace falta:

- **timeout por test**, y que agotarlo sea un resultado con nombre propio —`CUELGUE`—
  distinto de `FALLA`, porque no significan lo mismo;
- **recuperación**: intentar `RESET` por el wire y, si tampoco responde, marcar la placa
  como *necesita intervención* y **parar esa columna** en vez de dar por rojos los 40
  samples que quedan;
- y el log **se pide después del reset**, que para eso es post-mortem y vive en flash.

Un cuelgue que deja la matriz entera roja no informa de nada; un cuelgue que dice
*«aquí se colgó, y de aquí en adelante no se ha probado»* informa de todo.

#### El ORDEN de la matriz, y por qué no es cosmético (Eduardo, 29-ago)

> *«En la ESP32 se puede resetear a través de la conexión. En la STM32 no estoy seguro,
> pero en la Pico no se puede. Quizás para las pruebas es mejor por las que más se pueden
> automatizar. Si el primer equipo no pasa, puede ser del equipo, la VM o cualquier cosa.
> Pero si el primer equipo pasa y otro no, probablemente sea algo del micro.»*

Esto es **la cascada de verificación aplicada a las placas**, y tiene el mismo valor: cada
columna que pasa ESTRECHA lo que puede estar mal en la siguiente. La primera columna verde
no es un test más — es el **control** que convierte los rojos de las demás en información.

**Por qué unas se automatizan y otras no**, que es una diferencia de *hardware*, no de
firmware:

| placa | reset sin manos | por qué |
|---|---|---|
| host + `bpvm-sim` | ✅ total | es un proceso: se mata y se relanza |
| ESP32-S3 / P4 | ✅ | el chip USB-serie lleva el circuito DTR/RTS que resetea el micro |
| ESP32-C3 / C5 / C6 | ✅ (2-oct, la C6) | el USB-Serial-JTAG: `wire_serie.py resetcable` (RTS 200 ms) la reinicia con un programa colgado |
| STM32 | ✅ **por la sonda** (2-oct, la DK2) | el RTS del puerto del ST-LINK NO la resetea (sigue colgada); `STM32_Programmer_CLI -c port=SWD sn=… -hardRst` sí, y la placa dice «pin (NRST)» |
| RP2350 | ❌ | USB **nativo**: no hay chip serie que lleve ese circuito. Sólo se recupera desenchufando ([[pico-sin-reset-instrumentar]]) |

⚠️ **Matiz importante**: el verbo `RESET` del wire existe en todas, pero **sólo sirve si el
firmware está vivo para recibirlo**. Lo que distingue a las placas de arriba es el reset
por hardware, que es justo el que hace falta cuando hay un cuelgue — o sea, el único caso
en el que se necesita.

⏩ **El orden propuesto**, de menos a más coste humano: `sim` → `host` → `ESP32` → `STM32`
→ `Pico 2`. Las tres primeras corren desatendidas; la última exige a alguien delante *sólo
si se cuelga*.

🔴 **Y AQUÍ HAY UNA TENSIÓN REAL QUE NO SE PUEDE ESCONDER.** La Pico 2 es la última en
automatización y **la primera en severidad**: es la placa más estricta de memoria y donde
aparecen los bugs que las demás no ven. Dejarla al final del orden es *exactamente* el
fallo de V5 otra vez, ahora institucionalizado en una herramienta.

La salida no es cambiar el orden —el de Eduardo es correcto por coste—, sino que **el orden
de EJECUCIÓN y el criterio de VERDE sean cosas distintas**: un sample marcado
`memoria: APRETADA` corre el último por comodidad, pero **la matriz no se declara verde sin
esa celda**. El orden lo decide la automatización; el veredicto, la placa que más aprieta.

#### Un afinado a la regla, que hoy costó dos días

*«Si el primero pasa y otro no, probablemente sea algo del micro»* es cierto, pero conviene
leer «del micro» en sentido ancho. Hoy `JsonDemo` iba en la Metro y fallaba en la Pico 2 —
**misma familia, misma imagen, mismo binario**. No era el port ni el silicio: era la
*realidad de memoria* (PSRAM contra SRAM compartida).

Así que la regla útil es: **una diferencia entre columnas apunta a lo que DIFIERE entre
esas dos columnas** — y en la lista de lo que difiere hay que contar la memoria, no sólo el
código específico de la familia. Cuando la diferencia es la memoria, dos placas de la misma
familia pueden discrepar; y el verde de la generosa no exonera a la estricta.

#### Y en V6 entran dos columnas más: C3 y C6 (Eduardo, 29-ago)

> *«En V6 incorporamos 2 placas nuevas, ESP32-C3 y ESP32-C6, son RISC-V pero tienen poca
> memoria. Yo creo que se podría empezar con la ESP32-C6, es similar a la Pico.»*

Y empezar por el **C6** resuelve justo la tensión de arriba, porque junta las dos
propiedades que en las placas de hoy están separadas:

- **es estricta de memoria** (512 KB, sin PSRAM — el rango de la Pico 2), o sea que ve los
  bugs que la Metro esconde;
- **es un ESP32**, o sea que se resetea sin manos.

Es decir: **la primera placa que puede ser a la vez el control automatizado y el juez de
memoria.** Hoy hay que elegir — la que se automatiza (ESP32-S3) es generosa y la que
aprieta (Pico 2) hay que desenchufarla. Si el C6 cumple las dos, pasa a ser la columna
natural para gatear los samples de `memoria: APRETADA` sin coste humano.

⚠️ **Dos cosas por comprobar antes de darlo por bueno**, ninguna difícil:
1. **Que el reset por hardware funcione de verdad** en el devkit concreto. El C6 lleva
   USB-Serial-JTAG nativo; `esptool` sabe resetear por ahí, pero es distinto del circuito
   DTR/RTS de un puente USB-UART y **hay que verlo, no suponerlo**.
2. **Cuánta memoria le queda de verdad a la VM.** Ver la corrección de la ficha P1
   (29-ago): con el reparto de hoy, una VM de 160 KB da un tope de **1536 handles**, y hay
   samples de la batería que piden 3848. La columna nacería roja por diseño.

📌 Lo cual refuerza el orden ya escrito en P1 —las placas nuevas van **después** de la
unificación— y le añade a `U6` un motivo que antes no tenía: no es sólo que unificar
abarate la familia nueva; es que **sin unificar, la familia nueva no pasa la batería**.

#### 🔻 El golden NO cubre todos los samples — y hay prueba (29-ago)

Al cerrar #451 se comprobó `synclisttest` contra la VM-Java, y **no coinciden**:

```
VM-Java   consumidos c1 = 1707   c2 = 793
VM-C      consumidos c1 = 1254   c2 = 1246
```

No es un bug: el reparto entre los dos consumidores **depende del scheduling**, y el
propio sample lo dice en su comentario — *«el conteo de pops efectivos en cada hilo no es
predecible; pero sumando los dos deberíamos haber consumido EXACTAMENTE lo producido»*.
El **total** sí coincide (2500) y todas sus líneas `OK:` también.

Y encaja con algo que ya estaba: el corpus del arnés de paridad son **38 samples elegidos a
mano**, y `synclisttest` nunca estuvo en él. No por olvido — es que no puede estar.

⏩ **Consecuencia para el manifiesto**: hace falta una categoría más, y no es opcional.

| tipo de sample | cómo se juzga |
|---|---|
| determinista | `stdout` byte-idéntico al de la VM-Java (el golden) |
| **no determinista** | **por sus propias asercciones**: ninguna línea `ERROR`, y `exit 0` |

Los samples concurrentes son el caso claro, pero seguramente no el único (cualquiera que
imprima tiempos, direcciones o temperatura cae aquí). Marcarlos es barato; **no marcarlos
sale caro de dos maneras**: o el arnés da rojos falsos y se deja de mirar, o alguien
«arregla» el golden fijando un entrelazado concreto y a partir de ahí el test sólo prueba
que el scheduler no ha cambiado.

⚠️ Y un aviso sobre instrumentar para diagnosticar: `samples/SyncListDiag.bp` —la copia con
prints que localizó este bug— es **todavía menos comparable** que el original, porque sus
prints van DENTRO de los hilos y exponen el entrelazado entero. Es una herramienta de
diagnóstico, no un test: no debe entrar en la batería.

---

## H2 — lo que un `.mod` exporta y cómo se resuelven las dependencias (14-sep, día de análisis)

> Fichas: `#500` (la norma de versiones), `#493` (la sombra de `/app` y del pack), `#492` (los campos protegidos
> no cruzan módulos), `#441` (el `.mdn` no recuerda su receta). Lo que Eduardo nombró de memoria al abrir V7:
> *«limitaciones de los `.mod` a la hora de guardar dependencias intra-clases cuando estaban en diferentes módulos»*.
> Este apartado es el diseño; se escribió **antes** de tocar código, sobre un mapa del mecanismo verificado línea
> a línea (seis lectores + un verificador, 14-sep) y sobre una matriz medida (24 casos, `notas/v7-apertura/matriz492.py`).

### 1. Lo que hay hoy, medido — y lo que no hay

**El `.mod` no sabe nada de versiones.** La cabecera `MOD7` son nueve enteros (magic + ocho tamaños;
`ModWriter.java:2173-2189`, `loader.c:119-141`); cada import es `(nombre cualificado, fromPath)` y nada más
(`ModWriter.java:1983-1987`). Ni versión del módulo, ni de sus dependencias, ni CRC, ni compilador. La única
«versión» que existe en todo el ecosistema es **el dígito del `MAGIC`** (`bpvm_mods.c:12-15`, el `STAT` del wire,
`PicoExplorer.java:1120-1131`): dos `MOD7` distintos son «misma versión, CRC distinto», y el `/lib` de la placa y
el IDE **reemplazan** en ese caso. `ModFormat.FORMAT_VERSION = 7` es «informativa» y no se escribe. No hay sintaxis
`version` en el parser: sólo `import vN from pack "XXXX"`, que es la API nativa del `.mdn` (`bpvm_pack_api_verify`),
no el `.mod`.

**El compilador sabe contra qué compiló y no lo escribe.** Resuelve cada import buscando **la stdlib primero**
(`stdlibDir`, `#467`; `Main.java:1520-1531`) y guarda la ruta real de cada `.mod` usado en memoria
(`ctx.modsUsados`) — y de ahí sólo sale al pack (`PackStep.java:139-147`, que **mete en el pack cada `.mod` usado,
stdlib incluida**, y si el `outDir` tiene uno con ese nombre, «el del outDir manda»: así entró `Core` en
`SQLite.pack`, `#496`). El manifest lleva `requires-pack`, que **nadie lee**.

**Las dos VMs resuelven al revés que el compilador: la stdlib la última.** miVM: pack en ejecución → `workdir` →
tal cual → `modulePaths` → carpeta del raíz (`#431`) → `stdlibDir` (`ModuleManager.java:153-198`), y **usa**
`fromPath`. VM-C: pack en ejecución → `bpvm_fs_stat(<dir del entry>/X.mod)` → `bpvm_entry_resolve`: basedir →
tal cual → `/app` → `/lib` → `/sys` → zona XIP (`bpvm.c:490-605`, `:777-806`), y **tira** `fromPath`
(`loader.c:173-183`). Ninguna de las dos avisa por `OUTPUT` de dónde salió una dependencia; la VM-C lo dice por
`bpvm_diag` (`dep 'X' -> ruta`) y miVM no lo dice.

**La sombra de `#493`, con su mecanismo exacto.** `bpvm_pack_mount` instala la zona de packs como **respaldo del
FS** (`bpvm_pack.c:125-131`) y ese respaldo contesta al `stat`/`read` de **cualquier ruta** cuyo nombre de fichero
coincida (`bpvm_pack_key_de_path` se queda con el basename, `:167-181`). Con la app en `/app`, el pack contesta a
`/app/Gui.mod`: `have_fs = 1`, `/lib` no se prueba, el rastro dice `dep 'Gui' -> /app/Gui.mod`, el aviso «del FS
eclipsa al del pack» sale **cuando el que reclamó fue el pack**, y — lo que nadie había visto — **la rama XIP de
`discover_deps` es inalcanzable** (`bpvm.c:588-601` exige `!have_fs`): un módulo servido por un pack se carga por
*stream* a RAM con `en_pack=0` (`loader.c:276`) en vez de ejecutarse desde flash. `test_fsfallback.c:44-49`
consagra la sombra como correcta. Y el orden de arranque difiere por familia: STM32 monta la zona **antes** de
sincronizar `/lib` (`stm32_repl.c:805` vs `:814`), así que `bpvm_mods.c:24` ve «existe» por el respaldo; ESP32 al
revés; la Pico monta la zona en el primer RUN.

**Lo que el enlazador comprueba y lo que no.** Los dos enlazan **por nombre**, con marcadores
`Clase#metodo#slot` exportados como símbolos de datos (`ModWriter.java:409-421`; `link.c:170-181`: *«lib presente
pero no exporta … ¿versión vieja?»*; `ModuleManager.java:800-807`). Es el «modelo JRE»: hacia delante vale si es
*append-only*. Pero el *class-fixup* sólo parchea `parentOff` (`link.c:211-224`): **nadie compara el `numFields` /
`numMethods` del padre horneado en el hijo con el real** — si `Core.List` gana un campo, un `OwnerList` compilado
contra la vieja coloca los suyos encima y se corrompe en silencio. `#500` no tiene hoy ningún dato que comparar y
`#493` es exactamente el tramo del respaldo.

**La interfaz y la matriz (`#492`).** La sección `interface` es texto (`bpi 7`) con `class / layout nF nM bitmaps /
ctor / prop / staticconst / event / method`; no existe `field` (`ModuleInterface.java:1035-1097`), el importador no
crea ningún `VarSymbol` (`Main.java:2130-2330`), y **`private` se pierde en el parser para `var` y `property`**
(`Parser.java:405, :426`; sólo `:419` lo pega a `FuncDef`). La matriz de 24 casos (cada uno con su control en el
mismo módulo) dio: **no cruzan** el campo protegido (`#492`), el campo protegido leído desde una `native` del hijo
(el caso de Eduardo: mismo agujero), y el `raise` de un evento de la base (**el emisor peta**: *«Clase no
declarada: Base»*); **cruza lo que no debería**: un hijo en otro módulo sobreescribe un método `final` (la interfaz
no lleva `final`); **no cruzan por diseño «v1»**: método y property estáticos (la const estática sí). Y cuatro bugs
del compilador que no son de la frontera: `private var` leído por la subclase (es lo del parser), `super(5)` con
constructor sobrecargado (*«se esperaban 0, se pasaron 1»* — `super` sólo ve la primera sobrecarga), `Base.MAX` /
`Base.cuantos()` **dentro de una derivada** (*«el tipo 'void' no tiene miembros»*), y `return Nodo()` dentro de un
método de `Nodo` (`#507`: excepción del emisor).

**El coste de la alternativa a B, medido** (host, 2 000 000 de `this.n := this.n + 1`): campo 86 ms, property
193 ms → **×2,25**; y **+91 B** en el `.mod` por cada campo convertido (8 de data, 32 de código, 34 de exports, 17
de interfaz) ≈ 74 B de RAM. Exportar el campo en la interfaz: ≈25 B de flash en una sección que la VM salta, 0 de
RAM, 0 de tiempo. **Decidido con Eduardo (14-sep): B.**

**Y el `.mdn` (`#441`)**: cabecera de 20 B (`magic, version=1, abi=6, code_size, sym_count, arch`;
`mdn_format.h:93-101`); el cargador compara `version==1`, `abi==6` y `arch`; el IDE da un `.mdn` por rancio por
`mtime` o `arch`. Los flags viven en `AotBuild.java:53-54,108-109` y nadie calcula huella. El bloque nativo
embebido en el `MOD7` pasa por el mismo gate. De paso: `Disasm.java:56-66` sólo entiende `MOD6` (la herramienta
«module info» de `#284` desplaza las secciones con un `MOD7`).

### 2. El diseño — cuatro piezas, una sola subida de formato

**Pieza 1 · `#500`: el `.mod` lleva contra qué se compiló, y el cargador lo exige.** ✅ **Hecha el 15-sep**
(`5fc59ae4`; la especificación viva está en `MOD_FORMAT.md` §3/§3.1). Lo que cambió al hacerla: el sello va **por
módulo** en una subtabla `deps` (no por import); una dependencia MOD7 cuenta como gen 0 (si se exige más, «más
antigua»); y **el marcador de layout se cae**: lo sustituye la norma de Eduardo para `extends` (*«los hijos pueden
ser más recientes que los padres; los padres no pueden ser más recientes que los hijos»*) con una marca `EXTIENDE` en
el sello. El texto de abajo es el diseño de la mañana del 14-sep, que se conserva como registro.

Dos números por dependencia, los dos **automáticos** (sin sintaxis nueva):

- **`ifaceCrc`** — el CRC32 de la sección `interface` de la dependencia contra la que el compilador compiló. Es la
  *identidad exacta de la API*: dice «¿es la misma interfaz que vi?».
- **`gen`** — la *generación* de la dependencia: un entero monótono que el compilador estampa en cada `.mod` a
  partir de la **versión declarada del proyecto** (`"version"` en el `.bpbuild`; la stdlib de V7 sería `7`, y un
  proyecto sin versión, `0`). Es lo que da **orden** — sin él no existe «más antigua».

El módulo lleva en su cabecera su propio `gen` y su `ifaceCrc`; cada import lleva `(nombre, fromPath, gen,
ifaceCrc)` de la dependencia con la que se compiló. **La norma, al cargar, en las dos VMs:**

1. `dep.gen < necesitada.gen` → **rechazo**: *«'Core' (gen 6) es más antiguo que el que 'App' necesita (gen 7):
   actualiza /lib o recompila»*. Es la regla de Eduardo (12-sep) tal cual, venga la dependencia del pack, de
   `/app` o de `/lib`.
2. `dep.gen == necesitada.gen` y `dep.ifaceCrc != necesitada.ifaceCrc` → **rechazo**: *«'Core' (gen 7) no es la
   interfaz con la que se compiló 'App': recompila»*. Es el desfase de siempre (una stdlib reconstruida sin subir
   la versión), que hoy pasa en silencio si las ranuras coinciden.
3. `dep.gen > necesitada.gen` → **se acepta** si enlaza: los marcadores `Clase#metodo#slot` de `#324` **más un
   marcador de layout nuevo** por cada clase base extendida (`Clase#layout#nF#nM`, el mismo mecanismo, un símbolo
   más) — la promesa *append-only* del «modelo JRE» pasa de convención a comprobación. Si el padre creció por el
   medio, el marcador de layout no resuelve y grita en vez de corromper.
4. El rechazo sale por **`OUTPUT`** (donde mira el usuario), no sólo por el log de la placa; mismo texto en las
   dos VMs (es lo agnóstico de rol).

Formato: es cambio de cabecera y de la sección `imports` → **`MOD8`**. ✅ **Decidido por Eduardo (14-sep)**: *«el
compilador compila una versión concreta, no compila varias. Ahora mantenemos V7 o pasamos a V8: claramente V8, es un
cambio importante. Compatibilidad hacia atrás, sí: es un cambio aditivo, por lo que los V7 tendrían que poderse ejecutar
sin problemas.»* Es decir — y esto es DISTINTO de la subida `MOD6`→`MOD7` de V6, que fue por ABI y por eso «formato igual
o rechazo» (`#284`):

- **El compilador emite sólo `MOD8`.** No hay modo de compatibilidad ni doble emisión.
- **Las dos VMs cargan `MOD7` y `MOD8`.** Un `MOD7` es un `MOD8` sin los campos nuevos: sin `gen` ni `ifaceCrc` la norma
  de versiones no tiene qué comparar y **no se aplica** (se anota en el log: «sin datos de versión»), y el módulo corre
  exactamente como hoy — byte-idéntico. Lo aditivo es eso: todo lo que existe sigue valiendo; lo nuevo sólo actúa
  cuando está.
- `MOD8` en las VMs de V6 (el ZIP publicado) se rechaza con mensaje (magic desconocido), como debe: hacia delante no hay
  compatibilidad, hacia atrás sí.
- **La stdlib se rehace UNA vez, al terminar todos los cambios de formato** (*«mientras tanto se recompila lo que sea
  necesario»*): durante el trabajo, la stdlib `MOD7` de hoy sigue cargando en las VMs `MOD8` — que es justo la prueba de
  la compatibilidad hacia atrás, de gratis y todos los días.

Es UNA sola subida para todo H2: en el mismo salto entran el `field`/`final`/estáticos/`ctor`×N de la interfaz
(`bpi 8`) y la huella de flags del `.mdn` (`#441`, pieza 4; el `.mdn` pasa de versión 1 a 2 y el cargador acepta las
dos). ⚠️ El `bpi` de la interfaz y el `MOD` del contenedor son versiones distintas; suben a la vez, y `Disasm` aprende
`MOD7`/`MOD8` de paso.

**Pieza 2 · `#493`: la zona de packs deja de disfrazarse de `/app`.** *(15-sep, tras el mapa verificado — ver la ficha
en FICHAS: el «espacio propio» es `pack:<Pack>/X` (no `/packs/`), el «stat sin directorio» se tacha, el peldaño
«embebido» no existe, «honrar `fromPath`» se tacha, el criterio del aviso es «gemelo tapado», y los recursos siguen
saliendo de la zona como último candidato de `bpvm_fs_resolve`. El texto de abajo es el diseño del 14-sep.)*

1. **El respaldo contesta sólo en su propio espacio.** La zona de packs deja de reclamar rutas ajenas por basename:
   contesta a `/packs/<nombre>/X.mod` (o al `stat` sin directorio que haga `discover_deps` a propósito), nunca a
   `/app/X.mod` ni a `/lib/X.mod`. Con eso `bpvm_fs_stat("/app/Gui.mod")` dice la verdad, `/lib` vuelve a mirarse,
   el aviso «eclipsa» vuelve a su sentido — y **la rama XIP de `discover_deps` vuelve a ser alcanzable**: un módulo
   de pack se ejecuta desde flash, que es para lo que están los packs (y lo que `#468` quiere para la C6).
2. **Un solo orden, escrito, igual en las dos VMs**: pack en ejecución → `/app` (la carpeta del programa) → `/lib` →
   embebido → zona de packs. Lo que hoy difiere (miVM honra `fromPath` y la VM-C lo tira): se decide y se iguala
   (propuesta: honrarlo en las dos como *pista* de primer candidato, que es lo que el compilador quiso decir).
3. **Ruido donde mira el usuario**: cuando una dependencia **con nombre de stdlib** (existe en la tabla embebida /
   en `stdlibDir`) se resuelve fuera de `/lib`, una línea por `OUTPUT`: *«aviso: 'Str' cargado desde /app/Str.mod
   (la stdlib está en /lib)»*. En las dos VMs (miVM hoy no rastrea nada). Es la mitad «ruido, no lógica» de la
   ficha; con la pieza 1 detrás, la sombra que además sea vieja se **rechaza**.
4. **El pack builder no empaqueta la stdlib** por defecto (`PackStep` distingue lo que vino de `stdlibDir`), con una
   opción explícita para packs autocontenidos; `requires-pack` o se lee o se quita. Y el arranque sincroniza `/lib`
   **antes** de montar la zona en las tres familias (hoy STM32 al revés).
5. Tests: `test_fsfallback.c` pasa a afirmar lo contrario (la zona NO contesta a `/app`); un test de host de la
   precedencia con `/lib` real; y `tanda.py` ya comprueba la sombra antes de correr (`T1`, `843e2d0a`).

**Pieza 3 · `#492`: la interfaz exporta la superficie completa que un descendiente puede tocar (B).** ✅ **Hecha el
14-sep** salvo el marcador de layout (que va con la pieza 1). Lo que cambió respecto a lo escrito aquí por la mañana
está marcado; el resto salió como estaba.

0. ✅ `4a6ea0f4` — el parser **conserva `private` en `var` y `property`** y `checkVisibility` lo aplica.
1. ✅ `160e849b` — `field <nombre>:<tipo> slot <n> [owner]` por cada campo **no privado** (Eduardo: los privados no
   viajan; nadie de fuera los nombra y el layout ya va en los bitmaps); el importador los da de alta con su ranura y
   el emisor accede por ranura cuando la clase es externa. El AOT sigue sin leer campos (límite documentado, no
   silencioso: `UnsupportedAotException`).
2. ✅ `f4a3ca7e` — `final` en `method`/`prop`, `private` en `prop`; el importador los marca.
3. ✅ `024f5b24` — Estáticos. **Distinto de lo escrito aquí**: NO «función con nombre cualificado». Los estáticos ya
   se exportaban como funciones de módulo desde siempre (`Cls.m`, con punto) — lo que faltaba era publicarlos, y no
   se podía tal cual porque **los dos cargadores parten un import por el penúltimo punto** (`Mod.Cls.m` → library=Mod,
   módulo=Cls). Lo que se añadió sin tocar lo hecho (Eduardo: *«no hace falta que modifiques algo que ya está
   hecho»*): un **alias de export plano** al mismo offset (`ModWriter.addExportAlias`: `__cls_s_<Cls>_<clave>`,
   `__cls_sg_/__cls_ss_<Cls>_<prop>`; cero bytecode, nada en el `.dbg`) y **directivas nuevas** `staticmethod`/
   `staticprop` — no un flag de `method`: un lector anterior las salta (tolerant) en vez de darles ranura de vtable
   en silencio. El stub va a `staticMembers` sin ranura y sin `isExternal`; el emisor decide por
   `ownerClass.isExternal` como el `super` de `#451`. La stdlib no tiene ningún estático de clase: sus bytes no cambian.
3-bis. ✅ `9381c607` — **Los constructores sobrecargados** (Eduardo, 14-sep: *«había un problema con la sobrecarga de
   constructores; si vamos a modificar el formato de los `.mod`, se puede hacer todo»*). `ctor(...)` = cabeza (como
   hoy, aditivo) **+ `ctor+(...)` por sobrecarga** (directiva nueva por la misma razón; un 2º `ctor(` ya no pisa al
   primero en silencio: error). El importador encadena con el mismo `overloadMangle`; el dueño sintetiza
   `__cls_init_<Cls>$clave` por ctor **propio** y `__cls_new_<Cls>$clave` por ctor **efectivo**; `super(args)` resuelve
   por firma (también en el mismo módulo: `#509`) y el super implícito busca la sin-args en la cadena. **`#513`**, que
   salió del mapa: la derivada sin ctor propio exportaba `__cls_new_Der` con 0 params y sin `__init` — `Mod.Der(7)`
   daba 0 en silencio; la factoría va por la cadena efectiva y llama al `__init` de la clase que **declara** el ctor
   (local o `CALL_EXT`), igual que `super(...)` explícito e implícito.
4. ✅ `f4a3ca7e` — el `raise` con evento heredado de otro módulo va por la ranura que publicó `event … recv n`.
5. Las VMs no cambian en nada de esto: el acceso es por ranura. Lo que las protege de un hijo compilado contra otra
   base es el marcador de layout de la pieza 1. ⏭️ **Pendiente** (va con la subida de formato).

**Cómo se hizo (el método, para repetirlo)**: un mapa de 7 lectores + 7 refutadores ANTES de tocar (cada afirmación
con fichero:línea, refutada o confirmada; sacó `#513`, la trampa del flag `static` y la causa exacta de `#510`), luego
el código por tandas con oráculo de paridad al final de cada una, y una revisión adversarial del diff al terminar.

**Pieza 4 · `#441`: la receta del `.mdn`.** Una huella (CRC32 de los flags de compilación de `AotBuild`, por
destino) en la cabecera del `.mdn` — un campo más en los 20 B — comparada al cargar como ya se compara `arch`, y
publicada por el `INFO` de la placa junto con la ABI de helpers para que el IDE pueda decir *«este `.mdn` no es de
esta placa»* antes de subirlo (hoy lo decide por `mtime`). Misma subida de formato que la pieza 1.

### 3. El orden de trabajo y el cierre pequeño

1. ✅ `private` en el parser + `checkVisibility` (precondición de B; y cierra el bug de la matriz).
2. **La subida de formato, de una vez**: `MOD8` (gen + ifaceCrc en cabecera e imports), ✅ `bpi 8` (`field`, `final`,
   estáticos, `ctor+` — hecho por piezas el 14-sep, el contenedor sigue en MOD7), `.mdn` con huella. Compilador
   (`ModWriter`, `ModuleInterface`, importador, emisor de `raise`), `Disasm`, y los dos cargadores con la norma y
   sus mensajes — paridad: mismo texto de rechazo.
3. El marcador de layout en el enlace (las dos VMs).
4. `#493`: el respaldo en su espacio, el orden único, `fromPath` en las dos, el ruido por `OUTPUT`, el arranque de
   STM32, el builder sin stdlib, los tests.
5. La cadena: jar, stdlib como proyecto, samples, fat-jar del IDE (cerrado), los blobs de las cinco imágenes,
   `packs/Stdlib.pack`; `MOD_FORMAT.md` a v8, manual/referencia (`#492`, `L18`), `PENDIENTES` (L18 fuera).
6. **Cierre pequeño de H2**: todo lo del apartado 4 — los siete tests a medida en verde, las redes de siempre con
   número, y la ronda corta de `tanda.py` en una placa por familia (los blobs cambian en las cinco).

Los tres bugs sueltos del compilador (`super` con constructor sobrecargado, `Base.X` dentro de una derivada,
`#507`) y `#506` son de `H1`: ficha propia, se hacen cuando toque. — ✅ `#509` y `#510` cayeron el 14-sep con B
(eran el mismo código); quedan `#506`, `#507` y `#511` (decisión de Eduardo).

### 4. Cómo se prueba la subida de formato — «con los ejemplos de siempre más tests a medida» (Eduardo, 14-sep)

> *«El cambio de formato de los `.mod` puede ser fuente de problemas de compatibilidad. Hay que probarlo bien, con los
> ejemplos de siempre más tests hechos a medida para verificar estos cambios.»* La subida de `MOD6`→`MOD7` en V6 se
> probó con las redes de siempre y aun así un `Stdlib.pack` MOD6 viajó en el primer ZIP y cinco drivers MOD6 se
> quedaron en `bpdevices/` (los cazó el empaquetador en `F1`, no una prueba). Esta vez, las dos cosas desde el principio.

**Las redes de siempre**, todas, y con número (no «verde»): la suite del frontend (110) y la de miVM (39);
`compat.sh check` (59 PASS, sin `!! stderr:`); `samples_sweep.py` (348 OK); `doc_frags.py`; la guarda del
empaquetador (que ya mira dentro de los packs); y la ronda corta de `tanda.py` en **una placa por familia**
(Pico · S3 · C6 · P4 · DK2), porque los blobs embebidos de las cinco imágenes se regeneran.

**Los tests a medida**, uno por cada cosa que cambia — se escriben ANTES de tocar el formato, con el `.mod` de hoy
como control:

1. **El formato, en las dos direcciones.** Hacia atrás: **`MOD7` en las VMs `MOD8` carga y corre byte-idéntico** —
   el corpus entero de hoy (`samples/*.mod`, la stdlib `MOD7`, los 59 de paridad) sin recompilar, en las dos VMs, con
   la línea «sin datos de versión» en el log y nada en `OUTPUT`. Hacia delante: **`MOD8` en las VMs de V6** (el ZIP
   publicado, el oráculo) tiene que **rechazarlo con mensaje, no petar ni cargarlo mal**. `MOD5` sigue rechazado por
   ABI (`#284`). Hoy no se prueba ninguna de las tres cosas.
   ✅ **Hecho el 14-sep** (`50f73178`): la **cápsula V6** en `compat/compat.sh` — `gen-v6` congela el corpus (59 casos)
   compilado por el **compilador publicado** (sacado de `dist/BasicPlus-6.0-win.zip` en cada ejecución) más la stdlib
   MOD7 del ZIP y las librerías hermanas; `check-mod7` ejecuta los `.mod` congelados con las VMs de hoy byte a byte
   (59/59); `check-fwd` mete un `.mod` de hoy en la miVM publicada de V6 (corre o rechaza con mensaje; con `MOD8`
   exigirá el rechazo). Los dos entran en `check`. `.gitattributes` protege los goldens del CRLF.
2. **La cabecera y las secciones.** Un `.mod` con TODAS las secciones no vacías (imports con `gen`/`ifaceCrc`,
   interface con `field`/`final`/estáticos/`ctor`×N, native con dos bloques, data, code) cargado por las dos VMs y
   por `Disasm`, comprobando que `data` y `code` quedan donde deben (CRC de cada sección contra el escritor) — el
   comentario de `loader.c` lo dice: *«saltarla mal desplazaría `data` y `code`, que es como se corrompe un módulo en
   silencio»*. Hoy ese test no existe y `Disasm` ya se desplaza con `MOD7`.
   ✅ **Hecho el 14-sep** (`050739bf`): `make test-modhdr` (`test/test_modhdr.c`) recorre el fichero por la
   especificación —sin el loader— y comprueba que la suma de secciones es el fichero, que la sección native se
   recorre blob a blob con el relleno ABSOLUTO a 4 (fixtures con 0, 1 y 3 B de relleno, ARM y RISC-V), y que el
   cargador deja en RAM los bytes exactos de `code` y `data` (CRC). Cinco fixtures en `test/fixtures/modhdr/` (con
   `library` no vacía incluida). Y `Disasm` aprende `MOD7` (`nativeSize`, blobs, CRC por sección): sus CRC coinciden
   con los del test de C — dos lectores independientes de acuerdo. El test se extiende a `MOD8` con la subida.
3. **La norma de versiones, los cuatro casos** (`#500`), en host y en las dos VMs con el mismo texto: dependencia con
   `gen` menor → rechazo; misma `gen` y otra interfaz → rechazo; `gen` mayor que enlaza → carga; `gen` mayor que no
   enlaza (una ranura movida) → rechazo por marcador. Con `Core` de verdad, no con un módulo de juguete.
4. **El marcador de layout.** `Base` gana un campo; `Der` compilado contra la vieja → rechazo al enlazar (las dos
   VMs); `Der` recompilado → carga y `tanda` byte a byte. Y el mismo test en el AOT: una `native` del hijo leyendo el
   campo heredado.
5. **La interfaz: la matriz de `#492` entera** (24 casos + los controles negativos: lo privado NO cruza, `final` NO se
   salta, `super(5)` resuelve) como test de la suite del frontend; y una ida y vuelta de la interfaz (escribir →
   leer → escribir, byte-idéntica) para `field`, `final`, estáticos y `ctor` ×N.
   ✅ **Hecho el 14-sep**: la matriz es `HerenciaCrossModuleTest` (50 casos de caracterización, `4a6ea0f4`) y la ida
   y vuelta es `InterfazIdaYVueltaTest` (los 28 módulos de la stdlib, `e86f9f3f`). Suite del frontend: **188/188**.
   Al llegar `bpi 8`, los dos se amplían (los casos de la matriz cambian de valor esperado; la ida y vuelta cubre las
   directivas nuevas).
6. **La sombra y el XIP** (`#493`), en el simulador con `--pack`: `/app` real gana a `/lib`, `/lib` gana a la zona, la
   zona sólo contesta en su espacio; **un módulo servido por un pack se carga XIP** (`en_pack=1`), que hoy es
   inalcanzable; el aviso por `OUTPUT` sale cuando toca y sólo entonces; `test_fsfallback.c` afirmando lo contrario de
   hoy. Y en placa: el P4 con la zona grabada (`SqlDemo` sigue igual al host) y la Pico sin ella.
7. **Lo que no cambia**: la paridad de los 59 casos y las salidas de la tanda **byte-idénticas** a las de V6 en todo
   lo que no sea el mensaje nuevo — el ZIP de V6 como oráculo: lo que allí daba X sigue dando X.

📌 El orden manda: **primero los tests (en rojo o inexistentes hoy), luego el formato.** Un test que se escribe después
prueba lo que se hizo, no lo que se quería.

### 5. Lo que hay que decidir (Eduardo)

1. **`gen` = la versión declarada del proyecto** (`"version"` del `.bpbuild`; la stdlib entera va con una) — ¿o
   una versión por módulo? La primera es la que ya existe para el pack (`pack.version`) y no obliga a tocar cada
   `.bp`.
2. Con `gen` mayor: ¿aceptar si enlaza (marcadores de método y de layout), como propongo, o exigir también la
   misma interfaz? Lo segundo obliga a recompilar la app en cada release de la stdlib aunque no cambie nada que
   use.
3. `fromPath`: honrarlo en las dos VMs, o quitarlo del `.mod`.
4. El pack de librería **sin** la stdlib por defecto (y `requires-pack` que se lea, o fuera).
5. ✅ Una sola subida de formato (`MOD8`/`bpi 8`/`.mdn`) para H2, `#441` y los constructores sobrecargados — Eduardo,
   14-sep: *«si vamos a modificar el formato de los `.mod`, se puede hacer todo»*. Y **cómo**: el compilador emite sólo
   `MOD8`; las VMs cargan `MOD7` y `MOD8` (aditivo: los `MOD7` corren sin problemas); la stdlib se rehace una vez al
   final, y mientras tanto se recompila lo necesario. Lo otro que toca el formato y podría subirse al mismo tren:
   `CALL_REL` (H7: el `CALL` local PC-relativo que retira la base `cb` transitoria del XIP) — es cambio de compilador y
   de las DOS VMs, más grande; se decide aparte.

### 6. La fragilidad de la tabla (15-sep, tarde) — «llevamos cinco versiones dando vueltas a los slots»

Eduardo, al ver el mapa de `#514`/`#516`: *«la tabla ha demostrado ser frágil frente a cambios de versiones. Deberíamos
pensar en un mecanismo más robusto. No sé, igual en vez de una vtable deberíamos tener una por cada herencia»*. Y
después, afinando: *«se juntan varias cosas. Siguiendo nuestra norma, el compilador puede resolver lo que sabe en tiempo
de compilación. El problema aquí es que si se utiliza una clase diferente en runtime de la que utilizó el compilador el
castillo de naipes se cae»*. Esto es lo que se junta, separado, con lo que el mapa midió (5 lectores + 4 refutadores +
crítico, `C:/tmp/v7_514/`, 2,05 M tokens; el detalle en `FICHAS` `#514`).

**La familia, por versiones**: `BUG-4` de V2 (1-jun: los privados desalineaban la vtable; se decidió «vtable = sólo
públicos = exactamente el `.bpi`»), `#324` (los manejadores de evento), `#390` (tres visibilidades: lo protegido
ocupa ranura y hay que publicarlo), `#392` (la hija con sobrecargas), `#393` (el guardián que sólo mira la mitad),
`#451`, `#509`/`#513` (los constructores), `#518` (`library`), `#514`/`#516`. **Nueve fichas, un solo mecanismo.**

**Son DOS fragilidades, y el mapa demuestra que la que nos ha mordido nueve veces es la primera:**

1. **El compilador se equivoca con LA MISMA clase.** En todos los casos medidos de `#514` el `.mod` que corre es el
   que el compilador usó —paridad byte-idéntica, exit 0— y aun así llama al método equivocado. No es una versión
   distinta: es que la vtable la calcula un recorrido (`computeClassLayout`, la función única), la interfaz la escribe
   **otro** (`extractClass` sobre el AST, con sus exclusiones), y el consumidor **reconstruye** las ranuras contando lo
   que le llegó (`sembrarSlotsDeVtable`, posicional puro). Tres cálculos que tienen que coincidir por construcción, y
   cada feature (eventos, `protected`, estáticos, sobrecargas, ctors) añadió una regla a un lado que los otros tenían
   que imitar. Lo que el mapa midió que hoy se pierde: una firma con enum de cabeza (`100` en vez de `4`, silencioso);
   un `private function event` delante (ocupa ranura y **no se publica ni se avisa**); una property de tipo no
   exportable (dos ranuras); una hija importada que sobrecarga un nombre heredado, consumida por un tercero (silencioso
   **sin ninguna omisión**: `Main.java:2395` sólo mira los miembros propios del stub); el override en una hija
   cross-module cae en la ranura del enum omitido (el peor: `this.m(ROJO)` desde la base ejecuta `Hija.m(integer)`);
   las funciones de MÓDULO sobrecargadas con cabeza omitida (`f(4)` = `100`, sin marcador); y un evento opaco al que
   se puede suscribir (`externalOpaque` se escribe y nadie lo lee: el frame se corrompe en las dos VMs). El marcador
   `Cls#clave#slot` comprueba una **coincidencia**: cuando la reconstrucción se equivoca de forma consistente, pasa.

2. **En runtime hay otra clase que la que vio el compilador.** Es la que Eduardo nombra, y **desde el 15-sep tiene
   respuesta completa en `#500`, por rechazo**: `gen` (más antigua → no), `ifaceCrc` (misma `gen` y otra interfaz →
   no), regla 3 (un padre nunca más reciente que sus hijos → no), y para el uso puro de una dependencia más nueva, los
   marcadores (una ranura que se movió no existe con ese nombre → error de enlace nombrándola). El castillo no se cae
   en silencio: **se rechaza al cargar**. Lo que pasa es que **la reja aún no está en vigor**: la stdlib sigue en
   MOD7 (`«sin datos de version»`) hasta que se haga la cadena. Nada de la familia de arriba se habría evitado con
   ella: todos son «misma clase».

**La norma, aplicada** (*«el compilador resuelve lo que sabe»*): lo que el compilador sabe de otro módulo es **una
versión concreta de su interfaz**. Con eso sólo hay dos diseños coherentes: (a) **identidad o rechazo** — los
números compilados valen para esa interfaz y para ninguna otra, y el runtime lo comprueba entero (`#500`); (b)
**enlace tardío** — el `.mod` lleva nombres y el cargador construye las tablas contra la clase real (el modelo de
Java). Lo frágil es el punto medio en el que llevamos cinco versiones: números + una comprobación parcial. Hoy ya
estamos en (a) por diseño; falta que la comprobación sea **completa** y que el compilador no mienta.

**Los tres caminos, con el censo delante:**

| | C · la interfaz es la vtable | B · enlace tardío (Java) | T · una tabla por nivel (Eduardo) |
|---|---|---|---|
| qué cierra | la fragilidad 1 entera (las nueve de la familia, y las siete medidas del 15-sep) | la 1 y la 2: `Base` puede añadir métodos/campos sin recompilar hijos | la parte de MÉTODOS de la 2; los campos y el tamaño del objeto siguen horneados |
| cómo | `method … key k slot n`; lo que no se puede describir viaja `opaque` con clave y ranura (como `event … recv N opaque` y `field … slot n`); la interfaz se escribe **desde `computeClassLayout`**, no desde un segundo recorrido; el consumidor **copia**, no cuenta; `cruzarSlots` comprueba «sin huecos hasta `numMethods`»; opaco = no invocable ni sobreescribible (con guardián) | el `.mod` lleva `(Clase, clave)` por llamada cross-module (los marcadores ya son ese mapa); el cargador copia la vtable del padre real, sustituye overrides por clave, añade lo propio, rebasa los campos del hijo y el `numFields`; las llamadas se resuelven a su ranura al enlazar (tabla de referencias en RAM: en XIP no se parchea código) | cada clase lleva una tabla por ancestro; la llamada codifica (clase declarante, índice); las tablas del padre se construyen igualmente al cargar (el hijo no sabe qué añadirá el padre) — es B con una indirección más por llamada y sin tabla de referencias |
| VMs | **no cambian** (una ranura es un número en los dos lados) | cargador + enlazador de las DOS VMs, `INVOKE_VIRTUAL`/`GET_FIELD` por referencia (o parcheo en RAM), MOD9, el AOT (`slotOf` horneado en el `.mdn`) | como B, más el despacho nuevo del intérprete |
| coste por llamada | igual | +1 acceso indexado (la tabla de referencias) | +1 indirección |
| interfaz | claves y ranuras | sólo claves | sólo claves |
| `#500` | se queda entera | regla 3 sobra; `ifaceCrc` deja de ser fatal | regla 3 sobra para métodos, no para campos |
| tamaño | un día de compilador + stdlib regenerada + tests | días: cascada Java → C → Pico entera | como B |

**Lo que cambia mi recomendación de la tarde** (dije «ir a enlace tardío directamente»): con el mapa delante, la
robustez que pide Eduardo **no la da el enlace tardío, la da que la reja sea completa** — y `#500` ya lo es, por
rechazo, en cuanto la cadena la ponga en vigor. El enlace tardío compra otra cosa: **compatibilidad binaria** (actualizar
la stdlib sin recompilar las apps). Que haga falta depende del producto, no de la robustez: el usuario compila en el
IDE contra la stdlib que viene con él, la imagen lleva el `Core` de esa misma versión, y un pack es autocontenido
(`#493`) — la forma del producto **ya es (a)**. Y T es B con un coste más.

⏭️ **Propuesta**: **C ahora** (es el arreglo de la causa, no del caso; `bpi 8` sigue abierto y la stdlib se rehace en
la cadena de todas formas) + **la cadena** (que pone `#500` en vigor) = el bloque cerrado bajo la norma. **B queda
escrita aquí** como el camino si algún día se quiere compatibilidad binaria; no es un pendiente. Decisión de Eduardo.

### 7. El diseño: ENLACE TARDÍO (MOD9) — `#523` (15-sep, noche; mapa de 6 lectores + 3 diseños + 2 jueces + síntesis + crítico adversarial, 2,6 M tokens)

> **Decidido por Eduardo** (ver `#523`): compatibilidad hacia atrás como Java. **La norma que lo preside**: *«lo que puede resolver el compilador es
> estático y lo que puede resolver el cargador es dinámico. Como en tiempo de compilación no podemos saber qué módulos realmente nos vamos a
> encontrar, es un problema que el compilador no puede resolver. Y por la misma razón el cargador sí lo puede resolver.»* Condición: **«validarlo al
> 100x100»** — el plan está escrito aquí, antes que el código. Las correcciones del crítico adversarial (A1-A11) están **incorporadas** en el texto
> (marcadas «crítico, An»); lo que sigue abierto son las decisiones D1-D10 al final. Tamaño honesto: **17-23 sesiones** en cascada.

**Base:** el diseño «robustez» (mejor puntuado por los dos jueces), con los injertos señalados (refs dentro de secciones existentes, +6 sellado en todos, helpers de paseo centralizados, R5 verificada al cargar, cronómetro con línea base, umbrales numéricos, guardián `--barajar`) y corregidos sus errores (Collections NO es cerrada; la tabla de una clase cerrada lleva SÓLO lo propio; el canal de los rechazos es `load_error`/`link_error`, no stdout; regla explícita para Object y las clases sintetizadas; verificación de bitmaps acotada a lo comprobable).

**Confirmado hoy (15-sep) para este diseño:** `bpstdlib/Collections.bp:93` `SyncList extends Core.List` y `:169` `OwnerList extends Core.List` (dos clases abiertas YA en la stdlib; `Gui.bp:417` la consume: `var owner children: Collections.OwnerList`). `bpvm_opcodes.h:262` `OP_CHECKCAST_EXT = 0xB0`: 0xB1+ libres. `loader.c:241-247`: `ext_table_size = ext_count*4` se fija ANTES de data/code (las refs dentro de imports caben en ese mecanismo). `link.c:76-83` `bpvm_get_ext_table_addr` es un barrido lineal de módulos por cada CALL_EXT (`interp.c:1942`). `bpvm.c:1293-1300` `bpvm_arena_reserve` reserva en `next_free_address`, que `loader.c:448` fija a `end_addr+64` al FINAL de la carga. `bpvm.c:1008-1011` `load_err` → `vm->load_error`; `link.c:154-188` `bpvm_link_set_error` → `vm->link_error` (ambos al wire). `bpvm.c:1352-1362` hook `g_after_link`. `bpvm_mods.c:37-44` sincroniza /lib sólo por MAGIC. `bpvm_internal.h:192,202` `iface_crc` y `dep_crc` ya viven en `bpvm_module_t`.

#### 1. Objetivo y criterio de aceptación

**Objetivo.** Que un `.mod` no lleve ningún número que dependa de otro módulo: donde hoy hay una ranura de vtable, una ranura de campo, un recv de evento o un descriptor ajeno horneado, el `.mod` lleva una **referencia simbólica** (clase, miembro, firma completa con retorno) y el cargador la resuelve **una vez**, a una tabla en RAM, con TODOS los módulos cargados. Las clases con un ancestro de otro módulo no llevan descriptor: el cargador lo **construye** (copia la forma del padre REAL, sustituye overrides por firma, añade lo propio, rebasa campos y tamaño).

**Criterio de aceptación = el de Java.** (a) Donde Java correría — miembro añadido en cualquier posición, reordenado, movido a un ancestro, sobrecarga nueva delante de la cabeza, también para subclases de otra versión — corremos con stdout correcto e idéntico en las dos VMs. (b) Donde Java lanzaría — miembro quitado, firma o retorno cambiados, tipo de campo/property cambiado, clase quitada, override huérfano — **rechazo AL ENLAZAR** nombrando `Mod.Cls.clave` y el módulo que lo pide, con texto byte-idéntico en miVM y VM-C. (c) **Cero silenciosos**: no queda ningún número en el consumidor que dependa del dueño — el guardián `--barajar` (§11 de la validación) lo caza para las ranuras; los **inlineados** (valores de enum, `staticconst`, valores por defecto de parámetros, la máscara de anchos del `raise`) son semántica `static final` de Java y se documentan como divergencia (opcional y barato: un kind CONST en 3.3 verificado al enlazar → rechazo en vez de silencio). (d) Lo que no cruza módulo sigue resuelto al compilar (regla R2, no heurística). (e) XIP: nada del `code` se parchea.

#### 2. Reglas

- **R1 cerrada/abierta.** Una clase es ABIERTA si algún eslabón de su cadena de `extends` está en otro módulo; transitiva hacia abajo. Object/Thread/List/StringBuilder/Mutex se sintetizan por módulo (`MivmEmitter.java:4488-4492, 5032`): `extends Thread` es cerrada. Corpus: 219 clases, 11 abiertas — **dos en la stdlib** (Collections.SyncList/OwnerList).
- **R2 local/por referencia.** Un sitio va POR REFERENCIA si toca una clase de otro módulo o una clase abierta (también desde su propio módulo, también sus miembros propios: sus ranuras dependen del padre y en XIP no se renumera). Todo lo demás se hornea como hoy (INVOKE_VIRTUAL/GET_SET_FIELD u8, NEW_OBJECT/INSTANCEOF i16, PUSH del handler, `$recv/$dest`). El `code` de Core/Gui/Json no cambia un byte de operandos. **Y toda conversión implícita a un supertipo de otro módulo o abierto** (asignación, paso de parámetro, `return`, `raise`) genera una ref **SUBTIPO** `(Clase, 'Mod.Super')` (dedupe: un puñado por módulo) que el cargador verifica subiendo la cadena efectiva — sin ella, «`Derived` deja de extender `Base`» daría basura (`vt[s]` de otra clase) donde Java lanza `VerifyError` (crítico, A1).
- **R3 clave canónica**, calculada en UN sitio (`Symbol.claveCanonica`, sustituye a `overloadMangle/typeCode`, `Symbol.java:329-369`): método/accesor/función/ctor `nombre(códigos)ret` (`ret` = `V` si no devuelve; ctor sin ret); campo/property `nombre:código`; evento `nombre(códigos)`. Códigos = los de `typeCode` con tres cambios: clase declarada → `L<Mod.Cls>;` cualificada; clase sintetizada en el módulo emisor (Object, Thread, StringBuilder, Mutex — **`List` NO**: Core la declara, `Core.bp:427`, y `MivmEmitter.java:4585` sólo la sintetiza si no hay `ClassDef`) → `L<Simple>;`; enum → `E<Mod.Enum>;`. **El código es el de lo que el tipo RESUELVE en el módulo emisor, y el importador reproduce esa misma resolución al leer la bpi** (si no, `Core.List` se manglearía distinto en dueño y consumidor → E3 falso en todo lo que reciba o devuelva `List`; crítico, A10). Un tipo sin resolver es **error de compilación**, nunca `?`. Ejemplos: `toString()s`, `compareTo(LObject;)i`, `m(LLib.Base;)i`, `getP()i`, `setP(i)V`, `__cls_new_Box(i)`, `Lib.f(i)i`, `n:i`, `cambio(i)`.
- **R4 todo lo que cruza lleva clave completa**, también lo ya simbólico: `Mod.f(i)i` (hoy `Mod.f` pelado, #514), `__cls_new_Cls(i)`, `__cls_init_Cls(i)`, `__cls_m_Cls#m(i)i`, `__cls_s_/sg_/ss_`, `__prop_get_x()i`/`__prop_set_x(i)V`. La regla «1ª firma = pelada» (`SemanticAnalyzer.assignKeysInChain:1412-1440`) deja de decidir nada que cruce; `assignKeysInClass:1400-1408` no recorre stubs.
- **R5 invariantes del runtime**, verificadas por el cargador (rechazo nombrado si fallan): Object vt[0]=`toString()s`, vt[1]=`compareTo(LObject;)i`; Thread campos 0/1, vt[0]=`run()V`; Mutex campo 0; `Core.Exception` campo 0 = `msg:s`. Son de clases sintetizadas o del módulo raíz: no cruzan, no se resuelven por nombre.
- **R6 nada se parchea en código.** Lo resuelto vive en la ext-table del módulo (RAM), indexada por operando u16 (patrón CALL_EXT).

#### 3. Formato MOD9 (campo a campo)

**Cabecera:** MAGIC `MOD9`; los 11 enteros BE de MOD8 sin cambios (44 B). Orden de secciones igual. Lo nuevo son DOS subtablas aditivas (por bytes remanentes, como hoy `loader.c:203-235` y `:388-422`).

**§3 imports (v9):**
- 3.1 entradas: `count + (name UTF, fromPath UTF)*` — como hoy; nombres por R4.
- 3.2 deps: como v8 (`module, fromPath, gen, ifaceCrc, flags`). `gen` sigue siendo gate; `ifaceCrc` pasa a informativo (y gate del `.mdn`, §8); el bit EXTIENDE deja de rechazar.
- **3.3 refs (nueva):** `count u16` + por ref: `kind u8, depIdx i16, clase UTF, clave UTF`. `depIdx` ≥0 = índice en 3.2; `-2` = clase abierta de ESTE módulo. `kind`: 0 CLASE (clave vacía → dirección absoluta del descriptor efectivo), 1 MET (→ ranura de vtable), 2 CAMPO (→ ranura de campo), 3 EV_RECV (→ ranura del recv), 4 EV_DEST (→ recv+2), **5 SUBTIPO** (clave = `Mod.Super`; se verifica al enlazar → E10. *Corrección 16-sep, al código:* SÍ ocupa su entrada posicional en la ext-table como las demás, con valor 1 que nadie lee — cuatro bytes por par a cambio de un solo dimensionado en escritor y dos cargadores). Una entrada por (kind, dep, clase, clave) distinta (dedupe). **La ref i vive en `ext[count3.1 + i]`**: la ext-table se dimensiona a `(count3.1 + count3.3) × 4`.

**§4 exports (v9):**
- 4.1 funcs: como hoy, nombres por R4. Sin alias pelados.
- 4.2 data: SÓLO descriptores de clases cerradas (`Mod.Cls → csOff`). **Sin marcadores** `Cls#clave#slot`.
- 4.3 class fixups y 4.4 eh fixups: **no se emiten** (el padre va en 4.5; catch/cast por ref CLASE). Los parsers se conservan para MOD7/8.
- **4.5 CLASES (nueva):** `ver u16 (=1)`, `nClases u16` (TODAS las clases del módulo, sintetizadas incluidas). Por clase: `name UTF; flags u8` (bit0 ABIERTA, bit1 PÚBLICA); `csOff i16` (cerrada: descriptor en data; abierta: -1, **sin descriptor en data**); padre `depIdx i16` (-1 raíz, -2 local) + `parentName UTF`; `nFieldsOwn u16` (ranuras propias de 4 B), `nMethodsOwn u16` (entradas propias, antes de resolver overrides), `bwOwn u16`, `ownFieldBitmap[bwOwn]`, `ownOwnerBitmap[bwOwn]` (relativos a la ranura propia 0); métodos `n u16 + (clave UTF, slot u16, codeOff i32, flags u8: bit0 OVERRIDE, bit1 PRIVADO)`; campos `n u16 + (clave UTF, slot u16, ancho u8, flags u8: bit0 REF, bit1 OWNER, bit2 PRIVADO)`; eventos `n u16 + (clave UTF, recvSlot u16)`.
  - **Cerrada:** sólo miembros **propios** (no heredados), con ranura absoluta en SU vtable/objeto; campos no privados; **también los métodos privados que ocupan ranura** (accesores de `prop private`, `function event` privados), marcados PRIVADO, para que la cuenta y la reflexión sean completas. La cadena de una cerrada está entera en este mismo 4.5 (padre -2): quien busque sube localmente. (Corrige el error de «robustez»: con heredadas serían las 960 entradas de Gui otra vez.)
  - **Abierta:** TODOS sus miembros propios (privados incluidos: su propio módulo los pide por ref); `slot` de método = índice propio i (informativo; los overrides no consumen ranura nueva), `codeOff` = offset del cuerpo en `code`; campos y recv relativos a la ranura propia 0.

**§6 interface (bpi 9):** sin `layout nF nM fbm obm`, sin `slot s` en `field`, sin `recv N` en `event` (`ModuleInterface.java:22-28, 616-671`). Nombres, firmas con retorno, tipos, visibilidad, `staticconst` inlineada. `ifaceCrc` se sigue calculando.

**Descriptor de clase (runtime):** layout SIN cambios (`bpvm_internal.h:73-78`): `+0 nF, +2 nM, +4 bw, +8 parentOff` CS-relativo al módulo dueño, bitmaps, vtable `nM×i32` CS-relativos, `-1` = heredado. **Un cambio: `+6` (hoy `_pad`, sin lector) = índice de módulo dueño + 1, escrito por el CARGADOR** en todos los descriptores (cerrados al cargar, construidos al construir). El compilador sigue escribiendo 0 (MOD7/8 traen 0 → camino por rango).

#### 4. Emisor (lexer-java + generador/ModWriter)

- `ClassSymbol.isOpen()` (R1). `Symbol.claveCanonica` (R3) con error si un tipo no resuelve.
- `ModWriter.requireRef(kind, depIdx, clase, clave) → idx` con dedupe; el operando u16 se escribe como `idx` y se **backpatchea** a `idx + count3.1` al cerrar el módulo (como los saltos). Sustituye a `requireExternalSymbol(Mod.Cls#clave#slot)` (`MivmEmitter.java:4281-4289`).
- Sitios (censo del lector emisor, 18 filas) → `_REF`: #1 INVOKE_VIRTUAL externo (`:4271-4290`) → `INVOKE_VIRTUAL_REF` con ref MET sobre la **clase ESTÁTICA del receptor** (como `javac`); el cargador sube por la cadena desde ahí, así que cubre un método subido al padre Y uno bajado al hijo (crítico, A5); `super.m()` por `__cls_m_Cls#clave` se documenta como resolución exacta (divergencia con `invokespecial`); #2 método propio/override de abierta (`:4291-4315`) → ídem con depIdx -2; #3 campo externo (`:2200-2216`) y #4 campo propio de abierta (`ModWriter.java:800-811, 892-931`) → `GET/SET_FIELD[_LONG|_OWNER]_REF`; #7 recv/dest de evento externo o de abierta (`:2462-2468, 2523-2528`) → `SET_FIELD_LONG_REF` (EV_RECV) + `SET_FIELD_REF` (EV_DEST); #8 PUSH del slot del handler (`:2446-2453`, `SemanticAnalyzer.java:2172-2179`) → `PUSH_REF` (MET del receptor) si el receptor es externo/abierto; #9 accesores de property externa (`:2744-2761…`) → `INVOKE_VIRTUAL_REF` con `getX()t`/`setX(t)V`; #10/#11/#14 nombres por R4 (`ctorSuffix:1829-1834` deja de ser posicional; `Symbol.externalQualifiedName:300-309`); #13 `is`/cast/catch de externa o abierta → `INSTANCEOF_REF/CHECKCAST_REF/TRY_BEGIN_REF` (ref CLASE); el parser acepta `Mod.Cls` en `instanceof` (`Parser.java:1489-1497`; hoy revienta con traza); `new` de abierta local → `NEW_OBJECT_REF`; `new` externo sigue por CALL_EXT `__cls_new_Cls(firma)`.
- `endClass` (`ModWriter.java:1084-1170`): cerrada → descriptor completo como hoy + entrada 4.5 (propios); abierta → sin descriptor, entrada 4.5 con propios relativos y `codeOff`. `addClass(name, parentRef)` sin `ExternalParentLayout`/`__inhN` (`:778-832, 862-889`). `exportVtableMarkers` (`:457-477`) se borra. Cross-check #174b (`MivmEmitter.java:4296-4314`) sólo para cerradas.
- `computeClassLayout` (`ModuleInterface.java:892-1018`) calcula sólo lo propio, relativo.
- **Propiedad del compilador (guardián §11):** el `.mod` del consumidor compilado contra dos versiones de la misma interfaz (mismos nombres y firmas, ranuras distintas) sale **byte-idéntico salvo la subtabla deps**.

#### 5. Opcodes nuevos (11; ambas VMs; operando `idx u16` = entrada de la ext-table del módulo del CS actual)

| Op | Operandos | Semántica |
|---|---|---|
| 0xB1 `INVOKE_VIRTUAL_REF` | idx u16, nargs u8 | `slot = ext[idx]`; después idéntico a INVOKE_VIRTUAL (paseo por -1/parentOff incluido) |
| 0xB2 `GET_FIELD_REF` 0xB3 `SET_FIELD_REF` 0xB4 `GET_FIELD_LONG_REF` 0xB5 `SET_FIELD_LONG_REF` 0xB6 `SET_FIELD_OWNER_REF` | idx u16 | `slot = ext[idx]`; después idéntico al no-REF (`bpref_field`/`fieldAddr`) |
| 0xB7 `NEW_OBJECT_REF` | idx u16 | `desc = ext[idx]` (absoluto); después idéntico a NEW_OBJECT |
| 0xB8 `INSTANCEOF_REF` 0xB9 `CHECKCAST_REF` | idx u16 | `desc = ext[idx]`; identidad + cadena parentOff como hoy |
| 0xBA `TRY_BEGIN_REF` | idx u16 + resto de operandos de TRY_BEGIN | ídem; sin eh-fixup, sin tabla lateral, sin parcheo |
| 0xBB `PUSH_REF` | idx u16 | push i32 `ext[idx]` (slot del handler para `$dest`) |

Elección frente a reutilizar CHECKCAST_EXT/TRY_BEGIN_EXT: un solo mecanismo (lectura O(1) de la ext-table) en vez de dos (parcheo de código + tabla lateral buscada por `code_off` en XIP, `interp.c:1864-1874`); los `_EXT` quedan para MOD7/8. La ranura que llega por ref es i32: sin límite u8 en lo cruzado. `Disasm` imprime `ref[i] = kind Mod.Cls.clave`.

**Dos cambios de camino caliente en ambas VMs (mismo commit que la primera clase construida):** (1) `getCSForDataAddr`/`bpvm_get_cs_for_data_addr` (`ModuleManager.java:1310`, `link.c:23-42`): `idx = u16(desc+6)`; si `idx>0` → `modules[idx-1].code_start` (O(1)); si 0 → barrido por rango de hoy (MOD7/8). Aserto: `modules[idx-1]` contiene `desc` o `desc` está en la arena. Sin esto, un catch/raise sobre una clase construida (`events.c:121/128`, `exceptions.c:54`) cae a CS 0 en silencio. (2) `bpvm_get_ext_table_addr` (`link.c:76`, barrido lineal por CALL_EXT y ahora por cada `_REF`): caché de 1 entrada `(cs → ext_table_addr)` en el contexto del hilo, igual que `cb_cache_cs` (`link.c:70-73`); miVM ídem.

**Dos helpers por VM ya en fase 1:** `cls_parent(desc)` y `cls_method(desc, slot) → (cs, off)` concentran el paseo hoy repetido en `interp.c:1744-1774, 373-398`, `events.c:108-130`, `threading.c:249-263`, `builtins.c:1295-1310` (y sus gemelos de miVM `:3873-3895, 6112-6129, 4838-4848, 6356-6363`). Conservan el `-1`; dejan lista la vtable plana como optimización MEDIDA de fase 2 sin volver a tocar los 13 sitios. De paso `builtins.c:1310` usa `bpvm_cb_for_cs` (bug XIP lateral).

#### 6. Cargador paso a paso (algoritmo ÚNICO: miVM primero, luego link.c)

**Carga (`loadModuleToMemory` / `load_buffer_impl`), por módulo, orden de hoy (raíz primero):**
1. Cabecera; imports 3.1 + deps 3.2 + **refs 3.3** (retenidas como los imports hasta el enlace: `bpvm_strdup`, `bpvm.c:1603-1605`; miVM `String[]`). `ext_table_size = (count3.1 + count3.3)*4`; **las entradas de refs se inicializan a `0xFFFFFFFF`** (no a 0: la ranura 0 es `toString` legítima; `0xFFFFFFFF` no es ranura ni dirección válida).
2. Exports 4.1/4.2 como hoy (nombres por R4). 4.3/4.4 sólo si MOD7/8.
3. **4.5 CLASES:** XIP → guardar puntero a flash (0 B de RAM; el `.mod` entero está mapeado, `loader.c:596-599`). Stream/FS → copiar a la arena **al final de la carga, DESPUÉS de `next_free_address = end_addr+64` (`loader.c:448`)**, con `memmove` (el staging de exports vive en `end_addr`, `loader.c:262-271`, y puede solapar). Nunca `bpvm_arena_reserve` mientras `next_free_address == module_base` (pisaría la ext-table del propio módulo). miVM: objetos `ClaseInfo` por clase.
4. Interface y native saltadas; data a RAM; code según XIP. `comprobar_norma` para dueño MOD9: sólo `gen` (E0); `ifaceCrc` distinto → diag informativo + marca `enlazado_tarde` en `bpvm_module_t`/`ModuleMetadata` (la usa el gate del `.mdn`, §8).

**Enlace (`linkAll` / `bpvm_link_all`, con TODOS los módulos cargados, antes del heap: `ModuleManager.java:608-609`, `bpvm.c:1359`):**
1. Imports 3.1 → ext-table (como hoy).
2. **Registro de clases** `cls_reg[]` en arena: `(módulo u8, clsIdx u16, descAbs u32)` para toda clase de 4.5 (cerrada: `code_start+csOff`; abierta: 0 = pendiente). **Sellar `+6 = módulo+1`** en cada descriptor cerrado. **Verificar R5** (Core.Exception `msg:s` en 0; en cada módulo Object `toString()s`→0, `compareTo`→1; Thread `run()V`→0).
3. **Construcción memoizada padre→hijo**, `construir(C)`: si `descAbs != 0` → hecho. `P = buscar(depIdx, parentName)` (no existe → **E1**); `construir(P)` (ciclo → **E7**). `nF = P.nF + nFieldsOwn`; `bw = ceil(nF/32)`; `nM = P.nM`; por cada método propio en orden: `s = buscarClave(cadena de P, clave)` **ignorando las entradas PRIVADAS** (los accesores de `prop private` y los `function event` privados ocupan ranura pero no se sobreescriben: el hijo recibe ranura nueva — si no, el `self.p` interno del padre despacharía al getter del hijo, en silencio; crítico, A2); si existe → **override en s** (marcado o no: criterio JVM; si no estaba marcado se anota en el diag); si no existe y OVERRIDE → **E2**; si no existe → `s = nM++`. Rechazo **E6** si `nF` o `nM` > 65535. Reservar `12 + 8·bw + 4·nM` en la arena: cabecera, `+6 = módulo del hijo + 1`, `parentOff = P.descAbs − hijo.code_start`, bitmaps = bitmap efectivo de P (de su descriptor, ya completo) OR (propios << P.nF), vtable = `-1` en `[0, P.nM)` y `codeOff` en las propias/overrides. Registrar `Mod.Cls → descAbs` (data-symbol, para nietos y refs CLASE) y `cls_reg`. Guardar por clase construida `(P, baseFields = P.nF, baseMethods = P.nM, ownSlot[i])`: en C no se materializa la tabla efectiva, se recalcula al buscar (0 RAM extra).
4. **Resolver refs 3.3** del módulo, escribiendo `ext[count3.1 + i]`: CLASE → `descAbs` (E1); MET → `buscarClave(clase efectiva y sus padres)` → ranura (**E3**; si hay otra clave con el mismo nombre se cita: «tiene `k()i`»); CAMPO → ranura (**E4**; tipo distinto = clave distinta = E4); EV_RECV/EV_DEST → `recv`/`recv+2` (**E5**); SUBTIPO → sube la cadena efectiva de la clase (**E10**). **`__syncMutex:LMutex;` se resuelve al PRIMER eslabón desde la RAÍZ** (un solo cerrojo, como el monitor de Java; el prólogo del hijo escribe su Mutex en esa ranura tras `super()`; la ranura propia sobrante queda sin uso) — y el importador **propaga `isSync` de la bpi**: hoy `Main.java:2345` lo pierde, la hija cross-module declara su propio `__syncMutex` y los `sync` del padre y de la hija se excluyen contra cerrojos DISTINTOS sobre el mismo objeto (carrera preexistente, sólo visible en carrera; crítico, A6).
5. **Verificación post-construcción** (siempre, ambas VMs, lineal): toda vtable construida sin huecos (cada `-1` tiene pareja en la cadena del padre y `slot < P.nM`); sin dos claves iguales en la cadena efectiva (**excluyendo privados**: dos `helper` privados en la cadena son legítimos); `bw == ceil(nF/32)`; bits REF/OWNER propios ⊆ ranuras propias (`[P.nF, nF)`); toda ref del módulo `≠ 0xFFFFFFFF`. Fallo → **E8** nombrando la clase.
6. PC/CS de main como hoy. Diag: la línea de símbolos actual (`link.c:261`) + **`[bpvm] enlace tardio: C clases construidas, R refs, B bytes de arena, T us`** (cronómetro en ambas VMs).

**Mensajes (byte-idénticos en las dos VMs; canal: `vm->link_error`/`vm->load_error` → wire e IDE, `LinkException` en miVM con el MISMO texto; `mezcla.py` los compara). Los buffers `link_error`/`load_error` pasan de 160 a **256 B** (un E3 con nombres reales de la stdlib supera 160 y la VM-C truncaría donde miVM no: paridad rota; crítico, A7) y `bpvm_link_set_error` deja de añadir el sufijo `#layout#` (`link.c:180`, 0 emisores):**
- E0 gen viejo: texto actual de `comprobar_norma`.
- E1 `enlace: 'App' necesita la clase 'Lib.Base' y 'Lib' (gen 8) no la tiene: recompila 'App'`
- E2 `enlace: 'App.Hijo' redefine 'm(i)i' y 'Lib.Base' (gen 8) ya no lo tiene: recompila 'App'`
- E3 `enlace: 'App' necesita 'Lib.Base.k()s' y 'Lib' (gen 8) no lo tiene (tiene 'k()i'): recompila 'App'`
- E4 `enlace: 'App' necesita el campo 'Lib.Base.p:i' y 'Lib' (gen 8) no lo tiene (tiene 'p:s'): recompila 'App'`
- E5 `enlace: 'App' necesita el evento 'Lib.Base.cambio(i)' y 'Lib' (gen 8) no lo tiene: recompila 'App'`
- E6 `enlace: 'App.Hijo' tendria N ranuras de campo|vtable (maximo 65535)`
- E7 `ciclo de herencia en 'A.X'` (el código nombra la clase donde se cierra el ciclo, no el par)
- E8 `enlace: la clase 'App.Hijo' construida es inconsistente (<detalle>): .mod corrupto o bug del cargador`
- E9 `enlace: 'Lib' es MOD7 y 'App' necesita su tabla de clases: recompila 'Lib'` (§10)
- E10 `'App' asume que 'Lib.Derived' extiende 'Lib.Base' y 'Lib' (gen 8) ya no: recompila 'App'` (sin prefijo `enlace:`, como el resto: sale dentro de `=== ERROR DE LINK: … ===`; el dueño citado es el de S)

**Reflexión (Forms `__guiSlotOf`/`__methodRef`):** `slotOfMarker` (`ModuleManager.java:1331-1343`) y `builtins.c:1172-1185` pasan a: `class_ptr → +6 → módulo → cls_reg (por descAbs) → 4.5 de la clase y sus padres → primera clave cuyo nombre simple coincide` (misma semántica que `#nombre#`; documentado: Forms no sobrecarga manejadores). En XIP lee 4.5 de flash; en stream de la arena.

#### 7. XIP / packs

Sólo `code` en flash (`loader.c:296-314`); 4.5 se lee de flash sin copia; refs y descriptores construidos en la arena de `memory[]` (misma vida que el RUN, sin malloc de plataforma; precedentes `loader.c:535` y #430); operandos `_REF` en flash intactos; la tabla lateral de eh-fixups ya no la usan módulos MOD9. Módulo en pack + hijo abierto en /app: funciona (el hijo se construye en RAM). `bpvm_mods.c:37-44` (sincronizar /lib) compara **gen además de MAGIC** (#523).

#### 8. AOT (.mdn) — primera fase, estricta, EN EL MISMO COMMIT que quita los marcadores

Hoy la única barrera de la ranura horneada en C nativo (`AotCEmitter.java:1889, 1992`: `call_method_i32(vm, this, <slot>)`) es el marcador del `.mod` (medido: `b, 4` → `b, 5` al insertar un método delante). **Fase 1** (crítico, A3): (a) el emisor AOT **sólo hornea ranuras de clases CERRADAS del PROPIO módulo**; un método de clase externa o abierta → `UnsupportedAotException` (cuerpo interpretado, con aviso), como ya hace con lo que no traduce; (b) el bloque embebido se registra al cargar (`loader.c:548-590`) pero se **activa** en `g_after_link` (`bpvm.c:1352-1362`), y el mismo gate en `bpvm_mdn_escanear` (FS/pack): sólo si **ningún módulo de la clausura de deps cambió de FORMA** — medido por un **`layoutCrc`** = CRC de la subtabla 4.5 propia (privados con ranura incluidos), NO por `ifaceCrc` (la bpi no publica los manejadores privados: mismo texto, otra vtable, `.mdn` activo con la ranura vieja); si no, log `AOT: X corre interpretado: <dep> cambio de forma (%08x != %08x): recompila` y sigue interpretado, nunca error de carga; (c) `interp.c:299-302` «función destino no encontrada» nombra el símbolo; (d) receta `#441` sin cambio. **Fase 2** (después, medida): helpers `call_method_ref/call_bp_ref` al final de `aot_helpers_v2` (ABI 7) leyendo `modules[mod].ext[idx]`, `mod_idx` en `bpvm_aot_callctx_t` (`bpvm_internal.h:1046-1069`). Hallazgo lateral con ficha aparte: una `native` que toca una global → `.bss` → `MdnPack` la rechaza.

#### 9. GC, eventos, sync, threads, debugger

- **GC:** `markObject/mark_recursive/free_owned/objectTotalSize` no cambian (`heap.c:54-59, 259-282`; `VirtualMachine.java:1647-1660`): leen nF y bitmaps del descriptor de la clase del objeto; para construidos, del descriptor en arena (fuera del heap, sin refs: no se marca). La verificación §6.5 corre antes de la primera asignación.
- **Eventos:** cola, `EVENT_RAISE` y drenaje sin cambio (`events.c:108-130` vía `cls_method`); cambia quién escribe `$recv/$dest` y el slot del handler (refs).
- **Sync:** `__syncMutex` es un campo del primer eslabón con sync property (`MivmEmitter.java:1286-1294`); exportado en 4.5; va por ref como cualquier campo de abierta. Padre que añade su primer `sync` después del hijo → dos campos en la cadena: decisión D4.
- **Threads:** `THREAD_START` (vt[0]=run, campos 0/1) sobre Thread sintetizado local: sin cambio; con `cls_method` desaparece el rechazo «run() heredada» de `threading.c:252-259`.
- **Debugger/.dbg/wire/IDE/.win:** sin cambio (nada horneado cross-module; STAT = magic+crc). `Disasm` cosmético.
- **Paridad:** ninguna tabla nueva influye en stdout; el orden de carga no cambia; los rechazos son texto idéntico.

#### 10. Compatibilidad MOD7/MOD8

- **Cápsula V6 (MOD7 con MOD7):** camino intacto (marcadores, INVOKE_VIRTUAL u8, fixups 4.3/4.4, tabla lateral, `+6 = 0` → por rango, `comprobar_norma` con sus 3 reglas).
- **Consumidor MOD9 + dueño MOD7/8:** **rechazo E9** nombrando el módulo. Sin camino degradado: sintetizar claves desde marcadores reabre exactamente los cuatro silenciosos medidos (cabeza sin firma, retorno, tipo de property, firma de función). Los imports puros de función (3.1) sí resuelven si el nombre existe.
- **Consumidor MOD7/8 + dueño MOD9:** sus marcadores/`Mod.f` pelados no existen → error de enlace de hoy nombrándolos («recompila»). Política ya decidida: hacia atrás sí, hacia delante no.
- **Parque instalado — decisión de Eduardo (D10):** «un programa antiguo corre con JRE más moderna» incluye al `.mod` MOD8 de V6 que vive en `/app` de las placas: con firmware V7 la stdlib embebida MOD9 sustituye a `/lib` («versión anterior → se instala», `bpvm_mods.c:37-44`) y **todo consumidor MOD8 muere en el enlace** («no exporta `Gui.Window#show#12`»). Cierre barato y sin RAM (crítico, A4): un import MOD8 con forma `Mod.Cls#clave#N` que no exista se valida contra 4.5 de la clase cerrada (entrada N con nombre `clave` bajo la regla vieja: cabeza = nombre pelado, resto `$mangle`); el consumidor MOD8 queda **exactamente igual de (in)seguro que hoy** (no se reabre nada: sus cuatro silenciosos ya los tiene) y `comprobar_norma` con `ifaceCrc` sigue rechazando si la stdlib cambió de forma. La otra opción es rechazar («recompila») y asumir que el parque de V6 no corre en V7 sin recompilar. En cualquier caso, comparar `gen` además de `MAGIC` al sincronizar `/lib` es obligatorio.

#### 11. Lo que se BORRA

- **Interfaz** (`ModuleInterface.java`): `layout` (`:22, 616-633`), `slot s` de `field` (`:23, 940`), `recv N` de `event` (`:28, 653-671, 967-975`), `publishedLayoutOf:873-889`, rama externa de `baseLayoutOf:821-833`, `opaque` como concepto de ranura.
- **Importador** (`Main.java`): `sembrarSlotsDeVtable:1935-1945`, `cruzarSlots:1973-2005`, siembra Object/Thread `:2268-2302`, `clavesPropias/deferSlots :2380-2418, 2590-2593`, `binaryLayout :2244-2248`, `externalSlot :2335`, `externalRecvSlot :2361`, regla posicional para stubs/funciones/ctors (`:2192-2204, 2403-2406, 2459-2462, 2491, 2512`), sidecar `.slots` (`:776-815`; 0 lectores; `FrmMain.java:828` deja de abrirlo).
- **Símbolos** (`Symbol.java`): `externalMethodSlots:78`, `binaryLayout:85`, `externalRecvSlot:421`, `externalSlot:440`; `slotOf/ensureMethodSlots:145-189` sólo para cerradas; `overloadMangle/typeCode` → `claveCanonica`.
- **ModWriter:** `ExternalParentLayout`+`__inhN` (`:778-832, 862-889`), `exportVtableMarkers:457-477`, `methodSlotOrMinus1:1311-1319` para abiertas, `ctorSuffix` posicional.
- **Emisor:** `requireExternalSymbol` del marcador (`:4281-4289`), cross-check #174b para abiertas.
- **VMs:** regla 3 de #500 (EXTIENDE) para MOD9; el marcador `#layout#` de los mensajes (`link.c:180`, `ModuleManager.java:547`; 0 emisores).
- **Tests:** `CrossCheckSlotsTest`; `SlotsVtableCrossModuleTest:143-191` y `SobrecargaCrossModuleTest:123-148` reescritos en términos de (clase, clave).

#### 12. Orden de trabajo (cascada Java → C → Pico, un commit por paso, oráculo `dist/v6`)

0. **Línea base ANTES de tocar nada:** cronómetro alrededor de `linkAll`/`bpvm_link_all` en ambas VMs (host) y ciclos en la Pico; línea de símbolos; tamaños de los 11 `.mod`; matriz `mezcla.py` de hoy (84 filas) archivada.
1. **Frontend:** `claveCanonica` + error por tipo sin resolver; bpi 9; importador sin numeración; `ModWriter` (3.3, 4.5, sin marcadores/4.3/4.4, backpatch de refs); 11 sitios `_REF`; `instanceof Mod.Cls`; `--barajar/--relleno`; tests. Guardián §11 de la validación en verde con la stdlib compilada como MOD9 (aún sin VM que la ejecute).
2. **miVM:** parser 3.3/4.5, `cls_reg`, arena, `+6`, `construir`, resolución, verificación E8, mensajes E0-E9, 11 opcodes, `cls_parent/cls_method`, `getCSForDataAddr` O(1), caché de ext-table, reflexión por 4.5, cronómetro, Disasm. Tests JUnit del cargador (12 fixtures con la vtable esperada escrita de antemano). Primer caso real: `Collections.OwnerList` bajo `Gui`.
3. **VM-C host:** espejo en C (loader.c, link.c, interp.c, builtins.c, events.c, threading.c, exceptions.c, bpvm_internal.h) + gate AOT fase 1 en el MISMO commit que deja de leer marcadores; diff de stdout y de mensajes contra miVM en mezcla.py y en todos los samples; modo pack/XIP en host.
4. **Pico 2:** stream desde /lib + pack XIP; medidas; luego P4 (Forms) y lote cross-family.
5. **stdlib** como MOD9 en las 4 copias (`rm -rf bpstdlib/out`); docs (MOD_FORMAT §MOD9, OPCODES, HEAP_LAYOUT `+6`, BPVM_WIRE textos E0-E9, PHILOSOPHY criterio Java); FICHAS #500/#514/#516/#523.

#### 13. Presupuesto y criterio de aceptación numérico

Partida medida hoy (VM-C host): JsonDemo «470 simbolos, 12161 B de nombres, 20480 B en total»; GuiTableDemo «1739 simbolos, 43181 B, 81920 B» (malloc de plataforma = margen de 64 KB en la Pico, #449); 1290 de 1739 símbolos son marcadores; enlace 79 búsquedas × 1199 strcmp de media; Gui.mod 61 558 B (960 marcadores = 23,5 KB, exports 46 %).

- **RAM (Pico, programa Gui, stream):** tabla de símbolos ≈450 entradas / ≈18 KB de nombres (−35 KB de malloc); 4.5 retenida en arena ≈ Gui 4,5 + Json 1,7 + Core 2 + Collections 0,8 ≈ **9 KB** (0 en XIP); `cls_reg` ≈ 8 B × 55 ≈ 0,5 KB; refs 4 B cada una (GuiDemo 5, JsonDemo 12, Gui.mod ≈30); descriptores construidos sólo para abiertas (`MainWin extends Gui.Window` ≈ 190 B; `OwnerList` ≈ 60 B; ya estaban en data: ±0). **Neto ≈ −25 KB (stream) / −35 KB (XIP) de RAM de plataforma.** Criterio (corregido por el crítico, A8/A11: «en total» crece por duplicación, `link.c:110-131`, y no bajaría de ≈37 KB): **B de nombres ≤ 20 KB (Gui) / ≤ 5 KB (Core-only)**, medido también el pico transitorio del `realloc` en la Pico; arena ≤ 12 KB (Gui, stream) y ≤ 1 KB en XIP salvo clases abiertas; y **el heap disponible al programa** (run-guard `#339`) antes/después — la 4.5 en arena (≈9 KB en stream) sale del `memory[]` del programa, así que se retiene tras el enlace SÓLO en los módulos con clases alcanzables por `GUI_SLOT_OF` (Forms es genuinamente dinámico: `__guiSlotOf(win, nom + "_onChange")`, `Gui.bp:1381`) o se copia al malloc de plataforma y se libera al enlazar cuando no hay Forms; los samples al borde del OOM que cambien de stdout frente a la cápsula se listan de antemano con motivo, no se descubren en rojo.
- **Flash:** Gui.mod ≈ 42-45 KB (−23,5 marcadores, +4,5 de 4.5, +2 de firmas, −1 de bpi); Core ≈ 13,5-14,5 KB (sus marcadores eran cortos); stdlib total ≈ −15 %. Consumidores ±0 (una ref ≈ el marcador+fromPath de hoy). Criterio: `Gui.mod ≤ 46 KB`; suma de la stdlib ≤ hoy − 15 KB; grep de aceptación: 0 cadenas `#…#N`, 0 exports sin firma salvo `__init`.
- **Carga/enlace:** por ref ≈ búsqueda de clase (≤55 strcmp cortos) + miembro en la cadena (≤44 claves con fallo temprano) ≈ 100 strcmp × ≈100 refs ≈ 10 K strcmp frente a 95 K hoy; construcción lineal en nM+nF. Sin hash en fase 1 (la tabla baja un 74 %; el hash se añade sólo si el cronómetro lo pide). Criterio: **enlace ≤ 2 ms en Pico (Gui) y ≤ 0,5 ms (Core-only), medido con ciclos, y no peor que la línea base**; host < 1 ms.
- **Runtime:** `INVOKE_VIRTUAL_REF` = INVOKE_VIRTUAL + 1 lectura indexada − el barrido de módulos por despacho (`+6` O(1), también para cerrados) − el barrido de `bpvm_get_ext_table_addr` (caché). Criterio: micro-bench de 1e6 llamadas a un método heredado cross-module **no más lento** que hoy (esperado más rápido).

#### El plan de validación al 100 %

**Criterio global (Java):** donde Java correría, corremos con stdout correcto; donde Java lanzaría, rechazo al enlazar nombrando `Mod.Cls.clave`; 0 filas SILENCIOSO / PARIDAD / CRASH; stdout **y texto de rechazo** byte-idénticos entre VMs en todas las filas (el arnés no tira stderr, #494).

**V0 — Línea base (antes de cambiar nada).** Cronómetro en `linkAll`/`bpvm_link_all` (host, us) y ciclos en la Pico; línea de símbolos; `ls -l` de los 11 `.mod`; matriz mezcla.py de hoy (84 filas) archivada en `compat/informes`. Sin línea base no hay «no más lento».

**V1 — Guardián del COMPILADOR (`--barajar=<semilla>`, `--relleno=<n>`).** Permuta el orden de asignación de ranuras propias (vtable y campos) de cada clase de la librería e inserta n miembros privados de relleno. Test JUnit + bloque de mezcla.py: compilar App contra Lib con semillas 0..7 → `App.mod` byte-idéntico salvo la subtabla deps; ejecutar App(semilla 0) contra Lib(semilla k) en las dos VMs → stdout idéntico. Criterio: 8/8 y 8/8 con `--relleno=3`. Es la única prueba que caza cualquier número horneado que se escape del censo de 18 sitios sin leerlos.

**V2 — Unidades del frontend.** `claveCanonica`: tabla (tipo→código) y (firma→clave) con retorno; clases declaradas cualificadas vs sintetizadas simples; enum; tipo sin resolver → error (test negativo). Escritor 3.3/4.5: ida y vuelta campo a campo (`InterfazIdaYVueltaTest` ampliado). Emisor: contar por opcode en el `.mod` → 0 INVOKE_VIRTUAL/GET_FIELD u8 sobre clases abiertas o externas. Sustitutos de `SlotsVtableCrossModuleTest`/`SobrecargaCrossModuleTest`: «el consumidor pide exactamente estas (clase, clave)», «el dueño publica estas (clave, ranura) entre sus PROPIAS». Criterio: suite verde; grep de `#…#N`, `layout N`, `recv N`, `slot N` en src/test = 0.

**V3 — Cargador miVM (JUnit sobre ModuleManager, sin ejecutar programas).** Fixtures Base v1/v2 × Hija con la vtable esperada (lista de (cs, off) por ranura), nF y bitmaps ESCRITOS DE ANTEMANO: (a) método añadido delante/en medio/detrás, (b) property nueva, (c) reorden completo, (d) campo nuevo delante, (e) override que reutiliza ranura, (f) sobrecarga que añade, (g) quitado → E3, (h) firma cambiada → E3 con «tiene …», (i) retorno cambiado → E3, (j) tipo de property cambiado → E3/E4, (k) abuelo en tercer módulo, (l) hija de hija cross-module, (m) padre añade la firma del hijo → override (diag), (n) override huérfano → E2, (o) clase quitada → E1, (p) ciclo → E7, (q) nF > 65535 → E6, (r) `.mod` corrupto a mano (un -1 sin pareja) → E8, (s) ref sin resolver (0xFFFFFFFF) → E8, (t) bitmap que cruza la palabra 32 (padre de 30 ranuras + hijo con refs), (u) `+6` correcto en cerrados y construidos (aserto), (v) R5 rota (Core.Exception con msg en 1) → rechazo, (w) padre con `prop private p` e hijo con `prop p` → dos ranuras distintas, (x) dos helpers privados homónimos en la cadena → sin E8, (y) `Derived` deja de extender `Base` → E10. Criterio: 25/25 como la tabla.

**V4 — Matriz mezcla.py con criterio Java.** Reescribir `prediccion_usa/prediccion_ext` (`:297-305`): gen < sellado → E0; si no, variantes igual/cuerpo/met_final/met_medio/met_quitado_nousado/campo_mas/campo_menos/prop_mas/reorden/ev_privado/enum_cabeza → «corre» con la salida esperada (Usa Y Ext); met_quitado/firma/retorno/prop_tipo/f_firma → rechazo con texto E3/E4 exacto. Variantes nuevas: padre_anade_firma_del_hijo (corre, cuerpo del hijo), padre_anade_sync (según D4), abuelo (A→B→C, B recompilado en medio → corre), metodo_movido_al_padre (corre), evento_nuevo_delante (corre), campo_ref_delante_>32 (corre; se cruza con V6), sobrecarga_delante_de_cabeza para método/ctor/función de módulo (corre), clase_quitada (E1), override_huerfano (E2), instanceof_cross (corre), catch_cross_construido (corre), handler_obj::m_con_base_crecida (corre), consumidor MOD8 × dueño MOD9 con forma IGUAL y con forma DISTINTA (según D10), consumidor MOD9 × dueño MOD7 (E9), **derived_deja_de_extender** y **excepcion_deja_de_extender_Exception** (E10), **padre_anade_prop_privada_homonima** y **padre_anade_handler_privado_delante** (corre), **metodo_bajado_al_hijo** y **super_m_movido_al_abuelo** (corre), **enum_valor / default_param / staticconst cambiados** (salida esperada ESCRITA: divergencia `static final`), y **una variante con DOS averías** (met_quitado + campo_menos) para fijar que miVM y la VM-C reportan el MISMO primer error. ≈ 16+13 variantes × gen 6/7/8/9 × Usa/Ext/Puente × 2 VMs ≈ 250 filas. Criterio: **100 % de celdas = predicción escrita antes; 0 SILENCIOSO; 0 CRASH; PARIDAD en toda celda (stdout y texto de rechazo)**.

**V5 — Regresión de lo cerrado.** `compat/compat.sh check` + tanda.py sobre TODOS los samples (ok y de error) compilados a MOD9 contra la stdlib MOD9: diff byte a byte Java/C y contra la cápsula V6 (`dist/` como oráculo) para lo que no cambia de semántica; PASS = censo actual. Diff de secciones `code`/`data` de Core/Gui/Json MOD8 vs MOD9: byte-idénticas salvo los sitios `_REF` de las clases abiertas de Collections y los consumidores de OwnerList en Gui — cualquier otra diferencia es bug del emisor. Forms/GUI: GuiHambre, GuiTableDemo, GuiShot con `--screen` y captura PNG idéntica antes/después (reflexión por 4.5). Eventos cross-module (`obj::metodo` a clase externa), catch/cast cross-module, `instanceof Lib.Cls`, threads con `extends Thread`, sync en cadena cross-module.

**V6 — GC y bitmaps reconstruidos.** Sample con jerarquía cross-module de 3 niveles con campos ref, long y owner mezclados en cada nivel y el padre recompilado con campos ref nuevos delante (cruzando la palabra 32); bucle largo de asignación (bug probabilístico) con heap estrecho, `GC_EVERY=1` y `--nogc` en la VM-C; run-guard #339 = 0 fugas; salida idéntica en las dos VMs. Criterio: verde en 3 tamaños de heap decrecientes hasta OOM atrapable; nunca corrupción. Y la **carrera de A6**: `sync` en padre (Lib) e hijo (App) sobre el mismo objeto desde dos hilos con contador — hoy falla (`Main.java:2345`) y sirve de línea base rojo → verde.

**V7 — AOT.** C host con NatV7/XMethodNat: `.mdn` activo vs saltado → stdout idéntico; mismo App.mdn con Lib cambiada (ranura movida) → log «corre interpretado…» y stdout el de Java; símbolo ausente → error nombrándolo. Mismo gate por `bpvm_mdn_escanear` (FS/pack). Y los dos que atraviesan un gate por `ifaceCrc`: `--barajar` y un `function event` privado insertado delante → deben loguear «corre interpretado» y dar el stdout de Java.

**V8 — Cascada C → placas (tras Java y C verdes).** C host en modo pack/XIP (pack con Core+Gui+Collections MOD9 y app con `MainWin extends Gui.Window` y `MyError extends Core.Exception`): mismo stdout que miVM. Pico 2 (tanda.py, firmware compilado en cada tanda, binario known-good antes): (1) /lib MOD9 por stream + app en /app; (2) pack XIP con módulo abierto y catch cross-module; (3) /lib actualizado a medias (MOD8 + MOD9) → E9/rechazo por el wire con el mismo texto; (4) los casos g-j de V3 en placa. ESP32-P4: GuiHambre + Forms (`__methodRef`/`GUI_SLOT_OF` por 4.5; captura PNG). Metro: Orm con `extends Orm.Dao`. STM32/S3/C6 en lote cross-family (compilar las 5 familias, 2 samples cada una). Criterio: el log de la Pico es la referencia; misma salida que el host; ningún «no soportado».

**V9 — Medidas (criterio numérico, misma tanda).** B de nombres de símbolos ≤ 20 KB (Gui) / ≤ 5 KB (Core-only), con el pico del `realloc`; heap disponible al programa (run-guard) no menor que hoy en los samples de la cápsula (los excluidos, listados de antemano); arena ≤ 12 KB stream / ≤ 1 KB XIP salvo abiertas; `Gui.mod ≤ 46 KB`, stdlib ≤ hoy − 15 KB; enlace en Pico ≤ 2 ms (Gui) / ≤ 0,5 ms (Core-only) y ≤ línea base V0; host < 1 ms; micro-bench 1e6 INVOKE_VIRTUAL_REF sobre método heredado cross-module ≤ hoy. Aceptación de formato por grep: 0 `#…#N`, 0 exports sin firma salvo `__init`, `strings Gui.mod ≤ 45 KB`.

**V10 — Revisión adversarial (método del 14-sep).** 4 refutadores escriben lo que nadie escribe: abierta con 40+ métodos propios y 3 niveles en 3 módulos; dos módulos con clases homónimas como tipos de parámetro; override en cadena de 3 módulos con `super()` en medio; evento declarado en el abuelo y suscrito desde el nieto; sync property añadida después; clase privada abierta; excepción abierta atrapada por su padre cross-module; Forms con manejador sobrecargado; objeto de clase abierta atravesando un hilo y el GC; `native` público llamado por vtable con `.mdn` presente; pack roto a mitad (#493); **pack XIP con la 4.5 en flash y Forms leyendo claves de flash tras el enlace**; el rechazo E-largo con los nombres más largos de la stdlib, byte-idéntico. Cada hallazgo → fila nueva de V4 o ficha; se repite hasta 0 hallazgos nuevos.

**V11 — Cierre.** Docs (MOD_FORMAT MOD9, OPCODES, HEAP_LAYOUT `+6`, BPVM_WIRE E0-E9, PHILOSOPHY criterio Java); FICHAS (#500/#514/#516/#523 con commit); stdlib MOD9 en las 4 copias; matriz V4 archivada como línea base del nuevo criterio.

#### Decisiones para Eduardo

- D1. ¿Firma completa con retorno TAMBIÉN en el nombre de funciones de módulo, factorías e inits (`Lib.f(i)i`, `__cls_new_Box(i)`, `__cls_init_Box(i)`) en vez de la cabeza pelada? Recomendación: SÍ — es lo que cierra el silencioso de #514 (`f(4)=100`) y la firma de la cabeza; sólo rompe consumidores MOD8 contra dueños MOD9, que se rechazan igualmente por política.
- D2. ¿Los tipos de clase van cualificados en la clave (`L<Lib.Base>;`) y las sintetizadas por módulo simples (`L<Object>;`)? Recomendación: SÍ — dos clases homónimas en módulos distintos dejan de confundirse; el coste es que renombrar un módulo obliga a recompilar a sus consumidores (ya ocurre con `Mod.f`).
- D3. Consumidor MOD9 contra dueño MOD7/8 (p.ej. /lib actualizado a medias): ¿rechazo nombrando el módulo («recompila Lib») o camino degradado por marcadores? Recomendación: RECHAZO — el degradado reabre los cuatro silenciosos medidos (cabeza, retorno, tipo de property, firma de función) y es un tercer camino que mantener y probar; la convivencia se documenta y `bpvm_mods` compara gen además de MAGIC.
- D5. ¿Se retiene la sección CLASES (4.5) en RAM durante todo el RUN en carga por stream (≈9 KB en un programa Gui; 0 en pack XIP) para la reflexión de Forms? Recomendación: SÍ — el neto sigue siendo −25 KB de RAM de plataforma por quitar los marcadores; la alternativa (releer del FS bajo demanda) mete I/O en `__methodRef`.
- D6. AOT fase 1: ¿gate GRUESO (el .mdn de un módulo se activa sólo si ningún módulo de su clausura de deps cambió de FORMA — `layoutCrc` de su 4.5, no `ifaceCrc`; crítico A3) o FINO (el .mdn sella módulo+ifaceCrc de cada clase cuya ranura hornea y de sus ancestros; formato v3)? Recomendación: GRUESO — 0 B de RAM, entra en el mismo commit que quita los marcadores, y los packs (autocontenidos) casi nunca lo disparan; el fino es fase 2 junto con los helpers `_ref` (ABI 7).
- D7. `instanceof Mod.Cls` cross-module (hoy revienta con traza Java): ¿entra en esta tanda con `INSTANCEOF_REF` + nombre cualificado en el parser? Recomendación: SÍ — «es útil para el programador» y cuesta un opcode que ya se está añadiendo.
- D8. `staticconst` inlineada en el consumidor (un cambio de valor en el dueño no se ve sin recompilar): ¿se documenta como `static final` de Java o pasa a getter (una llamada por acceso)? Recomendación: DOCUMENTAR — es exactamente lo que hace Java y no cuesta nada.
- D9. ¿Se retiran en el MISMO commit que los marcadores: el sidecar `.slots` (0 lectores), la regla 3 de #500 (EXTIENDE) para MOD9, y `ifaceCrc` como gate (pasa a informativo + gate del .mdn)? Recomendación: SÍ, los tres juntos — dejarlos vivos es estado en dos sitios.
- D10. El parque MOD8 de V6 con firmware V7 (§10): ¿validar los marcadores viejos contra la tabla 4.5 de la clase cerrada (0 RAM; el consumidor MOD8 queda igual de seguro que hoy, no menos) o rechazar y asumir que lo compilado en V6 se recompila? Recomendación: VALIDAR — es «soporte por parque instalado», y no reabre nada.

#### Tamaño del trabajo

**0. Línea base e instrumentación (0,5 sesión).** Cronómetro en las dos VMs + ciclos en la Pico, archivo de la matriz actual, tamaños. Se hace ANTES de tocar nada o no hay «no más lento».

**1. lexer-java + generador/ModWriter (5-7 sesiones; el trozo más grande, ≈1 500 líneas netas).** `claveCanonica` en un sitio con error por tipo sin resolver; bpi 9; importador sin numeración (borrar siembra/cruce/layout/externalSlot/.slots); ModWriter (refs 3.3 con backpatch, CLASES 4.5, sin marcadores/4.3/4.4, `endClass` doble); MivmEmitter 11 sitios `_REF` + `instanceof Mod.Cls` en el parser; `--barajar/--relleno`; tests reescritos (SlotsVtableCrossModule, SobrecargaCrossModule, CrossCheckSlots fuera, InterfazIdaYVuelta ampliado, guardián V1). Riesgo de calendario: tocar importador y emisor a la vez deja la red roja varias sesiones; commit por paso y el guardián V1 como semáforo.

**2. miVM (3-4 sesiones, ≈900 líneas).** Parser 3.3/4.5 (~150), `cls_reg` + arena + `+6` + `construir` + resolución + verificación E8 + mensajes E0-E9 (~450), `cls_parent/cls_method` + 11 opcodes + `getCSForDataAddr` O(1) + caché de ext-table (~250), reflexión por 4.5 (~40), cronómetro, Disasm. Tests JUnit del cargador (V3, 22 fixtures) antes de ejecutar ningún programa. Primer caso real: `Collections.OwnerList` bajo `Gui`.

**3. bpgenvm-c host (3-4 sesiones, ≈900 líneas).** El mismo algoritmo en C: loader.c (3.3, 4.5 flash-ptr/arena tras `next_free_address`, ext-table con 0xFFFFFFFF), link.c (registro, construir, resolver, verificar, mensajes, caché), interp.c (11 opcodes, helpers), builtins.c (reflexión, cb de :1310), events.c/threading.c/exceptions.c (helpers), bpvm_internal.h (`enlazado_tarde`, `cls_reg`), bpvm_mods.c (gen), **gate AOT fase 1 en el mismo commit** (~80 líneas: `g_after_link`, `bpvm_mdn_escanear`, nombre del símbolo en interp.c:299). 0 líneas en pico/esp32/stm32 (0 consumidores medidos). Paridad de stdout y de textos E0-E9 con mezcla.py.

**4. Firmwares (1-2 sesiones).** No hay código de placa que cambiar; el trabajo es la tanda: compilar las 5 familias, Pico 2 (stream, pack XIP, /lib a medias), P4 (Forms), Metro (Orm), lote cross-family. Ciclos del enlace en la Pico contra V0.

**5. stdlib y docs (1,5 sesiones).** Rebuild MOD9 (`rm -rf bpstdlib/out`), las 4 copias (device, IDE, bpstdlib, ZIP); MOD_FORMAT §MOD9, OPCODES, HEAP_LAYOUT (`+6`), BPVM_WIRE (E0-E9), PHILOSOPHY (criterio Java), FICHAS/ESTADO.

**6. Validación (3-4 sesiones).** V4 (≈250 filas con predicción escrita antes), V5/V6/V7, V8 en placas, V9 medidas, V10 adversarial hasta 0 hallazgos.

**Total ≈ 17-23 sesiones**, en cascada Java → C → Pico, con commit por paso y `dist/v6` como oráculo. Fuera de esta tanda (fase 2, sólo si la medida lo pide): vtable plana de 8 B, hash FNV en claves/símbolos, helpers AOT `_ref` con ABI 7 y gate fino.


## H1 — el módulo raíz: el mapa de lo que hay y el plan (20-sep, día de análisis)

> La idea es de Eduardo (5-sep, de Turbo Pascal): *«el módulo del sistema se escribía en Pascal mismo, se compilaba
> con un comando especial para indicar que era el módulo raíz. Eso permitía tener todos los tipos definidos y las
> funciones intrínsecas. Me da más tranquilidad tener algo que se pueda LEER que algo que sólo existe en memoria. Y
> esto también es importante: un solo archivo, sin posibilidad de diferencias entre compilador y VM.»*

### 1. Dónde vive hoy cada cosa (medido el 20-sep)

Las **240 intrínsecas** (ids 0..239) están repartidas en **cinco sitios mantenidos a mano**:

| qué | dónde | cuántas | cómo se atan |
|---|---|---|---|
| el **id** (Java) | `miVM/…/bytecode/Builtin.java`, un `enum`: `STRLEN("strlen")` … el id es el `ordinal()` | 240 | el nombre BP (`bpName`) y el orden |
| el **id** (C) | `bpgenvm-c/src/builtins.c:41-400`, `enum { BUILTIN_STRLEN = 0, … }` escrito a mano, y el `switch` del despacho | 236 (faltan 4 sólo-host: `HEAP_FRAG`, `HEAP_MAP`, `INPUT`, `LIST_DIR`) | por el número; `scripts/check_builtins.py` (`#476`) es el andamio que compara las dos tablas |
| la **firma** de las globales sin cualificar (`strlen`, `abs`, `print`…) y de las internas `__x` que usa la stdlib | `SemanticAnalyzer.java`, 65 `addBuiltin(s, "nombre", ret, params, tipos)` | 65 | el emisor las emite con `Builtin.byName(fs.name)` (`MivmEmitter.java:4563`) |
| la **firma** de las intrínsecas de módulo | los `.bp` de la stdlib: `public intrinsic function sin(x: float): float` (Gui 84, Math 20, Machine 17, IO 15, Net 5, Gpio 5, App 4, I2c 3, Adc 2, Neopixel 2…) | ~175 | viajan en la interfaz del `.mod` sin código |
| el **enlace** nombre → id de esas intrínsecas de módulo | `lexer-java/…/Intrinsics.java`, 162 `register("Math.sin", w -> emitBuiltin(w, Builtin.SIN))` (algunas emiten una secuencia, no un solo builtin) | 162 | por el nombre cualificado; una `intrinsic` declarada sin `register` **compila y devuelve basura** (hueco conocido) |

Y los **tipos**: `integer`/`float`/… son un `case` de un `switch` en `resolveType` (`SemanticAnalyzer.java:1574` y
`:3828`, dos copias); `Object`, `Thread`, `Mutex` y `StringBuilder` se construyen a mano con 20 `makeMethod(...)`
(`SemanticAnalyzer.java:423-529`) + sus layouts en `ModuleInterface.objectBaseLayout/threadBaseLayout` (la fuente
única desde `#526`) + los métodos sintetizados en `MivmEmitter`; `Integer`, `Comparable`, `Exception`,
`RuntimeError`… son fuente en `bpstdlib/Core.bp`. La frontera «escrito en BP» / «cableado en Java» no responde a
ningún criterio: es lo que se pudo expresar en cada momento.

### 2. Lo que se quiere: UN fichero, y que el compilador y las VMs beban de él

`Core.bp` es «el módulo del sistema». El fichero legible es **`Core.bp`, compilado como raíz** (una marca: `--raiz`,
o el propio `module Core`): sus símbolos públicos son visibles **sin cualificar** en todo programa (como la unidad
`System`), y contiene:

1. **Las intrínsecas**, una línea cada una, con su firma BP y su id: `intrinsic function strlen(s: string): integer`.
   El id es el **orden de declaración** (como hoy el `ordinal()`), y el generador se niega a mover un id existente
   salvo que se le diga (errores sí, silenciosos no). Las de módulo (`Math.sin`) se declaran en su módulo como hoy y
   se atan a la raíz **por declaración**, no en `Intrinsics.java`: `public intrinsic function sin(x: float): float
   := Core.__sin` (o la forma que decida Eduardo). Una `intrinsic` sin atar es error de compilación.
2. **Los tipos del lenguaje** que hoy son Java: `Object`, `Thread`, `Mutex`, `StringBuilder` declarados en BP (sus
   métodos `intrinsic`, su layout el de siempre) — se quitan los 20 `makeMethod` y las tres copias de tablas.
3. De él se **generan** `Builtin.java` y el `enum` de `builtins.c` (y `check_builtins.py` se tira). El despacho de
   las dos VMs (los `switch`) sigue escrito a mano: es la implementación, no la tabla.

Lo que cuesta: `Core.mod` crece (≈240 firmas más en su interfaz; «declarar no cuesta RAM»: el cargador la salta) y va
embebido en las cinco imágenes → al final de H1, la cascada de siempre (stdlib, blobs, imágenes; sin grabar hasta que
toque).

### 3. El plan, por pasos que CIERRAN algo cada uno

| paso | qué | verifica |
|---|---|---|
| **P1** ✅ (20-sep) | **Cambió de forma con la decisión de Eduardo de la mañana**: una intrínseca es una función normal cuyo cuerpo es el bytecode escrito —`inline CALL_BUILTIN 24 … end sin`, el `inline($CD/$21)` de Turbo Pascal— y no hay que «atar» nada: el número ES el id, viaja en la interfaz del `.mod` (`func … inline …`) y el consumidor lo inlina. Hecho: la palabra `inline` (entra ella, sale `intrinsic`), parser/semántico/emisor/interfaz/importador; la stdlib entera migrada (177 funciones en 17 módulos, con `scripts/migrar_intrinsic_a_inline.py`, una vez); `Intrinsics.java` (162 `register`) borrado; `scripts/gen_builtins.py` comprueba los ids de los cuerpos contra `Builtin.java` (será el generador). Quedan en el compilador los 65 `addBuiltin` (las globales sin cualificar): son P2 | check 83/59/9/13/83, mezcla 194, frontend 220 |
| **P2** ✅ (20-sep, mediodía) | `Core.bp` lleva la sección «LAS PRIMITIVAS DE LA VM»: las 240, una por id, con su firma y su cuerpo (`inline CALL_BUILTIN n`, o el opcode directo para `newByteArray`…); las 64 globales son `public` y `Main.cargarRaiz` las registra en el scope global al arrancar cada compilación (Core.mod se localiza como un import; sin él, aviso ruidoso); las 176 restantes son la tabla (privadas; su tipo ajeno —`Component`— es `any`, que sólo la raíz puede escribir). Fuera los 65 `addBuiltin` salvo los 4 casts estrechos (`byte()`… son palabras reservadas: no se pueden declarar en BP). `gen_builtins.py --escribir` GENERA `Builtin.java` y `bpgenvm-c/src/builtin_ids.h` (que `builtins.c` incluye; sus notas se conservan en un `#if 0`); las 5 constantes que no seguían la regla nombre→CONSTANTE renombradas; `check_builtins.py` (el andamio de `#476`) borrado. Manual §10.0 | check + mezcla + frontend 220 + miVM 52 |
| **P2b** → **H5** (Eduardo, 24-sep: es del AOT; apuntado el 20-sep a pregunta suya) | el AOT y las intrínsecas: hoy traduce los builtins globales POR NOMBRE con una lista a mano (`AotCEmitter.emitBuiltinCall`) y rechaza `Math.sin(x)` desde una `native` («no soportado en native todavía»); `inline` no lo cambia. Lo que abre: como el compilador ya sabe el id (`CALL_BUILTIN n`), el AOT puede emitir para CUALQUIER intrínseca una llamada genérica al despachador de builtins de la VM (`vm->aot_helpers->call_builtin(vm, n, …)`, helper nuevo en la tabla del `.mdn` → sube `MDN_ABI_VERSION`) y tirar la lista a mano | `test-xmodnat` + un sample con `Math.sin` en una `native`, host y Pico |
| **P2c** ✅ 20-sep (idea de Eduardo) | sobrecargar intrínsecas: ahora que el cuerpo es `inline`, `sign(x: integer)`/`sign(x: float)` son dos funciones normales sobrecargadas (fuera `signF`), y `len(s: string)`/`len(a: integer[])`… podría dejar de ser un caso especial del semántico — con el matiz de que hoy `len` es «dirigida por tipo» (cualquier array, y clases con `length()`/`size()`), y sobrecargar por cada tipo de elemento pide un tipo «array de lo que sea» que no existe. **Hecho**: `sign` ×2 en Math.bp (fuera `signF`); `len(s: string)` / `len(a: any[])` en Core.bp, con `any[]` = «cualquier array» (una línea en `ArrayType.isAssignableFrom`, sólo la raíz lo escribe) y el caso especial fuera del semántico y del emisor; `registrarGlobalDeRaiz` encadena sobrecargas como el importador. Lo que se pierde: `len(obj)` sobre una clase con `length()`/`size()` (azúcar que se saltaba la visibilidad): es `obj.length()`; sólo `LenTest` lo usaba | Math con `sign` sobrecargado; `mathtest`; `LenTest`, `LocalArrTest`, `StrOps348` |
| **P3** ✅ 20-sep (tarde) | `Object`/`Thread`/`Mutex`/`StringBuilder` escritas en BP en `Core.bp` (`public root class Object`/`Thread`; Mutex y StringBuilder normales, con el orden de campos y métodos que las VMs dan por hecho); `import Core` lo pone el PARSER en todo módulo (decisión de Eduardo, §5); fuera los 20 `makeMethod`, las cuatro síntesis del emisor (~1000 líneas de Java escribiendo bytecode), las tablas de layout y el sembrado del importador; `ClassSymbol.isBuiltin` ya no existe. Toda clase sin `extends` cuelga de `Core.Object` → en MOD9 todas son abiertas (el cargador las construye). La stdlib adelgaza 25,5 KB (Str.mod 5715→4641). Dos bugs por el camino: el importador copiaba las ranuras de una base LOCAL aún diferida (tabla vacía → `Core.Integer#value#0` en MOD8) y `registrarGlobalDeRaiz` ya no existe (las globales de Core entran por analizador desde el import). Mezcla: una Lib MOD8 arrastra su Core MOD8, y la app MOD9 lo rechaza con nombre («otra interfaz») | check 83/59/9/13/83, mezcla 194, frontend 220, errores 22, miVM 52, tests C |
| **P4** ⛔ **cancelado** (Eduardo, 24-sep: «OK» a la propuesta del 20-sep) | los primitivos (`integer`, `long`, `byte`…) son palabras reservadas del lexer: no hay forma de DECLARARLOS en BP sin inventar una sintaxis (`type integer`) que no aportaría nada —no tienen métodos ni layout que leer— y dejaría los 4 casts estrechos (`byte(i)`…) igual de a mano. El `switch` de `resolveType` es la tabla y es legible. Se deja como está salvo que Eduardo lo vea de otra forma | — |
| **P5** — en marcha (20-sep, tarde) | la cascada: stdlib ✅, `bpdevices` ✅, blobs ✅, las siete imágenes construidas ✅ (ninguna grabada); manual ✅ (§10.0) | check ✅ + placas cuando Eduardo diga |

Después, encima del módulo raíz: sobrecarga de intrínsecas (`sign`/`signF`), destructor `~Clase()`, `Map`, `#396`
`Time`, `#19`, `#460`, los huecos.

### 4. Lo decidido (Eduardo, 20-sep, 8:00)

*«Una función intrínseca se declara como una función normal, por ejemplo `public function abs(d: double): double`;
la diferencia es el cuerpo de la función, utilizamos `inline opCode`. Y al final `end abs`. Es la sintaxis que
utilizaba Turbo Pascal.»* Con eso las dos preguntas de arriba desaparecen: no hay que atar nada (el bytecode va en
el cuerpo y viaja en la interfaz), y los nombres internos se quedan como están donde están (cada módulo declara
las suyas). Pendiente para P2: los nombres `__x` de `Core.bp` sólo los puede declarar la raíz (`M4`).

### 5. P3 — las clases del lenguaje en BP: el mapa y las dos decisiones (20-sep, tarde)

**Lo que hay.** `Object`, `Thread`, `Mutex` y `StringBuilder` viven en TRES sitios a mano, y además se COPIAN en
cada `.mod`:

- el semántico las da de alta con 20 `makeMethod` (`SemanticAnalyzer.builtinScope`, líneas 255-370): símbolos
  «fake» sin AST, `isBuiltin = true`;
- el emisor las **sintetiza en cada módulo** (`synthesizeObjectClass`/`StringBuilderClass`/`ThreadClass`/
  `MutexClass`, ~900 líneas de Java que escriben bytecode a mano): `Str.mod` —que no usa hilos— lleva las cuatro
  clases (`class Object padre=raiz`, `StringBuilder`, `Thread`, `Mutex` + las tuplas), ≈1,3 KB de código y
  descriptores por módulo, ×28 en la stdlib y en cada app;
- las tablas de layout (`ModuleInterface.objectBaseLayout`/`threadBaseLayout`/`builtinBaseMethodNames`) y el
  sembrado del importador (`Main.sembrarRaizBuiltin`) repiten el orden de campos y métodos para que los slots
  cuadren (la tercera copia de la tabla de Thread se desincronizó el 19-sep, `#526`).

Las dos VMs sólo dependen de **convenciones de layout**: `THREAD_START` lee el tid en el campo 0 y el `stackSize`
en el 1, y arranca `run()` en la **ranura 0 de la vtable**; `MUTEX_LOCK/UNLOCK` leen `__mid` en el campo 0;
`StringBuilder` es BP puro (`chars`/`size`/`cap` + `__charsToString`). Es lo que hace posible P3 sin tocar la VM:
`Core.bp` las declara con **el mismo orden** de campos y métodos y las convenciones siguen valiendo — y ahora se
LEEN en un fichero en vez de estar repartidas en Java.

**Lo que se quiere.** Las cuatro clases escritas en `Core.bp` (sus primitivas ya están: `__threadStart`,
`__threadJoin`, `__mutexCreate/Lock/Unlock`, `__charsToString`…), compiladas UNA vez en `Core.mod`, y todo módulo
las referencia por enlace tardío (`#523`), como ya hace con `List` desde `#450`. Fuera los `makeMethod`, las
síntesis del emisor, las tablas de layout y el sembrado: **una fuente**. Cada `.mod` adelgaza ≈1,3 KB; toda clase de
usuario pasa a ser «abierta» (su padre, `Core.Object`, es externo) y la construye el cargador — es lo que `#523`
diseñó; el coste de carga en la Pico se mide en P5.

**Decisión 1 — `Thread` no desciende de `Object`** (así es hoy, `#526`; por la ranura 0 = `run()`). ¿Cómo se
escribe eso en BP? (a) **regla de la raíz: en `Core.bp` una clase sin `extends` no tiene padre** — es la raíz la
que declara de quién se hereda; `Object` y `Thread` no llevan `extends`, y `Exception`, `Comparable`, `List`
escriben `extends Object` (tres líneas); fuera de la raíz sigue el implícito de siempre. La VM no cambia. (b) que
`Thread` descienda de `Object` y las dos VMs arranquen `run()` por su nombre (o ranura 2) según la gen: tocar VM,
gen 8 y cascada. (c) una sintaxis nueva (`extends none`). Recomiendo (a): cero VM, explícito en el único fichero que
puede decirlo, y el «salvo Thread» del modelo de objetos se lee en `Core.bp`.

**Decisión 2 — ¿se ven sin `import Core`?** Hoy `Thread`/`Mutex`/`StringBuilder` están en todo programa (24 samples
+ 6 de la VM-C + 1 de la stdlib los usan sin importar Core) y `Object` es el padre implícito de toda clase. La norma
de `#458` (30-ago) dice «si se usa un tipo de Core, se importa Core». Opciones: (i) **todo lo público de la raíz se
ve sin cualificar, funciones Y clases** —la unidad `System` entera, como P2 hizo con las funciones; `import Core`
pasa a ser redundante (no error) y `#458` queda como historia (su causa técnica, «las dos pasadas veían cosas
distintas», ya no existe: `cargarRaiz` corre en las dos); (ii) sólo las cuatro «del lenguaje» globales y el resto
(`List`, `Exception`, `Integer`…) por import: dos clases de público; (iii) las cuatro por import como cualquier tipo
de Core: 31 ficheros a tocar y toda clase obligaría a `import Core` por `Object` (o `Object` especial). Recomiendo
(i): una regla, y es la de Turbo Pascal.

**Lo que NO cambia:** la sintaxis del usuario; el orden de slots (`run` 0, `tid` 0…); los `.mod` viejos (la cápsula
V6 lleva sus copias y las VMs no distinguen); MOD8 (el padre externo va por placeholders, como `List` hoy).

### 6. `#19` — los arrays fijos, redefinidos (Eduardo, 21-sep)

*«Vamos a hacer una cosa, mantenemos los array fijos pero los limitamos a declaraciones locales y dentro de una clase
como private. No utilizamos referencias con ellos.»* Lo que se sigue de ahí:

- **Un array fijo `tipo[N]` NO es una referencia.** Es almacenamiento en sitio: en el marco de la función (local) o
  dentro del objeto (campo `private`). Sólo se indexa (`buf[i]`, con comprobación de rango en la VM) y se mide
  (`len(buf)`). Cualquier otro uso —asignarlo, pasarlo, devolverlo, imprimirlo— es error de compilación: no hay
  referencia que dar, y así no puede escapar (el UAF que #19 arrastraba desde V4 muere por definición).
- **Dónde**: locales y `private var` de clase. A nivel de módulo, fuera (hasta hoy se horneaba en el data block:
  una tabla en flash; quien la quiera escribe `tipo[] := [...]` o `newIntArray`). Elementos primitivos.
- **Cómo, sin tocar el modelo**: el marco lleva `[len][N elems]` y `LEA_LOCAL` (que existía desde H1.2a sin que el
  frontend lo emitiera) empuja su dirección como ref sin TAG — que las dos VMs ya resuelven como identidad (`bpref_deref`:
  «sin TAG = null o constante del data block»)— y `ALOAD/ASTORE_w` hacen el resto con su comprobación de rango. Para
  el campo, UN opcode nuevo, `LEA_FIELD slot` (la dirección del campo dentro del objeto; el GC no mueve objetos y la
  dirección sólo vive dentro de la expresión). `len(buf)` va por `len(any[])` de la raíz: `ALEN` lee la cabecera.
- **Coste**: el marco crece `4 + N·ancho` bytes por local fijo; el objeto, `1 + ⌈N·ancho/4⌉` ranuras por campo.
  El .mod no crece con N (los elementos se ponen a cero con un bucle, no con N stores).

**Lo decidido (Eduardo, 20-sep, tarde):** *«'root' o algo similar para indicar que no desciende de nada. Import
'core' se añade al árbol AST si no está incluido ya en módulo, así que se puede indicar o omitir, pero en el AST se
incluye siempre salvo en 'Core'.»* → `public root class Object` / `public root class Thread` (palabra contextual,
sólo en la raíz); el parser añade `import Core` al final de los imports de todo módulo que no sea Core. Con eso la
decisión 2 se contesta sola: todo lo público de Core se ve sin cualificar (clases y funciones), `import Core` es
redundante y no error. Consecuencia medida: un módulo sin clases también depende de Core al cargar (`Core.__init`),
como la unidad System. En MOD8 una clase con base externa necesita el layout de bpi 8: compilar MOD8 exige un Core
MOD8 (los tests y `mezcla.py` se lo fabrican).


### 7. `#540` — la asignación definida de las locales (24-sep, decidido por Eduardo)

**La regla** (Eduardo): *«Las variables globales siempre se inicializan ya que van en el bloque de constantes del
módulo. Las variables locales si no se inicializan su valor es indeterminado. En teoría utilizar una variable local
sin inicializar es un error.»* Y a la pregunta de error o aviso: **error de compilación**, como Java. Lo que se
detecta: leer una local declarada SIN inicializador (`var x: T`) en un punto al que se puede llegar sin haberle
asignado valor por algún camino. No cuentan los arrays fijos (`var b: byte[64]`: se ponen a cero al declararlos),
los parámetros, el iterador de un `for`, la variable de un `catch` ni la de una rama de `parallel`.

**Cómo** — una pasada aparte, `AsignacionDefinida`, al final del análisis de cada cuerpo (`analyzeFunction`, y los
`get`/`set` de `analyzeProperty`), sobre el AST ya anotado: el uso se casa con su declaración por el `VarSymbol` de
`exprSymbols` (su nombre, línea y columna son los del `DeclName`: `declSymbols` sólo guarda el primero de `var x, y`).
El estado es el conjunto de vigiladas CON valor en ese punto, más «este punto no se alcanza» (tras `return`,
`throw`, `break`, `continue`), que vale como «todas» al cruzar caminos. En BP asignar es una SENTENCIA, nunca una
expresión: las expresiones sólo leen, y eso ahorra la mitad de las reglas de Java (§16 de la JLS). Los caminos:
- `if`/`elseif`/`else`: la intersección de las ramas; sin `else`, la rama vacía cuenta.
- `switch`: la de los `case`, el `default` (o, sin él, el camino que no entra) y los `break` de dentro — en BP un
  `case` no cae al siguiente y `break` sale del `switch`.
- `while`/`for`: después, lo de antes (el cuerpo puede no ejecutarse); `while true`, sólo por sus `break`.
- `do … loop`: el cuerpo corre al menos una vez: después, el final del cuerpo con sus `continue`, y los `break`.
- `try`: el cuerpo, y cada `catch` desde el estado de ANTES (la excepción puede saltar en cualquier punto); con
  `finally`, además lo que el `finally` asigna.
- `parallel`: cada rama es un cuerpo aparte (no ve las locales de quien la lanza); el `default`, un bloque normal.
El error se dice una vez por variable, en el primer uso: «la variable local 'x' se usa sin tener valor…».

## H3 — ficheros: el cursor y la clase `File` (21-sep, día de análisis)

Lo que hay que saber antes de tocar nada, medido hoy, y el diseño que sale de las decisiones que ya
estaban tomadas (`V6_IDEAS.md` 17-ago, `#491` 10-sep) más la de Eduardo de hoy: *«una clase `File`, y
que dentro estuvieran los buffers y el handle; a partir de ahí todo debería ser bastante sencillo»*.

### 1. Lo que hay hoy, medido

| | dónde | qué |
|---|---|---|
| **Superficie BP** | `Core.bp` (38-42, 1075-1080) + `IO.bp` | fichero ENTERO: `readFile`/`writeFile`/`appendFile`/`fileExists`/`readFileBytes`/`writeFileBytes`; rutas y directorio en `IO`. **Ningún descriptor**: no hay dónde colgar una posición |
| **La fachada C** (`bpvm_fs.h`) | 17 ops **por ruta**; `read_at`/`write_at`/`truncate` en las tres cinturas (littlefs, FAT, host), rodadas por SQLite; `read_stream` y `crc32` sólo donde se cronometró (por llamador, no por causa: la enfermedad de `#398`/`#453`/`#473`) |
| **littlefs** (`fs_lfs.c:86`) | *«CERO malloc en las ops»*: **dos** buffers estáticos de 256 B → hoy caben **dos** ficheros abiertos, y sólo dentro de una op (cada op abre y cierra) |
| **FatFs** | `FF_FS_LOCK 0` (sin control de aperturas duplicadas), `FF_FS_TINY 0` |
| **Coste de un fichero abierto en el micro** (ARM, medido con `-mcpu=cortex-m33`) | littlefs `lfs_file_t` **84 B + caché 256 B = 340 B**; FatFs `FIL` **552 B** (lleva dentro el sector de 512); host `FILE*` 4-8 B |
| **miVM** | `Files.readAllBytes(sandboxPath(...))`, y `packResource(path)` antes del FS (`#310`/`#520`): el cursor sobre un recurso de pack es un `byte[]` en memoria |
| **El GC** (`heap.c:661`) | `gc_mark_phase` → `bpvm_handles_gc_sweep` (recorre la tabla: slot vivo sin marca → muerto) → `gc_sweep_phase`. **Entre la marca y el barrido** el bloque muerto sigue intacto: es el sitio del gancho. miVM tiene el espejo (`gcLocked`, «barrido de tabla, paso 6») |
| **El destructor** | ya existe (H1, 21-sep): `~Clase()` corre al morir un `var owner` / `try var`. Lo que `#491` llamaba «quinta capa» (que el GC avise) sigue sin existir |
| **Los restos de la fila** | `listDir` sigue sin estar en la VM-C (sólo el id 42 en `builtin_ids.h`); `SD_INFO`/`SD_MOUNT` siguen sólo en `pico/repl_v1.c`; `/sd` sin tarjeta cae a la flash; `BPVM_FS_PATH_MAX` sólo en C |

**El precedente que Eduardo nombra existe y hace justo esto**: MicroPython (`vfs_lfs`) y el `File` de
Arduino-LittleFS guardan `lfs_file_t` + `lfs_file_config` + **el buffer de caché dentro del objeto
fichero** y abren con `lfs_file_opencfg`. Sin tabla de descriptores, sin límite inventado: el límite es
la memoria. Es la forma de la propuesta del 10-sep, ya en producción en otros sitios.

### 2. El diseño

**Un fichero abierto es un objeto BP y TODO su estado vive en él** (`#491`: *«si una parte se queda en
un `FIL` estático del driver, para esa parte la objeción original sigue viva»*):

```
public class File                                   // BYTES, y nada más
  private var st: byte[]                            // el estado del driver: [cabecera de la fachada][lfs_file_t + caché | FIL | FILE*]
  private var ruta: string
  public function File(path: string, mode: string)  // "r" | "w" | "a" | "r+"  (abre; si no puede, RuntimeError con el porqué)
  public function isOpen(): boolean
  public function close()                           // idempotente
  public function ~File()                           // = close()  (H1: corre al morir un owner / try var)
  public function size(): integer
  public function tell(): integer
  public function seek(pos: integer)                // absoluto, 0..size(); fuera → RuntimeError
  public function skip(delta: integer)              // relativo (negativo vale); fuera → RuntimeError
  public function eof(): boolean                    // tell() >= size()
  public function readByte(): integer               // -1 al final
  public function read(buf: byte[]): integer        // rellena buf; devuelve leídos (0 = fin). SIN reservar memoria por llamada
  public function readBytes(n: integer): byte[]     // los que haya (más corto al final)
  public function write(b: byte[])                  // crudo, siempre
  public function writeByte(n: integer)
  public function flush()
end File

public class TextFile extends File                  // UTF-8 encima; AÑADE sobrecargas, no redefine (regla del 17-ago)
  public function readLine(): string                // sin terminador; tolera "\r\n"; "" al final (lo dice eof())
  public function readStr(n: integer): string       // n CARACTERES, no bytes
  public function write(s: string)                  // sobrecarga NUEVA (la base no tiene write(string))
  public function writeLine(s: string)              // s + "\n" (uno solo, igual en las dos VMs)
end TextFile
```

- `st` lo reserva el constructor con `newByteArray(__fstateSize())`: **0 bytes en miVM** (el estado es
  un `RandomAccessFile` de Java, en un mapa por handle), 340/552 B en el micro. El coste es honesto y
  visible: sale en el guardián de fin de RUN (`#339`) como cualquier otro bloque.
- **El heap no compacta** (`heap.c:119`): el driver puede guardar el puntero al bloque entre llamadas.
  Y la memoria por handles (V4) hace que un `st` rancio **grite** en vez de corromper.
- **Las primitivas** (ids nuevos en `Core.bp`, 8): `__fstateSize()`, `__fopen(st, path, modo)`,
  `__fclose(st)`, `__fread(st, dst, off, n)`, `__fwrite(st, src, off, n)`, `__fseek(st, pos)`,
  `__fsize(st)`, `__fflush(st)`. `tell`/`eof` se calculan en BP sobre la cabecera. Cada una toma el
  lock grueso del FS, como toda op de hoy.
- **Abajo**: la fachada gana `open/close/read/write/seek/size/flush` sobre un bloque de estado, con el
  tamaño del bloque como constante de cada cintura; el **overlay de packs** (`#310`) se sirve con la
  cabecera sola (posición + `ov_read` a offsets: es memoria XIP, no hay descriptor que abrir). Y con
  el descriptor debajo, `bpvm_fs_read_stream` pasa a ser genérico → **FAT gana el streaming** y caen
  el punto 4 de `#473` y `#475` sin tocar el wire.

**El registro de ficheros abiertos — enhebrado por los propios bloques, sin tabla ni tope**: la
cabecera de `st` lleva `next` (la ref del siguiente abierto) y la VM guarda la cabeza. Tres clientes:

1. **El GC** (entre la marca y el barrido, en las dos VMs): un `st` sin marca es un fichero cuyo objeto
   murió sin `close()` → se cierra en nativo (no asigna, no llama a BP, idempotente: las tres
   condiciones de `#491`) y se desengancha. La quinta capa, en ~30 líneas por VM.
2. **El fin de RUN**: cierra lo que quede y **lo dice** por el canal del guardián (`⚠ 2 ficheros
   quedaron abiertos: /sd/log.txt (w) …`), en vez de callar.
3. **Aperturas duplicadas**: al abrir se recorre la lista; la misma ruta ya abierta y alguna de las
   dos con escritura → RuntimeError *«ya está abierto»*. N lectores, sí. Igual en las dos VMs (FatFs
   con `FF_FS_LOCK 0` no lo vería; littlefs asserta: mejor que lo diga la fachada con texto).

**Paridad**: todo lo observable (`stdout`, el contenido del fichero) va por `close()`/`~File()`/`flush()`;
el camino del GC es sólo la red para lo anormal, y cerrar sin escrituras pendientes es invisible.

### 3. Las preguntas del 17-ago, contestadas (propuesta; Eduardo decide)

| # | pregunta | propuesta |
|---|---|---|
| 1 | dónde viven las clases | **`IO.File` / `IO.TextFile`** en `IO.bp` (donde ya están rutas y directorios): Core es el lenguaje, un fichero es biblioteca. La alternativa es Core (sin `import`, como `readFile`) |
| 2 | modo de apertura | cuatro strings, los de C que todo el mundo conoce: `"r"` (existe, sólo lectura) · `"w"` (crea/trunca, lectura y escritura) · `"a"` (crea, cursor al final) · `"r+"` (existe, lectura y escritura). Otro texto → RuntimeError en BP, mismo mensaje en las dos VMs |
| 3 | `seek`: origen y fuera de rango | `seek(pos)` absoluto + `skip(delta)` relativo; **fuera de `[0, size]` es error**. Extender sólo escribiendo al final → no hay «hueco» que rellenar (FatFs y littlefs rellenan distinto: así ni se pregunta) |
| 4 | `readLine` al final | devuelve `""` y `eof()` es quien lo dice: `while not f.eof() do … f.readLine()`. Un fichero sin `\n` final da su última línea igual |
| 5 | endianness / ancho de `write(n)` | en `File` sólo hay **bytes** (`readByte`/`writeByte`/`byte[]`). Si hace falta multibyte: `readInt`/`writeInt` (4 B) **little-endian** —la endianness de todos los micros del sobre— escritos en BP sobre `readBytes`, sin primitivas. Propongo dejarlos fuera de V7 salvo que los quieras |
| 6 | `TextFile.print(n)` | **cae**: `print` es sentencia del lenguaje, no puede ser método. Lo formateado se concatena (`f.writeLine("t=" + t)`), que es lo que ya hace todo BP |
| 7 | ¿qué pasa si no se cierra? | tres redes, en este orden: `close()`/`~File()` (H1) → el GC cierra lo inalcanzable → el fin de RUN cierra y **cuenta** |
| 8 | ¿cuántos a la vez? | **sin tope**: el límite es el heap (340/552 B cada uno) y se ve. En el PC, el del SO |

### 4. El plan, por pasos que cierran algo

| paso | qué | cierra |
|---|---|---|
| **P1** | la fachada C con descriptor sobre bloque de estado: littlefs, FAT, host, overlay; `read_stream` genérico; test C (`test-fs-cursor`) | el punto 4 de `#473`, `#475` |
| **P2** | las 8 primitivas en las dos VMs; el registro enhebrado; GC y fin de RUN; sample mínimo de paridad | la quinta capa |
| **P3** | `File`/`TextFile` en `IO.bp` con `~File()`; samples (`FileCursor`, `TextFileDemo`, `errores/FileMal`); manual, referencia es/en, `TARJETA_SD.md` | `#491` |
| **P4** | los restos de la fila: `/sd` reservado (sin tarjeta, error); `listDir` en la VM-C; `#456` (tope de path también en miVM + sample); `SD_INFO`/`SD_MOUNT` al REPL común; la SD del STM32 (la DK2 tiene ranura, el Nucleo no) — último, y sólo con placa | la fila H3 |
| **P5** | la cascada: blobs, siete imágenes, placas | — |

### 5. Lo decidido (Eduardo, 21-sep)

OK a las ocho propuestas de §3 y al plan de §4, con una precisión: *«`TextFile` extiende `File` y le añade un
encoder/decoder de UTF-8 ↔ bytes»* — en BP son `toBytes`/`fromBytes` (Core 119/120) sobre la capa de bytes de `File`;
`readLine`/`readStr` leen por trozos con `read(buf)` y devuelven al cursor lo que sobra con `skip`, así que `tell()`
sigue diciendo la verdad. P1-P4 hechos el mismo día (ver `ESTADO`); P5 va con la cascada pendiente de H1.

### 6. La medida: fichero entero contra cursor (21-sep, `samples/FileBench.bp`)

Eduardo: *«¿has hecho una comparación de rendimiento entre el sistema anterior y el nuevo?»*. No la había; va
aquí. 400 líneas de texto y 32 KB binarios, «como se hacía» (fichero entero: `appendFile` por línea,
`readFile`+`split`, `readFileBytes`) contra «como se hace ahora» (`IO.TextFile`/`IO.File`), en ms:

| caso | Pico (littlefs) | P4 (littlefs) | P4 (`/sd`, SDMMC) | host VM-C |
|---|---|---|---|---|
| 1. escribir 400 líneas: `appendFile` / `writeLine` | **19950 / 316** | **15316 / 180** | 901 / 100 | 36 / 2 |
| 2. leer 400 líneas: `readFile`+`split` / `readLine` | 23 / 87 | 14 / 80 | 9 / 71 | 0 / 2 |
| 3. leer 32 KB: `readFileBytes` / `File.read` ×512 | 5 / 11 | 13 / 21 | 7 / 20 | 11 / 1 |
| 4. 100 bytes dispersos: entero / `seek`+`readByte` | 11 / 17 | 14 / 33 | 7 / 32 | 0 / 0 |

Lo que dice: **escribir por líneas es 60-70× más rápido con el cursor en littlefs** (cada `appendFile` es
abrir + commit + cerrar: ~50 ms en la Pico) y 9× en la SD. Leer es otra historia: el fichero entero gana
en tiempo (3-8× en líneas, 2× por trozos), porque `readFile`+`split` es nativo de punta a punta y el cursor
paga una primitiva por operación — **pero exige tener el fichero ENTERO en el heap**, y eso es lo que el
cursor viene a quitar (con 64 KB × 3 copias la miVM se quedó sin heap en la primera versión del bench).
`readLine` salió primero en BP (trozos de 32 + `skip`): **354 ms en la Pico, 15× peor que `split`**; pasó a
nativo (`__freadLine`, dos pasadas sobre el cursor del motor) y quedó en 87 (0,2 ms por línea). Queda margen
—una pasada con la zona de rascar ahorraría el seek de vuelta— si algún día hace falta.


### 7. La SD al común (24-sep, noche) — lo que quedaba de la fila, y lo que le dejó `G4`

**Lo que hay, medido** (24-sep):

| placa | ranura | el medio | `/sd` al arrancar | `SD_INFO`/`SD_MOUNT` |
|---|---|---|---|---|
| Metro (RP2350) | sí | SPI: `bpvm_sd.c` sobre la HAL (`bpvm_spi_*`) y el árbitro del bus | sí | sí, en `pico/repl_v1.c` |
| P4 | sí | SDMMC: `esp32p4/main/blk_sdmmc_p4.c` | sí | **no** |
| C5 | sí, en el SPI 2 de la pantalla (`sd=bus:2,cs:23`, de `G4.a1`) | — (`G4.a3` le habló desde BP: `C5Sd.bp`) | no | no |
| C6 | sí (Waveshare LCD-1.3; los pines, en el esquemático de `P2.0`) | — | no | no |
| Pico 2, S3 DevKitC, C3, Nucleo, DK2 | no (la DK2: Eduardo, 24-sep) | | | |

Lo de debajo ya es común: el contrato de bloque (`bpvm_blk_backend_t`), FAT (`fs_fat.c`, `bpvm_fs_fat_montar`) y el
protocolo SPI (`bpvm_sd.c`, sin SDK: la HAL BP y el árbitro de `G4`). Las dos placas que montan hacen lo mismo —la línea
`sd` del ENV (variante SPI o SDIO), su dispositivo de bloque, `bpvm_fs_fat_montar(dev, "/sd")`—; sólo cambia el
dispositivo. Lo que no es común son los dos verbos, escritos para el SPI de la Metro. Y `G4` dejó aquí dos cosas
aprobadas (Eduardo, 24-sep, «2 también OK», §H6.11.9): **la TF retiene el bus en sus esperas** —hasta 1 s repitiendo
ACMD41 al arrancar y hasta 2 s mientras la tarjeta graba; la especificación deja soltar el CS en las dos— y **la C5 con
la TF montada por el firmware**, que es la prueba en placa de la concurrencia con la pantalla.

**El diseño:**
1. **El registro de la SD, en el común** (`src/bpvm_sd_placa.c`): al arrancar, la familia registra su dispositivo de
   bloque —o el motivo de no tenerlo: sin línea `sd` en el ENV, zócalo vacío—, y el montaje de arranque pasa también al
   común, con sus dos reglas de siempre: el pin de detección manda (Eduardo, 8-ago) y un fallo no degrada el arranque.
2. **Los dos verbos, en `bpvm_repl_dispatch`**, con los mismos mensajes y campos que lee el explorador del IDE
   (`SD_INFO_REPLY`: `ok`, `motivo`, `bloques`, `producto`, `oem`, `fabricante`, `version`, `altaCap`, `anno`, `mes`,
   `serie`, `leeSector0`…; `SD_MOUNT_REPLY`: `ok`, `motivo`, `prefijo`, `lba`, `etiqueta`, `kbLibres`, `kbTotal`,
   `entradasRaiz`, `primera`, `resumenFalla`). Sin SD registrada, **error con nombre** («esta placa no tiene SD: su ENV
   no lleva línea `sd`»), no «type no implementado» (el veredicto de `U3.0b`).
3. **La identidad de la tarjeta** (CID/CSD) entra en el contrato de bloque como método OPCIONAL (NULL si el medio no la
   da): el SPI ya la tiene (`bpvm_sd_blk_info`), el SDMMC del P4 la saca del `sdmmc_card_t`. Lo propio de un transporte
   —la traza del SPI: `peldano`, `ultimoCmd`, `traza`— entra por un gancho opcional del dispositivo, como `info_extra`
   en el `INFO`.
4. **La TF suelta el bus en sus esperas**: cada sondeo de ACMD41 y cada espera de «ocupada» es su propio turno corto,
   con el CS arriba entre medias (lo que la especificación permite): ninguna espera pasa del tope del bus.
5. **Las familias**: la Metro y el P4 registran lo que ya montan (y `pico/repl_v1.c` pierde sus dos handlers); la C5
   activa lo genérico (`sd=bus:2,cs:23` en su ENV y el mismo registro); la Pico 2, la S3, la C3, la Nucleo y la DK2 no
   registran nada → el error con nombre. **La C6** tiene ranura y es lo mismo que la C5 (su línea `sd=` con los pines
   del esquemático): **entra** (Eduardo, 24-sep: «OK, C6 también»)… **pero tras `H6.b`** (Eduardo, esa misma noche: «C6
   la a»): su pantalla habla con el SPI por `esp_lcd` directamente —`spi_bus_initialize` con DMA—, sin pasar por el
   árbitro, y la TF a la vez la pisaría. Primero su pantalla por el pack y la HAL, como la C5; luego su TF.

**Cómo se prueba:** en el PC, una prueba de C del común (`test-sdcomun`) con un dispositivo de bloque de RAM que lleva una
imagen FAT —`SD_INFO`/`SD_MOUNT` por el wire, y sin SD el error con nombre— y la TF contra la cinta del árbitro: ninguna
espera retiene el bus más que el tope, y su control (el comportamiento de antes) sale rojo; las dos, a `check-testc`. En
placa: la Metro (el control: lo de antes, igual), el P4 y la C5 con tarjeta —la C5, además, escribiendo en `/sd`
mientras `Gui.run()` anima—, y el explorador del IDE (`sd`, `sd mount`) contra cada una.

| paso | qué | se prueba con |
|---|---|---|
| **SD.1** | el registro, los dos verbos al común y la identidad opcional | `test-sdcomun`, en el PC |
| **SD.2** | la TF suelta el bus en sus esperas | la cinta del árbitro, con su control |
| **SD.3** | las familias: la Metro y el P4 al registro (fuera los handlers de `repl_v1.c`); la C5 (la C6, tras `H6.b`) | compilan las cinco familias; el micro simulado |
| **SD.4** | en placa, con tarjeta: la Metro, el P4 y la C5 (con la pantalla a la vez) | la tanda y el explorador del IDE |

**SD.1, hecho (24-sep, noche).** Dos cosas quedaron más sencillas que en el papel: **la traza viaja en la misma
identidad** (`bpvm_blk_identidad_t`, con `con_traza`: el SPI la da y un medio sin traza la manda VACÍA —un «00 00…»
diría «la línea flota»—), así que no hace falta el gancho `info_extra`; y **FAT llega por punteros**
(`bpvm_fs_fat_ops`, definido en `fs_fat.c`): el registro (`src/bpvm_sd_placa.c`) no enlaza FatFs, que hoy sólo llevan
la Pico y el P4 —ni las ESP32 chicas ni el host—, y una imagen sin ranura no paga FAT. El micro simulado tiene ranura:
`--sd=IMAGEN`, un fichero que cumple el contrato de bloque, y `tools/fat_imagen.py` hace la tarjeta como vienen las de
fábrica (MBR y FAT16 en el bloque 2048, con etiqueta y un fichero; el firmware no formatea). `tools/sd_smoke.py`, en
`check-sim`: con tarjeta (la identidad, el sector 0 y el MBR, el montaje del ARRANQUE, leer, escribir y que lo escrito
siga tras volver a montar), sin tarjeta (el motivo) y con una rota (su motivo, y el wire vivo). La Metro conserva sus
dos handlers hasta SD.3: su cadena los atiende antes que el común.

**SD.2, hecho (24-sep, noche).** `bpvm_sd.c`: el arranque suelta el bus tras CMD8 y cada intento de ACMD41 es un turno
corto —el que saca a la tarjeta de IDLE sigue, para el OCR, el CID y el CSD—; al escribir, la tarjeta graba sola tras
aceptar los datos y la espera de «ocupada» son turnos de un byte. `make test-sd-turnos` (en `check-testc`): una tarjeta
de mentira detrás de la cintura SPI (IDLE 250 ms, graba 200 ms) y una pantalla en otro hilo del SO: la pantalla consigue
turnos MIENTRAS la TF arranca (16) y MIENTRAS la tarjeta graba (13), y lo más que espera por uno es 1 ms. Su control
(`make test-sd-turnos-control`, la TF de antes con `-DBPVM_SD_TURNO_UNICO`) la deja fuera 249 y 184 ms: rojo, como
debe. La lectura no cambia: con el CS arriba mientras se espera el token, la tarjeta abortaría la lectura, y esa espera
es corta.

**SD.3, hecho (24-sep, noche).** Las tres placas que tienen ranura y la usan registran su SD, y las demás dicen por qué
no la tienen:
- **La Metro.** La línea `sd` del ENV (variante SPI) va al registro con `bpvm_sd_blk_registrar_desde_env`
  (`src/bpvm_sd_spi_env.c`). Vive aparte de `bpvm_sd_blk.c` porque aquél es el dispositivo PURO, el que `test-blk`
  enlaza sin FS ni registro. El montaje del arranque es el común, y `pico/repl_v1.c` pierde `SD_INFO`, `SD_MOUNT` y el
  vigía en caliente (−270 líneas): los atiende `bpvm_repl_dispatch`, con los campos de antes.
- **El P4.** Su SDMMC da la identidad: `p4_identidad`, del `sdmmc_card_t` que decodifica el IDF (el CID y el CSD, sin
  traza). Se registra como `SDMMC`, y `SD_INFO`/`SD_MOUNT` le llegan por primera vez (antes, «type no implementado»).
- **La familia del `main` común** (S3, C3, C5 y C6). Cada una lleva `CHIP_SD_REGISTRAR()` en su `chip_cfg.h`, que es el
  mecanismo que ya decide lo que cambia por silicio. Se registra DESPUÉS de los buses del ENV, porque la línea corta de
  la C5 (`sd=bus:2,cs:23`) se engancha al SPI 2, y el común monta si el FS está arriba. Sólo la C5 lleva el motor SPI y
  FAT: medido en su `.map`, 19,5 KB de flash y 1,8 KB de RAM. La S3 y la C3 contestan «esta placa no tiene ranura de
  SD». La C6 contesta el porqué de esperar a `H6.b`, y no «su ENV no lleva línea `sd`», que sería mentira.
- **El vigía en caliente sólo lo llama la Metro**, que es la única con pin de detección. Sin detector, el dispositivo
  contesta siempre «hay medio», y el vigía daría un intento de más y nada más. En el P4 y en la C5 se monta al arrancar
  o con `SD_MOUNT`.

Compilan las ocho imágenes (Pico, P4, C5, S3, C3, C6, Nucleo y DK2). Los tests de C de la SD y `compat` salen en verde.
Queda SD.4, en placa.

**SD.4, en marcha (24-sep, noche): la Metro y la C5 con tarjeta.** Antes de grabar nada, el CONTROL: la Metro con la
imagen de antes (`wire_serie.py sd`, verbo nuevo) → «el pin 34 no existe en rp2350a». Y de ahí, cinco fichas en fila,
cada una destapada por la anterior:
- **`#549`** (H4): la Metro tenía la tabla de capacidades de la A —`main()` la elegía antes de leer la variante— y su
  SD estaba muerta desde H4 P1. Con la tabla del silicio, monta al arrancar por el común; `SD_INFO`/`SD_MOUNT` dan su
  SanDisk de 32 GB (MBR, FAT32 en el bloque 2048).
- **`#550`**: la serie salía negativa (`long` de 32 bits); ahora sin signo, y el micro simulado lo caza.
- **`#551`**: ni `mkdir` ni `writeFile` en `/sd`. Acotado paso a paso —primero el porqué del dispositivo en el log (la
  capa de disco callaba), luego la traza de la fase de datos: `CMD17 -> 00 … 00`, la tarjeta aún grabando—, con el
  control de la TF de antes (escribía bien): la «ocupada» de SD.2 volvía antes de tiempo porque la tarjeta real, al
  volver a elegirla, tarda en tirar de la línea. La tarjeta de mentira de `test-sd-turnos` era más amable que la de
  verdad; ya no.
- **`#552`**: en la C5 la TF a 25 MHz tuerce bits (su MISO pasa por la matriz de GPIO); la línea `sd` gana `khz:` y la
  C5 lleva `sd=bus:2,cs:23,khz:10000`.
- **`#553`**: los datos de un bloque, de una vez: `SdCard` en la C5 de 29,7 s a 3,8; en la Metro, de 4,7 a 2,8.

Con eso, `SdCard` «SD OK — los seis» en las dos, y **la TF y la pantalla de la C5 a la vez** (`samples/C5SdGui.bp`): la
pantalla sola, 188 cambios de una etiqueta en 5 s y 30 ms de peor hueco; con un hilo escribiendo y releyendo ficheros
de 2,3 KB en `/sd`, 176 y 63 ms, y 30 ficheros, 0 distintos. De paso, `#554` (abierta, H7): concatenar en placa es unas
250 veces más lento que en el PC —el primer `C5SdGui` medía eso y no la TF—. Quedan el P4 (no estaba conectado; su
`SD_INFO` es nuevo) y la tarjeta en caliente en la Metro (sacarla y meterla).

## H4 — la caracterización de los periféricos de cada micro: la HAL BP de capacidades (22-sep, día de análisis)

Tres fichas que son un solo diseño (`#378` la madre, `#470` los arrays, `#479` el mapa de pines y `AnPin`) y
seis restos que caen con él. Lo que hay, medido hoy; el diseño; las preguntas para Eduardo; el plan.

### 1. Lo que hay hoy, medido

**Los mismos datos se contestan por DOS caminos y ya no coinciden.** `QuienSoy.bp` (BP) contra el `INFO` del wire,
en tres placas y el host (22-sep):

| | BP: `Machine.*` | wire: `INFO` | qué pasa |
|---|---|---|---|
| **Pico** | gpio 30 · adc 4 · **pwm 12** · uart/i2c/spi 2/2/2 · variant A · max 300 MHz | gpio 30 · adc 4 · **pwm 24** · pio 3 | 12 *slices* frente a 24 *canales*: la misma cosa con dos unidades, sin decir cuál |
| **P4** | gpio 55 · adc 14 · pwm 14 · **variant «B»** · **max 300 MHz** (corre a 360) | gpio 55 · adc 14 · pwm 14 | `variant()` es «B si gpioCount ≥ 40» (del RP2350) y contesta B en un ESP32; `MAX_CPU_MHZ` es el del RP2350 para las cinco familias |
| **C5** (ayer) | *(la familia)* **gpio 45 · adc 20 · pwm 8** | gpio 29 · adc 6 · pwm 6 | `gpio_esp32.c` es común a S3/C3/C6/C5 y devuelve los números del **S3** a pelo: un programa en el C3/C6/C5 se cree un S3 (`#470`) |
| **host** | micro host · 30/4/12 · 150 MHz | — | el perfil del RP2350 como defecto de `src/pico.c` |

- **Dos structs rellenadas a mano por familia**: `bpvm_pico_backend_t` (15 callbacks, lo que ve BP; `include/bpvm_pico.h`,
  el nombre `pico` en 14 `BUILTIN_PICO_*`, 15 `__pico*` en Core.bp y 81 usos en C) y `repl_board_id_t`/`bpvm_repl_info_t`
  (lo que ve el wire; `board_name, cpu_freq_hz, gpio_count, pio_count, pwm_slices, adc_channels, sram_bytes`). Nadie
  garantiza que digan lo mismo; el IDE (`PicoExplorer.java:2237`) muestra los del wire.
- **`UART_BUSES()`/`I2C_BUSES()`/`SPI_BUSES()` devuelven 2 en BP para todo el mundo** (`Machine.bp:194-204`: literales).
- **Sólo hay CONTADORES, nunca CUÁLES**: `gpioCount()` dice 45 y no dice que el 22-25 del S3 no existen; `adcChannels()`
  dice 20 y `Adc.Channel(ch)` deja usar `0..3` (`CH0..CH3`: los cuatro pines del RP2350 congelados como contrato);
  `Pwm.Slice(pin)`/`Pulse.Counter(pin)` toman un PIN sin que nadie sepa si ese pin puede.
- **El estado del pin no está en el común**: `src/gpio.c` reenvía y no guarda; sólo el STM32 lleva `s_mode[128]` en
  privado. Sin estado, «analógico ⇒ no digital» no es comprobable.
- **El descriptor de placa existe sólo en el RP2350** (`pico/board_desc.c`: variante por `SYSINFO.PACKAGE_SEL`, caps por
  variante, pines de placa desde `board.json`, flash por JEDEC) y no es común.
- Modos de GPIO: `init(pin, mode)` con 0/1 (INPUT/OUTPUT) y `pull`; sin ANALOG ni colector abierto.
- El breadcrumb (`setMark`…): cuatro slots que sólo el STM32 rellena; la fachada contesta 0/0/1 mudo (`#487`).

### 2. El diseño: UNA tabla de capacidades por micro, en la imagen, de la que beben todos

**La regla:** cada micro trae **una `const` en su cintura** —`bpvm_caps_t`— y **ningún otro sitio declara un número**:
el `INFO`, `Machine`, la validación de las fachadas y las clases de pines leen de ahí. El RP2350 la elige por
variante en el arranque (A/B, como hoy `board_desc`); el ESP32 una por chip (S3, C3, C6, C5, P4: cada uno tiene ya
su `chip_cfg.h`, que es donde va); el STM32 una por micro (U575, U5G9); el host, la del RP2350A.

```c
typedef struct {
    const char* micro;                 /* "esp32c5" — hoy CHIP_MICRO */
    const char* fabricante;            /* "Espressif" | "Raspberry Pi" | "STMicroelectronics" — #471: manufacturer() */
    uint32_t    cpu_hz_max, cpu_hz_min;/* POR MICRO (hoy 300/18 del RP2350 para todos) y si se puede cambiar en marcha */
    uint8_t     cpu_ajustable;
    /* LOS ARRAYS de Eduardo (#470): las INSTANCIAS válidas; el número es la longitud */
    const uint8_t* uart; uint8_t n_uart;
    const uint8_t* i2c;  uint8_t n_i2c;
    const uint8_t* spi;  uint8_t n_spi;
    uint8_t        n_pwm;              /* salidas PWM simultáneas, con su UNIDAD dicha: */
    const char*    pwm_unidad;         /* "slice" (RP2350) | "canal" (LEDC, timer) */
    /* EL MAPA DE PINES (#479): por GPIO, qué puede; 0 = no existe / no bondeado */
    const uint16_t* pin;  uint8_t n_pin;   /* bits: DIGITAL, ANALOG, PWM, PULSE, OPEN_DRAIN, INPUT_ONLY */
    const int8_t*   pin_adc;               /* por GPIO: su canal de ADC, o -1 */
    uint8_t         breadcrumb;            /* hay migas de verdad (RAM retenida) o no (#487) */
} bpvm_caps_t;
```

**De ahí salen, sin declarar nada dos veces:**
- **El `INFO`**: `gpioCount` = pines con bits ≠ 0; `adcChannels` = pines con ANALOG; `pwmSlices` = `n_pwm` **más
  `pwmUnidad`**; y los arrays de buses (`"uart":[0,1]`…). El IDE lee por nombre: un campo nuevo no le estorba.
- **`Machine` en BP** (tres primitivas nuevas, no quince: `__capsArray(kind): integer[]`, `__capsInt(kind)`,
  `__capsStr(kind)`): `uarts()`, `i2cs()`, `spis()`, `pins()`, `analogPins()`, `pwmPins()`, `hasPin(n)`,
  `isAnalog(n)`, `pwmUnit()`, `manufacturer()`, `MAX_CPU_MHZ()`/`MIN_CPU_MHZ()` por micro. Los contadores de
  hoy (`UART_BUSES()`, `ADC_CHANNELS()`, `GPIO_COUNT()`…) se quedan como azúcar (`len(uarts())`): ningún programa se
  rompe. `variant()` deja de inventar: «A»/«B» sólo si el micro es un RP2350, «» en los demás.
- **La validación, en el COMÚN y con nombre**: `Uart.Port(3, …)` en un C5 → `RuntimeError: UART 3 no existe en
  esp32c5 (válidos: 0, 1)`; `Pwm.Slice(22)` en un S3 → `el pin 22 no existe en esp32s3`; `AnPin(5)` en un pin sin
  ANALOG → `el pin 5 de esp32c5 no es analógico (analógicos: 1..6)`. Hoy cada cintura valida lo que le parece.
- **El estado del pin, en el común** (`src/gpio.c`: `uint8_t modo[n_pin]`): el invariante «analógico ⇒ no digital»
  se escribe UNA vez: `Pin(n, OUTPUT)` sobre un pin en ANALOG es error, `AnPin(n)` sobre un pin digital es error, y
  `on()`/`off()` sobre un `AnPin` es error. El STM32 tira su `s_mode` privado.
- **El lenguaje** (decidido el 6-sep): `Pin` con `INPUT/OUTPUT/OPEN_COLLECTOR/ANALOG`; **`AnPin extends Pin`**
  (`AnPin(num)` = `super(num, Pin.ANALOG)`, `read()`, `readVolts()`); `OPEN_COLLECTOR` sólo donde el silicio lo
  tiene (ESP32, STM32: bit OPEN_DRAIN del mapa; RP2350 → error con nombre, no emulación).
- **`Adc.Channel(ch)`** sigue existiendo como la vista «por canal» (ADC1), pero validada por el rango REAL del micro
  (S3 0..9, C5 0..5, RP2350 0..3, U5 0..19) y con el mapa canal↔pin de la tabla; `AnPin` es la vista «por pin».
  Las dos acaban en el mismo backend.
- **Los restos**: `manufacturer()` y `MAX/MIN` por micro (`#471`) están en la tabla; el nombre `pico` de la capa C se
  va con la refactorización (`bpvm_pico_backend_t` → `bpvm_machine_backend_t` con lo que NO es tabla: `uniqueId`,
  `tempC`, `uptime`, `resetCause`, las migas, `setCpuFreq`; 14 ids sin cambiar de número); el breadcrumb declara si
  está (`caps.breadcrumb`) y la fachada distingue slot NULL de «nada»; el tamaño de flash ya lo dice la placa en
  ESP32 (`esp_flash_get_size`, el `INFO` del C5 dijo 32 MB y el arranque cantó el desajuste) y en STM32 es interna y
  la dice el registro `FLASH_SIZE`: lo que faltaba era que el arranque lo COMPARARA, y ya lo hace.

**Lo que NO cambia**: lo que es de PLACA (LED, NeoPixel, panel, tamaño de FS) sigue en el ENV/`board.json` — la tabla
es verdad de silicio y viaja compilada, como decidió Eduardo el 6-sep.

### 2b. El apunte de Eduardo (22-sep): un micro NO es un hardware — la tabla es por VARIANTE y se elige en el arranque

*«Un micro puede tener diferente hardware. La RP2350A y RP2350B; en las ESP32 varían sobre todo en configuración de
RAM y flash; los STM32 son los más complejos: puede haber micros con 64, 80, 100, 144… pines. En muchos casos los
cambios son el número de pines y algún periférico más o menos. Si se puede construir una imagen única como la RP2350,
hay que tener presente esas diferencias. Algún micro tiene más de un ADC.»*

Cambia tres cosas del diseño de §2, y las tres van en la misma dirección — **la imagen única por familia decide la
tabla al arrancar**, no al compilar:

1. **La tabla se ELIGE en el arranque por lo que el silicio dice de sí mismo**, con el mecanismo de cada casa:
   - **RP2350**: `SYSINFO.PACKAGE_SEL` → A (QFN-60, 30 GPIO) / B (QFN-80, 48 GPIO). Ya existe (`board_desc.c`): es el
     modelo. Una imagen, dos tablas.
   - **STM32**: `DBGMCU_IDCODE` (qué die: U575/U585/U5G9…, y con él qué periféricos hay) + el **registro de paquete**
     (`PACKAGE`, en la U5 en `0x0BFA0500`: LQFP64/100/144, UFBGA169…) → **el mapa de pines del die enmascarado por los
     pines bondeados del paquete**. Una imagen por die con N paquetes dentro: la tabla del die lleva el mapa completo y
     una máscara por paquete. Es lo que hace posible que la placa 30 del STM32 sea sólo pines y ENV.
   - **ESP32**: los pines son fijos por chip (S3, C3, C6, C5, P4: cada uno su proyecto y su tabla); lo que varía por
     módulo —flash, PSRAM— **ya se mide al arrancar** (`esp_flash_get_size`, `esp_psram_get_size`) y no va en la tabla.
2. **La RAM y la flash NUNCA van en la tabla**: se miden (JEDEC en RP2350, `esp_flash_get_size` en ESP32, el registro
   `FLASH_SIZE` en STM32) — es lo que ya hace el `INFO` y lo que ayer cantó el desajuste del C5.
3. **Más de un ADC**: el mapa de pines guarda **(unidad, canal)** por pin, no sólo el canal. `AnPin(pin)` resuelve las
   dos cosas desde el mapa —la vista por pin no tiene el problema—; `Adc.Channel(ch)` es la vista por canal de la
   **unidad 1** (documentado), y si un día hace falta otra unidad por canal, es `Adc.Channel(unidad, ch)`: una
   sobrecarga, no un cambio.

Con eso la struct de §2 gana `paquete` (texto: "QFN-80", "LQFP144"), `n_adc` (unidades) y `pin_adc` pasa a dos arrays
(`pin_adc_unit`, `pin_adc_ch`); y `bpvm_caps()` es una FUNCIÓN que devuelve la tabla elegida en el arranque, no un
símbolo `const` fijo. El `INFO` publica `micro` y `paquete`; `Machine.package(): string` en BP.

### 3. Las preguntas (propuesta; Eduardo decide)

| # | pregunta | propuesta |
|---|---|---|
| 1 | los arrays en BP | `Machine.uarts()/i2cs()/spis()`: `integer[]` con las instancias válidas; los contadores de hoy se quedan como azúcar |
| 2 | PWM, ADC y contadores de pulsos: ¿array de instancias o de pines? | **de PINES**: en BP se construyen por pin (`Pwm.Slice(pin)`, `Pulse.Counter(pin)`, `AnPin(pin)`), así que lo útil es `pwmPins()`/`analogPins()`/`pulsePins()` del mapa; el contador (`PWM_SLICES()`) sigue diciendo cuántas salidas simultáneas hay, con `pwmUnit()` al lado |
| 3 | `Adc.Channel` | se queda (vista por canal, rango real) junto a `AnPin` (vista por pin). Quitarla rompería `AdcDemo` y los samples de placa sin ganar nada |
| 4 | `OPEN_COLLECTOR` en RP2350 | **error con nombre** («este micro no tiene salida en colector abierto»), no emulación |
| 5 | el `INFO` | crece con `micro`, `pwmUnidad` y los arrays de buses; el mapa de pines NO viaja (es para programas, no para el IDE) |
| 6 | el nombre de la capa C | `Machine`/`bpvm_machine_*` (como ya se llama en BP); ids de builtin sin cambiar |
| 7 | breadcrumb en RP2350 y ESP32 | **implementarlo**: RP2350 con los registros scratch del watchdog, ESP32 con RAM `RTC_NOINIT` — es barato y es la herramienta de «ya estoy en problemas» |
| 8 | NeoPixel ESP32/STM32 | ESP32 con RMT (`led_strip`), en el C6 devkit (GPIO8) que es la placa que lo tiene; STM32 después, sólo si alguna placa trae uno |

### 4. El plan, por pasos que cierran algo

| paso | qué | cierra |
|---|---|---|
| **P1** | `bpvm_caps_t` + una tabla por micro (rp2350a/b, s3, c3, c6, c5, p4, u575, u5g9, host); el `INFO` y `Machine` beben de ella; las tres primitivas; `manufacturer()`, `MAX/MIN` por micro, `variant()` honesto; el renombrado `pico` → `machine` en C | `#378`, `#470`, `#471` restos, el nombre `pico`, la flash |
| **P2** | la validación en el común con nombre; el estado del pin en el común; `Pin.ANALOG`/`OPEN_COLLECTOR`; `AnPin`; `Adc.Channel` sobre el mapa; el ADC del STM32 (canales externos, con la DK2/Nucleo a mano); el `Pulse` del C3 declara 0 | `#479`, `#469` resto, `#480` resto |
| **P3** | breadcrumb en RP2350 y ESP32 (`#487`); NeoPixel en el C6; el botón BOOT del C6 | `#487`, NeoPixel, BOOT |
| **P4** | samples (`Caps.bp` en el corpus con el host; `PinMap` de placa en la tanda), manual/referencia, cascada y tanda en las placas que haya | la fila H4 |

### 5. Lo que queda de H4 (26-sep, análisis): el NeoPixel de la familia ESP32, los pines del sistema y el ADC del STM32

De la fila de H4 quedan dos backends que sólo existen en una o dos familias —el NeoPixel sólo lo registra la Pico
(`pico/main.c`, PIO) y el ADC, la Pico y la familia ESP32— y, al mirarlos, salió un tercero que no estaba en la lista.

#### 5.1 El NeoPixel de la familia ESP32

- **La placa donde se prueba: la S3.** La pregunta 8 lo ponía en la C6 (el esquemático pone un WS2812B en el GPIO8),
  pero Eduardo, mirando las placas el 26-sep: en la C3 y en la C6 no se ve ningún LED RGB; la S3 lleva uno. En la
  DevKitC-1 va al GPIO48 (revisión 1.0) o al 38 (1.1), y la tabla calculada de `#470` acepta los dos.
- **El contrato, el de la Pico sin tocar** (`bpvm_neopixel.h`): `init(pin)` → 1/0; `show(pin, grb, n)` BLOQUEANTE;
  palabras GRB, 24 bits desde el más alto; hasta 2048 LEDs por trama (la zona de rascar, `#536`).
- **Cómo**: el driver RMT del NÚCLEO del IDF (`esp_driver_rmt`) y no el componente `led_strip` que decía la pregunta 8:
  ése es un componente gestionado que se DESCARGA del registro, o sea una dependencia de red para compilar la imagen.
  Con un *simple encoder* (`rmt_new_simple_encoder`, en el IDF v6) que traduce las palabras GRB a símbolos RMT al
  vuelo: sin copia y sin búfer. 10 MHz de resolución, los tiempos de la hoja del WS2812B (T0H 0,4 µs y T0L 0,85; T1H
  0,8 y T1L 0,45; ±150 ns) y, al final de la trama, el *reset* como un símbolo bajo de 300 µs (el WS2812B de hoy pide
  ≥ 280; la Pico espera 60, que vale para el suyo). `rmt_tx_wait_all_done`: bloqueante, como la Pico. Va en
  `esp32/common/gpio_esp32.c`, registrado en `esp32_hw_register` junto al PWM (LEDC) y el Pulse (PCNT): las cinco
  imágenes ESP ya compilan ese fichero, así que no hay fuente nueva que dar de alta.
- **Los límites, leídos en el IDF**: canales RMT de transmisión — S3 4, P4 4, C3 2, C5 2, C6 2 —, o sea tantas tiras a
  la vez. Un LED (24 símbolos + el reset) cabe en la memoria del canal (48): sin recargas. Una tira larga se recarga
  por interrupción, y una escritura en la flash (que para la caché) puede retrasarla y deformar la trama: sólo con
  tiras largas y coincidiendo con el FS. Se apunta, no se arregla (la S3 y el P4 tienen DMA para el RMT).
- **Robustez**: el `init` de un pin ya iniciado REHACE su canal. Un `Gpio.Pin` sobre ese pin entre medias reconfigura el
  pad y el RMT dejaría de llegar al cable sin un error. (En la Pico pasa lo mismo con la PIO: se apunta.)
- **En el común (las tres familias)**: `__npInit` valida el pin contra la tabla como ya hacen el PWM y el Pulse
  (`bpvm_gpio_reclamar(pin, OUTPUT)`: «el pin 25 no existe en esp32s3», «es del SPI 2», «lo usa un código nativo»…),
  SÓLO si hay driver —así el PC y `NeoCatch` siguen diciendo lo mismo en las dos VMs—; y distingue «no implementado en
  esta plataforma» (no hay driver) de «no se pudo iniciar» (el driver dijo que no: sin canal libre). Hoy las dos cosas
  dicen lo primero.
- **La alternativa del SPI (Eduardo, 26-sep: *«En MicroPython tienen un SPI por software. Si tuviésemos uno nosotros lo
  podríamos utilizar para programar los leds Neopixel»*)**. Mirado en su documentación: su `neopixel` no va por
  `SoftSPI` sino por `machine.bitstream` (un pin conmutado con los tiempos del WS2812, `(400, 850, 800, 450)` ns), y en
  la ESP32 ese `bitstream` *«uses an RMT channel by default»* — lo mismo que se propone aquí. En las demás es un bucle
  contado por ciclos: *«On Cortex M0 at 48MHz, it is at best +/- 120ns»*, con la tolerancia del WS2812 en ±150. Un SPI
  por SOFTWARE no llega con las interrupciones puestas (una interrupción, o en la ESP32 un fallo de caché, deforma el
  bit) y, con ellas quitadas, para el micro 30 µs por LED —61 ms una tira de 2048: el wire y los hilos, parados—. Donde
  la idea SÍ vale es con un SPI por HARDWARE: el MOSI a 2,4 MHz y tres bits de SPI por bit del LED (`100` y `110`), con
  DMA, da los tiempos exactos sin CPU. Es la forma clásica de dar NeoPixel a una familia sin RMT ni PIO —el STM32—, con
  una condición: el LED no tiene CS, así que lee TODO lo que pasa por el MOSI y necesita un SPI para él solo.
- **La prueba**: `NeoDemo` con el pin de la S3 (Eduardo mira el LED); un segundo `Strip` sobre el mismo pin después de
  un `Gpio.Pin` (sigue pintando); pedir más tiras que canales (error con nombre); `NpShowTope`. Y compilan las cinco
  imágenes ESP.

#### 5.2 Un hallazgo: la tabla de la ESP32 ofrece pines que usa el sistema (ficha nueva)

`esp32_caps_init` da por buenos los que dice `GPIO_IS_VALID_GPIO`, y eso incluye los de la flash y la PSRAM. En la C6
la flash va por los GPIO 24–30 (el MISO es el 25: `esp_hal_gpspi/esp32c6/.../spi_pins.h`), así que un
`Gpio.Pin(25, OUTPUT)` se acepta y le quitaría la flash al micro. El IDF ya los reserva al arrancar
(`esp_mspi_pin_reserve`, y la PSRAM los suyos) y los que va tomando cada driver (UART, SPI, I2C, LEDC, RMT…), y se
preguntan con `esp_gpio_is_reserved`. **Arreglo**: marcarlos en la tabla como del sistema, con un error con nombre
(«el pin 25 de esp32c6 lo usa el sistema»). **Y la pregunta**: los del wire —el USB-Serial-JTAG de la C3, la C5 y la
C6; el UART0 de la S3 y el P4— el IDF no los reserva, y usarlos corta la conexión con el IDE.

#### 5.3 El ADC externo del STM32 (el resto de `#469`)

Hoy no hay backend y la tabla no marca ningún pin analógico, así que `Adc.Channel(n)` y `AnPin` fallan con nombre («el
canal 0 no existe en stm32u575 (unidad 1: 0 canales)»): es honesto, pero no hay ADC. Falta:
1. **El mapa** pin → (ADC1, canal) del U575 (la Nucleo) y del U5G9 (la DK2), de sus hojas de datos. No están en el
   equipo, y el CubeIDE 2.x ya no trae la base de datos de CubeMX de la que se podría sacar.
2. **El backend**: el ADC1 a 12 bits (el contrato de BP es 0..4095; el del U5 es de 14 y se configura a 12), el pin en
   modo analógico y una conversión por sondeo, como ya hace la temperatura del chip (`stm32_adc_read_internal`).
3. **La prueba en la Nucleo**: un puente de un pin analógico del conector Arduino a 3V3 y a GND (≈ 4095 y ≈ 0).

Y de paso, en la misma tabla: la DK2 dice «UFBGA169» y su proyecto de CubeMX dice STM32U5G9ZJTxQ en LQFP144.

#### 5.4 Las preguntas (propuesta; Eduardo decide)

| # | pregunta | propuesta |
|---|---|---|
| 1 | el NeoPixel de la ESP32: ¿el RMT del núcleo del IDF o el componente `led_strip`? | el del núcleo, con el *simple encoder*: sin dependencias que descargar y sin copia |
| 2 | ¿validar el pin del NeoPixel en el común y separar «no implementado» de «no se pudo iniciar»? | sí (cambia también la Pico, a mejor) |
| 3 | los pines del sistema en la tabla: ¿sólo los que reserva el IDF (flash, PSRAM) o también los del wire? | también los del wire |
| 4 | las hojas de datos del U575 y del U5G9, y el manual de la Nucleo-144 (el conector) | que las ponga Eduardo en `C:\lenguajes\ref\`, o descargarlas con su permiso |
| 5 | la S3, para mirar el LED | conectada cuando toque; con su revisión (GPIO48 o 38), o se prueban los dos |
| 6 | el NeoPixel del STM32 (hoy «no implementado»; ninguna del parque lo trae) | cuando haga falta, por SPI HARDWARE (2,4 MHz, tres bits por bit, DMA) en un SPI para él solo; y un SPI por SOFTWARE, si se quiere, como fachada propia para dispositivos lentos en cualquier pin, no para el NeoPixel |

**El orden**: primero 5.1 y 5.2 (no necesitan papeles; la S3 para mirar), después 5.3 (con las hojas y la Nucleo con
puentes).

**Decidido** (Eduardo, 26-sep): 1, el del IDF; 2, sí; 3, también los del wire; 4, las bajó él (`C:\lenguajes\ref\DS_stm32u575ag.pdf` y `DS_stm32u5g7vj.pdf`); 5, no se sabía: se probó. La 6 (el SPI), apuntada.

**5.1 y 5.2, hechos** (`6a53f91f`). El backend RMT como estaba propuesto, y lo que salió al probarlo en la S3: la stdlib embebida de la familia ESP32 no llevaba `Neopixel` (no había driver) y el primer `RUN` dijo «falta el módulo 'Neopixel'» — va ya en la lista de `regen_esp32_mods.sh`. En la placa (grabada con su OK; sin packs): `gpioCount` de 45 a 36 (#574: los 7 de la flash y los 2 del wire); `samples/NeoEsp.bp` con los errores con nombre —el pin 25 no existe, el 43 es del wire, el 30 de la flash, la quinta tira no cabe—; y el LED, mirado por Eduardo: rojo, verde y azul, y otra vez después de pedir sus pines como `Gpio.Pin` (el `init` rehace el canal). Con una prueba de color —el 48 en rojo, el 38 en azul— sólo se vio el rojo: el LED va al GPIO48 y la placa es una DevKitC-1 v1.0. La pila, medida con los flags de la Pico, sin cambio (el `switch` de builtins 2096 B, el intérprete 256). Queda la C6, con `samples/PinesSistema.bp`, para la sesión de placas de P2b.3 (lleva packs nativos sellados); y 5.3.

**5.3, hecho** (`422a55dc`). El mapa, sacado con `pypdf` de la tabla «pin definitions» de las dos hojas (dan el mismo): los 17 canales del ADC1 —PC0–PC3 los 1–4, PA0–PA7 los 5–12, PC4–PC5 los 13–14, PB0–PB2 los 15–17— y 7 pines que sólo tiene el ADC4 (PF14, PF15, PG0, PG1, PD11–PD13; el U5G9 tiene además un ADC2 en los pines del ADC1). El backend, con el `hadc1` de la temperatura del chip, recortado a 12 bits y con el muestreo largo. Y `#574` también en el STM32: el wire es el USART1 del VCP en las dos placas, PA9 y PA10. **En la Nucleo**, con el divisor al 50 % que montó Eduardo en A0: `ADC1_IN8 = PA3: 2043 = 1,646 V` —esperaba 1,65— y lo mismo por `AnPin(3)`; los otros 16 canales, al aire, alrededor de 0,7 V. A0 = PA3 no salió de un manual (no lo teníamos): salió de la medida. Con esto, **H4 queda hecho en el código**; en placa faltan la C6 (`#574`), el ADC de la DK2 y el NeoPixel de la Metro con el `__npInit` nuevo, para la sesión de P2b.3.

---

## H5 — el AOT en la tanda de placas (25-sep, sesión remota; Eduardo: «meter AOT en la tanda de placas»)

**Por qué.** El 25-sep, al ejecutar por primera vez un `.mdn` en una placa sin FPU, salieron dos fallos que ninguna red
había visto: `#555` (fundir el nativo en un `.mod` MOD9 lo rompía: el AOT de los programas, roto en todas las placas
desde el 17-sep) y `#556` (MEMPROT: la C5 se reiniciaba al primer thunk). La tanda corre los `.mod` SIN AOT, y las
pruebas de C del AOT compilan el C emitido en el PC: nadie hacía el camino entero —compilar para la receta de la placa,
fundir, subir, ejecutar el nativo—.

**Qué:**
1. **El manifiesto**: `"aot": true` en una prueba. Y una opción general, `"ignorar": [regex…]`: las líneas que casan se
   quitan de las dos salidas antes de comparar (la línea de tiempo de `NatV7`).
2. **Por placa, la receta sale de su INFO** (`aotReceta`). Sin receta —el micro simulado, que es x86; la S3, Xtensa—
   la prueba es SALTADA, con ese motivo. Con receta, el `.mod` fundido se construye UNA vez por receta a partir del del
   oráculo, por el MISMO camino que el Run del IDE (`AotBuild`, vía `AotSingle`). `AotSingle` acepta `receta:<NOMBRE>`
   como destino: el catálogo de recetas y destinos sigue en un solo sitio (`NpackReloc`), no copiado en Python.
3. **Lo que se sube es el fundido** (su CRC, en el informe), y la salida se juzga como siempre (idéntico / no-peta).
4. **Y además, que el nativo SE USÓ.** Los números salen iguales interpretados (la trampa de H13-40: «fib(28) AOT»
   daba lo mismo que el interpretado), así que la salida no basta. La placa lo dice: el `EXITED` gana un campo,
   `nativos` —`bpvm_aot_count()`, los thunks registrados en ese RUN—, en las tres familias y en el micro simulado. Tiene
   que coincidir con los thunks que dijo el compilador. Veredictos nuevos: **SIN NATIVO** (rojo: `nativos` 0, corrió
   interpretado) y **SIN COMPROBAR** (no verde: una imagen de antes, sin el campo). No se usa el log: con `log=0`, que es
   lo de fábrica, la línea «MDN: n/n thunks» no se escribe, y un aviso que no distingue «no pasó» de «no se apuntó» no
   sirve (memoria: *aviso que no distingue*). `#555` ya sale como ERROR (fallo de carga) y `#556` como REINICIO.
5. **Las pruebas**: `NatNew` (siete valores, idéntico) y `NatV7` (método `native`, `double`, `print` desde nativo;
   idéntico salvo su línea de tiempo).

**Cómo se prueba**: el micro simulado (las dos AOT, SALTADAS con su motivo; las demás, como siempre); luego la Metro
(ARM), el P4 (RISC-V con FPU) y la C5 (sin FPU), con la imagen que manda `nativos`: las tres recetas. Y el control: el
`.mod` SIN fundir, subido como si fuera AOT, tiene que salir SIN NATIVO.

### H5.P2b — el AOT llama a CUALQUIER builtin por el despachador de la VM (26-sep, análisis y decisión)

> De la tabla de H1 (§3, P2b): *hoy traduce los builtins globales POR NOMBRE con una lista a mano y rechaza
> `Math.sin(x)` desde una `native`*. Eduardo, 24-sep: es del AOT, H5.

**Lo que hay hoy, medido.** `AotCEmitter.emitBuiltinCall` conoce diez nombres (`now`, `charAt`, `charCodeAt`,
`substring`, `intToString`, `len`, `newInt/Byte/Long/DoubleArray`) y los traduce a helpers TIPADOS de la tabla del
`.mdn` (`string_char_at`…). Cualquier otra intrínseca —`Math.sin`, `sqrt`, `Str.*`…— es `UnsupportedAotException`: la
función entera corre interpretada. Y cada helper tipado es una **COPIA** de su builtin, y las copias se van: al
compararlas una a una para este análisis salieron `#569` (`charAt` fuera de rango daba `""` con el AOT y un
`RuntimeError` interpretado; `substring(null…)`, `null` y no `""`; sin memoria, un `null` callado) y, de paso, `#570`
(un tamaño negativo MATABA el programa en la VM-C interpretada, donde miVM lanza un error atrapable). Ninguna red
comparaba la lista a mano con el intérprete: desde el 26-sep, `make test-aotstr`.

**La idea**: el compilador ya sabe el id —el cuerpo de una intrínseca es `inline CALL_BUILTIN n`, y viaja en la
interfaz del `.mod`—, así que el AOT puede emitir para cualquiera una llamada al MISMO despachador que usa el
intérprete (`bpvm_call_builtin(vm, tc, n)`). El nativo ejecuta entonces el código del intérprete, no una copia: la
paridad sale por construcción.

**El mecanismo** (leído en el código el 26-sep):

- Un helper nuevo AL FINAL de la tabla: `int64_t call_builtin(vm, id, tipos, args, tipo_ret)`. Los argumentos viajan
  como palabras de 64 bits con sus BITS (`tipos`: `i` i32, `l` i64, `f` float, `d` double, `r` referencia): así un
  `double` no cruza la frontera por valor y no le afecta el `-mfloat-abi` (el `BPVM_AOT_FP_ABI` de la tabla).
- El helper hace lo que `bridge_run_bp_frame` para `call_bp`: empuja los argumentos en la pila BP del hilo sobre su
  `sp` vivo (hoy `cc->sp_p`, que con la suspensión pasa a colgar del hilo: abajo; las referencias, ensanchadas a 8 B
  con `bpref_regen`), llama al
  despachador, saca el retorno según `tipo_ret` y deja `tc->sp/pc/cs` como estaban. El GC ve los argumentos (están bajo
  `tc->sp`) y las referencias de C del nativo (el escaneo conservador de `#302`).
- `MDN_ABI_VERSION` 8 → 9 (la tabla crece): se regeneran todos los `.mdn` —los de los packs, SQLite incluido— y las
  imágenes, y se regraban las nueve placas.

**Los tres problemas de verdad:**

1. **Una excepción dentro del builtin.** `builtin_throw` hace él mismo `bpvm_eh_unwind`: desde un nativo deshacería
   marcos BP con los marcos de C del nativo todavía vivos. Solución: una marca en el hilo («dentro de una native»)
   con la que `builtin_throw` NO deshace: deja la excepción pendiente —como `throw_ref` (`#213`)— y el helper hace
   `longjmp` al límite de `#186`, que la propaga desde el lado BP como hoy. Es UN sitio: en `builtins.c` es el único
   `bpvm_eh_unwind` (los ~200 `builtin_throw` pasan por él). Los `return BPVM_ERR_*` que quedan tras `#570` son errores
   internos (aridad de un evento, un mutex corrupto, no caben más hilos): desde el nativo, `throw_runtime` con su nombre.
2. **Un builtin que cede o bloquea el hilo** —por la primitiva (`tc->status =` y el mutex): `sleep`, `sleepSec`,
   `yield`, `__threadJoin` (`Thread.join`) y `__mutexLock` (`Mutex.lock`)—. La primera propuesta era rechazarlos al
   compilar. **Decidido de otra manera** (Eduardo, 26-sep): no se rechazan; la `native` se SUSPENDE (abajo).
3. **Un builtin que llama de vuelta a BP**: el lazo de la GUI (`__guiRun`, `__guiRunOnce`, `__guiInvokeByName/BySlot`).
   Con la suspensión deja de ser un caso aparte: corre como interpretado (un `__guiRun` no vuelve nunca, tampoco
   interpretado; los demás hilos siguen).

#### La decisión (Eduardo, 26-sep): la `native` se suspende COMO UNA INTERRUPCIÓN, en una pila que le presta el intérprete

> *«Lo que hacemos es que la función native llame a una función opCode y ahí se suspende. Comentaste de pasada en los
> bucles añadir una línea para permitir que el intérprete pudiese tomar el control y así permitir los cambios de
> Thread BPs entre otras cosas.»* — y a continuación: *«¿Y si el código nativo funcionase como una interrupción? Sin un
> stack propio, usa el stack que le ceda el intérprete.»*

**El problema de fondo era REANUDAR.** Salir de un nativo ya se sabe hacer (el `longjmp` de `#186`). Suspenderlo y
volver a él pide que su estado de C sobreviva mientras corren los demás hilos BP, y hoy corre en la pila de C del
worker, que en todas las placas es UNO (`bpvm_run`; el multi-worker, `bpvm_run_smp`, es opcional y sólo en la Pico).
Por eso el puente nativo→BP da error si la función BP cede («no soportado por el puente v1»). Tres salidas:

| | cómo | por qué no / por qué sí |
|---|---|---|
| anidar el planificador | los demás hilos corren ENCIMA de la native, en la misma pila | los hilos sólo vuelven en orden inverso: dos que se esperan dentro de natives se bloquean para siempre, y el mismo programa interpretado no; la native dejaría de ser una aceleración transparente |
| la native reentrable | el emisor la convierte en máquina de estados: guarda sus locales en el marco BP y vuelve a entrar con un `switch` | C puro y probado entero en el PC, pero la complejidad queda en el emisor para siempre: expresiones partidas, y cada llamada a otra native o a BP convertida en continuación |
| ✅ **una pila por hilo, PRESTADA** | la de Eduardo: como una interrupción, el contexto se guarda en la pila de la native, y cambiar de hilo es cambiar el puntero de pila | **la native no cambia**, y se puede suspender en cualquier sitio |

**Cómo funciona:**

- Al entrar en una native, el intérprete le **presta un trozo de su memoria** como pila de C y la llama sobre ella
  desde un **trampolín** que queda en el fondo; cuando la native vuelve, recupera el trozo. Sólo pagan los hilos que
  están DENTRO de una native. No puede ser la pila BP del propio hilo: los hilos lanzados tienen 2 KB
  (`BPVM_THREAD_STACK_BYTES`; el principal, 16 KB).
- **Suspender** es guardar en esa pila los registros que el C obliga a conservar, volver a la pila del intérprete y
  dejarle el hilo como a cualquier otro que cede. **Reanudar** es restaurarlos: la native sigue sin enterarse, como
  tras una interrupción. Puede reanudarse en otro worker: la pila es del hilo, no del worker.
- **Los puntos de suspensión**:
  - la **línea del bucle**, en el salto hacia atrás: «si el intérprete quiere el control —el quantum se acabó, un
    `kill`, mañana un breakpoint—, cedo»;
  - un **builtin que deja el hilo bloqueado**: el helper lo ve en `tc->status` y suspende. **No hace falta una lista de
    los que ceden** —la decisión (a) de más abajo desaparece—;
  - una **función BP llamada desde la native**: el intérprete anidado corre en la pila prestada, así que la función BP
    puede bloquear como cualquiera. Se acaba la restricción v1 del puente;
  - y una native que llama a otra: es una llamada de C normal dentro de la misma pila.
- El **límite de fallos** de `#186` y el **contexto de la llamada** (`cc->sp_p`/`bp_p`, que hoy apuntan a variables
  del marco del worker, que ya no existirán al reanudar) pasan a colgar del HILO, en el trampolín.
- El **GC** corre en la pila del intérprete y no en la prestada —su marcado es recursivo: una lista larga es una
  recursión igual de honda—, y mira además las pilas prestadas de las natives suspendidas y sus registros guardados,
  con el mismo escaneo conservador de `#302`. Hoy da por hecho que ninguna native está a medias («nunca a mitad de un
  native», `heap.c`): eso deja de ser cierto y pasa a estar cubierto.
- Un **canario** en el fondo de la pila prestada, mirado en cada cambio de contexto: si se desborda, un error con
  nombre, no memoria pisada en silencio. El tamaño, a medir: `gcc -fstack-usage` lo da por función.

**Lo que cuesta, y se acepta**: el cambio de contexto es **ensamblador**, unas 20 líneas por ABI —ARMv8-M con FPU
(Pico 2, Metro, STM32U5), RV32 sin coma flotante (C3, C6, C5), RV32 con ella (P4) y x86-64 (el PC)—, porque desde C no
se puede mover el puntero de pila (la interrupción lo hace con el hardware y el RTOS). Es lo único de todo esto que
sólo se prueba en su placa: en la tanda, con una prueba de estrés (dos hilos, flotantes vivos a través de cada
suspensión). Todo lo demás es C común, probado en el PC con la variante x86-64.

**Lo que resuelve además**: dos de los cabos sueltos del AOT (§«Los cabos sueltos del AOT»): parar una native (`kill`)
y el cambio de hilo dentro de ella. Y deja puesta la base de los otros dos —la línea del error y el breakpoint—: es la
misma marca que propuso Eduardo el 24-ago.

**Lo que NO cambia**: las intrínsecas cuyo cuerpo no es `CALL_BUILTIN n` sino un opcode (`newByteArray` →
`NEWARRAY_I8`…) siguen con su traducción propia; `print` (`#558`) y `len` (por el tipo), igual.

**Decisiones:**

- ~~**(a) Dónde se apunta que un builtin cede el hilo**~~: ya no hace falta; con la suspensión, el helper lo ve en el
  estado del hilo (Eduardo, 26-sep).
- **(b) La lista a mano** (~~abierta~~ **decidida y hecha**, abajo: la opción 3, `4bfdb1a8`): tirarla —todo por el despachador; paridad por construcción, a cambio de pasar
  cada llamada por la pila BP y el `switch`— o dejar los helpers tipados de las más calientes, ahora que `test-aotstr`
  los vigila. Propuesta: **medir primero** (`charAt` en un bucle nativo por las dos rutas; es también el «cronometrar
  nativo contra interpretado» que H5 tiene pendiente) y tirarla salvo donde el coste se vea. Los huecos de la tabla se
  quedan: la ABI no quita, sólo añade.

**Cómo se prueba**: en el PC, una prueba que fuerza la suspensión en CADA salto hacia atrás, con dos hilos y
flotantes vivos (salida igual a la interpretada); un `sleep` dentro de una native mientras otro hilo avanza; una
función BP que bloquea llamada desde una native; el `kill` de una native en un bucle; una excepción de un builtin
dentro de un nativo, atrapada en BP; `make test-xmodnat` y un ejemplo con `Math.sin` en una `native`; `test-aotstr`
(la lista entera, AOT == interpretado; desde la opción 3, por las primitivas compartidas). En las placas, la misma prueba de estrés y la tanda con
`aot`.

**Por pasos que cierran algo:**

- **P2b.0** ✅ (26-sep, en la Pico 2) — medir la línea del bucle. Resultado abajo: **un contador en registro, que
  sólo mira el indicador cada 1024 vueltas**.
- **P2b.1** — la suspensión en el PC: la pila prestada, el cambio de contexto x86-64, el trampolín con `#186`, el GC,
  reanudar y el `kill`; la línea del bucle en el emisor; las pruebas de arriba.
- **P2b.2** ✅ (`5f0cc4f1`) — el despachador: el helper `call_builtin` (los argumentos por sus bits en palabras de
  64 bits; lo que el builtin deja, comparado con su tipo), la marca de `builtin_throw` (la excepción se deja en el
  hilo y sale por el límite de fallos; el puente la apaga mientras corre BP anidado), la suspensión cuando el builtin
  bloquea, el respaldo donde no se puede suspender (los cinco que ceden, rechazados con nombre ANTES de ejecutarlos) y
  el camino genérico del emisor para toda intrínseca `CALL_BUILTIN n`, global y de otro módulo. `make test-aotbuiltin`
  8/8. La prueba del `sleep` tuvo que cambiar: sin suspender, el hilo se dormía después, en el primer builtin ya
  interpretado, y la salida salía igual; ahora la native lee tras el `sleep` una global que sólo pone el otro hilo.
  Destapó `#572` (los identificadores de BP chocan en el C del AOT).

**La lista a mano, medida en el PC** (5 millones de `charCodeAt` en un bucle nativo; dos pasadas, estables): por el
helper tipado, **44 ms**; por el despachador, **96 ms**; interpretado, **257 ms**. Para un builtin diminuto en un bucle
apretado, el camino genérico cuesta el DOBLE que el tipado, y sigue siendo 2,7 veces más rápido que interpretado.
Tres salidas: (1) tirar la lista (paridad por construcción, la mitad de rápido en esos casos); (2) dejarla como está,
vigilada por `test-aotstr`; (3) **dejarla, pero que cada helper tipado llame a LA MISMA función que su builtin** —una
implementación y dos entradas—: la velocidad del tipado y la paridad por construcción. Propuesta: la (3). Decide
Eduardo.

**Decidido: la (3)** (Eduardo, 26-sep: «la 3»). **Hecho** (`4bfdb1a8`, `fdfb2398`): `src/bpvm_prims.h` y, en `builtins.c`,
seis primitivas —`str_len`, `char_code_at`, `char_at`, `substring`, `int_to_string`, `nuevo_array`— a las que llaman los
builtins (`STRLEN`, `CHAR_CODE_AT`, `CHAR_AT`, `SUBSTRING`, `INT_TO_STRING`, `NEW_INT_ARRAY`, `NEW_REF_ARRAY`), los
opcodes `NEWARRAY*` y los helpers tipados; `array_length` llama a `bpref_arr_len`, la de `OP_ALEN`. Cada entrada sólo
pone lo suyo: de dónde saca los argumentos (la pila BP o los de C) y cómo lanza (`builtin_throw`, `BPVM_RT_THROW_TEXTO`,
el límite de fallos). `substring` recibe el HANDLE del origen y lo vuelve a resolver tras reservar: quien llama lo
mantiene vivo (el builtin en la pila, #350; la native en sus variables). De paso, lo que hacía la mejor copia lo hacen
todas: tras registrar se comprueba la tabla de handles (#430) —cuatro builtins y dos helpers empujaban un `null`
callado—.

**Lo que cambia en la red.** Un fallo DENTRO de una primitiva ya no lo ve `test-aotstr` —el AOT y el intérprete se
mueven juntos— y lo caza la paridad contra miVM; `test-aotstr` vigila las ENTRADAS. Las dos cosas, saboteadas: las seis
primitivas a la vez, cada una por un camino que la prueba ejercita → cambian diez líneas de la salida interpretada y el
AOT da lo mismo (verde), y la paridad de `CharAt` y `ArrayNegativo`, en rojo; un helper y un builtin con otro argumento
→ rojo. El builtin salió verde la primera vez: `NatStr` no tenía ningún `substring` con inicio positivo. Ahora sí, y
`len` de una cadena: 17 comprobaciones.

**La pila, medida** (la orden del firmware de la Pico con `-fstack-usage`, antes y después). El primer intento dejaba
el `case` de cada `NEWARRAY` con su búfer del texto y lanzaba con `BPVM_RT_THROW("%s", em)`: los dos búferes vivos a la
vez, y como el marco del intérprete lo fija el `case` más gordo, **`bpvm_interp_run_quantum` pasó de 248 a 336 B**.
Con `BPVM_RT_THROW_TEXTO` (lanza el texto ya hecho), 256: los 8 que quedan son los argumentos de la llamada de nueve
parámetros. `bpvm_call_builtin`, igual (2096). Los helpers del AOT crecen entre 48 y 136 B mientras se les llama
(ahora son dos marcos, el suyo y el de la primitiva) —en la pila de la native, que `P2b.3` tiene que dimensionar—.

**Lo que queda fuera** (no es la lista a mano, son OPERADORES): los helpers de `a[i]`, `a[i] := v` y el `+` de cadenas
también son copias —de opcodes—, y éstas SÍ se habían ido: `#573` (un índice fuera de rango en una native lanzaba otro
texto que interpretado; `a + b` sin memoria, un `null` callado). **Cerrada el mismo día** (`de04b689`) con la misma receta:
`bpvm_prim_texto_indice`, el texto de cada opcode de acceso en UN sitio, que usan el opcode y el helper; el concat
reserva con `bpvm_prim_nuevo_array`. `test-aotarr` 21/21 y `NatArr` en la paridad. La igualdad de cadenas no hacía falta:
`string_eq` y `__strequals` ya coincidían (`null` sólo es igual a `null`).
- **P2b.3** — ~~el cambio de contexto de ARM y de los dos RISC-V~~ (**rediseñado el 27-sep**: la native corre en un
  hilo de la plataforma, ver §P2b.3 abajo), la cascada de la ABI 9 (los `.mdn`, los packs, las imágenes) y la sesión
  de placas, junto con lo que ya espera placa.

#### P2b.0 — lo que cuesta la línea del bucle, MEDIDO (26-sep, Pico 2, RP2350 a 150 MHz)

Un módulo con dos bucles apretados, llamados 200 veces con 50 000 vueltas —10 millones por bucle, sin desbordar un
`integer`—: `sumaHasta` (el de `NatV7`: `t += i`) y `hashHasta` (`h := (h * 31 + i) mod 1048576`, que gcc no puede
convertir en una fórmula; `-Os` tampoco convirtió la suma). El C que emite el AOT, retocado a mano en tres variantes,
compilado con la receta de la placa (`ARMV8M_MAIN_SOFTFP`) y subido como `.mdn` suelto. La rama «cedo», que no se toma,
LLAMA de verdad a un helper de la tabla, para que gcc reparta los registros como lo hará con la línea real.

| | `sumaHasta` | ciclos/vuelta | `hashHasta` | ciclos/vuelta |
|---|---|---|---|---|
| interpretado | 72 279 ms | ≈1084 | 96 406 ms | ≈1446 |
| nativo, como hoy | 404 ms | 6,1 | 874 ms | 13,1 |
| + mirar el indicador en CADA vuelta | 873 ms (+116 %) | 13,1 | 1276 ms (+46 %) | 19,1 |
| + **contador en registro; el indicador, cada 1024 vueltas** | **539 ms (+33 %)** | **8,1** | **1008 ms (+15 %)** | **15,1** |

Dos ejecuciones por variante, iguales al milisegundo. Leer el indicador cuesta **unos 7 ciclos por vuelta** (una
`ldrb` y su comparación: en el bucle más apretado, más que el propio bucle); el contador, **2** (un `subs` que ya
pone los flags, y el salto). **Decisión técnica**: el contador; la latencia de respuesta, 1024 vueltas (≈ 55 µs en
`sumaHasta`), muy por debajo de un quantum. Y el orden de magnitud que no se pierde: con la línea, el nativo sigue
**134 y 96 veces** más rápido que el intérprete en estos dos bucles (179 y 110 sin ella) — es también el primer dato
del «cronometrar nativo contra interpretado» que H5 tiene pendiente.

⚠️ **Un tropiezo del instrumento, para no repetirlo**: la primera tanda leía el indicador de `vm->memory + 0x40`,
supuesto a cero por estar en la zona reservada, y en la Pico NO lo está: la rama «cedo» se tomaba en cada vuelta
(26 veces más lento, que no era el coste de la línea sino el de 10 millones de llamadas). Se vio porque el número era
absurdo. El indicador de la medida buena es un `static const volatile` del propio `.mdn`, que vale 0 seguro.

#### P2b.1 — el plan concreto, leído el código (26-sep)

Tres cosas que el diseño de arriba no sabía y el código sí:

- **El límite de fallos (`#186`) y el contexto de la llamada son UNO POR WORKER**, a propósito: meterlos en cada hilo
  infló la memoria en `#185`. Con la suspensión, un worker tiene varias natives vivas (una corriendo, otras
  suspendidas). Van DENTRO del bloque de la pila prestada, que sólo existe para los hilos que entran en una native:
  `bpvm_thread_t` gana un puntero, no un `jmp_buf` por hilo.
- **Anidar ya estaba roto — `#571`**: `aot_call_guarded` guarda `prev_armed` pero no el `jmp_buf`, así que una native
  que llama a BP que llama a otra native pierde su límite cuando la interior vuelve; si después lanza, el `longjmp`
  salta a un marco muerto. **Reproducido en el PC**: interpretado, «atrapada: charAt: idx fuera de rango 11»; con el
  AOT, el proceso muere en silencio tras la primera línea. Cada invocación tendrá su propio límite, y el programa de la
  reproducción será la prueba.
- **En el PC (MinGW, ABI de Windows x64), el TIB guarda los límites de la pila** y el `longjmp` de MinGW deshace
  marcos comprobándolos: el cambio de contexto los pone a los de la pila prestada, como hacen las fibras de Windows.
  Los registros que conserva son los que la ABI obliga (en Windows x64, también `xmm6`–`xmm15` y el MXCSR).

**Los pasos de P2b.1**, cada uno con sus pruebas en el PC:

- **a.** ✅ (`1507d734`) El cambio de contexto x86-64 (Windows) y su prueba (`make test-ctx`): 100 000 saltos con 8
  enteros y 6 `double` vivos a cada lado, un `setjmp`/`longjmp` y un marco de 40 KB DENTRO de la pila prestada. Tres
  sabotajes cazados; el del TIB, con el mismo `STATUS_BAD_STACK` que se esperaba.
- **b.** ✅ (`4b1414cc`) El bloque de la native y el trampolín: toda native corre en su pila prestada (256 KB en el
  PC, memoria del programa: la guarda de fin de RUN la ve volver). El GC dice en su diagnóstico QUÉ pila escanea
  («prestada» o «del worker»): así se comprobó en el artefacto. `#571` se cerró antes y aparte (`99b02ddb`), porque
  valía ya para todas las placas.
- **c.** ✅ (`be011deb`) Suspender y reanudar (`make test-aotcede`, 7/7 con el AOT igual que interpretado): una native
  guarda una cadena en C y llama a una función BP que espera a otro hilo, que reserva sin parar con GC en cada
  reserva; y otra se suspende y AL VOLVER lanza. El GC escaneó 246 veces la pila de una native suspendida. Sabotajes:
  sin escanear las suspendidas, la cadena se recicla y su hueco lo reusa la basura del otro hilo (`x0123…/7` en vez
  de `100000/7`, con status=OK); sin suspender, el error de antes; sin recuperar la TLS, se rompe al reanudarse.
- **d.** ✅ La línea del bucle en el emisor (al PRINCIPIO del cuerpo de `while`, `do … loop` y `for`: cuenta también
  las vueltas que acaban en un `continue`) y el helper `punto_control` (ABI 9): cede si hay un `kill` o si la native
  lleva corriendo su rebanada (`BPVM_NATIVA_REBANADA_MS`, 5 ms: el intérprete mide su quantum en operaciones, una
  native no tiene operaciones que contar). `make test-aotexpropia` (una native en un bucle que sólo termina si otro
  hilo corre: 4/4) y `make test-aotkill` (el `kill` de una native en un bucle sin fin: 99 ms); saboteado el helper,
  las dos se cuelgan y las corta el tope.

**Lo que quedó por el camino:**

- **Donde no hay cambio de contexto** (las placas, hasta P2b.3) la línea del bucle no puede ceder, pero un `kill` sí
  para la native: el helper LANZA por el límite de fallos de `#186`. En las placas el `kill` lo levanta el hilo `io`
  por su cuenta, así que con la imagen de la ABI 9 el `kill` de una native en bucle funcionará antes incluso del
  ensamblador de P2b.3.
- **La recursión sin bucles**: una native recursiva que no tenga ningún bucle no pasa nunca por la línea (p. ej. un
  `fib` recursivo). Si hiciera falta, la marca iría también a la entrada de la función; hoy no se ha hecho.
- **La ABI 9, abierta**: desde ya, todo `.mdn` que se genere —el IDE, la tanda con `aot`— es de la 9, y las placas,
  con la 8, lo RECHAZAN con nombre y la función corre interpretada, hasta que se regraben con P2b.3. P2b.2 añadirá
  `call_builtin` a la misma 9, sin subirla otra vez: ningún `.mdn` de la 9 habrá salido todavía.
- **El arnés de las pruebas del AOT** (`fixture`) dejaba correr una prueba con el `.mod` RANCIO cuando el `.bp` no
  compilaba (`3f71c5b6`): se vio al escribir la de P2b.1.c.

#### P2b.3 — REDISEÑADO (27-sep): la native corre en un hilo de la plataforma, y se pasan el testigo

> Eduardo, al ver lo que complicaba el ensamblador en las placas: *«FreeRtos hace un cambio de contexto cuando cambia
> de Thread, puedes mirar como lo hace»*, y luego: *«si es muy complicado podemos pensar en otras alternativas. Por
> ejemplo, pensar en la ejecución de código nativo como un Thread BP, con su propio stack. Y de vez en cuando devuelve
> el hilo de ejecución a otros Thread BP. Ese es un mecanismo que ya existe.»* Decidido el mismo día: *«creo que
> simplifica el diseño ya que la estructura ya existe y me parece mucho más robusto»*.

**Lo que complicaba el camino del ensamblador** (mirado el 27-sep en los tres puertos de FreeRTOS que usamos). El
ensamblador era lo de menos —las mismas ~20 líneas que el `PendSV` de FreeRTOS: r4–r11 y s16–s31 en ARM—. Lo difícil
era que el cambio lo hacíamos NOSOTROS, dentro de UNA tarea, y cada sistema vigila que la tarea use su propia pila:

- **ARM** (Pico 2, Metro, DK2, Nucleo): el `PSPLIM`, el límite de pila por hardware del Cortex-M33, era fácil —FreeRTOS
  lo guarda en el marco de cada tarea: bastaba con ponerlo—; pero la comprobación de desborde de FreeRTOS
  (`configCHECK_FOR_STACK_OVERFLOW 2`, en las dos familias) mira además que el puntero de pila guardado no quede por
  debajo de la pila de la tarea, y que saltara o no dependía de dónde cayera la pila prestada en la RAM.
- **ESP32 RISC-V** (C3, C5, C6, P4): el vigilante de pila por hardware del IDF (`CONFIG_ESP_SYSTEM_HW_STACK_GUARD`, el
  que cazó `#543` dos veces) toma sus límites del TCB (`pxStack`/`pxEndOfStack`) al salir de CADA interrupción: con
  la pila prestada fuera, salta en la primera. Las salidas eran apagarlo o reescribir esos campos del TCB.
- **El P4**, además: el IDF guarda la FPU de cada tarea al fondo de su pila y, al reservar ese hueco, comprueba el
  puntero de pila guardado (`pxPortGetCoprocArea`: «Stack overflow while saving coprocessor context») —justo los
  campos que habría que reescribir—; y con PSRAM, `malloc` pone los bloques de más de 2 KB en ella, donde el IDF
  prohíbe tener la pila mientras se escribe la flash (`assert(esp_task_stack_is_sane_cache_disabled())`).

Todo tenía arreglo, pero eran parches por plataforma, en tripas del IDF, que sólo se prueban en la placa.

**El diseño nuevo.** La native corre en un **ejecutor**: un hilo de la plataforma —una tarea de FreeRTOS con su propia
pila; en el PC, un pthread—. El intérprete y el ejecutor **se pasan el testigo**: sólo corre uno de los dos, así que
la VM sigue sin concurrencia. El intérprete reparte los hilos BP como siempre, y la native le devuelve el control en
los mismos puntos de P2b.1: la línea del bucle, un builtin que bloquea, una función BP que espera. El cambio de
contexto lo hace el sistema, con todas sus vigilancias en regla, igual en todas las placas; y el PC ejecuta el mismo
código, así que la cascada se mantiene.

- **El testigo**: dos semáforos binarios por ejecutor; pasarlo es dar el del otro y esperar el propio.
- **Los ejecutores, en un depósito de la VM**: se crean al entrar en una native y se reutilizan. Un hilo BP tiene uno
  sólo mientras está DENTRO de una native —corriendo o suspendida—: `tc->nativa` es ese ejecutor, y al acabar la
  native vuelve al depósito. Hay tantos como natives a medias a la vez: casi siempre, uno.
- **De P2b.1 cambia poco**: donde se saltaba de pila (`bpvm_ctx_saltar`) se pasa el testigo. El límite de fallos
  (`#186`) y el contexto de la llamada ya colgaban de la native, y sus vistas por worker se siguen cambiando en cada
  paso (en las placas son globales).
- **El GC corre en el worker**: si una reserva lo dispara DENTRO del ejecutor, éste le pasa el testigo con el encargo,
  y el marcado —recursivo— se hace en la pila del intérprete, como pedía el diseño de P2b. La pila del ejecutor se
  escanea como la de una native suspendida: desde su puntero de pila, con los registros volcados por `setjmp`, hasta
  su techo.
- **Al acabar el programa**, un ejecutor parado a mitad de una native se despierta con la orden de terminar y salta a
  su fondo (`longjmp`), abandonando esos marcos, como P2b.1 abandonaba la pila prestada.
- **Sin memoria para un ejecutor**, la native corre directa en el worker, como hoy en las placas (no puede cederse; un
  `kill` la para lanzando), y se dice una vez.
- **La plataforma gana dos piezas**: el semáforo binario, y un hilo con su tamaño de pila, a la prioridad de quien lo
  crea (como `io`), con su marca de agua. Tres implementaciones: pthread (PC); FreeRTOS (Pico y STM32:
  `xTaskCreateStatic` con memoria de `malloc`, porque el heap de FreeRTOS es de 24–32 KB —la STM32 necesita
  `configSUPPORT_STATIC_ALLOCATION`—); y ESP32 (el IDF pone las pilas de las tareas en la RAM interna).
- **Se retira el ensamblador x86 de P2b.1** (`bpvm_ctx`) y su prueba: ya cumplió —probar el diseño en el PC—, y si se
  quedara, el PC no probaría lo mismo que las placas.

**Lo que cuesta**: cada llamada a una native son dos cambios de tarea —estimado: 5–10 µs en las placas, frente a menos
de 1 µs con ensamblador; se mide—. Una native con un bucle no lo nota; una diminuta llamada miles de veces desde
código interpretado, sí. Si la medida lo pidiera, una native sin puntos de cesión (sin bucles, sin builtins, sin
llamadas a BP: el compilador lo sabe) podría seguir corriendo directa, pero eso después de medir.

**La pila del ejecutor puede ser PEQUEÑA** (Eduardo: *«en freertos el tamaño del stack de los Threads puede ser más
pequeño, no tiene que ser de 8K»*). Dentro corren la native, sus helpers y lo que llame —el GC no: va al worker—, así
que el tamaño se MIDE con la marca de agua de la tarea, que el sistema da gratis, y se fija por familia con lo
medido. Lo que ya se sabe que la infla: `bpvm_call_builtin`, con un marco de **2 KB** (2096 B en la DK2), porque su
`switch` reserva para cualquier builtin los búferes del `case` más gordo; cualquier builtin llamado desde una native
por el despachador los paga. Si la marca de agua lo pide, adelgazar ese `switch` —sacar a funciones aparte TODOS los
`case` de búfer grande, midiendo con `-fstack-usage` antes y después: sacar uno solo SUMA pila— beneficia además a la
pila del worker, que hoy paga esos 2 KB en cada builtin.

**Por mirar**: el depurador parando en código BP llamado desde una native, que ahora corre en el ejecutor (la Pico lee
el USB del depurador desde la tarea del REPL); si molesta, con el depurador las natives corren directas.

**Por pasos:**
- **P2b.3.a** — las dos piezas de la plataforma y `make test-ejecutor` (ida y vuelta con estado vivo, un `longjmp`
  dentro, un marco grande, terminar uno parado), con **lo que cuesta pasar el testigo en el PC**: la primera medida.
- **P2b.3.b** — las natives en ejecutores y el GC en el worker; fuera `bpvm_ctx`; en verde `test-aotcede`,
  `test-aotexpropia`, `test-aotkill`, `test-aotbuiltin`, `test-aotanida` y `compat.sh check`; y lo que cuesta una
  llamada a una native, antes y después.
- **P2b.3.c** — los firmwares (todas las familias compilan; el tamaño de la pila del ejecutor, por familia) y el
  HardFault de la STM32, que hoy se cuelga MUDO: un desborde de la pila de un ejecutor ahí sería un cuelgue sin nada en
  el log (la Pico habla desde `#447`).
- **P2b.3.d** — la sesión de placas, con el OK de Eduardo: las imágenes, las pruebas `Nat*` en la tanda con `aot`, la
  marca de agua y lo que cuesta una llamada en cada familia, y lo que esperaba placa (la C6 con `#574`, el ADC de la
  DK2, el NeoPixel de la Metro, `DriverSpi` en la C5 y la C6, SQLite y la Gui en el P4).

#### P2b.3.a y b, en el PC (27-sep)

- **a** (`0b997502`) — las dos piezas de la plataforma en sus cuatro implementaciones (el PC; la STM32, en
  `src/platform_freertos.c`; la Pico, en su copia; las ESP32) y `make test-ejecutor`. Lo que cuesta pasar el testigo
  en el PC: **2,3 µs** (Windows, pthread).
- **b** (`138e8626`) — las natives en ejecutores y el GC en el worker; fuera `bpvm_ctx` y `test-ctx`. `compat.sh
  check` verde. El diagnóstico enseña el GC escaneando la pila de la native suspendida EN SU EJECUTOR (246 veces en
  `test-aotcede`, las mismas que en P2b.1) y los GC pedidos desde dentro, servidos en el worker; dos sabotajes
  cazados. Una native diminuta llamada desde un bucle interpretado: **5,2 µs** por llamada (0,10 con el
  ensamblador; 0,05 interpretada).

**Las natives DIRECTAS** (Eduardo: «Hazlo ahora»; `f8146e4d`). Una native sin puntos de cesión no puede
suspenderse aunque tenga ejecutor: corre directa, en la pila en la que se esté, como todas antes de P2b, y se ahorra el
testigo. Lo decide el emisor sobre el C que EMITE (`AotCEmitter.nativesDirectas`): es directa si ni su C ni el de
ninguna native del módulo a la que llame —transitivo, recursión incluida— llama a `punto_control`, `call_builtin`,
`call_bp_i32`, `call_method_i32` o `pack_sym`. Transitiva por la pregunta de Eduardo: *«la rutina en sí puede ser
pequeña pero puede llamar a una mucho más grande con bucles»*. Y sobre el C y no sobre el `.bp`: si el emisor gana
otro sitio que cede, no se escapa. La marca viaja en el NOMBRE del thunk (`thunkd_`, por donde `MdnPack` ya reconoce
los thunks) y, en el `.mdn` **v3**, en el bit alto de `thunk_offset` (ningún código de micro mide 2 GB, y los seis
lectores que dan por hecho símbolos de 36 B no cambian); un `.mdn` v2 se sigue cargando y manda todo al ejecutor;
un cargador de la v2 rechaza la v3 por su versión, con nombre.

Medido: la native diminuta, ahora directa, hace las 200 000 llamadas en **9,7 ms** —0,05 µs—, más rápido que con el
ensamblador (20,6: P2b.1 cambiaba de pila en CADA llamada) y que interpretada (10,3). Pruebas: `test-aotdirecta` (en
el C generado, 4 directas y 5 al ejecutor —entre ellas la de Eduardo, `pequena`, sin bucle, que llama a una con
bucle, y otra dos niveles más arriba—; y 9/9 igual que interpretado) y `test-mdnv3` (el cargador: la marca llega al
registro y no se cuela en la dirección del thunk; la v2 sin marca; la v4, rechazada). Sabotajes: sin la regla
transitiva, ROJO (`pequena` y `llamaPequena` salían directas); sin limpiar el bit del offset, ROJO. Y el `.mdn` ARM
real de `NatDirecta`, enlazado con `mdn.ld`, sale con las cuatro directas marcadas.

⚠️ **Una falsa alarma, para no repetirla** (y ya estaba en FICHAS): un `.mdn` hecho a mano desde el `.o`, SIN el
enlace de `bpgenvm-c/aot/mdn.ld`, choca con `.rodata` —los tipos de `call_builtin`, el nombre que busca el puente,
un literal de BP—. El pipeline enlaza SIEMPRE, y `mdn.ld` funde `.rodata` en `.text` (`#428`).

**La línea del bucle se queda** (decidido por Eduardo, 27-sep: *«OK, mantenemos la línea del bucle»*). Él mismo había
planteado quitarla —*«ahora ya no necesitamos insertar código para permitir la interrupción del código nativo, el SO lo
hace por nosotros»*—, pero con el testigo no se cumple: mientras la native corre, el intérprete espera el testigo, y
el sistema reparte la CPU con otras tareas (`io`), no con otros hilos BP. Para eso la native y el intérprete tendrían
que correr A LA VEZ —dos hilos tocando el heap, el GC parando la native donde esté: el mundo del SMP—. Y los dos
costes van en ejes distintos: el testigo, por llamada; la línea, por vuelta (+15–33 % en los bucles más apretados,
P2b.0). La línea da la rebanada, el `kill` y, mañana, el breakpoint, igual en todas las plataformas.

#### P2b.3.c y la sesión de placas (27-sep): la DK2, la Metro, el P4 y la Nucleo

**c** (`abb21836`, `9f7227f5`): el informe de un HardFault de `#447` sube al común (`src/bpvm_fallo_cm33.c`) y lo usan
la Pico y, por fin, las STM32 —que tenían el bucle mudo de CubeMX—, con lo que no decía: **STKOF** (el desborde de la
pila de una tarea por su límite PSPLIM: justo el de un ejecutor), STKERR/UNSTKERR, la tarea en curso y su pila. Y los
fallos FATALES (el HardFault, el canario de FreeRTOS, la parada del RTOS) escriben en el registro aunque el log esté
apagado: el contrato de `#423` ya lo decía del arranque.

**En las placas** (con el OK de Eduardo en cada grabación; `log=1` sólo mientras se medía):

| | DK2 (STM32U5, 160 MHz) | Metro (RP2350, 150 MHz) | P4 (RISC-V con FPU, 360 MHz) | Nucleo (STM32U5, 160 MHz) |
|---|---|---|---|---|
| ronda de V7 (20) | **20/20** | **18/18** (sin pantalla) | **20/20** | **18/18** (sin pantalla) |
| llamada con ejecutor (`NatCoste`, 20 000) | 206 → 929 ms: **~36 µs** | 170 → 667 ms: **~25 µs** | 86 → 436 ms: **~17,5 µs** | 161 → 783 ms: **~31 µs** |
| pila del ejecutor, pruebas | 3,3 KB | 3,2 KB | 3,5 KB | 3,3 KB |
| pila del ejecutor, SQLite (antes de la regla del pack) | 4960 B de 6144 | 4960 B | **5516 B** (628 libres) | — (grabada ya con la regla: sin ejecutor) |

Las seis pruebas de natives de la ronda (`NatCede`, `NatExpropia`, `NatBuiltin`, `NatAnida`, `NatDirecta`,
`NatCoste`), IDÉNTICAS al PC en las tres: por primera vez, natives que se suspenden y se reanudan EN PLACA (en el puente
a BP, en la línea del bucle, en un `sleep` por el despachador). El cargador lee la marca v3 («9/9 thunks registrados
(4 directas)»). Y lo que esperaba placa: el ADC de la DK2 (los 17 canales; los errores con nombre), el NeoPixel de la
Metro con el `__npInit` nuevo («el pin 99 no existe en rp2350b»; el ciclo de colores), SQLite y la Gui regrabados en
el P4 y SQLite en la Metro (sus `.mdn` eran de la ABI 8: el cargador los rechazaba con nombre).

**Lo que la medida cambió** (Eduardo, «OK, adelante», `b9b3f0ac`): **llamar a un pack no cede** —su código sólo ve la
BIOS, y ninguna ranura ejecuta BP—, así que las natives de SQLite y de Gui2 son DIRECTAS: SQLite corre en la pila del
worker como antes de P2b.3 (en las tres placas, «16/16 thunks (16 directas)» y `SqlDemo` sin crear ejecutor); y la pila
del ejecutor, a **8 KB**. Los packs, regenerados (`.mdn` v3) y regrabados; en el P4 y la DK2, con `PACK_FORMAT` (sus
zonas no daban para los nuevos junto a los borrados) y en el orden de siempre (el reparto de RAM de G1: `mini`, SQLite,
Gui en la DK2).

**El testigo, caro en las placas** (17–36 µs por llamada, con semáforos binarios y `configASSERT`): lo pagan sólo las
natives que pueden ceder. Si hiciera falta abaratarlo, las notificaciones directas entre tareas de FreeRTOS son la
siguiente perilla; hoy no se toca.

**El P4 y sus 32 MB** (pregunta de Eduardo): la flash física es de 32 MB (el INFO mide 33 554 432 B), pero la imagen usa
16: su caché sólo mapea los primeros 16 MB, y los packs se ejecutan en sitio (el revert `2dc12d5b`, 18-ago). El
arreglo —los packs abajo, el FS arriba— es «los 32 MB del P4» de H7. Tras regrabar, su zona de packs queda en 1,4 MB
libres (tras un `PACK_FORMAT`).

**La Nucleo** (con el OK de Eduardo; COM5): la imagen de HEAD, `softfp` —su primer arranque con la convención de
`G5.b0` y con el adaptador de packs de la familia—, arranca a la primera («bios: lista (47 ranuras, v8)», el ancla en su
sitio). La ronda, 18/18, con `FloatNat` 23/23 nativo; el testigo, ~31 µs por llamada (la DK2, al mismo reloj, 36); la
pila del ejecutor, 3296 B de 8192, como la DK2. Y la prueba que `G5.b0` le había dejado: `packRam=256` en el ENV («vm:
256 KB … | packs 256 KB delante»); el `mini` y SQLite podados y sellados para ARM (24 y 456 KB, en `0x08182000` y
`0x08188000`; la RAM de SQLite en `0x20000010`, detrás de la del `mini`); al primer INFO, los dos motores cargados
(«mini: TODO OK -> 0», que lee `packRam` del ENV; «sqlite: initialize OK» y la API `'SQLI'` publicada); y `SqlDemo`
entero con el `.mdn` del pack —«16/16 thunks (16 directas)», sin ejecutor—. `Core.mod` lo sirve el `mini` en sitio
(`#468`).

**Quedan** de P2b.3.d: la S3 (sólo la imagen: no tiene AOT), la C3, la C5 y la C6 (RISC-V sin FPU), y lo que esperaba
placa en ellas: la C6 con `#574`, `DriverSpi` en la C5 y la C6 — en la ficha suelta `#581` desde que H5 se cerró
(27-sep), como el resto de lo que quedaba en su fila (`#577`–`#586`).

#### Las conversiones estrechas en una native (27-sep, la opción A de Eduardo)

`byte(x)`, `int8(x)`, `word(x)` e `int16(x)` (y `short(x)`, que el parser deja en `int16`) dentro de una native
caían en «AOT: call a función desconocida 'byte'» —con un texto que listaba cinco builtins y daba `#169` por
pendiente, rancio desde P2b.2— y la native no se traducía (el compilador lo trata como un error de la native). En el
intérprete no son una llamada sino un opcode (`OP_I32_TO_U8/I8/U16/I16`, `interp.c`; miVM igual) que comprueba el
rango: dentro, el valor tal cual; fuera, un `RuntimeError` atrapable, «I32_TO_U8: valor fuera de rango 300». Y son la
ÚNICA puerta a un tipo estrecho: `var b: byte := i` no compila («requiere cast explícito: byte(<valor>)»). Como dijo
Eduardo, la conversión con pérdida está siempre en la asignación a un byte o a un short: aparece en pocos sitios.

| | qué | lo bueno | lo que cuesta |
|---|---|---|---|
| **A** | el emisor escribe la comprobación en el C del módulo (`aot_<Mod>_estrecha`) y, fuera de rango, arma el texto con los helpers que ya hay (`string_from_cstr` + `int_to_string` + `string_concat` → `throw_str`) | no toca la ABI del `.mdn`: ni regenerar los packs ni regrabar las placas; cambiarla es cosa del compilador | el texto está dos veces (la VM y el emisor) y algo más de código en cada sitio |
| B | un helper nuevo que llama a la misma función que el opcode | una sola implementación, como la opción 3 de P2b | sube la ABI a 10: regenerar los `.mdn` de los packs y regrabar todas las placas |

**Decidido: A** (Eduardo: *«A nos permite más flexibilidad, si se quiere cambiar algo solamente hace falta modificar
el compilador. También es verdad que genera más código, pero siempre lo podemos cambiar más adelante»*). El texto
duplicado lo vigilan `make test-aotestrecha` —los dos bordes de cada conversión y uno fuera por cada lado, con el AOT
y sin él— y `NatEstrecha` en el corpus de paridad (interpretada, miVM == VM-C).

**Hecho** (`5efb4c76`): `conversionEstrecha()` (los cuatro rangos y textos) y `aot_<Mod>_estrecha` en el prólogo de
cada módulo, `static inline` (sólo cuesta donde se usa). De paso, un estrecho en una concatenación (`"b=" + b`) va
como un integer —el intérprete lo pasa con `INT_TO_STRING` (`MivmEmitter.coerceToString`, L8 v2)— y el texto de
«función desconocida» dice lo que hay: una función del módulo, una intrínseca, una conversión o una clase, y lo de
otro módulo, cualificado. `samples/NatEstrecha.bp`: 23 comprobaciones, 23/23 con el AOT igual que interpretado (el GC
en cada alocación), una de ellas con bucle —lanza desde un EJECUTOR— y las demás directas; con el emisor de antes, el
compilador ni lo compila (ROJO). En placa, sin imagen nueva (la ABI no cambia): `NatNombres` (4/4 thunks nativos) y `NatEstrecha` (7/7) IDÉNTICOS al PC en la DK2, la Metro, el P4 y la Nucleo (`compat/informes/tanda_estrecha_27sep.md`).

#### `#580` (1) — la recursión SIN bucles: medido en la C3 (3-oct, para decidir)

**El problema, con un ejemplo:**

```
native function fib(n: integer): integer
  if n < 2 then
    return n
  endif
  return fib(n - 1) + fib(n - 2)
end fib
```

No tiene bucles, así que nunca pasa por la línea del bucle: ni cede ni ve un `kill`. Y el AOT la marca DIRECTA (sin
puntos de cesión: corre sin ejecutor). En la C3, `fib(40)` nativo son 32 s, y un KILL a los 500 ms no hace nada: la
native acaba, el programa sigue con lo que hay detrás de la llamada y el EXITED llega a los 31,6 s del KILL. Mientras
tanto, los demás hilos, los eventos y la GUI esperan. Le pasa también a una recursiva CON bucles cortos (un
quicksort): el contador de la línea es LOCAL a cada llamada y empieza en 1024 en cada una, así que en lo hondo de la
recursión no llega a cero.

**El mecanismo que propongo: una recursión es un bucle.** La marca va a la ENTRADA de cada native que está en un ciclo
de llamadas entre natives del módulo (`fib` → `fib`; `a` → `b` → `a`); las demás no cambian. El contador no puede ser
local (cada llamada tendría el suyo), así que viaja por un parámetro oculto: un puntero a un contador que vive en el
thunk (donde BP entró en la native) o en la primera native que entra en el ciclo. Cada 1024 entradas llama a
`punto_control`, el mismo de la línea del bucle: cede si lleva 5 ms y para si hay un kill. Es **sólo el compilador**:
no hay campo nuevo en `bpvm_t` ni helper nuevo, así que **la ABI no cambia** (ni packs que regenerar ni placas que
regrabar). Con un punto de cesión la native deja de ser directa y va a un ejecutor, como hoy una con bucle. Lo que cruza
por BP (native → función BP → native) ya cede al llamar a BP y no cuenta para el ciclo.

**Medido** el 3-oct en la C3 (RV32IMC a 160 MHz). El método, el de P2b.0: el C del AOT retocado a mano, compilado con
la receta de la placa y fundido en el `.mod`. Con el C sin tocar, el arnés da el `.mod` del AOT byte a byte. Tres
pasadas por variante, iguales al milisegundo:

| `fib(30)` (2,69 M llamadas) | ms | ciclos por llamada | KILL a los 500 ms de `fib(40)` |
|---|---|---|---|
| hoy (directa, sin marca) | 262 | 15,6 | no para: el EXITED, a los 31,6 s |
| la marca a la entrada, directa | 390 (+49 %) | 23,2 | para a los 45 ms, pero salía RUNTIME_ERROR (→ `#640`) |
| **la marca, con ejecutor** (lo que sería) | **398 (+52 %)** | 23,7 | **KILLED a los 26 ms** |
| una bandera en memoria a la entrada (lo de `#592`) | 448 (+71 %) | 26,6 | — |

- `fib` es el peor caso: toda ella es llamar. Una recursión que trabaje algo en cada llamada paga proporcionalmente
  menos.
- Ceder cada 5 ms cuesta ~2 % (398 frente a 390).
- Ir a un ejecutor cuesta **+30 µs por llamada DESDE BP** (`NatCoste` en la C3: 11 µs directa, 41 con ejecutor). No
  se nota en una recursión larga; sí en una native recursiva diminuta llamada miles de veces desde BP.
- La bandera cuesta MÁS que el contador también a la entrada.

**Tres caminos:**

| | qué | lo bueno | lo que cuesta |
|---|---|---|---|
| **A** (propuesta) | la marca a la entrada de las natives en un ciclo, y van a un ejecutor | para Y cede, como un bucle; sólo el compilador | +49 % en `fib`; +30 µs por llamada desde BP para esas natives |
| B | la marca, pero siguen directas | para (KILLED, con `#640`); sin el coste del ejecutor | no cede: mientras corre, los demás hilos y la GUI esperan, como hoy |
| C | nada; lo dice el manual | nada | una recursiva larga no se puede parar ni deja correr a nadie |

**Y `#592`** (el 2º temporizador) se decide aparte. Aquí sólo se midió lo que costaría mirar la bandera a la entrada
(+11 ciclos, frente a +7,6 del contador). Una bandera que la native pueda leer necesita un campo en el prefijo
congelado de `bpvm_t` o un helper, así que sube la ABI (9 → 10).

**Decidido: A** (Eduardo, 3-oct: *«Como un bucle»*).

**Hecho** (el emisor, `AotCEmitter`):
- **El grafo, del C que sale.** Antes de emitir, una emisión EN SECO del mismo módulo (otra instancia del emisor, con
  `soloGrafo`) apunta cada llamada de C a otra native en el sitio donde la escribe (`llamadasDirectas`): el grafo es
  el del C que sale, con el mismo criterio que `nativesDirectas`. Las natives de un ciclo (`enCiclo`; también `f` →
  `f`) son las `recursivas`.
- **El contador viaja con la llamada.** Esas funciones llevan un parámetro oculto, `int32_t* __bpvm_cr`, en la firma
  y en la declaración adelantada. Una llamada desde dentro del ciclo pasa el mismo puntero. Una que entra desde fuera
  (el thunk, u otra native) pasa uno nuevo, un literal compuesto `&(int32_t){ 1024 }` que vive en el marco de quien
  llama.
- **La marca**, la de la línea del bucle, a la entrada: `if (--*__bpvm_cr == 0) { *__bpvm_cr = 1024;
  punto_control(vm); }`. `nativesDirectas` ve el helper y la native va a un ejecutor.
- **El control**: el C de los 56 módulos con natives del árbol, antes y después. 51 salen idénticos byte a byte; sólo
  cambian los cinco que tienen recursión: `NatDirecta` (`fibR`), `Fibo`, `Bench`, `fibobench` e `intbench`.
- **Las pruebas**:
  - `test-aotkill` gana dos pasadas con `samples/NatRecurKill.bp` (`fib(45)`, recursiva sin bucles), la interpretada
    (el control) y con el AOT: KILLED en 96 ms.
  - `test-aotdirecta`: `fibR` va al ejecutor (3 directas y 6 al ejecutor), y sigue 9/9 igual que interpretada.
  - **Saboteado** el emisor (sin `recursivas`), las dos se ponen rojas: la recursiva acaba sola (`status=OK` a los
    1,1 s) y `fibR` sale directa.
- **La ABI no cambia**: el `.mdn` sigue en la 9, ni packs que regenerar ni placas que regrabar.
- **En la C3** (`d66e19d0`; sin grabar nada, porque la ABI no cambia): las once de natives de la ronda salen IDÉNTICAS
  al PC con el nativo en uso (`NatDirecta`, 9/9, con `fibR` ya en un ejecutor), y `KillLoop`, `NatKill` y
  `NatRecurKill` salen PARADA a los 12, 2 y 8 ms del KILL.

**Lo que destapó la medida: `#640`.** La variante «directa» paró a los 45 ms, pero el programa salió RUNTIME_ERROR
(«excepcion no atrapada: … se ha detenido (kill) dentro de una native»). Es la rama de `punto_control` para una native
SIN ejecutor, que lanza; y hoy se llega a ella cuando no cabe el ejecutor (8 KB de pila; la C6 con la GUI va al
límite). El `main` moría con esa excepción y `bpvm_run` devolvía el error aunque hubiera un KILL. `bpvm_run_smp` ya
daba KILLED. Arreglado en `bpvm_run`: «la bandera es la verdad» (`#462`), también cuando el `main` muere de una
excepción. `test-aotkill` lleva una pasada sin ejecutor (el kill desde `io`, como en la placa): roja antes del
arreglo, verde después.

#### `#577` — las tuplas DENTRO de una native: lo que hay y tres caminos (3-oct, para decidir)

**Lo que hay**, probado el 3-oct con el compilador del día. Desde `#519`, una native que no se puede traducir es un
error de compilación, y con tuplas se rechazan las dos direcciones:

```
native function sumaPartes(a: integer, b: integer): integer
  var c: integer
  var r: integer
  { c, r } := divide(a, b)        // «AOT: statement no soportado: DestructAssignStmt»
  return c + r
end sumaPartes

native function divideN(a: integer, b: integer): (integer, integer)    // «TypeRef no soportado: TupleTypeRef»
  return (a / b, a mod b)
end divideN
```

Una tupla es un objeto de una clase que el compilador sintetiza por forma, con campos pelados `_0`, `_1`… y sin
getters. Leer o escribir esos campos desde una native pide algo que hoy no existe:

| | qué | lo bueno | lo que cuesta |
|---|---|---|---|
| **A** | la clase sintética gana getters (y un constructor con los campos); la native los llama por el puente de métodos que ya hay (`call_method_i32`) | sólo el compilador, sin tocar la ABI; la regla «público ⇒ property» | cada campo, una llamada al intérprete (microsegundos), y la native va a un ejecutor; ranuras de vtable en todo módulo que use tuplas |
| B | helpers de campo (`get_field_*`, `set_field_*`), como los de los arrays | una lectura de memoria: rápido | sube la ABI (9 → 10): regenerar los `.mdn` de los packs y regrabar todas las placas y sus packs |
| C | no se hace; el manual lo dice: se desestructura en BP y la native recibe los campos como parámetros | nada | una native no habla tuplas |

**Decidido: A** (Eduardo, 3-oct: *«Getters, sin ABI»*). Se hace después de cerrar H9 y H10 (Eduardo: *«podemos cerrar
H9 y H10 antes de pasar a otras cosas»*).

**Una variante, vista al leer el código, para confirmarla al retomarlo.** Las natives ya crean objetos por una
FACTORÍA: `__cls_new_<Clase>`, una función del módulo que llaman por el puente a BP (`call_bp_i32`, buscándola por
nombre). Los getters y el constructor de la tupla pueden ser lo mismo: funciones ocultas del módulo que tiene la
native (`__tup_get_<forma>_<i>` y `__tup_new_<forma>`), en vez de métodos de la clase sintética. Funciona porque la
clase de una tupla tiene el mismo layout por forma en cualquier módulo; es lo que ya hace el intérprete al
desestructurar. Lo que se gana:
- no se añaden ranuras a la tabla de métodos de ninguna clase, sólo funciones en los módulos que tienen natives;
- no depende del módulo que CREÓ la tupla. Con métodos, una tupla que viniera de un `Str.mod` compilado antes (el de una
  placa) daría «slot no resoluble» (`call_method_i32` lo comprueba).

El coste es el mismo: una llamada al intérprete por campo, y la native va a un ejecutor. Un límite que se hereda del
puente: sólo lleva valores de 4 bytes, así que una tupla con `float`, `long` o `double` seguiría siendo un error al
compilar dentro de una native. La stdlib devuelve tuplas en `Str.parseInt` (`boolean`, `integer`), `parseLong`,
`parseDouble`, `parseHex` y `Time.civilDesdeDias`.

## H7 — `#554`: concatenar en placa, y QUÉ ES un literal de cadena (27-sep, análisis)

### La causa, medida

`#554` decía que concatenar en placa era ~250 veces más lento que en el PC (10 ms por concatenación en la Metro y la
C5). No era el GC ni la reserva: `a + b` llamaba a `__strconcat`, una función que el compilador SINTETIZA en bytecode y
que copiaba **byte a byte** en un bucle interpretado —13 opcodes por byte: unas 26 000 instrucciones para alargar una
cadena de 2 KB—. `CatCrono` (el bucle `s := s + trozo` de `#554`, y `a + b + c + d + i` 500 veces), antes y con lo que
hay hecho en el árbol (sin commit):

| | bucle (192) | cadena (500) |
|---|---|---|
| VM-C (PC) | 8 → 1 ms | 3 → 1 ms |
| miVM | 166 → 3 ms | 51 → 0 ms |
| **Metro** (sin regrabar: sólo el programa) | **2122 → 45 ms** | **474 → 83 ms** |

### Lo hecho en el árbol (sin commit), con las decisiones de Eduardo

- `__strconcat` copia con `move` (el builtin 55, en C en las dos VMs), y sólo si hay algo que copiar: una cadena `null`
  (un campo sin inicializar) concatena como `""` (`#512`, `StrNulo`).
- Una CADENA de `+` (`a + b + c + d`, la idea de Eduardo) se hace de una vez: `__strconcatN`, la longitud total, UNA
  reserva y un `move` por operando (por grupos de 8). Se aplana sólo lo que es concatenación (`1 + 2 + "x"` suma
  primero), también a través de paréntesis. La reserva cuenta BYTES (`ALEN`), no caracteres: `"año·€ñ"` bien.
- **El tropiezo**: los literales viven en el bloque de datos del módulo y `LEA_GLOBAL` empuja una referencia CRUDA (sin
  cabecera de heap); `move` sólo acepta arrays del heap → «move: src no es ref a heap». Eduardo: *«es una llamada
  implícita a una función de conversión y punto»* —un literal es un array de bytes constante, y usarlo como string es
  `toString(b'abc')`—: en el árbol, el literal usado como valor sale `LEA_GLOBAL + fromBytes` (el builtin 120, que lo
  copia al heap); en `print`, `==`/`!=` y los `case` de un switch de cadenas se lee tal cual (sólo leen). La stdlib,
  regenerada (15 módulos cambian) y `Stdlib.pack`. `CatCadena` —se comprueba ella misma: 23 casos— igual que antes.
- **Lo que eso rompe, comprobado en la Metro**: un programa nuevo que concatena el `"B"` que devuelve el `Machine.mod`
  VIEJO de su `/lib` da «move: src no es ref a heap». Cualquier módulo compilado antes (la stdlib de una imagen, los
  packs —`SQLite.pack` pasa 16 literales crudos a `RuntimeError`—, una librería de un usuario) rompe la concatenación
  de uno nuevo hasta que se recompila.

### La propuesta de Eduardo: el literal como ENTRADA ESTÁTICA de la tabla de handles

*«Si tuviésemos una entrada especial en la tabla […] donde no hace falta alojar espacio para el array de bytes,
simplemente se guarda su dirección, nos ahorramos el copiar al heap. Pero el destructor del string debería en ese caso
solamente eliminar la entrada de la tabla.»* Y: *«un string es por definición constante […] así que una entrada […] de
tipo string a array de bytes no cambia casi nada, salvo en el momento de la destrucción. Pero analízalo a fondo.»*

Analizado con tres barridos de lectura (la VM-C, miVM, y el compilador con el formato), cada hallazgo con su sitio.

**Lo que ya encaja.** Todas las LECTURAS de una cadena van por la indirección (`bpref_deref`/`refDeref`): ALEN, ALOAD,
PRINT_STRING, los builtins y las primitivas de cadena, los helpers del AOT, el mensaje de una excepción, el
`READ_STRING` del depurador de la VM-C. `CHECKCAST string(o)` ya acepta lo que está por debajo del heap. El bloque de
datos va SIEMPRE a RAM (también con XIP: sólo el código se queda en flash), no se descarga durante un RUN y vive más que
cualquier handle (la tabla se reinicia en cada RUN; miVM, una VM por ejecución). El GC no compacta. Y «liberar sólo la
entrada» ya existe (`bpvm_handle_kill`/`handleKillIdx`).

**Los problemas**, de más a menos grave:

1. **Un handle NUEVO en cada evaluación del literal no se puede liberar con el GC de hoy.** La marca de alcanzable va
   EN la cabecera del bloque del heap, y una entrada estática no tiene cabecera: el marcado se la salta y el barrido de
   la tabla también (`if (hh < heap_start …) continue; /* defensivo */`, en las dos VMs). Resultado: 8 B perdidos por
   evaluación, la tabla comiéndose el heap desde arriba hasta el OOM (un literal en un bucle). Liberarlas —el
   «destructor que quita la entrada»— pide una marca POR ENTRADA fuera de la cabecera, en el GC de las dos VMs; y
   registrar fuera de `heap_alloc` se salta la puerta que pide un GC cuando la tabla aprieta (`#430`): `LEA_GLOBAL`
   pasaría a poder fallar por memoria.
2. **Lo que funciona con el GC de hoy es UN handle por LITERAL, que dura lo que el RUN.** Pero entonces lo comparten
   todos los usos del literal, y su «destructor» no puede quitar la entrada (la mataría para los demás): no hace nada
   —como ya hace hoy la liberación de algo fuera del heap (`bpvm_free_owned`: *«no-heap: NOP»*, antes del kill; y
   `var owner s: string` compila)— y la entrada muere con la tabla, al acabar el RUN.
3. **`LEA_GLOBAL` no sirve para saber que apunta a un literal.** Los `.mod` MOD7/MOD8 (y los MOD9 del 17 al 21-sep)
   lo emiten también para los arrays fijos de módulo, que se ESCRIBEN (comprobado: `GcMisalign2.bp` compilado con el
   compilador de V6, `var buf: byte[10]`, las dos VMs de hoy lo ejecutan); el propio `ArraysTest` de miVM escribe en un
   `int[]` del bloque de datos. Ni el MAGIC ni ningún metadato lo distinguen, y el cargador no sabe dónde están los
   literales. Hace falta un OPCODE nuevo para el literal (hay libres: 0x6D-0x6E, 0xBE-0xFF).
4. **La caché de «el handle de este literal» no tiene sitio en el formato**: el relleno tras un literal es de 0 a 3
   bytes y `LEA` sólo lleva un offset. Hace falta una tabla por módulo en RAM (como la ext-table: `ext[i]`, que vale
   para XIP porque no se parchea el código), llena a la primera ejecución de cada literal —atómica con SMP— y NO en la
   carga (miVM reinicia la tabla de handles tras cargar: `setHeapStart`). Formato nuevo: las dos VMs y sus cargadores,
   `ModWriter`, `ModFormat`, `Disasm`, el MAGIC/cabecera; la cápsula V6 (MOD7) sigue igual.
5. **`move`**: saca el tipo y el tamaño de elemento de la cabecera. Una entrada estática se reconoce sin campo nuevo
   —su dirección cae por debajo de `heap_start`: sólo los literales tendrían handle ahí— y tiene que aceptarse como
   origen de BYTES y rechazarse como DESTINO: escribir en un literal compartido lo cambiaría para todos sus usos. Hoy
   la VM no distingue `string` de `byte[]` (los dos son I8): lo único que impide escribir un literal es el tipado del
   compilador (ASTORE*, los `array_store_*` del AOT, `FREAD`, los builtins de bus).
6. **Los módulos compilados antes siguen dando literales crudos**: con esta propuesta también hay que recompilarlos
   (la stdlib, los packs, los blobs de las imágenes, el IDE) —lo mismo que con `fromBytes`—.
7. **Las natives no pueden tener literales estáticos**: sus literales son de C, en el `.text` del `.mdn` (en flash con
   XIP), fuera de `memory[]`. Siguen reservando (`string_from_cstr`), como hoy.
8. **Paridad y detalles**: `==` entre `any`/`Object` compara los 8 bytes (con uno por literal, el mismo literal sigue
   siendo idéntico a sí mismo, como hoy); `object@N` imprime números de handle y crear entradas los desplaza (fuera
   del contrato de paridad, y las dos VMs las crearían en el mismo orden: el de la ejecución); los fallos de «32 bits
   truncados» (`#302`) pasarían a poder tocar a los literales.
9. **«Todas las referencias son handles» no se consigue sólo con los literales**: siguen crudas `LEA_LOCAL` y
   `LEA_FIELD` (los arrays fijos de `#19`, que sólo se indexan y se miden).

**Coste en ejecución** (uno por literal): la primera evaluación registra la entrada; las demás leen la tabla del
módulo —lo que cuesta un `GET_GLOBAL_L`—. Frente a `fromBytes`, que reserva y copia en CADA evaluación.

### Tres caminos, para decidir

| | qué | lo bueno | lo que cuesta |
|---|---|---|---|
| **A** (lo del árbol) | el literal usado como valor = `fromBytes` (una copia al heap) | sólo el compilador; ya medido | una copia por evaluación (salvo `print`, `==`, `case`); los módulos viejos rompen la concatenación hasta recompilarlos |
| **B** (la de Eduardo) | una entrada ESTÁTICA por literal, a la primera ejecución, en una tabla del módulo; `move` la acepta como origen | sin copias; todo string que nace en código nuevo es un handle | opcode y formato nuevos (las dos VMs, cargadores, herramientas); el «destructor» no hace nada (la entrada muere con el RUN); los módulos viejos igual que en A |
| **C** | un builtin de copia PROPIO de las cadenas para los helpers de la concatenación, que acepte las dos formas (el que el AOT ya tiene: `h_string_concat` acepta refs crudas) | los literales siguen crudos (coste cero) y los módulos viejos NO rompen nada | un builtin nuevo en las dos VMs; la doble representación de una cadena se queda |

Se pueden combinar: B para que el código nuevo no tenga literales crudos, y la copia de C en los helpers para que un
módulo viejo no rompa la concatenación.

**Decidido: C** (Eduardo: *«OK, C entonces»*, tras preguntar cuántas entradas crearía B —ninguna con C; con B, sólo
los literales del código que se ejecutan, una vez cada uno: los strings de la cabecera del módulo no son valores—).
**Hecho en el PC** (`e5761dd1`): `__strCopy(src: string, dst: byte[], pos): integer`, el builtin 263, en las dos VMs
(`memmove`/`arraycopy`), privado de `Core.bp`; acepta la cadena en sus dos formas y `null` como vacía, y devuelve dónde
acabó, así que las copias se encadenan. `__strconcat` y `__strconcatN` lo usan; los literales, como eran (fuera la
conversión con `fromBytes`). La stdlib regenerada (14 módulos), `Stdlib.pack` y la stdlib embebida de las tres
familias; `CatCadena` en la paridad (106) y, con `CatCrono`, en la ronda de V7. Queda la placa: una imagen con el
builtin 263 —lo que compile el compilador de hoy lo necesita: en una imagen vieja, «builtin no soportado» en la
primera concatenación—.

**En placa** (27-sep, tarde, con el OK de Eduardo): la Metro, la DK2, el P4 y la Nucleo grabadas con la imagen de
`e5761dd1` —con el INFO que ya trae `"mdnAbi": 9`, `#576`— y la ronda de V7 entera verde en las cuatro (22, 24,
24 y 22: `CatCadena` y `CatCrono` idénticas al PC; `compat/informes/tanda_554_27sep.md`). `CatCrono`, el bucle de
192 y la cadena de 500: la **Metro 2122 → 44 ms y 474 → 60**; la DK2, 22 y 54; el P4, 7 y 21; la Nucleo, 34 y 53.

### Hallazgos de paso (con ficha)

- **`#588`** — el AOT calcula a mano la posición de las globales del módulo (-4, -8…, sólo para integer, float y
  boolean) y se equivoca si antes hay una global `string`, ref, `long` o `double`: con `var s: string := "hola"` y
  luego `var n: integer := 5`, la native lee `cs-4` y `n` está en `cs-20`. Silencioso. (Anterior a todo esto.)
  ✅ Cerrada el 27-sep (`455ca895`): la posición la da el compilador (`MivmEmitter.offsetsDeGlobales`).
- **`#589`** — el depurador de miVM muestra una cadena del HEAP como `@4000…` (no hace la indirección: sólo los
  literales crudos salían con su texto; con A o con B, TODAS las cadenas) y la vista de properties lee 4 de los 8
  bytes de una referencia.
  ✅ Cerrada el 27-sep (`b777f25f`): la indirección (`VirtualMachine.leeCadenaDeRef`, espejo del `READ_STRING` de
  la VM-C), UN render de locales para la VM-Java y para una placa, la palabra baja en las properties, y una prueba
  que ejecuta el depurador de miVM por el socket (`DepuradorCadenasTest`). Salieron `#590` (el valor inicial de una
  property de módulo se descarta) y `#591` (los helpers sintetizados heredan la última línea del `.dbg`).
- **`#587`** — `StringBuilder` (`Core.bp`): un `integer[]` (4 bytes por carácter) y `appendStr` carácter a carácter
  con una llamada por carácter. Eduardo: *«debería ser un array de strings, así que la copia y el cálculo del espacio
  necesario se simplifica»* —las piezas, y `toString()` como `__strconcatN`: la suma de las longitudes en BYTES, una
  reserva y un `move` por pieza—; `appendInt` es una pieza como otra (ya convierte), `appendChar` una pieza de un
  carácter (*«un poco tonto, pero nada grave»*).
  ✅ Cerrada el 27-sep (`ec5b15c7`), con `__strCopy` (el camino C) en vez de `move` y, para `appendChar`, una
  caché de 64 caracteres que se vacía como una pieza más (Eduardo: *«un array de longitud fija […] hasta que llega
  un appendInt o appendStr y entonces aprovechas y vacías la caché»*). En la Metro, 192 `appendStr` de 94 a 19 ms;
  2000 `appendChar`, de 78 a 74-75.
- Menores: la base de cada módulo (CS) no va alineada a 4 aunque `MOD_FORMAT` §7 lo dice; `OPCODES.md` cuenta 186
  opcodes y hay 188.

## H7 — `#531`: el cuanto de miVM por INSTRUCCIONES (27-sep, análisis: Eduardo, *«analízalo más antes»*)

**Lo que hay.** Las dos VMs drenan los eventos igual —uno por cuanto, al darle la CPU al hilo destinatario
(`drainOneEvent` / `bpvm_event_drain_one`)— y reparten igual —round-robin entre los RUNNABLE—. Lo que difiere es
DÓNDE acaba un cuanto: la VM-C cuenta opcodes (`bpvm_interp_run_quantum`: `if (ops >= max_ops)` arriba del lazo y
`ops++` después, exactamente 1024 por cuanto); miVM lo corta con un TEMPORIZADOR de 10 ms (`startPreemptTimer` marca
`yieldRequested` en todo hilo RUNNING). Es el único sitio del planificador donde «el mismo algoritmo» no lo es, y
por eso miVM ni siquiera repite su propio orden (`EvDos`: «onB» tras `v250` una vez y tras `fin` dos).

**Medido con un prototipo** (desechable, no subido: un contador en el lazo de `runOnContext` con el criterio exacto
de la VM-C, y el temporizador apagado):
- `EvDos`: miVM 3 de 3 IDÉNTICA a la VM-C («onB» tras `v50`).
- El coste (`Bucle`, 3 M vueltas, la JVM de 32 bits de este equipo): un hilo 2566 → 2730 ms (**+6 %**), dos hilos
  2958 → 3080 ms (**+4 %**). La frontera de cuanto de miVM es cara —salir del lazo guardando los registros, el
  `vmLock`, reencolar, elegir, drenar, volver a cargarlos— y con 1024 opcodes pasa unas 170 veces más que con 10 ms
  (en este PC miVM hace unos 17,5 M opcodes por segundo: 1024 son unos 60 µs; la VM-C, unos 400 M: 2,6 µs).
- La red entera con el prototipo: VERDE —paridad 109/109 en MOD9 y MOD8, pruebas de C 25/25—: ningún sample del corpus cambia de salida con el cuanto por instrucciones.

**Qué cambiaría para un programa.**
1. De un hilo y sin eventos: nada visible (la frontera vuelve a elegir al mismo hilo).
2. Los eventos se entregan en la MISMA instrucción en las dos VMs (el caso de `#531`).
3. Varios hilos que calculan sin esperar: el reparto pasa a ser determinista y el mismo en las dos VMs. Con `sleep`
   de por medio, el despertar sigue dependiendo del reloj en las dos, como hoy.
4. El GUI de miVM (Swing en su hilo; `GUI_RUN` bombea con esperas) no depende del temporizador.
5. `--workers=N` (el SMP experimental de miVM): cada worker corta cada 1024 por su cuenta; con varios workers el
   intercalado sigue sin ser determinista, como en el SMP de la VM-C.
6. Una native (el AOT, sólo en la VM-C) no cuenta opcodes igual —cede en sus bucles—: con natives y varios hilos
   el reparto puede seguir difiriendo, y eso ya está fuera del contrato de paridad del stdout.
7. miVM pierde un hilo (el del temporizador).

**Si el coste molestara**: un camino rápido en la frontera —si no hay otro hilo RUNNABLE, ni un evento para éste,
ni un dormido que ya deba despertar, seguir sin salir del lazo—. En la VM-C eso es lo que pasa en efecto (vuelve a
elegir al mismo hilo), así que el orden observable no cambia; pide leer ese estado sin el `vmLock` (contadores
volátiles).

**Propuesta**: hacerlo —el contador, sin el temporizador—, con `EvDos` en la paridad, y el camino rápido sólo si el
+6 % molesta.

**Hecho el 27-sep** (`8614d7af`, con el OK de Eduardo). Su pregunta al leerlo —*«si hay un cambio de contexto
cada cierto tiempo, en ese momento ya se asegura un punto donde atender otros eventos dentro de una función nativa;
si se quiere afinar más, ¿un 2º timer?»*—: la VM-C ya mide así a una native (por tiempo, 5 ms, mirado en su línea
del bucle cada 1024 vueltas), y en miVM una native se interpreta y cuenta instrucciones como todo; el 2º
temporizador, para que una native con vueltas pesadas ceda a tiempo, es `#592`.

## H7 — `#494`: el heap descarrilado, UNA EXCEPCIÓN (27-sep, decisión de Eduardo; el cómo, para mirarlo)

**Lo que hay.** El guardián (`buildValidObjectsSet` en miVM, `build_gc_valid_map` en la VM-C) ve que el recorrido
de cabeceras no acaba en `heapNext`, lo dice (stderr / `bpvm_diag_urgente`) y el GC SIGUE: marca con un mapa
truncado y el barrido se lleva objetos vivos; el programa muere después, en otro sitio, con un use-after-free. La
VM-C decidió no abortar a propósito (`heap.c`); Eduardo, 27-sep: ni avisar y seguir ni parar la VM — una excepción.

**El diseño propuesto.**
1. Si el recorrido descarrila, el GC NO barre (con el mapa truncado, barrer es destruir) y queda marcado.
2. La reserva que pidió ese GC falla con una excepción BP en ESE hilo, atrapable como «No space in heap», con el
   mismo texto en las dos VMs: `RuntimeError` «heap descarrilado (fallo interno de la VM)». Los números (dónde
   acabó el recorrido, la cabecera leída) siguen saliendo por el diagnóstico, como hoy.
3. Lanzarla no puede reservar —el heap está roto—: una PREFABRICADA, como la de sin memoria (`#430`: `oomExc` en
   miVM y la de `exceptions.c` en la VM-C), creada al arrancar con ese mensaje fijo.
4. Cada GC posterior vuelve a descarrilar y a lanzarla: quien la atrape puede cerrar sus ficheros y salir; si nadie
   la atrapa, el programa acaba con salida 1 y el mensaje.
5. La prueba: en C y en miVM, estropear a propósito la cabecera de un bloque (un largo falso), pedir un GC y esperar
   la excepción con su texto; y con el guardián saboteado, roja.

**Hecho el 27-sep** (`7a1a9374`, con el OK de Eduardo), tal cual. La prueba enseñó además un punto ciego del
guardián, que ya estaba: una cabecera con un largo DENTRO del heap hizo saltar el recorrido hasta el borde de otro
bloque, que acabó en `heap_next` sin decir nada (`#593`); la prueba usa por eso un largo que se sale del heap.

## H7 — el diagnóstico del heap DESDE BP (28-sep, propuesta para Eduardo)

**Lo que hay, medido hoy.** `heapFrag()` y `heapMap(cols)`, primitivas públicas de `Core`, devuelven TEXTO y sólo
existen en miVM: el mismo programa en la VM-C —la de las placas— muere con «builtin 121 no soportado en esta VM
(subconjunto C)». La VM-C sabe los números (una línea de diagnóstico por colecta: vivos, liberados, huecos, el
mayor, las reservas de la lista y del bump, las astillas, las fusiones), pero un programa no puede preguntarlos. Es
lo que pidió Eduardo el 5-ago (`V5_IDEAS` §«Diagnóstico del heap DESDE BP»): *«¿hay algún handle de memoria
degradado?, ¿la memoria está muy fragmentada?»*, llamable desde un programa.

**La propuesta**: un módulo `Heap` en la stdlib, en las DOS VMs, que LEE contadores —no reserva nada ni recorre el
heap; sólo su lista de libres, que es corta— y devuelve NÚMEROS (la lección de `MemInfo.bp`: un instrumento que
reserva memoria para medir memoria miente):

| función | qué da |
|---|---|
| `Heap.getSize()` | el tamaño del heap, en bytes |
| `Heap.getFree()` | los bytes libres: lo que queda al final más la lista de libres |
| `Heap.getLargestFree()` | el mayor bloque que se puede reservar de una vez |
| `Heap.getFreeBlocks()` | cuántos huecos tiene la lista de libres |
| `Heap.getHandles()` | cuántos objetos vivos hay (handles en uso) |
| `Heap.getCollections()` | cuántas colectas lleva el programa |

`getFree` y `getLargestFree` contestan «¿está fragmentada?»:

```
if Heap.getLargestFree() < 4096 then
  print "fragmentada: " + Heap.getFree() + " B libres, el mayor " + Heap.getLargestFree()
endif
```

Cada una, una primitiva nueva de `Core` (como todas desde P2) con su espejo en las dos VMs; `Heap.bp`, la capa fina
de encima, como `Machine`.

**Lo que NO son: números de paridad.** Son de cada VM y no miden lo mismo: la VM-C guarda la tabla de handles y la
reserva de emergencia dentro del bloque de memoria, su heap tiene otro tamaño que el de miVM y colectan en momentos
distintos (medido hoy: la VM-C del PC colecta cada 47910 B, con el tope en 387232). Un programa los usa para
DECIDIR (un umbral), y una prueba mira RELACIONES —el mayor ≤ lo libre; tras soltar una lista grande, lo libre
sube—, nunca el número en el stdout.

**Para decidir**:
1. El nombre del módulo y de las funciones (`Heap.get*`: verbo y minúsculas, la convención de la casa).
2. ¿También los contadores del alocador —reservas de la lista y del bump, astillas, fusiones—? Sirven para
   diagnosticar la VM más que a un programa; propuesta: no, por ahora (la stdlib, pequeña).
3. `heapFrag`/`heapMap`: quedarse como están (texto, sólo miVM, para mirar en el PC) o retirarse. Propuesta:
   quedarse, y que el manual diga que lo portable es `Heap`.

**Decidido (Eduardo, 28-sep) y hecho** (`ac0f3d71`, `#594`): el módulo `Heap` con esos nombres; sin los contadores
del alocador; `heapFrag`/`heapMap` se quedan. Medido: el tamaño y lo libre difieren entre VMs (254753 y 376424 al
arrancar, en el PC), lo usado coincide (108 B). Y Eduardo añadió: *«Me gustaría la posibilidad de que el usuario
pudiese pedir compactar el heap. Siempre en condiciones controladas, como por ejemplo al terminar la aplicación»*:
`#595`, con su análisis (el mecanismo, el censo de punteros crudos y las condiciones) en la ficha.

**`#595`, decidido y hecho el mismo día** (`6b265162`): Eduardo, «Si» a las condiciones —sólo desde `Main`, sin
otros hilos—, y una intrínseca, no una `native` (una native es BP compilado a C y no ve la tabla de handles). Sin
memoria aparte: la tabla se enhebra en las cabeceras. El censo tenía un error: el estado de un fichero abierto
vive en el heap en la VM-C y littlefs lo enlaza; se clava. Lo que Eduardo ve detrás: los overlays
(`loadModule`/`unloadModule`, `V2_BACKLOG` §17) y los programas que corren días —con hilos, el programa aprovecha
una calma para pararlos, cerrar lo abierto, compactar y reposar (`HeapReposo`)—.

## H7 — `#600`: la salida por TRANSACCIONES (28-sep, idea de Eduardo; propuesta para decidir)

> **Decidido el mismo día (Eduardo), y hecho (`cc073e52`, ver `FICHAS` `#600`)**: *«la otra opción es
> utilizar stringbuilder y acumular las salidas y al final hacer un print stringbuilder.toString(). No sé, lo que
> sea más sencillo»*, con la regla *«si haces print a; print b entre los dos se puede colgar un print de otro
> hilo, eso es correcto. Ahora si haces print "Resultado=", r que se cuele algo en medio no es correcto»*. Así
> que no hay `beginPrint`/`endPrint`: **un `print` sale entero** —sus piezas se guardan en la línea del hilo y
> salen con el salto, de un empujón— y varias líneas juntas son un `StringBuilder` y un `print`. Lo de abajo se
> queda como el análisis que llevó ahí.

Eduardo, al ver `#600` (con dos workers, la línea de un hilo se parte con la de otro): *«Este problema ya lo hemos
tenido con I2C y SPI y al final optamos por trabajar con transacciones. Sería algo así como Ini Trans, print a,
print b Fin Trans.»*

**El problema, entero.** Un `print` no es una instrucción. `print a, b` son cuatro —la cadena, el separador, el
número y el salto: `PRINT_STR_NONL`, `PRINT_CHAR`, `PRINT_NONL`, `PRINT_NL`, visto con `--trace`—, y hasta el
`print` de una cadena sola empuja dos trozos (la VM-C, la cadena y luego el salto; miVM, `writeText` y `newline`).
Entre dos trozos cabe el de otro hilo:

- con **dos workers**, en cualquier momento (`#600`: «  A 1000  B 1000»);
- con **uno** —todas las placas hoy—, en el cambio de turno, cada 1024 instrucciones: determinista e igual en las
  dos VMs, así que la paridad no lo ve, pero la línea sale mezclada igual. Y entre dos `print` seguidos de un hilo
  puede caer la línea de otro, que para una tabla o un informe de varias líneas es lo mismo.

**Lo que enseñó el bus (G4.a3), y vale aquí**: la transacción se PREPARA y se ejecuta de UNA vez; nunca un cerrojo
abierto entre instrucciones BP —con hilos verdes, el hilo que espera un cerrojo de C bloquea el hilo del sistema, y
con él al que lo tiene: cuelgue—; y lo que cabe en una lo marca el micro más pequeño.

**La propuesta, en dos piezas:**

1. **Cada instrucción de `print` sale entera** (la base, sin API): lo que empuja una instrucción —el texto y su
   salto— va a la cola de salida de una vez, esperando a que quepa entero (hoy, si la cola está medio llena, la
   parte en dos y otro hilo puede entrar en medio). Es la transacción de una instrucción; arregla el `print x` de
   siempre con dos workers, que es el caso de `#600`.

2. **La transacción, para varias** —la forma de Eduardo—:

   ```basic
   IO.beginPrint()              // Ini Trans: lo que imprima ESTE hilo, se guarda
   print "Temperatura: ", t
   print "Humedad: ", h
   IO.endPrint()                // Fin Trans: sale todo junto, de un empujón
   ```

   Entre el principio y el fin, los `print` de ese hilo no van a la cola: se acumulan en un búfer suyo, y el fin lo
   empuja de una vez. **Nadie espera a nadie** mientras tanto: no hay cerrojo entre instrucciones; los demás hilos
   imprimen como siempre y sus líneas salen antes o después del bloque, nunca en medio. Es el «se prepara y se
   ejecuta de una vez» del bus, con los `print` de siempre dentro (se puede llamar a una función que imprime).
   Coste: por hilo, una referencia al búfer, que sólo existe mientras dura la transacción —nada de los 4 KB fijos
   por hilo que tumbaron la línea por hilo de `#185` en la Pico—; en cada `print`, mirar si su hilo está en una.

**Lo que hay que decidir** (la columna de la propuesta es mía):

| | propuesta | por qué |
|---|---|---|
| el nombre | `IO.beginPrint()` / `IO.endPrint()` | en un módulo que ya existe; sin palabras reservadas |
| el límite | lo que cabe en la cola de salida del más pequeño: la Pico, 1 KB (ESP32 y STM32, 2 KB; el PC, 4 KB) | como el bus: lo marca el piso, igual en todas |
| pasar del límite | sale lo acumulado y la transacción sigue, en trozos de 1 KB, cada uno entero, con un aviso | la salida es también el canal de depuración: un `print` que lanza una excepción escondería justo lo que se quería ver. La otra, la del bus: un error con nombre |
| anidar | cuenta: sólo el fin de fuera empuja | una función con su transacción se puede llamar dentro de otra |
| el hilo acaba —o muere— con la transacción abierta | sale lo acumulado, antes que el mensaje de la excepción | no se pierde texto |
| leer de la consola dentro | sale lo acumulado antes de leer | si no, la pregunta no se vería |

**Cómo, en las dos VMs** (para cuando se decida): en la VM-C, `emit_text` mira el hilo en curso —cada worker sabe
el suyo— y, si está en una transacción, añade a su búfer; `bpvm_oq_push` gana una forma que espera a que quepa
ENTERO (la de hoy parte lo que no cabe); en miVM, el `ThreadContext` lleva el búfer y `OutputSink` una escritura
entera, sincronizada en cada sumidero (el de stdout, el del cable y los del IDE). La prueba: dos hilos que imprimen
bloques a la vez y un sample que comprueba que cada bloque sale seguido; `EvDos` vuelve a `check-smp` con
`// smp: orden libre`. Y en placa, la salida por el cable de las tres familias (es el camino de todo `print`).

## H7 — lo que queda: el MAPA VERIFICADO (29-sep, para decidir el orden)

Eduardo: *«continuar H7»*. Cinco lecturas del código de hoy —la memoria por placa (`U6`), el heap y las pilas (el
muro, `#593`), el arranque y los packs, la flash (`U1.4`, los 32 MB) y `CALL_REL` con la Pico sobre
`platform_freertos.c`— y lo decisivo comprobado a mano; tres cosas, EJECUTÁNDOLAS en las dos VMs (`PilaHonda`,
`HilosMuchos` y `OwnerSmp`: van a la red con su arreglo). Lo que dice cada asunto contra lo que hay:

| asunto | lo que hay hoy | tamaño | ¿decide Eduardo? |
|---|---|---|---|
| **El muro heap/pilas** (idea de Eduardo; su ficha no tiene cuerpo) → `#605` | Entre el heap y la tabla de handles hay software desde `#451` (el bump contra el techo; la tabla no baja sobre el heap vivo), y las pilas crecen HACIA ARRIBA, lejos de la tabla. Lo que no tiene muro son las PILAS: ninguna de las dos VMs mira un desbordamiento (en `interp.c` no aparece `stack_top`: `CALL` y `ENTER` empujan sin mirar; miVM, igual). Medido (`PilaHonda`): un hilo con la pila por defecto (2 KB) que recursa 100 niveles pisa la del hilo vecino —en miVM el vecino muere con «el receptor no es un objeto» y el programa acaba con los resultados mal; en la VM-C el programa muere con ese texto; a 400 niveles, miVM salta a un PC basura—, y ningún mensaje dice «pila». El `main` usa en silencio toda la región: sus 16 KB son nominales. Y la VM-C no respeta `Thread(stackSize)`: todos los hilos, 2 KB (`C5SdGui` pide 4096). | medio: comprobar en `CALL`/`INVOKE`/`ENTER` contra el tope de la pila menos una zona roja, en las dos VMs y en la misma instrucción, con una excepción prefabricada como la del heap lleno (`#430`) y la del heap descarrilado (`#494`); antes, medir cuánta pila usa el corpus | sí: ¿es esto el muro?; al agotarse la pila, ¿una `RuntimeError` atrapable, matar el hilo o parar el programa?; y el `main` pasaría a tener de verdad sus 16 KB (un programa que hoy funciona gracias al desbordamiento callado dejaría de funcionar) |
| **Los hilos de la VM-C: 32 en toda la vida del programa** → `#604` | `bpvm_thread_spawn` no reutiliza nunca el hueco ni la pila de un hilo terminado (`thread_count` y `next_thread_stack` sólo crecen). Medido (`HilosMuchos`: 40 hilos de uno en uno, `start` y `join`): miVM, los 40; la VM-C muere en el 32 con «Thread.start falló», un error que no se atrapa. miVM reutiliza las regiones de los muertos y no tiene tope. Un programa de días que lance un hilo por tarea muere en la VM-C. | medio (reutilizar el hueco y la región respetando `stackSize`; va de la mano del muro) | no: es un fallo, y rompe la paridad |
| **`FREE_REF` sin el cerrojo con `--smp`** → `#603` | Soltar un `var owner` (o sustituir un campo owner) mete el bloque en la lista de libres SIN `vm_lock` (`bpvm_free_owned` → `add_to_free_list`), mientras el alocador de otro worker la recorre. Medido (`OwnerSmp`: dos hilos que crean y sueltan objetos con dueño): con `--smp=2`, 8 de 8 ejecuciones rotas —«heap descarrilado», «use-after-free», «strCopy: dst no es ref a heap»—; con un worker, bien; miVM con dos, bien (libera bajo `vmLock`). `check-smp` no lo ve: ningún sample suelta owners desde dos hilos a la vez. | pequeño (el cerrojo, y el sample al corpus) | no |
| `#593` — el punto ciego del guardián del heap | El guardián mira DÓNDE acaba el recorrido (y, sólo en la VM-C, un tamaño mayor que el heap entero). Contar BYTES no lo arregla: si el bloque que miente se traga bloques ocupados, la suma sale igual. Hay que contar BLOQUES: una cuenta que llevan la reserva, la liberación y el barrido, comparada con la del recorrido en cada GC (4 B y una suma por reserva). Por este agujero se coló `#597` una vez de cada tres. | pequeño, en las dos VMs (y el gancho de la prueba, relativo —«tragarse justo el bloque siguiente»—, porque los heaps de las dos no son iguales) | sí: ¿merece la pena? (propuesta: sí, la cuenta de bloques, con la excepción de `#494`) |
| Los restos de `U6` | Nada ha entrado desde el 3-sep (en el código no hay `coser` ni `vmheap`), pero el terreno se movió: el S3 y la C5 tienen la VM en PSRAM (con un margen de 512 KB «prudente y no medido»), las cinco familias leen el ENV antes de repartir la memoria (el obstáculo de `vmheap=N` ya no existe) y hay consumidores nuevos de `malloc` (8 KB de pila por native). **Lo que no está anotado: la C3 ya no llega a su objetivo** —`vmHeapBytes` 65536 el 12-sep, 61440 el 24 y 57344 el 25 (las tandas): el bloque, de 128 a 120 KB, a saltos de 4 KB, que es el paso con el que el arranque reintenta—. Y `test-mem` pasa, pero no está en la red y describe placas de antes (la DK2 con 1536 KB; hoy, 1792). | coser la C3: grande (además de las cuatro comprobaciones de la ficha, el cargador, la compactación y `Heap.*`); ~~los márgenes de la Pico y la STM32 como MEDIDA~~ (✅ **medidos el 30-sep**, `a2bdff6e`: la línea `mem:` en las dos y `tools/medir_margen.py`; los números y la propuesta, en «Los márgenes de la Pico y de la STM32, MEDIDOS», más abajo); el del S3 es ahora el de su PSRAM; `vmheap=N`: pequeño; el +10 % del `AllocBench` de la C3: la premisa no se sostiene (antes y después ya llevaban el planificador) y el camino que medía ya no existe (`#554`); un nativo sobre PSRAM: pequeño-medio, en la Metro; `U6.F1`: medio | **Decidido por Eduardo el 30-sep**: **la C3, a V8** —coserla y devolverle su objetivo— (*«la C3 se optimizará más adelante (V8). De momento, aunque con límites más bajos, que funcione lo mínimo»*), y con ella el +10 % del `AllocBench`, cerrado aquí; **`vmheap=N`: N es el bloque entero** (heap y pilas; `stack=` lo reparte por dentro) —✅ hecho el mismo día, `ee89f444`, y probado en la Pico, la DK2, la C5 y la P4—; **el nativo sobre PSRAM y `U6.F1`, a estudiar en un futuro, sin versión** (*«sin asignar una versión concreta»*). Antes: sí, cada uno: coser choca con su regla de no encoger el heap del RTOS; ¿qué es N en `vmheap` (el bloque o el heap)?; ¿cerrar el +10 % y abrir «la C3 por debajo de su objetivo»?; ¿qué decisión depende del nativo sobre PSRAM?; ¿`U6.F1` sale del aparcamiento? |
| Boot 1 / Boot 2, las capas del SO y el montaje de packs | `bpvm_boot.h` no se toca desde el 18-jul: cuatro estados y ningún peldaño de packs. El «Boot 2» está escrito a mano en cinco sitios (Pico, ESP32, el P4 dos veces —TCP y UART— y STM32), con el mismo esqueleto y órdenes distintos. Montar la zona son tres funciones (y el simulado, a mano) y tres criterios: la Pico y el P4 exigen FS; las otras ESP32 y la STM32, sólo particiones; el simulado, siempre. El cargador de motores son tres casi-copias (~570 líneas); «cargar una vez», tres; el bloque de RAM, tres reglas (la Pico no entiende `packRam=`, sólo `SQLite=`). `pack_s3.c` ya no hace falta: el S3 monta por el común desde el 8-sep. Las capas del SO no están declaradas en ningún sitio. | medio (un Boot 2 común, con `mapear()` por familia); medio (el cargador común); pequeño (declarar las capas). Pide la ronda de placas | **Respondido por Eduardo el 30-sep**: (1) *«después de crear la partición de la flash y la zona de packs, ésta se ha de formatear. Si no se formatea no es un error, es correcto. Sin embargo tampoco se pueden subir packs, así que lo podemos señalar como un estado más, ni totalmente correcto pero casi»* —el cómo, propuesto abajo, en «Boot 2 y el montaje de packs»—; (2) *«el FS es necesario para el funcionamiento normal del SO, así que si no está bien hay que solucionarlo»*: montar la zona exige el FS; (3) **las tres cosas**: el montaje, el cargador y el bloque de RAM (con `packRam=` en la Pico). Antes: ¿los packs, un estado con número (renumerar y tocar el IDE) o una capa al lado que no corta la subida?; ¿montar exige FS o bastan las particiones?; ¿sólo el montaje, o también el cargador y el bloque de RAM (con `packRam=` en la Pico)? |
| `U1.4` — una cintura de flash por familia | La STM32 ya la tiene (`stm32_flash.c`: los cuatro consumidores por el mismo par de funciones; por eso el borrado por bancos de `ec0a48b2` les llegó a todos a la vez); la ESP32 usa `esp_partition_*` en los cuatro (el IDF hace de cintura); sólo la Pico llama al SDK desde cuatro sitios, con el candado en seis. No hay cabecera común. | pequeño (un `pico_flash.c`, como la STM32) o medio (un contrato común) | ✅ **decidido por Eduardo el 30-sep: la pequeña**, un `pico_flash.c` como el `stm32_flash.c` —✅ **hecho el mismo día, `47fe6fef`**: el FS, el ENV, los packs y el log por la cintura, con el candado y la zona permitida (la zona 2 o las particiones) dentro; probada en la Pico por los cuatro caminos—. Antes: ¿la pequeña, la grande, o cerrarla como «no se hace»? |
| Los 32 MB del P4 | El techo de 16 MB lo pone el IDF (el mapeo rechaza lo que pase de 16 MB salvo con una opción experimental), no el chip. La C5 tiene el mismo techo y lo esquiva: declara 32 MB y su `bpdata` acaba en 16. Los 32 MB son de la P4 Waveshare; el kit tiene 16, y una imagen declarada de 32 no arranca en él (el IDF para en un `assert`). La línea del log que explicaba el techo (la dirección física contra el límite de 24 bits) se borró el 8-sep (`630a221e`) al pasar el mapeo al común: hoy un fallo sale como «err=258» y nada más; y a la Waveshare el arranque le aconseja «reflashear bootloader», que no toca (los 16 MB son a propósito). | ~~pequeño el paso 0 (devolver la línea y corregir el aviso)~~ ✅ 30-sep, `cc503a54`, visto en la Waveshare; medio una partición por encima de 16 MB (con una segunda imagen del P4); grande invertir el orden común (reformatear diez placas) | sí: ¿dos imágenes del P4 (V4 lo rechazó), o repartir dentro de los 16 MB y cerrarla?; ¿la C5, por el mismo camino? |
| `CALL_REL` | Sólo sacaría `cb` del `CALL` local: `cb` sigue haciendo falta en el despacho por vtable, los hilos, los eventos y el secuestro del AOT; y el `CALL` de siempre (0x07) se queda para siempre, porque lo usa la cápsula de V6. Hay 68 opcodes libres. Es poco código, pero sube el formato (MOD10, o dentro de MOD9, que no está publicado) con toda su cascada: la stdlib, los packs, las imágenes y las nueve placas. No hay ninguna medida de lo que cuesta `cb` hoy (una caché de una entrada por hilo). | medio, por la cascada | sí: ¿entra en V7? (propuesta: medirlo antes con `fibInterp(28)` y un prototipo desechable; si no se nota, cerrarla) |
| La Pico sobre `platform_freertos.c` (✅ paso 0 MEDIDO el 30-sep, `d8db24b7`: +10,8 %, fuera del ±3 %; la Pico se queda con su fichero; detalle en `FUSION_BACKENDS_FREERTOS.md`, Paso 0) | El 32 % de `PrintBench` se midió en un camino de `print` que ya no existe: `#600` lo cambió (una línea entera, un empuje), y `#485` no tocó nada en la Pico. En ese camino, la única diferencia del común es el `broadcast` (coge el guard N+1 veces). El paso 0: dos imágenes del mismo commit —A con el fichero de la Pico, B con el común—, alternadas, con `PrintBench`, el KILL, `Bench`/`AllocBench` y `fuga_hilos.py`. | pequeño en código; pide grabar la Pico 2 | sí: el OK para grabarla con A y con B, y el umbral (dentro del ruido) |

**Encontrado de paso** (comprobado en el código):

- Dos fallos LATENTES del XIP → `#606`: `GUI_INVOKE_BY_SLOT` (`builtins.c`) calcula la dirección del método SIN `cb`
  —el manejador de una ventana de Forms cuya clase viva en un pack saltaría a basura—; y `bpvm_run_smp` (`bpvm.c`)
  busca el módulo del `main` con el rango viejo, que en XIP está vacío —con `--smp` y el `main` en un pack,
  `BAD_PC`; `bpvm_run` se arregló en su día y su gemelo no—.
- El autorun de la STM32 no mira `APP && !degraded`, como las otras tres (la ESP32 lo tuvo mal y se arregló): hoy no
  hace daño —su capa `app` siempre sube, y sin FS no hay autorun—, pero es la regla escrita en tres sitios y
  olvidada en el cuarto; un Boot 2 común la pone en uno.
- El `HELLO` dice capacidades distintas: `PACKS`, sólo la STM32 y el simulado (la Pico y la ESP32 también tienen
  packs); `DEBUG` y `BOOTSEL`, sólo la Pico. Nadie las lee hoy. → **Hecho** (30-sep, `cc503a54`): la Pico y la ESP32
  dicen `PACKS`, y la ESP32 `DEBUG`; la STM32 sigue sin `DEBUG` porque su wire no tiene el depurador (H10). La tabla,
  en `BPVM_WIRE_PROTOCOL` §5.3; visto en la Pico 2 y en la P4 (`wire_serie.py info` enseña ahora el `HELLO`).
- miVM saca por stderr `[B1-trigger] … se auto-encola RUNNABLE` en ejecuciones normales: una medida de `#474`
  (`2e94620b`) que se quedó; no es un error (su comentario: «es la condición previa»). → **No era sólo ruido:
  `#613`** (30-sep, `d6d72384`): con varios workers, esa condición previa abría la ventana de B1 por `sleep(0)`
  (9 de 30 con cuatro); arreglado como en la VM-C —no se reclama un hilo con dueño— y el aviso, fuera.
- Documentos rancios: `HEAP_LAYOUT.md` no dibuja la tabla de handles entre el heap y las pilas (es de `#451`), y el
  documento dice de sí mismo que manda sobre el código; `OPCODES.md` dice 186 y «`0xBC .. 0xFF` libres», pero `0xBC`
  y `0xBD` son `LEA_FIELD` y `LEA_FIELD_REF`; los punteros de `V7_BACKLOG` §H7 a `FICHAS`, todos rancios; y
  comentarios que dicen lo contrario del código (montar la zona después de `/lib`, el S3 sin packs, «`vm_buffer_init`
  corre antes del ENV»).

**Propuesta de orden**: primero los fallos, que no piden decisión y cierran algo —`#603` (el cerrojo), `#604` (los
hilos de la VM-C, como miVM), `#606` (los dos latentes del XIP), el paso 0 de los 32 MB, `test-mem` a la red y los
documentos rancios—; después, con las decisiones de Eduardo, el muro de las pilas (`#605`, junto a `#604`) y `#593`,
todo en el PC; y lo que pide placa (el arranque y los packs, `U1.4`, la Pico, los márgenes de `U6`, los 32 MB), a la
ronda de placas, cuando toque.

### `#604` y `#605` — las pilas de los hilos: el diseño (29-sep)

Eduardo, sobre `#605`: *«excepción stack overflow, sí»*. Los dos van juntos porque tocan lo mismo —dónde vive la pila
de cada hilo y hasta dónde puede crecer— y el orden es `#604` primero: sin él, el tope que vigilaría `#605` sería el
de una pila que no es la que el programa pidió (el hilo de la GUI pide 8 KB y en la VM-C tiene 2).

**`#605` — el muro:**

- **Dónde**: en `ENTER`. Toda función empieza por él (el prólogo que pone `ModWriter`, también con cero locales), así
  que es una comparación por llamada: `sp + locales + zona roja > tope del hilo` → `RuntimeError` «desbordamiento de
  pila», atrapable, con el mismo texto en las dos VMs. Las dos VMs llevan el mismo marco (el mismo bytecode, las mismas
  cuentas de `sp`), así que la excepción sale en la MISMA instrucción: la paridad incluye el desbordamiento.
- **La zona roja** (256 B): lo que una función puede apilar después de su `ENTER` sin volver a mirar —los operandos
  de una expresión, los argumentos de una llamada y los 12 B que empuja el `CALL`— hasta el `ENTER` siguiente. La
  inyección de un evento ya mira su propio sitio (`events.c`).
- **El `main`, 16 KB de verdad** (lo que ya dicen las dos VMs; hoy usaba en silencio toda la región). Antes de fijarlo
  se mide cuánta pila usa el corpus: una marca del máximo por hilo, que queda como diagnóstico al acabar el RUN.
- **Las natives**: en la VM-C corren en la pila de C de su ejecutor y en miVM son bytecode, así que una recursión
  DENTRO de una native sólo la ve miVM. Diferencia conocida; se documenta.

**`#604` — los hilos de la VM-C, como miVM:**

- **Reutilizar el hueco** de un hilo terminado cuando ya no le queda nada: terminado, sin eventos pendientes (`#342`),
  sin nadie en su `join` y sin ejecutor de natives atado. Al reutilizarlo, el campo `__tid` del `Thread` viejo pasa a 0
  —su `join` vuelve enseguida, como el de un hilo terminado en miVM— y se limpia lo que arrastraría del anterior (la
  pila de manejadores de excepción, el ancla del GC, el paso a paso, la línea de `print`, la caché de `cb`).
- **Reutilizar su región de pila** si le cabe lo que pide el nuevo (la primera que quepa); si no, una nueva de la
  región libre.
- **Respetar `Thread(stackSize)`**: 0 = 2 KB, como miVM; el tope del hilo es el tamaño PEDIDO, no el de la región
  que le tocó, para que `#605` salte en el mismo sitio en las dos VMs.
- **Los límites, con excepciones atrapables y el mismo texto en las dos VMs**: más de 32 hilos VIVOS a la vez (hoy la
  VM-C muere con un error que no se atrapa, y miVM no tiene tope: se le pone el mismo) y sin memoria para la pila del
  hilo (el punto en que pasa depende del tamaño de la región de pilas de cada VM, como el heap lleno).

**Las pruebas**: `HilosMuchos` (40 hilos de uno en uno) y `PilaHonda` (un hilo que recursa; con `try`, la excepción
atrapada y el vecino intacto) al corpus, en la paridad y con dos workers; y los sabotajes: sin la reutilización, rojo
en el 32; sin el muro, el vecino roto.

### `#593` — la reserva en UN método, y el GC que comprueba (29-sep, decidido por Eduardo)

Eduardo, a la propuesta de cruzar la tabla de handles y la lista de huecos para corregir un heap que miente: *«la
cuestión real es ¿ese error se está produciendo de verdad o es hipotético? Y si se produce, ¿por qué?»*; *«lo que hay
que garantizar es que la operación sea atómica, no interrumpible»*; *«al final la reserva la ha de hacer un método
único, no n métodos»*; y *«si se detecta en un bloque vivo y un programa funcionando se debería producir una
excepción»*. Lo real, medido: las cabeceras que mintieron fueron fallos de la VM (`#597`, `#598` y `#603`, una
liberación sin cerrojo); ninguna vino de un programa.

**Lo que hay** (29-sep): la reserva la reparten el asignador —pone el tipo y un 0 en la palabra de la longitud o la
clase— y los llamadores —23 en la VM-C, 15 en miVM—, que escriben esa palabra a mano y dan de alta el handle aparte,
a veces después de llenar el contenido. Hoy todos lo hacen bien (sin otra reserva por medio), pero por disciplina.

**El método único** (`bpvm_heap_nuevo` en la VM-C; `heapNuevo` en miVM): bajo el cerrojo y de una vez, el hueco, la
cabecera ENTERA (el tipo, y la longitud o la clase), el contenido a cero y el handle, y cuenta el bloque. Devuelve el
handle; quien llama llena el contenido por él. El asignador de antes queda por dentro, sin llamadores fuera. La ABI
de los `.mdn` no cambia: los helpers del AOT siguen igual por fuera y reservan por dentro con el método único.

**Las dos comprobaciones de cada GC**, las dos con la excepción de `#494` y sin corregir nada:

- **La cuenta**: la reserva suma un bloque, la liberación de un owner resta uno y el barrido deja la de los que
  quedan; el recorrido cuenta los bloques no libres que ve. Si no cuadran, el recorrido se ha saltado bloques (`#593`)
  o se ha inventado alguno (el sobrante de `#597`). Contar BYTES no valdría: el bloque que miente se traga justo los
  bytes de los que se salta.
- **Los handles**: todo handle vivo que apunta dentro del heap tiene que caer en una cabecera que el recorrido vio.

**Las pruebas**: el gancho de `test-heapdescarrilado` (VM-C) y de `HeapDescarriladoTest` (miVM) gana el caso de
`#593` —una cabecera que dice lo justo para tragarse el bloque SIGUIENTE, calculado en cada VM porque los heaps no son
iguales—, que hoy pasa mudo; con el arreglo, la excepción. Y la red entera: ningún programa sano puede dar un falso
positivo.

**La prueba de estrés, antes de tocar nada** (29-sep, noche). Eduardo: *«Antes de hacerlo yo lo testearía en el PC,
con 1 hilo y con varios, a ver que pasa. Los temas de memoria ya los hemos padecido otras veces y hay que estar muy
seguro antes de tocar nada.»* Dos programas, cientos de veces, con 1, 2 y 4 workers, contra el árbol de antes de
`#593` y el de ahora: `HeapEstres` —4 hilos crean cadenas, arrays y objetos con los builtins que reservan, se los
pasan por un buzón con `Mutex` y comprueban lo que reciben; con poca memoria, ~1.000 colectas por ejecución— y
`TablaCrece` —4 hilos guardan cadenas VIVAS, así que la tabla de handles crece mientras los otros la leen—. Tres
fallos, los tres anteriores a `#593`, cada uno probado cambiando UNA cosa:

- **miVM, `split` y `listDir`** (`#611`): reservan los trozos ANTES que el array y los guardan en un `long[]` de Java
  que el GC no ve (el ancla sólo guarda el último); si una reserva dispara el GC, los trozos ya hechos se liberan. Con
  un heap pequeño falla SIEMPRE, también con UN worker: use-after-free, o un `OutOfMemoryError` de Java al leer una
  longitud basura. Con el array primero y apilado, como la VM-C: de 60/60 fallos a 0/60. Y al reventar así, miVM
  acababa con salida 0 y sin imprimir nada (`#612`).
- **VM-C antes de `#593`, con 2 o más workers** (del 3 al 7 % de las ejecuciones): use-after-free falsos, «strCopy:
  dst no es ref a heap», un `ALOAD` con longitud 0 y resultados mal SIN ningún error. La causa: la tabla de handles
  **crece moviéndose** —`bpvm_handles_grow`, un `memmove` dentro del heap: el `addr[]` nuevo cae encima del `gen[]`
  viejo, y el final del `addr[]` viejo se pone a cero— mientras los otros workers la leen sin cerrojo (`bpref_deref`).
  Quien lee con los punteros viejos encuentra una DIRECCIÓN donde espera una generación («gen del slot=19972», con el
  heap en 12864..30720) o un cero donde espera una dirección. Pasaba por el crecimiento de «último recurso» del alta
  del handle, que iba en su propia toma del cerrojo, separada de la reserva: varios workers pasaban la puerta con la
  tabla casi llena y el último alta la desbordaba. La prueba: el mismo código con la tabla nueva puesta sin pisar la
  vieja, de 19/400 fallos a 0/400.
- **VM-C de ahora** (`#610`): `#593` cierra ese camino —la puerta y el alta van juntas, y la tabla sólo crece en la
  puerta, justo tras una colecta propia, con los demás esperando el cerrojo: 0/240 con el GC en marcha—, pero la causa
  sigue. Con el GC apagado (`--nogc`, el `gc=0` del ENV) y 2 o 4 workers, la tabla crece sin parar a nadie:
  `TablaCrece` corrompe 58 y 93 de 150. Eduardo avisó: *«aunque GC esté parado todavía el final de una función con
  variables owner podría intentar liberar objetos»*. Descartado aquí —cero `FREE_REF` y `SET_FIELD_OWNER` en los
  programas y en Core, y es el único camino que libera fuera del GC—, así que sin GC ningún slot se recicla y una
  generación distinta de 0 es otra cosa leída en su sitio. El arreglo: crecer con el mundo parado, en la puerta de la
  reserva (quien reserva aún no tiene nada a medias), como se colecta. El prototipo, sobre una copia: 0/300, y nada
  roto con el GC en marcha ni en las pruebas de C del heap y del SMP.

**Lo que decide el diseño de la reserva**: una ESPERA dentro de la ventana entre la reserva y el alta del handle
corrompe. Medido: parar el mundo en el alta (para crecer allí) subió los fallos al 26-35 %, porque el GC de otro worker
liberaba el bloque a medio nacer. Así que el método único se queda como está, con el handle al nacer: separar otra vez
el alta (llenar fuera y publicar al final) volvería a abrir esa ventana.

**Para H12** (multinúcleo de verdad, con memoria de orden débil: el P4, la RP2350). El alta publica el slot con
RELEASE y `bpref_deref` lo lee con ACQUIRE (el «paso 7c» de V4); con el handle al nacer, lo que se escribe DESPUÉS
—el contenido que llena quien reserva— ya no va detrás de esa barrera. Un programa que pasa un objeto a otro hilo con
un `Mutex`, un `join` o un `start` no lo nota: esas operaciones ya ordenan la memoria. Uno que lo comparta SIN
sincronizar (una carrera del programa) podría, en otro núcleo, ver el handle antes que el contenido. Tampoco antes era
estanco (el handle guardado en un campo compartido podía verse antes que la barrera del alta). Cuando H12 llegue:
decidir si se garantiza también la publicación sin sincronizar (una barrera RELEASE al guardar una referencia en un
campo, un array o una global) o se documenta que compartir sin sincronizar no está definido. Y en miVM, la tabla de
handles la leen los workers sin sincronizar y sus campos no son `volatile`: un lector podría ver un array viejo tras
crecer (teórico; no se ha visto).

### `#607` — los manejadores de la GUI: lo que corre ANIDADO de verdad (30-sep, el mapa y lo medido)

**La premisa de la ficha ya no es cierta en lo principal.** Desde `#324` (tanda 2c) el `onClick`/`onChange` que escribe
el usuario —suscrito con `btn.onClick := obj::m`, o por Forms con `bindHandler` y `__methodRef`— NO corre anidado: lo
único anidado del lazo es un trampolín, `Gui.__guiDispatch(self)` → `self.__fireClick()` → `raise onClick(this)`, y
`raise` sólo encola. El manejador lo inyecta el mecanismo de eventos de H5.c en el hilo de la GUI (`HiloGui`), entre
cuantos, como una llamada normal: puede ceder y lanzar como cualquier función.

**Medido** (`GuiCede`, un clic sintético con `Gui.__guiClick`; las dos VMs, el mismo stdout):

| el caso | lo que pasa |
|---|---|
| un `onClick` con `sleep(20)` en medio | funciona: lo de después del `sleep` se ejecuta; salida 0 |
| un `onClick` que lanza sin atrapar | muere el hilo de la GUI, el `main` sigue, y al final salida 1 (la regla de `#608`) |
| el setter de una ventana llamado POR RANURA (lo que hace Forms al cargar: `bindMember` → `__guiInvokeBySlot`) con un `sleep` | se CORTA en el `sleep`, en silencio (la VM-C lo dice sólo en el stderr: «la función BP cedió/falló»); salida 0 |
| ese setter lanzando, con un `try` alrededor en el `main` | la VM-C la atrapa —el `catch` y el resto del `main` corren DENTRO del intérprete anidado— y sale con 0; miVM no la atrapa, mata el `main` y sale con 1: **las dos VMs divergen** |

**Lo que corre anidado hoy** (el mapa, comprobado en el código): `__guiInvokeBySlot` —Forms: el setter de la ventana al
cargar, en el hilo que llama a `load`—; `__guiInvokeByName` y `__guiRun`, sin llamadores en la stdlib; y el trampolín
dentro de `__guiRunOnce`, que sólo levanta el evento (cedería sólo si una subclase sobrescribiera `__fireClick` o
`__fireChange`, que son públicos). Las natives del AOT que llaman a BP tienen su propia frontera (P2b).

**Para el setter, tres caminos:**
- (A) **Un opcode, `INVOKE_SLOT`** (`0xBE`, el primero libre; MOD9 aún no está publicado): un `INVOKE_VIRTUAL` que toma
  la ranura de la pila. `__guiInvokeBySlot` pasa a ser `inline INVOKE_SLOT` en `Gui.bp`, y el setter, una llamada
  normal en el mismo hilo: cede, y su excepción llega al `catch` de quien llamó, igual en las dos VMs.
- (B) El builtin monta el marco y vuelve al intérprete (la VM-C ya relee pc/sp/bp/cs tras un `CALL_BUILTIN`; miVM sólo
  sp): el mismo efecto, menos claro.
- (C) Dejar el anidado y hacer honesto el puente: una `RuntimeError` atrapable, «un setter de Forms no puede ceder»,
  igual en las dos VMs. La restricción se queda.

**El resto**: `__guiInvokeByName` y `__guiRun`, sin llamadores, se pueden quitar de la API (sus números se quedan
reservados: van dentro de los `.mod`); el trampolín, o declarar internos los `__fire*`, o que la VM encole el evento
directamente (con la misma tabla que `__guiSlotOf`).

**Decide Eduardo**: el camino del setter; y qué hace una excepción sin atrapar en un manejador de la GUI —hoy, como en
cualquier hilo (`#608`): se acaba el hilo de la GUI, la pantalla se queda quieta y el programa sale con 1; o se dice y
la GUI sigue, como el hilo de eventos de Swing—.

**Decidido y hecho** (30-sep, `85de188f`): el camino (A), el opcode `INVOKE_SLOT`; y la excepción de un manejador
—la de cualquier hilo que no es el main—, «que no sea mudo»: acaba su hilo (`#608`) y lo dice en la salida del
programa en ese momento, igual en las dos VMs. `__guiInvokeBySlot` se queda (interfaz de `Gui`, `#500`). El
detalle, en `FICHAS` `#607`.

### `CALL_REL` — medido (30-sep)

`fib(32)` interpretado (unos 7 millones de `CALL`) en el PC, las dos VMs alternadas, 15 y 15, dos tandas:

- quitar del `CALL` la base `cb` del todo —lo más que daría `CALL_REL`—: del **−1,0 al −1,7 %**;
- el acierto de la caché de `cb` en línea, sin tocar el formato: del **0,0 al +0,3 %**, nada. Lo que cuesta no es
  llamar a `bpvm_cb_for_cs`, es la comprobación misma.

En el peor caso posible (un programa que no hace más que llamar), un 1,7 % como mucho; en uno normal, menos. En
contra, subir el formato con toda su cascada (la stdlib, los packs, las imágenes y las nueve placas) y el `CALL` de
siempre para siempre. **Propuesta: cerrarla.** Si se quiere la certeza del micro, la misma medida en la Pico (grabar).
**CERRADA** (Eduardo, 30-sep: *«1 la cerramos»*). La base `cb` se queda como está.

### Los 32 MB del P4 — dónde está de verdad cada techo (30-sep)

Eduardo, al ver el paso 0: *«el límite de la flash de 16M en la P4 era para código ejecutable. Para FS no debería
haber límite»*; y *«cuando es el kit tiene 16M y cuando es la de Waveshare son 32M y lo sabemos porque nos lo han
dicho ellas»*. Leído en el IDF (v6.0.1), son dos techos distintos:

- **Ejecutar desde la flash** (la zona de packs, que se mapea): `spi_flash_mmap` rechaza con `ESP_ERR_INVALID_ARG`
  todo lo que pase de 24 bits (16 MB), en todas las ESP32, salvo con `CONFIG_BOOTLOADER_CACHE_32BIT_ADDR_*`
  (experimental; no la usamos). Es el `err=258` del revert `2dc12d5b`. No depende del chip ni de cómo se grabe.
- **Leer, escribir y borrar** (el FS): `esp_flash_read/write/erase` sólo miran `chip->size`, y ése es el tamaño de la
  CABECERA de la imagen (`esp_flash_types.h`: *«this stands for the size in the binary image header»*; al arrancar,
  *«Detected size larger than the size in the binary image header. Using the size in the binary image header»*).
  Grabamos con `--flash-size 16MB` para que la misma imagen arranque en el kit (con 32, el kit para al arrancar:
  *«Detected size smaller than the size in the binary image header. Probe failed»*). Por encima de 16 MB, el driver
  genérico ya manda comandos de 4 bytes en cada operación (`IS_ADDR_32BIT`), sin opciones.
- **El tamaño real** lo dice cada placa: `esp_flash_get_physical_size` —la línea `flash:` del arranque, y el INFO,
  33 554 432 B en la Waveshare—.
- Grabar no es el problema (lo que Eduardo trajo de la red sobre esptool: el stub, `--flash-size`, QIO/DIO): el stub
  ya se usa, la imagen entera vive por debajo de 16 MB y grabamos en DIO. `--flash-size detect` escribiría en la
  cabecera, al grabar, lo que mida cada placa; pero depende de cómo se grabe (el IDE, `idf.py`), y leerlo al
  arrancar, no.

**El camino que sale** (una imagen; el tamaño, el que diga la placa al arrancar, como la variante y el panel): si el
chip físico es mayor que la cabecera, subir `esp_flash_default_chip->size` al físico; la zona de packs sigue en
`bpdata`, por debajo de 16 MB —lo único que pide el mapeo—, y el FS gana los 16 MB de arriba. Dos formas de
dárselos, a decidir: (a) el FS entero arriba, contiguo, y los packs con todo `bpdata`; (b) el FS en dos trozos —el
suyo de `bpdata` y el de arriba— unidos en el dispositivo de bloques de littlefs. Las dos piden reformatear el FS de
la Waveshare una vez; el kit y las demás placas no cambian. La C5 lo tendría más fácil: su imagen ya declara 32 MB
(su `chip->size` ya es 32) y su `bpdata` acaba en 16. **Lo primero, medir** en la Waveshare con una imagen de prueba
(con el OK de Eduardo): subir el tamaño y LEER por encima de 16 MB comparando con lo de abajo —si el driver cortara a
24 bits, leería lo de abajo: se sabe sin escribir nada—; y sólo si no hay eco, escribir un sector de arriba y
releerlo.

**Medido el 30-sep** (con el OK de Eduardo; una imagen de prueba que no se commitea, que sólo LEE): sin subir el
tamaño, leer en 16 MB + 8 KB da `rc=258` (`ESP_ERR_INVALID_ARG`): el límite es el `size` de la cabecera. Subido a
32 MB, las diez lecturas de arriba dan `rc=0` y `ff ff…` (borrado), y las de abajo, lo suyo: el bootloader
(`e9 03…` en 0x2000), la tabla (`aa 50…` en 0x8000), la app, el ENV (`42 50 45 56`, «BPEV») y el FS. **Sin eco**:
con la imagen de 16 MB, la Waveshare LEE sus 16 MB de arriba en cuanto el driver sabe el tamaño real (las dos
«iguales» eran zonas borradas por los dos lados, la cola vacía de la partición de la app). Lo que falta si se
sigue: la ESCRITURA —un sector arriba, con el OK de Eduardo; en 16 MB + 0x300000, cuyo eco, si lo hubiera, caería en
esa cola vacía— y decidir (a) o (b).

**La escritura, medida el 30-sep** (con el OK; otra imagen de prueba, ya retirada): con el tamaño subido, borrar y
escribir el sector de 16 MB + 0x300000 dan `rc=0`, se relee el patrón exacto («BP32»…), su eco de abajo (0x300000)
sigue borrado, y el sector se deja borrado. **La Waveshare lee, escribe y borra sus 16 MB de arriba con la imagen de
16.**

**Decidido por Eduardo (30-sep):** *«para ahora con 16M de FS la P4 va sobrada. Y el resto para el SO y los packs.
Más adelante podemos estudiar hacer FS más grandes de 16M. En cuanto a los packs, los 16M físicos hacen de límite.
Cuando el P4 soporte FS >16M entonces podemos hacer packs también más pequeños.»* O sea, la (a): en la Waveshare,
el FS entero en los 16 MB de arriba y `bpdata` entero para los packs; el kit, como está. Lo que pide al hacerlo:
(1) la regla, en el común de la ESP32 —si el chip dice más que la cabecera, subir el `size` del driver y dar la
región de arriba al FS—; (2) qué hace `part.fs.size` en esa placa; y (3) en la Waveshare, formatear el FS nuevo (el
de hoy, con su `/app`, se queda en `bpdata` sin montar) y rehacer la zona de packs, que pasa a empezar en
`bpdata+0` (`PACK_FORMAT` y volver a grabar sus packs) —las dos cosas, con el OK de Eduardo—.

✅ **HECHO el 30-sep** (`d5b828b1`, `a91b8dad`; Eduardo: *«si la C5 tiene 32M hacemos lo mismo que con la P4»*):
- **La regla** (común a las cinco ESP32): si el chip tiene flash por encima del final de `bpdata` (al menos 1 MB),
  el arranque sube el `size` del driver al físico, registra ese trozo como la partición «bpfsalto» y el FS se
  monta ahí, entero; `bpdata` queda para los packs. En el kit, la S3, la C3 y la C6 no cambia nada. El reparto es
  `bpvm_part_layout_fs_arriba` (común, con su prueba) y el gestor lo cuenta: `PART_LS` con `fsArriba`, y
  `PART_APPLY`/`PART_DEFAULTS` contestan «no se reparte»; el IDE lo enseña. `part.fs.size` no se aplica allí.
- **En las dos placas**, paso a paso con su OK: copia del FS al PC (61 ficheros en la Waveshare, 33 en la C5;
  menos `/lib`, que repone el SO, y la SD); la imagen (el FS nuevo nace vacío y formateado, y la zona de packs,
  en `bpdata+0` sobre el FS viejo, dice «cadena ROTA» sin caerse); `PACK_FORMAT`; los packs sellados otra vez
  (la Waveshare, SQLite y Gui, `riscv`: le quedan 8,5 MB libres, antes 1,4; la C5, Gui y DriverSpi,
  `riscv-imc`); la copia subida, igual fichero a fichero; y los programas: `Bench` con AOT, `SqlDemo` y
  `GuiShot` en la Waveshare; `GuiFlushBench`, `GuiShot` y `C5SdGui` en la C5.
- **Lo que salió en la C5**: con la zona de 15 MB, la segunda vista (DATOS) no cabe en el espacio virtual
  (`ESP_ERR_NOT_FOUND`: `esp_mmu_map` busca un hueco del tamaño entero antes de ver que ese trozo ya está
  mapeado) y se lee por la de INST, que en estos micros es la misma dirección: funciona, y el log ya lo explica.
- **Más adelante**, lo que dijo Eduardo: un FS de más de 16 MB, y entonces una zona de packs más pequeña.

## H7 — los márgenes de la Pico y de la STM32, MEDIDOS (30-sep)

Del mapa (fila «Los restos de `U6`»): *«los márgenes de la Pico y la STM32 como MEDIDA: pequeño, pero hoy no hay
instrumento»*. El de la Pico eran los 64 KB de `VM_SRAM_MALLOC_MARGIN`, puestos a mano en julio (y desbordados una
vez: `#440`/`#449`); el de la STM32, lo que deja el enlazador. Ninguno se había medido.

**El instrumento** (`a2bdff6e`): la línea `mem:` tras cada RUN, antes de soltar la VM, como la de la ESP32 (`#336`):
- en la Pico, `malloc` —lo más alto que ha llegado `sbrk`, sobre lo que tiene— y el heap de FreeRTOS con su mínimo
  histórico;
- en la STM32, lo mismo (newlib-nano no devuelve memoria a `sbrk`: su `arena` es la marca de agua) y la pila del MSP,
  pintada en `main()` antes de arrancar el planificador;
- y el protocolo de `U6.4` para las cinco familias: `tools/medir_margen.py` (el `.ps1` sólo hablaba con las ESP32).
  Se niega si el ENV no tiene `log=1`: la primera pasada en la DK2 salió MUDA —sólo las líneas del arranque— porque
  el rastro de ejecución está apagado por defecto (`#423`).

**Lo medido** (con el OK de Eduardo, las dos grabadas con esa imagen y `log=1` en el ENV de la DK2): el protocolo de
`U6.4` (`Bench` ×2) y la tanda de V7 ENTERA, programa a programa tras un RESET —la Pico, 27 idénticas y 2 saltadas
por pantalla; la DK2, las 29—:

| | Pico 2 (RP2350A) | DK2 (STM32U5G9) |
|---|---|---|
| **`malloc`: lo que tiene** | 64 KB (`VM_SRAM_MALLOC_MARGIN`) | 333 KB (lo que deja el enlazador) |
| — con `Bench` ×2 | 35 888 B | 29 420 B |
| — con la tanda de V7 | **48 176 B** (`NatV7`) | **141 560 B** (`GuiShot` detrás de `GuiWinJson`; sola, 117 096) |
| — lo que sobra | 17 360 B (26 %) | 199 568 B (58 %) |
| **heap de FreeRTOS** | 32 KB | 24 KB |
| — mínimo libre | 10 368 B (32 %) | **2 176 B (9 %)** |
| **pila del MSP** | — (la Pico la tiene aparte, en `SCRATCH_Y`) | 16 KB, de los que usa **≤ 272 B** |

Lo que se comprobó para fiarse de las cifras:
- el pico es del programa, no de lo que corrió antes: `NatV7` solo tras un reset da los mismos 48 176 B, y repetido no
  crece; en la DK2, las dos de GUI alternadas tres veces se quedan en 141 556. Lo que sí pesa es el orden: `GuiShot`
  sola, 117 096; detrás de `GuiWinJson`, 24 KB más —la fragmentación de newlib-nano—;
- lo que queda en uso al final de un RUN vuelve a su valor con el mismo programa, sin fuga (con `Bench`, 23 440 B en la
  Pico y 23 476 en la DK2);
- el heap de FreeRTOS pierde 280 B mientras vive un nativo (su `fr_thread_t` y su semáforo) y los recupera al soltar
  la VM.

**Lo que dice:**
1. **La Pico, bien.** El peor programa de la tanda usa el 74 % del margen, y lo que lo empuja se sabe: la pila de 8 KB
   del ejecutor de cada nativo sale de `malloc` (desde P2b.3), además de las tablas del cargador.
2. **La DK2, tres cosas:**
   - la pila del MSP es una reliquia: 16 KB desde V3/H5.0, cuando el intérprete corría sobre ella (lo dice su `.ld`);
     desde A1.5 corre en la tarea `vm` y el MSP sólo lo usan las interrupciones —≤ 272 B con la GUI, el táctil y el
     wire en marcha—. La Nucleo tiene 4 KB;
   - el heap de FreeRTOS es el más justo del parque: 2 176 B en el peor momento (la tarea `vm`, 16 KB; `io`, 4 KB; la
     ociosa y los objetos del kernel). Una tarea o una cola más y no cabe. Es el `FreeRTOSConfig.h` de la familia, así
     que la Nucleo va igual (no se midió: no estaba conectada);
   - `malloc` tiene 333 KB para un pico de 142: lo sube la GUI (la tabla de símbolos de `Gui` y la fragmentación), y
     crece con el tamaño de la app.

**Lo que decide Eduardo** (la propuesta) — ✅ **decidido el 30-sep: las dos; hecho con `2a597152`**:
- **el MSP de la DK2, de 16 a 4 KB**, como la Nucleo: 15 veces lo medido;
- **el heap de FreeRTOS de la STM32, de 24 a 32 KB** —el de la Pico—: en la DK2, con lo que suelta el MSP; en la
  Nucleo, de lo que deja el enlazador;
- **lo que sobra de `malloc` en la DK2, dejarlo**: es el único consumidor que crece con la app, y la VM ya tiene 1 MB.

**Medido después, en la DK2** (grabada con `2a597152`, la tanda de V7 entera otra vez, programa a programa tras un
RESET, las 29 bien): el heap de FreeRTOS, **10 368 B** libres en el peor momento (antes 2 176); el MSP, 272 B de 4 096;
y `malloc`, 141 560 B de 345 224 (tenía 341 128: suelta 12 KB el MSP y se lleva 8 FreeRTOS). La Nucleo compila con
los dos cambios (le quedan ~129 KB para `malloc`); no se ha grabado: no estaba conectada.

De paso (`64487df4`): el aviso del arranque de la ESP32 «el asignador solo dio 6653 KB de los 6653 planificados» (en la
C5) era falso —comparaba el bloque pedido, en KB enteros, con el plan sin redondear—.

## H7 — Boot 2 y el montaje de packs: lo que hay y la propuesta (30-sep, noche)

Eduardo respondió a las tres preguntas del mapa (arriba, en su fila): la zona sin formatear *«no es un error, es
correcto; sin embargo tampoco se pueden subir packs, así que lo podemos señalar como un estado más, ni totalmente
correcto pero casi»*; montar exige el FS; y se unifican el montaje, el cargador y el bloque de RAM. Lo que hay hoy,
leído en el código de las cinco familias (Pico, ESP32, P4 —con sus dos transportes—, STM32 y el micro simulado):

**El montaje.** El único común es `bpvm_pack_mount` (guarda la base y el tamaño; no lee la zona). Cada familia
lo envuelve a su manera y con su condición: la ESP32 y la STM32 montan con sólo las particiones; la Pico y la P4,
con el FS (lo que decidió Eduardo). De ahí dos incoherencias: en la Pico sin FS, `PACK_LS` enseña la zona que no
está montada (e INFO dice `packsBytes=0`); en la P4 sin FS, `PACK_LS` y `PACK_FORMAT` dicen «sin zona».

**Qué es «sin formatear».** La zona no tiene cabecera: la cadena de packs empieza en el byte 0. Una zona BORRADA
(`0xFF`) ya es una cadena vacía y válida —se puede grabar—; lo que Eduardo llama sin formatear es la que tiene
RESTOS de antes: la cadena no es válida y grabar da `NO_SPACE`. Hoy el arranque no lo dice; `PACK_LS` sí
(`chainOk:false`), y en el primer RUN el log dice «la zona no tiene ningún pack grabado», el mismo texto que con
una zona vacía. Y un pack cuyo motor no carga sólo se ve con `LOG_DUMP`.

**El cargador.** La validación ya es común (`bpvm_npack.c`: la escalera, el tope y el reparto de G1; y
`bpvm_bios.c`: `publica`/`busca` y la arena). Lo triplicado es el orquestador de cada familia —unas 160 líneas
iguales por copia: la cola de `cargar`, el núcleo de `saltar`, lo común de `explicar`—; lo propio de verdad son 8
líneas en la Pico, 14 en la STM32 y 30 en la ESP32. La guarda «cargar una vez» vive en el REPL de cada una, y la
dispara el primer RUN (la Pico) o el primer RUN o INFO (la ESP32 y la STM32). Y un hueco latente: el ancla de la
BIOS apunta al cargador SIN la guarda —hoy no lo llama nadie; si alguien lo hiciera, volvería a copiar `.data` y
`.bss` encima de un motor vivo—.

**El bloque de RAM.** Tres reglas: la ESP32 y la STM32 leen `packRam=` (KB, redondeado a 4 KB) y si no `SQLite=`
(MB); la Pico, sólo `SQLite=`. Y dos sitios: en la Pico y la STM32 sale del plan de memoria común; en la ESP32 y la
P4, de una reserva aparte, antes del heap.

**El Boot 2.** Cinco copias del mismo esqueleto con órdenes distintos. Dos fallos de paso: la P4 por TCP no aplica
`stack=`, `quantum=` ni `log=` (el camino UART sí); y el autorun de la STM32 no mira si la placa llegó a la app sin
degradar (las otras tres, sí).

### La propuesta — ✅ **decidida por Eduardo el 30-sep**: el «casi» así, y las fases en este orden (empezando ya por la 1)

**El «casi» de Eduardo, sin renumerar.** El arranque sigue subiendo 0 → 1 → 2 (FS) → 3 (app) —los números los leen
el IDE, las pruebas y los documentos—, y al llegar al FS se monta la zona (la regla de Eduardo) y se mira su ESTADO,
que es de la zona y no de la subida:

| la zona | ¿sube a la app? | lo que se dice |
|---|---|---|
| sin partición de packs | sí | nada: es una placa sin packs |
| **sin formatear** (restos: la cadena no es válida) | sí | **casi**: «la zona de packs está sin formatear: no se pueden subir packs» |
| lista (vacía o con packs) | sí | nada |
| un pack cuyo motor no carga | sí | **casi**, con el motivo y el nombre del pack |
| el FS no monta | no (se queda en 1, como hoy) | degradada; la zona ni se monta |

`STATE` añade el campo (`"casi":true,"aviso":"…"`); el IDE lo enseña en la gestión de placa y, al conectar, la
propone igual que con una degradada (allí está «Formatear la zona»). La otra forma —un estado con número, 3 = packs
y 4 = app— obliga a renumerar y, sobre todo, cortaría la subida con la zona sin formatear, que Eduardo quiere
correcta.

**Por fases**, cada una probada en las cuatro placas de código distinto (la Pico, la DK2, la C5 y la P4) y en el
micro simulado:
1. ✅ **HECHA el 30-sep, `da19145e`** —en el host (`test_pack` 15-16), en el micro simulado con `--zona-sucia` (el
   «casi» de punta a punta: `STATE`, `PACK_LS`, formatear) y en la Pico, la DK2, la C5 y la P4 (el camino normal:
   monta con el FS, «zona lista», sin «casi», los packs y un programa)—. **El estado de la zona y el montaje,
   comunes** (pequeño-medio): `bpvm_packs_montar` —exige el FS, monta con el
   gancho de la familia (XIP fijo en la Pico y la STM32; el mapeo de la MMU en la ESP32 y la P4), mira la cadena y lo
   dice en el log del arranque—; el `casi` en `STATE` y en el IDE. Arregla las dos incoherencias de arriba.
2. ✅ **HECHA el 30-sep, `ef55fc9d`** —`src/bpvm_npack_carga.c`; en el host (`test_npack`, todo menos el salto:
   sin zona, sin RAM, zona vacía, un motor de otra arquitectura, y el `STATE`), y en las placas: la DK2 carga sus tres
   motores y la C5 sus dos UNA vez con dos INFO y dos RUN, la P4 con SQLite y la GUI, la Pico sin packs igual—. Un
   cambio sobre lo propuesto: la guarda marca, como marcaban las tres, sólo cuando se saltó a alguno (si no, el
   siguiente RUN lo reintenta: lo que se quiere tras grabar el primer pack, sin reiniciar). **El cargador común**
   (medio): la cola de `cargar`, `saltar` y `explicar` en `src/`, con la guarda «una vez»
   DENTRO —cierra el hueco del ancla— y el `log_flush` antes de saltar en las tres; lo dispara el primer RUN o INFO
   en todas; un motor que no carga, al `casi`. Ganchos: la zona, la BIOS, el bloque de RAM y (sólo la ESP32) releer
   la cabecera para el diagnóstico.
3. ✅ **HECHA el 30-sep, `72c3ab6f`** —`bpvm_packram_region`; en el host (`test_sqlmem`, y `test-sqlmem` a la red) y
   en las placas: los mismos bloques de antes (la Pico 8 KB en la cola de la SRAM, la DK2 768 KB, la C5 1024, la P4
   4 MB por `SQLite=4`) y, en la Pico sin PSRAM, `packRam=64` hace la cola de 64 KB en `0x20070000` y la VM baja lo
   mismo (346 → 290 KB)—. En la Pico sin PSRAM, `packRam=` cambia la COLA de la SRAM, que crece hacia abajo desde el
   techo: un motor sellado contra la de 8 KB sigue dentro. **La regla del bloque de RAM, común** (pequeño): `packRam=` y, si no, `SQLite=`, en una función; la familia sólo
   COLOCA (el plan en la Pico y la STM32, la reserva aparte en la ESP32). `packRam=` llega a la Pico.
4. ✅ **HECHA el 1-oct, `e9ca7c30`** —en el host (`test_boot2`, saboteada dos veces, y a la red) y en las cuatro
   placas: el arranque en el orden del diseño, sano y con sus packs, y un programa (la Pico `Hello`, la DK2 y la C5
   `GuiFlushBench`, la P4 `SqlDemo`); y en la Pico, el fallo nuevo de punta a punta: con un autorun corriendo, la
   imagen de antes contesta al `HELLO` «no ha registrado su cintura» y la nueva, el `HELLO_REPLY`. La P4 por TCP,
   compilada (no hay placa con Ethernet); la puerta del autorun de la STM32, sólo en el host (en la placa no se da:
   su capa `app` siempre sube, y sin FS no hay `auto.txt`)—. **El Boot 2 común** (medio): el orden, una vez, con
   ganchos por familia; arregla los dos fallos de paso (la P4 por TCP y el autorun de la STM32) y declara las capas
   del SO (`H9_KERNEL_CAPAS.md`). El diseño, abajo («La fase 4»).

### La fase 4 — el Boot 2 común: lo que hay y el diseño (1-oct)

**Lo que hay**, leído en las cinco copias (la Pico, la ESP32, la P4 por UART y por TCP, la STM32). El esqueleto es el
mismo —el ENV, la memoria, subir, la zona, `/lib`, el hardware, los buses, la SD, el cable, el autorun, el REPL— y lo
que cambia de una a otra es esto:

| paso | Pico | ESP32 | P4 (UART · TCP) | STM32 |
|---|---|---|---|---|
| `stack=` y `quantum=` | antes de la memoria | al final | al final · **nunca** | al final |
| `log=` | al final | al final | al final · **nunca** | al final |
| el autorun | sólo con la placa sana | sólo sana | sólo sana | **siempre** |
| la cintura del REPL | **tras el autorun** | **tras el autorun** | **tras el autorun** | antes de subir |
| los buses y la SD | antes de la memoria | tras el hardware | la SD, antes del hardware | sin SD (ni su línea) |
| la BIOS | antes de subir | tras la zona | en `app_main` | antes de subir |
| volcar el log a la flash | al entrar al REPL, tras el autorun | tras subir | tras subir | antes del cable |
| la línea «boot: estado» | `main.c` | `board_mgr_esp32.c` | ídem | `stm32_repl.c`, con «DEGRADED» |

**Tres fallos de paso**: los dos sabidos —la P4 por TCP no aplica `stack=`, `quantum=` ni `log=`; el autorun de la
STM32 no mira si la placa llegó sana— y uno nuevo: en la Pico y en las ESP32 (la P4 también) la cintura del REPL se
registra al entrar al REPL, o sea DESPUÉS del autorun. Un IDE que se conecta durante un autorun —en su ventana de
10 s o con el programa corriendo— saluda (`HELLO`, lo único que se atiende entonces con `KILL` y `RESET`) y recibe
`HELLO: este firmware no ha registrado su cintura de REPL`: el IDE se lo traga («handshake falló») y se queda sin la
identidad de la placa. La STM32 y el micro simulado la registran antes de nada.

**El diseño:**
- **Boot 1, de la familia**: lo que no existe hasta que arranca el silicio —el reloj, el RTOS, el log y sus primeras
  líneas, el cable donde ya esté (el USB de la Pico) y el ENV a la vista—.
- **Boot 2, común** (`src/bpvm_boot2.c`), en **dos tiempos**, porque la memoria se reparte ANTES de que nadie más la
  pida —la dirección del bloque de los motores va sellada en ellos— y en la P4 la red pide memoria entre medias: allí
  el primero va en `app_main` y el segundo en la tarea del cable.
  - **2a, repartir** (`bpvm_boot2_repartir`): las claves del reparto (`stack=`, `quantum=`), la memoria (gancho:
    DÓNDE; la regla ya es común) y la BIOS (gancho: la tabla y el ancla).
  - **2b, subir** (`bpvm_boot2_subir`; no vuelve): subir (gancho: las capas de la familia) y la línea «boot: estado»,
    una para todas; la zona (común, con el gancho de `mapear`); con el FS, `/lib` (gancho) y lo de la familia que pide
    el FS (gancho: `board.json`, la fachada de ficheros); el hardware (gancho); los buses del ENV (común); la SD (el
    gancho la registra, el común la monta con el FS); la identidad (gancho); **la cintura del REPL** (gancho, antes del
    cable); el cable (gancho); y al final, común: el porqué si no hay autorun, volcar el log (una vez), `log=`, el
    autorun **sólo con la app sana** y el REPL (gancho).
- La capa que pide cada paso, escrita UNA vez: en el código (un `if` por capa, en un sitio) y en
  `H9_KERNEL_CAPAS.md` (la tabla).
- Un gancho NULL es que la familia no lo tiene o lo hizo en el Boot 1 (la Pico: el hardware y el cable, en `main()`).
- **El micro simulado se queda con su arranque**: es un servidor, sin escalera ni autorun, y virgen trabaja desde el
  minuto cero —el IDE y los smokes de `check-sim` cuentan con eso—. Del común toma las claves del reparto
  (`bpvm_boot2_claves`), y gana `quantum=`, que no aplicaba.
- **Lo que se mueve, y por qué no rompe nada**: en la Pico, los buses y el registro de la SD pasan detrás de subir (la
  SD sólo copia sus pines; el bus se usa al montar, que ya iba después); en la ESP32 y la STM32, `stack=` y `quantum=`
  pasan delante de la memoria (se leen en el primer RUN); en la P4, la SD detrás del hardware y los buses (el SDIO no
  pasa por el árbitro); la línea de la BIOS de la ESP32, delante de subir; y el volcado del log, el del arranque, ANTES
  del autorun (en la Pico iba al entrar al REPL, o sea después).
- **La prueba**: `test_boot2` en el host —una familia que apunta lo que se le llama: sana, sin particiones, sin FS,
  sin VM, ganchos NULL— y a la red; las cuatro placas; y en la Pico, el control del fallo nuevo: un `auto.txt` con un
  programa largo y un `HELLO` mientras corre —con la imagen de antes, «no ha registrado su cintura»; con la nueva, el
  `HELLO_REPLY` con el nombre de la placa—.

## H9 — `#623`: packs LIBRERÍA y packs PROGRAMA, y las copias de la stdlib (2-oct, idea de Eduardo; la está pensando)

**Lo que pasó** (`FICHAS` `#623`): `GuiMini.pack` llevaba copias viejas de Gui, Json, Collections, Str y Core; entre
packs gana el último grabado, así que tapaban las del `Stdlib.pack` de hoy, y la tanda subía las frescas a `/lib`, que
van a RAM: en la C6 no cabía `GuiWinJson`.

**La idea de Eduardo** (2-oct): *«hay que distinguir de los packs librerías de los packs programas. Los packs programas
la idea es que funcionen como un contenedor y pase lo que pase fuera del pack, la ejecución del programa esté
asegurada. Los packs librería no deberían necesitar librerías estándar. Lo que ocurre es que tenemos algunas
librerías estándar que no se incluyen en las imágenes de las placas ni en el pack estándar. No sé, quizás necesitamos
un segundo pack que incluya estas librerías y así no gastemos tanta RAM.»*

**Los hechos** (medidos el 2-oct, para pensarla):

- **La imagen** lleva 18 módulos (ESP32 y Pico; la STM32, 17: sin Neopixel): Core Math IO Heap Gpio I2c Spi Uart Pulse
  Pwm Pico Machine Rtc Adc Wdt Timer Time Neopixel —83857 B—. El SO los repone en `/lib` y, cargados desde ahí, van a
  RAM; si un pack sirve los mismos bytes, el instalador quita la copia de `/lib` y corren XIP desde el pack (`#468`).
- **El `Stdlib.pack` lleva los 30**: los 18 y los 11 que NO van en la imagen —App Collections Compress Freq Gui Json Log
  Net Packs Stats Str, más la raíz `Stdlib`: 104488 B, Gui sola 45112—. O sea que **ese «segundo pack» ya existe: es el
  `Stdlib.pack`**, y además saca de RAM los 18 de la imagen. Lo que lo estropeó fue otra cosa: que packs LIBRERÍA
  llevaran sus propias copias.
- **Quién lleva copias de la stdlib** (el censo del 2-oct): `GuiMini` y `Gui` (Collections Core Gui Json Str), `SQLite`
  (Core Str), y los drivers `DriverSpi`, `DriverC5` y `Mini` (Core). No las llevan `DriverNulo` ni `DemoLib`. Todos son
  librerías; el único pack PROGRAMA del repo es `samples/sampleproject/out/App.pack`.
- **El constructor ya distingue los dos**: dice «pack: LIBRERIA (el modulo raiz no tiene `main`)». Y el cargador
  también: si el principal está en un pack, ese pack va PRIMERO (`#493`: pack del principal → `/app` → `/lib` →
  packs), así que las copias de un pack PROGRAMA ya ganan dentro de su programa, que es lo de «contenedor».

**Y no es sólo la tanda** (2-oct, a pregunta de Eduardo: *«¿pero Gui.mod no está en el pack gráfico?»*): sí está —en
`GuiMini` y en el `Stdlib.pack`—, y no se cargó dos veces: se cargó desde `/lib` EN VEZ de desde el pack. El IDE hace lo
mismo que la tanda al ejecutar en una placa (`BpIde` `PicoExplorer.uploadAndRun`, el caso `pack:`): si la copia que la
placa resolvería —la del pack— no es la del PC, *«un pack lo tiene distinto: se sube delante del pack»*, a `/lib`,
porque un pack no se puede retocar y `/lib` va antes en la resolución. Así el programa corre con la stdlib contra la
que se compiló; pero `/lib` se carga en RAM y el pack corre XIP. En la C6, un usuario con un `GuiMini` rancio se habría
quedado sin memoria igual en cuanto cambiara la stdlib. Con las copias iguales, nada se sube y Gui corre XIP (la
tanda del 2-oct, después: «pack:GuiMini/Gui.mod, crc igual: se ejecuta XIP, no se sube»).

**Lo que cambiaría la idea**: un pack librería sin copias de la stdlib —sus módulos sacan Gui, Json… del `Stdlib.pack`
(XIP) o de `/lib`—, y una sola fuente de la stdlib en la zona, la que la red ya comprueba byte a byte
(`compat/stdlib_pack.sh`): se acaban las copias rancias de raíz, y con ellas lo que pedía la guarda de `#623`.

**Lo que habría que mirar antes de decidir**:

1. Un pack librería pasa a DEPENDER de la stdlib de la placa. Con MOD9 manda la versión mínima: si la de la placa es
   más antigua que la que pide, el cargador lo rechaza con nombre («actualiza /lib o recompila»); si es igual o más
   nueva, enlaza símbolo a símbolo (`#523`), y lo que falte sale con su nombre —ruidoso, no mudo—. *(Corregido el
   2-oct: decía que una interfaz distinta también la rechaza; eso es la regla 2 de `#500`, que sólo vale para MOD8 —
   `comprobar_norma`, en las dos VMs—.)*
2. Sin `Stdlib.pack` en la placa, Gui y compañía vendrían de `/lib`, a RAM: en la C6 no cabe la GUI. ¿El manifiesto de
   un pack librería dice «necesito el `Stdlib.pack`» y el que graba (`wire_serie.py`, el panel del IDE) avisa si falta?
3. ¿Un pack PROGRAMA lleva las 30 o sólo el cierre de su programa? Hoy, el cierre.
4. Lo ya grabado: los packs de la GUI de la P4, la DK2 y la C5 llevan copias viejas; con la regla nueva, al regrabarlos.

## H10 — el `Rtc`: saber si el equipo está en hora (2-oct, idea de Eduardo; decidido y hecho: `#626`)

**El problema** (`V7_BACKLOG` H10): en la Pico y las ESP32 el reloj de pared es por software y empieza en 0 a cada
arranque; lo calibra el IDE al conectar (`TIME`). Con autorun nadie lo calibra, `Rtc.Clock.epochSec()` devuelve los
segundos desde el arranque como si fueran la hora, y un programa no tiene forma de saberlo.

**La idea de Eduardo** (2-oct): *«se puede utilizar una variable de entorno que guarde la fecha en la que se grabó la
imagen y si la fecha actual es menor es que está fuera de hora»*.

**De paso salió `#625`** (`8b67a0b1`): en el PC la VM-C contaba desde el arranque del PC y la VM-Java daba la hora; ahora
las dos dan la hora del PC, que es lo que la regla necesita para decir «en hora» igual en las dos.

**Lo que propuse** (`9f37130a`): la clave `imagen.fecha`, escrita por el arranque común con la fecha de compilación menos
un día «si falta o es anterior», y una primitiva específica (`__rtcIsSet`) o una general (`getEnv`).

**Lo que decidió Eduardo** (2-oct):
- **La primitiva, general**: *«General: getEnv(clave)»* —un programa lee cualquier clave del ENV, e `isSet()` se escribe en
  BP—.
- **La fecha, UNA vez**: *«La fecha de la imagen. Se escribe una vez nada más. Cuando arranca el equipo, si está conectado
  lee la hora del PC. Si no tiene la variable del environment con la fecha entonces la añade. Solamente se hace esa vez
  hasta que se borre. Si lo grabamos cada vez que arranque podríamos llegar a estropear la flash.»* Corrige mi «si falta
  o es anterior»: una imagen nueva NO la mueve; para ponerla al día, se borra la clave y la anota el siguiente arranque.

**Hecho** (`#626`):
- **`Machine.getEnv(key): string`** (la primitiva 271, `__machineGetEnv` en Core): el valor de la clave, o `""`. En
  `Machine` y no suelta en Core: el ENV es la configuración de la placa, junto a `getMicro`/`getBoard`, y no gasta un
  nombre global. La regla, la del ENV: claves sin distinguir mayúsculas, valor entero o nada. Es el ENV **que leyó el
  arranque**, como el hardware que configura: un cambio desde el IDE se ve al reiniciar. En el PC no hay ENV: `""` en las
  dos VMs.
- **La VM-C** lo lee por un lector que registra el **arranque común** (`bpvm_machine_set_env`, en Boot 2): dónde está el
  ENV de cada familia ya lo sabe su gancho `env`, así que no hay código de la primitiva por familia.
- **La fecha**: el arranque común (Boot 2), después de la línea `boot: estado`, mira la clave en el ENV; si falta, la
  anota con la fecha de la imagen (`__DATE__` a las 00:00, en segundos desde 1970) por un gancho nuevo de la familia,
  `env_anadir` —la misma cintura de flash que `ENV_SET`, y la vista del ENV al día para que el programa la vea sin
  reiniciar—. Si está, la flash ni se mira. Con un ENV virgen, se crea con ella. Una función común,
  `bpvm_env_anadir`, construye el bloque nuevo sin tercer búfer: en un sector de la zona de rascar (la Pico, que lee las
  copias por XIP) o en la copia rancia misma (las ESP32 y la STM32, que las tienen en RAM).
- **`Rtc.Clock.isSet(): boolean`**, en BP: sin la clave, en hora (el PC: su reloj es el del PC); con ella, en hora si
  `epochSec() >= imagen.fecha - 86400` —el día de margen, porque la fecha de compilación no lleva zona horaria; la clave
  guarda la fecha tal cual—. Una clave que no son cifras (escrita a mano) LANZA con el valor en el mensaje.
- **La fecha es la de compilación de `bpvm_boot2.c`**: en un build incremental, la de la última vez que se compiló —
  anterior o igual a la de la imagen—. Como cota vale igual: ninguna hora correcta es anterior.

**Visto de paso, y arreglado**: la vista del ENV de la **Pico** apunta a la flash (XIP) con la longitud del arranque, y el
SEGUNDO `ENV_SET` de una sesión escribe justo esa copia (el A/B alterna): leía el payload nuevo cortado, o con el
relleno detrás —un pin a medias para un pack, un `gc=` perdido—. Ahora, si la copia elegida ya no es la que era, se
elige otra vez. Las ESP32 y la STM32 no lo tenían: copian el ENV a RAM al arrancar.

**Las pruebas**: `test_env` (8: añadir sin tercer búfer, virgen, ya está, no cabe, formato), `test_boot2` (7: la fecha de
`__DATE__`, una vez, no se pide si está, se dice si no se puede; el lector de `getEnv`), `RtcEnHora` (paridad y tanda:
la misma salida en el PC y en placa) y `env_smoke` (el micro simulado, tres arranques sobre la misma flash: sin la clave,
con ella, y con una que no es una fecha).

## H10 — la PIO del RP2350: programarla desde BP (2-oct, el planteamiento de Eduardo, decidido; para más adelante: V7 si da tiempo, si no V8)

**El porqué** (Eduardo, 2-oct): *«Para la Pico igual necesito programar la PIO»*. Lo que hay: el `Neopixel` de la Pico
va por PIO (H7.4, `pico/neopixel.c`), con un programa FIJO —el `ws2812.pio` de la SDK, ensamblado por `pioasm` al
compilar el firmware— en `pio0`, y no suelta nada de lo que coge. Un programa BP no puede escribir el suyo.

**Lo que decidió Eduardo** (2-oct), mirando cómo lo hace MicroPython (`rp2.asm_pio` y `rp2.StateMachine`):
1. **Sin ensamblador**: *«en vez de escribir un ensamblador […] con un writer y con llamadas a métodos vayan añadiendo
   las líneas. Nos ahorramos el parseador»*. Cada método añade una instrucción, ya codificada en sus 16 bits.
2. **Sin clase de programa**: *«piosm.addInst() (addInst puede ser AddJump, AddLabel, etc). Sin necesidad de crear una
   clase para el programa, lo guarda dentro de una lista la StateMachine»*.
3. **Un módulo `RP2350`, con la clase dentro**: *«y así solamente lo cargan los que vayan a utilizar el micro»* (no una
   clase de `Machine`, que cargan todos los que importan `Gpio` o `Rtc`: el prototipo ya pesaba 1880 B de código, más
   que los 785 de `Machine` entero). Su `.mod` va sólo en la imagen de la Pico: en otra familia, el programa que lo
   importe se para al cargar («falta el módulo»); y aunque llegara por un pack, la primitiva mira la tabla de
   capacidades y lo dice con nombre.

**La forma** (los nombres, de la convención: un verbo y en minúsculas):
```
import RP2350

var sm: RP2350.StateMachine := RP2350.StateMachine(0)    // la máquina 0 (de pio0)
sm.setSideset(1)
sm.addWrapTarget()
sm.addLabel("bitloop")
sm.addOut(RP2350.X, 1).side(0).delay(3)
sm.addJump(RP2350.NOT_X, "do_zero").side(1).delay(2)
sm.addJump("bitloop").side(1).delay(2)
sm.addLabel("do_zero")
sm.addNop().side(0).delay(2)
sm.addWrap()
sm.setFreq(8000000)
sm.setSidesetBase(16)
sm.active(true)          // aquí se resuelven las etiquetas, se carga en la PIO y arranca
sm.put(color, 8)
```
Las instrucciones: `addJump`, `addWait`, `addIn`, `addOut`, `addPush`, `addPull`, `addMov`, `addIrq`, `addSet`, `addNop`, y
`addWord` (una instrucción ya codificada); las marcas, `addLabel`, `addWrapTarget` y `addWrap`; `.side(v)` y
`.delay(n)` modifican la última. La máquina, calcada de MicroPython: `active`, `restart`, `exec`, `put`, `get`, y
cuántas palabras hay en cada FIFO. Reutilizar un programa es una función que se lo añade a una máquina (`ws2812(sm)`).

**Lo que se probó** (2-oct, un prototipo desechable, fuera del repo): el `ws2812` de la SDK escrito así da los códigos
de `pioasm` —`0x6321 0x1223 0x1200 0xa242`, `wrap_target` 0, `wrap` 3— en las dos VMs, y el blink de MicroPython
también. De paso: con una CLASE de programa (la subclase de MicroPython), BP pedía `this.` delante de cada instrucción
y `set`, `in` y `word` chocaban con palabras reservadas; la forma de Eduardo no tiene ninguno de los dos problemas.
`put` y `get` valen como nombres de método (`get` sólo es reservada dentro de una propiedad). `null` sí choca —las
palabras de BP no distinguen mayúsculas—: la fuente de ceros y el destino que descarta necesita otro nombre.

**Por debajo, en C**:
- Un solo reparto de la PIO, que comparte el `Neopixel`: la memoria de instrucciones de cada PIO es COMPARTIDA —si
  varias máquinas llevan el mismo programa, se carga una vez— y cada máquina es EXCLUSIVA de quien la coge.
- Con la máquina en marcha, su programa queda fijo (un `addX` entonces es un error con nombre).
- Al acabar el RUN —también con KILL— se suelta lo que cogió el programa: la máquina, su programa y los pines.
- `get()` y `put()`, mientras esperan, ceden a los otros hilos de BP, y un KILL los corta.
- El RP2350: 3 PIO × 4 máquinas = 12, numeradas 0–11 como en MicroPython; 32 instrucciones por PIO; en el RP2350B
  (la Metro, 48 pines) cada PIO ve 32 pines, desde la base 0 o la 16 (el `gpio_base` de MicroPython).
- Las primitivas, nuevas en Core; el backend, sólo en la Pico; el número de PIO, en la tabla de capacidades (el
  `pioCount` del `INFO` ya existe).

**El PC** (Eduardo, 2-oct): *«En el PC solamente se escribe el programa, no funciona igual que el I2C o el SPI»*. Sin
emulador: el programa se construye y se comprueba igual que en la placa (es BP: las etiquetas, los rangos, las 32
instrucciones, con prueba de paridad), y la máquina hace lo que el I2C y el SPI hacen en el PC: comprueba los
argumentos y dice por la salida lo que haría (`[pio] …`, las mismas líneas en las dos VMs), y `get` da 0. Lo que la PIO
hace de verdad se ve en la placa.

**Para qué, y cómo se prueba** (Eduardo, 2-oct): *«La prueba se puede hacer fácil en la Metro con el código
equivalente al del Neopixel. En realidad lo necesito para recibir una comunicación serie TTL a 9600 N 8 1»*.
- **La prueba**: en la Metro, un `ws2812` en BP que encienda su NeoPixel como el driver de C —se ve, y se compara con
  el de C—.
- **El uso**: un RECEPTOR serie, el `uart_rx` de los ejemplos de la SDK: `wait 0 pin 0` (el bit de arranque),
  `set x, 7 [10]` (hasta la mitad del primer bit), `in pins, 1` y `jmp x-- bitloop [6]` (ocho bits, ocho ciclos cada
  uno), `jmp pin` (el bit de parada), y `push` —o `irq` si la trama viene mal—. La máquina, a 8 × 9600 = 76800 Hz (el
  divisor, 1953,125 a 150 MHz); el byte entra por la derecha y queda en los bits 31..24 (`get() >> 24`). Pide
  `addWait`, `addIn`, `addJump` con `X_DEC` y `PIN`, `addPush` y `addIrq`, y `setInBase` y `setJmpPin`.
- **Lo que implica recibir**: la FIFO de la máquina tiene 4 palabras (8 si se une la de recepción); a 9600 baudios llega
  un byte cada ~1 ms, así que una pausa del GC de más de 8 ms perdería bytes. Lo mismo que resolvió el `Uart` de la
  Pico (`#473`: un anillo de 512 B que llena una interrupción): la capa de C vacía la FIFO en un anillo, por la
  interrupción de la PIO, y `get()` lee del anillo. Y se dice cuándo el anillo se llenó (errores sí, silenciosos no).
- **Su prueba en placa**: un puente entre el TX de un UART de hardware (`Uart`) y el pin RX de la máquina; se mandan
  bytes a 9600 8N1 y se comparan los recibidos (también con el GC trabajando entre medias).
- **Antes de la PIO, el `Uart`**: la Pico tiene dos UART de hardware, libres (el cable del IDE va por USB), que ya
  reciben por interrupción en su anillo. Si el RX puede ir a un pin de UART (en la Pico 2: GP1, GP13, GP17 y GP29 del
  UART0; GP5, GP9, GP21 y GP25 del UART1), funciona hoy. La PIO hace falta para un pin cualquiera o para un tercer
  puerto. **Y es el caso** (Eduardo, 2-oct): *«necesito más de 2 puertos serie»*.
- **Un receptor hecho, con la forma de `Uart.Port`** (propuesta): en el módulo `RP2350`, además de `StateMachine`, una
  clase escrita en BP sobre una máquina —el programa `uart_rx`, a 8 × baudios, con su anillo— con la interfaz del
  `Uart` (`read(data, count, timeoutMs)`, `available()`), para que el tercer puerto y los siguientes se usen igual que
  los dos de hardware. Cada uno gasta UNA máquina; el programa (5 instrucciones) se carga una vez por PIO y lo
  comparten: caben de sobra en las 12 (menos la del `Neopixel`, si se usa). Transmitir por PIO (el `uart_tx`) sería
  la misma forma.

**Recibir y enviar, y los buses por software** (Eduardo, 2-oct): *«En este caso solamente necesito recibir, lo normal
sería poder recibir y enviar. En MicroPython tienen un I2C y un SPI por software lo cual es muy útil para cuando se te
acaban los puertos físicos. Tener una Uart por software estaría muy bien pero el código se complica un poco ya que
sería necesario que funcionase a diferentes baudios»*.
- **El puerto serie por PIO, los dos sentidos**: el receptor (`uart_rx`, 5 instrucciones) y el emisor (`uart_tx`, 4),
  cada uno en su máquina, con la interfaz de `Uart.Port`.
- **Los baudios no lo complican, con la PIO**: el programa es el MISMO a cualquier velocidad; sólo cambia el reloj de
  la máquina (8 ciclos por bit: 8 × baudios). El divisor del RP2350 (16 bits enteros y 8 de fracción) va de ~290
  baudios a varios millones: a 9600, 1953,125 exacto; a 115200, 162,76 (con la fracción, el error queda por debajo
  del 0,01 %).
- **Por qué MicroPython tiene I2C y SPI por software y no UART**: en el I2C y el SPI el RELOJ lo pone el maestro, así
  que unos bits a golpe de CPU con el ritmo irregular (el RTOS, la VM, el GC) siguen valiendo: el otro lado espera a
  los flancos. En una UART no hay reloj: los dos lados llevan el tiempo, y recibir por software es muestrear cada bit
  en su instante —a 9600, ~104 µs por bit—; una interrupción de unas decenas de µs estropea el byte. Con un RTOS y la
  VM, no es fiable. Por eso en el RP2 se hace con la PIO, que es la UART por software bien hecha.
- **El I2C y el SPI por software** (como `SoftI2C`/`SoftSPI` de MicroPython): viables en las cinco familias por lo de
  arriba, y útiles cuando se acaban los buses. Irían en C, en la capa común sobre la fachada de GPIO, explícitos (una
  clase propia, no un bus que se elige solo: nada de cosas inteligentes) y con la transacción de una vez de G4.a3.
  Idea anotada, sin decidir.
- **Cuántas UART da hoy cada familia** (la tabla de capacidades): el RP2350, 2; las ESP32, la UART0 es el cable del
  IDE —a los programas les quedan 2 en la S3 y la P4, y 1 en la C3, la C5 y la C6—, y la P4 tiene 5 en el silicio
  pero el backend sólo da la 1 y la 2; la STM32, 3 o 4. Sin PIO, la salida para más puertos en las ESP32 sería su
  periférico RMT, no la CPU.

**¿Y las ESP32?** (Eduardo, 2-oct: *«sin PIO no es tan sencillo hacer una UART. No sé si las ESP32 tenían algo de
PIO»*). No tienen PIO. Lo que dicen los `soc_caps.h` del IDF v6.0.1, de lo más barato a lo más caro:
1. **Las UART que ya están**: la P4 tiene 5 de alta velocidad y el backend sólo da la 1 y la 2 (`gpio_esp32.c`: `b <= 2`);
   darle las otras es poco trabajo. Y la C5, la C6 y la P4 tienen además una **UART de bajo consumo** (`SOC_UART_LP_NUM`
   1), con sus pines fijos: en la C5 y la C6, que hoy dejan 1 sola a los programas, sería la segunda.
2. **El RMT** (en las cinco: S3, C3, C5, C6 y P4; 48 palabras por canal, y DMA en la S3 y la P4): lo más parecido a la
   PIO, sin ser programable —manda y recibe trenes de pulsos (nivel y duración) con el tiempo puesto por el
   hardware—. Para una UART: enviar es fácil (un byte, como mucho 10 pulsos con su duración exacta); recibir, el RMT
   captura la línea como pulsos con precisión de hardware y la CPU, después, en C, cuenta cuántos bits caben en cada
   pulso a los baudios dados: el tiempo lo garantiza el hardware, y una pausa del RTOS o del GC no estropea nada. Y el
   RMT ya está en el firmware: el `Neopixel` de la familia va por él desde el 26-sep (`6a53f91f`, probado en la S3).
3. **Un segundo procesador pequeño**: el núcleo LP de la C5, la C6 y la P4 (`SOC_LP_CORE_SUPPORTED`) o el coprocesador
   ULP RISC-V de la S3, que harían una UART por software sin las interrupciones del RTOS. Posible, y mucho más trabajo:
   un firmware propio para ese núcleo.
(También tienen PARLIO y ETM la C5, la C6 y la P4; para esto, el RMT es más directo.)

**Lo que decidió Eduardo** (2-oct): *«la opción 1 para V8. El RMT para enviar se puede estudiar, el Neopixel o
equivalente salen casi directo»* —las UART que ya están, a V8; enviar por el RMT, a estudiar—. Los dos, en el bloque de
V8 de `FICHAS`.

**Y lo último** (Eduardo, 2-oct): `null` se llama **`RP2350.NONE`** (*«NONE está bien»*); y **cuándo**: *«Más adelante,
si tenemos tiempo de V7, si no en V8»* —anotado en el bloque de V8 de `FICHAS`—.

**Por fases**: 1) el writer, con su prueba de paridad (el oráculo, `pioasm` sobre los programas de los ejemplos de la
SDK); 2) la máquina, en el PC como el I2C y el SPI, y de verdad en la Pico —el blink y un `ws2812` en BP que dé lo mismo
que el driver de C—, en la tanda; 3) más tarde, las interrupciones de la PIO como eventos de BP y el DMA.

## H10 — `#473`: los diez «rompe-el-modelo», mirados contra el código (2-oct); y el motivo de un fallo del FS (`#628`)

**El triaje** (un agente leyó el código de hoy, sin tocarlo; lo concreto, comprobado a mano después). De los diez que el
triaje del 10-sep mandó a V7 con ficha nueva:

| # | qué | hoy | lo que queda |
|---|---|---|---|
| 1 | los tres backends de FreeRTOS | vivo | la fusión, bloqueada por el +10,8 % de `PrintBench` en la Pico sin explicar (`#615`, aparcada) |
| 2 | `lv_conf.h` con dos condiciones iguales (:33 y :1020) | vivo | desde `G5` ninguna imagen compila LVGL: decide `BPVM_EN_PACK`; una macro, y el pack byte-idéntico. Minutos |
| 3 | `setRotation` en la DK2 | **resuelto** (`G5.b`, `568e976a`) | restos sin efecto bajo `BPVM_LVGL`, que la DK2 ya no compila |
| 4 | la fachada del FS dependía de littlefs | vivo | → `#628`, abajo |
| 5 | la clasificación del fallo, sólo en littlefs | vivo | → `#628`. Y la otra lectura: el `exitCode` de un fallo de CARGA diverge (la Pico, el ordinal del enum; la ESP32, `bpvm_exit_code`; la STM32 y el sim, −2) |
| 6 | los tres `board_mgr_*` | vivo | el adaptador que envuelve al despachador común (47 de ~65 líneas iguales): un `bpvm_bmgr_handle()` común con una cintura de cuatro piezas |
| 7 | el lazo del wire de la STM32 | vivo | no cede nunca (`stm32_repl.c:1087`): con la placa ociosa, la tarea ociosa de FreeRTOS no corre. Y `scheduler_smp.c:334` sale de un fallo sin unir los hilos ya creados |
| 8 | `RUN` cuatro veces | vivo | el más caro: un `bpvm_repl_run` común con 5-6 ganchos, y la tanda en las placas |
| 9 | el sink de `OUTPUT` cinco veces | vivo | **la STM32 tira EN SILENCIO la línea cuyo escape no cabe en 2048 B** (`stm32_repl.c:300`; una línea del hilo `io` llega a 512 B, y 512 de control escapan a 3072). Un `send_output` común que trocee |
| 10 | la puerta de arranque | a medias (`#601`) | la lista ya es una; la puerta sigue triplicada (el `if`, tragar el bulk, el `NOT_READY` del RUN), con tres mensajes |

Y los lotes D (FS, 15) y E (GUI, 10) de `473_TRIAJE_COLA.md` §3 siguen sin mirar.

**Hecho el 3-oct** (`89da2e63`): el **2** —`lv_conf.h` decide con UNA macro (`BPVM_EN_PACK`); comparadas todas las
macros del fichero antes y después, con las banderas del pack (670) y con las del PC (676): idénticas—; el **7** —el lazo
de la STM32 cede un tick cuando no hay nada que leer ni que mandar (sólo donde la RX tiene anillo: la FIFO de 8 B no
aguantaría el milisegundo), y `bpvm_scheduler_run_smp` para y une a los workers ya nacidos si uno no nace—; y el
**10** —la puerta del arranque, `bpvm_repl_puerta_arranque` en el común, con el mensaje de la Pico (dice el estado);
sin prueba en el PC: el micro simulado no la usa (sirve ficheros sin particiones)—. Siguen vivos el 1 (FreeRTOS, fuera
de V7), el 6 (el `board_mgr`, con la fachada de la flash), el 8 y el 9 (el `RUN` y su sink, en un solo sitio).

**`#628` — el motivo de un fallo del FS.** `bpvm_fs_last_fail()` es de la fachada (`bpvm_fs.h`), pero vivía en
`fs_lfs.c` como un `int` de littlefs. Tres consecuencias, las tres medidas: la fachada dependía de littlefs por un
símbolo —`test-crc`, que la enlaza con el motor del PC, no enlaza desde el 31-ago, y el 15-sep otras tres se
«arreglaron» enlazando la librería entera—; FAT y el motor del PC no anotan nada, así que un `PUT` que falla en `/sd`
contesta con el motivo RANCIO de la última operación de littlefs; y dentro de littlefs sólo lo anotaban `write` y el
cursor: un `DEL` que falla (`lfs_remove`) también contesta con el de antes.

El mecanismo, como `errno`:
- **el motivo vive en la fachada**: la clase (lo que va al wire), el motor que falló, su código crudo y un texto (lo que
  va al log). La línea del log no cambia para littlefs: `FALLO lfs=-28 NOSPC (sin espacio)`; FAT dice `fat=4 NO_FILE…`
  y el PC `host=2 ENOENT…`;
- **lo anota el que falla**: cada camino de fallo de los tres motores llama a `bpvm_fs_anota_fallo(motor, crudo, clase,
  texto)`; y la fachada, los suyos (mover entre unidades, el bloque del fichero que no cabe, «ninguna unidad atiende
  esa ruta»);
- **cada fallo lleva SU motivo**: la fachada cuenta los que se anotan; si una operación falla y la cuenta no se ha movido
  —el motor no dijo nada—, anota ella «el motor no dice por qué». Así un fallo nunca hereda el de otro, y un camino que
  mañana se olvide de anotar sale como «sin detalle», no con un motivo falso. No se borra nada al empezar una
  operación: un éxito no escribe, y lo que tiene sentido es leerlo justo después de un −1 (lo que ya hace el REPL).

Ejemplo, el caso que lo destapó: un `PUT` encima de la carpeta `/lib` (littlefs: `ISDIR`, «argumento inválido») y
luego un `PUT` a `/sd/no/esta/x.txt`. Antes, el segundo contestaba «argumento inválido» —el motivo del primero—; ahora,
`NOT_FOUND` —el suyo, `FR_NO_PATH` de FAT—. Lo prueban `sd_smoke` (por el cable, en el micro simulado) y una prueba de C
nueva, `test-fsfallo` (cada clase en el motor del PC y en littlefs, y el motor que falla sin decir por qué).

**Visto de paso, sin tocar** (lote D, FS): FAT no cumple tres cosas del contrato de la fachada que cumplen littlefs y
el PC: su `mkdir` no es recursivo (un `PUT` a `/sd/a/b/x.txt` con `a` nueva falla en placa y va en el PC: el
`crear_dirs_padre` de `#455` dice «recursivo en los 3 backends»); su `stat` de una carpeta contesta 0 (los otros, −1); y
su `mkdir` encima de un FICHERO contesta éxito.

## H10 — `#473`, los lotes D (FS) y E (GUI): el MAPA VERIFICADO (2-oct, decidido por Eduardo: «Mapa verificado»)

Un agente leyó el código de hoy (sin compilar ni ejecutar) para los 25 hallazgos de `473_TRIAJE_COLA.md` §3 D y E;
lo concreto con consecuencia, comprobado a mano después (180, 236, 466 y el código muerto de 264). **De 25: 15 vivos,
2 a medias, 8 resueltos, ninguno «no se sostiene».** Lo de pantalla se arregló casi todo de rebote al sacar LVGL a un
pack (23-26 sep); en el FS sólo lo que tuvo hito propio (la SD, el cursor, los packs). Y hoy, de paso, `#631`: el
`mkdir` de FAT recursivo, su `stat` de carpetas y su `mkdir` encima de un fichero.

**Con consecuencia para quien programa** (de más a menos daño):

| # | qué | hoy | arreglo |
|---|---|---|---|
| 180 | si el driver de pantalla no arranca (la C5/C6: el bus SPI tomado; el P4: sin panel), sólo lo dice el log —`pant_lvgl.c:152`— y el programa sigue con una pantalla muerta | vivo, **mudo** | tras crear el screen, si el display no está abierto, excepción (~6 líneas, imágenes); con el motivo exacto, tocar la API del pack |
| 236 | `copyFile` en `/sd` lanza «error al copiar»: FAT no tiene `copy` y la fachada no hace el «read+write» que promete su comentario; en la flash y en el PC, copia | vivo | un `fat_copy` como `fat_crc32` (~25 líneas, común) |
| 466 | un `FORMAT` que falla contesta OK en las tres familias (sus `fs_format` devuelven 0 fijo); el sim sí lo propaga | vivo | que la cintura devuelva lo que pasó (~12 líneas, firmware) |
| 268 | `ensure_parent_dirs` ×4 (tres de familia, para `/lib`, y la común del `PUT`) | vivo (la parte de FAT, cerrada con `#631`) | `bpvm_fs_mkdir_padres` en la fachada y fuera las cuatro (−40) |
| 248 | `DF` mezcla volúmenes (el total de la raíz, la cuenta con `/sd`); en el sim con `--sd`, también `INFO` | vivo, sin consumidor real | ~8 líneas |
| 244 | `hay_medio()` dice 1 sin detector: la vigilancia en caliente fuera de la Pico sería inservible | latente | un bit «sé detectar» en la cintura de bloque (~15 líneas) |

**Decidido** (Eduardo, 2-oct): la fachada de la flash (256) entra en V7, con el `board_mgr` común.

**Duplicación, orden o documentación** (sin consecuencia hoy): 256 —no hay fachada de flash: cuatro formas, y el
ENV en estáticos de cada familia; **es el único que sigue fabricando copias** (`#626` trajo tres `env_anadir`); grande,
~250 líneas, escribe flash—; 264 —el shim viejo del FS ×3: borrar lo muerto (`fs_put_append` ×3, `fs_delete`/`fs_del`,
`fs_exists`, `fs_status_str`, `fs_list_omitidas`: cero llamadores, comprobado) son ~95 líneas sin riesgo; entero,
unas −450—; 324/328 —las operaciones de espacio, por familia: una en cada motor y fuera 3 punteros y 11 funciones—;
276 —el STM32 compila la pila de SD sin usarla (0 bytes: el enlazador la descarta)—; **280 —`CENSO_FAMILIAS.md`
miente en más filas que el 10-sep: una trampa para quien planifique con él—**; 360 (`ldo` del P4 en `include\`,
aceptarlo); E: 176, 188 (el GT911 con dos drivers), 200 (3 de 8 verbos sólo del host), 340 (`lv_conf.h`, a medias:
la consecuencia ya no puede darse). Y los restos de los resueltos: ~400 líneas muertas bajo `BPVM_LVGL` (el LTDC y el
DSI) y notas rancias.

**Resueltos**: 240 (el streaming de FAT, `b0e15592`), 260 (la SD al común, H3), 450 (el escaneo de `.mdn`, 4 de 4),
184, 192, 196, 204 y 368 (todo por LVGL en un pack, `G1`–`G5`).

**Hecho el 3-oct** (`89da2e63`; la 180, en `c382a66c`), cada uno con su prueba en el PC:
- **236** `fat_copy` (la forma de `fat_crc32`); `sd_smoke` corre un programa que copia en la tarjeta del micro
  simulado (5000 B byte a byte, encima de uno más largo, uno que no existe) —contra el sim de antes, el «error al
  copiar» exacto—;
- **248** el `DF` y el `INFO` cuentan sólo el volumen propio (`bpvm_fs_count_files` y el recorrido del sim saltan lo
  que atiende otra unidad montada, por la tabla de montajes); `sd_smoke` —el control: 1→2 ficheros y 36→5036 B—;
- **268** `bpvm_fs_mkdir_padres` en la fachada, y fuera las cuatro copias; seis casos en `test-fsfallo`;
- **264** lo muerto de los tres shims (contado por llamadores: `fs_exists`, `fs_put_append` ×3, `fs_delete`/`fs_del`,
  `fs_free_bytes` de la Pico, `fs_status_str`, `fs_list_omitidas`, y los comentarios de lo que ya no existía), y la
  excepción de `-Wformat-truncation` de los cinco CMake de las ESP32 (el `snprintf` se fue con `#461`). El resto del
  shim (las cuentas, `fs_put` del instalador, el formateo) espera a la fachada de la flash (256);
- **466** el `FORMAT` de las tres familias devuelve lo que pasó (formatear y las tres carpetas), y el común deja el
  motivo en el log;
- **180** la pantalla cuyo driver falla al arrancar: `Gui.screen()` lanza, sólo si HAY driver (`disp_native_size`);
  sin prueba en el PC (no hay driver que falle: compilado con LVGL dentro y con el pack);
- **276** la STM32 deja de compilar la pila de SD (`bpvm_sd.c`, `bpvm_sd_blk.c`, `bpvm_sd_spi_env.c`,
  `bpvm_blk_sdmmc_cfg.c`, `bpvm_blk.c`: sus `.cproject`); `bpvm_sd_placa.c` se queda (dice «no tiene SD»);
- **280** `CENSO_FAMILIAS.md`: lo de agosto, rotulado como historia, y el reparto de hoy GENERADO por
  `tools/censo_fuentes.py`, que lo lee de lo que compiló el último build de cada imagen;
- y los restos de los resueltos: fuera las mitades `BPVM_LVGL` de `gui_display_dsi.c` (P4) y `gui_display_ltdc.c`
  (DK2), −442 líneas que ninguna imagen compila desde `G5`.
Quedan, sin consecuencia hoy: 244 (`hay_medio()`), 256 (la fachada de la flash, con el `board_mgr`), 324/328 (las
operaciones de espacio, con ella), 360 y los de E (176, 188, 200, 340).

## H10 — la GUI: `#477` resto, `G2` resto e `invokeLater`/`Command` — el MAPA VERIFICADO (2-oct, decidido por Eduardo)

Un agente leyó el código de hoy; lo concreto, comprobado: **el orden de los hijos del volcado rompía la paridad**
(medido: «crear A y B, borrar A, crear C» volcaba C, B en la VM-C y B, C en la VM-Java —la VM-C reutiliza ranuras
desde `G2` y las recorría por ranura—; cerrado hoy, `#632`), **Bar, Led, Chart y Table no llaman a `attach()`** (leído:
`Gui.bp:880`, `935`, `1169`, `1209`; Label y Button sí) y **reasignar un `owner` local no llamaba al destructor**
(cerrado hoy, `#633`, con un sample que se comprueba solo y su sabotaje).

**1. `#477` resto — los huecos del oráculo `__guiDumpTree`.** El Chart sigue sin volcar sus datos (el modelo los
guarda en las dos VMs; el volcado no tiene rama). Lo de la rotación de la DK2 lo cerró `G5.b` (`568e976a`); quedan
arreglos baratos —`rot=N` en la línea de `screen` sólo si no es 0 (no cambia ningún golden), la cabecera de `Gui.shot`
con la rotación del modelo (en placa sale 0 desde `G5`), y borrar la rama muerta de `BPVM_LVGL`— y uno caro: que el
modelo sólo gire si el panel giró (cambia la firma del contrato con el pack). El táctil de los dos drivers SPI (C5,
C6) sigue sin girar, desde H6.a. Color y fuente cargada sólo viven en el render: es el diseño de V3. Y fuera de la
lista: `min`/`max` no salen en el volcado; los botones de un Msgbox y el textarea de un Keyboard no están en el modelo.
**Coste**: poco código, pero cada cambio del volcado obliga a rehacer el pack de LVGL en tres ABI y a regenerar a
sabiendas los goldens de la cápsula V6 que lo congelan (`GuiParidad2.out`, `GuiShot.out`). **Decide Eduardo**: ¿el
formato del volcado es un contrato congelado o se amplía?; ¿color y fuente al modelo?; ¿la rotación, cerrada con
`G5.b` y los arreglos baratos?; ¿el táctil de la C5 y la C6?

**Decidido** (Eduardo, 2-oct): *sólo lo barato*. El volcado sigue siendo el contrato que congela la cápsula; color y fuente se quedan en el render (el diseño de V3).

**2. `G2` resto.** El destructor existe desde H1 (21-sep), pero sólo corre al morir un `var owner` local o al salir de
un `try var`; las VMs no lo conocen (la cascada de `FREE_REF` y el GC liberan sin él) y `Gui.bp` no tiene ninguno. Hoy
el nodo de un widget que el programa abandona sigue en la tabla y en pantalla hasta un `delete()` de un antepasado o
el fin del RUN; y si su wrapper muere por `FREE_REF` (un `owner`), el nodo se queda con un puntero colgando —un
evento posterior puede llegar a OTRO objeto si el hueco se recicla—: hoy nadie lo dispara (ningún programa usa
`owner` con la GUI). `toJson()`: no emite `min`/`max`, ni `options`/`items` (que `__guiGetText` ya devuelve), ni los
botones de un Msgbox, ni los colores; y Bar, Led, Chart y Table no están en el árbol: no cuentan, se pierden en la ida
y vuelta del `.win` y sus `"clic"` nunca llegan. ImageView, Tabview y Keyboard no los sabe construir `makeWidget`.
**Coste**: casi todo en `Gui.bp` (decenas de líneas, sin VMs, compilador ni imágenes). **Decidido** (Eduardo, 2-oct): (a) y (b) —el árbol y `toJson()` en `Gui.bp`, y un `~Component` que borre el nodo, con `Screen` exento—; y sobre `owner`: *«Owner solamente puede haber uno activo. El owner debe estar en su contenedor si lo pones en la lista de componentes del contenedor se queda ahí. Si aparece en otro sitio debe ser una referencia normal.»* O sea: «`owner` en las properties de la ventana», no.

**Lo que se le preguntó**: ¿`~Component`
hace `delete()` (con `Screen` exento)?; ¿se abandona «`owner` en las properties de la ventana» —el agente no lo
recomienda: el widget ya tiene dueño, la `OwnerList` de su contenedor—?; ¿la red nativa para el wrapper que muere por
cascada (~30 líneas por VM y el contrato del pack)?; ¿`toJson()` con campos BP (sin cascada) o con getters (`Core` y
todas las imágenes)?; ¿qué widgets entran en el `.win`?

**3. `invokeLater`/`Command`.** No existe nada. Las dos colas que hay no sirven: la de entrada de la GUI va del backend
al hilo que bombea, y la de eventos (H5.c) entrega al hilo que levanta el evento. Hoy tocar widgets desde cualquier
hilo es seguro (con `G1` todos los hilos BP corren en una tarea del sistema y cada primitiva es atómica; el manual lo
promete y tres samples lo usan). **Lo que cambia es H12**: con dos runs en el P4 —la placa de pantalla grande— eso deja
de ser cierto (LVGL va sin SO, y ni `g_nodes` ni el `HashMap` de la VM-Java tienen cerrojo). Dos caminos: un `Command`
y un `Gui.invokeLater` mínimos en `Gui.bp` (40-60 líneas, sin VMs ni imágenes; la cola en `HiloGui`), o un CERROJO de la
GUI en los trampolines (30-50 líneas en la imagen; mantiene lo que promete el manual). **Decide Eduardo**, antes de
encender el segundo run en el P4: ¿regla + `invokeLater` (incompatible con lo de hoy) o cerrojo (compatible)?;
¿alcance, forma y nombres (la API de `Gui` está en inglés)?

**Decidido** (Eduardo, 2-oct): *«El GUI hace tiempo que corre en su propio hilo BP. Si desde otro hilo se pide algo entiendo que puede haber conflictos de hilos, así que invokeLater puede ser una buena opción. Habrá que verificar específicamente en H12»*. Va a H12, donde se verifica con el segundo run del P4.

**La lectura del agente**: lo de más valor para quien programa una placa con pantalla es la parte BP de `G2` (Bar, Led,
Chart y Table en el árbol; la ida y vuelta del `.win`); a medio plazo, la decisión de H12; y `#477` es lo más caro para
lo que aporta.

**Hecho el 3-oct** (`c382a66c`): **`G2` (a)** —Bar, Led, Chart y Table se dan de alta en su contenedor; `toJson()`
escribe el rango, las opciones y los ítems, los botones de un Msgbox, los colores (`"0xRRGGBB"`, campos BP: el render
no los devuelve), la forma de un Chart (`points`, `chartType`, su rango: las series son datos del programa) y la
rejilla de un Table (`rows`, `cols`, `cells`), y el cargador construye Chart y Table— y **(b)** —`~Component` borra el
nodo y se da de baja en el padre cuando el wrapper muere por su dueño (un `var owner` local, un `try var`); la
pantalla no—. **`#477`, lo barato**: `rot=N` en la línea del `screen` (en las dos VMs, sólo si gira: ningún golden
cambia), la cabecera de `Gui.shot` con el giro del modelo, y fuera las mitades `BPVM_LVGL` muertas. Lo prueba
`GuiArbolG2` (en el corpus; se comprueba solo, paridad exacta; con el destructor vacío, dos `FALLO`). Sin hacer, y
fuera de (a): ImageView, Tabview y Keyboard en el `.win` (el título de una pestaña, la imagen, el campo del teclado).

## H10 — `RUN` en un solo sitio (`#473`, punto 8; entra en V7, Eduardo, 2-oct): el análisis

`RUN` está escrito cuatro veces —la Pico (`pico/repl_v1.c`), la ESP32 (`esp32/common/repl_esp32.c`, para las cinco), la
STM32 (`stm32/port/stm32_repl.c`) y el micro simulado (`tools/bpvm_sim.c`)— y desde el 10-sep sólo subieron al común
piezas sueltas. Un agente de diseño leyó las cuatro (sin ejecutar); lo concreto, comprobado a mano después (el mensaje
del `KILL` y el prefijo `load:`).

**Las divergencias** (además de las cuatro que se sabían: el `exitCode` de un fallo de carga, el registro AOT que el sim
vacía después de cargar, el sink que la STM32 tira si no cabe y el sim corta en el primer NUL):
- el fallo de carga con detalle sale **sin `load:`** en la ESP32 y el sim, y `tanda.py` no lo reconoce como fallo del
  arnés (comprobado); y la Pico manda el ORDINAL del enum: «falta un módulo» = 1, que la tabla común lee como
  «excepción no atrapada»;
- **el `KILL` o el `RESET` pueden llevar el texto de una excepción VIEJA**: la Pico y la ESP32 anteponen `rt_err`, que
  se escribe en cada `throw`, también en los atrapados (comprobado); la STM32 y el sim lo hacen bien;
- **un carácter UTF-8 partido**: el hilo `io` corta las líneas a 512 B sin mirar caracteres, y el IDE decodifica cada
  trama por separado: un carácter en el byte 512 sale como dos «�» (en las cuatro); la ESP32 además parte sus tramas;
- rutas de 80 caracteres o más dan `NOT_FOUND` en la STM32 (búferes de 80 y 64; el núcleo admite 255);
- un `KILL` en el hueco entre arrancar `io` y `bpvm_run` puede perderse (`preparar_run` lo borra; por lectura);
- en el sim, el depurador y el poll leen el socket a la vez; y `sim_dbg_smoke.py` ni está en la red ni carga (su `.mod`
  es MOD5);
- líneas que no son del programa dentro de `OUTPUT` (el sim: «[sim] ejecutando el pack…» y los avisos del escáner de
  `.mdn`; la STM32, los del escáner) —el PC los sacó a stderr por la paridad—;
- `elapsedMs` mide cuatro cosas distintas; el `EXITED` de un `bpvm_init` fallido sale de cuatro formas (la Pico, por
  `fputs`, fuera del contrato); un `KILL` en reposo es `NO_SESSION` en las placas y `KILL_REPLY` en el sim; el `RESET`
  de la ESP32 no lo apunta el log; y el quinto sink, el de `test/debug_listen.c`, corta a 1 KB y no escapa controles.

**El diseño**: `bpvm_repl_run(id, path, arg)` en `src/bpvm_repl.c` (ya lo compilan los siete builds: sin altas nuevas)
con una cintura de ganchos `bpvm_repl_run_ops_t` que pone cada familia —obligatorios: de dónde sale la memoria de la
VM, leer una línea sin esperar, el búfer de línea y el de mensajes, reiniciar; opcionales: los motores nativos, el
escaneo de `.mdn`, lo de después de cargar (`esp_aot_register` debe seguir en `repl_esp32.c`, por el enlazador), lo de
antes y después de correr (el MPU de la Pico, la línea `mem:`), cómo correr (el SMP de la Pico), el LED de la STM32, el
depurador común (el de la Pico y la ESP32 es idéntico y sube al común) y el consejo del sim para un módulo que falta—.
Un campo nuevo al final de `bpvm_repl_ops_t`: con `NULL`, la familia sigue con su `RUN`, y así se migra una a una y se
revierte limpio. Una sola regla para el mensaje del `EXITED` (OK: nada; KILLED: el del `RESET` o el del `KILL`, nunca
`rt_err`; LINK_ERROR: el de enlace; el resto: `rt_err` o el estado). El poll, común, «pegajoso» con un `KILL` pedido
(cierra la carrera de `preparar_run`). **El sink de `OUTPUT`, común** (`src/wire_v1_proto.c`): escapa por longitud (un
NUL sale `\u0000`), trocea en tramas de 1 KB sin partir nunca un escape ni un carácter UTF-8 —el que llegue a medias se
guarda para la llamada siguiente, lo que arregla también el corte a 512 de `io`— y no tira nada. Saldo: unas −800
líneas (las familias pierden ≈1700 y el común gana ≈600).

**Los pasos**, cada uno su commit y reversible: (0) sin cambio de conducta: el sim con inicializadores designados,
`sim_dbg_smoke.py` compilando su `.mod`, y un `run_smoke.py` con los casos que la red no cubre (el `EXITED` de un fallo
de carga, un NUL, una línea de controles, un carácter en el byte 512, el `KILL` en reposo) corrido contra el `RUN` viejo
para fijar qué cambia; (1) el sink y su prueba de C, sin llamadores; (2) el `RUN` común, estrenado por el SIM —todo en
el PC: `check-sim`, `check-sombra`, `tanda.py --sim` y el IDE—; (3) la STM32 (la de menos ganchos; arregla la línea
perdida); (4) la Pico; (5) la ESP32; (6) cerrar (el `KILL` en reposo, la escucha del autorun ×3, la cabecera, el
protocolo). Del 3 al 5, a placa: por lotes.

**Las decisiones** (el agente propone; mi lectura entre paréntesis): D1 el fallo de carga sale con exitCode 2 —el de la
tabla común y el del CLI— (sí: es aplicar un contrato que ya existe); D2 los mensajes `falta el modulo 'X'`,
`load: <fallo>` y `load: <estado>` —los prefijos que leen `tanda.py` y `sim_sombra.py`— (sí); D3 con `KILL`, «terminado
por KILL» o nada (el texto: un error que no dice nada es mudo); D4 `elapsedMs` = de `bpvm_run` a parar `io`; D5 los
avisos del escáner y del pack, sólo al log (sí: `OUTPUT` es del programa); D6 `gc=0` y `latido=1`, hoy sólo en la Pico,
en las cuatro por el ENV; D7 los textos de la Pico para «falta path» y `NOT_FOUND`; D8 el `KILL` en reposo, `NO_SESSION`
en todas; D9 el búfer del `EXITED`, uno por familia o un tamaño (detalle mío).

**Decidido** (Eduardo, 2-oct): *«De acuerdo»* con las reglas —el fallo de carga con exitCode 2 y sus mensajes con `load:` y `falta el modulo`; el `KILL` o el `RESET` con su texto y nunca el de una excepción vieja; `OUTPUT` sólo con lo del programa; `gc=0` y `latido=1` en las cuatro familias por el ENV; el `KILL` sin programa, `NO_SESSION` en todas—.

**Hecho el 3-oct** (los pasos 0 a 5; el 6, tras probarlo en placa):
- **paso 0** (`1bfbb08e`, sin cambio de conducta): la cintura del sim con nombres; `sim_dbg_smoke` a la red (llevaba meses
  con un `.mod` MOD5 que nadie cargaba; ahora compila su programa y prueba también un KILL en pausa, `#634`); y
  `run_smoke`, que FIJÓ lo que hacía el RUN del sim en los cinco casos que nada más miraba;
- **paso 1** (`90ae0501`): el sink común, `wire_v1_salida` (en `wire_v1_proto.c`): escapa por longitud, trocea en tramas de
  1 KB sin partir un escape ni un carácter —el que llega a medias se guarda para la llamada siguiente— y no tira nada
  (un byte que no es UTF-8, U+FFFD). `test-salida`, con dos sabotajes;
- **paso 2** (`8448a9d7`): `bpvm_repl_run` y su cintura (`bpvm_repl_run_ops_t`) en el común, estrenados por el sim (−300
  líneas allí). Lo llama la FAMILIA que pone `.run`, no el despachador común: así el enlazador no mete el RUN común en
  las que aún no lo usan. `run_smoke` cambió sus cuatro expectativas en el mismo commit (el diff dice qué cambió: el
  KILL sin programa, `NO_SESSION`; un módulo que falta, exitCode 2; un NUL, `\u0000` y el resto; una ñ en el byte
  512, entera). La red, verde; la tanda de V7 en el sim, como antes;
- **paso 3** (`9c3cb27d`): la STM32 —deja de tirar líneas, las rutas largas ya valen y GANA EL DEPURADOR—;
- **pasos 4 y 5** (`d56dbaab`): la Pico y las cinco ESP32. La ruta del RUN, COPIADA al empezar: en las placas apunta al
  búfer de línea del REPL, que es el que lee `io` durante el RUN.
En las tres familias, su RUN de siempre se quedó DETRÁS DEL PUNTERO hasta probarlo en placa.

**El paso 6, hecho el 3-oct** (`5c2d6bb0`), tras probarlo en placa ese día —la Pico, la C6, el P4 y la Discovery:
`run_placa.py` (los casos de `run_smoke` y `sim_dbg_smoke` por el cable serie, y lo que el sim no da: un KILL y un RESET
con el programa corriendo de verdad, con su hilo `io`; nueve casos, A–I, verdes en las cuatro; la Discovery, con
depurador por primera vez) y la tanda de V7 (todo IDÉNTICO salvo lo que no tiene la placa)—: fuera los RUN viejos de
las tres familias con sus sinks, su EXITED, su poll, su latido y su cintura del depurador (−1 633 líneas; en la STM32,
4,3 KB de búferes estáticos); el RUN, el KILL sin programa y el depurador de antes del RUN, en el despachador común (el
sim deja su copia); la escucha de la ventana del autorun, una (`bpvm_repl_autorun_escucha`, por la cintura del RUN:
una línea rota ya no cuenta como «hay alguien»); la cabecera y el protocolo (`BPVM_WIRE_PROTOCOL.md`: el RUN es uno,
el KILL sin programa, las reglas de la salida y la del EXITED), al día. De paso, en la STM32, las rutas del `.mdn` y
del autorun pasan de 80 y 64 B a `BPVM_FS_PATH_MAX`. Lo que se queda: el sink de `test/debug_listen.c` (una
herramienta de pruebas de C, no del producto: el IDE depura con la VM-Java).

## H10 — el `board_mgr` común y la fachada de la flash (256): el análisis (3-oct; entra en V7, Eduardo, 2-oct)

Un agente leyó el código (sin compilar ni ejecutar); lo de más consecuencia, comprobado a mano después (`#636`, `#637`).

**Hoy.** El protocolo ya es común: `bpvm_bmgr_wire_dispatch` atiende los 15 verbos (STATE, ENV_*, PART_*, PACK_*) y
dice qué sector del ENV hay que volcar; los PACK_* escriben solos por su cintura. Lo que queda en cada familia es el
adaptador de alrededor: los tres `handle` tienen 81, 77 y 79 líneas de código y **50 son idénticas**; difieren en cinco
cosas (la guarda, leer las dos copias del ENV, la geometría, de dónde sale la zona de packs y escribir el sector). Y
fuera del `handle`, por familia: el enrutado con el bulk de `PACK_BURN_DATA` (×4, con un `s_burn_chunk` de 4 KB en la
STM32), la cintura de packs (×4), la vista del ENV (×4), el ENV «temprano» (×3), la capa de particiones del arranque
(×3, casi igual) y las tres `env_anadir` de `#626`. **La flash, cuatro formas**: la Pico (sector de 4 KB, página de 256
B, sólo zona 2 o desde `BP_PART_BASE`, el candado dentro), la STM32 (página de 8 KB por banco, quadword de 16 B que se
programa UNA vez, **cualquier página que exista**, sin candado), las ESP32 (`esp_partition_*`, el IDF guarda) y el
sim (un fichero, sin semántica NOR).

**Con consecuencia** (de más a menos):
1. **`#636` — un `ENV_SET`, `ENV_DEL` o `PART_APPLY` cuya escritura en flash falla contesta OK, en las cuatro**: la
   respuesta se construye antes de escribir y nadie mira lo que devuelve la escritura (comprobado). En placa, el IDE
   refresca y enseña el valor viejo sin decir por qué; en el sim, la RAM ya tiene el nuevo y se pierde al reiniciar.
2. **`#638` — en la Pico la vista del ENV cambia a mitad de sesión**: apunta a la flash y sólo vuelve a elegir copia si
   cambia la del arranque; el primer `ENV_SET` no se ve y el segundo sí (con lo del primero). Las otras tres mantienen
   el ENV del arranque hasta reiniciar —y `env_smoke` lo exige—. Choca con la regla del `RUN` común (`gc=` y
   `latido=` por el ENV en las cuatro).
3. **`#637` — la STM32 escribe la flash sin red de zona**: su primitiva acepta cualquier página que exista (comprobado),
   y la comprobación `.ld`↔cabecera sólo está en el `handle` y en el log, no en su `env_anadir`, que escribe durante el
   ARRANQUE cuando falta `imagen.fecha`. Si el `.ld` y `flash_layout_stm32.h` dejan de casar, ese arranque borra la
   página que caiga (firmware incluido; se recupera por SWD).
4. El sim es más amable que la placa: su STATE sale del plan del ENV y no del arranque real; su flash no es NOR ni
   respeta el quadword único del U5 (la RMW de `PACK_DEL` existe por esa regla y en el PC no se prueba);
   `boardsim_smoke` «reconecta» al mismo proceso y la persistencia nunca se prueba.
5. El IDE dice «el device se reiniciará» tras aplicar las particiones, y nadie reinicia.
6. Menores: el bulk que no cabe recibe tres códigos distintos; la ESP32 rechaza los PACK_* si falta `bpenv`; la flash
   usable de la Pico se calcula en dos sitios.
**Sin verificar** (por lectura): en la ESP32, un error al leer `bpenv` se trata en silencio como placa virgen; en la
STM32 nada impide dos escritores de la flash a la vez (hoy ningún camino lo provoca); y en la STM32 un doble error de
ECC al leer la flash dispara una NMI cuyo manejador es el bucle mudo de CubeMX —si se va la luz a mitad de programar
un quadword, la placa podría colgarse en cada arranque— (propuesta: ficha aparte, y la fachada es el sitio).

**El diseño propuesto**:
- **la fachada, `include/bpvm_flash.h`**: REGIONES con nombre (`ENV`, `LOG` fijas; `FS`, `PACKS` las fija el arranque),
  offsets relativos a la región, cuatro verbos (`leer`, `borrar`, `programar`, `ver` —un puntero legible o NULL—), la
  geometría (borrado de 4 u 8 KB; programa en unidades de 16 B, la del U5, la más estricta); el común comprueba una vez
  para todas que la operación cabe en su región, la alineación, que cada unidad se programa una vez entre borrados y
  salta las unidades todo 0xFF; la comprobación `.ld`↔cabecera, al registrar la tabla (si no casan, ninguna
  escritura, y lo dice). La cintura de cada familia: `pico_flash.c`, `stm32_flash.c` más un mutex, `esp_partition_*`
  por región, el sim imitando la NOR (y con un perfil U5);
- **el ENV, común** (en `src/bpvm_bmgr.c`): leer las dos copias, escribir un sector, `anadir` y la vista del arranque;
  fuera las tres `env_anadir`, las tres escrituras, los dos «temprano», los lectores por familia y el ENV a mano del
  sim;
- **`bpvm_bmgr_handle()` común** (en `src/bpvm_repl.c`, como el RUN): lo del `handle` de hoy más contestar
  `FLASH_ERROR` si el volcado falla y leer él mismo el bulk de `PACK_BURN_DATA`; la cintura que le queda a la familia,
  dos datos que ya se llaman igual en las tres (`board_boot_status()` y la RAM de los motores). Un campo nuevo al final
  de `bpvm_repl_ops_t`; con NULL, la familia sigue con su `handle`.
Saldo: unas −400 líneas (`board_mgr_pico.c` desaparece; a la ESP32 le quedan ~400 de 656 y a la STM32 ~140 de 360).
Pruebas nuevas, cada una con su sabotaje: `test-flash` (fuera de región, programar sin borrar, el quadword dos veces
con el perfil U5, un arranque con `imagen.fecha` que escribe 0 veces); `boardsim_smoke` con la persistencia
comprobada REINICIANDO el sim, `--flash-falla` → `FLASH_ERROR`, y el perfil U5; `env_smoke` con dos `ENV_SET` en el
mismo arranque.

**Los pasos** (cada uno su commit, reversible; después de los pasos 0–2 del `RUN` común, que tocan lo mismo): 0, sin
cambio de conducta (renombrar `bpvm_flash_lock_*` a `pico_flash_lock_*`; los casos nuevos de los smokes, fijados contra
lo de hoy); 1, la fachada en el PC (el sim, y `test-flash` a la red); 2, el ENV y el `handle` comunes, estrenados por el
sim; 3, la STM32 (su tabla de regiones comprobada EN COMPILACIÓN contra sus constantes, y el mutex); 4, la Pico; 5, las
ESP32 (al migrar cada familia, `PACK_` entra en su `lo_lee_el_comun` en el mismo commit); 6, si D5 lo incluye, el log y
el FS por la fachada; 7, cerrar (el contrato, el protocolo, el censo, el IDE). Del 3 al 6, a placa en UN lote, con
copia del ENV antes de grabar. Los riesgos y sus defensas, en el informe del agente (el que una placa no arranque sólo
pasa si se borra firmware, y no es permanente en ninguna familia).

**Las decisiones** (el agente recomienda; pendientes de Eduardo):
- **D1** la vista del ENV, la del ARRANQUE, en RAM y en las cuatro (un `ENV_SET` se ve al reiniciar; la fecha que añade
  el arranque, sin reiniciar). Coste: la Pico +4 KB (hoy no tiene copia, a propósito); la ESP32 −4 KB y la STM32 −8 KB.
  La alternativa —leer siempre de la flash— enseñaría `stack=256` a un programa que corre con el reparto viejo;
- **D2** si el volcado del ENV falla, `ERROR FLASH_ERROR` («sigue el de antes») en vez de OK (el IDE ya convierte un
  ERROR en un diálogo);
- **D3** regiones con nombre y offsets relativos, y la LECTURA dentro del contrato (la ESP32 no puede leer el ENV por
  puntero: mapear `bpenv` gasta espacio virtual, y en la C5 no queda);
- **D4** el candado, en cada familia (en la Pico no hay elección: va dentro de cada operación; la STM32, un mutex nuevo,
  con un camino sin espera para el volcado del log de un fallo);
- **D5** qué entra: los cuatro consumidores (ENV y packs en los pasos 1–5; log, FS y la capa de particiones en el 6,
  donde está el riesgo de datos) o sólo los del gestor (y entonces la fachada sería una quinta forma más);
- **D6** el sim tan estricto como la placa: (a) su flash como NOR, con perfil U5 (quadword único); (b) su STATE, el real
  del arranque (`boardsim_smoke` tendría que reiniciar el sim tras `PART_APPLY`, como ya hace `env_smoke`);
- **D7** tras `PART_APPLY`, ¿quién reinicia?: (a) la placa, después de contestar; (b) el IDE manda `RESET` y reconecta;
  (c) sólo cambiar el texto. El agente recomienda (b): la respuesta nunca se pierde en una carrera con el reinicio.

**Decidido** (Eduardo, 3-oct):
- **D1, el ENV**: *«Las variables de entorno se han de escribir y leer normalmente. Cuando entran en efecto no depende de
  ellas, si una variable se lee al arrancar, entrará en efecto después de un reset. En cuanto a si se leen directamente
  de la flash o de una copia en RAM, pues estamos en las mismas, depende del caso. En realidad deberían de leerse
  siempre de la flash. Si lo que se está preguntando en realidad es qué valor está en vigor esa es otra variable.»* O
  sea, al revés de lo que proponía el agente: el ENV se lee de la FLASH, siempre —el último valor escrito, en las
  cuatro (`#638` se arregla por ahí: la vista de la Pico sigue siempre a la copia más nueva, y las ESP32 y la STM32 dejan
  de leer la foto del arranque)—; CUÁNDO entra en efecto lo decide quien la lee (`gc=` y `latido=`, que se leen en
  cada RUN, en el RUN siguiente; el reparto de memoria y las particiones, que se leen al arrancar, tras un reset); y el
  valor EN VIGOR —lo que aplicó el arranque— es otra cosa, que se guarda y se dice aparte (no es el ENV). `env_smoke`,
  que hoy exige que el programa siga viendo el ENV del arranque, cambia con ello;
- **D2**: `FLASH_ERROR` («no se pudo escribir la flash; sigue el valor de antes»);
- **D3 y D5, la flash: lo mínimo.** Eduardo preguntó por qué volvía si ya se había unificado: lo unificado (V7/H7,
  `U1.4`, 30-sep, «la pequeña») fue DENTRO de cada familia —la Pico, un `pico_flash.c` para los cuatro que escriben, con
  el candado y la zona permitida dentro; la STM32 ya tenía su `stm32_flash.c`; las ESP32, la partición del IDF—; lo que
  no hay es una cara COMÚN, y el `board_mgr` común la necesita. Así que: una cara común DELGADA, sólo con lo que
  necesita el `board_mgr` común (escribir el ENV y la zona de packs), que llama a la cintura que ya tiene cada familia;
  y la guarda de zona dentro de `stm32_flash.c`, como la de la Pico (`#637`). El log y el FS, como están;
- **D4** (mío: en la Pico no hay alternativa): el candado, en cada familia;
- **D6**: para más adelante (*«esto hay que pensarlo»*);
- **D7**: el IDE manda el `RESET` tras el `PART_APPLY` y se reconecta.

**El plan, con las decisiones** (3-oct, noche; leído el código de las cuatro y del sim). Es lo único que le queda a
H10 (Eduardo: *«podemos cerrar H9 y H10 antes de pasar a otras cosas»*).

- **La cintura de una familia**, `bpvm_bmgr_familia_t` (en `bpvm_bmgr_wire.h`): el sector del ENV (4 u 8 KB); dónde
  se LEEN las dos copias —un puntero a la flash mapeada (la Pico y la STM32: `copia(slot)`) o copiándolas (la ESP32,
  que no puede mapear `bpenv`: `leer(slot, dst)`, con un espejo de 2 sectores que pone ella)—; `escribir(slot, src)`
  (borrar y programar; 0 o −1); `rellenar(bm)` (lo de cada placa: base y tamaño de las particiones, el FS arriba, el
  estado real del arranque, la zona de packs y su cintura de grabar, el bloque de los motores); y `no_vale()` (el
  motivo para no tocar nada, o NULL). Es la «cara común delgada» de D3/D5: sólo lo del ENV y de los packs; el log y
  el FS, como están.
- **La vista del ENV, común** (D1, `#638`): `bpvm_bmgr_vista()` elige la copia buena DE LA FLASH; cada escritura que
  pasa por el común sube una generación, y la vista vuelve a elegir (y la ESP32 vuelve a leer su espejo) en cuanto
  cambia. Los únicos que escriben el ENV son el gestor y el arranque, los dos por aquí: la vista es siempre lo que
  tiene la flash, sin leerla en cada consulta. Las vistas de cada familia (`board_env()`, `board_mgr_stm32_env*()`,
  `board_mgr_env()`) y `Machine.getEnv` del sim pasan a ser ésta. La STM32 deja de copiar el ENV a la RAM (−16 KB de
  `.bss`): su flash está mapeada.
- **El `anadir` común** (`#626`): el bloque nuevo, en un sector de la zona de rascar (como la Pico), y por `escribir`;
  fuera las tres `env_anadir` y los dos «temprano».
- **El `handle` común**, `bpvm_bmgr_handle()` (en `bpvm_repl.c`, junto al RUN): lo que hacen hoy los tres —el reparto
  del búfer, las copias en la zona de rascar, rellenar el gestor, la petición desde el JSON, despachar y volcar— más
  D2: si `escribir` falla, `ERROR FLASH_ERROR` «no se pudo escribir la flash: sigue el valor de antes» en vez del OK
  (`#636`). El bulk de `PACK_BURN_DATA` lo sigue leyendo la familia (ya lo hace antes de llamar).
- **`#637`**: la guarda de zona DENTRO de `stm32_flash.c` —sólo de `BP_LOG` al final de las particiones, y nada si el
  `.ld` y la cabecera no casan—, con la regla en una función común que se prueba en el PC (`test-flash`, con las tablas
  de la Pico y de la STM32), como la de `pico_flash.c`. Y **D4**: en la STM32, un mutex de FreeRTOS en
  `stm32_flash.c` (el FS desde la VM, el gestor desde el REPL y el log pueden escribir a la vez), con el volcado del
  log de un fallo sin esperar.
- **D7**: el IDE manda `RESET` tras un `PART_APPLY` bueno y reconecta. El protocolo: `FLASH_ERROR` y la vista.

**Los pasos** (cada uno su commit; las placas en UN lote al final, con copia del ENV antes de grabar):
0. sin cambio de conducta: `bpvm_flash_lock_*` → `pico_flash_lock_*` (es de la Pico);
1. el común (la cintura, la vista, el `anadir` y el `handle`), estrenado por el sim, con sus pruebas: el sim con
   `--flash-falla` (`FLASH_ERROR`, y el valor de antes al relanzarlo), dos `ENV_SET` seguidos que un programa ve los
   dos (D1) y la persistencia RELANZANDO el sim; y en C, la vista y el `anadir` con una flash de RAM que puede fallar;
2. la STM32 (la cintura, la vista sin copias en RAM, `#637`, el mutex y `test-flash`);
3. la Pico (la cintura y la vista: `#638`);
4. las ESP32 (la cintura y el espejo);
5. el IDE (D7), el protocolo y el contrato;
6. el lote: las placas con la imagen nueva —lleva también `#640` y, a la Pico, el P4 y la Discovery, `#639` (3)—, y en
   cada una: `ENV_SET` ×2 con un programa que lee `Machine.getEnv` (D1), `run_placa` y la tanda.

**Hecho** (3-oct, noche): los pasos 0 a 5 (`6a5b079d`, `a88520e4`, `7ffeebfc`, `60b6b223`, `e5e3676d`, `4eb1cf00`) y el
lote en las NUEVE placas, con el OK de Eduardo y una copia del ENV de cada una antes de grabar (por SWD las dos STM32,
por el cable la Pico 2 y la Metro, sólo la app las cinco ESP32): en todas, la imagen igual a la del PC, el ENV igual
que antes, D1 verde (dos `ENV_SET` y un `ENV_DEL` que un programa ve sin reiniciar: `#638`, en la Pico donde estaba),
`run_placa` 34 de 34 y la tanda verde; la línea del arranque de las particiones, vista en las tres familias. Lo que
salió por el camino:
- **los packs sellados AGUANTARON** donde el `nm` decía que no (`_heap_start` +32 B en la C6, la C5 y el P4): el bloque
  de los motores está en la PSRAM (la C5, el P4) o alineado a 4 KB (la C6), y no se movió; la verdad, el `packRamBase`
  del INFO antes y después. Pero el `Gui` de la C5, que cargaba, era anterior a `rot=` (allí `gui.c` va en el pack):
  `GuiArbolG2` FALLO hasta regrabarlo (OK de Eduardo: `PACK_FORMAT`, `Gui` y `DriverSpi`);
- **la tanda llenaba el FS de la Nucleo** (504 KB en bloques de 8 KB, y 40 pruebas): ahora cada prueba borra lo suyo de
  `/app` al acabar (`e57c5e64`);
- **D7, probado por Eduardo en el IDE** con la Nucleo: aplicar, reinicio, reconexión sola y «ha vuelto». El primer aviso
  era modal y estaba delante del reinicio, que esperaba a que se aceptara: ahora va sin modal y se cierra solo
  (`15d1366a`);
- D2 (`FLASH_ERROR`) no se puede provocar en placa: se prueba en el sim (`--flash-falla`) y en C (`test-bmgr`).

D6 (el sim tan estricto como la placa; Eduardo: *«esto hay que pensarlo»*), a SUELTAS al cerrar H10 (Eduardo, 3-oct):
`#647`. **H10 CERRADO el 3-oct.**

## H12 — los dos núcleos: el modelo de Eduardo (1-oct), y lo que hay

Eduardo, al vaciar H7 (*«lo del modelo final de H12, lo normal sería decidirlo en H12, pero bueno»*):

> *«Mi idea, en las placas con 1 núcleo → 2 hilos (IO y run1 de la VM). Los 2 hilos del SO se van ejecutando 1 detrás
> de otro. El hilo de ejecución de la VM, si no hay ningún programa BP cargado, no hace nada, mira la cola, no hay
> nada, así que pasa el control al siguiente hilo del SO. Si hay varios hilos BP en la cola (el hilo principal del
> programa y el resto de hilos creados con Thread), ejecuta un thread BP y cuando le toca pasa al siguiente, y así
> hasta el final de la cola, y luego continúa desde el principio, y así indefinidamente o hasta que el programa BP se
> termine. Ahora, dispositivos con 2 núcleos (la Pico, la Metro y las dos placas P4). Con 2 núcleos creamos 3 hilos de
> ejecución a nivel del SO: el IO, el run1 de la VM y el run2 de la VM. El funcionamiento es casi igual que en los
> dispositivos de 1 núcleo: tenemos 3 hilos a nivel de SO que los ejecutan los 2 núcleos. Así que los hilos están en
> una cola con 3 elementos y los cores van mirando cuál es el siguiente hilo en espera y lo ejecutan. A nivel de
> programas BP, funciona casi igual que antes; lo que ocurre es que los 2 runs de la VM van mirando la cola de hilos
> BP y van ejecutando lo que haya en ella.»*

| | hilos del SO | quién los reparte | los hilos BP |
|---|---|---|---|
| **un núcleo** (la C3, la C5, la C6, las STM32) | `io` y `run1` | el RTOS, uno tras otro | `run1` los recorre en rueda, un cuanto cada uno |
| **dos núcleos** (la Pico, la Metro, las dos P4 y la S3) | `io`, `run1` y `run2` | los dos núcleos toman el siguiente listo, sin anclar | `run1` y `run2` sacan de la MISMA cola |

Sin programa, una VM mira la cola, la ve vacía y cede el núcleo.

**Lo que hay hoy** (leído el 1-oct):
- **Todas las placas corren con UN núcleo**: la Pico compila con `BPVM_PICO_NUM_CORES=1` (el valor por defecto), y la
  S3 y la P4 con `CONFIG_FREERTOS_UNICORE=y` (`c080e1f0`, de `A4`). O sea que el modelo de un núcleo es el de hoy:
  `io` y la tarea de la VM, la arquitectura de `A1` (Eduardo, 4-sep: *«de momento 1 VM, 1 core y 2 hilos a nivel de
  OS; en un futuro tendremos dos VM y una cola de threads BP»*).
- **Las dos VM sobre una cola ya existen en el PC**: `scheduler_smp.c` (`--smp=N` en la VM-C, `--workers=N` en miVM),
  con el corpus con dos workers en la red (`check-smp`) desde `#599`; en la Pico, la opción de compilación de H2
  (`BPVM_PICO_SMP_WORKERS`, mayo), que no usa nadie.
- **Lo que difiere del modelo**: `io` vive sólo durante un RUN —las cuatro familias lo arrancan y lo paran en el camino
  del RUN— y, fuera de él, el cable lo atiende la propia tarea de la VM: es el REPL. En el modelo, la VM sin programa
  no hace nada (*«mira la cola, no hay nada, pasa el control»*), así que `io` vive siempre y el REPL es suyo, que es lo
  que `A1` decidió el 4-sep (*«`io` = wire, OUTPUT por línea, log, REPL, LVGL»*) y se quedó a medias. Es el primer paso
  de H12, y vale igual con un núcleo.
- **En reposo** (encendida, sin programa; Eduardo, 1-oct: *«¿hay algún hilo del SO ejecutándose?»*): sí, siempre
  la tarea de la VM haciendo de REPL, más la ociosa de FreeRTOS —`io` no existe—, y cada familia espera distinto
  (el censo de `#490`, 10-sep, comprobado hoy en el código): la Pico mira el USB y la SD y duerme 10 ms (~100
  vueltas/s), y su ociosa gira sin parar el núcleo; las ESP32 y la P4 esperan en el driver hasta 100 ms (~10/s), y
  su ociosa para el núcleo (`waiti`/`wfi`) hasta la siguiente interrupción; la STM32 **no cede**: lee el anillo de la
  UART sin parar y su ociosa no llega a ejecutarse nunca. En el modelo, esa VM sin programa cedería.

**El arranque y los runs** (Eduardo, 1-oct, después: *«Empecemos por el principio, el boot»*):

> *«El SO arranca, va haciendo su trabajo: activa el sistema, lee las variables de entorno, las particiones, monta el
> FS, etc. En un momento dado activa la comunicación con el PC (o al menos se pone a la escucha). Y cuando termina todo
> eso, si no tiene nada más, se queda ahí. Pero para que le podamos decir que haga algo tiene que estar a la escucha.
> Para mí ese hilo de ejecución es el hilo principal del SO al que llamamos IO (para mí, no digo que ahora sea así).
> Como parte del sistema de arranque debería crear 1 o 2 hilos de ejecución de la VM, independientemente de si hay o
> no algún programa que ejecutar. Un tema importante: VM hay 1, pero runs puede haber hasta 2. Los runs comparten la
> memoria, el heap, etc. Lo que tienen independiente son sus registros BP, su stack BP y alguna cosa más que puedan
> necesitar.»*

| | hoy | el modelo |
|---|---|---|
| quién arranca y se queda escuchando | un solo hilo: hace el arranque y luego es el REPL | `io`: hace el arranque y escucha siempre |
| cuando llega un RUN | ese mismo hilo ejecuta el programa —es el run— y crea un `io` provisional para el cable, que muere al acabar | `io` sigue escuchando; el programa lo ejecuta un run que ya existía |
| los runs | nacen con cada RUN —en las placas, uno; en el PC con `--smp=N`, N— y desaparecen al acabar | nacen con el arranque (uno o dos) y viven siempre; sin programa, esperan |

**Lo que ya existe, en el PC**: una VM con varios runs es el planificador de varios workers de la VM-C
(`scheduler_smp.c`, `--smp=N`, de H2) y el de miVM (`--workers=N`), en la red con dos workers. Es lo que describe el
modelo: una VM —el heap, la tabla de handles, los módulos, las globales— que comparten los runs; cada run coge un hilo
BP listo que no tenga ya otro run, lo ejecuta un cuanto y lo suelta; lo independiente es DE CADA HILO BP —sus registros
(pc, sp, bp) y su pila—, que el run toma prestados mientras lo ejecuta, y lo del run es poco: qué hilo lleva. Un run
sin nada que hacer espera dormido (una condición), y el GC para a todos (stop-the-world). Al mirarlo salió `#616`:
el de la VM-C no repartía en rueda —buscaba siempre desde el primer hilo—; arreglado el mismo día (`c671d3a2`). En las
placas, ese planificador está compilado (la Pico y la P4) y apagado.

**La cola de hilos BP con dos núcleos** (Eduardo, 1-oct: *«Entiendo que la cola de tareas del SO la gestiona
FreeRTOS sin problemas y ahí no hay que hacer nada. Ahora la cola de Threads BP la gestionamos nosotros. Aquí es
fundamental que pueda funcionar en un entorno multitarea real ya que tendremos 2 núcleos corriendo. Todas las
operaciones de añadir/borrar Thread BP en la cola como reservar y liberar deben soportar multihilo a nivel de 2
núcleos trabajando»*). Cómo está hoy en el planificador de varios runs (leído el 1-oct):

| operación | cómo va | con dos núcleos |
|---|---|---|
| **añadir** un hilo (`start`) | entero bajo el cerrojo de la VM: el hueco, su pila, el marco y, lo último, «listo» (`bpvm_thread_spawn`) | sí |
| **reservar** (un run coge el siguiente) | bajo el cerrojo: uno listo y sin dueño, y el run lo marca como suyo (`sched_owner`) | sí; y en rueda desde `#616` |
| **liberar** (acaba su cuanto) | bajo el cerrojo: suelta el dueño y avisa a los que esperan | sí |
| bloquear y despertar por un **mutex BP** | bajo el cerrojo, y apuntarse y bloquearse JUNTOS (la carrera de B1, 9-sep) | sí |
| **dormir, `join`, terminar, ceder** | el hilo cambia SU estado sin cerrojo —nadie más lo toca mientras tiene dueño, y el cerrojo lo publica al soltarlo—; despertarlo es del planificador, bajo el cerrojo | sí |
| **borrar** (dar el hueco de un hilo acabado a uno nuevo) | bajo el cerrojo, y sólo si acabó del todo y no tiene dueño (`#604`) | sí |
| el **GC** | para a todos los runs (stop-the-world), bajo el cerrojo | sí |

Probado en la red con dos y cuatro workers (el corpus, `CedeCero`, `HeapEstres`, `TablaHandles`, `Rueda`), pero en un
PC: un x86 ordena la memoria entre núcleos mejor de lo que están obligados el ARM y el RISC-V, y en el PC las esperas
son las de pthreads, no las de FreeRTOS. **Lo que falta para dos núcleos de verdad:**
1. **La variable de condición de FreeRTOS**, en las tres plataformas: un semáforo BINARIO, y un aviso a todos da un
   `Give` por cada uno; el binario no pasa de 1, así que con dos esperando se pierde un aviso y uno se queda dormido —y
   los runs esperan sin plazo—. Visto el 10-sep (`FUSION_BACKENDS_FREERTOS.md`, R8 y su paso 3, que no depende de
   fundir nada), sin síntoma hoy porque en las placas sólo hay un run. Con dos, es lo primero que se notaría.
2. `kill_requested`, un `volatile int` que escribe `io` y leen los runs: atómico y con orden explícito.
3. En la Pico, el `printf` de la consola comparte el USB con el wire sin su cerrojo.
4. Lo que el inventario de `A4` marcó como compartido sin dueño.

(Del 2 al 4 ya estaban en la lista de H12 de «antes de activar el segundo núcleo»; el 1 se añade.)

**Lo que el modelo decide de lo que H7 dejaba abierto** (el índice: «VMs ociosas a piñón o parqueadas» y «los
workers como cola de hilos de SO en vez de N fijos»):
- **Cuántas VM**: tantas como núcleos —una o dos, fijas por placa—, más `io`; la cola de hilos de SO es la del RTOS, de
  la que tiran los núcleos. Sin anclar: el índice de H12 decía «`io` en un núcleo y `vm` en el otro», y el modelo deja
  que cada núcleo coja el siguiente que esté listo.
- **Una VM ociosa cede.** Queda por medir CÓMO, porque «pasar el control» tiene dos lecturas en un RTOS: ceder el turno
  (`taskYIELD`) —si los demás hilos están bloqueados vuelve enseguida, y la VM da vueltas con el núcleo al 100 %— o
  esperar en la cola —dormida hasta que llegue un hilo BP, sin gastar, a cambio de la latencia de despertar—. Es la
  medida que el índice ya pedía antes de decidir (y el bajo consumo de V8 preferirá la segunda).

**La S3**: también tiene dos núcleos (dos LX7) y el índice de H12 la contaba entre las placas de la prueba; el modelo
nombraba la Pico, la Metro y las dos P4. **Decidido por Eduardo** (1-oct, a la pregunta): *sí, con tres hilos*, como
las demás de dos núcleos.

### El plan por ETAPAS (2-oct, PROPUESTA; el criterio, de Eduardo)

> Eduardo, 2-oct: *«Hay que ir pensando como abordar H12. Lo primero es preveer que podamos fracasar parcialmente o
> totalmente. Entonces lo abordamos por etapas y si una etapa no funciona, no seguimos. O se soluciona o se cambia el
> enfoque. Aquí el criterio ha de ser la seguridad, si no es seguro 100 x 100 no se implementa. El orden concreto como
> lo veas. Quizás habría que aclarar lo del hilo IO y que en todas las imágenes esté igual.»*

**Las reglas de todas las etapas**:
- **una puerta al final de cada una**, escrita ANTES de empezarla: qué tiene que pasar, dónde y cuánto tiempo. Si no
  pasa, no se sigue: se arregla o se cambia el enfoque. Un fallo que no se explica no es un fallo raro: es una puerta
  cerrada;
- **cada etapa se puede deshacer**: un interruptor (en el ENV cuando es de la placa, en el build cuando es de la
  imagen) que deja la placa como estaba la etapa anterior; y lo que quede a medias, fuera de la imagen que se publica;
- **«seguro al 100 %» se demuestra, no se supone**: un concurrente no se prueba sólo con que la red salga verde (el x86
  ordena la memoria mejor que ARM y RISC-V, y una carrera puede no salir en mil vueltas). Por eso, en cada etapa:
  (1) el INVENTARIO de todo lo compartido —quién lo escribe, quién lo lee y qué lo protege—, revisado por otro par de
  ojos que intenta romperlo (revisión adversarial); (2) pruebas que PROVOCAN la carrera que el inventario dice
  imposible —con la ventana ensanchada, como `#609`—; (3) el SABOTAJE de control: quitar un cerrojo y ver que la prueba
  lo caza; (4) horas de resistencia en placa, no minutos;
- **un núcleo antes que dos, y una placa antes que todas**: lo que se puede probar con un núcleo se prueba con uno.

**Las etapas**, en el orden que decidió Eduardo (2-oct: *«Podemos, después de hacer IO, hacer el Run1 de VM. Esto
debería ser válido para todos los micros y debería de funcionar y si no hay nada raro se puede dejar permanente,
funcione o no el resto. Después el 2º run de la VM pero solo para micros de 2 núcleos (hacemos IO y 2 runs pero todavía
con 1 núcleo) y por último activar el 2º núcleo. Las 2 últimas etapas se pueden hacer en el PC y si funcionan en la
Pico o Metro. Y si todo funciona se extiende al resto de placas de 2 núcleos.»*):

| etapa | qué | dónde | la puerta (resumen) | volver atrás |
|---|---|---|---|---|
| **0 · `io`** | **el hilo `io`, IGUAL en todas las imágenes**: primero el `RUN` común (decidido en H10: su poll y su sink son el trabajo de `io`); después, `io` permanente —hace el arranque y atiende el cable siempre— | todas las placas, con UN núcleo | la tanda entera en las nueve; `KILL`, `RESET` y el depurador; el rendimiento, igual (`PrintBench`) | familia a familia: el `RUN` viejo detrás de su puntero |
| **1 · `run1`** | **la VM como un run permanente**: nace con el arranque, haya programa o no; sin programa, ESPERA en la cola (y cede el núcleo: hoy la STM32 no cede nunca); con programa, recorre los hilos BP en rueda. Es el modelo de un núcleo entero —`io` y `run1`— y **se queda aunque el resto no llegue** | todas las placas | la tanda; el reposo medido (la ociosa del RTOS corre en todas, y cuánto tarda en arrancar un `RUN`); horas sin un cuelgue | el run que nace con cada `RUN`, como hoy |
| **2 · el 2º run, con UN núcleo** | **dos runs que sacan de la misma cola, repartidos por el RTOS con expropiación**. Antes, lo que dos runs necesitan aunque haya un núcleo: la variable de condición con semáforo CONTADOR (con el binario, un aviso a todos con dos esperando despierta a uno), `kill_requested` atómico y con orden, el `printf` de la Pico con el cerrojo del cable, el inventario de `A4` repasado | sólo las placas de dos núcleos: **primero el PC** (la VM-C con `--smp=2` atada a una CPU), **después la Pico o la Metro** | el corpus y las de estrés (`HeapEstres`, `CedeCero`, `TablaHandles`, `Rueda`, `synclisttest`); pruebas que provocan el aviso perdido, con su sabotaje; horas de resistencia | `vm.runs=1` en el ENV |
| **3 · el 2º núcleo** | **los dos runs en dos núcleos de verdad**; en placa, en dos pasos: el RTOS en modo de dos núcleos con UN run (todo lo demás igual), y luego los dos runs | **el PC** (ya hoy: `check-smp`), **la Pico o la Metro**, y si todo va, **el resto de las de dos núcleos** (la otra RP2350, la S3 y las P4 —con la GUI: aquí se verifica `invokeLater`—) | lo de la 2, más el inventario de lo que cambia un RTOS de dos núcleos (interrupciones, la flash, el USB), placa por placa | la imagen de un núcleo; `vm.runs=1` |
| **4 · medir y decidir** | la batería de rendimiento HW+SW (de H9), con uno y con dos núcleos | las de dos núcleos | Eduardo decide quedarse o revertir —revertir es un resultado válido— | la imagen de un núcleo |

Las etapas 0 y 1 valen por sí mismas aunque H12 se pare ahí: un solo `RUN`, un solo `io`, y una VM que en reposo
espera sin gastar (lo que pedirá también el bajo consumo de V8).

### Etapa 0 — el hilo `io` hoy, imagen por imagen (mapa verificado, 2-oct)

Un agente leyó el código de las diez imágenes (y el IDF y el kernel de FreeRTOS); lo que toca la seguridad, comprobado
a mano (`#634`, `#635`). **Lo que es igual**: el lazo, el troceo, las colas y la parada de `io` (`src/bpvm_io.c`); que
nace con cada `RUN` desde la tarea de la VM y muere con él; y la regla de prioridad (hereda la de quien lo crea).
**Lo que difiere**:

| | tarea VM/REPL (prio · pila) | cómo espera el cable en reposo | ociosa | `io` (pila · prio · cola) | fin del hilo | depurador |
|---|---|---|---|---|---|---|
| Pico/Metro | `vm_task` 2 · 16 KB | sondeo de USB y SD, 10 ms | gira (sin WFI) | 4 KB · 2 · 1 KB | se borra sola | por la cola de control |
| S3, C3, C5, C6 | `main` 1 · 16 KB | bloqueada en el driver, ≤100 ms | `wfi` | 6 KB · 1 · 2 KB | se borra sola | por la cola |
| P4 | `wire_uart` **5** · 32 KB | driver (o TCP), ≤100 ms | `wfi`; **watchdog de tareas** 5 s | 6 KB · **5** · 2 KB | se borra sola | por la cola |
| Nucleo, DK2 | `vm` 2 · 16 KB | **sondeo del anillo sin ceder nunca** | **no corre** | 4 KB · 2 · 2 KB | suspender + `join` | no hay |
| sim | hilo principal | `accept`/`recv` | — | hilo · — · 4 KB | `pthread_join` | **dos lectores del socket** |

Y además: `io` NO lleva el log ni LVGL, aunque `bpvm_io.h` lo diga (LVGL lo bombea la tarea de la VM; el log lo escribe
cada hilo, sin cerrojo); la espera de `io` es de 5 ms, salvo en las ESP32, donde el tick de 100 Hz la sube a 10; el
arranque lo hace `boot2` en las placas, partido en la P4 y propio en el sim; y la P4 es la única con el watchdog de
tareas y con la VM a prioridad 5 —sin un porqué en el código—.

**Lo que hay que tocar para que `io` sea el mismo y permanente** (el riesgo, del mapa): separar `io` de la VM (hoy
nace y muere con ella, y el orden «la última línea y el `KILL_REPLY` antes del `EXITED`» descansa en cerrar su cola y
hacer `join`) —alto—; las ramas que dependen de `vm->io == NULL` —medio—; el contrato de prioridad («hereda de quien lo
crea» se invierte si `io` crea los runs; la «misma prioridad» está medida) —alto—; el `RUN` y el poll ×4 (el `RUN` común
de H10) —alto—; los cuatro lazos de reposo pasan a `io` (en la STM32 tendría que bloquearse de verdad: la interrupción
del cable de la DK2 está por encima del umbral de FreeRTOS) —medio-alto—; el arranque a `io` —medio—; pilas y heap (el
arranque y los verbos del REPL corren hoy en los 16 KB de la VM, y `io` tiene 4 o 6; el heap de FreeRTOS de la Pico y la
STM32, 32 KB, ya va justo) —alto—; el fin de los hilos, uno solo (`#635`) —medio—; el `KILL` (la ventana con
`preparar_run`, y en pausa: `#634`) —medio—; el log sin cerrojo —bajo-medio—; cómo espera `io` en reposo (cola y cable a
la vez) —consumo y latencia—. Y para la etapa 2, confirmado leyendo el kernel: la variable de condición con un semáforo
BINARIO pierde un aviso con dos esperando (y una espera con plazo que vence descuenta el contador aunque un aviso ya la
hubiera contado); hoy ninguna tiene dos esperando a la vez.

### Etapas 0 y 1 en la Pico 2 (4-oct): el diseño (decidido: `run` permanente)

Eduardo (4-oct): *«nos centramos en una placa (la Pico 2) y más adelante lo aplicamos al resto»*. Por la cascada,
primero el micro simulado —comparte `bpvm_io.c`, el `RUN` común y el REPL común—, después la Pico 2; la Metro lleva la
misma imagen.

**Lo que hay hoy** (leído el 4-oct, después del `RUN` común y del `board_mgr` común):
- **La Pico**: `main()` crea UNA tarea, `vm_task` (16 KB de pila, del heap de FreeRTOS —32 KB en total—, prioridad 2),
  que hace el arranque (el de la placa y el común, `boot2`), el autorun y el REPL (`repl_v1_run`: el USB sin esperar y,
  si no hay nada, 10 ms dormida). Un `RUN` lo ejecuta ella misma (`bpvm_repl_run`): crea la VM, carga, contesta
  `RUN_REPLY`, arranca `io` —otra tarea, 4 KB y prioridad 2: lee el cable (`run_poll`) y saca la salida por líneas— y
  llama a `bpvm_run`; al volver, para `io` (cierra la cola, la drena entera y la une) y manda `KILL_REPLY` y `EXITED`.
- **El simulado**: igual, con hilos: el principal acepta al cliente, hace de REPL y ejecuta el programa; `io`, un hilo
  por `RUN`.
- **Medido** (las tandas del 3-oct): a `vm_task` le sobraron 7,6 KB de sus 16 en la Metro tras la tanda entera (gasta
  ~8,7 KB, con el arranque y el REPL); a `io`, 3,3 de 4 KB; el heap de FreeRTOS bajó a 10,4 KB libres en el peor momento.

**El cambio**:
- La tarea que arranca y hace de REPL se queda así PARA SIEMPRE y es **`io`**: durante un `RUN`, en vez de ejecutar el
  programa, hace lo que hacía el `io` de cada `RUN` —el lazo de `io_loop`: la salida por líneas y el cable con
  `run_poll`—.
- El programa lo ejecuta una tarea nueva, **`run`**.
- Entre las dos, dos avisos: **«trabajo»** (`io` → `run`: la VM ya cargada, y si se depura) y **«hecho»** (`run` →
  `io`: lo que devolvió `bpvm_run`). La última línea antes del `EXITED`: `run`, al acabar, pone una bandera (`fin`:
  ya no escribe más) y da el «hecho»; el lazo de `io` cierra la cola ÉL MISMO al ver la bandera y sale cuando queda
  seca; después espera el «hecho», y sólo entonces `KILL_REPLY`, `EXITED` y destruir la VM. `run` no cierra ni
  destruye nada de `io`: lo que se destruye lo destruye quien lo cerró, en su hilo (la primera idea, que `run` cerrara
  la cola, dejaba una ventana: `io` podía destruir la cola con `run` aún dentro de su cierre).
- Lo demás, como hoy: la carga y el `RUN_REPLY`, en `io` y antes del «trabajo»; el depurador, en `run` (su lazo de
  pausa saca los comandos de la cola de control, que llena `io`); el `KILL`, `io` → `kill_requested`; el `RESET`,
  después del `EXITED`.

**La decisión**: ¿`run` nace con cada `RUN` y muere al acabar —la etapa 0 del plan— o nace con el arranque y espera
—la etapa 1—? **Decidido por Eduardo (4-oct): `run` permanente, las dos etapas juntas.** Por qué (lo que se le
propuso):
- crear y borrar una tarea por `RUN`: una tarea que se borra a sí misma la acaba de liberar más tarde la ociosa de
  FreeRTOS, y un `RUN` que llega enseguida (el IDE lo hace: `KILL` y otra vez `RUN`) pediría otros 12 KB con los
  anteriores aún sin devolver —«no cabe»—, o, con la pila estática, pisaría la de una tarea que no ha acabado de morir;
- la GUI del simulado (SDL, en Windows): una ventana sólo recibe sus mensajes en el hilo que la creó, y la crea el
  programa la primera vez que usa la GUI (`lvgl_ensure_init`) y sigue abierta para el siguiente: con un hilo por `RUN`,
  el segundo programa bombearía una ventana ajena, congelada. Con `run` permanente, todos los programas corren en el
  mismo hilo;
- y sería código que la etapa 1 tiraría.
De la etapa 1 queda para su placa que la VM ociosa ceda el núcleo en la STM32 (allí el REPL no cede nunca).

**La memoria** (la Pico): la pila de `run`, un array ESTÁTICO (`xTaskCreateStatic`), en el `.bss`: la VM recibe menos
ella sola —su bloque es «lo que queda» tras el `.bss`—, sin tocar el margen de `malloc` (64 KB, que es de donde las
natives sacan la suya). **16 KB, los mismos que `vm_task`** (se propusieron 12; la revisión adversarial lo tumbó): el
GC corre en ella y su marcado es RECURSIVO —32 B por nivel de una cadena de referencias, medido en la Pico 2: con la
imagen del 3-oct, una lista de ~360 nodos ya desbordaba (`#648`); con 12 KB, el techo bajaba a ~300—. Los 4 KB de más
salen del heap de FreeRTOS (de 32 a 28 KB: el `io` de 4 KB de cada `RUN` ya no nace, y el peor momento se queda en los
10,4 KB de antes). La VM, de 333 KB a 321. El INFO publica las dos marcas: `runStackFree` (la de `run`, nueva) e
`ioStackFree` (la de la tarea del REPL, que hace de `io`).

**La revisión adversarial** (4-oct, un agente con el código, verificada a mano): el protocolo aguanta con un núcleo
—ni línea perdida, ni `EXITED` adelantado, ni semáforos desparejados (todas las salidas tempranas de `bpvm_repl_run` van
antes del «trabajo»), ni uso tras liberar, ni interbloqueo del depurador—. Lo que salió, arreglado: (1) la pila, 16 KB
(arriba); (2) el `EXITED` llegaba hasta 5 ms tarde en un programa que no imprime al final —nadie despertaba a `io`—: ahora
`run`, tras poner `fin`, lo despierta sin cerrar la cola (saboteado: 18 ms por `RUN` en vez de 1,2; caso G de
`run_tarea_smoke`); (3) `run` escribía en el log al nacer, a la vez que el arranque (el log no tiene cerrojo): lo dice
quien la crea; (4) una tarea de FreeRTOS que vuelve cuelga la placa: `bpvm_repl_run_tarea` ya no vuelve nunca; y el
comentario de `bpvm_dbg_wire.c` que decía que el `pause_cb` corría en la tarea del REPL. **Para dos runs o dos núcleos
(etapas 2 y 3), lo que dejó apuntado**: el testigo de fallos y el `callctx` del AOT son globales (`bpvm_aot_helpers.c`):
por tarea antes de la etapa 2; la variable de condición de semáforo BINARIO (ya en la lista); `run` no pasa por el
trampolín que prepara el candado de la flash (`pico_flash_lock_init_victim`) ni tiene afinidad; el MPU de `mpu=1` lo
arma `run` y lo desarma `io` (por núcleo, con dos); `thread_join` libera la pila de un ejecutor que aún puede correr en
el otro núcleo; `fin`, `viva` y `kill_requested`, atómicos con orden; el log, con cerrojo; y `s_tarea`, `s_run_*` y el
estado del depurador son únicos.

**Hecho en el PC** (4-oct): el común (`bpvm_io_preparar`/`bpvm_io_lazo` y la tarea en `bpvm_repl.c`) y el simulado
(`--run-viejo` es el interruptor). `check-sim`, 10 de 10; y `run_tarea_smoke` (en la red): mil `RUN` seguidos sin una
línea perdida, la salida tardía, el `KILL` barriendo el final del programa (8 parados y 20 acabados solos, cada uno con
su `EXITED` y su respuesta), la ventana ensanchada (`--ensayo-hecho=150`), el sabotaje cazado (`--sabotaje-cierre`: se
pierde la última línea) y `--run-viejo`. Y la GUI del simulado con LVGL: dos programas seguidos, el segundo pinta en la
ventana que creó el primero (capturas «PRIMERO» y «SEGUNDO»).

**Lo compartido entre `io` y `run`** (el inventario, para la revisión adversarial):

| qué | escribe | lee | lo protege |
|---|---|---|---|
| la VM (`bpvm_t`) | `io` la crea y la carga; `run` la ejecuta | `io` lee sus errores y la destruye | los dos avisos: nada antes del «trabajo», nada después del «hecho» |
| `vm->io` (la cola de salida, la de control, el cerrojo del cable) | `io` la crea y la destruye; `run` escribe la salida | `io` la salida; `run` la de control | sus mutex, y los dos avisos |
| `kill_requested` | `io` | `run`, entre cuantos | un núcleo: `volatile`; con dos, atómico (etapa 2) |
| lo que devuelve `bpvm_run` | `run`, antes del «hecho» | `io`, después | el aviso |
| `s_run_kill`, `s_run_reset` | `io` (`run_poll`); `run` sólo en el camino sin `io`, que deja de existir | `io` | — |
| el cable | `io` y `run` (la salida; las respuestas del depurador) | `io` | el cerrojo del cable (`tx`) y el de `wire_v1` |
| el latido (`latido=1`) | `run` (el pc y el estado de los hilos) | `io`, sin cerrojo | nada: diagnóstico, puede leer rancio (como hoy) |
| el FS (un `GET`/`LIST` troceado mientras el programa usa ficheros) | los dos | los dos | el cerrojo del FS por trozo (`#495`, como hoy) |
| el log | los dos | — | nada (como hoy; el mapa lo marcó bajo-medio) |

**Las pruebas, primero en el simulado**: mil `RUN` seguidos de un programa que imprime y acaba (ni una línea perdida,
el `EXITED` siempre detrás); un `KILL` justo cuando el programa acaba, barriendo el instante; un `RUN` nada más llegar
el `EXITED`; la ventana ensanchada (como `#609`): una espera en `run` entre `bpvm_run` y el «hecho»; el sabotaje: `io`
sin esperar el «hecho» destruye la VM con `run` aún dentro, y una prueba tiene que cazarlo; y la red entera
(`check-sim`, `run_smoke`, `sim_dbg_smoke`, `env_smoke`).

**La puerta** (la Pico 2): la tanda de V7 entera; `run_placa` 34 de 34 (el `KILL`, el `RESET` y el depurador con el
programa corriendo); `PrintBench` como hoy (±3 %); las dos marcas de pila, con margen; el reposo (la ociosa corre, y
un `RUN` arranca igual de rápido); y dos horas de resistencia (`RUN`, `KILL` y `RESET` en bucle, y la tanda) sin un
cuelgue. Después, la Metro con la tanda.

**La puerta en la Pico 2: PASADA** (4-oct, con el OK de Eduardo): la tanda de V7 verde; `run_placa` 34 de 34;
PrintBench, +2,3 % en la mediana de 10 vueltas (836 frente a 817 ms; cada imagen baila ±5 % de una vuelta a otra, así
que el ±3 % se mira con medianas de 10, no con tiradas sueltas); el heap de FreeRTOS, 10,4 KB libres en el peor momento,
como antes; la pila de `run`, 9,6 KB libres en el peor momento de la tanda (el GC de `HeapInfo`) y el GC aguanta ~423
niveles (antes ~360); la del REPL, ~4 KB libres —la CARGA de `TimeDemo`, que ya era suya: `#649`—; y **~120 minutos de
resistencia, ~4 570 vueltas sin un fallo** (un programa de 21 líneas, un bucle parado con KILL a un instante al azar y
una salida tardía, en bucle, con un RESET con programa cada 40 vueltas: ~114), las marcas de pila y del heap quietas. (La
cortó el límite de 120 minutos de la herramienta unos segundos antes de su resumen: el log de la placa enseña vueltas
normales hasta ese instante.)

**La Metro y el P4: PASADA también** (4-oct, conectadas por Eduardo «para que puedas probar lo que quieras»; grabadas
con su OK). La Metro, la imagen de la Pico 2: su VM entera va a la PSRAM (6 MB), así que los 12 KB de la SRAM no le
quitan nada a la VM; su SQLite, vivo. El P4, la parte ESP32 escrita hoy (`repl_esp32.c`, encendida SÓLO en el P4 con
`BPVM_ESP32_RUN_TAREA`): `run` con 16 KB estáticos en la RAM interna (una pila en la PSRAM no puede escribir la flash) y
a la prioridad de `wire_uart` (5); sus packs (SQLite, Gui, Stdlib), vivos —el bloque de sus motores está en la PSRAM—.
En las dos: la tanda verde —en el P4, con las tres de GUI: LVGL en `run`, sobre la pantalla de verdad—; `run_placa` 34
de 34 —el depurador del P4, con su lazo de pausa en `run`—; PrintBench: el P4 idéntico (20 362 ms: lo limita su UART),
la Metro +3 % en la mediana (877 frente a 851 ms), dentro del ruido (dos tandas de la MISMA imagen difieren un 5 %); el
KILL, 2–6 ms en la Metro y 22–28 en el P4 (su tick de 10 ms); el GC de la Metro, ~423 niveles; y 30 minutos de
resistencia en cada una, sin un fallo (1 101 y 974 vueltas, 27 y 24 RESET).

**La memoria, ajustada** (Eduardo, 4-oct: *«si necesitamos más stack y nos ahorramos heap, podemos reducir un poco el
heap para compensar»*): en la Pico, el heap de FreeRTOS de 28 a 24 KB —el peor momento medido dejaba 10,4–10,7 KB
libres; con 24, ~6,4—, y la pila de `run` cuesta 8 KB netos. Compilado; se graba en el próximo lote. En la ESP32 no hay
nada que ajustar: su heap es la RAM libre. Y con `#648` y `#649` arregladas (las dos recursiones), las dos pilas
podrían bajar a ~8–10 KB.

**Las dos recursiones, fuera** (`#648` y `#649`, 4-oct: `07c16b4f` y `b2216270`): el marcado del GC, en las dos VMs
(la VM-Java también recurría, y con 4000 nodos ya caía en el PC), y la carga (`discover_deps`); las dos, con una pila
explícita y el mismo resultado. Lo que cambia para H12: el peor momento de `run` en la tanda era el GC de `HeapInfo`
(9,6 KB libres de 16 en la Pico 2, y ~423 niveles de lista como techo), y el de la tarea del REPL, la carga de
`TimeDemo` (~4 KB libres). Sin recursiones, lo que gastan deja de depender del programa —de lo hondas que sean sus
listas o sus cadenas de imports—, y las dos pilas pueden encoger: se mide en placa con el próximo lote (`GcHondo` e
`ImportHondo`, en la tanda; las marcas, prueba a prueba) antes de decidir cuánto. En el PC, `--pila=N` (la carga y el
RUN en un hilo con esa pila, y su marca de agua al acabar) mide antes de grabar: `hello`, 21 000 B en el PC de 64 bits.

**Volver atrás**: un interruptor de la imagen —la familia pone o no su tarea `run` en la cintura del `RUN`—; sin ella,
el `RUN` común de hoy. Las familias que no se han pasado siguen así (las ESP32 que no son el P4, y la STM32 —Eduardo:
*«puede esperar»*: antes, su REPL tiene que esperar de verdad, o `run` iría a media velocidad—).

**El orden**: (1) el simulado, con sus pruebas y la red; (2) la Pico 2 y su puerta; (3) la Metro; (4) el resto,
familia a familia —las ESP32 (el `main`, 16 KB y prioridad 1); la STM32, cuyo REPL tiene que bloquearse de verdad antes
(la interrupción del cable está por encima del umbral de FreeRTOS); y en las de pantalla, LVGL en `run`—.

**El lote, y la regla** (4-oct): grabadas la Pico 2, la Metro y el P4, y `run` en toda la familia ESP32 en el S3, la C5 y
la C6 (la tanda verde; en la C6, GuiArbolG2 sin RAM: `#651`). Eduardo, al ver la puerta entera repetida placa a placa:
*«Habíamos quedado que íbamos a trabajar con la Pico primero y si todo iba bien pasamos al resto»* — las etapas
siguientes, en el PC y la Pico 2; las demás placas, una vez, al final de H12.

### Etapa 2 — la COLA de hilos BP, permanente, y los runs permanentes (4-oct): las decisiones, el mapa y el plan

**Las decisiones de Eduardo** (4-oct, a la pregunta de cómo nacen los dos runs):
- *«Runs permanentes sí, pero antes de crearlos hace falta la cola de Threads BP que también es permanente, aunque esté
  vacía.»*
- Como `Collections.SyncList`, pero *«tiene que ser en C y una cola, no una lista; por lo demás es muy parecida»*: cada
  operación, atómica bajo un cerrojo; y quien saca de la cola vacía espera dormido —no con `sleep(1)`, como
  `popBlocking`—. (La clase de BP no puede ser la cola: la cola existe antes que cualquier programa, y quien saca de ella
  es el run, el C que ejecuta el bytecode.)
- *«Si los Threads BP tuviesen una referencia 'next' y 'back' formarían ellos mismos una doble lista y no haría falta un
  array.»* — una lista DOBLE e intrusiva en los propios descriptores, y UNA sola (*«no veo la necesidad de una
  segunda»*): todos los hilos vivos, recorridos en rueda, saltando los que esperan (un `sleep`, un `Mutex`, un `join`).
- *«¿Por qué tiene que haber un límite?»* — sin tope de hilos: el límite es la memoria (el descriptor, de la RAM del
  sistema, y la pila BP, de la región de pilas), con el error que ya existe («no hay memoria para la pila del hilo»). En
  las DOS VMs, por la paridad: el 32 de `#604` se va también de miVM. El número de un hilo nuevo, el siguiente, sin
  reutilizar ninguno (es su handle: abajo, «dónde vive cada cosa»; aquí decía «el más bajo libre, como hoy», y lo
  corrigió la decisión de después).
- *«Tiene que haber al menos una referencia a un elemento de la cola en la VM. Inicialmente puede ser null y si se vacía
  puede volver a ser null. Es importante que cuando se busque el siguiente elemento (o el anterior) no se dé vueltas
  indefinidamente.»* — la lista es CIRCULAR y la VM guarda una referencia: el cursor de la rueda (null, vacía). Buscar el
  siguiente listo da como mucho UNA vuelta (se para al volver al punto de partida); si no hay ninguno, el run espera
  dormido, y si nadie puede despertar a nadie, es un interbloqueo y se dice. (Hoy `bpvm_t` se rehace en cada RUN: la
  referencia vive en la parte PERMANENTE de la VM, la cola, que nace con el arranque; los descriptores del programa
  cuelgan de ella mientras corre.)

**Lo que hay hoy** (el mapa verificado del 4-oct: un agente con el código; lo que pesa, comprobado a mano):
- **Ninguna placa usa el planificador de varios workers**: la Pico no lo compila (`BPVM_PICO_SMP_WORKERS` no está en su
  build) y las ESP32, la STM32 y el micro simulado corren el simple (`scheduler.c`); el de varios, sólo en el PC
  (`--smp=N`).
- **La tabla de hilos, un array FIJO** de 32 descriptores dentro de `bpvm_t`: 136 B cada uno en un micro de 32 bits,
  4 352 B de los 12 376 de `bpvm_t`, que se pide a la RAM del sistema en cada RUN. **El planificador de varios workers**
  (`bpvm_smp_t`: el cerrojo, la condición, `shutdown`, `fallo`…) nace y muere con cada RUN (`bpvm_smp_init`/`destroy`),
  y sus workers también: el `join` es la barrera del fin del programa.
- **Los dos planificadores no hacen lo mismo**: el simple cede el núcleo entre cuantos, detecta el interbloqueo (todos
  bloqueados sin un `sleep`: RUNTIME_ERROR) y sirve al depurador (su pausa congela el programa entero); el de varios, no.
- **Para dos runs falta, además** (de más a menos grave): el contexto del AOT y el testigo de fallos son GLOBALES en las
  placas —`__thread` sólo en el PC— (crítico: un `longjmp` a la pila de otra tarea); la variable de condición de FreeRTOS
  es un semáforo BINARIO (crítico: con dos esperando se pierde un aviso, y su plazo tiene una carrera); el `gc()` del
  programa toma el cerrojo pero no para a los demás workers (alto); los eventos se encolan sin cerrojo (alto); el KILL no
  despierta a quien espera sin plazo; el depurador y la GUI son de un solo hilo; el log, la zona de rascar y algunos
  búferes estáticos, sin cerrojo.

**Y dónde vive cada cosa** (4-oct, después; Eduardo): *«¿Estás utilizando punteros o referencias? Tal como lo veo
deberían ser referencias»*; *«lo puedes poner todo dentro del objeto, pero vigila porque los objetos BP y las
estructuras de C no tienen por qué coincidir»*. Poner los enlaces en el objeto `Thread` (campos nuevos de `Core.Thread`)
dejaba fuera a los programas de V6 que la red sigue ejecutando (`SlotThreadSub` y `ThreadTrasMain`, de la cápsula: su
`Thread` lo sintetizaba V6 en cada `.mod`, con dos campos). Y lo decidido, de Eduardo: *«Esto se puede hacer de muchas
maneras pero cada una tiene consecuencias. Si haces una estructura de C que sea un Thread BP por mí vale, pero eso
implica que la estructura deberá alojarse con malloc y que Thread BP debería tener un handle a la estructura C.»* Así:
- **El hilo, para la VM, es una `struct` de C** (`bpvm_thread_t`: `pc`, `sp`, `bp`, los límites de su pila, el estado,
  la pila de manejadores, el ejecutor de natives…), pedida con `malloc` al arrancar el hilo (la del `main`, al empezar
  el RUN) y devuelta cuando sale de la cola. Uno por hilo VIVO: sin array y sin tope.
- **La cola son esas estructuras**: `next` y `back`, una lista doble y circular; la VM guarda un puntero a una —el punto
  donde sigue la rueda—, NULL si está vacía. Todo lo que la toca, en un solo módulo y bajo el cerrojo.
- **El `Thread` de BP tiene un handle a su estructura: su `__tid`**, el número del hilo, que NO se reutiliza nunca
  (como en la VM-Java). La VM lo resuelve dando UNA vuelta a la cola; si no está, el hilo acabó y su `join` vuelve
  enseguida. BP nunca guarda un puntero de C, y un handle caducado no resuelve. El objeto `Thread` no cambia de forma:
  valen los programas de V6.
- *«No puede haber programas sin Core»* (Eduardo): uno que no usa nada de `Core` se carga hoy sin él (`hello`) → `#652`.

**El plan, en dos pasos**, cada uno con su puerta escrita antes y su vuelta atrás (en el PC y después en la Pico 2):
- **2a · la cola permanente, con UN run.** La cola —el cerrojo, la condición y la referencia— nace en el arranque y vive
  siempre. Los descriptores, una lista doble circular sin array y sin tope (y el tope, fuera de miVM). El run espera en
  la cola; `io` carga la VM, la prepara (enlazar y prefabricar las excepciones, ANTES de colgarla) y la cuelga; cuando el
  programa acaba, el último run que sale la descuelga y avisa a `io`, que manda el `EXITED`. UN planificador, el de la
  cola, con lo que hoy sólo tiene el simple: ceder entre cuantos, el interbloqueo, la regla del KILL y el depurador (con
  un run). **Puerta**: el corpus en las dos VMs, `check-smp`, las de estrés y `run_tarea_smoke` en el simulado; en la
  Pico 2, la tanda, `run_placa`, PrintBench (±3 %) y la resistencia. **Volver atrás**: el interruptor de la imagen (el
  RUN de la etapa 1).
- **2b · el 2º run, con un núcleo.** Antes, los requisitos: un contexto del AOT de verdad por tarea; la condición con
  semáforo contador y el plazo arreglado; el `gc()` con stop-the-world; los eventos bajo el cerrojo; el KILL que despierta
  y atómico; el depurador y la GUI, con un solo run; el log con cerrojo. **Puerta**: la de la 2a, más las pruebas que
  provocan el aviso perdido (con su sabotaje) y horas de resistencia en la Pico 2. **Volver atrás**: `vm.runs=1` en el
  ENV. Y lo que añadió la revisión de la 2a.2 (4-oct): la condición de FreeRTOS, en sus TRES copias (`pico/`, `src/` y
  `esp32/common/`), pierde avisos con dos esperando —también los de la cola de salida, con dos productores— y un plazo
  que vence a la vez que un aviso deja una ficha suelta: un aviso por tarea (`xTaskNotifyGive`), con una prueba en placa
  de tres esperadores; cada run permanente con su número (hoy los dos serían el 0); el RUN publica con tantos runs como
  haya permanentes, y con uno si se depura; sin `io`, el cable lo miraría cada run (que lo mire uno); los eventos: el
  encolado bajo el cerrojo, resucitar sólo a un hilo que no tenga run (`sched_owner == -1`; si no, la excepción sin
  atrapar de un hilo que no es el main —#608— se tomaría por la del programa) y que el `join` cuente los eventos que su
  hilo debe (#595).

### Etapa 2a.2 — la cola permanente y UN planificador (4-oct): el diseño, para revisar antes de tocar código

**Lo que hay hoy** (después de la 2a.1, leído en el código):
- La rueda de hilos de la 2a.1 cuelga del PROGRAMA (`bpvm_t`): nace con él —el main, en `bpvm_init`— y se va con él.
- Un programa lo ejecuta, según dónde: en las placas y el micro simulado, la tarea `run`, que espera el aviso «trabajo»
  y llama a `bpvm_run`; en el PC (el ejecutable y más de 40 pruebas de C), el hilo que llama a `bpvm_run`, o los N
  workers de `bpvm_run_smp`, que nacen y mueren con el programa; en la STM32, la tarea del REPL (no tiene `run`).
- Dos planificadores que no hacen lo mismo: el simple (`scheduler.c`) cede el núcleo entre cuantos, ve el interbloqueo,
  para en cuanto llega un KILL, y la pausa del depurador congela el programa entero; el de varios (`scheduler_smp.c`)
  tiene el cerrojo, la condición, el mundo parado del GC y el primer fallo, pero ni cede ni ve el interbloqueo —se queda
  esperando para siempre—, y su estado (`bpvm_smp_t`) nace y muere con cada RUN.

**El diseño** —el plan de la etapa 2 («2a · la cola permanente, con UN run»), en detalle—:

1. **La cola, un objeto PERMANENTE**: el cerrojo, la condición, **la referencia** —el punto de la rueda, el hilo por
   donde sigue; NULL, vacía— y el programa publicado, más lo que los runs necesitan para repartirse el trabajo (cuántos
   tienen un hilo entre manos, el mundo parado del GC, el primer fallo, la orden de parar). Es lo que hoy es
   `bpvm_smp_t`, que deja de nacer y morir con cada RUN: nace con el arranque —en la placa, cuando la familia crea la
   tarea `run`; en el PC, con la primera VM— y no muere nunca. Un programa a la vez: colgar uno con la cola ocupada es
   un error, y se dice.
2. **Los hilos, en la cola.** La rueda de la 2a.1 deja de colgar del programa y cuelga de la cola. El main entra al
   COLGAR el programa, antes de PREPARARLO: la preparación reserva memoria (las dos excepciones prefabricadas, con el
   main de ancla), y el GC busca sus raíces en la cola. Los demás entran con `Thread.start` y salen al acabar, como en
   la 2a.1. Al acabar el programa, los que quedan —el main, y los que paró un KILL— se van con él, al destruirlo: hasta
   entonces los necesitan el ejecutor de natives que los apunta, las líneas a medias del final y la línea `pila:`.
   Destruido el programa, la cola queda vacía —la referencia, a NULL— hasta el siguiente. Una sola lista, como hasta
   ahora.
3. **Colgar, publicar y descolgar** (en la placa):
   - `io` —la tarea del REPL— carga el programa, mete su main en la cola, lo prepara (el enlace, el main desde el punto
     de entrada y las excepciones prefabricadas: hoy lo hacía `run` al empezar `bpvm_run`) y lo PUBLICA: desde ese
     momento lo ven los runs. Después, su lazo hasta `fin` y el «hecho», como en la etapa 1. Si la preparación falla
     (un fallo de enlace), no se publica, y el `EXITED` sale como hoy.
   - El run espera dormido en la cola mientras no haya un programa publicado —no en el aviso «trabajo», que
     desaparece—; con uno, el planificador (el punto 4).
   - Al acabar, **el último run que sale** escribe las líneas a medias y las líneas `pila:` y `hilos:` (fuera del
     cerrojo: escribir puede tener que esperar a `io`), lo DESCUELGA y avisa a `io` —`fin`, despertarlo y el «hecho»,
     como en la etapa 1—. Desde ahí no toca el programa: `io` manda el `EXITED` y lo destruye.
   - Si `io` no puede nacer (no hay memoria para sus colas), el RUN de siempre: el programa corre en la tarea del REPL,
     que atiende el cable entre cuantos, como antes de la 2a.2 (se pensó correrlo en `run`, y la revisión lo tumbó: la
     pila de `run` no está medida para el despachador del cable). Un programa colgado así no es para los runs
     permanentes: no lo cogen.
   - **En el PC, la STM32 y las pruebas de C**: `bpvm_run(vm)` cuelga el programa y hace de run el hilo que llama,
     hasta que acaba; `bpvm_run_smp(vm, N)`, N runs que acaban con él. La misma cola y el mismo planificador: sólo
     cambia quién espera el final. Y los runs permanentes sólo cogen lo que se publica para ellos (y `bpvm_run`, lo
     suyo): si coincidieran alguna vez, que no debe pasar, un programa no lo ejecutarían a la vez dos que no se
     esperan.
4. **UN planificador, el de la cola** —fuera los dos de hoy—, con lo que hoy sólo tiene el simple. En cada vuelta, un
   run:
   - si el mundo está parado (otro run colecta), espera;
   - con un **KILL**, un fallo o el STOP del depurador, deja de elegir; y si nadie tiene un hilo entre manos, el
     programa acaba: KILLED si se pidió parar, aunque acabara solo (`#462`, `#640`); si no, con el primer fallo;
   - despierta a los muertos con eventos pendientes (`#342`; antes que los `join`, `#595`), los sueños vencidos y los
     `join` cumplidos, y saca de la cola a los que acabaron del todo (la 2a.1);
   - elige en rueda (la 2a.1);
   - si no hay ninguno listo: sin hilos vivos y sin nadie con uno entre manos, **el fin**; sin ninguno durmiendo y sin
     nadie con uno entre manos, **el interbloqueo** —RUNTIME_ERROR, con el aviso de hoy; con `--smp`, hasta ahora, el
     programa se quedaba colgado—; si no, duerme en la cola hasta el sueño más cercano;
   - con uno: su evento pendiente (H5.c), **cede el núcleo** (`#462`: hoy sólo lo hace el simple) y le da un cuanto.

   **«Nadie tiene un hilo entre manos» se cuenta aparte del contador del GC**: un run que espera a que otro acabe de
   colectar no cuenta para el mundo parado, pero tiene su hilo a medias; contándolo con el mismo número, otro run
   vería un interbloqueo que no existe (un hilo que espera el `Mutex` que tiene ése). **El KILL despierta**:
   `bpvm_request_kill` avisa a la cola; hoy, con todos los hilos durmiendo, el run lo veía al vencer su espera —hasta
   50 ms, el tope de esa espera, y sin el tope (`bpvm_run` sin `poll`), el sueño entero—. **El depurador, con un
   run**: su pausa para al run, y con él al programa entero, como hoy. **Con un run, sin coste**: en la vuelta de cada
   cuanto el run no toma el cerrojo —nadie más toca la rueda: `io` sólo pone `kill_requested` y avisa—; lo toma para
   dormirse y para descolgar, como hoy el simple. Con dos (la 2b), siempre, y se mide.
5. **Lo que se va**: el lazo de `scheduler_smp.c` (se queda lo que es sólo del PC: arrancar y unir los runs de
   `bpvm_run_smp`, y la comm task del `--smp` sin `io`); el `bpvm_smp_init`/`destroy` de cada RUN; y el `correr` de la
   cintura, con el camino SMP de la Pico (`BPVM_PICO_SMP_WORKERS`, que su build no define: código muerto; en la 2b lo
   hace el segundo run).

**Lo compartido entre `io` y `run`** (la tabla de la etapa 1, con lo que cambia; lo demás, igual):

| qué | escribe | lee | lo protege |
|---|---|---|---|
| la cola: la referencia y el programa publicado | `io` mete el main y publica; el run descuelga; `io` la vacía al destruir el programa | los dos | su cerrojo; mientras el programa está publicado, con un run, la rueda la toca sólo el run |
| la VM | `io` la crea, la carga y la prepara —ahora también el enlace y el main— antes de publicarla; el run la ejecuta | `io` lee sus errores y la destruye | publicar (`io` no la toca después) y el «hecho» (el run no la toca después) |
| `kill_requested` | `io` (y el run, desde el depurador, si se va el cable) | el run | con un núcleo, `volatile`; el aviso, con el cerrojo de la cola: el run mira la bandera con el cerrojo cogido antes de dormirse, y no se pierde |

**Las pruebas** (primero en el PC; cada una nueva, con su sabotaje):
- la red entera: `compat check` (la paridad del corpus en MOD9, MOD8 y con dos workers, la cápsula de V6 y el estrés),
  las pruebas de C, `check-smp`, `check-sim` y `run_tarea_smoke` (los mil RUN —si la cola no se vaciara, el siguiente
  no se podría colgar—, el KILL barriendo el final, la ventana ensanchada);
- **el KILL que despierta**: un programa con todos sus hilos durmiendo un minuto se para enseguida, con `bpvm_run`, con
  dos runs y en el simulado. Sabotaje: sin el aviso, no se para hasta que vence el sueño;
- **el interbloqueo**: dos hilos y dos `Mutex` cruzados → RUNTIME_ERROR, con uno y con dos runs (hoy, con dos,
  colgado); y el FALSO, que no puede salir: hilos que se disputan un `Mutex` mientras reservan sin parar, con dos runs
  y muchas vueltas (colectas con un run esperando a otro). Sabotaje: contar «con un hilo entre manos» con el contador
  del GC → algún interbloqueo falso;
- **el último que sale**: con dos runs y la ventana ensanchada (un run se entretiene después de su cuanto), el fin se
  decide siempre después de que vuelva el último cuanto. Sabotaje: decidirlo sin mirar → rojo.

(La VM-Java no ve el interbloqueo —se queda colgada, ya hoy—: esas dos, sólo en la VM-C, como `test-hilos`.)

**La puerta**: todo lo de arriba, verde en el PC; en la Pico 2, la tanda de V7, `run_placa` 34 de 34, PrintBench ±3 %
(la mediana de 10), las marcas de la pila de `run` y del heap de FreeRTOS con margen, y unas dos horas de resistencia
(`RUN`, `KILL` y `RESET` en bucle, y la tanda) sin un fallo.

**Volver atrás**: un interruptor de la imagen (`BPVM_RUN_EN_COLA=0`; en el simulado, `--run-etapa1`): el run espera el
«trabajo» y ejecuta el programa con `bpvm_run`, como en la etapa 1. El planificador es el nuevo también ahí: los dos de
hoy recorren la misma rueda, y dejarlos de reserva sería guardar dos planificadores que nadie prueba; si fallara el
nuevo, se revierte su commit, que va aparte. **Decidido por Eduardo** (4-oct, a la pregunta de si le valía que el
planificador se deshiciera con git y no con el interruptor): *«Sí, con git»*.

**Por pasos**, cada uno con su commit: (1) la cola y el planificador, en el PC —`bpvm_run` y `bpvm_run_smp` encima; la
tarea `run` de las placas, todavía con el «trabajo»—, con la red entera; (2) los runs permanentes —el run espera en la
cola e `io` cuelga—, con el simulado; (3) la Pico 2 y su puerta: una sola grabación para los dos pasos.

**Hecho en el PC** (4-oct): el paso 1 (`b776a325`) con la red entera verde —la paridad 133/133 en MOD9 y en MOD8, el
corpus con dos workers igual que con uno, la cápsula de V6, las pruebas de C, el simulado, la sombra y el estrés del
heap— y sus pruebas nuevas, cada una con su sabotaje cazado: `test-kill`, fases 7-8 (un programa dormido una hora, con
`io` y sin poll, para en ~300 ms —lo que tarda en llegar el KILL— con uno y con dos runs; sin el aviso a la cola no se
para) y `test-cola` (el interbloqueo de verdad, RUNTIME_ERROR con uno y con dos runs; ninguno falso en 20 vueltas con
la ventana ensanchada, y contando con el contador del GC, uno falso; el fin, del último que sale en 20 de 20, y
decidiéndolo sin mirar, 20 de 20 al revés). Y el paso 2 (`156db15e`) con `check-sim` 11/11: mil RUN seguidos
colgándose en la cola permanente (2,6 s), el KILL que despierta (11 ms) y su sabotaje, y el interruptor
`--run-etapa1`. Compilan la Pico 2, el S3 y la Nucleo. Lo que se vio por el camino: con el sabotaje del contador,
detrás del interbloqueo falso el programa se descuelga con un run todavía dentro de su cuanto, que al despertar ya no lo
encuentra —un fallo de segmento—: es lo que evita «nadie tiene un hilo entre manos».

**La revisión adversarial** (4-oct, un agente con el código; lo que pesa, comprobado a mano): en lo que corre hoy —un run
en las placas, los runs del PC (que se unen antes de destruir la VM) y `bpvm_run`— no encontró ningún fallo. Con dos
runs PERMANENTES (la 2b), dos críticos, ARREGLADOS ya porque son del protocolo de la cola: (1) «el último que sale»
sólo contaba los runs con un hilo entre manos, y otro run sin hilo —dormido en la cola, o entre dos vueltas— todavía
podía mirar la VM después del aviso, ya destruida (y si la siguiente caía en la misma dirección, tomarla por la suya);
(2) nada limitaba cuántos runs entraban en un programa publicado para uno. El arreglo: la cola cuenta los runs DENTRO
del programa (`dentro`: de cogerlo a salir de él); no entran más de los publicados, y el que lo cierra espera a ser el
único dentro antes de descolgarlo y avisar. Su prueba, la parte 4 de `test-cola` (al avisar, nadie dentro; 20 de 20),
con su sabotaje (el que cierra no espera: 20 de 20 con otro run dentro). Y los bajos, también: colgar con la cola
ocupada, o para runs permanentes sin ninguno, dice el porqué en el EXITED (antes, «RuntimeError no atrapado» y el
motivo sólo en el log); y sin memoria para `io` el programa vuelve a correr en la tarea del REPL, como antes de la 2a.2
(la pila de `run` no está medida para el despachador del cable). Lo demás que señaló es de dos runs y va a la lista de
la 2b (abajo). Una nota que se queda: la preparación corre en `io` con sus colas ya puestas y su lazo parado —si
escribiera en la salida del programa y la llenara, `io` se esperaría a sí mismo—; hoy no escribe nada (el enlace no
emite, los avisos son del cargador, que va antes), y así tiene que seguir.

**La puerta en la Pico 2** (4-oct, con el OK de Eduardo; la imagen de `3669215d`): la tanda de V7, verde (34 idénticas,
las 3 de KILL paradas, `synclisttest` con sus aserciones y `MachineId` sin petar; las 3 de GUI, saltadas: no tiene
pantalla); `run_placa` 34 de 34 (el depurador, con su lazo de pausa en `run`; el KILL en pausa y corriendo; el RESET);
las marcas, con margen: a `run` le sobran 13,8 KB de 16, a la tarea del REPL 9, y al heap de FreeRTOS 6,4 KB en el peor
momento (la cola es estática: la VM, 176 B menos). **PrintBench**: la mediana de 10, 887 y 900 ms en dos tandas
seguidas, frente a los 836 de la puerta de la etapa 1 (+6–8 %); el tramo sin print —el del intérprete— igual (14 ms), y
en la 2a.2 nada toca el camino de la salida (la cola de `io` y su lazo no han cambiado; la vuelta de cada cuanto, sin
cerrojo, es la del simple). Aquellos 836 se midieron por la mañana, con otra imagen y otras placas en el USB.
**Decidido por Eduardo** (4-oct, a la pregunta de si hacer ya una A/B grabando dos veces más): seguir con la
resistencia; la A/B —la imagen de antes de la 2a.2 y la de ahora, en la misma sesión— va al lote del final de H12.
**La resistencia**: 110 minutos (el límite de la herramienta), 3 271 vueltas SIN UN FALLO —cada una, las 20 líneas en
orden, un bucle que no cede parado con un KILL a un instante al azar, la salida tardía y, nuevo de la 2a.2, un programa
dormido una hora parado con un KILL: 5 ms de mediana y 16 como mucho del KILL al EXITED—, con 81 RESET con un programa
corriendo; las marcas, quietas de principio a fin (`run`, 13,5 KB libres; la tarea del REPL, 9; el heap de FreeRTOS,
6,4 KB en el peor momento). **La puerta de la 2a.2 en la Pico 2: PASADA**, con PrintBench pendiente de la A/B.

### Etapa 2b — el 2º run, con UN núcleo: el MAPA (4-oct, noche), para el diseño

Un agente recorrió el código con la lista de requisitos de la 2b (arriba) y lo que añadió la revisión de la 2a.2; lo
que pesa, comprobado a mano. Lo que es de hoy ya se arregló aparte (`#653`, el log sin cerrojo). Lo que pide la 2b,
por gravedad:

- **Críticos** (con dos runs, corrupción o cuelgue):
  - el contexto del AOT —el testigo de fallos y el `callctx`— es global en los micros: con dos runs, un `longjmp` a la
    pila de otra tarea. Por tarea con los punteros TLS de FreeRTOS: la Pico, `configNUM_THREAD_LOCAL_STORAGE_POINTERS`
    a 1; las ESP32, a 2 (el índice 0 es de pthreads y lwIP); la STM32 y el PC, como están. Lo necesitan cada run, cada
    ejecutor de natives y la tarea del REPL cuando corre ella el programa;
  - la variable de condición de FreeRTOS (sus tres copias) pierde avisos con dos esperando, y no basta un semáforo
    contador (un tercero que llega después de un aviso se lleva la ficha de otro): una lista de esperadores, cada uno
    con su notificación de tarea en un índice propio, con plazo y sin fichas sueltas; UNA implementación para las
    tres. La necesita cualquier imagen con `vm.runs=2`, aunque el programa vaya con un run (el run ocioso espera en la
    misma condición);
  - el `gc()` del programa no para el mundo (sólo coge el cerrojo): con el otro run en marcha, colecta sobre una pila
    rancia;
  - UNA NATIVE APARCADA por el mundo parado deja su pila de C sin escanear: una reserva desde una native que encuentra
    a otro run colectando espera DENTRO de la native, y el GC sólo mira la pila del colector y las natives
    suspendidas (el comentario de `heap.c` que dice «nunca a mitad de un native» es falso). Pasa ya en el PC con
    `--smp=2` y AOT, que ninguna prueba combina;
  - el depósito de ejecutores de natives va sin cerrojo;
- **Altos**: los eventos (encolar bajo el cerrojo; resucitar sólo a un hilo que no tenga run; el `join` que cuente
  los eventos que su hilo debe); el puente native→BP sin ejecutor, que con el mundo parado da vueltas sin fin; la GUI
  (LVGL y su modelo no son reentrantes); SQLite del pack, compilado sin hilos (la Metro).
- **Medios y bajos**: la zona de rascar compartida (`listDir`, el `npShow`, los verbos del cable); el registro de
  ficheros abiertos; el árbitro del bus (lo arregla la condición); el número de cada run; el RUN publicando con tantos
  runs como permanentes; `vm.runs` leído en el arranque (`bpvm_boot2_claves`); y el coste: con dos runs cada cuanto
  coge el cerrojo y avisa al otro —hay que medir un programa de un hilo—.

**El orden propuesto**, cada paso con su puerta: (0) un arnés en el PC con el kernel de FreeRTOS de verdad (su port de
Windows) para la condición —la de hoy tiene que salir ROJA: si no, el arnés no prueba nada—; (1) la condición nueva;
(2) el contexto del AOT por tarea —la prueba, en el PC compilado SIN la TLS de hoy—; (3) los arreglos del núcleo, cada
uno con su prueba y su sabotaje (el `gc()`, los eventos, los ejecutores, la native aparcada, el puente, la zona de
rascar, los ficheros), y un lote en la Pico 2 con `vm.runs=1` (la puerta de la 2a.2 otra vez); (4) los dos runs
permanentes en el simulado; (5) la Pico 2 con `vm.runs=2`: la tanda, `run_placa`, el estrés con dos runs, el aviso
perdido con su imagen de control, PrintBench de un programa de un hilo, las pilas y dos horas de resistencia.

**Se le propusieron cuatro decisiones** (la GUI y SQLite con un run, la zona de rascar, la pila del segundo run) y
**Eduardo cambió el planteamiento** (5-oct, 00:15): *«El AOT de momento lo dejamos pendiente […] de momento hemos
pospuesto los natives, así que este problema queda aparcado temporalmente […] Ponemos el 2º run, conseguimos que
funcione con lo mínimo y vamos ampliando la cobertura. De momento solamente 2 run ejecutando código BP normal, sin AOT
ni LVGL.»* Y preguntó qué querían decir dos de los críticos —explicados con un ejemplo: la condición (pieza nuestra
sobre FreeRTOS: una caja de un aviso; con dos runs dormidos y un KILL, despierta uno, el otro no, y el programa no
acaba) y el `gc()` (coge el cerrojo pero no para al otro run, que sigue ejecutando con un objeto nuevo que el GC no
ve)—.

**La 2b mínima** (el alcance de Eduardo): dos runs ejecutando código BP normal.
1. el segundo run: su tarea, su número, y `vm.runs` en el ENV (la vuelta atrás);
2. la condición con un aviso por run (las notificaciones de tarea de FreeRTOS); sin ella, cuelgues;
3. el `gc()` del programa con el mundo parado;
4. lo que comparte el código BP normal y hoy nada protege, porque con un run nunca coinciden dos hilos dentro de un
   builtin: los eventos (`raise`), la zona de rascar de `listDir` y el registro de ficheros abiertos;
5. mientras tanto, un programa que carga natives o la GUI va con UN run aunque haya dos (que no se rompa nada).
Aparcado (AOT y natives, LVGL, SQLite): el contexto del AOT por tarea, la native aparcada por el mundo parado, el
depósito de ejecutores, el puente native→BP. El orden: el micro simulado con dos runs permanentes primero; la Pico 2
después (la condición de FreeRTOS sólo se puede probar allí: la de pthreads no pierde avisos).

**El GC con dos runs** (5-oct, con Eduardo). Propuso una tarea GC en la rueda —IO, Run1, Run2, GC— que limpie cuando
los runs no corren, y que los bloquee hasta acabar si `io` deja el turno pronto. Lo que se le explicó: que un run no
corra no basta —FreeRTOS lo expropia en cualquier punto, cada tick, también a mitad de crear un objeto con su
referencia aún en un registro, donde el GC no mira—; tiene que estar parado en un PUNTO SEGURO (entre dos
instrucciones BP), y eso es el «mundo parado» que ya existe (el colector levanta la bandera, cada run la ve en su
siguiente punto seguro y espera; el colector limpia y la baja). A *«Pero si tenemos 2 runs, ¿tenemos 2 GC? Eso no
puede ser»*: no —un heap y una colecta a la vez; la hace el run que se queda sin memoria, y si el otro también se
queda, no empieza otra: espera a que acabe y reintenta (`gc_stw`, «UN SOLO COLECTOR», probado con dos y cuatro runs en
el PC)—. **Decidido** (Eduardo: *«OK»*): para el mínimo, el GC de hoy —uno, con el mundo parado—, con el `gc()` del
programa por el mismo camino; la tarea GC de su esquema (un GC visible como tarea, que además podría limpiar en los
ratos en que los dos runs ya esperan), como mejora después.

#### La 2b mínima, en el PC (5-oct)

Lo de la lista, hecho y probado en el PC, cada arreglo con su prueba y su sabotaje (en rojo):
- **El 2º run**: `vm.runs` en el ENV (1..2; `bpvm_boot2_claves`, antes de la memoria y de la cintura); en el micro
  simulado, `--runs=N`; el número de cada run (su marca en los hilos que tiene entre manos). Un programa con natives,
  la GUI, SQLite o el depurador va con UNO aunque haya dos, y lo dice el log del RUN.
- **El `gc()` del programa**, por `gc_stw`: el mundo parado. `test-cola` 5 (la regla: ninguna colecta con el otro run
  dentro de su cuanto; con el camino de antes, miles).
- **Los eventos** (`raise` encola bajo el cerrojo; no se resucita un hilo que otro run tiene entre manos; el `join`
  cuenta los eventos que su hilo debe): `test-cola` 6 y 7.
- **La zona de rascar**: mirar si está libre y quedársela, de una vez (con el cerrojo de la cola, que es permanente).
- **Los ficheros abiertos**: la regla de siempre —para escribir, uno a la vez; para leer, los que quieran (Eduardo: *«no
  se podía abrir el mismo fichero 2 veces en modo escritura, pero en modo lectura sí»*)— no cambia; lo que rompían dos
  runs era CUMPLIRLA: los dos miraban «¿ya está abierto?» a la vez, lo veían libre y lo abrían los dos. Mirar, abrir y
  apuntarlo es ahora un paso, bajo el cerrojo. `test-cola` 8 (`FicherosDosRuns`: dos escritores nunca a la vez, dos
  lectores siempre; sin el cerrojo, «a la vez: 2»).
- **La reserva de emergencia (#355)**: quién podía gastarla lo decía una bandera DE LA VM (`building_error`), puesta
  mientras se construía una excepción: con dos runs la veía el otro —una reserva cualquiera suya se llevaba la del error,
  y al acabar uno la bajaba con el otro aún construyendo—. Ahora lo dice cada reserva (`bpvm_heap_nuevo_error`,
  `bpvm_heap_alloc_string_error`): nada compartido. No había NINGUNA prueba que la gastara: `test-reserva`.
- **La condición de FreeRTOS**, UNA para las tres familias (`src/platform_freertos_cond.inc`, que incluyen
  `src/platform_freertos.c` —la STM32—, `pico/platform_freertos.c` y `esp32/common/platform_esp32.c`; fuera 333 líneas
  de copias): una lista de esperadores, cada uno en su pila y con SU notificación de tarea en un índice propio (el 1;
  `configTASK_NOTIFICATION_ARRAY_ENTRIES` 2 en la Pico y la STM32, y en el `sdkconfig` de las cinco ESP). Probada en el PC
  con el KERNEL DE FREERTOS DE VERDAD —su port de Windows, `make test-frcond`—, con la de antes como control, que sale
  roja donde decía el mapa: un `broadcast` con dos esperando despierta a uno («2 esperando, 1 despiertos»), un tercero
  se lleva el aviso de otro, y en el estrés con esperas sin plazo —como esperan los runs— uno se queda DORMIDO. Las tres
  familias compilan con ella (la Pico, la STM32 y la S3: `fr_cond_esperar` en las tres imágenes).
- **La Pico**: el 2º run, `run2`, a la misma prioridad. Su pila (16 KB, como la del primero: el marcado del GC es
  recursivo y corre en el run que se queda sin memoria) sale del TECHO del bloque de la VM sólo con `vm.runs=2` —con la
  VM en la PSRAM, de malloc, que tiene la SRAM—: con un run no cuesta nada (el `.bss` crece 96 B, el TCB). El INFO
  dice los runs EN VIGOR (`runs`, común) y la marca de la pila del 2º (`run2StackFreeBytes`).

Las tandas contra el micro simulado con `--runs=2`: la V7 (42 pruebas) igual que con uno —todo IDÉNTICO salvo lo que
tampoco lo es con uno (`GuiShot`: el simulado sin GUI no deja la captura) y las de AOT, que el simulado salta—, con los
cuantos repartidos entre los dos runs (115 659 y 81 229 en la más larga); `run_tarea_smoke` con el caso K.

#### La revisión adversarial de la 2b (5-oct): lo que comparte el código BP normal con dos runs

Tres frentes en paralelo —el núcleo; los builtins y el FS; la HAL y la plataforma—, con la pregunta: ¿qué estado
compartido toca el código BP normal sin cerrojo, donde dos runs a la vez corrompen, cuelgan, pierden algo o dan otro
resultado que con uno? Antes de la 2b nada de esto se veía: con un run, un builtin entero —y cada instrucción— va de
una vez. Cada hallazgo, comprobado a mano; los de la 2b, arreglados con su prueba (`test-cola`, las partes 9 a 15) y su
sabotaje, rojo:
- **Un `owner` soltado dos veces** (parte 9, `OwnerCarrera`). Dos hilos que asignan a la vez el MISMO campo owner —el
  propietario es uno; los hilos, dos, sin Mutex: la carrera es del programa, pero la VM no puede romperse— leían el
  mismo objeto, los dos lo veían vivo y los dos lo soltaban: el bloque dos veces en la lista de libres, que se apuntaba
  a sí misma (la reserva siguiente, dando vueltas con el cerrojo cogido: el sabotaje, el proceso colgado). Eduardo
  preguntó cómo, si el GC es uno y liberar pasa por un método único: la 2ª liberación llega detrás, sí, pero MIRÓ si
  vivía antes, fuera del cerrojo. Ahora suelta quien MATA el handle (`bpvm_handle_kill` dice si lo mató él: mirar y
  matar van juntos), y soltar un bloque ya libre no hace nada. Y con dos runs el hueco no vuelve a la lista hasta el
  siguiente GC: otro hilo puede estar a mitad de una instrucción sobre el objeto —miró que vivía y aún no ha escrito— y
  el hueco, ya devuelto, podía ser de un objeto nuevo. El objeto muere en el acto («objeto eliminado» para quien lo use
  después); sólo su memoria espera. Con un run, como siempre.
- **Un `print` que no cabe en la cola de salida** (parte 10, `LineasLargas`: 8192 caracteres; la cola, 4 KB en el PC y
  1 KB en la Pico): entra a trozos soltando el cerrojo, y el `print` del otro hilo se colaba entre dos (#600). Ahora,
  quien escribe a trozos marca la cola; los demás esperan a que acabe.
- **El `join` y el despertar dan por acabado un hilo que su run aún no ha soltado** (parte 11, `JoinLinea`): el hilo
  queda TERMINATED antes de soltar su línea a medias, y «finN» salía antes que «aN». La regla de la resurrección,
  general: lo que otro run tiene entre manos no se toca ni se da por acabado. Cubre también el `sleep` (la hora y el
  estado en dos escrituras sin cerrojo).
- **La zona de rascar, ocupada por el otro run** (parte 12, `ListDirDos`): un `listDir` correcto lanzaba «la zona de
  rascar está ocupada». Ahora se espera, y sin contarse como run en marcha —el otro la tiene mientras reserva memoria y
  puede necesitar parar el mundo—; el `show` del NeoPixel, igual (se saltaba la trama en silencio).
- **Un `gc()` tras un mundo parado ajeno** (parte 13, `ColectaAjena`): parar el mundo no es siempre colectar (lo para
  `Heap.compact()` —que con otro hilo vivo mira y no compacta— y la tabla de handles al crecer); quien llegaba en ese
  momento volvía sin colecta: un `gc()` que no colectaba, y una reserva con «No space in heap» sobre un heap lleno de
  basura. Ahora, si la cuenta de colectas no se movió mientras esperaba, colecta él.
- **El error del programa, el primero** (parte 14, `ErrorPrimero`): cada `throw` escribía el texto del EXITED, también
  los atrapados; un hilo moría sin atrapar (#608), otro atrapaba algo después, y el EXITED decía el de éste. Ya pasaba
  con UN run. Ahora lo fija el primero que lo es, bajo el cerrojo.
- **Los ficheros**: cerrar sacaba el fichero de la lista y lo cerraba en el motor FUERA del cerrojo (lo había hecho yo
  esa mañana): en la ventana, otro run lo abría para escribir —en la SD, FatFs libera sus clusters y el cierre del
  primero escribía en ellos: la tarjeta, corrompida—. Ahora mirar, sacarlo y cerrarlo van juntos, y cada operación del
  cursor (leer, escribir, `seek`, `flush`, `readLine` en dos tramos) mira si está abierto y hace lo suyo bajo el cerrojo
  de los File (parte 15, `FileCompartido`: un hilo escribe y otro cierra; con el sabotaje, el proceso caído —un motor ya
  cerrado—). Y `parseFloat`: el `strtod` de newlib, que en la Pico reparte sus números grandes desde un estado único para
  todas las tareas, bajo el cerrojo.
- **Builtins que leen un array del programa en dos pasadas** (`__charsToString`, `__i2cExec`, `__spiDevExec`): con otro
  hilo cambiándolo entre medias (un StringBuilder o una Transaction compartidos), escribían fuera del bloque, de la pila
  de C o de tx/rx. Ahora: no más allá de lo reservado; cada operación leída una vez; o vuelta a mirar contra lo validado
  (si cambió, «la transacción cambió mientras se ejecutaba»). Por construcción: provocarlo pide un hilo dentro del
  builtin, entre sus dos pasadas.
- **La HAL** (por construcción; la puerta en la placa lo ejercita): reclamar un pin, abrir o soltar un bus —la cuenta y
  el hardware, juntos—, iniciar una UART, una tira o un canal del ADC, y leer el ADC —elegir el canal y convertir: el
  conversor es uno, y un hilo leía el canal del otro—, las migas y el reloj (64 bits en una de 32), bajo un cerrojo de
  la HAL (el suyo, no el de la VM: la HAL escribe trazas en la salida, la salida puede esperar a `io`, e `io` coge el de
  la VM para avisar de un KILL). Sacar del anillo de la UART, bajo el cerrojo común. El árbitro de los buses y la memoria
  de la fachada SPI, creados con un solo hilo vivo.
- **Lo que se queda en fichas** (anterior a la 2b, o sólo diagnóstico): `#654` (el `STAT` con `base` cambia el proyecto
  en mitad de un RUN), `#655` (un hilo resucitado sin evento que inyectar), `#656` (soltar un `owner`, recursivo),
  `#657` (los builtins de fichero entero no miran los abiertos), `#658` (el volcado del log a la flash) y `#659` (el
  diagnóstico que dos runs mezclan).
- **Por un fallo de la red, mío**: la zona de rascar con el cerrojo de la cola hacía depender `bpvm_util.c` de la VM, y
  ese fichero vive aparte para que las herramientas lo enlacen sin ella (`test-bmgr` no enlazaba). El cerrojo lo
  registra ahora la VM con un gancho, como el canal de diagnóstico.

#### La puerta de la 2b en la Pico 2 (5-oct, con el OK de Eduardo para grabar y para el ENV)

La imagen de `4a608d49`; con `vm.runs=1` y luego con `vm.runs=2` (el ENV, y un RESET: lo lee el arranque):

| | un run | dos runs |
|---|---|---|
| tanda V7 | verde (el informe, `compat/informes/tanda_h12_2b.md`) | verde, el mismo resultado |
| `run_placa` | 34 de 34 | 34 de 34 |
| PrintBench (mediana de 10) | 774 ms | 779 ms (+0,6 %) |
| `fib(26)` interpretado | 3 178 ms (siete veces, todas iguales; medido después, con el ENV vuelto a un run) | 3 411 ms (±1 ms): **+7,3 %**, `#662` |
| el heap de la VM (el INFO) | 240 112 B | 223 728 B: los 16 KB de la pila del 2º run salen del bloque |

El arranque lo dice en el log («la pila del 2o run, 16 KB del techo del bloque @ 0x2007a000», «el 2o run, creado: dos
runs en la cola»), y el INFO, `runs: 2`. PrintBench baja de los 887–900 ms de la puerta de la 2a.2 a 774: con esta
imagen, la A/B pendiente se resuelve sola (lo más probable, la condición nueva: notificaciones de tarea en vez de un
semáforo y su guarda en el camino de la salida; sin medir aparte). La resistencia con dos runs (`resistencia_run.py
--runs=2`, con `RunCompartido`: un contador bajo un Mutex de dos hilos y basura para el GC), 110 minutos: **SIN
FALLOS** —1 614 vueltas, 40 RESET y 161 INFO; el KILL a un programa dormido, 6 ms de mediana y 33 de máximo; las
pilas, sin moverse (a `run` y a `run2` les sobran 12,5 KB de 16; a `io` y a la tarea de la VM, 8 960 B; el heap de
FreeRTOS, 6 752 B libres en el peor momento)—. (El «mínimo −1» del resumen del script es el INFO justo tras un RESET,
sin RUN todavía: no es una marca.)

## Etapa 3 — el 2º núcleo: el MAPA (5-oct, para el diseño)

Eduardo, al pasar la puerta de la 2b: *«Bien, 2º núcleo, puedes empezar a analizar»*, y *«Lo de la fecha es orientativo
no una meta. Lo importante es que sea fiable.»* Tres lecturas en paralelo —lo decidido y medido; la plataforma (FreeRTOS
de dos núcleos en la RP2350, la flash, el USB, las interrupciones); el orden de memoria entre dos núcleos de verdad, que
el x86 del PC esconde—, y lo que pesa, comprobado a mano.

**Lo que nadie recordaba**: la ÚNICA vez que se encendió el 2º núcleo de la Pico (29-may, `#153`, `SMP_ARCH.md`
§«Dual-core: DIFERIDO A v2») el firmware NO ARRANCÓ: el núcleo 0, colgado DENTRO de `vTaskStartScheduler` esperando a
que el 1 contestase —el LED fijo, el USB sin enumerar—; sin sonda SWD no se vio por qué, se aplazó y no se ha vuelto a
intentar (`#153` no tiene ficha). Esa causa sigue sin saberse. Y hoy habría OTRA, nueva (el descriptor de placa es de
junio) y verificada en el código: lo primero que hace la tarea de la VM, con el planificador ya en marcha, es leer el
tamaño de la flash (`board_desc_early_init` → `detect_flash_bytes`, `pico/board_desc.c:31-40`), que saca la flash de
XIP con sólo las interrupciones del núcleo PROPIO apagadas: el otro núcleo, ejecutando desde la flash, leería basura.
Lo mismo, el arranque de la PSRAM. Los dos, antes del planificador; y el primer paso de la 3.1, un arranque de dos
núcleos INSTRUMENTADO (las migas por núcleo y el perro guardián), para ver dónde se para, si se para.

**Dos agujeros que YA tiene la 2b** (con dos runs en un núcleo; raros, pero de memoria corrompida; con dos núcleos,
frecuentes), y que hay que cerrar ANTES de la etapa 3:
1. **Valores escritos a trozos** (`#660`). La VM escribe una palabra byte a byte (`bpvm_write_u32_be`,
   `include/bpvm_internal.h:814`; en la Pico, a `-Os`, cuatro escrituras de un byte —desensamblado el intérprete del
   firmware—; en el PC el compilador las junta, y por eso no se ve), y una referencia —8 bytes— en dos. Un hilo que
   publica un objeto en un campo y otro que lo mira sin Mutex (una carrera del programa, pero muy corriente: «¿ya está
   el resultado?») puede leer una referencia a medio escribir: con el índice 0,
   `bpvm_ref_dead` la da por viva y `bpref_deref` la resuelve a la dirección 0 —donde la «longitud» de un array son los
   bytes centinela del principio de la memoria: cientos de millones—, y un `ASTORE` pasa la comprobación de límites y
   escribe en cualquier sitio. Con un run no pasa: el cambio de hilo es entre instrucciones.
2. **Soltar un objeto que el otro run está usando** (`#661`; incompleto lo de esta mañana): con dos runs, el HUECO de un
   `owner` soltado espera al GC, pero su NÚMERO de handle se recicla en el acto. Un run comprueba que el objeto vive,
   otro lo suelta y reserva otro con ese número, y el primero —que vuelve a resolver el handle a mitad de la
   instrucción— escribe en el objeto nuevo (o en la dirección 0, como arriba).
El arreglo, en tres piezas: (a) cada palabra de una vez (escribirla y leerla como palabra, con su orden de bytes; la
precondición, todas las ranuras alineadas —hoy las globales de un segundo módulo pueden no estarlo: su bloque empieza
donde acabó el código del anterior, más 64 (`src/loader.c:313-316` y `:550`); el cargador, en las dos VMs, redondeando;
y dentro del bloque de datos, lo que coloque el compilador, por mirar—); (b) UNA resolución validada por instrucción
(índice dentro de la tabla, dirección no nula, la generación leída DESPUÉS y la misma; si no, «objeto eliminado»), y
la instrucción entera con esa dirección; (c) el
número de handle de un objeto soltado, como su hueco, de vuelta en el GC. Con eso, una referencia desgarrada sólo da
el valor viejo, el nuevo o «objeto eliminado». Pruebas: el desgarro se puede provocar ya en el PC (compilando el
núcleo sin juntar escrituras, `-fno-store-merging`), y soltar mientras otro usa, con la ventana ensanchada; cada una
con su sabotaje. Y la puerta de la 2b, otra vez, en la Pico 2.

**Lo que la etapa 3.1 necesita** (el RTOS de dos núcleos con UN run; verificado en el código):
- el arranque: lo que toca la flash fuera de su cintura (el sondeo, el ENV, la PSRAM), antes del planificador; y el
  arranque instrumentado (las migas por núcleo y el perro guardián) para no ir a ciegas;
- **el candado de la flash** (#153): hoy NADIE se registra como víctima (sólo un hilo anclado a un núcleo ≠ 0, y no hay
  ninguno), así que la primera escritura de la flash esperaría a una víctima que no existe; la ventana del SDK no es
  atómica entre tareas (suelta su mutex al aparcar al otro núcleo); los dos núcleos como víctimas, y un mutex nuestro
  que cubra la ventana entera;
- **el log**: su sección crítica sólo apaga las interrupciones del núcleo propio; un spinlock (y dentro, `#658`);
- los caminos fatales (los ganchos de FreeRTOS corren con sus cerrojos cogidos: volcar el log ahí es un interbloqueo
  mudo);
- cerrar un ejecutor de natives (hoy se libera su pila con la tarea quizá aún corriendo en el otro núcleo: esperar a
  que esté suspendida);
- **el USB**, NO (lo dijo un agente y no se sostiene): la `stdio` del SDK lo protege con su mutex —se lee y se escribe
  desde cualquier núcleo; sólo su arranque, en el núcleo de la alarma, el 0 (`stdio_usb.c:102-163`, `:195`)— y el
  firmware no llama a TinyUSB por su cuenta (mirado); lo que queda del USB es lo de `A4`, abajo;
- el MPU de `mpu=1` es por núcleo; y las banderas que cruzan núcleos (`kill_requested`, `fin`, el anillo de la UART),
  con su orden explícito;
- y lo que dejó dicho `A4` (`FICHAS`, el inventario del 5-sep, §3 «Lo que falta ANTES de activar el segundo núcleo»):
  `kill_requested`, ya en la lista; y el USB de la Pico con un solo escritor —el diagnóstico de la VM ya va al log
  (`#353`); lo que quede escribiendo en `stdout` fuera del cerrojo del cable, por censar—.
(Lo que NO falta: `malloc` ya va con mutex —el puerto RP2350 de FreeRTOS enlaza `pico_multicore` y pone
`PICO_USE_MALLOC_MUTEX`—, y los cerrojos del SDK cooperan con FreeRTOS, `configSUPPORT_PICO_SYNC_INTEROP`.)
**Y la 3.2** (los dos runs en dos núcleos): la 2b ya hecha, con las tres piezas de arriba; y el orden de memoria, que
en la M33 de la RP2350 (en orden y sin caché de datos) es sobre todo formal, en la P4 (RISC-V) real.

**Lo que ya está decidido, y no se pregunta**: nada anclado —`io`, `run1` y `run2`, que los núcleos van tomando SIN
anclar (Eduardo, 1-oct)—; las etapas intermedias, sólo en la Pico 2, y la Metro (lo único que prueba ella: la PSRAM
con el candado de la flash), en el lote del final (4-oct); para el otro núcleo al escribir la flash, ~~el
`multicore_lockout` del SDK con un mutex nuestro (el `flash_safe_execute` crea una tarea por operación: más piezas
para lo mismo)~~ —**NO**: la puerta de la 3.1 lo cazó (5-oct, tarde): aparcar al otro núcleo por interrupción se
interbloquea con FreeRTOS SMP, y no era «lo mismo»: el SDK aparca con una tarea; abajo, «La etapa 3.1», el paso 2—; y
un fallo fatal en un núcleo para la placa ENTERA —el otro núcleo, aparcado—, con el log en la flash, como hoy con uno
(el LED parpadeando; no se reinicia) —también cambiado: «fatal error, el log y reset», el paso 4—.

**Para Eduardo** (con su mecanismo y un ejemplo): (1) **el modelo de memoria de BP ante una carrera** (un programa con
un campo que un hilo escribe y otro lee sin Mutex). Con las piezas de arriba, (a) el lector obtiene la referencia
vieja, la nueva o «objeto eliminado» —una excepción en un programa con una carrera; la VM, intacta—, y si DOS hilos
escriben a la vez el mismo campo, el campo puede quedarse «eliminado» (media de cada uno); o (b) como en Java, siempre
entera, la vieja o la nueva, y de dos escritores gana el último: pide un cerrojo cortito al ESCRIBIR una referencia en
el heap o en una global (leer, sin cerrojo: en orden y validada), con su coste, a medir. En los `long` y `double`, en
los dos casos como Java: sin Mutex, pueden salir a medias (un valor que nadie escribió; la VM, intacta). (2) **La vuelta
atrás de la 3**: sólo la imagen de un núcleo, o además un interruptor en el ENV (`vm.cores=1`: la imagen de dos núcleos
con todo anclado al 0). (3) **¿Hay sonda SWD** para la Pico 2? (la de mayo se aplazó por no tenerla; vale otra Pico con
`debugprobe`).

**El orden propuesto**: (1) las tres piezas de la 2b (`#660` y `#661`), en el PC y con su prueba, y la puerta de la 2b
otra vez en la Pico 2; (2) la 3.1, paso a paso, empezando por el arranque instrumentado; (3) la 3.2. Cada paso, con su
puerta.

#### El turno: el cambio de run, sólo entre dos pasos (Eduardo, 5-oct; por decidir)

Eduardo, al leer `#660` y `#661`: *«el cambio de contexto de los runs, ¿no se puede aplazar hasta que se termina un paso
completo (un paso es la interpretación de un opCode)? Y de esta forma nos aseguramos que las cosas terminan de una forma
controlada.»*

**Con un núcleo, sí.** Hoy FreeRTOS cambia de `run` a `run2` en cualquier tick, a mitad de un paso: los dos (y la tarea
del REPL, que hace de `io`) tienen la MISMA prioridad, a propósito (`pico/repl_v1.c:833-834`: por debajo, el KILL no
llegaba; por encima, PrintBench iba más lento), y el reparto por tiempo está puesto (`configUSE_TIME_SLICING`, por
defecto). Quitar el reparto no sirve: castigaría a `io`. La herramienta es **el turno**: un cerrojo justo (por orden de
llegada) que sólo se disputan los runs. Un run lo coge antes de elegir un hilo y ejecutar su cuanto (`src/scheduler.c`,
`correr_programa`: 1 024 pasos, o hasta que el hilo cede o se bloquea) y lo suelta al acabarlo, y antes de dormirse en
la cola. Si FreeRTOS le da la CPU al otro run a mitad de un paso, el otro está esperando el turno y no puede entrar:
el paso acaba entero. `io` y las demás tareas siguen como hoy (no lo cogen).

**Lo que arregla, por construcción**: `#660` y `#661` (nadie ve un paso a medias del otro run: ni una palabra a
trozos ni un handle reciclado a mitad); la pregunta del modelo de memoria (cada paso, entero: también los `long` y los
`double`; lo que no es atómico es lo de siempre, varios pasos —`x := x + 1`—, que ya pide un Mutex con un run); y el
mundo parado del GC (quien colecta tiene el turno: el otro run está entre dos pasos, sin nada a medias, o dormido). Y
no añade interbloqueos: el run que espera el turno está entre dos pasos y no tiene ningún otro cerrojo; la regla, el
turno es el cerrojo de FUERA (nunca se coge teniendo otro). Coste: coger y soltar una vez por cuanto, no por paso.

**El precio, con dos núcleos**: ya no hay un «cambio» que aplazar —los dos runs corren a la vez—, y el turno hace que
se turnen: sólo un run ejecuta pasos en cada momento (como el GIL de Python). Dos hilos que calculan tardan con dos
núcleos lo mismo que con uno. El 2º núcleo sí da: `io` en paralelo (la salida, el cable: la 3.1, con o sin turno) y un
run que ESPERA dentro de un paso (la cola de salida llena, la SD, el bus) sin parar al otro, si esa espera suelta el
turno —una lista corta y cerrada; cada una deja lo suyo completo antes de soltarlo y vuelve a resolver sus referencias
al cogerlo—. Sin el turno (las tres piezas de arriba), dos hilos que calculan podrían tardar casi la mitad, sin medir.

**La prueba**: un contador «dentro de un paso», por run: con el turno, nunca dos a la vez; el sabotaje (sin turno),
dos. Las pruebas de la 2b de hoy (`test-cola` 9 a 15) siguen corriendo SIN el turno (un interruptor de prueba), para
que los arreglos finos sigan probados.

**La propuesta**: el turno para V7 —la 2b, segura por construcción; y la 3 con él—; correr en paralelo de verdad (las
tres piezas y el modelo de memoria), en otra versión, medido contra el turno.

#### Seguir pensando: el 2º núcleo es para ir más rápido (Eduardo, 5-oct)

Eduardo, a lo del turno: *«En principio el 2º núcleo es sobre todo para ejecutar los programas más rápido, si al final
no lo conseguimos es poco progreso así que merece la pena pensar un poco. Como idea, ¿qué pasaría si bloqueamos la
interrupción de cambio de contexto (entiendo que es la de un timer de la CPU)?»*

**Bloquear el tick** (sí: el temporizador SysTick, a 1 kHz; cada núcleo tiene el suyo). Con UN núcleo, bloquearlo
durante un paso quita el cambio a mitad de paso POR TIEMPO, pero no el que provoca una espera dentro del paso (un
builtin que espera la SD, la cola de salida llena, un cerrojo): ahí FreeRTOS cambia de tarea igual, y el otro run
entra a mitad. Y un paso de más de 1 ms (un builtin largo) pierde ticks: el reloj de FreeRTOS se atrasa. Es un turno
peor. Con DOS núcleos no sirve de nada: no hay cambio que bloquear —los dos runs ejecutan a la vez—; el núcleo 0
escribe una referencia byte a byte mientras el 1, en esos mismos nanosegundos, la lee. No interviene ninguna
interrupción: coinciden.

**Lo que sí da de sí la idea del paso entero, con dos núcleos**: aplicarla sólo a los pasos que tocan lo COMPARTIDO.
Un paso que sólo toca la pila de su hilo —la aritmética, las locales, los saltos, `CALL`/`RET`, `TRY_*`— no puede
estorbar a nadie: nadie más mira esa pila (salvo el GC, con el mundo parado). Los que tocan lo de todos —objetos,
arrays, globales, `NEW_*`, `INVOKE_*`/`INSTANCEOF`/`CHECKCAST` (leen la clase del objeto), `THROW`, `PRINT*`,
`CALL_BUILTIN`, `FREE_REF`— van enteros, con un cerrojo cortito, uno a la vez. Así: los privados, en paralelo y sin
cerrojo; los compartidos, como con el turno. `fib(26)` en dos hilos: todo privado, en paralelo; un bucle que suma un
array: el `ALOAD`, de uno en uno, lo demás en paralelo.

Por qué es seguro, y verificable: lo compartido sólo lo toca un paso a la vez, entero (el argumento del turno), y lo
privado, un solo hilo. La clasificación no se fía al ojo: en el build de pruebas del PC, cada acceso al heap o a una
global comprueba que el run tiene el cerrojo; un opcode mal clasificado es un error en la red. Y de regalo, lo que se
prueba en el PC vale en la placa: como todo lo compartido va bajo el cerrojo (con sus barreras), ni las escrituras a
trozos ni el orden de memoria de ARM/RISC-V importan, que es justo lo que el x86 esconde.

Lo delicado, para el diseño: el coste del cerrojo en cada paso compartido (a medir: en la RP2350, con los spinlocks del
SIO o las exclusivas de la M33); el GC (un run que espera el cerrojo cuenta como parado, con su `sp` guardado, o el
mundo no se para nunca); los builtins que esperan dentro del paso (con el cerrojo, el otro run sigue con lo privado y
se para en su primer paso compartido; afinar después, uno a uno); y lo que un paso privado pueda tocar sin parecerlo
(contadores del perfilador, la tabla de enlaces si se rellenara al vuelo: por mirar). Lo primero, MEDIR en el PC qué
parte de los pasos es privada en nuestros programas (fib, PrintBench, AllocBench, SyncListTest, la tanda): es el
techo de lo que ganaría, antes de escribir nada.

#### La medida (5-oct; Eduardo: *«a mí me gusta medir antes de hacer cambios así que adelante»*)

Un experimento en el scratchpad, sin tocar el repo: la VM-C del PC con un contador por opcode (una copia de
`interp.c`; los demás objetos, los del build), y un programa de dos hilos, `ParaleloBench` (cuatro cargas, cada una dos
veces seguidas en el main y en dos hilos a la vez), corrido con un run y con dos (`--smp=2`: dos runs en dos núcleos
del PC, el diseño de la 2b de hoy, sin cerrojo en los pasos compartidos). El recuento vale para cualquier máquina (el
mismo intérprete y el mismo bytecode); los tiempos son del PC, no de la Pico. (Los ciclos por opcode con `rdtsc` no
sirven: leer el reloj cuesta lo que un paso barato.)

**1. Qué parte de los pasos es privada** (por recuento; los 12 benchmarks de `samples/benchmarks` y 47 programas de la
tanda): el cálculo puro, el 100 % (`fibobench`, `intbench`, `floatbench`, `Bench`, los `FloatBench`, `DblNat`); los
arrays, del 86 al 95 % (`arraybench` 85,8, `sortbench` 87,0, `sumbench` 93,3, `BenchMem` 95,6); lo de objetos, GUI y
JSON, del 80 al 88 % (`synclisttest` 81,2, `JsonDemo` 82,4, `GuiArbolG2` 83,0, `GcHondo` 87,9); con cadenas y
builtins, del 80 al 94 % (`stringbench` 93,5 con un 6,5 % de builtins, `AllocBench` 83,0); y lo que imprime mucho, menos
(`PrintBench` 81,8, `MathRango` 55,4). Los compartidos más frecuentes: `ALOAD`, `CALL_BUILTIN`, `INVOKE_VIRTUAL_REF`,
`GET_FIELD`, `ALEN`.

**2. Cuánto gana hoy un programa con dos hilos** (dos runs; con un run, el secuencial y el paralelo tardan lo mismo):

| carga | pasos compartidos | techo del paso entero para lo compartido, `2/(1+s)` | medido hoy, dos runs (cuanto de 16 384) contra un run |
|---|---|---|---|
| cálculo (`fib(27)` ×2) | 0 % | 2,00× | **2,12×** (51 → 24 ms) |
| arrays (sumar 20 000, ×60) | 6,7 % | 1,88× | **1,72×** (105 → 61 ms) |
| objetos (lista de 5 000, ×60) | 14,1 % | 1,75× | **1,73×** (78 → 45 ms) |
| cadenas (`s := s + "x"`, ×2 000) | 13,5 % | 1,76× | **0,61×** (329 → 536 ms: más LENTO) |

El 2º núcleo SÍ hace los programas más rápidos: casi el doble en cálculo, ×1,7 con arrays y objetos. El paso entero
para lo compartido costaría poco: su techo está por encima de lo medido hoy sin cerrojo. Las cadenas no escalan ni con
un run (dos hilos que reservan a la vez tardan 458 ms contra 329 en secuencia): la reserva y el GC son de todos, y eso
es aparte, con cualquier esquema.

**3. Una sorpresa: con dos runs, un programa de UN hilo va un 35 % más lento** (`fib`: 51 → 71 ms), y no es el cálculo:
es lo que se hace al final de cada cuanto, cada 1 024 pasos (el aviso a la cola que despierta al run ocioso,
`src/scheduler.c:394`; el `yield`; el cerrojo). Con cuantos de 16 384 pasos desaparece (50 ms) y el paralelo mejora
(30 → 24 ms). En la Pico 2, medido después (Eduardo: *«puedes medir con runs=1»*): `fib(26)`, 3 178 ms con un run y
3 411 con dos, **+7,3 %** —menos que en el PC, pero está—.

Lo que falta por medir antes de decidir: el coste del cerrojo en cada paso compartido (un prototipo en el PC, con un
run para el coste sin competencia y con dos para la competencia) y, en la etapa 3, lo mismo en la Pico.

**4. Medido: el cerrojo de los pasos compartidos se COME la ganancia** (5-oct, tras el balance de Eduardo: *«lo de los
2 núcleos creo que es cuestión de solucionar problemas que van surgiendo… Así que adelante»*). El prototipo, otra copia
de `interp.c` en el scratchpad: los 67 opcodes compartidos cogen un cerrojo (un spinlock con `__atomic_exchange`; si
mientras espera otro run para el mundo, deshace el paso y sale como en el punto seguro), sólo con varios runs. Dos runs,
cuanto de 16 384, la mediana de cinco (los resultados, bien en todas):

| carga | hoy, sin cerrojo: secuencial / paralelo | con el cerrojo: secuencial / paralelo | tomas del cerrojo, y las que esperaron |
|---|---|---|---|
| cálculo | 51 / 25 ms | 55 / 27 ms | 56, ninguna |
| arrays | 103 / 53 ms | 107 / **127** ms | 4,9 millones, el 13 % |
| objetos | 78 / 43 ms | 84 / **141** ms | 4,9 millones, el 13 % |
| cadenas | 330 / 529 ms | 336 / 490 ms | 0,6 millones, el 8 % |

Con dos hilos que tocan lo compartido, el paralelo con el cerrojo va MÁS LENTO que un solo run (arrays, 127 ms contra
104; objetos, 141 contra 78). El techo de Amdahl de la tabla 2 era falso: no contaba lo que cuesta que el cerrojo
cambie de núcleo —en una CPU con caché, cada vez que el otro núcleo lo coge, la línea del cerrojo viaja de una caché a
otra: unos 15 ns por toma, millones de veces—. Sin competencia, el cerrojo y la comprobación por paso cuestan del 4 al
8 %. En la Pico (sin caché de datos, con los spinlocks del SIO) podría salir distinto; en la P4 (con cachés) y en el
PC, no. **Conclusión**: un cerrojo para todos los pasos compartidos no sirve para ir más rápido; el camino que guarda la
velocidad es el fino —los pasos compartidos en paralelo y sin cerrojo, cada uno seguro por sí mismo: las tres piezas de
`#660` y `#661`—, que en el x86 no cuesta nada y en la M33 abarata la escritura (`REV` + `STR` en vez de cuatro
`STRB`).

#### El camino fino: `#660` y `#661`, el diseño (5-oct; Eduardo: *«adelante»*)

**El principio**: los pasos compartidos corren en paralelo y sin cerrojo, y cada uno es seguro por sí mismo: resuelve
cada referencia UNA vez, validada; lee y escribe lo compartido palabra a palabra, de una vez; y nada de lo que un paso
tiene entre manos se recicla mientras dura.

- **(a) Palabra a palabra** (`#660`). Los accesos de siempre de la VM (`bpvm_read_u32_be`/`bpvm_write_u32_be` y los
  de 16 bits, `include/bpvm_internal.h:801-828`): si la dirección está alineada, una carga o un almacén de una vez
  (`__atomic_load_n`/`__atomic_store_n`, relajados, con su orden de bytes: en la M33, `REV` + `STR`, más corto que los
  cuatro `STRB` de hoy); si no —los operandos del código, que no se tocan a la vez—, byte a byte como hoy. Sin censo
  de quién lee qué: todo lo alineado, de una vez, también lo que escriben los builtins. Los de 8 bytes (`long`,
  `double`, una referencia), dos palabras. Para que lo compartido esté SIEMPRE alineado: los objetos y los arrays ya lo
  están (`bpvm.c:75`); las globales, desde GC-2 (v3.0.1) en huecos múltiplos de 4 desde CS (`ModWriter.registerSymbol`),
  pero el bloque de un segundo módulo empieza donde acabó el anterior, más 64 (`src/loader.c:550`, tras la subtabla
  de clases de `:478-483`): el cargador lo alineará, en las dos VMs a la vez (la paridad).
- **(b) Ningún handle se recicla en mitad de un paso** (`#661`). Con más de un run, matar un handle (soltar un
  `owner`) sube su generación en el acto —las referencias rancias mueren desde el paso siguiente: el contrato B de
  V4—, pero su DIRECCIÓN sigue en la tabla y su número no vuelve a la lista libre: lo hace el GC, con el mundo parado,
  antes de marcar (el bloque ya esperaba al GC desde la 2b). Así, un paso que resuelve una referencia dos veces (hoy,
  `ALOAD`/`ASTORE`: `bpref_arr_len` y luego `bpref_arr_elem`) obtiene la misma dirección las dos —como mucho, la de un
  objeto recién soltado, cuya memoria nadie reutiliza hasta el GC: inofensivo, como si el paso hubiera ido justo
  antes—; nunca la de otro objeto ni la dirección 0. No hace falta rehacer los opcodes para resolver una sola vez. Con
  un run, como hoy.
- **(c) La validación de una referencia** (`#660`). Con (a), una referencia leída a medias es una mezcla de PALABRAS:
  la generación de un valor y el índice de otro. Las reglas: sin TAG, la palabra alta tiene que ser 0 (null y las
  constantes del bloque de datos); con TAG, la generación de la referencia tiene que ser la del número en la tabla. Hoy
  `bpvm_ref_dead` da por viva una referencia sin TAG aunque su palabra alta no sea 0: es el agujero de `#660` (la
  dirección 0). Con la regla, rechazada.
- **(d) Las generaciones, de un contador COMÚN** (el hueco que quedaba). Hoy cada número lleva su contador (0, 1, 2…),
  y con números tan pequeños DOS hilos que escriben a la vez el mismo campo —una carrera del programa— pueden dejar en
  él, para siempre, la generación de uno y el índice del otro; hoy eso da «objeto eliminado», pero cuando el GC recicle
  ese número y su contador llegue a esa generación, la mezcla apuntaría a un objeto que no tiene nada que ver. Con un
  contador común (cada alta y cada baja toman el siguiente valor), una generación no se repite en dos números: la
  mezcla no puede coincidir nunca —salvo que el contador de 32 bits dé la vuelta Y caiga justo en ese número: una entre
  cuatro mil millones—. Sin cerrojos ni coste: es la misma suma de hoy, en otro sitio. Y la generación de un handle ya
  no es nunca 0: con TAG y palabra alta 0, también rechazada.
- **Las dos VMs**: miVM tiene lo mismo (escribe la memoria byte a byte, `VirtualMachine.java:2595-2618`; recicla el
  número en el acto, `handleKillIdx`; un contador por número), y su `--workers=2` corre el mismo riesgo. (b), (c) y
  (d) son algoritmo: primero en miVM, como pide la cascada, y luego en la VM-C; (a) es representación: la VM-C (en
  miVM, el mecanismo de Java 8 está por elegir: su `--workers` es experimental).
- **Lo que ve el programa** (la pregunta 1 de arriba, contestada por el diseño): sin Mutex, un hilo que lee una
  referencia que otro está escribiendo obtiene la vieja, la nueva o «objeto eliminado»; nunca otro objeto, y la VM
  nunca se rompe. Siempre entera, como Java, pediría un cerrojo al escribir referencias: medido el coste de uno común
  (arriba), queda para otra versión si hiciera falta.

**La regla de Eduardo** (5-oct, al leer lo anterior): *«para que no nos volvamos locos. La memoria que se utilice
compartida por 2 threads BP es responsabilidad del programador. Si quiere que esté segura debe protegerla de forma
adecuada.»* Con eso, lo que lee un programa que comparte sin Mutex es cosa suya: fuera la pieza (d) —sólo servía a
dos hilos que escriben a la vez el mismo campo sin protegerlo— y fuera cualquier garantía tipo Java sobre los valores.
Lo que quedaba por decidir: si un programa así sólo puede leer valores malos, o si se acepta también que rompa la VM
—hoy, en la Pico con dos runs, escribir en cualquier sitio de la memoria y colgarse más tarde, quizá sin decir nada—;
las piezas (a), (b) y (c) son lo que lo impide. **Decidido** (Eduardo, 5-oct): *«OK a a, b y c. Yo me refiero a
properties y listas, el BP ya tiene mecanismos de protección y es el usuario el que ha de utilizarlos. El resto es
responsabilidad nuestra, a:=a+1 siendo a una variable local debe funcionar siempre.»* Las propiedades y las listas
compartidas, del programador (con los Mutex de BP); la VM y lo privado de cada hilo, nuestros. Se hacen (a), (b) y
(c); la (d), no.

#### El camino fino, hecho en el PC (5-oct; `#660`, `#661`)

- **(a)**, como en el diseño, con un ajuste medido: los caminos sin alinear, FUERA de línea (`bpvm_util.c`) y el
  alineado marcado como el probable. Con los dos en línea, el ejecutable del PC crecía un 7 % y el cálculo iba un 9,6 %
  más lento; así, en una A/B alternada contra la VM de `#662` (la mediana de siete): cálculo +1,9 % —ruido—, arrays
  −15 %, objetos −13 %, cadenas igual. En la Pico, el código crece 2 040 bytes (+0,5 %). Y el cargador alinea el bloque
  de cada módulo en las dos VMs.
- **(b)**, más sencilla que lo escrito: no hace falta rehacer los opcodes para resolver una sola vez. Con varios runs,
  matar un handle sube su generación en el acto pero deja su dirección y aparta su número hasta el GC (que los aplica
  antes de marcar): resolver dos veces en un paso da la misma dirección, la de un bloque que nadie reutiliza hasta el
  GC.
- **(c)**, y además: con TAG y un índice que no existe, muerta (se daba por viva y se resolvía a la dirección 0); el
  control en `PRINT_STRING`/`PRINT_STR_NONL`, los únicos que recorrían una cadena sin mirarla (censados los demás: los
  20 opcodes que resuelven, con él; `INSTANCEOF`/`CHECKCAST` validan la dirección por su cuenta); y su aviso propio, por
  el canal normal y las tres primeras veces (el urgente vuelca el log a la flash, y un programa con una carrera puede
  dar muchas).
- **La prueba**, `test-palabras` (en la red): (a) dos hilos de verdad, 292 millones de lecturas mientras el otro escribe,
  ninguna a medias; (b) soltar con varios runs y otra reserva no hereda el número, el GC lo devuelve; (c) las
  referencias que no pueden existir, muertas, y null, una constante y un handle vivo, vivos. Tres sabotajes, rojos.
  `test-smphandles`, que contaba con el reciclado en el acto, hace lo del GC cada 64 vueltas.
- **miVM**: su `--workers` sigue con su carrera B1 (experimental desde V2): fuera de H12. Con un worker, nada de esto
  cambia lo que hace; sólo el bloque alineado, igual en las dos (la paridad).
- **Corregido en la Pico** (A/B de tres imágenes con el OK de Eduardo): la versión con la comprobación de alineación
  hacía `fib(26)` un 6 % más lento (3 367 contra 3 178 ms; con los accesos de antes, 3 171): a `-Os` no iba en línea y
  cada acceso de la pila era una llamada. Por arquitectura: en la M33 y el x86 (una instrucción para una palabra sin
  alinear, de una pieza si está alineada), sin comprobar y siempre en línea —leer una local y empujarla, un `LDR` y un
  `STR`; el código de la Pico, 424 860 bytes, menos que esta mañana—; en RISC-V y Xtensa, con la comprobación.

#### Lo que dijo Eduardo después (5-oct, tarde)

- **La sonda SWD**: *«No tengo sonda SWD para la Pico.»* La 3.1 va como siempre con la Pico: las migas por núcleo en la
  RAM que sobrevive al reinicio, el log en la flash, el LED y el perro guardián.
- **El 2º run, inactivo**: *«se podría marcar como inactivo hasta que en la cola de Threads BP haya 2 entradas o más»*
  —es lo que hace `#662`: el ocioso duerme en la cola y sólo lo despierta lo que puede darle trabajo—. Y la memoria
  del 2º run (16 KB de pila) sólo le cuesta a la Pico 2: dos núcleos y sin PSRAM.
- **Natives y LVGL, antes de pasar al resto de micros** (*«antes de pasar de la Pico al resto de micros queda por
  resolver los nativos y LVGL ¿no?»*): sí. Hoy un programa con natives, GUI o SQLite corre con UN run aunque haya dos.
- **LVGL con dos runs: una cola FIFO de comandos** (idea de Eduardo: *«¿El límite de LVGL se soluciona con una cola
  FIFO de comandos? … lo normal es que 1 hilo crease un formulario y el otro cambie el texto de una etiqueta»*). Encaja
  con lo que hay: `src/gui.c` ya guarda una copia de cada widget (tipo, texto, valor) y cada setter la actualiza y llama
  a LVGL en el acto (`bpvm_gui_set_text` → `lv_label_set_text`). Con la cola: cada función que toca LVGL actualiza la
  copia (con un cerrojo cortito) y deja el comando en la FIFO, desde cualquier hilo y también desde el de la GUI (el
  orden); sólo el hilo en `Gui.run()` ejecuta los comandos, en orden, antes de cada vuelta de LVGL; las lecturas, de la
  copia, como hoy; si la cola se llena, espera quien encola, y el hilo de la GUI nunca se espera a sí mismo. Igual con
  uno o dos runs, en el lado de la VM (antes del pack de LVGL). Por diseñar del todo cuando se llegue.

#### La etapa 3.1, el diseño (5-oct, tarde; leído en el código mientras la resistencia de la 2b)

Los dos núcleos de la RP2350 con UN run. La imagen ya se puede pedir (`-DBPVM_PICO_NUM_CORES=2`: `configNUMBER_OF_CORES`,
la afinidad y las prioridades múltiples, `pico/FreeRTOSConfig.h`); lo que falta es lo que hoy sólo es seguro porque
hay un núcleo. Paso a paso, cada uno con su prueba:

0. **Arrancar, instrumentado** (sin sonda: Eduardo no la tiene). Las migas por núcleo en la RAM que sobrevive al
   reinicio (`bpvm_breadcrumb_attach`), el perro guardián armado durante el arranque —si se cuelga, reinicia y el
   arranque siguiente cuenta en el log las migas del anterior— y el LED. Primer objetivo: llegar al REPL con los dos
   núcleos, sin hacer nada más. (En mayo, el núcleo 0 se quedaba DENTRO de `vTaskStartScheduler` esperando al 1.)
1. **Lo que saca la flash del XIP, antes del planificador.** Hoy, en `vm_task` y con las tareas ya en marcha
   (`pico/main.c:1604-1612`): el sondeo JEDEC (`board_desc_early_init` → `flash_do_cmd`, con sólo las interrupciones
   del núcleo propio apagadas) y la PSRAM (`board_desc_psram_from_env`: reconfigura el QMI, que comparte con la flash;
   sólo la Metro). A `main()`, con un solo núcleo vivo. La lectura del ENV (por XIP) se puede quedar donde está.
2. **El candado de la flash** (`pico/flash_lock.c`, que usa la cintura `pico_flash.c`). (a) Las DOS víctimas: cada
   núcleo se registra a sí mismo (`multicore_lockout_victim_init`): el 0 en `main()`; el 1, una tarea anclada a él al
   arrancar, que avisa con un semáforo; y el arranque no escribe la flash hasta que los dos lo están —hoy sólo se
   registraba un hilo anclado a un núcleo ≠ 0, y no hay ninguno: la primera escritura esperaría para siempre—. (b) Un
   mutex NUESTRO que cubra la ventana entera: `multicore_lockout_start_blocking` suelta el cerrojo del SDK después de
   aparcar al otro núcleo, y antes de apagar las interrupciones (`flash_lock.c:25-33`) otra tarea del mismo núcleo
   podía abrir otra ventana y, al cerrarla, despertar al otro núcleo con la primera aún escribiendo. (El puerto de
   FreeRTOS de la RP2350 avisa entre núcleos con los doorbells, no con la FIFO: la FIFO queda para el lockout.)
3. **El log** (`pico/log.c`): su sección (`log_entrar`, hoy apagar las interrupciones: sólo el núcleo propio), con un
   spinlock del SIO (`spin_lock_blocking`: vale en una tarea y en una interrupción). Y `#658` (el volcado a la flash,
   fuera de la sección): copiar el log bajo el spinlock y escribirlo FUERA, con el candado de la flash —nunca aparcar al
   otro núcleo con un spinlock cogido: si el otro está girando en él con las interrupciones apagadas, no puede
   aparcarse, y los dos se quedan esperando—.
4. **Los caminos fatales** (`vAssertCalled`, el desbordamiento de pila, la memoria de FreeRTOS, el HardFault): hoy
   vuelcan el log a la flash, y con dos núcleos eso es aparcar al otro desde donde FreeRTOS tiene sus cerrojos cogidos:
   un interbloqueo mudo. Lo propuesto: el log, en la RAM que sobrevive al reinicio (sin escribir la flash), el otro
   núcleo, parado, y el LED, como hoy; el arranque siguiente lo encuentra («log: RAM SUPERVIVIENTE»). Lo que cambia: tras
   un fallo fatal, si se corta la corriente antes de leerlo, ese log se pierde. **Decidido** (Eduardo, 5-oct: *«OK»*),
   para la imagen de dos núcleos; con uno, como hoy.
   **Y afinado al escribirlo** (5-oct, tarde; tres preguntas de Eduardo). Parar al otro núcleo resultó lo incómodo: al 1
   lo para el SDK (lo reinicia), pero al 0 no hay con qué —escrito con una alarma del timer cuya interrupción forzaba
   el 1—, y el tick de FreeRTOS vive en el 0. Eduardo: *«¿por qué necesitas parar el núcleo 0?»*; y el principio: *«Lo
   importante es que IO sea sólido como una roca, todo lo demás tiene solución»* —contener el fallo en el run, que el
   SO sigue estable: terminar la TAREA y no el núcleo, que suelte sus cerrojos y que `io` informe; y el MPU de la M33
   protegiendo la memoria del SO y la de `io`—. Pero *«es un tema complejo. Lo podemos dejar para más adelante (V8).
   De momento hacemos lo que podemos, sabiendo que nos puede quedar algún caso suelto. Para eso fatal error, el log y
   reset»*. **Lo hecho**: con dos núcleos, cada camino fatal llama primero a `pico_fatal`, que ARMA EL PERRO GUARDIÁN
   (250 ms: pase lo que pase después, la placa se reinicia; nunca muda), deja una marca en la RAM que sobrevive y pone
   el log sin flash (el volcado ya no aparca al otro núcleo); ni se para al otro núcleo ni se cambia nada más de cada
   camino —su informe, y su bucle: el perro lo corta—. El arranque siguiente lo dice (en el log, y como causa del
   reinicio en el `INFO`: «fallo fatal»), vuelca el log a la flash como siempre y **no hace el autorun**: es la regla
   de H9 que no se había hecho —*«un fallo previo o el flag de modo seguro lo inhiben»* (`H9_KERNEL_CAPAS.md` §3)—,
   para los fallos fatales. Y el CONTADOR de reinicios de `V3_IDEAS` («Pagar doble»: varios seguidos, sin autorun),
   que a un equipo autónomo le relanzaría la app tras un fallo suelto, a V8 (*«OK, el contador a V8»*) con la
   contención y el MPU (`FICHAS`, el bloque de V8). La alarma, fuera.
5. **Cerrar el ejecutor de una native** (`pico/platform_freertos.c`, `bpvm_platform_thread_join`): hoy se borra la
   tarea y se libera su pila en cuanto avisa que acabó, y con dos núcleos puede estar todavía corriendo en el otro
   (entre el aviso y su `vTaskSuspend`). Esperar a que esté suspendida (`eTaskGetState`) antes de borrarla.
6. **Las banderas que cruzan núcleos, con orden**: `kill_requested` (`volatile int`, `include/bpvm_internal.h:670`),
   el fin de la cola y el anillo de la UART, con `__atomic` (escribir con release, leer con acquire). Es lo que dejó
   dicho `A4`.
7. **El MPU** (`mpu=1`, `#440`): cada núcleo tiene el suyo; configurarlo en los dos.

8. **La vuelta atrás** (decidido, Eduardo, 5-oct: *«OK»*): además de la imagen de un núcleo, un interruptor en el
   ENV, `vm.cores=1`: la imagen de dos núcleos con todas las tareas ancladas al núcleo 0 (el 1, sólo con su tarea
   ociosa: sigue siendo víctima del candado de la flash). Sirve para volver atrás sin grabar y para medir uno contra
   dos núcleos con la MISMA imagen.

**Escrita en el código** (5-oct, tarde; las dos imágenes de la Pico compilan —`pico/build`, un núcleo; `pico/build2`,
`-DBPVM_PICO_NUM_CORES=2`—; sin grabar todavía):
- **0** — las marcas del arranque, una palabra por núcleo en la RAM que sobrevive (`pico_marca_arranque`: 0x01 `main`,
  0x02 la identidad, 0x03 `vm_task` creada, 0x04 el planificador; 0x10 `vm_task`, 0x11 el núcleo 1 víctima, 0x12 el
  reparto, 0x13 la subida, 0x20 el arranque completo), que el arranque siguiente cuenta en el log; y con dos núcleos el
  perro guardián del ARRANQUE (15 s; lo apaga el arranque completo, antes del autorun o del REPL).
- **1** — el sondeo de la flash, el ENV y la PSRAM, de `vm_task` a `main()` (en las dos imágenes: una sola forma).
- **2** — `pico/flash_lock.c`: ~~cada núcleo se registra a sí mismo como víctima (dos tareas ancladas, al empezar
  `vm_task`; hasta que están los dos, escribir la flash FALLA y se dice)~~; el mutex de la ventana, nuestro, creado en
  `main()`; y `pico_flash_lock_begin` devuelve -1 si no puede. **Corregido en la puerta** (5-oct, tarde): con la
  imagen de dos núcleos y `vm.cores=1`, `ThrowSinAtrapar` colgó la placa en la tanda —ni el KILL ni el RESET—, y no
  siempre. Sin sonda: el log de la flash estaba VACÍO al arrancar (el último volcado, cortado tras borrar), y con unas
  marcas en el volcado y en el candado, y el perro de BP (`Wdt`, que reinicia sin quitar la corriente: la RAM
  sobrevive) en un programa que lanza sin atrapar, en bucle: el núcleo 0 se quedaba DENTRO de
  `multicore_lockout_start` (la marca 0x52), y con un tope de 500 ms en el saludo, igual —el saludo terminaba; lo que
  no volvía era lo de después—. El porqué: el núcleo 1 estaba a mitad de un `vTaskSuspendAll` (su tarea ociosa,
  liberando una tarea borrada —con `mpu=1`, las del MPU, en cada RUN—: el cerrojo de tareas del kernel, cogido, con
  las interrupciones abiertas), se aparcaba en la interrupción de la FIFO CON ESE CERROJO, y el núcleo 0, al soltar el
  mutex del SDK (la interoperación con FreeRTOS: `xEventGroupSetBits` → `vTaskSuspendAll`), lo necesitaba. Lo dice el
  propio SDK (`pico_flash/flash.c`): *«we cannot mix multicore_lockout and FreeRTOS»*, y con FreeRTOS SMP aparca con
  una TAREA. **Arreglo**: un aparcador por núcleo —una tarea estática anclada, de la prioridad más alta, que espera un
  aviso: sólo corre cuando su núcleo no tiene cerrojos del kernel—, que apaga sus interrupciones y espera en la RAM;
  con tope (500 ms): si no se aparca, la escritura FALLA y el log dice qué corre en el otro núcleo —nunca un cuelgue
  mudo—. Fuera las víctimas por interrupción. Visto: el bucle que colgaba en las primeras vueltas, 150 sin colgarse.
  Lo que enseña: «más piezas para lo mismo» no era lo mismo; la elección del SDK tenía un porqué, y estaba escrito.
- **3** — el log: con dos núcleos, un spinlock del SDK (`pico/log.c`); y `#658` en el común (`src/bpvm_log.c`): un
  volcado a la vez con una bandera y su petición pendiente, y la cabecera y el relleno dentro de la sección. Con su
  prueba, `test-logmt --volcado`, verde y roja con el volcado de antes (y roja también con sólo el relleno fuera).
- **4** — los caminos fatales, como quedó decidido arriba (`pico_fatal`, en `pico/main.c`), y el informe del HardFault
  sin `xTaskGetSchedulerState` con dos núcleos (coge los cerrojos del kernel; `src/bpvm_fallo_cm33.c`).
- **5** — `bpvm_platform_thread_join` (`pico/platform_freertos.c`): el ejecutor, borrado sólo suspendido y fuera de
  los dos núcleos (`eTaskGetState` y `xTaskGetCurrentTaskHandleForCore`).
- **6** — en el común: `kill_requested` por `bpvm_kill_pedido`/`bpvm_kill_marcar` (acquire/release), el `fin` del
  run a `io` y el anillo de la UART (el byte antes de publicar `head`; el sitio, después de ver `tail`).
- **7** — el MPU de `mpu=1` en los dos núcleos: una tarea anclada a cada uno lo aplica, y se espera a las dos.
- **8** — `vm.cores=1`, hecho con el 0.
Y la red del PC, por el común tocado (`#658`, el paso 6).

**La puerta de la 3.1** (la Pico 2, un run, la imagen de dos núcleos): arranca 20 de 20 (con el USB y sin él); la
tanda y `run_placa`, verdes; escribir la flash con el otro núcleo ejecutando (un programa que escribe ficheros mientras
otro calcula, el ENV y el log), sin un fallo; PrintBench (con `io` en el otro núcleo debería bajar) y `fib(26)`; y los
110 minutos de resistencia. **Y la 3.2**, `vm.runs=2` con los dos núcleos: la puerta de la 2b otra vez y `ParaleloBench`
en la placa —lo que gana de verdad un programa con dos hilos—.

**La puerta de la 3.1 en la Pico 2** (5-oct, tarde; con el OK de Eduardo para el lote: la imagen de un núcleo, la de
dos con `vm.cores=1` y con los dos núcleos; los commits `05d02f74`, `3438cf3d` y `b9a6e5eb`):
- **la imagen de UN núcleo** (también cambia: la identidad, el ENV y la PSRAM, antes del planificador): la tanda con
  los mismos veredictos que la puerta de la 2b (34 idénticas, las 3 paradas a propósito, las 3 de la GUI saltadas y
  `synclisttest` en «memoria apretada») y `run_placa` 34 de 34.
- **la de dos núcleos con `vm.cores=1`**: arranca (el cuelgue de mayo no se repite: el núcleo 1 vive y lo dicen las
  marcas); y la tanda destapó EL fallo de la 3.1 —`ThrowSinAtrapar` colgaba la placa, a veces—: el candado de la
  flash, que aparcaba al otro núcleo por interrupción (arriba, el paso 2, «Corregido en la puerta»). Con el aparcador:
  el programa que colgaba, 150 vueltas sin colgarse; la tanda, los mismos veredictos que con la imagen de un núcleo;
  `run_placa` 34 de 34.
- **con los dos núcleos**: 20 arranques seguidos de 20; la tanda, los mismos veredictos que con un núcleo, y
  `run_placa` 34 de 34; escribir la flash con el otro núcleo activo —`IoMt`, tres hilos que escriben ficheros, 40 RUN
  seguidos, con el ENV escrito y borrado entre RUN y RUN y el log volcado en cada uno: 24 000 escrituras—, sin un
  fallo (cada RUN, 11,5 s contra 10,7 con un núcleo: aparcar como tarea cuesta en cada escritura); el programa que
  colgaba, 150 vueltas sin colgarse; y **el camino fatal** (`prueba.fallo=1`: un HardFault al empezar un RUN, en el
  núcleo 1): la placa se reinicia en medio segundo, el `INFO` dice «fallo fatal», el log trae el informe —la tarea,
  la pila con su límite, el PC, UNDEFINSTR— y el arranque no hace el autorun (con `/sys/auto.txt` puesto, como lo
  pone el IDE); limpio después, y un RESET normal vuelve a decir «reinicio pedido».
- **Dos cosas más que destapó**: la causa del reinicio del `INFO` decía «watchdog» sin haber disparado nada en cuanto
  se armaba un perro (`watchdog_enable_caused_reboot` mira una palabra que escribe el propio `watchdog_enable`; con un
  núcleo, también tras el `Wdt` de un programa): ahora se calcula una vez al arrancar. Y las tareas ociosas de la
  imagen de dos núcleos GIRABAN ejecutando desde la flash, compitiendo por la caché del XIP: ahora duermen (`__wfi`).
- **Lo medido con UN run** (el PC en reposo; la tabla, con la misma placa):

  | imagen | `fib(26)` | PrintBench |
  |---|---|---|
  | un núcleo | 2 843 ms | 889 ms |
  | dos, ociosas girando: `vm.cores=1` / dos | 3 027 / 2 992 | 1 105 / 1 077 |
  | dos, ociosas dormidas: `vm.cores=1` / dos | 2 924 / 2 895 | 1 080 / 1 063 |

  Con un run, el 2º núcleo NO acelera: `io` en el otro núcleo apenas se nota (−2 %), y la imagen SMP cuesta un +1,8 %
  en cálculo y un +20 % en `print` (cada aviso entre tareas pasa por los cerrojos del kernel SMP, y de un núcleo a
  otro). La ganancia, si la hay, es con dos runs: la 3.2. Y aparte, PrintBench con la imagen de UN núcleo ha subido
  desde la mañana (774 ms con la imagen de `4a608d49`; 881–889 desde el camino fino), con más dispersión: a mirar.
- **La resistencia de 110 minutos con los dos núcleos: SIN FALLOS** (959 vueltas, 23 RESET, el KILL a un dormido en 7 ms
  de mediana —22 de máximo— y las pilas quietas; el mínimo libre del heap de FreeRTOS, 4,4 KB —con un núcleo, 6,7—: a
  vigilar). **La puerta de la 3.1, PASADA.**
- **La A/B de PrintBench** (Eduardo: *«pruébalo»*): la imagen de la mañana (`4a608d49`), medida otra vez a la vez que la
  de hoy, da 852 ms (791–870), no los 774 de la mañana; la de hoy, 881 (860–902). No hay regresión que perseguir (un +3 %,
  dentro del ruido de esa prueba, que depende del PC que lee el USB); `fib(26)`, 3 179 contra 2 844: −10,5 %.

#### La 3.2: dos runs en los dos núcleos (5-oct, noche)

Eduardo: *«a ver si podemos medir cuánto ganamos al ejecutar un programa BP con 2 núcleos respecto a ejecutarlo con 1
núcleo»*. Con `ParaleloPico` (`ParaleloBench` con tamaños para la placa: cuatro cargas, cada una dos veces seguidas en el
`main` —secuencial— y en dos hilos a la vez —paralelo—), el mismo programa en tres configuraciones de la Pico 2
(medianas de 3; ms, secuencial / paralelo):

| configuración | cálculo | arrays | objetos | cadenas |
|---|---|---|---|---|
| 1 núcleo, 1 run | 2 168 / 2 176 | 3 037 / 3 047 | 2 528 / 2 538 | 1 347 / 1 487 |
| 1 núcleo, 2 runs | 2 259 / 2 270 | 3 158 / 3 171 | 2 648 / 2 668 | 1 684 / 1 839 |
| **2 núcleos, 2 runs** | 2 393 / **1 305** | 3 348 / **1 822** | 2 881 / **1 635** | 2 337 / 2 273 |

- **Lo que gana un programa con dos hilos que trabajan a la vez**, con dos núcleos frente al mejor de un núcleo: cálculo
  ×1,67, arrays ×1,67, objetos ×1,55; las cadenas, PEOR (×0,65). Frente a la misma configuración en secuencial, ×1,83,
  ×1,84 y ×1,76 (la estimación de `PROJECT_ROADMAP` §5 era ×1,9 para el cálculo paralelizable).
- **Lo que paga el código de UN hilo** con dos runs en dos núcleos: +10 % en cálculo y arrays, +14 % en objetos; y la
  imagen de dos núcleos con UN run, +1,8 % (la 3.1). Con dos runs en un núcleo, +4 % (el punto seguro de cada paso).
- **El patrón de los programas de Eduardo** (`PatronSondas`: un hilo atiende órdenes cada 10 ms y otro gestiona el
  conversor —inicializar, esperar el «ready» con pausas hasta 500 ms y unos 50 ms de cálculo—, con una lista bajo
  `Mutex`): en las tres configuraciones, el hilo de órdenes con 0–1 ms de retraso como mucho y cada lectura en 550–557
  ms. Para ese patrón BP ya va bien con un núcleo (los dos hilos duermen casi siempre); el 2º núcleo no añade.
- **«No space in heap»** en la 2ª vuelta con los tamaños primeros: la PRUEBA, demasiado grande para el heap de un
  programa con dos runs (153 KB con dos runs en la imagen de dos núcleos; 195 con uno: la pila del 2º run sale del
  bloque, y algo más); con tamaños menores, tres vueltas limpias en las tres configuraciones.
- **Las cadenas, peor con dos núcleos**: Eduardo, *«no tiene sentido salvo que mientras un hilo está manipulando cadenas
  el otro hilo tenga que esperar»*. Era eso. En paralelo, cada «+» reserva bajo el cerrojo de la VM (el heap es uno) y
  cada GC para el mundo: casi en fila india. Y en SECUENCIAL —un hilo, el otro run ocioso—, `CadenasSolo` dio **309
  «vueltas sin hilo»** —tantas como colectas— y su único hilo **repartido entre los dos runs** (137 y 442 cuantos):
  cada GC avisaba a la cola al parar y al reanudar el mundo, y eso despertaba al run OCIOSO, que a veces se llevaba el
  hilo al otro núcleo. `#662` quitó ese despertar al final de cada cuanto, no en el GC. **Arreglado en el PC**: el
  mundo parado, con su propia condición (`mundo_cond`, `include/bpvm_smp.h`): la esperan el colector, los runs que
  esperan a que acabe una colecta y la zona de rascar, y la avisan parar, reanudar y cada «un run menos en marcha»;
  `sched_cond` se queda para el trabajo, el KILL y la salida. Su prueba, `test-cola` 17 (`CadenasUnHilo`: un hilo que
  concatena con dos runs): 2 vueltas sin hilo como mucho; con el sabotaje (el mundo avisa también a la cola, como
  antes), hasta 243 (`c3fe9277`, con la red del PC verde). **Medido en la placa** (dos núcleos, dos runs):
  `CadenasSolo`, 2 vueltas sin hilo (antes 309), el hilo quieto en su run (579 cuantos en uno, 0 en el otro) y de 2 133
  a ~2 010 ms (−6 %); `ParaleloPico`, igual en cálculo, arrays y objetos (×1,81, ×1,81, ×1,76) y las cadenas, de
  2 337 / 2 273 a 2 172 / 2 173 ms. Los despertares eran una parte; lo que queda de las cadenas con dos runs es el
  cerrojo de cada reserva y más colectas (el heap de dos runs, más pequeño): lo de V8, un trozo del almacén por run.
- **La puerta de la 3.2** (dos runs en dos núcleos, con el código de `b9a6e5eb`, sin `mundo_cond`): la tanda, los mismos
  veredictos que con un núcleo y un run (`compat/informes/tanda_h12_32.md`); `run_placa` 34 de 34; y la resistencia de
  110 minutos con dos runs y el programa de carreras, **SIN FALLOS** (883 vueltas, 22 RESET, 883 carreras sin `Mutex`
  sin que la VM se rompa, el KILL a un dormido en 8 ms de mediana —15 de máximo— y las pilas quietas). **La puerta de
  la 3.2, PASADA.** El arreglo de `mundo_cond`, aparte (Eduardo: *«Ahora lo que interesa es que los 2 núcleos
  funcione»*): la red del PC, y después su medida en la placa, la tanda y su propia resistencia: 110 minutos con dos
  runs, **SIN FALLOS** (889 vueltas, 22 RESET, 889 carreras; el KILL a un dormido, 8 ms de mediana y 26 de máximo; las
  pilas quietas).

**Lo que dijo Eduardo al acabar** (5-oct, noche): *«Con la última prueba de resistencia los 2 núcleos quedan validados,
a falta de terminar LVGL y natives, no?»* —sí, en la Pico 2 y para el código BP normal—. Y dos decisiones:
- **El depurador, con UN run, el primero** (*«El debug de momento solamente sobre el run1. Alguna vez he tenido que
  depurar en Java con varios hilos y es muy difícil de seguir. Así que de momento tiene esa limitación, se documenta y
  ya»*): es lo que hace `un_solo_run` y, desde «LVGL con dos runs», con un run lo corre el run 0 (su Run1);
  documentado en `PENDIENTES.md`.
- **Las placas de UN núcleo, con un run** (*«C3, C5 y C6 más las STM32 tienen 1 núcleo así que 1 run, 2 runs no aporta
  nada»*): el resto de placas de H12 son, para ellas, su tanda con la imagen nueva y `vm.runs=1`; el 2º run (la 2b)
  sólo en las de dos núcleos —la Metro, la S3 y las P4— y en la P4 de hoy (un núcleo) como banco de LVGL con dos runs.

**Y para afinar** (medido en la puerta de la 2b): con dos runs, `fib(26)` va un 4 % más lento que con uno (2 948 contra
2 835 ms): lo más probable, el punto seguro de cada paso, que con dos runs lee dos punteros (`vm->smp` y su
`stop_the_world`): guardar la cola en una variable del cuanto. A medir en el próximo lote.

**Las pruebas** (en el PC, con dos runs, cada una con su sabotaje): (a)+(c), un hilo que alterna referencias (y null)
en un campo y otro que lo lee y lo usa sin parar, con la ventana ensanchada entre las dos palabras de un almacén (un
gancho de prueba): con las piezas, sólo valores buenos o «objeto eliminado» atrapado; sin ellas, el proceso roto o un
array escrito fuera. (b), un hilo suelta un `owner` y reserva otro del mismo tipo mientras el otro escribe en el
primero, con la ventana entre las dos resoluciones del paso: con la pieza, el nuevo intacto; sin ella, escrito por quien
no era. (d), dos hilos escriben a la vez el mismo campo y otro suelta y reserva sin parar: ninguna lectura llega a otro
objeto. Y después, la puerta de la 2b otra vez en la Pico 2 (con el OK de Eduardo para grabar).

#### LVGL con dos runs: el análisis (5-oct, noche; leído en el código, para que Eduardo decida)

Eduardo: *«para probar el LVGL necesitamos una placa que tenga pantalla»* → la P4, con un núcleo y `vm.runs=2` (la 2b
de la familia ESP32), y antes el PC. Hoy un programa con GUI va con UN run (`un_solo_run`, `src/bpvm_repl.c`).

**Lo que hay.** El lazo de la GUI es un hilo BP (`HiloGui`, `bpstdlib/Gui.bp`) que llama a `__guiRunOnce` (builtin 222):
drena los eventos —cada uno, una llamada ANIDADA al handler (`bpvm_call_bp_from_builtin`)—, mira el KILL y da una
vuelta de LVGL (`lv_timer_handler`). Los demás builtins de la GUI (crear, poner texto, leer el ancho…) entran en
`src/gui.c`, que guarda una copia de cada control y llama a LVGL en el acto. Ni LVGL ni `gui.c` son reentrantes.

**Lo que ha salido al leerlo, y cambia la idea de la cola FIFO:**
- **La ventana del PC sólo la atiende el hilo de SO que la creó.** El driver SDL de LVGL (`lv_sdl_window_create`,
  `src/gui_display_sdl.c`) lee los eventos de la ventana DENTRO de `lv_timer_handler`, y en Windows la cola de mensajes
  es del hilo que creó la ventana (SDL: *sólo desde el hilo que arrancó el vídeo*). Con dos runs, `HiloGui` salta de un
  run a otro entre cuantos: la vuelta que le toque al run 1, LVGL lee desde otro hilo y la ventana se queda sin atender.
  O sea: **todo lo de la GUI tiene que hacerlo SIEMPRE el mismo run, con cola o sin ella.**
- **Hay lecturas que van a LVGL**, no a la copia: el ancho de un control de tamaño automático (`bpvm_gui_get_width` →
  `lv_obj_get_width`). Con una cola, leerlo justo después de cambiar el texto daría el ancho viejo, y leerlo desde otro
  hilo es tocar LVGL a la vez que el lazo: las lecturas tendrían que esperar al lazo igual.

**Las dos formas:**

| | (A) la cola FIFO (la idea de Eduardo) | (B) **la llamada viaja al run 0** |
|---|---|---|
| mecanismo | cada setter actualiza la copia y deja el comando en la cola; el lazo los aplica antes de cada vuelta | cada builtin de la GUI se ejecuta SÓLO en el run 0; un hilo en el run 1 que llega a uno cede su cuanto marcado «para el run 0», y el run 0 repite la instrucción |
| semántica | asíncrona: crear tiene que devolver un control que aún no existe; las lecturas que van a LVGL, esperando al lazo | la de hoy con un run: crear devuelve el control, leer tras escribir da lo escrito |
| la ventana del PC | el lazo, fijo a un run aparte | resuelta: el run 0 es siempre el mismo hilo |
| qué cambia | `gui.c` (separar «la copia» de «aplicar a LVGL»), que va DENTRO del pack: los tres packs, otra vez; el tamaño de la cola, qué hacer llena, y el formulario creado ANTES de arrancar el lazo (nadie la vacía) | sólo la VM: el intérprete y el planificador; ni `gui.c` ni el pack |
| lo que espera un hilo | nada al escribir | lo que le quede al run 0 de su cuanto o de su vuelta de LVGL (milisegundos); mientras, el run 1 sigue con otros hilos |

**(B), por dentro** (propuesta):
1. En `OP_CALL_BUILTIN` (`src/interp.c`), un mapa de bits de los builtins de la GUI —no son un rango: 135–199, 206–207,
   210, 214–220, 222–223, 234–239—. Si hay varios runs (`vm->smp`) y el hilo no está en el run 0 (`tc->sched_owner`):
   el pc vuelve a la instrucción, el hilo queda RUNNABLE con `run_fijo = 0` y el cuanto acaba. Con un run, una
   comparación y nada más.
2. `bpvm_hilos_elegir` (`src/threading.c`) salta los hilos fijados a otro run; el run 0, al elegirlo, borra la marca:
   el hilo sigue en el run 0 hasta acabar el cuanto, y en el siguiente lo coge cualquiera.
3. El aviso: un hilo que queda listo para OTRO run avisa a la cola (`#662` sólo avisa si el hilo deja de estar
   RUNNABLE, y éste lo sigue estando).
4. El interbloqueo (`src/scheduler.c`): «ningún hilo que pueda elegir» ya no quiere decir «todos bloqueados»; uno listo
   para el otro run es trabajo, no un interbloqueo.
5. Un builtin de la GUI dentro de una llamada anidada fuera del run 0 (el puente de las natives) no puede ceder: si
   llegara a pasar, un RuntimeError con nombre. Hoy no puede: las natives van con un run.
6. Con UN run (las natives, el depurador, SQLite), entra el run 0 (hoy, el primero que despierta): la ventana que el
   simulado deja abierta para el siguiente programa sigue en el mismo hilo.
7. `un_solo_run` sin «la GUI».

**Lo que no cambia**: el GC —el hilo que viaja cede en un punto seguro, como cualquier fin de cuanto—; las raíces de la
GUI (`bpvm_gui_visit_roots`) se leen con el mundo parado, y nadie toca la copia fuera de un cuanto del run 0; los
handlers, anidados dentro de `__guiRunOnce`, ya están en el run 0. El run 0: en el PC, el hilo que llama a `bpvm_run`
(`src/scheduler_smp.c`); en las placas y el simulado, la tarea `run`, permanente.

**El precio de (B)**: lo que se hace en la GUI no va en paralelo —LVGL no lo permite de ninguna forma— y un hilo que la
toca mucho trabaja en el run 0 mientras lo hace.

**Las pruebas**: en el PC con dos runs, un programa de dos hilos —uno con el lazo; el otro cambia una etiqueta y lee su
ancho sin parar— y la cuenta de builtins de la GUI ejecutados fuera del run 0: 0; con el sabotaje (sin el viaje), no:
rojo. Los samples de GUI con dos runs, con la paridad de la VM-Java. La ventana del PC con dos runs: que responda (los
clics del guión, `BPVM_GUI_SCRIPT`, y la captura). En la P4, la 2b de la familia ESP32 —el 2º run— y después la GUI con
dos runs en la pantalla de verdad.

**Decidido** (Eduardo: *«OK»*): (B), la llamada viaja al run 0.

#### LVGL con dos runs: hecho en el PC (5-oct, noche)

**El código**, los siete puntos de (B):
- `src/interp.c`, `OP_CALL_BUILTIN`: con varios runs (`vm->smp`), un builtin de la GUI en un hilo que no está en el run
  0 deja el pc en la instrucción (`pc -= 3`, el opcode y su id), el hilo RUNNABLE con `al_run0` y el cuanto acabado.
  Con un run, una comparación. En el puente de las natives (`bridge_run_bp_frame`), un hilo que vuelve con `al_run0`
  es un RuntimeError con nombre, no una vuelta sin fin.
- `src/threading.c`: `bpvm_hilos_elegir(vm, libre, run)` salta los que esperan al run 0 si quien elige no es el 0; y
  `bpvm_hilos_esperan_al_run0`, para el interbloqueo.
- `src/scheduler.c`: el run 0, al elegirlo, borra la marca; avisa a la cola un viaje (el hilo sigue RUNNABLE, `#662`
  no avisaba) y la vuelta de uno (tras su cuanto en el 0 lo puede coger cualquiera); «ningún hilo que pueda elegir» no
  es un interbloqueo para otro run si hay alguno esperando al 0; un programa para UN run lo coge el run 0 (antes, el
  primero que despertaba); y la línea `runs:` del final, con los viajes; y un aviso urgente si algún builtin de la GUI
  se ejecutó fuera del run 0.
- `src/builtins.c`: `bpvm_builtin_de_la_gui` —los 83 `BUILTIN_GUI_*`, por nombre: no son un rango— y, al entrar en
  `bpvm_call_builtin`, la cuenta de los ejecutados fuera del run 0 (un error con nombre si lo llama una native).
- `src/bpvm_repl.c`: `un_solo_run` sin «la GUI».
- `src/gui_display_sdl.c`: la GUARDA de la ventana del PC: el hilo que la crea es el único que la bombea; si otro lo
  hace, «[gui] AVISO: la ventana, bombeada desde OTRO hilo…» (la primera vez).
- `test/main.c`: `BPVM_SABOTAJE_GUI=1` (SÓLO PRUEBAS): el builtin, donde se encuentra, como antes.

**Lo medido**:
- **`test-cola` 18** (`samples/GuiDosRuns.bp`: dos hilos que crean 40 etiquetas cada uno y les cambian el texto, la x
  y el ancho y los leen, con cálculo entre medias): 20 de 20, 624 viajes, NINGUNO fuera del run 0, el resultado de un
  run y de la VM-Java. **Sabotaje** (`--sabotaje-gui`): 0 de 20, 5 143 builtins de la GUI en el run 1, y alguna pasada
  muerta con un RuntimeError (el modelo de `gui.c`, roto). Los otros quince sabotajes, rojos. Y la lista de los 83,
  comprobada contra `builtin_ids.h` (quitando uno a mano, lo dice).
- **La ventana del PC** (`make LVGL=1`, `--smp=2`, `samples/GuiVentanaDosRuns.bp`: el lazo en su hilo, un botón y un
  hilo trabajador que calcula y pone su progreso en una etiqueta cada 20 ms; un guion con dos clics, la captura y
  cerrar): los dos clics, el trabajador hasta el final, la captura bien (`clics: 2`, `trabajador: 59 / 296512`) y 168
  viajes. **Con el sabotaje, el programa SE CUELGA**: el trabajador no vuelve de su primer `setText` (la etiqueta se
  queda en «trabajador: -») y lo mata el plazo. Y la guarda, con un trabajador que sólo calcula (el lazo es lo único
  que toca la GUI): con el viaje, la GUI en el run 0 (263 cuantos) y el cálculo en el 1 (14 986), sin aviso; con el
  sabotaje, el aviso.
- **El micro simulado** (el camino del IDE; `make sim LVGL=1`, `--runs=2`): tres programas seguidos en la MISMA
  ventana —dos con dos runs (el guion: los clics) y el tercero con el depurador (PAUSE antes del RUN: un run)—, tres
  veces, sin un aviso. Sin la regla «con un run, el run 0», el aviso en una de tres: el programa del depurador cae en
  el run 1 y la ventana del anterior se queda sin atender (en el IDE, congelada).

**La familia ESP32, el 2º run** (`esp32/common/repl_esp32.c`): la tarea `run2`, el run número 1, con `vm.runs=2`; su
pila, de la RAM interna del heap y sólo con dos runs (la C3 y la C6 van justas: con uno no la pagan); y
`run2StackFreeBytes` en el `INFO`, como la Pico. Compilan las cinco (P4, S3, C3, C5 y C6). **Los packs de la P4
aguantan** (la cuenta, antes de grabar): con la imagen nueva el código acaba en `0x400739DE` y los datos de sólo
lectura en `0x400BB604`, por debajo de la página `0x400C0000`: la zona sigue en `0x400C8000`, donde está hoy; y ni
`gui.c` ni la BIOS cambiaron desde la imagen que lleva (4-oct, 14:01, anterior a la 2b).

#### LVGL con dos runs: la P4 (5 y 6-oct, noche; la imagen de `7c038943`, con el OK de Eduardo)

Grabada sólo la aplicación (`idf.py -p COM15 app-flash`): los packs aguantaron como decía la cuenta —la zona en
`0x400C8000`, el bloque de RAM de los motores en `0x48001000`, el `STATE` limpio, sin «casi»—. La P4 llevaba la imagen
del 4-oct (anterior a la 2b): recibe H12 entero de una vez. Las pruebas, en `compat/informes/tanda_h12_p4.md`:
- **Con un run** (el ENV como estaba): la tanda corta, 6 de 6; la de V7 entera, 42 pruebas —36 idénticas al PC (las
  11 natives RISC-V incluidas), 2 «no peta», las 3 de KILL en PARADA y `synclisttest` con sus aserciones— y
  `run_placa` 34 de 34.
- **Con `vm.runs=2`** (el OK de Eduardo): el `INFO` con `runs` 2 y el log, «el 2o run, creado (vm.runs=2; 16 KB de
  pila, RAM interna)»; la tanda de V7 entera, con los mismos veredictos —sus pruebas de GUI (`GuiWinJson`, `GuiShot`,
  `GuiArbolG2`), ya con dos runs y en la pantalla de verdad—; `run_placa` 34 de 34; un programa de dos hilos en la
  pantalla —uno pone su progreso en una etiqueta cada 20 ms, el otro sólo calcula— que acaba bien y cuya captura
  (`Gui.shot`, 480×800) enseña los valores correctos (la suma del trabajador, la del PC); y **110 minutos de
  resistencia con dos runs, SIN FALLOS** (1 089 vueltas, 27 RESET, 1 089 carreras; el KILL a un dormido, 24 ms de
  mediana y 35 de máximo —el ida y vuelta de la UART de la P4, como antes de H12—; lo que sobró de pila en el peor
  momento: `run` 11 648 B, `io` 24 604 y `run2` 13 320).
- **Lo que falta ver en la placa**: CUÁNTOS viajes al run 0 hicieron los programas de GUI (la línea `runs:` del final).
  El log de la P4 está apagado por defecto (`log=0`: sólo guarda el arranque) y sólo lo enciende el ENV: pedido a
  Eduardo.

#### Las natives con dos runs: el mapa, y un fallo de la GUI que salió al hacerlo (6-oct, madrugada)

Eduardo, tras pensarlo juntos (la native usa DOS pilas —la de C: la de su run si es directa, la de su ejecutor si
puede ceder— y la BP del hilo que la llamó): *«está algo mejor de lo que pensaba. Hay que hacer unos cambios
localizados»*. El mapa, leído en el código:

| | qué pasa con dos runs | el arreglo |
|---|---|---|
| **N1** el contexto de la native (`bpvm_aot_callctx`, `bpvm_aot_fault_slot`: el hilo, su pila y adónde salta si lanza) | en el PC es uno por hilo del sistema (`__thread`); en los micros, UN global: dos natives a la vez se lo pisan | uno por tarea (los punteros por tarea de FreeRTOS), en las familias que tengan dos runs |
| **N2** el depósito de ejecutores (`ejecutor_libre`, `soltar`) | se devolvía el libre SIN reservarlo, y la lista crecía sin cerrojo: dos natives podían coger el mismo ejecutor | mirar y reservar juntos, bajo el cerrojo de la VM |
| **N3** una native que espera al mundo DENTRO de sí (una reserva que encuentra al otro run colectando, `stw_esperar_al_otro`; la zona de rascar) | se descuenta de los runs en marcha y el colector del otro no mira su pila de C —el comentario del GC decía «nunca a mitad de un native»—: un intermedio sólo en un local de C, reciclado | apunta su trozo de pila (con los registros volcados) en una lista de la cola, y el colector la escanea (`heap.c`, 2f) |
| **N4** el puente native→BP sin ejecutor, y la línea del bucle de una native sin ejecutor | el mundo parado corta el cuanto anidado y el puente lo volvía a llamar SIN FIN; la línea del bucle no esperaba: el colector, para siempre | el puente y la línea del bucle se aparcan con el mundo parado (`bpvm_stw_aparcar_anidado`) |

**El fallo que destapó N4 está en la GUI con dos runs (`7c038943`)**: el despacho de cada clic (`__guiRunOnce` →
`__guiDispatch`) y `Gui.__guiInvokeBySlot` corren BP ANIDADO por ese mismo puente, en el run 0. Si el otro run para el
mundo mientras tanto —un hilo que reserva y provoca un GC—, el run 0 giraba y el colector lo esperaba para siempre: un
cuelgue. La ventana del despacho de un clic es pequeña (unas decenas de pasos) pero real: con el patrón de Eduardo
(sondas + pantalla) acabaría pasando. Las pruebas de anoche no lo vieron (sus manejadores corren después, como evento,
no anidados). Arreglado con N4, y `test-cola` 19 (`GuiAnidadaGc`: un `invoke` largo mientras otro hilo reserva sin
parar): 10 de 10; con el sabotaje, colgado.

**De paso, dos cosas viejas de los formularios** (fuera de H12; `#663` y `#664`, SUELTAS): `Gui.__guiInvokeByName` no encuentra nada desde el
enlace tardío (`#523`: busca la clase del anfitrión en los datos de su módulo, y las clases ya no viven ahí) —no lo usa
nadie—; y en la VM-Java `__guiInvokeBySlot` no invoca (la VM-C sí): una diferencia de paridad que ningún sample del
corpus toca.

**Hecho en el PC: N2, N3 y N4** (el PC ya tiene N1: su contexto es por hilo):
- **N2** (`src/bpvm_nativa.c`): `ejecutor_libre` mira y reserva bajo el cerrojo de la VM (el ejecutor sale ya
  CORRIENDO), y uno nuevo entra en la lista con él; `soltar`, igual.
- **N3** (`src/heap.c`, `src/builtins.c`): `bpvm_aparcada_poner` / `quitar` —el trozo de pila de C de la native, desde
  un nodo en el marco de quien espera (con los registros volcados por `setjmp`) hasta el techo de su native más
  externa— en `stw_esperar_al_otro` y en la espera de la zona de rascar; el colector los escanea (el paso 2f).
- **N4** (`src/interp.c`, `src/bpvm_nativa.c`): el puente se aparca con el mundo parado (`bpvm_stw_aparcar_anidado`); la
  línea del bucle de una native en un ejecutor cede también con el mundo parado (antes, al acabar su rebanada de 5
  ms) y la de una native sin ejecutor se aparca.
- **Las pruebas**: `test-cola` 19 (la GUI: arriba) y **`test-nat2runs`** (`samples/NatDosRuns.bp`, con el GC
  colectando en cada reserva y dos runs: una native DIRECTA con dos reservas encadenadas llamada a la vez desde los
  dos runs; dos natives con bucle a la vez; y una que gira hasta que un hilo del otro run, que reserva, la suelte): con
  el AOT, con y sin ejecutor, lo mismo que interpretado. Los sabotajes: sin apuntar la pila, **9 etiquetas malas con
  `status=OK`** (la corrupción muda que N3 evita); sin el cerrojo del depósito, el programa muerto; sin aparcarse en la
  línea del bucle, colgado.

**Y una carrera en la LIBRERÍA `Gui` (no en la VM), para que decida Eduardo** (6-oct, madrugada): `test-cola` 18 falló
UNA pasada de ciento veinte con un RuntimeError. No era el viaje (ningún builtin fuera del run 0): era `Gui.bp`. Crear
un control lo apunta en la lista de hijos de su contenedor —`Container.register` → `this.children.add(c)`, código BP,
una `OwnerList` sin cerrojo—, y los dos pintores de la prueba creaban etiquetas en la MISMA pantalla a la vez. Con
dos runs las dos inserciones corren en paralelo de verdad y la lista se rompe; con uno también puede pasar (el cuanto
puede cambiar a mitad de un `add`), pero es mucho más raro. Confirmado aparte (`GuiMismoPadre`: dos hilos, 200
etiquetas cada uno en la misma pantalla): una de ocho pasadas con dos runs, **400 creadas y 399 hijos**: una se
perdió en silencio. La prueba, arreglada para lo que prueba (cada pintor, en su `Panel`); lo de fondo:

| | mecanismo | ejemplo (el patrón de Eduardo: el main monta el formulario; las sondas cambian textos) |
|---|---|---|
| **(A) la librería con cerrojo** | `Container` protege su lista de hijos con un `Mutex` de `Gui.bp` (dar de alta, de baja, recorrerla) | crear controles en el mismo contenedor desde dos hilos, correcto; cuesta un `lock`/`unlock` por control creado o borrado |
| **(B) documentarlo** | «los controles de un contenedor, se crean y se borran desde UN hilo; cambiar textos y valores, desde cualquiera» | lo de Eduardo, sin cambios; dos hilos que crean en el mismo contenedor, un programa incorrecto (como compartir una lista sin `Mutex`) |

La propuesta: (A), porque el programador sólo comparte la pantalla —algo natural— y no tiene forma de saber que crear
un control toca una lista interna de la librería («lo compartido por el programa, es del programador; el resto, nuestro»).

**Decidido: (A)** (Eduardo, 6-oct: *«OK A»*). **Hecho** (`bpstdlib/Gui.bp`): UN cerrojo para todo el árbol
(`cerrojoArbol`, un `Mutex` de módulo creado en su `__init`; `Container` lo pide con `__guiCerrojoArbol()`: una clase no
lee un var de módulo que no sea público), y dentro, con `try`/`finally`, cada operación de la lista: dar de alta, de
baja, contar y coger uno. Recorrer el árbol (`toJson`, `delete`, `clean`) es una serie de ésas, no una sola (un `delete`
dentro de otro se bloquearía: el `Mutex` no es reentrante). Uno y no uno por contenedor: altas y bajas son raras, y un
`Button` también es contenedor. La stdlib, recompilada como PROYECTO (`--project bpstdlib/Stdlib.bpbuild`: compilar
`Gui.bp` suelto le cambió la versión a `Core` —el `gen`, la cabecera— y se deshizo): sólo cambian `Gui.mod` y
`Stdlib.mod`, y `packs/Stdlib.pack`, regenerado (`compat/stdlib_pack.sh --regenerar`). **Medido**: `GuiMismoPadre`
(dos hilos, 200 etiquetas cada uno en la misma pantalla, dos runs), 40 pasadas: con la `Gui` de antes, **7 mal**; con el
cerrojo, 40 de 40. `test-cola` 20 (sin sabotaje por bandera: el fallo era de la librería; el control, esa medida). **En
las placas con pantalla, el `Gui.mod` va DENTRO del pack de LVGL**: el arreglo les llega al regrabar ese pack (el lote
del final de H12).

#### El lote de N1 y del cuelgue (6-oct, mañana; con el OK de Eduardo)

- **La Pico 2** (la imagen de dos núcleos, `vm.runs=2`): la tanda de V7 —35 idénticas, entre ellas las 13 de natives
  CON DOS RUNS (`NatDosRuns`, 3 de 3 thunks ARM), y las de GUI saltadas (sin pantalla)— y `run_placa` 34 de 34; en su
  log, ningún programa mandado a un run.
- **La P4** (la aplicación; los packs aguantaron otra vez): la tanda de V7 —38 idénticas: `GuiAnidadaGc` en la pantalla
  de verdad (el puente aparcado: en el log, el run 1 con 1 427 cuantos reservando mientras el 0 corría el `invoke`) y
  `NatDosRuns` con las natives RISC-V—; y con `log=1` (el OK de Eduardo; quitado después), los VIAJES al run 0 de las
  de GUI: `GuiWinJson` 1, `GuiShot` 15, `GuiArbolG2` 7; y ningún builtin de la GUI fuera del run 0.

**N1 en los micros, hecho** (6-oct, madrugada; compilan las dos imágenes de la Pico, las cinco ESP y las dos STM32):
un PUNTERO por tarea a su contexto (`bpvm_aot_contexto_t`: el límite de fallos y el contexto de la llamada), no el
contexto entero en cada tarea —lo que costaría en todas, también en las del sistema—. Lo ponen las tareas que corren
natives, cada run (`bpvm_repl_run_tarea`) y cada ejecutor (`ejecutor`), al empezar; las demás, sin puntero, usan el de
siempre. La Pico (`BPVM_AOT_POR_TAREA`, en sus dos imágenes): el puntero por tarea de FreeRTOS, el 0
(`configNUM_THREAD_LOCAL_STORAGE_POINTERS` 1: 4 bytes por TCB). La P4 y la S3 (las ESP32 de dos núcleos): un `__thread`
de 4 bytes —el IDF le da a cada tarea su zona TLS en la pila (`uxInitialiseStackTLS`), y la imagen lo confirma:
`t_aot_contexto` en `.flash.tbss`, al lado de `errno`—. La C3, la C5, la C6 y las STM32 (un núcleo, un run: Eduardo),
sin él. Y `un_solo_run` deja de mandar a un run los programas con natives en las imágenes que lo tienen
(`BPVM_AOT_CONTEXTO_POR_TAREA`: el PC, la Pico, la P4, la S3); SQLite y el depurador, con uno. Las vistas que el run y
el ejecutor se guardan y reponen al pasarse el testigo sobran ahora, como en el PC, y se quedan. **En la placa, por
probar** (con el OK de Eduardo): `NatDosRuns` y `GuiAnidadaGc`, a la tanda de V7 (`tools/tanda_v7.json`, 44
pruebas); contra el micro simulado con dos runs, `GuiAnidadaGc` idéntica (las natives, saltadas: el simulado no ejecuta
`.mdn`). La familia
ESP32 tiene `__thread` de verdad (el IDF le da a cada tarea su zona TLS): basta encenderlo en su build (la STM32 no:
su `__thread` es el emulado, `#472`). La Pico, con los punteros por tarea de FreeRTOS (hoy, 0). Las placas de un núcleo
van con un run (Eduardo): no lo necesitan.

#### Los dos núcleos en la S3 y la P4, y lo de por defecto en V7 (6-oct, mañana)

Eduardo: *«si, activa los 2 nucleos en la S3 y la P4»*; y, preguntado qué imagen y qué `vm.runs` van por defecto
(con lo medido en la Pico 2 delante: un programa de un hilo no gana con el 2º núcleo —la imagen de dos con un run, +1,8 %
en `fib(26)` y +20 % en PrintBench; con dos runs, un 10–14 % más lento— y dos hilos que calculan a la vez, ×1,6):
*«En la Pico y Metro, 2 nucleos y 2 runs. En la P4 y la S3 igual. En el resto 1 nucleo y 1 run»*.

- **Lo de por defecto: un run por núcleo de la imagen.** La familia del arranque común dice sus núcleos
  (`bpvm_boot2_familia_t.nucleos`, su `configNUMBER_OF_CORES`; la STM32, 1) y `bpvm_boot2_repartir` pone esos runs
  ANTES de las claves —`vm.runs` del ENV manda— y antes de la memoria (la Pico reserva ahí la pila del 2º run). Sin la
  clave y sin ENV, igual. `test-boot2` 8 (dos núcleos sin la clave y en una placa virgen: dos; `vm.runs=1` y
  `vm.cores=1`: la vuelta atrás; un núcleo: uno, y con `vm.runs=2`, dos —la 2b—). **La Pico y la Metro: la imagen de
  dos núcleos, la de por defecto** (`BPVM_PICO_NUM_CORES` 2 en su `CMakeLists`; `pico/build`, el que graba
  `flash_pico.py`, la construye ya así); la de uno, la vuelta atrás grabando (`-DBPVM_PICO_NUM_CORES=1`).
- **La S3 y la P4, con los dos**: `CONFIG_FREERTOS_UNICORE=n` en sus `sdkconfig.defaults` —explícito y no borrado: el
  `sdkconfig` está fuera de git, y el de la P4 llevaba la orden de V6, *«un solo núcleo hasta nueva orden»*, que se
  queda escrita con la nueva— y en el `sdkconfig` local (los defaults sólo siembran uno nuevo). Las tareas, SIN
  afinidad, como en la Pico (allí se validó así; el comentario de V6 preveía fijarlas a mano, y la medida de la 3.1 dijo
  que no hace falta). `vm.cores=1` las ancla todas al núcleo 0 sin grabar (`bpvm_set_cores`, de
  `bpvm_boot2_claves`; `esp32_afinidad()` en `esp32/common/afinidad_esp32.h`): el FreeRTOS del IDF no cambia la
  afinidad de una tarea creada, así que cada una nace con la suya —`run` y `run2`, `io`, los ejecutores de las
  natives, los hilos y, en la P4, la del cable y la del log por red—; la `main` de la S3 (la del REPL) la fija el IDF al
  0 siempre. La línea del arranque, la de la Pico: «boot: imagen de DOS nucleos[; vm.cores=1: …]».
- **Lo que se miró antes de encender, en el IDF de esta máquina (v6.0.1)**:
  1. *Los atómicos y la PSRAM de la S3.* En Xtensa la instrucción atómica no vale sobre la RAM externa (el propio IDF lo
     dice en `esp_cpu_compare_and_set`: *«the native CAS instruction cannot be used»*, y lo emula con un cerrojo), y el
     bloque de la VM está en la PSRAM. Los atómicos de la VM que leen-modifican-escriben (`exchange`, `fetch_add`)
     caen todos en la RAM interna: el volcado del log (`s_volcando`, `s_pendiente`, estáticas) y los contadores de la
     cola (`vm->smp` es `&bpvm_la_cola`, estática); los demás son cargas y almacenamientos alineados (las palabras de
     `#660`, los handles, el KILL, el `fin` del run, el anillo de la UART), que no usan esa instrucción y valen en la
     PSRAM. Los objetos de FreeRTOS, en su heap: interno.
  2. *Las cachés de instrucciones de la P4, una por núcleo.* El `.mdn` cargado a la RAM se invalida por dirección
     (`esp_cache_msync`, tipo INST); para la RAM interna, el IDF usa `CACHE_LL_ID_ALL` →
     `CACHE_MAP_L1_ICACHE_MASK`: las de los dos núcleos. En la S3, las natives van a IRAM, que no pasa por caché.
  3. *Ningún cerrojo «de un núcleo»* en lo que se compila para la ESP32: ni `vTaskSuspendAll` ni interrupciones
     apagadas como cerrojo (con el IDF de dos núcleos sólo pararían el propio); la sección crítica del log ya es el
     spinlock del IDF (`#653`).
  4. *Los perros guardianes.* El de tareas de la S3 no se arranca; el de la P4, sin pánico, vigila ahora también la
     ociosa del 1 (un aviso por la consola USB, no por el cable: como ya pasaba con la del 0). El de interrupciones, con
     el 1 también: el IDF lo lleva durante las escrituras de la flash (el otro núcleo, parado en IRAM).
- **Compilan** las cinco ESP (la S3 y la P4 con `CONFIG_FREERTOS_NUMBER_OF_CORES` 2), la Pico (dos núcleos por
  defecto, `pico_multicore` enlazado) y las dos STM32. **Los packs sellados** (el `nm` de los `.elf` de antes y de
  ahora): en la P4, la C3, la C5 y la C6, las páginas de la app no cambian; **en la S3 el texto pasa una página de
  64 KB por 1 868 B** (+2 858 B; la C3, con los mismos cambios y un núcleo, +258: el resto es el FreeRTOS de dos), y el
  rodata se mueve una página: un pack nativo
  sellado en la S3 habría que regrabarlo (mirarlo en la placa antes: `packRamBase` del `INFO` y el `STATE`).
- **Por probar en la placa** (la tarde, con el OK de Eduardo): la S3 y la P4 con los dos núcleos —la tanda de V7,
  `run_placa` y la resistencia, con uno y con dos runs— y la medida de `fib(26)` y PrintBench con uno y con dos; la
  Metro con la imagen de dos; la C3, la C5, la C6, la Nucleo y la DK2, con un run; y los packs de la GUI con el
  `Gui.mod` del cerrojo (la C5, la P4 y la DK2; `GuiMini` en la C6).

#### La guarda de N3, apretada (6-oct, mañana; la red la cazó)

La red de este cambio salió con un rojo: en `test-nat2runs`, el sabotaje de N3 (`--sabotaje-aparcada`: una native que
espera al mundo dentro de sí no apunta su pila de C) **no se cazó** —«etiquetas malas: 0»—. No era la VM: era la
prueba, que se fiaba de UNA pasada para un fallo probable. Medido dos veces: **8 y 6 de 10 pasadas saboteadas rojas**
(de 0 a 11 etiquetas malas de 600 llamadas). Y peor que la guarda: la pasada normal ejercitaba N3 tan pocas veces que, si N3 se
rompía, la red lo habría visto sólo a veces.

**El porqué**: N3 pide que la SEGUNDA reserva de `etiqueta` (con el intermedio sólo en un local de C) encuentre al
otro run colectando. Pero con el GC en cada reserva, la native solía colectar ella misma en la primera y seguía al
instante; el otro run, que salía de esperar el mundo, tardaba en despertar: la carrera la ganaba casi siempre la
native. **El arreglo**, un gancho de prueba como `ensayo_colecta_us`: `ensayo_reserva_nativa_us`, una reserva hecha
DESDE UNA NATIVE espera unos µs —activa y SIN el cerrojo— antes de entrar (`heap_nuevo`). Medido (diez pasadas
saboteadas y cinco normales por valor):

| espera | N3 en la pasada normal (veces) | sabotaje: rojas | etiquetas malas (de 600) |
|---|---|---|---|
| 0 µs (antes) | 388 – 1 573 | 6 de 10 | 0 – 6 |
| 100 µs | 1 891 – 2 158 | 10 de 10 | 358 – 496 |
| **300 µs** (la de la prueba) | 1 857 – 1 921 | 10 de 10 | 501 – 526 |
| 1000 µs | 1 851 – 1 903 | 10 de 10 | 524 – 548 |

Y la receta ya no se fía de que la pasada normal «no falle»: exige que EJERCITE N3 —el colector cuenta las natives
aparcadas que escanea (`nativas_aparcadas`, en la cola y en la línea `runs:`), y la pasada normal tiene que ver al
menos 100 (ve ~1 900)—. El control: con el sabotaje, y en la pasada interpretada, la cuenta es 0. La prueba no se
alarga (4–6 s por pasada).

#### El lote de los dos núcleos (6-oct, tarde; con el OK de Eduardo)

Las nueve placas, con la imagen de `1558123c`: la S3 y la P4 con los dos núcleos encendidos, la Pico 2 y la Metro con
la imagen de dos (ya la de por defecto), y TODAS sin `vm.runs` en el ENV —lo de por defecto: dos runs en las de dos
núcleos, uno en las demás; a la P4 y a la Pico 2 se les quitó el `vm.runs=2` que llevaban, y su `INFO` dice `runs: 2`
igual—.

| placa | núcleos / runs | la tanda de V7 (44) | `run_placa` | además |
|---|---|---|---|---|
| P4 | 2 / 2 | 38 idénticas, como antes | 34 de 34 | resistencia de 110 min SIN FALLOS (1 160 vueltas, 29 RESET; el KILL a un dormido, 25 ms de mediana); `GuiMismoPadre`, 5 de 5 (400 hijos) |
| S3 | 2 / 2 | 22 idénticas (las de GUI y las de natives, saltadas: sin pantalla, y Xtensa sin motor) | 34 de 34 | resistencia de 110 min SIN FALLOS (536 vueltas, 13 RESET; 28 ms) |
| Pico 2 | 2 / 2 | 35 idénticas (las de GUI, saltadas) | 34 de 34 | — |
| Metro | 2 / 2 | 35 idénticas | 34 de 34 | — |
| C5 | 1 / 1 | 38 idénticas, con su pantalla | — | — |
| C6 | 1 / 1 | 37 idénticas (`GcHondo` y `synclisttest`, saltadas: su heap) | — | — |
| C3 | 1 / 1 | 34 idénticas (ídem, 48 KB de heap) | — | — |
| Nucleo | 1 / — (sin tarea `run`) | 35 idénticas | — | — |
| DK2 | 1 / — | 38 idénticas, con su pantalla | — | — |

(En todas, `MachineId` y `GuiShot` no-peta, y `KillLoop`, `NatKill` y `NatRecurKill` paradas: sus criterios.) **Lo medido**
con uno y con dos runs en la imagen de dos núcleos (`fib(26)`, mediana de 5; PrintBench, de 10): en la P4, 2 520 y
2 500 ms —el 2º run no le cuesta a un programa de un hilo—; en la S3, 8 084 y 8 297 (+2,6 %; en la Pico 2 era un 10–14 %,
la 3.2). PrintBench, el mismo con uno y con dos en las dos (~20,5 s): lo marca la UART del cable, no la CPU.

**Los packs**: en la P4, el `Gui` nuevo (el del cerrojo) detrás del viejo —gana el último; el viejo queda tapado—. En la
C5, la imagen MOVIÓ el bloque de RAM de los packs una página (0x420A1000 → 0x420B1000): `Gui` y `DriverSpi`, «realojado
para OTRA dirección», regrabados —el `nm` de la mañana se miró contra el build de la madrugada, no contra la imagen que
llevaba la placa, que era más vieja: lo que la nota de la memoria ya decía—. En la C6 y la DK2 el pack de la GUI nuevo
no cabía en su zona (488 KB libres para 512; 304 para 752) y un `packdel` no libera la flash: `PACK_FORMAT` (OK de
Eduardo) y los tres regrabados —la C6: Stdlib, DriverSpi y GuiMini; la DK2: Mini, SQLite y Gui—; sus pruebas de GUI,
otra vez, bien. La S3 no tiene packs: que su texto pasara una página no le tocó nada. Y en el `/app` de la P4, los tres
ficheros de prueba, borrados (OK de Eduardo).

**Lo que quedaba de H12** —el pack de la GUI con el cerrojo a las placas con pantalla, y el resto de placas una vez—,
**hecho**. Para cerrar, Eduardo.

#### Las medidas finales: uno y dos núcleos, la Pico 2 y la P4 (6-oct, tarde; con el OK de Eduardo)

Eduardo: *«los tiempos finales de la Pico y la P4 con 1 y con 2 núcleos… no sé si algo ha cambiado desde entonces»*.
Con las imágenes de `1558123c` (las del 5-oct eran anteriores a las natives con dos runs y a la GUI con dos runs): un
hilo —`fib(26)`, mediana de 5; PrintBench, de 10— y DOS HILOS A LA VEZ —`ParaleloPico`, recompilado: cada carga dos
veces en el `main` y en dos hilos; aquí, lo que tardan los dos trabajos de la mejor manera que tiene cada configuración
(medianas de 3)—. «Un núcleo» es, en la Pico, la imagen de un núcleo (la de V6, grabada un momento y vuelta a la de
dos), y en la P4 la vuelta atrás sin grabar (`vm.cores=1` y `vm.runs=1`: el FreeRTOS de dos núcleos, todas las tareas en
el 0). Las claves del ENV, quitadas después: las dos placas, con lo de por defecto.

**La Pico 2** (ms):

| | un núcleo | dos, 1 run | dos, 2 runs |
|---|---|---|---|
| `fib(26)` (un hilo) | 2 866 | 2 918 (+1,8 %) | 3 108 (+8,4 %) |
| PrintBench (un hilo) | 817 | 1 275 | 1 270 |
| dos hilos: cálculo | 2 186 | 2 225 | **1 312 (×1,67)** |
| dos hilos: arrays | 3 062 | 3 110 | **1 826 (×1,68)** |
| dos hilos: objetos | 2 574 | 2 661 | **1 666 (×1,55)** |
| dos hilos: cadenas | 1 233 | 1 318 | 1 483 (×0,83) |

**La P4** (ms):

| | un núcleo (`vm.cores=1`) | dos, 1 run | dos, 2 runs |
|---|---|---|---|
| `fib(26)` (un hilo) | 2 531 | 2 520 | 2 500 |
| PrintBench (un hilo) | 20 534 | 20 545 | 20 373 |
| dos hilos: cálculo | 1 957 | 1 949 | **974 (×2,01)** |
| dos hilos: arrays | 2 840 | 2 829 | **1 433 (×1,98)** |
| dos hilos: objetos | 2 543 | 2 532 | **1 288 (×1,97)** |
| dos hilos: cadenas | 658 | 614 | **494 (×1,33)** |

- **La ganancia con dos hilos, la del 5-oct en la Pico** (×1,67, ×1,67 y ×1,55) y **mejor en las cadenas** (de ×0,65 a
  ×0,83: el arreglo de `mundo_cond`). **En la P4, el doble de verdad** (×2 en cálculo, arrays y objetos; ×1,33 en
  cadenas): cada núcleo con su caché de instrucciones, y la PSRAM con su caché; en la Pico los dos núcleos comparten los
  16 KB de la caché de la flash.
- **Un programa de un hilo**: en la P4 no paga nada por el 2º núcleo ni por el 2º run; en la Pico, un 1,8 % por la
  imagen de dos núcleos y un 8,4 % con dos runs.
- **Una cifra que no cuadra, sin explicar**: PrintBench con la imagen de dos núcleos de la Pico, 1 270 ms hoy frente a
  1 063 el 5-oct (+19 %), cuando con la de uno ha bajado (de 889 a 817). Mirado: lo de estos días no pasa por el camino de
  `print` (el contexto de la native por tarea, N1, sólo se lee al llamar a una native, en el GC y en `Heap.compact`).
  En la Pico PrintBench ya osciló un ±10 % entre imágenes por dónde cae el código en la caché de la flash (la A/B del
  5-oct), pero eso no lo explica todo. A mirar con una A/B de las imágenes intermedias. En la P4, PrintBench lo marca la
  UART del cable (~20,5 s), no la CPU.


## H8 — IDE y wire: el MAPA VERIFICADO (28-sep, para decidir el orden)

Eduardo: *«Vamos a continuar por H8.»* Cuatro lecturas del código de hoy, y lo que decide el plan comprobado a mano.
Lo que dice cada asunto contra lo que hay:

| asunto | lo que hay hoy | tamaño | ¿decide Eduardo? |
|---|---|---|---|
| `#538` — el IDE y las dependencias | `resolveDeviceDeps` (`FrmMain`) recorre con una regex las líneas `import X` de los `.bp`: no ve el `Core` implícito, se equivoca con `import I:M` (coge `I`) y con `import acme.Lib` (coge `acme`), y corta la cadena si una dependencia sólo existe como `.mod`. El `.mod` MOD9 SÍ lleva sus dependencias (la subtabla `deps`, `Core` implícito incluido): es lo que lee la VM al cargar. `statModule` ya decide bien con un pack (`STAT` → `pack:`; mismo CRC, no sube; distinto, sube delante). Con el `Stdlib.pack` cargado, **`/lib` está vacío** (Eduardo): es el caso normal, y el que prueba el arreglo. | medio: sacar las dependencias del `.mod` (hay un lector, package-private: `PackStep.duenosDeImports`) | no: es un fallo |
| `#495` — `GET`/`LS` durante un RUN | La premisa 1 de la ficha NO se cumple: durante el RUN el cable lo atiende el hilo `io` (`io_loop`: saca la salida y luego llama al `poll`, en el mismo hilo), así que un `print` no puede colarse entre la cabecera de un `GET_REPLY` y sus bytes. Lo que SÍ pasaría al levantar el `BUSY`: mientras dura un `GET` (60 KB, ~5 s en la DK2) `io` ni vacía la salida —el programa se para al imprimir— ni lee un `KILL`; y el cerrojo del FS abarca el envío entero (littlefs y FAT: el `readFile`/`writeFile` del programa espera). `LS` es corto. `LIST_DIR` usa la zona de rascar global, la misma que el `listDir` del programa: en paralelo, carrera. Son cuatro `BUSY` (Pico; ESP32 —S3, C3, C5, C6 y P4—; STM32; micro simulado), no cinco. | pequeño si sólo `LS`; grande si `GET` | sí: ¿`LS` ya y `GET` troceado (entre trozo y trozo vaciar la salida y mirar el `KILL`, con el cerrojo del FS por trozo), o `GET` sólo sin RUN? |
| `#452` — los cinco verbos | `PING`, `DF`, `FORMAT`, `RENAME`, `RMDIR`: ni el IDE ni `tanda.py` los mandan, pero SÍ los ejercita `sim_smoke` contra el micro simulado, en la red (`check-sim`): ya no están sin probar. El IDE NO puede formatear el FS de una placa (su «Formatear zona…» es `PACK_FORMAT`, la zona de packs). `pico/scripts/bpvm-pico.py` manda `FORMAT` en el protocolo de texto retirado en `#305`: muerto. | pequeño | sí: ¿`FORMAT` en el IDE, con confirmación?, ¿`DF` fuera (el IDE lo saca de `INFO` + `LIST`)? |
| `M5` — las properties heredadas en el depurador | NO es una comprobación: el depurador no despliega NINGÚN objeto (un local objeto sale como `@<valor>`; el `INSPECT` de miVM es mínimo; la VM-C no lo tiene; la tabla de variables es plana). Haría falta el layout de cada clase en el `.dbg` (padre, campos, properties, offsets; con el padre en otro módulo los offsets los recalcula el enlace tardío al cargar), desplegar desde el IDE leyendo la memoria (`READ_INT`, que la VM-C ya tiene) y la vista en árbol. | grande: una función nueva | decidido (1-oct): **a V8** —Eduardo: *«M5, no entra en V7, lo pasamos a V8. Es una mejora, no es urgente»*— |
| `E1` — el rojo del árbol | No existe: `colorDeTipo` sólo mira la extensión y nunca da rojo. Los candidatos a «problema» están en un comentario (`.mdn` de otra arquitectura, `.mod` de ABI incompatible, `/lib` rancio, listado truncado), y el `LIST` manda `crc:-1`. | medio | decidido (1-oct): *«En rojo mostrar módulos/packs dañados o con un formato no válido»* (Eduardo) |
| `E1` — el `EXITED` de un `RESET` | Dice «terminado por KILL»: el `poll` trata `RESET` como `KILL` (devuelve 1 → `BPVM_KILLED`) y reinicia después. Cuatro sitios (Pico, ESP32, STM32, micro simulado); en la STM32 y el simulado sale sólo «(KILLED)». | pequeño | no |
| `E1` — los packs en el árbol | El árbol sale sólo de `LIST`, que no lista la zona de packs (desde `#493` un módulo de pack es `pack:<Pack>/x`, no `/lib`). `PACK_LS`/`PACK_ENTRIES` existen y sólo los usa el panel de packs. | pequeño, en el IDE: una rama `pack:` | sí, poco: ¿rama `pack:` en el árbol? |
| El `INFO` de ESP32 y STM32 | Siguen sin `floatAbi`, `packsXipBase`/`Bytes` ni `sqlite*` (sólo la Pico tiene `info_extra`); el IDE ya los pinta si llegan. `floatAbi` y los packs pueden ir al `INFO` común (sus funciones ya son comunes); `sqlite*`, por familia. | pequeño-medio | no |
| El guardián del fat-jar (`#429`) | `Version.avisoDeDesfase` compara el fat-jar con el `.jar` del compilador; la miVM embebida, nada. Y su aviso, desde la miVM del IDE, se perdería: el stderr de ese proceso va a un `diag` sin sumidero. | pequeño | no |

**Encontrado de paso**: la puerta de «sin FS → `NOT_READY`» es una lista COPIADA en cada familia y las tres difieren
—la ESP32 no para `MKDIR`, `RMDIR`, `RENAME` ni `FORMAT`: sin partición montada, un `FORMAT` pasaría— → `#601`. Y, por
mirar: en el micro simulado, con breakpoints, leen el socket dos (el `dbgw_next_cmd` de la tarea `vm` y el `poll` de
`io`); `Disasm` rotula los MOD9 como «NO MATCH, esperado MOD8»; los comentarios de `repl_esp32.c` y `stm32_repl.c`
siguen hablando de un solo hilo.

**Propuesta de orden**: primero lo que no pide decisión y cierra algo —`#538` (con la prueba de `Run on Device` con el
`Stdlib.pack` cargado y `/lib` vacío), `#601`, el `EXITED` del `RESET`, el `INFO` y el guardián del fat-jar—; después,
con las decisiones de Eduardo, `#495`, `#452` y los packs en el árbol; el rojo del árbol y `M5` son diseño, a hablar.

**Decidido por Eduardo (28-sep, tras el mapa):**

- **`#495`**: *«el get tiene que trocearse y los paquetes de los diferentes comandos se pueden mezclar, tiene que ser
  el destinatario el que vaya recogiendo los diferentes paquetes y los vaya agrupando. En un entorno multitarea no
  puede ser que una comunicación monopolice la comunicación.»* → un protocolo TROCEADO y MEZCLABLE: cada respuesta
  larga en paquetes con su id, intercalados con los `OUTPUT` y con las demás respuestas, y el IDE (o quien lea) los
  junta por id. El diseño, antes de tocar código.
- **`#452`**: *«desde el IDE ya hay un botón para formatear la zona de packs, ¿hacen falta más formats?»* → no: el FS
  se formatea solo al arrancar si no monta (`fs_lfs.c`, «formatea si no monta»), y el `FORMAT` del wire (en caliente,
  con `confirm:"YES"`) sólo lo usan las pruebas contra el micro simulado. Se quedan los cinco verbos como están,
  probados en `check-sim`.
- **Los packs en el árbol de ficheros**: *«solamente si están en el FS; si mezclamos los del FS y los de la zona de
  packs vamos a liar a los usuarios»* → no hay rama `pack:`: un `.pack` en el FS sale como cualquier fichero (ya sale),
  y la zona de packs se ve en su panel.

### `#495` — el cable TROCEADO y MEZCLABLE: el diseño (28-sep, para revisar antes de tocar código)

*«El get tiene que trocearse y los paquetes de los diferentes comandos se pueden mezclar, tiene que ser el destinatario
el que vaya recogiendo los diferentes paquetes y los vaya agrupando. En un entorno multitarea no puede ser que una
comunicación monopolice la comunicación.»* (Eduardo)

**Lo que hay hoy** (leído en el código):

- Un mensaje es una línea JSON y, si lleva `"bulk":N`, N bytes crudos detrás. El protocolo ya admite varias peticiones
  en vuelo y eventos entre respuestas (`BPVM_WIRE_PROTOCOL.md`). `GET_REPLY` es UN mensaje con el fichero entero de
  bulk; `LIST_REPLY` es UNA línea que se escribe a trozos.
- En la placa, el hilo `io` saca la salida y luego mira el cable, en el mismo lazo: un `GET` largo en un `poll` deja
  sin salida y sin `KILL` mientras dura (por eso el `BUSY`). Y el `read_stream` del FS tiene el cerrojo cogido todo el
  envío: el `readFile`/`writeFile` del programa espera.
- En el IDE, un solo hilo lector; el bulk se lee ahí mismo. Una respuesta se empareja por id y **el PRIMER mensaje con
  ese id la completa; los siguientes con el mismo id se tiran en silencio**. Un IDE viejo que recibiera trozos se
  quedaría con el primero sin avisar: el troceado lo tiene que PEDIR el cliente.
- Leen `GET`/`LIST`: el IDE, `tanda.py`, `wire_serie.py`, `sim_smoke`, `sd_smoke`, `list_trunc_smoke`,
  `medir_margen.ps1`. Y la VM Java también los SIRVE (`DebugServer`).
- Límites: una línea entrante, 2 KB; el USB-Serial-JTAG (la C3, la C5, la C6), un anillo de 2 KB que pierde la
  respuesta si el host no lee en 2 s (`#545`); el bulk va SIN cerrojo en la ESP32, el P4 por TCP y la STM32, y el
  cerrojo de la Pico no es recursivo; la pila de `io`, 4 KB; el parser JSON de las placas, 16 pares.

**La propuesta:**

1. **El mensaje, la unidad que no se parte**: línea + su bulk, de una vez bajo el cerrojo del cable (`send_msg` en cada
   transporte: coge el cerrojo UNA vez y escribe las dos cosas). Entre dos mensajes puede ir cualquier cosa: un
   `OUTPUT`, un `BP_HIT`, un trozo de otra respuesta.
2. **`GET` troceado, si el cliente lo pide**: `{"type":"GET","id":7,"path":"/app/x","trozos":true}` → `GET_PART` (id,
   `off`, `bulk` de hasta 1 KB)… y `GET_END` (id, `size`, `crc`). Un error a mitad sale como `ERROR` con ese id (hoy,
   a mitad de un bulk, no hay forma de avisar). Sin `"trozos"`, el `GET` de siempre: `tanda.py`, los smokes y un IDE
   viejo siguen igual; y un dispositivo viejo ignora el campo y contesta el `GET_REPLY` entero, que el IDE nuevo
   también acepta. **`LIST` igual**: `LIST_PART` (unas cuantas entradas, una línea acotada) y `LIST_END` (con
   `omitted`).
3. **Los manda el hilo `io`, UN trozo por vuelta**: la petición deja una transferencia (el fichero abierto, por dónde
   va, el id); en cada vuelta de `io_loop` —sacar salida, mirar el cable, mandar un trozo de cada transferencia viva—
   ninguna monopoliza: entre trozo y trozo sale la salida del programa y llega un `KILL`. En reposo, lo mismo desde el
   lazo del REPL. Un tope de transferencias a la vez (2, por ejemplo); pasado, `BUSY`.
4. **El FS, por trozo**: con el cursor de `#491` —una apertura y el cerrojo por cada `fread`—, el programa no se para
   mientras se baja un fichero. Si el fichero cambia a mitad, `GET_END` lo dice (tamaño o CRC distintos) y el IDE avisa.
5. **Durante un RUN** se atienden los de LECTURA (`GET`, `LIST`, `LIST_DIR`, `STAT`); los que escriben (`PUT`, `DEL`,
   `MKDIR`, `RMDIR`, `RENAME`, `FORMAT`, `SAVE`) siguen con `BUSY`: escribir donde el programa lee es otra conversación.
   `LIST_DIR`, con un búfer suyo y no la zona de rascar global que usa el `listDir` del programa (hoy chocarían).
6. **El destinatario junta**: el IDE guarda las transferencias por id, junta los `GET_PART` por `off` y completa la
   petición con el `GET_END` (comprobando tamaño y CRC); lo demás sigue su camino entre medias.
7. **La VM Java**, igual: también sirve `GET` y `LIST`.

**Por partes, cada una cierra algo**: (a) `send_msg` atómico en los cuatro transportes y el simulado; (b) `GET`/`LIST`
troceados en el REPL común, las transferencias en el lazo de `io` y el cursor; (c) el IDE: el colector por id y pedir
`trozos`; (d) levantar el `BUSY` para los de lectura; (e) la VM Java. La prueba: un smoke contra el micro simulado que
baja 60 KB mientras un programa imprime sin parar —la salida no se corta más de un trozo, el `KILL` llega, el fichero
llega entero y con su CRC—; y en placa, la DK2 (11 KB/s: donde más dura un `GET`).

**Para decidir**: el tamaño del trozo (512 B–1 KB: mandan la Pico y el anillo del USB-JTAG); cuántas transferencias a
la vez; y si el `PUT` entra en el mismo esquema (hoy va troceado, pero con una sola sesión global y un acuse por trozo).

**Decidido por Eduardo (1-oct):**
- **El trozo**: *«512 bytes o 1K está bien. Si el buffer en las placas es pequeño entonces 512, si no 1K»*; y *«en mi
  experiencia trabajar con una pequeña ventana de paquetes mejora el rendimiento»*.
- **Las transferencias**: *«enviar varios ficheros a la vez, en principio no, 1 a 1. Pero si al mismo tiempo que se
  envía un archivo se hace un list o un print, no hay problema»*.
- **El `PUT`**: *«¿también entra? Si se puede, sí»*.
- **Y dos búferes, no uno** (a la vista de la medida de abajo): *«estamos mezclando dos cosas: por un lado un buffer de
  comunicaciones y por otro el buffer de archivos. Un buffer de comunicaciones con una ventana de 4 paquetes de 512 bytes
  son 2K. El buffer de archivos puede estar condicionado por el tamaño del bloque de flash. Yo los separaría.»*

**Lo medido, antes y después del paso 1** (1-oct; 64 KB aleatorios subidos con `PUT_BEGIN`/`PUT_DATA`/`PUT_END`,
bajados con `GET` y comparados byte a byte: iguales en todos los casos):

> ⚠️ **La primera medida de este apartado estaba mal** (de 2 a 9 s con trozos de 8 KB, de 16 a 25 con los de 1 KB, y de
> ahí «el cuello no es el cable, es la placa: ~0,27 s por trozo»). La tomé con el `esperar()` de `wire_serie.py`, que
> lee el puerto 0,2 s enteros por vuelta aunque la respuesta llegue en un milisegundo: con un acuse por trozo, medía
> sobre todo su propia espera. Rehecha con un instrumento que vuelve en cuanto llega el acuse; y el «antes», grabando
> otra vez en la C5 y la Pico la imagen de `6db432e0` (con el OK de Eduardo) y luego la nueva.

| placa | 8 KB, parada y espera (el IDE de hoy) | 512 B, parada y espera | 512 B, ventana de 4 | `GET` de 64 KB |
|---|---|---|---|---|
| C5 (USB-Serial-JTAG) | 0,72 → **0,34–0,52 s** | 5,04 → **0,41–0,81 s** | 5,12 → **0,44–0,59 s** | 0,10 s |
| Pico 2 (USB CDC) | 6,40 → **6,07–6,13 s** | 14,31 → **7,25–7,27 s** | 13,24 → **6,12–6,19 s** | 0,14 s |
| P4 (UART por el CH343) | → 6,19 s | → 7,38 s | (paso 2) | 5,70 s |
| DK2 (UART por el ST-Link) | → 6,32 s | → 7,37 s | (paso 2) | 5,71 s |

(Antes → después del paso 1, `1c19eabb`; el después, en dos tandas. En la P4 y la DK2, sólo el después: ahí manda el
cable —115 200 baudios son 11,2 KB/s, y 64 KB no bajan de 5,7 s—.)

Lo que dice:
1. **El paso 1 hace lo que tenía que hacer**: abrir, añadir y cerrar en cada trozo costaba ~35 ms por trozo en la C5 y
   ~55 ms en la Pico; con el fichero abierto toda la sesión y la flash escrita por bloques, un `PUT` de 512 B cuesta ya
   lo mismo que el de 8 KB de hoy. El trozo pequeño —el que hace falta para mezclar— ya no se paga.
2. **La Pico recibe a 12 KB/s, y eso es lo que la frena, no su flash**: un `PUT_DATA` sin sesión (la placa lee el bulk
   y contesta `NO_SESSION` sin tocar el FS) tarda 5,2 s por 64 KB en la Pico y 0,11 s en la C5; escribir los 64 KB en
   la flash de la Pico añade ~0,9 s. Su cable (`pico/wire_v1.c`) lee byte a byte y, en cuanto la FIFO de recepción del
   CDC se vacía, duerme 5 ms; y esa FIFO es la del SDK, **64 B** a velocidad completa (`CFG_TUD_CDC_RX_BUFSIZE`, en
   `pico_stdio_usb/include/tusb_config.h`): un paquete USB por siesta, 64 B / 5 ms = 12,8 KB/s. El búfer de
   comunicaciones de la Pico, de hecho, es de 64 B.
3. **La ventana pierde bytes en la P4** (esto la medida mala ya lo vio, y sigue en pie: es la placa la que contesta
   «JSON inválido»): mientras la ESP32 escribe en la flash se apaga la caché y las interrupciones que no están en RAM
   esperan; la de la UART no lo está (`CONFIG_UART_ISR_IN_IRAM` apagado en la P4 y la S3), así que la FIFO de 128 B se
   desborda con lo que el PC manda entretanto. Con parada y espera no pasa porque el PC calla mientras la placa escribe.

**El diseño, con los dos búferes separados:**

| | qué es | tamaño |
|---|---|---|
| **búfer de comunicaciones** | los paquetes en vuelo por el cable, en los dos sentidos: hasta 4 de 512 B sin acuse | 2 KB |
| **búfer de ficheros** | lo que llega se junta aquí y va a la flash por bloques; el fichero, abierto toda la sesión (el cursor de `#491`: `bpvm_fs_fopen`/`fwrite`/`fclose`) | un bloque de borrado: 4 KB (RP2350, ESP32), 8 KB (STM32U5) |

Lo que pide el de comunicaciones a cada placa —aguantar 2 KB mientras escribe un bloque en la flash—:

| placa | recepción | hoy |
|---|---|---|
| Pico 2 / Metro | USB CDC: la FIFO de recepción, 64 B (la del SDK) | no pierde —el USB espera: control de flujo del propio USB—, pero **recibe a 12 KB/s**: un paquete de 64 B por cada siesta de 5 ms del cable. La FIFO, a 2 KB, y leerla por bloques |
| C3, C5, C6 | USB-Serial-JTAG, 4 KB (envío: 2 KB, la ventana justa) | sí, por lo mismo |
| S3, P4 | UART, 8 KB de anillo | **no**: la interrupción de la UART a RAM (`CONFIG_UART_ISR_IN_IRAM` y `ESP_INTR_FLAG_IRAM`) |
| STM32 (DK2, Nucleo) | UART, anillo de 256 B | **no**: el anillo, a 4 KB (los paquetes con su cabecera) |
| el micro simulado | TCP | sí |

**El orden** (cada paso, medido con lo de arriba como referencia):
1. ✅ **El búfer de ficheros**, común (1-oct, `1c19eabb`): la sesión del `PUT` con el fichero abierto y la escritura
   por bloques. Medido arriba: un `PUT` de trozos de 512 B ya no cuesta más que el de 8 KB de hoy.
2. ✅ **El búfer de comunicaciones en cada placa** (1-oct, `ae3ca338`). 🧭 Eduardo (1-oct): *«En la Pico hay que
   activar el DMA, si no vas a tener muchos problemas o de rendimiento o de desbordamiento de la FIFO.»* Hablado con lo
   que hay —el cable de la Pico no es una UART sino el USB nativo, que no se desborda (con la FIFO llena la Pico contesta
   NAK y el PC reintenta: la ventana de 4 × 512 B sobre la FIFO de 64 B llegó entera), y el RP2350 no tiene canal de DMA
   para el USB (no hay DREQ del USB)—, decidió una vía por cable:
   - **la Pico**: la FIFO del CDC, de 64 B a 2 KB (`CFG_TUD_CDC_RX_BUFSIZE`), y el bulk leído por bloques. La
     interrupción de TinyUSB la llena de fondo y hace el papel del DMA;
   - **la STM32** (DK2, Nucleo): la RX por **DMA circular** de 4 KB (GPDMA1, canal 11: una lista enlazada de un nodo que
     se recarga a sí mismo; hasta dónde ha escrito lo dice lo que le falta del bloque, BNDT). Si no arrancara, la IRQ de
     siempre sobre el mismo anillo, y el log lo dice;
   - **la S3 y la P4**: la interrupción de la UART en RAM (`CONFIG_UART_ISR_IN_IRAM`), no el DMA (UHCI).

   Medido (64 KB, subidos, bajados y comparados: iguales; el antes, la imagen del paso 1 —en la S3, la del 26-sep,
   anterior a `#495`—):

   | placa | 8 KB, parada y espera | 512 B, parada y espera | 512 B, ventana de 4 | recibir sin escribir (8 KB) |
   |---|---|---|---|---|
   | Pico | 6,07–6,13 → **1,10 s** | 7,25–7,27 → **2,16 s** | 6,12–6,19 → **1,12 s** | 5,20 → **0,23 s** |
   | P4 | 6,20 → 6,20 s | 7,37 → 7,30 s | **fallaba** (sin acuse tras 4608 B) → **6,18 s** | 5,74 → 5,73 s |
   | DK2 | 6,32 → 6,33 s | 7,37 → 7,34 s | **fallaba** (sin acuse tras 64 000 B) → **6,29 s** | 5,78 → 5,78 s |
   | S3 | 5,96 → 6,23–6,38 s | 10,65 → 7,16–7,39 s | (no se probaba) → **6,17–6,19 s** | 5,74 → 5,74 s |

   La Pico pasa a ir al ritmo de su flash (~0,9 s de los 1,1); la P4, la DK2 y la S3, al del cable (115 200 baudios son
   11,2 KB/s: 64 KB no bajan de 5,7 s), y la ventana recupera lo que costaban las idas y vueltas de los trozos de 512 B.
   En la S3, con trozos de 8 KB, 0,3–0,4 s peor que con la imagen del 26-sep: una sola medida del antes y cinco días de
   cambios en medio; sin partir. Sin probar en placa: la Nucleo (el mismo USART1 y el mismo DMA que la DK2).
3. ✅ **El trozo y la ventana, anunciados por la placa, y el segundo intento del IDE** (1-oct, `e97b4c38`):
   - la placa los declara en su cintura (`put_trozo`, `put_ventana`: 512 B y 4 en las cuatro familias y el micro
     simulado) y los anuncia en el `HELLO` (`putChunk`, `putWindow`); un cliente viejo no los mira;
   - el IDE (`BpvmClient`) sube por trozos con esa ventana —hasta 4 sin acuse—; sin anuncio (un firmware anterior, la
     VM-Java), como antes: 8 KB y un acuse por trozo. Lo que cabe en 8 KB sigue subiendo de una pieza;
   - **el segundo intento, del IDE** (Eduardo, 1-oct: *«Sí, que lo haga el IDE»*): uno, desde el principio, y sólo si
     falla el cable —sin respuesta a tiempo, un `FATAL`, `SIZE_MISMATCH`, `NO_SESSION`—; si la placa dice que no con
     nombre, a la primera. Antes de repetir, el IDE espera a que el cable lleve callado 3,5 s en los dos sentidos y manda
     un `HELLO`, que en la placa acaba la subida a medias y la borra;
   - para eso, **un bulk a medias se da por perdido a los 3 s** (`WIRE_V1_BULK_ESTANCADO_MS`, en el contrato): sin
     plazo, la Pico, la ESP32 y la P4 por TCP esperaban para siempre los bytes que faltaban, y el reintento se habría
     leído como datos (la STM32 ya tenía 3 s). El micro simulado, igual, y con el plazo de la línea estancada de las
     placas (300 ms), que no tenía;
   - y **la subida de una pieza que se rompe también se borra**: la regla de la subida rota sólo cubría la de trozos,
     y casi todos los `.mod` suben de una pieza;
   - `wire_serie.py` y `tanda.py` dejan de esperar 0,2 s por respuesta (el instrumento de la medida mala); `puts` de
     `wire_serie.py` sube con la ventana.

   Probado en el micro simulado (`sim_smoke`: el `HELLO`, una subida con 4 en vuelo, el bulk y la línea estancados;
   saboteado el plazo, en rojo) y con el cliente del IDE (`SubidaSmoke`: tres tamaños subidos y bajados, y un trozo
   recortado 16 bytes —un cable que pierde bytes— que llega entero por el segundo intento) en el simulado y en la Pico,
   la P4, la DK2 y la S3: 64 KB en 1,18 s en la Pico y en 6,15–6,25 s en las de UART. El segundo intento tarda de 4,9 s
   (la Pico y la DK2 contestan `FATAL` en cuanto les llega la basura) a 16,7 s (la P4 y la S3: el lazo de la ESP32 tira
   sin `FATAL` una línea que no empieza por `{`, y el IDE espera sus 10 s de silencio).
4. ✅ **El `GET` y el `LIST` troceados y mezclables** (1-oct, `847873f5` y `be628ee3`) — la propuesta del 28-sep (arriba), afinada el 1-oct con el código
   delante y con lo que Eduardo decidió después (un fichero cada vez; un `LIST` o un `print`, entre medias):
   - **El mensaje no se parte**: `wire_v1_send_msg(línea, bulk)` en cada transporte —la Pico, la ESP32 por UART y por
     USB-Serial-JTAG, la P4 por TCP, la STM32 y el micro simulado— coge el cerrojo del cable UNA vez y escribe la línea
     y sus bytes. Hoy el bulk sale sin cerrojo (la Pico lo coge, pero aparte de su línea), y con la VM contestando al
     depurador desde su hilo, una línea suya podría caer entre la cabecera y los bytes. El micro simulado no tiene
     cerrojo de cable.
   - **Lo pide el cliente**: `{"type":"GET","id":7,"path":"/app/x","chunked":true}` → `GET_PART` (`id`, `off`, y un bulk
     de hasta `putChunk`, 512 B)… y `GET_END` (`id`, `size`, `crc`: el CRC-32 de lo mandado, el mismo de `STAT`). Un
     error a mitad sale como `ERROR` con ese id —hoy, con el bulk anunciado, no hay forma de decirlo—. Sin `chunked`, el
     `GET_REPLY` entero de siempre (las herramientas, un IDE viejo); un firmware viejo no mira el campo y contesta
     entero, y el IDE nuevo acepta las dos formas. **El `LIST`, igual**: `LIST_PART` (las entradas de un directorio; uno
     grande, en varias) y `LIST_END` (`omitted`). Los nombres, en inglés como el resto del contrato.
   - **Sin acuse en este sentido** (el plan decía «con la misma ventana en el otro sentido»; mirado, no hace falta): la
     ventana de la subida protege a quien RECIBE, que allí es la placa —2 KB que no pueden desbordarse mientras escribe
     en la flash—. En el `GET` recibe el PC: lo lee siempre un hilo del IDE, con sitio de sobra, y cada cable trae su
     control de flujo (el USB contesta NAK, TCP tiene su ventana, la UART espera al escribir). Un acuse por trozo sólo
     añadiría idas y vueltas.
   - **Un trozo por vuelta**: la petición deja una TRANSFERENCIA en el común —el fichero abierto con el cursor de `#491`
     (el cerrojo del FS, por lectura y no todo el envío), por dónde va y el CRC—, y `bpvm_repl_paso()` manda UN trozo.
     Lo llaman el lazo del REPL de cada familia (en reposo; mientras quede, sin dormir) y el `poll` del hilo `io`
     (durante un RUN: entre trozo y trozo sale la salida del programa y llega un `KILL`). Una descarga a la vez
     (Eduardo: «1 a 1»; una segunda, o una con una subida a medias: `BUSY`), y un listado, que sí puede ir con ella. El
     `LIST` avanza un directorio por vuelta.
   - **Durante un RUN** se atienden el `GET` y el `LIST` troceados y `STAT`. Los enteros siguen con `BUSY` (un `GET`
     entero dejaría a `io` sin salida ni `KILL` mientras dura), y los que escriben también: escribir donde el programa
     lee es otra conversación. ⚠️ **`LIST_DIR`, también `BUSY`** (corregido al hacerlo): su foto del directorio sale de
     la zona de rascar, la misma que el `listDir` del programa, y cogerla no tiene cerrojo; un búfer suyo serían ~6,6 KB
     de RAM en cada placa, y que el `listDir` de un programa falle porque el IDE mira un directorio no puede ser. El
     `LIST` troceado ya da el árbol durante un RUN.
   - **Lo que corta una descarga**: lo mismo que acaba una subida —un `HELLO`, un `RESET`, otra subida, un verbo que
     escribe en el FS—, con un `ERROR` con su id (`ABORTED`); el `RUN` no, la descarga sigue en `io`. Y si el fichero
     cambia de tamaño mientras se baja (el programa escribe en él), `GET_END` lo dice (`changed`) y el IDE avisa: lo
     bajado puede no cuadrar. Lo que cambie sin cambiar de tamaño no se ve: habría que releerlo entero.
   - **El IDE junta**: guarda lo que llega por id, junta los `GET_PART` por `off` y completa la petición con el
     `GET_END`, comprobando el tamaño y el CRC. El plazo, de SILENCIO (tantos segundos sin un byte) y no del total: lo
     es desde `#454` (corregido al hacerlo: aquí ponía que un `GET` tenía 10 s en total).
   - **La VM-Java**, igual: también sirve `GET` y `LIST`.

   Por partes, cada una cierra algo: (a) `send_msg` en los cinco transportes y el simulado; (b) las transferencias y
   los verbos troceados en el común, y el paso en el lazo de cada familia; (c) los de lectura durante un RUN; (d) el
   IDE; (e) la VM-Java. La prueba: en el micro simulado, 60 KB bajados mientras un programa imprime sin parar —la salida
   no se corta más de un trozo, el `KILL` llega, el fichero llega entero y con su CRC—; y en placa, la DK2 (11 KB/s:
   donde más dura un `GET`). Lo que hay que medir: la pila del hilo `io` (4 KB en la Pico y la STM32), que pasa a
   correr el `LIST` con su examen y la lectura del FS.

   **Hecho y medido** (1-oct): en el micro simulado, 23 comprobaciones nuevas en `sim_smoke` (y saboteado el troceado
   —la descarga de un paso—, tres en rojo); el cliente del IDE contra el simulado y la VM-Java; y en las cuatro placas
   (con el OK de Eduardo), `paso4_placa.py` y `SubidaSmoke`:

   | placa | 40 KB en reposo: `GET` entero / troceado | troceado DURANTE un RUN que imprime sin parar | a `io` le sobra de pila |
   |---|---|---|---|
   | Pico (USB) | 0,09 / 0,15 s | 1,45 s, con 1616 líneas del programa entre el primer trozo y el último | 1208 B de 4 KB |
   | DK2 (UART) | 3,49 / 3,86 s | 14,17 s, con 1613 | 1208 B de 4 KB |
   | P4 (UART) | 3,49 / 3,84 s | 14,09 s, con 1613 | 700 B de 4 KB → **2748 B de 6 KB** |
   | S3 (UART) | 3,48 / 3,84 s | 14,09 s, con 1613 | 332 B de 4 KB → **2316 B de 6 KB** |
   | C5 (USB-Serial-JTAG; grabada después, ya con 6 KB) | 0,06 / 0,07 s | 0,22 s, con 1606 | 2828 B de 6 KB (su FS, el mayor: 147 entradas) |

   Las 1613 líneas entre trozos son las mismas en todas: `io` saca hasta 256 B de salida por vuelta y un trozo, así
   que el reparto lo da la vuelta, no el reloj. Con el programa hablando, la descarga se lleva más o menos un tercio del
   cable (calculado, no medido: 40 KB de trozos y unos 100 KB de `OUTPUT` —1613 líneas de ~65 B con su JSON— a 11,2 KB/s
   son ~12,5 s, de los 14 medidos). La pila de `io` —con 4 KB, 332 B libres en la S3— pasó a 6 KB en
   la familia ESP32 (`be628ee3`); la Pico y la STM32 se quedan en 4 KB, con un 30 % de margen y el gancho de
   desbordamiento de FreeRTOS. Salió de paso `#617`: la VM-Java no tenía la subida por trozos (`PUT_BEGIN`) y atendía a un cliente por proceso
   (cerrada el mismo día, `b33794df`). Y al grabar la C5, `#618`: el cliente del IDE leía las líneas de un byte en uno, y
   por el USB-Serial-JTAG la bajada troceada perdía bytes (cerrada el mismo día, `34167432`: lee lo que haya).

**La subida que no termina** (1-oct, `ae3ca338`). Medir el paso 1 lo destapó: una subida abandonada (el PC deja de
mandar, se suelta el cable) dejaba el fichero abierto —uno de los dos huecos del FS— hasta el siguiente `PUT_BEGIN`, y
al cerrarlo daba por bueno el trozo subido. Eduardo eligió que se **borre**: *«por tema de espacio, en algunas placas no
vamos sobrados de espacio. Y por otro lado si intentamos subir un archivo y queda la versión anterior el usuario se va a
pensar que tiene el nuevo»*. La acaban un `HELLO` (un cliente nuevo), un `RUN`, un `RESET`, otra subida, un verbo que
escribe en el FS, un error o un `PUT_END` que no cuadra; lo que sólo lee puede ir entre medias. La regla está en el común
(`bpvm_repl_antes_de_despachar`) y cada familia la llama con cada mensaje; el log lo dice («put: X no termino (...):
borrado»). Un corte de corriente a mitad no pasa por ahí: littlefs no da nada por bueno hasta cerrar, así que el fichero
queda como estaba al abrirlo.

🧭 Y Eduardo: *«Aquí la cuestión es si debería haber un 2º intento.»* La propuesta, para el paso 3 (el que toca al
IDE): el segundo intento es del **cliente** —la placa sólo recibe—, **uno**, desde `PUT_BEGIN`, y **sólo si el fallo es
del cable** (sin acuse a tiempo, una respuesta rota, `SIZE_MISMATCH`); si la placa lo rechaza con nombre (`NO_SPACE`,
`INVALID_PATH`, `NOT_READY`), repetir no arregla nada y se dice a la primera. Si el segundo también falla, el IDE lo dice
con el fichero: no está en la placa. A decidir.


### ✅ `E1` — el rojo del árbol: el diseño (1-oct, antes de tocar código; hecho el mismo día, `d7251bd5`)

*«En rojo mostrar módulos/packs dañados o con un formato no válido.»* (Eduardo, 1-oct)

**Quién lo sabe: la VM que tiene el fichero** —la placa, o la VM-Java en el PC—, porque es la que tendría que
cargarlo, y con SUS reglas (qué versiones de `.mod` carga, de qué arquitectura es su código nativo). El IDE no tiene los
bytes de lo que hay en la placa.

**Cuándo: en el `LIST`**, que ya recorre el FS, y sólo en los ficheros cuyo formato conoce la VM —`.mod`, `.mdn`,
`.pack`—, leyendo SÓLO la cabecera (`bpvm_fs_read_at`, hasta 128 B): una lectura corta por fichero.

**Qué cuenta** —lo que dicen la cabecera y el tamaño, sin leer el contenido—:

| | formato no válido | dañado |
|---|---|---|
| `.mod` | un MAGIC que esta VM no carga (`MOD5`, o desconocido) | la cabecera y el tamaño no cuadran: el fichero acaba justo donde acaba `code` (`MOD_FORMAT` §1), así que más corto es truncado y más largo, roto |
| `.mdn` | MAGIC o versión de cabecera desconocidos; de otra arquitectura o de otra ABI de helpers (esta placa no lo cargaría) | más corto que lo que su cabecera dice |
| `.pack` | MAGIC (`BPAK`) o versión de formato desconocidos | el CRC de la cabecera no cuadra; el tamaño que declara no es el del fichero |

Lo que NO se mira, y por qué: el CONTENIDO. Un `.mod` no lleva checksum (un bit cambiado en medio no se ve sin
cargarlo); un `.pack` sí lleva el CRC de su contenido, pero comprobarlo es leer el pack entero —un `SQLite.pack` es
1 MB— en cada `LIST`: lo comprueba la grabación (`PACK_BURN`), que valida la imagen entera.

**Cómo llega**: la entrada del `LIST` lleva `"mal":"<motivo>"` SÓLO si hay problema —las buenas, como hoy: el listado
no crece—. El IDE la pinta en rojo, con el motivo en la ayuda emergente. Las dos VMs, con las mismas reglas y los
mismos textos (la VM-Java también sirve el `LIST`). Y el panel de la zona de packs ya dice «MAL» por pack
(`PACK_LS`, `crcOk`): esa fila, en rojo también.

**La trampa del cerrojo**: el `LIST` emite cada entrada desde el callback del recorrido del FS, y el recorrido tiene el
cerrojo del FS cogido (`be_list`); leer una cabecera desde ahí lo pediría otra vez —el de la Pico no es recursivo: se
colgaría—. Así que, por cada directorio, el recorrido APUNTA los módulos y packs (nombre y tamaño, en un búfer fijo) y
los examina y emite al acabar ese directorio, con el cerrojo suelto; lo demás sale como hoy. Si el búfer se llena, lo
que no cabe sale sin examinar (y el log lo dice).

**Las piezas**: (a) en la VM-C, las tres funciones que examinan una cabecera —la del `.mod` sacada del cargador, para
que la regla sea UNA (`load_buffer_impl` lee la cabecera con la misma función)—, y el `LIST`; (b) la VM-Java, igual;
(c) el IDE: el campo, el rojo y la ayuda, en el árbol y en el panel de packs; (d) las pruebas: el micro simulado con un
`.mod` cortado, uno `MOD5`, uno con bytes de más, un `.mdn` de otra arquitectura y un `.pack` con la cabecera tocada →
el `LIST` los marca y a los buenos no; y la VM-Java, lo mismo.

## H6 — LVGL en un pack: lo que hace falta (22-sep, día de análisis)

El encargo de Eduardo: *«Vamos a analizar qué necesitamos para meter LVGL en un pack. De momento nada más
analizar.»* La forma del hito ya está decidida (`V7_BACKLOG` §H6): **`H6.a` = el pack en la C5** (sólo puede
añadir; las tres imágenes con LVGL no se tocan) y **`H6.b` = quitar LVGL de C6/P4/DK2**, después y tras una ronda
sellada como punto de restauración. Aquí va lo que hay medido, el diseño, las preguntas para Eduardo y el plan.
La sección §1 de este mismo fichero («Código nativo empotrable») es la base: la reja de `publica()`, la
resolución perezosa, las ranuras, la arena por código nativo. Esto es su primer cliente.

### 1. Lo que hay hoy, medido

**Lo que cuesta la GUI dentro de una imagen** (mapas del enlazador, C6 con GUI+LVGL contra C5 sin nada, 22-sep):

| pieza | flash | RAM estática | de qué |
|---|---|---|---|
| **LVGL 9.2.2** (`liblvgl.a`) | **643 711** | 604 | fuentes Montserrat 12…48: **418 060** (65 %) · motor+widgets+draw ≈ 200 000 · lodepng 26 504 |
| `src/gui.c` (modelo + render) | 10 844 | **105 936** | `g_nodes[512]` × ~207 B: el MODELO, en `.bss`, en toda imagen con `BPVM_GUI` |
| driver del panel (`gui_display_st7789.c`) | 1 106 | 12 | el C6; el del P4 (DSI + GT911) son 729 líneas |
| los 83 builtins `GUI_*` | ≈ 4 700 | 400 | `builtins.c.obj`: 26 220 con `BPVM_GUI` contra 21 546 sin él |
| **imagen entera** | C6 **1 175 884** · C5 **511 526** | C6 161 185 · C5 54 440 | la GUI es **664 KB de flash y 107 KB de RAM**: más de la mitad de la imagen del C6 |

Hoy la C5 (como C3, S3, Pico, Nucleo) compila **sin `BPVM_GUI`**: los 83 builtins no existen y un programa con
`Gui` muere en el id del builtin. La C6, el P4 y la DK2 llevan `BPVM_GUI` + `BPVM_LVGL` con todo dentro.

**La superficie que LVGL necesita de la plataforma, medida con `nm -u` sobre `liblvgl.a` del C6: 17 símbolos.**
`__assert_func` · libgcc (`__ctzsi2 __divdi3`) · `malloc free realloc` · `memcmp memcpy memmove memset strcat
strcmp strcpy strlen strncat strncpy` · `vsnprintf`. **Nada de `math.h`, nada de ficheros, nada de tiempo** (el
tick se lo da el driver con `lv_tick_set_cb`). Con `lv_conf.h` en `LV_STDLIB_BUILTIN` (TLSF + sus propias
`lv_string`/`lv_sprintf`, que hoy están en `LV_STDLIB_CLIB`) la lista baja a libgcc + `assert`: LVGL es **más
autocontenida que SQLite** (36 símbolos en la prueba A de V5).

**Y nada de coma flotante** (pregunta de Eduardo, 22-sep): con `LV_USE_FLOAT 0` y `LV_USE_MATRIX 0` (lo de hoy)
LVGL calcula en enteros —punto fijo, `lv_trigo_sin`, `lv_value_precise_t` = `int32_t`—. En el archivo hay dos
rutinas de float (`__extendsfdf2`, `__floatunsisf`) y las pide **un solo objeto, `lv_example_scale_7.c`: un ejemplo
que no se enlaza**. Comprobado en el mapa del C6: las 22 rutinas soft-float de libgcc que lleva la imagen
(`__muldf3`, `__addsf3`…) las piden **`interp.c` y `builtins.c`** —el `double`/`float` de BasicPlus—, ninguna LVGL.
Y **la C5 no tiene FPU**: su ELF dice `rv32imac`, *soft-float ABI* (como C3 y C6; el P4 es el que lleva `f`,
*single-float ABI*). Float y double funcionan, por software (libgcc) — hoy ya lo hace la VM ahí. Para el pack da
igual por tres motivos: LVGL no lo usa; si un código nativo lo usara, sus rutinas van DENTRO del `.npk` (SQLite
lleva sus 18 `__aeabi_*`); y por la frontera (BIOS, `bpvm_gui.h`, `'PANT'`) **no cruza ningún float**, todo son
enteros y punteros — así que la ABI sólo importa como peldaño del `.npk` (`float_abi` «ilp32»: un pack del P4 se
rechaza en la C5 con nombre).

**El que habla con el SDK es el driver del panel**, no LVGL: el del C6 pide 28 símbolos (`esp_lcd_*`,
`spi_bus_initialize`, `gpio_config`, `heap_caps_malloc` DMA, `esp_timer_get_time`, `esp_log`); el del P4, 65
(DSI, `esp_lcd_touch_gt911`, LEDC, `i2c_new_master_bus`, LDO, FreeRTOS). Ésa es la frontera que un pack no puede
cruzar por nombre.

**Las dos direcciones del acoplamiento de `gui.c`** (contadas):
- `gui.c` → VM: `bpvm_fs_*` (8 usos: PNG e imágenes, fuentes `.bin`, `.shot`), `bpvm_malloc/free/realloc` (33),
  `bpvm_diag` (4). Nada más: *«gui.c sigue sin saber nada de la VM (objptr opaco)»*.
- VM → `gui.c`: las **92 funciones de `bpvm_gui.h`**, desde `builtins.c` (83 casos), `heap.c` (`visit_roots`, las
  raíces del GC), `bpvm.c` (`reset` al fin de RUN) y `bpvm_repl.c` (`disp_native_size` para el `INFO`). Los eventos
  van por una cola que `GUI_RUN`/`GUI_RUN_ONCE` **drenan** (modelo *pull*): el pack nunca tendría que llamar a BP.

**Lo que el código nativo empotrable tiene hoy** (V5/H3-H4, `bpvm_bios.h`, `bpvm_npack.h`, `bpvm_pack_api.h`):
- La **tabla BIOS v4**: 29 ranuras —log, memoria/cadenas de libc, `malloc/free/realloc` (chivatos en el P4),
  `localtime`, 9 de ficheros, `arena`, `publica/busca`—. **Sin ninguna ranura de hardware** (GPIO, SPI, I2C, tick).
- `publica()` acepta un `const void*` sin mirar marca ni versión (la reja de §1.1 sigue pendiente); **4 ranuras**.
- El ancla, la escalera de 8 peldaños, el sello (`linked_flash`/`linked_ram`) y el barrido: **sólo la Pico y el P4**
  las tienen (`pico/bios_pico.c`+`pack_pico.c`, `esp32p4/main/bios_p4.c`+`pack_p4.c`, 145+188 y 141+248 líneas).
  El mapeo de la zona sí subió al común de ESP32 el 8-sep; la tabla, el ancla y el cargador no. El S3, el C3, el
  C6 y la C5 **no pueden cargar un pack nativo**, y `board_mgr_esp32.c:496` publica `pack_ram_base` bajo
  `#if CONFIG_IDF_TARGET_ESP32P4` (justo lo que H4 §6 llamó «nombrar un silicio en vez de una capacidad»).
- **Una sola RAM y un solo pack**: la RAM del pack es el principio del bloque de la BD (`SQLite=<MB>` en el ENV,
  reservado ANTES que el heap de la VM para que el sello cuadre en cada arranque; `PACK_RAM_BYTES` 8/16 KB de
  estáticos y el resto es la arena, que hoy se llama SQLite); `bpvm_npack_buscar` **barre la zona entera** de 4 en
  4 y **salta al primer** candidato válido; `packs_cargar_una_vez` lo hace en el primer `Run` (decisión de Eduardo:
  no en el arranque, para que un cuelgue no obligue a regrabar).
- Los destinos del `.npk`: `arm-cortex-m33` (Metro/STM32) y `riscv32-esp-p4` (`ilp32f`). **La C5 no tiene destino**:
  publica la receta `RV32IMC_ILP32` (la del C3/C6) y `NpackReloc.porReceta` devuelve null — es el resto de `#502`.
- La C5 ya trae lo que la 1.ª etapa dejó: zona de packs de 13,7 MB mapeada, `SOC_MMU_DI_VADDR_SHARED=1` (como C6 y
  P4: una sola dirección para ejecutar y para leer `.rodata`, que es lo que SQLite ya demostró en el P4), PSRAM de
  8 MB **entera para la VM** (7677 KB): no hay bloque para un pack. ⚠️ El `.npk` tiene que llevar `-fno-jump-tables`
  y `-mno-relax` como el del P4 (recetas de `bpvm_aot_recetas.h`).
- La resolución de módulos (`bpvm.c:663`): `/app` → base → `/lib` → zona de packs (XIP), y **el FS eclipsa al pack**.
  `Gui.mod` (45 038 B) no va embebido en ninguna imagen ESP32: lo sube el IDE a `/lib` en cada Run — lo que taparía
  al `Gui.mod` del pack (el fleco del 23-jul: `MODULE_QUERY`).
- La fachada SPI del ESP32 (`gpio_esp32.c:349`) va **sin DMA** (`SPI_DMA_DISABLED`, `max_transfer_sz = 0`: el FIFO,
  64 B por transacción). Un frame de 240×320 son 2400 transacciones.

**La placa** (docs.waveshare.com y el repo `waveshareteam/ESP32-C5-Touch-LCD-2.8`, leídos hoy):

| | qué | pines |
|---|---|---|
| panel | **ST7789**, 240×320, SPI (el mismo controlador que el C6, con otra geometría y el bus compartido con la TF) | SCK GPIO6 · MOSI GPIO7 · DC GPIO9 · CS GPIO10 (TF: MISO GPIO8, CS GPIO23) |
| táctil | **CST3530**, I2C 0x58, con interrupción | SDA GPIO0 · SCL GPIO1 · INT GPIO5 |
| expansor | **CH32V003** en I2C 0x24, firmware de fábrica | **LCD_RST = EXIO1, TP_RST = EXIO0, backlight = PWM del CH32** |
| resto en el mismo I2C | QMI8658, PCF85063A (RTC), SHTC3 | GPIO0/1 |

⚠️ **Ni el reset del panel ni el backlight son un GPIO**: van por I2C al CH32 (su protocolo está en el ejemplo
`01_ch32_test` del repo; su BSP de ESP-IDF usa LVGL v9). El driver del C6 no vale «con otra geometría» sin más: el
arranque del panel pasa por el expansor. Y el táctil CST3530 no es un componente de Espressif: el mapa de registros
sale de su ejemplo.

### 2. El diseño

#### 2.1 Qué va en el pack y qué se queda en la imagen

**En el pack**, tres cosas, y la frontera es la que ya existe:

1. **LVGL entero + `gui.c` entero (modelo y render) + lodepng + las fuentes**, compilados para el destino con la
   `lv_conf.h` del pack (`LV_STDLIB_BUILTIN`, sin SDL, las fuentes como perilla del build). El modelo viaja con el
   render porque son el mismo fichero y porque así los **106 KB del `g_nodes[]` salen de la imagen** y van a la RAM
   del pack. La paridad del `dumpTree` no cambia: es el mismo `gui.c`.
2. **El driver del panel** —para la C5: ST7789 + CH32 + CST3530— escrito **contra la HAL que le presta la BIOS**
   (GPIO/SPI/I2C/tick), no contra el IDF. Es lo que hace portable el driver y lo que permite que el usuario ponga
   el suyo sin toolchain de Espressif dentro del contrato.
3. **`Gui.mod`** (`#468`), como un `.mod` más del `.pack`: XIP desde la zona por el mecanismo que ya hay.

**En la imagen** se quedan: los **83 builtins** (4,7 KB: son NUESTRO catálogo, no una librería ajena) y un
`src/gui_pack.c` con **~90 trampolines** (las funciones de `bpvm_gui.h` que la VM llama; las `disp_*` de host
—headless, título, salir al cerrar— no cruzan) que implementan `bpvm_gui.h` llamando a una tabla que se resuelve
en el primer uso con `busca('LVGL')`; sin pack, cada trampolín lanza `RuntimeError` **con nombre** («GUI: esta placa no
tiene el pack 'LVGL' grabado») — nunca un 0 mudo. `heap.c`, `bpvm.c` y `bpvm_repl.c` no cambian: llaman a
`bpvm_gui.h` como hoy. Y la BIOS crece (v5).

**En el PC no cambia nada**: el host y el simulador siguen enlazando `gui.c` + LVGL + SDL (el mismo `gui.c`, con
un `#ifdef BPVM_EN_PACK` que cambia `bpvm_fs`/`bpvm_malloc`/`bpvm_diag` por las ranuras de la BIOS); miVM no se
toca; los 92 samples de paridad y `make test-gui*` siguen valiendo. Las tres imágenes con LVGL tampoco (`H6.b`).

#### 2.2 El contrato VM → pack: `bpvm_gui.h`, publicado por nombre bajo `'LVGL'`

La tabla que el pack publica es una `bpvm_pack_api_t` (marca `'LVGL'`, versión, 92 publics **por nombre**) — el
modelo de `SQLI`, sin inventar nada. Los trampolines resuelven los 92 nombres una vez (`bpvm_pack_api_find`) y los
guardan; un nombre que falte se dice. Consecuencia que hay que asumir: **`bpvm_gui.h` pasa a ser una frontera
binaria** como la BIOS — añadir funciones es gratis (se resuelven por nombre), cambiar una firma sube la versión y
un pack viejo se rechaza con nombre. Es la «cuánta reja más» de §1, contestada: marca + versión + nombre.

**Los dos enlaces posibles de BP al pack, y por qué el primero ahora:**

| | **B1 — builtins + trampolines** | **B2 — `native` como `SQLite.bp`** (`import v1 from pack "LVGL"`) |
|---|---|---|
| qué toca | `gui_pack.c` nuevo; `Gui.bp` intacto; miVM intacta | `Gui.bp` reescrito con 90 `native`; un `Gui.mdn` por destino (ARM, RISC-V P4, RISC-V C5; **Xtensa no tiene**); `GUI_RUN` deja de existir en C y el lazo se despacha en BP |
| crecer el catálogo (un widget nuevo) | un builtin nuevo = rehacer las siete imágenes | sólo el pack y `Gui.bp` — el modelo que Eduardo pidió en V5 («si cada librería necesita opcodes se pierde lo que el pack compró») |
| miVM | corre `Gui` como hoy (su `GuiBackend`) | **no corre `Gui`**: miVM no ejecuta `native` sin cuerpo (SQLite tampoco corre en miVM: va por el simulador `make sim SQLITE=1`). Habría que darle a miVM un proveedor de símbolos por nombre |
| el GC | `visit_roots` por trampolín, como hoy | el pack tendría que publicar un visitante que la VM llame por convención: es un builtin con otro nombre |

La tabla publicada es la MISMA en los dos casos: B2 no la cambia, cambia quién la consume. Por eso **B1 ahora**
(cierra en la C5 sin tocar compilador ni miVM) **y B2 queda posible** como paso propio, si el catálogo tiene que
crecer sin tocar imágenes — con la condición de resolver antes qué hace miVM con un `native` de pack.

#### 2.3 El contrato pack → VM: la BIOS v5 (lo que le falta a la tabla)

La BIOS presta hoy memoria, cadenas, ficheros y el punto de encuentro. Para que un driver de panel viva en un pack
tiene que prestar **la HAL BP** —las fachadas que la VM ya tiene y que son las mismas en las cinco familias—:

```c
/* v5 — la HAL, para los drivers de panel y de táctil (y para lo que venga: un sensor, un DAC…) */
void (*gpio_init)(int pin, int mode);  void (*gpio_write)(int pin, int v);  int (*gpio_read)(int pin);
void (*spi_init)(int bus, int sck, int mosi, int miso, int hz, int mode);  int (*spi_write)(int bus, const uint8_t*, size_t);
void (*i2c_init)(int bus, int sda, int scl, int hz);  int (*i2c_write)(int bus, int addr, const uint8_t*, size_t);
int  (*i2c_read)(int bus, int addr, uint8_t*, size_t);
uint32_t (*now_ms)(void);  void (*sleep_ms)(int ms);      /* el tick de LVGL y las esperas del arranque del panel */
```

Son `bpvm_gpio_*`, `bpvm_spi_*`, `bpvm_i2c_*`, `bpvm_platform_now_ms` y `bpvm_platform_thread_sleep_ms`: **cero
código nuevo por familia**, sólo direcciones en `BPVM_BIOS_TABLA`. Lo que LVGL y `gui.c` necesitan ya está:
`log` (el `bpvm_diag` del pack), `abrir/leer/tamano` (PNG, fuentes `.bin`), la `arena` (§2.5). Falta `__assert_func`
y libgcc: van DENTRO del pack, como en SQLite. Y **`publica()` gana la reja** (§1.1): lee marca y versión de la
tabla que le pasan y se niega si no cuadran — una tarde, con test.

**Lo que NO se presta**: `esp_lcd`, `heap_caps_malloc(DMA)`, LEDC. Un driver que los quiera no es portable, y la
tabla no puede tener ranuras que sólo existen en una familia. El atajo existe y se llama `'ESPL'`: que la imagen
del ESP32 publique su `esp_lcd` bajo otra marca y un pack lo busque — sirve para un experimento, no para el
contrato. Propuesta: no abrirlo.

**El coste de ir por la HAL, con número**: la fachada SPI del ESP32 va sin DMA a 64 B por transacción. Un flush de
24 líneas (11 520 B) son 180 transacciones; un frame entero, 2400. A 40 MHz el frame son 31 ms de bus más la
sobrecarga por transacción (del orden de 10-20 µs cada una): **~50-70 ms por frame entero, ~15 fps**; una pantalla
de widgets repinta zonas, no frames. Se **mide en la C5** antes de tocar nada; si molesta, la palanca es **la
fachada** (DMA + `max_transfer_sz` en `gpio_esp32.c`, con un búfer de rebote interno para que el pack pueda tener
sus búferes en PSRAM), que además se la lleva la SD por SPI. Nada de eso cambia el contrato del pack.

#### 2.4 LVGL ↔ el panel: `'PANT'`, la marca que ya estaba prevista

LVGL no sabe de paneles: quiere un `flush_cb`, un `read_cb` del táctil, el tamaño y la rotación. Ése es el contrato
entre los dos códigos nativos, y es pequeño:

```c
typedef struct {            /* lo que publica un driver de panel bajo 'PANT' (versión 1) */
    int  (*init)(void);                                   /* enciende el panel; 0 = bien */
    int  (*size)(int* w, int* h);                         /* el tamaño físico — bpvm_gui_disp_native_size */
    void (*flush)(int x1, int y1, int x2, int y2, const uint8_t* px565);   /* lv_display_flush_ready lo da el que llama */
    int  (*touch)(int* x, int* y);                        /* 1 = pulsado; sin táctil: NULL */
    void (*rotate)(int deg);                              /* MADCTL; sin soporte: no-op con aviso */
    int  (*open)(void);                                   /* bpvm_gui_disp_is_open */
} bpvm_pant_t;
```

LVGL lo pide con **`busca('PANT')` en su primer uso** (`lvgl_ensure_init`), no al cargar: el orden de carga deja
de importar (§1.2), y si no hay panel el error tiene nombre. El mismo contrato tiene **dos proveedores posibles**:
un pack (la C5, el C6, el panel del usuario) o **la propia imagen** — el P4 con su MIPI-DSI y la DK2 con su LTDC
publican `'PANT'` desde `gui_display_dsi.c`/`gui_display_ltdc.c` y LVGL sale de la imagen igual. Eso es lo que
hace viable `H6.b` sin pedirle a la BIOS que preste DSI.

#### 2.5 La RAM: la arena por código nativo, y N packs en vez de uno

> ⚠️ **Corregido al hacerlo (`G1`, 23-sep) — leer §8 antes que esto.** Ni `ram_bytes` en la cabecera
> ni `bp_pack_init(bios, ctx)`: **el reparto es el SELLO** de cada motor, que la placa asigna al grabar
> (el tope), y la arena es común y con dueño. Lo de abajo es el diseño tal como se escribió.

Hoy hay UNA base de RAM (el bloque de la BD) para UN pack. LVGL necesita: sus estáticos (~1 KB), los 106 KB del
modelo, los búferes de dibujo del panel (2 × 11,5 KB) y un heap para los objetos (TLSF de LVGL; para 240×320 con
`Gui.Window` y sus widgets, del orden de 100-200 KB). Y en la C5 la PSRAM entera es hoy de la VM.

- **La reserva sale del ENV con una clave genérica** —propuesta `packRam=<KB>`— reservada ANTES que el heap de la VM
  (como el bloque de la BD en el P4, por el sello). `SQLite=<MB>` se queda como está (es un nombre público) y suma a
  la misma zona, o se documenta como alias: **lo decide Eduardo** (pregunta 3).
- **Cada `.npk` declara cuánta RAM quiere** —`ram_bytes` en la cabecera, formato 2, el campo `reserved[0]`— y el
  cargador reparte la zona **en el orden de la cadena BPAK** (primero grabado, primera base), alineando a 16. El
  reparto es una función pura de la cadena, así que el IDE lo calcula igual al grabar (ya calcula el sello) y el
  sello sigue valiendo. Borrar un pack de en medio mueve los siguientes: la escalera lo dice («regrábalo») y la
  compactación del PC lo arregla — el mismo caso NORMAL que cambiar `SQLite=<MB>`.
- **La entrada crece a `bp_pack_init(bios, ctx)`** con `ctx = {ram_base, ram_bytes}`: el pack coloca ahí sus
  estáticos y el resto es SU arena; `bios->arena()` deja de ser singular. LVGL la usa con su TLSF
  (`LV_MEM_ADR` apuntando a `ctx`), SQLite con MEMSYS5: **el bloque es crudo, el motor lo gestiona** — la regla de
  `bpvm_sqlmem.h`, aplicada al segundo motor.
- **El cargador carga TODOS los `npk` de la cadena**, no el primero, y **recorre la cadena** (`bpvm_pack_find` por
  tipo `npk`) en vez de barrer la zona de 4 en 4 (13,7 MB en la C5). Cada entrada publica lo suyo; `BPVM_PACK_MAX`
  sube de 4 a 8 (una línea; medido: hoy hay 1 marca y mañana 3).

El SQLite existente se **regenera** (BIOS v5 + entrada nueva): su `.npk` de ARM ya estaba rancio (LEEME del 19-ago)
y el pack de MOD9 se rehízo el 15-sep — es una grabación más en la Metro y el P4, no un cambio de diseño.

#### 2.6 La cadena de construcción: `bpstdlib/gui/` como `bpstdlib/sqlite/`

```
bpstdlib/gui/
  Gui.bp  Gui.bpbuild              (out:pack; main no — es librería)
  nativo/src/lv_conf.h             la del pack: BUILTIN stdlib, sin SDL, fuentes por perilla
  nativo/src/lvgl_pack.c           bp_pack_init: verifica BIOS, arena→TLSF, lv_init, publica 'LVGL' (92 nombres)
  nativo/src/gui_pack_glue.c       lo que gui.c pide a la VM, sobre la BIOS (fs, malloc, diag) + __assert_func
  nativo/src/panel_c5.c            'PANT' de la C5: ST7789 + CH32 (reset/backlight) + CST3530, sobre la HAL v5
  nativo/src/build_lvgl_c5.sh      como build_sqlite_rv.sh: -march=rv32imc_zicsr_zifencei -mabi=ilp32 …
  nativo/lvgl.npk.RV32IMC          ≈ 650 KB (LVGL 620 + gui 11 + panel 5 + glue); la tabla de relocs la come el IDE
  out/Gui.pack                     npk + Gui.mod (+ un día fuentes .bin como recursos, spec §5)
```

`src/gui.c` se compila DOS veces desde la misma fuente (host con `bpvm_*`, pack con la BIOS): un solo fichero, dos
`#ifdef`. `NpackReloc` gana el destino **`riscv32-imc`** (`RV32IMC_ILP32`, ilp32 sin FPU: los mismos relocs que el
P4, otra receta) — que es exactamente el resto de `#502` y sirve al C3 y al C6. `PackStep` ya recoge `npk` del
`outDir`. El pack se graba con el botón de siempre; `PACK_BURN_BEGIN` ya da al IDE el sitio de RAM (`pack_ram_base`),
que en la C5 hoy es 0.

#### 2.7 Lo que se verifica, y en qué orden (el método de V5: el mini antes que el motor)

1. **`mini.npk` en la C5** (el control de 20 líneas de V5: tabla BIOS válida, `log`, `.rodata`, `.data/.bss`, devuelve 0).
   Sin esto un fallo de LVGL no se distingue de un fallo del cargador. Es la primera prueba de P1.
2. **`test_bios`** con las ranuras v5 y la reja de `publica()`; **test de trampolines** en host con una tabla falsa
   («sin pack → error con nombre», «con tabla → llama»); la escalera con dos `npk` en la cadena y el reparto de RAM.
3. **El oráculo del `.npk`** (`NpackBuild` re-enlaza en la dirección destino y compara byte a byte) sobre el de LVGL.
4. **En la C5**: `GuiDemo`, `GuiClickMeas` (la latencia de un clic, que `#434` ya mide) y `GuiShot` en la tanda, con
   cronómetro: ms por flush de 24 líneas, ms por frame, RAM del pack en uso. Números, no impresiones.

### 3. Las preguntas (propuesta; Eduardo decide)

| # | pregunta | propuesta |
|---|---|---|
| 1 | el enlace BP → pack | **B1** ahora (builtins + trampolines; miVM y compilador intactos); B2 (`native`) como paso propio después, si se quiere crecer sin tocar imágenes — y antes decidir qué hace miVM con un `native` de pack |
| 2 | ¿un pack o dos (LVGL y `'PANT'`)? | el contrato `'PANT'` desde el primer día y **un solo `.npk` para el primer encendido** de la C5; dos `npk` en cuanto el cargador cargue N (P4 del plan). Así el primer encendido no espera al cargador nuevo y no se pinta nada en un rincón |
| 3 | la RAM | `packRam=<KB>` genérico en el ENV, reservado antes que la VM; `ram_bytes` en la cabecera del `npk`; `SQLite=<MB>` se documenta como alias de la misma zona. Defecto en placas con PSRAM: 0 (nada se reserva sin pedirlo, como hoy la BD); la C5 con pantalla pide ~512 KB |
| 4 | el driver del panel | **contra la HAL v5 de la BIOS**, portable; la fachada SPI del ESP32 gana DMA sólo si la medida lo pide (y beneficia a la SD). El atajo `'ESPL'` (esp_lcd desde la imagen) no se abre |
| 5 | las fuentes | perilla del build del pack; en la C5 las once (418 KB de 13,7 MB); un pack «pequeño» (12/14/16/20/24: ≈ 90 KB) para placas con menos zona. Fuentes `.bin` como recursos del pack, después |
| 6 | el tamaño de pantalla ANTES del primer Run | hoy el pack se carga en el primer `Run` y el `INFO` publica `screenW/H` sólo con LVGL dentro: la tanda y el IDE preguntan antes. Propuesta: el `INFO` también dispara `packs_cargar_una_vez` (es «una vez» y ya es reintentable); la política de no cargar en el arranque no cambia |
| 7 | `Gui.mod` y el Run del IDE | el Run **no sube a `/lib` lo que la zona ya sirve** (el verbo `MODULE_QUERY` del 23-jul: la VM contesta «¿tienes este módulo y por dónde?»). Va dentro de H6: sin él, cada Run tapa el `Gui.mod` del pack con una copia en `/lib` (funciona, pero la RAM que `#468` quería ahorrar no se ahorra) |
| 8 | la reja de `publica()` y las ranuras | en P1: leer marca y versión de la tabla, `BPVM_PACK_MAX` 4 → 8, test |
| 9 | `H6.b` | aparte y después (la ronda sellada): el C6 → su ST7789 al pack de la C5 (mismo driver, otra geometría, sin CH32); el P4 y la DK2 → `'PANT'` publicado desde la imagen (DSI/LTDC no se prestan); LVGL sale de las tres |

### 4. El plan, por pasos que cierran algo

| paso | qué | cierra |
|---|---|---|
| **P1** | el pack nativo llega a **toda la familia ESP32**: `bios_p4.c`/`pack_p4.c` → `esp32/common/` (BIOS, ancla, cargador; `pack_ram_base` sin `#if P4`); la reserva `packRam` en el arranque común (la C5, de su PSRAM); el destino `riscv32-imc` en `NpackReloc`; la reja de `publica()` y 8 ranuras; **`mini.npk` devuelve 0 en la C5** | `#502` resto, §1.1 (reja), §1.3 (ranuras); «packs nativos en C3/C6/C5» |
| **P2** | **BIOS v5** (la HAL: gpio/spi/i2c/tick/sleep) con `test_bios`; `src/gui_pack.c` (92 trampolines, error con nombre) y los 83 builtins en la C5 bajo `BPVM_GUI_PACK`; el `INFO` con el tamaño del pack (pregunta 6); SQLite regenerado contra v5 y grabado en Metro/P4 | el contrato en los dos sentidos, probado en host |
| **P3** | **el pack**: `bpstdlib/gui/` (lv_conf del pack, `gui.c` bajo `BPVM_EN_PACK`, `lvgl_pack.c`, `panel_c5.c` sobre la HAL —ST7789 + CH32 + CST3530—, el build), `Gui.pack` con `Gui.mod`; grabado en la C5; `GuiDemo`/`GuiClickMeas`/`GuiShot` con cronómetro; referencia es/en («la pantalla llega por el pack») | **`H6.a`** |
| **P4** | la disciplina de §1: `ram_bytes` en la cabecera y `bp_pack_init(bios, ctx)`, el cargador de N `npk` por la cadena, `'PANT'` como `npk` propio con `busca` perezoso; el IDE no sube lo que la zona sirve (`MODULE_QUERY`); la tanda con la C5 con pantalla | la fila H6 salvo `H6.b`; `#468` (el `Gui.mod` en flash) |
| **H6.b** | tras la ronda sellada de mitad de V7: C6 (pack), P4 y DK2 (`'PANT'` desde la imagen) sin LVGL dentro; medir lo que cada imagen adelgaza (el C6: −664 KB, −107 KB de RAM) | `H6.b` |

**Los riesgos, dichos antes**: (1) la velocidad del flush por la fachada SPI sin DMA —se mide en P3; la palanca es
la fachada—; (2) el protocolo del CH32 y los registros del CST3530 salen del ejemplo de Waveshare, no de un
componente del IDF: son dos ficheros que leer antes de escribir `panel_c5.c`; (3) el `.npk` de ~650 KB con las
fuentes: el realojado ya se hizo con 618 KB en el P4, y el oráculo lo comprueba byte a byte; (4) dos `lv_conf.h`
(host y pack) son dos sitios que divergen — una sola con `#ifdef BPVM_EN_PACK`; (5) `bpvm_gui.h` como frontera
binaria: cada cambio de firma es una versión, y hay que quererlo así; (6) el `--no-screen` del simulador y el
`INFO` antes del primer Run (pregunta 6); (7) el `Gui.mod` tapado por `/lib` (pregunta 7) no rompe, pero engaña a
quien mida la RAM.

### 5. Lo hecho el 22-sep (tarde y noche), y lo que MIDIÓ

**P1 — el pack nativo llega a toda la familia ESP32.** `bios_p4.c`/`pack_p4.c` → `esp32/common/`
(`bios_esp32.c`, `pack_esp32.c`, `pack_esp32.h`): S3, C3, C6, C5 y P4 con el mismo código, y
`board_mgr_esp32.c` publica `pack_ram_base` sin `#if CONFIG_IDF_TARGET_ESP32P4`. El bloque de RAM lo
reserva el arranque común (`pack_esp32_ram_init`, era `vm_sqlite_init_psram` del P4) con **dos claves
para la misma zona**: `packRam=<KB>` (genérica, manda si está) y `SQLite=<MB>` (la de V5, con su regla
medida). **La arena empieza detrás de lo que el pack se llevó de verdad** (`s_pack_ram_estaticos`), no
de la constante de 16 KB: sin eso el pack de la GUI —392 KB de `.bss`— lo rechazaba el peldaño TAMAÑO.
Destino **`riscv32-imc`** en `NpackReloc` (el resto de `#502`), y el IDE elige toolchain por
`esRiscv()` en vez de por identidad con el destino del P4. Y **el mini, versionado esta vez**
(`bpgenvm-c/tools/npack_mini/`): diez pasos que se cuentan por el log y devuelven su número si fallan;
en el P4, «mini: TODO OK -> 0».

**La BIOS v5.** Diez ranuras de HAL (`gpio_init/write/read`, `spi_init/write`, `i2c_init/write/read`,
`now_ms`, `sleep_ms`) sobre las fachadas que la VM ya tiene: **cero código por familia**. La tabla
**crece por el final** y el pack exige `version >= la suya` (antes `!=`, y cada ranura nueva obligaba a
regrabar todos los packs); `publica()` gana **la reja** (§1.1) y el registro sube a **8 marcas** (§1.3).
El pin que toma un código nativo queda **de pack**: el fin de RUN no lo libera y un programa que lo pida
recibe «lo usa un código nativo».

**P2 — la costura, GENERADA.** `bpvm_gui.h` es el contrato y `scripts/gen_gui_pack.py` saca de él la
tabla, los **87 trampolines** (`src/gui_pack.c`), lo que el pack publica (`src/gui_pack_publics.c`) y un
doble para probarlo en el PC (`test/gui_pack_stubs.c`). `make test-guipack`: 28 comprobaciones.
`BPVM_GUI_RENDER` = «hay render» (LVGL dentro o por el pack) y es lo que miran el bombeo de `Gui.run` y
el `screenW/H` del INFO — que ahora también dispara la carga «una vez» (pregunta 6).

**P3 — el pack.** `bpstdlib/gui/`: LVGL 9.2.2 entera + **el mismo `gui.c`** bajo `BPVM_EN_PACK` + el
pegamento sobre la BIOS + el panel. Números del artefacto:

| | RV32IMC (C5, panel ST7789) | RISCV (P4, panel nulo) |
|---|---|---|
| flash | 715 308 B (text 275 320 · **fuentes y demás rodata 439 908** · eh_frame 80) | 703 188 B |
| RAM (`.data`+`.bss`) | 392 KB (pool 256 + modelo 106 + búferes) | 372 KB |
| `.npk` con su tabla | **759 996 B**, 2 774 relocalizaciones | 746 452 B, 2 685 |
| `Gui.pack` (los dos `npk` + 6 `.mod` + el `.mdn`) | 1 622 016 B; **819 200 B grabados** tras podar | |

**Y LA PRUEBA EN EL P4, que es la idea de Eduardo** (*«¿se podría probar en la P4 sin desactivar el
LVGL actual? No sé, con un `Gui2.bp` que llame a la LVGL del pack»*). Dos piezas lo hacen posible:

- **`panel_nulo.c`** — un display que no pinta, elegido **en el build** (`build_lvgl.sh p4 nulo`). Un
  panel mudo porque su chip no contesta y uno que no pinta a propósito son cosas distintas, y
  confundirlas sería el fallo silencioso de siempre.
- **`bpstdlib/gui/Gui2.bp`** — la MISMA tabla publicada bajo `'LVGL'`, vista por el otro camino:
  `native … from pack`, sin builtins de por medio. Es a la vez el **primer ensayo del camino B2** de
  §2.2. El puente rechazó con nombre `integer[]` como caja de salida (*«ese tipo de array no cruza a un
  pack: sólo `long[]` y `double[]`, y sólo como caja de salida de un valor de 8 bytes»*), así que el
  tamaño del panel se pregunta por el modelo —`get_width(screen_active())`—, que además prueba que el
  modelo recogió el tamaño nativo que dijo el panel.

`samples/GuiPack.bp` habla con las dos a la vez, y esto es lo que contestó la placa:

```
== el pack ==                          == la imagen ==
  pantalla del pack: 240 x 320           pantalla de la imagen: 480 x 800
  screen handle: 1                       label de la imagen creado
  label 2 dice: hola desde el pack
  y mide 123 de ancho                  [log] pack: gui: pack vivo (LVGL 9.2.2, pool 256 KB)
  boton 3 dice: boton                  [log] pack: gui: lv_init hecho (TLSF sobre el pool)
  texto largo: 64 caracteres           [log] pack: gui: API publicada como 'LVGL'
                                       [log] dep 'Gui2' -> PACK (XIP) · Gui2.mdn del pack (10 thunks)
```

Y `GuiShot` siguió pintando su captura en el MIPI-DSI con el pack cargado (480×800, PNG mirado). O sea:
**dos instancias de LVGL en el mismo proceso** —la de la imagen en su `.bss`, la del pack en su bloque
de PSRAM—, cada una con sus globales, y sólo se mueve la que se llama. Sin el pack, la sonda muere con
nombre: «falta el modulo 'Gui2'».

**Lo que esto deja probado sin la C5**: el realojado de 746 KB y el salto; LVGL arrancando dentro de un
pack con su TLSF sobre el pool estático y su `.rodata` —las fuentes— realojada; `gui.c` con el
pegamento sirviéndole memoria, log y ficheros desde la tabla BIOS; la tabla de 87 símbolos publicada y
resuelta por nombre; el modelo moviéndose; y la convivencia con el LVGL de la imagen.
**Lo que NO**: el panel, el táctil y el tiempo de flush. Eso es la C5 y no hay atajo.

**Lo que queda de H6**: con la C5 en la mesa, grabar su imagen, el `mini.npk.RV32IMC` (el control antes
del motor) y `Gui.pack`, encender el ST7789 y medir el flush; el reparto de RAM entre VARIOS `npk`
(`ram_bytes` en la cabecera, §2.5) para que el pack de la GUI y el de SQLite convivan en una placa
—hoy se cargan de uno en uno—; `'PANT'` como `npk` propio; que el Run del IDE no suba lo que la zona
sirve (`MODULE_QUERY`); y `H6.b`, que va después de la ronda sellada.

### 6. La C5, y `H6.a` cerrada (22-sep, noche)

**La placa pinta y responde al tacto con LVGL viniendo de un pack**, en una imagen que no lleva ni
`gui.c` ni LVGL: los 83 builtins y los ~87 trampolines, y nada más. `GuiShot` pintó su captura EN LA
PANTALLA —confirmado por Eduardo mirando la placa— y `samples/C5Touch.bp` contó cinco pulsaciones
reales llegando a su manejador BP. El pack: 827 392 B grabados, 88 símbolos resueltos por nombre, el
ST7789 arrancado por SPI, el reset y el backlight por el CH32V003 en I2C y el táctil CST3530 leído
dentro del bombeo.

**Lo medido, que era lo que este análisis dejó pendiente:**

| | C5 (pack, SPI sin DMA) |
|---|---|
| repintar la PANTALLA ENTERA (153 600 B) | **143 ms** (~7 fps) |
| repintar UNA ETIQUETA (la zona sucia) | **1 ms** |

O sea: una interfaz **responde**; lo que cuesta es el peor caso. La predicción de §2.3 (≈50-70 ms) se
quedó corta — son 2 400 transacciones de 64 B y la sobrecarga por transacción pesa más de lo estimado.
La palanca, como decía el análisis, es **la fachada** (DMA en `gpio_esp32.c`, que además se la lleva la
SD por SPI), no el driver: el día que la fachada admita transferencias largas, `panel_c5.c` no cambia.

**Los dos fallos del camino, porque enseñan más que el resultado:**

1. **`Gui.mod` con generación 0.** Compilado como proyecto suelto, el sello sale de `BpVM.cfg` y no del
   `.bp` de al lado: el programa murió con «'Gui' (gen 0) es mas antiguo que el que 'GuiShot' necesita
   (gen 7)». El pack lleva ahora los `.mod` **canónicos** de `bpstdlib/`, que es lo que el de SQLite ya
   hacía y estaba escrito en su LEEME.
2. **La placa se reiniciaba MUDA** en el primer `Gui.screen()`. Con la consola sólo en UART0 el pánico
   no se ve, y costó dos vueltas de suposiciones —incluida la mía y la de Eduardo, las dos hacia el
   driver del panel— hasta activar la copia por USB y **leerlo**: `Stack protection fault` en la tarea
   `main`, 44 bytes por debajo del límite. Traducido con `addr2line`, el culpable no era ni el panel ni
   el pack:

   ```
   discover_deps (bpvm.c:684) → bpvm_pack_find / bpvm_fs_stat → lfs_stat_ → lfs_dir_find
     → lfs_dir_fetchmatch → lfs_bd_read → esp_bd_read → esp_flash_read → cache_hal_suspend
   ```

   La **recursión de `discover_deps`** (dos buffers de 256 B de ruta por nivel) sobre littlefs y una
   lectura de flash que **en esta placa no va directa porque el destino está en PSRAM** (el IDF pide
   búfer de rebote y el camino se hace más hondo). Un programa con GUI arrastra el grafo de
   dependencias más profundo que había visto la placa, y los 8 KB no daban. Main task a 16 KB, con el
   volcado escrito al lado en `sdkconfig.defaults`. ⏭️ **Y queda una tarea para H7**: 512 B de buffers
   de ruta por nivel de recursión es mucho para lo que hace, y la profundidad crece con el grafo.

📌 **La receta de la consola queda escrita** en `esp32c5/sdkconfig.defaults`: cuando haya que ver un
pánico, `CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG=y`, reconstruir, leer, y volver a apagarla.
Sin eso un reinicio es mudo, y un instrumento mudo cuesta el doble.

**Lo que queda de H6**: el reparto de RAM entre VARIOS `npk` (§2.5) —hoy la C5 con la GUI no puede
llevar además SQLite—; `'PANT'` como `npk` propio; que el Run del IDE no suba lo que la zona sirve
(`MODULE_QUERY`); la C5 en `tanda.py` como placa CON pantalla; y `H6.b`, que va tras la ronda sellada.

### 7. Generalizar el pack a CUALQUIER placa (22-sep, noche — pregunta de Eduardo)

> *«Para generalizar el pack a cualquier equipo, ¿qué necesitaríamos? Entiendo que los pines y el bus se
> han de poder parametrizar. Y luego tenemos el driver de pantalla: ¿cómo podemos generalizar el driver?»*

#### 7.1 Lo que hay hoy, medido

**El driver es lo pequeño.** En el pack de la C5, `panel_c5.c` compila a **2 161 B de código** (más 23 KB de
`.bss`, que son los dos búferes de dibujo) dentro de un pack de **715 KB**: el **0,3 %**. Todo lo demás
—LVGL, sus fuentes, `gui.c`— es idéntico para cualquier placa de la misma ISA. Ese número es el que
decide el diseño: **lo que varía por placa no debe viajar dentro de lo que no varía**.

**Qué cambia de verdad entre los cuatro paneles que ya tenemos:**

| | C6 | C5 | P4 | DK2 |
|---|---|---|---|---|
| transporte | SPI | SPI | MIPI-DSI | LTDC (paralelo) |
| controlador | ST7789 | ST7789 | EK79007 / ST7701 | — |
| tamaño | 240×240 | 240×320 | 1024×600 / 480×800 | 800×480 |
| gap por rotación | 0/0/80/0 (!) | ninguno | — | — |
| reset | GPIO 21 | **I2C → CH32V003 EXIO1** | GPIO 27 | — |
| backlight | GPIO 22 on/off | **I2C → PWM del CH32** | LEDC, y una placa lo tiene INVERTIDO | — |
| táctil | no tiene | CST3530 (I2C 0x58) | GT911 (I2C 0x14) + transformación | sí |
| extra | — | el bus SPI lo **comparte con la TF** | un LDO para el DPHY | — |

Dos ST7789 por SPI (C6 y C5) y **ya no comparten ni el reset ni el backlight**. Lo que varía no es «el
driver»: son cuatro cosas distintas metidas en el mismo fichero.

**Y ya hay un precedente nuestro, del P4** (`gui_display_dsi.c`, `#311`): un **catálogo** de paneles
—resolución, tiempos, polaridad del backlight, transformación del táctil y la función que crea el IC—
elegido por el ENV (`display=st7701`). Funciona desde V6. Lo que le falta es salir de la imagen: hoy
añadir un panel es recompilar y regrabar el firmware. Su propio comentario lo dice: *«Params pin-a-pin
data-driven = V4»*, o sea que ya se vio y se aplazó.

**Tres cosas que hoy NO se pueden hacer, y son las que bloquean:**
- **Un pack no puede leer el ENV.** La BIOS v5 presta log, memoria, cadenas, ficheros, la arena, el punto
  de encuentro y la HAL — pero **ni una ranura de entorno** (comprobado: cero menciones en `bpvm_bios.h`).
  Un driver en un pack no tiene de dónde sacar sus números salvo compilándolos dentro.
- **Los recursos de un pack GRABADO no se leen.** El overlay de `pack:<Pack>/<fichero>` está atado a
  `vm->run_pack_on` — el pack **en ejecución**—, así que un perfil de placa dentro del pack de la zona
  no se puede abrir. Es el cabo que `V5_IDEAS` ya dejó escrito («la zona de packs debe servir recursos»).
- **Sólo se carga UN `.npk` a la vez** (el primero válido de la zona). Con el driver fuera del pack de
  LVGL harían falta dos: eso mueve el reparto de RAM entre varios `npk` (§2.5) de «pulido» a **camino
  crítico**.

⚠️ **Y un cuarto, que la C5 pone delante de las narices**: el panel toma el **bus** SPI2, y en esa placa
**la tarjeta TF cuelga del mismo bus**. Hoy `Spi.Bus(2, …)` desde BP hace `spi_bus_free` + re-init
(`gpio_esp32.c:340`) — o sea que un programa que abra la TF **mata el panel en silencio**. H4 puso el
estado de los PINES en el común y el pin de un pack sobrevive al fin de RUN; el **bus** no tiene esa
protección. Generalizar «los pines y el bus» incluye decidir **de quién es el bus**.

#### 7.2 Las cuatro capas que «el driver de pantalla» confunde

1. **Transporte** — cómo salen los bytes: SPI, I2C, paralelo 8080, RGB, MIPI-DSI. Hoy la HAL de la BIOS
   presta SPI e I2C; los otros tres **no** y no deberían (son periféricos de una familia).
2. **Controlador** — ST7789, ILI9341, GC9A01, ST7796…: una secuencia de arranque, cómo se direcciona la
   ventana (`CASET`/`RASET`/`RAMWR`) y qué bits de `MADCTL` giran. Es lo MENOS variable: el mismo IC
   aparece en decenas de placas.
3. **Panel** — tamaño, `gap` por orientación (el C6 tiene 80 filas de desfase y la C5 ninguna, ¡con el
   mismo IC!), si va invertido, orden RGB/BGR.
4. **Placa** — pines, qué bus, a qué frecuencia, **cómo se hace el reset**, **cómo se enciende la luz**,
   qué táctil hay y cómo se transforman sus coordenadas.

La (2) es un catálogo corto. La (3) y la (4) son datos. La (4) es donde está toda la rareza.

#### 7.3 La consecuencia inmediata: el driver SALE del pack de LVGL

Es la marca `'PANT'` que §2.4 ya dejó dibujada, y ahora se ve para qué sirve de verdad:

```
  pack de LVGL   (por ISA: RV32IMC · ilp32f · ARM)    ~760 KB   ·  se compila 3 veces, nunca más
  npk del panel  (por PLACA)                            ~5 KB   ·  lo escribe quien tiene la placa
```

LVGL pide `busca('PANT')` **en su primer uso** —no al cargar, así que el orden de los dos packs da
igual— y si no hay panel, lo dice con nombre. El mismo contrato lo puede publicar **la imagen**: el P4
por MIPI-DSI y la DK2 por LTDC siguen dentro, y así LVGL sale de esas imágenes sin pedirle a la BIOS
que preste DSI. Eso es lo que hace posible `H6.b` sin inventar nada.

```c
typedef struct {                 /* lo que publica un panel bajo 'PANT' (versión 1) */
    int  (*init)(void);                                  /* 0 = bien; enciende panel y luz  */
    int  (*size)(int* w, int* h);                        /* el tamaño FÍSICO                */
    void (*flush)(int x1, int y1, int x2, int y2, const uint8_t* px565);
    void (*rotate)(int deg);                             /* MADCTL/gap; sin soporte: no-op  */
    int  (*open)(void);
    void (*backlight)(int por_ciento);                   /* 0..100; sin control: no-op      */
    int  (*touch)(int* x, int* y);                       /* 1 = pulsado; sin táctil: NULL   */
} bpvm_pant_t;
```

#### 7.4 La escalera de generalización — tres niveles, UN contrato

> ⚠️ **Corregido el mismo día por una pregunta de Eduardo — leer §7.9 antes que esto.** Lo de abajo
> habla de «un catálogo de controladores», y eso está mal planteado: no son N drivers, es **UN driver
> genérico y N PERFILES**, que son datos.

Lo que contesta «¿cómo generalizamos el driver?» no es un mecanismo sino **tres**, y lo importante es
que los tres publican `'PANT'`, así que LVGL no se entera de cuál se usó:

**Nivel 0 — EL PERFIL: sólo números, sin compilar nada.**
Un `.npk` GENÉRICO —«controladores conocidos por SPI»— que lee su configuración y se monta solo:

```
display   = st7789        # el controlador del catálogo
lcd.bus   = 2 · lcd.sck = 6 · lcd.mosi = 7 · lcd.dc = 9 · lcd.cs = 10 · lcd.hz = 40000000
lcd.w     = 240 · lcd.h = 320 · lcd.inv = 1 · lcd.bgr = 0
lcd.gap   = 0,0,0,80      # por orientación (el caso del C6)
lcd.rst   = gpio:21       # o "exio:0x24/1" — ver la pregunta 3
lcd.bl    = gpio:22       # o "pwm:0x24/0x05"
touch     = cst3530 · touch.addr = 0x58 · touch.i2c = 0 · touch.swap = 0,0,0
```

Con esto, **la C6 y la mayoría de placas caseras no necesitan que nadie escriba C**. Y el catálogo de
controladores es corto: ST7789, ILI9341, GC9A01 y ST7796 cubren casi todo lo que se compra hoy.

**Nivel 1 — LA SECUENCIA COMO DATOS**, para un controlador que no esté en el catálogo: una tabla
`(cmd, n, bytes…, espera_ms)`. No es invento nuestro — es lo que hacen TFT_eSPI y LovyanGFX, y lo que el
propio `esp_lcd` admite como `init_cmds` en su `vendor_config`. Cabe en un recurso del pack o en el ENV.
Con eso, un panel nuevo entra **sin recompilar el pack**.

**Nivel 2 — UN `.npk` DE PLACA**, para lo que no es declarativo: el CH32 de la C5, el LDO del P4, un
táctil con su propio protocolo, una secuencia con condicionales. Son ~200 líneas de C contra la HAL de la
BIOS, y el ejemplo ya está escrito y probado (`panel_c5.c`).

📌 **Y la C5 dice dónde está la frontera**: su panel es Nivel 0 salvo por dos cosas —el reset y la luz
van por I2C a un expansor—. O sea que la frontera entre el nivel 0 y el 2 se puede mover con MUY poco:
si el perfil sabe expresar «este pin es el bit 1 del registro 0x03 del dispositivo I2C 0x24», la C5
entera cae en el nivel 0. Es la pregunta 3.

#### 7.5 Lo que hay que construir (y no es tanto)

| | qué | por qué |
|---|---|---|
| **A** | **BIOS v6: `env_get(clave, buf, cap)`** — una ranura, de sólo lectura | sin ella un driver no puede parametrizarse; es LA pieza que convierte «un npk por placa» en «un npk para todas» |
| **B** | **el reparto de RAM entre varios `npk`** (§2.5: `ram_bytes` en la cabecera, el cargador recorre la cadena) | con el driver fuera, hacen falta dos packs a la vez. Deja de ser opcional |
| **C** | **`'PANT'`**: el contrato, `busca()` perezoso desde LVGL, y `panel_c5.c` movido a su propio `npk` | es el corte; y lo prueba que el P4 y la DK2 lo publiquen desde la imagen |
| **D** | **el `npk` genérico**: ST7789 + ILI9341 + GC9A01 por SPI, leyendo el perfil | el nivel 0, que es el que le sirve a alguien de fuera |
| **E** | **el bus, con dueño** — que tomarlo desde BP no mate al panel, como ya pasa con los pines. ⚠️ **Corregido por Eduardo el 23-sep (§11)**: el bus es COMPARTIDO; lo exclusivo es el dispositivo | la TF de la C5 comparte bus con el panel: hoy es un fallo mudo esperando |
| **F** | **la HAL: SPI con DMA / transferencias largas** | los **143 ms** de pantalla entera son 2 400 transacciones de 64 B; la palanca es la fachada, y se la lleva también la SD |
| **G** | *(opcional)* **los recursos del pack grabado**, para que el perfil pueda viajar dentro del pack | hoy sólo se leen los del pack en ejecución; el cabo ya estaba escrito en `V5_IDEAS` |

#### 7.6 Lo que NO se generaliza, y hay que decirlo

**MIPI-DSI, LTDC y RGB paralelo no van a salir de la imagen**, y no es una limitación temporal: son
periféricos de una familia, y una ranura de la BIOS que sólo existe en un micro rompe la propiedad que
hace útil la tabla (26 de 29 ranuras eran idénticas en cualquier placa cuando se montó la segunda). Por
eso `'PANT'` tiene **dos proveedores**: un pack o la propia imagen. El usuario con una pantalla SPI —que
es lo que se compra— la pone él; el que quiere DSI usa una placa donde ya está.

#### 7.7 Las preguntas (propuesta; Eduardo decide)

| # | pregunta | propuesta |
|---|---|---|
| 1 | ¿dónde viven los números de la placa? | **el ENV**, que es donde ya vive la config de placa y lo que el P4 usa (`display=`). Con `env_get` en la BIOS, el driver los lee al arrancar. Un perfil en fichero (`/sys/panel.cfg`) es el plan B y no hace falta hoy |
| 2 | ¿el táctil va en `'PANT'` o en su propia marca? | **en `'PANT'`, opcional** (NULL si no hay): es un `npk` menos que construir en el caso normal, y si algún día hace falta mezclar panel nuestro con táctil ajeno, separarlo es una marca más — caben ocho |
| 3 | ¿el perfil sabe decir «este pin va por un expansor I2C»? | **sí, con una notación mínima** (`gpio:21` · `exio:0x24/1` · `pwm:0x24/0x05`): mete a la C5 entera en el nivel 0 y los expansores I2C son el pan de cada día en las placas con pantalla. Si crece más que eso, es nivel 2 |
| 4 | ~~¿el `npk` genérico lleva varios controladores o uno por `npk`?~~ | **contestada en §7.9**: ni lo uno ni lo otro — UN driver genérico (MIPI-DCS sobre SPI) y N **perfiles**, que son datos. El catálogo deja de ser código |
| 5 | el bus compartido (la TF y el panel de la C5) | ~~el bus, **con dueño**, como los pines: quien lo tomó lo tiene, y el segundo recibe un error CON NOMBRE en vez de un panel muerto. Compartir de verdad (dos CS sobre un bus ya abierto) es un paso aparte~~ — ⚠️ **mal planteado, lo corrigió Eduardo el 23-sep (§11)**: el bus se COMPARTE, lo exclusivo es el CS de cada periférico, y el que ordena el uso es un árbitro en la HAL BP |
| 6 | ¿esto es H6 o es otro hito? | **`H6.c`**, después de `H6.b`: `H6.a` ya demostró que el mecanismo funciona; esto es convertirlo en algo que use alguien de fuera, y es justo lo que pide «la base es finita, lo demás en packs» |

#### 7.8 El plan, por pasos que cierran algo

| paso | qué | se prueba con |
|---|---|---|
| **G1** | `env_get` en la BIOS (v6) + el reparto de RAM entre varios `npk` | el mini leyendo una clave; el pack de LVGL y el de SQLite **a la vez** en el P4 |
| **G2** | `'PANT'`: el contrato, `busca()` perezoso, y `panel_c5.c` a su propio `npk` | la C5 otra vez, con DOS packs, dando lo mismo que hoy |
| **G3** | el `npk` genérico por perfil (ST7789 primero) | la **C6** con su ST7789 240×240 y su gap de 80 — sin escribir una línea de C, y esa placa ya funciona hoy con LVGL dentro, así que hay con qué comparar |
| **G4** | el bus con dueño + la fachada SPI con DMA — ⚠️ **rediseñado el 23-sep (§11)**: bus compartido con árbitro (`G4.a`) + DMA (`G4.b`) | los 143 ms bajando, y la TF de la C5 conviviendo con el panel |
| **G5** | el P4 y la DK2 publicando `'PANT'` desde la imagen | `H6.b`: LVGL sale de esas dos imágenes |

**El orden no es caprichoso**: G3 es el que de verdad contesta la pregunta de Eduardo —una placa nueva
sin escribir C— y **la C6 es el banco perfecto** porque hoy funciona con LVGL dentro: si el pack genérico
la pinta igual, el nivel 0 está demostrado contra un oráculo, no contra una opinión.

#### 7.9 La corrección: NO son N drivers, es UN driver y N PERFILES (22-sep, noche)

Eduardo, sobre §7.4: *«imagina que nosotros incluimos 8 drivers en el pack pero el usuario necesita otro
que no está incluido»*. La pregunta destapa que «ocho drivers» es el planteamiento equivocado. Lo que de
verdad separa a un ST7789 de un ILI9341 o un GC9A01:

| | ST7789 | ILI9341 | GC9A01 |
|---|---|---|---|
| direccionar la ventana | `0x2A`/`0x2B`/`0x2C` | **iguales** | **iguales** |
| rotar (MADCTL `0x36`) | mismos bits | **iguales** | **iguales** |
| RGB565 (COLMOD `0x3A`) | `0x55` | **igual** | **igual** |
| secuencia de arranque | ~15 comandos | otros ~20 | ~100 (es famoso) |
| inversión, tamaño, gap, orden de color | datos | datos | datos |

Todo eso es **MIPI-DCS**, un estándar. Lo único que cambia de verdad es **la secuencia de arranque y unos
números**. Así que:

> **UN driver genérico —«MIPI-DCS sobre SPI de 4 hilos»— y N perfiles, que son DATOS** (100-500 B cada
> uno). Un driver + nueve perfiles ≈ 6 KB, contra ~18 KB de nueve drivers escritos a mano.

**Y el noveno del usuario deja de ser un problema: es un fichero de texto.** El fabricante siempre da un
ejemplo de Arduino o ESP-IDF con su secuencia, y pasarla a la tabla es mecánico. Sin toolchain, sin
compilar, sin nosotros. El catálogo que enviamos deja de ser una lista cerrada y pasa a ser «los que
damos probados».

**Dónde vive cada cosa** — y no hace falta inventar formato, porque ya está elegido dos veces (el ENV de
H9 y el manifest de los packs son los dos `clave=valor`):

| | dónde | por qué |
|---|---|---|
| **la PLACA** — pines, bus, frecuencia, qué panel, cómo se resetea, cómo se enciende la luz | **el ENV**, pocas claves | es de *esta* placa y cambia por unidad; y la config de placa ya vive ahí |
| **el CONTROLADOR** — la secuencia de arranque y sus flags | **un recurso** (`st7789.seq`) | es del IC, no de la placa: el mismo sirve para mil placas, y es largo (el GC9A01 son ~100 comandos). Una clave de ENV de 1000 caracteres no es sitio para eso |

📌 **Y el apunte de Eduardo sobre el IDF** (*«recuerdo que trabajaba con ficheros de configuración,
aunque creo que era para cuando compilaba la imagen»*): eso es **Kconfig** y su `sdkconfig` (las líneas
`CONFIG_X=y`), y es de **tiempo de compilación** — justo lo contrario de nuestra decisión de fondo
(*«una imagen única por familia; variante, pines y panel se deciden en runtime, no con macros»*). Lo que
sí conviene copiarle es lo que lo hace usable: **Kconfig describe las opciones aparte de los valores**
(tipo, defecto, rango, ayuda) y por eso `menuconfig` puede enseñar un menú y validar. Un `lcd.sck=6` a
pelo no guía a nadie y una errata es muda. Nosotros tenemos para eso algo MEJOR que una lista escrita a
mano: **la tabla de capacidades de H4**, que valida contra el silicio real («el pin 99 no existe en
esp32c5», «el SPI 5 no existe: válidos 2, 3»). ⏭️ Y a medio plazo tiene sitio natural en el **panel de
placa del IDE**: un formulario de pantalla que escriba esas claves sería nuestro `menuconfig`, pero en
caliente y contra la placa enchufada.

**Dónde está la frontera del driver genérico**: 4-wire SPI con MIPI-DCS. Fuera se quedan SPI de 9 bits
(el D/C como noveno bit), QSPI, paralelo 8080, RGB y DSI — eso sí es un `npk` propio (nivel 2).

**Considerado y descartado: el driver en BasicPlus.** Hay `Spi.Bus`, `Gpio.Pin` e `I2c.Bus`, así que el
ARRANQUE cabría en BP. Pero el `flush` es el camino caliente —143 ms ya en C— y además obligaría al pack
a llamar HACIA BP en mitad del render, reentrando por la puerta de atrás. La secuencia como datos
resuelve lo mismo sin nada de eso.

⚠️ **El agujero honesto que esto deja al descubierto**: el nivel 2 (un `npk` propio) hoy **sólo lo
podemos hacer nosotros**. Hace falta la cadena AOT, saber del `.mdn`, de las versiones de ABI, del byte
de arquitectura y del alineado, y la receta vive en notas y no en la documentación — es literalmente lo
que ya avisaba la memoria `base-finita-lo-demas-en-packs`. Si `'PANT'` va a ser la válvula de escape del
usuario, **un proyecto plantilla + un script + documentación son un ENTREGABLE de `H6.c`**, no un extra.

#### 7.10 Lo temporal, mientras tanto (acordado con Eduardo el 22-sep)

*«¿Podemos resolver la situación de forma temporal incluyendo todos los drivers de pantalla en el pack
de LVGL?»* — **sí, y cuesta un +2 %**: cada driver son ~2 KB de código sobre 715 KB, y el `.bss` no se
multiplica (los 23 KB son los búferes de dibujo y sólo corre uno). Con dos condiciones:

1. **Detrás de la misma interfaz que tendrá `'PANT'`**, aunque de momento se elija con un `if` dentro
   del pack. Así sacarlos mañana es *mover* un fichero, no reescribirlo.
2. **Sin esperar a `env_get`**: la BIOS ya presta ficheros, así que el pack puede leer su perfil de
   `/sys/panel.cfg` hoy. `env_get` sigue siendo lo correcto —la config de placa vive en el ENV— pero no
   bloquea.

Es un **puente** hacia §7.9, no un destino: mientras los drivers vivan dentro, añadir un panel es
republicar 715 KB, y eso no escala a «decenas de placas STM32».

#### 7.11 El búfer de 64 B y la fachada SPI (23-sep, mañana — pregunta de Eduardo)

*«Ayer durante las pruebas reduciste el tamaño del buffer para el refresco a 64 bytes. Al final el
problema era el stack, pero el buffer sigue en 64. ¿Se puede hacer más grande y mejorar el refresco?»*

**Tocando ese 64, no — y no es un resto de la depuración.** La fachada SPI del ESP32
(`gpio_esp32.c`, `esp32_spi_init_impl`) abre el bus SIN DMA, y sin DMA el periférico mueve en una
transacción lo que cabe en su búfer: **64 bytes** (`SOC_SPI_MAXIMUM_BUFFER_SIZE` en el IDF del C5); si
se le pide más, el IDF rechaza la transacción ENTERA («txdata transfer > host maximum»). Antes del
troceo el driver pasaba 11 520 B de golpe y cada flush se rechazaba: se arregló en plena caza del
reinicio, y por eso parecía parte de la depuración, pero era otro fallo, independiente del stack.

**Adónde se van los 143 ms** (estimación, sin medir; con los números del propio IDF):

| | ms |
|---|---|
| el cable: 153 600 B a 40 MHz | ~31 |
| el peaje: 2 400 transacciones × ~40 µs | ~100 |
| pintar + girar los bytes | ~10 |

El IDF documenta **22 µs** por transacción en el C5 en el modo que usamos (interrupción, sin DMA; 12 en
polling, 24/15 con DMA); con nuestra cadena (pack → BIOS → fachada → IDF, desde flash) las cuentas
cuadran con ~40. Más de dos tercios es peaje, no datos.

**La palanca es la fachada, y es `G4`:**
1. **Que trocee la FACHADA, no quien llama**: `spi_write(n)` escribe n bytes sea cual sea n, como ya
   hacen la Pico y el STM32. Cierra además un agujero de portabilidad: hoy un `Spi.write` de 100 bytes
   desde BP funciona en la Pico y devuelve -1 en cualquier ESP32.
2. **DMA para lo grande, polling para lo pequeño**: una franja de 24 líneas pasa de 180 transacciones
   a una. Estimación: **~143 → ~45 ms** (7 → ~22 imágenes/s); una etiqueta ya va en 1 ms.
3. Después: el doble búfer asíncrono (una ranura nueva en la BIOS) y 80 MHz (por encima de la hoja
   del ST7789, 62,5 MHz: a probar en placa) → hacia 15-20 ms.

**Dos cuidados**: el pack y la imagen se versionan aparte —un pack que ya no trocea sobre una imagen
vieja dejaría la pantalla negra sin avisar—, así que el driver mira la versión de la BIOS (trocea a 64
con una BIOS que no lo prometa); y con DMA lo que muerde es lo PEQUEÑO: en el C3, el C6 y el S3 una
lectura de 1 byte a una dirección sin alinear hace que el IDF reserve un búfer de rebote POR BYTE —la
SD, que lee byte a byte, iría más lenta—, así que ≤ 4 bytes van por los búferes de la propia
transacción, y en la C5 se mide la SD (comparte bus con el panel) además de la pantalla.

Eduardo eligió seguir con `G1` y dejar esto para `G4`. De paso salió un fallo aparte —la VM-C recorta
en silencio a 256 bytes los builtins de I2C/SPI/UART y la Java no, lo que rompe la paridad por encima
de 256— que va en su propia sesión.

### 8. `G1` hecho (23-sep): varios motores a la vez, y `env_get`

**El criterio de §7.8 se cumple en las dos placas**: el P4 carga LVGL y SQLite en el MISMO arranque
(«2 motores, 2 cargables»; `SqlDemo` == host y `GuiPack` «las dos vivas, cada una con su LVGL»), y en
la C5 el mini lee una clave del ENV junto al pack de la GUI (`env_get('packRam') = 512`, «TODO OK»,
`GuiFlushBench` 144/1 ms como ayer). Commit `65756e91`.

#### 8.1 La corrección al diseño de §2.5 — y por qué

Al leer el código salió algo más sencillo que lo escrito, y Eduardo lo aprobó antes de programar:

| §2.5 decía | se hizo | por qué |
|---|---|---|
| `ram_bytes` en la cabecera (formato 2) | **el reparto es el SELLO** de cada motor | ya estaba escrito: `linked_ram` |
| el cargador reparte por orden de cadena | **la placa asigna al grabar**: `PACK_BURN_BEGIN` da el **tope** (lo primero libre por encima de los vivos) | el cargador sólo comprueba «dentro del bloque y sin pisar a otro» |
| `bp_pack_init(bios, ctx)` | la entrada no cambia; **la arena es común y con dueño** | el pool de LVGL acabó en su `.bss`: hoy sólo SQLite pide arena |

Lo que se gana: **sin campo nuevo ni cambio de formato**, lo ya grabado sigue valiendo (el SQLite del P4
y la GUI de la C5 cargaron sin regrabar); y **borrar un pack de en medio no rompe a los de detrás** —su
RAM se queda de hueco hasta formatear, como su flash—, que con el reparto por orden de cadena obligaba
a regrabarlos todos. El precio: actualizar un pack grabando el nuevo SIN borrar el viejo pide RAM para
los dos; el IDE lo dice ANTES de grabar («no cabe: el motor 'lvgl' pide 383 KB … le quedan 129 KB»).

#### 8.2 Lo que hay ahora

- **La cadena, no un barrido** (`bpvm_npack_planear`): se recorren los packs y sus entradas —la zona ya
  es un índice— en vez de 13,7 MB de 4 en 4. Si dos motores vivos se llaman igual **gana el de más
  adelante**, como los `.mod` y los `.mdn`; antes ganaba el PRIMERO, o sea que grabar una versión nueva
  sin borrar la vieja ejecutaba el motor viejo con el `.mod` nuevo.
- **La escalera juzga la RAM por rango** y cambió de nombre (`bpvm_npack_escalera`): mismos seis
  enteros, otra regla — un llamante que no se enterase no compila.
- **BIOS v6**: `env_get(clave, buf, cap)` al final (40 ranuras), sólo lectura, **entero o nada**
  (`bpvm_env_get_exacto`: un pin a medias es un número plausible y equivocado). Cada familia la
  conecta en `BPVM_BIOS_TABLA`, que la pide como parámetro: olvidarla no compila (y así lo cazó el doble
  de host de SQLite, que seguía pasando su arena en esa posición).
- **La arena con dueño** (`bpvm_bios_arena`, común): lo que queda por encima del último motor, del
  primero que la pide desde su `bp_pack_init`; al segundo, NULL y el cargador escribe de quién es. El
  mini ya no la pide: se la quitaría a SQLite si coincidieran.
- **El IDE mira si cabe** (`PackBurn.sellar` con `ramSize`, en el panel de Packs y en
  `PackTool`/`wire_serie.py`).

#### 8.3 Lo que salió por el camino

1. **`test-bmgr` no enlazaba desde el 13-ago** (#362 metió `bpvm_fs_set_fallback` en `bpvm_pack.c`) y
   nadie lo vio en seis semanas: no está en ninguna de las redes de cada día. Se destapó al añadirle el
   caso del tope. Es [[arnes-tiraba-stderr-guardian-mudo]] otra vez: un guardián que no corre no guarda.
2. **El STM32 no compilaba desde H6 P2** (22-sep): `gui_pack.c` y `gui_pack_publics.c` viven en `src/`,
   y los proyectos de CubeIDE compilan TODO `src/` —las ESP32 y la Pico listan sus ficheros—. En la DK2
   los trampolines chocaban con `gui.c` al enlazar (91 errores) y en el Nucleo la tabla no compilaba.
   Guardas puestas en el generador (`0386c5ce`). Es el reverso de «un `.c` nuevo del core: alta en los
   cinco builds»: en el STM32 el alta es AUTOMÁTICA, y un fichero que no es de todas las imágenes
   necesita su guarda.
3. **El P4 no se reinicia al abrir su puerto** (va por el CH343, no por el USB-JTAG): tras grabar un pack
   hace falta `wire_serie.py COM15 reset` para que lo cargue. Costó una lectura del log confusa —las
   líneas eran de un único arranque de tres minutos—.

**Lo siguiente es `G2`**: `'PANT'` —el contrato, `busca()` perezoso desde LVGL— y `panel_c5.c` a su propio
`npk`. Ya se puede: dos motores conviven, cada uno con su RAM.

### 9. `G2` hecho (23-sep): el driver de pantalla SALE del pack de LVGL

**La C5 funciona con DOS packs** —`Gui.pack` (LVGL, sin driver) y `DriverC5.pack`— y da lo mismo que
antes: `GuiFlushBench` **144 ms** la pantalla entera y **1 ms** una etiqueta (commit `84acbe0c`). El
P4 carga a la vez SQLite, LVGL y el driver nulo (`GuiPack` «las dos vivas», `SqlDemo` == host).
⚠️ **Falta VERLO**: la
pantalla y el táctil de la C5 no se han mirado (Eduardo estaba fuera). El camino del píxel es el mismo
que el 22-sep —girar los bytes y luego `CASET`/`RASET`/`RAMWR` y los datos, en ese orden—, sólo que
repartido entre dos packs; pero no se ha visto, y no se da por visto.

#### 9.1 Dos aportaciones de Eduardo antes de programar

1. **«Cuando hablas de paneles, ¿es a lo que yo llamaba drivers?»** Sí — y su palabra era la buena:
   en §7.2 «panel» es UNA de las cuatro capas (tamaño, inversión, desfase), y llamar así al pack entero
   chocaba con ella. Todo pasó a llamarse **driver de pantalla**: `bpstdlib/gui/drivers/<placa>/`,
   `DriverC5.pack`, `DriverNulo.pack`. La marca `'PANT'` (de *pantalla*) se queda.
2. **«En algunas placas el orden RGB va cambiado, y en otras algunos colores usan menos bits por falta
   de pines: ¿lo soportamos?»** Hoy no: los cuatro drivers fijan RGB y 16 bits, porque nuestras cuatro
   pantallas son así. Pero cada caso tiene su sitio:

   | caso | quién lo resuelve | toca `'PANT'` |
   |---|---|---|
   | orden RGB/BGR | el driver: un bit de MADCTL en los controladores por SPI (lo hace el chip, gratis); en `G3`, un campo del perfil | no |
   | bus paralelo con menos pines (RGB/DPI/LTDC) | el periférico y el cableado; el driver de la imagen (`G5`) | no |
   | controlador que no admite 16 bits por su interfaz (el ILI9488 por SPI: sólo 18) | **LVGL pinta en 24 bits** y el chip se queda con los 6 altos de cada byte — como el driver MIPI genérico del propio LVGL | **sí** |

   De ahí el símbolo opcional **`formato`**: `RGB565` (sin publicar, lo de un LTDC o un DSI),
   `RGB565_BE` (bytes girados, lo de cualquier controlador por SPI) o `RGB888`. Nuestro LVGL ya trae
   compilado el pintado en 24 bits (`LV_DRAW_SW_SUPPORT_RGB888`). Y de paso **el giro de bytes pasó
   del driver al lado de LVGL**, con su rutina: el driver ya no sabe de LVGL y sólo manda bytes.

#### 9.2 Lo que hay ahora

- **El contrato** (`bpgenvm-c/include/bpvm_pant.h`), publicado POR NOMBRE como `'LVGL'` y `'SQLI'`:
  `init`, `size` (tiene que contestar ANTES de `init`: `gui.c` pregunta el tamaño antes del screen) y
  `flush` obligatorias; `rotate` (0 = girado, -1 = no sabe, y la resolución no se toca), `touch` y
  `formato` opcionales. La versión es una puerta EXACTA, y por eso sólo sube si cambia la firma de algo
  que existe: añadir una opcional no la toca. Se quitaron `open` y `backlight` del diseño de §7.3:
  nadie los llamaba, y añadirlos luego no rompe nada.
- **El pack de LVGL no lleva driver** (`pant_lvgl.c`): busca `'PANT'` en el PRIMER USO —el orden de los
  packs da igual—, y allí viven el display, los búferes (11 520 B, las 24 líneas de siempre a 240 px en
  RGB565), el formato, el táctil como `lv_indev` y el reloj. **Es el mismo pack para todas las placas
  de una ISA**: se compila tres veces y ya.
- **Los drivers, cada uno en su pack**: `DriverC5` (2 364 B de código, 8 B de estáticos; los pines
  siguen escritos dentro — sacarlos a un perfil es `G3`) y `DriverNulo` (468 B: la PLANTILLA mínima,
  sólo las tres obligatorias). **Se construyen con UNA orden**: `build_driver.sh <driver> <destino>`
  deja el `.npk` y el `.pack` — el primer trozo del entregable «plantilla + script + docs» de §7.9.
- **Sin driver, error CON NOMBRE**: `Gui.Screen()` lanza «GUI: no hay driver de pantalla ('PANT'):
  graba el pack del driver de esta placa» en vez de una pantalla negra. Corto a propósito: el detalle
  del EXITED que ve quien ejecuta recorta a 90 caracteres con la clase delante, y la primera versión
  llegaba como «…graba el pack del dri».
- **`wire_serie.py packdel <pack>`** (borrar uno suelto, que en el P4 hace falta para conservar SQLite)
  y `packs` marca los BORRADOS.

#### 9.3 El fallo del camino: un motor sin `.data` se enlazaba MAL, y en silencio

La C5 se reinició en el `init` del driver. Con la copia USB de la consola (la receta de ayer) salió
**«Load access fault»**, y `addr2line` sobre el ELF del driver lo puso en `di()`, leyendo `g_bios`:
el driver no tiene NINGUNA variable inicializada, así que no hay `.data`, `-Tdata=0x20000000` no tiene
a quién colocar, y su `.sbss` se quedó en `0x193c`, pegada al código. `NpackReloc` clasifica por
dirección, así que `g_bios` se realojó **como si fuera flash**: lo que el driver guardaba no llegaba a
ninguna parte (en `bp_pack_init` no se notó porque el compilador reutilizó el valor en un registro).
Arreglado por los dos lados: los drivers se enlazan con **`bpgenvm-c/tools/npk.ld`**, que pone la RAM
en `0x20000000` aunque no haya `.data`, y **`NpackReloc` rechaza con nombre** una sección de RAM por
debajo del umbral o una de flash por encima — probado: el driver mal enlazado ya no construye. El mini,
SQLite y LVGL no lo tenían (todos tienen `.data`), y el mini sale idéntico. Es «un control descarta lo
que ejercita»: el mini no lo cazó porque sí tiene datos inicializados.

**Lo siguiente es `G3`**: el driver genérico MIPI-DCS por perfil, con la C6 de oráculo (y ahí también
hará falta mirar la pantalla).

### 10. `G3` hecho (23-sep): el driver GENÉRICO — la C5 funciona sin una línea de C

**La C5 pinta con `DriverSpi` y tres líneas de ENV**, sin su driver de placa: `DriverC5` borrado de la
zona, `DriverSpi.pack` grabado, y

    lcd=st7789,spi:2,sck:6,mosi:7,dc:9,cs:10,rst:x1,bl:xpwm,mhz:40,w:240,h:320,inv
    exp=ch32,i2c:0,sda:0,scl:1,addr:0x24
    touch=cst3530,addr:0x58,rst:x0

`GuiFlushBench` da **144 ms** la pantalla entera y **1 ms** una etiqueta (con `DriverC5` y la misma imagen,
145/1), y el log enseña al driver leyendo el ENV y su `st7789.seq` desde su propio pack (`#362`). Commit
`f78cb29b`. ⚠️ **Falta VERLO**: Eduardo estaba fuera; la C5 quedó con `C5Touch` en marcha —el botón
«PULSA» y su contador en la pantalla—, que sirve para mirar los colores y pulsar sin el PC.

**Un cambio respecto al plan de §7.8**: el primer cliente es la C5 y no el C6. El C6 no puede cargar hoy el
pack de LVGL (sus 256 KB de pool van en la `.bss` del pack y el C6 no tiene PSRAM: es `H6.b`), y la C5
tenía delante su `DriverC5` para comparar en la misma placa. Eso obligó a meter el expansor (`ch32`) y el
táctil (`cst3530`), y con ellos se cumple lo que decía §7.4: la frontera entre «perfil» y «driver de placa»
se movía con muy poco.

#### 10.1 El formato: la placa en el ENV, el controlador en un `.seq`

- **Una línea por dispositivo**, con el formato de `sd=` (`clave:valor` separados por comas, y el tipo
  delante). `lcd=` es la pantalla: el bus y sus pines, el tamaño y, opcionales, `rst`, `bl`, `mhz` (20 si no
  se dice), `ram`, `off`, `inv`, `bgr`, `seq` y `traza`. Un pin es un GPIO (`21`), una salida del expansor
  (`x1`) o su PWM (`xpwm`). `exp=` declara el expansor una vez y `touch=` el táctil (sin bus propio, usa el
  del expansor). La referencia entera está en la cabecera de `driver_spi.c`.
- **El controlador es texto**: `st7789.seq`, un comando por línea en hexadecimal (valen `0x` y comas, que
  es como vienen en los ejemplos de los fabricantes), `espera N` y comentarios con `#`. Va en `resources/`
  del pack y el driver lo lee por la BIOS de `pack:DriverSpi/st7789.seq`. **Sólo lo del chip**: el del
  ST7789 son dos comandos (SLPOUT y RAMCTRL); MADCTL, COLMOD, la inversión y DISPON los pone el driver
  después, a partir del ENV. Un controlador que no traemos es un `.seq` del usuario en su FS y
  `seq:/sys/mio.seq` en la línea `lcd=` — sin C y sin herramientas (probado con uno de mentira en la prueba
  de host, escrito como lo copiaría alguien de un ejemplo: `0x`, comas y CRLF).
- **El desfase por orientación sale de dos datos físicos**: `ram` (la memoria del controlador) y `off`
  (dónde cae el cristal dentro, a 0°). MX espeja las columnas de esa memoria y MY sus filas, con MV o sin
  él; así salen la tabla del C6 que se confirmó en placa (0/0, 0/0, 0/80, 80/0) y la de las pantallas de
  135×240 (52/40, 40/53, 53/40, 40/52) sin escribir ninguna tabla.

#### 10.2 Una errata no puede ser una pantalla negra

§7.9 lo dejó dicho: *«un `lcd.sck=6` a pelo no guía a nadie y una errata es muda»*. Tres cosas:

1. **El parser es ESTRICTO**: una clave que no conoce es un error con su nombre. La línea `sd=` ignora las
   desconocidas porque una misma línea sirve a dos placas; ésta no tiene esa excusa, y un `rts` por `rst`
   —o una clave de un driver más nuevo que éste no sabe hacer— sería una pantalla negra sin una palabra.
2. **`'PANT'` gana `estado`** (opcional, así que la versión no sube y los drivers ya grabados siguen
   valiendo): NULL = listo; si no, el porqué. La VM lo pregunta en el primer `Gui.Screen()` y el programa
   recibe `Core.RuntimeError: GUI: pantalla: ENV lcd: falta 'dc'` — medido así en la C5, y cabe en los 90
   caracteres del EXITED. Mientras no vale, el driver relee el ENV en cada llamada: arreglada la línea, el
   siguiente intento pasa sin recargar nada.
3. **Un táctil que no contesta se dice, una vez**: una lectura fallida sigue siendo «sin dedo» (se sondea
   decenas de veces por segundo y un NACK suelto no debe parar la GUI), pero la primera va al log.
   `DriverC5` callaba. Y sirvió enseguida: en la C5, 10 s de sondeo sin aviso con `addr:0x58`, y el
   CONTROL con `addr:0x59` dio «el tactil no contesta en I2C 0x59» a los 178 ms — o sea que el CST3530
   contesta de verdad, y lo que falta es sólo pulsar.

#### 10.3 Cómo se sabe que manda lo mismo: una BIOS que graba los cables

`make test-driverspi` compila **los dos drivers tal cual** para el host (renombrando su entrada y su
`g_bios` para que convivan) y los ejecuta contra una BIOS falsa que graba cada byte de SPI con el nivel de
DC y de CS, cada escritura y lectura de I2C y cada espera. Con el perfil de la C5:

- el **volcado**, los **cuatro giros** y el **táctil** son byte a byte iguales;
- el **arranque** manda los mismos seis comandos con los mismos parámetros; sólo RAMCTRL cambia de sitio
  (`11|36 00|3A 55|B0 00 F0|21|29` frente a `11|B0 00 F0|36 00|3A 55|21|29`), porque ahora va en el
  `.seq`, antes de lo de la placa;
- el mismo bus SPI, el mismo I2C, y el expansor igual al final (los ocho EXIO de salida, los dos resets
  sueltos, la luz a 255).

Y contra sus oráculos, lo que `DriverC5` no sabe hacer: los desfases del C6 y de las 135×240, el reset por
GPIO, el SWRESET y el INVOFF, el `.seq` del usuario, y 17 erratas, cada una con su nombre. **Con control**:
tres sabotajes (troceado a 32, los ejes del desfase cambiados, RAMCTRL con otro valor), y la prueba caza
los tres. El `.seq` que usa es el de verdad, el que va en el pack.

#### 10.4 Lo que cuesta, y lo que no se ha hecho a propósito

- **12,6 KB de flash y 980 B de RAM**, frente a los 2,4 KB de `DriverC5`: el parser y los mensajes. Es el
  1,7 % del pack de LVGL, y es lo que sustituye a un driver por placa.
- **Sólo lo que se ha podido probar**: el `.seq` del ST7789, el expansor `ch32` y el táctil `cst3530`. Las
  secuencias del ILI9341, el ST7735 y el ST7796 están en los drivers de nuestra propia copia de LVGL
  (`third_party/lvgl/src/drivers/display/`), que es la fuente para quien las necesite; entran en el pack
  cuando haya una pantalla con la que probarlas.
- **Fuera todavía**: los 18 bits (el ILI9488 por SPI: el `formato` RGB888 ya existe en `'PANT'`, falta la
  clave), el táctil girado con la pantalla (pendiente desde `H6.a`, igual que en `DriverC5`), el modo SPI
  (fijo en 0), un panel sin CS y una luz activa en bajo. Cada uno es una clave más cuando aparezca la placa.
- **`DriverC5` se queda**: es el ejemplo de driver de placa en C (el nivel 2, para lo que no se describe con
  datos) y la referencia contra la que se compara el genérico.
- Lo que queda de `H6.c`: **`G4`** —el bus con dueño (la TF de la C5 comparte bus con la pantalla) y la
  fachada SPI con DMA (§7.11)— y **`G5`** —el P4 y la DK2 publicando `'PANT'` desde la imagen—; y, para
  cerrar, el entregable de §7.9: plantilla + script + **la documentación de usuario** de estas líneas.

### 11. `G4` rediseñado (23-sep, mañana): el bus es COMPARTIDO; lo exclusivo es el dispositivo

> Eduardo, sobre la pregunta 5 de §7.7 (yo proponía «el bus, con dueño, como los pines»): *«Los buses
> SPI e I2C son buses que muchas veces comunican con varios periféricos. En el SPI el que sí es de uso
> exclusivo es el CS, uno por periférico. He visto que en algunas placas el SPI se utiliza para la
> pantalla y la tarjeta micro SD. Así que esa exclusividad no parece buena idea.»*

Tiene razón, y es la topología de nuestras propias placas: en la C5 la TF y la pantalla van en el SPI2
(SCK 6, MOSI 7, MISO 8; CS 10 la pantalla, CS 23 la TF), y en el I2C 0 el expansor (0x24) y el táctil
(0x58). Hasta el comentario de `Spi.bp` lo decía («un mismo bus suele compartirse entre varios slaves con
CS distintos»). **Lo que muerde hoy no es compartir, es REINICIAR**: cada `spi_init` suelta el bus y lo
vuelve a crear (`gpio_esp32.c`: `spi_bus_free` + `spi_bus_initialize`), `i2c_init` igual
(`i2c_del_master_bus`), y la velocidad y el modo van con el bus. El segundo que «abre» un bus se lo quita
al primero, sin una palabra — la pantalla a 40 MHz y la TF a 400 kHz son dos reinicios del mismo SPI2.

#### 11.1 Dónde vive: en la HAL BP (Eduardo)

> *«La operación de los buses sería algo propio de la BIOS. En nuestro caso tenemos HAL de fabricante y
> HAL BP; creo que necesitamos a los dos para hacer que todo sea genérico, sin depender de hardware ni
> HAL.»* Y: *«Necesitamos arbitrar, quizás con un mutex, que asegure que no se mezclan tramas ni usos. El
> árbitro, quizás implementado a nivel de HAL BP.»*

```
Hardware
 └─ HAL del fabricante (ESP-IDF, pico-sdk, STM32 HAL)            la CINTURA: abre el bus y transfiere
     └─ HAL BP (spi.c, i2c.c, gpio.c + capacidades + pines)      COMÚN: buses, dispositivos, ÁRBITRO
         ├─ la VM: los builtins Spi/I2c → programas BP
         ├─ el firmware: bpvm_sd.c (la TF por SPI)
         └─ la BIOS → los packs (los drivers de pantalla)
```

El árbitro va en la HAL BP porque es **la única capa por la que pasan los TRES usuarios de un bus**. En la
BIOS —que en nuestro vocabulario es la ventanilla de los packs: presta la HAL BP, no la sustituye— se lo
saltarían los programas BP y la TF, que no pasan por ella. Y es la doctrina de siempre (la HAL BP común):
el motor único en el común y la cintura por familia, que aquí ADELGAZA — abrir una vez y transferir.

#### 11.2 La configuración del bus va en el ENV, y quien llegue después no la cambia (Eduardo)

> *«Los pines se configuran en el ENV, tanto para el SPI como para el I2C. Los que vengan detrás tienen
> esa configuración ya establecida, así que si intentan cambiarla debería saltar un error.»*

```
spi2=sck:6,mosi:7,miso:8            el BUS: sus pines, una vez
i2c0=sda:0,scl:1,khz:400            (en I2C la velocidad es del bus: la tienen que aguantar todos)
lcd=st7789,spi:2,dc:9,cs:10,rst:x1,bl:xpwm,mhz:40,w:240,h:320,inv
exp=ch32,i2c:0,addr:0x24
touch=cst3530,i2c:0,addr:0x58,rst:x0
sd=spi:2,cs:23                      (la TF de la C5, el día que se use)
```

- **Cada dispositivo dice su bus por número y SÓLO lo suyo**: el CS (y el DC), su velocidad y su modo. En
  SPI la velocidad es del DISPOSITIVO: la pantalla a 40 MHz y la TF a 20 en el mismo bus.
- **Los pines de un bus declarado quedan reservados** en la tabla de H4 desde el arranque:
  `Pin(6, OUTPUT)` → «el pin 6 es del SPI 2 (ENV spi2)». Hoy nadie los marca, y un programa podía quitarle
  el reloj a la pantalla en silencio.
- **Quien pida OTRA configuración recibe un error con nombre** —«el SPI 2 es sck 6, mosi 7, miso 8 (ENV
  spi2); pides sck 12»—, y quien pida la MISMA, se engancha.
- **Un bus que el ENV NO declara está LIBRE: lo establece el primero que aparece** (Eduardo: *«si no se
  declara en el ENV, la asignación de pines está libre; queda disponible para el primero que aparezca»*),
  y desde ese momento rige la misma regla. Así siguen valiendo los samples y los montajes de protoboard
  (`Spi.Bus(0, 2, 3, 4, …)` con sus pines: un sensor en pines libres no es un dato de placa) y la línea
  `sd=sck:34,mosi:35,miso:36,cs:39,cd:40` de la Metro, que lleva los pines dentro.
- **Y se puede LIBERAR** (Eduardo: *«también debería poder liberarse; aunque no es habitual, podría darse
  el caso»*): `bus.release()`, como el `Pin.release()` que ya existe. Suelta la configuración y los pines
  para que otro establezca otra. Con tres reglas:
  - sólo un bus que NO declara el ENV: el de la placa son las pistas del circuito, y se «libera» quitando
    su línea (liberarlo desde un programa es un error con nombre, no un no-op);
  - sólo si nadie más lo usa: con un pack o la TF enganchados, error con el nombre de quién;
  - el fin de RUN libera lo que estableció el programa, igual que sus pines; lo que estableció un pack o el
    firmware se queda.
- **`-1` en un pin quiere decir «el que tenga el bus»**: un dispositivo (la pantalla, la TF, un programa)
  usa un bus declarado sin repetir sus pines, y el MISO a -1 de la pantalla («no lo uso») casa con un bus
  que sí lo tiene. Con el bus sin establecer hay que dar al menos SCK y MOSI (en I2C, SDA y SCL).
- **En I2C la configuración incluye la velocidad** (es del bus: la aguantan todos o no vale); **en SPI no**
  (es de cada dispositivo).
- Cambia el formato de `G3` antes de publicarlo: `lcd=` pierde `sck`/`mosi`, y `exp=`/`touch=` pierden
  `sda`/`scl`/`khz`.

#### 11.3 El árbitro

- **Qué arbitra: la TRANSACCIÓN** —del CS abajo al CS arriba en SPI, de START a STOP en I2C—, que es lo que
  no se puede mezclar («tramas»). Al empezarla aplica la velocidad y el modo de ESE dispositivo («usos»).
- **Con qué: un mutex por bus**, de la plataforma (`bpvm_platform_mutex`: FreeRTOS en todas las familias,
  pthread en el PC). Cubre `vm` contra `io` —el que atiende el wire puede leer la TF mientras `vm` pinta—
  y las dos VM de mañana.
- **La trampa son los hilos BP.** Si un programa tuviera el bus tomado ENTRE dos llamadas, el siguiente
  que lo pidiera desde el mismo hilo del SO —`Gui.run()` pintando— bloquearía ese hilo entero: un bloqueo
  mutuo (y con un mutex recursivo, justo la mezcla que se quiere evitar). La regla que lo quita de raíz:
  - **desde BP, una transacción nunca abarca dos llamadas**: cada `write`/`read`/`transfer` de un
    dispositivo es una transacción entera, con su CS; y `writeRead` para «comando y respuesta» en la misma
    ventana;
  - **el código nativo** (la TF, un driver de un pack) sí puede tomar el bus, hacer varias cosas y
    soltarlo —leer un bloque de la SD, un volcado de la pantalla—, porque entre medias no cambia de hilo BP.
- **Nada espera para siempre**: el árbitro sabe QUIÉN tiene el bus y desde cuándo, y el que espera más de
  un plazo recibe «SPI 2 ocupado por la pantalla (CS 10) desde hace 1000 ms», no un cuelgue mudo.
- **I2C igual**, con «escribir y leer» en una transacción (repeated start) en la fachada: el registro del
  CST3530 hoy son dos transferencias sueltas.

#### 11.4 Lo que cambia en cada capa

| capa | hoy | con `G4` |
|---|---|---|
| cintura (`gpio_esp32.c`, Pico, STM32) | `init` suelta y reabre el bus en cada llamada; velocidad y modo por bus | abre el bus UNA vez y transfiere; un dispositivo por CS con su velocidad y su modo (el IDF ya funciona así; la Pico y el STM32 cambian el formato al cambiar de dispositivo) |
| HAL BP (`spi.c`, `i2c.c`) | pasa las llamadas tal cual | la tabla de buses (del ENV o del primero), los dispositivos (su CS reclamado en la tabla de H4), el árbitro y los errores con nombre |
| BIOS (los packs) | `spi_init`/`spi_write`, `i2c_*` | ranuras nuevas AL FINAL (v7): dar de alta un dispositivo, y «tomar / transferir / soltar» |
| BP | `Spi.Bus(id, sck, mosi, miso, hz, modo)` y el CS a mano con `Gpio` | `Spi.Device(bus, cs, hz, modo)`, cada llamada una transacción; `Spi.Bus` sigue valiendo para quien está solo en el bus; los mismos errores en miVM |

#### 11.5 Cómo se prueba

En el PC, antes que en ninguna placa: dos hilos del SO machacando dos dispositivos del mismo bus contra un
backend que graba la cinta (el de `test-driverspi`): ninguna trama puede salir mezclada, y **con el mutex
quitado la prueba TIENE que fallar** (el control). Y los errores de configuración, uno por uno. En placa,
la prueba de verdad es la C5 con la TF y la pantalla A LA VEZ: un programa escribiendo en `/sd` mientras
`Gui.run()` anima.

#### 11.6 El plan

| paso | qué | se prueba con |
|---|---|---|
| **G4.a1** | los buses en el ENV (`spiN=`, `i2cN=`), sus pines reservados, «se establece una vez y otra configuración es un error»; la cintura deja de reabrir — en las cinco familias | los errores en el PC (y en miVM, los mismos); la C5 con las líneas nuevas dando lo mismo que hoy |
| **G4.a2** | los dispositivos y el árbitro; la BIOS v7; `DriverSpi` con el formato nuevo | la cinta con dos hilos (y su control); la C5 con la TF y la pantalla a la vez |
| **G4.a3** | `Spi.Device` en BP (y el `writeRead` de I2C), con paridad | samples en las dos VM |
| **G4.b** | el DMA en la cintura del ESP32 (§7.11) | los 144 ms bajando |

#### 11.6b `G4.a2`, el diseño concreto (23-sep, 19 h): SIN tabla de dispositivos — cada petición lleva su configuración (Eduardo)

> Yo proponía dar de alta cada dispositivo en la HAL BP (su CS, su velocidad, su modo, su nombre). Eduardo:
> *«Si tenemos varios usuarios trabajando con su propia configuración se necesita algún tipo de estructura
> donde se guarde esa configuración. No sé si haría falta; si no, cada vez que se vaya a utilizar el bus
> tendría que utilizar la configuración a utilizar. Todas son operaciones petición-fin, no veo que haya que
> guardar nada intermedio.»*

Tiene razón: cada usuario YA tiene su configuración —la pantalla en su `lcd=`, la TF en su `sd=`, el programa
en su objeto— y la trae en cada petición. Guardarla también en la HAL BP sería una segunda copia que
desincronizar.

- **El TURNO**: `tomar(bus, cs, hz, modo)` espera a que el bus esté libre, aplica la velocidad y el modo (la
  cintura se lo salta si no cambian) y baja el CS; `transferir` las veces que haga falta; `soltar` sube el CS
  y despierta al siguiente. `cs = -1`: la HAL BP no toca ningún CS (lo lleva quien toma).
- **El único estado es el del bus, y dura lo que el turno**: de quién es (el programa, el firmware o un pack,
  y su CS o su dirección) y desde cuándo — para esperar y para el error: «SPI 2 ocupado (CS 10, un pack) desde
  hace 1000 ms». Se borra al soltar.
- **Un mutex y una condvar para toda la tabla**, no uno por bus: el mutex sólo guarda el «quién» y se suelta
  enseguida; la transferencia va fuera. Se crean al arrancar, con los buses del ENV.
- **Nada espera para siempre**: 1 s de plazo, y el error con nombre. También el hilo que pide un bus que ya
  tiene: era un cuelgue, y pasa a ser ese mismo error.
- **El CS lo reserva quien lo usa**, como hoy (el pack con `gpio_init`, la TF al montar, el programa con su
  objeto); la HAL BP sólo lo mueve durante el turno.
- **Nativo y BP**: el código nativo (la TF, un driver) hace varias transferencias en su turno y puede subir el
  CS dentro —la TF lo necesita: sus 80 relojes con el CS arriba y el byte tras cada bloque—. Desde BP, cada
  llamada es un turno entero.
- **I2C igual**: cada llamada es un turno (la dirección hace de CS), y `escribir_leer` escribe y lee en UN
  turno: nadie habla en medio. Con STOP entre las dos, por ahora; el repeated start de verdad, con `G4.a3`.
- **Lo que ya existe pasa por el árbitro**: los builtins de `Spi.Bus`/`I2c.Bus` (cada llamada, un turno), las
  ranuras v6 de la BIOS y la TF. Y el `abrir` deja de ajustar la velocidad en el acto: se ajusta al tomar, que
  es cuando nadie más transfiere.
- ⚠️ **La excepción, temporal**: los builtins de `Spi.Bus` no llevan la velocidad en cada llamada, así que hasta
  `G4.a3` el bus recuerda la de cada usuario de las ranuras de siempre (la de su `abrir`) y la aplica en sus
  turnos. En `G4.a3` la pasa el objeto BP, como todos.
- ⚠️ **El límite, a sabiendas**: un programa que baja su CS a mano con `Gpio` y hace dos llamadas de `Spi.Bus`
  puede quedar intercalado con la pantalla entre ellas. `Spi.Device` (`G4.a3`) lo quita: el CS va dentro del
  turno.
- **BIOS v7**, cinco ranuras al final: `spi_tomar(bus, cs, hz, modo)`, `spi_cs(bus, nivel)`,
  `spi_transferir(bus, tx, rx, n)`, `spi_soltar(bus)` e `i2c_escribir_leer(bus, addr, tx, ntx, rx, nrx)`.
  `DriverSpi` las usa y exige la v7; las ranuras v6 siguen, por el árbitro.

**Se prueba** (§11.5): en el PC, dos hilos del SO machacando dos dispositivos del mismo bus contra una cintura
que graba la cinta —ningún turno mezclado, cada byte con su CS y su velocidad—, y **con el árbitro quitado la
prueba TIENE que fallar** (el control, repetible en el Makefile); los errores uno a uno; `test-driverspi`
contra DriverC5 y `test-sd` con la TF por el árbitro. En placa: la C5 con la pantalla y el táctil por la v7, y
un programa usando el SPI 2 mientras la GUI anima. Después, con la TF montada también en el ESP32 (el punto de
H3 «SD al común»; la microSD ya está en la C5), la prueba de verdad: la TF y la pantalla a la vez.

#### 11.7 `G4.a1` hecho (23-sep, mediodía): el bus, UNA configuración — en las cinco familias

**En la C5**: el ENV declara `spi2=sck:6,mosi:7,miso:8` e `i2c0=sda:0,scl:1,khz:400` y el arranque lo dice
(«bus: SPI 2 de la placa (ENV spi2): sck 6, mosi 7, miso 8»); la pantalla —`DriverSpi` con las líneas sin
pines de bus— se engancha y da lo mismo (145/1 ms). Y `samples/C5Bus.bp` lo prueba con la pantalla VIVA:
un programa se engancha al SPI 2 (con -1) y la etiqueta sigue a 1 ms antes y después —hasta hoy esa
apertura le reabría el bus a la pantalla—; pedir otros pines, un `Gpio.Pin(6)` o soltar el bus de la placa
dan su error con nombre; y el programa lee el CH32 por el mismo I2C 0 que usan el expansor y el táctil.
Commit `05615664`.

**Lo que hay:**
- **`bpvm_bus.c`, en la HAL BP**: la tabla de buses con la regla de §11.2 —la configuración la establece el
  ENV o el primero; otra es un error con nombre; -1 en un pin = el del bus; `release()`; el fin de RUN
  suelta lo del programa— y los pines de cada bus reservados en la tabla de H4 («el pin 6 es del SPI 2»).
- **Las fachadas `spi.c`/`i2c.c` son la puerta de los TRES usuarios** —los builtins (el programa), la BIOS
  (los packs) y la TF (`bpvm_sd.c`, el firmware)—, y **la cintura se partió en abrir / ajustar / cerrar**:
  abre el bus UNA vez. En las tres cinturas (la ESP32 —S3, C3, C5, C6, P4—, la Pico y el STM32), y cada
  familia lee `spiN=`/`i2cN=` al arrancar (el STM32 ganó un lector de texto del ENV); también el simulador
  del IDE. De paso la Pico dejó de pasarle un `-1` al configurar los pines (el MISO de una pantalla).
- **BP**: `Spi.Bus.release()`, `I2c.Bus.release()` y `Spi.release(bus)`/`I2c.release(bus)` (builtins 256 y
  257), sin destructor A PROPÓSITO: dos `Bus` del mismo id son el MISMO bus, y que el recolector se llevara
  uno cerraría el del otro. **miVM lleva la misma regla y los mismos textos**: `samples/BusCompartido.bp`,
  en el corpus de paridad (93/93).
- **La TF**: `sd=bus:2,cs:23` (la forma corta: los pines, los del bus); la larga de la Metro sigue valiendo.
  Un bus sin MISO o con otra configuración es un peldaño nuevo de su escalera (`E_BUS`), con el porqué.
- **`DriverSpi`**: las líneas de dispositivo pierden los pines del bus —un `sck` en `lcd=` es «es del bus: va
  en su linea spiN=»—, pide -1 a la BIOS, y `estado` avisa si falta la línea del bus.
- **Se prueba**: `make test-bus` (la regla entera contra una cintura que CUENTA aperturas y cierres —el fallo
  de antes—), `test-driverspi` con el formato nuevo, la paridad, y las ocho imágenes compiladas.

**Por el camino**, y se cuenta porque es el patrón del mes: `test-sd` y `test-blk` no enlazaban desde #478 y
H4 —fuera de las redes, nadie lo vio; ya enlazan y pasan—, y `sim-smoke` falla también SIN estos cambios (el
simulador arranca con el FS vacío y no encuentra `Core`): comprobado en `HEAD` y apuntado aparte. Y un
agujero pequeño de H4: `Gpio.release(9)` —o el destructor de un `Pin(9)` que había fallado— le quitaba a un
pack su pin DC; ahora `release()` sólo suelta lo del programa.

**Lo que queda, a sabiendas:**
- **UN ajuste por bus hasta `G4.a2`**: si la pantalla (40 MHz) y la TF (20 MHz) comparten el SPI, manda el
  último que abrió. Que cada dispositivo lleve su velocidad y su modo en cada transacción es el árbitro.
- **Tres drivers de imagen no pasan por la HAL BP**: el táctil del P4 (I2C 0), la pantalla del C6 (SPI 2,
  con DMA) y el táctil de la Discovery (I2C 2). Con ellos, un programa que abra ese bus recibe ahora «no se
  pudo abrir» (antes, un -1 mudo); si lo abre ANTES que la GUI, el `ESP_ERROR_CHECK` de la imagen reinicia
  la placa, como antes. Se arregla cuando esos drivers salgan de la imagen o se registren (`H6.b`, `G5`).
- En el STM32 el I2C sigue fijo a 100 kHz (deuda anterior): un `i2cN=…,khz:400` hoy va a 100.

#### 11.8 `G4.a2` hecho (23-sep, noche): EL ÁRBITRO — el turno en la HAL BP

Commit `c5b701a8`. El diseño, en §11.6b (Eduardo: sin tabla de dispositivos —cada petición lleva su
configuración— y la velocidad sólo se cambia si es otra que la actual).

**Lo que hay:**
- **El turno** (`bpvm_bus.c`): `tomar`/`dejar`, con UN mutex y UNA condvar para toda la tabla; del bus sólo
  se sabe de quién es el turno y desde cuándo. Plazo de 1 s y el error con nombre. **Cortesía**: quien llega
  con otros esperando se pone a la cola (ver abajo, la inanición).
- **La fachada SPI** (`spi.c`): `tomar(bus, cs, hz, modo)` / `cs` / `xfer` / `dejar`; guarda la velocidad y
  el modo ACTUALES del bus y ajusta sólo si cambian —en las cinco familias por igual: la cintura de la Pico
  no lo comprobaba—; el `abrir` ya no ajusta (se cambiaría en mitad del turno de otro). Las ranuras de
  siempre, cada llamada un turno con la velocidad de su `abrir` (la excepción temporal, hasta `G4.a3`).
- **I2C** (`i2c.c`): cada llamada un turno (la dirección hace de CS) y `escribir_leer` en uno.
- **La BIOS v7**, cinco ranuras al final (45). **`DriverSpi`** la exige: cada comando y cada bloque, un turno
  —el DC se pone antes de pedirlo, así la cinta queda como la de DriverC5— y el táctil lee registro y trama
  en uno. **La TF** (`bpvm_sd.c`), por turnos de firmware con su velocidad en `info->hz`, subiendo el CS
  dentro del turno donde el protocolo lo pide.

**Cómo se sabe:**
- `make test-arbitro`: dos hilos del SO, la pantalla (CS 10, 40 MHz) y la TF (CS 23, 20 MHz) machacando el
  SPI 2 contra una cintura que graba cada trozo con los dos CS y la velocidad del instante: los 150 trozos
  limpios. **Su control** (`make test-arbitro-control`: el árbitro apagado con `-DBPVM_SIN_ARBITRO`): 54
  trozos con el CS mal, 50 a otra velocidad, 90 cortes — la pantalla transfiriendo con los DOS CS abajo.
- **La inanición**: la destapó la propia prueba, una vez y por casualidad (ese día el `Sleep(1)` de Windows
  duraba ~15 ms): quien suelta el bus y lo vuelve a pedir enseguida gana siempre —ya está corriendo; el que
  espera tiene que despertar— y el otro agotaba su plazo. Arreglada con la cortesía, **quitarla y correr la
  prueba 6 veces dio 6 verdes**: no la protegía. Tiene su medida propia: un glotón encadenando turnos y la
  TF pidiendo uno, cinco intentos por ejecución, el tope en TURNOS del glotón (no en ms). Con la cortesía
  8/8 verdes (el peor: 33 ms, 1 turno); sin ella, 8/8 rojos.
- `test-bus` (la regla de la velocidad, el CS del turno, los plazos, el I2C), `test-bios` (45 ranuras),
  `test-driverspi` (49 turnos, ninguno fuera de sitio ni anidado, con sabotaje) y `test-sd`/`blk`/`sdsc`
  (con dobles de un hilo: la espera «vence» en el acto, y el árbitro no se cuelga con un reloj parado);
  paridad 93/93 + cápsula + norma + sombra + MOD8; y las ocho imágenes compilan.
- **En la C5**: «bios: tabla COMPLETA (45 ranuras)»; `GuiFlushBench` 148 ms por pantalla (145 antes: los
  turnos cuestan un 2 %) y 1 ms por etiqueta; `C5Touch` pinta y responde (Eduardo).

**Lo que salió por el camino:**
- **El segundo programa de GUI hereda el FONDO del anterior**: el `reset` de `gui.c` limpia los widgets de
  la pantalla pero no su estilo, y `GuiFlushBench` la deja azul oscuro (las etiquetas del siguiente cuestan
  de leer). No es del árbitro —el segundo programa vuelca igual, 149 ms— y hasta hoy no se veía: cada `run`
  reiniciaba la C5. En miVM cada ejecución empieza con el modelo limpio. **Arreglado el mismo día**: al
  terminar un programa, `bpvm_gui_reset` estrena una pantalla NUEVA (el tema, las banderas de fábrica, sin
  callbacks: un `onClick` en la pantalla también sobrevivía) y borra la del programa con todo lo suyo. Va en
  `gui.c`, dentro del pack de LVGL (los dos `.npk` regenerados) y de las imágenes con GUI. En la C5,
  `GuiFlushBench` y después `C5Touch` en el mismo arranque: fondo blanco y el botón responde (Eduardo)
  (`b2bcd5cb`).
- **Y la ROTACIÓN también se heredaba** —es del display, no de la pantalla—: `samples/C5GiraYSale.bp` gira a
  90° y termina, y `C5Touch` salía girado. `bpvm_gui_reset` vuelve a 0° antes de estrenar la pantalla (sólo
  si alguien giró); repetida la prueba, sale derecho (Eduardo) (`4f11b7fb`).
- `sim-smoke` estaba rojo en `main` porque su arreglo (`#537`) vivía en una rama sin fusionar; fusionada esa
  noche con la de `#534` (`622390b6`, `f037e050`): paridad 94/94 con `BusCount` y el frente `sim` en verde.
- **La fusión con `#534` trajo una interacción**: `#534` manda un SPI largo en tramos de 256, una llamada a
  la fachada por tramo, y con el árbitro cada llamada es un turno — otro dispositivo se colaría ENTRE dos
  tramos con el CS del programa abajo. Ahora la llamada entera de BP es UN turno (`bpvm_spi_tomar_como` y
  los tramos dentro), como dice §11.3.

**Lo que queda, a sabiendas:**
- **`G4.a3`**: `Spi.Device` en BP (el CS dentro del turno, la velocidad la pasa el objeto y se va la
  excepción temporal) y el repeated start de I2C.
- **La prueba de verdad, la TF y la pantalla a la vez en la C5**, cuando el ESP32 monte la TF (el punto de
  H3 «SD al común»).
- **La TF retiene el bus mientras la tarjeta graba** (hasta 2 s en el peor caso) y la pantalla espera hasta
  1 s: el protocolo deja soltarlo en esa espera, y se hará si la C5 lo pide.
- Los tres drivers de imagen fuera de la HAL BP (táctil del P4, pantalla del C6, táctil de la Discovery).

#### 11.9 `G4.a3`, el diseño (24-sep): el dispositivo, la TRANSACCIÓN y el tope del bus (Eduardo)

> Eduardo, sobre el bus: *«El bus se identifica por número en todas las placas. El número lo asigna el
> fabricante, no hay que inventar nada.»* Sobre agrupar: *«Las operaciones de SPI e I2C se deberían poder
> agrupar varias de forma similar a las transacciones en SQL, pero cuidado de no monopolizar el bus, el total
> en tiempo tiene que ser corto.»* Y después: *«Las transacciones también deberían poder llamarse desde C, por
> si un código nativo las necesita (de hecho tiene más sentido que se llamen desde ahí). El tiempo máximo
> configurable, pero en cualquier caso algo limitado para que otros dispositivos sobre el mismo bus puedan
> funcionar simultáneamente. Lo que hay que evitar es que si trabajo con un conversor analógico-digital con bus
> SPI y hago una conversión que me lleva 200 ms, mientras esté trabajando, nadie más pueda utilizar el bus.»*
> Sobre la velocidad: *«Lo normal no es ir cambiando la velocidad, solamente en algunos casos se cambia de unos
> dispositivos a otros. Así que sí, la velocidad inicial se puede utilizar por defecto.»*

- **La transacción se PREPARA y se ejecuta de UNA vez** (como `spi_message` e `i2c_transfer` en Linux), no un
  BEGIN…COMMIT con código en medio. Los hilos de BP van en un solo hilo del SO: un programa que se quedara el
  bus entre dos llamadas bloquearía ese hilo en cuanto otro hilo BP —la GUI pintando— pidiera el mismo bus (el
  bloqueo mutuo de §11.3), y el código de en medio puede tardar lo que quiera. Dentro de una transacción sólo
  hay transferencias —ni esperas, ni código de quien llama—: corta por construcción. Y entre dos transacciones
  seguidas, la cortesía del árbitro ya deja pasar al que espera.
- **El tope es del BUS**: lo dice su línea del ENV (`spi2=sck:6,mosi:7,miso:8,ms:20`, `i2c0=…,ms:20`); 20 ms si
  no lo dice, y nunca más de 100 (un `ms:` mayor es un error con nombre). Antes de empezar se calcula lo que
  ocupará el bus —en SPI 8 relojes por byte a la velocidad del dispositivo; en I2C 9 por byte, la dirección de
  cada mensaje incluida, a la del bus— y si no cabe, error con nombre y no se transfiere nada: «SPI 2: la
  transacción ocuparía el bus 32 ms a 1 MHz (máximo 20 ms)». La cuenta es entera: la misma en las dos VM.
- **El conversor de 200 ms** es el caso para el que está hecho: una transacción que arranca la conversión, el
  bus libre mientras convierte (un `sleep`, o su pin de listo) y otra que lee. No hay forma de esperar dentro.
- **Primero en C, en la HAL BP**: `bpvm_spi_transaccion(bus, cs, hz, modo, ops, n)` —el CS abajo de principio a
  fin— y `bpvm_i2c_transaccion(bus, addr, msgs, n)` —cada operación un mensaje, repeated start entre ellos y
  STOP al final: la semántica de Linux—. Los packs las tienen en la BIOS v8 (dos ranuras al final). BP va
  encima:
  - `Spi.Device(bus, cs, baudrate, mode)`: el bus por su número, ya establecido (el ENV o un `Spi.Bus`); reserva
    su CS (salida, en reposo alto). `write`, `read`, `transfer` y `writeRead` son una transacción cada una;
    `execute(t)` ejecuta una `Spi.Transaction` (sus `write`/`read`/`transfer`, que se van añadiendo). Una
    transacción se reutiliza: cada ejecución lee lo que tienen en ese momento los arrays de escritura y rellena
    los de lectura. `release()` suelta el CS.
  - `I2c.Bus`: `writeRead(addr, tx, ntx, rx, nrx)` y `execute(addr, t)` con una `I2c.Transaction`; hasta 256
    bytes por transacción (el tope de siempre de una transacción I2C, #534).
  - En el PC imprimen lo que hoy imprime el patrón a mano —el CS abajo, las tramas, el CS arriba—; la de I2C,
    en UNA línea: es una sola transacción.
- **Lo de siempre se queda como está**: `Spi.Bus`, las funciones sueltas e `I2c.Bus.write/read` —la velocidad
  del init, sin CS dentro, sin tope— son para quien está solo en el bus. La velocidad del init deja de ser la
  «excepción temporal» de §11.6b: es el contrato de lo de siempre (un `.mod` ya compilado que llame a
  `Spi.writeBus` lleva el builtin dentro).
- **El turno a mano** (`spi_tomar`…`spi_soltar`) no se puede calcular antes, y cortarlo a medias dejaría un
  dispositivo a mitad de algo (la pantalla a media escritura de su memoria). Queda para lo que ya lo usa —la TF y
  los packs de la v7—, MEDIDO: si pasa del tope, el log lo dice con el nombre de quién. `DriverSpi` pasa a
  transacciones (Eduardo: «1 OK»): cada comando y cada bloque ya tenían esa forma, y el bloque de LVGL son
  11 520 bytes, 2,3 ms a 40 MHz.
- **Nuestra TF hace hoy lo del conversor**: al arrancar retiene el bus mientras repite ACMD41 (hasta 1 s) y al
  escribir mientras la tarjeta graba (hasta 2 s). La especificación de la SD deja soltar el CS en las dos
  esperas: se arregla en H3 «SD al común», donde se prueba de verdad con la pantalla (Eduardo: «2 también OK»).
  Hasta entonces, la medida lo dice.
- **Un fallo que salió al mirar**: sin DMA el IDF no acepta más de 64 bytes por transferencia
  (`SOC_SPI_MAXIMUM_BUFFER_SIZE`; `spi_device_transmit` devuelve `ESP_ERR_INVALID_ARG`) y la cintura del ESP32 se
  lo pasaba todo de una vez: desde BP, un `Spi.write` de más de 64 bytes fallaba en el ESP32. La cintura trocea
  a 64; con el CS por GPIO, en el cable sólo quedan huecos.
- **El repeated start, por familia**: ESP32, `i2c_master_execute_defined_operations` (IDF v6); Pico, `nostop`;
  STM32 (U5), a nivel de registro —la HAL sólo lo trae para registros de 1 o 2 bytes—, probado en la Nucleo.

**Se prueba**: en el PC, `test-bus` contra la cinta (el CS dentro, la velocidad de cada dispositivo, el tope que
rechaza sin tocar el bus, el `ms:` del ENV, el I2C por mensajes, el turno a mano medido) y `samples/SpiDevice.bp`
en la paridad. En la C5, un programa habla con la microSD (CS 23, 400 kHz) por `Spi.Device` —CMD0 → 0x01—
mientras la GUI anima a 40 MHz en el mismo bus: dos dispositivos, dos velocidades, por primera vez en el
hardware; y el latido del táctil con `writeRead` mientras `DriverSpi` también lo lee.

#### 11.10 `G4.a3` hecho (24-sep): el dispositivo y la transacción, en C y en BP — y en la C5, la microSD con la pantalla

El diseño, en §11.9. **Lo que hay:**
- **La HAL BP**: `bpvm_spi_transaccion(bus, cs, hz, modo, ops, n)` y `bpvm_i2c_transaccion(bus, addr, msgs, n)`
  —calcular, tomar, transferir, soltar; y si no cabe, -1 con el porqué antes de tocar nada—; el tope del bus
  (`bpvm_bus_tope_ms`, el `ms:` del ENV: 1..100, 20 si no lo dice) con la cuenta entera de `bpvm_bus_cabe`; el
  turno a mano MEDIDO al dejarlo («SPI 2: CS 10, un pack tuvo el bus 16 ms, y el tope de un turno es 5 ms», en el
  log). En I2C, la cintura gana `transaccion` (repeated start entre mensajes) y `escribir_leer` es una de dos
  mensajes; un dispositivo que no contesta es -2 y no va al log (el táctil se sondea cada pocos ms).
- **La BIOS v8** (47 ranuras): `spi_transaccion` e `i2c_transaccion`. **`DriverSpi` la exige**: cada comando y
  cada bloque es una transacción; ya no trocea él (lo hace la cintura).
- **BP**: `Spi.Device(bus, cs, baudrate, mode)` con `write`/`read`/`transfer`/`writeRead`/`execute`/`release`,
  `Spi.Transaction`, `I2c.Bus.writeRead`/`execute`, `I2c.writeRead` e `I2c.Transaction` (builtins 258-262, en
  las dos VM con los mismos textos). La validación de los arrays que se guardan en una transacción la hace BP
  (el mismo texto sale solo en las dos VM); la de lo que llega al builtin, las dos VM.
- **Las cinturas**: el ESP32 trocea el SPI a 64 bytes (el fallo de §11.9: desde BP, más de 64 fallaba) y hace la
  transacción I2C con `i2c_master_execute_defined_operations`; la Pico con `nostop`; el STM32 a nivel de registro
  (TXIS/RXNE, TC entre mensajes, AUTOEND en el último, RELOAD para más de 255) —y su SPI trocea a 65 535, que la
  HAL truncaba en silencio—.

**Lo que salió por el camino:**
- **El ESP32 manda en lo que cabe en una transacción I2C.** El IDF la hace en una lista de órdenes del bus (START,
  datos, STOP) de como mucho 8 (`I2C_LL_CMD_REG_NUM`), y no deja encadenar dos listas sin un STOP en medio (se
  mira en `i2c_master.c`: sin STOP, la espera del final no llega). Una lectura de más de un byte son dos órdenes
  (el último byte va con NACK). La regla, IGUAL en todas las familias y en las dos VM —manda el piso—: hasta 3
  mensajes de al menos un byte y 8 órdenes (cada mensaje 2, una lectura larga 3, y el STOP). El error dice la
  cuenta: «la transacción necesita 9 órdenes del bus y caben 8».
- **El formato MOD8 no admite una clase usada antes de declararse** (`Device.execute(t: Transaction)` con
  `Transaction` debajo), y había un ciclo `Transaction`↔`Device`. `Transaction` va antes y su `executeOn` recibe
  los datos del dispositivo como enteros. Lo cazó el frente MOD8 de la red.
- **`PACK_LS` sólo lista los 16 primeros packs de la cadena**: la C5 tiene 19 (17 borrados), así que el listado
  —y el panel del IDE— enseñaba 16 borrados y ESCONDÍA los dos vivos. No es de `G4.a3`: tarea aparte.
- Al grabar la imagen nueva, el pack de LVGL no cargó: «realojado para OTRA dirección» —la imagen creció 64 KB y
  se movió su sitio—. Se regraban `Gui` y `DriverSpi`, en ese orden (el reparto de RAM es el sello, §H6.8).

**Cómo se sabe:**
- `test-bus` (el `ms:` del ENV, la cuenta, la transacción con el CS abajo de principio a fin y la velocidad del
  dispositivo, el tope que rechaza sin tocar el cable, el turno a mano medido —y una transacción NO medida ni con
  una cintura que tarda 12 ms—, el I2C por mensajes en UNA llamada a la cintura, el NAK, las órdenes, el mensaje
  vacío, la cintura sin repeated start), `test-bios` (47), `test-driverspi` (la cinta igual a la de DriverC5 byte a
  byte: 49 turnos, los 49 transacciones), `test-arbitro`, `test-sd`/`blk`/`sdsc`, `test-guipack`, `test-bmgr`;
  `samples/SpiDevice.bp` en la paridad (95/95) y la red entera; las ocho imágenes compiladas.
- **En la C5**: «bios: tabla COMPLETA (47 ranuras)» y «tope de un turno 20 ms» en el arranque; `GuiFlushBench`
  147 ms por pantalla (148 antes) y 1 ms por etiqueta con `DriverSpi` por transacciones. Y **`samples/C5Sd.bp`**:
  un programa habla con la microSD (CS 23, 400 kHz) por `Spi.Device` en el SPI 2 de la pantalla (CS 10, 40 MHz):
  CMD0 → 0x01, CMD8 → el eco 0x1AA, ACMD41 lista con el bus libre entre intentos, CMD58 → OCR 0xC0 (alta
  capacidad), el bloque 0 en UNA transacción de 997 bytes con su firma 55 AA, y **30 de 30** comandos intercalados
  con la pantalla (3 ms por vuelta). El tope, igual que en el PC (1001 bytes a 400 kHz: 21 ms, rechazado; 1000:
  20 ms, pasa). Y el latido del táctil con `writeRead` (repeated start) avanza igual que con escribir y leer:
  unos 32 000 por segundo — el CST3530 acepta el repeated start.

**Lo que queda, a sabiendas:**
- **El repeated start del STM32, sin probar en placa**: compila; la Nucleo, a la tarde (Eduardo).
- La TF (`bpvm_sd.c`) sigue reteniendo el bus en sus esperas: H3 «SD al común» (Eduardo, «2 también OK»).
- La C5 no prueba la CONCURRENCIA en placa (la pantalla y el programa van en el mismo hilo del SO): la prueba con
  dos hilos de verdad es `test-arbitro` en el PC; en placa llegará con la TF montada por el firmware (H3).

#### 11.11 `G4.b`, el diseño concreto (25-sep, sesión remota): el DMA en la cintura SPI del ESP32

El plan es el de §7.11 (Eduardo lo dejó para `G4`). Lo que ha cambiado desde entonces es que el primer punto ya está
hecho: desde `G4.a3` trocea la CINTURA, no quien llama, y `DriverSpi` manda cada franja de LVGL (11 520 B en la C5) en
UNA transacción de la BIOS v8. **El pack no se toca**, y la cautela de §7.11 sobre versiones sobra: el driver ya exige
la v8. Queda la cintura (`esp32/common/gpio_esp32.c`), que hoy parte esa franja en 180 transacciones de 64 B sin DMA.

**Medido antes, en la C5** (25-sep): `GuiFlushBench` 147 ms por pantalla y 1 ms por etiqueta; `SdCard` 4,2 s;
`C5SdGui`, la pantalla sola con 30 ms de peor hueco y con la TF 63 ms (0 ficheros distintos); `C5Bus` bien.

**Las decisiones:**
1. **El bus se abre CON DMA** (`SPI_DMA_CH_AUTO`) y `max_transfer_sz` de 16 KB: una franja en UNA transacción (también
   la de un panel de 320×480, 15 360 B); por encima, la cintura trocea a 16 KB. Lo mismo en el `init` de siempre.
2. **Lo pequeño, sin DMA ni búfer de rebote**: hasta 4 bytes van en los búferes de la propia transacción
   (`SPI_TRANS_USE_TXDATA/RXDATA`). Es el byte a byte de la TF: comandos, espera del token y de «ocupada».
3. **Polling para lo pequeño, interrupción para lo grande**: hasta 64 B, `spi_device_polling_transmit`
   (unos 12 µs frente a 22-24 con interrupción, según el IDF); por encima, `spi_device_transmit`, y la CPU queda libre
   mientras el DMA manda los 2,3 ms de una franja. La C5 tiene un solo núcleo, y es el hilo `io` el que se aprovecha.
4. **Un búfer en PSRAM o sin alinear** (el de LVGL vive en el bloque del pack, en PSRAM) lo copia el IDF a un búfer
   interno apto para DMA, de hasta 16 KB. Si no hubiera memoria, la cintura lo dice UNA vez en el log y devuelve -1:
   nada en silencio.
5. **`tx` NULL mantiene su contrato, «manda ceros»**: sólo lo usa `Spi.read` de BP. Va por un búfer estático de 64
   ceros, en tramos de 64.

**Cómo se prueba**: en la C5, `GuiFlushBench` (la estimación de §7.11: 147 → ~45 ms), la etiqueta (sigue en 1 ms),
`SdCard`, `C5SdGui` y `C5Bus`, contra las medidas de arriba; y compilan las cinco imágenes ESP32. **Falta VERLO en el
cristal**: la sesión es remota, y lo que BP no mide (que los píxeles sean los que tocan) lo mira Eduardo.

**`G4.b` hecho (25-sep), medido en la C5:**

| | antes | con DMA |
|---|---|---|
| `GuiFlushBench`, pantalla entera | 147 ms | **70 ms** |
| `GuiFlushBench`, una etiqueta | 1 ms | 1 ms |
| `SdCard` (500 añadidos y la relectura) | 4,2 s | 3,8 s |
| `C5SdGui`, la pantalla sola | 188 cambios, 30 ms de peor hueco | 188, 30 ms |
| `C5SdGui`, con la TF escribiendo | 176, 63 ms, 30 ficheros | 190, 53 ms, 32 ficheros, 0 distintos |
| `C5Bus` (el bus compartido y sus errores) | bien | bien |

Ni un error de SPI ni de SD en el log, y los packs `Gui` y `DriverSpi` cargan sin regrabar: `nm` da las mismas páginas.
Pero al rodata de la C5 le quedan unos 4 KB antes de cruzar a la página siguiente, y la próxima imagen que crezca
obligará a regrabar los dos. **La pantalla, 2,1 veces más rápida, y no 3,2 como estimaba §7.11 (~45 ms)**: el cable a
40 MHz son ~31 ms, y el resto no es peaje de transacciones. Lo más probable es pintar en PSRAM y copiar cada franja a
un búfer de rebote interno (el de LVGL vive en el bloque del pack, en PSRAM), pero sin medir no se reparte. Lo siguiente
de §7.11 —el doble búfer asíncrono y 80 MHz— sale de aquí. Dos detalles de paso: el búfer de ceros de la cintura y el
`0xFF` de la TF (`#553`) viven en RAM y no en `.rodata`, porque el DMA no lee de la flash y el IDF los copiaría a un
búfer de rebote en cada tramo.

### 12. El táctil que «se colgaba» era la HERRAMIENTA (23-sep, tarde): `wire_serie.py` reiniciaba la C5 al cerrar el puerto

**El síntoma** (Eduardo, al volver a la C5 tras horas con `C5Touch` en marcha): la pantalla bien, el botón no
reacciona; al recargar el programa, sí.

**Lo que creí, y lo que hice con ello.** Leyendo el chip a mano (`samples/C5TouchRaw.bp`) vi una vez al
CST3530 quedarse con su última trama; y el código del fabricante (Hynitron, `cst66xx`) hace dos cosas que no
hacíamos: el **MODO NORMAL** al despertarlo (`D0 00 04 00` dos veces —fuera el bajo consumo del I2C—, luego
`D0 00 00 00`, `D0 00 0C 00` y `D0 00 01 00`) y un **LATIDO**, su comprobación ESD (4 bytes de `D0 04 04 00`,
un contador que avanza mientras el chip escanea: unos 33 000 por segundo, medido con `samples/C5TouchCst.bp`).
Con eso `DriverSpi` ganó el modo normal al arrancar y un vigilante que reiniciaba el chip si el latido se
paraba (`431e2838`), y dejé una «prueba larga» en marcha.

**Lo que era.** Eduardo pulsó a los tres minutos y no reaccionó, y el log no tenía NI UNA línea tras la
primera lectura del táctil. Y el RUN aparecía a los 74 s de un arranque —si abrir el puerto reiniciara la
placa, habrían sido unos cientos de ms—. Medido con dos `info` separados 10 s: 142 s de uptime y luego
**9,5 s**: **la C5 se reinicia al CERRAR el puerto**. pyserial abre con DTR y RTS activos y Windows los
suelta al cerrar, y el USB-Serial-JTAG lleva dentro la lógica de reinicio de esptool. Así que cada `run` de
`wire_serie.py` dejaba el programa en marcha mientras escuchaba y lo MATABA al terminar, con la pantalla aún
pintada: ni el panel ni el CH32 se reinician. Todo lo que había funcionado —los cinco clics del 22, las cinco
pulsaciones de hoy— fue con el puerto abierto; los dos «cuelgues», con el puerto ya cerrado.

**El arreglo, en la herramienta**: el puerto se abre con DTR y RTS APAGADOS, fijados antes de abrir —no se
mueven ni al abrir ni al cerrar: tres `info` seguidos dan 97, 108 y 119 s—, y el reinicio es explícito:
`reset` (el verbo del wire) o `resetcable` (RTS 200 ms como esptool, reescribiendo DTR tras cada cambio porque
`usbser.sys` no manda el RTS nuevo sin eso; vale aunque corra un programa, que es cuando el wire no atiende).
**Medido**: con el puerto cerrado, cinco pulsaciones de Eduardo con su traza, y un latido de 5 min con 7 502
lecturas y ningún fallo. El precio: lo que se hace una vez por arranque —cargar los packs, leer el ENV— pide un
reset explícito tras `pack`, `packdel` o `env`.

**Lo que queda en el driver**: el modo normal al arrancar —lo pide el fabricante y no cuesta nada, pero **no
arregla nada demostrado**— y la traza del táctil, que ahora lee el latido ESD en su línea de cada 5 min (sólo
ella). **El vigilante se quitó** (Eduardo): reiniciaba un chip por un cuelgue que nunca existió, y chocaría con
el bajo consumo (un chip dormido no late, `#490`). Si la prueba larga —ahora sí válida— enseña un cuelgue de
verdad, la traza lo dirá con el latido; el diseño del vigilante está en `431e2838`.

**Lo que queda, a sabiendas:**
- ~~La prueba larga~~ ✅ **hecha** (23-sep, 16:34 → 19:04, con el driver sin vigilante): 2 h 13 min sin tocarla
  y el botón respondió; en la traza, 7 500 lecturas cada 5 min —unas 225 000— sin un fallo, y el latido ESD
  avanzando todo el tiempo (unos 32 000/s).
- **Una observación sin explicar**: en la 1.ª pasada de `C5TouchRaw`, a 100 ms, no salieron tramas nuevas (en
  la 2.ª, a 100 ms, sí). Puede que fuera el momento en que se tocó.
- ~~El IDE, sin medir~~ ✅ **medido y arreglado (19 h)** — y era PEOR: al desconectarse dejaba la C5 en **MODO
  DESCARGA** (esptool contesta sin reiniciarla: el cargador de la ROM). El firmware ni arranca hasta un reset a
  mano, y la pantalla sigue con la última imagen, así que la placa parece viva. `BpvmClient` abre con DTR=1 y
  RTS=1 —la Pico necesita DTR para mandar su salida— y cierra sin más. Un arnés que abre igual (purejavacomm)
  y cierra de tres maneras, dos veces cada una: **cerrar sin más → descarga; DTR abajo y luego RTS → descarga;
  RTS abajo y luego DTR → sigue como estaba**. Es la lógica de esptool: DTR abajo con RTS arriba pide reinicio
  con la patilla de arranque marcada. `BpvmClient.cerrarPuerto` baja RTS, luego DTR, y cierra; medido con el
  `BpvmClient` real (2 de 2) y con el IDE de Eduardo: el uptime sigue contando. En la Pico es lo mismo que
  cerrar; en los puentes USB-UART (S3, P4) RTS primero tampoco toca el EN —razonado, sin medir—.

**La lección** —estaba en la memoria y no bastó—: **antes de culpar al dispositivo, mirar si el instrumento le
cambia el estado**; aquí bastaba el `uptime`. Media tarde se fue en un modo normal y un vigilante contra un
fallo que ponía la herramienta.

### 13. `#468` (25-sep, sesión remota): el `Gui.mod` en flash — lo que queda es que `/lib` no tape a la zona

**Lo que ya estaba, aunque el plan (§3, pregunta 7; §4, `P4`) lo diera por pendiente.** El `MODULE_QUERY` del 23-jul
es el `STAT` por nombre de `#466` —el IDE pregunta por cada dependencia y contesta el resolvedor del RUN, sin un
gemelo del orden de búsqueda—, y desde `#493` ese resolvedor prueba la zona la última (`pack:<P>/X.mod`) y el Run del
IDE no sube lo que un pack ya sirve idéntico («ya en la placa, en un pack (XIP), idéntico»). Medido hoy en la C5 con
`GuiWinJson`: `Gui`, `Json`, `Collections` y `Str` salen del pack de la GUI, XIP («cargado XIP desde pack (codigo en
sitio)»).

**Lo que falta: `Core`.** En el mismo arranque: «dep 'Core' -> /lib/Core.mod · aviso: 'Core' del FS eclipsa al del
pack». Lo pone el INSTALADOR de fábrica (`#466`, `src/bpvm_mods.c`): en cada arranque repone en `/lib` la stdlib que
la imagen lleva embebida, y `/lib` va delante de la zona. En la C5 es un módulo; en una placa sin PSRAM (C6, C3, Pico 2,
Nucleo) con el `Stdlib.pack` grabado es TODO: la salida que dio Eduardo el 8-sep —*«si alguien quiere ahorrar memoria
puede montar su propio pack y subirlo»*— hoy no ahorra un byte, porque cada módulo del pack tiene en `/lib` un gemelo
idéntico que lo tapa. En el P4 pasa con los seis del pack de la GUI (su imagen los embebe todos).

**La propuesta** — la que la ficha dejó escrita el 4-sep («el instalador no repone en `/lib` lo que la zona ya sirve con
el mismo MAGIC+CRC»), con lo que ha pasado desde:
1. **La zona se monta ANTES que el instalador**, en las cinco familias. Hoy va detrás en todas: `#493` la puso
   «después de /lib» porque sólo necesitaba tenerla antes del primer RUN; montarla pide las particiones, no el FS.
2. **El instalador cede a la zona**: si la zona sirve X con los MISMOS bytes que el blob embebido (CRC), la copia de
   `/lib` sobra: no se instala si falta, y si está la de la imagen (idéntica al blob), se quita. Si `/lib` tiene una
   MÁS NUEVA (versión o generación), se queda, como hoy: el FS eclipsa, que es la regla de desarrollo de `#310`. Si
   la zona sirve OTRA versión que la imagen, nada cambia: la imagen repone su `/lib` como siempre.
3. **Borrar o formatear un pack re-sincroniza `/lib` en el acto** (`PACK_DEL`, `PACK_FORMAT`): lo que la zona deja de
   servir vuelve a `/lib` desde la imagen, sin esperar al siguiente arranque ni dejar un hueco en medio.

Se mide en placa: la C5 con `Core` del pack (sin el aviso) y la Metro con el `Stdlib.pack`, el libre del heap antes y
después (la ficha estimó ~22 KB con `Core`+`Json`+`Gui` en la C6).

**Lo que queda aparte, y es de Eduardo desde el 8-sep**: cargar la stdlib de fábrica EN SITIO desde la propia imagen —sus
blobs ya están en flash, mapeados, y el cargador acepta XIP con un puntero—, sin pack y sin `/lib`: el ahorro en TODAS
las placas sin grabar nada. Cambia el modelo de `#466` (la imagen dejaría de reponer `/lib`) y el `STAT` por nombre
tendría un origen más.

**Decidido (Eduardo, 25-sep, sesión remota):** las tres, como se proponen —la zona antes del instalador; el instalador
no instala lo que la zona sirve con los mismos bytes y quita la copia de la imagen si ya está (una más nueva en `/lib`
se queda); y `/lib` se re-sincroniza al borrar o formatear un pack—. **Y la vía de la imagen se CANCELA**: `#468` se
cierra con el pack; quien necesite la RAM graba el `Stdlib.pack`, que con esto ahorra de verdad.

**Hecho (25-sep, sesión remota), y MEDIDO en placa.** El resolvedor expone su búsqueda en la zona
(`bpvm_entry_zona`: el último pack de la cadena que lleve el módulo) y el instalador la usa: `memcmp` contra el blob
embebido, sin calcular CRC. Resultados nuevos `EN_ZONA` (falta y la zona lo sirve: en silencio) y `QUITADO` (cuenta
como escritura: la ESP32 guarda la partición por eso). La familia registra su instalación (`bpvm_mods_set_resync`) y
el wire común de packs la vuelve a correr tras `PACK_BURN_END`, `PACK_DEL` y `PACK_FORMAT` —al grabar también: sin
eso, un `Stdlib.pack` recién grabado no ahorraría nada hasta reiniciar—. La zona se monta antes que la stdlib en las
cinco familias (la ESP32 común, las dos variantes del P4, la Pico y el STM32).
- **La Metro** (su `SQLite.pack` lleva `Core` y `Str` canónicos): al arrancar, «`/lib/Core.mod` fuera: lo sirve el pack
  'SQLite' con los mismos bytes que la imagen (en sitio, XIP)»; `dep 'Core' -> PACK (XIP)`. **La objeción de Eduardo,
  en placa**: `packdel SQLite` → «preinstall: /lib/Core.mod» en el acto, sin reiniciar, y los programas siguen igual;
  regrabado el pack, fuera otra vez. `MathRango` y `FloatNat` idénticos en los tres estados.
- **La C5**: `Core` sale ya del pack `DriverSpi` (el último de la cadena con un `Core` canónico) y se acabó el aviso
  «el FS eclipsa»; los cinco módulos de la GUI, XIP. Con `GuiWinJson`, libre del heap 6529 → 6536 KB.
- **El P4**: nada que quitar. Su imagen no embebe `Gui`, `Json`, `Collections` ni `Str` —esas copias de `/lib` las
  subió alguien y no son del instalador— y el `Core` de su pack de la GUI no era el canónico.

**Lo que destapó**: el pack de la GUI llevaba un `Core.mod` de **generación 0** (**`#563`**: su proyecto compila
`bpstdlib/` como fuente y la receta no copiaba el canónico de `Core`; ahora el build del pack rechaza con nombre un
módulo de la stdlib sin sellar), y el cierre transitivo de los packs no leía MOD9 (**`#564`**: `duenosDeImports` se
quedó en MOD8, como `#555`). Queda regrabar la GUI del P4 con el pack bueno en su próximo `PACK_FORMAT` (le quedan
8 KB de zona); hasta entonces su `Core` sale de `/lib`, que es lo seguro.

### 14. `H6.b` en la C6 — el análisis (25-sep, tarde de placas; «OK» de Eduardo a empezar por la C6)

Con la tarjeta metida en la C6, `SD_INFO` contesta lo previsto por SD.3: *«la TF de esta placa aún no se usa: comparte
el SPI con la pantalla, que no pasa por el árbitro»*. Para usarla, la pantalla de la C6 tiene que ir como la de la C5
—por el pack y el driver genérico, sobre el bus arbitrado—, y eso es `H6.b` en la C6. Va primero la C6 porque su
camino ya existe entero: la imagen sin LVGL de la C5, `Gui.pack`, `DriverSpi` y el ENV. El P4 y la DK2 son otro
paso: necesitan `G5`, `'PANT'` publicado desde la imagen (DSI y LTDC no se prestan). El punto de restauración ya
está hecho: `dist/firmware-v7-25sep`.

#### 14.1 Lo que hay, medido hoy

- **La imagen**: LVGL 9.2.2 + `gui.c` + `gui_display_st7789.c` (`esp_lcd`, SPI2 a 40 MHz, DMA, dos búferes de 24
  líneas y `flush_ready` al acabar el DMA: el flush es **asíncrono**), con `BPVM_GUI` + `BPVM_LVGL` + `BPVM_BOARD_C6`.
  La GUI es 664 KB de flash y 107 KB de RAM estática de la imagen (§1, 22-sep).
- **La RAM al arrancar**: 282 604 B libres de DRAM antes de la VM; la VM se lleva un bloque de 128 KB (64 KB de
  heap); quedan 151 528 B, de los que la LVGL de la imagen pide en marcha lo suyo (libc).
- **El pack de la C5** (`lvgl.npk.RV32IMC`, el mismo para toda la ISA): **382 KB de `.bss`** —el pool de LVGL,
  256 KB (la perilla de `build_lvgl.sh`); el modelo `g_nodes[512]`, 96 KB; los dos búferes de dibujo, 23 KB; eventos
  e imágenes, 7 KB— y 717 KB de flash (`.text` 277 + `.rodata` 441, las fuentes 418 de ellos). `DriverSpi`: 12,6 KB
  de flash y 980 B de RAM. **Y todo lo que `gui.c` pide en el pack sale del pool de LVGL** (`gui_pack_glue.c`):
  imágenes, fuentes `.bin` y la captura de `GuiShot`, cuyo tope es 256 KB en el pack y **40 KB en la imagen de la C6**
  (`BPVM_SHOT_MAX`, el único `#ifdef BPVM_BOARD_C6` de `gui.c`).
- **La zona de la C6**: 1 261 568 B; con el `Stdlib.pack` grabado, 1 073 152 libres.
- **El perfil de la pantalla ya está probado en el host**: `make test-driverspi`, caso 3, con
  `lcd=st7789,spi:2,dc:15,cs:14,rst:21,bl:22,mhz:40,w:240,h:240,ram:240x320,inv` —reset y luz por GPIO, sin SWRESET,
  la luz encendida después de DISPON, y la tabla de desfases de la imagen (0/0, 0/0, 0/80, 80/0) byte a byte—.
- **La TF** (esquemático, `P2.0`): MISO GPIO5, CS GPIO4, sobre el mismo SCLK/MOSI (7/6) que la pantalla.
- ✅ **La línea base, medida** (25-sep, paso 1 de §14.6): **29 ms por pantalla entera y 2,8 ms por etiqueta**, tres
  pasadas idénticas; y `GuiRotCycle` en el cristal, bien en las cuatro orientaciones —cada marca en su esquina, sin
  franja ni desplazamiento— (Eduardo). El cable solo ya son ≈ 23 ms (115 200 B a 40 MHz): la C6 de hoy va casi al
  límite del bus porque dibuja mientras envía. Con el pack síncrono cabe esperar ≈ 52 ms (los 70 de la C5 en 240×320),
  ≈ 1,8 veces más lenta en pantalla entera; una etiqueta, pocos ms en los dos.
  **Antes, `GuiFlushBench` no medía**: en la C6 daba «0 ms por pantalla (10 vueltas en 2 ms)». LVGL sólo repinta
  cuando vence su refresco (`LV_DEF_REFR_PERIOD`, 33 ms) y el ejemplo metía diez `__guiRunOnce` seguidos: en la C5
  salía bien por casualidad —su envío síncrono tarda 70 ms, más que el período— y en la C6 las diez cabían dentro.
  Ahora cada vuelta espera 40 ms y se cronometra sólo la llamada que pinta.

#### 14.2 Lo que cambia en la imagen: lo mismo que la C5

| fuera | dentro |
|---|---|
| `gui_display_st7789.c`, `src/gui.c`, los componentes `lvgl` y `esp_lcd`, `EXTRA_COMPONENT_DIRS` | `src/gui_pack.c` (los trampolines), con `BPVM_GUI` + `BPVM_GUI_PACK` |
| `BPVM_LVGL`, `BPVM_BOARD_C6`, `LV_CONF_PATH` | el motor de la SD por SPI: `bpvm_sd.c`, `bpvm_sd_blk.c`, `bpvm_sd_spi_env.c`, `bpvm_blk.c`, `fs_fat.c` y FatFs (los mismos ficheros que la C5) |
| `CHIP_SD_REGISTRAR()` → «sin SD» | `CHIP_SD_REGISTRAR()` → la línea `sd` del ENV, como la C5 |

Se espera −664 KB de flash y −107 KB de RAM estática: se mide con el `.map`, no se da por hecho.

#### 14.3 El ENV de la C6 (es de la unidad, no de la imagen)

```
spi2=sck:7,mosi:6,miso:5
lcd=st7789,spi:2,dc:15,cs:14,rst:21,bl:22,mhz:40,w:240,h:240,ram:240x320,inv
sd=bus:2,cs:4                 (+ khz: — la C5 necesitó 10 000: su MISO va por la matriz; aquí también, a medir)
packRam=<KB>                  (§14.4)
```

#### 14.4 La RAM: la decisión de fondo

Sin la GUI en la imagen, la C6 tendría unos **389 KB** libres al arrancar (282 + 107; a medir). El pack de la C5
pide 382 KB: **no cabe**, y no puede caber con la VM al lado. Hace falta un pack para placas sin PSRAM. Sus piezas:

| pieza | hoy (C5) | para la C6 | cómo |
|---|---|---|---|
| pool de LVGL | 256 KB | 64-96 KB | la perilla que ya existe (`build_lvgl.sh imc <KB>`) |
| modelo `g_nodes` | 512 nodos, 96 KB | 256 nodos, 48 KB | `GUI_MAX_NODES` pasa a perilla del build (hoy es un `#define` fijo: una línea con `#ifndef`) |
| captura de `GuiShot` | tope 256 KB | tope 40 KB | `BPVM_SHOT_MAX`, que ya es perilla (`#ifndef`), como en la imagen de hoy |
| búferes de dibujo | 23 KB | 23 KB | — |
| fuentes | 418 KB de flash | todas caben (≈ 900 KB con los módulos, de 1 073 libres), o un juego 12-24 (≈ 90 KB) | perilla de fuentes |

Con un pool de 96 KB y 256 nodos, el pack pide **≈ 175 KB**: 389 − 175 − 128 (la VM) = **≈ 86 KB** para el sistema.
Hoy, después de la VM quedan 151 KB, pero la LVGL de la imagen se come una parte en marcha. O sea, a primera vista
es **neutro en RAM** para un programa con GUI, y con `packRam=0` —sin GUI— la RAM entera vuelve a la placa: *«la
pantalla la decide el usuario»*. El pool se dimensiona midiendo, en la C5, lo que `GuiWinJson` y `GuiShot` usan de
verdad (`lv_mem_monitor`), no a ojo.

**Las dos formas de tener un pack pequeño**:
- **(A) una segunda compilación del pack `imc`** con las perillas de arriba (p. ej. `GuiMini.pack`): sin tocar
  código de la VM ni de LVGL, y la C5 no cambia; el precio, dos packs `imc` que construir y distribuir, y que el
  usuario (o el IDE, por la RAM que publica el INFO) elija cuál graba.
- **(B) un solo pack con el pool sacado de la arena en marcha** (`lv_mem_add_pool` sobre el bloque de `packRam`): un
  pack por ISA para todas las placas; pero la arena tiene UN dueño (§8.2) y SQLite ya la pide, así que en el P4 —que
  lleva los dos— haría falta repartirla: es un cambio de diseño, no una perilla.

Propuesta: **(A) ahora**; (B) sólo si aparece un tercer tamaño de RAM.

#### 14.5 La velocidad: el riesgo

Hoy la C6 pinta con el `esp_lcd` del IDF, con el flush asíncrono: LVGL dibuja la franja siguiente mientras el DMA
manda la anterior. El pack va por `DriverSpi` y la fachada, con DMA desde `G4.b` pero **esperando a cada franja**: en
la C5, 70 ms por pantalla de 240×320 a 40 MHz, así que en la C6 (240×240) cabe esperar ≈ 52 ms. Si la C6 de hoy es
claramente más rápida, **`H6.b` la haría más lenta**, y eso adelanta la decisión pendiente del doble búfer asíncrono
(una ranura nueva en la BIOS, §7.11 y `G4.b`). Por eso la línea base va PRIMERO, con una prueba que mida igual en los
dos caminos.

#### 14.6 Cómo se prueba, en orden

0. **La línea base**, con la imagen de hoy (la del punto de restauración): la prueba de velocidad arreglada para que
   mida en los dos caminos, `GuiRotCycle` en el cristal (las cuatro esquinas, Eduardo) y la tanda.
1. **El pack pequeño** (A): el pool medido en la C5, `size` del `.npk`, el oráculo del `.npk` byte a byte.
2. **La imagen de la C6 sin LVGL**: el `.map` (lo que adelgaza), la RAM libre al arrancar y el tamaño de la VM.
3. **El ENV y los packs**: las cuatro líneas de §14.3; `DriverSpi` y el pack pequeño grabados (el IDE dice «cabe» o
   «no cabe» antes de grabar), y un `reset`.
4. **En la placa**: la tanda (`GuiShot`, `GuiWinJson`: 14/14 como hoy), `GuiRotCycle` en el cristal, la velocidad
   contra la línea base, y la TF: `SD_INFO`, `SdCard` y un `C6SdGui` (la TF y la pantalla a la vez, como `C5SdGui`).
5. Si se rompe y no se arregla: la imagen de `dist/firmware-v7-25sep` y el ENV de hoy, y se cancela sin perder nada.

#### 14.7 Las preguntas (Eduardo decide)

| # | pregunta | propuesta |
|---|---|---|
| 1 | el pack para placas sin PSRAM | **(A)**: una segunda compilación del pack `imc` con las perillas; (B) sólo si hace falta |
| 2 | los nodos del modelo en el pack pequeño | **256** (48 KB menos); 512 si algún programa real los necesita |
| 3 | las fuentes del pack pequeño | **todas** mientras quepan en la zona de la C6; el juego 12-24 cuando haga falta sitio |
| 4 | si el pack sale más lento que la C6 de hoy | medirlo primero; si la diferencia se nota, el doble búfer asíncrono entra en `H6.b` |
| 5 | el P4 y la DK2 | después, con `G5` (`'PANT'` desde la imagen), como estaba previsto |

#### 14.8 Lo que decidió Eduardo (25-sep, tarde de placas)

- **Un segundo pack de LVGL para pantallas pequeñas** (pregunta 1: la (A)), **con las fuentes pequeñas** (pregunta 3):
  *«si la pantalla es muy pequeña como la de la C6 no tiene sentido las fuentes grandes»*. La pregunta 2 (los nodos)
  no tuvo respuesta: va la propuesta, 256, que encaja con «pantallas pequeñas».
- **El doble búfer** (pregunta 4, y la decisión pendiente de §7.11): *«creo recordar que LVGL era opcional, podía
  funcionar con los dos. No es nada urgente pero estaría bien soportarlo»*, con dos condiciones: **que lo elija el
  usuario** y **que haya memoria** (porque la pantalla tiene poca resolución o porque la placa tiene mucha). O sea, no
  entra en `H6.b`: si el pack pequeño pinta más lento que la C6 de hoy, se acepta por ahora.
- **Los 80 MHz: CANCELADOS** —*«si el límite oficial es 60 MHz nos ceñimos a ese límite, no tiene sentido probar fuera
  de límites, podría ser que a nosotros nos funcionase y a otro usuario no»*; la hoja del ST7789 da 62,5 MHz—. **El P4 y la DK2** (pregunta 5): como estaba previsto, con `G5`.

**Lo que se vio en el código al contestarle** (no una suposición): el pack de hoy YA declara dos búferes de 11,5 KB
(`pant_lvgl.c`: `lv_display_set_buffers(…, s_buf1, s_buf2, …, PARTIAL)`), pero su flush es síncrono
(`lv_display_flush_ready` al volver del driver), así que el segundo no aporta nada: 11,5 KB pagados sin uso. Así que
el pack pequeño va con **un búfer**. Y el doble búfer que pide Eduardo tiene dos formas que no hay que confundir:

| | qué es | memoria | qué falta |
|---|---|---|---|
| **dos franjas, flush asíncrono** | el DMA manda una franja mientras LVGL dibuja la otra (lo que hace hoy `esp_lcd` en la C6) | la de hoy: dos franjas | una ranura de la BIOS: un envío SPI que avisa al acabar |
| **dos pantallas enteras** | el modo FULL/DIRECT de LVGL, sin parpadeo | 240×240 en RGB565 son 112,5 KB cada una | sólo memoria: es la de «pantalla pequeña o mucha RAM» |

Las dos, cuando se hagan, las elige el usuario en su línea `lcd=` (p. ej. `buf:1|2`), y la placa dice con nombre si no
le cabe. Con un búfer, el pack pequeño pide ≈ 131-163 KB (pool 64-96 + 48 de nodos + 11,5 de franja + 7).

#### 14.9 Hecho (25-sep): `H6.b` en la C6, y con él `H3`

**La línea base** (`f0412363`): 29 ms por pantalla entera y 2,8 por etiqueta con `esp_lcd` asíncrono, y el giro bien en
el cristal. `GuiFlushBench` no medía (el refresco de LVGL, 33 ms, y diez `__guiRunOnce` seguidos); arreglado.

**El pack pequeño** (`b719359a`): `GuiMini`, cuatro perillas del mismo LVGL + `gui.c` —256 nodos, fuentes hasta 24 px,
un búfer, la captura a 40 KB— y el pool. **El pool, medido** con el log que ahora escribe el pack al acabar cada
programa: pico de 6 KB (`GuiFlushBench`), 7 (`GuiWinJson`) y 13 (`GuiShot`, con la captura) en la C5, y 54 KB con 255
etiquetas. Por el camino, `#568` (la tabla de widgets llena, muda). Probado primero en la C5 (con `packRam` de 512 a
1024, OK de Eduardo): la tanda de GUI igual, 62 ms por pantalla, y «tras 255 etiquetas: … sitio para 256».

**La C6** (`7dbbc2a7`): la imagen como la de la C5 —1 243 920 → 603 104 B; 282 604 → 386 812 B de heap al arrancar—, las
cuatro líneas de §14.3 y los packs `Stdlib`, `DriverSpi` y `GuiMini`. **Lo que no salió como se esperaba, y por qué**:

- **La RAM**: con el pool de 96 KB (`packRam=176`) la VM bajaba a 100 KB (36 KB de heap) y `SdCard` moría con «readFile:
  sin memoria». El margen del sistema de la C6 (103 824 B) se había medido con LVGL en la imagen y el comentario se lo
  atribuía a LVGL; medido otra vez SIN ella, el sistema sigue pidiendo ≈ 94 KB de pico (mínimo histórico 22 360 B con
  la VM en 128): no era de LVGL. El margen se queda y el pool baja a **64 KB** (54 de pico con la tabla llena):
  `packRam=136`, la VM en su objetivo de 128 KB (64 de heap), como antes de `H6.b`.
- **La velocidad**: 35 ms por pantalla, no los ≈ 52 que se extrapolaban de la C5. En la C5 la RAM del pack —la franja
  de dibujo incluida— está en PSRAM; en la C6, en la SRAM interna. El pack síncrono es un 20 % más lento que el
  `esp_lcd` asíncrono de antes, no 1,8 veces.

**Medido en la C6**: la tanda de V7 entera 14/14 (`compat/informes/tanda_h6b_c6.md`); `GuiRotCycle` en el cristal, cada
marca en su esquina y sin franjas (Eduardo); **la TF por fin en uso** —`SdCard` «los seis»; `C5SdGui` RESULT OK: 27
ficheros escritos y releídos con la pantalla a la vez, 0 distintos, peor hueco 30 ms sola y 62 con la TF, lo mismo que
la C5—, con lo que **`H3` queda cerrado**; y `GuiLleno`, «sitio para 256». Quedan de `H6.b` el P4 y la DK2 (`G5`).

### 15. `G5` — el P4 y la DK2: su pantalla publicada como `'PANT'` desde la imagen, y LVGL fuera (26-sep, análisis)

Eduardo, 26-sep: *«Yo continuaría por H6 y dejamos H5 para el final»*. De H6 queda `H6.b` en el P4 y la DK2, que es
`G5` (§7.8): el panel de esas dos placas no se puede sacar a un pack —MIPI-DSI y LTDC son periféricos de la familia, no
se prestan por la BIOS—, así que su driver se queda en la imagen y PUBLICA lo mismo que `DriverSpi`; LVGL sale de la
imagen y viene del pack, como en la C5 y la C6.

#### 15.1 Lo que hay, mirado hoy en el código

- **La frontera ya lo prevé**: `bpvm_pant.h` dice que `'PANT'` lo publica «hoy un pack propio por placa, mañana también
  la imagen (el P4 por MIPI-DSI y la DK2 por LTDC, `G5`)». No hay nada que diseñar en el contrato.
- **El P4 tiene todo lo demás.** Carga packs nativos (la BIOS y el cargador de la familia ESP32; el pack de LVGL se
  probó allí el 22-sep, con un panel nulo y al lado de su LVGL). El pack de su ISA existe y está al día
  (`lvgl.npk.RISCV`, 747 KB, construido el 25-sep junto al de la C5 y la C6). Y su driver
  (`esp32p4/main/gui_display_dsi.c`, 729 líneas) ya hace lo de un `'PANT'` —vuelca una zona con
  `esp_lcd_panel_draw_bitmap`, con el giro por software, y lee el GT911—, sólo que se lo da a un LVGL de la imagen
  (`lv_display_create`, `lv_indev`). Lo que cambia: el driver publica `init`, `size`, `flush`, `rotate` y `touch`, y
  LVGL y `gui.c` salen de la imagen. La imagen única sigue eligiendo el panel por el ENV (`display=`).
- **La zona de packs del P4**: el 24-sep le quedaban 8 KB (SQLite, la GUI vieja y `DriverNulo`); el pack de LVGL no
  cabe sin un `PACK_FORMAT` y regrabar, como se hizo en la C6.
- **La DK2 NO tiene lo demás.** El STM32 tiene packs de DATOS (los `.mod` en XIP, de H3), pero **no packs nativos**: ni
  BIOS ni cargador de `npk`. Los tienen la ESP32 y la RP2350: `bios_*.c` y `pack_*.c`, unas 130 + 250–370 líneas por
  familia sobre lo común de `src/`. Tampoco hay LVGL compilado como pack ARM (la receta ARM sí existe: el motor de
  SQLite de la Metro). Su driver es `stm32/port/gui_display_ltdc.c` (170 líneas) y el táctil va por el I2C 2. O sea,
  `G5` en la DK2 es: **los packs nativos en el STM32** (el adaptador de la familia, una vez; y abre de paso SQLite y lo
  demás), **LVGL como pack ARM** y su `'PANT'` por LTDC.

#### 15.2 El plan, por pasos que cierran algo

| paso | qué | se prueba con |
|---|---|---|
| **G5.a** | el P4: `'PANT'` desde la imagen (el DSI y el GT911) y la imagen sin LVGL; la GUI por el pack de LVGL | la tanda con GUI en el P4 (`GuiShot`, `GuiWinJson`…), el táctil y la velocidad contra la de hoy |
| **G5.b0** | los packs nativos en el STM32 —la BIOS y el cargador de la familia— y LVGL como pack ARM | SQLite en la Nucleo, con el `SqlDemo` de la Metro |
| **G5.b** | la DK2: `'PANT'` por LTDC y su táctil, y la imagen sin LVGL | la tanda con GUI en la DK2 |

#### 15.3 Las preguntas (propuesta; Eduardo decide)

| # | pregunta | propuesta |
|---|---|---|
| 1 | ¿el orden? | el P4 primero (lo tiene todo) y la DK2 después |
| 2 | la DK2 pide antes los packs nativos en el STM32: ¿se hacen, o la DK2 se queda con LVGL en la imagen? | hacerlos: «la base es finita, lo demás en packs» vale también para el STM32, y el adaptador de la familia se hace una vez |
| 3 | el `PACK_FORMAT` del P4 y regrabar SQLite y la GUI | cuando `G5.a` esté listo, con su OK en ese momento |

**Decidido** (Eduardo, 26-sep): 1, el P4 primero; 2, **packs nativos para la Discovery** (el adaptador del STM32);
3, el `PACK_FORMAT` del P4 cuando `G5.a` esté listo.

**`G5.a`, el código** (`1a48870a`; compila, falta la placa). `gui_display_dsi.c` con dos modos por el build: con
`BPVM_LVGL`, lo de antes; con `BPVM_GUI_PACK`, la tabla `'PANT'` (`init`, `size`, `flush`, `rotate`, `touch`),
publicada con `bpvm_bios_publica` desde `main.c`. El giro —un panel DPI no gira por hardware— con las cuentas de
LVGL 9.2.2 portadas tal cual (la zona, los píxeles y el punto del táctil), para que `Gui.setRotation` quede como antes.
La imagen: de 1 410 496 a 750 592 B (−644 KB) y la `.bss` de 889 a 521 KB (−368 KB). En placa falta: la imagen, el
`PACK_FORMAT` y regrabar SQLite y la GUI (la poda tiene que elegir el motor `RISCV`, no el `RV32IMC` de la C5/C6), y
la tanda con GUI, el táctil, el giro y la velocidad. Punto de restauración: `dist/firmware-v7-25sep`.

**`G5.a`, en la placa ✅** (26-sep, con el OK de Eduardo a cada paso). Grabada la imagen, la zona de packs se corrió (de `0x40154000` a `0x400b4000`: la app es 644 KB más pequeña) y los tres packs dijeron «realojado para OTRA dirección», como se esperaba; `PACK_FORMAT` y SQLite y la Gui regrabados —la poda eligió el motor `RISCV`—, sin `DriverNulo` (la pantalla la publica ahora la imagen). En el log: «el P4 publica 'PANT' desde la imagen», los dos motores vivos y «pack 'LVGL' resuelto (88 funciones)». La tanda de V7 **14/14** (`GuiWinJson` idéntica; y los nativos AOT otra vez en el P4, que ya es de la ABI 9: NatNew 7/7, NatV7 4/4, FloatNat 23/23). `GuiFlushBench`: **25 ms** por pantalla entera (480×800) y **2 ms** por etiqueta —más rápido que la C5 (70) y la C6 (35) por SPI—; la comparación con el LVGL de la imagen NO se midió antes de grabar (un descuido: la imagen de `dist` permite medirla si hace falta). `GuiRotCycle`, en el cristal (Eduardo): los giros bien y los textos en su sitio, dos vueltas. `C5Touch`: 5 toques, 5 pulsaciones. `GuiRotDemo`: el botón «Girar +90» respondió en las cuatro orientaciones —el punto del táctil, bien girado—. Sigue sin pasar por la HAL BP el bus del táctil (el I2C 0, abierto por la imagen), como antes: es la nota de `G4.a1`.

#### 15.4 `G5.b0` — los packs nativos en el STM32 (26-sep, análisis)

Mirado hoy en el código, **lo común ya está en la imagen del STM32** —compila todo `src/`: la escalera y el plan
(`bpvm_npack.c`), la tabla BIOS (`bpvm_bios*.c`), el punto de encuentro (`publica`/`busca`)— y **la zona de packs ya se
graba**: los packs de datos de H3 van por `PACK_BURN_*` con la cintura de flash del U5, que invalida el ICACHE tras cada
borrado y cada escritura (`stm32_flash.c`), así que un motor recién grabado se lee y se ejecuta sin restos de la caché.
Falta lo de la familia, calcado de la RP2350 (la zona es XIP en una dirección fija, la misma vista para leer y ejecutar):

- `stm32/port/bios_stm32.c` (~130 líneas, como `bios_esp32.c`): `BPVM_BIOS_TABLA` con la voz (el log persistente), los
  chivatos y el ENV del STM32 (`board_mgr_stm32_env_get`, que ya existe), y el ancla. La BIOS se verifica en cada
  arranque, como en la Pico.
- `stm32/port/pack_stm32.c` (~200, como `pack_pico.c`): montar, planear, explicar y saltar; y el disparo en el primer
  RUN (`packs_cargar_una_vez`), no en el arranque.
- `PACK_BURN_BEGIN` con la RAM: hoy `board_mgr_stm32_handle` deja `pack_ram_base`/`pack_ram_size` a 0, y el IDE diría
  «esta placa no da RAM a packs nativos».
- Sin linked folders nuevos: los dos ficheros van en `port/`, que ya lo es.

Y tres cosas que deciden el diseño:

**(a) La coma flotante.** Las dos imágenes STM32 compilan con `-mfloat-abi=hard` (su `.cproject`); los motores ARM
—`sqlite.npk.ARMV8`, el `mini` y el de LVGL cuando se construya— son `softfp`, los de la Metro. La escalera los
rechazaría en su peldaño 4 («otra convención de coma flotante — daría números mal»). Es el choque del 25-ago con el
`.mdn` (en el STM32, `(3.0+5.0)/2.0` daba NaN sin un solo error), que se resolvió fijando a `aapcs` la frontera de los
helpers (`BPVM_AOT_FP_ABI`), con gcc comprobando que el tipo y la definición coinciden. Para los motores hay tres salidas:

| | qué | lo bueno | lo que cuesta |
|---|---|---|---|
| **A** | las dos imágenes STM32 a `softfp` (una línea de cada `.cproject`) | la familia ARMv8-M con UNA ABI: un `.mdn` y un `.npk` para la Pico y las STM32, la escalera tal cual, y el IDE ya elige bien (`arch` 40 → `arm` → `ARMV8`) | recompilar las dos y pasar la tanda en las dos |
| B | seguir en `hard` y aceptar el motor `softfp` porque su frontera no lleva coma flotante | nada que recompilar | la escalera tendría que separar la ABI de la imagen de la de la frontera. Hoy es verdad —la BIOS, `'PANT'`, la API de LVGL y la de SQLite no pasan ningún `float` ni `double` por valor (SQLite los saca por arrays)—, pero la API de un pack se llama por un puntero CONVERTIDO (se busca por nombre): un `float` futuro daría números mal sólo en el STM32, y gcc no lo vería |
| C | un motor más por pack (`ARMV8` en `hard`) | nada que recompilar en las imágenes | cada motor se compila dos veces para ARM, los packs crecen (SQLite, 464 KB; LVGL, del orden de los 747 del de RISC-V) y el IDE y `PackTool` tendrían que elegir también por la ABI |

Comprobado para A: el toolchain de CubeIDE (GNU Tools for STM32 14.3) trae la multilib `thumb/v8-m.main+fp/softfp`; no
hay ninguna `.a` precompilada en los dos proyectos; y nada del código depende de `hard` (sólo `BPVM_AOT_FP_ABI`, que
sigue valiendo y pasa a ser cinturón y tirantes). La FPU se usa igual —`softfp` calcula con ella; sólo cambia por dónde
viajan los `float` en una llamada—. Y si un día alguien volviera a compilar el STM32 en `hard`, la escalera lo diría con
su nombre.

**(b) La RAM de los motores, en una dirección FIJA.** El sello de un motor lleva la dirección de sus estáticos, así que
el bloque tiene que estar siempre en el mismo sitio. En el STM32 la VM es un array estático (`s_vm_mem`: 512 KB la
Nucleo, 1536 la DK2) que se mueve con cada enlace. Propuesta, con lo que ya hay:
- los dos `.ld` ponen `s_vm_mem` lo PRIMERO de la RAM, en una sección propia sin carga: `0x20000000` siempre (no hace
  falta ponerla a cero en el arranque: `bpvm_init` no lo supone, y cada RUN ya reutiliza el mismo búfer);
- el bloque es una **reserva con nombre DELANTE** de ese array —la del planificador común (`bpvm_mem`), la misma con la
  que la Metro aparta la BD— del tamaño que diga el ENV: `packRam=<KB>` (la clave genérica de la ESP32) o, si no está,
  `SQLite=<MB>` con su regla (en la Nucleo dirá «no cabe»: su mínimo son 2 MB). La VM se queda con el resto;
- así el bloque empieza en `0x20000000` cambie la imagen o no, y cambiar `packRam` sólo mueve su final (un motor que ya
  no quepa lo dice la escalera: «regrábalo»);
- un detalle de orden: hoy la memoria de la VM se planifica ANTES de leer el ENV (`stm32_mem_planificar` va antes que
  las particiones): hay que leerlo antes, como la ESP32 (`env_temprano`).

Cuánto: `packRam=512` deja la VM de la DK2 en 1 MB (de 1,5), y a SQLite le sobra (8 KB de estáticos y el resto de
arena: su MEMSYS5 no pide mínimo; los 2 MB son la regla de la clave `SQLite=`). En la Nucleo, `packRam=256` la dejaría
en 256 KB. LVGL (`G5.b`) pide ~382 KB de estáticos.

**(c) La zona de packs y los dos bancos de la flash.**
- La DK2, leída hoy (`part`): el FS, 2016 KB, y los packs, **528 KB** en `0x0837C000`. SQLite ARM (464 KB) cabe; LVGL
  no: `G5.b` tendrá que repartir de nuevo (un FS más pequeño), y eso borra el FS — con el OK de Eduardo, en `G5.b`.
- `stm32_flash_erase` saca el banco de la PRIMERA página y borra N seguidas. Si un borrado cruzara del banco 1 al 2 (en
  la DK2 la frontera es `0x08200000`; la zona empezaría en el banco 1 con un FS de menos de 496 KB), hoy funcionaría
  **de chiripa**: la página 256 desborda el campo `PNB` (8 bits) justo sobre el bit `BKER`, que es el que elige el
  banco (visto hoy en `FLASH_PageErase`, del HAL). Propuesta: partir el borrado por banco (unas líneas), para que valga
  por contrato. Hoy no muerde: la zona de la DK2 está entera en el banco 2, y la de la Nucleo empieza en el suyo.

**El plan de `G5.b0`:**

| paso | qué | se prueba con |
|---|---|---|
| 1 | (con A) las dos STM32 a `softfp` | compilarlas; la tanda de V7 en la DK2, y en la Nucleo cuando esté conectada |
| 2 | `bios_stm32.c` y `pack_stm32.c`; el bloque (los `.ld`, `packRam=`, la reserva); la RAM en `PACK_BURN_BEGIN`; el borrado por banco | el `mini` (1,6 KB): la cadena entera sin nada más que pueda fallar |
| 3 | SQLite | el `SqlDemo` de la Metro en la DK2, con `packRam=512` |
| 4 | LVGL como pack ARM (`build_lvgl.sh arm` y el `.mdn` ARM de la Gui) | construirlo; se prueba en `G5.b` |

La prueba, en la **DK2** (conectada, COM12; la Nucleo hoy no está) en vez de la Nucleo de §15.2; la Nucleo después, con
el mismo código. Grabar las imágenes y tocar el ENV (`packRam=`), con el OK de Eduardo en su momento.

#### 15.5 Las preguntas de `G5.b0` (propuesta; Eduardo decide)

| # | pregunta | propuesta |
|---|---|---|
| 1 | la coma flotante: A, B o C | **A**: las STM32 a `softfp`, una ABI para toda la familia ARMv8-M |
| 2 | el bloque de RAM | `s_vm_mem` al principio de la RAM por el `.ld`, y `packRam=` como reserva delante |
| 3 | el borrado de flash que cruza bancos | partirlo por banco ahora, en el paso 2 |
| 4 | la placa de la prueba | la DK2 (conectada); la Nucleo después |

**Decidido** (Eduardo, 26-sep): 1, **A** —y antes la pregunta de si no se podía «que funcionen por soft pero que, si
hay hardware, lo aproveche», que es justo lo que es `softfp`: la convención de `soft` hacia fuera y la FPU por dentro—;
2, «decide tú»: la propuesta (`s_vm_mem` lo primero de la RAM y `packRam=` como reserva delante); 3, sí; 4, la DK2
(«en las pruebas finales se prueban todas»).

La regla que queda, en sus términos: **el juego de instrucciones decide qué hardware hay** (el M0, sin FPU; el M4 y el
M33, de 32 bits; el M7 y los M55/M85, de 32 o de 64 según el chip), y por eso hay un motor por juego —como el P4 y la
C5 en RISC-V—; **la convención con que viaja un `float` NO la decide el chip** (con el mismo M33, la Pico era `softfp`
por su SDK y el STM32 `hard` por CubeIDE): es del proyecto, y en ARM es `softfp`, la misma que `soft`, así que todos
los ARM hablan igual tengan FPU o no. Un motor por juego de instrucciones, nunca dos por la coma flotante. (Un matiz
para cuando llegue una placa M4: lo compilado para el M4 corre en nuestros M33, pero no al revés.)

**Paso 1 ✅** (`ab87ece6`): las dos `.cproject` a `softfp`. En el artefacto, las `.elf` sin `Tag_ABI_VFP_args` (antes
«VFP registers») y con la FPU (`FPv5`, `SP only`). En la DK2, `floatbench` (π por Leibniz, tres veces): con `hard`,
2,523–2,524 s interpretado y 0,04 s nativo; con `softfp`, 2,521–2,522 s y 0,04–0,041 s — nada que perder. La tanda de
V7 14/14 (`compat/informes/tanda_g5b0_dk2.md`), `FloatNat` 23/23 thunks nativos.

**Paso 2, el código** (`ec0a48b2`): `bios_stm32.c` y `pack_stm32.c` (calcados de la RP2350: XIP en dirección fija, la misma
vista para leer y saltar); `s_vm_mem` en la sección `.bss.bp_vm`, que los dos `.ld` ponen LO PRIMERO de la RAM
(`.bp_vm (NOLOAD)`, y el arranque lo comprueba contra `__bp_vm_start`/`__bp_ram_start`: si no, no hay bloque); la
reserva `packRam=` (o `SQLite=`) delante de la VM con el planificador común, con el ENV leído antes
(`board_mgr_stm32_env_temprano`); el disparo en el primer Run; la RAM en `PACK_BURN_BEGIN`; la BIOS y el ancla
comprobadas en cada arranque; y `stm32_flash_erase` partido por banco. Medido en el `.elf`: `s_vm_mem` =
`__bp_vm_start` = `0x20000000` en las dos (`.bp_vm` de 1536 KB en la DK2 y 512 en la Nucleo, `.data` detrás), y la
marca del ancla UNA vez en cada imagen. **En la DK2**, el arranque: «vm: 1536 KB», «packs: sin bloque de RAM (ni
packRam= ni SQLite= en el ENV)», «bios: lista (47 ranuras, v8)» y «ancla: en 0x080DEDD4 (la busqueda la encuentra, y es
LA misma)», en 15 ms.

**Y por el camino, `#575`** (`848ded39`): al preparar SQLite salió que sus `.mdn` (y el de Gui2) seguían en la ABI 8 desde que
la VM subió a la 9 esta mañana —SQLite y Gui2 sin natives en toda placa con imagen nueva— y que el AOT no podía
regenerarlos (`#542` contaba `import v1 from pack "SQLI"` como un módulo que falta). Arreglado, regenerados (un byte
distinto: la ABI), los packs reconstruidos y una red nueva en `compat.sh check` (`compat/packs_abi.py`). Los números
de `SQLite.pack` podado para ARM: 456 KB, y el `mini` 24 KB: caben juntos en los 528 KB de la zona de la DK2.

**Pasos 2 y 3, en la DK2 ✅** (26-sep, con el OK de Eduardo al lote). La zona de datos, repartida de nuevo —FS 1008 KB,
packs 1536 KB, el banco 2 entero (`0x08280000`): los 528 KB de antes se quedaban cortos para la Gui de `G5.b`—, con el
FS reformateado y `PACK_FORMAT` de la zona nueva; `packRam=512` en el ENV: «vm: 1024 KB … | packs 512 KB delante» y
«packs: bloque de RAM 512 KB @0x20000000 … en su sitio fijo». El `mini`, podado a 24 KB y sellado para `0x08280000` y
la RAM `0x20000000`: el Run que dispara el cargador vuelve en 37 ms, pero su informe no se ve —el log de ejecución está
apagado por defecto (`#423`) y hace falta `log=1`—. SQLite: 456 KB en `0x08286000`, su RAM en `0x20000010` —el tope
por encima de los 16 B del `mini`: el reparto de G1, en el STM32— y **`SqlDemo` entero**: «AOT: SQLite.mdn cargado del
pack (3184 B, 16 thunks)», la base en la flash, los `double` (20.5…) y los `long` (las marcas de tiempo) bien, la media
21, en 3650 ms; la salida cuadra con lo que inserta el programa (no había una de referencia guardada). La tanda de V7,
otra vez 14/14, con la VM en 1 MB detrás del bloque. Queda el paso 4: LVGL como pack ARM (`build_lvgl.sh arm`) y el
`.mdn` ARM de la Gui; se prueba en `G5.b`.

**El informe del `mini`** (con `log=1` en el ENV de la DK2 —OK de Eduardo— y quitado después): «2 motor(es), 2
cargable(s) | bloque 512 KB @0x20000000: estaticos 7744 B, arena 504 KB»; el `mini` en `0x082800B0`, «paso 4 ok (tabla,
magic, version, log)» … «paso 10 ok (env_get v6)» —`packRam` le devuelve `512`— y «TODO OK -> 0»; SQLite con `.data`
6360 + `.bss` 1368 en `0x20000010`, «arena 504 KB en 20001E40», «initialize OK — motor arrancado y vfs 'bp'
registrado», «API publicada como 'SQLI' — 17 simbolos, v1»; «packs: 2 motor(es) cargado(s)». Y de propina, `#468` en
el STM32: «'Core' cargado XIP desde pack (codigo en sitio)».

**Paso 4 ✅** (`17dbf2d2`): `build_lvgl.sh arm` —la receta de la Metro y las STM32, ahora que las dos son `softfp`—:
`lvgl.npk.ARMV8` de 660 KB (código 206 KB y datos de sólo lectura 429 KB, las fuentes; `.bss` 383 KB), cero símbolos
sin resolver, y **el oráculo del realojado IDÉNTICO a `ld`** (1618 relocalizaciones, enlazando la misma lista en flash
`0x08300140` y RAM `0x20002000`). El `.mdn` ARM de Gui2 (10 thunks) y `Gui.bpbuild` con `aot.targets` ARM y RISC-V. El
pack universal de la Gui, 2,3 MB; **podado para ARM, 744 KB**: cabe en los 1056 KB que quedan en la zona de la DK2.

**`G5.b0` CERRADO.** Lo que deja dicho para `G5.b`: los estáticos de LVGL (383 KB) y los de SQLite, con `packRam=512`,
dejarían a SQLite unos 122 KB de arena; el reparto de la RAM de la DK2 se decide ahí, cuando LVGL salga de su imagen y
libere la suya.

**Y en la Nucleo** (27-sep, en la sesión de placas de `P2b.3.d`): lo mismo, con el mismo código —`packRam=256`, el `mini`
y SQLite, `SqlDemo` entero—; los detalles, en §H5.P2b.3.

#### 15.6 `G5.b` — la DK2: su pantalla publicada como `'PANT'` desde la imagen, y LVGL fuera (26-sep, análisis)

Lo mismo que `G5.a` en el P4. Mirado hoy:

- **El driver** (`stm32/port/gui_display_ltdc.c`, 170 líneas, entero bajo `BPVM_LVGL`): un framebuffer RGB565 de
  800×480 en la SRAM (750 KB) que el LTDC escanea (capa 0), la luz por PWM (TIM3_CH4), un búfer de dibujo de 48 líneas
  (75 KB) y el GT911 por el I2C 2 (`hi2c2`, el de CubeMX), leído por sondeo. **No gira**: `setRotation` avisa «no
  soportado en este display (LTDC) todavia» y no hace nada.
- **Lo que sale de la imagen con LVGL**: sus fuentes y su código (buena parte de los 996 KB de `.text` + `.rodata`), la
  tabla de nodos del modelo (`g_nodes`, 96 KB), el búfer de dibujo (75 KB) y lo que LVGL pide a `malloc`. **Lo que se
  queda**: el framebuffer, que es del LTDC. Medido en el `.elf` de hoy: `.bss` 1036 KB, de los que 750 son el
  framebuffer.
- **El build**: la DK2 compila LVGL como una entrada `lvgl` de `sourceEntries` en su `.cproject` (versionado); la
  carpeta enlazada vive en el `.project`, que es local y puede quedarse. Y `gui.c` no está guardado contra
  `BPVM_GUI_PACK`: como el STM32 compila todo `src/`, con `gui_pack.c` las funciones `bpvm_gui_*` estarían dos veces (el
  comentario de `gui_pack.c` cuenta que ya chocó el 23-sep, al revés).
- **El pack de la Gui** dibuja en dos búferes de 11 520 B (`BPVM_PANT_DRAW_BYTES`): a 800 px son ~7 líneas por franja,
  frente a las 48 de hoy. **Medido ANTES de tocar nada** (la lección de `G5.a`): `GuiFlushBench` en la DK2, con LVGL en
  la imagen, **37 ms por pantalla entera y 2 ms por etiqueta**, igual en tres pasadas.

**El plan** (como el P4):

1. `gui_display_ltdc.c` con dos modos por el build. Con `BPVM_GUI_PACK`, la tabla `'PANT'`: `init` (la luz y la capa
   del LTDC, una vez), `size` (800×480), `flush` (a 0°, fila a fila al framebuffer; girado, cada píxel a su sitio con
   las cuentas de LVGL 9.2.2 que ya se portaron para el P4 —aquí sin búfer intermedio: el destino ES el framebuffer—),
   `rotate` y `touch` (el GT911 como hoy, con el punto girado); publicada desde el arranque (`stm32_pant_publicar`).
   **La DK2 gana el giro**, que hoy no tiene.
2. `gui.c` bajo `#if !defined(BPVM_GUI_PACK)`, el espejo de la guarda de `gui_pack.c`: uno u otro, por el build.
3. El `.cproject` de la DK2: fuera `BPVM_LVGL`, `LV_CONF_INCLUDE_SIMPLE` y la entrada `lvgl`; dentro `BPVM_GUI_PACK`.
4. La memoria de la DK2: se mide la `.bss` sin LVGL y se fija `BOARD_VM_BYTES`. Cuenta de partida: sin los 171 KB de
   la imagen, `BOARD_VM_BYTES` de 1536 a **1792 KB** con **`packRam=768`** deja la VM en 1 MB, como hoy, y el bloque de
   los motores con LVGL (383 KB), SQLite (8 KB) y ~377 KB de arena para SQLite (con `packRam=512`, ~122).

**Se prueba con**: la tanda de V7 (con `GuiWinJson` y `GuiShot`), `GuiFlushBench` contra 37 / 2 ms, `GuiRotCycle` (el
giro, nuevo en la DK2; en el cristal, Eduardo) y el táctil (`C5Touch` y `GuiRotDemo` en las cuatro orientaciones). En
la placa: la imagen, el pack de la Gui para ARM (744 KB; caben en los 1056 KB libres de la zona) y `packRam=` —con el
OK de Eduardo en su momento—.

#### 15.7 Las preguntas de `G5.b` (propuesta; Eduardo decide)

| # | pregunta | propuesta |
|---|---|---|
| 1 | el giro en la DK2, que hoy no tiene | hacerlo ya en `'PANT'`: son las cuentas del P4, y es para lo que `'PANT'` tiene `rotate` |
| 2 | la memoria de la DK2 | medir la `.bss` sin LVGL y, si cuadra, `BOARD_VM_BYTES` 1792 KB con `packRam=768` |
| 3 | las franjas de 7 líneas del pack (hoy 48) | medir primero; si la pantalla entera sale mucho peor que 37 ms, el tamaño del búfer como perilla (el pack ARM es el mismo para la Metro y las STM32) |

**Decidido** (Eduardo, 26-sep): 1, 2 y 3, como la propuesta.

#### 15.8 `G5.b`, en la placa ✅ (26-sep)

El código (`568e976a`): `gui_display_ltdc.c` con dos modos por el build —lo común (el framebuffer, la luz y la capa,
el GT911) y la tabla `'PANT'` con `BPVM_GUI_PACK`—, el giro con las cuentas del P4 compuestas en una y escrito DIRECTO
al framebuffer (el píxel lógico (X,Y) va a (Y, H-1-X) a 90°, (W-1-X, H-1-Y) a 180° y (W-1-Y, X) a 270°), `gui.c` bajo
`#if !defined(BPVM_GUI_PACK)`, el `.cproject` sin LVGL (el build de 27 a 10 s) y `BOARD_VM_BYTES` a 1792 KB. En el
`.elf`: cero símbolos `lv_`, `.text` + `.rodata` de 996 a 369 KB, `.bss` de 1036 a 857 KB, y ~350 KB libres hasta la
pila con la VM en 1792.

En la DK2 (con el OK de Eduardo): la imagen, `packRam=768` («vm: 1024 KB … | packs 768 KB delante») y el pack de la
Gui para ARM, con su RAM en `0x20001E40`, justo encima de la de SQLite —el reparto de G1, con tres motores—. La
primera tanda SALTÓ las dos pruebas de GUI: el INFO no decía el tamaño de la pantalla, porque con la GUI por el pack
eso exige el pack cargado y el STM32 sólo lo cargaba en el primer Run. Arreglado como en la ESP32 (`bba97e0b`: el INFO
dispara la carga «una vez»). Entonces: la tanda de V7 **14/14** (`GuiWinJson` idéntica, `GuiShot` bien, pintada ya por
el pack); `GuiFlushBench` **48 ms** por pantalla entera y **1 ms** por etiqueta, contra 37 y 2 con LVGL en la imagen
—la pantalla entera, un 30 % más lenta por las franjas de 7 líneas; la zona sucia, que es lo que hace una interfaz,
más rápida: se deja así, y el tamaño de la franja sería una perilla del build del pack si hiciera falta—;
`GuiRotCycle` bien en el cristal (Eduardo: las esquinas y los textos en su sitio), **el giro que la DK2 no tenía**;
`C5Touch` 5 de 5; `GuiRotDemo` respondiendo en las cuatro orientaciones (once giros por toque; los seguidos, pulsaciones
de Eduardo).

**`G5` hecho**: las cuatro placas con pantalla —la C5, la C6, el P4 y la DK2— pintan con el LVGL de un pack, y la
imagen de cada una ya no lleva LVGL.

**✅ H6 CERRADO** (Eduardo, 26-sep).
