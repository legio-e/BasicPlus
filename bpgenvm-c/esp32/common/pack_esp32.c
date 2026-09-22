/* pack_esp32.c — cargador del pack nativo en la FAMILIA ESP32 (V5/H7 en el P4;
 * V7/H6 P1 al común: S3, C3, C6, C5 y P4 con el MISMO cargador).
 *
 * Hermano de `pico/pack_pico.c`. Lo que cambia es CÓMO se llega al código:
 *
 *   Pico    la flash está XIP en una dirección FIJA -> el pack se lee y se
 *           ejecuta donde está, sin más.
 *   ESP32   hay que MAPEAR la zona, y la dirección la asigna la MMU en tiempo
 *           de ejecución -> nadie puede saberla desde el PC. El mapeo es
 *           común (`board_mgr_esp32_mapear_packs`) y da una vista de lectura
 *           y otra de ejecución (medido el 9-sep: en los cuatro micros son la
 *           misma; si un día no lo fueran, `bpvm_pack_exec_ptr` traduce).
 *
 * Lo demás —la escalera, el sello, el salto— es el MISMO código común. No hay
 * una copia local de `bpvm_npack_check` a propósito: esa regla no puede
 * divergir entre familias.
 *
 * Y aquí vive también el BLOQUE DE RAM del código nativo (`pack_esp32_ram_init`),
 * que en el P4 estaba en su `main.c` como «el bloque de la BD»: es la misma
 * reserva para cualquier motor, y ahora la hacen los cinco micros.
 */
#include "pack_esp32.h"
#include "bpvm_npack.h"
#include "bpvm_pack.h"   /* V5/H7: saber si hay packs antes de barrer */
#include "bpvm_bios.h"
#include "bpvm_part.h"
#include "mdn_loader.h"   /* bpvm_mdn_host_arch / _float_abi: lo que ESTA placa es */
#include "board_mgr_esp32.h"
#include "log.h"

#include "esp_partition.h"
#include "esp_heap_caps.h"   /* el bloque de RAM: PSRAM si la hay, si no SRAM */
#include "bpvm_env.h"        /* packRam= / SQLite= del ENV */
#include "bpvm_sqlmem.h"     /* la regla medida de la arena de la BD */

#include <stdint.h>
#include <string.h>   /* memcpy/memset: la .data y la .bss del pack */

/*
 * La base de la RAM del pack. La rellena el arranque (`main.c`) con el principio
 * del bloque de la BD. 0 = no hay bloque, o sea que tampoco hay dónde poner los
 * estáticos del pack.
 */
uint8_t* s_pack_ram_base = 0;
uint32_t s_pack_ram_size = 0;
int      s_pack_ram_res  = 0;   /* bpvm_sqlite_res_t */

/* Los mapeos se hacen UNA vez y se quedan: soltarlos invalidaría el código que
 * el pack deja publicado (SQLite publica su tabla de funciones y la VM la usa
 * durante toda la sesión). */

/*
 * Mapea la zona de packs de las dos formas y deja dicho qué salió.
 * Devuelve 1 si se puede seguir (un mapeo utilizable), 0 si no.
 */
/* El mapeo se fue a `esp32/common/board_mgr_esp32.c` el 8-sep. No porque sobrara
 * aqui, sino porque nunca fue del P4: lo unico que cambia entre micros es si la
 * MMU da una direccion o dos, y eso no es una familia, es una capacidad. Mientras
 * vivio en este fichero, el C6 —que comparte D/I vaddr igual que el P4— se quedo
 * sin packs por vivir en el directorio de al lado. Correccion de Eduardo.
 *
 * Se conserva el nombre local para no tocar a sus tres llamantes de aqui. */
static int mapear_zona(void)
{
    return board_mgr_esp32_mapear_packs();
}

static int hay_algun_pack(const uint8_t* base, uint32_t bytes) {
    if (base == 0 || bytes == 0) return 0;
    uint32_t fin = 0;
    int n = bpvm_pack_scan(base, bytes, 0, 0, 0, &fin);
    return n > 0;
}

int32_t pack_esp32_mapear(void)
{
    return mapear_zona() ? 0 : -1;
}

int32_t pack_esp32_cargar(void)
{
    /* La zona sale de la vista MONTADA, que la pone el mapeo comun. Aqui es un
     * puntero de LECTURA y nada mas: el npack se copia a RAM y se ejecuta desde
     * RAM (la escalera), asi que no hace falta la vista ejecutable. */
    uint32_t       zbytes = 0;
    const uint8_t* zbase  = bpvm_pack_mounted(&zbytes);
    if (!zbase || zbytes == 0) { if (!mapear_zona()) return -1;
                                 zbase = bpvm_pack_mounted(&zbytes); }
    if (!mapear_zona()) return -1;

    if (s_pack_ram_base == 0) {
        log_printf("pack: no hay bloque de RAM para sus estaticos "
                   "(ni packRam ni SQLite en el ENV, o no cupo) — no se carga");
        return -(int32_t) BPVM_NPACK_E_TAMANO;
    }

    /* La BIOS, comprobada ANTES de prestarla: un hueco se manifestaria DENTRO
     * del pack, o sea un cuelgue donde no hay depurador. */
    const char* falta = 0;
    if (bios_esp32_get() == 0) falta = "la BIOS de esta placa tiene huecos";

    /* Si no hay ni un pack grabado, no hay ancla que buscar (ver arriba). */
    if (!hay_algun_pack(zbase, zbytes)) {
        log_printf("pack: la zona no tiene ningun pack grabado - no se barre");
        return -(int32_t) BPVM_NPACK_E_MAGIC;
    }

    /* Barrer + subir la escalera. La MISMA de las tres familias. */
    bpvm_npack_hallazgo_t h = bpvm_npack_buscar(
            zbase, zbytes,
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
        for (uint32_t i = 0; i + sizeof(bpvm_npack_hdr_t) <= zbytes; i += 4) {
            const uint32_t* w = (const uint32_t*) (const void*)
                                (zbase + i);
            if (*w == BPVM_NPACK_MAGIC) { cand = i; break; }
        }
        log_printf("pack: el candidato esta en la zona +%u (0x%x)",
                   (unsigned) cand, (unsigned) cand);

        const bpvm_npack_hdr_t* m = (const bpvm_npack_hdr_t*)
                                    (zbase + cand);
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
                   (unsigned) ((uint32_t) (uintptr_t) zbase + cand
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
    /* V7/H6: la vista MONTADA es la de lectura; se salta por la de ejecución.
     * Medido (9-sep): en S3, C3, C6 y P4 son la misma dirección, así que hoy
     * esto es la identidad — y el día que un micro las separe, ya está. */
    entry = (uint32_t) (uintptr_t) bpvm_pack_exec_ptr((const uint8_t*) (uintptr_t) entry);

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
    int32_t rc = init(bios_esp32_get());

    log_printf("pack: volvio, rc=%d", (int) rc);
    return rc;
}

/*
 * ─── EL BLOQUE DE RAM DEL CÓDIGO NATIVO (V7/H6 P1; era `vm_sqlite_init_psram` del P4) ───
 *
 * ANTES que el heap de la VM, para que la dirección sea la MISMA en cada
 * arranque: es contra la que el pack está sellado. Ver `pack_esp32.h`.
 *
 * ALINEADO A PÁGINA, y las dos razones son distintas:
 *
 * 1. CORRECCIÓN. Las secciones `.data`/`.sdata` del pack piden alineación 8
 *    (lo dice su ELF) y ahí viven estructuras con `double` y enteros de 64
 *    bits. `heap_caps_malloc` devuelve alineado a 4: el primer arranque del P4
 *    dio 0x48000a7c, que NO es múltiplo de 8.
 *
 * 2. ESTABILIDAD DEL SELLO, que es lo que de verdad importa. Ese 0xa7c de
 *    desplazamiento es de lo que la IDF reservara ANTES que nosotros; si un
 *    día eso crece o mengua, nuestra dirección se mueve y el pack grabado deja
 *    de valer. Pidiendo página (4 KB), el alocador redondea hacia arriba y una
 *    variación pequeña de lo que hay delante YA NO NOS MUEVE — sólo saltaría de
 *    página si el cambio pasara de 4 KB. No es una garantía absoluta (no existe
 *    con un alocador de por medio), pero convierte "frágil ante cualquier byte"
 *    en "estable salvo cambio grande". Y si aun así se moviera, el SELLO lo caza.
 *
 * Se dice en el log SIEMPRE y con el motivo, porque "no se pidió", "se pidió
 * poco" y "no cupo" mandan a sitios distintos.
 */
void pack_esp32_ram_init(void)
{
    board_mgr_esp32_env_temprano();
    long pack_kb  = bpvm_env_get_long(board_mgr_env(), "packRam", 0);
    long sql_mb   = bpvm_env_get_long(board_mgr_env(), "SQLite", 0);

    /* De dónde: la PSRAM si la hay (S3, C5, P4); si no, la SRAM interna. */
    size_t   libre_ps = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    uint32_t caps     = libre_ps ? MALLOC_CAP_SPIRAM : (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t   libre    = libre_ps ? libre_ps : heap_caps_get_free_size(caps);
    const char* fuente = libre_ps ? "PSRAM" : "SRAM";

    size_t bytes = 0;
    if (pack_kb > 0) {
        /* La clave genérica: lo que se pide, redondeado a página, dejando a la
         * VM su mínimo. Sin la regla de la BD (2 MB, ~40 %): ésa es de SQLite. */
        bytes = ((size_t) pack_kb * 1024u + 4095u) & ~(size_t) 4095u;
        if (bytes + BPVM_VM_MIN_BYTES > libre) {
            s_pack_ram_res = (int) BPVM_SQLITE_NO_CABE;
            log_printf("packs: SIN bloque de RAM — packRam=%ld KB no cabe dejando la VM viable (%s libre %u KB)",
                       pack_kb, fuente, (unsigned) (libre / 1024u));
            return;
        }
        s_pack_ram_res = (int) BPVM_SQLITE_OK;
    } else {
        bpvm_sqlite_res_t r = bpvm_sqlite_region(sql_mb, libre, &bytes);
        s_pack_ram_res = (int) r;
        if (r != BPVM_SQLITE_OK) {
            if (r == BPVM_SQLITE_OFF)
                log_printf("packs: sin bloque de RAM (ni packRam= ni SQLite= en el ENV): un pack nativo no podría cargar");
            else
                log_printf("packs: SIN bloque de RAM — %s (SQLite=%ld en el ENV, %s libre %u KB)",
                           bpvm_sqlite_res_str(r), sql_mb, fuente, (unsigned) (libre / 1024u));
            return;
        }
    }

    s_pack_ram_base = (uint8_t*) heap_caps_aligned_alloc(4096, bytes, caps);
    if (s_pack_ram_base == NULL) {
        /* Pedirlo y no conseguirlo NO se traga: la regla dijo que cabía y el
         * alocador dice que no, y esa discrepancia hay que verla. La VM sigue
         * arrancando (sin pack), que es mejor que abortar por una función opcional. */
        s_pack_ram_res = (int) BPVM_SQLITE_NO_CABE;
        log_printf("packs: la regla daba %u KB pero heap_caps_aligned_alloc DIJO QUE NO "
                   "(%s libre %u KB) — se sigue SIN bloque",
                   (unsigned) (bytes / 1024u), fuente, (unsigned) (libre / 1024u));
        return;
    }
    s_pack_ram_size = (uint32_t) bytes;
    log_printf("packs: bloque de RAM %u KB @%p en %s (packRam=%ld KB, SQLite=%ld MB) | estaticos %u B delante, arena %u KB",
               (unsigned) (bytes / 1024u), (void*) s_pack_ram_base, fuente, pack_kb, sql_mb,
               (unsigned) PACK_RAM_BYTES, (unsigned) ((bytes - PACK_RAM_BYTES) / 1024u));
}
