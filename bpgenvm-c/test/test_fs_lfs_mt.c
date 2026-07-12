/*
 * test_fs_lfs_mt.c — B1.4 (H2 fase A): estrés MULTITAREA de la fachada
 * bpvm_fs sobre littlefs (el requisito de Eduardo: FS robusto con varios
 * clientes — hoy N Threads BP + la comm task; aquí modelado con pthreads
 * martilleando la MISMA fachada que usan los builtins y el wire).
 *
 * Cada thread t hace M rondas de:
 *   - append de una línea marcada a su fichero PROPIO (own<t>.txt)
 *   - append de una línea marcada al fichero COMPARTIDO (shared.log)
 *   - fileSize + read-verify de su propio fichero
 * Verificación FINAL (single-thread, por PROPIEDADES):
 *   - own<t>.txt: byte-exacto contra lo esperado (orden determinista propio)
 *   - shared.log: tamaño EXACTO = suma de todas las líneas; el conteo de
 *     líneas por marcador == M para cada thread; NINGUNA línea rota
 *     (atomicidad por operación de la fachada = una línea de log jamás se
 *     entrelaza — la garantía del caso de uso del LOG).
 *
 * CONTROL (metodología 7a): 1 thread → 0 corrupciones (test sano).
 * ROJO→VERDE: sin el lock grueso, littlefs (no reentrante) se corrompe o
 * revienta con N>1; con el lock (B1.4) todo verde.
 *
 * Uso: test_fs_lfs_mt <imagen.img> [nthreads] [rondas]   (default 4, 60)
 */
#include "bpvm_fs.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXT 16

static int g_rondas = 60;
static int g_corr[MAXT];

/* línea propia: "own<t>:r<ronda>\n" · línea compartida: "L<t>\n" */
static int own_line(char* dst, size_t cap, int t, int r) {
    return snprintf(dst, cap, "own%d:r%03d\n", t, r);
}
static int shared_line(char* dst, size_t cap, int t) {
    return snprintf(dst, cap, "L%d\n", t);
}

static void* worker(void* arg) {
    int t = (int)(intptr_t) arg;
    char path[32], line[48];
    snprintf(path, sizeof path, "own%d.txt", t);
    bpvm_fs_write(path, NULL, 0, 0);   /* trunca/crea vacío */

    long esperado = 0;
    for (int r = 0; r < g_rondas; r++) {
        int n = own_line(line, sizeof line, t, r);
        if (bpvm_fs_write(path, (const uint8_t*) line, (uint32_t) n, 1) != 0) g_corr[t]++;
        esperado += n;

        int m = shared_line(line, sizeof line, t);
        if (bpvm_fs_write("shared.log", (const uint8_t*) line, (uint32_t) m, 1) != 0) g_corr[t]++;

        uint32_t sz = 0;
        if (bpvm_fs_stat(path, &sz) != 0 || sz != (uint32_t) esperado) g_corr[t]++;
    }
    return NULL;
}

static uint8_t g_buf[65536];

int main(int argc, char** argv) {
    if (argc < 2) { printf("uso: test_fs_lfs_mt <imagen.img> [nthreads] [rondas]\n"); return 2; }
    const char* img = argv[1];
    int nthreads = argc > 2 ? atoi(argv[2]) : 4;
    if (argc > 3) g_rondas = atoi(argv[3]);
    if (nthreads < 1) nthreads = 1;
    if (nthreads > MAXT) nthreads = MAXT;

    remove(img);
    if (bpvm_fs_register_lfs_filebd(img, 4096, 64, 1) != 0) {   /* 256 KB */
        printf("FAIL: no monta littlefs sobre %s\n", img);
        return 1;
    }
    bpvm_fs_write("shared.log", NULL, 0, 0);   /* compartido vacío */

    pthread_t th[MAXT];
    memset(g_corr, 0, sizeof g_corr);
    for (int t = 0; t < nthreads; t++)
        pthread_create(&th[t], NULL, worker, (void*)(intptr_t) t);
    long corr = 0;
    for (int t = 0; t < nthreads; t++) { pthread_join(th[t], NULL); corr += g_corr[t]; }

    /* ---- verificación final por propiedades (single-thread) ---- */
    char line[48];
    for (int t = 0; t < nthreads; t++) {
        char path[32], exp[8192];
        snprintf(path, sizeof path, "own%d.txt", t);
        size_t eoff = 0;
        for (int r = 0; r < g_rondas; r++)
            eoff += (size_t) own_line(exp + eoff, sizeof exp - eoff, t, r);
        long n = bpvm_fs_read(path, g_buf, sizeof g_buf);
        if (n < 0 || (size_t) n != eoff || memcmp(g_buf, exp, eoff) != 0) {
            printf("FAIL: %s no es byte-exacto (n=%ld esperado=%zu)\n", path, n, eoff);
            corr++;
        }
    }
    long n = bpvm_fs_read("shared.log", g_buf, sizeof g_buf);
    long esperado_total = 0;
    for (int t = 0; t < nthreads; t++)
        esperado_total += (long) strlen((shared_line(line, sizeof line, t), line)) * g_rondas;
    if (n != esperado_total) {
        printf("FAIL: shared.log tamano=%ld esperado=%ld (lineas perdidas/rotas)\n",
               n, esperado_total);
        corr++;
    }
    /* conteo por marcador + ninguna línea rota */
    int cnt[MAXT]; memset(cnt, 0, sizeof cnt);
    long i = 0;
    while (i < n) {
        if (g_buf[i] != 'L') { printf("FAIL: linea rota en shared.log off=%ld\n", i); corr++; break; }
        long j = i + 1, id = 0;
        while (j < n && g_buf[j] >= '0' && g_buf[j] <= '9') { id = id * 10 + (g_buf[j] - '0'); j++; }
        if (j >= n || g_buf[j] != '\n' || id < 0 || id >= nthreads) {
            printf("FAIL: linea rota en shared.log off=%ld\n", i); corr++; break;
        }
        cnt[id]++;
        i = j + 1;
    }
    for (int t = 0; t < nthreads; t++) {
        if (cnt[t] != g_rondas) {
            printf("FAIL: marcador L%d aparece %d veces (esperado %d)\n", t, cnt[t], g_rondas);
            corr++;
        }
    }

    bpvm_fs_lfs_filebd_close();
    remove(img);
    printf("fs-mt: threads=%d rondas=%d corrupciones=%ld %s\n",
           nthreads, g_rondas, corr, corr == 0 ? "(VERDE)" : "(ROJO)");
    return corr == 0 ? 0 : 1;
}
