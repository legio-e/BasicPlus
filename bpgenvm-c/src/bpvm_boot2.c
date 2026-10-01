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
#include "bpvm_pack.h"       /* el montaje de la zona y su estado (fase 1) */
#include "bpvm_sd_placa.h"   /* la SD de la placa: el registro común */

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

void bpvm_boot2_claves(const bpvm_env_t* env) {
    if (!env || !env->valid) return;   /* sin ENV, lo de siempre: el 25 % a pilas, 1024 opcodes */
    /* `stack=N` (KB): el usuario reparte el bloque de la VM entre pilas y heap; la regla vive
     * en bpvm_stack_region_bytes(). Antes de la memoria: la línea de la Pico ya lo cuenta. */
    bpvm_set_stack_kb((unsigned long) bpvm_env_get_long(env, "stack", 0));
    bpvm_set_quantum_ops(bpvm_env_get_long(env, "quantum", 0));   /* #462 */
}

void bpvm_boot2_repartir(const bpvm_boot2_familia_t* f) {
    if (!f) return;
    s_f = f;
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
    s_f = f;

    /* 0 → 3: sube mientras cada capa arranca y para en la primera que falla (H9). */
    const bpvm_boot_status_t* st = f->subir ? f->subir() : 0;
    if (!st) st = &s_sin_capas;
    log_printf("boot: estado %d (%s)%s%s", (int) st->state, bpvm_boot_state_name(st->state),
               st->degraded ? " DEGRADADO: " : "", st->degraded ? st->reason : "");
    const int con_fs = (st->state >= BPVM_BOOT_FS);
    const int sana   = (st->state == BPVM_BOOT_APP && !st->degraded);

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

    /* La historia del arranque, a la flash ANTES del autorun: si la app se cuelga o se va
     * la corriente, el arranque está. Y #423: a partir de aquí el log lo manda el ENV
     * (`log=0|1`, apagado por defecto); lo de arriba se registra siempre. */
    log_flush();
    const bpvm_env_t* env = env_de(f);
    bpvm_log_set_enabled(env ? bpvm_env_get_bool(env, "log", 0) : 0);

    if (sana && f->autorun) f->autorun();
    if (f->repl) f->repl();
}
