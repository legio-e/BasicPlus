/*
 * test_boot2.c — V7/H7 (Boot 2, fase 4): el ARRANQUE COMÚN (bpvm_boot2), en el PC.
 *
 * Una familia de mentira que APUNTA lo que se le llama, en orden, y una escalera que
 * se queda donde diga cada caso: sana, sin FS, sin VM, virgen, ganchos NULL. Se mira el
 * ORDEN (la cintura del REPL antes del cable y del autorun; la zona antes de /lib), las
 * PUERTAS de cada capa (con el FS, la zona, /lib y montar la SD; con la app sana, el
 * autorun) y las claves del ENV (`stack=`, `log=`), que la P4 por TCP no aplicaba. Y
 * (V7/H12) los runs por defecto: uno por núcleo de la imagen, con `vm.runs` del ENV encima.
 *
 * La SD de la placa (bpvm_sd_placa.c) es capa de placa y no va en la lib del host: sus
 * dos funciones, de mentira, apuntan también.
 *
 *   make test-boot2
 */
#include "bpvm_boot2.h"
#include "bpvm.h"
#include "bpvm_log.h"
#include "bpvm_machine.h"   /* #626: el ENV de Machine.getEnv, que registra el arranque */
#include "bpvm_sd_placa.h"

#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok  : %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail++; } \
} while (0)

/* ── lo que se llama, en orden ── */
static char g_traza[512];
static void apunta(const char* paso) {
    if (g_traza[0]) strncat(g_traza, " ", sizeof g_traza - strlen(g_traza) - 1);
    strncat(g_traza, paso, sizeof g_traza - strlen(g_traza) - 1);
}

/* ── la SD de la placa, de mentira ── */
static int g_sd_registrada = 0;
int  bpvm_sd_placa_hay(void)             { return g_sd_registrada; }
void bpvm_sd_placa_montar_arranque(void) { apunta("sd_montar"); }

/* ── el log, en RAM; los volcados a la flash se cuentan ── */
static uint8_t g_region[8192];
static int     g_volcados = 0;
static int  log_leer(uint8_t* dst, uint32_t len)        { (void) dst; (void) len; return -1; }
static int  log_escribir(const uint8_t* s, uint32_t n)  { (void) s; (void) n; g_volcados++; return 0; }
static char g_log[8192];
static void log_a_buffer(const char* d, size_t n, void* u) {
    (void) u;
    size_t ya = strlen(g_log);
    if (ya + n >= sizeof g_log) n = sizeof g_log - 1 - ya;
    memcpy(g_log + ya, d, n);
    g_log[ya + n] = '\0';
}
static int en_log(const char* s) {
    g_log[0] = '\0';
    log_dump(log_a_buffer, NULL);
    return strstr(g_log, s) != NULL;
}

/* ── el ENV y la escalera de cada caso ── */
static const char* ENV_TEXTO = "stack=128\nquantum=500\nlog=1\n";
static bpvm_env_t        g_env;
static const bpvm_env_t* g_env_ptr = NULL;
static bpvm_boot_status_t g_st;

static const bpvm_env_t* f_env(void) { apunta("env"); return g_env_ptr; }
/* #626: la escritura del ENV, de mentira: apunta, guarda lo que se le pide y contesta lo que diga el caso */
static int  g_anadir_r = 0;
static char g_anadido[64];
static int  f_env_anadir(const char* clave, const char* valor) {
    apunta("anadir");
    snprintf(g_anadido, sizeof g_anadido, "%s=%s", clave, valor);
    return g_anadir_r;
}
static void f_memoria(void)   { apunta("memoria"); }
static void f_bios(void)      { apunta("bios"); }
static const bpvm_boot_status_t* f_subir(void) { apunta("subir"); return &g_st; }
static int  f_mapear(void)    { apunta("mapear"); return 0; }   /* sin zona: la familia lo diría */
static void f_stdlib(void)    { apunta("stdlib"); }
static void f_con_fs(void)    { apunta("con_fs"); }
static void f_hw(void)        { apunta("hw"); }
static void f_sd(void)        { apunta("sd"); g_sd_registrada = 1; }
static void f_identidad(void) { apunta("identidad"); }
static void f_cintura(void)   { apunta("cintura"); }
static void f_cable(void)     { apunta("cable"); }
static void f_autorun(void)   { apunta("autorun"); }
static void f_repl(void)      { apunta("repl"); }

/* V8 #784 — una HAL de la frecuencia de mentira, la de una S3: 80, 160 o 240 MHz; empieza en 160 */
static int g_hal_mhz = 160;
static int hal_cpu_hz(void)    { return g_hal_mhz * 1000000; }
static int hal_cpu_mhz(int m)  { if (m != 80 && m != 160 && m != 240) return 0; g_hal_mhz = m; return 1; }

static const bpvm_boot2_familia_t FAMILIA = {
    f_env, f_env_anadir, f_memoria, f_bios, f_subir, f_mapear, f_stdlib, f_con_fs, f_hw, f_sd,
    f_identidad, f_cintura, f_cable, f_autorun, f_repl
};

/* La traza sin los `env` (se pide cuando hace falta: no es un paso del orden). */
static const char* pasos(void) {
    static char sin[512];
    const char* p = g_traza;
    sin[0] = '\0';
    while (*p) {
        const char* fin = strchr(p, ' ');
        size_t n = fin ? (size_t) (fin - p) : strlen(p);
        if (!(n == 3 && strncmp(p, "env", 3) == 0)) {
            if (sin[0]) strcat(sin, " ");
            strncat(sin, p, n);
        }
        p += n;
        while (*p == ' ') p++;
    }
    return sin;
}

static void caso(bpvm_boot_state_t estado, int degradada, const char* motivo, int con_env) {
    g_traza[0] = '\0';
    g_sd_registrada = 0;
    g_volcados = 0;
    g_anadir_r = 0;
    g_anadido[0] = '\0';
    log_clear_ram();
    bpvm_log_set_enabled(1);
    bpvm_set_stack_kb(0);
    g_st.state = estado;
    g_st.degraded = degradada;
    snprintf(g_st.reason, sizeof g_st.reason, "%s", motivo);
    g_env_ptr = con_env ? &g_env : NULL;
}

int main(void) {
    printf("=== test_boot2 (V7/H7 fase 4: el arranque comun) ===\n");
    bpvm_log_cintura_t c = { NULL, log_leer, log_escribir, g_region, sizeof g_region };
    bpvm_log_init(&c);
    g_env.payload = (const uint8_t*) ENV_TEXTO;
    g_env.payload_len = (uint16_t) strlen(ENV_TEXTO);
    g_env.seq = 1;
    g_env.valid = 1;
    const size_t MB = 1024u * 1024u;

    printf("--- 1. la placa sana: todo, en su orden ---\n");
    caso(BPVM_BOOT_APP, 0, "", 1);
    bpvm_boot2_repartir(&FAMILIA);
    CHECK(strcmp(pasos(), "memoria bios") == 0, "2a: la memoria y la BIOS");
    CHECK(bpvm_stack_region_bytes(MB) == 128u * 1024u, "2a: stack=128 del ENV, aplicado ANTES de la memoria");
    g_traza[0] = '\0';
    bpvm_boot2_subir(&FAMILIA);
    printf("       %s\n", pasos());
    CHECK(strcmp(pasos(), "subir anadir mapear stdlib con_fs hw sd sd_montar identidad cintura cable autorun repl") == 0,
          "2b: subir, la fecha (#626), la zona, /lib, lo del FS, hw, la SD, la identidad, la CINTURA, el cable, el autorun, el REPL");
    CHECK(en_log("boot: estado 3 (app)") && !en_log("DEGRADADO"), "la linea del arranque, una para todas");
    CHECK(!en_log("autorun: no se mira"), "sana: el autorun no se excusa");
    CHECK(g_volcados == 1, "el log del arranque, a la flash UNA vez");
    CHECK(bpvm_log_enabled() == 1, "log=1 del ENV, aplicado al final (la P4 por TCP no lo hacia)");

    printf("--- 2. sin FS: ni zona, ni /lib, ni montar la SD, ni autorun ---\n");
    caso(BPVM_BOOT_PARTITIONS, 1, "littlefs no monta ni formatea", 1);
    bpvm_boot2_subir(&FAMILIA);
    printf("       %s\n", pasos());
    CHECK(strcmp(pasos(), "subir anadir hw sd identidad cintura cable repl") == 0,
          "sin FS: lo del kernel sigue (la fecha, del ENV, tambien); la zona, /lib, montar la SD y el autorun, no");
    CHECK(en_log("boot: estado 1 (particiones) DEGRADADO: littlefs no monta"), "el estado y el motivo");
    CHECK(en_log("packs: la zona no se monta sin el FS"), "la zona dice por que no");
    CHECK(en_log("sd: el FS de la placa no esta montado"), "la SD registrada dice por que no se monta");
    CHECK(en_log("autorun: no se mira") && en_log("(estado 1, degradada)"), "y el autorun, por que no se mira");

    printf("--- 3. sin VM (el FS si): la zona y /lib si, el autorun no (la STM32 lo corria) ---\n");
    caso(BPVM_BOOT_FS, 1, "heap de la VM no disponible", 1);
    bpvm_boot2_subir(&FAMILIA);
    printf("       %s\n", pasos());
    CHECK(strcmp(pasos(), "subir anadir mapear stdlib con_fs hw sd sd_montar identidad cintura cable repl") == 0,
          "sin VM: todo menos el autorun");
    CHECK(en_log("(estado 2, degradada)"), "el autorun dice por que no");

    printf("--- 4. virgen (sin ENV): el suelo, y las claves por defecto ---\n");
    caso(BPVM_BOOT_KERNEL, 1, "sin particiones en el env", 0);
    bpvm_boot2_repartir(&FAMILIA);
    bpvm_boot2_subir(&FAMILIA);
    CHECK(strcmp(pasos(), "memoria bios subir anadir hw sd identidad cintura cable repl") == 0,
          "virgen: el kernel y el REPL (y la fecha: el ENV se crea con ella)");
    CHECK(bpvm_stack_region_bytes(MB) == 16u * 1024u, "sin ENV, la pila del main y nada mas (#740)");
    CHECK(bpvm_log_enabled() == 0, "sin ENV, el log de ejecucion apagado (#423)");

    printf("--- 5. ganchos NULL: lo que la familia no tiene, o hizo en su Boot 1 ---\n");
    {
        bpvm_boot2_familia_t poca;
        memset(&poca, 0, sizeof poca);
        poca.env = f_env; poca.subir = f_subir; poca.repl = f_repl;
        caso(BPVM_BOOT_APP, 0, "", 1);
        bpvm_boot2_repartir(&poca);
        bpvm_boot2_subir(&poca);
        printf("       %s\n", pasos());
        CHECK(strcmp(pasos(), "subir sd_montar repl") == 0,
              "sin ganchos: subir, montar la SD (es del comun) y el REPL, sin caerse");
        CHECK(en_log("stdlib: esta imagen no trae la stdlib embebida"), "/lib que falta, dicho");
        CHECK(en_log("env: esta familia no escribe su ENV en el arranque: imagen.fecha no se anota"),
              "#626: sin el gancho, la fecha no se anota, y se dice");
        CHECK(bpvm_stack_region_bytes(MB) == 128u * 1024u, "las claves, igual");
    }
    {
        bpvm_boot2_familia_t nada;
        memset(&nada, 0, sizeof nada);
        caso(BPVM_BOOT_APP, 0, "", 1);
        bpvm_boot2_subir(&nada);
        CHECK(en_log("boot: estado 0 (kernel) DEGRADADO: la familia no da sus capas"),
              "sin capas: el suelo, y lo dice");
    }

    printf("--- 6. bpvm_boot2_claves sola (la usa el micro simulado) ---\n");
    bpvm_set_stack_kb(0);
    bpvm_boot2_claves(&g_env);
    CHECK(bpvm_stack_region_bytes(MB) == 128u * 1024u, "stack= del ENV");
    bpvm_set_stack_kb(0);
    bpvm_boot2_claves(NULL);
    CHECK(bpvm_stack_region_bytes(MB) == 16u * 1024u, "sin ENV no toca nada (la del main, #740)");

    printf("--- 7. #626: la fecha de la imagen, UNA vez; y el ENV de Machine.getEnv ---\n");
    {
        CHECK(bpvm_boot2_fecha_de("Oct  2 2026") == 1790899200L, "\"Oct  2 2026\" = 1790899200 (el 2-oct a las 00:00)");
        CHECK(bpvm_boot2_fecha_de("Jan  1 1970") == 0L && bpvm_boot2_fecha_de("Mar  1 2000") == 951868800L
              && bpvm_boot2_fecha_de("Feb 29 2028") == 1835395200L && bpvm_boot2_fecha_de("Dec 31 2037") == 2145830400L,
              "1970, el 1-mar-2000, el 29-feb-2028 y el ultimo dia que cabe en 32 bits");
        CHECK(bpvm_boot2_fecha_de("Oct 2 2026") == -1 && bpvm_boot2_fecha_de("Xyz  2 2026") == -1
              && bpvm_boot2_fecha_de("Oct  2 1969") == -1 && bpvm_boot2_fecha_de("Oct  2 2038") == -1
              && bpvm_boot2_fecha_de("Oct 32 2026") == -1 && bpvm_boot2_fecha_de(NULL) == -1,
              "lo que no es una fecha de __DATE__, o no cabe: -1");

        char esperado[64];
        snprintf(esperado, sizeof esperado, "imagen.fecha=%ld", bpvm_boot2_fecha_de(__DATE__));
        caso(BPVM_BOOT_APP, 0, "", 1);
        bpvm_boot2_subir(&FAMILIA);
        CHECK(strcmp(g_anadido, esperado) == 0, "sin la clave: se pide anotar la fecha de la imagen (la de __DATE__)");
        CHECK(en_log("env: imagen.fecha=") && en_log("anotada, una vez"), "y se dice");

        static const char* CON_FECHA = "stack=128\nimagen.fecha=1700000000\n";
        bpvm_env_t ef = g_env;
        ef.payload = (const uint8_t*) CON_FECHA;
        ef.payload_len = (uint16_t) strlen(CON_FECHA);
        caso(BPVM_BOOT_APP, 0, "", 1);
        g_env_ptr = &ef;
        bpvm_boot2_subir(&FAMILIA);
        CHECK(strstr(g_traza, "anadir") == NULL && g_anadido[0] == '\0' && !en_log("env: imagen.fecha"),
              "con la clave: ni se pide ni se dice (la flash no se toca en cada arranque)");

        caso(BPVM_BOOT_APP, 0, "", 1);
        g_anadir_r = -1;
        bpvm_boot2_subir(&FAMILIA);
        CHECK(en_log("env: no se pudo anotar imagen.fecha"), "si la familia no puede escribir, se dice");

        char val[32];
        caso(BPVM_BOOT_APP, 0, "", 1);
        bpvm_boot2_repartir(&FAMILIA);
        CHECK(bpvm_machine_env_get("STACK", val, sizeof val) == 3 && strcmp(val, "128") == 0,
              "Machine.getEnv lee el ENV de la familia (la clave sin distinguir mayusculas)");
        CHECK(bpvm_machine_env_get("nada", val, sizeof val) == -1 && val[0] == '\0', "una clave que no esta: -1 y \"\"");
        CHECK(bpvm_machine_env_get("stack", val, 3) == -2, "un valor que no cabe: -2 (entero o nada)");
        g_env_ptr = NULL;
        CHECK(bpvm_machine_env_get("stack", val, sizeof val) == -1, "sin ENV: -1");
        bpvm_machine_set_env(NULL);
        g_env_ptr = &g_env;
        CHECK(bpvm_machine_env_get("stack", val, sizeof val) == -1, "sin registrar (la VM de host): -1");
    }

    printf("--- 8. V7/H12: un run por nucleo de la imagen; vm.runs y vm.cores del ENV mandan ---\n");
    {
        /* Eduardo (6-oct): «En la Pico y Metro, 2 nucleos y 2 runs. En la P4 y la S3 igual. En el resto 1 nucleo y 1 run». */
        bpvm_boot2_familia_t dos = FAMILIA;
        dos.nucleos = 2;
        bpvm_boot2_familia_t uno = FAMILIA;
        uno.nucleos = 1;

        caso(BPVM_BOOT_APP, 0, "", 1);   /* el ENV de siempre: sin vm.runs */
        bpvm_set_runs(1);
        bpvm_boot2_repartir(&dos);
        CHECK(bpvm_runs() == 2, "dos nucleos, el ENV sin vm.runs: dos runs");

        caso(BPVM_BOOT_KERNEL, 1, "sin particiones en el env", 0);   /* virgen: sin ENV */
        bpvm_set_runs(1);
        bpvm_boot2_repartir(&dos);
        CHECK(bpvm_runs() == 2, "dos nucleos y sin ENV (una placa virgen): dos runs");

        static const char* VUELTA = "vm.runs=1\nvm.cores=1\n";
        bpvm_env_t ev = g_env;
        ev.payload = (const uint8_t*) VUELTA;
        ev.payload_len = (uint16_t) strlen(VUELTA);
        caso(BPVM_BOOT_APP, 0, "", 1);
        g_env_ptr = &ev;
        bpvm_boot2_repartir(&dos);
        CHECK(bpvm_runs() == 1 && bpvm_cores() == 1, "vm.runs=1 y vm.cores=1 del ENV mandan: la vuelta atras sin grabar");

        caso(BPVM_BOOT_APP, 0, "", 1);
        bpvm_set_runs(2);
        bpvm_boot2_repartir(&uno);
        CHECK(bpvm_runs() == 1, "un nucleo: un run");

        static const char* DOS_RUNS = "vm.runs=2\n";
        bpvm_env_t e2 = g_env;
        e2.payload = (const uint8_t*) DOS_RUNS;
        e2.payload_len = (uint16_t) strlen(DOS_RUNS);
        caso(BPVM_BOOT_APP, 0, "", 1);
        g_env_ptr = &e2;
        bpvm_boot2_repartir(&uno);
        CHECK(bpvm_runs() == 2, "un nucleo con vm.runs=2 en el ENV: dos runs (la etapa 2b: los dos en uno)");

        caso(BPVM_BOOT_APP, 0, "", 1);
        bpvm_set_runs(1);
        bpvm_boot2_repartir(&FAMILIA);   /* la de mentira no dice sus nucleos */
        CHECK(bpvm_runs() == 1, "una familia que no dice sus nucleos: uno, lo de siempre");
        bpvm_set_cores(2);
    }

    printf("--- 9. V8 #784: cpu.mhz del ENV, en la familia que lo dice ---\n");
    {
        static const char* CPU240 = "cpu.mhz=240\n";
        static const char* CPUABC = "cpu.mhz=abc\n";
        static const char* CPU200 = "cpu.mhz=200\n";
        static const bpvm_machine_backend_t SOLO_80_160_240 = {   /* una HAL de mentira: la de una S3 */
            .cpuFreqHz = hal_cpu_hz, .setCpuFreqMHz = hal_cpu_mhz };
        bpvm_env_t ec = g_env;
        bpvm_boot2_familia_t con = FAMILIA;
        con.cpu_desde_env = 1;

        ec.payload = (const uint8_t*) CPU240;
        ec.payload_len = (uint16_t) strlen(CPU240);
        bpvm_machine_set_cpu_freq_mhz(150);   /* la del host simulado, de partida */
        caso(BPVM_BOOT_APP, 0, "", 1);
        g_env_ptr = &ec;
        bpvm_boot2_subir(&FAMILIA);
        CHECK(bpvm_machine_cpu_freq_hz() == 150000000 && !en_log("cpu:"),
              "una familia que no lo dice (la RP2350, de momento): ni la mira");

        caso(BPVM_BOOT_APP, 0, "", 1);
        g_env_ptr = &ec;
        bpvm_boot2_subir(&con);
        CHECK(bpvm_machine_cpu_freq_hz() == 240000000 && en_log("cpu: 240 MHz (cpu.mhz del ENV)"),
              "cpu.mhz=240: a 240, y se dice");

        ec.payload = (const uint8_t*) CPUABC;
        ec.payload_len = (uint16_t) strlen(CPUABC);
        bpvm_machine_set_cpu_freq_mhz(150);
        caso(BPVM_BOOT_APP, 0, "", 1);
        g_env_ptr = &ec;
        bpvm_boot2_subir(&con);
        CHECK(bpvm_machine_cpu_freq_hz() == 150000000
              && en_log("cpu: cpu.mhz=abc no es un número de MHz: se queda en 150 MHz"),
              "cpu.mhz=abc: no es un numero; se queda la de la imagen, y se dice");

        bpvm_machine_set_backend(&SOLO_80_160_240);
        ec.payload = (const uint8_t*) CPU200;
        ec.payload_len = (uint16_t) strlen(CPU200);
        caso(BPVM_BOOT_APP, 0, "", 1);
        g_env_ptr = &ec;
        bpvm_boot2_subir(&con);
        CHECK(bpvm_machine_cpu_freq_hz() == 160000000
              && en_log("cpu: cpu.mhz=200 no lo da esta placa: se queda en 160 MHz"),
              "cpu.mhz=200 en una que no lo da: se queda la de la imagen, y se dice");

        ec.payload = (const uint8_t*) CPU240;
        ec.payload_len = (uint16_t) strlen(CPU240);
        caso(BPVM_BOOT_APP, 0, "", 1);
        g_env_ptr = &ec;
        bpvm_boot2_subir(&con);
        CHECK(bpvm_machine_cpu_freq_hz() == 240000000, "y cpu.mhz=240 en la misma: a 240, con su HAL");
        bpvm_machine_set_backend(NULL);

        caso(BPVM_BOOT_APP, 0, "", 1);   /* el ENV de siempre, sin la clave */
        bpvm_machine_set_cpu_freq_mhz(150);
        bpvm_boot2_subir(&con);
        CHECK(bpvm_machine_cpu_freq_hz() == 150000000 && !en_log("cpu:"), "sin la clave: la de la imagen, sin decir nada");
    }

    printf(g_fail == 0 ? "[status=OK]\n" : "[status=FAIL: %d]\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
