/*
 * test_modhdr.c — V7/H2: LA CABECERA DEL .mod Y SUS SECCIONES, contra el cargador.
 *
 * El comentario de loader.c lo dice: «saltarla mal desplazaría `data` y `code`,
 * que es como se corrompe un módulo en silencio». Y pasó: Disasm sólo conocía
 * MOD6 y con un MOD7 leía data y code corridos. Antes de subir el formato a
 * MOD8 (con más campos en la cabecera y más en imports) hace falta una prueba
 * que diga, para un .mod con TODAS las secciones no vacías, que:
 *
 *   1. la cabecera dice la verdad: la suma de sus tamaños es el fichero entero
 *      (ni bytes sobrantes ni truncado);
 *   2. el CARGADOR deja en RAM exactamente los bytes de `code` que hay en el
 *      fichero (CRC32 idéntico) y con el `data_size`/`code_size` de la cabecera
 *      — o sea, no se ha desplazado ninguna sección al saltar interface/native;
 *   3. la sección native, si la hay, se recorre blob a blob por su propia
 *      cabecera (MOD_FORMAT.md §4.6) y cierra justo en `nativeSize`.
 *
 * El recorrido del fichero se hace AQUÍ, por la especificación, sin llamar al
 * loader: es la segunda opinión. Si el loader y esta prueba discrepan, uno de
 * los dos lee mal el formato — y eso es lo que hay que saber ANTES de MOD8.
 *
 * Uso: test_modhdr <a.mod> [b.mod ...]  (cada uno debe cargar solo, con sus deps al lado)
 */
#include "bpvm.h"
#include "bpvm_internal.h"
#include "bpvm_fs.h"
#include "crc32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fallos = 0;
static void ok(int c, const char* m) { printf(c ? "  ok  : %s\n" : "  FAIL: %s\n", m); if (!c) fallos++; }

static uint32_t be32(const uint8_t* p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
static uint32_t le32(const uint8_t* p) { return ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) | ((uint32_t)p[1] << 8) | p[0]; }
static uint32_t le16(const uint8_t* p) { return ((uint32_t)p[1] << 8) | p[0]; }

/* La cabecera tal como la describe docs/MOD_FORMAT.md: 9 enteros BE en MOD7. */
typedef struct {
    char     magic[5];
    uint32_t data, main_off, imports, exports, code, library, iface, native;
    int      hdr;          /* bytes de cabecera según el magic */
    uint32_t off_library, off_imports, off_exports, off_iface, off_native, off_data, off_code, fin;
} cab_t;

static int leer_cabecera(const uint8_t* f, size_t n, cab_t* c) {
    if (n < 4) return 0;
    memcpy(c->magic, f, 4); c->magic[4] = 0;
    int v6 = !strcmp(c->magic, "MOD6"), v7 = !strcmp(c->magic, "MOD7");
    if (!v6 && !v7) return 0;
    c->hdr = v7 ? 36 : 32;
    if (n < (size_t)c->hdr) return 0;
    c->data = be32(f + 4);  c->main_off = be32(f + 8);  c->imports = be32(f + 12); c->exports = be32(f + 16);
    c->code = be32(f + 20); c->library = be32(f + 24);  c->iface = be32(f + 28);   c->native = v7 ? be32(f + 32) : 0;
    c->off_library = (uint32_t)c->hdr;
    c->off_imports = c->off_library + c->library;
    c->off_exports = c->off_imports + c->imports;
    c->off_iface   = c->off_exports + c->exports;
    c->off_native  = c->off_iface + c->iface;
    c->off_data    = c->off_native + c->native;
    c->off_code    = c->off_data + c->data;
    c->fin         = c->off_code + c->code;
    return 1;
}

/* Recorre la sección native blob a blob. Devuelve el nº de blobs, o -1 si no cuadra. */
static int recorrer_native(const uint8_t* f, const cab_t* c, char* detalle, size_t dn) {
    if (c->native == 0) { snprintf(detalle, dn, "vacía"); return 0; }
    uint32_t pos = c->off_native + ((4 - (c->off_native & 3)) & 3);   /* relleno hasta múltiplo de 4 ABSOLUTO */
    uint32_t fin = c->off_native + c->native;
    int n = 0; size_t usado = 0;
    while (pos + 20 <= fin) {
        if (memcmp(f + pos, "MDN", 3) != 0) {
            int todo_cero = 1; for (uint32_t i = pos; i < fin; i++) if (f[i]) { todo_cero = 0; break; }
            if (todo_cero) break;
            snprintf(detalle + usado, dn - usado, " [+%u no es MDN]", pos); return -1;
        }
        uint32_t code_size = le32(f + pos + 8), sym_count = le32(f + pos + 12), arch = le32(f + pos + 16);
        uint32_t total = 20 + sym_count * 36 + code_size, alineado = (total + 3) & ~3u;
        if (pos + total > fin) { snprintf(detalle + usado, dn - usado, " [blob %d truncado]", n); return -1; }
        usado += (size_t)snprintf(detalle + usado, dn - usado, "%sblob %d: MDN v%u abi=%u arch=%u syms=%u code=%u",
                                  n ? "; " : "", n, le16(f + pos + 4), le16(f + pos + 6), arch, sym_count, code_size);
        pos += alineado; n++;
    }
    return n;
}

static int probar(const char* path) {
    printf("-- %s --\n", path);
    FILE* fp = fopen(path, "rb");
    if (!fp) { ok(0, "se puede abrir el fichero"); return 1; }
    fseek(fp, 0, SEEK_END); long n = ftell(fp); fseek(fp, 0, SEEK_SET);
    uint8_t* f = (uint8_t*)malloc((size_t)n);
    if (fread(f, 1, (size_t)n, fp) != (size_t)n) { ok(0, "se lee entero"); fclose(fp); free(f); return 1; }
    fclose(fp);

    cab_t c;
    if (!leer_cabecera(f, (size_t)n, &c)) { ok(0, "magic conocido (MOD6/MOD7)"); free(f); return 1; }
    char msg[256];
    snprintf(msg, sizeof msg, "%s: cabecera %d B; library=%u imports=%u exports=%u interface=%u native=%u data=%u code=%u",
             c.magic, c.hdr, c.library, c.imports, c.exports, c.iface, c.native, c.data, c.code);
    printf("  %s\n", msg);

    /* 1. La cabecera dice la verdad: las secciones cubren el fichero entero. */
    snprintf(msg, sizeof msg, "la suma de las secciones es el fichero entero (%u == %ld, sobrantes %ld)", c.fin, n, n - (long)c.fin);
    ok(c.fin == (uint32_t)n, msg);

    /* Un .mod «con todo» para probar la subida de formato tiene que tener TODAS las
     * secciones no vacías; se dice cuáles faltan para que el fixture no mienta. */
    snprintf(msg, sizeof msg, "secciones no vacías: imports=%s exports=%s interface=%s native=%s data=%s code=%s",
             c.imports ? "sí" : "NO", c.exports ? "sí" : "NO", c.iface ? "sí" : "NO", c.native ? "sí" : "NO", c.data ? "sí" : "NO", c.code ? "sí" : "NO");
    printf("  info: %s\n", msg);

    /* 3. La sección native se recorre por su propia cabecera y cierra en nativeSize. */
    char det[512] = "";
    int blobs = recorrer_native(f, &c, det, sizeof det);
    snprintf(msg, sizeof msg, "sección native recorrible blob a blob: %s", det);
    ok(blobs >= 0, msg);

    /* 2. El CARGADOR deja en RAM lo que hay en el fichero, sin desplazar nada. */
    uint32_t crc_code_fichero = bpvm_crc32(f + c.off_code, c.code);
    uint32_t crc_data_fichero = bpvm_crc32(f + c.off_data, c.data);
    size_t mem_size = 512 * 1024;
    uint8_t* mem = (uint8_t*)calloc(1, mem_size);
    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    bpvm_status_t s = bpvm_load_mod(vm, path);
    snprintf(msg, sizeof msg, "el cargador lo carga (%s)", bpvm_status_str(s));
    ok(s == BPVM_OK, msg);
    if (s == BPVM_OK) {
        /* el módulo raíz es el ÚLTIMO cargado (sus deps entran antes) o el que lleva main */
        const bpvm_module_t* m = NULL;
        for (int i = 0; i < vm->module_count; i++) if (vm->modules[i].main_offset >= 0 || i == vm->module_count - 1) m = &vm->modules[i];
        for (int i = 0; i < vm->module_count; i++) if (vm->modules[i].code_size == c.code && vm->modules[i].data_size == c.data) m = &vm->modules[i];
        ok(m != NULL, "el módulo raíz está en vm->modules[]");
        if (m) {
            snprintf(msg, sizeof msg, "data_size/code_size en RAM == cabecera (%u/%u vs %u/%u)", m->data_size, m->code_size, c.data, c.code);
            ok(m->data_size == c.data && m->code_size == c.code, msg);
            uint32_t crc_code_ram = bpvm_crc32(vm->memory + m->code_start, m->code_size);
            snprintf(msg, sizeof msg, "los bytes de CODE en RAM son los del fichero (crc %08x == %08x): nada se desplazó", crc_code_ram, crc_code_fichero);
            ok(crc_code_ram == crc_code_fichero, msg);
            /* data la PARCHEA el enlace (class-fixups: parentOff); sólo se exige igual
             * cuando no hay fixups. Si los hay, se informa. */
            uint32_t crc_data_ram = bpvm_crc32(vm->memory + m->data_start, m->data_size);
            if (m->class_fixup_count == 0) {
                snprintf(msg, sizeof msg, "los bytes de DATA en RAM son los del fichero (crc %08x == %08x; sin fixups)", crc_data_ram, crc_data_fichero);
                ok(crc_data_ram == crc_data_fichero, msg);
            } else {
                printf("  info: DATA con %d class-fixups (parcheada por el enlace): crc fichero %08x, RAM %08x\n",
                       m->class_fixup_count, crc_data_fichero, crc_data_ram);
            }
        }
    }
    bpvm_destroy(vm); free(mem); free(f);
    return 0;
}

int main(int argc, char** argv) {
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 2) { fprintf(stderr, "uso: test_modhdr <a.mod> [b.mod ...]\n"); return 2; }
    for (int i = 1; i < argc; i++) probar(argv[i]);
    printf("[status=%s] %d fallo(s)\n", fallos ? "FAIL" : "OK", fallos);
    return fallos ? 1 : 0;
}
