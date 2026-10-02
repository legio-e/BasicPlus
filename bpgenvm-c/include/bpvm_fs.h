/*
 * bpvm_fs.h — fachada de file I/O para la VM C (H10 / #247).
 *
 * Los builtins readFile/writeFile/appendFile/fileExists (src/builtins.c)
 * delegan en el backend registrado por la plataforma:
 *   - host (test/main.c): backend libc (bpvm_fs_register_host) → FS real.
 *   - device (Pico/STM32/ESP32): backend sobre el FS del firmware
 *     (fs_get/fs_put), registrado en el main de cada placa.
 * Sin backend, las operaciones fallan limpio (-1/0) y el builtin lanza un
 * RuntimeError BP atrapable — mismo comportamiento que un builtin no soportado.
 *
 * Mismo patrón que bpvm_gpio.h: tabla de punteros + set_backend + funciones
 * efectivas con fallback.
 */
#ifndef BPVM_FS_H
#define BPVM_FS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── #456 — EL ÚNICO TOPE DE PATH QUE QUEDA ──────────────────────────────────
 *
 * Y sólo vale para los paths que hay que GUARDAR o CONSTRUIR: la sesión de
 * PUT por trozos (que sobrevive entre mensajes, así que no puede apuntar a la
 * línea), el path que devuelve el resolvedor, y el basedir que se salva y se
 * restaura. Un path que sólo se LEE del mensaje no se copia y por tanto no
 * tiene tope: ver `json_str_inplace` en json_min.h.
 *
 * De dónde sale el número, porque antes no salía de ninguna parte (había un 40,
 * un 64, un 96, un 192, un 256 y un 512 para la misma cosa):
 *   · littlefs  LFS_NAME_MAX = 255  → por NOMBRE, sin tope de path completo
 *   · FatFs     FF_MAX_LFN   = 255  → idem
 * o sea que 256 es «un nombre largo del FS más sus carpetas», y es el mismo
 * número que ya usaba el basedir del proyecto (`fs_facade.c`). */
#define BPVM_FS_PATH_MAX  256

/* V7/H3 — el prefijo de la tarjeta es RESERVADO: sin un montaje en él, una ruta
 * `/sd/...` falla limpio en vez de resolver al backend raíz (la flash interna). */
#define BPVM_FS_PREFIJO_SD "/sd"

/* V6 (#453) — trozo de fichero entregado por `read_stream`. Devolver != 0
 * aborta la lectura (p.ej. el wire se corto y ya no hay a quien mandarlo). */
typedef int (*bpvm_fs_chunk_cb)(const uint8_t* data, uint32_t len, void* user);

typedef struct {
    /* 0 + *size si el fichero existe; -1 si no. */
    int  (*stat)(const char* path, uint32_t* size);
    /* lee hasta `cap` bytes en `dst`; devuelve nº de bytes leídos, o -1. */
    long (*read)(const char* path, uint8_t* dst, uint32_t cap);
    /* escribe `len` bytes; append!=0 → al final (crea si no existe). 0 / -1. */
    int  (*write)(const char* path, const uint8_t* data, uint32_t len, int append);
    /* #240 — ops opcionales (NULL → el builtin lanza "no soportado" limpio).
     * Campos AL FINAL: los backends existentes usan designated initializers
     * y los dejan a NULL sin tocarse. */
    /* borra el fichero. 0 / -1. */
    int  (*remove)(const char* path);
    /* renombra/mueve; si `to` existe lo SOBREESCRIBE (semántica de la VM-Java:
     * REPLACE_EXISTING). 0 / -1. */
    int  (*rename)(const char* from, const char* to);
    /* #240 (2ª pasada) — resto de IO.bp; opcionales como remove/rename. */
    /* crea el directorio (e intermedios); ok si ya existe. 0 / -1. */
    int  (*mkdir)(const char* path);
    /* borra el directorio SOLO si está vacío. 0 / -1. */
    int  (*rmdir)(const char* path);
    /* copia from → to sobreescribiendo. 0 / -1. */
    int  (*copy)(const char* from, const char* to);
    /* 1 si path es un directorio existente; 0 en otro caso. */
    int  (*isdir)(const char* path);
    /* mtime en ms epoch (puede truncarse a i32 arriba); -1 si error. */
    long long (*mtime_ms)(const char* path);
    /* H2·B1.3 — listado de directorio: invoca cb por cada entrada (sin '.'
     * ni '..'; name SIN el prefijo del dir). Lo necesitan el LS+CRC del wire
     * y el explorer del IDE. Campo AL FINAL (backends viejos → NULL). 0 / -1. */
    int  (*list)(const char* path,
                 void (*cb)(const char* name, int is_dir, uint32_t size, void* user),
                 void* user);
    /* #305 — lee `cap` bytes DESDE `off`. Es la primitiva que permite tratar un
     * fichero por trozos en vez de entero: sin ella, calcular el CRC de un
     * fichero o mandarlo por el wire obliga a un buffer del tamaño del fichero
     * más grande (en el Pico eran 128 KB de SRAM permanentes). Devuelve los
     * bytes leídos (0 al final) o -1. Campo AL FINAL: los backends que no la
     * implementen la dejan a NULL y el llamante cae al camino de siempre. */
    long (*read_at)(const char* path, uint32_t off, uint8_t* dst, uint32_t cap);
    /*
     * V5/H2 — escritura POSICIONAL y recorte. Las gemelas que le faltaban a
     * read_at, y sin ellas no hay base de datos: una BD reescribe la página N
     * en medio de un fichero de diez megas, y con sólo `write` habría que
     * leerlo entero, modificarlo y volver a escribirlo entero. Sobre una SD eso
     * no es lento: es inviable.
     *
     * write_at: bytes escritos, o -1. El fichero se CREA si no existe (una BD
     * crea el suyo y escribe la página 0), y no se trunca al abrirlo.
     *
     * ⚠️ Escribir MÁS ALLÁ del final extiende el fichero, y **el contenido del
     * hueco queda INDEFINIDO**. No es pereza: FatFs deja ahí lo que hubiera en
     * los clústeres y littlefs rellena con ceros. Prometer ceros sería mentir
     * en una de las dos, y rellenarlos a mano puede ser escribir megas.
     *
     * truncate: fija el tamaño exacto. Encoger libera; agrandar extiende con el
     * mismo hueco indefinido de arriba. 0 / -1.
     */
    long (*write_at)(const char* path, uint32_t off, const uint8_t* data, uint32_t len);
    int  (*truncate)(const char* path, uint32_t size);
    /*
     * V5 (#398) — CRC32 DEL FICHERO ENTERO, hecho por el backend.
     *
     * Existe por una medida, no por elegancia. La fachada sabe calcular el CRC
     * con `read_at` en trozos de 256 B, y eso parecía razonable hasta que se
     * cronometró el refresco del árbol en la P4 (15-ago):
     *
     *     ls: 27 ent en 6953 ms | crc 6903 ms de 1486 KB | ... sd:8/5314ms
     *
     * El 99 % del listado era el CRC. Y el coste NO era leer: `read_at` recibe
     * el PATH, así que cada trozo de 256 B abre el fichero, hace `lseek` desde
     * el principio, lee y cierra. 1358 KB de tarjeta = 5432 aperturas, con un
     * seek que crece con el offset — cuadrático con el tamaño del fichero.
     * Medido: 80 KB/s en el flash interno, 255 KB/s en la SD.
     *
     * Con el fichero abierto UNA vez y leído en secuencia, eso desaparece. El
     * backend es el único que puede hacerlo: la fachada no tiene descriptores.
     *
     * ⚠️ EL VALOR NO PUEDE CAMBIAR: tiene que ser el mismo CRC-32 que
     * `java.util.zip.CRC32`, porque el IDE compara el suyo con éste para
     * saltarse subidas. Un CRC "casi igual" haría que dejara de subir ficheros
     * que sí cambiaron, en silencio. Por eso el camino nuevo se verifica contra
     * el viejo en `test/test_fscrc.c`, no sólo contra sí mismo.
     *
     * Devuelve 0 y escribe *crc, o -1. Campo AL FINAL: el backend que no lo
     * implemente lo deja a NULL y la fachada usa el bucle de `read_at`.
     */
    int  (*crc32)(const char* path, uint32_t* crc);
    /*
     * V6 (#453) — LEER EL FICHERO ENTERO POR TROZOS, con UNA sola apertura.
     *
     * La MISMA enfermedad que arriba y el MISMO remedio, aplicados al segundo
     * llamador. Cuando se arregló el CRC (#398/#424) se añadió `crc32` porque
     * era el caso que se había cronometrado — y el otro bucle de `read_at`, el
     * del `GET` del wire, se quedó como estaba.
     *
     * Medido el 30-ago en un S3, abriendo ficheros del propio proyecto:
     *
     *     AOT_ABI8_IDEAS.md    9.178 B ->  36 trozos -> abre
     *     ESTADO.md          120.763 B -> 472 trozos -> TIMEOUT (10 s del IDE)
     *
     * Mismo código y misma placa: sólo cambia el tamaño. Con el seek creciendo
     * con el offset, 472 aperturas no caben en el timeout.
     *
     * `cb` recibe cada trozo en orden; devolver != 0 aborta la lectura (el
     * llamador ya no puede seguir, p.ej. el wire se cortó). Devuelve los bytes
     * entregados, o -1.
     *
     * Campo AL FINAL y opcional, igual que `crc32`: el backend que no lo traiga
     * lo deja a NULL y la fachada cae al bucle de `read_at` de siempre.
     */
    long (*read_stream)(const char* path, bpvm_fs_chunk_cb cb, void* user);

    /* ── V7/H3 (#491) — EL CURSOR: un fichero ABIERTO ───────────────────────
     * Las ops de arriba reciben RUTA, y por eso la misma enfermedad volvió tres
     * veces (#398, #453, #473): abrir y cerrar por cada trozo. Esto es el
     * descriptor que faltaba, con la condición de la casa: NADIE reserva
     * memoria aquí. El estado del motor por fichero abierto (lfs_file_t + su
     * caché, FIL, FILE*) vive en un bloque que pone el LLAMADOR —en la VM, el
     * byte[] del objeto File (V7_IDEAS §H3)— y que el motor recibe como `st`.
     * Sin tabla, sin tope: el límite es la memoria de quien abre. Precedente:
     * el File de MicroPython y el de Arduino-LittleFS guardan lfs_file_t y la
     * caché dentro del objeto fichero. `file_state_size` dice cuántos bytes
     * necesita el motor por fichero; a 0 (motor sin estas ops) la fachada sirve
     * la LECTURA con `read_at` a offsets y no escribe. Todos AL FINAL y
     * opcionales, como `crc32` y `read_stream`. */
    uint32_t file_state_size;
    int  (*fopen) (void* st, const char* path, int mode);       /* BPVM_FS_O_*; 0 / -1 */
    int  (*fclose)(void* st);                                   /* 0 / -1 */
    long (*fread) (void* st, uint8_t* dst, uint32_t n);         /* leídos; 0 = fin; -1 */
    long (*fwrite)(void* st, const uint8_t* src, uint32_t n);   /* escritos; -1 */
    int  (*fseek) (void* st, uint32_t pos);                     /* absoluto; 0 / -1 */
    long (*fsize) (void* st);                                   /* tamaño ACTUAL; -1 */
    int  (*fsync) (void* st);                                   /* 0 / -1 */
} bpvm_fs_backend_t;

/* Registra el backend RAÍZ (una vez al boot). Limpia cualquier montaje
 * adicional (ver bpvm_fs_mount). NULL → sin FS (todo falla limpio). */
void bpvm_fs_set_backend(const bpvm_fs_backend_t* backend);

/* H2·B1.3 — MONTAJES (andamiaje multi-motor para la fase B, sin estrecharlo
 * a "solo flash"): monta `backend` bajo `prefix` (p.ej. "/sd" → FatFs en V5).
 * Las ops rutan por prefijo más largo; el backend raíz atiende el resto.
 * rename/copy ENTRE montajes distintos → -1 (fase B decidirá). 0 / -1. */
int  bpvm_fs_mount(const char* prefix, const bpvm_fs_backend_t* backend);

/*
 * ¿Atiende este camino el backend RAÍZ (1), o un volumen montado encima (0)?
 *
 * No es curiosidad: hay trabajo que sólo tiene sentido sobre el FS propio del
 * micro. El caso que lo pidió es el CRC del listado (V5/H2) — existe para que
 * el IDE se salte los PUT comparando contenido, y al volumen montado no se le
 * sube nada, así que calcularlo allí es leerse la tarjeta entera por SPI para
 * tirar el resultado. Con una SD de 119 GB eso deja de ser un detalle.
 *
 * Preguntar aquí y no adivinar por el nombre del camino: los prefijos los
 * conoce la tabla de montajes y nadie más, y una comparación con "/sd" a mano
 * caduca en cuanto alguien monte otra cosa.
 */
int  bpvm_fs_en_raiz(const char* path);

/* V5/H2 — escritura posicional y recorte (ver el contrato en el backend). El
 * backend que no las traiga devuelve -1 en vez de fingir. */
long bpvm_fs_write_at(const char* path, uint32_t off,
                      const uint8_t* data, uint32_t len);
int  bpvm_fs_truncate(const char* path, uint32_t size);

/* Funciones efectivas (sin backend → fallo limpio). */
int  bpvm_fs_stat  (const char* path, uint32_t* size);
long bpvm_fs_read  (const char* path, uint8_t* dst, uint32_t cap);
int  bpvm_fs_write (const char* path, const uint8_t* data, uint32_t len, int append);
int  bpvm_fs_exists(const char* path);   /* 1 / 0 */
int  bpvm_fs_remove(const char* path);                    /* #240: 0 / -1 */
int  bpvm_fs_rename(const char* from, const char* to);    /* #240: 0 / -1 */
int  bpvm_fs_mkdir (const char* path);                    /* #240 2ª: 0 / -1 */
int  bpvm_fs_rmdir (const char* path);                    /* #240 2ª: 0 / -1 */
int  bpvm_fs_copy  (const char* from, const char* to);    /* #240 2ª: 0 / -1 */
int  bpvm_fs_isdir (const char* path);                    /* #240 2ª: 1 / 0 */
long long bpvm_fs_mtime_ms(const char* path);             /* #240 2ª: ms / -1 */
/* #461 — cuenta los ficheros del volumen, recursivo, SIN guardar sus nombres.
 * Sustituye los tres `fs_list` de familia, que desde `U3.23` solo contaban. */
int  bpvm_fs_count_files(void);

/* H2·B1.3 — lista el directorio `path` (cb por entrada). 0 / -1. */
int  bpvm_fs_list(const char* path,
                  void (*cb)(const char* name, int is_dir, uint32_t size, void* user),
                  void* user);
/* #305 — lectura parcial (ver read_at). Bytes leídos, 0 = fin, -1 = error o
 * backend sin soporte. */
long bpvm_fs_read_at(const char* path, uint32_t off, uint8_t* dst, uint32_t cap);
/* #305 — CRC-32 de un fichero SIN cargarlo: lee por trozos con un buffer de
 * pila. Es lo que el LS del wire necesita de cada fichero, y la razón por la
 * que había un scratch enorme. 0 y *crc_out puesto; -1 si no se pudo leer. */
int  bpvm_fs_crc32(const char* path, uint32_t* crc_out);
/* #453 — lee el fichero entero por trozos. Usa el `read_stream` del backend si
 * lo hay (UNA apertura); si no, cae al bucle de `read_at` de siempre. */
long bpvm_fs_read_stream(const char* path, bpvm_fs_chunk_cb cb, void* user);

/* ── H19-F1 — base-dir por ejecución (modelo de proyecto / paths web-app) ──
 * Cuando un proyecto está activo (p.ej. "/app/<proj>"), los paths RELATIVOS de
 * readFile / load(.win) / imágenes resuelven bajo esa raíz; los ABSOLUTOS
 * (/sys, /lib, /app/...) NO cambian. Sin base-dir = modo plano de hoy. */
void        bpvm_fs_set_basedir(const char* dir);   /* NULL/"" = limpiar (plano) */
const char* bpvm_fs_basedir(void);                  /* "" si no hay proyecto (= projectPath) */
/* H19 — ruta COMPLETA del módulo principal (entry) en ejecución. App.mainModulePath()
 * la devuelve; App.projectPath() devuelve bpvm_fs_basedir(); App.mainModule() = su
 * nombre. La fija el host (arg .mod) o el firmware (set_basedir_from_module). */
void        bpvm_fs_set_main_module_path(const char* path);
const char* bpvm_fs_main_module_path(void);         /* "" si no hay módulo fijado */
/* Deriva el base-dir del path de un módulo de arranque: "/app/<proj>/entry.mod"
 * → fija "/app/<proj>"; cualquier otra cosa → modo plano. Lo llaman los repls
 * del firmware al ejecutar un módulo (y el host si se arranca con proyecto). */
void        bpvm_fs_set_basedir_from_module(const char* modpath);
/* Resuelve un path de recurso BP a su ruta efectiva en el FS. Absoluto → tal
 * cual. Relativo → (1) <basedir>/<path> si hay proyecto y existe; (2) <path>
 * tal cual; (3) "/app/<path>" (modo plano); (4) [V7/H2, #493] lo que el
 * RESPALDO diga que tiene con ese nombre, ya cualificado ("pack:<Pack>/<path>"):
 * la zona de packs es el ÚLTIMO sitio donde se busca un recurso, y se busca
 * ahí A PROPÓSITO, no porque conteste a rutas ajenas. Escribe en out[outsz] y
 * lo devuelve. Para los builtins de lectura de recursos (readFile, load de
 * .win, imágenes). */
const char* bpvm_fs_resolve(const char* path, char* out, size_t outsz);

/* ── #310: OVERLAY de lectura ────────────────────────────────────────────────
 * Quien ejecuta un pack puede interponer una resolución de LECTURA que se
 * consulta ANTES del backend: así los recursos del pack en ejecución van
 * primero, exactamente igual que sus módulos.
 *
 * El FS NO sabe de packs y no tiene por qué: sólo ofrece el gancho. La política
 * la pone quien la tiene (la VM). Sin overlay instalado, cero coste — un
 * puntero a NULL.
 *
 * Contrato: `stat` devuelve 0 si el recurso es SUYO (y rellena size), !=0 para
 * decir "no lo tengo, sigue por el camino normal". `read`/`read_at` sólo se
 * llaman si el stat lo reclamó. SÓLO LECTURA: escribir nunca pasa por aquí
 * (un pack es de sólo lectura). */
typedef int  (*bpvm_fs_ov_stat_fn)(void* user, const char* path, uint32_t* size);
typedef long (*bpvm_fs_ov_read_fn)(void* user, const char* path, uint32_t off,
                                   uint8_t* dst, uint32_t cap);
void bpvm_fs_set_overlay(bpvm_fs_ov_stat_fn st, bpvm_fs_ov_read_fn rd, void* user);

/* ── #362: RESPALDO de lectura (el espejo del overlay) ───────────────────────
 * Mismo contrato y mismos tipos que el overlay, pero se consulta DESPUÉS del
 * backend, y sólo si éste no tiene el fichero.
 *
 * Existe porque el orden no es uno solo, son tres, y el de en medio es el FS:
 *
 *     pack EN EJECUCIÓN  →  FS  →  zona de packs GRABADOS
 *     (overlay, arriba)     (backend)   (respaldo, aquí)
 *
 * Con un único gancho "antes" no se puede expresar: la zona quedaría por
 * delante del FS, y entonces un fichero puesto a mano en el dispositivo ya no
 * podría tapar al de un pack grabado — que es justo lo que el FS debe poder
 * hacer (spec §4: el FS ECLIPSA al pack).
 *
 * [V7/H2, #493] EL RESPALDO CONTESTA SÓLO EN SU PROPIO ESPACIO. Hasta el 15-sep
 * reclamaba CUALQUIER ruta cuyo basename coincidiera (`/app/Str.mod`,
 * `/lib/Str.mod`, el nombre pelado...): `stat("/app/Str.mod")` decía que sí sin
 * que en /app hubiera nada, la resolución de módulos se creía que el módulo
 * estaba en el FS, lo copiaba a RAM (el XIP llevaba muerto desde el 13-ago) y
 * un pack con una stdlib vieja TAPABA a /lib disfrazado de /app. Ahora `stat`
 * y `read` sólo reclaman rutas cualificadas (`pack:<Pack>/x`), y para "¿alguien
 * tiene x?" está `locate`: devuelve la ruta cualificada del que lo tiene, y la
 * fachada la prueba como ÚLTIMO candidato de `bpvm_fs_resolve`. Los recursos
 * siguen saliendo de la zona; los módulos, por la rama XIP de discover_deps.
 *
 * Sólo LECTURA, igual que el overlay, y coste cero sin instalar. `locate` puede
 * ser NULL. */
typedef int (*bpvm_fs_ov_locate_fn)(void* user, const char* nombre, char* out, size_t cap);
void bpvm_fs_set_fallback(bpvm_fs_ov_stat_fn st, bpvm_fs_ov_read_fn rd,
                          bpvm_fs_ov_locate_fn locate, void* user);

/* Backend host (libc). Implementado en fs_host.c (host-only); el firmware
 * registra el suyo (fs_get/fs_put). */
void bpvm_fs_register_host(void);

/* [V7/H2, #493] `/lib` EN EL HOST: monta en `prefix` (p.ej. "/lib") un backend
 * host de sólo lectura enraizado en `root`: `/lib/Str.mod` → `<root>/Str.mod`.
 * Es el espejo del `--stdlibDir` de miVM; sin él la VM-C del PC no tenía forma
 * de decir dónde está la stdlib y `/lib/X.mod` era un `fopen` literal. Una sola
 * raíz (la última llamada manda). 0 = OK. */
int bpvm_fs_register_host_dir(const char* prefix, const char* root);

/* H2 fase A (B1.1) — backend littlefs sobre una IMAGEN en fichero (host-only,
 * fs_lfs_host.c): el modo ORÁCULO de la VM-C en PC (mismo motor que el micro).
 * Monta img_path (block_size/block_count en 0 → defaults 4096/256 = 1 MB);
 * si el mount falla y format_if_needed, formatea. 0 / -1. Solo tipos planos
 * aquí: el API que arrastra lfs.h vive en bpvm_fs_lfs.h. */

/* ── #329 (cerrado en V6/U3 g5) — por qué falló la última op de FS ──────────
 * La fachada devuelve 0/-1 y el -1 no dice nada. `bpvm_fs_lfs_last_err()` sí,
 * pero es un `int` de littlefs y para leerlo hay que arrastrar lfs.h — cosa que
 * quien contesta al IDE no debería tener que hacer. Esto es la MISMA
 * información sin el acoplamiento.
 *
 * Sale a la capa de arriba como una clasificación corta, no como un código de
 * wire: los nombres del protocolo son del protocolo, no del sistema de
 * ficheros. */
typedef enum {
    BPVM_FS_FAIL_OTHER = 0,
    BPVM_FS_FAIL_NO_SPACE,
    BPVM_FS_FAIL_TOO_BIG,
    BPVM_FS_FAIL_NAME_TOO_LONG,
    BPVM_FS_FAIL_EXISTS,
    BPVM_FS_FAIL_NOT_FOUND,
    BPVM_FS_FAIL_IO
} bpvm_fs_fail_t;

bpvm_fs_fail_t bpvm_fs_last_fail(void);

/* #628 (2-oct) — QUIÉN ANOTA EL MOTIVO, y dónde vive. Vivía en fs_lfs.c como un
 * `int` de littlefs: la fachada dependía de littlefs por un símbolo (test-crc no
 * enlazaba desde el 31-ago), FAT y el motor del PC no anotaban nada —un PUT que
 * fallaba en /sd contestaba con el motivo RANCIO de la última operación de
 * littlefs— y en littlefs sólo `write` y el cursor (un DEL que fallaba, igual).
 * Ahora vive en la fachada y lo anota el que falla: el motor (su nombre corto,
 * su código crudo, la clase y un texto fijo) o la fachada. Y cada fallo lleva el
 * SUYO: si una operación falla sin que nadie anote nada, anota la fachada «el
 * motor no dice por qué». Como errno: sólo vale leerlo justo después de un -1. */
void bpvm_fs_anota_fallo(const char* motor, int crudo, bpvm_fs_fail_t clase, const char* texto);

/* Deja en el log la línea de un fallo de FS (op, fichero, tamaño, el motivo
 * anotado —motor, código crudo y texto— y espacio libre). Vivía DUPLICADA en la
 * Pico y en el STM32, idéntica carácter por carácter; desde #628, en la fachada,
 * que es quien guarda el motivo. */
void bpvm_fs_log_fail(const char* op, const char* path, unsigned long size,
                      unsigned long libre, unsigned long total);
int  bpvm_fs_register_lfs_filebd(const char* img_path,
                                 unsigned block_size, unsigned block_count,
                                 int format_if_needed);
void bpvm_fs_lfs_filebd_close(void);

/* ── V7/H3 (#491) — EL CURSOR, cara de arriba ──────────────────────────────
 * Un fichero abierto es un BLOQUE de bytes que pone el llamador (la VM: el
 * byte[] del objeto File). Dentro, por este orden: la cabecera de la fachada
 * (bpvm_fs_file_t), el estado del motor (alineado a 8) y la ruta con su NUL.
 * `bpvm_fs_file_block_size(path)` dice cuánto hace falta para ESA ruta (el
 * motor al que rutea más la ruta). Los modos: */
enum { BPVM_FS_O_READ   = 0,   /* "r"  : existe; sólo lectura */
       BPVM_FS_O_WRITE  = 1,   /* "w"  : crea o trunca; lectura y escritura */
       BPVM_FS_O_APPEND = 2,   /* "a"  : crea si falta; el cursor al FINAL al abrir (no
                                *        fuerza cada escritura al final, como el "a" de C) */
       BPVM_FS_O_RW     = 3 }; /* "r+" : existe; lectura y escritura */

#define BPVM_FS_FILE_MAGIC 0x42504631u   /* 'BPF1': el bloque tiene un fichero abierto */
typedef struct {
    uint32_t magic;      /* BPVM_FS_FILE_MAGIC = abierto; cualquier otra cosa = cerrado */
    uint32_t pos;        /* el cursor */
    uint32_t size;       /* tamaño conocido (se actualiza al escribir) */
    uint32_t next;       /* DE LA VM, la fachada no lo toca: su registro de abiertos se
                          * enhebra por los propios bloques (la ref del siguiente) */
    uint16_t st_off;     /* dónde empieza el estado del motor dentro del bloque */
    uint16_t path_off;   /* dónde empieza la ruta */
    uint8_t  mode;       /* BPVM_FS_O_* */
    uint8_t  kind;       /* de dónde sale: 1 motor · 2 overlay (pack en ejecución) · 3 respaldo
                          * (zona de packs) · 4 read_at (motor sin cursor). 2/3/4 = sólo lectura */
    uint8_t  pad[2];
    const bpvm_fs_backend_t* be;   /* kind 1 y 4 */
} bpvm_fs_file_t;

uint32_t bpvm_fs_file_block_size(const char* path);
/* 0 / -1 (el porqué en bpvm_fs_last_fail: NOT_FOUND; TOO_BIG = el bloque no
 * cabe; OTHER = nadie sabe escribir ahí, o el bloque ya tiene un fichero). */
int  bpvm_fs_fopen (void* blk, uint32_t cap, const char* path, int mode);
int  bpvm_fs_fclose(void* blk);                                   /* idempotente; 0 / -1 */
long bpvm_fs_fread (void* blk, uint8_t* dst, uint32_t n);         /* leídos; 0 = fin; -1 */
long bpvm_fs_fwrite(void* blk, const uint8_t* src, uint32_t n);   /* escritos (== n) o -1 */
int  bpvm_fs_fseek (void* blk, uint32_t pos);                     /* 0..size; fuera → -1 */
long bpvm_fs_ftell (void* blk);                                   /* -1 si cerrado */
long bpvm_fs_fsize (void* blk);                                   /* -1 si cerrado */
int  bpvm_fs_fflush(void* blk);                                   /* 0 / -1 */
int  bpvm_fs_fisopen(void* blk);                                  /* 1 / 0 */
const char* bpvm_fs_fpath(void* blk);                             /* "" si cerrado */

#ifdef __cplusplus
}
#endif

#endif /* BPVM_FS_H */
