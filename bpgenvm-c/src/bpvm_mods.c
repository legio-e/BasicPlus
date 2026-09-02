/*
 * bpvm_mods.c — la regla del /lib de la imagen (#466). Ver bpvm_mods.h.
 */
#include "bpvm_mods.h"
#include "bpvm_fs.h"
#include "crc32.h"
#include <stdio.h>
#include <string.h>

#define DI(...) do { if (linea && cap) snprintf(linea, cap, __VA_ARGS__); } while (0)

int bpvm_mods_version(const uint8_t* m) {
    if (!m || m[0] != 'M' || m[1] != 'O' || m[2] != 'D') return 0;
    return (m[3] >= '0' && m[3] <= '9') ? (int) (m[3] - '0') : 0;
}

bpvm_mods_res_t bpvm_mods_sincronizar(const char* path, const uint8_t* data, uint32_t len,
                                      bpvm_mods_put_fn put, char* linea, size_t cap)
{
    if (linea && cap) linea[0] = '\0';
    if (!path || !data || !put) return BPVM_MODS_FALLO;

    uint32_t sz = 0;
    if (bpvm_fs_stat(path, &sz) != 0) {                 /* falta → se instala */
        if (put(path, data, len) != 0) {
            DI("lib: %s NO se pudo instalar (%u bytes)", path, (unsigned) len);
            return BPVM_MODS_FALLO;
        }
        DI("preinstall: %s (%u bytes)", path, (unsigned) len);
        return BPVM_MODS_INSTALADO;
    }
    /* Fuera de /lib es del usuario (el Hello.mod de muestra): sólo si falta. */
    if (strncmp(path, "/lib/", 5) != 0) return BPVM_MODS_SE_DEJA;

    /* La VERSIÓN: 4 bytes, sin cargar el fichero. */
    uint8_t m[4] = { 0, 0, 0, 0 };
    int v_fs  = (bpvm_fs_read_at(path, 0, m, 4) == 4) ? bpvm_mods_version(m) : 0;
    int v_emb = (len >= 4) ? bpvm_mods_version(data) : 0;

    const char* motivo = NULL;
    if (v_fs == 0)            motivo = "no es un módulo";
    else if (v_fs < v_emb)    motivo = "versión anterior";
    else if (v_fs > v_emb) {
        DI("lib: %s es MÁS NUEVO que el embebido (MOD%d > MOD%d): se deja", path, v_fs, v_emb);
        return BPVM_MODS_SE_DEJA;
    } else if (sz != len)     motivo = "misma versión, tamaño distinto";   /* gratis */
    else {                                                              /* CRC sólo si empatan */
        uint32_t c_fs = 0;
        if (bpvm_fs_crc32(path, &c_fs) != 0)        motivo = "no se pudo leer";
        else if (c_fs != bpvm_crc32(data, len))     motivo = "misma versión, CRC distinto";
    }
    if (!motivo) return BPVM_MODS_IGUAL;

    if (put(path, data, len) != 0) {
        DI("lib: %s NO se pudo reponer (%s)", path, motivo);
        return BPVM_MODS_FALLO;
    }
    DI("lib: %s repuesto: %s (MOD%d %u B -> MOD%d %u B)",
       path, motivo, v_fs, (unsigned) sz, v_emb, (unsigned) len);
    return BPVM_MODS_REPUESTO;
}
