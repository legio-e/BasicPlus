/*
 * fs_host.c — backend de file I/O sobre libc, para la VM C en HOST.
 *
 * HOST-ONLY: no se linka en el firmware (el device registra su propio backend
 * sobre fs_get/fs_put). Lo registra test/main.c con bpvm_fs_register_host().
 * Sin sandbox: el path se usa tal cual contra el FS del host (paridad con la
 * VM-Java cuando no hay workdir configurado).
 */
#include "bpvm_fs.h"
#include <stdio.h>

static int host_stat(const char* path, uint32_t* size) {
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    long sz = ftell(f);
    fclose(f);
    if (sz < 0) return -1;
    if (size) *size = (uint32_t) sz;
    return 0;
}

static long host_read(const char* path, uint8_t* dst, uint32_t cap) {
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    size_t n = fread(dst, 1, (size_t) cap, f);
    fclose(f);
    return (long) n;
}

static int host_write(const char* path, const uint8_t* data, uint32_t len, int append) {
    FILE* f = fopen(path, append ? "ab" : "wb");
    if (!f) return -1;
    if (len > 0 && data) {
        if (fwrite(data, 1, (size_t) len, f) != (size_t) len) { fclose(f); return -1; }
    }
    fclose(f);
    return 0;
}

static const bpvm_fs_backend_t s_host_fs = {
    .stat  = host_stat,
    .read  = host_read,
    .write = host_write,
};

void bpvm_fs_register_host(void) {
    bpvm_fs_set_backend(&s_host_fs);
}
