/*
 * bpvm_bios_fs.c — las ranuras de FICHEROS de la tabla BIOS (V5/H2).
 *
 * Portable de nacimiento: todo va por la fachada `bpvm_fs`, así que sirve igual
 * en la Pico, en el ESP32, en el STM32 y en el host. Cada firmware sólo tiene
 * que apuntar sus ranuras aquí.
 *
 * ─── QUÉ ES UN DESCRIPTOR AQUÍ, Y QUÉ NO ───
 *
 * Es una entrada de esta tabla que guarda **el camino**. No hay ningún fichero
 * abierto de verdad detrás: cada lectura y cada escritura llaman a la fachada,
 * que abre y cierra. La forma es la que quiere una base de datos; el fondo, de
 * momento, es el que ya había.
 *
 * Se hace así a sabiendas y por un motivo concreto: **la forma de la API es lo
 * único que un pack ya grabado no puede cambiar**. Si naciera por camino y
 * mañana hiciera falta velocidad, habría que romper el contrato y con él todos
 * los packs. Naciendo por descriptor, el día que moleste se guarda aquí el
 * fichero abierto de verdad y el pack no se entera.
 *
 * ⚠️ El coste de HOY, dicho para que nadie se sorprenda midiendo: cada lectura
 * vuelve a recorrer el directorio. Sobre una SD por SPI eso se nota, y se notará
 * más cuanto más grande sea el árbol. Es deuda ELEGIDA, no descuido.
 */
#include "bpvm_bios.h"
#include "bpvm_fs.h"

#include <string.h>

/* Ocho basta y sobra: una BD abre su fichero y su diario. Que el número sea
 * pequeño es a propósito — así "no quedan descriptores" sale en las pruebas y
 * no en producción dentro de un año. */
#define BIOS_FS_MAX      8
#define BIOS_FS_CAMINO  96

typedef struct {
    char camino[BIOS_FS_CAMINO];
    int  usado;
    int  escribible;
} bios_fd_t;

static bios_fd_t s_fds[BIOS_FS_MAX];

/* Los descriptores empiezan en 1: el 0 es un valor demasiado fácil de obtener
 * por accidente (una struct a cero, un campo sin poner) y sería indistinguible
 * de un fichero legítimamente abierto. Con la base en 1, el cero es siempre un
 * error. */
static bios_fd_t* dame(int fd) {
    if (fd < 1 || fd > BIOS_FS_MAX) return NULL;
    bios_fd_t* d = &s_fds[fd - 1];
    return d->usado ? d : NULL;
}

int bpvm_bios_fs_abrir(const char* camino, int para_escribir) {
    if (!camino || camino[0] == '\0') return -1;
    if (strlen(camino) >= BIOS_FS_CAMINO) return -1;

    /* Para LEER, el fichero tiene que existir; para escribir, no —`write_at` lo
     * crea, que es lo que hace una BD la primera vez. Distinguirlo aquí evita
     * que "no existe" se confunda con "no se puede escribir". */
    if (!para_escribir && !bpvm_fs_exists(camino)) return -1;

    for (int i = 0; i < BIOS_FS_MAX; i++) {
        if (s_fds[i].usado) continue;
        memcpy(s_fds[i].camino, camino, strlen(camino) + 1);
        s_fds[i].usado      = 1;
        s_fds[i].escribible = para_escribir ? 1 : 0;
        return i + 1;
    }
    return -1;                          /* no quedan descriptores */
}

int bpvm_bios_fs_cerrar(int fd) {
    bios_fd_t* d = dame(fd);
    if (!d) return -1;
    d->usado = 0;
    d->camino[0] = '\0';
    return 0;
}

long bpvm_bios_fs_leer(int fd, uint32_t desde, void* dst, uint32_t n) {
    bios_fd_t* d = dame(fd);
    if (!d || !dst) return -1;
    return bpvm_fs_read_at(d->camino, desde, (uint8_t*) dst, n);
}

long bpvm_bios_fs_escribir(int fd, uint32_t desde, const void* src, uint32_t n) {
    bios_fd_t* d = dame(fd);
    if (!d || !src) return -1;
    /* Abierto para leer y escribiendo: se dice que NO. Dejarlo pasar sería
     * convertir un error del llamante en una corrupción silenciosa. */
    if (!d->escribible) return -1;
    return bpvm_fs_write_at(d->camino, desde, (const uint8_t*) src, n);
}

int bpvm_bios_fs_truncar(int fd, uint32_t tam) {
    bios_fd_t* d = dame(fd);
    if (!d) return -1;
    if (!d->escribible) return -1;
    return bpvm_fs_truncate(d->camino, tam);
}

long bpvm_bios_fs_tamano(int fd) {
    bios_fd_t* d = dame(fd);
    if (!d) return -1;
    uint32_t sz = 0;
    if (bpvm_fs_stat(d->camino, &sz) != 0) return -1;
    return (long) sz;
}

int bpvm_bios_fs_sincronizar(int fd) {
    /* Hoy no hay nada pendiente que volcar: cada operación abre, escribe y
     * CIERRA, y el cierre es quien vuelca en los dos motores. Devuelve 0 —que
     * es la verdad, no un apaño— y el día que el descriptor guarde un fichero
     * abierto de verdad, aquí irá el volcado y el pack ya la estará llamando. */
    return dame(fd) ? 0 : -1;
}

int bpvm_bios_fs_borrar(const char* camino) {
    if (!camino) return -1;
    return bpvm_fs_remove(camino);
}

int bpvm_bios_fs_existe(const char* camino) {
    if (!camino) return 0;
    return bpvm_fs_exists(camino) ? 1 : 0;
}

void bpvm_bios_fs_reset(void) {
    for (int i = 0; i < BIOS_FS_MAX; i++) { s_fds[i].usado = 0; s_fds[i].camino[0] = '\0'; }
}
