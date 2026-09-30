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

/*
 * ─── POR QUÉ UN MOTOR NO SE CARGA, CON LOS NÚMEROS ───
 *
 * El peldaño dice QUÉ comparación falló pero no CON QUÉ. "Otra arquitectura"
 * lo dan dos causas muy distintas: que la cabecera esté bien y de verdad no
 * cuadre, o que la cabecera se esté LEYENDO MAL. Así que, para los peldaños que
 * un mapeo roto produciría, se lee la MISMA cabecera por los DOS caminos:
 *   · por el mapeo           — como la lee la escalera
 *   · por esp_partition_read — que no pasa por la MMU y no puede mentir
 * Si los dos dicen lo mismo, el mapeo es fiable y el problema son los datos. Si
 * difieren, el problema es el mapeo (accesos de 4 bytes) y la escalera está
 * juzgando basura. Un control, como manda la casa.
 */
static void explicar(const bpvm_npack_carga_t* c, int i, const bpvm_npack_carga_t* todos,
                     const uint8_t* zbase)
{
    if (c->eclipsado_por >= 0) {
        log_printf("pack: [%d] '%s' (motor '%s') NO se carga: hay otro motor '%s' mas adelante "
                   "(el pack '%s', grabado despues) y gana el ultimo, como los .mod — borra el viejo",
                   i, c->pack, c->npk, c->npk, todos[c->eclipsado_por].pack);
        return;
    }
    if (c->pisa_a >= 0) {
        const bpvm_npack_carga_t* o = &todos[c->pisa_a];
        log_printf("pack: [%d] '%s' NO se carga: su RAM [0x%08x, +%u) pisa la de '%s' [0x%08x, +%u) "
                   "— se grabo con una imagen de antes de G1 (daba la base a todos): regrabalo",
                   i, c->pack, (unsigned) c->ram, (unsigned) c->ram_bytes,
                   o->pack, (unsigned) o->ram, (unsigned) o->ram_bytes);
        return;
    }
    log_printf("pack: [%d] '%s' (motor '%s') NO se carga — %s",
               i, c->pack, c->npk, bpvm_npack_res_str(c->motivo));
    if (c->hdr == 0) return;
    const bpvm_npack_hdr_t* m = c->hdr;

    if (c->motivo == BPVM_NPACK_E_SELLO) {
        log_printf("pack: [%d] su sello dice flash 0x%08x ram 0x%08x | aqui esta en flash 0x%08x "
                   "y el bloque de RAM es [0x%08x, +%u)",
                   i, (unsigned) m->linked_flash, (unsigned) m->linked_ram, (unsigned) c->codigo,
                   (unsigned) (uintptr_t) s_pack_ram_base, (unsigned) s_pack_ram_size);
        return;
    }
    if (c->motivo == BPVM_NPACK_E_TAMANO) {
        log_printf("pack: [%d] pide .data %u + .bss %u B desde 0x%08x; el bloque acaba en 0x%08x",
                   i, (unsigned) m->data_bytes, (unsigned) m->bss_bytes, (unsigned) m->linked_ram,
                   (unsigned) ((uintptr_t) s_pack_ram_base + s_pack_ram_size));
        return;
    }
    if (c->motivo != BPVM_NPACK_E_MAGIC && c->motivo != BPVM_NPACK_E_FORMAT &&
        c->motivo != BPVM_NPACK_E_ARCH  && c->motivo != BPVM_NPACK_E_ABI) return;

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
    log_printf("pack: esta placa ES arch %u, abi '%s'",
               (unsigned) bpvm_mdn_host_arch(), bpvm_mdn_host_float_abi());
}

/*
 * ─── SALTAR A UN MOTOR ───
 *
 * Su RAM: copiar su `.data` y poner su `.bss` a cero, EN SU TROZO (el de su
 * sello). Sin esto sus variables arrancan CON BASURA. Y no es teórico: se me
 * olvidó al escribir este cargador, y el pack de SQLite lo cazó con su propia
 * comprobación —`sqlite3_libversion()` no empezaba por "3."— devolviendo rc=6
 * en vez de reventar. `mini` NO lo destapó porque tiene `data 0` y `bss 8`, y
 * su entrada se asigna `g_bios` ella misma: un control descarta lo que
 * ejercita, no más.
 *
 * El reparto viene de la cabecera y está fijado en `bpvm_npack.h`:
 *
 *     codigo                   imagen de flash   (flash_bytes)
 *     codigo + flash_bytes     imagen de .data   (data_bytes)
 *     ram + data_bytes         la .bss, a cero   (bss_bytes)
 *
 * La escalera ya comprobó que `data+bss` cabe desde `ram` hasta el final del
 * bloque, y el plan que no pisa a otro motor: aquí no hay que volver a mirarlo.
 */
static int32_t saltar(const bpvm_npack_carga_t* c, int i)
{
    const bpvm_npack_hdr_t* hdr = c->hdr;
    uint8_t* ram = (uint8_t*) (uintptr_t) c->ram;
    const uint8_t* data_img = (const uint8_t*) (uintptr_t) (c->codigo + hdr->flash_bytes);
    if (hdr->data_bytes) memcpy(ram, data_img, hdr->data_bytes);
    if (hdr->bss_bytes)  memset(ram + hdr->data_bytes, 0, hdr->bss_bytes);

    uint32_t entry = bpvm_npack_entry_addr(hdr, c->codigo);
    /* V7/H6: la vista MONTADA es la de lectura; se salta por la de ejecución.
     * Medido (9-sep): en S3, C3, C6 y P4 son la misma dirección, así que hoy
     * esto es la identidad — y el día que un micro las separe, ya está. */
    entry = (uint32_t) (uintptr_t) bpvm_pack_exec_ptr((const uint8_t*) (uintptr_t) entry);

    /* MIGA DE PAN. Se escribe ANTES de saltar y se vuelca: si el motor se
     * cuelga, el siguiente arranque enseña este paso SIN su pareja de despues, y
     * eso dice exactamente donde se quedo. Es lo unico que hay: ahi dentro no
     * hay depurador. */
    log_printf("pack: [%d] '%s' (motor '%s') valido @0x%08x | .data %u B a %p, .bss %u B a cero"
               " | saltando a 0x%08x",
               i, c->pack, c->npk, (unsigned) (c->codigo - BPVM_NPACK_HDR_BYTES),
               (unsigned) hdr->data_bytes, (void*) ram, (unsigned) hdr->bss_bytes,
               (unsigned) entry);
    log_flush();

    /* La arena, con dueño: sólo se la puede llevar desde su entrada. */
    bpvm_bios_arena_entra(c->pack);
    int32_t (*init)(const bpvm_bios_t*) =
        (int32_t (*)(const bpvm_bios_t*)) (uintptr_t) entry;
    int32_t rc = init(bios_esp32_get());
    bpvm_bios_arena_sale();

    const char* no = bpvm_bios_arena_negada();
    log_printf("pack: [%d] '%s' volvio, rc=%d%s%s", i, c->pack, (int) rc,
               no ? " | pidio la arena y se le nego: " : "", no ? no : "");
    return rc;
}

int32_t pack_esp32_cargar(void)
{
    /* La zona sale de la vista MONTADA, que la pone el mapeo comun. Aqui es un
     * puntero de LECTURA: la escalera y el plan leen por él, y el salto se
     * traduce a la vista ejecutable. */
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

    /* Si no hay ni un pack grabado, no hay cadena que recorrer. */
    if (!hay_algun_pack(zbase, zbytes)) {
        log_printf("pack: la zona no tiene ningun pack grabado - no se recorre");
        return -(int32_t) BPVM_NPACK_E_MAGIC;
    }

    /* EL PLAN: la cadena, en orden, con el veredicto de cada motor. La MISMA
     * regla en las familias, y la misma que usa `PACK_BURN_BEGIN` para decirle
     * al IDE dónde sellar el siguiente. */
    uint32_t ram_base = (uint32_t) (uintptr_t) s_pack_ram_base;
    bpvm_npack_carga_t c[BPVM_NPACK_MAX];
    bpvm_npack_plan_t  p;
    int vistos = bpvm_npack_planear(zbase, zbytes, ram_base, s_pack_ram_size, falta,
                                    c, BPVM_NPACK_MAX, &p);
    int n = (vistos < BPVM_NPACK_MAX) ? vistos : BPVM_NPACK_MAX;
    if (vistos == 0) {
        log_printf("pack: hay packs grabados pero ninguno trae motor nativo (npk)");
        return -(int32_t) BPVM_NPACK_E_MAGIC;
    }
    uint32_t usada = p.ram_tope - ram_base;
    log_printf("pack: %d motor(es) en la zona, %d cargable(s) | bloque %u KB @%p: "
               "estaticos %u B, arena %u KB",
               vistos, p.cargables, (unsigned) (s_pack_ram_size / 1024u),
               (void*) s_pack_ram_base, (unsigned) usada,
               (unsigned) ((s_pack_ram_size - usada) / 1024u));
    if (vistos > n)
        log_printf("pack: hay %d motores y solo se miran los %d primeros (BPVM_NPACK_MAX)",
                   vistos, n);

    /* La arena: lo que queda por encima del último motor. Se fija ANTES de
     * saltar a ninguno — y con ella se olvida cualquier dueña anterior. */
    bpvm_bios_arena_fija(s_pack_ram_base + usada, s_pack_ram_size - usada);

    int saltados = 0;
    bpvm_npack_res_t ultimo = BPVM_NPACK_E_MAGIC;
    for (int i = 0; i < n; i++) {
        if (!bpvm_npack_cargable(&c[i])) {
            explicar(&c[i], i, c, zbase);
            if (c[i].eclipsado_por < 0)
                ultimo = (c[i].pisa_a >= 0) ? BPVM_NPACK_E_SELLO : c[i].motivo;
            continue;
        }
        (void) saltar(&c[i], i);
        saltados++;
    }
    /* A cuántos se saltó (lo que devolvió cada uno ya está en el log), o el
     * peldaño del último rechazado: el que llama sólo necesita saber si se
     * saltó a ALGUNO — volver a saltar republicaría sus marcas. */
    return saltados > 0 ? saltados : -(int32_t) ultimo;
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
