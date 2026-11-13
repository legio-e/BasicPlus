/*
 * bpvm_repl.h — EL CONTRATO del REPL wire v1, separado de la familia.
 *
 * ─── Por qué existe (V6/U3, 26-ago-2026) ─────────────────────────────────────
 *
 * El REPL estaba TRIPLICADO: `repl_v1.c` (Pico, 2054 líneas), `repl_esp32.c`
 * (S3+P4, 1344) y `stm32_repl.c` (920) — 4.318 líneas sin contrato, y el 100 %
 * de las asimetrías que nos han mordido: verbos que faltan en una familia,
 * respuestas distintas al mismo caso, arreglos que no viajan.
 *
 * El censo U3.0/U3.0b (FICHAS) midió las dos cosas que hacían falta ANTES de
 * escribir esto: qué verbos tiene cada familia (la Pico es superconjunto
 * estricto de las tres), y qué hay DEBAJO de cada verbo ausente — porque un
 * verbo enrutado a nada no es un verbo (pregunta de Eduardo, y hasta la Pico
 * tenía verbos-fachada).
 *
 * ─── El reparto ──────────────────────────────────────────────────────────────
 *
 *   COMÚN (src/bpvm_repl.c)  lo que solo toca contratos ya comunes: el wire
 *                            (bpvm_wire_v1.h), el FS (bpvm_fs.h), el log
 *                            (bpvm_log.h), el RTC... Interpretar "PING" no es
 *                            hardware.
 *   FAMILIA                  el cable, RUN (por ahora), y los verbos de SU
 *                            hardware (BOOTSEL...). Donde no hay soporte, la
 *                            respuesta es un ERROR CON NOMBRE, nunca silencio.
 *
 * ─── Cómo se usa ─────────────────────────────────────────────────────────────
 *
 *   El dispatcher de la familia, tras parsear type/id/obj, pregunta PRIMERO:
 *
 *       if (bpvm_repl_dispatch(type, id, &obj)) return;   // atendido
 *       ...sigue su cadena de verbos propios...
 *
 *   Si una familia necesita sombrear un verbo del común (no debería), lo
 *   atiende ANTES de llamar aquí. La migración es por GRUPOS y cada grupo se
 *   verifica en placa antes del siguiente — criterio de Eduardo para todo el
 *   cordón: *«pasitos pequeños, cada cambio verificado en placa; nos importa
 *   más la seguridad que la velocidad»*.
 *
 * ─── Qué verbos atiende hoy el común ─────────────────────────────────────────
 *
 *   Grupo 1 (meta):  PING · TIME · LOG_DUMP · LOG_CLEAR
 *
 *   Divergencias que este grupo UNIFICA (medidas el 26-ago, U3.0b):
 *     · TIME sin `epochSec`: la Pico daba INVALID_PARAM; S3/STM32 contestaban
 *       OK en silencio. Gana la Pico: el error se DICE.
 *     · LOG_CLEAR: la Pico dejaba la marca «LOG cleared via wire v1» en el log
 *       nuevo; el STM32 no. Se deja siempre: fecha el corte al leer un dump.
 *     · el sink de LOG_DUMP del STM32 PERDÍA chunks en silencio si el escape
 *       no cabía en su buffer; el del común trocea y no pierde.
 */
#ifndef BPVM_REPL_H
#define BPVM_REPL_H

#include "json_min.h"
#include "bpvm_boot.h"   /* #691: la puerta del arranque recibe el estado con su motivo */

#ifdef __cplusplus
extern "C" {
#endif

/* ══════════════ LA CINTURA: lo que cada familia pone ══════════════
 *
 * Medido antes de diseñarla (26-ago), comparando el INFO de las tres: **18
 * campos son comunes a todas** y 11 son sólo del RP2350 (variante, packs/XIP,
 * los cinco de SQLite, `floatAbi`, dos del RTOS). De ahí la forma:
 *
 *   · los 18 comunes viajan como VALORES en `bpvm_repl_info_t` — la familia
 *     rellena una struct, no implementa 18 funciones;
 *   · lo propio entra por UN gancho (`info_extra`), que añade sus campos al
 *     mismo mensaje;
 *   · y sólo son punteros a función las ACCIONES (formatear, guardar) y los
 *     contadores del FS, que no tienen fachada común todavía.
 *
 * ⚠️ Un campo que una familia no sepa: se deja a 0 / cadena vacía, NO se omite.
 * El IDE lee por nombre y un campo ausente y uno a cero se distinguen; que la
 * forma del mensaje sea SIEMPRE la misma es justo lo que este hito persigue. */

typedef struct {
    const char* unique_id;        /* serie del micro, ya formateado */
    const char* board_name;
    const char* reset_reason;
    unsigned    arch;             /* el tag del `.mdn` que ejecuta esta imagen */
    unsigned long cpu_hz;
    unsigned long uptime_ms;
    long        temp_milli_c;
    int         gpio_count, pio_count, pwm_slices, adc_channels;
    unsigned long flash_bytes, sram_bytes, psram_bytes;
    unsigned long vm_heap_bytes, vm_stack_bytes;
    unsigned long fs_total_bytes, fs_used_bytes;
    /* V7/H8 #602 (1-oct) — EL BLOQUE DE RAM DE LOS MOTORES NATIVOS (V7/H6.c G1: el reparto de la RAM es el sello),
     * igual en las cuatro familias: dónde y cuánto (0 y 0 = no hay), el motivo de la regla común
     * (`bpvm_packram_region`: off/ok/low/nofit) y la clave del ENV que lo pidió, con lo que pedía en SU unidad (KB
     * `packRam=`, MB `SQLite=`). Era de la Pico, con los nombres de V5 (`sqlite*`); la ESP32 y la STM32 no lo decían. */
    unsigned long pack_ram_base, pack_ram_bytes;
    int           pack_ram_res;        /* bpvm_sqlite_res_t */
    const char*   pack_ram_clave;      /* "packRam", "SQLite" o NULL (ninguna) */
    long          pack_ram_pedido;
} bpvm_repl_info_t;

/* ── V7/H10 — EL RUN, EN UN SOLO SITIO: la cintura de cada familia (el RUN común, paso 2; 3-oct) ──────────────────
 * `bpvm_repl_run` hace el RUN entero —resolver, la sesión, crear la VM, cargar, los nativos, el depurador, `io`, el
 * poll, correr, el KILL_REPLY y el EXITED— con las reglas de Eduardo (2-oct, `V7_IDEAS` §H10 «RUN en un solo sitio»):
 * un fallo de carga sale con exitCode 2 y «falta el modulo 'X'», «load: <fallo>» o «load: <estado>»; un KILL o un
 * RESET, con SU texto y nunca el de una excepción vieja; OUTPUT es sólo del programa (los avisos van al log); `gc=0` y
 * `latido=1` por el ENV; y el poll, «pegajoso» con un KILL pedido. Lo que es de verdad de cada familia, aquí. Las
 * cuatro la ponen en `bpvm_repl_ops_t.run` (el paso 6, 3-oct, borró sus RUN viejos tras probar el común en placa): con
 * ella, el despachador común atiende el RUN, el KILL sin programa y el depurador de antes del RUN, y la familia sólo
 * llama a `bpvm_repl_run` en su autorun (con id < 0) y le presta `bpvm_repl_autorun_escucha` a la ventana de #345. */
struct bpvm;
typedef struct {
    /* ── obligatorios ── */
    /** El bloque de la VM (base y bytes). -1 = no hay: no se arranca a medias. */
    int  (*memoria)(uint8_t** base, size_t* bytes);
    /** Una línea del cable SIN ESPERAR: su longitud (>0), 0 si no hay nada (o llegó rota), <0 si el cable se ha ido
     *  (el programa se para, como con un KILL). */
    int  (*leer_linea)(char* buf, size_t cap);
    /** El búfer de esa lectura (lo usa `io`) y el del depurador (lo usa la tarea de la VM): uno por rol (A1.7). */
    char*  linea;     size_t linea_cap;
    char*  dbg_linea; size_t dbg_linea_cap;
    /** Donde se arman el RUN_REPLY, el EXITED y las respuestas del depurador (todo, en la tarea de la VM). */
    char*  mensaje;   size_t mensaje_cap;
    /** El RESET pedido durante el RUN, servido después del EXITED. No vuelve (o, en el simulado, corta el cable). */
    void (*reiniciar)(long id);
    /* ── opcionales: NULL = nada ── */
    void (*cargar_motores)(void);                                  /* los packs nativos, una vez (antes de resolver) */
    const uint8_t* (*mdn_del_fs)(void* user, const char* nombre, uint32_t* len);   /* `user` = la VM */
    void (*tras_cargar)(struct bpvm* vm);                          /* los nativos de la imagen (por el enlazador) */
    void (*antes_de_correr)(struct bpvm* vm);
    /* (V7/H12, etapa 2a.2 — fuera `correr`: el programa lo corre el planificador de la cola; el de la Pico era su SMP,
     *  que su build no encendía.) */
    void (*despues_de_correr)(struct bpvm* vm);                    /* tras el EXITED, antes de soltar la VM */
    void (*led)(int estado);                                       /* 1 corre, 0 acabó bien, -1 error */
    int  (*env_bool)(const char* clave, int defecto);              /* `gc`, `latido`… */
    const char* (*consejo_falta)(void);                            /* qué hacer si falta un módulo (el simulado) */
    unsigned long io_cola;                                         /* la cola de salida de `io`; 0 = la de por defecto */
} bpvm_repl_run_ops_t;

typedef struct {
    /** Rellena los 18 campos comunes. Obligatoria. */
    void (*info)(bpvm_repl_info_t* out);

    /* ── Lo que HELLO tiene de propio, y sólo eso ──────────────────────────
     * El resto del saludo (protoVersion, la forma del mensaje) es del común.
     *
     * ⚠️ `server_build` NO se calcula en el común, y es a propósito: un
     * `__DATE__` puesto en `src/bpvm_repl.c` sería la fecha en que se compiló
     * ESE fichero — común y rara vez recompilado—, o sea una fecha que no
     * identifica la imagen. Ya nos costó un diagnóstico falso una vez. Que lo
     * ponga la familia no lo arregla del todo (sigue siendo la fecha de SU
     * fichero, no la del enlace), pero al menos no empeora. */
    const char* server_name;      /* "bpvm-pico", "bpvm-stm32"… */
    const char* server_build;     /* sello de compilación de la familia */
    const char* capabilities;     /* el array JSON literal: "[\"META\",…]" */

    /* ── PUT: el scratch y la política de persistencia ─────────────────────
     * El buffer lo pone la FAMILIA y no el común, por dos razones medidas: su
     * tamaño tiene restricciones propias (el STM32 lo usa además de scratch del
     * sector de env, la Pico lo comprueba con un assert de compilación), y una
     * familia pequeña no tiene por qué pagar 8 KB que no usa. */
    unsigned char* put_buf;
    unsigned long  put_buf_size;

    /* Qué hacer cuando una subida ha aterrizado bien. NO es lo mismo en todas:
     * el STM32 persiste el FS (salvo bajo /lib/, que el embebido reinstala en
     * cada boot — así se ahorra un erase+program por Run), y la Pico no hace
     * nada. Es política de familia, así que la decide la familia. NULL = nada. */
    void (*after_put)(const char* path);

    /** Añade los campos PROPIOS de la familia al mensaje a medio construir.
     *  Recibe el buffer y el offset actual; devuelve el offset nuevo, o -1 si
     *  no cabe. Puede ser NULL: la mayoría de familias no tienen extras. */
    int (*info_extra)(char* buf, unsigned long buf_max, int off);

    /** Contadores del FS para DF. Obligatorias (no hay fachada común aún). */
    unsigned long (*fs_total_bytes)(void);
    unsigned long (*fs_used_bytes)(void);
    int           (*fs_file_count)(void);

    /** Formatear el FS. NULL = esta familia no lo soporta (y el común
     *  responderá con un error CON NOMBRE, no con «type no implementado»). */
    int (*fs_format)(void);

    /** Persistir el FS. NULL = no hace falta (littlefs persiste al cerrar):
     *  el común contesta OK, que es la verdad, no un no-op disfrazado. */
    int (*fs_save)(void);

    /* V7/H8 #495 (1-oct) — EL BÚFER DE FICHEROS del PUT troceado, SEPARADO del de comunicaciones (Eduardo:
     * «estamos mezclando dos cosas: un buffer de comunicaciones y el buffer de archivos [...] yo los
     * separaría»). Lo que llega por el cable aterriza en `put_buf` y se junta aquí; cuando se llena, va al
     * fichero, que se queda ABIERTO toda la sesión. Un bloque de borrado de SU flash: 4 KB la RP2350 y la
     * ESP32, 8 KB la STM32U5. NULL = sin búfer: cada trozo va al fichero abierto según llega. */
    unsigned char* fich_buf;
    unsigned long  fich_bytes;

    /* V7/H8 #495 (1-oct) — EL TROZO Y LA VENTANA que la placa ANUNCIA en el HELLO (`putChunk`, `putWindow`):
     * el cliente sube en trozos de `put_trozo` bytes con hasta `put_ventana` sin acuse. Los decide el búfer de
     * comunicaciones de la familia (Eduardo: «512 bytes o 1K [...] si el buffer en las placas es pequeño
     * entonces 512»; «una ventana de 4 paquetes de 512 bytes son 2K»). 0 = no se anuncia, y el cliente sube como
     * siempre (un trozo, un acuse). */
    unsigned long  put_trozo;
    unsigned long  put_ventana;

    /* V7/H9 (2-oct) — EL SELLO DE LA IMAGEN, en el HELLO (`imageCrc`): el CRC32 —el de zlib— de los bytes de la imagen
     * que corre, tal como están en su flash. `server_build` es la hora en que se compiló UN fichero y miente en un
     * build incremental; esto identifica la imagen exacta, y el PC puede calcular el mismo número sobre lo que acaba de
     * compilar. La familia lo calcula la primera vez que se pide y lo guarda (recorrer la imagen cuesta unas decenas de
     * ms). NULL = no lo da, y el HELLO va sin el campo. */
    uint32_t (*image_crc)(void);

    /* V7/H10 — la cintura del RUN común (arriba). NULL = sin RUN: el despachador común no atiende RUN, KILL ni el
     * depurador (hoy, ninguna familia). */
    const bpvm_repl_run_ops_t* run;
} bpvm_repl_ops_t;

/** La familia registra su cintura UNA vez, antes de atender el primer mensaje.
 *  Sin esto, los verbos que la necesitan responden error con nombre. */
void bpvm_repl_set_ops(const bpvm_repl_ops_t* ops);

/* Traga y tira los `n` bytes de bulk anunciados por un mensaje.
 *
 * Lo necesita quien RECHAZA un mensaje que traía bulk antes de que llegue al
 * handler — típicamente la puerta de arranque de cada familia («sin FS montado,
 * nada de ficheros»). El bulk viaja DETRÁS de la línea JSON: si se contesta el
 * error y se vuelve sin consumirlo, esos bytes se quedan en el cable y el
 * mensaje siguiente se lee desde la mitad de los datos. El síntoma no es «falló
 * el PUT», es que a partir de ahí no funciona nada.
 *
 * Best-effort: 0 si se consumieron todos, <0 si el cable se cortó a medias (en
 * cuyo caso ya no hay sincronía que salvar). */
int bpvm_repl_drain_bulk(unsigned long n);

/** Atiende `type` si es un verbo del común. Devuelve 1 si lo atendió (la
 *  respuesta ya salió por el wire), 0 si no es suyo y la familia debe seguir
 *  con su cadena. `obj` es el mensaje ya parseado (para los parámetros). */
int bpvm_repl_dispatch(const char* type, long id, json_obj_t* obj);

/** V7/H8 #601 (28-sep) — ¿`type` es un verbo que necesita el sistema de ficheros MONTADO? La puerta de «sin FS →
 *  NOT_READY» de cada familia pregunta AQUÍ. Era una lista copiada en cada una y las tres habían divergido: la ESP32
 *  no paraba MKDIR, RMDIR, RENAME ni FORMAT (sin partición montada, un FORMAT pasaba); la Pico y la STM32, LIST_DIR;
 *  la STM32, SAVE. Un verbo de fichero nuevo se añade aquí, una vez. */
int bpvm_repl_verbo_de_fs(const char* type);

/** V7/H10 #473 (punto 10; 3-oct) — LA PUERTA DEL ARRANQUE, una para las tres familias: sin el FS montado (estado del
 *  arranque < BPVM_BOOT_FS) un verbo de fichero no pasa, y sin la VM (< BPVM_BOOT_APP) un RUN tampoco («si no hay
 *  partición, nada con el sistema de ficheros», Eduardo). La lista ya era una (#601); la puerta seguía escrita tres
 *  veces —el `if`, tragar el bulk y el NOT_READY del RUN—, con tres mensajes. Si rechaza, contesta NOT_READY con el
 *  estado y devuelve 1; si no, 0. `bulk_en_cable`: el bulk del mensaje sigue sin leer (hay que tragárselo, o el
 *  mensaje siguiente se lee desde la mitad de los datos).
 *  #691 (12-oct): recibe el ESTADO entero —el de board_boot_status()—, no sólo su número, para decir el motivo: un RUN
 *  con el FS montado y sin VM dice por qué no hay VM (bpvm_boot_no_listo_msg), no «configurar particiones». */
int bpvm_repl_puerta_arranque(const char* type, long id, json_obj_t* obj, const bpvm_boot_status_t* st,
                              int bulk_en_cable);

/** V7/H8 #495 — lo que la familia llama con CADA mensaje del lazo, en cuanto sabe su `type` y antes de despachar
 *  nada (también los que atiende ella: RUN, RESET...). Hoy, una cosa: si el verbo no puede convivir con una subida a
 *  medias —un HELLO, un RUN, un RESET, otra subida o un verbo que escribe en el FS—, la acaba: cierra el fichero y
 *  BORRA lo subido (decisión de Eduardo, 1-oct). Los que sólo leen sí pueden ir entre medias. */
void bpvm_repl_antes_de_despachar(const char* type);

/* ── V7/H8 #495 (paso 4) — LAS TRANSFERENCIAS ─────────────────────────────────────────────────────────────────────
 * Un GET o un LIST que el cliente pide troceados ("chunked":true) no se mandan de una vez: dejan una TRANSFERENCIA en
 * el común, y cada paso manda un trozo —un GET_PART (o el GET_END), un directorio del LIST—. Entre trozo y trozo, el
 * cable es de los demás: la salida del programa, un KILL, otra respuesta (Eduardo: «en un entorno multitarea no puede
 * ser que una comunicación monopolice la comunicación»). Una descarga a la vez (Eduardo: «1 a 1») y un listado, que
 * sí puede ir con ella. V7_IDEAS §H8, el paso 4. */

/** Manda UN trozo de cada transferencia viva. Lo llaman el lazo del REPL de cada familia cuando no hay nada que leer
 *  (mientras devuelva 1, sin dormir) y el poll del hilo `io` durante un RUN, en cada vuelta. 1 si queda algo. */
int bpvm_repl_paso(void);

/** ¿Hay una transferencia a medias? Para el lazo que espera el cable sin plazo (la ESP32): con una, mira el cable sin
 *  esperar y, si no hay nada, da el paso. */
int bpvm_repl_hay_transferencia(void);

/** ¿Se atiende `type` DURANTE UN RUN? Los de lectura que no monopolizan el cable: el GET y el LIST troceados y STAT.
 *  El poll de cada familia lo pregunta antes de su BUSY y, si es que sí, llama a bpvm_repl_dispatch. Los enteros
 *  siguen con BUSY (un GET entero dejaría a `io` sin salida ni KILL mientras dura), LIST_DIR también (su foto sale de
 *  la zona de rascar, la del `listDir` del programa) y los que escriben, también. */
int bpvm_repl_atiende_en_run(const char* type, json_obj_t* obj);

/** El BUSY de lo demás durante un RUN, el mismo texto en las cuatro familias y el simulado. */
#define BPVM_REPL_OCUPADO_EN_RUN "ejecución en curso: sólo HELLO, KILL, RESET, STAT y GET/LIST troceados"

/** V8 #693 (29-oct) — UNA LÍNEA QUE SE ESTANCÓ: lo que llama el lector de cada familia (el del REPL y el del RUN) cuando
 *  `wire_v1_recv_line` da -2, y el simulado. Lo dice al cliente con un ERROR SIN id —lo que llegó no dice de qué orden
 *  era: el cliente sabe que es la última que mandó, y la repite— y lo apunta en el log. Antes se tiraba EN SILENCIO, y
 *  así se perdió el KILL de #692. No lee ni espera nada: el lector sigue como antes (el del RUN, con su 0). */
#define BPVM_REPL_LINEA_ESTANCADA "LINE_TIMEOUT"
void bpvm_repl_linea_estancada(void);

/** V7/H10, el board_mgr común (3-oct; V7_IDEAS §H10) — STATE, ENV_*, PART_* y PACK_*, para la familia registrada con
 *  `bpvm_bmgr_familia` (bpvm_bmgr_wire.h). Era el `handle` de cada placa, el mismo código salvo la cintura. `scratch`
 *  es el búfer prestado del que salen el sector de trabajo y la respuesta; `bulk`, el de PACK_BURN_DATA, que la
 *  familia ya ha leído del cable. Si la escritura del ENV falla contesta `FLASH_ERROR` (D2: «sigue el valor de antes»)
 *  en vez del OK que salía siempre (#636). */
#define BPVM_REPL_FLASH_ERROR "no se pudo escribir la flash: sigue el valor de antes"
void bpvm_bmgr_handle(long id, const json_obj_t* obj, const char* type,
                      unsigned char* scratch, unsigned long scratch_len,
                      const unsigned char* bulk, unsigned long bulk_len);

/** El cliente se ha ido (la familia lo sabe cuando su cable lo dice: el socket del simulado): lo que se le mandaba,
 *  cortado, y su RAM y su fichero abierto, libres. Lo mismo que hace un HELLO, que es como se entera una placa. */
void bpvm_repl_cliente_ido(void);

/** V7/H10 — el RUN común (ver `bpvm_repl_run_ops_t`). `id` < 0: el autorun (sin cliente: ni RUN_REPLY, y los errores
 *  de antes de empezar, al log). `arg`: el argumento de ejecución, o NULL. Lo llaman el despachador común (el RUN del
 *  cable) y la familia en su autorun. */
void bpvm_repl_run(long id, const char* path, const char* arg);
/** ¿Hay un RUN común en curso? */
int  bpvm_repl_run_activo(void);
/** V7/H10 (paso 6) — la `escucha` de la ventana del autorun (#345, `bpvm_autorun_wire_t`), por la cintura del RUN:
 *  0 nada, 1 hay alguien (un HELLO se contesta), 2 un KILL (su KILL_REPLY sale aquí: no arrancar). */
int  bpvm_repl_autorun_escucha(void* user);

/** V7/H12, etapas 0 y 1 (4-oct) — LA TAREA `run`: el programa en una tarea propia y PERMANENTE (nace con el arranque
 *  y, sin programa, espera), y la del REPL hace de `io` durante el RUN. Es el interruptor de la imagen: la familia que
 *  la quiere, en su arranque (antes del autorun), llama a `prepara()` y, si da 0, crea su tarea con
 *  `bpvm_repl_run_tarea` de entrada, su pila y la MISMA prioridad que la del REPL (el contrato de `io`,
 *  bpvm_platform.h). La que no, sigue con el RUN de siempre. El protocolo, en bpvm_repl.c. */
int  bpvm_repl_run_tarea_prepara(void);
void bpvm_repl_run_tarea(void* arg);
/** V7/H12, etapa 2a.2 — dónde espera la tarea `run`: en LA COLA (1: es un run permanente, y el RUN cuelga ahí los
 *  programas) o en el aviso «trabajo», como en la etapa 1 (0). Es el interruptor de la imagen (BPVM_RUN_EN_COLA, 1 por
 *  defecto); el micro simulado lo cambia con --run-etapa1, antes de `prepara()`. */
int  bpvm_repl_run_en_cola(void);
void bpvm_repl_run_tarea_en_cola(int si);
/** La familia llamó a `prepara()` y no pudo crear la tarea: el RUN no cuelga programas para un run que no hay. */
void bpvm_repl_run_tarea_no_nacio(void);
/** Lo que le sobró de pila a `run` tras el último programa, en bytes (-1: sin programa, o la plataforma no lo sabe).
 *  Va al INFO (`runStackFree`), como el de `io`: con eso se dimensiona su pila en cada familia. */
long bpvm_repl_run_pila_libre(void);
/** V7/H12, 2b: los runs permanentes en vigor (las tareas `run` creadas, en la cola); 0, sin tarea `run`. Al INFO (`runs`). */
int  bpvm_repl_runs(void);
/** SÓLO PRUEBAS (las del micro simulado): `espera_ms` > 0 ensancha la ventana entre el final del programa y el aviso
 *  de `run` (como `#609`); `sabotaje` hace que `io` cierre la cola de salida sin esperar a `run` —se pierde la salida
 *  tardía, y la prueba tiene que cazarlo—. */
void bpvm_repl_run_tarea_ensayo(int espera_ms, int sabotaje);
/** SÓLO PRUEBAS (V8 #812): `espera_ms` > 0 hace que `io` tarde en ponerse a esperar, con el programa ya colgado en la
 *  cola: uno que no imprime acaba y avisa antes. El aviso no se tiene que perder (bpvm_oq_despertar deja su marca). */
void bpvm_repl_run_tarea_ensayo_io(int espera_ms);

/** V7/H8 (restos de E1, 28-sep) — el `errorMessage` del EXITED de un programa parado por un RESET, el MISMO en las
 *  cuatro (Pico, ESP32, STM32 y el micro simulado). El `poll` trata el RESET como un KILL —lo para, y reinicia
 *  después, fuera de `bpvm_run`— y el EXITED decía «terminado por KILL» (Pico y ESP32) o nada (STM32 y simulado). El
 *  `status` sigue siendo KILLED: es lo que esperan los clientes. */
#define BPVM_REPL_MSG_RESET "reiniciado por RESET"

#ifdef __cplusplus
}
#endif

#endif /* BPVM_REPL_H */
