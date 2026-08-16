/* pack_p4.c — cargador del pack nativo en el ESP32-P4 (V5/H7, paso 3).
 *
 * Hermano de `pico/pack_pico.c`. Lo que cambia es CÓMO se llega al código:
 *
 *   Pico    la flash está XIP en una dirección FIJA -> el pack se lee y se
 *           ejecuta donde está, sin más.
 *   P4      hay que MAPEAR la zona, y la dirección la asigna la MMU en tiempo
 *           de ejecución -> nadie puede saberla desde el PC.
 *
 * Lo demás —la escalera, el sello, el salto— es el MISMO código común. No hay
 * una copia local de `bpvm_npack_check` a propósito: esa regla no puede
 * divergir entre familias.
 *
 * ─── LA PREGUNTA QUE DECIDE LA FORMA, Y POR QUÉ SE MIDE ───
 *
 * La escalera valida leyendo la cabecera, y compara `float_abi` BYTE A BYTE.
 * Pero el mapeo ejecutable del ESP32 es `MMU_MEM_CAP_32BIT`: sólo admite
 * accesos alineados a 4. Leer bytes sueltos por ahí no da error — DEVUELVE
 * BASURA, que es la peor clase de fallo.
 *
 * Ahora bien, el P4 declara `SOC_MMU_DI_VADDR_SHARED = 1` ("D/I vaddr are
 * shared"), así que es MUY posible que mapear el mismo rango como datos y como
 * instrucciones devuelva LA MISMA dirección — y entonces el problema no existe.
 *
 * "Muy posible" no es un dato. Así que se mapea de las dos formas, se DICEN las
 * dos direcciones, y el código se comporta según lo que salga:
 *
 *   · iguales  -> un solo mapeo sirve para validar y para ejecutar. Adelante.
 *   · distintas-> se dice CUÁL es cada una y NO se salta. Con dos direcciones,
 *                 el sello sólo puede cuadrar con una, y saltar sin saber cuál
 *                 es exactamente lo que la escalera existe para evitar.
 *
 * ⚠️ Y ES LA DIRECCIÓN DE INSTRUCCIONES la que tiene que ir en el sello: es
 * desde donde se ejecuta, y es la que el pack lleva dentro como `linked_flash`.
 */
#include "pack_p4.h"
#include "bpvm_npack.h"
#include "bpvm_pack.h"   /* V5/H7: saber si hay packs antes de barrer */
#include "bpvm_bios.h"
#include "bpvm_part.h"
#include "mdn_loader.h"   /* bpvm_mdn_host_arch / _float_abi: lo que ESTA placa es */
#include "board_mgr_esp32.h"
#include "log.h"

#include "esp_partition.h"

#include <stdint.h>
#include <string.h>   /* memcpy/memset: la .data y la .bss del pack */

/*
 * La base de la RAM del pack. La rellena el arranque (`main.c`) con el principio
 * del bloque de la BD. 0 = no hay bloque, o sea que tampoco hay dónde poner los
 * estáticos del pack.
 */
uint8_t* s_pack_ram_base = 0;

/* Los mapeos se hacen UNA vez y se quedan: soltarlos invalidaría el código que
 * el pack deja publicado (SQLite publica su tabla de funciones y la VM la usa
 * durante toda la sesión). */
static const void*             s_map_inst  = 0;
static const void*             s_map_data  = 0;
static esp_partition_mmap_handle_t s_h_inst = 0;
static esp_partition_mmap_handle_t s_h_data = 0;
static uint32_t                s_zona_bytes = 0;

/*
 * Mapea la zona de packs de las dos formas y deja dicho qué salió.
 * Devuelve 1 si se puede seguir (un mapeo utilizable), 0 si no.
 */
static int mapear_zona(void)
{
    if (s_map_inst) return 1;                    /* ya estaba */

    const esp_partition_t* bpdata =
        (const esp_partition_t*) board_mgr_esp32_bpdata();
    const bpvm_part_t* packs = board_mgr_esp32_packs();

    if (!bpdata || !packs) {
        log_printf("pack: sin zona de packs (el arranque no llego a particiones)");
        return 0;
    }
    if (packs->size == 0u) {
        log_printf("pack: la zona de packs mide 0 — mira el reparto FS|Packs del ENV");
        return 0;
    }
    s_zona_bytes = packs->size;

    /* El offset es RELATIVO a bpdata, igual que el del FS. */
    esp_err_t ei = esp_partition_mmap(bpdata, packs->offset, packs->size,
                                      ESP_PARTITION_MMAP_INST, &s_map_inst, &s_h_inst);
    esp_err_t ed = esp_partition_mmap(bpdata, packs->offset, packs->size,
                                      ESP_PARTITION_MMAP_DATA, &s_map_data, &s_h_data);

    log_printf("pack: zona %u KB en bpdata+0x%x | mapeo INST %s @%p | DATA %s @%p",
               (unsigned) (packs->size / 1024u), (unsigned) packs->offset,
               (ei == ESP_OK) ? "ok" : "FALLO", s_map_inst,
               (ed == ESP_OK) ? "ok" : "FALLO", s_map_data);

    if (ei != ESP_OK) {
        log_printf("pack: sin mapeo EJECUTABLE no hay nada que hacer (err=%d)", (int) ei);
        s_map_inst = 0;
        return 0;
    }

    /* LA MEDIDA. Con las dos direcciones delante ya no hay que suponer nada. */
    if (ed == ESP_OK && s_map_data == s_map_inst) {
        log_printf("pack: INST y DATA dan LA MISMA direccion "
                   "(D/I vaddr compartidos) — un solo mapeo vale para todo");
    } else if (ed == ESP_OK) {
        log_printf("pack: OJO — INST y DATA dan direcciones DISTINTAS. "
                   "El sello va con la de INST (@%p), que es desde donde se ejecuta. "
                   "Validar por la de DATA y saltar a la de INST exige tratarlas "
                   "aparte: NO se salta hasta que eso este escrito.", s_map_inst);
        return 0;
    } else {
        log_printf("pack: no hubo mapeo de DATOS (err=%d); se valida por el de "
                   "INST, que solo admite accesos de 4 bytes -> si la cabecera "
                   "sale rara, es esto", (int) ed);
    }

    /* V5/H7 — y AHORA el IDE puede ver la zona. Hasta este registro, `PACK_LS`
     * contestaba "sin zona de packs" y no había forma de grabar nada desde el
     * IDE en esta familia: los verbos PACK_* estaban encaminados pero sin nada
     * detrás. El puntero que hacía falta es justo el que acabamos de conseguir.
     *
     * Va aquí, y no en el arranque, porque antes del mapeo no existe. */
    board_mgr_esp32_set_packs_view(s_map_inst, s_zona_bytes);
    return 1;
}

/* V5/H7 - MAPEAR la zona, y sólo eso. Lo llama el ARRANQUE.
 *
 * Está separado de `pack_p4_cargar` desde el 16-ago porque las dos cosas que
 * hacía esa función tienen tiempos y riesgos muy distintos, y se midieron:
 *
 *   mapear  ->  0 ms en el log, y el IDE lo NECESITA desde el arranque: sin la
 *               vista publicada, `PACK_LS` contesta «sin zona de packs» y no
 *               hay forma de grabar nada desde el IDE.
 *   barrer  ->  338 ms, y es el único paso que puede COLGAR (el salto al pack).
 *
 * Así que el mapeo se queda aquí y el barrido se va al primer `Run`, que es
 * donde la Pico lo tenía desde el principio. `mapear_zona` es idempotente, así
 * que da igual quién llegue primero. */
/* V5/H7 (16-ago) - ¿HAY ALGO QUE BUSCAR? El barrido del ancla recorre la zona
 * ENTERA de 4 en 4 bytes: 2800 KB son ~700.000 comparaciones y 338 ms medidos en
 * la P4. Se pagaban en cada arranque; desde que la carga es perezosa se pagan en
 * CADA `Run` mientras no haya pack, porque sin pack la carga no se marca como
 * hecha (a propósito: así, tras grabar uno, funciona sin reiniciar).
 *
 * Se pueden no pagar, y sin tocar el buscador ni contradecir el ancla: un
 * `.npk` vive SIEMPRE dentro de un pack, así que si en la zona no hay ningún
 * pack grabado no hay ancla que encontrar. Y saberlo es barato — `bpvm_pack_scan`
 * lee la primera cabecera y para.
 *
 * ⚠️ Esto NO es «si la zona empieza virgen, no busques» metido en el buscador.
 * Esa era la idea equivocada: `test_npack.c` tiene un caso que pone el pack en
 * el offset 256 entre basura, porque el ancla existe justo para no depender de
 * dónde esté. Aquí el buscador sigue barriendo TODO lo que se le dé; lo único
 * que cambia es que no se le llama cuando se sabe que no hay nada. */
static int hay_algun_pack(const uint8_t* base, uint32_t bytes) {
    if (base == 0 || bytes == 0) return 0;
    uint32_t fin = 0;
    int n = bpvm_pack_scan(base, bytes, 0, 0, 0, &fin);
    return n > 0;
}

int32_t pack_p4_mapear(void)
{
    return mapear_zona() ? 0 : -1;
}

int32_t pack_p4_cargar(void)
{
    if (!mapear_zona()) return -1;

    if (s_pack_ram_base == 0) {
        log_printf("pack: no hay bloque de RAM para sus estaticos "
                   "(SQLite=0 en el ENV, o no cupo) — no se carga");
        return -(int32_t) BPVM_NPACK_E_TAMANO;
    }

    /* La BIOS, comprobada ANTES de prestarla: un hueco se manifestaria DENTRO
     * del pack, o sea un cuelgue donde no hay depurador. */
    const char* falta = 0;
    if (bios_p4_get() == 0) falta = "la BIOS de esta placa tiene huecos";

    /* Si no hay ni un pack grabado, no hay ancla que buscar (ver arriba). */
    if (!hay_algun_pack((const uint8_t*) s_map_inst, s_zona_bytes)) {
        log_printf("pack: la zona no tiene ningun pack grabado - no se barre");
        return -(int32_t) BPVM_NPACK_E_MAGIC;
    }

    /* Barrer + subir la escalera. La MISMA de las tres familias. */
    bpvm_npack_hallazgo_t h = bpvm_npack_buscar(
            s_map_inst, s_zona_bytes,
            (uint32_t) (uintptr_t) s_pack_ram_base, PACK_RAM_BYTES, falta);

    if (h.addr == 0) {
        /* `candidatos` separa dos cosas que se parecen y no lo son: "aqui no hay
         * pack grabado" y "hay uno pero le pasa algo". */
        if (h.candidatos == 0) {
            log_printf("pack: no hay ninguno grabado en la zona (0 candidatos)");
            return -(int32_t) h.motivo;
        }
        log_printf("pack: %u candidato(s) y ninguno vale — %s",
                   (unsigned) h.candidatos, bpvm_npack_res_str(h.motivo));

        /*
         * ─── Y AHORA LOS NÚMEROS, QUE ES LO QUE FALTABA ───
         *
         * El peldaño dice QUÉ comparación falló pero no CON QUÉ. "Otra
         * arquitectura" lo dan dos causas muy distintas: que la cabecera esté
         * bien y de verdad no cuadre, o que la cabecera se esté LEYENDO MAL.
         *
         * Así que se lee la MISMA cabecera por los DOS caminos:
         *   · por el mapeo    — como la lee la escalera
         *   · por esp_partition_read — que no pasa por la MMU y no puede mentir
         *
         * Si los dos dicen lo mismo, el mapeo es fiable y el problema son los
         * datos. Si difieren, el problema es el mapeo (accesos de 4 bytes) y la
         * escalera está juzgando basura. Un control, como manda la casa.
         */
        /* ¿DÓNDE está el candidato? No en el principio de la zona: el .npack va
         * DENTRO de un contenedor BPAK, detrás de sus cabeceras. Se busca el
         * magic en vez de suponer el offset — que es lo que hace la escalera, y
         * si aquí supusiera otro estaría comparando bytes que no son. */
        uint32_t cand = 0;
        for (uint32_t i = 0; i + sizeof(bpvm_npack_hdr_t) <= s_zona_bytes; i += 4) {
            const uint32_t* w = (const uint32_t*) (const void*)
                                ((const uint8_t*) s_map_inst + i);
            if (*w == BPVM_NPACK_MAGIC) { cand = i; break; }
        }
        log_printf("pack: el candidato esta en la zona +%u (0x%x)",
                   (unsigned) cand, (unsigned) cand);

        const bpvm_npack_hdr_t* m = (const bpvm_npack_hdr_t*)
                                    ((const uint8_t*) s_map_inst + cand);
        bpvm_npack_hdr_t d;
        const esp_partition_t* bpdata =
            (const esp_partition_t*) board_mgr_esp32_bpdata();
        const bpvm_part_t* packs = board_mgr_esp32_packs();
        int leido = (bpdata && packs &&
                     esp_partition_read(bpdata, packs->offset + cand,
                                        &d, sizeof d) == ESP_OK);

        log_printf("pack: [mapeo] magic %08x fmt %u arch %u abi '%.8s' "
                   "flash %u data %u bss %u",
                   (unsigned) m->magic, (unsigned) m->format, (unsigned) m->arch,
                   m->float_abi, (unsigned) m->flash_bytes,
                   (unsigned) m->data_bytes, (unsigned) m->bss_bytes);
        if (leido)
            log_printf("pack: [flash] magic %08x fmt %u arch %u abi '%.8s' "
                       "flash %u data %u bss %u%s",
                       (unsigned) d.magic, (unsigned) d.format, (unsigned) d.arch,
                       d.float_abi, (unsigned) d.flash_bytes,
                       (unsigned) d.data_bytes, (unsigned) d.bss_bytes,
                       (m->arch == d.arch && m->format == d.format)
                           ? "  <- IGUAL que el mapeo" : "  <<< DIFIERE DEL MAPEO");
        /* ⚠️ El sello esperado cuenta desde DONDE ESTÁ el candidato, no desde el
         * principio de la zona: el .npack va dentro de un BPAK y su cabecera
         * cae 176 bytes más allá. La primera versión de esta línea se olvidó de
         * sumar `cand` y anunciaba 0x40159040 cuando lo correcto era
         * 0x401590F0 — o sea que, el día que el sello fuera lo que falla, este
         * chivato habría mandado a buscar a la dirección equivocada. */
        log_printf("pack: esta placa ES arch %u, abi '%s' | sello esperado "
                   "flash 0x%08x ram 0x%08x",
                   (unsigned) bpvm_mdn_host_arch(), bpvm_mdn_host_float_abi(),
                   (unsigned) ((uint32_t) (uintptr_t) s_map_inst + cand
                               + BPVM_NPACK_HDR_BYTES),
                   (unsigned) (uintptr_t) s_pack_ram_base);
        log_printf("pack: el sello DEL PACK dice flash 0x%08x ram 0x%08x",
                   (unsigned) m->linked_flash, (unsigned) m->linked_ram);
        return -(int32_t) h.motivo;
    }

    const bpvm_npack_hdr_t* hdr  = (const bpvm_npack_hdr_t*) (uintptr_t) h.addr;
    uint32_t aqui_flash = h.addr + BPVM_NPACK_HDR_BYTES;

    /*
     * ─── LA RAM DEL PACK: copiar su `.data` y poner su `.bss` a cero ─────────
     *
     * Sin esto sus variables arrancan CON BASURA. Y no es teórico: se me olvidó
     * al escribir este cargador, y el pack de SQLite lo cazó con su propia
     * comprobación —`sqlite3_libversion()` no empezaba por "3."— devolviendo
     * rc=6 en vez de reventar.
     *
     * `mini` NO lo destapó porque tiene `data 0` y `bss 8`, y su entrada se
     * asigna `g_bios` ella misma. O sea: el control pasó por un camino que este
     * paso no toca. Un control descarta lo que ejercita, no más — y esto no lo
     * ejercitaba.
     *
     * El reparto viene de la cabecera y está fijado en `bpvm_npack.h`:
     *
     *     aqui_flash                    imagen de flash   (flash_bytes)
     *     aqui_flash + flash_bytes      imagen de .data   (data_bytes)
     *     s_pack_ram_base + data_bytes  la .bss, a cero   (bss_bytes)
     *
     * El peldaño 6 de la escalera ya comprobó que `data+bss` cabe en lo que
     * ofrecemos, así que aquí no hay que volver a mirarlo.
     */
    const uint8_t* data_img = (const uint8_t*) (uintptr_t) (aqui_flash + hdr->flash_bytes);
    if (hdr->data_bytes) memcpy(s_pack_ram_base, data_img, hdr->data_bytes);
    if (hdr->bss_bytes)  memset(s_pack_ram_base + hdr->data_bytes, 0, hdr->bss_bytes);

    uint32_t entry = bpvm_npack_entry_addr(hdr, aqui_flash);

    /* MIGA DE PAN. Se escribe ANTES de saltar y se vuelca: si el pack se cuelga,
     * el siguiente arranque enseña este paso SIN su pareja de despues, y eso
     * dice exactamente donde se quedo. Es lo unico que hay: ahi dentro no hay
     * depurador. */
    log_printf("pack: valido @0x%08x | .data %u B copiada a %p, .bss %u B a cero"
               " | saltando a 0x%08x",
               (unsigned) h.addr, (unsigned) hdr->data_bytes,
               (void*) s_pack_ram_base, (unsigned) hdr->bss_bytes,
               (unsigned) entry);
    log_flush();

    int32_t (*init)(const bpvm_bios_t*) =
        (int32_t (*)(const bpvm_bios_t*)) (uintptr_t) entry;
    int32_t rc = init(bios_p4_get());

    log_printf("pack: volvio, rc=%d", (int) rc);
    return rc;
}
