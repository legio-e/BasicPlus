# V7 — ideas y diseño

> Documento abierto el 21-ago con la visión de Eduardo sobre el código nativo en packs.
> **Va en `docs/` y no en `notas/` a propósito**: `notas/` está en el `.gitignore` y por
> esa vía ya se perdió un fichero (el `SqlDemo.bpbuild`, reconstruido el 19-ago).

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

**4. La RAM, y la corrección de Eduardo.** Ver la nota de `notas/V6_IDEAS.md` sobre la
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
