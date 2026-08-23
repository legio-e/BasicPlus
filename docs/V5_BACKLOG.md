# BasicPlus — Backlog de V5

> 📦 **ARCHIVO — V5 se publicó el 22-ago-2026.** Este documento ya no dirige trabajo:
> queda como registro de cómo se planificó el hito. Guardado en `docs/` el 23-ago al
> abrir V6; hasta entonces vivía fuera del repositorio. Lo que quedó vivo al cerrar V5
> está en `FICHAS.md`, y lo aplazado, en `V6_BACKLOG.md`.
>
> **Fue la fuente única del backlog de V5.** V5 se trabaja en **modo exploración**: sin
> obligaciones de alcance, y un experimento que sale «no» es un buen resultado.
> Se relaja la ceremonia, **no el método** — acotar antes de tocar, chivatos que
> digan dónde falla, y una placa a fondo antes que tres a medias.
>
> Estado: `pendiente` / `en curso` / `cerrado`. Convención: B=bug · L=limitación ·
> N=hallazgo · M=mejora.

---

## H1 — Lector de SD ✅ CERRADO EN PLACA (7-ago-2026)

La Metro RP2350B identifica la tarjeta y entrega bloques. Detalle en la memoria
`v5-h1-sd-lector`.

---

## H2 — La SD como sistema de ficheros 🏁 **CERRADO** (8-ago-2026)

FatFs R0.16 vendorizado, montaje sobre el lector de H1, y el ciclo completo
probado en la Metro: listar, leer, **crear y escribir** un fichero que el PC
lee después. La respuesta conocida fue `autorun.inf` + `System Volume
Information` (nombre largo de 25 caracteres) contrastados contra Windows.

Coste medido: **12,7 KB de flash y 1,7 KB de RAM** en la imagen de la Pico.
Detalle y trampas en la memoria `v5-h2-fat-sd`.

### Lo que queda pendiente de H2 (NO bloquea; se cierra igual)

Eduardo cierra H2 el 8-ago con esto anotado. El criterio: lo que queda no
impide trabajar con la tarjeta ni con una base de datos encima — es cobertura y
pulido, y arrastrar el hito abierto por eso sólo emborrona qué está hecho.

Ordenado por lo que bloquea, no por lo que se ve.

#### H2-P1 — `write_at` y `truncate` · ✅ **HECHO** (8-ago, `make test-fspos` 17/17)

La fachada tiene `read_at(camino, desplazamiento, …)` pero su gemela no existe.
`write(camino, datos, longitud, append)` sólo sabe **reescribir el fichero
entero** o **añadir al final**.

Una base de datos reescribe la página N en medio de un fichero de diez megas.
Hoy la única forma sería leerlo entero, modificarlo y volver a escribirlo
entero — sobre una SD eso no es lento, es inviable.

Toca: `bpvm_fs_backend_t` (dos ops nuevas), la fachada, y las dos cinturas
(`fs_fat.c` con `f_lseek`+`f_write`+`f_truncate`, `fs_lfs.c` con
`lfs_file_seek`+`lfs_file_write`+`lfs_file_truncate`). Backend que no las
implemente → NULL → la fachada devuelve -1, como el resto de ops opcionales.

⚠️ `f_truncate` de FatFs exige `FF_FS_MINIMIZE 0` — comprobar antes.

#### H2-P2 — Ficheros en la tabla BIOS · ✅ **HECHO** (8-ago, BIOS v2, 26 ranuras)

Lo que se le presta hoy al pack nativo es memoria, cadenas, `malloc` y
`localtime`. **Cero ficheros.** Y SQLite corre como pack nativo: hoy no tiene
por dónde abrir un fichero, aunque el FS esté montado y funcionando debajo.

DECIDIDO: la **forma** es por descriptor (`abrir/leer/escribir/truncar/
`sincronizar`/`tamaño`/`cerrar` + `borrar`/`existe`), porque la forma de la API
es lo único que un pack ya grabado NO puede cambiar. El **fondo**, de momento,
es la fachada por camino: cada lectura vuelve a recorrer el directorio.

⚠️ **Deuda elegida, no descuido**: sobre una SD por SPI ese recorrido se nota, y
más cuanto mayor sea el árbol. Cuando moleste, el descriptor pasa a guardar el
fichero abierto de verdad **y el pack no se entera** — que es justo lo que
compra haber acertado la forma primero. `sincronizar` hoy devuelve 0 y ya: cada
operación cierra, y el cierre es quien vuelca.

#### H2-P3 — El árbol del IDE se corta con muchos ficheros · `pendiente`

El recorrido plano que alimenta el árbol tiene tope de **16 directorios** y
**96 entradas por directorio**. Con una tarjeta con contenido real trunca. Avisa
al log, pero **el árbol no se lo dice al usuario**: sólo enseña menos cosas, que
es la peor forma de fallar.

Observado por Eduardo el 8-ago mirando el árbol con la SD montada.

Arreglo bueno: **árbol perezoso** — pedir los hijos al expandir, con `LIST_DIR`,
que ya existe y ya reporta lo que trunca. Salida barata si estorba antes: que el
recorrido plano **no baje** a los volúmenes montados (el árbol enseñaría `sd/`
como carpeta y para dentro se iría con `dir`). Tres líneas, a cambio de perder
la vista anidada.

No bloquea trabajar con una BD: para eso basta `dir /sd`, que va por directorio
y sí avisa por pantalla cuando trunca.

#### H2-P4 — Seis operaciones del backend NUNCA se han ejecutado · `pendiente`

Han corrido en placa: `list`, `stat`, `isdir`, `read`, `write` y `read_at` (ésta
de rebote, por el barrido de CRC del listado). **No han corrido nunca**:
`remove`, `rename`, `mkdir`, `rmdir`, `mtime_ms` y el `write` en modo *append*.
Están escritas y enlazadas, nada más — y un camino compilado no es un camino
probado.

#### H2-P5 — Sólo una tarjeta, y sólo la Metro · `pendiente`

Probado con UNA SanDisk de 128 GB, SDHC/SDXC, FAT32 con MBR y partición en el
bloque 2048. Sin probar: tarjetas pequeñas (SDSC, que direccionan por BYTE y no
por bloque — camino distinto en `bpvm_sd_leer_bloque`), sin MBR
(«superfloppy»), y exFAT (que se rechaza a propósito, con mensaje).

`bpvm_sd.c`, `fs_fat.c` y el cambio de la fachada son portables de nacimiento
(cero `#ifdef` de familia), pero **sólo están dados de alta en el build de la
Pico**. Las demás familias, en bloque y cuando la cadena esté probada.

#### H2-P6 — Montaje automático y en CALIENTE · ✅ **CERRADO EN PLACA** (8-ago)

Hasta el 8-ago había que teclear `sd mount` después de cada reset, y una app de
autoarranque encontraría `/sd` inexistente. Estaba así a propósito —no tocar
hardware que puede no estar hasta tener la cadena probada— y ese motivo ya
caducó.

Criterio de Eduardo (8-ago): **el pin de detección manda**. Si no hay tarjeta
metida no tiene sentido montar, y así el arranque no toca ni el SPI.

Verificado en la Metro: arranque con y sin tarjeta, y **meterla y sacarla en
caliente** — se monta y se desmonta sola en medio segundo. Desde el IDE basta
con [Refrescar]; no hace falta montar ni desmontar a mano.

##### 🐛 Y por el camino salió un defecto que llevaba desde H1

La primera versión sólo montaba al arranque, y Eduardo probó justo el caso que
la rompía: arrancar SIN tarjeta y meterla después. Al mirarlo aparecieron DOS
cosas, y la primera no era la que se buscaba:

**`bpvm_sd_hay_tarjeta` leía el pin pero no lo configuraba** — eso lo hacía
`bpvm_sd_init`. Parecía correcto porque desde init se llama justo después de
configurarlo, pero se llama de otros dos sitios que ocurren ANTES de que init
exista (el arranque y `disk_status`), y ahí se leía un pad virgen. Los pads del
RP2350 arrancan en pull-DOWN → lee 0 → «hay tarjeta». **El detector decía
siempre que sí**, y el guardián del arranque no guardaba nada.

Es la MISMA trampa que la MISO de H1. La lección general, y por eso el arreglo
va donde va: **configurar y leer un pin no pueden vivir en sitios distintos**.
Quien lee lo deja usable, y entonces da igual quién llame y en qué orden.


---

## H3 — SQLite en la placa · ⚠️ **SECCIÓN PERDIDA** (11-ago)

> 🔴 **Aviso honesto**: el cuerpo de esta sección —unas 80 líneas escritas
> entre el 8 y el 11 de agosto— **se perdió** al truncarse este fichero por un
> error mío (un script lo abrió en modo escritura y falló antes de escribir).
> Lo demás se recuperó: H1/H2 y la cola de V4 salen de git (`62d4b3b^`), y H6
> se reescribió entero.
>
> **No bloquea nada**: H3, H4 y H5 están CERRADOS, así que lo perdido era
> registro, no plan. Lo vivo de aquella sección está en las memorias
> `v5-packs-sqlite-sd` y `v5-h4-sqlite-libreria`, y el diseño completo en
> `V5_IDEAS.md`.

Lo único que sobrevivió del final de la sección, dos preguntas que quedaban
abiertas:

- Los ≈ 7 KB de estáticos de SQLite **salen del bloque de la BD** (para no tener
  una segunda dirección que sellar); Eduardo recordaba que iban en memoria
  convencional. Decide qué reserva el firmware.
- **¿`SQLite.mod` en el mismo pack que SQLite?** El diseño del 4-ago dice que sí,
  pero otra línea dice que el IDE compila sus `native` en cada Run. Las dos no
  pueden ser ciertas, y de eso depende para qué sirve el gate de ABI.

### Lo que decía al abrirse (recuperado de git)

**Aún no empezado.** Instrucción de Eduardo al cerrar H2 (8-ago):

> *"Empezamos H3, pero paso a paso, que si hay algún problema lo detectemos
> claramente. No tenemos prisa, de hecho vamos mucho más rápido de lo que tenía
> pensado. Así que lo vamos asegurando todo."*

O sea: peldaños cortos con su verificación cada uno, no tandas grandes. Ir por
delante del plan **no** es motivo para acelerar; es margen para asegurar.

---

## 📌 ANTES DE H3 — cerrar la cola de V4

Pendiente de la sesión del 8-ago, para hacer **a la vuelta de la pausa**:

**Subir a GitHub lo que queda de V4.** El arreglo de la cintura de flash de la
Pico —`ee26e35`, *"la cintura de flash de la Pico ignoraba la ALINEACIÓN de la
dirección"*— está en el historial LOCAL pero no en el remoto, y va mezclado
entre commits de V5.

Contexto de por qué importa: ese bug hacía **imposible grabar packs** en las
placas RP2350 y estaba **en la V4 ya publicada**. El ZIP de GitHub ya se
actualizó a mano con la imagen corregida (cubre al usuario que se baja el ZIP y
prueba con el IDE), pero **la release 4.0.1 no está hecha**.

Estado al cerrar la sesión: **15 commits locales sin subir**, de `d0ae180` a
`d4c44a4`. Sólo `ee26e35` es de V4; el resto es V5 (packs nativos, H1 y H2).

---

## H6 — La SD del P4 (SDMMC) · `en curso` (abierto 11-ago-2026)

Hito propuesto por Eduardo al cerrar H5: **el lector de SD del P4, a nivel de
hardware, driver nuevo**. El resto del camino es migrar lo que ya funciona en la
Metro.

Investigación previa (pines, alimentación, 4 bits): `V5_IDEAS.md`, sección «la
SD a 4 bits no se hace en el RP2350; se hará en el P4».

### El hallazgo que da forma al hito: **el invariante es el BLOQUE, no «el driver de SD»**

Los dos caminos **no se juntan donde parecía**:

    Metro (SPI)   FatFs → disk_read → bpvm_sd.c  ← EL PROTOCOLO ES NUESTRO
                                       └→ bpvm_spi_* / bpvm_gpio_* → cintura → SDK

    P4  (SDMMC)   FatFs → disk_read → ???        ← EL PROTOCOLO ES DEL SDK
                                       └→ sdmmc_host (ESP-IDF)

En SPI, `CMD0/8/58/9/10/17` los mandamos nosotros; por eso `src/bpvm_sd.c` no
tiene **un solo `#ifdef` de familia** y vive en la base común. En SDMMC eso lo
hace `sdmmc_card_init()` del IDF: ahí **no hay sitio** para `bpvm_sd.c`.

⇒ **La frontera no es «familia», es QUIÉN HABLA EL PROTOCOLO.** Y lo único común
a los dos caminos está una capa más abajo de FatFs: **el dispositivo de bloque**
—dame N sectores, toma N sectores, cuántos hay, ¿hay tarjeta?—. Eso es lo que
tiene que declarar la mitad común; lo de arriba (FatFs, la fachada, los verbos)
se reutiliza **entero**.

Ojo a la consecuencia para el reparto de V6: si se parte por familia,
`bpvm_sd.c` acabaría empujado a la mitad de hardware, y el día que aparezca un
STM32 con el zócalo cableado a SPI habría que duplicarlo o importarlo desde el
proyecto de otra familia. Es protocolo, no familia.

### Paso 1 — el contrato de bloque · ✅ **CERRADO Y VERIFICADO EN PLACA** (11-ago, Metro)

`66344a8`. Sale un `bpvm_blk_backend_t` (leer / escribir N bloques / hay medio /
cuántos / el PELDAÑO del último fallo), `fs_fat` pasa a depender de él y deja de
saber qué es un pin, y `bpvm_sd.c` lo implementa sin que su protocolo se toque.

Lo que estaba soldado, y no sólo por dentro: `bpvm_fs_fat_montar` llevaba
`bpvm_sd_pines_t` **en su firma pública**.

Dos cosas que la separación destapó:

- `disk_read/write` pasan el `count` ENTERO al dispositivo en vez de trocearlo.
  Con SPI da lo mismo; en SDMMC la lectura múltiple (CMD18/CMD25) es donde está
  la ganancia, y trocear aquí la habría matado sin que se notara.
- La tabla de particiones sale del montaje a `bpvm_blk_lba0_de_mbr`, **pura**.
  Era el sitio con casos raros —un sector de arranque de FAT lleva la misma
  firma `0x55AA` que un MBR— donde equivocarse **no revienta**: devuelve un
  número, y un número malo manda a reformatear una tarjeta que estaba bien.
  Hasta hoy no se podía ejercer sin placa Y sin tarjeta. `make test-blk` 18/18.

**La verificación salió mejor de lo pedido.** En vez del ciclo de ficheros,
Eduardo corrió **`SqlDemoSd` — SQLite abriendo y escribiendo `/sd/medidas.db`**:
montaje, lectura multibloque (el árbol lista `System Volume Information`, nombre
largo) y escritura de páginas, por el consumidor más duro que tenemos. `exit 0`.
Y después las dos que faltaban:

- **La vigilancia del zócalo**: sacar → `/sd` desaparece del árbol; meter →
  vuelve con su contenido. Las cuatro ramas tocadas, ejercidas.
- **Los bytes están en la tarjeta, y lo dice un TERCERO**: `medidas.db` abierto
  en el PC con DB Browser for SQLite — las 6 filas y el esquema
  `medidas(id INTEGER, sensor TEXT, valor REAL, t INTEGER)`. No es nuestro
  código leyendo lo que escribió nuestro código: es un lector independiente que
  no tiene ningún motivo para ser amable. El mejor oráculo de la tanda, y gratis.

### Paso 2 — la cintura SDMMC del P4 · ✅ **MONTA EN PLACA** (11-ago)

    sd: SDIO slot 0, 4 bit(s), clk 43 cmd 44 d0 39 | pwr 45 (activo bajo) | ldo 4
    sd: montada en /sd (4 bits, particion en el bloque 2048)

Segunda familia leyendo una tarjeta, por un camino que no se parece al de la
Metro — y **encima de la frontera no se tocó NADA**. Era la apuesta del paso 1.
`bpvm_blk_lba0_de_mbr` se estrena además en hardware distinto (bloque 2048).

**Las dos causas, y las dos de análisis, no de código:**

1. **El LDO de las E/S no se encendía.** `pwr` (VDD de la TARJETA, MOSFET fuera
   del micro) y `ldo` (los pads del MICRO, dentro del chip) son dos raíles
   distintos y hacen falta los dos. Se descartó el segundo leyendo la ayuda de
   un Kconfig («check if the SD **VDD**…») en vez del API («…to power the
   SDMMC **IO**»).
2. **La polaridad de `pwr` es activo BAJO**, como decía el análisis del
   transistor. Una medida de 3,3 V en el zócalo pareció decir lo contrario y
   llevó a invertirla — lo que **enmascaró el arreglo del LDO durante dos
   rondas**: la tarjeta corría sin alimentar mientras se miraba el bus.

**⚠️ La lección, que no es «mide»**: aquella medida tenía DOS explicaciones (raíl
controlado con polaridad invertida, o raíl siempre encendido) y no distinguía
entre ellas. Una medida que dos hipótesis explican igual **no desempata nada**, y
tratarla como si desempatara es peor que no tenerla. Lo que desatascó no fue otra
medida: fue la TABLA de combinaciones probadas — cuatro casillas llenas y una
vacía, que resultó ser la buena. Ver [[medida-ambigua-no-desempata]].

**Y el instrumento que faltaba**: el driver ahora DICE su configuración antes de
intentar montar. Esa línea contesta de un vistazo «¿es la imagen nueva?» y
«¿llegó el env?», dos preguntas que se resolvieron tres veces a mano con
`strings` y `nm` sobre el ELF.

### Detalles de la cintura · `esp32p4/main/blk_sdmmc_p4.c`

Nuestro FatFs arriba (NO `esp_vfs_fat_sdmmc_mount`: serían dos FatFs con dos
configuraciones divergiendo en silencio) y debajo una cintura que llame a
`sdmmc_host` del IDF.

### Paso 3 — `LIST_DIR` al sitio común · ✅ **HECHO Y VERIFICADO** (11-ago)

`6f03027`. Núcleo en `src/bpvm_listdir.c`; cada familia sólo pone **por dónde
sale el texto** (el Pico a `stdout`, el ESP32 troceando con `wire_v1_send_bulk`)
y el cierre de línea. Las dos primitivas ya existían: la costura no hubo que
inventarla, sólo verla.

- **Metro** (regresión, la que ya lo tenía): sigue igual. La extracción no rompió
  al consumidor que funcionaba.
- **P4** (capacidad nueva): `dir /sd` contesta **instantáneo** con las 7 entradas.
- **Host**: `make test-listdir` 16/16 — antes esto sólo se podía ejercer con placa.

⚠️ **Y un error mío que conviene no repetir**: dije que esto arreglaría el árbol
del IDE. Era necesario pero **no suficiente** — el árbol NUNCA ha usado
`LIST_DIR`: `onRefresh()` llama al `LIST` plano (CRC por fichero) y monta el
árbol partiendo paths. `LIST_DIR` sólo estaba enganchado al `dir` de la consola.
Diagnosticado leyendo el IDE, no adivinando. → tarea #398 (árbol perezoso).

### Los otros verbos (`SD_INFO`, `SD_MOUNT`) · `pendiente`

Siguen sólo en `pico/repl_v1.c`. No corrían prisa: el P4 monta al arrancar por el
ENV, y `SD_INFO` no es una copia — saca CID/CSD del driver SPI y en SDMMC esos
datos vienen del `sdmmc_card_t`, así que necesita la abstracción de «qué dice la
tarjeta».

### Nota original del paso 3

`SD_INFO`, `SD_MOUNT` y `LIST_DIR` viven hoy sólo en `pico/repl_v1.c`; el P4 usa
`esp32/main/repl_esp32.c`. Suben a código compartido — primer ladrillo del
reparto de V6, estrenado con dos consumidores reales a la vez, que es lo que
casi nunca tiene una extracción. **Va con el paso 2, no antes**: hacerlo ahora
con un solo consumidor tira justo ese beneficio.

⚠️ `SD_INFO` no es una copia: hoy saca CID/CSD del driver SPI y en SDMMC esos
datos vienen del `sdmmc_card_t` del IDF. Necesita una abstracción de «qué dice
la tarjeta».

### ✅ EL ESQUEMÁTICO, LEÍDO (11-ago) — y una suposición que era FALSA

`docs/ESP32-P4-WIFI6-Touch-LCD-4.3-schematic.pdf`.

- **El C6 no comparte NADA.** Zócalo SD = GPIO **39–44**; ESP32-C6 (WiFi6, SDIO)
  = GPIO **14–19** + su UART. Disjuntos, slots distintos. Era el riesgo caro y
  está descartado. Y los seis del zócalo coinciden con los del ejemplo
  `05_sdmmc`: dos fuentes independientes.
- **Pull-ups: los hay** — R5–R10, **51 K** (el bloque del C6 usa el mismo valor).
  Existen, que era la pregunta; pero 51 K es **flojo** para reloj alto ⇒ empezar
  conservador y subir MIDIENDO.
- **La alimentación NO es el LDO interno.** Eduardo, leyendo el esquema: el VDD
  del zócalo sale de **3V3 conmutado por Q1 (AO3401, MOSFET de canal P), y la
  puerta de Q1 la manda GPIO45**.

**La suposición que se cae**: el `Kconfig` del ejemplo decía *LDO interno,
canal 4*, y de ahí salió el aviso de «encender el LDO antes de montar». Ese
default es del kit de Espressif, **no de esta placa**. El ejemplo empaquetado por
el fabricante no es el esquema.

**Lo que cambia en la cintura**, y a mejor:

- Nada de `esp_ldo_acquire_channel`: es **un GPIO**. Menos API y menos IDF.
- ⚠️ **La polaridad se confirma, no se deduce.** Un AO3401 conduce con la puerta
  BAJA, pero si lleva adaptación de nivel se invierte. Y esta placa **ya mordió
  una vez por lo mismo**: el backlight invertido (#296). Si se equivoca, el
  síntoma es «la tarjeta no contesta» y se depuraría el driver con el raíl apagado.
- 🎁 **Regalo**: cortar la alimentación es la única forma FIABLE de resetear una
  SD colgada a media transacción. En la Metro no se puede; aquí sí. Vale como
  recuperación y, sobre todo, para FORZAR el caso en la suite.
- El `sd=` del ENV del P4 necesita una etiqueta más: **`pwr:45`**.

### 🔴 CONFIRMADO: el P4 NO TIENE detección de tarjeta

Eduardo, leyendo el esquema: *«~CD se pone a nivel alto por una resistencia de
pull-up y nada más»*. La patilla está en el conector y **no va a ningún GPIO**.
Los seis del bus más el 45 de alimentación explican todo lo que hay.

**Consecuencia 1 — el zócalo no avisa.** El meter/sacar en caliente que la Metro
hace (y que se acaba de verificar) **no es posible aquí**. El contrato ya lo
cubre sin cambios: sin detector, `hay_medio()` contesta 1, porque decir «no hay»
sería mentir. Diferencia de CAPACIDAD entre placas → material de #378.

**Consecuencia 2 — y ésta NO es obvia: `bpvm_fs_fat_vigilar` DEGENERA.** Quien
rearma el reintento es la rama `!hay` (`s_intentada = 0` cuando el zócalo se
queda vacío). Si `hay_medio()` contesta 1 para siempre, esa rama **no se ejecuta
nunca** ⇒ la vigilancia da **UN intento por arranque**. Si la tarjeta no estaba
puesta al encender, meterla después no hace nada, y el síntoma sería «el P4 no
monta la SD» sin ninguna pista.

⇒ **En el P4 no se usa `vigilar`**: montaje explícito (`sd mount`, o al arranque
si el ENV lo dice). No hay que tocar la función — hay que no llamarla.

### ✅ DECIDIDO: la detección de tarjeta se hace LEYENDO, y A DEMANDA (11-ago)

Idea de Eduardo, y el fundamento es de uso, no de simetría: *«en el mundo real no
estás todo el día metiendo y sacando la SD. Lo normal es no tocarla nunca, o
sacarla únicamente para exportar/importar ficheros»*. Con eso, sondear la
tarjeta **cuando alguien pregunta** basta, y desaparecen el temporizador, el
tráfico de fondo y el estado que mantener.

Se implementa con el paso 2. Aplica a las DOS placas.

#### La señal es el CÓDIGO DE RETORNO, no el contenido

«Si no hay nada, no hay SD» tiene un falso negativo dentro: una tarjeta recién
formateada o virgen puede tener el sector 0 a ceros y es perfectamente
legítima. Juzgando por los bytes, esa tarjeta se declararía ausente.

Lo que distingue es que **la transacción complete**. En SDMMC es sólido (CRC en
CMD y en DAT); en SPI también, pero con la reserva de [[bus-sin-pullup-miente]]
— apoyarse en el token de datos, nunca en el contenido.

#### Y sale gratis algo que el PIN de la Metro NO da

Al montar ya se lee el sector 0 (de ahí sale `s_lba0`). Guardando una firma
corta de ese sector, la comprobación contesta **tres** cosas en vez de dos:

    la lectura falla        -> no hay tarjeta
    lee y la firma coincide -> la misma de siempre
    lee y la firma CAMBIÓ   -> han metido OTRA tarjeta

El tercer caso es el que corrompe de verdad, y **el pin no lo ve**: si se cambia
una tarjeta por otra entre dos vigilancias, el detector sólo dice «hay algo».
O sea que esto no es un sustituto pobre del detector: es mejor. Por eso va
también en la Metro, que sí tiene pin.

#### Dónde se engancha: según QUIÉN pregunta

- **Lo pide una PERSONA** (refrescar el árbol, `sd mount`, `LIST_DIR`): se
  comprueba SIEMPRE. Pasa pocas veces por minuto; el coste no existe.
- **Lo pide un PROGRAMA** (`open`, lectura, escritura): **no** se comprueba por
  delante — un bucle que abre cien ficheros pagaría cien lecturas de sector en
  un camino caliente. Se **reacciona al fallo**: si la operación falla, entonces
  se comprueba y, si no hay tarjeta, se desmonta. Coste cero en el caso bueno, y
  el estado final es el mismo que da el pin: `/sd` existe y falla limpio en vez
  de escribir al vacío.

Resumen: **comprobación puntual donde la pide una persona, reacción al fallo
donde la pide un programa.**

#### En el contrato: operación APARTE, y opcional

`hay_medio()` es barato —en la Metro, leer un pin— y esto no lo es (una
transacción de bus, cientos de microsegundos). Va como op propia y opcional
(NULL = este dispositivo no puede), **no metida dentro de `hay_medio()`**: si una
función barata se vuelve cara en secreto, alguien acabará llamándola en un bucle
y nadie sabrá por qué la placa va lenta.

### Lo que queda por mirar

**La polaridad de Q1**: con qué nivel de GPIO45 se enciende el raíl.

### Criterio de aceptación

**«Monta y se ven los ficheros» NO vale.** Un bus marginal no falla en el
`mount`: falla a ratos, a reloj alto o con la placa caliente. Debajo de SQLite
eso no da error, **corrompe la base en silencio**.

La prueba que distingue *funciona* de *parece que funciona* es barata: **escribir
unos MB de patrón conocido, releerlos y comparar byte a byte, al reloj
objetivo**. Una sola diferencia = mirar el hierro antes de seguir construyendo.

---

## 🏁 H6 CERRADO (11-ago-2026)

Los tres pasos hechos y verificados **en placa**:

1. **Contrato de bloque** — FatFs deja de saber lo que es un pin. Verificado en
   la Metro por su consumidor más duro (SQLite escribiendo en la tarjeta) y con
   los bytes confirmados desde el PC por un lector ajeno.
2. **Cintura SDMMC del P4** — monta a 4 bits. Segunda familia leyendo una
   tarjeta, por un camino que no comparte protocolo con el primero, y **sin
   tocar nada por encima de la frontera**.
3. **`LIST_DIR` común** — en las tres familias, con test de host.

**Lo que queda del hito, y NO bloquea el cierre** (son de otro sitio):

- El **criterio de aceptación del bus**: los MB de patrón conocido leídos y
  comparados byte a byte al reloj objetivo. Montar y listar NO prueba que el bus
  sea sano, y con pull-ups de 51 K ése es el punto flojo conocido de la placa.
  Sigue pendiente y hay que hacerlo antes de fiarse de la tarjeta del P4.
- El **árbol del IDE** (#398), que es trabajo de Java.
- La polaridad de `pwr`: **cerrada** — activo bajo, medido.

---

## H7 — SQLite en el P4 (abierto 11-ago) · tarea #399

Lo que Eduardo preguntó al abrirlo — *«no sé si están las llamadas a la BIOS»*—
comprobado:

**Ya se compila en el P4** (el núcleo portable): `bpvm_bios.c`, `bpvm_npack.c`,
`bpvm_pack.c`, `bpvm_sqlmem.c`, `bpvm_mdn_scan.c`.

**Falta el adaptador de familia**, que hoy sólo existe para la Pico:

    pico/bios_pico.c   -> LLENA las ranuras de la tabla     -> falta bios_p4.c
    pico/pack_pico.c   -> encuentra/valida/carga el pack    -> falta pack_p4.c

O sea: **la tabla BIOS existe y está vacía** en el P4, y nadie carga el pack. Es
[[portar-familia-adaptador-completo]] otra vez — encaminar el verbo no es
implementarlo.

**Un regalo de una línea**: `src/bpvm_bios_fs.c` (las ranuras de FICHERO, las que
SQLite necesita para su VFS) es **portable** y sólo está dado de alta en el build
del Pico. Alta en los dos del ESP32 y hecho.

### ✅ La pregunta que decidía la forma: CONTESTADA — **un solo modelo**

¿Puede el P4 ejecutar desde su zona de packs, como la Pico por XIP? **Sí.**

- `esp_partition_mmap(..., ESP_PARTITION_MMAP_INST)` no es un camino especial del
  ESP32 original: acaba en `esp_mmu_map()` con
  `caps = MMU_MEM_CAP_EXEC | MMU_MEM_CAP_32BIT` — la API genérica de la MMU
  (`components/spi_flash/flash_mmap.c:93`). El *"only for esp32"* del header se
  refiere a la cifra de 11 MB, no a la capacidad.
- Y el P4 lo tiene de serie: `soc_caps.h` dice
  **`SOC_MMU_DI_VADDR_SHARED = 1`** — *"D/I vaddr are shared"*. La región
  mapeada se alcanza como datos **y** como instrucciones.

⇒ **La divergencia que yo describí por la mañana era mía, no del silicio.** El
`.mdn` va a RAM en el P4 porque es pequeño y se regenera en cada Run — una
decisión POR ARTEFACTO que yo estaba extendiendo a un pack de 410 KB por inercia
de familia. Ver [[hal-bp-capa-comun]], sección de la carga de la prueba.

⚠️ **El detalle que hay que tener en cuenta al escribir `pack_p4.c`**: el mapeo
ejecutable es `MMU_MEM_CAP_32BIT` — sólo admite accesos alineados a 4 bytes. Leer
la cabecera del pack byte a byte por ESE puntero daría basura. Se resuelve
leyendo/validando por `esp_partition_read` (o un segundo mapeo de datos) y
usando el mapeo INST **sólo para ejecutar**. Es exactamente la clase de fallo que
no revienta: devuelve bytes.

**Y no cuesta RAM**: es un mapeo, no una copia. Los 410 KB se quedan en flash.

### 📦 UN pack con las DOS arquitecturas — decidido 11-ago (idea de Eduardo)

**El plan**: hoy la versión RISC-V; mañana los dos juegos en un solo pack, ya con
la BD y la BIOS probadas.

**Por qué encaja** (y por qué mi primera respuesta estaba mal): yo dije que el
IDE no transforma el pack al grabar. Es al revés, y lo dice el código —
`bpvm_npack.h:68`: *«La tabla de relocalizaciones NO viaja: la consume el IDE al
grabar»*, y el peldaño 5 de la escalera rechaza lo que llegue sin realojar
(`bpvm_npack.c:48`). Eduardo lo vio antes que yo: *«si tiene que recolocar el
código, eso seguro, si no no funcionaría»*.

Con eso:

- **El artefacto distribuible y el grabado YA son cosas distintas.** Lo que se
  distribuye es portable (con su tabla); lo grabado es de ESA placa y ESA
  dirección, y el sello lo garantiza. Un pack gordo es esa misma idea con dos
  juegos *(imagen + tabla)* en vez de uno.
- **Descartar el otro NO es una optimización de flash: es lo único posible.** La
  cabecera grabada tiene UN sello, UNA `entry_off` y UN `reloc_count = 0`. No
  caben dos imágenes. (Mi «opción A» —grabar el gordo entero y que la placa
  elija— era irrealizable: la placa no tiene recolocador.)
- **La llave de selección ya existe**: `arch` + `float_abi`, del mismo catálogo
  `MDN_ARCH_*` que la escalera comprueba en los peldaños 2 y 3. El IDE elige con
  la MISMA llave con la que la placa rechazaría el equivocado ⇒ un catálogo, dos
  usuarios, imposible que divergan. ⚠️ `arch` sola no basta: el peldaño 3 avisa
  de que la ABI de coma flotante no da error, **da números mal**.

**Dónde cae el coste**: nombrar los dos juegos en el contenedor (lo único nuevo,
y es pequeño) · elegir por `(arch, float_abi)` antes de recolocar (una búsqueda;
el recolocador ya existe) · tipos de reloc RISC-V, que **hacen falta igual**.

⏰ **Se decide ANTES de escribir el grabado en el IDE**, no después: hoy eso no
existe (`notas/v5-sqlite-prueba/I/pack.py` es un script a mano). Escribirlo
sabiendo que el pack trae dos juegos no cuesta nada; retrofitarlo es cambiar el
formato.

### ✅ 11-ago tarde — EL PACK RISC-V EXISTE Y EL ORÁCULO DICE IDÉNTICO

`sqlite_rv.npack`, 536.776 B. **5.087 relocalizaciones aplicadas y byte-idéntico
a lo que produce `ld` enlazando en la dirección destino** (529.796 B de flash +
6.916 de datos). El mismo oráculo de la versión ARM, que sigue verde.

**El corte por arquitectura, MEDIDO** (censo del ELF enlazado, no supuesto):

| | una dirección viaja… | a parchear |
|---|---|---|
| ARM | ENTERA en el *literal pool* (`R_ARM_ABS32`) | 2.794 |
| RISC-V | **PARTIDA** entre dos instrucciones (`HI20` + `LO12_I/_S`) | 5.087 |

Lo que **no** cambia es el criterio —clasificar por el valor del símbolo y sumar
el delta que toque—, así que hay **UN realojador con una tabla de arquitecturas**,
no dos scripts. Copiarlo habría copiado el oráculo, y entonces la validación del
nuevo no valdría nada: es el aviso que el propio fichero ya llevaba escrito.

**La red del refactor**: el `.npack` ARM del 10-ago sirvió de oráculo del cambio.
Sigue saliendo **byte-idéntico** (md5 `e7a11029…`), o sea que la máquina nueva no
tocó la vieja.

**Cuatro cosas que sólo aparecen en RISC-V** y estaban a mano de pasar mudas:

1. **`.sdata`/`.sbss`** (datos pequeños) no existen en ARM. Sin meterlos en la
   extracción se perderían **en silencio**: 564 B de imagen y 68 de bss.
2. **`gp`**: `__global_pointer$` está definido, pero con `-mno-relax` hay **cero**
   accesos por `gp` — comprobado en el desensamblado. Si los hubiera, el pack
   usaría el `gp` del *firmware* y leería memoria ajena sin dar error.
3. **El carry de `LO12`**: se interpreta con signo ⇒ `HI20 = (dir + 0x800) >> 12`.
   Olvidarlo desvía el puntero 4 KB en ~la mitad de los casos.
4. **`readelf` TRUNCA el nombre del tipo a 16 caracteres**: `R_RISCV_RVC_BRANCH`
   sale como `…RVC_BRANC`. Filtrar por lo que imprime una herramienta es
   justo donde un recorte cosmético se vuelve un fallo de verdad.

**`fabs` en el pack** (`sqlite_pack.c`): en ARM GCC lo mete en línea y el símbolo
nunca llegaba al enlace; con `ilp32f` (FPU de simple precisión) sale una llamada
real. Va en el fichero COMÚN, no en uno por familia: la diferencia está en quién
lo inlinea, no en lo que hace. ⚠️ Eso cambia el `.npack` ARM en unos bytes
muertos ⇒ **hay que reconstruir y re-verificar el ARM al juntarlos**.

**Bonus del refactor**: el «no saltar en silencio» destapó que la versión ARM
ignoraba 10.692 relocs (`THM_CALL`/`THM_JUMP24`) sin decirlo. Estaba BIEN —son
ramas PC-relativas— pero *correcto y mudo* ≠ *correcto y dicho*: lo mudo no
distingue «no hacía falta» de «se me olvidó». Ahora salen nombradas.

### 🔴 LO QUE BLOQUEA GRABARLO EN EL P4 — y no es el realojador

**La dirección del sello no se puede calcular desde el PC.** En la Pico la flash
es XIP en dirección FIJA, así que el PC sabe dónde caerá el pack. En el P4 **la
asigna `esp_mmu_map()` en tiempo de ejecución**, y la tabla de particiones sólo
da el *offset* de flash (`bpdata @ 0x618000`); además el límite FS|Packs vive en
el env, o sea que depende de la config de la placa.

⇒ **La dirección la tiene que DECIR LA PLACA** (por el INFO), mismo principio que
el JEDEC y la arquitectura: la fuente de verdad es quien lo sabe de verdad.
Es una decisión de diseño de `pack_p4.c`, y **no me la invento**.

Mientras tanto `sqlite_rv.npack` se sella con las direcciones de la Metro y
**NO es grabable en el P4**. No es un riesgo mudo —el peldaño 5 diría *"realojado
para OTRA dirección"*— pero el aviso sale ya al construirlo.

### 🔎 LA TABLA BIOS DEL P4 — comprobado, y NO se copia

Verificado en el árbol (no de memoria): **no existe `bios_*.c` ni `pack_*.c` en
`esp32/` ni en `esp32p4/`**. El núcleo portable `bpvm_bios.c` sí se compila en
las tres familias; `bpvm_bios_fs.c` (las ranuras de FICHERO, las que SQLite
necesita para su VFS) está **sólo en el Pico** — `pico/CMakeLists.txt:90`.

Ahora el dato que decide la forma. De las 154 líneas de `pico/bios_pico.c`,
**26 de las 29 ranuras son idénticas para cualquier placa**:

| ranuras | quién las pone | ¿difiere? |
|---|---|---|
| 12 de libc (`memcpy`…`strcspn`) | newlib — que el ESP32 **también** usa | no |
| 9 de fichero | `bpvm_bios_fs_*`, ya portable | no |
| 2 del punto de encuentro | `bpvm_bios_publica` / `_busca` | no |
| 3 de memoria | chivatos que gritan al log | no (hoy) |
| `log` · `localtime` · `arena` | de la placa | **sí, 3** |

⚠️ **Copiar `bios_pico.c` sería crear una SEGUNDA COPIA DEL ORDEN DE LAS
RANURAS**, y ese orden cruza una frontera binaria: si un día las dos copias no
coinciden, el pack llama a `memcpy` y ejecuta otra cosa — sin error de
compilación ni de enlace. Es el mismo error que ya mordió dos veces
([[v4-...#299]] layout de clase, #315 slots de vtable), y el propio fichero lo
avisa en su línea 100: *«escribirlas por familia es exactamente como divergen
las familias»*.

⇒ **Sacar la tabla a un INICIALIZADOR COMÚN en cabecera** que reciba los tres
punteros que sí cambian. Sigue siendo `const` —vive en la imagen, que es lo que
el pack necesita para que se la presten— y el orden queda escrito en UN sitio.
Cuesta lo mismo que copiarla y quita una divergencia antes de que exista.
Red: el Pico está verde, se recompila para comprobar que no se movió.

### ✅ 12-ago — PASO 1: LA TABLA BIOS DEL P4, COMPILANDO EN LAS 3 FAMILIAS

Método de Eduardo para esta fase: *«suponer que no va a funcionar e ir probando
cada cosa por turno»*. El orden lo da la cadena de dependencias, y el control
barato es **`mini`** (254 B): si `mini` corre en el P4, quedan probados de golpe
BIOS + ancla + escalera + MMU + salto, y un fallo YA NO puede ser de SQLite.

Hecho y verificado en el artefacto (no en el fuente):

- **`BPVM_BIOS_TABLA`** en `bpvm_bios.h`, **pegado a la struct**. De las 29
  ranuras sólo 3 dependen de la placa. El Pico pasa a usarlo y se comprobaron
  **las 31 casillas resolviendo cada puntero a su símbolo en el ELF**.
- **`bios_p4.c`** (voz, reloj, arena) · **`pack_p4.c`** = andamio que DICE que no
  está hecho, con la receta del paso 3 dentro.
- **El bloque de la BD muerde PRIMERO** (`SQLite=<MB>` del ENV + regla portable
  `bpvm_sqlite_region`). `PACK_RAM_BYTES` del P4 = **16 KB** (medido: el pack
  RISC-V pide 8.284; los 8192 del Pico lo rechazarían por 92 bytes).
- **ENV temprano** (`board_mgr_esp32_env_temprano`), ADITIVO: el arranque
  escalonado de siempre queda intacto.
- **`bpvm_bios_fs.c` de alta en los DOS ESP32** — faltaba en ambos.

**🛠️ Y EL ESP32 SE COMPILA AQUÍ.** La nota decía que hacía falta Eduardo: era
falso. El IDF está instalado; lo que pasa es que **Git Bash no vale** (`export.sh`
aborta con *"MSys/Mingw is not supported"*) y hay que ir por **PowerShell**,
activando y construyendo en la MISMA orden. **29 s** el P4. Ver
[[compilar-firmware-pico-en-cada-tanda]].

### ✅✅ 12-ago — CÓDIGO NATIVO RISC-V EJECUTÁNDOSE EN EL P4

    pack: valido @0x401590b0, saltando a 0x401590f0 (data 0 B, bss 8 B)
    pack: mini: vivo, y este texto viene de mi .rodata
    pack: volvio, rc=6

**Tercera familia con packs nativos.** Y la frase del medio es la que prueba las
relocalizaciones: la dirección de esa cadena la parcheó `pack.py` de base 0 a
`0x401590F0+`. Los 10 parches —**4 a flash y 6 a RAM**— quedan ejercitados,
porque `g_bios` y `s_veces` viven en el bloque de la BD: las dos clases de delta,
no sólo una.

**QUÉ QUEDA DESCARTADO** para lo que venga después (que es el valor del control):
el ancla · la escalera · el mapeo de la MMU · el salto y la vuelta · la tabla
BIOS · el sello · el realojador HI20/LO12. Un fallo de SQLite ya **no puede ser**
de ninguno de ellos.

**QUÉ NO PRUEBA, dicho para que nadie lo dé por hecho**: `mini` NO pide arena
(`data 0`, `bss 8`), así que `bios_arena` sigue sin estrenar — la estrenará
SQLite. Y son 213 bytes contra 530 KB: la escala tampoco está probada.

#### Lo que hizo falta por el camino (y no estaba previsto)

1. **Los packs nunca se cablearon en la familia ESP32.** `PACK_LS` contestaba
   "sin zona de packs" en el S3 **y** en el P4: faltaban `bm.packs_base/size`,
   `bm.packs_flash` y cuatro campos del `req`. Las DOS mitades de
   [[portar-familia-adaptador-completo]], y no era de H7 — venía de #327, que
   sólo cerró el Pico. Ahora: la familia REGISTRA su vista (el P4 la tiene tras
   mapear), y la cintura de escritura sobre `esp_partition_*` es común.
2. **⚠️ El `confirm` del `PACK_FORMAT` es la cadena `"YES"`, no un booleano.**
   Estuve a punto de leerlo como número: el FORMAT habría rechazado SIEMPRE
   mandando el IDE el confirm correcto, y habría parecido un fallo del IDE.
3. **La alineación del bloque de la BD.** `heap_caps_malloc` da 4 bytes y el pack
   pide 8 (el Pico ya lo hacía con `& ~7u` y al portarlo se me quedó fuera). Se
   pasó a `heap_caps_aligned_alloc(4096, …)`: arregla la alineación **y** hace la
   dirección estable frente a lo que la IDF reserve antes.

#### 🔬 Y una lección de instrumento, que es la que más vale

El primer intento dijo **«compilado para OTRA arquitectura»** — y era **CIERTO**:
se había grabado `mini.pack`, el de ARM. Pero el mensaje no decía CON QUÉ números,
así que no se podía distinguir "el pack está mal" de "la cabecera se lee mal".

El diagnóstico que lo resolvió lee la MISMA cabecera por **dos caminos** —el
mapeo y `esp_partition_read`, que no pasa por la MMU— y los enseña juntos. En una
línea salió `arch 40, abi 'softfp', sello 0x10bc40f0`: el pack de la Metro.

Dos cosas que deja:
- **La escalera nunca se equivocó; estaba MUDA.** [[instrumento-mudo-dudar-de-el]]
  al derecho: lo que faltaba eran los operandos, no la lógica.
- **`[mapeo]` y `[flash]` salieron IDÉNTICOS** ⇒ el mapeo es fiable para leer byte
  a byte. La preocupación del `MMU_MEM_CAP_32BIT` queda archivada **con una
  medida detrás**, no con un "parece que sí".

### 🎯 DECISIÓN (Eduardo, 12-ago): EL CARGADOR DE PACKS, A COMÚN

*«Estas discrepancias entre la Pico y la P4 las podemos volver a encontrar en
otras placas. Esta parte del código no se puede hacer común en un futuro y así
no ocurre "se me olvidó".»*

**El detonante**: al escribir `pack_p4.c` se me olvidó el paso 5 —copiar la
`.data` y poner la `.bss` a cero— y SQLite arrancó con estáticos de basura. Lo
cazó la comprobación que el propio pack lleva dentro (`rc=6`), no el firmware.

**Y hoy es la TERCERA vez con la misma enfermedad**: el orden de la tabla BIOS,
los verbos de packs del ESP32, y ahora el cargador. Siempre
[[portar-familia-adaptador-completo]]: *encaminar el verbo no es implementarlo*.

**El reparto, contado**: de los 6 pasos de `pack_pico.c`, **5 son comunes**.

| paso | | |
|---|---|---|
| 1 | dónde está la zona | **familia**: XIP fijo / `esp_partition_mmap` / … |
| 2 | la BIOS antes de nada | común |
| 3 | ¿hay sitio para `.data`/`.bss`? | común |
| 4 | buscar + escalera | común (ya lo es) |
| 5 | **copiar `.data`, `.bss` a cero** | común ← el olvidado |
| 6 | miga de pan + salto + vuelta | común |

⇒ La familia rellena cinco datos y el común hace el resto:

```c
typedef struct {
    const void*        zona;  uint32_t zona_bytes;   /* ya alcanzable por la CPU */
    uint8_t*           ram;   uint32_t ram_bytes;    /* dónde van sus estáticos */
    const bpvm_bios_t* bios;
} bpvm_npack_sitio_t;
int32_t bpvm_npack_cargar(const bpvm_npack_sitio_t* s);
```

**⭐ Y LO QUE DE VERDAD LO CIERRA: se podrá probar EN EL PC.** Hoy la copia de
`.data` sólo se ejercita con una placa delante, y por eso el olvido sobrevivió a
compilar, enlazar y al mapa de símbolos. En común entra en `test_npack.c` —que ya
existe— con una zona de mentira y un buffer de RAM: comprobar que los bytes caen
donde deben y que la `.bss` queda a cero. `make test-npack`, dos segundos.

Es el mismo movimiento que ya funcionó con `bpvm_npack_check`: está en común y
**la validación no divergió entre familias ni una vez**. Lo que divergió fue
exactamente lo que quedó fuera.

📌 Se hace CUANDO cierre la prueba en placa, no antes: tocarlo ahora sería mover
el suelo mientras se mide encima. Y al hacerlo, `pack_pico.c` se recorta también
⇒ **la Metro hay que volver a probarla** (se suma a los pendientes de abajo).

### ✅✅✅ 12-ago — SQLITE CORRE EN EL P4 (motor arrancado, API publicada)

    pack: valido @0x401590b0 | .data 6916 B copiada a 0x48001000, .bss 1368 B a cero
    pack: sqlite: pack vivo
    pack: 3.53.4
    pack: sqlite: arena 4080 KB en 48005000
    pack: sqlite: initialize OK — motor arrancado y vfs 'bp' registrado
    pack: sqlite: API publicada como 'SQLI' — 17 simbolos, v1
    pack: volvio, rc=0

**529.796 B de C ajeno, realojados en el PC (5.087 relocs), grabados, mapeados y
ejecutados en RISC-V.** Segunda familia con SQLite dentro, y la primera que no es
XIP de dirección fija.

Lo que cada línea prueba, que no es lo mismo que "va":

- **`3.53.4`** — cadena de la `.rodata` de SQLite **atravesando una
  relocalización**. Es la comprobación que el propio pack lleva dentro
  (`if (v[0] != '3') return 6`), y es la que cazó el olvido de la `.data`.
- **`arena 4080 KB en 0x48005000`** — cuadra al byte: `0x48001000 + 16384`. El
  reparto `[estáticos | arena]` y los `PACK_RAM_BYTES` de 16 KB, confirmados.
  `bios_arena` ESTRENADA (`mini` no la tocaba).
- **`initialize OK` + vfs `'bp'`** — pasó la trampa de
  [[sqlite-initialize-exige-vfs]], que falla con `SQLITE_ERROR` **y en silencio**.
- **`API publicada como 'SQLI', 17 símbolos`** — el punto de encuentro (V5/H4)
  funciona en esta familia: el pack dejó su tabla bajo una marca y **la VM no
  sabe nada de SQLite**. Es la propiedad que hace que los packs valgan la pena.

⇒ La mitad NATIVA de H7 está cerrada. Falta la mitad BP: el `.mdn` de RISC-V (para
que BP pueda LLAMAR a esos 17 símbolos), los módulos en el pack, y la consulta.

### 📦 EL PACK COMPLETO — no es «un blob», son DOS PAREJAS + DOS MÓDULOS

Eduardo (12-ago): *«en el pack ahora sólo está el SQLite nativo. Pero cuando esto
funcione habrá que añadir el SQLite.mod y SQLite.mdn y el ORM.»*

Correcto, y afina lo decidido ayer. El reparto, **comprobado en los ficheros**:

| pieza | qué es | ¿por arquitectura? | hoy |
|---|---|---|---|
| `sqlite.npk` | el blob nativo | **sí** | ARM ✅ · RISC-V ✅ |
| `SQLite.mdn` | los thunks AOT que puentean BP→nativo | **sí** | ARM ✅ · RISC-V ❌ |
| `SQLite.mod` | el módulo BP | no (bytecode) | ✅ |
| `Orm.mod` | el ORM | no (bytecode) | ✅ |

Que el `.mdn` es por arquitectura está **medido**, no supuesto: su cabecera lleva
`arch` en el offset 16 (`mdn_format.h`), y el de hoy dice **0x28 = 40 = ARM**.

⇒ **La llave `(arch, float_abi)` del pack gordo tiene que elegir LA PAREJA
ENTERA**, no un blob. Un `.npk` de RISC-V con un `.mdn` de ARM es exactamente el
desastre que el gate de #284 existe para evitar — y encima uno donde las dos
piezas se creen compatibles porque vienen del mismo pack.

**Trabajo que NO estaba contado: el `.mdn` de RISC-V.** Buena noticia — el camino
ya existe en el IDE: `AotBuild.java:63` (`RISCV_P4_FLAGS`), `:74`
(`RISCV_LINK_FLAGS`, con `-mno-relax`) y `target = "riscv"`. No hay que
inventarlo, hay que usarlo con el toolchain que hoy ya está verificado.

### 🔴🔴 EL IDE LLEVABA UN COMPILADOR DE OTRA ÉPOCA — y nada lo decía

12-ago. Eduardo: *«Aquí hay una diferencia entre hacerlo desde el IDE a hacer
desde la línea de comandos. ¿Seguro que estamos con la misma versión de
compilador?»* Sí había diferencia, y era toda la explicación.

**El experimento que lo cerró en un minuto** — mismo proyecto, dos compiladores:

| compilador | resultado |
|---|---|
| `lexer-java/target/classes` (11-ago) | **0 errores** |
| `BpIde-4.0-shaded.jar` (8-ago) | **17 errores de sintaxis** |

Y el primer error lo dice todo:
`[57:23] se esperaba nombre del pack tras 'from pack', encontrado '"SQLI"'`.
El jar no conocía la sintaxis con el nombre del pack entre comillas; al
atragantarse, el parser se descolocó y los 16 `public native function`
cayeron detrás como cascada. `SQLite.bp` se escribió DESPUÉS de ese jar.

**La cadena de rancio era más larga de lo que parecía.** Reconstruir sólo el
IDE no habría bastado: tira de `~/.m2`, y ahí estaba
`bpgenvm-1.0.jar` del **3-ago** y `basicplus-frontend-1.0.0.jar` del **6-ago**.
El orden obligatorio es `miVM` → `lexer-java` → `BpIde`, los tres con `install`
menos el último. Hecho y verificado: el jar nuevo da 0 errores.

**Lo que queda del incidente, y es lo que importa:** un banner de versión en
toda compilación (ver abajo). Emparenta con
[[el-sello-de-build-no-identifica-la-imagen]] — es el mismo error de fondo en
otra capa: no poder saber QUÉ artefacto estás ejecutando.

### ✅ El compilador DICE quién es, en toda compilación (Eduardo, 12-ago)

*«Cuando compila, dé o no errores, que se vea la versión del compilador, si no
es un lío.»* Hecho en `basicplus/frontend/Version.java`, llamado desde las TRES
raíces de build de `Main` (full / sólo-interfaz / sólo-diagnósticos):

```
compilador BP 4.0 | BpIde-4.0-shaded.jar | 2026-08-12 12:19
compilador BP dev | clases sueltas       | 2026-08-12 12:18
```

📌 **La decisión de diseño:** el NÚMERO no basta — los dos artefactos del
incidente dirían «4.0» igual, porque sólo cambia cuando alguien lo sube en el
pom. Lo que identifica al compilador es **de dónde salió y de cuándo es**. Por
eso la línea lleva el artefacto y su fecha; puestas una debajo de otra el
diagnóstico es inmediato.

⚠️ Dos trampas evitadas, anotadas en el código: separador ASCII (`·` sale como
`?` en la consola de Windows, probado), y NO imprimirlo una-vez-por-JVM — el
IDE vive arrancado toda la sesión y entonces sólo saldría en la primera
compilación, faltando justo el día que hace falta.

### 📦 El jar del IDE pasa de 4,4 a 18 MB — sqlite-jdbc

Efecto colateral de reconstruirlo: el fat-jar del IDE incorpora ahora
`sqlite-jdbc 3.46.0.0`, que entró con el ORM (`DaoGen.verificar` abre la BD del
proyecto). Son 25 MB sin comprimir de binarios NATIVOS para FreeBSD, Linux,
Android, Musl, Mac y Windows — de los que en cada máquina se usa uno.

No es un fallo y no urge; es una decisión de EMPAQUETADO para el cierre de V5.
Curiosidad a mirar entonces: en `lexer-java/pom.xml` la dependencia está
marcada `<optional>true</optional>` —o sea, NO transitiva—, así que llega
porque el propio `basicplus-frontend` es un jar shaded que se la traga dentro.
Es el mismo mecanismo de #218 (el frontend empaquetaba una copia de miVM).

### 🐛 Un módulo de stdlib compilado SUELTO miente sobre por qué falla

Abrir `bpstdlib/SQLite.bp` a solas → **18 errores** `no se puede llamar a
'RuntimeError'`. En modo proyecto: **0 errores**. La causa real es una sola —
suelto no resuelve `Core`, y `RuntimeError` vive ahí (#248)— pero ni uno de los
18 mensajes la nombra.

Y lo peor de la forma: **16 de los 18 señalan líneas `public native function
...`**, donde no se llama a nada. El compilador se sintetiza una llamada por
cada `native` (el camino de «native no disponible»), así que el usuario mira 16
declaraciones perfectas buscando un error que no está ahí. Es el mismo patrón
que el hallazgo 40: el diagnóstico apunta al sitio equivocado.

Arreglo: cuando `Core` no resuelve, decirlo UNA vez y en su sitio —
`no encuentro 'Core' (¿falta el proyecto?)`— en vez de N errores derivados.
Control ya hecho: `Json.bp` suelto da 15 del mismo error, así que **no es de
SQLite ni del `import ... from pack`**, es de cualquier módulo de la stdlib.

Lo pagó Eduardo en tiempo real (12-ago) siguiendo una indicación mía errónea
(«ábrelo suelto»). Emparenta con [[instrumento-mudo-dudar-de-el]]: aquí el
instrumento no está mudo, habla de más y de otra cosa.

### 🔧 H8 EN MARCHA — el relocalizador ya coincide con `ld` (RISC-V)

12-ago, paso 1 de #401. Tres piezas nuevas en el frontend:

| pieza | qué es | verificación |
|---|---|---|
| `Elf32.java` | el lector de ELF32, sacado de `MdnPack` para compartirlo | `.mdn` **byte a byte** + guardián **idéntico** antes/después |
| `NpackReloc.java` | el motor de re-basado + la tabla de destinos | ✅ RISC-V contra `ld` |
| `NpackRelocOraculo.java` | el arnés | 10 relocs, flash 213 B **IDENTICO** |

**El oráculo es el enlazador.** Se le da el mismo código enlazado en la
dirección destino y se comparan bytes. Ni una expectativa escrita a mano: si
`ld` y nosotros discrepamos, los que nos equivocamos somos nosotros.

📌 **Lo mejor fue un fallo del guardián.** Puse una comprobación de que las
secciones fueran contiguas y saltó al primer disparo: *«hueco de 2 B antes de
.rodata»*. `.text` acababa en 0x72 y `.rodata` empieza en 0x74 — el enlazador
ALINEA. Yo estaba **concatenando** cuando la imagen es un **rango de
direcciones con sus huecos rellenos** (lo que hace `objcopy -O binary`, y por
eso el prototipo Python no se enteró nunca). Sin ese guardián el oráculo habría
dicho «difieren 180 bytes» y habría buscado el fallo en los parcheadores, que
estaban bien. Emparenta con [[una-medida-ambigua-no-desempata]].

📌 **Los números de tipo se MIDIERON**, no se copiaron: salieron de `readelf -r`
sobre nuestros propios binarios. Y el puerto mejora al prototipo — aquél
filtraba por el NOMBRE que imprime `readelf`, que trunca a 16 caracteres y le
obligó a aceptar `R_RISCV_RVC_BRANC` además de `R_RISCV_RVC_BRANCH`. Con el
número esa clase de fallo no existe.

⏳ **ARM pendiente (#402), a propósito.** Criterio de Eduardo: se escribe para
las dos familias pero se verifica de una en una. Y no es trámite — ARM es REL
(el addend va DENTRO de la palabra) y ésa es **otra rama** de `relocalizar()`,
hoy con cero pasadas. Orden acordado: cerrar H8 en RISC-V → construir el Pack →
verificar SQLite entero en RISC-V en placa → volver y verificar ARM.

### ⏸️ EL PACK DE SQLITE QUEDA SUSPENDIDO — se hace la herramienta primero

12-ago. Al ir a meter `SQLite.mod` + `SQLite.mdn` dentro del pack del motor
salió que **la posición del `.npk` dentro del pack es cargante** (`pack.py` da
por supuesto un pack de UNA entrada: datos en +176; con tres serían +272 y el
sello no cuadraría). Eduardo lo vio venir antes de que lo midiéramos:

> *«Hagamos las cosas bien. No podemos ir haciendo packs forzando las cosas.
> Construimos las herramientas, las verificamos que funcionen bien, y entonces
> el pack sale solo.»*

Se descartó el apaño (pasarle a `pack.py` el nº de entradas) porque habría que
rehacerlo igual en cuanto entre el pack de dos arquitecturas — que es el
siguiente paso comprometido.

**Elegida la opción B: el IDE relocaliza AL GRABAR.** El diseño completo, con
las decisiones y el orden de construcción, está en **`docs/V5_IDEAS.md`**, y el
trabajo es **#401 = V5/H8**, hito propio.

🔑 **H8 NO bloquea a H7.** El pack combinado es una mejora de empaquetado, no un
requisito para que SQLite corra en el P4: la ruta de ficheros sueltos —grabar
`sqlite_rv.pack` con el motor solo y subir `SQLite.mod` + `SQLite.mdn` + los
`.mod` del ejemplo al FS— es la que usó la Metro y funciona. Va en hito aparte
porque (a) no es de SQLite: sirve a cualquier pack nativo futuro, (b) se
verifica distinto —H7 en placa, H8 en el PC contra `ld`—, y (c) metida dentro,
una herramienta de calibre «enlazador» quedaría escondida en un hito que se
llama «SQLite en el P4», y H7 no podría cerrarse hasta escribirla.

📌 El hallazgo que fija el presupuesto: **no existe ningún relocalizador en el
producto**. `bpvm_npack.h:68` describe el modelo previsto, pero el lado del IDE
no está escrito; el único del repo es `notas/v5-sqlite-prueba/I/pack.py`. B es
portarlo a Java — con su oráculo contra `ld` como test, que hoy no tiene red.

Estado de las piezas del P4 mientras tanto (todas hechas y verificadas):
`sqlite_rv.pack` (motor RISC-V, corre en placa) · `SQLite.mod` 8.325 B ·
`SQLite.mdn` 4.058 B **arch 243**, 16 símbolos.

### 💡 EL PACK ÚNICO, PARTE BP — dos mejoras (Eduardo, 12-ago)

Para que UN pack sirva a ARM y a RISC-V no basta con llevar los dos blobs: hace
falta que las dos parejas convivan también del lado BP. Idea de Eduardo:

1. **Que el proyecto pueda declarar MÁS DE UN target.** Hoy `aot` es
   `{enabled, target}` con UNA cadena, así que el `.bpbuild` queda atado a una
   familia — y este mismo proyecto tiene que dar el `.mdn` de la Metro mañana.
2. **Que la carga del módulo busque en VARIOS `.mdn`** y coja el suyo. Nombres
   propuestos: `miModulo.mdn.arm` / `miModulo.mdn.risc`.

⚠️ Ojo al nombrar: el target del build se compara con `"riscv"` (con uve). Si el
sufijo del fichero fuera `.risc`, serían DOS cadenas para la misma idea — la
clase de detalle que se copia mal la segunda vez. Mejor que la tabla de
arquitecturas (`MDN_ARCH_*` → nombre) sea la única fuente, como ya pasa con
`(arch, float_abi)` en el pack nativo.

Encaja con lo del pack gordo: la llave `(arch, float_abi)` elige **la pareja**
(blob + mdn), y esto es esa misma elección en el lado del módulo.

### 🔴 ENCONTRADO AL INTENTARLO: el proyecto NO tenía la entrada `aot`

`SqlDemo.bpbuild` no la traía, y `aotEnabled` es **false por defecto**:

```java
if (haveProject && !currentProject.aotEnabled) return;   // apagado a propósito
```

⇒ Compilar ese proyecto **no generaba ningún `.mdn`**, y el `return` es MUDO
porque para el IDE está apagado adrede. Lo vio Eduardo (*«creo que añadimos una
entrada donde se podía indicar la familia y el proyecto no tiene la entrada»*)
antes de gastar el intento. Añadida: `"aot": { "enabled": true, "target": "riscv" }`.

📌 Y el comentario de `BpBuild.java:66` se quedó viejo: *«Futuro (V4): "esp32"
(Xtensa / RISC-V)»*. Ese futuro es hoy — repasarlo al cerrar H7.

### ⏳ PENDIENTES DE VERIFICAR EN PLACA (12-ago) — se aparcan a propósito

Decisión de Eduardo: *«seguimos con la P4; la Metro y la S3 lo probamos más
adelante»*. Las dos están **compiladas y sin correr**, y cada una por un motivo
distinto:

| placa | qué cambió | riesgo |
|---|---|---|
| **Metro (RP2350)** | `bios_pico.c` usa ahora la macro común · `sqlite_pack.c` gana `fabs` | tabla verificada casilla a casilla en el ELF; el `.npack` ARM **cambia** por `fabs` ⇒ hay que **reconstruirlo y re-verificarlo** antes de grabar |
| **S3 (Xtensa)** | `bpvm_bios_fs.c` de alta en su build (nuevo) · macro común | sólo enlaza más código; no se usa hasta que el S3 tenga su `bios_s3.c` |

⚠️ Ninguna de las dos se da por buena hasta correrla. La Metro tiene además su
propia batería (`SqlDemoSd`), que es la que decide.

### 🐛 ARRASTRADO: el timeout de INFO en el P4 (observado mejor el 12-ago)

    [Explorer] la placa ejecuta nativo riscv (arch=243)
    [FrmBoard] ENV_SET SQLite=4
    [Explorer] la placa ejecuta nativo riscv (arch=243)
    [Placa ERROR] timeout esperando respuesta a 'INFO' (id=4)

**NO es de H7**: Eduardo confirma que viene de antes (quizá de cuando se hizo la
comunicación con la SD, quizá de más atrás). Detalles nuevos que afinan lo que
ya había apuntado como #379:

- Es concretamente **INFO** el que se queda sin respuesta.
- **El árbol de ficheros acaba saliendo igual**, o sea que el wire NO se queda
  roto: se pierde una respuesta y se recupera.
- Pasa con el wire ya funcionando (las dos líneas `arch=243` de antes salieron).

⚠️ Que se recupere es justo lo que lo hace fácil de convivir y difícil de cazar.
Cuando toque: mirar si INFO tarda más que su timeout (es el comando que más datos
reúne) o si es que su respuesta se pierde. Son dos cosas distintas y la medida
que las separa es cronometrar la respuesta en el firmware, no en el IDE.

### ⏸️ APARCADO (12-ago, decisión de Eduardo): la media flash del P4

El log del arranque lo dice solo, y no es nuevo:

    flash: configurada 16384 KB | chip fisico 32768 KB  <<< EL BOOTLOADER USA
                                                        MENOS FLASH DE LA QUE HAY

La placa lleva **32 MB** y el bootloader está configurado para **16**, así que
la mitad no existe para el SDK. Hay `partitions_32m.csv` preparado
(`bpdata` de ~26 MB) — es cambiar `CONFIG_ESPTOOLPY_FLASHSIZE` +
`PARTITION_TABLE_CUSTOM_FILENAME` y **reflashear el bootloader** (un reflasheo
sólo de la app NO lo actualiza: es la trampa de #328).

No estorba a H7: SQLite necesita ~530 KB de zona de packs y hay de sobra.
Se hace cuando la BD esté cerrada. Emparentado con [[#374]].

### 📋 PARA MAÑANA, POR ORDEN

1. **Que el P4 diga su dirección de packs** (por el INFO). Bloquea el sello, y
   sin ella no se puede grabar nada. Es lo primero.
2. **Inicializador común de la tabla BIOS** + alta de `bpvm_bios_fs.c` en los
   dos builds ESP32 (`esp32/main/CMakeLists.txt` y `esp32p4/main/CMakeLists.txt`).
   Recompilar el Pico como control.
3. **`pack_p4.c`** — buscar/validar/cargar. ⚠️ El mapeo INST es
   `MMU_MEM_CAP_32BIT`: la cabecera se lee por `esp_partition_read`, y el mapeo
   ejecutable **sólo para ejecutar** (ver más arriba).
4. **Sellar con la dirección de verdad, grabar y probar en la placa.**
5. **Reconstruir y re-verificar el pack ARM** (cambió por `fabs`).
6. **Los dos juegos en un solo pack.**
