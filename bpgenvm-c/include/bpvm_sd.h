/*
 * bpvm_sd.h — V5/H1: hablar con una tarjeta SD en MODO SPI.
 *
 * ─── POR QUÉ SPI Y NO SDIO ───
 *
 * El zócalo de la Metro RP2350B está cableado, según Adafruit, *"para SPI"*, con
 * dos pines extra (`SDIO_DATA1/2`) para quien quiera SDIO de 4 bits — y ellos
 * mismos avisan de que **no hay código de SDIO ni en Arduino ni en Python**. Y
 * el mapeo cae redondo sobre el periférico:
 *
 *     GPIO34 SD_SCK   -> SPI0_SCLK      GPIO39 SD_CS          -> GPIO normal
 *     GPIO35 SD_MOSI  -> SPI0_TX        GPIO40 SD_CARD_DETECT -> GPIO normal
 *     GPIO36 SD_MISO  -> SPI0_RX
 *
 * Las tres líneas de datos y reloj caen en su función SPI0 NATIVA (comprobado en
 * la tabla de `io_bank0.h` del RP2350): cero PIO, cero bit-banging.
 *
 * ⚠️ **El CS va por GPIO normal, y eso NO es un apaño.** El `SS_N` del bloque SPI
 * se levanta entre tramas, y una transacción de SD necesita CS abajo durante
 * varios bytes seguidos. Todos los drivers SD/SPI lo conducen por software. Que
 * el GPIO39 no sea `SS_N` da igual — no lo querríamos aunque lo fuera.
 *
 * SDIO de 4 bits queda para cuando el rendimiento moleste: el cableado ya está.
 *
 * ─── POR QUÉ NO HACE FALTA NINGUNA CINTURA NUEVA ───
 *
 * El diálogo con la tarjeta (CMD0, CMD8, ACMD41, CMD58, CMD9/10) es idéntico en
 * cualquier micro. Lo único que cambia es "mándame estos bytes por SPI" y "mueve
 * este pin", y de eso ya hay fachada portable en las tres familias:
 * `bpvm_spi_*` (bpvm_spi.h) y `bpvm_gpio_*` (bpvm_gpio.h). Así que este fichero
 * es 100 % portable y no lleva un solo #ifdef de familia.
 *
 * ─── LA ESCALERA ───
 *
 * Criterio de Eduardo: *"hay que poner chivatos que nos digan qué es lo que no
 * funciona, para saber dónde y no sólo funciona/no funciona"*. Por eso el
 * resultado no es un bool: es el PELDAÑO donde se paró. La diferencia entre
 * "no hay tarjeta metida", "hay tarjeta pero el bus está mudo" y "contesta pero
 * no arranca" manda a sitios completamente distintos — la primera es el zócalo,
 * la segunda el cableado, la tercera la tarjeta o la alimentación.
 */
#ifndef BPVM_SD_H
#define BPVM_SD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Los peldaños, en el orden en que se suben. */
typedef enum {
    BPVM_SD_OK = 0,
    BPVM_SD_E_PINES,        /* la configuración no es usable (pines sin poner)  */
    BPVM_SD_E_SIN_TARJETA,  /* el card-detect dice que el zócalo está vacío     */
    BPVM_SD_E_MUDA,         /* CMD0 no contesta NADA: bus mal cableado o sin Vcc*/
    BPVM_SD_E_NO_IDLE,      /* contesta, pero no entra en IDLE                  */
    BPVM_SD_E_VOLTAJE,      /* CMD8: no acepta 2,7-3,6 V, o el eco no cuadra    */
    BPVM_SD_E_NO_ARRANCA,   /* ACMD41 no sale de IDLE dentro del plazo          */
    BPVM_SD_E_OCR,          /* CMD58 (leer OCR) falló                           */
    BPVM_SD_E_CID,          /* CMD10 (leer CID) falló                           */
    BPVM_SD_E_CSD,          /* CMD9 (leer CSD) falló                            */
    BPVM_SD_E_CSD_VER,      /* el CSD dice una versión que no sabemos decodificar*/
    BPVM_SD_E_LECTURA       /* CMD17: la tarjeta arrancó pero no entrega datos   */
} bpvm_sd_res_t;

/* Texto del peldaño. Literal estático; nunca hay que liberarlo. */
const char* bpvm_sd_res_str(bpvm_sd_res_t r);

/* Los pines. Salen del ENV (criterio de Eduardo: la config de placa va en el
 * entorno, no horneada) — ver [[config-de-placa-en-el-env]]. `cd` negativo =
 * esta placa no tiene detector de tarjeta, y entonces el peldaño SIN_TARJETA
 * no puede darse: lo dirá MUDA, que es menos preciso pero honesto. */
typedef struct {
    int bus;    /* instancia SPI (0/1) */
    int sck, mosi, miso;
    int cs;     /* GPIO normal, salida */
    int cd;     /* GPIO normal, entrada; <0 = no hay */
    int cd_activo_bajo;  /* casi siempre 1: tarjeta dentro => pin a 0 */
} bpvm_sd_pines_t;

/*
 * Lee la entrada `sd` del ENV. UNA sola entrada con etiquetas dentro — decisión
 * de Eduardo (8-ago): *"en el env en vez de n entradas, se puede crear una
 * entrada nada mas con toda la configuración. Mas sencillo para el usuario"*.
 *
 *     sd=sck:34,mosi:35,miso:36,cs:39,cd:40
 *
 * Con etiquetas y no por posición porque así el orden da igual y, sobre todo,
 * porque cuando falte algo el mensaje puede DECIR QUÉ falta en vez de dejar un
 * pin a cero en silencio. `bus` (0) y `cd` (ninguno) tienen valor por defecto;
 * los cuatro de SPI son obligatorios.
 *
 * ⚠️ Las claves que no conoce se IGNORAN a propósito: así una versión vieja del
 * firmware no rechaza una línea escrita para una nueva. El precio es que una
 * errata en un campo opcional (`dc:40` por `cd:40`) se traga en silencio — pero
 * una errata en uno obligatorio sale como "falta miso", que es lo que importa.
 *
 * Devuelve 0, o -1 y escribe en `motivo` (si no es NULL) un texto para el log.
 */
int bpvm_sd_pines_parse(const char* valor, bpvm_sd_pines_t* p,
                        char* motivo, unsigned motivo_cap);

/* Lo que la tarjeta dice de sí misma. */
typedef struct {
    uint8_t  version;    /* 1 = SDSC (CSD v1), 2 = SDHC/SDXC (CSD v2)          */
    uint8_t  alta_cap;   /* 1 si CCS del OCR está puesto: direcciona por BLOQUE */
    uint8_t  cid[16];
    uint8_t  csd[16];
    uint32_t bloques;    /* capacidad en bloques de 512 B                      */
    /* Sacados del CID, ya legibles (el CID es empaquetado y poco amable). */
    uint8_t  fabricante;      /* MID */
    char     oem[3];          /* OID, con NUL */
    char     producto[6];     /* PNM, con NUL */
    uint8_t  rev_mayor, rev_menor;
    uint32_t serie;           /* PSN */
    uint16_t anno;            /* MDT ya sumado a 2000 */
    uint8_t  mes;

    /* ─── DIAGNÓSTICO: qué se leyó DE VERDAD ───
     *
     * Se rellena SIEMPRE, también (y sobre todo) cuando la escalera se para. Un
     * peldaño dice dónde, pero no QUÉ contestó la tarjeta, y ahí está la
     * diferencia entre "me responde algo raro" y "estoy leyendo una línea
     * flotante": si la MISO no tiene pull-up, el pad del RP2350 arranca en
     * pull-DOWN y se lee 0x00 — que tiene el bit 7 a cero y por tanto pasa por
     * respuesta válida. Con la traza eso se ve; sin ella, se adivina. */
    uint8_t  ultimo_cmd;      /* el último comando enviado                     */
    uint8_t  traza[8];        /* los bytes leídos esperando SU respuesta       */
} bpvm_sd_info_t;

/*
 * Arranca la tarjeta y la deja lista para leer bloques, rellenando `info`.
 *
 * Hace la secuencia estándar: 74+ pulsos de reloj con CS alto a ~400 kHz, CMD0
 * (a IDLE), CMD8 (¿v2?), ACMD41 hasta que arranque, CMD58 (¿alta capacidad?),
 * CMD10/CMD9 (CID y CSD) — y sólo al final sube el reloj a `baud_rapido`.
 *
 * ⚠️ Lo de los 400 kHz no es una precaución de manual: la tarjeta EXIGE estar
 * por debajo de 400 kHz mientras negocia. Subir antes de tiempo es un fallo que
 * se manifiesta como "unas tarjetas sí y otras no", que es el peor de todos.
 *
 * Devuelve OK o el peldaño donde se paró. No aloca ni escribe fuera de `info`.
 */
bpvm_sd_res_t bpvm_sd_init(const bpvm_sd_pines_t* pines, int baud_rapido,
                           bpvm_sd_info_t* info);

/* ¿Hay tarjeta metida? Sólo mira el detector; si la placa no lo tiene devuelve
 * 1 (no podemos saberlo, y decir "no hay" sería mentir). */
int bpvm_sd_hay_tarjeta(const bpvm_sd_pines_t* pines);

/*
 * Decodifica la capacidad a partir del CSD. SEPARADA A PROPÓSITO: es la parte
 * con dos fórmulas distintas (CSD v1 y v2) y campos a caballo entre bytes, o
 * sea el sitio natural de un error silencioso que se manifiesta como "la
 * tarjeta de 32 GB dice que tiene 4". Al ser función pura se prueba en el PC
 * con CSD reales, sin placa y sin tarjeta.
 *
 * Devuelve 0 y deja `bloques`/`version` puestos, o el peldaño CSD_VER.
 */
bpvm_sd_res_t bpvm_sd_csd_capacidad(const uint8_t csd[16],
                                    uint32_t* bloques, uint8_t* version);

/* Rellena los campos legibles a partir del CID. Pura, como la de arriba. */
void bpvm_sd_cid_desglosar(const uint8_t cid[16], bpvm_sd_info_t* info);

/*
 * Lee UN bloque de 512 B. `lba` es siempre el número de bloque; la conversión a
 * byte para las tarjetas antiguas la hace esta función mirando `alta_cap`.
 *
 * ⚠️ Y ése es justo el sitio donde equivocarse sale caro: en una SDHC/SDXC el
 * argumento de CMD17 es el BLOQUE, y en una SDSC es el BYTE. Confundirlos no da
 * error — lee otro sitio, multiplicado o dividido por 512. Por eso el `alta_cap`
 * no se supone: sale del OCR, que se pregunta en el arranque.
 *
 * `info` tiene que venir del `bpvm_sd_init` que salió bien: de ahí sale el
 * modo de direccionamiento, y ahí se deja la traza si algo falla.
 */
bpvm_sd_res_t bpvm_sd_leer_bloque(const bpvm_sd_pines_t* pines,
                                  bpvm_sd_info_t* info,
                                  uint32_t lba, uint8_t dst[512]);

#ifdef __cplusplus
}
#endif
#endif /* BPVM_SD_H */
