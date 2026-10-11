/* bpvm_npack_carga.c — V7/H7 (Boot 2, fase 2; 30-sep): EL CARGADOR de los motores nativos,
 * común a las tres familias. Ver bpvm_npack.h (bpvm_npack_cargar_zona) para el porqué.
 *
 * Aparte de bpvm_npack.c a propósito: aquél sólo DECIDE —la escalera, el plan, el tope— y se
 * prueba entero en el PC; éste copia la `.data`, pone la `.bss` a cero y SALTA. Lo que era de
 * cada placa (encontrar la zona) ya lo hace el arranque al montarla (fase 1); lo que sigue
 * siendo suyo son la BIOS que presta, el bloque de RAM y cómo volcar el log: bpvm_npack_familia_t.
 */
#include "bpvm_npack.h"
#include "bpvm_pack.h"       /* la zona MONTADA, y la vista de ejecución para saltar */
#include "bpvm_bios.h"       /* la arena con dueño */
#include "mdn_loader.h"      /* bpvm_mdn_host_arch / _float_abi: lo que ESTA placa es */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int     s_hecho     = 0;   /* la guarda: se saltó a algún motor (ver bpvm_npack.h) */
static int32_t s_resultado = 0;
static int     s_sin_bloque = 0;  /* V7 #682: el último intento no cargó nada por no haber bloque de RAM */

static void di(const bpvm_npack_familia_t* f, const char* fmt, ...) {
    if (!f->log) return;
    char l[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(l, sizeof l, fmt, ap);
    va_end(ap);
    f->log(l);
}

/* Por qué un motor no se carga — con los números cuando los hay. */
static void explicar(const bpvm_npack_familia_t* f, const bpvm_npack_carga_t* c, int i,
                     const bpvm_npack_carga_t* todos, const uint8_t* zona) {
    if (c->eclipsado_por >= 0) {
        di(f, "pack: [%d] '%s' (motor '%s') NO se carga: hay otro motor '%s' mas adelante "
              "(el pack '%s', grabado despues) y gana el ultimo, como los .mod — borra el viejo",
           i, c->pack, c->npk, c->npk, todos[c->eclipsado_por].pack);
        return;
    }
    if (c->pisa_a >= 0) {
        const bpvm_npack_carga_t* o = &todos[c->pisa_a];
        di(f, "pack: [%d] '%s' NO se carga: su RAM [0x%08lX, +%u) pisa la de '%s' [0x%08lX, +%u) "
              "— dos sellos solapados (uno de antes de G1, que daba a todos la base): regrabalo",
           i, c->pack, (unsigned long) c->ram, (unsigned) c->ram_bytes,
           o->pack, (unsigned long) o->ram, (unsigned) o->ram_bytes);
        return;
    }
    di(f, "pack: [%d] '%s' (motor '%s') NO se carga — %s", i, c->pack, c->npk,
       bpvm_npack_res_str(c->motivo));
    if (c->hdr) {
        const bpvm_npack_hdr_t* m = c->hdr;
        if (c->motivo == BPVM_NPACK_E_SELLO)
            di(f, "pack: [%d] su sello dice flash 0x%08lX ram 0x%08lX | aqui esta en flash 0x%08lX "
                  "y el bloque de RAM es [0x%08lX, +%u)",
               i, (unsigned long) m->linked_flash, (unsigned long) m->linked_ram,
               (unsigned long) c->codigo, (unsigned long) (uintptr_t) f->ram, (unsigned) f->ram_bytes);
        else if (c->motivo == BPVM_NPACK_E_TAMANO)
            di(f, "pack: [%d] pide .data %u + .bss %u B desde 0x%08lX; el bloque acaba en 0x%08lX",
               i, (unsigned) m->data_bytes, (unsigned) m->bss_bytes, (unsigned long) m->linked_ram,
               (unsigned long) ((uintptr_t) f->ram + f->ram_bytes));
        else if (c->motivo == BPVM_NPACK_E_ARCH || c->motivo == BPVM_NPACK_E_ABI)
            di(f, "pack: [%d] el motor es arch %u, abi '%.8s' | esta placa ES arch %u, abi '%s'",
               i, (unsigned) m->arch, m->float_abi,
               (unsigned) bpvm_mdn_host_arch(), bpvm_mdn_host_float_abi());
    }
    if (f->explicar_mas) f->explicar_mas(c, i, zona);
}

/* Saltar a UN motor: su RAM —la `.data` copiada, la `.bss` a cero, EN SU TROZO: sin la copia
 * sus variables arrancarían con basura (lo cazó SQLite en el P4)—, la miga de pan VOLCADA
 * —si el siguiente arranque enseña el «saltando» sin su «volvio», se colgó dentro— y la arena
 * con dueño. */
static int32_t saltar(const bpvm_npack_familia_t* f, const bpvm_npack_carga_t* c, int i) {
    const bpvm_npack_hdr_t* hdr = c->hdr;
    uint8_t* ram = (uint8_t*) (uintptr_t) c->ram;
    const uint8_t* data_img = (const uint8_t*) (uintptr_t) (c->codigo + hdr->flash_bytes);
    if (hdr->data_bytes) memcpy(ram, data_img, hdr->data_bytes);
    if (hdr->bss_bytes)  memset(ram + hdr->data_bytes, 0, hdr->bss_bytes);

    /* Se salta por la vista de EJECUCIÓN: la identidad en todas las placas de hoy, y resuelto
     * el día que un micro separe datos e instrucciones (bpvm_pack_mount). */
    uint32_t entrada = bpvm_npack_entry_addr(hdr, c->codigo);
    entrada = (uint32_t) (uintptr_t) bpvm_pack_exec_ptr((const uint8_t*) (uintptr_t) entrada);

    di(f, "pack: [%d] '%s' (motor '%s') valido @0x%08lX | .data %u B a 0x%08lX, .bss %u B a cero"
          " | saltando a 0x%08lX",
       i, c->pack, c->npk, (unsigned long) (c->codigo - BPVM_NPACK_HDR_BYTES),
       (unsigned) hdr->data_bytes, (unsigned long) c->ram, (unsigned) hdr->bss_bytes,
       (unsigned long) entrada);
    if (f->flush) f->flush();

    bpvm_bios_arena_entra(c->pack);
    typedef int32_t (*entrada_fn)(const bpvm_bios_t*);
    int32_t r = ((entrada_fn) (uintptr_t) entrada)((const bpvm_bios_t*) f->bios);
    bpvm_bios_arena_sale();

    const char* no = bpvm_bios_arena_negada();
    di(f, "pack: [%d] '%s' volvio, rc=%ld%s%s", i, c->pack, (long) r,
       no ? " | pidio la arena y se le nego: " : "", no ? no : "");
    return r;
}

static int32_t cargar(const bpvm_npack_familia_t* f) {
    uint32_t zbytes = 0;
    const uint8_t* zona = bpvm_pack_mounted(&zbytes);
    s_sin_bloque = 0;
    if (!zona || zbytes == 0) {
        di(f, "pack: no hay zona de packs montada — no se carga ningun motor");
        return -(int32_t) BPVM_NPACK_E_MAGIC;
    }
    if (!f->ram || f->ram_bytes == 0) {
        di(f, "pack: no hay bloque de RAM para los motores (ni packRam= ni SQLite= en el ENV, "
              "o no cupo) — no se carga ninguno");
        s_sin_bloque = 1;   /* #682: para que quien eche en falta un motor sepa por qué */
        return -(int32_t) BPVM_NPACK_E_TAMANO;
    }
    /* La BIOS, comprobada ANTES de prestarla: un hueco se manifestaría DENTRO del motor,
     * donde no hay depurador. */
    const char* falta = f->bios ? 0 : "la BIOS de esta placa tiene huecos";

    uint32_t fin = 0;
    if (bpvm_pack_scan(zona, zbytes, 0, 0, 0, &fin) <= 0) {
        di(f, "pack: la zona no tiene ningun pack grabado - no se recorre");
        return -(int32_t) BPVM_NPACK_E_MAGIC;
    }

    /* EL PLAN: la cadena, en orden, con el veredicto de cada motor. La MISMA regla que usa
     * PACK_BURN_BEGIN para decirle al IDE dónde sellar el siguiente. */
    uint32_t ram_base = (uint32_t) (uintptr_t) f->ram;
    bpvm_npack_carga_t c[BPVM_NPACK_MAX];
    bpvm_npack_plan_t  p;
    int vistos = bpvm_npack_planear(zona, zbytes, ram_base, f->ram_bytes, falta,
                                    c, BPVM_NPACK_MAX, &p);
    int n = (vistos < BPVM_NPACK_MAX) ? vistos : BPVM_NPACK_MAX;
    if (vistos == 0) {
        di(f, "pack: hay packs grabados pero ninguno trae motor nativo (npk)");
        return -(int32_t) BPVM_NPACK_E_MAGIC;
    }
    uint32_t usada = p.ram_tope - ram_base;
    di(f, "pack: %d motor(es) en la zona, %d cargable(s) | bloque %u KB @0x%08lX: "
          "estaticos %u B, arena %u KB",
       vistos, p.cargables, (unsigned) (f->ram_bytes / 1024u), (unsigned long) ram_base,
       (unsigned) usada, (unsigned) ((f->ram_bytes - usada) / 1024u));
    if (vistos > n)
        di(f, "pack: hay %d motores y solo se miran los %d primeros (BPVM_NPACK_MAX)", vistos, n);

    /* La arena: lo que queda por encima del último motor. Se fija ANTES de saltar a ninguno,
     * y con ella se olvida cualquier dueña anterior. */
    bpvm_bios_arena_fija(f->ram + usada, f->ram_bytes - usada);

    int saltados = 0, avisado = 0;
    bpvm_npack_res_t ultimo = BPVM_NPACK_E_MAGIC;
    for (int i = 0; i < n; i++) {
        if (!bpvm_npack_cargable(&c[i])) {
            explicar(f, &c[i], i, c, zona);
            if (c[i].eclipsado_por < 0) {   /* eclipsado no es un fallo: gana el último */
                ultimo = (c[i].pisa_a >= 0) ? BPVM_NPACK_E_SELLO : c[i].motivo;
                char av[120];
                snprintf(av, sizeof av, "el pack '%s' no carga su motor: %s", c[i].pack,
                         (c[i].pisa_a >= 0) ? "su RAM pisa la de otro motor (regrabalo)"
                                            : bpvm_npack_res_str(c[i].motivo));
                if (!avisado) { bpvm_pack_aviso_motor(av); avisado = 1; }   /* el «casi»: el primero */
            }
            continue;
        }
        (void) saltar(f, &c[i], i);
        saltados++;
    }
    if (saltados > 0) di(f, "packs: %d motor(es) cargado(s)", saltados);
    else              di(f, "packs: sin motor utilizable (peldano %ld) — se sigue sin el", (long) ultimo);
    return saltados > 0 ? saltados : -(int32_t) ultimo;
}

int bpvm_npack_motores_cargados(void) { return s_hecho; }   /* V7 #671 */

/* V7 #682 (10-oct) — POR QUÉ FALTA UN MOTOR, para quien lo eche en falta. La GUI decía «esta placa no tiene el pack
 * 'LVGL' grabado» también con el pack grabado y su motor sin cargar —sin `packRam=` en el ENV—, y eso engaña. NULL si
 * en la zona no hay ningún pack vivo con un motor de ese nombre (y entonces «no está» es la verdad); si lo hay, el
 * porqué, corto (el EXITED recorta el mensaje): el bloque de RAM que falta, el aviso que dejó el cargador sobre ESE pack
 * o, si no dejó ninguno, el log. Lo pregunta quien ya sabe que el motor no publicó su API. */
const char* bpvm_npack_por_que_falta(const char* npk) {
    static char porque[160];   /* del tamaño del aviso que copia: las imágenes compilan con -Werror=format-truncation */
    uint32_t zbytes = 0;
    const uint8_t* zona = bpvm_pack_mounted(&zbytes);
    if (!npk || !zona || zbytes == 0) return NULL;
    bpvm_npack_carga_t c[BPVM_NPACK_MAX];
    bpvm_npack_plan_t  p;
    int vistos = bpvm_npack_planear(zona, zbytes, 0, 0, NULL, c, BPVM_NPACK_MAX, &p);   /* sólo los nombres */
    int n = (vistos < BPVM_NPACK_MAX) ? vistos : BPVM_NPACK_MAX;
    for (int i = 0; i < n; i++) {
        if (strcmp(c[i].npk, npk) != 0) continue;
        if (s_sin_bloque) return "falta packRam= en el ENV";
        char av[160], quien[BPVM_PACK_NAME_LEN + 4];
        snprintf(quien, sizeof quien, "'%s'", c[i].pack);
        if (bpvm_pack_aviso_motor_de(av, sizeof av) && strstr(av, quien)) {
            const char* tras = strstr(av, "su motor: ");
            snprintf(porque, sizeof porque, "%s", tras ? tras + 10 : av);
            return porque;
        }
        return "mira el log de la placa";
    }
    return NULL;
}

void bpvm_npack_carga_reinicia(void) {
    s_hecho = 0;
    s_resultado = 0;
    s_sin_bloque = 0;
    bpvm_pack_aviso_motor(NULL);
}

int32_t bpvm_npack_cargar_zona(const bpvm_npack_familia_t* f) {
    if (!f) return -(int32_t) BPVM_NPACK_E_MAGIC;
    if (s_hecho) return s_resultado;              /* ya se saltó: volver a saltar republicaría sus marcas */
    bpvm_pack_aviso_motor(NULL);
    s_resultado = cargar(f);
    if (s_resultado >= 1) s_hecho = 1;            /* si no se saltó a ninguno, el siguiente RUN lo reintenta */
    return s_resultado;
}
