/*
 * bpvm_mods.c — la regla del /lib de la imagen (#466). Ver bpvm_mods.h.
 */
#include "bpvm_mods.h"
#include "bpvm_fs.h"
#include "bpvm_entry.h"   /* V7/H6 (#468): bpvm_entry_zona — la búsqueda en la zona del resolvedor */
#include "bpvm_pack.h"    /* BPVM_PACK_NAME_LEN */
#include "crc32.h"
#include <stdio.h>
#include <string.h>

#define DI(...) do { if (linea && cap) snprintf(linea, cap, __VA_ARGS__); } while (0)

int bpvm_mods_version(const uint8_t* m) {
    if (!m || m[0] != 'M' || m[1] != 'O' || m[2] != 'D') return 0;
    return (m[3] >= '0' && m[3] <= '9') ? (int) (m[3] - '0') : 0;
}

/* V7/H2 (#523) — la GENERACIÓN del módulo: desde MOD8 la cabecera lleva `gen` (be32) en
 * el décimo entero (offset 36), detrás de los nueve de siempre; MOD9 la conserva ahí.
 * Antes, 0 (no hay dato). `len` es lo que se tiene leído de la cabecera. */
uint32_t bpvm_mods_gen(const uint8_t* m, uint32_t len) {
    if (bpvm_mods_version(m) < 8 || len < BPVM_MODS_CABECERA) return 0;
    return ((uint32_t) m[36] << 24) | ((uint32_t) m[37] << 16) | ((uint32_t) m[38] << 8) | (uint32_t) m[39];
}

bpvm_mods_res_t bpvm_mods_sincronizar(const char* path, const uint8_t* data, uint32_t len,
                                      bpvm_mods_put_fn put, char* linea, size_t cap)
{
    if (linea && cap) linea[0] = '\0';
    if (!path || !data || !put) return BPVM_MODS_FALLO;

    /* V7/H6 (#468) — LA ZONA PRIMERO. Si un pack grabado sirve este módulo con los
     * MISMOS bytes que la imagen, la copia de /lib sobra: /lib va delante de la zona
     * en la resolución, así que esa copia idéntica TAPABA al pack y lo cargaba en RAM
     * en vez de en sitio (XIP) — el `Stdlib.pack` no ahorraba un byte. La imagen sigue
     * siendo la fuente: si el pack se borra, esta misma regla lo repone (en el acto,
     * `bpvm_mods_resync`, o en el siguiente arranque). */
    const uint8_t* zdat = NULL; uint32_t zlen = 0; char zpk[BPVM_PACK_NAME_LEN + 1];
    int en_zona = strncmp(path, "/lib/", 5) == 0
               && bpvm_entry_zona(path + 5, &zdat, &zlen, zpk) == 0
               && zlen == len && memcmp(zdat, data, len) == 0;

    uint32_t sz = 0;
    if (bpvm_fs_stat(path, &sz) != 0) {                 /* falta → se instala */
        if (en_zona) return BPVM_MODS_EN_ZONA;          /* ...salvo que la zona ya lo sirva igual: en silencio */
        if (put(path, data, len) != 0) {
            DI("lib: %s NO se pudo instalar (%u bytes)", path, (unsigned) len);
            return BPVM_MODS_FALLO;
        }
        DI("preinstall: %s (%u bytes)", path, (unsigned) len);
        return BPVM_MODS_INSTALADO;
    }
    /* Fuera de /lib es del usuario (el Hello.mod de muestra): sólo si falta. */
    if (strncmp(path, "/lib/", 5) != 0) return BPVM_MODS_SE_DEJA;

    /* La VERSIÓN (4 bytes) y, desde MOD8, la GENERACIÓN (offset 36): sin cargar el fichero. */
    uint8_t m[BPVM_MODS_CABECERA]; memset(m, 0, sizeof m);
    int leidos = bpvm_fs_read_at(path, 0, m, BPVM_MODS_CABECERA);
    int v_fs  = (leidos >= 4) ? bpvm_mods_version(m) : 0;
    int v_emb = (len >= 4) ? bpvm_mods_version(data) : 0;
    uint32_t g_fs  = (leidos > 0) ? bpvm_mods_gen(m, (uint32_t) leidos) : 0;
    uint32_t g_emb = bpvm_mods_gen(data, len);

    const char* motivo = NULL;
    if (v_fs == 0)            motivo = "no es un módulo";
    else if (v_fs < v_emb)    motivo = "versión anterior";
    else if (v_fs > v_emb) {
        DI("lib: %s es MÁS NUEVO que el embebido (MOD%d > MOD%d): se deja%s", path, v_fs, v_emb,
           en_zona ? " (y tapa al del pack)" : "");
        return BPVM_MODS_SE_DEJA;
    } else if (g_fs > g_emb) {
        /* V7/H2 (#523) — mismo formato y una generación MÁS NUEVA en /lib (la subió el
         * IDE con la app que la necesita): pisarla con la embebida dejaría la app sin
         * enlazar en el siguiente RUN («'Core' (gen 8) es más antiguo que 'App' (gen 9)»).
         * Comparar sólo el MAGIC era eso. */
        DI("lib: %s es MÁS NUEVO que el embebido (MOD%d gen %u > gen %u): se deja%s", path, v_fs,
           (unsigned) g_fs, (unsigned) g_emb, en_zona ? " (y tapa al del pack)" : "");
        return BPVM_MODS_SE_DEJA;
    } else if (g_fs < g_emb)  motivo = "generación anterior";
    else if (sz != len)       motivo = "misma versión, tamaño distinto";   /* gratis */
    else {                                                              /* CRC sólo si empatan */
        uint32_t c_fs = 0;
        if (bpvm_fs_crc32(path, &c_fs) != 0)        motivo = "no se pudo leer";
        else if (c_fs != bpvm_crc32(data, len))     motivo = "misma versión, CRC distinto";
    }

    /* #468 — la zona sirve exactamente lo que la imagen pondría aquí: donde la regla
     * dejaría la copia igual o la repondría, se QUITA. Una más nueva ya salió arriba. */
    if (en_zona) {
        if (bpvm_fs_remove(path) != 0) {
            DI("lib: %s NO se pudo quitar (lo sirve el pack '%s')", path, zpk);
            return BPVM_MODS_FALLO;
        }
        DI("lib: %s fuera: lo sirve el pack '%s' con los mismos bytes que la imagen (en sitio, XIP)", path, zpk);
        return BPVM_MODS_QUITADO;
    }
    if (!motivo) return BPVM_MODS_IGUAL;

    if (put(path, data, len) != 0) {
        DI("lib: %s NO se pudo reponer (%s)", path, motivo);
        return BPVM_MODS_FALLO;
    }
    if (g_fs != g_emb)
        DI("lib: %s repuesto: %s (MOD%d gen %u -> MOD%d gen %u)",
           path, motivo, v_fs, (unsigned) g_fs, v_emb, (unsigned) g_emb);
    else
        DI("lib: %s repuesto: %s (MOD%d %u B -> MOD%d %u B)",
           path, motivo, v_fs, (unsigned) sz, v_emb, (unsigned) len);
    return BPVM_MODS_REPUESTO;
}

unsigned bpvm_mods_instalar_tabla(const bpvm_mod_embebido_t* t, unsigned n,
                                  bpvm_mods_put_fn put, bpvm_mods_log_fn log)
{
    unsigned escritos = 0;
    for (unsigned i = 0; i < n; i++) {
        char linea[160];
        bpvm_mods_res_t r = bpvm_mods_sincronizar(t[i].path, t[i].data, t[i].len,
                                                  put, linea, sizeof linea);
        if (r == BPVM_MODS_INSTALADO || r == BPVM_MODS_REPUESTO || r == BPVM_MODS_QUITADO) escritos++;
        if (log && linea[0]) log(linea);
    }
    return escritos;
}

/* V7/H6 (#468) — la instalación de la familia, para volver a correrla cuando cambia la
 * zona (bpvm_mods.h). */
static void (*s_resync)(void) = NULL;

void bpvm_mods_set_resync(void (*instalar)(void)) { s_resync = instalar; }

void bpvm_mods_resync(void) { if (s_resync) s_resync(); }
