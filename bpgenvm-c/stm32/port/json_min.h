/*
 * json_min.h — parser JSON minimal (copia portable del de pico/, para el
 * firmware STM32). C99 puro, sin deps. Subset del wire v1
 * (docs/BPVM_WIRE_PROTOCOL.md): objeto top-level plano + objeto anidado de
 * 1 nivel (capturado como string crudo). Sin floats, sin arrays anidados.
 */
#ifndef BPVM_JSON_MIN_H
#define BPVM_JSON_MIN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JSON_MAX_PAIRS  16

typedef enum {
    JSON_TYPE_STRING,
    JSON_TYPE_NUMBER,
    JSON_TYPE_BOOL,
    JSON_TYPE_NULL,
    JSON_TYPE_OBJECT
} json_value_type_t;

typedef struct {
    const char*       key;
    size_t            key_len;
    const char*       value;
    size_t            value_len;
    json_value_type_t type;
} json_pair_t;

typedef struct {
    json_pair_t pairs[JSON_MAX_PAIRS];
    int         n_pairs;
} json_obj_t;

int json_parse(const char* buf, size_t len, json_obj_t* out);
const json_pair_t* json_find(const json_obj_t* obj, const char* key);
long json_get_long(const json_obj_t* obj, const char* key, long def);
int  json_get_bool(const json_obj_t* obj, const char* key, int def);
int  json_get_str(const json_obj_t* obj, const char* key, char* dst, size_t dst_size);
const char* json_get_str_raw(const json_obj_t* obj, const char* key, size_t* out_len);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_JSON_MIN_H */
