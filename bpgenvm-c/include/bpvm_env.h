/*
 * bpvm_env.h — H9: bloque de "environment" en flash (identidad de placa +, más
 * adelante, tabla de particiones). Lo lee el KERNEL en el estado 0 —SIN heap—,
 * así que el parser es diminuto y opera sobre buffers que da el llamador (la
 * cintura de flash por-micro NO vive aquí: aquí solo va el FORMATO y la lógica,
 * host-testable). Diseño: docs/H9_KERNEL_CAPAS.md §Formato del bloque de env.
 *
 * Formato: variables "clave=valor" (modelo env, NO JSON — se parsea en el suelo
 * sin heap y es abierto por construcción: una clave desconocida se ignora, así
 * que añadir entradas nunca rompe a un kernel viejo). Marco big-endian:
 *
 *   off 0  magic[4]   "BPEV"
 *   off 4  version    u16   (BPVM_ENV_VERSION)
 *   off 6  len        u16   (bytes de payload)
 *   off 8  crc        u32   (CRC-32 sobre [seq+payload] = bytes [12 .. 16+len))
 *   off 12 seq        u32   (para el A/B: gana la copia válida con seq mayor)
 *   off 16 payload    "clave=valor\n clave=valor\n ..."
 *   ...    0xFF pad    hasta el fin del sector
 *
 * El CRC cubre seq+payload (contiguos) → detecta escritura a medias; magic +
 * version + len se validan estructuralmente. A/B (dos copias en dos sectores):
 * la robustez contra corte de corriente a mitad de escritura.
 */
#ifndef BPVM_ENV_H
#define BPVM_ENV_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_ENV_MAGIC        "BPEV"
#define BPVM_ENV_VERSION      1u
#define BPVM_ENV_HEADER_SIZE  16u   /* magic(4)+version(2)+len(2)+crc(4)+seq(4) */
#define BPVM_ENV_MAX_PAYLOAD  0xFFFFu

/* Vista de SOLO LECTURA sobre un bloque validado. `payload` apunta DENTRO del
 * buffer del llamador (no se copia, no se aloca): el buffer debe seguir vivo. */
typedef struct {
    const uint8_t* payload;      /* inicio de "clave=valor\n..."; NULL si inválido */
    uint16_t       payload_len;  /* bytes de payload */
    uint32_t       seq;          /* secuencia (A/B) */
    int            valid;        /* 1 si magic+version+len+crc OK */
} bpvm_env_t;

/* Valida y parsea un bloque. 1 si válido (rellena `env`), 0 si no (env->valid=0).
 * No aloca. */
int bpvm_env_parse(const uint8_t* block, size_t block_len, bpvm_env_t* env);

/* Elige entre las copias A y B la VÁLIDA con `seq` mayor. Escribe la elegida en
 * `out`. Devuelve 0 si gana A, 1 si gana B, -1 si NINGUNA es válida (out->valid=0). */
int bpvm_env_pick(const uint8_t* blockA, size_t lenA,
                  const uint8_t* blockB, size_t lenB, bpvm_env_t* out);

/* Busca `key`. Copia el valor (sin '\n', NUL-terminado) en `val_out` (cap `val_cap`).
 * Devuelve la longitud del valor (>=0), o -1 si la clave no está (tolerante). */
int bpvm_env_get(const bpvm_env_t* env, const char* key, char* val_out, size_t val_cap);

/* Conveniencias tipadas (delegan en bpvm_env_get → una sola fuente). `def` si la
 * clave falta o no parsea. bool acepta 1/0, y/n, true/false, yes/no, on/off. */
int  bpvm_env_get_bool(const bpvm_env_t* env, const char* key, int def);
long bpvm_env_get_long(const bpvm_env_t* env, const char* key, long def);

/* Serializa un payload "clave=valor\n..." a un bloque completo (marco + crc + seq)
 * en `out` (cap `out_cap`, típicamente el tamaño de sector), rellenando con 0xFF
 * hasta `out_cap`. Devuelve los bytes usados (marco+payload, sin el pad), o -1 si
 * no cabe. El llamador (cintura de flash) borra el sector y escribe `out`. */
int bpvm_env_serialize(const char* payload, size_t payload_len, uint32_t seq,
                       uint8_t* out, size_t out_cap);

/* Siguiente `seq` para una escritura A/B: max(seqA_válido, seqB_válido) + 1, o 1
 * si ninguna es válida. La copia a escribir es la que NO ganó bpvm_env_pick. */
uint32_t bpvm_env_next_seq(const uint8_t* blockA, size_t lenA,
                           const uint8_t* blockB, size_t lenB);

/* Edita un PAYLOAD de env (no el bloque): reemplaza —o añade si no existe— la
 * línea `key=value`; si `value` es NULL, BORRA la clave. Copia el resultado a
 * `out` (cap). Preserva las demás claves. Devuelve la nueva longitud (>=0) o -1 si
 * no cabe. Es la lógica detrás de ENV_SET/ENV_DEL del protocolo de gestión de placa:
 * el kernel lee el env, edita el payload, y re-serializa a la copia A/B rancia. */
int bpvm_env_payload_set(const char* payload_in, size_t in_len,
                         const char* key, const char* value,
                         char* out, size_t out_cap);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_ENV_H */
