/*
 * bpvm_sonido.h — V8/H4 (27-oct, #708): EL SONIDO, la interfaz común de la HAL BP.
 *
 * Eduardo: «la reproducción de sonidos debería ser siempre en paralelo al resto. Por ejemplo se puede utilizar para
 * dar un aviso pero el programa debería seguir.» Y: «Si tenemos una interfaz HAL BP común, cuando se reproduce un
 * sonido llama a la interfaz. Si la HAL del SO no lo soporta tendría que ser la HAL BP la que indicara el error.»
 *
 * Lo COMÚN (bpvm_sonido.c): leer la cabecera del WAV, la tarea que lo toca a trozos —la VM no espera nunca—, las
 * órdenes (tocar, parar, el volumen) y el error de una placa sin sonido, con su motivo, el mismo texto en todas.
 * Lo de CADA FAMILIA, la cintura: abrir el camino del sonido con un formato, darle PCM de 16 bits y cerrarlo. La
 * registra la familia al arrancar si la placa tiene sonido (en la P4, el ES8311 por I2S, con su línea `audio` del
 * ENV; en el PC, el audio del sistema). Una familia sin sonido no escribe nada: la HAL BP contesta por ella.
 *
 * Los builtins (287–292) llaman aquí; el espejo en la VM-Java es Sonido.java. V8_IDEAS §E4, «27-oct».
 */
#ifndef BPVM_SONIDO_H
#define BPVM_SONIDO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BPVM_SONIDO_VOLUMEN_DEFECTO 85   /* Eduardo, 19-oct: «a 85 sería un volumen 'normal'» */

/* LA CINTURA de una familia. Todo lo llama la tarea del sonido, nunca la VM, y siempre en este orden: abrir, volumen y
 * escribir las veces que haga falta, cerrar. */
typedef struct bpvm_sonido_cintura {
    const char* nombre;   /* para el log: "es8311", "winmm", "mudo"… */
    /* Enciende el camino (en la P4, el códec y el amplificador) para PCM de 16 bits con signo, `canales` (1 o 2)
     * entrelazados, a `hz`. 0, o -1 con el motivo en `err`. */
    int  (*abrir)(uint32_t hz, int canales, char* err, size_t n);
    /* `muestras` muestras (marcos × canales) de 16 bits. BLOQUEA hasta que entran: es lo que marca el paso de la tarea
     * al ritmo del sonido. 0, o -1 si el camino falla (la tarea para y lo anota). */
    int  (*escribir)(const int16_t* pcm, uint32_t muestras);
    /* De 0 (nada) a 100, cuando haga falta: al abrir y cada vez que el programa lo cambie mientras suena. */
    void (*volumen)(int v);
    /* Apaga el camino. `drenar` = 1 al acabar el fichero (lo que quede encolado, que suene); 0 al parar o al cambiar
     * de sonido (cortar en seco). */
    void (*cerrar)(int drenar);
} bpvm_sonido_cintura_t;

/* La familia, al arrancar: su cintura (NULL = ninguna). La HAL BP crea su cerrojo aquí, así que va antes de que corra
 * ningún programa. */
void bpvm_sonido_registrar(const bpvm_sonido_cintura_t* c);
/* El porqué de que una placa no tenga sonido, para el texto del error (como bpvm_sd_placa_sin_sd con la SD). */
void bpvm_sonido_sin_cintura(const char* motivo);

/* ── Lo que llaman los builtins ───────────────────────────────────────────────────────────────────────────────────── */

/* Errores de bpvm_sonido_tocar: el texto lo arma quien llama (el builtin), igual que la VM-Java. */
enum {
    BPVM_SONIDO_OK = 0,
    BPVM_SONIDO_SIN_PLACA = 1,     /* la placa no tiene sonido: el motivo, en `det` */
    BPVM_SONIDO_NO_ABRE = 2,       /* el fichero no se pudo abrir */
    BPVM_SONIDO_NO_ES_WAV = 3,     /* no es un WAV (o le falta el fmt o el data) */
    BPVM_SONIDO_FORMATO = 4,       /* un WAV que no se admite: lo que trae, en `det` */
    BPVM_SONIDO_SIN_TAREA = 5      /* no se pudo crear la tarea del sonido */
};
/* Toca el WAV de `ruta` (ya resuelta) y VUELVE EN SEGUIDA. Uno a la vez: si ya sonaba otro, lo corta. */
int  bpvm_sonido_tocar(const char* ruta, char* det, size_t n);
void bpvm_sonido_parar(void);
int  bpvm_sonido_sonando(void);       /* 1 desde tocar hasta que acaba (o hasta parar) */
void bpvm_sonido_poner_volumen(int v);   /* 0..100: el builtin ya lo ha comprobado */
int  bpvm_sonido_volumen(void);
/* EL PROGRESO del último tocar (Eduardo: «Haría falta una función para consultar el progreso»): bytes del sonido ya
 * entregados a la cintura, y los que tiene (los de su trozo `data`, recortados a lo que trae el fichero), así que el tanto
 * por ciento da justo de 0 a 100. Sin ningún tocar, 0 y 0. */
int32_t bpvm_sonido_posicion(void);
int32_t bpvm_sonido_largo(void);
int  bpvm_sonido_hay(void);           /* 1 si la placa tiene sonido */
const char* bpvm_sonido_motivo(void); /* por qué no lo tiene */

/* El fin de RUN: lo que suene se para (y se espera a que la tarea suelte el camino) y el volumen vuelve a 85. */
void bpvm_sonido_fin_de_run(void);

/* El PC (sonido_host.c): el audio del sistema —en Windows, winmm—, o en silencio, al ritmo del sonido, con
 * BPVM_SONIDO=mudo en el entorno (las pruebas de la red). En una placa no hace nada. */
void bpvm_sonido_register_host(void);

#ifdef __cplusplus
}
#endif

#endif /* BPVM_SONIDO_H */
