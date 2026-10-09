/*
 * board_mgr_esp32.c — H9: adaptador de gestión de placa de la familia ESP32.
 * Cintura del env A/B sobre la partición vendor "bpenv" (esp_partition) + arranque
 * escalonado + board_boot_status + ramo STATE/ENV_x/PART_x del wire. Ver el .h.
 *
 * Modelo de particiones (Eduardo 19-jul): la tabla vendor fija la ZONA DE DATOS
 * entera = partición "bpdata"; el LÍMITE FS|Packs vive en el env (un mando, FS es la
 * knob, Packs el resto). bpvm_part opera con base=0 (relativo a bpdata) y
 * usable=bpdata->size → el mismo núcleo que el RP2350, solo cambia de dónde salen.
 */
#include "board_mgr_esp32.h"
#include "bpvm_pack.h"   /* V5/H7: bpvm_pack_flash_t, la cintura de escritura */

#include "bpvm.h"        /* #338: bpvm_scratch_take/give (zona de rascar compartida) */
#include "bpvm_bmgr.h"
#include "bpvm_bmgr_wire.h"
#include "bpvm_repl.h"   /* V7/H10: el handle común (bpvm_bmgr_handle) */
#include "bpvm_part.h"
#include "bpvm_env.h"
#include "wire_v1.h"
#include "fs.h"
#include "log.h"
#include "esp32_mods.h"   /* V6/U4: la tabla GENERADA de la stdlib embebida (sólo datos) */
#include "bpvm_mods.h"    /* V6/U4: el bucle y la regla, comunes a las tres familias */
#include "pack_esp32.h"   /* V5/H8: s_pack_ram_base — la RAM del motor nativo (V7/H6: común, sin #if P4) */

#include "esp_partition.h"
#include "esp_flash.h"    /* #328 — el tamaño REAL del chip, no el que dice la tabla */
#include "esp_flash_chips/esp_flash_types.h"   /* V7/H7: el `size` del chip, para subirlo al físico (FS arriba) */
#include "esp_log.h"
#include "esp_mmu_map.h"  /* #676: esp_mmu_paddr_to_vaddr — la direccion de DATOS de unas paginas ya mapeadas */
#include "esp_cache.h"    /* #676: esp_cache_msync — invalidar esa vista tras escribir la flash */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define BP_ENV_SECTOR   4096u   /* sector de borrado de la flash SPI */

/* El techo del mapeo de la flash: `spi_flash_mmap` (IDF) no pasa de 24 bits salvo
 * con la opcion experimental de 32. Ver `board_mgr_esp32_mapear_packs`. */
#if CONFIG_BOOTLOADER_CACHE_32BIT_ADDR_QUAD_FLASH || CONFIG_BOOTLOADER_CACHE_32BIT_ADDR_OCTAL_FLASH
#define PACKS_MMAP_24BIT 0
#else
#define PACKS_MMAP_24BIT 1
#endif

/* V7/H7 (30-sep) — lo mínimo que tiene que haber por encima de la tabla para que el FS vaya allí. Con nuestras tablas
 * (bpdata llega a la flash que declara la imagen) es 0 o una potencia de dos (16 MB en la Waveshare y en la C5); esto
 * sólo evita que una cola suelta dejara el FS en nada. */
#define FS_ARRIBA_MIN   (1024u * 1024u)

static const char* TAG = "board_mgr";

/* Particiones vendor (localizadas una vez en board_mgr_esp32_boot). */
static const esp_partition_t* s_bpenv  = NULL;   /* env A/B (sectores 0 y 1) */
static const esp_partition_t* s_bpdata = NULL;   /* zona de datos (FS + Packs) */
/* V7/H7 (30-sep) — la flash que el CHIP tiene por encima de la tabla, registrada como partición «bpfsalto» cuando
 * la hay (board_mgr_esp32_boot): entonces el FS es ella, entera, y bpdata es sólo de packs. NULL = lo de siempre. */
static const esp_partition_t* s_bpfs_alto = NULL;

/* Estado del arranque escalonado + el env/layout del climb (estáticos: el layout
 * debe sobrevivir para que layer_fs lo use; el boot lo lee board_boot_status). */
static bpvm_boot_status_t s_boot;
static bpvm_part_layout_t s_layout;
/* V7/H10, el board_mgr común: el ESPEJO de las dos copias del ENV (A y B, seguidas). `bpenv` no se puede leer por
 * puntero —mapearla gasta espacio virtual, y en la C5 no queda—, así que la vista común las copia aquí y las vuelve a
 * leer de la flash tras cada escritura (D1). Son los mismos 8 KB que antes eran s_env_a y s_env_b. */
static uint8_t s_env_espejo[2u * BP_ENV_SECTOR];

/* #311 — el ENV, para que lo consulte quien configure hardware (hoy el panel del P4). Antes se parseaba en una local
 * de layer_partitions y se tiraba, así que la única config de placa accesible luego era /sys/board.json —un fichero
 * DENTRO del FS, que se pierde al formatear—. El env está en su partición, sobrevive al reflasheo y ya está disponible
 * en el estado 2. V7/H10 (D1): es la vista común, la de la FLASH —un ENV_SET se ve sin reiniciar—; el hardware ya
 * configurado no cambia por eso: CUÁNDO entra en efecto una clave lo decide quien la lee (el panel, al arrancar). */
static void asegura_familia(void);

/* La memoria de la VM la definen los main.c (S3 array SRAM / P4 puntero PSRAM). */
extern uint8_t* s_vm_buffer;
extern uint32_t s_vm_buffer_size;

const bpvm_boot_status_t* board_boot_status(void) { return &s_boot; }

const bpvm_env_t* board_mgr_env(void) {
    asegura_familia();
    return bpvm_bmgr_vista();
}

/*
 * ─── V5/H7 — LA ZONA DE PACKS, POR FIN CABLEADA EN ESTA FAMILIA ──────────────
 *
 * Hasta hoy `PACK_LS` contestaba "sin zona de packs" en los DOS ESP32, y no era
 * un fallo de configuración de nadie: `bm.packs_*` no se rellenaba. Los verbos
 * PACK_* estaban encaminados pero sin nada detrás — la media función de
 * [[portar-familia-adaptador-completo]], que ya mordió en el Pico (#327) y que
 * su propio comentario describe: *"encaminar los verbos y no rellenar su
 * petición es la misma media función que dejar bm.packs_* a cero"*. Aquí
 * faltaban LAS DOS mitades.
 *
 * ─── POR QUÉ LA VISTA SE REGISTRA Y NO SE CALCULA AQUÍ ───
 *
 * Leer la zona necesita un PUNTERO que la CPU pueda seguir, y ahí las dos
 * familias no se parecen: el P4 tiene que MAPEARLA (`esp_partition_mmap`, y la
 * dirección la asigna la MMU en runtime), mientras que el S3 hará lo suyo
 * cuando le toque. Calcularlo aquí obligaría a este fichero —que es común— a
 * saberse el mapeo de cada micro.
 *
 * Así que la familia la REGISTRA cuando la tiene. Y el que no la registre se
 * queda como estaba: `packs_base` a NULL y `PACK_LS` diciendo "sin zona de
 * packs", que para el S3 hoy es LA VERDAD. Un `#ifdef` habría dado el mismo
 * resultado escondiendo que una familia está a medias; esto lo deja a la vista.
 */
static const uint8_t* s_packs_view      = 0;   /* por donde se LEE la zona */
static const uint8_t* s_packs_view_exec = 0;   /* por donde se SALTA (== view salvo en la S3: #676) */
static uint32_t       s_packs_view_size = 0;

void board_mgr_esp32_set_packs_view(const void* base_lectura,
                                    const void* base_ejecucion, uint32_t size) {
    s_packs_view      = (const uint8_t*) base_lectura;
    s_packs_view_exec = (const uint8_t*) (base_ejecucion ? base_ejecucion : base_lectura);
    s_packs_view_size = size;

    /* ─── Y MONTARLA, que son DOS consumidores, no uno ────────────────────
     *
     * Con la vista registrada, el IDE ya puede listar y grabar. Pero la VM
     * busca los módulos (`mod`) y los puentes (`mdn`) del pack por otro sitio
     * —`bpvm_pack_mounted()`—, y si nadie ha montado, ahí no hay nada que
     * encontrar. Son la misma dirección para dos usos distintos.
     *
     * ⚠️ Esto FALTABA en la familia ESP32 y no era una regresión: el camino
     * «cargar un módulo desde la zona» no se había ejercitado nunca aquí. El
     * pack nativo de V5/H8 sí corría, porque a ése se le encuentra BARRIENDO
     * la zona, que no necesita montaje. Se vio con el caso mínimo de Eduardo
     * (13-ago): un pack con UN módulo dentro y `falta el modulo 'mod1'`,
     * mientras el mismo pack en host iba. En la Pico ya se había cazado igual
     * — ver el aviso gemelo en `pico/pack_pico.c`.
     *
     * Va AQUÍ y no en `pack_p4.c` a propósito: éste es el punto por el que
     * pasa cualquier familia que consiga su puntero, así que el S3 lo tendrá
     * el día que registre el suyo sin que nadie se acuerde de esta línea.
     *
     * Y va pase lo que pase con el código nativo: un pack de sólo módulos,
     * sin `npk`, tiene que valer igual. */
    bpvm_pack_mount(s_packs_view, s_packs_view_exec, s_packs_view_size);
    if (s_packs_view_exec == s_packs_view) {
        log_printf("pack: zona montada en %p (%u KB) — modulos y .mdn visibles",
                   (const void*) s_packs_view, (unsigned) (s_packs_view_size / 1024u));
    } else {
        log_printf("pack: zona montada — lectura @%p, ejecucion @%p (%u KB); "
                   "este micro NO comparte direccion virtual para datos e instrucciones",
                   (const void*) s_packs_view, (const void*) s_packs_view_exec,
                   (unsigned) (s_packs_view_size / 1024u));
    }
}

/* ── EL MAPEO DE LA ZONA, COMUN A LAS CUATRO ESP32 ─────────────────────────────
 *
 * Vivia en `esp32p4/main/pack_p4.c` y por eso el C6 no tenia packs: no por el
 * silicio —declara `SOC_MMU_DI_VADDR_SHARED` igual que el P4— sino porque el
 * codigo estaba en el directorio del vecino. Correccion de Eduardo (8-sep): «el
 * sistema deberia ser identico, igual que el environment».
 *
 * Lo unico que cambia entre micros es cuantas direcciones devuelve la MMU:
 *   una vista  -> INST y DATA dan LA MISMA (los RISC-V: C3, C5, C6 y P4)
 *   dos vistas -> se lee por DATA y se ejecuta por INST (la S3, Xtensa)
 *   ⚠️ #676 (9-oct): el 9-sep se midio «las cuatro dan la misma, la S3 incluida»
 *   y se dio por bueno. En la S3 era el IDF: al pedir DATA, las paginas ya estaban
 *   mapeadas (por la peticion de INST) y `esp_mmu_map` devuelve el bloque que hay
 *   —la direccion de INSTRUCCIONES—, que `spi_flash_mmap` da por buena. En Xtensa
 *   el bus de instrucciones solo se lee de 4 en 4 bytes: un byte suelto es un
 *   LoadStoreError. Con la S3 sin packs no se habia visto; su Stdlib.pack la
 *   tumbo en el CRC del END. La direccion de DATOS se le pide ahora a la MMU
 *   (`esp_mmu_paddr_to_vaddr`), abajo.
 * Antes el segundo caso se reconocia y se ABANDONABA («NO se salta hasta que eso
 * este escrito»). Ya esta escrito, y no hizo falta codigo de familia: solo dejar
 * de suponer que un puntero sirve para las dos cosas (ver `bpvm_pack_mount`).
 *
 * Va en el board manager, que es quien ya era dueno de la vista, y no en un
 * fichero nuevo: los cuatro CMakeLists listan las fuentes A MANO, asi que un
 * fichero nuevo serian cuatro altas y una oportunidad de olvidar una. */
int board_mgr_esp32_mapear_packs(void) {
    static const void* s_map_inst = 0;
    static const void* s_map_data = 0;
    static esp_partition_mmap_handle_t s_h_inst = 0;
    static esp_partition_mmap_handle_t s_h_data = 0;

    if (s_map_inst) return 1;                      /* ya estaba */

    const esp_partition_t* bpdata = s_bpdata;
    const bpvm_part_t*     packs  = board_mgr_esp32_packs();

    if (!bpdata || !packs) {
        log_printf("pack: sin zona de packs (el arranque no llego a particiones)");
        return 0;
    }
    if (packs->size == 0u) {
        log_printf("pack: la zona de packs mide 0 — mira el reparto FS|Packs del ENV");
        return 0;
    }

    /* El offset es RELATIVO a bpdata, igual que el del FS. */
    esp_err_t ei = esp_partition_mmap(bpdata, packs->offset, packs->size,
                                      ESP_PARTITION_MMAP_INST, &s_map_inst, &s_h_inst);
    esp_err_t ed = esp_partition_mmap(bpdata, packs->offset, packs->size,
                                      ESP_PARTITION_MMAP_DATA, &s_map_data, &s_h_data);

    /* Los 32 MB del P4, paso 0 (30-sep) — la direccion FISICA, que es la que
     * decide. El offset relativo a bpdata no dice nada por si solo: el mapeo del
     * IDF (`spi_flash_mmap`) rechaza con ESP_ERR_INVALID_ARG todo lo que pase de
     * 24 bits (16 MB) —salvo con CONFIG_BOOTLOADER_CACHE_32BIT_ADDR_*, que es
     * experimental y no usamos—, y lo dice por la consola del IDF, no por este
     * log. `pack_p4.c` lo decia y se perdio al pasar el mapeo al comun
     * (630a221e): desde entonces un fallo salia como «err=258» y nada mas. */
    uint32_t fis_ini = (uint32_t) bpdata->address + packs->offset;
    uint32_t fis_fin = fis_ini + packs->size;

    /* #676 — LA VISTA DE DATOS, A LA MMU. Lo que devolvio la peticion de DATA arriba es, si las paginas ya estaban
     * mapeadas, la direccion de INST (ver la cabecera). La MMU de la S3 es una sola tabla para los dos buses: las
     * mismas paginas se ven tambien por el de DATOS, y esta llamada da esa direccion. En los RISC-V devuelve la misma
     * que INST y no se toca nada: solo cambia el micro en el que de verdad hay dos. */
    void* v_datos = NULL;
    if (ei == ESP_OK
        && esp_mmu_paddr_to_vaddr((esp_paddr_t) fis_ini, MMU_TARGET_FLASH0, MMU_VADDR_DATA, &v_datos) == ESP_OK
        && v_datos != NULL && v_datos != s_map_inst) {
        s_map_data = v_datos;
        ed = ESP_OK;
    }
    log_printf("pack: zona %u KB en bpdata+0x%x (fisica %u..%u KB) | INST %s @%p | DATA %s @%p%s",
               (unsigned) (packs->size / 1024u), (unsigned) packs->offset,
               (unsigned) (fis_ini / 1024u), (unsigned) (fis_fin / 1024u),
               (ei == ESP_OK) ? "ok" : "FALLO", s_map_inst,
               (ed == ESP_OK) ? "ok" : "FALLO", s_map_data,
               (PACKS_MMAP_24BIT && fis_fin > 0x1000000u)
                   ? "  <<< PASA DE 16 MB: el mapeo del IDF solo llega a 24 bits" : "");

    if (ei != ESP_OK) {
        log_printf("pack: sin mapeo EJECUTABLE no hay nada que hacer (err=%d, %s)",
                   (int) ei, esp_err_to_name(ei));
        s_map_inst = 0;
        return 0;
    }

    if (ed == ESP_OK && s_map_data != s_map_inst) {
        /* Dos vistas: se lee por DATA (acepta accesos de cualquier ancho) y se
         * salta por INST. La traduccion la hace `bpvm_pack_exec_ptr` en el unico
         * sitio donde el puntero pasa de dato a codigo. Entra la S3 (#676). */
        board_mgr_esp32_set_packs_view(s_map_data, s_map_inst, packs->size);
    } else if (ed == ESP_OK) {
        /* P4 y C6: la misma direccion para las dos cosas. */
        board_mgr_esp32_set_packs_view(s_map_inst, s_map_inst, packs->size);
    } else {
        /* Sin mapeo de DATOS se valida por el de INST, que en algunos micros solo
         * admite accesos de 4 bytes -> si la cabecera sale rara, es esto. */
        /* V7/H7 (30-sep): con la zona grande (15 MB en la C5, con el FS arriba) la segunda vista no cabe en el espacio
         * virtual: `esp_mmu_map` busca un hueco del tamano entero ANTES de ver que ese trozo ya esta mapeado (y
         * devolveria la misma direccion). NOT_FOUND es eso; la de INST es la misma direccion en estos micros. */
        log_printf("pack: no hubo mapeo de DATOS (err=%d, %s)%s; se lee por el de INST", (int) ed, esp_err_to_name(ed),
                   (ed == ESP_ERR_NOT_FOUND) ? ": no queda espacio virtual para una segunda vista" : "");
        board_mgr_esp32_set_packs_view(s_map_inst, s_map_inst, packs->size);
    }
    return 1;
}

/* ── Cintura de ESCRITURA. Ésta SÍ es común a los dos ESP32: los dos graban por
 *    `esp_partition_*`, así que no hay nada que separar por familia.
 *
 * Los offsets que llegan son RELATIVOS a la región de packs; aquí se suman al
 * offset de la región dentro de `bpdata`, que es la partición real. Esa suma es
 * justo lo que la cintura existe para hacer: el núcleo de packs no sabe —ni
 * tiene por qué— dónde empieza la región. */
static uint32_t packs_off_en_bpdata(void) {
    const bpvm_part_t* pp = bpvm_part_get(&s_layout, BPVM_PART_PACKS);
    return pp ? pp->offset : 0u;
}

/* #676 — tras escribir la flash, el IDF invalida la cache por la direccion del bloque que tiene registrado: la de
 * INSTRUCCIONES (`spi_flash_check_and_flush_cache`), y en la S3 la ROM invalida la cache del bus de esa direccion,
 * no la del otro. Si la zona se LEE por el de DATOS, las lineas que ya estaban en su cache se quedarian con lo de
 * antes: la relectura del END —o el siguiente PACK_LS— veria la flash vieja. Se invalidan aqui, alineadas a la linea
 * (64 B cubre las tres longitudes que admite la S3). Con una sola vista ya lo hizo el IDF y no se toca nada. */
static void packs_invalida_lectura(uint32_t off, uint32_t len) {
    if (s_packs_view == 0 || s_packs_view == s_packs_view_exec || len == 0u) return;
    uint32_t a = off & ~63u;
    uint32_t b = (off + len + 63u) & ~63u;
    esp_err_t e = esp_cache_msync((void*) (uintptr_t) (s_packs_view + a), b - a,
                                  ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
    if (e != ESP_OK)
        log_printf("pack: no se pudo invalidar la cache de lectura de la zona (+0x%x, %u B): %s",
                   (unsigned) a, (unsigned) (b - a), esp_err_to_name(e));
}

static int packs_fl_erase(void* user, uint32_t off, uint32_t len) {
    (void) user;
    if (!s_bpdata) return -1;
    int r = esp_partition_erase_range(s_bpdata, packs_off_en_bpdata() + off, len)
            == ESP_OK ? 0 : -1;
    packs_invalida_lectura(off, len);   /* #676: tambien si falla —lo que haya quedado escrito, que se lea—. */
    return r;
}

static int packs_fl_program(void* user, uint32_t off, const uint8_t* d, uint32_t len) {
    (void) user;
    if (!s_bpdata) return -1;
    int r = esp_partition_write(s_bpdata, packs_off_en_bpdata() + off, d, len)
            == ESP_OK ? 0 : -1;
    packs_invalida_lectura(off, len);   /* #676 */
    return r;
}

static const bpvm_pack_flash_t s_packs_fl = {
    packs_fl_erase, packs_fl_program, 0,
    4096u    /* granularidad de borrado del ESP32 */
};

/* V5/H7 — dónde está la zona de packs, para que el cargador nativo la mapee.
 * Devuelto como `const void*` en el .h para no arrastrar `esp_partition.h` a
 * quien sólo quiera el layout; el que mapea ya lo incluye de todas formas. */
const void* board_mgr_esp32_bpdata(void) { return (const void*) s_bpdata; }

const bpvm_part_t* board_mgr_esp32_packs(void) {
    /* Sin layout válido no hay zona: el arranque no llegó al estado 2. Devolver
     * NULL y no un rango a cero, porque "no hay" y "hay pero mide 0" mandan a
     * sitios distintos y el llamante tiene que poder decirlo. */
    if (!s_layout.complete && s_layout.parts[BPVM_PART_PACKS].size == 0u) return 0;
    return bpvm_part_get(&s_layout, BPVM_PART_PACKS);
}

/* ── cintura del env: lee/escribe los 2 sectores A/B de la partición bpenv ── */

/* Una copia del ENV (0 = A, 1 = B), de la flash. 0 leída; -1 no (la vista y el gestor la dan por borrada). */
static int env_leer(int slot, uint8_t* dst) {
    if (!s_bpenv) return -1;
    return esp_partition_read(s_bpenv, slot == 0 ? 0u : BP_ENV_SECTOR, dst, BP_ENV_SECTOR) == ESP_OK ? 0 : -1;
}

static int env_write_slot(int slot, const uint8_t* buf) {
    if (!s_bpenv) return -1;
    uint32_t off = (slot == 0) ? 0u : BP_ENV_SECTOR;
    /* esp_partition_{erase_range,write} son seguros para multitarea (esp-idf) y no
     * suspenden el XIP como el RP2350 → sin gimnasia de IRQs aquí. */
    if (esp_partition_erase_range(s_bpenv, off, BP_ENV_SECTOR) != ESP_OK
        || esp_partition_write(s_bpenv, off, buf, BP_ENV_SECTOR) != ESP_OK) {
        log_printf("env: la flash no escribe el sector %d de bpenv: no se ha escrito", slot);
        return -1;
    }
    return 0;
}

/* ── V7/H10, el board_mgr común (3-oct): LA CINTURA DE LA ESP32 (bpvm_bmgr_familia_t) ──
 * Lo que quedaba de propio en su `handle` y en su `env_anadir`: las copias se LEEN a un espejo (bpenv no se mapea), un
 * sector se escribe por la partición del IDF, lo de la placa en el gestor y la partición que falta. Lo demás —el
 * `handle`, la vista del ENV y el `anadir`— es común, el mismo de las otras familias. */
static uint32_t data_usable(void);

static void env_rellenar(bpvm_bmgr_t* bm) {
    bm->part_base = 0u;                 /* offsets relativos a bpdata */
    bm->usable_flash = data_usable();   /* la tabla vendor ES el límite; sin clamp #292 */
    bm->fs_arriba = s_bpfs_alto ? (uint32_t) s_bpfs_alto->size : 0u;   /* V7/H7: el FS arriba (PART_LS lo cuenta) */
    bm->live = &s_boot;                 /* STATE cuenta el estado REAL del boot */
    /* V5/H7 — la zona de packs. Sólo si la familia registró su vista Y el layout dice que la región existe: las dos
     * condiciones, porque una vista sin región sería leer fuera, y una región sin vista no se puede leer. */
    const bpvm_part_t* pp = bpvm_part_get(&s_layout, BPVM_PART_PACKS);
    if (s_packs_view && pp && pp->size > 0) {
        bm->packs_base  = s_packs_view;
        bm->packs_size  = (s_packs_view_size < pp->size) ? s_packs_view_size : pp->size;
        bm->packs_flash = &s_packs_fl;
    }
    /* V5/H8 — la RAM de trabajo de un motor nativo: el PRINCIPIO del bloque (`[estaticos | arena]`). Lo reserva el
     * arranque común (`pack_esp32_ram_init`); aquí se expone para que PACK_BURN_BEGIN se lo diga al IDE. */
    bm->pack_ram_base = (uint32_t) (uintptr_t) s_pack_ram_base;
    bm->pack_ram_size = s_pack_ram_size;
}

static const char* env_no_vale(void) {
    return s_bpenv ? NULL : "sin particion bpenv (reflashear tabla)";
}

static const bpvm_bmgr_familia_t s_familia = {
    BP_ENV_SECTOR, NULL, env_leer, s_env_espejo, env_write_slot, env_rellenar, env_no_vale
};

static void asegura_familia(void) {
    if (bpvm_bmgr_familia_actual() != &s_familia) bpvm_bmgr_familia(&s_familia);
}

/* V7/H10 (#626) — el arranque común AÑADE una clave que falta (la fecha de la imagen, una vez), por el común: el
 * bloque nuevo en la zona de rascar, por la partición, y la vista lo ve sin reiniciar. */
int board_mgr_esp32_env_anadir(const char* clave, const char* valor) {
    asegura_familia();
    return bpvm_bmgr_anadir(clave, valor);
}

static uint32_t data_usable(void) { return s_bpdata ? (uint32_t) s_bpdata->size : 0u; }

/*
 * V5/H7 — EL ENV, ANTES DE TIEMPO. Sólo el env: ni particiones, ni FS, ni VM.
 *
 * ─── POR QUÉ HACE FALTA ───
 *
 * El bloque de memoria de la BD tiene DOS exigencias que chocan en el orden de
 * arranque del P4:
 *
 *   · su TAMAÑO lo dice el env (`SQLite=<MB>`)      -> hay que leer flash
 *   · su DIRECCIÓN sólo es determinista si muerde
 *     ANTES que el heap de la VM                    -> hay que reservar pronto
 *
 * Y en el P4 el env se carga DESPUÉS del reparto de PSRAM: `app_main` llama a
 * `vm_buffer_init_psram()` enseguida, y `board_mgr_esp32_boot()` sólo corre más
 * tarde, ya dentro de la tarea de transporte. (En el Pico no se nota porque allí
 * el env ya está leído cuando se parte la PSRAM.)
 *
 * Que la dirección sea determinista NO es un lujo: el pack nativo se realoja en
 * el PC para una dirección concreta y lleva ese SELLO dentro. Si `heap_caps_malloc`
 * devolviera otra en el siguiente arranque, el pack grabado dejaría de valer —
 * y el síntoma sería de los peores: funciona una vez y luego no, sin patrón.
 *
 * ─── POR QUÉ ADITIVA Y NO MOVER EL BOOT ───
 *
 * Adelantar `board_mgr_esp32_boot()` entero movería el arranque de una placa que
 * hoy funciona, y arrastraría con él las particiones, el FS y la VM. Esto es
 * sólo encontrar `bpenv`: la vista común (V7/H10) lee las dos copias la primera
 * vez que se le pide el ENV, y otra vez tras cada escritura.
 *
 * Idempotente a propósito: la vista de `board_mgr_env()` es la misma antes y
 * después del arranque de verdad.
 */
void board_mgr_esp32_env_temprano(void) {
    if (!s_bpenv) {
        s_bpenv = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                           ESP_PARTITION_SUBTYPE_ANY, "bpenv");
    }
    /* V7/H10: registrar la familia OTRA VEZ obliga a la vista a releer: si alguien pidió el ENV antes de encontrar
     * bpenv, se quedó con uno vacío. */
    bpvm_bmgr_familia(&s_familia);
}

/* ── arranque escalonado: particiones del env → FS (sub-rango de bpdata) → VM ── */

static bpvm_boot_step_t layer_partitions(void* u) {
    (void) u;
    bpvm_boot_step_t r; r.ok = 0; r.reason[0] = '\0';
    if (!s_bpenv)  { snprintf(r.reason, sizeof r.reason, "sin particion bpenv"); return r; }
    if (!s_bpdata) { snprintf(r.reason, sizeof r.reason, "sin particion bpdata"); return r; }
    asegura_familia();
    const bpvm_env_t* env = bpvm_bmgr_vista();   /* V7/H10: el ENV de la flash */
    if (s_bpfs_alto) {   /* V7/H7: el FS arriba; los packs, bpdata entera. El reparto no sale del env */
        bpvm_part_layout_fs_arriba(0u, data_usable(), (uint32_t) s_bpfs_alto->size, BP_ENV_SECTOR, &s_layout);
        char l[160];   /* V7/H10: de dónde sale el reparto, y cómo queda */
        if (bpvm_part_describe(&s_layout, "el FS arriba de bpdata (la flash del chip; el ENV no reparte)",
                               l, sizeof l) > 0) log_printf("%s", l);
        r.ok = 1;
        return r;
    }
    int bad = -1;
    bpvm_part_err_t e = bpvm_part_layout(env, 0u, data_usable(), BP_ENV_SECTOR, &s_layout, &bad);
    if (e == BPVM_PART_OK) {
        char l[160];   /* V7/H10: de dónde sale el reparto, y cómo queda */
        if (bpvm_part_describe(&s_layout, "del ENV (part.fs.size)", l, sizeof l) > 0) log_printf("%s", l);
        r.ok = 1;
        return r;
    }
    snprintf(r.reason, sizeof r.reason, "%s", bpvm_part_err_str(e));
    return r;
}

static bpvm_boot_step_t layer_fs(void* u) {
    (void) u;
    bpvm_boot_step_t r; r.ok = 0; r.reason[0] = '\0';
    const bpvm_part_t* fsp = bpvm_part_get(&s_layout, BPVM_PART_FS);
    if (!fsp) { snprintf(r.reason, sizeof r.reason, "sin region FS"); return r; }
    /* fsp->offset es RELATIVO a bpdata (base=0) → el 1er byte de la partición de
     * datos. fs_init_at monta littlefs en ese sub-rango. V7/H7: con el FS ARRIBA,
     * fs_init_at monta en «bpfsalto» (la busca él) desde su principio. */
    if (fs_init_at(s_bpfs_alto ? 0u : fsp->offset, fsp->size) != FS_OK) {
        snprintf(r.reason, sizeof r.reason, "littlefs no monta ni formatea");
        return r;
    }
    r.ok = 1;
    return r;
}

static bpvm_boot_step_t layer_app(void* u) {
    (void) u;
    bpvm_boot_step_t r; r.reason[0] = '\0';
    r.ok = (s_vm_buffer != NULL && s_vm_buffer_size > 0);
    if (!r.ok) snprintf(r.reason, sizeof r.reason, "heap de la VM no disponible");
    return r;
}

void board_mgr_esp32_boot(void) {
    s_bpenv  = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "bpenv");
    s_bpdata = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "bpdata");
    bpvm_bmgr_familia(&s_familia);   /* V7/H10: con bpenv encontrada, la vista vuelve a leer (ver env_temprano) */

    /* #328 — la tabla de particiones DICE dónde acaba la flash; el chip decide.
     * Si la tabla promete más de lo que hay, las particiones altas existen para
     * el SDK pero no tienen silicio detrás: se escribe, no se guarda, y al releer
     * sale otra cosa — que es exactamente lo que littlefs reporta como CORRUPT.
     * Nadie se entera hasta que algo escribe lejos, así que se comprueba aquí y
     * se DICE, en vez de descubrirlo por un error que habla de otra cosa. */
    {
        uint32_t cfgsz = 0, phys = 0;
        esp_err_t e1 = esp_flash_get_size(NULL, &cfgsz);            /* el CONFIGURADO */
        esp_err_t e2 = esp_flash_get_physical_size(NULL, &phys);    /* el del CHIP */
        uint32_t need = s_bpdata ? (uint32_t) (s_bpdata->address + s_bpdata->size) : 0u;
        if (e1 == ESP_OK && e2 == ESP_OK) {
            /* SON DOS COSAS DISTINTAS y ahí estuvo la trampa del #328: el chip
             * puede ser de 16 MB mientras el driver trabaja con el tamaño que
             * trae la cabecera del BOOTLOADER. Si el bootloader es viejo (2 MB),
             * todo acceso por encima de ese límite no surte efecto — sin error:
             * se escribe, no se guarda, y al releer sale otra cosa, que es lo
             * que littlefs reporta como CORRUPT. Y explica el patrón exacto que
             * vimos: los superbloques (al principio) bien y el resto mal.
             * Un reflasheo SOLO de la app no actualiza el bootloader ⇒ la placa
             * arrastra el límite viejo y nada te lo dice. Aquí sí.
             *
             * Y sólo cuando la TABLA pasa de lo configurado (30-sep, los 32 MB del
             * P4, paso 0): que el chip tenga más flash de la que usa la imagen no
             * es un fallo. La P4 Waveshare tiene 32 MB y su imagen declara 16 a
             * propósito —la misma imagen arranca en el kit de 16, y el mapeo del
             * IDF no pasa de 16 MB—, y este aviso le aconsejaba reflashear el
             * bootloader, que no arregla nada. */
            const char* aviso =
                (need > phys)  ? "  <<< LA TABLA SE SALE DEL CHIP" :
                (need > cfgsz) ? "  <<< LA TABLA SE SALE DE LA FLASH CONFIGURADA: el bootloader usa menos de la que hay (reflashear el bootloader)" :
                                 "";
            log_printf("flash: configurada %u KB | chip fisico %u KB | bpdata acaba en %u KB%s",
                       (unsigned) (cfgsz / 1024u), (unsigned) (phys / 1024u),
                       (unsigned) (need / 1024u), aviso);
            /* V7/H7 (30-sep, los 32 MB del P4 y la C5). Eduardo: «para ahora con 16M de FS la P4 va sobrada. Y el
             * resto para el SO y los packs». La flash que el CHIP tiene por encima del final de la tabla es del FS,
             * entera, y bpdata queda para los packs, que por el mapeo no pueden pasar de 16 MB (24 bits). La tabla es
             * la misma en todas las placas de una imagen —el kit del P4 tiene 16 MB y la Waveshare 32—: lo que decide
             * es lo que dice el chip, en ejecución. Medido en la Waveshare antes de hacerlo (V7_IDEAS §H7 «Los 32 MB
             * del P4»): con el tamaño del driver subido al físico, leer, escribir y borrar arriba van bien y sin eco;
             * sin subirlo, INVALID_ARG, porque el driver acota por la cabecera de la imagen (`size`). */
            if (s_bpdata && phys > need && phys - need >= FS_ARRIBA_MIN) {
                if (esp_flash_default_chip->size < phys) esp_flash_default_chip->size = phys;
                esp_err_t er = esp_partition_register_external(esp_flash_default_chip, need, phys - need,
                                                               "bpfsalto", ESP_PARTITION_TYPE_DATA,
                                                               (esp_partition_subtype_t) 0x42, &s_bpfs_alto);
                if (er == ESP_OK) {
                    log_printf("flash: el FS va ARRIBA, %u..%u KB (%u KB); bpdata, entera para los packs (%u KB); "
                               "part.fs.size no se aplica en esta placa",
                               (unsigned) (need / 1024u), (unsigned) (phys / 1024u),
                               (unsigned) ((phys - need) / 1024u), (unsigned) (s_bpdata->size / 1024u));
                } else {
                    s_bpfs_alto = NULL;
                    log_printf("flash: la flash de encima de la tabla (%u KB) no se pudo registrar (%s): el FS "
                               "sigue en bpdata", (unsigned) ((phys - need) / 1024u), esp_err_to_name(er));
                }
            }
        } else {
            log_printf("flash: no se pudo leer el tamano (cfg=%d fis=%d)", (int) e1, (int) e2);
        }
    }

    bpvm_boot_layers_t layers;
    layers.to_partitions = layer_partitions;
    layers.to_fs         = layer_fs;
    layers.to_app        = layer_app;
    layers.user          = NULL;
    layers.max_state     = BPVM_BOOT_APP;
    bpvm_boot_climb(&layers, &s_boot);

    ESP_LOGI(TAG, "boot: estado %d (%s)%s%s", (int) s_boot.state,
             bpvm_boot_state_name(s_boot.state),
             s_boot.degraded ? " DEGRADADO: " : "",
             s_boot.degraded ? s_boot.reason : "");
    /* Al log PERSISTENTE lo dice el arranque común (V7/H7, Boot 2 fase 4: una línea para
     * todas las familias), y lo vuelca a la flash antes del autorun. */
}

/* ── ramo del wire (STATE/ENV_x/PART_x): repl_esp32 encamina aquí ── */

/* #338 — la zona compartida tiene que dar para las DOS copias del env de ESTA
 * familia. Si alguien cambia el sector de borrado (o porta a un micro con uno
 * mayor) sin subir BPVM_SCRATCH_BYTES, aqui no compila — en vez de descubrirlo
 * en placa como un "zona de scratch no disponible" al abrir el panel. */
typedef char bp_chk_scratch_env[(BPVM_SCRATCH_BYTES >= 2u * BP_ENV_SECTOR) ? 1 : -1];

void board_mgr_esp32_handle(long id, const json_obj_t* obj, const char* type,
                            unsigned char* scratch, unsigned long scratch_len,
                            const unsigned char* bulk, unsigned long bulk_len) {
    /* V7/H10, el board_mgr común (3-oct): lo que había aquí es el `handle` común (bpvm_repl.c), el mismo de las otras
     * familias y del sim; lo propio de la ESP32 es su cintura (s_familia, arriba). Y si la flash no escribe,
     * FLASH_ERROR en vez del OK (D2, #636). */
    asegura_familia();
    bpvm_bmgr_handle(id, obj, type, scratch, scratch_len, bulk, bulk_len);
}

/* V6/U4 — la stdlib embebida a /lib. Aquí sólo lo propio de esta familia: el
 * `put` (fs_put) y el LOTE — sin suspender, cada escritura auto-persiste la
 * partición entera (~3 s en la bpfs de 10 MB del P4) y el primer boot tardaba
 * ~46 s (021fdbf). Antes esto vivía dentro del fichero GENERADO, y lo que se
 * puso ahí a mano (#422) murió en la siguiente regeneración (#446). */
static int  mods_put(const char* p, const uint8_t* d, uint32_t n) {
    return fs_put(p, d, n) == FS_OK ? 0 : -1;
}
static void mods_log(const char* l) { log_printf("%s", l); }

void esp32_mods_install(void) {
    /* V7/H6 (#468) — y se deja registrada: el wire de packs la vuelve a correr cuando
     * cambia la zona (grabar, borrar o formatear), con el mismo lote. */
    bpvm_mods_set_resync(esp32_mods_install);
    fs_autosave_suspend();
    unsigned escritos = bpvm_mods_instalar_tabla(esp32_mods, esp32_mods_n, mods_put, mods_log);
    fs_autosave_resume(escritos > 0);
}
