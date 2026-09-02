/* bpvm_mem.c — el planificador de memoria de la VM. Ver bpvm_mem.h para el porqué.
 *
 * Sin dependencias a propósito: solo <stddef.h> y <stdio.h> (para la línea del
 * log). Así el test es host-only y no arrastra la VM — mismo criterio que
 * bpvm_env/bpvm_part/bpvm_boot/bpvm_sqlmem.
 */
#include "bpvm_mem.h"
#include <stdio.h>

static const char* K_REGION   = "la región";
static const char* K_CONTIGUO = "el bloque contiguo";
static const char* K_MARGEN   = "el margen del sistema";
static const char* K_MARGEN_R = "el margen de la región";

bpvm_mem_res_t bpvm_mem_plan(const bpvm_mem_region_t* r, int n,
                             const bpvm_mem_cfg_t* cfg, bpvm_mem_plan_t* out)
{
    if (out) { out->res = BPVM_MEM_SIN_REGIONES; out->idx = -1; out->bytes = 0;
               out->techo = 0; out->limita = ""; out->reserva_bytes = 0; }
    if (!r || n <= 0 || !cfg || !out) return BPVM_MEM_SIN_REGIONES;

    /* 1. ELEGIR: la exclusiva mayor si la hay; si no, la compartida mayor.
     *    Exclusiva primero no es una preferencia de velocidad —U6.3 midió que la
     *    PSRAM solo cuesta 4-7 %— sino de CAPACIDAD y de dejar la SRAM entera para
     *    lo que no es la VM, que es lo que Eduardo quiere explotar más adelante. */
    int mejor = -1;
    for (int pase = 1; pase >= 0 && mejor < 0; pase--) {          /* 1 = exclusivas, 0 = compartidas */
        for (int i = 0; i < n; i++) {
            if ((r[i].exclusiva != 0) != (pase != 0)) continue;
            if (r[i].bytes == 0) continue;
            if (mejor < 0 || r[i].bytes > r[mejor].bytes) mejor = i;
        }
    }
    if (mejor < 0) return BPVM_MEM_SIN_REGIONES;
    const bpvm_mem_region_t* R = &r[mejor];
    out->idx = mejor;

    /* 2. EL TECHO de esa región: lo que cabe dejando lo debido. UNA cuenta para
     *    las dos clases de memoria (U6.10, y lo enseñó el P4 con números):
     *
     *        techo = min( contiguo − reserva ,  libre − reserva − margen )
     *
     *    · lo CONTIGUO es el tope de lo que un malloc puede dar (P1.C3.3: en el
     *      C3 hay 280 KB libres en siete trozos y el mayor son 136);
     *    · el MARGEN va contra el TOTAL, no contra el contiguo: lo que se deja a
     *      los demás inquilinos —malloc/RTOS, o los 4 MiB del display del P4— lo
     *      piden ellos en trozos y no necesitan ser contiguos con nada. La primera
     *      versión lo restaba del contiguo en la exclusiva y el P4 perdió 508 KiB
     *      respecto a su imagen anterior (32256 − 4096 en vez de 32765 − 4096);
     *    · la RESERVA con nombre (SQLite) se aparta DELANTE del bloque, así que
     *      resta a los dos.
     *    Se dice cuál de las dos manda. */
    size_t res = 0;
    if (R->exclusiva) {
        /* La validez de la reserva (mínimos, dejar VM viable) la decide quien la
         * pide —bpvm_sqlmem—; aquí sólo se aparta si cabe dejando el suelo. */
        res = cfg->reserva_bytes;
        if (res > 0 && (res >= R->bytes || R->bytes - res < cfg->vm_min)) res = 0;
    }
    out->reserva_bytes = res;
    size_t libre    = (R->libre > R->bytes) ? R->libre : R->bytes;
    size_t contiguo = R->bytes - res;
    size_t total    = (libre > res + R->margen) ? libre - res - R->margen : 0;
    size_t techo;
    const char* limita;
    if (contiguo < total) { techo = contiguo; limita = K_CONTIGUO; }
    else {
        techo  = total;
        limita = R->exclusiva ? (R->margen ? K_MARGEN_R : K_REGION) : K_MARGEN;
    }

    /* 3. CUÁNTO: el objetivo si cabe, y nunca más que el objetivo aunque quepa. */
    size_t bytes = techo;
    if (cfg->objetivo > 0 && cfg->objetivo < bytes) bytes = cfg->objetivo;

    out->techo  = techo;
    out->limita = limita;
    if (bytes < cfg->vm_min || bytes == 0) {
        /* No se arranca a medias: sin VM, y el que llame lo dice con los números. */
        out->bytes = 0;
        out->res   = BPVM_MEM_NO_CABE;
        return out->res;
    }
    out->bytes = bytes;
    out->res   = BPVM_MEM_OK;
    return out->res;
}

int bpvm_mem_plan_str(const bpvm_mem_plan_t* p, const bpvm_mem_region_t* r,
                      const bpvm_mem_cfg_t* cfg, char* buf, size_t cap)
{
    if (!p || !buf || cap == 0) return 0;
    const char* nombre = (p->idx >= 0 && r) ? (r[p->idx].nombre ? r[p->idx].nombre : "?") : "-";
    unsigned kb    = (unsigned)(p->bytes / 1024u);
    unsigned techo = (unsigned)(p->techo / 1024u);
    unsigned obj   = (unsigned)((cfg ? cfg->objetivo : 0) / 1024u);
    int n;
    switch (p->res) {
    case BPVM_MEM_OK:
        if (obj && p->bytes < (cfg ? cfg->objetivo : 0))
            n = snprintf(buf, cap, "vm: %u KB en %s, POR DEBAJO del objetivo (%u): manda %s (techo %u KB)",
                         kb, nombre, obj, p->limita, techo);
        else if (obj)
            n = snprintf(buf, cap, "vm: %u KB en %s (objetivo %u, techo %u por %s)",
                         kb, nombre, obj, techo, p->limita);
        else
            n = snprintf(buf, cap, "vm: %u KB en %s (todo lo que deja %s)",
                         kb, nombre, p->limita);
        if (p->reserva_bytes && cfg && cfg->reserva_nombre && n > 0 && (size_t) n < cap)
            n += snprintf(buf + n, cap - (size_t) n, " | %s %u KB delante",
                          cfg->reserva_nombre, (unsigned)(p->reserva_bytes / 1024u));
        break;
    case BPVM_MEM_NO_CABE:
        n = snprintf(buf, cap, "vm: NO CABE en %s — techo %u KB por %s, suelo %u KB: sin VM",
                     nombre, techo, p->limita, (unsigned)((cfg ? cfg->vm_min : 0) / 1024u));
        break;
    default:
        n = snprintf(buf, cap, "vm: SIN REGIONES: la placa no ofreció memoria — sin VM");
        break;
    }
    return (n < 0) ? 0 : n;
}
