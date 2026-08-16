/*
 * test/main.c — CLI para la VM C. Equivalente del bpgenvm de Java a
 * efectos de smoke-testing.
 *
 * Uso:
 *   bpgenvm-c <fichero.mod>            ejecuta el módulo
 *   bpgenvm-c --trace <fichero.mod>    activa trace per-instrucción
 *   bpgenvm-c --mem=N <fichero.mod>    memorySize en bytes (default 512 KiB)
 *   bpgenvm-c --fs=lfs:<img> <f.mod>   H2·B1.2: FS littlefs sobre la imagen
 *                                       <img> (modo ORÁCULO, mismo motor que
 *                                       el micro; default = FS del host/libc)
 *   bpgenvm-c --debug-trace[=N] <m>    #139: instala un debug hook que
 *                                       cuenta cambios de línea (modo
 *                                       sintético: pc_to_line=NULL, así
 *                                       cada opcode es una "línea"). Sin
 *                                       N imprime sólo el total; con N
 *                                       imprime los primeros N hits.
 *   bpgenvm-c --pack=<f.pack> <m>      H3: "graba" la imagen en una región RAM
 *                                       que simula la partición de packs de la
 *                                       flash interna del micro (repetible).
 *                                       --pack-del=<nombre> tumba (tombstone)
 *                                       un pack tras el grabado. Imprime el
 *                                       LIST resultante de la cadena.
 */

#include "bpvm.h"
#include "bpvm_fs.h"
#include "bpvm_net.h"   /* H11 — registro del backend TCP del host */
#include "bpvm_pack.h"  /* H3 — zona de packs simulada (--pack=) */
#include "bpvm_entry.h" /* #344 — el RUN, escrito una vez */
#ifdef BPVM_GUI
#include "bpvm_gui.h"   /* H10 — --screen=WxH / --no-screen (micro simulado) */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t parse_size(const char* s) {
    char* end = NULL;
    long v = strtol(s, &end, 0);
    if (v <= 0) return 0;
    return (size_t) v;
}

/* #139 — Smoke hook: incrementa counter, opcionalmente imprime las
 * primeras N invocaciones. Como el modo es sintético (pc_to_line=NULL)
 * cada `line` recibido es el valor de pc, así que vemos la secuencia
 * de PCs ejecutados — equivalente a --trace pero pasando por el
 * camino del hook. */
typedef struct {
    long total_hits;
    long max_print;
    long printed;
} debug_trace_state_t;

/* ── H3: simulación de la zona de packs ─────────────────────────────────── */
#define PACKS_MAX_FILES   8
#define PACKS_MAX_DELS    4
#define PACKS_REGION_SIZE (1024u * 1024u)   /* 1 MB de "flash" de packs */

/* Graba cada --pack= en la región (ADD = append a la cadena), aplica los
 * --pack-del= (tombstone) y deja el LIST en stdout. Devuelve 0 o -1. */
static int packs_setup(uint8_t* region,
                       const char** files, int n_files,
                       const char** dels, int n_dels) {
    memset(region, 0xFF, PACKS_REGION_SIZE);   /* NOR borrada */
    for (int i = 0; i < n_files; i++) {
        FILE* f = fopen(files[i], "rb");
        if (!f) { fprintf(stderr, "--pack: no puedo abrir %s\n", files[i]); return -1; }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint8_t* img = (sz > 0) ? (uint8_t*) malloc((size_t) sz) : NULL;
        if (!img || fread(img, 1, (size_t) sz, f) != (size_t) sz) {
            fprintf(stderr, "--pack: error leyendo %s\n", files[i]);
            free(img); fclose(f); return -1;
        }
        fclose(f);
        int32_t off = bpvm_pack_add(region, PACKS_REGION_SIZE, img, (uint32_t) sz);
        free(img);
        if (off == BPVM_PACK_ERR_BADIMG) {
            fprintf(stderr, "--pack: %s no es un pack valido (magic/CRC/formato)\n", files[i]);
            return -1;
        }
        if (off == BPVM_PACK_ERR_NOSPACE) {
            fprintf(stderr, "--pack: %s no cabe en la region (%u B)\n", files[i], PACKS_REGION_SIZE);
            return -1;
        }
        printf("[packs] + %s grabado en 0x%06X\n", files[i], (unsigned) off);
    }
    for (int i = 0; i < n_dels; i++) {
        int32_t off = bpvm_pack_remove(region, PACKS_REGION_SIZE, dels[i]);
        if (off < 0) {
            fprintf(stderr, "--pack-del: no hay ningun pack activo llamado '%s'\n", dels[i]);
            return -1;
        }
        printf("[packs] - '%s' tombstone en 0x%06X\n", dels[i], (unsigned) off);
    }
    /* LIST de la cadena resultante (lo que veria el micro en su flash). */
    bpvm_pack_info_t inf[PACKS_MAX_FILES];
    uint32_t end = 0;
    int n = bpvm_pack_scan(region, PACKS_REGION_SIZE, inf, PACKS_MAX_FILES, 1, &end);
    int alive = 0;
    for (int i = 0; i < n && i < PACKS_MAX_FILES; i++) {
        printf("[packs]   0x%06X  %-24s v'%s'  %7u B  %2u fich  %s%s\n",
               (unsigned) inf[i].off, inf[i].nombre, inf[i].vercont,
               (unsigned) inf[i].size_total, (unsigned) inf[i].n_entries,
               inf[i].alive ? "activo" : "borrado",
               inf[i].crc_ok ? "" : "  [CRC MAL]");
        if (inf[i].alive) alive++;
    }
    printf("[packs] %d packs (%d activos), libre %u B\n",
           n, alive, (end == BPVM_PACK_NO_SPACE) ? 0u : (unsigned) (PACKS_REGION_SIZE - end));
    return 0;
}

static void debug_trace_hook(bpvm_t* vm, bpvm_thread_t* tc,
                              uint32_t pc, int line, const char* source,
                              void* user) {
    (void) vm; (void) source;
    debug_trace_state_t* st = (debug_trace_state_t*) user;
    st->total_hits++;
    if (st->printed < st->max_print) {
        fprintf(stderr, "[dbg] hit tid=%d pc=%u line=%d\n",
                bpvm_thread_id(tc), pc, line);
        st->printed++;
    }
}

int main(int argc, char** argv) {
    /* stdout unbuffered para que un crash no se trague output. */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    const char* path  = NULL;
    int   trace       = 0;
    int   debug_trace = 0;
    long  debug_print = 0;
    int   smp_workers = 0;        /* 0 = single-worker legacy */
    int   no_gc       = 0;        /* #355: --nogc = el recolector no corre (ver repl_v1) */
    long  handle_cap  = -1;       /* #430: --handlecap=N; -1 = el default del build */
    size_t mem_size   = 512 * 1024;
    const char* basedir = NULL;   /* H19-F1: raíz de proyecto (paths relativos) */
    const char* fs_lfs_img = NULL;/* H2·B1.2: modo ORÁCULO — littlefs sobre imagen */
    const char* pack_files[PACKS_MAX_FILES]; int n_pack_files = 0;  /* H3 --pack= */
    const char* pack_dels[PACKS_MAX_DELS];   int n_pack_dels  = 0;  /* H3 --pack-del= */

    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];
        if (strcmp(a, "--trace") == 0)         { trace = 1; }
        else if (strcmp(a, "--debug-trace") == 0) {
            debug_trace = 1;
        }
        else if (strncmp(a, "--debug-trace=", 14) == 0) {
            debug_trace = 1;
            debug_print = strtol(a + 14, NULL, 10);
            if (debug_print < 0) debug_print = 0;
        }
        else if (strncmp(a, "--mem=", 6) == 0) {
            size_t n = parse_size(a + 6);
            if (n > 0) mem_size = n;
        }
        else if (strncmp(a, "--smp=", 6) == 0) {
            smp_workers = (int) strtol(a + 6, NULL, 10);
            if (smp_workers < 1) smp_workers = 1;
        }
        else if (strcmp(a, "--nogc") == 0) {
            no_gc = 1;   /* #355: espejo en el PC del `gc=0` del ENV de la placa */
        }
        else if (strncmp(a, "--handlecap=", 12) == 0) {
            /* #430: fuerza en host el tope de tabla de un puerto (0 = sin tope).
             * Con el la prueba del OOM-por-tabla corre aqui, sin build especial. */
            handle_cap = strtol(a + 12, NULL, 10);
            if (handle_cap < 0) handle_cap = 0;
        }
        else if (strncmp(a, "--basedir=", 10) == 0) {
            basedir = a + 10;        /* H19-F1: raíz de proyecto (paths relativos) */
        }
        else if (strncmp(a, "--fs=lfs:", 9) == 0) {
            fs_lfs_img = a + 9;      /* H2·B1.2: FS littlefs sobre <imagen> (oráculo) */
        }
        /* H10 — pantalla del micro simulado. Sin GUI en el build, se aceptan y se
         * ignoran: así el IDE puede pasarlas siempre sin saber cómo se compiló. */
        else if (strncmp(a, "--screen=", 9) == 0) {
            int sw = 0, sh = 0;
            if (sscanf(a + 9, "%dx%d", &sw, &sh) == 2 && sw > 0 && sh > 0) {
#ifdef BPVM_GUI
                bpvm_gui_set_screen_size(sw, sh);
#endif
            } else {
                fprintf(stderr, "--screen espera ANCHOxALTO (p.ej. --screen=480x320)\n");
                return 2;
            }
        }
        else if (strcmp(a, "--no-screen") == 0) {
#ifdef BPVM_LVGL
            bpvm_gui_disp_set_headless(1);
#endif
        }
        else if (strncmp(a, "--pack=", 7) == 0) {
            if (n_pack_files >= PACKS_MAX_FILES) {
                fprintf(stderr, "Demasiados --pack (max %d)\n", PACKS_MAX_FILES);
                return 2;
            }
            pack_files[n_pack_files++] = a + 7;   /* H3: pack "ya grabado" en flash */
        }
        else if (strncmp(a, "--pack-del=", 11) == 0) {
            if (n_pack_dels >= PACKS_MAX_DELS) {
                fprintf(stderr, "Demasiados --pack-del (max %d)\n", PACKS_MAX_DELS);
                return 2;
            }
            pack_dels[n_pack_dels++] = a + 11;    /* H3: tombstone tras el grabado */
        }
        else if (a[0] == '-')                  {
            fprintf(stderr, "Argumento desconocido: %s\n", a);
            return 2;
        }
        else if (!path)                        { path = a; }
        else {
            fprintf(stderr, "Sólo se admite un .mod por invocación.\n");
            return 2;
        }
    }
    if (!path) {
        fprintf(stderr, "Uso: bpgenvm-c [--trace] [--mem=N] [--screen=WxH] [--no-screen] <fichero.mod>\n");
        return 1;
    }

    /* H3.c — la región de packs se TALLA a continuación de la memoria de la VM,
     * dentro del mismo buffer: así los offsets virtuales del código XIP
     * (cb = puntero_región − vm->memory) caben en uint32 también en host de
     * 64 bits. En placa la resta envuelve módulo 2^32 y cae en la flash real. */
    size_t alloc_size = mem_size
        + ((n_pack_files > 0 || n_pack_dels > 0) ? (size_t) PACKS_REGION_SIZE : 0);
    uint8_t* mem = (uint8_t*) calloc(1, alloc_size);
    if (!mem) {
        fprintf(stderr, "No se pudo alocar %zu bytes\n", alloc_size);
        return 1;
    }
    /* Diagnóstico: BPVM_POISON=1 rellena el buffer de la VM con 0xAA ANTES del
     * init → simula la SRAM SIN INICIALIZAR de un micro (bare-metal no zeroa el
     * buffer como sí hace calloc en el host). Reproduce en host los bugs que
     * solo salen en placa por leer memoria sin inicializar (gen de handle en
     * pila/local, etc.). Runs normales sin la env → intactos. */
    if (getenv("BPVM_POISON")) {
        memset(mem, 0xAA, mem_size);
        fprintf(stderr, "[BPVM_POISON] buffer de %zu bytes envenenado con 0xAA\n", mem_size);
    }

    bpvm_t* vm = bpvm_init(mem, mem_size, 0);
    if (!vm) {
        fprintf(stderr, "bpvm_init falló (memSize=%zu)\n", mem_size);
        free(mem);
        return 1;
    }
    bpvm_set_tracing(vm, trace);
    /* H2·B1.2 — selección de backend de FS: libc (default, dev-loop de siempre)
     * o littlefs sobre imagen (--fs=lfs:<img> → el ORÁCULO: mismo motor que el
     * micro; formatea la imagen solo si no monta = primer arranque). */
    /* #344 — el ORDEN importa desde que el núcleo carga por la fachada: el
     * .mod que se ejecuta vive en el disco del HOST aunque `--fs=lfs:` ponga
     * una imagen littlefs para lo que ve el programa BP. Así que se registra
     * el host, se carga el entry, y sólo DESPUÉS se cambia a la imagen.
     * (Antes daba igual porque el loader hacía fopen a pelo — un atajo que
     * escondía justo esta distinción.) */
    bpvm_fs_register_host();
    if (basedir) bpvm_fs_set_basedir(basedir);   /* H19-F1: readFile/load relativos resuelven bajo la raíz */
    bpvm_fs_set_main_module_path(path);          /* H19: App.mainModulePath() = el .mod ejecutado */
    bpvm_net_register_host();  /* H11 — sockets TCP del SO (host) */

    /* H3 — zona de packs simulada: la región RAM hace de flash interna del
     * micro; los --pack= quedan "grabados" en ella antes de arrancar la VM. */
    uint8_t* packs_region = NULL;
    if (n_pack_files > 0 || n_pack_dels > 0) {
        packs_region = mem + mem_size;   /* tallada en el mismo buffer (ver arriba) */
        if (packs_setup(packs_region, pack_files, n_pack_files,
                        pack_dels, n_pack_dels) != 0) {
            bpvm_destroy(vm); free(mem);
            return 1;
        }
        /* H3.c — montar la región: la resolución de imports (discover_deps) la
         * consulta como fallback tras el FS. Como en la placa. */
        bpvm_pack_mount(packs_region, PACKS_REGION_SIZE);
    }

    debug_trace_state_t dbg_state = { 0, debug_print, 0 };
    if (debug_trace) {
        /* pc_to_line=NULL → modo sintético "todo opcode es una línea". */
        bpvm_set_debug_hook(vm, debug_trace_hook, NULL, &dbg_state);
    }

    /* #344 — UNA sola carga: bpvm_load_entry mira la extensión y despacha
     * .mod/.pack, resuelve las dependencias con la regla común y avisa si
     * falta alguna. El `if` del pack ya no vive aquí (vivía SÓLO aquí, y por
     * eso los packs no llegaban a la placa). */
    bpvm_entry_t entry;
    memset(&entry, 0, sizeof entry);
    bpvm_status_t s = bpvm_load_entry(vm, path, &entry);
    if (s == BPVM_OK && entry.from_pack)
        printf("[packs] ejecutando '%s' (main=%s)\n", path, entry.main_module);
    if (s != BPVM_OK) {
        if (entry.missing[0])
            fprintf(stderr, "load %s: falta el modulo '%s'\n", path, entry.missing);
        else
            fprintf(stderr, "load_mod %s: %s\n", path, bpvm_status_str(s));
        bpvm_destroy(vm); free(mem);
        return (int) s;
    }

    /* Ya con los módulos DENTRO, el FS pasa a ser el que verá el programa BP.
     * `--fs=lfs:` = modo ORÁCULO (mismo motor que el micro); la imagen se
     * formatea sólo si no monta = primer arranque. Va aquí y no antes porque
     * el .mod que se ejecuta vive en el disco del host: el orden separa "de
     * dónde saco el programa" de "qué FS ve el programa", que antes se
     * confundían porque el loader hacía fopen a pelo. */
    if (fs_lfs_img) {
        if (bpvm_fs_register_lfs_filebd(fs_lfs_img, 0, 0, 1) != 0) {
            fprintf(stderr, "--fs=lfs: no se pudo montar littlefs sobre %s\n", fs_lfs_img);
            bpvm_destroy(vm); free(mem);
            return 1;
        }
        if (basedir) bpvm_fs_set_basedir(basedir);
        bpvm_fs_set_main_module_path(path);
    }

    if (no_gc) {
        bpvm_set_gc_enabled(vm, 0);
        printf("=== GC DESACTIVADO (--nogc): memoria de un solo uso ===\n");
    }

    if (handle_cap >= 0) {
        bpvm_set_handle_cap_max(vm, (uint32_t) handle_cap);   /* #430 */
        fprintf(stderr, "config: handle_cap_max=%ld (--handlecap)\n", handle_cap);
    }

    if (smp_workers > 0) {
        printf("=== INICIANDO EJECUCION DE LA VM-C (SMP, workers=%d) ===\n",
               smp_workers);
        s = bpvm_run_smp(vm, smp_workers);
    } else {
        printf("=== INICIANDO EJECUCION DE LA VM-C ===\n");
        s = bpvm_run(vm);
    }
    {
        const char* le = bpvm_link_error(vm);   /* paso 4 — detalle de lib/símbolo no resuelto */
        if (le[0]) printf("=== ERROR DE LINK: %s ===\n", le);
        /* Detalle del RuntimeError no atrapado (en vez del status genérico). */
        const char* re = bpvm_runtime_error(vm);
        if (s == BPVM_ERR_RUNTIME && re[0]) printf("=== RuntimeError: %s ===\n", re);
    }
    printf("=== FIN DE LA EJECUCION (status=%s) ===\n", bpvm_status_str(s));

    if (debug_trace) {
        fprintf(stderr, "[dbg] total hook hits: %ld\n", dbg_state.total_hits);
    }

    bpvm_destroy(vm);
    free(mem);
    (void) packs_region;   /* tallada dentro de mem: se libera con él */
    return (int) s;
}
