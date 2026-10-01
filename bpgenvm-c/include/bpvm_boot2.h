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
} bpvm_boot2_familia_t;

/* Las claves del ENV que REPARTEN (`stack=`, `quantum=`): se leen en el arranque y hace
 * falta un reset para que entren. Va sola porque el micro simulado, que no sube por aquí,
 * las aplica igual. */
void bpvm_boot2_claves(const bpvm_env_t* env);

/* 2a: las claves del reparto, la memoria y la BIOS. Antes de que nadie más pida memoria. */
void bpvm_boot2_repartir(const bpvm_boot2_familia_t* f);

/* 2b: subir, la zona, /lib, el hardware, los buses del ENV, la SD, la identidad, la
 * cintura del REPL, el cable, `log=`, el autorun —sólo con la app SANA— y el REPL. No
 * vuelve en placa (el REPL no vuelve). */
void bpvm_boot2_subir(const bpvm_boot2_familia_t* f);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_BOOT2_H */
