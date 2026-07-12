/*
 * test_lfs_fault.c — B0.3 (H2 fase A): INYECCIÓN DE FALLOS en el block-device
 * emulado (lfs_emubd). La pieza que hace del host el oráculo de ROBUSTEZ (§4
 * del plan): reproduce determinísticamente lo casi imposible de provocar en
 * hardware — corte de corriente en CADA punto posible + bloques dañados.
 *
 * 1) BARRIDO EXHAUSTIVO DE CORTES DE CORRIENTE (metodología upstream):
 *    para nth = 1, 2, 3, ...  (hasta que la actualización complete sin corte)
 *      · escenario fresco: baseline COMMITTEADO (/app/a.dat = vA, 6000 B)
 *      · se arma el corte en la op de escritura nº nth (setpowercycles)
 *      · actualización "arriesgada": a.dat → vB (9000 B, multi-bloque con
 *        erases de por medio → ~42 puntos de corte) + crear /app/b.dat
 *      · el corte llega a media operación (powerloss_cb → longjmp)
 *      · "REBOOT": remonta el MISMO bd (estado tal cual quedó) y verifica:
 *          - el mount NUNCA falla (FS jamás brickeado)
 *          - a.dat es EXACTAMENTE vA o vB — nunca mezcla, nunca corrupto
 *            (garantía copy-on-write: commit en el close, o nada)
 *          - b.dat ausente, VACÍO (la creación committea en el open(O_CREAT),
 *            el contenido en el close — verificado empíricamente) o íntegro;
 *            y JAMÁS b.dat presente con a.dat aún vieja — el orden de commits
 *            se respeta (b se creó después del close de a)
 *          - el FS sigue USABLE: escribir + releer + borrar /app/c.tmp
 *    Se barre DOS veces: powerloss NOOP (progs atómicos) y OOO (bloques
 *    aterrizan out-of-order — más adversarial, emula caché del chip).
 *
 * 2) BLOQUES DAÑADOS: 5 bloques marcados muertos (PROGERROR al programar);
 *    littlefs debe REUBICAR en caliente y los datos salir byte-idénticos.
 *
 * Nota: tras un corte (longjmp) el lfs_t queda a medias; el remount lo
 * re-inicializa (mismo gesto que un reboot real). La caché del fichero
 * abierto en ese momento se fuga (~256 B/escenario) — irrelevante en test.
 */
#include "lfs.h"
#include "bd/lfs_emubd.h"
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#define BLOCK_SIZE   4096u
#define BLOCK_COUNT  32u
#define LEN_A        6000u    /* payload vA multi-bloque (versión vieja) */
#define LEN_B        9000u    /* payload vB multi-bloque (versión nueva) */
#define LEN_BIG      24000u   /* fichero grande del test de bloques malos */
#define MAX_SWEEP    1000     /* red de seguridad del barrido */

static lfs_t      lfs;
static lfs_emubd_t bd;
static jmp_buf    g_cut;      /* el "corte de corriente" salta aquí */

static void on_powerloss(void* ctx) { (void) ctx; longjmp(g_cut, 1); }

/* emubd config: power_cycles=0 (desarmado; se arma por escenario con
 * setpowercycles). erase_value=0xFF = flash real. behavior varía por barrido. */
static struct lfs_emubd_config ebcfg = {
    .read_size          = 16,
    .prog_size          = 16,
    .erase_size         = BLOCK_SIZE,
    .erase_count        = BLOCK_COUNT,
    .erase_value        = 0xFF,
    .erase_cycles       = 0,
    .badblock_behavior  = LFS_EMUBD_BADBLOCK_PROGERROR,
    .power_cycles       = 0,
    .powerloss_behavior = LFS_EMUBD_POWERLOSS_NOOP,
    .powerloss_cb       = on_powerloss,
};

static const struct lfs_config cfg = {
    .context = &bd,
    .read  = lfs_emubd_read,
    .prog  = lfs_emubd_prog,
    .erase = lfs_emubd_erase,
    .sync  = lfs_emubd_sync,

    .read_size      = 16,
    .prog_size      = 16,
    .block_size     = BLOCK_SIZE,
    .block_count    = BLOCK_COUNT,
    .cache_size     = 256,
    .lookahead_size = 16,
    .block_cycles   = 500,
};

#define CHK(expr, msg) do {                                            \
    int _e = (int) (expr);                                             \
    if (_e < 0) { printf("FAIL: %s (lfs err=%d)\n", (msg), _e); return -1; } \
} while (0)

/* payloads deterministas, distinguibles entre sí y por tamaño */
static uint8_t byte_a  (uint32_t i) { return (uint8_t) ((i ^ 0xA5u) & 0xFFu); }
static uint8_t byte_b  (uint32_t i) { return (uint8_t) ((i * 5u + 7u) & 0xFFu); }
static uint8_t byte_big(uint32_t i) { return (uint8_t) ((i * 13u + 1u) & 0xFFu); }

static int write_file(const char* path, uint8_t (*gen)(uint32_t), uint32_t len) {
    lfs_file_t f;
    static uint8_t buf[LEN_BIG];
    for (uint32_t i = 0; i < len; i++) buf[i] = gen(i);
    CHK(lfs_file_open(&lfs, &f, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC), path);
    CHK(lfs_file_write(&lfs, &f, buf, len), path);
    CHK(lfs_file_close(&lfs, &f), path);
    return 0;
}

/* 1 = es exactamente (gen,len) · 0 = no existe · -1 = corrupto/mezcla */
static int file_state(const char* path, uint8_t (*gen)(uint32_t), uint32_t len) {
    struct lfs_info info;
    int e = lfs_stat(&lfs, path, &info);
    if (e == LFS_ERR_NOENT) return 0;
    if (e < 0 || info.size != len) return -1;
    lfs_file_t f;
    if (lfs_file_open(&lfs, &f, path, LFS_O_RDONLY) < 0) return -1;
    static uint8_t buf[LEN_BIG];
    lfs_ssize_t n = lfs_file_read(&lfs, &f, buf, sizeof(buf));
    lfs_file_close(&lfs, &f);
    if (n < 0 || (uint32_t) n != len) return -1;
    for (uint32_t i = 0; i < len; i++) if (buf[i] != gen(i)) return -1;
    return 1;
}

/* Estados válidos de b.dat tras un corte: 0 = AUSENTE · 1 = VACÍO (creación
 * committeada en el open, contenido aún no) · 2 = ÍNTEGRO · -1 = corrupto. */
static int b_state(const char* path, uint8_t (*gen)(uint32_t), uint32_t len) {
    struct lfs_info info;
    int e = lfs_stat(&lfs, path, &info);
    if (e == LFS_ERR_NOENT) return 0;
    if (e < 0) return -1;
    if (info.size == 0) return 1;
    return (file_state(path, gen, len) == 1) ? 2 : -1;
}

/* Barrido exhaustivo de cortes para un powerloss_behavior dado.
 * Devuelve nº de puntos de corte barridos, o -1 si algún invariante cae. */
static int powerloss_sweep(lfs_emubd_powerloss_behavior_t behavior, const char* name) {
    int n_old = 0, n_new = 0, n_new_b = 0;

    for (int nth = 1; nth <= MAX_SWEEP; nth++) {
        ebcfg.powerloss_behavior = behavior;
        ebcfg.power_cycles       = 0;                 /* baseline sin cortes */
        CHK(lfs_emubd_create(&cfg, &ebcfg), "emubd_create");
        CHK(lfs_format(&lfs, &cfg), "format");
        CHK(lfs_mount(&lfs, &cfg), "mount baseline");
        CHK(lfs_mkdir(&lfs, "/app"), "mkdir /app");
        if (write_file("/app/a.dat", byte_a, LEN_A) < 0) return -1;   /* vA committeada */

        CHK(lfs_emubd_setpowercycles(&cfg, (lfs_emubd_powercycles_t) nth), "armar corte");

        if (setjmp(g_cut) == 0) {
            /* actualización arriesgada: a.dat→vB, luego crear b.dat */
            if (write_file("/app/a.dat", byte_b, LEN_B) < 0) return -1;
            if (write_file("/app/b.dat", byte_b, 32) < 0) return -1;
            /* llegó ENTERA sin corte → el barrido cubrió todos los puntos */
            CHK(lfs_unmount(&lfs), "unmount final");
            lfs_emubd_destroy(&cfg);
            printf("powerloss %s: %d puntos de corte barridos — 0 corrupciones "
                   "(a=vieja %d, a=nueva sin b %d, a=nueva con b %d)\n",
                   name, nth - 1, n_old, n_new, n_new_b);
            return nth - 1;
        }

        /* ---- CORTE a media op nº nth. "Reboot": remontar el MISMO bd. ---- */
        CHK(lfs_emubd_setpowercycles(&cfg, 0), "desarmar");
        if (lfs_mount(&lfs, &cfg) < 0) {
            printf("FAIL(%s nth=%d): el FS quedo BRICKEADO (mount fallo)\n", name, nth);
            return -1;
        }
        int sa = file_state("/app/a.dat", byte_a, LEN_A);
        int sb = file_state("/app/a.dat", byte_b, LEN_B);
        int st_b = b_state("/app/b.dat", byte_b, 32);
        /* a.dat: exactamente vA o vB (nunca mezcla/corrupto/ausente) */
        if (!((sa == 1 && sb != 1) || (sb == 1 && sa != 1))) {
            printf("FAIL(%s nth=%d): a.dat corrupto/mezcla (sa=%d sb=%d)\n",
                   name, nth, sa, sb);
            return -1;
        }
        if (st_b < 0) {
            printf("FAIL(%s nth=%d): b.dat corrupto\n", name, nth);
            return -1;
        }
        /* orden de commits: b.dat (en cualquier forma) exige a.dat ya en vB */
        if (sa == 1 && st_b != 0) {
            printf("FAIL(%s nth=%d): b.dat presente con a.dat AUN vieja (orden roto)\n",
                   name, nth);
            return -1;
        }
        if (sa == 1) n_old++; else if (st_b == 2) n_new_b++; else n_new++;

        /* el FS sigue usable tras recuperar */
        if (write_file("/app/c.tmp", byte_a, 64) < 0) return -1;
        if (file_state("/app/c.tmp", byte_a, 64) != 1) {
            printf("FAIL(%s nth=%d): FS no usable tras recuperar\n", name, nth);
            return -1;
        }
        CHK(lfs_remove(&lfs, "/app/c.tmp"), "rm c.tmp");
        CHK(lfs_unmount(&lfs), "unmount recuperado");
        lfs_emubd_destroy(&cfg);
    }
    printf("FAIL(%s): la actualizacion no completo en %d cortes (no converge)\n",
           name, MAX_SWEEP);
    return -1;
}

/* Bloques dañados: 5 bloques muertos (PROGERROR) → littlefs reubica. */
static int badblock_test(void) {
    ebcfg.powerloss_behavior = LFS_EMUBD_POWERLOSS_NOOP;
    ebcfg.power_cycles       = 0;
    ebcfg.erase_cycles       = 1000;   /* habilita la maquinaria de wear */
    CHK(lfs_emubd_create(&cfg, &ebcfg), "emubd_create");
    for (lfs_block_t b = 8; b <= 12; b++)          /* lejos de superblocks 0/1 */
        CHK(lfs_emubd_setwear(&cfg, b, 1000), "setwear");

    CHK(lfs_format(&lfs, &cfg), "format");
    CHK(lfs_mount(&lfs, &cfg), "mount");
    CHK(lfs_mkdir(&lfs, "/app"), "mkdir");
    if (write_file("/app/big.bin", byte_big, LEN_BIG) < 0) return -1;  /* 6 bloques */
    if (file_state("/app/big.bin", byte_big, LEN_BIG) != 1) {
        printf("FAIL badblock: big.bin no es byte-identico\n");
        return -1;
    }
    if (write_file("/app/ok.txt", byte_a, 100) < 0) return -1;
    if (file_state("/app/ok.txt", byte_a, 100) != 1) {
        printf("FAIL badblock: ok.txt no es byte-identico\n");
        return -1;
    }
    CHK(lfs_unmount(&lfs), "unmount");
    lfs_emubd_destroy(&cfg);
    ebcfg.erase_cycles = 0;
    printf("badblock OK: 5 bloques muertos (PROGERROR) esquivados — %u+100 bytes "
           "byte-identicos tras reubicar\n", LEN_BIG);
    return 0;
}

int main(void) {
    if (powerloss_sweep(LFS_EMUBD_POWERLOSS_NOOP, "NOOP") < 0) return 1;
    if (powerloss_sweep(LFS_EMUBD_POWERLOSS_OOO,  "OOO")  < 0) return 1;
    if (badblock_test() < 0) return 1;
    printf("littlefs fault-injection OK (cortes exhaustivos NOOP+OOO + bloques malos)\n");
    return 0;
}
