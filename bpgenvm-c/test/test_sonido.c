/*
 * test_sonido.c — [V8/H4, #708] LO COMÚN DEL SONIDO (bpvm_sonido.c), en el PC y sin que suene nada.
 *
 * Lo que SonidoParalelo (el corpus) no puede ver desde BP, con una cintura que lo APUNTA todo: el formato con el que se
 * abre, el volumen que le llega, CADA MUESTRA —las de 8 bits, pasadas a 16 con signo; las de 16, tal cual— y cómo se
 * cierra (drenando al acabar el fichero, en seco al parar o al cambiar de sonido). Y la línea `audio` del ENV, la de
 * cualquier placa con códec, con lo que tiene que salir escrito a mano. Y antes de registrar ninguna cintura, el error
 * de una placa sin sonido, que lo da la HAL BP.
 */
#include "bpvm.h"
#include "bpvm_fs.h"
#include "bpvm_platform.h"
#include "bpvm_sonido.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fallos = 0, buenas = 0;
static void ok(const char* que, int cond) {
    printf("%s %s\n", cond ? "ok   " : "FALLO", que);
    if (cond) buenas++; else fallos++;
}

/* ── La cintura que apunta ────────────────────────────────────────────────────────────────────────────────────────── */
#define MAX_MUES 65536
static uint32_t c_hz;
static int      c_canales, c_abiertos, c_cerrados, c_drenar = -1, c_vol = -1, c_pausa_ms = 0;
static int16_t  c_mues[MAX_MUES];
static uint32_t c_n;

static int ap_abrir(uint32_t hz, int canales, char* err, size_t n) {
    (void) err; (void) n;
    c_hz = hz;
    c_canales = canales;
    c_abiertos++;
    return 0;
}
static int ap_escribir(const int16_t* pcm, uint32_t m) {
    uint32_t i;
    for (i = 0; i < m && c_n < MAX_MUES; i++) c_mues[c_n++] = pcm[i];
    if (c_pausa_ms > 0) bpvm_platform_thread_sleep_ms(c_pausa_ms);   /* un sonido que dura */
    return 0;
}
static void ap_volumen(int v) { c_vol = v; }
static void ap_cerrar(int drenar) { c_cerrados++; c_drenar = drenar; }
static const bpvm_sonido_cintura_t APUNTA = { "apunta", ap_abrir, ap_escribir, ap_volumen, ap_cerrar };

static void limpia(void) { c_n = 0; c_abiertos = c_cerrados = 0; c_drenar = c_vol = -1; }

/* ── Un WAV a mano ────────────────────────────────────────────────────────────────────────────────────────────────── */
static void pon16(uint8_t* b, uint32_t v) { b[0] = (uint8_t) v; b[1] = (uint8_t) (v >> 8); }
static void pon32(uint8_t* b, uint32_t v) { pon16(b, v & 0xFFFFu); pon16(b + 2, v >> 16); }

static void wav(const char* ruta, int canales, uint32_t hz, int bits, const uint8_t* datos, uint32_t n) {
    uint8_t h[44];
    FILE* f = fopen(ruta, "wb");
    if (!f) { printf("FALLO no se pudo escribir %s\n", ruta); exit(1); }
    memcpy(h, "RIFF", 4); pon32(h + 4, 36 + n); memcpy(h + 8, "WAVE", 4);
    memcpy(h + 12, "fmt ", 4); pon32(h + 16, 16); pon16(h + 20, 1); pon16(h + 22, (uint32_t) canales);
    pon32(h + 24, hz); pon32(h + 28, hz * (uint32_t) canales * (uint32_t) bits / 8u);
    pon16(h + 32, (uint32_t) (canales * bits / 8)); pon16(h + 34, (uint32_t) bits);
    memcpy(h + 36, "data", 4); pon32(h + 40, n);
    fwrite(h, 1, sizeof h, f);
    if (n) fwrite(datos, 1, n, f);
    fclose(f);
}

static int espera(int ms) {
    while (bpvm_sonido_sonando() && ms > 0) { bpvm_platform_thread_sleep_ms(5); ms -= 5; }
    return !bpvm_sonido_sonando();
}

static int linea(const char* v, bpvm_sonido_linea_t* l, char* m) { return bpvm_sonido_linea_parse(v, l, m, 96); }

int main(void) {
    bpvm_sonido_linea_t l;
    char m[96], det[128];
    static uint8_t largo[32000];
    bpvm_fs_register_host();
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("--- V8/H4 #708 lo comun del sonido: la linea `audio` y la tarea, con una cintura que lo apunta ---\n");

    /* 1 — sin cintura: lo dice la HAL BP */
    ok("1 sin cintura: no hay sonido", !bpvm_sonido_hay());
    ok("1 sin cintura: tocar da el motivo de la HAL BP",
       bpvm_sonido_tocar("build/no_importa.wav", det, sizeof det) == BPVM_SONIDO_SIN_PLACA
       && strcmp(det, "esta placa no tiene sonido: no registra ninguno (su ENV no lleva linea `audio`)") == 0);

    /* 2 — la línea `audio` */
    ok("2 la de la P4",
       linea("es8311,i2c:0,mclk:13,bclk:12,ws:10,dout:9,pa:53", &l, m) == 0 && strcmp(l.codec, "es8311") == 0
       && l.i2c == 0 && l.dir == 0x18 && l.sda == -1 && l.scl == -1 && l.mclk == 13 && l.bclk == 12 && l.ws == 10
       && l.dout == 9 && l.pa == 53 && l.pa_alto == 1);
    ok("2 en otro orden, la dirección en hex, paalto y el bus con sus pines",
       linea("es8311, dout:9,ws:10,bclk:12,mclk:13,dir:0x19,paalto:0,pa:7,sda:7,scl:8,i2c:1", &l, m) == 0
       && l.dir == 0x19 && l.pa_alto == 0 && l.pa == 7 && l.sda == 7 && l.scl == 8 && l.i2c == 1);
    ok("2 una clave que no conoce, se salta", linea("es8311,raro:5,mclk:1,bclk:2,ws:3,dout:4", &l, m) == 0 && l.dout == 4);
    ok("2 sin pa: ninguno", linea("es8311,mclk:1,bclk:2,ws:3,dout:4", &l, m) == 0 && l.pa == -1);
    ok("2 sin dout: dice cuál falta",
       linea("es8311,mclk:1,bclk:2,ws:3", &l, m) != 0 && strcmp(m, "a la entrada 'audio' le falta dout") == 0);
    ok("2 sin el codec, lo primero",
       linea("mclk:1,bclk:2,ws:3,dout:4", &l, m) != 0
       && strcmp(m, "a la entrada 'audio' le falta el codec, lo primero (es8311)") == 0);
    ok("2 vacía", linea("", &l, m) != 0 && strcmp(m, "la entrada 'audio' esta vacia") == 0);
    ok("2 un valor que no es un número",
       linea("es8311,mclk:x,bclk:2,ws:3,dout:4", &l, m) != 0
       && strcmp(m, "valor no numerico en 'audio', cerca de mclk:x,bclk:2,ws:3,dout:4") == 0);

    /* 3 — la tarea, con la cintura que apunta */
    bpvm_sonido_registrar(&APUNTA);
    ok("3 con cintura: hay sonido", bpvm_sonido_hay());
    {   /* 8 bits, estéreo: 0, 128, 255 y 64 → -32768, 0, 32512 y -16384 */
        static const uint8_t d8[] = { 0, 128, 255, 64 };
        wav("build/son8.wav", 2, 8000, 8, d8, sizeof d8);
        limpia();
        bpvm_sonido_poner_volumen(70);
        ok("3 tocar un WAV de 8 bits", bpvm_sonido_tocar("build/son8.wav", det, sizeof det) == BPVM_SONIDO_OK);
        ok("3 acaba solo", espera(3000));
        ok("3 se abre a 8000 Hz y 2 canales", c_abiertos == 1 && c_hz == 8000 && c_canales == 2);
        ok("3 las de 8 bits, a 16 con signo",
           c_n == 4 && c_mues[0] == -32768 && c_mues[1] == 0 && c_mues[2] == 32512 && c_mues[3] == -16384);
        ok("3 el volumen le llega al abrir", c_vol == 70);
        ok("3 al acabar el fichero, cierra drenando", c_cerrados == 1 && c_drenar == 1);
        ok("3 el progreso, 4 de 4", bpvm_sonido_posicion() == 4 && bpvm_sonido_largo() == 4);
    }
    {   /* 16 bits, mono, little-endian: 1, -2 y 300 tal cual; y un byte suelto al final (medio marco) que no se toca */
        static const uint8_t d16[] = { 0x01, 0x00, 0xFE, 0xFF, 0x2C, 0x01, 0x7F };
        wav("build/son16.wav", 1, 22050, 16, d16, sizeof d16);
        limpia();
        ok("3 tocar un WAV de 16 bits", bpvm_sonido_tocar("build/son16.wav", det, sizeof det) == BPVM_SONIDO_OK);
        ok("3 acaba", espera(3000));
        ok("3 se abre a 22050 Hz y 1 canal", c_hz == 22050 && c_canales == 1);
        ok("3 las de 16 bits, tal cual, sin el medio marco",
           c_n == 3 && c_mues[0] == 1 && c_mues[1] == -2 && c_mues[2] == 300);
        ok("3 la longitud, en marcos enteros: 6", bpvm_sonido_largo() == 6);
    }
    {   /* parar: un sonido que dura (la cintura tarda 20 ms por trozo); y uno que corta al otro */
        wav("build/largo.wav", 1, 8000, 16, largo, sizeof largo);
        limpia();
        c_pausa_ms = 20;
        ok("3 tocar uno largo", bpvm_sonido_tocar("build/largo.wav", det, sizeof det) == BPVM_SONIDO_OK);
        bpvm_platform_thread_sleep_ms(100);
        bpvm_sonido_parar();
        ok("3 tras parar, no suena", !bpvm_sonido_sonando());
        bpvm_platform_thread_sleep_ms(200);
        ok("3 al parar, cierra en seco", c_cerrados == 1 && c_drenar == 0);
        ok("3 y no llegó al final", bpvm_sonido_posicion() > 0 && bpvm_sonido_posicion() < bpvm_sonido_largo());
        limpia();
        bpvm_sonido_tocar("build/largo.wav", det, sizeof det);
        bpvm_platform_thread_sleep_ms(100);
        c_pausa_ms = 0;
        ok("3 uno que corta al otro", bpvm_sonido_tocar("build/son8.wav", det, sizeof det) == BPVM_SONIDO_OK);
        ok("3 acaba el nuevo", espera(3000));
        ok("3 el de antes se cerró en seco, el nuevo drenando", c_cerrados == 2 && c_drenar == 1 && c_abiertos == 2);
        ok("3 el progreso es el del nuevo", bpvm_sonido_largo() == 4 && bpvm_sonido_posicion() == 4);
    }
    {   /* los errores de la cabecera, como los ve Sound.play */
        static const uint8_t d[] = { 1, 2 };
        FILE* f = fopen("build/nowav.txt", "wb");
        if (f) { fputs("hola", f); fclose(f); }
        wav("build/s24.wav", 1, 8000, 24, d, sizeof d);
        ok("3 no existe", bpvm_sonido_tocar("build/no_existe.wav", det, sizeof det) == BPVM_SONIDO_NO_ABRE);
        ok("3 no es un WAV", bpvm_sonido_tocar("build/nowav.txt", det, sizeof det) == BPVM_SONIDO_NO_ES_WAV);
        ok("3 24 bits", bpvm_sonido_tocar("build/s24.wav", det, sizeof det) == BPVM_SONIDO_FORMATO
                        && strcmp(det, "24 bits") == 0);
    }

    /* 4 — el fin de RUN */
    bpvm_sonido_poner_volumen(40);
    bpvm_sonido_fin_de_run();
    ok("4 el fin de RUN: el volumen, a 85; el progreso, a 0",
       bpvm_sonido_volumen() == 85 && bpvm_sonido_posicion() == 0 && bpvm_sonido_largo() == 0);

    remove("build/son8.wav"); remove("build/son16.wav"); remove("build/largo.wav");
    remove("build/nowav.txt"); remove("build/s24.wav");
    printf("[status=%s] %d/%d\n", fallos ? "FALLO" : "OK", buenas, buenas + fallos);
    return fallos ? 1 : 0;
}
