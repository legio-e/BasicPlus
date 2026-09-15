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
