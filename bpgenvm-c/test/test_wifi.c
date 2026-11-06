/*
 * test_wifi.c — [V8/H3] LO COMÚN DE LA WIFI (bpvm_wifi.c, bpvm_red.c), en el PC y sin radio.
 *
 * LA MÁQUINA, con un reloj de mentira: los reintentos 1, 2, 4… hasta 60 s, los tres rechazos de la clave, el apagado por
 * inactividad con su regla (una conexión abierta lo aguanta; al cerrarla, otros `autoOff` segundos) y lo que no debe pasar
 * (un aviso tardío con la radio apagada). Una hora de placa en un milisegundo.
 *
 * LOS FICHEROS: /sys/wifi.json y /sys/net.json, con lo que tiene que salir escrito a mano, y sus erratas, con su motivo.
 *
 * LA TAREA, de verdad y con una radio que APUNTA: publicada como 'WIFI' por el mismo registro que usa un pack, encender,
 * conectarse, la clave mala, apagar, la tabla sin una función y el Entorno mandando sobre el fichero.
 */
#include "bpvm.h"
#include "bpvm_bios.h"
#include "bpvm_fs.h"
#include "bpvm_net.h"
#include "bpvm_pack_api.h"
#include "bpvm_platform.h"
#include "bpvm_red.h"
#include "bpvm_wifi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fallos = 0, buenas = 0;
static void ok(const char* que, int cond) {
    printf("%s %s\n", cond ? "ok   " : "FALLO", que);
    if (cond) buenas++; else fallos++;
}

/* ── La máquina ────────────────────────────────────────────────────────────────────────────────────────────────────── */

static int paso(bpvm_wifi_maq_t* m, int e, int64_t t) { return bpvm_wifi_maq_paso(m, e, t); }

/* Encendida y conectándose, en t. */
static void hasta_conectando(bpvm_wifi_maq_t* m, int64_t t) {
    bpvm_wifi_maq_iniciar(m);
    paso(m, BPVM_WIFI_E_ENCENDER, t);
    paso(m, BPVM_WIFI_E_ARRIBA, t);
}

static void prueba_encender(void) {
    bpvm_wifi_maq_t m;
    bpvm_wifi_maq_iniciar(&m);
    ok("empieza apagada", m.estado == BPVM_WIFI_OFF);
    ok("apagada, nada que esperar", bpvm_wifi_maq_cuando(&m) == 0);
    ok("apagar lo apagado no hace nada", paso(&m, BPVM_WIFI_E_APAGAR, 0) == BPVM_WIFI_A_NADA);
    ok("encender pide encender la radio", paso(&m, BPVM_WIFI_E_ENCENDER, 10) == BPVM_WIFI_A_ENCENDER);
    ok("  y va a starting", m.estado == BPVM_WIFI_STARTING);
    ok("encender dos veces, una", paso(&m, BPVM_WIFI_E_ENCENDER, 11) == BPVM_WIFI_A_NADA);
    ok("arriba: a conectarse", paso(&m, BPVM_WIFI_E_ARRIBA, 20) == BPVM_WIFI_A_CONECTAR);
    ok("  connecting", m.estado == BPVM_WIFI_CONNECTING);
    paso(&m, BPVM_WIFI_E_CONECTADA, 30);
    ok("conectada", m.estado == BPVM_WIFI_CONNECTED);
    ok("apagar: se apaga la radio", paso(&m, BPVM_WIFI_E_APAGAR, 40) == BPVM_WIFI_A_APAGAR);
    ok("  off", m.estado == BPVM_WIFI_OFF);

    /* Lo que llega tarde, con la radio apagada: al apagar, la radio aún cuenta que se ha desconectado. */
    ok("un aviso tardio no despierta nada", paso(&m, BPVM_WIFI_E_CAIDA, 41) == BPVM_WIFI_A_NADA && m.estado == BPVM_WIFI_OFF);
    ok("  ni otro", paso(&m, BPVM_WIFI_E_CONECTADA, 42) == BPVM_WIFI_A_NADA && m.estado == BPVM_WIFI_OFF);

    bpvm_wifi_maq_iniciar(&m);
    paso(&m, BPVM_WIFI_E_ENCENDER, 0);
    ok("no arranca: se limpia lo que dejara a medias", paso(&m, BPVM_WIFI_E_NO_ARRANCA, 5) == BPVM_WIFI_A_APAGAR);
    ok("  y se queda en off", m.estado == BPVM_WIFI_OFF);
}

static void prueba_reintentos(void) {
    static const int ESPERADO[] = { 1, 2, 4, 8, 16, 32, 60, 60, 60 };
    bpvm_wifi_maq_t m;
    int64_t t = 1000;
    int i, bien = 1;
    hasta_conectando(&m, t);
    m.auto_off = 0;
    for (i = 0; i < 9; i++) {
        int64_t c;
        paso(&m, BPVM_WIFI_E_SIN_RED, t);
        c = bpvm_wifi_maq_cuando(&m);
        if (m.estado != BPVM_WIFI_NO_NETWORK || c != t + ESPERADO[i] * 1000) bien = 0;
        if (paso(&m, BPVM_WIFI_E_TIEMPO, c - 1) != BPVM_WIFI_A_NADA) bien = 0;      /* antes de la hora, nada */
        if (paso(&m, BPVM_WIFI_E_TIEMPO, c) != BPVM_WIFI_A_CONECTAR) bien = 0;      /* a la hora, otra vez */
        t = c;
    }
    ok("los reintentos: 1, 2, 4, 8, 16, 32, 60, 60, 60 s", bien);
    ok("  y conectandose otra vez", m.estado == BPVM_WIFI_CONNECTING);
    paso(&m, BPVM_WIFI_E_CONECTADA, t);
    paso(&m, BPVM_WIFI_E_CAIDA, t + 5000);
    ok("conectar reinicia la espera: se cae, y al segundo", bpvm_wifi_maq_cuando(&m) == t + 5000 + 1000);
    ok("  en noNetwork", m.estado == BPVM_WIFI_NO_NETWORK);
    paso(&m, BPVM_WIFI_E_CONECTADA, t + 5500);
    ok("noNetwork y conectada: la radio se ha adelantado", m.estado == BPVM_WIFI_CONNECTED && bpvm_wifi_maq_cuando(&m) == 0);

    /* La radio no contesta: sin aviso, a los 30 s se da el intento por perdido. */
    hasta_conectando(&m, 0);
    m.auto_off = 0;
    ok("conectando: se vigila", bpvm_wifi_maq_cuando(&m) == BPVM_WIFI_CONECTANDO_MS);
    paso(&m, BPVM_WIFI_E_TIEMPO, BPVM_WIFI_CONECTANDO_MS);
    ok("  y sin noticias, a reintentar", m.estado == BPVM_WIFI_NO_NETWORK);
}

static void prueba_clave(void) {
    bpvm_wifi_maq_t m;
    hasta_conectando(&m, 0);
    ok("1 rechazo: reintenta", paso(&m, BPVM_WIFI_E_CLAVE, 100) == BPVM_WIFI_A_NADA && m.estado == BPVM_WIFI_NO_NETWORK);
    paso(&m, BPVM_WIFI_E_TIEMPO, 1100);
    ok("2 rechazos: reintenta", paso(&m, BPVM_WIFI_E_CLAVE, 1200) == BPVM_WIFI_A_NADA && m.estado == BPVM_WIFI_NO_NETWORK);
    paso(&m, BPVM_WIFI_E_TIEMPO, 3200);
    ok("3 rechazos: se apaga", paso(&m, BPVM_WIFI_E_CLAVE, 3300) == BPVM_WIFI_A_APAGAR);
    ok("  y dice badKey", m.estado == BPVM_WIFI_BAD_KEY);
    ok("  sin nada que esperar", bpvm_wifi_maq_cuando(&m) == 0);
    ok("apagar deja de decir badKey, sin tocar la radio", paso(&m, BPVM_WIFI_E_APAGAR, 3400) == BPVM_WIFI_A_NADA && m.estado == BPVM_WIFI_OFF);

    hasta_conectando(&m, 0);
    paso(&m, BPVM_WIFI_E_CLAVE, 100);
    paso(&m, BPVM_WIFI_E_TIEMPO, 1100);
    paso(&m, BPVM_WIFI_E_CLAVE, 1200);
    paso(&m, BPVM_WIFI_E_TIEMPO, 3200);
    paso(&m, BPVM_WIFI_E_CONECTADA, 3300);
    paso(&m, BPVM_WIFI_E_CLAVE, 9000);
    ok("una conexion buena borra los rechazos", m.estado == BPVM_WIFI_NO_NETWORK && m.fallos_clave == 1);

    hasta_conectando(&m, 0);
    paso(&m, BPVM_WIFI_E_CLAVE, 100);
    paso(&m, BPVM_WIFI_E_TIEMPO, 1100);
    paso(&m, BPVM_WIFI_E_SIN_RED, 1200);
    paso(&m, BPVM_WIFI_E_TIEMPO, 3200);
    paso(&m, BPVM_WIFI_E_CLAVE, 3300);
    ok("un 'no aparece' de por medio no cuenta como rechazo", m.fallos_clave == 2 && m.estado == BPVM_WIFI_NO_NETWORK);

    /* Encenderla otra vez, desde badKey, es volver a probar: la clave puede ser otra. */
    hasta_conectando(&m, 0);
    m.estado = BPVM_WIFI_BAD_KEY;
    m.fallos_clave = 3;
    ok("desde badKey se puede encender", paso(&m, BPVM_WIFI_E_ENCENDER, 10) == BPVM_WIFI_A_ENCENDER);
    ok("  con los rechazos a cero", m.fallos_clave == 0);
}

static void prueba_apagado(void) {
    bpvm_wifi_maq_t m;
    hasta_conectando(&m, 0);
    m.auto_off = 600;
    paso(&m, BPVM_WIFI_E_CONECTADA, 5000);
    ok("conectarse cuenta como actividad", bpvm_wifi_maq_cuando(&m) == 605000);
    ok("antes de la hora no se apaga", paso(&m, BPVM_WIFI_E_TIEMPO, 604999) == BPVM_WIFI_A_NADA);
    bpvm_wifi_maq_actividad(&m, 100000);
    ok("una busqueda vuelve a contar desde ahi", bpvm_wifi_maq_cuando(&m) == 700000);
    bpvm_wifi_maq_conexion(&m, 1, 200000);
    ok("con una conexion abierta no hay hora de apagado", bpvm_wifi_maq_cuando(&m) == 0);
    ok("  aunque pase el tiempo", paso(&m, BPVM_WIFI_E_TIEMPO, 9000000) == BPVM_WIFI_A_NADA && m.estado == BPVM_WIFI_CONNECTED);
    bpvm_wifi_maq_conexion(&m, 1, 300000);
    bpvm_wifi_maq_conexion(&m, 0, 400000);
    ok("con dos abiertas, cerrar una no cuenta", bpvm_wifi_maq_cuando(&m) == 0);
    bpvm_wifi_maq_conexion(&m, 0, 500000);
    ok("al cerrar la ultima, otros autoOff segundos", bpvm_wifi_maq_cuando(&m) == 1100000);
    bpvm_wifi_maq_conexion(&m, 0, 500000);
    ok("cerrar de mas no deja la cuenta en negativo", m.conexiones == 0);
    ok("a la hora se apaga", paso(&m, BPVM_WIFI_E_TIEMPO, 1100000) == BPVM_WIFI_A_APAGAR && m.estado == BPVM_WIFI_OFF);

    hasta_conectando(&m, 0);
    m.auto_off = 0;
    paso(&m, BPVM_WIFI_E_CONECTADA, 5000);
    ok("autoOff 0: no se apaga nunca", bpvm_wifi_maq_cuando(&m) == 0);
    ok("  ni mirando", paso(&m, BPVM_WIFI_E_TIEMPO, 900000000) == BPVM_WIFI_A_NADA);

    /* Sin red también se apaga: una placa que busca su red a pilas no debe hacerlo para siempre. */
    hasta_conectando(&m, 0);
    m.auto_off = 100;
    paso(&m, BPVM_WIFI_E_SIN_RED, 1000);
    { int i;
      for (i = 0; i < 40 && m.estado != BPVM_WIFI_OFF; i++) {
          int64_t c = bpvm_wifi_maq_cuando(&m);
          if (paso(&m, BPVM_WIFI_E_TIEMPO, c) == BPVM_WIFI_A_CONECTAR) paso(&m, BPVM_WIFI_E_SIN_RED, c + 500);
      }
      ok("sin red, a los autoOff segundos tambien se apaga", m.estado == BPVM_WIFI_OFF); }

    /* El reintento y el apagado, a la vez: gana el que llegue antes. */
    hasta_conectando(&m, 0);
    m.auto_off = 30;
    paso(&m, BPVM_WIFI_E_SIN_RED, 29500);
    ok("la hora que importa es la primera", bpvm_wifi_maq_cuando(&m) == 30000);
}

/* ── Los ficheros ──────────────────────────────────────────────────────────────────────────────────────────────────── */

static int cfg_de(const char* j, bpvm_wifi_cfg_t* c, char* mot) { return bpvm_wifi_cfg_parse(j, strlen(j), c, mot, 96); }
static int red_de(const char* j, bpvm_red_cfg_t* c, char* mot)  { return bpvm_red_cfg_parse(j, strlen(j), c, mot, 96); }

static void prueba_ficheros(void) {
    bpvm_wifi_cfg_t c;
    bpvm_red_cfg_t r;
    char mot[96], t[20];
    uint32_t ip;

    ok("wifi.json entero", cfg_de("{\"ssid\":\"Casa\",\"password\":\"clave 1\",\"country\":\"ES\",\"autoStart\":true,\"autoOff\":0}", &c, mot) == 0
       && !strcmp(c.ssid, "Casa") && !strcmp(c.clave, "clave 1") && !strcmp(c.pais, "ES") && c.auto_start == 1 && c.auto_off == 0);
    ok("con saltos de linea, como lo deja un editor", cfg_de("{\n  \"ssid\": \"Casa\",\n  \"autoOff\": 60\n}", &c, mot) == 0 && c.auto_off == 60);
    ok("solo el ssid: los demas, por defecto", cfg_de("{\"ssid\":\"Casa\"}", &c, mot) == 0 && c.clave[0] == 0 && c.pais[0] == 0
       && c.auto_start == 0 && c.auto_off == 600);
    ok("una clave con comillas y barra", cfg_de("{\"ssid\":\"a\",\"password\":\"x\\\"y\\\\z\"}", &c, mot) == 0 && !strcmp(c.clave, "x\"y\\z"));
    ok("sin ssid", cfg_de("{\"password\":\"x\"}", &c, mot) != 0 && !strcmp(mot, "falta ssid"));
    ok("ssid vacio", cfg_de("{\"ssid\":\"\"}", &c, mot) != 0 && !strcmp(mot, "falta ssid"));
    ok("ssid largo", cfg_de("{\"ssid\":\"123456789012345678901234567890123\"}", &c, mot) != 0 && !strcmp(mot, "ssid no cabe (32 bytes)"));
    ok("ssid de 32 justos", cfg_de("{\"ssid\":\"12345678901234567890123456789012\"}", &c, mot) == 0);
    ok("clave larga", cfg_de("{\"ssid\":\"a\",\"password\":\"12345678901234567890123456789012345678901234567890123456789012345\"}", &c, mot) != 0
       && !strcmp(mot, "password no cabe (64 bytes)"));
    ok("country de tres letras", cfg_de("{\"ssid\":\"a\",\"country\":\"ESP\"}", &c, mot) != 0 && !strcmp(mot, "country son dos letras (ES)"));
    ok("country de una", cfg_de("{\"ssid\":\"a\",\"country\":\"E\"}", &c, mot) != 0);
    ok("autoOff negativo", cfg_de("{\"ssid\":\"a\",\"autoOff\":-1}", &c, mot) != 0 && !strcmp(mot, "autoOff son segundos (0: no se apaga sola)"));
    ok("no es JSON", cfg_de("ssid=Casa", &c, mot) != 0 && !strcmp(mot, "no es JSON valido"));
    ok("el motivo no lleva la clave", cfg_de("{\"ssid\":\"\",\"password\":\"secreta\"}", &c, mot) != 0 && !strstr(mot, "secreta"));

    ok("ip: 192.168.1.50", bpvm_red_ip_parse("192.168.1.50", &ip) == 0 && ip == 0x3201A8C0u);
    bpvm_red_ip_texto(0x3201A8C0u, t, sizeof t);
    ok("  y de vuelta", !strcmp(t, "192.168.1.50"));
    ok("ip: 0.0.0.0", bpvm_red_ip_parse("0.0.0.0", &ip) == 0 && ip == 0);
    ok("ip: 255.255.255.255", bpvm_red_ip_parse("255.255.255.255", &ip) == 0 && ip == 0xFFFFFFFFu);
    ok("ip mala: 256", bpvm_red_ip_parse("192.168.1.256", &ip) != 0);
    ok("ip mala: tres numeros", bpvm_red_ip_parse("192.168.1", &ip) != 0);
    ok("ip mala: cinco", bpvm_red_ip_parse("1.2.3.4.5", &ip) != 0);
    ok("ip mala: letras", bpvm_red_ip_parse("1.2.x.4", &ip) != 0);
    ok("ip mala: cuatro cifras", bpvm_red_ip_parse("1.2.3.1000", &ip) != 0);
    ok("ip mala: vacia", bpvm_red_ip_parse("", &ip) != 0);

    ok("net.json: el nombre", red_de("{\"hostname\":\"taller\"}", &r, mot) == 0 && !r.fija && !strcmp(r.nombre, "taller"));
    ok("net.json: vacio es DHCP", red_de("{}", &r, mot) == 0 && !r.fija && !r.nombre[0]);
    ok("net.json: la IP fija entera", red_de("{\"ip\":\"192.168.1.50\",\"mask\":\"255.255.255.0\",\"gateway\":\"192.168.1.1\",\"dns\":\"8.8.8.8\"}", &r, mot) == 0
       && r.fija && r.ip == 0x3201A8C0u && r.mascara == 0x00FFFFFFu && r.puerta == 0x0101A8C0u && r.dns == 0x08080808u);
    ok("net.json: sin puerta ni DNS, una red aislada", red_de("{\"ip\":\"10.0.0.2\",\"mask\":\"255.0.0.0\"}", &r, mot) == 0 && r.fija && !r.puerta && !r.dns);
    ok("net.json: ip sin mask", red_de("{\"ip\":\"10.0.0.2\"}", &r, mot) != 0 && strstr(mot, "mask"));
    ok("net.json: ip mala", red_de("{\"ip\":\"10.0.0\",\"mask\":\"255.0.0.0\"}", &r, mot) != 0 && !strcmp(mot, "ip no es una direccion"));
    ok("net.json: gateway malo", red_de("{\"ip\":\"10.0.0.2\",\"mask\":\"255.0.0.0\",\"gateway\":\"x\"}", &r, mot) != 0 && !strcmp(mot, "gateway no es una direccion"));
}

/* ── La tarea, con una radio que apunta ────────────────────────────────────────────────────────────────────────────── */

static bpvm_wifi_aviso_fn r_aviso = NULL;
static volatile int r_encendidas = 0, r_apagadas = 0, r_conexiones = 0, r_falla_encender = 0;
static char r_ssid[40], r_clave[70], r_pais[4];
static volatile int r_responde = BPVM_WIFI_AV_CONECTADA;

static int  ap_encender(const char* pais, bpvm_wifi_aviso_fn aviso, char* err, size_t n) {
    snprintf(r_pais, sizeof r_pais, "%s", pais);
    r_aviso = aviso;
    r_encendidas++;
    if (r_falla_encender) { snprintf(err, n, "sin radio"); return -1; }
    return 0;
}
static void ap_apagar(void) { r_apagadas++; }
static int  ap_conectar(const char* ssid, const char* clave, char* err, size_t n) {
    (void) err; (void) n;
    snprintf(r_ssid, sizeof r_ssid, "%s", ssid);
    snprintf(r_clave, sizeof r_clave, "%s", clave);
    r_conexiones++;
    r_aviso(r_responde, 0);     /* como la familia, desde otra tarea; aquí basta con que sea asíncrono para la máquina */
    return 0;
}
static int  ap_rssi(void) { return -52; }

static const bpvm_pack_sym_t SIMBOLOS[] = {
    { "encender", (bpvm_pack_fn_t) ap_encender },
    { "apagar",   (bpvm_pack_fn_t) ap_apagar },
    { "conectar", (bpvm_pack_fn_t) ap_conectar },
    { "rssi",     (bpvm_pack_fn_t) ap_rssi },
};
static const bpvm_pack_api_t TABLA = { BPVM_WIFI_MARCA, BPVM_WIFI_API_VER, 4, SIMBOLOS };

/* La IP, un backend que apunta. */
static char n_nombre[48];
static uint32_t n_fija;
static int n_dinamicas = 0, n_fijas = 0;
static int ip_dinamica(const char* nombre) { snprintf(n_nombre, sizeof n_nombre, "%s", nombre); n_dinamicas++; return 0; }
static int ip_fija(uint32_t ip, uint32_t m, uint32_t p, uint32_t d) { (void) m; (void) p; (void) d; n_fija = ip; n_fijas++; return 0; }
static int ip_leer(uint32_t* ip, uint32_t* m, uint32_t* p, uint32_t* d) { *ip = 0x3201A8C0u; *m = *p = *d = 0; return 0; }
static const bpvm_net_backend_t NET = { NULL, NULL, NULL, NULL, ip_dinamica, ip_fija, ip_leer };

/* Espera a que la tarea llegue a un estado: lo normal es enseguida, y el tope es para que una prueba rota no se cuelgue. */
static int llega(int estado) {
    int i;
    for (i = 0; i < 400; i++) {
        if (bpvm_wifi_estado() == estado) return 1;
        bpvm_platform_thread_sleep_ms(5);
    }
    return 0;
}

static const char* s_wifi_json = NULL;
static const char* s_net_json  = NULL;
static int  fs_stat(const char* p, uint32_t* tam) {
    const char* t = !strcmp(p, BPVM_WIFI_FICHERO) ? s_wifi_json : !strcmp(p, BPVM_RED_FICHERO) ? s_net_json : NULL;
    if (!t) return -1;
    *tam = (uint32_t) strlen(t);
    return 0;
}
static long fs_read(const char* p, uint8_t* dst, uint32_t cap) {
    const char* t = !strcmp(p, BPVM_WIFI_FICHERO) ? s_wifi_json : !strcmp(p, BPVM_RED_FICHERO) ? s_net_json : NULL;
    size_t n;
    if (!t) return -1;
    n = strlen(t);
    if (n > cap) n = cap;
    memcpy(dst, t, n);
    return (long) n;
}
static const bpvm_fs_backend_t FS = { .stat = fs_stat, .read = fs_read };

static void prueba_tarea(void) {
    char det[128], t[40];
    int antes;

    /* Sin radio publicada. */
    ok("sin tabla, no hay WiFi", !bpvm_wifi_hay());
    ok("  y se dice", strstr(bpvm_wifi_motivo(), "no tiene WiFi") != NULL);
    ok("encender dice el motivo", bpvm_wifi_encender(det, sizeof det) != 0 && strstr(det, "no tiene WiFi"));
    bpvm_wifi_arranque(1);
    ok("wifi=1 sin radio: sigue apagada", bpvm_wifi_estado() == BPVM_WIFI_OFF);

    /* Publicada, por el registro de los packs. */
    bpvm_fs_set_backend(&FS);
    bpvm_net_set_backend(&NET);
    ok("la tabla se publica", bpvm_bios_publica(BPVM_WIFI_MARCA, &TABLA) == 0);
    ok("hay WiFi", bpvm_wifi_hay());

    /* Sin fichero. */
    ok("sin fichero no enciende", bpvm_wifi_encender(det, sizeof det) != 0 && strstr(det, "no hay /sys/wifi.json"));
    bpvm_wifi_arranque(-1);
    ok("sin fichero y sin pedirlo, apagada", bpvm_wifi_estado() == BPVM_WIFI_OFF && r_encendidas == 0);

    /* El fichero, sin autoStart: sin `wifi=1` no se enciende; con wifi=0, tampoco. */
    s_wifi_json = "{\"ssid\":\"Casa\",\"password\":\"la clave\",\"country\":\"ES\",\"autoOff\":0}\n";
    bpvm_wifi_arranque(-1);
    bpvm_platform_thread_sleep_ms(20);
    ok("sin autoStart ni wifi=1, apagada", r_encendidas == 0);
    bpvm_wifi_arranque(0);
    bpvm_platform_thread_sleep_ms(20);
    ok("wifi=0, apagada", r_encendidas == 0);
    s_wifi_json = "{\"ssid\":\"Casa\",\"password\":\"la clave\",\"country\":\"ES\",\"autoStart\":true,\"autoOff\":0}\n";
    bpvm_wifi_arranque(0);
    bpvm_platform_thread_sleep_ms(20);
    ok("wifi=0 manda sobre autoStart", r_encendidas == 0);

    /* autoStart, sin Entorno. */
    bpvm_wifi_arranque(-1);
    ok("autoStart: llega a connected", llega(BPVM_WIFI_CONNECTED));
    ok("  a la red del fichero", !strcmp(r_ssid, "Casa") && !strcmp(r_clave, "la clave") && !strcmp(r_pais, "ES"));
    ok("  con el nombre de la placa por DHCP", n_dinamicas == 1 && !strncmp(n_nombre, "basicplus-", 10));
    bpvm_wifi_ssid(t, sizeof t);
    ok("  el ssid, a la vista", !strcmp(t, "Casa"));
    ok("  el rssi", bpvm_wifi_rssi() == -52);
    bpvm_red_hay_ip(1);
    ok("  la IP", bpvm_red_ip(t, sizeof t) == 1 && !strcmp(t, "192.168.1.50"));

    /* Se cae: la IP se va con el enlace, sin esperar a nadie. */
    r_responde = BPVM_WIFI_AV_CONECTADA;
    r_aviso(BPVM_WIFI_AV_SIN_RED, 201);
    ok("se cae: noNetwork", llega(BPVM_WIFI_NO_NETWORK));
    bpvm_platform_thread_sleep_ms(20);
    ok("  sin IP", bpvm_red_ip(t, sizeof t) == 0 && t[0] == 0);
    ok("  sin rssi", bpvm_wifi_rssi() == 0);
    antes = r_conexiones;
    ok("  y reintenta solo, al segundo", llega(BPVM_WIFI_CONNECTED) && r_conexiones == antes + 1);

    bpvm_wifi_apagar();
    ok("apagar: off", llega(BPVM_WIFI_OFF));
    bpvm_platform_thread_sleep_ms(20);
    ok("  la radio, apagada", r_apagadas == 1);

    /* La clave mala, de verdad: tres rechazos (1 s y 2 s de espera) y se apaga. */
    r_responde = BPVM_WIFI_AV_CLAVE;
    antes = r_conexiones;
    ok("encender otra vez", bpvm_wifi_encender(det, sizeof det) == 0);
    ok("clave mala: badKey", llega(BPVM_WIFI_BAD_KEY) || llega(BPVM_WIFI_BAD_KEY));
    ok("  a los tres intentos", r_conexiones == antes + 3);
    bpvm_platform_thread_sleep_ms(20);
    ok("  y la radio, apagada", r_apagadas == 2);

    /* Otra clave en el fichero: encender la relee. Y la IP fija. */
    s_wifi_json = "{\"ssid\":\"Otra\",\"password\":\"buena\",\"autoOff\":0}";
    s_net_json  = "{\"ip\":\"192.168.1.50\",\"mask\":\"255.255.255.0\"}\n";
    r_responde  = BPVM_WIFI_AV_CONECTADA;
    ok("encender relee el fichero", bpvm_wifi_encender(det, sizeof det) == 0);
    ok("  connected", llega(BPVM_WIFI_CONNECTED));
    ok("  a la otra red", !strcmp(r_ssid, "Otra") && !strcmp(r_clave, "buena"));
    ok("  con la IP fija", n_fijas == 1 && n_fija == 0x3201A8C0u);
    bpvm_wifi_apagar();
    ok("off", llega(BPVM_WIFI_OFF));

    /* net.json mal escrito: por DHCP, y se dice. */
    s_net_json = "{\"ip\":\"192.168\",\"mask\":\"255.255.255.0\"}";
    antes = n_dinamicas;
    (void) bpvm_wifi_encender(det, sizeof det);
    ok("net.json mal: connected igual", llega(BPVM_WIFI_CONNECTED));
    ok("  por DHCP", n_dinamicas == antes + 1);
    bpvm_wifi_apagar();
    ok("off", llega(BPVM_WIFI_OFF));

    /* No enciende: motivo al log, y limpia. */
    r_falla_encender = 1;
    antes = r_apagadas;
    (void) bpvm_wifi_encender(det, sizeof det);
    bpvm_platform_thread_sleep_ms(50);
    ok("no enciende: off, y la radio apagada", bpvm_wifi_estado() == BPVM_WIFI_OFF && r_apagadas == antes + 1);
    r_falla_encender = 0;

    /* La radio se rompe: off, apagada, sin reiniciar nada. */
    (void) bpvm_wifi_encender(det, sizeof det);
    ok("connected", llega(BPVM_WIFI_CONNECTED));
    antes = r_apagadas;
    r_aviso(BPVM_WIFI_AV_ROTA, 0);
    ok("la radio rota: off", llega(BPVM_WIFI_OFF));
    bpvm_platform_thread_sleep_ms(20);
    ok("  apagada", r_apagadas == antes + 1);
}

int main(void) {
    prueba_encender();
    prueba_reintentos();
    prueba_clave();
    prueba_apagado();
    prueba_ficheros();
    prueba_tarea();
    printf("\n%d buenas, %d fallos  [status=%s]\n", buenas, fallos, fallos ? "FALLO" : "OK");
    return fallos ? 1 : 0;
}
