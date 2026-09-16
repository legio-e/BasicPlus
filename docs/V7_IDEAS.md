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
| STM32 | ❓ **por comprobar** | el ST-LINK sí puede resetear el target, pero por su propia vía, no por el puerto serie |
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
