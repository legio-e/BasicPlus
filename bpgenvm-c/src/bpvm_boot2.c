/*
 * bpvm_boot2.c — V7/H7 (Boot 2, fase 4; 1-oct): el ARRANQUE COMÚN. Ver bpvm_boot2.h.
 *
 * Lo que estaba escrito cinco veces —la Pico, la ESP32, la P4 por UART y por TCP, la
 * STM32— con el mismo esqueleto y órdenes distintos, y con tres fallos de paso: la P4 por
 * TCP no aplicaba `stack=`, `quantum=` ni `log=`; el autorun de la STM32 no miraba si la
 * placa había llegado sana; y en la Pico y en las ESP32 la cintura del REPL se registraba
 * al entrar al REPL, DESPUÉS del autorun —un IDE que se conectaba durante uno recibía
 * «no ha registrado su cintura» al saludar (HELLO)—. Aquí el orden va una vez, y la capa
 * que pide cada paso, en un sitio:
 *
 *   capa      paso
 *   ─────     ──────────────────────────────────────────────────────────────────────
 *   kernel    las claves del reparto · la memoria · la BIOS                   (2a)
 *   0 → 3     subir: particiones → FS → app, y la línea «boot: estado»
 *   kernel    la fecha de la imagen al ENV, si falta (una vez: #626)
 *   FS        la zona de packs, al lado · /lib · lo de la familia con el FS
 *   kernel    el hardware · los buses del ENV · registrar la SD
 *   FS        montar la SD
 *   kernel    la identidad · la cintura del REPL · el cable · volcar el log · `log=`
 *   app sana  el autorun
 *   —         el REPL
 */
#include "bpvm_boot2.h"
#include "bpvm.h"            /* bpvm_set_stack_kb, bpvm_set_quantum_ops */
#include "bpvm_bus.h"        /* los buses que la placa declara en su ENV */
#include "bpvm_log.h"
#include "bpvm_machine.h"    /* #626: el lector del ENV de Machine.getEnv */
#include "bpvm_pack.h"       /* el montaje de la zona y su estado (fase 1) */
#include "bpvm_sd_placa.h"   /* la SD de la placa: el registro común */

#include <stdio.h>
#include <stdlib.h>          /* strtol: cpu.mhz (#784) */
#include <string.h>

/* La familia de este arranque: los buses leen el ENV por una función sin contexto. */
static const bpvm_boot2_familia_t* s_f = 0;

/* El ENV, si vale. Una placa virgen no lo tiene, y eso no es un fallo del arranque. */
static const bpvm_env_t* env_de(const bpvm_boot2_familia_t* f) {
    const bpvm_env_t* e = (f && f->env) ? f->env() : 0;
    return (e && e->valid) ? e : 0;
}

/* La regla de siempre para una clave de texto: entera o nada (V7/H6.c G4.a1). */
static int env_exacto(const char* clave, char* buf, size_t cap) {
    const bpvm_env_t* e = env_de(s_f);
    return e ? bpvm_env_get_exacto(e, clave, buf, cap) : -1;
}

static void linea_al_log(const char* l) { log_printf("%s", l); }

/* V8 #784 — `cpu.mhz=N`: la frecuencia de la CPU, al arrancar, con el setter de Machine.setCpuFreqMHz (la HAL de la
 * familia sabe qué valores da su chip). Un valor que no es un número de MHz, o que la placa no da, se dice y se queda
 * la de la imagen: no se arranca a una frecuencia que nadie pidió, y tampoco en silencio. */
static void frecuencia_del_env(void) {
    char v[16], *fin;
    long mhz;
    int r = env_exacto("cpu.mhz", v, sizeof v);
    if (r == -1) return;                                   /* sin la clave: la de la imagen */
    if (r < 0) {
        log_printf("cpu: cpu.mhz no cabe en %u bytes: se queda en %d MHz", (unsigned) sizeof v,
                   bpvm_machine_cpu_freq_hz() / 1000000);
        return;
    }
    mhz = strtol(v, &fin, 10);
    if (fin == v || *fin != '\0' || mhz <= 0 || mhz > 100000) {
        log_printf("cpu: cpu.mhz=%s no es un número de MHz: se queda en %d MHz", v, bpvm_machine_cpu_freq_hz() / 1000000);
        return;
    }
    if (bpvm_machine_set_cpu_freq_mhz((int) mhz))
        log_printf("cpu: %ld MHz (cpu.mhz del ENV)", mhz);
    else
        log_printf("cpu: cpu.mhz=%ld no lo da esta placa: se queda en %d MHz", mhz, bpvm_machine_cpu_freq_hz() / 1000000);
}

/* La familia de este arranque, y con ella el ENV de Machine.getEnv (#626): el programa lee el mismo ENV que el
 * arranque, por la misma regla (entero o nada). */
static void fija(const bpvm_boot2_familia_t* f) {
    s_f = f;
    bpvm_machine_set_env(env_exacto);
}

long bpvm_boot2_fecha_de(const char* d) {
    static const char MESES[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    if (!d || strlen(d) != 11 || d[3] != ' ' || d[6] != ' ') return -1;
    int m = 0;
    for (int i = 0; i < 12 && !m; i++)
        if (memcmp(d, MESES + 3 * i, 3) == 0) m = i + 1;
    if (!m || (d[4] != ' ' && (d[4] < '0' || d[4] > '9')) || d[5] < '0' || d[5] > '9') return -1;
    for (int i = 7; i < 11; i++)
        if (d[i] < '0' || d[i] > '9') return -1;
    int dia  = (d[4] == ' ' ? 0 : (d[4] - '0') * 10) + (d[5] - '0');
    int anyo = (d[7] - '0') * 1000 + (d[8] - '0') * 100 + (d[9] - '0') * 10 + (d[10] - '0');
    if (dia < 1 || dia > 31 || anyo < 1970 || anyo > 2037) return -1;
    /* Los días desde 1970-01-01: el «days from civil» de H. Hinnant (el año empieza en marzo). */
    long y   = anyo - (m <= 2);
    long era = y / 400;
    long yoe = y - era * 400;
    long doy = (153L * (m > 2 ? m - 3 : m + 9) + 2) / 5 + dia - 1;
    long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097L + doe - 719468L) * 86400L;
}

/* V7/H10 (#626) — LA FECHA DE LA IMAGEN, en el ENV: la que mira Rtc.Clock.isSet(). La idea es de Eduardo (2-oct):
 * «una variable de entorno que guarde la fecha en la que se grabó la imagen, y si la fecha actual es menor es que
 * está fuera de hora». Se anota UNA vez, si falta, y se queda hasta que se borre: «si lo grabamos cada vez que
 * arranque podríamos llegar a estropear la flash». En un arranque normal la clave ya está y la flash ni se mira.
 *
 * Es la fecha en que se compiló ESTE fichero: en un build incremental, la de la última vez —anterior o igual a la
 * de la imagen—, y como cota vale igual: ninguna hora correcta es anterior. */
static void anota_fecha_de_la_imagen(const bpvm_boot2_familia_t* f) {
    char v[24];
    if (env_exacto(BPVM_BOOT2_CLAVE_FECHA, v, sizeof v) != -1) return;   /* ya está (o no cabe en v: está) */
    long fecha = bpvm_boot2_fecha_de(__DATE__);
    if (fecha < 0) {
        log_printf("env: la fecha de esta imagen (\"%s\") no se entiende: %s no se anota", __DATE__, BPVM_BOOT2_CLAVE_FECHA);
        return;
    }
    if (!f->env_anadir) {
        log_printf("env: esta familia no escribe su ENV en el arranque: %s no se anota", BPVM_BOOT2_CLAVE_FECHA);
        return;
    }
    snprintf(v, sizeof v, "%ld", fecha);
    int r = f->env_anadir(BPVM_BOOT2_CLAVE_FECHA, v);
    if (r == 0)
        log_printf("env: %s=%s (%s, la fecha de esta imagen): anotada, una vez", BPVM_BOOT2_CLAVE_FECHA, v, __DATE__);
    else if (r < 0)
        log_printf("env: no se pudo anotar %s (no cabe en el ENV o la flash no escribe): Rtc.Clock.isSet() no tiene fecha",
                   BPVM_BOOT2_CLAVE_FECHA);
}

void bpvm_boot2_claves(const bpvm_env_t* env) {
    if (!env || !env->valid) return;   /* sin ENV, lo de siempre: la pila del main (V8 #740), 1024 opcodes */
    /* `stack=N` (KB): el usuario reparte el bloque de la VM entre pilas y heap; la regla vive
     * en bpvm_stack_region_bytes() (V8 #740: la región es la pila del main; las de los hilos, del
     * heap). Antes de la memoria: la línea de la Pico ya lo cuenta. */
    bpvm_set_stack_kb((unsigned long) bpvm_env_get_long(env, "stack", 0));
    bpvm_set_quantum_ops(bpvm_env_get_long(env, "quantum", 0));   /* #462 */
    bpvm_set_runs(bpvm_env_get_long(env, "vm.runs", 0));          /* V7/H12, 2b: los runs permanentes; antes de la cintura
                                                                     (sin la clave, uno por núcleo: bpvm_boot2_repartir) */
    bpvm_set_cores(bpvm_env_get_long(env, "vm.cores", 0));        /* V7/H12: la vuelta atrás de los dos núcleos (ESP32) */
}

void bpvm_boot2_repartir(const bpvm_boot2_familia_t* f) {
    if (!f) return;
    fija(f);
    /* V7/H12 — un run por núcleo de la imagen (Eduardo, 6-oct), ANTES de las claves: `vm.runs` del ENV manda sobre
     * él. Y antes de la memoria: la Pico reserva la pila del 2º run en su reparto. */
    bpvm_set_runs(f->nucleos);
    bpvm_boot2_claves(env_de(f));
    if (f->memoria) f->memoria();
    /* La BIOS de los motores, verificada aunque no haya ninguno grabado: un hueco que sólo
     * se descubre el día que alguien graba un pack cuesta el doble. */
    if (f->bios) f->bios();
}

/* Si la familia no da sus capas, se queda en el suelo y lo dice. */
static const bpvm_boot_status_t s_sin_capas = { BPVM_BOOT_KERNEL, 1, "la familia no da sus capas" };

void bpvm_boot2_subir(const bpvm_boot2_familia_t* f) {
    if (!f) return;
    fija(f);

    /* 0 → 3: sube mientras cada capa arranca y para en la primera que falla (H9). */
    const bpvm_boot_status_t* st = f->subir ? f->subir() : 0;
    if (!st) st = &s_sin_capas;
    log_printf("boot: estado %d (%s)%s%s", (int) st->state, bpvm_boot_state_name(st->state),
               st->degraded ? " DEGRADADO: " : "", st->degraded ? st->reason : "");
    const int con_fs = (st->state >= BPVM_BOOT_FS);
    const int sana   = (st->state == BPVM_BOOT_APP && !st->degraded);

    /* La fecha de la imagen, si falta (#626). Es del kernel —el ENV no pide el FS— y va antes del autorun: el
     * programa que arranca solo ya la ve. */
    anota_fecha_de_la_imagen(f);

    /* La zona de packs, AL LADO del FS: se monta con él —sin el FS no, y se dice— y se dice
     * su estado (fase 1). ANTES que /lib (#468): el instalador le pregunta qué sirve ya con
     * los mismos bytes que la imagen, para no copiar a /lib lo que la taparía. */
    (void) bpvm_pack_montar_arranque(con_fs, f->mapear_packs, linea_al_log);
    if (con_fs) {
        if (f->stdlib) f->stdlib();
        else           log_printf("stdlib: esta imagen no trae la stdlib embebida");
        if (f->con_fs) f->con_fs();
    }

    /* El hardware y, después, los buses del ENV: la tabla de capacidades que deja el
     * hardware dice qué buses existen. Se llama siempre —sin ENV también—: arranca el
     * árbitro con un solo hilo vivo. */
    if (f->hw) f->hw();
    if (f->cpu_desde_env) frecuencia_del_env();   /* V8 #784: antes de los buses, que la pueden usar */
    (void) bpvm_bus_desde_env(env_exacto);

    /* La SD: se REGISTRA siempre (SD_INFO contesta aunque no haya FS) y se MONTA con el FS,
     * que es donde se cuelga /sd. Después de los buses: la línea corta (`sd=bus:2,cs:23`)
     * se engancha a uno. Un fallo no degrada el arranque. */
    if (f->sd) f->sd();
    if (con_fs)                    bpvm_sd_placa_montar_arranque();
    else if (bpvm_sd_placa_hay())  log_printf("sd: el FS de la placa no esta montado - /sd no se monta (SD_INFO si contesta)");

    /* La identidad y la CINTURA del REPL, antes del cable y antes del autorun: un IDE que se
     * conecta con el autorun corriendo saluda (HELLO), y el saludo lo contesta el REPL común
     * con la cintura de la familia (durante el autorun, sólo HELLO, KILL y RESET). */
    if (f->identidad) f->identidad();
    if (f->cintura)   f->cintura();
    if (f->cable)     f->cable();

    /* El autorun, sólo con la placa SANA en la app (H9): una placa degradada se queda
     * esperando al IDE, que es quien la arregla. Y si no se mira, se dice. */
    if (!sana)
        log_printf("autorun: no se mira — la placa no llegó sana (estado %d%s)",
                   (int) st->state, st->degraded ? ", degradada" : "");

#ifdef BPSTR_PRUEBA_PLACA
    /* V8 #730 (el paso 2): en una IMAGEN DE PRUEBA de la biblioteca de cadenas, sus pruebas y su banco (bpstr/placa.c),
     * con el FS montado y antes del autorun; lo que dicen, en /app/bpstr.txt. En las imágenes de siempre no existe. */
    if (con_fs) { void bpstr_en_placa(void); bpstr_en_placa(); }
#endif

    /* La historia del arranque, a la flash ANTES del autorun: si la app se cuelga o se va
     * la corriente, el arranque está. Y #423: a partir de aquí el log lo manda el ENV
     * (`log=0|1`, apagado por defecto); lo de arriba se registra siempre. */
    log_flush();
    const bpvm_env_t* env = env_de(f);
    bpvm_log_set_enabled(env ? bpvm_env_get_bool(env, "log", 0) : 0);

    if (sana && f->autorun) f->autorun();
    if (f->repl) f->repl();
}
