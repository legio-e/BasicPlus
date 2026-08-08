# BasicPlus — Backlog de V5

> **Fuente única del backlog de V5.** V5 se trabaja en **modo exploración**: sin
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

## H2 — La SD como sistema de ficheros ✅ FUNCIONA EN PLACA (8-ago-2026)

FatFs R0.16 vendorizado, montaje sobre el lector de H1, y el ciclo completo
probado en la Metro: listar, leer, **crear y escribir** un fichero que el PC
lee después. La respuesta conocida fue `autorun.inf` + `System Volume
Information` (nombre largo de 25 caracteres) contrastados contra Windows.

Coste medido: **12,7 KB de flash y 1,7 KB de RAM** en la imagen de la Pico.
Detalle y trampas en la memoria `v5-h2-fat-sd`.

### Lo que queda pendiente de H2

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
