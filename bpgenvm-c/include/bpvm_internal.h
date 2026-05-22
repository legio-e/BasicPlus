/*
 * bpvm_internal.h — tipos y constantes compartidos entre las TUs de la VM.
 * No exportado: el caller usa `bpvm.h`.
 */
#ifndef BPVM_INTERNAL_H
#define BPVM_INTERNAL_H

#include "bpvm.h"
#include <stdbool.h>

/* ============================================================ */
/*  Constantes extraídas de docs/MOD_FORMAT.md / HEAP_LAYOUT.md  */
/* ============================================================ */

#define BPVM_MAGIC          0x4D4F4435u   /* "MOD5" big-endian */
#define BPVM_HEADER_SIZE    28
#define BPVM_FORMAT_VERSION 5

/* Tamaño en bytes de una entrada de la ext-table (per-module). */
#define BPVM_EXT_ENTRY_SIZE 4

/* Stack del thread main; ver HEAP_LAYOUT.md §8 */
#define BPVM_MAIN_STACK_BYTES   (16 * 1024)
#define BPVM_THREAD_STACK_BYTES (2 * 1024)

/* Dirección inicial reservada (sentinela THREAD_EXIT en memory[0]). */
#define BPVM_INITIAL_FREE_ADDR  0x0100

/* Tag bits del header de objeto en heap (HEAP_LAYOUT.md §2.1). */
#define BPVM_TAG_MARK_BIT   0x80000000u
#define BPVM_TAG_FREE_BIT   0x40000000u
#define BPVM_TAG_TYPE_MASK  0x3F000000u
#define BPVM_TAG_TYPE_SHIFT 24

/* Códigos de tipo de heap. */
#define BPVM_TYPE_ARRAY_I8  0
#define BPVM_TYPE_ARRAY_I16 1
#define BPVM_TYPE_ARRAY_I32 2
#define BPVM_TYPE_ARRAY_REF 3
#define BPVM_TYPE_OBJECT    4

/* Header de objeto en heap. */
#define BPVM_OBJ_HEADER_SIZE 8
#define BPVM_MIN_FREE_BLOCK  12

/* Offsets del class descriptor (MOD_FORMAT.md §8). */
#define BPVM_CLS_OFF_NUM_FIELDS   0
#define BPVM_CLS_OFF_NUM_METHODS  2
#define BPVM_CLS_OFF_BITMAP_WORDS 4
#define BPVM_CLS_OFF_PARENT_OFF   8
#define BPVM_CLS_OFF_FIELD_BITMAP 12

/* Bytes sentinela en memory[0] — vea HEAP_LAYOUT.md §1. */
#define BPVM_SENTINEL_THREAD_EXIT 0x70

/* Máximo de módulos cargables simultáneamente.
   F1 no carga deps, así que 8 sobra. F3+ puede crecer si hace falta. */
#define BPVM_MAX_MODULES 16

/* Máximo de threads BP simultáneos. F1: sólo main (= 1). */
#define BPVM_MAX_THREADS 32

/* ============================================================ */
/*  Tipos                                                        */
/* ============================================================ */

typedef struct {
    char     library[64];      /* "" si no hay */
    char     name[64];
    uint32_t module_base;      /* dirección absoluta donde empieza la ext-table */
    uint32_t ext_table_addr;   /* = module_base */
    uint32_t ext_count;        /* nº de entries de la ext-table */
    uint32_t data_start;       /* = module_base + ext_count*4 */
    uint32_t data_size;
    uint32_t code_start;       /* = data_start + data_size; CS del módulo */
    uint32_t code_size;
    uint32_t end_addr;         /* code_start + code_size; primer byte libre */
    int32_t  main_offset;      /* -1 si no es entry-point */
} bpvm_module_t;

typedef enum {
    BPVM_THREAD_RUNNABLE = 0,
    BPVM_THREAD_RUNNING,
    BPVM_THREAD_TERMINATED,
    BPVM_THREAD_BLOCKED_SLEEP,
    BPVM_THREAD_BLOCKED_MUTEX,
    BPVM_THREAD_BLOCKED_JOIN,
    BPVM_THREAD_BLOCKED_PROMPT
} bpvm_thread_status_t;

typedef struct {
    int32_t  id;
    uint32_t pc;
    uint32_t sp;
    uint32_t bp;
    uint32_t cs;
    uint32_t stack_base;       /* dirección baja de su región de pila */
    uint32_t stack_top;        /* dirección alta (excluida) */
    bpvm_thread_status_t status;
    /* F4 añade: blockedOnMutex, wakeAt, handlerStack, allocAnchor */
} bpvm_thread_t;

struct bpvm {
    /* Buffer del caller. */
    uint8_t* memory;
    size_t   memory_size;
    uint32_t stack_base;       /* offset donde termina heap y empiezan stacks */

    /* Allocator del data block (bump). */
    uint32_t next_free_address;
    uint32_t heap_start;       /* fijado tras último módulo cargado */
    uint32_t heap_next;        /* bump del heap (F2) */

    /* Módulos cargados. */
    bpvm_module_t modules[BPVM_MAX_MODULES];
    int           module_count;
    uint32_t      main_absolute_address;   /* 0 = sin entry-point */

    /* Threads BP. F1: sólo main. */
    bpvm_thread_t threads[BPVM_MAX_THREADS];
    int           thread_count;
    int           current_thread_idx;

    /* Output sink. */
    bpvm_output_cb output_cb;
    void*          output_user;

    /* Flags. */
    bool tracing;

    /* Buffer staging para imports/exports al cargar — re-usable. */
    uint8_t* scratch;
    size_t   scratch_size;
};

/* ============================================================ */
/*  Helpers (util.c)                                             */
/* ============================================================ */

/* Lectura big-endian del memory[] (sin alignment requirements). */
static inline uint32_t bpvm_read_u32_be(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}
static inline int32_t bpvm_read_i32_be(const uint8_t* p) {
    return (int32_t) bpvm_read_u32_be(p);
}
static inline uint16_t bpvm_read_u16_be(const uint8_t* p) {
    return (uint16_t)(((uint32_t)p[0] << 8) | (uint32_t)p[1]);
}
static inline int16_t bpvm_read_i16_be(const uint8_t* p) {
    return (int16_t) bpvm_read_u16_be(p);
}
static inline void bpvm_write_u32_be(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8);
    p[3] = (uint8_t) v;
}
static inline void bpvm_write_i32_be(uint8_t* p, int32_t v) {
    bpvm_write_u32_be(p, (uint32_t) v);
}

/* Acceso al memory[] por dirección absoluta. Sin bounds-check en
   release; F2+ añade variantes "checked" en debug. */
static inline uint32_t bpvm_mem_read_u32(const bpvm_t* vm, uint32_t addr) {
    return bpvm_read_u32_be(vm->memory + addr);
}
static inline int32_t bpvm_mem_read_i32(const bpvm_t* vm, uint32_t addr) {
    return bpvm_read_i32_be(vm->memory + addr);
}
static inline void bpvm_mem_write_i32(bpvm_t* vm, uint32_t addr, int32_t v) {
    bpvm_write_i32_be(vm->memory + addr, v);
}

/* ============================================================ */
/*  Interfaces internas entre TUs                                */
/* ============================================================ */

/* loader.c */
bpvm_status_t bpvm_loader_load(bpvm_t* vm, const char* path);

/* interp.c */
bpvm_status_t bpvm_interp_run(bpvm_t* vm);

#endif /* BPVM_INTERNAL_H */
