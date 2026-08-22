# H13 — Guión de pruebas finales de V5

> **Borrador para acordar con Eduardo (20-ago).** El de V4 (`H13_PRUEBAS.md`, 2.413
> líneas, 7 placas) sigue siendo válido como referencia de lo que se puede probar.
> Esto es la versión de V5, y es **deliberadamente más corta**. El porqué va primero,
> porque un recorte sin criterio es una prueba que no prueba.

---

## Por qué esta vez es más corto

**V4 cambió los cimientos** —el modelo de memoria y el sistema de ficheros— y por eso
tocaba pasar la batería entera por las 7 placas: cualquier cosa podía haberse movido.

**V5 es aditivo.** Tarjeta SD, SQLite, el ORM y los packs se apoyan en esos cimientos
sin tocarlos, y los cimientos llevan desde agosto corriendo. Repetir la batería completa
mediría otra vez lo que V4 ya midió.

**Pero V5 tiene UN cambio con riesgo de cimiento, y hay que tratarlo como tal:** el
20-ago, `Comparable` ganó cinco métodos para las conversiones. Eso **corre las ranuras
de su vtable**, y con ellas las de los cinco envoltorios y las de cualquier clase que la
extienda. Además se regeneró la stdlib entera.

Y ese cambio tiene una propiedad desagradable: **en el PC pasa siempre**, porque se
reconstruye todo junto. En placa no, porque allí sobrevive lo viejo — un `.mod` rancio en
`/lib`, un pack grabado hace días. **No es teórico: pasó ese mismo día.** `DaoDemo` y
`GenDemo` compilaban y **fallaban en ejecución** hasta reconstruir la librería de SQLite
contra el `Core` nuevo.

De ahí la forma del guión: **una puerta de regresión que no se puede saltar**, y luego
sólo lo nuevo y lo que quedó pendiente.

---

## Punto de partida (20-ago, tarde) — ya hecho

Todo construido **del mismo árbol y en una sola tanda**, y sellado. Al empezar en placa
no hay que reconstruir nada: se flashea desde `dist/firmware/`.

- ✅ toolchain en limpio · stdlib (27 módulos) · `Stdlib.pack` · `SQLite.pack`
- ✅ el `.npk` de ARM **regenerado desde sus fuentes** (464.127 → 464.399 B). Era el
  rancio; ahora el binario se corresponde con el código. ⚠️ **Por eso la Metro es
  obligatoria**: ese binario no se ha ejecutado nunca.
- ✅ las 5 imágenes en `dist/firmware/` con su `SHA256SUMS.txt`
- ✅ `BasicPlus-5.0-win.zip` (23 MB), con sus dos autocomprobaciones
- ✅ Puerta 0 pasada: batería **48/18/4/0 con diff vacío** y los tres demos de BD `OK`

📌 **Y el ZIP no se rehace hasta el final** (norma de Eduardo, 20-ago): si salen fallos
pequeños se anotan, se corrigen si hace falta, y el ZIP se monta **una vez**, al cerrar.

📌 **Reparto previsto: dos días** (viernes y sábado). Lo razonable es partirlo por
puertas, no por placas — así cada día termina con algo cerrado:
- **Día 1:** Puerta 1 en las tres familias (el ABI, que es lo que bloquea) + Puerta 2 en
  la Metro.
- **Día 2:** Puerta 2 en el P4, y la Puerta 3 entera (las fichas abiertas).

---


## ✅ RESULTADOS — día 1 (21-ago)

**Pico (RP2350) — CERRADA.** Imagen `bpvm_pico.uf2` rehecha con el arreglo de `#414`.
- `ListGets` → **21 líneas idénticas** a miVM y a la VM-C de host. Puerta 1 pasada
  para la familia RP2350: el ABI nuevo de `Comparable` va bien en placa.
- `PacksDemo` con `Stdlib.pack` grabado → **32 líneas idénticas** al host.
- 🩸 **Bug `#414` encontrado en placa y arreglado** (`494d7bba`): los cuatro builtins
  de packs vivían dentro de `#ifdef BPVM_GUI`. Nunca habían funcionado en una placa
  sin pantalla. Ver la tabla de imágenes pendientes, arriba.
- 🩸 **`PacksDemo.bp` no compilaba** — `List.get()` devuelve `Object` desde `#389`.
  Arreglado con los captadores tipados.

📌 **Y un hallazgo de MÉTODO, que vale más que los dos bugs:** la cascada
Java→C→placa no podía cazar el `#414` porque **la VM-C de host se construye CON
GUI y los firmwares sin pantalla no**. El doble era más permisivo que el original.
*«Lo que no funciona en C tampoco funcionará en la Pico»* sólo se sostiene si el C
que se prueba lleva la MISMA configuración que la placa. Hoy no la lleva.
⏭️ Para V6: que la batería de host corra también en la configuración sin GUI, o al
menos que el censo compare los builtins compilados de cada imagen.


**Metro (RP2350B) — Puertas 1 y 2 CERRADAS** (salvo GUI, que esta placa no tiene).
Misma imagen que la Pico; la variante (48 GPIO, 8 ADC, 16 MB flash, 8 MB PSRAM) la
detecta el arranque. Todas las comparaciones son contra la salida del host.

| prueba | resultado |
|---|---|
| `ListGets` (el ABI de `Comparable`) | ✅ **21/21 idénticas** |
| `PacksDemo` (packs en placa) | ✅ lista los packs y su contenido |
| `SqlDemo` (SQLite) | ✅ **25/25**, única diferencia la ruta `/sd/medidas.db` |
| `DaoDemo` (ORM a mano) | ✅ **25/25 sin ni una diferencia** |
| `GenDemo` (ORM generado por `@BD`) | ✅ **10/10 sin ni una diferencia** |
| SD | ✅ monta sola: `sd: montada en /sd (particion en el bloque 2048)` |
| `SdDoc` (la API de ficheros, FS interno) | ✅ **5/5 idénticas** — los ejemplos de `TARJETA_SD.md` hacen lo que dicen |
| `SdCard` (la TARJETA, sobre `/sd`) | ✅ **los seis**: escribir/leer, añadir, truncar, bytes crudos, 32.000 B en 500 trozos, y releerlo |
| `Bench` (AOT al vuelo) | ✅ **105×** — `fib(28)` interp 8.819 ms → AOT **84 ms**, y los dos dan `317811` |
| streaming `#294` | ✅ `SQLite.pack` (1,13 MB) subido sin incidencias |

📌 **Lo que vale de verdad del `SqlDemo`**: con UNA ejecución quedan probados el
`.npk` de ARM regenerado (que nunca se había ejecutado), SQLite corriendo desde un
pack en **XIP**, la arena de 2 MB que reserva el ENV, y la escritura en la SD.
📌 **Y del `DaoDemo`/`GenDemo`**: el apóstrofo de `O'Brien` escapado por el `Where`
y por el DAO generado — que es lo que separa un ORM de una inyección de SQL— y el
`delete` repetido devolviendo `false` en vez de reventar.
📌 **Y el `Bench` prueba la OTRA cadena de nativo**, que es independiente de la del
`.npk`: el IDE **genera** un `.mdn` de ARM para esta placa (`1 thunk, 64 B nativo`),
lo sube y la VM lo ejecuta. La de SQLite es nativo *precompilado y empaquetado*; ésta
es nativo *generado al vuelo*. Las dos quedan verificadas en la Metro el 21-ago.
📌 **El arreglo de los blobs de `IO`/`Math`, VERIFICADO en placa el 21-ago.** Tras
reflashear con la imagen corregida, el arranque preinstala `Math.mod (2320 bytes)` e
`IO.mod (2401 bytes)` —los tamaños nuevos— y **no queda ni una línea de `NO es el de
esta imagen`**. La placa y la distribución dicen por fin lo mismo.
📌 **Y el ancla se demostró sola**: al cambiar la imagen se movió de `0x10058B6C` a
`0x10058ABC`, y el log dice *«la busqueda la encuentra, y es LA misma»*. Es justo para
esto que se diseñó buscándola en vez de clavar la dirección.


**Puerta 3 — cola de `#439` en la Metro: ✅ PASA (21-ago).**
`CuelgaLog` → `kill` → `reset` → al arrancar, `log: RAM SUPERVIVIENTE (lineas de
ANTES del reset)`. El log post-mortem sobrevive tambien en RP2350, no solo en el P4.
⚠️ **Alcance exacto**: se probo el **reset por wire**, que se implementa con watchdog
(`causa del reset = WATCHDOG`). El **boton fisico** (pin de RUN) es otro camino y NO
se ha probado. Si se quiere cerrar del todo, repetir pulsando el boton.
📌 De la misma traza salen tres confirmaciones que no se buscaban:
- `GC por TABLA de handles: 4033/4096 slots` — el segundo eje de presion del `#430`
  disparando por NUMERO de handles y no por volumen. Es el caso que motivo la ficha.
- `RUN finished: terminado por KILL` — el `kill` corta limpio.
- `fin de RUN: la memoria vuelve a su sitio (0 bloques sin liberar)` — el guardian
  del `#339` dando el visto bueno.

## Puerta 0 — en el PC, antes de tocar una placa

Gratis, mecánico y encuentra regresiones sin gastar un flasheo.

- [ ] **Reconstruir TODO en limpio, y en este orden.** No es ceremonia: un `out/` sucio
      ya mintió una vez este mes (tenía módulos del 15-ago y el pack salió contaminado).
  - [ ] `mvn -f lexer-java/pom.xml clean install` · `mvn -f miVM/pom.xml clean install`
  - [ ] `rm -rf bpstdlib/out` y reconstruir la stdlib como proyecto
  - [ ] `rm -rf bpstdlib/sqlite/out`, copiar `nativo/*` y reconstruir el pack de SQLite
  - [ ] `packs/Stdlib.pack` y `packs/SQLite.pack` actualizados
  - [ ] `bash bpgenvm-c/scripts/regen_all_mods.sh` (los blobs de las 3 familias)
  - [ ] los **5 firmwares** compilados sin avisos nuevos
- [ ] **La batería:** `bash scripts/h13-lista.sh` → **48 corren · 18 compilan · 4 NO
      compilan · 0 fallan**, y `diff` vacío contra la corrida anterior.
- [ ] **Los tres demos de BD** en el host (`make test-sqldemo` + `sqldemo.exe`):
      `SqlDemo`, `DaoDemo`, `GenDemo`, los tres `status=OK` y 0 bloques sin liberar.
- [ ] **Paridad dual-VM** de los samples nuevos de esta versión: `ListGets`, `SdDoc`,
      `CastExt`, `ListaBp`, `SyncXMod` — salida idéntica byte a byte.
- [ ] `make test-env test-part test-boot test-bmgr test-pack test-crc test-sd test-sdio
      test-sdsc` en `bpgenvm-c`
- [ ] `mvn test` en `miVM`

> **Regla de V4 que se mantiene:** lo que se prueba y lo que se publica es el **mismo
> fichero, bit a bit**. Cada imagen se genera una vez, se copia a `dist/firmware/` y se
> sella con `sha256sum`. Si hay que recompilar, se vuelve a sellar y **se vuelve a probar**.

---


## ✅ IMÁGENES: TODAS AL DÍA (21-ago)

El arreglo de `#414` toca `src/builtins.c`, común a las cinco imágenes. Pero **sólo
afecta a las que se compilan SIN `BPVM_GUI`**, y eso está *demostrado*, no supuesto:
compilando `builtins.c` con `-DBPVM_GUI` antes y después del arreglo sale un objeto
**byte a byte idéntico** — cerrar el `#ifdef` y reabrirlo es un no-op cuando está activo.

| imagen | estado |
|---|---|
| **Pico / Metro** (`bpvm_pico.uf2`) | ✅ rehecha, resellada y **probada en placa**. Lleva además el arreglo de los blobs de `IO`/`Math` |
| **ESP32-S3** (`bpvm_esp32_merged.bin`) | ✅ **rehecha el 21-ago** (502.144 → 504.224 B); 5 símbolos de packs verificados con `nm` |
| **STM32 Nucleo** (`bpvm_stm32_nucleo.bin`) | ✅ **rehecha el 21-ago** (246.848 → 247.448 B); 5 símbolos verificados |
| **ESP32-P4** (`bpvm_esp32p4_merged.bin`) | ✅ no necesita rehacerse — tiene GUI, el arreglo es no-op |
| **STM32 DK2** (`bpvm_stm32_dk2.bin`) | ✅ no necesita rehacerse — tiene GUI (`BPVM_GUI` en su `.cproject`) |

📌 Las cinco entradas de `SHA256SUMS.txt` verifican. **Ya se puede flashear cualquiera
de las cinco placas sin más preparación.**
✅ **Sin deuda: `SdCard` repetido en la Metro con la imagen FINAL** (602.112 B, la
de `dist/firmware/`, con el `#414` y los blobs de `IO`/`Math` dentro). Los seis en
verde. Lo que se ha probado es, bit a bit, lo que se va a publicar.
📌 Y de paso queda **exonerado por la prueba** —no por argumento— el susto del final
de la sesión: el cuelgue no era el `#414` ni los blobs. Con esa misma imagen la placa
corre con normalidad; era el estado persistente que se fichó aparte.

## Puerta 1 — el ABI nuevo, en placa. NO NEGOCIABLE

Lo único que no se puede comprobar en el PC. Una placa por familia, y basta con un
programa pequeño que toque lo que cambió.

- [ ] **Reflashear las tres familias** con la imagen sellada. La stdlib embebida cambió:
      un firmware viejo con `.mod` nuevos es el fallo que se busca.
- [ ] **Borrar `/lib` y `/app` y dejar que el IDE los suba de nuevo**, o regrabar
      `Stdlib.pack`. Un `.mod` de antes del cambio de `Comparable` es exactamente lo que
      tiene que fallar aquí y no en producción.
- [ ] Ejecutar en cada familia un programa que use **`List`, un envoltorio y un captador
      tipado** (vale `ListGets`): tiene que dar **la misma salida que en el PC**.
- [ ] `INFO` y el arranque sin líneas de error nuevas.

Si esto pasa en las tres, el resto del guión es sobre features, no sobre cimientos.

---

## Puerta 2 — lo nuevo de V5, en la placa que lo tiene

| | prueba | Metro | P4 | STM32 |
|---|---|---|---|---|
| **SD** | monta sola al arrancar; escribir/leer/append; ficheros grandes | ✔ | ✔ | — |
| **SQLite** | el pack graba y arranca; `SqlDemo` con la base en `/sd` | ✔ | ✔ | — |
| **ORM** | `DaoDemo` y `GenDemo` en placa: CRUD, `Where`, el apóstrofo | ✔ | ✔ | — |
| **Packs** | grabar, listar, borrar, formatear zona; `Packs.list()` | ✔ | ✔ | ✔ |
| **GUI** | que no se haya roto con el `Core` nuevo | — | ✔ | ✔ (DK2) |

---

## Puerta 3 — las fichas que quedan abiertas

- [ ] **`#379`** — verificar que era `#398` (el refresco del árbol) y no una
      desincronización del wire. Repetir en el P4 con tarjeta: 5 ciclos `run`→`stop`→`Info`.
- [ ] **`#408`** — medir los dos cuellos: el árbol en la P4 y el formateo en la Metro.
      Ahora la medida vale, porque el P4 ya no se compila a `-Og`.
- [ ] **`H2-P5`** — exFAT y el «superfloppy», reformateando una de las dos tarjetas.
- [ ] **cola de `#439`** — repetir en **Metro y STM32** la prueba que cerró la ficha en el
      P4: `CuelgaLog.bp` → `kill` → `reset` → «RAM SUPERVIVIENTE». ⚠️ En la Metro el botón
      físico SÍ conserva la RAM (pin de RUN); en ESP32 no.

---

## Lo que NO se prueba, y por qué

Dicho explícitamente, para que sea una decisión y no un olvido:

- **Pico 2 (a1), ESP32-S3 (b1), P4 Waveshare (b3), Discovery (c2).** Son *variantes*: la
  imagen es la misma que la de su hermana y lo que cambia entre ellas —pines, panel,
  variante— **no lo tocó V5**. Se cubren con un humo corto: arrancar, conectar, `INFO` y
  un `run` de `blink`. Si la Puerta 1 pasa en la familia, aquí no hay nada nuevo que medir.
- **La batería A–J completa de V4.** Midió los cimientos, y los cimientos no se han
  movido. Lo que sí se repite de ella es la Puerta 0, que es barata.
- **La instalación desde cero.** Se hizo en V4 y el procedimiento no ha cambiado; se
  repite sólo si cambia el ZIP.

---

## Decisiones pendientes ANTES de publicar

1. ✅✅ **El `.npk` de ARM: RESUELTO DEL TODO el 21-ago — ya no es «nunca ejecutado».**
   Regenerado desde sus fuentes el 20-ago (464.127 → 464.399 B) y **ejecutado en la
   Metro el 21-ago**: `SqlDemo` dio **25 de 25 líneas idénticas** al host (única
   diferencia, la ruta: `/sd/medidas.db` frente al `medidas.db` del PC, que la imprime
   el propio programa). Antes de grabarlo se verificó que los bytes del `.npk`
   regenerado están **completos dentro de `SQLite.pack`** (ARM en el offset 176,
   RISC-V en el 464.624). Con una sola ejecución quedan probados a la vez: el nativo
   nuevo, SQLite corriendo desde un pack en XIP, la arena de 2 MB que reserva el ENV,
   y la escritura en la tarjeta SD.
2. 🟡 **El manual inglés se dejó siete secciones de V4** (eventos, sobrecarga,
   `dospasadas`). No es regresión de V5, pero conviene no publicar creyendo que está
   completo.
3. ✅ **Los samples que no compilaban: RESUELTO el 21-ago, y eran más de cuatro.**
   El censo de los **277 `.bp` que viajan en el ZIP** (la batería sólo cubre ~70) dio
   18 fallos. Triados: **6 rotos de verdad**, todos por el mismo motivo —`List.get()`
   devuelve `Object` desde `#389` y el downcast ahora hay que pedirlo—, arreglados y
   verificados con paridad dual-VM (`PacksDemo`, `AnyNumGc`, `n5_popblocking`,
   `ownerlistremove`, `stdtest`, `synclisttest`); **8 eran artefacto del censo** (los
   del grupo BD, que se construyen con su `.bpbuild`); y quedan **2 por decidir**, que
   NO son erratas (ver punto 4).
4. ✅ **`List` ya no admite arrays ni tuplas: DECIDIDO por Eduardo el 21-ago.**
   *«No lista tuplas, pues vale. El que quiera pares de valores o se crea una clase
   con 2 propiedades o utiliza Map.»* Se queda como está y se documenta.
   - `TupleFirstClass` reescrito: la sección de colecciones ahora enseña **la clase
     `Par`** en vez de meter la tupla, y sale lo mismo (`hi42`). Paridad dual-VM OK.
   - `MemInfo` **sí se arregló**, porque su caso era otro: la `List` no guardaba
     «pares de valores» sino bloques de `newByteArray` para que el GC no se los
     llevara mientras mide. Con una clase envoltorio de una propiedad mide igual
     (244 KB en trozos de 4 KB, verificado).
   - Documentado en `PENDIENTES.md` → «Una tupla no entra en una colección», con
     las dos alternativas escritas y la nota de que a los arrays les pasa igual.
5. ✅ **La instrucción de ACTUALIZAR: CONTESTADA el 22-ago — tres familias, tres
   comportamientos, y NINGUNO cubre las dos carpetas salvo el RP2350.**
   ⚠️ **Corregido el mismo día**: la primera versión de este punto decía que en STM32 *«no
   hay que hacer nada porque se autocura»*. **Falso, y lo desmintió la placa media hora
   después**: el Nucleo dio `exit 11` con `/lib` recién reembebido. La causa era un
   `Core.mod` rancio en **`/app`**, que el autocurado no toca — Eduardo lo encontró con
   `dir /app` y lo borró.

   🔑 **La pieza que faltaba: hay DOS carpetas y el problema puede estar en cualquiera.**
   El IDE sube las dependencias del programa a **`/app`**, y lo que hay ahí **tapa** a lo
   de `/lib`. Limpiar sólo `/lib` no basta.

   | familia | `/lib` | `/app` | ¿avisa? |
   |---|---|---|---|
   | **RP2350** (Pico, Metro) | instala si falta **o difiere** | ✅ **también lo revisa** | ✅ sí, de las dos (`pico/main.c:1323`) |
   | **STM32** (Nucleo, DK2) | ✅ lo **vacía y reembebe** cada boot | ❌ **no lo toca** | ❌ no |
   | **ESP32** (S3, P4) | instala **sólo si falta** | ❌ no lo toca | ❌ no |

   📌 **El RP2350 es la ÚNICA familia con la red completa**, y por eso allí el problema se
   ve en el arranque en vez de en un `exit 11` sin pistas. En STM32 y ESP32 hay que
   saberlo de antemano.
   ⏭️ **Para las notas de la release**, la instrucción honesta es una sola frase que vale
   para todas: **«al actualizar, borra `/lib` Y `/app` de la placa; el firmware repondrá
   lo suyo»**. Es más bruta de lo necesario en RP2350 y STM32, pero es correcta en las
   tres y no obliga al usuario a saber en qué familia está.
   🔗 **Y este incidente es un caso concreto de una ficha de V6 que ya existía**: *«NO
   copiar dependencias que el dispositivo YA TIENE — y que lo diga él»*. Si el IDE no
   hubiera puesto un `Core.mod` en `/app`, no habría habido nada que envejecer. La ficha
   deja de ser una optimización y pasa a ser también un arreglo de robustez.
   🔬 De paso quedó explicado el crecimiento del FS del Nucleo (294.912 → 344.064 →
   409.600 → 417.792 B, a saltos de 8 KB = bloque de littlefs): es el precio de vaciar y
   reembeber 14 módulos en cada arranque. **Se estabiliza** al cuarto, así que es desgaste
   asumido —el propio código lo dice: *«mismo desgaste que el snapshot viejo»*— y no fuga.

6. 🔴 **`appv1lsp.bp` y `appv2.bp` viajan en el ZIP y NO compilan** (encontrado el
   21-ago, fichado en `FICHAS.md` → «la sustitución por LSP entre interfaces de módulo
   NO funciona»). **No es regresión de V5**: es el subsistema de interfaces de módulo,
   y apunta al mismo sitio que el bug ya aparcado de la pasada `INTERFACE_ONLY`.
   **Tres salidas, hay que elegir una antes de publicar**: (a) arreglarlo, (b) sacar los
   dos samples de la distribución, (c) moverlos a `samples/errores/` con una nota de que
   hoy no funciona. Publicar dos ejemplos rotos no es una de ellas.
