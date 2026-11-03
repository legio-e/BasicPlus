/*
 * bpvm_boot2.h — V7/H7 (Boot 2, fase 4; 1-oct): EL ARRANQUE COMÚN, escrito una vez.
 *
 * Boot 1 es de la familia: lo que no existe hasta que arranca el silicio —el reloj, el
 * RTOS, el log y sus primeras líneas, el cable donde ya esté (el USB de la Pico) y el ENV
 * a la vista—. Boot 2 es esto: el ORDEN, la CAPA que pide cada paso y las reglas (con el
 * FS, la zona de packs y /lib; con la app sana, el autorun), con un gancho para lo que sí
 * es de cada familia. Antes eran cinco copias del mismo esqueleto con órdenes distintos,
 * y cada una tenía su fallo de paso (V7_IDEAS §H7, «La fase 4»).
 *
 * En DOS TIEMPOS, porque la memoria se reparte antes de que nadie más la pida —la
 * dirección del bloque de los motores va sellada en ellos—: la P4 llama a repartir en
 * app_main, antes de la red, y a subir en la tarea del cable; las demás, seguidos.
 *
 * Un gancho NULL es que la familia no lo tiene, o que lo hizo en su Boot 1. Las capas,
 * en docs/H9_KERNEL_CAPAS.md («Boot 1 y Boot 2»).
 *
 * Capa de PLACA, como bpvm_repl.c: no va en la lib de la VM de host; la enlazan los
 * cuatro firmwares, el micro simulado (sólo bpvm_boot2_claves) y su prueba.
 */
#ifndef BPVM_BOOT2_H
#define BPVM_BOOT2_H

#include "bpvm_boot.h"
#include "bpvm_env.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* El ENV de la placa, ya leído en el Boot 1 (NULL o inválido: una placa sin ENV). */
    const bpvm_env_t* (*env)(void);
    /* V7/H10 (#626) — AÑADIR al ENV una clave que falta (bpvm_env_anadir), por la cintura de flash de la familia, y
     * dejar la vista de `env` al día: lo que se anota en el arranque, el programa lo ve sin reiniciar. 0 escrita;
     * 1 ya estaba; -1 no se pudo. NULL: esta familia no escribe su ENV (y el arranque lo dice). */
    int (*env_anadir)(const char* clave, const char* valor);

    /* ── 2a, repartir ── */
    void (*memoria)(void);   /* el bloque de los motores y el de la VM: la regla es común
                                (bpvm_mem_plan, bpvm_packram_region); aquí, DÓNDE */
    void (*bios)(void);      /* la tabla de la BIOS (y el ancla), verificada en cada arranque */

    /* ── 2b, subir ── */
    const bpvm_boot_status_t* (*subir)(void);   /* particiones → FS → app: las capas de la
                                                   familia (bpvm_boot_climb) y su estado */
    int  (*mapear_packs)(void);   /* dónde ve la CPU la zona (bpvm_pack_montar_arranque) */
    void (*stdlib)(void);         /* poblar /lib con la stdlib embebida (bpvm_mods) */
    void (*con_fs)(void);         /* lo de la familia que pide el FS (board.json, el log del FS) */
    void (*hw)(void);             /* los backends del hardware (la HAL BP) */
    void (*sd)(void);             /* registrar la SD de la placa, o decir por qué no hay */
    void (*identidad)(void);      /* la identidad que dicen INFO y HELLO */
    void (*cintura)(void);        /* registrar la cintura del REPL (bpvm_repl_set_ops) */
    void (*cable)(void);          /* abrir el cable del wire */
    void (*autorun)(void);        /* /sys/auto.txt (bpvm_autorun_*) */
    void (*repl)(void);           /* el lazo del REPL: no vuelve (en las pruebas, sí) */

    /* V7/H12 — los NÚCLEOS de esta imagen (su configNUMBER_OF_CORES), y con ellos los runs por defecto: uno por núcleo
     * (Eduardo, 6-oct: «En la Pico y Metro, 2 nucleos y 2 runs. En la P4 y la S3 igual. En el resto 1 nucleo y 1
     * run»). `vm.runs` del ENV manda sobre él. 0: la familia no lo dice, y va con uno. Al final, para que una familia
     * escrita por posición (la de la prueba) siga valiendo. */
    int nucleos;

    /* V8 #784 — 1: esta familia pone al arrancar la frecuencia de la CPU de la clave `cpu.mhz` del ENV, después del
     * hardware y antes de los buses, con el mismo setter que Machine.setCpuFreqMHz (bpvm_machine_set_cpu_freq_mhz: la
     * HAL de la familia, que sabe qué valores da su chip). 0: no la mira —la RP2350, de momento: cambiar su reloj no
     * recalcula los divisores de la PSRAM ni de la flash—. Al final, como `nucleos`. */
    int cpu_desde_env;
} bpvm_boot2_familia_t;

/* Las claves del ENV que REPARTEN (`stack=`, `quantum=`): se leen en el arranque y hace
 * falta un reset para que entren. Va sola porque el micro simulado, que no sube por aquí,
 * las aplica igual. */
void bpvm_boot2_claves(const bpvm_env_t* env);

/* 2a: las claves del reparto, la memoria y la BIOS. Antes de que nadie más pida memoria. */
void bpvm_boot2_repartir(const bpvm_boot2_familia_t* f);

/* 2b: subir, la fecha de la imagen (una vez), la zona, /lib, el hardware, los buses del
 * ENV, la SD, la identidad, la cintura del REPL, el cable, `log=`, el autorun —sólo con la
 * app SANA— y el REPL. No vuelve en placa (el REPL no vuelve). */
void bpvm_boot2_subir(const bpvm_boot2_familia_t* f);

/* V7/H10 (#626) — la clave del ENV con la fecha de la imagen (la lee Rtc.Clock.isSet()). */
#define BPVM_BOOT2_CLAVE_FECHA "imagen.fecha"

/* V7/H10 (#626) — «Oct  2 2026» (el formato de __DATE__) → los segundos desde 1970 de ese día
 * a las 00:00 (como UTC: la fecha no lleva zona horaria, y el margen lo pone quien compara);
 * -1 si no se entiende. Años 1970..2037: el reloj de BP es de 32 bits. Suelta, para su prueba. */
long bpvm_boot2_fecha_de(const char* fecha);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_BOOT2_H */
