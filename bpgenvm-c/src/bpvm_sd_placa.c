/*
 * bpvm_sd_placa.c — V7/H3 (24-sep): la SD de la placa, en el común. Ver `bpvm_sd_placa.h`.
 *
 * Los dos verbos vienen de `pico/repl_v1.c` (V5/H1 `SD_INFO` y V5/H2 `SD_MOUNT`), donde se escribieron para
 * el SPI de la Metro: el mismo orden, los mismos campos y los mismos textos, pero hablando con el DISPOSITIVO
 * registrado —su `init`, su `identidad`, su `leer`, su `motivo`— y no con el protocolo SPI. Lo que el
 * explorador del IDE lee de cada respuesta no cambia; se añade `medio` («SPI», «SDMMC»…), que no rompe nada.
 */
#include "bpvm_sd_placa.h"
#include "bpvm_wire_v1.h"
#include "bpvm_log.h"
#include "bpvm_platform.h"

#include <stdio.h>
#include <string.h>

#define PREFIJO         "/sd"
#define VIGILA_CADA_MS  500     /* la escala de un dedo metiendo una tarjeta, no la del micro */

static const bpvm_blk_backend_t* s_dev   = NULL;
static const bpvm_sd_placa_fs_t* s_fs    = NULL;
static const char*               s_medio = "";
static char s_motivo[112] = "esta placa no tiene SD: no registra ninguna (su ENV no lleva linea `sd`)";

/* El búfer de las respuestas: el tamaño del de la Metro, donde nacieron (el SD_INFO más largo ronda 500 B). */
static char s_buf[1024];

void bpvm_sd_placa_registrar(const bpvm_blk_backend_t* dev, const bpvm_sd_placa_fs_t* fs,
                             const char* medio) {
    s_dev   = dev;
    s_fs    = fs;
    s_medio = medio ? medio : "";
    if (!dev || !fs) bpvm_sd_placa_sin_sd("registro incompleto: falta el dispositivo o su FS");
}

void bpvm_sd_placa_sin_sd(const char* motivo) {
    s_dev = NULL;
    s_fs  = NULL;
    snprintf(s_motivo, sizeof s_motivo, "%s", motivo && motivo[0] ? motivo : "sin SD");
}

int bpvm_sd_placa_hay(void) { return s_dev != NULL && s_fs != NULL; }

/* ── El arranque y la tarjeta en caliente ──────────────────────────────────────────────────────────── */

void bpvm_sd_placa_montar_arranque(void) {
    if (!bpvm_sd_placa_hay()) {
        log_printf("sd: %s — no se monta", s_motivo);
        return;
    }
    /* EL PIN DE DETECCIÓN MANDA (Eduardo, 8-ago): sin tarjeta no se toca ni el bus. Un medio sin detector
     * contesta 1 («no podemos saberlo»), y entonces se intenta. */
    if (s_dev->hay_medio && !s_dev->hay_medio()) {
        log_printf("sd: zocalo VACIO (pin de deteccion) — no se monta");
        return;
    }
    /* Un fallo NO degrada el arranque: se anota el motivo y se sigue. Degradar el boot por un accesorio
     * dejaría al usuario sin IDE para averiguar qué pasa, justo cuando más falta hace. */
    char motivo[80];
    if (s_fs->montar(s_dev, PREFIJO, motivo, sizeof motivo) == 0) {
        log_printf("sd: montada en %s por %s (particion en el bloque %u)", PREFIJO, s_medio,
                   (unsigned) (s_fs->lba_particion ? s_fs->lba_particion() : 0u));
    } else {
        log_printf("sd: NO montada — %s", motivo);
    }
}

void bpvm_sd_placa_vigilar(void) {
    if (!bpvm_sd_placa_hay() || !s_fs->vigilar) return;
    static int64_t s_proxima = 0;
    int64_t ahora = bpvm_platform_now_ms();
    if (ahora < s_proxima) return;
    s_proxima = ahora + VIGILA_CADA_MS;

    char motivo[80];
    motivo[0] = '\0';
    if (s_fs->vigilar(s_dev, PREFIJO, motivo, sizeof motivo)) {
        /* Sólo cuando CAMBIA algo: un log que repite «sigue igual» dos veces por segundo no lo lee nadie. */
        log_printf("sd: %s", motivo[0] ? motivo : "cambio en el zocalo");
        log_flush();
    }
}

/* ── Los dos verbos ───────────────────────────────────────────────────────────────────────────────── */

static void envia(long id, int off, const char* tipo) {
    if (off >= 0) off = wire_v1_msg_end(s_buf, sizeof s_buf, (size_t) off);
    if (off < 0) {
        char m[48];
        snprintf(m, sizeof m, "%s no cabe", tipo);
        wire_v1_send_error(id, "INTERNAL_ERROR", m);
        return;
    }
    wire_v1_send_line(s_buf, (size_t) off);
}

#define CAMPO_B(k, v) do { if (off >= 0) off = wire_v1_field_bool  (s_buf, sizeof s_buf, (size_t) off, (k), (v)); } while (0)
#define CAMPO_S(k, v) do { if (off >= 0) off = wire_v1_field_string(s_buf, sizeof s_buf, (size_t) off, (k), (v)); } while (0)
#define CAMPO_L(k, v) do { if (off >= 0) off = wire_v1_field_long  (s_buf, sizeof s_buf, (size_t) off, (k), (long) (v)); } while (0)
/* #550 — lo que es un uint32 (la serie, los bloques, las LBA, los KB) va SIN signo: `long` es de 32 bits en las
 * placas y en MinGW, y (long) de un PSN ≥ 2^31 salía NEGATIVO (la SanDisk de la Metro: -38726973). Lo mismo
 * que el CRC de #466. */
#define CAMPO_U(k, v) do { if (off >= 0) off = wire_v1_field_ulong (s_buf, sizeof s_buf, (size_t) off, (k), (unsigned long) (v)); } while (0)

static void hexa(const uint8_t* b, int n, char* out) {
    static const char* D = "0123456789ABCDEF";
    int k = 0;
    for (int i = 0; i < n; i++) {
        if (i) out[k++] = ' ';
        out[k++] = D[(b[i] >> 4) & 0xF];
        out[k++] = D[b[i] & 0xF];
    }
    out[k] = '\0';
}

/* Los bytes 3..10 de un sector de arranque son el nombre del formateador ("MSDOS5.0", "mkfs.fat"…); en una
 * tabla de particiones son código o ceros. Devuelve 1 si son ASCII imprimible. */
static int nombre_formateador(const uint8_t* sec, char out[9]) {
    int ascii = 1;
    for (int i = 0; i < 8; i++) {
        uint8_t c = sec[3 + i];
        out[i] = (c >= 32 && c < 127) ? (char) c : '.';
        if (c < 32 || c >= 127) ascii = 0;
    }
    out[8] = '\0';
    return ascii;
}

static uint32_t le32(const uint8_t* p) {
    return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24);
}

/* SD_INFO — el DIAGNÓSTICO: arranca la tarjeta y dice qué es, sin montar nada (un diagnóstico que cambia el
 * estado deja de servir para diagnosticar). */
void bpvm_sd_placa_sd_info(long id) {
    int off = wire_v1_msg_begin(s_buf, sizeof s_buf, 0, "SD_INFO_REPLY", id);
    if (!bpvm_sd_placa_hay()) {
        CAMPO_B("ok", 0);
        CAMPO_S("motivo", s_motivo);
        envia(id, off, "SD_INFO_REPLY");
        return;
    }

    int r = s_dev->init ? s_dev->init() : -1;
    bpvm_blk_identidad_t idn;
    memset(&idn, 0, sizeof idn);
    int hay_idn = s_dev->identidad && s_dev->identidad(&idn) == 0;
    const char* mot = s_dev->motivo ? s_dev->motivo() : (r == 0 ? "OK" : "fallo sin motivo");
    log_printf("sd: SD_INFO por %s -> %s", s_medio, mot);

    CAMPO_B("ok", r == 0);
    CAMPO_S("motivo", mot);
    CAMPO_S("medio", s_medio);
    CAMPO_L("peldano", hay_idn ? idn.peldano : (r == 0 ? 0 : -1));
    /* LA TRAZA: los bytes que la tarjeta mandó de verdad esperando su respuesta. La diferencia entre leer
     * 0x00 (línea flotante) y 0x05 (comando ilegal) manda a sitios opuestos. Sólo si el medio la tiene. */
    {
        char hex[3 * 8 + 1];
        hex[0] = '\0';
        if (hay_idn && idn.con_traza) hexa(idn.traza, 8, hex);
        CAMPO_L("ultimoCmd", hay_idn && idn.con_traza ? idn.ultimo_cmd : 0);
        CAMPO_S("traza", hex);
    }
    if (r == 0) {
        CAMPO_L("version", idn.version);
        CAMPO_B("altaCap", idn.alta_cap);
        CAMPO_U("bloques", hay_idn && idn.bloques ? idn.bloques : (s_dev->bloques ? s_dev->bloques() : 0u));
        CAMPO_S("producto", idn.producto);
        CAMPO_S("oem", idn.oem);
        CAMPO_L("fabricante", idn.fabricante);
        CAMPO_U("serie", idn.serie);
        CAMPO_L("anno", idn.anno);
        CAMPO_L("mes", idn.mes);

        /* EL SECTOR 0 — que la tarjeta se identifique prueba que responde a COMANDOS; esto prueba que
         * entrega DATOS, que es otro camino. Y de regalo, el formato. Estático: 512 B en la pila de la tarea
         * del wire no caben. */
        static uint8_t sec0[BPVM_BLK_TAM];
        int rl = s_dev->leer ? s_dev->leer(0, 1, sec0) : -1;
        CAMPO_B("leeSector0", rl == 0);
        if (rl == 0) {
            char firma[6], oemfs[9];
            hexa(sec0 + 510, 2, firma);
            CAMPO_S("firma", firma);
            /* ¿MBR o VBR? Decide DÓNDE está el sistema de ficheros: con MBR, dentro de una partición. */
            int ascii = nombre_formateador(sec0, oemfs);
            CAMPO_S("clase", ascii ? "VBR" : "MBR");
            if (ascii) {
                CAMPO_S("oemFs", oemfs);
            } else {
                /* La tabla: 4 entradas de 16 B desde el 446; de cada una, el TIPO y dónde empieza. */
                int cuantas = 0;
                uint32_t lba1 = 0, secs1 = 0;
                uint8_t tipo1 = 0;
                for (int i = 0; i < 4; i++) {
                    const uint8_t* e = sec0 + 446 + i * 16;
                    if (e[4] == 0) continue;
                    if (cuantas == 0) { tipo1 = e[4]; lba1 = le32(e + 8); secs1 = le32(e + 12); }
                    cuantas++;
                }
                CAMPO_L("particiones", cuantas);
                CAMPO_L("parteTipo", tipo1);
                CAMPO_U("parteLba", lba1);
                CAMPO_U("parteSectores", secs1);
                /* Y el sector de arranque de esa partición: ahí están el formateador y su propio 55 AA. */
                if (cuantas > 0 && s_dev->leer(lba1, 1, sec0) == 0) {
                    (void) nombre_formateador(sec0, oemfs);
                    CAMPO_S("oemFs", oemfs);
                }
            }
        } else {
            CAMPO_S("motivoSector0", s_dev->motivo ? s_dev->motivo() : "no se pudo leer");
        }
    }
    envia(id, off, "SD_INFO_REPLY");
}

/* SD_MOUNT — montar a mano (el arranque ya monta solo si hay tarjeta: esto es para después de meterla, o
 * para volver a mirar). VERBO APARTE de SD_INFO a propósito: el diagnóstico no cambia el estado. */
void bpvm_sd_placa_sd_mount(long id) {
    int off = wire_v1_msg_begin(s_buf, sizeof s_buf, 0, "SD_MOUNT_REPLY", id);
    if (!bpvm_sd_placa_hay()) {
        CAMPO_B("ok", 0);
        CAMPO_S("motivo", s_motivo);
        envia(id, off, "SD_MOUNT_REPLY");
        return;
    }
    static char motivo[80];
    motivo[0] = '\0';
    int r = s_fs->montar(s_dev, PREFIJO, motivo, sizeof motivo);
    log_printf("sd: montar por %s -> %s%s%s", s_medio, r == 0 ? "OK" : "FALLO",
               r == 0 ? "" : " — ", r == 0 ? "" : motivo);

    CAMPO_B("ok", r == 0);
    CAMPO_S("motivo", r == 0 ? "montada" : motivo);
    CAMPO_S("prefijo", PREFIJO);
    CAMPO_S("medio", s_medio);
    /* El LBA SIEMPRE, también al fallar: un montaje que se cae con LBA 0 en una tarjeta que trae MBR es un
     * fallo al leer la tabla, no en el FAT — dos sitios distintos. */
    CAMPO_U("lba", s_fs->lba_particion ? s_fs->lba_particion() : 0u);
    if (r == 0) {
        bpvm_fs_fat_resumen_t res;
        if (s_fs->resumen && s_fs->resumen(&res) == 0) {
            log_printf("sd: raiz %d entradas, 1a='%s'", res.entradas_raiz, res.primera);
            CAMPO_S("etiqueta", res.etiqueta);
            CAMPO_U("kbTotal", res.kb_total);
            CAMPO_U("kbLibres", res.kb_libres);
            CAMPO_L("entradasRaiz", res.entradas_raiz);
            CAMPO_S("primera", res.primera);
        } else {
            /* Montada pero no recorrible: eso ES un hallazgo, no un detalle. */
            log_printf("sd: montada pero el resumen FALLA");
            CAMPO_B("resumenFalla", 1);
        }
    }
    envia(id, off, "SD_MOUNT_REPLY");
}
