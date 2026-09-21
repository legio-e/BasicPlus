/*
 * test_fs_cursor.c — V7/H3 (#491): EL CURSOR de la fachada de FS.
 *
 * ─── POR QUÉ ESTE TEST EXISTE ───
 *
 * Hasta hoy la fachada no tenía descriptores: cada op abría y cerraba, y la
 * misma enfermedad volvió tres veces (#398, #453, #473). El cursor es un
 * fichero ABIERTO sobre un bloque de estado que pone el llamador —en la VM, el
 * byte[] del objeto File— y su contrato tiene que ser EL MISMO en los tres
 * motores, porque es lo que las dos VMs van a prometer al programa BP.
 *
 * Así que el mismo guion corre sobre el backend de HOST (ficheros de verdad) y
 * sobre littlefs (imagen filebd, el motor de la placa): lo que difiera entre
 * los dos es un bug de contrato, no de plataforma. Y se comprueba lo que
 * distingue un cursor de un fichero entero: la posición después de cada op, lo
 * que queda alrededor de una escritura en medio, el "a" con el cursor al final
 * (y sin forzar cada escritura al final), el seek fuera de rango como ERROR,
 * y VARIOS ficheros abiertos a la vez — el tope de dos que imponían los buffers
 * estáticos de littlefs desaparece porque cada bloque lleva su caché.
 */
#include "bpvm_fs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int fallos = 0;
static int total  = 0;

static void ok(int cond, const char* que) {
    total++;
    if (!cond) { fallos++; printf("  FAIL: %s\n", que); }
}

/* Un bloque del tamaño que pide la fachada para esa ruta (alineado a 4, como
 * un byte[] de la VM: la fachada tiene que apañarse con eso). */
static void* bloque(const char* path, uint32_t* cap) {
    *cap = bpvm_fs_file_block_size(path);
    uint8_t* b = (uint8_t*) malloc(*cap + 4);
    memset(b, 0xEE, *cap + 4);           /* basura a propósito: el magic manda */
    return (((uintptr_t) b) & 4u) ? b + 4 : b;   /* 4 mod 8, el caso incómodo */
}

static void guion(const char* motor, const char* F) {
    printf("--- %s: %s ---\n", motor, F);
    uint32_t cap = 0, sz = 0;
    uint8_t buf[64];
    void* b = bloque(F, &cap);
    bpvm_fs_remove(F);

    /* 0 — el tamaño del bloque cubre cabecera + holgura + ruta (y el estado del motor) */
    ok(cap >= (uint32_t) sizeof(bpvm_fs_file_t) + 8u + (uint32_t) strlen(F) + 1u, "block_size cubre cabecera+holgura+ruta");
    ok(bpvm_fs_fisopen(b) == 0, "un bloque con basura NO está abierto");

    /* 1 — "r" de lo que no existe: -1, y el bloque sigue cerrado */
    ok(bpvm_fs_fopen(b, cap, F, BPVM_FS_O_READ) == -1, "\"r\" sin fichero → -1");
    ok(bpvm_fs_fisopen(b) == 0, "sigue cerrado");

    /* 2 — "w": escribir, posicionarse, escribir EN MEDIO sin tocar lo de alrededor */
    ok(bpvm_fs_fopen(b, cap, F, BPVM_FS_O_WRITE) == 0, "\"w\" abre (crea)");
    ok(bpvm_fs_fisopen(b) == 1 && strcmp(bpvm_fs_fpath(b), F) == 0, "abierto y con su ruta");
    ok(bpvm_fs_ftell(b) == 0 && bpvm_fs_fsize(b) == 0, "recién creado: pos 0, size 0");
    ok(bpvm_fs_fwrite(b, (const uint8_t*) "ABCDEFGH", 8) == 8, "write 8");
    ok(bpvm_fs_ftell(b) == 8 && bpvm_fs_fsize(b) == 8, "tras escribir: pos 8, size 8");
    ok(bpvm_fs_fseek(b, 3) == 0 && bpvm_fs_ftell(b) == 3, "seek 3");
    ok(bpvm_fs_fwrite(b, (const uint8_t*) "xyz", 3) == 3, "write en medio");
    ok(bpvm_fs_ftell(b) == 6 && bpvm_fs_fsize(b) == 8, "pos 6, size sigue 8");
    ok(bpvm_fs_fseek(b, 0) == 0, "seek 0");
    memset(buf, 0, sizeof buf);
    ok(bpvm_fs_fread(b, buf, sizeof buf) == 8 && memcmp(buf, "ABCxyzGH", 8) == 0, "lee ABCxyzGH: lo de alrededor intacto");
    ok(bpvm_fs_fread(b, buf, sizeof buf) == 0, "al final: 0 = fin");
    ok(bpvm_fs_fseek(b, 9) == -1 && bpvm_fs_ftell(b) == 8, "seek más allá del final: ERROR y la pos no se mueve");
    ok(bpvm_fs_fseek(b, 8) == 0, "seek AL final vale");
    ok(bpvm_fs_fwrite(b, (const uint8_t*) "IJ", 2) == 2 && bpvm_fs_fsize(b) == 10, "extender escribiendo al final: size 10");
    ok(bpvm_fs_fflush(b) == 0, "flush");
    ok(bpvm_fs_fopen(b, cap, F, BPVM_FS_O_READ) == -1, "abrir sobre un bloque YA abierto: -1");
    ok(bpvm_fs_fclose(b) == 0 && bpvm_fs_fisopen(b) == 0, "close");
    ok(bpvm_fs_fclose(b) == 0, "close otra vez: idempotente");
    ok(bpvm_fs_stat(F, &sz) == 0 && sz == 10, "stat: 10 bytes");

    /* 3 — "r": sólo lectura */
    ok(bpvm_fs_fopen(b, cap, F, BPVM_FS_O_READ) == 0, "\"r\" abre");
    ok(bpvm_fs_fsize(b) == 10 && bpvm_fs_ftell(b) == 0, "size 10, pos 0");
    ok(bpvm_fs_fread(b, buf, 4) == 4 && memcmp(buf, "ABCx", 4) == 0 && bpvm_fs_ftell(b) == 4, "read 4 → ABCx, pos 4");
    ok(bpvm_fs_fwrite(b, (const uint8_t*) "no", 2) == -1, "write en \"r\": -1");
    ok(bpvm_fs_fflush(b) == 0, "flush en \"r\": no hace nada y no falla");
    ok(bpvm_fs_fseek(b, 10) == 0 && bpvm_fs_fread(b, buf, 4) == 0, "seek al final y leer: 0");
    ok(bpvm_fs_fclose(b) == 0, "close");

    /* 4 — "a": el cursor al final AL ABRIR; después manda la posición */
    ok(bpvm_fs_fopen(b, cap, F, BPVM_FS_O_APPEND) == 0, "\"a\" abre");
    ok(bpvm_fs_ftell(b) == 10 && bpvm_fs_fsize(b) == 10, "\"a\": pos 10 = size");
    ok(bpvm_fs_fwrite(b, (const uint8_t*) "KL", 2) == 2 && bpvm_fs_fsize(b) == 12, "append: size 12");
    ok(bpvm_fs_fseek(b, 0) == 0 && bpvm_fs_fwrite(b, (const uint8_t*) "ab", 2) == 2, "seek 0 y escribir: PISA, no añade");
    ok(bpvm_fs_fsize(b) == 12, "size sigue 12");
    ok(bpvm_fs_fclose(b) == 0, "close");
    memset(buf, 0, sizeof buf);
    ok(bpvm_fs_read(F, buf, sizeof buf) == 12 && memcmp(buf, "abCxyzGHIJKL", 12) == 0, "el fichero: abCxyzGHIJKL");

    /* 5 — "r+": existe, sin truncar */
    ok(bpvm_fs_fopen(b, cap, F, BPVM_FS_O_RW) == 0, "\"r+\" abre");
    ok(bpvm_fs_ftell(b) == 0 && bpvm_fs_fsize(b) == 12, "\"r+\": pos 0, size 12 (no trunca)");
    ok(bpvm_fs_fseek(b, 12) == 0 && bpvm_fs_fwrite(b, (const uint8_t*) "M", 1) == 1, "extender por el final");
    ok(bpvm_fs_fclose(b) == 0 && bpvm_fs_stat(F, &sz) == 0 && sz == 13, "size 13");
    { char no[96]; snprintf(no, sizeof no, "%s-no", F);
      ok(bpvm_fs_fopen(b, cap, no, BPVM_FS_O_RW) == -1, "\"r+\" de lo que no existe: -1"); }

    /* 6 — "w" sobre lo que existe: TRUNCA */
    ok(bpvm_fs_fopen(b, cap, F, BPVM_FS_O_WRITE) == 0 && bpvm_fs_fsize(b) == 0, "\"w\" trunca: size 0");
    ok(bpvm_fs_fclose(b) == 0 && bpvm_fs_stat(F, &sz) == 0 && sz == 0, "stat: 0");

    /* 7 — el bloque que no cabe */
    ok(bpvm_fs_fopen(b, cap - 1, F, BPVM_FS_O_READ) == -1 && bpvm_fs_fisopen(b) == 0, "bloque corto: -1 y cerrado");

    /* 8 — VARIOS abiertos a la vez (littlefs tenía dos buffers estáticos: ya no es el tope) */
    {
        enum { N = 4 };
        void* bs[N]; uint32_t caps[N]; char nombres[N][64];
        for (int i = 0; i < N; i++) {
            snprintf(nombres[i], sizeof nombres[i], "%s.%d", F, i);
            bs[i] = bloque(nombres[i], &caps[i]);
            bpvm_fs_remove(nombres[i]);
            ok(bpvm_fs_fopen(bs[i], caps[i], nombres[i], BPVM_FS_O_WRITE) == 0, "abre uno más");
        }
        for (int v = 0; v < 3; v++)
            for (int i = 0; i < N; i++) {
                uint8_t c = (uint8_t) ('0' + i);
                ok(bpvm_fs_fwrite(bs[i], &c, 1) == 1, "escribe entrelazado");
            }
        for (int i = 0; i < N; i++) {
            ok(bpvm_fs_fclose(bs[i]) == 0, "cierra");
            memset(buf, 0, sizeof buf);
            char esperado[4] = { (char) ('0' + i), (char) ('0' + i), (char) ('0' + i), 0 };
            ok(bpvm_fs_read(nombres[i], buf, sizeof buf) == 3 && memcmp(buf, esperado, 3) == 0, "cada uno lo suyo");
            bpvm_fs_remove(nombres[i]);
        }
    }
    bpvm_fs_remove(F);
}

int main(void) {
    printf("--- V7/H3 (#491): el cursor de la fachada — mismo guion en host y en littlefs ---\n");

    /* HOST: ficheros de verdad en el directorio actual */
    bpvm_fs_register_host();
    guion("host", "build/test_cursor.bin");

    /* Un motor SIN cursor (el montaje /lib del host, sólo lectura): la fachada
     * sirve la lectura con read_at a la posición de la cabecera (kind 4). */
    {
        bpvm_fs_mkdir("build/cursor_lib");
        bpvm_fs_write("build/cursor_lib/x.txt", (const uint8_t*) "hola mundo", 10, 0);
        ok(bpvm_fs_register_host_dir("/lib", "build/cursor_lib") == 0, "monta /lib");
        uint32_t cap = 0; uint8_t buf[32];
        void* b = bloque("/lib/x.txt", &cap);
        ok(bpvm_fs_fopen(b, cap, "/lib/x.txt", BPVM_FS_O_READ) == 0, "/lib (sin fopen): abre en lectura");
        ok(bpvm_fs_fsize(b) == 10, "size 10");
        ok(bpvm_fs_fseek(b, 5) == 0 && bpvm_fs_fread(b, buf, 32) == 5 && memcmp(buf, "mundo", 5) == 0, "seek 5 + read → mundo");
        ok(bpvm_fs_fread(b, buf, 32) == 0, "fin");
        ok(bpvm_fs_fclose(b) == 0, "close");
        ok(bpvm_fs_fopen(b, cap, "/lib/x.txt", BPVM_FS_O_WRITE) == -1, "/lib: escribir no (nadie sabe)");
        bpvm_fs_remove("build/cursor_lib/x.txt");
    }

    /* LITTLEFS: el motor de la placa, sobre una imagen */
    const char* IMG = "build/fs_cursor_test.img";
    remove(IMG);
    ok(bpvm_fs_register_lfs_filebd(IMG, 4096, 32, 1) == 0, "attach littlefs + format");
    ok(bpvm_fs_mkdir("/app") == 0, "mkdir /app");
    guion("littlefs", "/app/cursor.bin");
    bpvm_fs_lfs_filebd_close();
    remove(IMG);

    printf("%d/%d, %d fallos\n", total - fallos, total, fallos);
    return fallos ? 1 : 0;
}
