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
 * Lo demás —la escalera, el sello, el plan, el salto— es el MISMO código común.
 * No hay una copia local de `bpvm_npack_escalera` ni de `bpvm_npack_planear` a
 * propósito: esas reglas no pueden divergir entre familias.
 *
 * Y aquí vive también el BLOQUE DE RAM del código nativo (`pack_esp32_ram_init`),
 * que en el P4 estaba en su `main.c` como «el bloque de la BD»: es la misma
 * reserva para cualquier motor, y ahora la hacen los cinco micros.
 *
 * ─── V7/H6.c G1: TODOS LOS MOTORES, NO EL PRIMERO ───
 *
 * Hasta G1 se cargaba UN motor —el primero que aparecía barriendo la zona— y el
 * bloque de RAM entero era suyo. Ahora se recorre la cadena y se salta a todos
 * los que el plan da por buenos, en el orden en que se grabaron, cada uno a su
 * trozo de RAM (el que dice su sello); la arena es lo que queda por encima.
 */
#include "pack_esp32.h"
#include "bpvm_npack.h"
#include "bpvm_pack.h"   /* V5/H7: saber si hay packs antes de recorrer */
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
#include <string.h>   /* memcpy/memset: la .data y la .bss de cada motor */

/*
 * El bloque de RAM del código nativo. Lo rellena el arranque (`main.c`, vía
 * `pack_esp32_ram_init`). 0 = no hay bloque, o sea que tampoco hay dónde poner
 * los estáticos de ningún motor.
 */
uint8_t* s_pack_ram_base = 0;
uint32_t s_pack_ram_size = 0;
int      s_pack_ram_res  = 0;   /* bpvm_sqlite_res_t */

/*
 * ─── V7/H7 (Boot 2, fase 2): EL CARGADOR ES COMÚN ───
 *
 * El plan, la arena, la `.data` y la `.bss`, la miga de pan, el salto por la vista de
 * ejecución y la guarda «una vez» son `src/bpvm_npack_carga.c`: la misma copia estaba en las
 * tres familias. La zona la montó el arranque (fase 1: con el FS). Aquí quedan los tres datos
 * de esta placa y lo único que de verdad es suyo: el control de abajo.
 *
 * ─── POR QUÉ UN MOTOR NO SE CARGA, CON LOS NÚMEROS (el control de la ESP32) ───
 *
 * El peldaño dice QUÉ comparación falló pero no CON QUÉ. "Otra arquitectura" lo dan dos
 * causas muy distintas: que la cabecera esté bien y de verdad no cuadre, o que la cabecera se
 * esté LEYENDO MAL. Así que, para los peldaños que un mapeo roto produciría, se lee la MISMA
 * cabecera por los DOS caminos:
 *   · por el mapeo           — como la lee la escalera
 *   · por esp_partition_read — que no pasa por la MMU y no puede mentir
 * Si los dos dicen lo mismo, el mapeo es fiable y el problema son los datos. Si difieren, el
 * problema es el mapeo (accesos de 4 bytes) y la escalera está juzgando basura. Un control,
 * como manda la casa. (El arch y la ABI de la placa ya los dice el común.)
 */
static void cabecera_por_dos_caminos(const bpvm_npack_carga_t* c, int i, const uint8_t* zbase)
{
    if (c->hdr == 0 || c->eclipsado_por >= 0 || c->pisa_a >= 0) return;
    if (c->motivo != BPVM_NPACK_E_MAGIC && c->motivo != BPVM_NPACK_E_FORMAT &&
        c->motivo != BPVM_NPACK_E_ARCH  && c->motivo != BPVM_NPACK_E_ABI) return;
    const bpvm_npack_hdr_t* m = c->hdr;
    uint32_t off = (uint32_t) ((const uint8_t*) m - zbase);
    bpvm_npack_hdr_t d;
    const esp_partition_t* bpdata = (const esp_partition_t*) board_mgr_esp32_bpdata();
    const bpvm_part_t* packs = board_mgr_esp32_packs();
    int leido = (bpdata && packs &&
                 esp_partition_read(bpdata, packs->offset + off, &d, sizeof d) == ESP_OK);
    log_printf("pack: [%d] [mapeo] magic %08x fmt %u arch %u abi '%.8s' flash %u data %u bss %u",
               i, (unsigned) m->magic, (unsigned) m->format, (unsigned) m->arch,
               m->float_abi, (unsigned) m->flash_bytes,
               (unsigned) m->data_bytes, (unsigned) m->bss_bytes);
    if (leido)
        log_printf("pack: [%d] [flash] magic %08x fmt %u arch %u abi '%.8s' flash %u data %u bss %u%s",
                   i, (unsigned) d.magic, (unsigned) d.format, (unsigned) d.arch,
                   d.float_abi, (unsigned) d.flash_bytes,
                   (unsigned) d.data_bytes, (unsigned) d.bss_bytes,
                   (m->arch == d.arch && m->format == d.format)
                       ? "  <- IGUAL que el mapeo" : "  <<< DIFIERE DEL MAPEO");
}

static void log_linea(const char* l) { log_printf("%s", l); }

int32_t pack_esp32_cargar(void)
{
    bpvm_npack_familia_t f;
    memset(&f, 0, sizeof f);
    f.bios         = bios_esp32_get();
    f.ram          = s_pack_ram_base;   /* la reserva de pack_esp32_ram_init, antes del heap */
    f.ram_bytes    = s_pack_ram_size;
    f.log          = log_linea;
    f.flush        = log_flush;
    f.explicar_mas = cabecera_por_dos_caminos;
    return bpvm_npack_cargar_zona(&f);
}

/*
 * ─── EL BLOQUE DE RAM DEL CÓDIGO NATIVO (V7/H6 P1; era `vm_sqlite_init_psram` del P4) ───
 *
 * ANTES que el heap de la VM, para que la dirección sea la MISMA en cada
 * arranque: es contra la que los motores están sellados. Ver `pack_esp32.h`.
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
    /* V7/H6.c G1: el reparto [estáticos de cada motor | arena] ya no se sabe
     * aquí — depende de qué motores haya grabados, y lo dice el cargador. */
    log_printf("packs: bloque de RAM %u KB @%p en %s (packRam=%ld KB, SQLite=%ld MB) — "
               "los estaticos de cada motor y la arena se reparten al cargar",
               (unsigned) (bytes / 1024u), (void*) s_pack_ram_base, fuente, pack_kb, sql_mb);
}
