# BasicPlus — Pendientes vivos

> Documento **vivo**: bugs abiertos y limitaciones/decisiones del lenguaje que
> aplican a cualquier versión (se pueden tocar en cualquier momento, también
> durante V3 — son bugs/pulido, no features).
>
> **Mapa de docs:**
> - **`V3_BACKLOG.md`** — lo aparcado para V3 (fuente única del backlog de V3).
> - **`HECHO_V2.md`** — snapshot **inmutable** del backlog tal como quedó al
>   cerrar V2 (el "diario" de cómo se resolvió cada cosa; no se actualiza).
> - **`HECHO_V1.md`** — puntero a `PROJECT_ROADMAP.md` (crónica de v1).
>
> Convención: B=bug · L=limitación · N=hallazgo · M=mejora.

---

## 🔴 Bugs abiertos

> Bugs **delicados** (vtable/módulos + GC) movidos a **`V4_BACKLOG.md`** (27-jun): `B-174b`,
> `B-gc-allocanchor`, `B-freeref-no-recursivo` — exigen tocar slots/GC con red de pruebas, no
> son fixes contenidos de V3.

### #441 — el `.mdn` no recuerda con qué RECETA se compiló
`mdnIsStale` (`BpIde/.../PicoExplorer.java:1239`) decide por **fecha**: si el `.mod` es más nuevo, el
`.mdn` se rehace. Eso caza el fuente cambiado, pero **no** que hayan cambiado los *flags* de
compilación ni que el `.mdn` sea de **otra familia** — su propio comentario ya nombra "los `.o` sin
memoria de sus flags" como una de las cuatro mordeduras de artefacto rancio del proyecto.
Lo destapó **#440** el 17-ago: al añadir `-mcmodel=medany` a `RISCV_P4_FLAGS`, **todos** los `.mdn`
de RISC-V ya generados quedaron mal —con direccionamiento absoluto, o sea cuelgue mudo en placa— y
ninguno se habría regenerado, porque ningún `.bp` había cambiado. Hubo que borrarlos a mano.
Forma del arreglo: sellar en el `.mdn` una huella de su receta (arquitectura + hash de los flags) y
que un sello distinto cuente como rancio, igual que la fecha. La arquitectura ya viaja en la
cabecera (`arch=40`/`243`) pero **nadie la compara** con la de la placa antes de subirlo.

*(Cerradas el 17-ago y sacadas de aquí, que estaban dando una lista de bugs más larga que la real:*
*`#389` —el estrechamiento de `Object` ya COMPRUEBA en ejecución en las dos VMs, opcode `CHECKCAST`,*
*`05acc0d`; era el último bug conocido del lenguaje— y `N-readfile-msg-skew` —miVM pegaba la ruta*
*normalizada por Java, o sea distinta por SO; gana el mensaje de la VM-C, paridad 28/0/0, `RfSkew.bp`*
*en el repo—. Ficha completa de las dos en `notas/FICHAS.md`.)*

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

## 🟢 Pulido (no urgente)

- **El «pwm» del arranque y el del INFO no son la misma unidad, y se llaman
  igual.** El log de boot dice `pwm=12` (SLICES, de `board_desc`) y el INFO
  responde `24` (SALIDAS: cada slice tiene canales A y B, que es la cifra que
  anuncian las placas). Las dos son correctas y el porqué está comentado en
  `pico/repl_v1.c:1089`, pero quien ponga las dos líneas una al lado de otra ve
  una contradicción y va a buscarla — pasó el 17-ago. Basta con que cada una
  DIGA su unidad (`pwm=12 slices` / `PWM: 24 salidas`). El campo del wire
  conserva el nombre histórico `pwmSlices` aunque lleve salidas, que es la otra
  mitad de la confusión.


- **N-listado-plano-trunca-mudo — el árbol del IDE se corta con muchos ficheros,
  y sólo lo dice el log.** El recorrido que alimenta el árbol es PLANO y recorre
  el FS entero, con tope de **16 directorios** y **96 entradas por directorio**.
  Al pasarse trunca: avisa al log del device, pero **al usuario no le dice nada**
  — el árbol sólo enseña menos cosas, que es la peor forma de fallar.
  Observado por Eduardo el 8-ago al montar una tarjeta SD (V5/H2), donde deja de
  ser teórico; pero el tope **es de siempre** y aplica a cualquier versión y a
  cualquier placa con el FS interno lleno.
  Arreglo bueno: **árbol perezoso** — pedir los hijos al expandir con `LIST_DIR`
  (existe desde V5/H2 y ya reporta cuántas entradas dejó fuera). La consola ya lo
  usa: `dir [ruta]` sí avisa por pantalla cuando trunca.

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
