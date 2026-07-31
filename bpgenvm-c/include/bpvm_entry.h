/*
 * bpvm_entry.h — EL RUN, ESCRITO UNA VEZ (#344).
 *
 * Hasta aquí había CINCO implementaciones independientes de "cargar lo que se
 * va a ejecutar": test/main.c, tools/bpvm_sim.c y los REPL de las tres
 * familias. Cada una con su bucle de dependencias y su regla de búsqueda. Y no
 * eran cinco variantes: eran la MISMA regla copiada — las 15 líneas de
 * `v1_resolve_path` están palabra por palabra en Pico, ESP32 y STM32.
 *
 * Eso choca de frente con la norma de Eduardo: «el mecanismo ha de ser el mismo
 * para todas las familias; que por debajo la HAL sea distinta no nos debería
 * afectar». Y tenía factura pendiente: meter packs en el device era copiar la
 * misma regla en cuatro sitios más.
 *
 * SE PUEDE UNIFICAR DEL TODO porque la parte "de sistema" ya estaba abstraída
 * antes: la fachada del FS (bpvm_fs_stat / bpvm_fs_read_at, #305) y la zona de
 * packs (bpvm_pack_*) son portables. Aquí no queda ni una línea propietaria.
 *
 * QUÉ HACE, en orden:
 *   1. Resuelve la ruta con la regla ÚNICA: basedir del proyecto → tal cual →
 *      /app → /lib.
 *   2. Mira la extensión y despacha: `.pack` → módulo principal del manifest;
 *      cualquier otra cosa → `.mod`. **El `if` vive aquí y sólo aquí.**
 *   3. Carga POR TROZOS desde el FS (nada de "buffer del fichero más grande
 *      imaginable" en un micro).
 *   4. Resuelve las dependencias con la MISMA regla en las 5: el FS primero y,
 *      si no está, los packs grabados (XIP, sin copiar a RAM).
 *   5. Guardián: si algún import se queda sin dueño, lo NOMBRA. Mejor un error
 *      limpio que un CALL_EXT al vacío.
 *
 * QUÉ NO HACE, a propósito: ejecutar ni reportar. Ahí las familias difieren de
 * verdad (bpvm_run vs bpvm_run_smp(N); JSON por el wire vs stdout), así que eso
 * se queda en cada REPL. Unificar lo que es igual, respetar lo que no.
 */
#ifndef BPVM_ENTRY_H
#define BPVM_ENTRY_H

#include "bpvm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* --- SALIDA --- */
    /* Módulo que se quedó sin resolver ("" si todo cuadró). El llamante lo usa
     * para un mensaje que sirva: "falta el modulo 'Gui'" en vez de un fallo de
     * link doscientas líneas más tarde. */
    char missing[48];
    /* Ruta REAL de la que salió el entry, ya resuelta (diagnóstico: "no aparece
     * por ningún lado" y "salió de /lib, no del que acabas de subir" son dos
     * ratos de búsqueda distintos). */
    char resolved[128];
    /* Nombre del módulo principal. Con un .pack lo dice su manifest; con un
     * .mod es el nombre del propio módulo. */
    char main_module[BPVM_PACK_NAME_LEN + 1];
    /* 1 si el entry era un pack. */
    int  from_pack;

    /* --- ENTRADA (opcional; 0/NULL = nada) --- */
    /* Se llama por cada módulo cargado, para que el REPL pueda contarlo por la
     * consola del IDE. `origen` = "fs" | "pack". */
    void (*on_module)(const char* name, const char* origen, uint32_t size, void* user);
    void* user;
} bpvm_entry_t;

/* Deja la VM con el entry y TODAS sus dependencias cargadas, lista para correr.
 * `e` puede ser NULL si no interesa el detalle.
 *
 * Devuelve BPVM_OK, o el error del loader. Si el fallo es "falta un módulo",
 * devuelve BPVM_ERR_IO y deja el nombre en `e->missing`. */
bpvm_status_t bpvm_load_entry(bpvm_t* vm, const char* path, bpvm_entry_t* e);

/* La regla de búsqueda, suelta, para quien necesite sólo resolver (el autorun
 * mira si existe antes de arrancar, el REPL valida la ruta del RUN...).
 * Devuelve 0 y rellena `out`/`size_out` si lo encuentra; -1 si no. */
int bpvm_entry_resolve(const char* name, char* out, size_t out_cap,
                       uint32_t* size_out);

/* Carga UN .mod ya resuelto, por trozos desde la fachada del FS (nada de
 * traerse el fichero entero a RAM). Es lo que usan por dentro bpvm_load_entry y
 * la resolución de dependencias; se expone porque algún llamante ya tiene la
 * ruta resuelta y no quiere volver a buscarla. */
bpvm_status_t bpvm_load_entry_file(bpvm_t* vm, const char* resolved_path);

/* #345 — QUÉ arranca solo al encender: la primera línea de /sys/auto.txt.
 *
 * Otra vez las mismas doce líneas copiadas en Pico, ESP32 y STM32 (saltar
 * espacios, cortar en el primer CR/LF, recortar por la derecha). Y otra vez sin
 * un motivo: leer la cabeza de un fichero y limpiar una ruta no tiene nada de
 * propietario — la fachada del FS ya estaba abstraída.
 *
 * Sólo se lee la PRIMERA LÍNEA, y sólo la cabeza del fichero: en un micro no se
 * trae uno un fichero entero a RAM para mirar un renglón.
 *
 * Devuelve 1 y deja la ruta en `out` si hay autorun; 0 si no lo hay (no existe,
 * está vacío o su primera línea son espacios) — que NO es un error: es el caso
 * normal de una placa que arranca al REPL y espera al IDE.
 *
 * Ojo con lo que NO decide: si ESE autorun debe arrancarse AHORA. Eso depende
 * de la ventana de rescate y del historial de arranques, y va aparte. */
int bpvm_autorun_entry(char* out, size_t out_cap);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_ENTRY_H */
