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


## ⚠️ IMÁGENES PENDIENTES DE REHACER (21-ago) — antes de probar en esas placas

El arreglo de `#414` (los builtins de packs estaban dentro de `#ifdef BPVM_GUI`,
commit `494d7bba`) cambia `src/builtins.c`, que es **común a las cinco imágenes**.

| imagen | estado |
|---|---|
| **Pico / Metro** (`bpvm_pico.uf2`) | ✅ **REHECHA y resellada** el 21-ago (602.112 B) |
| **ESP32-S3** (`bpvm_esp32_merged.bin`) | ❌ **pendiente** — comprobado con `nm`: 0 referencias a `bpvm_pack_iter` |
| **STM32 Nucleo** (`bpvm_stm32_nucleo.bin`) | ❌ **pendiente** — sin GUI, se presume igual |
| **ESP32-P4** (`bpvm_esp32p4_merged.bin`) | ✅ sana (tiene GUI; verificado en su objeto) |
| **STM32 DK2** (`bpvm_stm32_dk2.bin`) | ✅ se presume sana (tiene GUI) |

📌 **Criterio de Eduardo (21-ago)**: se rehace **sólo la Pico** ahora, y las demás
**antes de probar en su placa**. *«Así si aparecen más errores, nos ahorramos
reconstruir las imágenes cada vez.»* Las tres sanas/pendientes se rehacen en una
sola tanda al final, con lo que haya salido para entonces.

⚠️ Mientras estén pendientes, **el S3 y el Nucleo de `dist/firmware/` NO valen para
probar packs**: darían el fallo ya diagnosticado, no uno nuevo.

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

1. ✅ **El `.npk` de ARM, RESUELTO el 20-ago.** Eduardo: *«se trata de verificar,
   tiene que funcionar»*. Regenerado desde sus fuentes (464.127 → 464.399 B), así que el
   binario ya se corresponde con el código. **Lo que queda no es decidir, es probar**:
   ese binario no se ha ejecutado nunca, y es lo que hace obligatoria la Metro en la
   Puerta 2.
2. 🟡 **El manual inglés se dejó siete secciones de V4** (eventos, sobrecarga,
   `dospasadas`). No es regresión de V5, pero conviene no publicar creyendo que está
   completo.
3. 🟡 **Los cuatro samples que no compilan** en la batería (`MemInfo`, `synclisttest`,
   `TupleFirstClass` y uno más). Vienen de antes; hay que mirar si son fósiles o bugs.
