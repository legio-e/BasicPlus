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
 * tal cual; (3) "/app/<path>" (modo plano). Escribe en out[outsz] y lo devuelve.
 * Para los builtins de lectura de recursos (readFile, load de .win, imágenes). */
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
 * Sólo LECTURA, igual que el overlay, y coste cero sin instalar. */
void bpvm_fs_set_fallback(bpvm_fs_ov_stat_fn st, bpvm_fs_ov_read_fn rd, void* user);

/* Backend host (libc). Implementado en fs_host.c (host-only); el firmware
 * registra el suyo (fs_get/fs_put). */
void bpvm_fs_register_host(void);

/* H2 fase A (B1.1) — backend littlefs sobre una IMAGEN en fichero (host-only,
 * fs_lfs_host.c): el modo ORÁCULO de la VM-C en PC (mismo motor que el micro).
 * Monta img_path (block_size/block_count en 0 → defaults 4096/256 = 1 MB);
 * si el mount falla y format_if_needed, formatea. 0 / -1. Solo tipos planos
 * aquí: el API que arrastra lfs.h vive en bpvm_fs_lfs.h. */
int  bpvm_fs_register_lfs_filebd(const char* img_path,
                                 unsigned block_size, unsigned block_count,
                                 int format_if_needed);
void bpvm_fs_lfs_filebd_close(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_FS_H */
