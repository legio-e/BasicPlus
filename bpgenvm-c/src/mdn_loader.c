/*
 * mdn_loader.c — loader .mdn (H3 #158).
 *
 * Diseño zero-copy: el código Thumb-2 del thunk vive en el buffer
 * `data` que el caller nos pasa (típicamente `.data` con el blob
 * embebido o el buffer del FS donde guardamos el .mdn). Como ese
 * buffer ya está en RAM ejecutable con una dirección estable, NO
 * hacemos memcpy NI necesitamos pool de RAM aparte. Solo:
 *
 *   1. Validamos magic + version + abi.
 *   2. Validamos consistencia de tamaños (header + symtab + code).
 *   3. Para cada símbolo, calculamos thunk_addr = (data + hdr_total
 *      + sym.thunk_offset) | 1u   (bit Thumb).
 *   4. bpvm_aot_register_by_name lo guarda en el registry global.
 *
 * REQUISITO IMPORTANTE: el buffer pasado a bpvm_load_mdn debe
 * permanecer válido y en RAM mientras los thunks estén registrados.
 * Para .mdn embebidos en firmware (.data array) eso es trivial. Para
 * .mdn cargados desde FS, el caller debe pinear la región en RAM
 * mientras dure el RUN.
 */

#include "mdn_loader.h"
#include "bpvm_log.h"   /* las trazas van al log comun (antes: hook débil) */
#include "mdn_format.h"
#include "bpvm_aot_recetas.h"   /* [V7/H2, #441] la receta que ESTE firmware exige */
#include "crc32.h"              /* bpvm_crc32: la huella de la receta */
#include "bpvm_pack.h"   /* bpvm_pack_exec_ptr: la vista ejecutable de la zona */
#include "aot_registry.h"

#include "bpvm.h"

#include <stdint.h>
#include <string.h>

/* H4 — arquitectura del firmware (gate del .mdn) + bit de modo del puntero a
 * función. Compile-time: un .mdn solo puede ejecutar en la ISA que lo compiló,
 * así que la arch del host la fija el toolchain que compila ESTE fichero. */
#if defined(__arm__) || defined(__thumb__)
#  define MDN_HOST_ARCH   MDN_ARCH_ARM
#  define MDN_FUNCPTR_BIT 1u    /* ARM: bit 0 del address = modo Thumb */
#elif defined(__riscv)
#  define MDN_HOST_ARCH   MDN_ARCH_RISCV
#  define MDN_FUNCPTR_BIT 0u    /* RISC-V: dirección plana (entradas 2-byte aligned) */
#elif defined(__XTENSA__)
#  define MDN_HOST_ARCH   MDN_ARCH_XTENSA
#  define MDN_FUNCPTR_BIT 0u    /* Xtensa: dirección plana */
/* V6 — EL PC TAMBIEN DECLARA SU ARQUITECTURA, y con eso el gate SE ENCIENDE.
 *
 * Hasta hoy el host era MDN_ARCH_NONE, que no significa «PC»: significa «sin
 * gate». Consecuencia medida en la auditoria del 5-sep: coger `SQLite.mdn.ARMV8`,
 * subirle el byte de ABI y cargarlo en el PC devolvia **0 = OK** — registraba
 * thunks a codigo ARM en memoria que ademas no es ejecutable. El fallo llevaba
 * armado desde siempre y no habia disparado solo porque el host nunca llama al
 * escaner. Con packs nativos en el PC eso deja de ser teorico.
 *
 * Es el mismo doble dano que ya se corrigio para el Xtensa (ver mdn_format.h): el
 * que cae en NONE ni dice a que compilar ni tiene gate. */
#elif defined(__x86_64__) || defined(_M_X64)
#  define MDN_HOST_ARCH   MDN_ARCH_X64
#  define MDN_FUNCPTR_BIT 0u    /* x86-64: dirección plana */
#elif defined(__i386__) || defined(_M_IX86)
#  define MDN_HOST_ARCH   MDN_ARCH_X86
#  define MDN_FUNCPTR_BIT 0u    /* x86: dirección plana */
#else
#  define MDN_HOST_ARCH   MDN_ARCH_NONE   /* arquitectura desconocida: sin gate */
#  define MDN_FUNCPTR_BIT 0u
#endif

/* H11 — la arquitectura del firmware, PUBLICADA. Estaba aquí dentro sirviendo
 * sólo al gate del .mdn, y el IDE la necesita para saber a qué compilar el
 * código nativo: con un `.bp` suelto no hay proyecto donde apuntarla, así que
 * la fuente de verdad pasa a ser la PLACA — que es quien lo sabe de verdad.
 * Mismo principio que el JEDEC frente al #define de tamaño de flash. */
uint16_t bpvm_mdn_host_arch(void) { return (uint16_t) MDN_HOST_ARCH; }

/* V5/H — la CONVENCIÓN DE COMA FLOTANTE, publicada al lado de la arch y por el
 * mismo motivo: el IDE tiene que sellar con ella el pack de código nativo.
 *
 * Por qué NO basta con la arch: `MDN_ARCH_ARM` no distingue `hard` de `softfp`,
 * y una discrepancia de ABI de coma flotante **no da error de enlace — da
 * números mal en silencio**, que es la peor clase de fallo. Se ve físicamente en
 * el toolchain: hay una libgcc por ABI (.../v8-m.main+fp/softfp/libgcc.a).
 *
 * Compile-time, como la arch: la fija el toolchain que compila ESTE fichero.
 *  - ARM: __ARM_PCS_VFP ⇒ hard (args en registros VFP) · __SOFTFP__ ⇒ soft
 *    (sin FPU) · ninguna de las dos ⇒ softfp (FPU para calcular, args en
 *    registros enteros). OJO: con -mfloat-abi=softfp NO se define ninguna.
 *  - RISC-V: el toolchain define __riscv_float_abi_* directamente. */
const char* bpvm_mdn_host_float_abi(void) {
#if defined(__arm__) || defined(__thumb__)
#  if defined(__ARM_PCS_VFP)
    return "hard";
#  elif defined(__SOFTFP__)
    return "soft";
#  else
    return "softfp";
#  endif
#elif defined(__riscv)
#  if defined(__riscv_float_abi_double)
    return "ilp32d";
#  elif defined(__riscv_float_abi_single)
    return "ilp32f";
#  else
    return "ilp32";
#  endif
#else
    return "";        /* host/Xtensa: sin pack nativo que sellar */
#endif
}

/* [V7/H2, #441] LA RECETA, publicada al lado de la arch y la float-abi y por el
 * mismo motivo: es la tercera cosa que hace inservible un `.mdn` sin que nada
 * lo diga. La elige `bpvm_aot_recetas.h` por las macros de la ISA con que se
 * compila ESTE fichero; el CRC se calcula una vez sobre los bytes del literal —
 * los mismos que el IDE hashea al escribir la cabecera. */
const char* bpvm_mdn_host_receta_nombre(void) { return BPVM_AOT_HOST_RECETA_NOMBRE; }

/* [V7/H5, 27-sep] La ABI de helpers, sólo donde hay receta: sin ella esta imagen no ejecuta ningún `.mdn`, y un 9
 * ahí diría lo contrario. */
int32_t bpvm_mdn_host_abi(void) { return BPVM_AOT_HOST_RECETA_NOMBRE[0] ? (int32_t) MDN_ABI_VERSION : 0; }

uint32_t bpvm_mdn_host_receta_crc(void) {
    static uint32_t s_crc = 0;
    static int      s_hecho = 0;
    if (!s_hecho) {
        const char* r = BPVM_AOT_HOST_RECETA;
        s_crc = r[0] ? bpvm_crc32((const uint8_t*) r, strlen(r)) : 0u;
        s_hecho = 1;
    }
    return s_crc;
}

/* [V6, 24-ago-2026] LAS TRAZAS VAN AL LOG COMÚN, sin intermediario.
 *
 * Aquí había un hook débil (`bpvm_mdn_log`, no-op) que cada port podía
 * sobreescribir, *«así el fichero no depende de ningún log.h»*. Sólo lo
 * sobreescribía el Pico — o sea que el S3, la P4 y el STM32 eran **CIEGOS** a
 * todo lo que este cargador tiene que decir: cuántos thunks se registraron, un
 * rechazo por ABI, un desajuste de arquitectura, un símbolo que no está en el
 * `.mod`.
 *
 * 📌 Se vio probando AOT en la P4: el bloque nativo cargaba y no había forma de
 * saber si sus thunks habían entrado. El fallo del método `native` que se cazó
 * el 24-ago en la Pico (`MDN: skip 'X' rc=-2`) habría pasado **invisible** en
 * las otras tres familias.
 *
 * Y el motivo del hook había CADUCADO: en V6/U1.2 el log pasó al común
 * (`src/bpvm_log.c` + `include/bpvm_log.h`), que compilan las cuatro imágenes —
 * el STM32 incluido, por su cintura. Depender de él ya no acopla nada. En host
 * `log_printf` sale sin hacer nada (nadie llama a `log_init`), así que no
 * aparece salida nueva y la paridad no se toca. */

/* El FS pinea cada fichero 4-aligned (fs.c v4), así que data viene
 * ya correctamente alineado para Thumb-2. NO necesitamos staging.
 * Las funciones legacy quedan como no-ops por compat. */
void   bpvm_mdn_reset(void)        { /* no-op en zero-copy */ }
size_t bpvm_mdn_used_bytes(void)   { return 0; }

/* [#565] La vista ejecutable de la RAM: la registra la cintura (ver mdn_loader.h). */
static bpvm_mdn_vista_ejec_t s_vista_ram = NULL;
void bpvm_mdn_set_vista_ejecutable(bpvm_mdn_vista_ejec_t fn) { s_vista_ram = fn; }

int bpvm_load_mdn(struct bpvm* vm, const uint8_t* data, size_t size) {
    /* La cabecera mínima es la v1 (20 B); la v2 (24 B) se comprueba en cuanto
     * se sabe la versión. `sizeof(mdn_header_t)` es la actual y NO vale aquí:
     * rechazaría por «truncado» un v1 de 20 bytes justos. */
    if (!data || size < MDN_HEADER_SIZE_V1) return MDN_ERR_TRUNCATED;

    /* Validar alineación. Thumb-2 requiere PC bit 1 = 0, lo que exige
     * que el code_base (= data + hdr_total) sea al menos 2-aligned.
     * Como hdr_total siempre es múltiplo de 4 (header=20/24, sym=36 cada
     * uno), basta con que data sea 4-aligned. El FS v4 lo garantiza
     * para ficheros del FS; los .mdn embebidos en .data del firmware
     * también vienen alineados por el compilador. Si llega misaligned,
     * es bug del caller. */
    if (((uintptr_t) data) & 0x3u) {
        log_printf("MDN: ABORT — data %p no alineado a 4 (FS v4 debería garantizar)",
                   (const void*) data);
        return MDN_ERR_TRUNCATED;
    }

    const mdn_header_t* h = (const mdn_header_t*) data;

    /* Magic + version + ABI. */
    static const uint8_t expected[4] = MDN_MAGIC;
    if (memcmp(h->magic, expected, 4) != 0) return MDN_ERR_MAGIC;
    /* v1 (sin huella de receta) y v2 (con ella). Un v1 se carga «a ciegas» con
     * aviso, más abajo; es la transición, como un MOD7 sin datos de versión. */
    /* v1, v2 y v3 (V7/H5 P2b.3: la v3 sólo añade la marca DIRECTA en el bit alto de thunk_offset). */
    if (h->version < 1 || h->version > MDN_VERSION) return MDN_ERR_VERSION;
    const uint32_t hsz = mdn_header_size(h->version);
    if (size < hsz) return MDN_ERR_TRUNCATED;
    /* Gate de ABI — IGUAL, no "menor o igual" (norma #284: el formato debe
     * COINCIDIR; si no, se recompila). El `>` de antes sólo protegía de los
     * .mdn del FUTURO y dejaba pasar los RANCIOS, que son el caso real: un
     * .mdn de ABI 1 lleva código que pasa offsets crudos a helpers que desde
     * #302 esperan HANDLES de 64 bits ⇒ corrupción y reset mudo. Preferimos
     * quedarnos sin overlay (interpretado, correcto) que ejecutar a ciegas. */
    if (h->abi_version != MDN_ABI_VERSION) {
        log_printf("MDN: RECHAZADO — ABI %u, esta VM habla %u. El .mdn es de "
                     "otra era de los helpers AOT: hay que REGENERARLO.",
                     (unsigned) h->abi_version, (unsigned) MDN_ABI_VERSION);
        return MDN_ERR_ABI;
    }

    /* Gate de arquitectura (H4, estilo #284): ejecutar código de otra ISA =
     * instrucciones basura → crash. Se rechaza. arch==0 = .mdn legacy (pre-tag,
     * siempre ARM): se acepta SOLO en firmware ARM. En el host (arch NONE) no hay
     * gate (para tests). */
    if (MDN_HOST_ARCH != MDN_ARCH_NONE) {
        uint32_t a = h->arch;
        int ok = (a == MDN_HOST_ARCH)
              || (a == MDN_ARCH_NONE && MDN_HOST_ARCH == MDN_ARCH_ARM);
        if (!ok) {
            log_printf("MDN: arch mismatch .mdn=%u firmware=%u — RECHAZADO",
                       (unsigned) a, (unsigned) MDN_HOST_ARCH);
            return MDN_ERR_ARCH;
        }
    }

    /* #502 (12-sep, medido en el C6) — LA ARCH NO BASTA EN RISC-V: el C3 y el C6
     * son RV32 con coma flotante por SOFTWARE (`ilp32`) y el unico destino RISC-V
     * que emite el compilador es el del P4 (`ilp32f`, con FPU). La cabecera del
     * .mdn no lleva la float-ABI (la del pack nativo si, y por eso los packs ya
     * la comprobaban), asi que aqui pasaba por la arch: se cargaba el bloque
     * embebido en el .mod y la placa se REINICIABA al ejecutarlo (Bench en el
     * C6: sin EXITED, uptime a cero). Un .mdn v1 no dice su receta, y hasta V7
     * solo se emitian para el P4: en un firmware RISC-V sin FPU ninguno vale
     * por construccion — se rechaza con mensaje y la funcion sigue interpretada.
     *
     * [V7/H2, #441] SOLO para v1. Un v2 lleva la huella de su receta y lo juzga
     * el gate de abajo: la de este firmware es `RV32IMC_ILP32` y la del P4
     * `RV32IMAFC_ILP32F`, asi que el del P4 se rechaza igual (MDN_ERR_RECETA) y
     * el dia que el IDE tenga el destino ilp32 su .mdn entra sin tocar nada. */
#if defined(__riscv) && !defined(__riscv_float_abi_single) && !defined(__riscv_float_abi_double)
    if (h->arch == MDN_ARCH_RISCV && h->version < 2) {
        log_printf("MDN: RECHAZADO — este firmware es RISC-V ilp32 (sin FPU) y un "
                   ".mdn v1 no dice su receta (hasta V7 solo los habia del P4, ilp32f): "
                   "se ignora y la funcion corre interpretada (#502)");
        return MDN_ERR_ARCH;
    }
#endif

    /* [V7/H2, #441] EL GATE DE LA RECETA — tras el de arch (un blob de otra ISA
     * se salta en silencio, es lo normal en un `.mod` multifamilia; uno de la
     * MISMA ISA con otros flags es el fallo real y se dice). El caso que lo
     * motivó: `-mcmodel=medany` (#440) dejó inservibles todos los `.mdn` RISC-V
     * ya generados, misma arch, misma fecha, y lo que fallaba era la placa,
     * muda. Sin receta en este firmware (CRC 0: PC, Xtensa) no hay gate. */
    {
        uint32_t mia = bpvm_mdn_host_receta_crc();
        if (h->version < 2) {
            log_printf("MDN: sin huella de receta (formato v1) — se carga a ciegas; "
                       "regenera el .mdn para que la lleve (#441)");
        } else if (mia != 0 && h->flags_crc != mia) {
            log_printf("MDN: RECHAZADO — receta %08x, este firmware es %s (%08x): "
                       "el .mdn se compilo con OTROS FLAGS, hay que REGENERARLO (#441)",
                       (unsigned) h->flags_crc, bpvm_mdn_host_receta_nombre(), (unsigned) mia);
            return MDN_ERR_RECETA;
        }
    }

    /* Layout sanity. */
    size_t hdr_total = (size_t) hsz
                     + (size_t) h->sym_count * sizeof(mdn_symbol_t);
    if (size < hdr_total + h->code_size) return MDN_ERR_TRUNCATED;

    /* Registro zero-copy: el código nativo ya está en RAM (ejecutable) en data[],
     * solo apuntamos el thunk ahí. MDN_FUNCPTR_BIT añade el bit de modo que pida
     * la ISA (ARM Thumb = 1; RISC-V/x86 = 0). */
    /* AQUI el puntero deja de ser DATO y pasa a ser CODIGO, y es el unico sitio
     * del cargador donde eso ocurre. Si el `.mdn` vive dentro de la zona de packs
     * y el micro diera direcciones distintas para instrucciones y datos, habria
     * que saltar por la vista EJECUTABLE aunque se haya parseado por la de datos.
     *
     * ⚠️ HOY NINGUNO LO HACE: medido en placa el 9-sep en C3 y S3, INST y DATA
     * salen la MISMA direccion, y la Pico, el STM32, el P4 y el C6 tampoco las
     * separan. Antes este comentario decia «(S3 y C3, que no declaran
     * SOC_MMU_DI_VADDR_SHARED)» y era FALSO — ver bpvm_pack.h. Asi que esta
     * llamada es hoy la identidad en las cinco familias; se queda por si algun
     * dia aparece un micro Harvard de verdad. Un `.mdn` del FS o embebido en un
     * `.mod` no esta en la zona y pasa de largo igualmente.
     *
     * [#565] Y ese es el que SI lo necesita: en la C3 la RAM tiene dos vistas (la
     * zona no), asi que lo que no es zona pasa por la de la cintura. */
    const uint8_t*      code_leido = data + hdr_total;
    const uint8_t*      code_base  = bpvm_pack_exec_ptr(code_leido);
    if (code_base == code_leido && s_vista_ram) {
        code_base = s_vista_ram(code_leido);
        if (code_base != code_leido)
            log_printf("MDN: el codigo se lee en %p y se ejecuta en %p (#565)",
                       (const void*) code_leido, (const void*) code_base);
    }
    const mdn_symbol_t* syms      = (const mdn_symbol_t*)
                                      (data + hsz);
    int registered = 0, directas = 0;
    for (uint32_t i = 0; i < h->sym_count; i++) {
        /* V7/H5 P2b.3 — en la v3 el bit alto es la marca DIRECTA, no parte del offset. */
        uint32_t off   = h->version >= 3 ? MDN_SIM_OFFSET(syms[i].thunk_offset) : syms[i].thunk_offset;
        uint32_t flags = (h->version >= 3 && (syms[i].thunk_offset & MDN_SIM_DIRECTA)) ? BPVM_AOT_DIRECTA : 0u;
        uintptr_t thunk_addr = (uintptr_t)(code_base + off)
                             | MDN_FUNCPTR_BIT;
        int rc = bpvm_aot_register_by_name_flags(vm, syms[i].name,
                                                   (bpvm_aot_thunk_t) thunk_addr, flags);
        if (rc == 0) {
            registered++;
            if (flags & BPVM_AOT_DIRECTA) directas++;
        } else {
            log_printf("MDN: skip '%s' rc=%d (symbol no en .mod?)",
                       syms[i].name, rc);
        }
    }
    log_printf("MDN: %d/%u thunks registrados (%d directas), %u code bytes (zero-copy)",
               registered, (unsigned) h->sym_count, directas, (unsigned) h->code_size);
    return MDN_OK;
}
