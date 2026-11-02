/*
 * builtins.c — dispatch de CALL_BUILTIN para F2.
 *
 * IDs estables del enum Builtin de la VM Java (docs/BUILTINS.md).
 * F2 cubre el subset mínimo para programas que usan strings y arrays
 * comunes; el resto se va añadiendo según haga falta.
 */

#include "bpvm_internal.h"
#include "bpvm_io.h"   /* V6/A1: con io, el wire no se mira desde el builtin del GUI */
#include "bpvm_alloc.h"   /* #339: reservas del nucleo con guardian */
#include "bpvm.h"       /* #338: la zona de rascar compartida */
#include "bpvm_platform.h"
#include "bpvm_out.h"   /* V8 #755: BPVM_EOL */
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>       /* H7 — NAN para eval() */
#include <stdlib.h>

#include "bpvm_gpio.h"
#include "bpvm_net.h"    /* H11 (#241) — cliente TCP */
#include "bpvm_i2c.h"
#include "bpvm_spi.h"
#include "bpvm_bus.h"     /* V7/H6.c G4.a1: BPVM_BUS_PROGRAMA */
#include "bpvm_pulse.h"
#include "bpvm_pwm.h"
#include "bpvm_machine.h"
#include "bpvm_neopixel.h"
#include "bpvm_rtc.h"
#include "bpvm_pack.h"   /* #414 — recorrer los packs grabados desde BP */
#include "bpvm_fs.h"     /* V7/H3 (#491) — el cursor */
#include "bpvm_sonido.h" /* V8/H4 (#708) — el sonido, lo común de la HAL BP */

/* ═══════════════════════════════════════════════════════════════════════════
 * V7/H3 (#491) — EL CURSOR desde BP: las nueve primitivas de IO.File y el
 * registro de ficheros abiertos, enhebrado por los propios bloques de estado.
 *
 * El bloque es el byte[] del objeto File; la fachada (bpvm_fs.h) lo rellena y
 * el motor guarda dentro su estado. La VM sólo pone dos cosas: el registro
 * (`vm->files_head` → cabecera.next → …) y los mensajes, que tienen que ser los
 * MISMOS que en miVM: son stdout cuando el programa los imprime.
 * ═══════════════════════════════════════════════════════════════════════════ */
/* V7/H3 — listDir: los nombres, NUL-separados, en la zona de rascar. */
typedef struct { uint8_t* buf; size_t cap; size_t len; uint32_t n; int desbordado; } listdir_acc_t;
static void listdir_cb(const char* name, int is_dir, uint32_t size, void* user) {
    (void) is_dir; (void) size;
    listdir_acc_t* a = (listdir_acc_t*) user;
    size_t l = strlen(name);
    if (a->desbordado || a->len + l + 1 > a->cap) { a->desbordado = 1; return; }
    memcpy(a->buf + a->len, name, l + 1);
    a->len += l + 1;
    a->n++;
}

static const char* fmodo_txt(int mode) {
    return (mode == BPVM_FS_O_READ) ? "r" : (mode == BPVM_FS_O_WRITE) ? "w"
         : (mode == BPVM_FS_O_APPEND) ? "a" : "r+";
}
/* El bloque (dirección física de sus bytes) de una ref de byte[], o NULL. */
static uint8_t* fblk_de(bpvm_t* vm, uint32_t ref, uint32_t* cap) {
    uint32_t a = (ref == 0) ? 0 : bpref_deref(vm, bpref_from_addr(ref));
    if (a == 0) return NULL;
    if (cap) *cap = bpvm_read_u32_be(vm->memory + a);
    return vm->memory + a + 4;
}
static bpvm_fs_file_t* fhdr_de_ref(bpvm_t* vm, uint32_t ref) {
    uint8_t* b = fblk_de(vm, ref, NULL);
    return b ? (bpvm_fs_file_t*) b : NULL;
}
/* V7/H12, 2b — EL CERROJO DE LOS File: el de la VM (con un run, nada). Abrir, cerrar y cada operación del cursor miran
 * si el fichero está abierto y hacen lo suyo con él cogido: con dos runs, otro hilo puede estar cerrando o abriendo el
 * mismo a la vez (con un run, el builtin entero va de una vez). Nunca se lanza con él cogido: lanzar reserva memoria,
 * y la reserva lo coge (no es recursivo). SÓLO PRUEBAS (test_cola): con el sabotaje, sin él. */
static void ficheros_cerrar(bpvm_t* vm) { if (!bpvm_la_cola.sabotaje_ficheros) bpvm_smp_lock(vm); }
static void ficheros_abrir(bpvm_t* vm)  { if (!bpvm_la_cola.sabotaje_ficheros) bpvm_smp_unlock(vm); }

/* V7/H12, 2b (la revisión) — LA ZONA DE RASCAR PARA UN BUILTIN, con dos runs. Si la tiene el OTRO run (su listDir, su
 * show), se espera a que la suelte —con dos runs, un programa correcto no puede recibir «ocupada»—; y se espera como se
 * espera al colector: sin contarse en `running_workers`. El otro puede tenerla mientras reserva memoria (listDir) y
 * necesitar parar el mundo; si éste esperase contado, el colector le esperaría a él y él a la zona: interbloqueo.
 * Quien llama no tiene nada a medias (lo que necesita de la pila BP, ya copiado, o se vuelve a validar después). La
 * zona no avisa al soltarse: se mira cada 5 ms. Con un run, la de siempre: ocupada (por `io`) es NULL, con su aviso.
 * SÓLO PRUEBAS (test_cola): con el sabotaje, sin esperar —la de antes—. */
static void* zona_para_un_builtin(bpvm_t* vm, size_t n, const char* quien) {
    for (;;) {
        void* z = bpvm_scratch_take_quieto(n, quien);
        if (z != NULL || n > bpvm_scratch_capacity()) return z;
        bpvm_cola_t* q = vm->smp;
        if (q == NULL || bpvm_la_cola.sabotaje_zona) return bpvm_scratch_take(n, quien);   /* ocupada: con su aviso */
        bpvm_platform_mutex_lock(&q->vm_lock);
        bpvm_aparcada_t nodo;                    /* V7/H12: llamado desde una native, su pila de C a la vista (heap.c) */
        bpvm_aparcada_poner(q, &nodo);
        q->running_workers--;
        if (q->stop_the_world) bpvm_cola_aviso_mundo(q);                      /* el colector espera quedarse solo */
        (void) bpvm_platform_cond_timed_wait(&q->sched_cond, &q->vm_lock, 5);
        while (q->stop_the_world)                                               /* no volver a mitad de una colecta */
            bpvm_platform_cond_wait(&q->mundo_cond, &q->vm_lock);               /* 3.2: la del mundo (bpvm_smp.h) */
        bpvm_aparcada_quitar(q, &nodo);
        q->running_workers++;
        bpvm_platform_mutex_unlock(&q->vm_lock);
    }
}

/* V7/H12, 2b (la revisión) — EL CERROJO DE LA HAL, con dos runs (con uno, nada): ver GPIO_INIT. El suyo, no el de la
 * VM (bpvm_smp.h: la HAL puede escribir en la salida con él cogido). `vm->smp` no cambia mientras corre el programa. */
static void hal_cerrar(bpvm_t* vm) { if (vm->smp) bpvm_platform_mutex_lock(&bpvm_la_cola.hal_lock); }
static void hal_abrir(bpvm_t* vm)  { if (vm->smp) bpvm_platform_mutex_unlock(&bpvm_la_cola.hal_lock); }

/* V7 #657 — un fichero que tiene abierto un builtin de fichero entero mientras dura (abrir_breves): el apunte vive en la
 * pila del builtin. */
struct bpvm_abierto_breve { const char* path; int mode; struct bpvm_abierto_breve* sig; };
typedef struct bpvm_abierto_breve bpvm_abierto_breve_t;

/* ¿Hay ya un fichero abierto con esa ruta y alguno de los dos escribe? (Un File, o un builtin de fichero entero: #657.) */
static int fichero_ya_abierto(bpvm_t* vm, const char* path, int mode) {
    for (uint32_t r = vm->files_head; r != 0; ) {
        bpvm_fs_file_t* h = fhdr_de_ref(vm, r);
        if (!h) break;
        if (strcmp(bpvm_fs_fpath(h), path) == 0 && (mode != BPVM_FS_O_READ || h->mode != BPVM_FS_O_READ)) return 1;
        r = h->next;
    }
    for (const bpvm_abierto_breve_t* a = vm->abiertos_breves; a != NULL; a = a->sig)
        if (strcmp(a->path, path) == 0 && (mode != BPVM_FS_O_READ || a->mode != BPVM_FS_O_READ)) return 1;
    return 0;
}

/* V7 #657 (7-oct) — UN BUILTIN DE FICHERO ENTERO ES UN OPEN Y UN CLOSE. Eduardo: «trabajamos siempre con handlers sobre
 * ficheros, así que siempre ha de haber un open y un close». writeFile, appendFile, removeFile, rename, copyFile y
 * readFile abrían, hacían lo suyo y cerraban por su cuenta, sin mirar la lista de abiertos: con un File abierto para
 * escribir, un writeFile al mismo fichero liberaba en la SD (FatFs, sin FF_FS_LOCK) la cadena de clusters del otro, y el
 * cierre de éste escribía en ellos —el volumen, corrupto, con un solo hilo—. Ahora se apuntan en la lista MIENTRAS DURAN,
 * como un File —mirar y apuntarse, juntos, bajo el cerrojo de los File: con dos runs, un File de otro hilo no se cuela
 * en medio—, con su misma regla: un fichero abierto para escribir no se abre otra vez; para leer, los que quieran. Dos
 * rutas (rename, copyFile), las dos o ninguna; la misma dos veces, una, para escribir. La excepción, fuera del cerrojo
 * (lanzarla reserva memoria). Devuelve 0 si se apuntaron; si no, cuál choca (1 o 2). */
static int abrir_breves(bpvm_t* vm, bpvm_abierto_breve_t* a, const char* p1, int m1,
                        bpvm_abierto_breve_t* b, const char* p2, int m2) {
    int choca = 0;
    if (b != NULL && strcmp(p1, p2) == 0) { b = NULL; m1 = BPVM_FS_O_WRITE; }
    ficheros_cerrar(vm);
    if (fichero_ya_abierto(vm, p1, m1)) {
        choca = 1;
    } else if (b != NULL && fichero_ya_abierto(vm, p2, m2)) {
        choca = 2;
    } else {
        a->path = p1; a->mode = m1; a->sig = vm->abiertos_breves; vm->abiertos_breves = a;
        if (b != NULL) { b->path = p2; b->mode = m2; b->sig = vm->abiertos_breves; vm->abiertos_breves = b; }
    }
    ficheros_abrir(vm);
    return choca;
}
static void cerrar_breves(bpvm_t* vm, bpvm_abierto_breve_t* a, bpvm_abierto_breve_t* b) {
    ficheros_cerrar(vm);
    for (bpvm_abierto_breve_t** p = &vm->abiertos_breves; *p != NULL; ) {
        if (*p == a || *p == b) *p = (*p)->sig; else p = &(*p)->sig;
    }
    ficheros_abrir(vm);
}
static void fregistrar(bpvm_t* vm, uint32_t ref, bpvm_fs_file_t* h) {
    h->next = vm->files_head;
    vm->files_head = ref;
}
static void fdesregistrar(bpvm_t* vm, uint32_t ref) {
    uint32_t* link = &vm->files_head;
    while (*link) {
        bpvm_fs_file_t* h = fhdr_de_ref(vm, *link);
        if (!h) { *link = 0; break; }
        if (*link == ref) { *link = h->next; h->next = 0; return; }
        link = &h->next;
    }
}
/* Entre la marca y el barrido: el bloque de un File inalcanzable sigue intacto y
 * su handle vivo, así que cerrar en nativo es seguro (no asigna, no llama a BP,
 * es idempotente: las tres condiciones de #491). Después el barrido lo libera. */
void bpvm_files_gc(bpvm_t* vm) {
    uint32_t* link = &vm->files_head;
    while (*link) {
        uint32_t a = (*link == 0) ? 0 : bpref_deref(vm, bpref_from_addr(*link));
        if (a == 0) { *link = 0; break; }
        bpvm_fs_file_t* h = (bpvm_fs_file_t*) (vm->memory + a + 4);
        uint32_t tag = bpvm_read_u32_be(vm->memory + a - 4);
        if ((tag & BPVM_TAG_MARK_BIT) == 0u) {
            uint32_t next = h->next;
            bpvm_fs_fclose(h);
            *link = next;
        } else {
            link = &h->next;
        }
    }
}
/* V7/H7 #595 — antes de compactar (heap.c, con el mundo parado): el bloque de un fichero abierto NO se mueve. El
 * motor guarda dentro su estado, y littlefs, además, lo enlaza en su `mlist` (fs_lfs.c): moverlo dejaría esa
 * lista apuntando al sitio viejo. Se clava con la etiqueta, y la compactación lo deja donde está. */
void bpvm_files_clavar(bpvm_t* vm) {
    for (uint32_t r = vm->files_head; r != 0; ) {
        uint32_t a = bpref_deref(vm, bpref_from_addr(r));
        if (a == 0) break;
        uint32_t h = a - 4u;
        if (h >= vm->heap_start && h < vm->heap_next)
            bpvm_write_u32_be(vm->memory + h, bpvm_read_u32_be(vm->memory + h) | BPVM_TAG_CLAVADO);
        r = ((bpvm_fs_file_t*) (vm->memory + a + 4))->next;
    }
}
/* El fin de RUN cierra lo que quede y LO DICE (errores sí, silenciosos no). */
void bpvm_files_fin_de_run(bpvm_t* vm) {
    int n = 0;
    char lista[160]; lista[0] = 0;
    for (uint32_t r = vm->files_head; r != 0; ) {
        bpvm_fs_file_t* h = fhdr_de_ref(vm, r);
        if (!h) break;
        uint32_t next = h->next;
        if (n < 3) {
            size_t l = strlen(lista);
            snprintf(lista + l, sizeof lista - l, "%s%s (%s)", n ? ", " : "", bpvm_fs_fpath(h), fmodo_txt(h->mode));
        }
        bpvm_fs_fclose(h);
        n++;
        r = next;
    }
    vm->files_head = 0;
    if (n > 0) bpvm_diag("[bpvm] fin de ejecución con %d fichero(s) abierto(s) sin cerrar: %s%s", n, lista, (n > 3) ? ", …" : "");
}

#include "bpvm_adc.h"
#include "bpvm_wdt.h"
#include "bpvm_uart.h"
#include "bpvm_fs.h"
#ifdef BPVM_GUI
#include "bpvm_gui.h"   /* V3 / H4 — backend GUI (modelo). Sólo en el build con GUI. */
#endif

/* H1 P2 (20-sep) — LOS IDS YA NO SE ESCRIBEN AQUÍ: se generan de bpstdlib/Core.bp (el módulo raíz)
 * con scripts/gen_builtins.py, en builtin_ids.h — los mismos que el ordinal del enum Builtin de miVM,
 * del mismo fichero. Lo que sigue son las NOTAS que acompañaban a los números (se conservan; el
 * número de cada una está en la cabecera, con su firma BP). */
#include "builtin_ids.h"
#include "bpvm_prims.h"   /* V7/H5 P2b: las primitivas compartidas con el AOT */
#include "bpvm_opcodes.h" /* #573: bpvm_prim_texto_indice nombra el opcode */

#if 0   /* notas de los ids (histórico; el enum vive en builtin_ids.h) */
    /* #348 — cadenas de cómputo puro. Estaban SÓLO en miVM: un programa que
     * las usara pasaba en el host y moría en la placa. Ids del enum Builtin. */
    /* #348 tanda 2 — matemáticas de cómputo puro. Todas calculan en DOUBLE y
     * estrechan a float AL FINAL, igual que miVM (Math.* de Java toma double y
     * el resultado se castea a (float)). Usar las variantes 'f' de libm daría
     * otro último bit y rompería la paridad. */
    /* #348 tanda 3 — rutas. SIEMPRE con '/' (decisión de Eduardo): el FS del
     * dispositivo es '/', y la semántica la definimos nosotros en vez de
     * heredar la de java.nio.file, que es dependiente de plataforma. */
    /* #484 - `IO.prompt` SI existe en la VM-C, y lo que hace es DECIR QUE NO
     * PUEDE. El dialogo lo pinta el IDE (verbo PROMPT_REQUEST del wire, que aqui
     * no esta implementado), asi que en esta VM nunca hay quien lo conteste.
     * Antes caia en el `default` y lanzaba «builtin 77 no soportado en esta VM
     * (subconjunto C)»; miVM sin IDE lanza «prompt: no hay IDE conectado», que
     * ademas es lo que promete la doc de IO.bp. Dos mensajes distintos para el
     * mismo programa = paridad rota, y no lo veia nadie porque no habia ningun
     * sample suyo en el corpus. */
    /* Numéricas enteras (V2/GAP-1) — byte-exactas, sin riesgo de paridad float. */
    /* File I/O (V2/H10/#247) — paridad con enum Builtin Java (ids 38..41). */
    /* #240 (logger) — gestión de ficheros via IO.bp. */
    /* #240 (2ª pasada) — resto de IO.bp. */
    /* L13 — concat string + float/long/double (formateo canónico GAP-4). */
    /* H11 (#241) — cliente TCP simple via fachada bpvm_net. */
    /* Gpio.* — paridad con enum Builtin Java (ordinals 78..81). */
    /* I2c.* — ordinals 82..84. */
    /* Allocator visible al user (ordinal 85). */
    /* Spi.* — ordinals 86..89. */
    /* Uart.* — ordinals 90..93. */
    /* Pulse counter — ordinals 94..98. */
    /* PWM — ordinals 99..103. */
    /* Pico (info MCU) — ordinals 104..108. */
    /* Time (sleep variantes) — ordinals 109..110. */
    /* Pico overclock — ordinal 111. */
    /* Rtc — ordinales 112..113. */
    /* Adc — ordinales 114..115. */
    /* Wdt — ordinales 116..118. */
    /* H2 (V2) — conversión string <-> byte[] (ambos TYPE_ARRAY_I8). */
    /* 121/122 = HEAP_FRAG/HEAP_MAP: diagnósticos SÓLO de la VM-Java; la VM C
     * no los implementa. Hueco intencional para mantener el id alineado al
     * ordinal del enum Builtin.java. */
    /* H7.3 — board-aware (RP2350A/B): GPIO de la variante desde board_desc. */
    /* H7.4 — NeoPixel WS2812 (device-only; no-op en host). */
    /* H10/#247 — file I/O binario (byte[]). Mismo cuerpo que READ_FILE/WRITE_FILE
     * (heap TYPE_ARRAY_I8, bytes crudos sin pasar por string); el frontend los
     * tipa byte[]. Ids al final, alineados con el enum Builtin.java (126/127). */
    /* V3 / H4 — GUI (modelo de comportamiento + dumpTree). Ids 135..152 =
     * ordinal del enum Builtin Java. Sólo se despachan en el build con BPVM_GUI;
     * sin él caen al default (= micro sin-GUI). */
    /* H6 — geometría (backend=verdad) + scroll (opt-in) + refresh. Ids = ordinal
     * del enum Builtin Java (idénticos en ambas VMs; aditivo). */
    /* H6 — checkbox (1er value-widget): create + checked + el inyector __guiChange
     * que cierra el camino onChange (kind=CHANGE). Ids = ordinal del enum Java. */
    /* H6 widgets — switch + slider + bar (value-widgets enteros). */
    /* H6 widgets — spinbox (entero+rango) + led (indicador on/off). */
    /* H6 widgets — dropdown (opciones + índice) + textarea (texto editable). */
    /* H6 widgets — list (ítems + índice) + keyboard. */
    /* H6 widgets — msgbox (aviso async: mensaje + botones). */
    /* H6 widgets — tabview (pestañas; addTab devuelve el handle de la página). */
    /* H6 widgets — table (rejilla de celdas filas×columnas). */
    /* H6 widgets — image (asset separado del control que lo muestra). */
    /* H6 — fuente: tamaño de texto por componente (catálogo). */
    /* H6 — textarea read-only (sin cursor, no editable). */
    /* H7 — eval("expr"): calculadora de constantes (id 200). */
    /* H10 — Pico.resetCause(): causa del último reset como string (id 201). */
    /* H10 — breadcrumb en RAM retenida: migas que sobreviven al reset (202-205). */
    /* H13 (V3) — Forms: call-by-name del handler (host, name, sender) → void. */
    /* H13.1 (V3) — Forms Camino A: dispatch del handler por SLOT de vtable
     * (win, slot, sender) → void. El handler es un MÉTODO de la ventana cuyo slot
     * horneó el IDE en el .win (resuelto vía .bpi/slotOf). */
    /* H14 — counts de periféricos board-aware (del board_desc / board.json). */


    /* H19 — introspección del proyecto en ejecución (módulo App). */

    /* Orientación del display en runtime (0/90/180/270 grados). */

    /* H7 — Chart. Los ids DEBEN coincidir con el ordinal del enum Builtin.java
     * (allí id = ordinal(), por eso las entradas nuevas van al final). El eje Y
     * y el repintado reusan los genéricos SET_RANGE / REFRESH. */

    /* H5.c — `raise ev(args)`. Encola; NO llama al handler (lo inyecta el
     * scheduler entre quanta). Ver docs/V4_IDEAS.md §contrato de __eventRaise. */

    /* #324 — UNA PASADA del bombeo del GUI, en vez del lazo entero.
     *   devuelve 1 = "queda trabajo, vuelve a llamarme"
     *            0 = "no queda nada, puedes salir"
     *
     * POR QUÉ SE PARTIÓ. __guiRun bloqueaba hasta cerrar la ventana, y mientras
     * bombeaba la VM ENTERA estaba parada: ni avanzaban los threads ni se drenaba
     * la cola de eventos. Con el lazo en BP (Gui.run) hay frontera de instrucción
     * entre pasadas, que es el ÚNICO sitio donde el scheduler puede inyectar el
     * frame de un handler — events.c lo exige: "El thread NO puede estar
     * corriendo". Medido en samples/GuiEvSpike.bp: sin esto, un evento levantado
     * desde el upcall del GUI no se drena JAMÁS.
     *
     * En headless devuelve 1 si esta pasada drenó algo: así el lazo da UNA VUELTA
     * MÁS y el scheduler tiene dónde correr los handlers antes de salir. */

    /* #414 — recorrer los packs GRABADOS y su contenido desde BP. Dos por
     * nivel: uno AVANZA (devuelve la posición siguiente, -1 al final) y otro
     * dice el TEXTO de esa posición. Así ninguno devuelve dos cosas y la lista
     * se arma en BP — ningún builtin construye objetos.
     *
     * ⚠️ Estos números se escriben A MANO aquí y tienen que ser el ordinal() del
     * enum Builtin de miVM. Si divergen, la VM-C ejecuta OTRO builtin y el
     * síntoma no apunta a esto. */

    /* V6/L1 — los cuatro que le faltaban a `Math`. AL FINAL, y los ids son los
     * MISMOS que el ordinal() del enum Builtin de la VM-Java: si aquí y allí no
     * coinciden, un `.mod` ejecuta otra funcion y no lo dice nadie. */

    /* V6/#412 — el argumento de ejecucion, SIEMPRE en el heap. Lo emite
     * `__startup` pasandole el valor POR DEFECTO que diga el fuente; devuelve
     * el argumento de ejecucion si lo hay, y si no una COPIA del defecto —
     * alojada igual. Asi Main recibe siempre una referencia del heap. */
    /* V6 (9-sep) — el MICRO. La PLACA la da BUILTIN_MACHINE_BOARD_NAME: son DOS
     * campos distintos desde el rediseno de `Machine` (getMicro/getBoard).
     * 233 = ordinal() de MACHINE_MICRO en el enum Builtin de miVM. */
    /* V6/G2-4 (11-sep) — la geometria AUTORADA del widget, para el toJson() de
     * Gui.bp. 234..238 = ordinal() de sus gemelos en el enum Builtin de miVM. */
    /* V6/C1 (12-sep) — Gui.shot(path): la captura de pantalla a fichero .shot.
     * 239 = ordinal() de GUI_SHOT en el enum Builtin de miVM. */
#endif

/* Helpers: pop / push del thread actual. */
static int32_t pop_i32(bpvm_t* vm, bpvm_thread_t* tc) {
    tc->sp -= 4;
    return bpvm_read_i32_be(vm->memory + tc->sp);
}
static void push_i32(bpvm_t* vm, bpvm_thread_t* tc, int32_t v) {
    bpvm_write_i32_be(vm->memory + tc->sp, v);
    tc->sp += 4;
}
/* H1.2a (V4): refs = 8 bytes (plano, low32 = dirección). */
/* V4: adaptadores finos sobre la abstracción bpref_* (bpvm_internal.h). Mantienen
 * la firma uint32_t para no tocar aún los ~99 callers; el ancho/codificación ya
 * pasa por BPVM_REF_SIZE + bpref_load/store. Migración capa a capa después. */
static uint32_t pop_ref(bpvm_t* vm, bpvm_thread_t* tc) {
    return bpref_addr(bpref_pop(vm, tc));
}
/* V4 (tanda 2): dirección FÍSICA del objeto tras un pop_ref — deref del handle
 * por la tabla (identidad para direcciones directas/constantes). Obligatorio
 * antes de tocar vm->memory: el valor de pop_ref es un handle (idx|TAG), no
 * una dirección. */
static uint32_t ref_addr(bpvm_t* vm, uint32_t ref) {
    return (ref == 0) ? 0 : bpref_deref(vm, bpref_from_addr(ref));
}
static void push_ref(bpvm_t* vm, bpvm_thread_t* tc, uint32_t ref) {
    /* V4: `ref` puede ser un handle tageado de 32b (idx|TAG) cuyo llamante PERDIÓ
     * la generación al truncar el bpref_t de 64b a uint32_t (p.ej. el retorno de
     * bpvm_heap_alloc_string). bpref_regen reconstruye la gen VIVA del slot (aquí
     * la correcta: el objeto se acaba de registrar) → el handle en pila lleva su
     * gen real. Mismo patrón, misma implementación que el guardado del msg en
     * exceptions.c (bpvm_internal.h). */
    bpref_push(vm, tc, bpref_regen(vm, ref));
}

/* #348 — lee un argumento ref SIN sacarlo de la pila (0 = el de más arriba).
 *
 * POR QUÉ EXISTE: el GC marca escaneando [stack_base, sp) — EXCLUYE lo que ya
 * se ha sacado. Y bpvm_heap_alloc PUEDE colectar (umbral de bump, y el
 * reintento tras OOM). Así que el patrón «pop del origen → alocar el resultado
 * → seguir leyendo del origen» deja la cadena origen SIN RAÍZ justo cuando
 * puede pasar el GC: si el llamante no la tenía además en un local, se la
 * lleva. Con handles no corrompe en silencio —la gen no cuadra y grita— pero
 * el programa falla igual.
 *
 * Con peek el argumento sigue por debajo de sp, o sea que sigue siendo raíz.
 * Se saca AL FINAL, justo antes de empujar el resultado. Todo builtin que
 * alogue y siga leyendo de un argumento tiene que usar esto. */
static bpref_t peek_ref(bpvm_t* vm, bpvm_thread_t* tc, int depth) {
    return bpref_load(vm, tc->sp - (uint32_t)(depth + 1) * BPVM_REF_SIZE);
}

/* #348 — mayúsculas/minúsculas LATIN-1 (decisión de Eduardo 31-jul).
 *
 * miVM usaba toUpperCase()/toLowerCase() de Java: Unicode completo y —peor—
 * DEPENDIENTE DEL LOCALE de la máquina (en un locale turco, 'i' sube a 'İ').
 * Reproducir eso en un micro pide tablas de kilobytes, así que se acordó
 * LATIN-1 en las DOS VMs: ASCII + el bloque U+00C0..U+00FF, que es lo que un
 * proyecto en español necesita, y sale algorítmico sin ninguna tabla.
 *
 * Propiedad que aprovecha el llamante: en UTF-8 la longitud en BYTES no cambia
 * (á U+00E1 y Á U+00C1 son 2 bytes los dos; ÿ U+00FF → Ÿ U+0178 también), así
 * que el resultado se puede alocar del tamaño exacto del origen.
 *
 * Lo que NO hace, a propósito: 'ß' (U+00DF) se queda como está — su mayúscula
 * Unicode es "SS", dos caracteres, y eso rompería la propiedad de la longitud.
 * Java sí lo convierte; es la divergencia que se aceptó al elegir Latin-1. */
static uint32_t latin1_upper_cp(uint32_t c) {
    if (c >= 'a' && c <= 'z')            return c - 32;
    if (c >= 0x00E0 && c <= 0x00FE && c != 0x00F7) return c - 32;  /* ÷ no es letra */
    if (c == 0x00FF)                     return 0x0178;            /* ÿ → Ÿ */
    return c;
}
static uint32_t latin1_lower_cp(uint32_t c) {
    if (c >= 'A' && c <= 'Z')            return c + 32;
    if (c >= 0x00C0 && c <= 0x00DE && c != 0x00D7) return c + 32;  /* × no es letra */
    if (c == 0x0178)                     return 0x00FF;            /* Ÿ → ÿ */
    return c;
}

/* #348 tanda 2 — bits de un float BP (f32) y vuelta. El stack de BP guarda los
 * floats como sus bits en un i32, igual que miVM (Float.intBitsToFloat). */
static float bits_to_f32(int32_t bits) { float f; memcpy(&f, &bits, 4); return f; }
static int32_t f32_to_bits(float f)    { int32_t b; memcpy(&b, &f, 4); return b; }

/* #348 tanda 2 — conversión double→int32 con la semántica de JAVA, que NO es la
 * de C. En C, convertir a int un double fuera de rango o NaN es COMPORTAMIENTO
 * INDEFINIDO (en x86 suele salir 0x80000000, pero no hay promesa y en ARM/RISC-V
 * puede ser otra cosa). Java lo define: NaN → 0, y saturación a MIN/MAX.
 * floor/ceil/round tienen que coincidir en las dos VMs TAMBIÉN en los extremos,
 * que es justo donde nadie mira. */
static int32_t java_d2i(double d) {
    if (d != d) return 0;                                  /* NaN */
    if (d >=  2147483647.0) return  2147483647;
    if (d <= -2147483648.0) return -2147483647 - 1;
    return (int32_t) d;                                    /* trunca hacia cero */
}

/* #348 tanda 2 — Lanczos g=7, n=9. RÉPLICA EXACTA de lanczosGamma de miVM:
 * mismos coeficientes, mismo orden de operaciones y misma reflexión. Cualquier
 * reordenación cambia el último bit y rompe la paridad. */
static double lanczos_gamma(double x) {
    static const double p[9] = {
        0.99999999999980993,
        676.5203681218851,
        -1259.1392167224028,
        771.32342877765313,
        -176.61502916214059,
        12.507343278686905,
        -0.13857109526572012,
        9.9843695780195716e-6,
        1.5056327351493116e-7
    };
    const double PI_D = 3.14159265358979323846;
    if (x < 0.5) return PI_D / (sin(PI_D * x) * lanczos_gamma(1.0 - x));
    x -= 1.0;
    double a = p[0];
    double t = x + 7.5;
    for (int i = 1; i < 9; i++) a += p[i] / (x + i);
    return sqrt(2.0 * PI_D) * pow(t, x + 0.5) * exp(-t) * a;
}

/* #348 — busca `q` (qn bytes) dentro de `p` (n bytes) desde el byte `from`.
 * Devuelve el OFFSET EN BYTES o -1. A nivel de byte y no de codepoint a
 * propósito: UTF-8 es auto-sincronizante, o sea que una secuencia válida no
 * puede casar a mitad de otra. El resultado es el mismo que buscar por
 * caracteres, y sin decodificar nada. Aguja vacía → casa en `from` (como Java). */
static long bp_find_bytes(const uint8_t* p, uint32_t n,
                          const uint8_t* q, uint32_t qn, uint32_t from) {
    if (qn == 0) return (long) (from <= n ? from : n);
    if (qn > n) return -1;
    for (uint32_t i = from; i + qn <= n; i++) {
        if (memcmp(p + i, q, qn) == 0) return (long) i;
    }
    return -1;
}

/* Lee un string BP (TYPE_ARRAY_I32 con codepoints) a un buffer C UTF-8.
 * Devuelve el número de bytes escritos (sin null terminator). Si el
 * codepoint > 127 lo escribe como '?' (F2 v1 sólo ASCII). */
static size_t read_bp_string(const bpvm_t* vm, uint32_t ref, char* dst, size_t dst_size) {
    /* H2 (V2): strings son byte[] UTF-8 → copiamos los bytes tal cual.
     * V4: la resolución ref→bytes pasa por la abstracción (bpref_deref/arr_len);
     * la firma sigue tomando uint32_t (boundary a migrar con el modelo handle). */
    if (ref == 0) { if (dst_size) dst[0] = '\0'; return 0; }
    bpref_t s = bpref_from_addr(ref);
    uint32_t nbytes = bpref_arr_len(vm, s);
    uint32_t base   = bpref_deref(vm, s) + BPVM_ARR_DATA_OFF;
    size_t out = 0;
    for (uint32_t i = 0; i < nbytes && out + 1 < dst_size; i++) {
        dst[out++] = (char) vm->memory[base + i];
    }
    /* V8/H5 (#711): si no cabe entera, se corta por CARACTERES, no en medio de uno (el siguiente byte, una continuación
     * 10xxxxxx, decía que sí): lo recortado sigue siendo UTF-8 válido. Lo pedía la GUI: setText guarda el texto en un
     * búfer de 1 KB, y getText lo devuelve como cadena, sin validar (una fuente de confianza). Utf8Gui lo prueba. */
    if (out < nbytes)
        while (out > 0 && (vm->memory[base + out] & 0xC0u) == 0x80u) out--;
    if (out < dst_size) dst[out] = '\0';
    return out;
}

/* #456 - LEER UN PATH DE BP. Igual que read_bp_string, pero AVISA si no cupo en
 * vez de dejar un path recortado -que apunta a otro fichero, o a ninguno- y
 * seguir como si nada. Devuelve 0 si cupo, -1 si no.
 *
 * Aqui el tope NO se puede quitar como se quito en el wire: alli el path ya
 * venia en la linea del mensaje y bastaba con no copiarlo (json_str_inplace).
 * Aqui la cadena vive en el heap de la VM SIN terminador, y las dos APIs de FS
 * de debajo -littlefs y FatFs- piden un `const char*` terminado en NUL. O sea
 * que hay que copiar, y copiar es tener un tamanyo. Lo que si se puede es que
 * el programa SE ENTERE, que es de lo que iba la ficha. */
static int read_bp_path(const bpvm_t* vm, uint32_t ref, char* dst, size_t dst_size) {
    if (dst_size == 0) return -1;
    dst[0] = '\0';
    if (ref == 0) return 0;
    bpref_t s = bpref_from_addr(ref);
    if ((size_t) bpref_arr_len(vm, s) + 1 > dst_size) return -1;
    (void) read_bp_string(vm, ref, dst, dst_size);
    return 0;
}

/* #486 - Resuelve los dos dispatchers del GUI UNA vez por RUN y los cachea en la
 * VM. Antes esto era un barrido de la tabla de simbolos entera con strcmp en
 * CADA pasada del bombeo, duplicado ademas en GUI_RUN y GUI_RUN_ONCE.
 *
 * El centinela 1 significa "buscado y no esta": sin el, un programa sin
 * handlers seguiria barriendo la tabla en cada vuelta, que es justo el caso que
 * mas vueltas da. Devuelve 0 en `*click`/`*change` cuando no hay. */
static void gui_dispatchers(bpvm_t* vm, uint32_t* click, uint32_t* change) {
    if (vm->gui_disp_click == 0 && vm->gui_disp_change == 0) {
        uint32_t c = 1, h = 1;                     /* 1 = buscado y no esta */
        for (int i = 0; i < vm->symbol_count; i++) {
            const char* n = bpvm_symbol_name(vm, i);
            if      (strcmp(n, "Gui.__guiDispatch")       == 0) c = vm->symbols[i].abs_addr;
            else if (strcmp(n, "Gui.__guiDispatchChange") == 0) h = vm->symbols[i].abs_addr;
        }
        vm->gui_disp_click  = c;
        vm->gui_disp_change = h;
    }
    *click  = (vm->gui_disp_click  == 1) ? 0 : vm->gui_disp_click;
    *change = (vm->gui_disp_change == 1) ? 0 : vm->gui_disp_change;
}

/* Helpers UTF-8 (utf8_cp_count / utf8_byte_offset / utf8_decode / utf8_encode)
 * viven en bpvm_internal.h (fuente única compartida con el AOT). */

/* BUG-7b — Lanza un RuntimeError BP atrapable desde un builtin nativo. Como el
 * dispatcher de OP_CALL_BUILTIN re-sincroniza pc/sp/bp/cs desde `tc` al volver:
 *   - ATRAPADO: eh_unwind dejó tc->pc en el handler (+empujó el ref), thread
 *     sigue RUNNING ⇒ devolvemos BPVM_OK y el intérprete continúa en el catch.
 *   - NO atrapado: eh_unwind dejó el thread TERMINATED ⇒ devolvemos
 *     BPVM_ERR_RUNTIME y el dispatcher termina el quantum con error.
 * tc->cs ya está fijado por el dispatcher (lo necesita throw_runtime_error). */
/* #355 — EL CAMINO DEL THROW, CONTADO PASO A PASO.
 *
 * Aqui habia tres cosas distintas que podian estar fallando y desde fuera las
 * tres se ven IGUAL (el programa acaba sin decir nada):
 *   1. nadie llama a throw            -> no sale ninguna linea
 *   2. se llama, pero la excepcion NO SE PUEDE CONSTRUIR (sin memoria para el
 *      mensaje o para el objeto)      -> sale "no se pudo construir"
 *   3. se construye pero NO HAY CATCH -> sale "SIN handler"
 *   4. todo bien                      -> sale "ATRAPADA"
 *
 * Adivinar cual de las tres era nos costo varios intentos. Que lo diga el log.
 * Tope de 5 avisos por ejecucion: un throw no es camino caliente, pero si un
 * programa entra aqui en bucle no quiero repetir el error del log sin freno. */
/* ── #362: pedir un recurso DE UN PACK CONCRETO y que no esté es un error ────
 * "pack:<Pack>/<fichero>" no es una ruta que se busca por ahí: es una petición
 * explícita ("quiero ESE, del pack ESE"). Devolver un objeto vacío en silencio
 * —que es lo que hacía loadFont— deja al programa dibujando con una fuente que
 * no está y sin manera de enterarse. Como es un error BP, el programa puede
 * atraparlo con try/catch si prefiere seguir sin el recurso.
 *
 * Ojo: sólo se aplica a la forma CUALIFICADA. Sin cualificar, la ausencia de un
 * recurso sigue comportándose como siempre; cambiar eso es otra decisión y
 * afecta a programas que ya existen. */
static int pack_uri_falta(const char* path) {
    if (!path || strncmp(path, BPVM_PACK_URI, sizeof(BPVM_PACK_URI) - 1) != 0) return 0;
    return !bpvm_fs_exists(path);
}

/* El mensaje, en un buffer estático: se construye para tirarlo inmediatamente
 * (builtin_throw ya copia el texto al objeto de la excepción). */
static const char* recurso_no_esta(const char* path) {
    static char msg[400];
    /* %.200s: el path puede venir de un buffer mayor que éste. Se acota aquí y
     * a la vista —y el buffer da de sobra para el texto fijo—, en vez de dejar
     * que el truncado ocurra por accidente. */
    snprintf(msg, sizeof msg,
             "el recurso '%.200s' no esta: o ese pack no esta grabado en la placa,"
             " o no lleva ese fichero", path ? path : "");
    return msg;
}

static bpvm_status_t builtin_throw_clase(bpvm_t* vm, bpvm_thread_t* tc, const char* clase, const char* msg);

static bpvm_status_t builtin_throw(bpvm_t* vm, bpvm_thread_t* tc, const char* msg) {
    return builtin_throw_clase(vm, tc, "Core.RuntimeError", msg);
}

/* V8/H5 (#711): un EncodingError (lo de fuera que no es UTF-8 válido, un appendChar que no es un carácter). */
static bpvm_status_t builtin_throw_codif(bpvm_t* vm, bpvm_thread_t* tc, const char* msg) {
    return builtin_throw_clase(vm, tc, "Core.EncodingError", msg);
}

/* V8/H5 (#711): el texto del error de descodificar, el mismo que miVM (Utf8.mensaje): «<que>: UTF-8 mal formado en el
 * byte 5 (0xF1)». */
static bpvm_status_t throw_utf8_malo(bpvm_t* vm, bpvm_thread_t* tc, const char* que, int32_t pos, uint8_t byte_malo) {
    char em[640];
    snprintf(em, sizeof em, "%s: UTF-8 mal formado en el byte %" PRId32 " (0x%02X)", que, pos, (unsigned) byte_malo);
    return builtin_throw_codif(vm, tc, em);
}

/* V8 #716 — QUÉ ES UN NÚMERO para parseInt y parseFloat, lo mismo en las dos VMs (Eduardo: «Me valen las propuestas»).
 * Antes, strtol y strtod decidían, y leen lo que pueden y callan ("12abc" → 12, "abc" → 0, "0x1F" → 31 en parseFloat; un
 * entero grande, el tope), y cambian entre MinGW, newlib y picolibc; miVM mataba el hilo. La regla, sobre los BYTES de
 * la cadena (un dígito es 0-9 de ASCII):
 *   - alrededor, espacios: los bytes de 0 a 32, como el trim de Java; dentro, nada;
 *   - entero:  [+-] dígitos, y que quepa en 32 bits;
 *   - decimal: [+-] (dígitos [. dígitos] | . dígitos) [(e|E) [+-] dígitos] (sin hexadecimal, inf ni nan), y que quepa
 *     en un float.
 * Lo que no, un RuntimeError atrapable con el texto que llegó (40 bytes como mucho, cortado por caracteres). El espejo,
 * Numeros (miVM). Para convertir un decimal ya validado, strtod sigue sirviendo: con uno limpio, da lo mismo que
 * Double.parseDouble. */
enum { NUM_VALE = 0, NUM_NO_ES = 1, NUM_NO_CABE = 2 };

static uint32_t num_ini(const uint8_t* p, uint32_t n) {
    uint32_t i = 0;
    while (i < n && p[i] <= 32u) i++;
    return i;
}
static uint32_t num_fin(const uint8_t* p, uint32_t a, uint32_t n) {
    uint32_t f = n;
    while (f > a && p[f - 1] <= 32u) f--;
    return f;
}
static int num_digito(uint8_t c) { return c >= '0' && c <= '9'; }

/* El entero de p[a, b): NUM_VALE (con *v), NUM_NO_ES o NUM_NO_CABE. Mira todos los bytes antes de decir NUM_NO_CABE. */
static int num_entero(const uint8_t* p, uint32_t a, uint32_t b, int32_t* v) {
    uint32_t i = a;
    int neg = 0, grande = 0;
    uint64_t acc = 0;
    if (i < b && (p[i] == '+' || p[i] == '-')) { neg = (p[i] == '-'); i++; }
    if (i >= b) return NUM_NO_ES;
    for (; i < b; i++) {
        if (!num_digito(p[i])) return NUM_NO_ES;
        if (!grande) {
            acc = acc * 10u + (uint64_t) (p[i] - '0');
            if (acc > 2147483648ull) grande = 1;
        }
    }
    if (grande || (!neg && acc > 2147483647ull)) return NUM_NO_CABE;
    *v = neg ? (int32_t) (0 - (int64_t) acc) : (int32_t) acc;
    return NUM_VALE;
}

/* ¿Es p[a, b) un decimal? [+-] (dígitos [. dígitos] | . dígitos) [(e|E) [+-] dígitos]. */
static int num_es_decimal(const uint8_t* p, uint32_t a, uint32_t b) {
    uint32_t i = a, ent = 0, frac = 0, exp = 0;
    if (i < b && (p[i] == '+' || p[i] == '-')) i++;
    while (i < b && num_digito(p[i])) { i++; ent++; }
    if (i < b && p[i] == '.') {
        i++;
        while (i < b && num_digito(p[i])) { i++; frac++; }
    }
    if (ent == 0 && frac == 0) return 0;
    if (i < b && (p[i] == 'e' || p[i] == 'E')) {
        i++;
        if (i < b && (p[i] == '+' || p[i] == '-')) i++;
        while (i < b && num_digito(p[i])) { i++; exp++; }
        if (exp == 0) return 0;
    }
    return i == b;
}

/* El texto del error: «<que>: '<lo que llegó>' <qué le pasa>», con 40 bytes como mucho de lo que llegó, cortado por
 * caracteres y con «...». */
static bpvm_status_t num_throw(bpvm_t* vm, bpvm_thread_t* tc, const char* que, const uint8_t* p, uint32_t n,
                               const char* falla) {
    uint32_t k = (n <= 40u) ? n : 40u;
    if (n > 40u) while (k > 0u && (p[k] & 0xC0u) == 0x80u) k--;
    char em[160];
    snprintf(em, sizeof em, "%s: '%.*s%s' %s", que, (int) k, (const char*) p, (n > 40u) ? "..." : "", falla);
    return builtin_throw(vm, tc, em);
}

/* V8/H5 (#711) — el bloque `blk` (un byte[] recién leído de FUERA: un fichero, una línea; no está en la pila) pasa a
 * ser una cadena. Si es UTF-8 válido, nace tal cual, en su sitio; si no, EncodingError, o, con `ignorar`, nace otra con
 * cada trozo malo cambiado por U+FFFD (más larga: se mide antes de reservar). Deja la cadena en la pila. El espejo,
 * nacerDeFuera de miVM. */
static bpvm_status_t nacer_de_fuera(bpvm_t* vm, bpvm_thread_t* tc, bpref_t blk, int ignorar, const char* que) {
    uint32_t a = bpref_deref(vm, blk);
    uint32_t n = bpvm_read_u32_be(vm->memory + a);
    int32_t malo = utf8_primer_malo(vm->memory + a + 4u, n);
    if (malo < 0) {
        bpvm_str_nace(vm, a);
        bpref_push(vm, tc, blk);
        return BPVM_OK;
    }
    if (!ignorar) return throw_utf8_malo(vm, tc, que, malo, vm->memory[a + 4u + (uint32_t) malo]);
    uint32_t m = utf8_largo_corregido(vm->memory + a + 4u, n);
    bpref_push(vm, tc, blk);   /* en la pila: vivo mientras se reserva el otro */
    bpref_t outh = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, m, m);
    if (outh.v == 0u) {
        tc->sp -= BPVM_REF_SIZE;
        return builtin_throw(vm, tc, "No space in heap");   /* H1: OOM atrapable */
    }
    a = bpref_deref(vm, blk);   /* otra vez, tras la reserva */
    uint32_t o = bpref_deref(vm, outh);
    utf8_corrige(vm->memory + a + 4u, n, vm->memory + o + 4u);
    bpvm_str_nace_ascii(vm, o, 0);   /* lleva al menos un U+FFFD: no es ASCII */
    tc->sp -= BPVM_REF_SIZE;         /* el leído, fuera */
    bpref_push(vm, tc, outh);
    return BPVM_OK;
}

static bpvm_status_t builtin_throw_clase(bpvm_t* vm, bpvm_thread_t* tc, const char* clase, const char* msg) {
    /* V7 #659: los cinco primeros de ESTE RUN (era un `static`, de proceso), sin perder cuentas con dos runs. */
    int hablar = vm->avisos_throw < 5 && __atomic_fetch_add(&vm->avisos_throw, 1, __ATOMIC_RELAXED) < 5;
    if (hablar) bpvm_diag("[bpvm] throw: \"%s\"", msg ? msg : "");

    bpref_t ref = bpvm_throw_error_clase(vm, tc, clase, msg);
    if (bpref_is_null(ref)) {
        if (hablar) bpvm_diag_urgente("[bpvm] throw: NO SE PUDO CONSTRUIR la excepcion "
                              "(sin memoria para el mensaje o el objeto, o sin "
                              "clase RuntimeError exportada) -> el programa NO se entera");
        return BPVM_ERR_RUNTIME;
    }
    /* V7/H5 P2b.2 — llamado DESDE UNA NATIVE (call_builtin): los marcos de C de la native siguen vivos, así
     * que aquí no se deshace nada. La excepción se deja en el hilo y call_builtin la lanza por el límite de
     * fallos de #186, que la propaga desde el lado BP como cualquier throw de una native. */
    if (tc->builtin_de_nativa) {
        tc->excepcion_pendiente = (uint32_t) ref.v;
        return BPVM_ERR_RUNTIME;
    }
    if (bpvm_eh_unwind(vm, tc, ref)) {
        if (hablar) bpvm_diag("[bpvm] throw: ATRAPADA por un catch del programa");
        return BPVM_OK;
    }
    if (hablar) bpvm_diag("[bpvm] throw: construida pero SIN handler -> el RUN termina en error");
    return BPVM_ERR_RUNTIME;
}

#ifdef BPVM_GUI
/* V3 Forms — Crea un widget hijo: saca `parent` de la pila, valida que es un
 * contenedor vivo (si no, lanza RuntimeError "widget sin contenedor") y empuja el
 * handle del nuevo nodo. `mk` = la bpvm_gui_create_* del modelo. La regla y el
 * mensaje se replican en miVM (GuiBackend) -> paridad de comportamiento. */
/* #568 — la tabla de widgets del modelo, LLENA. Hasta el 25-sep el 0 que devolvía el modelo se metía en la
 * pila como un handle más y el widget simplemente no aparecía, sin una palabra. Ahora el modelo (gui.c, en
 * la imagen o en el pack) da su capacidad EN NEGATIVO; un pack anterior da 0, y se dice sin el número. miVM
 * no tiene tope: es un límite de la VM-C —256 en el pack para pantallas pequeñas, 512 en el resto—, como
 * su heap. */
static bpvm_status_t gui_sin_sitio(bpvm_t* vm, bpvm_thread_t* tc, int h) {
    char m[96];
    if (h < 0) snprintf(m, sizeof m, "Gui: no caben mas widgets: esta GUI tiene sitio para %d", -h);
    else       snprintf(m, sizeof m, "Gui: no caben mas widgets en esta GUI");
    return builtin_throw(vm, tc, m);
}

static bpvm_status_t gui_make_child(bpvm_t* vm, bpvm_thread_t* tc, int (*mk)(int)) {
    int parent = pop_i32(vm, tc);
    if (!bpvm_gui_parent_alive(parent))
        return builtin_throw(vm, tc,
            "Gui: no se puede crear un widget sin un contenedor valido; crea Gui.Screen() o Gui.Window() primero");
    int h = mk(parent);
    if (h <= 0) return gui_sin_sitio(vm, tc, h);
    push_i32(vm, tc, h);
    return BPVM_OK;
}
#endif /* BPVM_GUI */

/* H13 (V3) — Forms: resuelve una función PÚBLICA por nombre SIMPLE en el módulo
 * dueño del objeto `host` (su class_ptr vive en el data block de ese módulo).
 * Espejo de invokeHandlerByName de miVM (getCSForDataAddr + resolveExportInModule).
 * Devuelve la dirección absoluta o 0 si no existe. */
static uint32_t bpvm_resolve_handler(bpvm_t* vm, uint32_t host, const char* simple) {
    if (host == 0) return 0;
    uint32_t ha = bpref_deref(vm, bpref_from_addr(host));   /* V4: handle→addr */
    uint32_t class_ptr = (uint32_t) bpvm_read_i32_be(vm->memory + ha);
    const bpvm_module_t* m = NULL;
    for (int i = 0; i < vm->module_count; i++) {
        const bpvm_module_t* mm = &vm->modules[i];
        if (class_ptr >= mm->data_start && class_ptr < mm->data_start + mm->data_size) {
            m = mm; break;
        }
    }
    if (m == NULL) return 0;
    char qual[200];
    if (m->library[0] != '\0')
        snprintf(qual, sizeof(qual), "%s.%s.%s", m->library, m->name, simple);
    else
        snprintf(qual, sizeof(qual), "%s.%s", m->name, simple);
    for (int i = 0; i < vm->symbol_count; i++)
        if (strcmp(bpvm_symbol_name(vm, i), qual) == 0) return vm->symbols[i].abs_addr;
    if (m->library[0] != '\0') {   /* fallback: clave corta name.simple */
        snprintf(qual, sizeof(qual), "%s.%s", m->name, simple);
        for (int i = 0; i < vm->symbol_count; i++)
            if (strcmp(bpvm_symbol_name(vm, i), qual) == 0) return vm->symbols[i].abs_addr;
    }
    return 0;
}

/* H7 — calculadora de constantes para eval(). Descenso recursivo que evalúa
 * SOBRE LA MARCHA (sin AST): + - * / paréntesis y unario sobre literales. Réplica
 * byte-a-byte del EvalCalc de miVM (VirtualMachine.java): mismas operaciones double
 * + parseo numérico MANUAL (no strtod) para garantizar paridad. Error -> NaN. */
typedef struct { const char* s; int pos; int len; int err; } evalcalc_t;
static double ec_expr(evalcalc_t* c);
static void ec_ws(evalcalc_t* c) {
    while (c->pos < c->len && (c->s[c->pos] == ' ' || c->s[c->pos] == '\t')) c->pos++;
}
static double ec_number(evalcalc_t* c) {
    ec_ws(c);
    double v = 0; int any = 0;
    while (c->pos < c->len && c->s[c->pos] >= '0' && c->s[c->pos] <= '9') { v = v * 10 + (c->s[c->pos] - '0'); c->pos++; any = 1; }
    if (c->pos < c->len && c->s[c->pos] == '.') {
        c->pos++; double sc = 1;
        while (c->pos < c->len && c->s[c->pos] >= '0' && c->s[c->pos] <= '9') { v = v * 10 + (c->s[c->pos] - '0'); sc *= 10; c->pos++; any = 1; }
        v = v / sc;
    }
    if (!any) c->err = 1;
    return v;
}
static double ec_factor(evalcalc_t* c) {
    ec_ws(c);
    if (c->pos >= c->len) { c->err = 1; return 0; }
    char ch = c->s[c->pos];
    if (ch == '-') { c->pos++; return -ec_factor(c); }
    if (ch == '+') { c->pos++; return ec_factor(c); }
    if (ch == '(') {
        c->pos++; double v = ec_expr(c); ec_ws(c);
        if (c->pos < c->len && c->s[c->pos] == ')') c->pos++; else c->err = 1;
        return v;
    }
    return ec_number(c);
}
static double ec_term(evalcalc_t* c) {
    double v = ec_factor(c); ec_ws(c);
    while (c->pos < c->len && (c->s[c->pos] == '*' || c->s[c->pos] == '/')) {
        char op = c->s[c->pos++]; double r = ec_factor(c); v = (op == '*') ? v * r : v / r; ec_ws(c);
    }
    return v;
}
static double ec_expr(evalcalc_t* c) {
    double v = ec_term(c); ec_ws(c);
    while (c->pos < c->len && (c->s[c->pos] == '+' || c->s[c->pos] == '-')) {
        char op = c->s[c->pos++]; double r = ec_term(c); v = (op == '+') ? v + r : v - r; ec_ws(c);
    }
    return v;
}
static double bpvm_eval_calc(const char* s, int len) {
    evalcalc_t c; c.s = s; c.pos = 0; c.len = len; c.err = 0;
    double v = ec_expr(&c); ec_ws(&c);
    if (c.pos != c.len) c.err = 1;
    return c.err ? NAN : v;
}

/* ── #534 (V7, 23-sep) — el `count` de los builtins de bus, IGUAL en las dos VMs. Espejo de
 * busArray y de los bucles de tramos de miVM: los MISMOS textos y los MISMOS tramos.
 *
 * Antes cada builtin copiaba a un buffer de pila con `n = count > sizeof(buf) ? sizeof(buf) :
 * count`: RECORTABA en silencio (a 64 en I2C, a 256 en SPI y UART, donde miVM hacía `count`
 * entero: el stdout de las dos VMs era distinto); con un count mayor que el array leía y escribía
 * FUERA de él; con count < 0 el `(size_t) n` se salía del buffer de pila (segfault en el host); y
 * con el array null mandaba basura de vm->memory[4..], o la ESCRIBÍA ahí, donde miVM lanzaba.
 *
 * Decisión de Eduardo (23-sep):
 *  - array null, count < 0 o count > longitud → RuntimeError atrapable;
 *  - SPI y UART sin tope, en TRAMOS de BUS_TRAMO bytes, una llamada a la fachada por tramo (en el
 *    cable no se nota: el CS lo lleva el programa y la UART es un flujo). El buffer sigue siendo
 *    fijo y de pila: ni reserva que pueda fallar (#338) ni tope;
 *  - I2C es UNA transacción (START…STOP): partirla haría que el dispositivo tomara el segundo
 *    trozo por una escritura nueva. No se parte; tope de BUS_TRAMO por llamada, con error.
 *
 * Los buffers los declara cada `case`, como antes, y los helpers los reciben: así viven en el
 * marco de bpvm_call_builtin, que ya los tenía, y la pila no crece. Medido en ARM con los flags del
 * firmware y -fstack-usage: con los buffers DENTRO de bus_por_tramos, 568 B más en cada llamada
 * SPI/UART, encima de los 2072 de bpvm_call_builtin; y bus_i2c sólo cabía gratis porque gcc lo
 * inlinaba, que a -O0 no pasa. */
enum { BUS_TRAMO = 256 };   /* CONTRATO DE PARIDAD con VirtualMachine.BUS_TRAMO (miVM) */

/* Valida el array de un builtin de bus: 0 y su dirección física en *addr, o -1 y el texto del
 * RuntimeError en em. `cual`: "el array", "el array tx" o "el array rx"; `tope`: 0 = sin tope
 * (SPI/UART), BUS_TRAMO en I2C. El orden de las comprobaciones es parte del contrato. */
static int bus_array(bpvm_t* vm, uint32_t ref, int32_t count, const char* op, const char* cual,
                     int32_t tope, uint32_t* addr, char* em, size_t emsz) {
    if (ref == 0) { snprintf(em, emsz, "%s: %s es null", op, cual); return -1; }
    if (count < 0) { snprintf(em, emsz, "%s: count negativo (%" PRId32 ")", op, count); return -1; }
    uint32_t a = ref_addr(vm, ref);
    uint32_t len = bpvm_read_u32_be(vm->memory + a);
    if ((uint32_t) count > len) {
        snprintf(em, emsz, "%s: count (%" PRId32 ") mayor que %s (%" PRIu32 ")", op, count, cual, len);
        return -1;
    }
    if (tope > 0 && count > tope) {
        snprintf(em, emsz, "%s: count (%" PRId32 ") supera el máximo de %" PRId32
                 " bytes por transacción", op, count, tope);
        return -1;
    }
    *addr = a;
    return 0;
}

/* Layout del array (TYPE_ARRAY_I32): [0..3] longitud, [4..] un i32 BE por elemento, y cada
 * elemento transporta un byte en el byte bajo. Estos dos pasan `n` elementos desde `desde`. */
static void bus_a_bytes(bpvm_t* vm, uint32_t addr, int32_t desde, int n, uint8_t* buf) {
    const uint8_t* p = vm->memory + addr + 4u + (uint32_t) desde * 4u;
    for (int i = 0; i < n; i++) buf[i] = (uint8_t) (bpvm_read_i32_be(p + (uint32_t) i * 4u) & 0xFF);
}
static void bus_de_bytes(bpvm_t* vm, uint32_t addr, int32_t desde, int n, const uint8_t* buf) {
    uint8_t* p = vm->memory + addr + 4u + (uint32_t) desde * 4u;
    for (int i = 0; i < n; i++) bpvm_write_i32_be(p + (uint32_t) i * 4u, (int32_t) buf[i]);
}

typedef enum { BUS_SPI_WRITE, BUS_SPI_READ, BUS_SPI_TRANSFER, BUS_UART_WRITE, BUS_UART_READ } bus_verbo_t;

/* SPI y UART: `count` bytes (ya validados) en tramos de BUS_TRAMO, una llamada a la fachada por
 * tramo. Devuelve el total, o el error del driver (< 0) si un tramo falla. Un tramo CORTO (el
 * driver dio menos de lo pedido: en Uart.read, venció el plazo) termina: se devuelve lo hecho.
 * En Uart.read el plazo es POR TRAMO: cada tramo recibe el timeout del programa y no «lo que
 * queda», que dependería del reloj y rompería la paridad del host. El do-while conserva
 * count 0 = una llamada vacía, como antes. `tx`/`rx`: BUS_TRAMO bytes del `case` (el que el
 * verbo no usa puede ser NULL). */
static int32_t bus_por_tramos(bpvm_t* vm, bus_verbo_t v, int bus, uint32_t tx_addr,
                              uint32_t rx_addr, int32_t count, int timeout,
                              uint8_t* tx, uint8_t* rx) {
    int escribe = (v == BUS_SPI_WRITE || v == BUS_SPI_TRANSFER || v == BUS_UART_WRITE);
    int lee     = (v == BUS_SPI_READ  || v == BUS_SPI_TRANSFER || v == BUS_UART_READ);
    /* V7/H6.c G4.a2 — el SPI se COMPARTE (en la C5, con la pantalla y la TF): la llamada
     * ENTERA es un turno del árbitro, con todos sus tramos dentro. Un turno por tramo dejaría
     * colarse a otro dispositivo entre dos tramos con el CS del programa abajo. */
    int spi = (v == BUS_SPI_WRITE || v == BUS_SPI_READ || v == BUS_SPI_TRANSFER);
    int32_t hecho = 0;
    if (spi && bpvm_spi_tomar_como(BPVM_BUS_PROGRAMA, bus) != 0) return -1;
    do {
        int n = (count - hecho > BUS_TRAMO) ? BUS_TRAMO : (int) (count - hecho);
        if (escribe) bus_a_bytes(vm, tx_addr, hecho, n, tx);   /* ANTES de leer: tx y rx pueden ser el mismo array */
        int r = -1;
        switch (v) {
            case BUS_SPI_WRITE:    r = bpvm_spi_xfer(bus, tx, NULL, (size_t) n); break;
            case BUS_SPI_READ:     r = bpvm_spi_xfer(bus, NULL, rx, (size_t) n); break;
            case BUS_SPI_TRANSFER: r = bpvm_spi_xfer(bus, tx, rx, (size_t) n); break;
            case BUS_UART_WRITE:   r = bpvm_uart_write(bus, tx, (size_t) n); break;
            case BUS_UART_READ:    r = bpvm_uart_read(bus, rx, (size_t) n, timeout); break;
        }
        if (r < 0) { hecho = r; break; }   /* el error del driver, tras soltar el turno */
        if (r > n) r = n;   /* un driver no puede dar más de lo pedido: nunca se escribe fuera */
        if (lee) bus_de_bytes(vm, rx_addr, hecho, r, rx);
        hecho += r;
        if (r < n) break;
    } while (hecho < count);
    if (spi) bpvm_spi_dejar(bus);
    return hecho;
}

/* I2C: la transacción entera en UNA llamada (count ya validado contra el tope BUS_TRAMO).
 * `buf`: BUS_TRAMO bytes del `case`. */
static int32_t bus_i2c(bpvm_t* vm, int escribe, int bus, int addr, uint32_t data_addr, int32_t count,
                       uint8_t* buf) {
    if (escribe) {
        bus_a_bytes(vm, data_addr, 0, (int) count, buf);
        return bpvm_i2c_write(bus, addr, buf, (size_t) count);
    }
    int got = bpvm_i2c_read(bus, addr, buf, (size_t) count);
    if (got > count) got = (int) count;
    if (got > 0) bus_de_bytes(vm, data_addr, 0, got, buf);
    return got;
}

/* ── V7/H6.c G4.a3 — el DISPOSITIVO SPI y las TRANSACCIONES desde BP (V7_IDEAS §H6.11.9).
 * Espejo de miVM: los MISMOS textos, el MISMO orden de comprobaciones y los MISMOS tramos.
 *
 * Cada llamada es UNA transacción: se valida, se calcula contra el tope del bus (fuera de él,
 * RuntimeError y nada al cable), se toma UN turno con el CS del dispositivo y su velocidad, y
 * dentro sólo hay transferencias —en tramos de BUS_TRAMO, porque el array de BP lleva un byte
 * por slot—. Lo que no deja empezar es un RuntimeError; una cintura que falla, -1. */
enum { DEV_WRITE = 1, DEV_READ = 2, DEV_TRANSFER = 3, DEV_WRITE_READ = 4 };
static const char* dev_nombre(int op) {
    return op == DEV_WRITE ? "spi.write" : op == DEV_READ ? "spi.read"
         : op == DEV_TRANSFER ? "spi.transfer" : "spi.writeRead";
}

/* El dispositivo puede transferir: su bus existe y está abierto, y su CS sigue siendo una
 * salida del programa (tras release(), no). */
static int dev_listo(int bus, int cs, const char* op, char* em, size_t cap) {
    if (bpvm_gpio_instancia_ok(2, bus, em, cap) != 0) return -1;
    if (cs < 0 || bpvm_gpio_estado(cs) != BPVM_GPIO_OUTPUT) {
        snprintf(em, cap, "%s: el CS %d no está reservado como salida (¿release()?)", op, cs);
        return -1;
    }
    if (!bpvm_bus_abierto(BPVM_BUS_SPI, bus)) { snprintf(em, cap, "%s: el SPI %d no está abierto", op, bus); return -1; }
    return 0;
}

/* `n` bytes de una operación, en tramos, DENTRO del turno ya tomado: escribe desde
 * tx_addr[tx_desde..] y/o lee a rx_addr[rx_desde..]. Una operación de 0 bytes no manda nada.
 * 0, o -1 si la cintura falla. `tx`/`rx`: BUS_TRAMO bytes del `case`. */
static int dev_tramos(bpvm_t* vm, int bus, int escribe, int lee, uint32_t tx_addr, int32_t tx_desde,
                      uint32_t rx_addr, int32_t rx_desde, int32_t n, uint8_t* tx, uint8_t* rx) {
    int32_t hecho = 0;
    while (hecho < n) {
        int t = (n - hecho > BUS_TRAMO) ? BUS_TRAMO : (int) (n - hecho);
        if (escribe) bus_a_bytes(vm, tx_addr, tx_desde + hecho, t, tx);   /* ANTES de leer: pueden ser el mismo array */
        if (bpvm_spi_xfer(bus, escribe ? tx : NULL, lee ? rx : NULL, (size_t) t) < 0) return -1;
        if (lee) bus_de_bytes(vm, rx_addr, rx_desde + hecho, t, rx);
        hecho += t;
    }
    return 0;
}

/* El elemento i de un array de enteros de BP. */
static int32_t arr_i32(bpvm_t* vm, uint32_t addr, int32_t i) {
    return bpvm_read_i32_be(vm->memory + addr + 4u + (uint32_t) i * 4u);
}

/* #535 — bytes por elemento de un bloque de array del heap, o 0 si el tipo no es de array.
 * Los mismos tamaños que `block_total_size` (heap.c) y que `elemSize` de miVM. */
static uint32_t move_elem(int tipo) {
    switch (tipo) {
    case BPVM_TYPE_ARRAY_I8:  return 1u;
    case BPVM_TYPE_ARRAY_I16: return 2u;
    case BPVM_TYPE_ARRAY_I32: return 4u;
    case BPVM_TYPE_ARRAY_I64: return 8u;   /* long[] y double[] (H1.2) */
    case BPVM_TYPE_ARRAY_REF: return 8u;   /* V4: la ref plana mide 8 B */
    default:                  return 0u;
    }
}

/* ═══ V7/H5 P2b (26-sep, la opción 3 de Eduardo) — LAS PRIMITIVAS COMPARTIDAS ═══════════════════════════
 *
 * Las usan los builtins de abajo, los opcodes NEWARRAY de interp.c y los helpers tipados del AOT
 * (bpvm_aot_helpers.c): UNA implementación, varias entradas. Ver bpvm_prims.h. */

/* ═══ V8/H5 (21-oct) — EL LENTO CON DOS CONTADORES, bytes y caracteres (Eduardo: «si después se hace un substring o
 * algo similar podemos optimizar si trabajamos con punteros de C o con dos contadores de bytes y de caracteres») ═══
 *
 * Una cadena con caracteres especiales va por el lento: los índices son caracteres y hay que contar. Hasta ahora cada
 * llamada contaba DESDE EL PRINCIPIO, a veces tres veces (substring: la cadena entera para el recorte, y luego hasta
 * `desde` y hasta `hasta`). Ahora:
 *   1. dentro de cada llamada, UNA pasada con los dos contadores a la vez —el carácter y el byte donde empieza—:
 *      substring camina hasta `desde`, y de ahí hasta `hasta`; charAt camina hasta `i`, y si la cadena se acaba
 *      antes, está fuera de rango;
 *   2. entre llamadas, el CURSOR del hilo (bpvm_strcur_t): la siguiente sobre la misma cadena sigue desde donde quedó,
 *      hacia delante o hacia atrás. Recorrer una cadena por índice deja de ser n².
 * Dónde empieza un carácter es lo de utf8_byte_offset —en el byte 0 y en todo byte que no es de continuación
 * (10xxxxxx)—, hacia delante y hacia atrás: el resultado es el de antes. Una cadena que empezara por un byte de
 * continuación (UTF-8 mal formado) contaría con utf8_cp_count un carácter menos que con esa definición. Tuvo un caso
 * aparte, el lento de siempre, mientras quedaba una puerta por la que nacer; desde V8 #739 (2-nov) no lo tiene: no nace
 * ninguna (#711, #714, #724, #763), y si naciera, lo diría su nacimiento (bpvm_str_nace_ascii). Con una así, el cursor
 * no se sale de la cadena (test-malnacida). Sin hilo (una native), un cursor propio que sólo dura la llamada.
 * Por qué el cursor no puede mentir: la clave es la referencia entera —el handle con su generación, que distingue una
 * cadena muerta de la que ocupa su sitio, o la dirección de un literal—; guarda posiciones desde el principio de la
 * cadena, no direcciones, así que la compactación no le estorba; y una cadena no cambia mientras existe, salvo por
 * `move`, que sube vm->str_epoca y los invalida todos. */

#define BP_CONT(b) (((b) & 0xC0u) == 0x80u)

/* ¿Sirve el cursor para esta cadena? La misma referencia entera y la misma época (ningún `move` desde entonces). La
 * época se lee con ACQUIRE, la mitad lectora de la publicación: si se ve la que subió un `move` (con RELEASE, después de
 * escribir), se ven también los bytes que escribió —con dos runs, en otro núcleo—. */
static int strcur_vale(const bpvm_t* vm, const bpvm_strcur_t* c, uint64_t clave) {
    return clave != 0u && c->ref == clave && c->epoca == __atomic_load_n(&vm->str_epoca, __ATOMIC_ACQUIRE);
}

/* Que el cursor sea de esta cadena: si era de otra (o de otra época), lo de antes se olvida. */
static void strcur_de(const bpvm_t* vm, bpvm_strcur_t* c, uint64_t clave) {
    if (strcur_vale(vm, c, clave)) return;
    c->ref = clave; c->epoca = __atomic_load_n(&vm->str_epoca, __ATOMIC_ACQUIRE);
    c->ci = 0u; c->bo = 0u; c->ncp = -1;
}

/* Cuántos caracteres tiene (utf8_cp_count): se cuenta una vez por cadena. */
static uint32_t strcur_ncp(const bpvm_t* vm, bpvm_strcur_t* c, uint64_t clave, const uint8_t* p, uint32_t n) {
    strcur_de(vm, c, clave);
    if (c->ncp < 0) c->ncp = (int32_t) utf8_cp_count(p, n);
    return (uint32_t) c->ncp;
}

/* El byte donde empieza el carácter `i`, caminando desde el punto conocido más cerca —el principio, el cursor (hacia
 * delante o hacia atrás) o, si se sabe cuántos caracteres tiene, el final (hacia atrás)—; el cursor queda ahí. Si la
 * cadena se acaba antes, devuelve n, y entonces ya se sabe cuántos caracteres tiene. */
static uint32_t strcur_byte_de(const bpvm_t* vm, bpvm_strcur_t* c, uint64_t clave, const uint8_t* p, uint32_t n,
                               uint32_t i) {
    uint32_t ci = 0u, bo = 0u, pasos = i;                                  /* desde el principio, i pasos */
    uint32_t dcur;
    strcur_de(vm, c, clave);
    if (c->ncp >= 0 && i >= (uint32_t) c->ncp) return n;                   /* ya se sabe: no hay carácter i */
    dcur = (i >= c->ci) ? i - c->ci : c->ci - i;
    if (dcur < pasos) { ci = c->ci; bo = c->bo; pasos = dcur; }            /* más cerca el cursor */
    if (c->ncp >= 0 && (uint32_t) c->ncp - i < pasos) { ci = (uint32_t) c->ncp; bo = n; }   /* o el final */
    if (i >= ci) {
        while (ci < i && bo < n) {
            bo++;
            while (bo < n && BP_CONT(p[bo])) bo++;
            ci++;
        }
    } else {
        while (ci > i) {
            bo--;
            while (bo > 0u && BP_CONT(p[bo])) bo--;
            ci--;
        }
    }
    c->ci = ci;
    c->bo = bo;
    if (bo >= n) { c->ncp = (int32_t) ci; return n; }                      /* se acabó: tiene ci caracteres */
    return bo;
}

int32_t bpvm_prim_str_len(bpvm_t* vm, bpvm_strcur_t* cur, uint64_t clave, uint32_t s) {
    bpvm_strcur_t propio = {0};
    if (s == 0u) return 0;
    uint32_t nbytes = bpvm_read_u32_be(vm->memory + s);
    if (bpvm_str_ascii(vm, s)) return (int32_t) nbytes;   /* V8/H5: el rápido, un carácter = un byte */
    if (cur == NULL) { cur = &propio; clave = s; }
    return (int32_t) strcur_ncp(vm, cur, clave, vm->memory + s + BPVM_ARR_DATA_OFF, nbytes);   /* una vez por cadena */
}

/* V8 #717: fuera de rango devuelve -1 (ningún carácter lo es), y quien llama lanza el error de charAt; antes, 0 en silencio. */
int32_t bpvm_prim_char_code_at(bpvm_t* vm, bpvm_strcur_t* cur, uint64_t clave, uint32_t s, int32_t i) {
    bpvm_strcur_t propio = {0};
    uint32_t nbytes, off, adv;
    const uint8_t* p;
    if (s == 0u) return -1;   /* sin valor: la vacía */
    nbytes = bpvm_read_u32_be(vm->memory + s);
    p = vm->memory + s + BPVM_ARR_DATA_OFF;
    if (bpvm_str_ascii(vm, s))   /* V8/H5: el rápido: el carácter i ES el byte i (fuera de rango, -1, como abajo) */
        return (i < 0 || (uint32_t) i >= nbytes) ? -1 : (int32_t) p[i];
    if (i < 0) return -1;
    if (cur == NULL) { cur = &propio; clave = s; }
    off = strcur_byte_de(vm, cur, clave, p, nbytes, (uint32_t) i);   /* V8/H5: desde el cursor, una pasada */
    if (off >= nbytes) return -1;                                      /* fuera de rango */
    return (int32_t) utf8_decode(p + off, nbytes - off, &adv);
}

int bpvm_prim_char_at(bpvm_t* vm, bpvm_strcur_t* cur, uint64_t clave, uint32_t s, int32_t i,
                      bpref_t* out, char* err, size_t n) {
    bpvm_strcur_t propio = {0};
    uint8_t enc[4];
    uint32_t enc_len = 0, o, k;
    /* #684 (11-oct) — un string SIN VALOR (una global o un campo sin inicializar: `s == 0`) es el vacío, como en
     * miVM (`readVmString(0)` es ""): su índice está fuera de rango igual que el de "". Aquí se saltaba la
     * comprobación y devolvía "" —el programa seguía en la VM-C y lanzaba en miVM—. */
    uint32_t nbytes = s ? bpvm_read_u32_be(vm->memory + s) : 0u;
    const uint8_t* p = s ? vm->memory + s + BPVM_ARR_DATA_OFF : vm->memory;
    uint32_t off, adv;
    if (cur == NULL) { cur = &propio; clave = s; }
    if (bpvm_str_ascii(vm, s)) {   /* V8/H5: el rápido: el carácter i ES el byte i */
        if (i < 0 || (uint32_t) i >= nbytes) {
            /* BUG-7b — idx fuera de rango → RuntimeError atrapable (paridad VM-Java). */
            snprintf(err, n, "charAt: idx fuera de rango %" PRId32 " (len=%d)", i, (int) nbytes);
            return -1;
        }
        enc[0] = p[i];
        enc_len = 1;
    } else {   /* V8/H5: el lento con los dos contadores, desde el cursor */
        off = (i < 0) ? nbytes : strcur_byte_de(vm, cur, clave, p, nbytes, (uint32_t) i);
        if (off >= nbytes) {
            snprintf(err, n, "charAt: idx fuera de rango %" PRId32 " (len=%d)", i,
                     (int) strcur_ncp(vm, cur, clave, p, nbytes));
            return -1;
        }
        enc_len = utf8_encode(utf8_decode(p + off, nbytes - off, &adv), enc);   /* antes de reservar */
    }
    *out = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, enc_len, enc_len); /* V7/H7 #593: la cabecera y el handle, al nacer */
    if (out->v == 0u) { snprintf(err, n, "No space in heap"); return -1; }   /* #355, #430 */
    o = bpref_deref(vm, *out);
    for (k = 0; k < enc_len; k++) vm->memory[o + BPVM_ARR_DATA_OFF + k] = enc[k];
    bpvm_str_nace(vm, o);   /* V8/H5: el bit, por el carácter que lleva */
    return 0;
}

int32_t bpvm_prim_str_cuenta_hasta(bpvm_t* vm, bpvm_strcur_t* cur, uint64_t clave, uint32_t s, uint32_t off) {
    bpvm_strcur_t propio = {0};
    const uint8_t* p = vm->memory + s + BPVM_ARR_DATA_OFF;
    uint32_t nbytes = bpvm_read_u32_be(vm->memory + s);
    uint32_t ci, dcur;
    if (off == 0u) return 0;
    if (cur == NULL) { cur = &propio; clave = s; }
    strcur_de(vm, cur, clave);
    /* Contar los caracteres de [0, off) desde el punto conocido más cerca, en bytes: el principio, el cursor o el final. */
    dcur = (cur->bo <= off) ? off - cur->bo : cur->bo - off;
    if (cur->ncp >= 0 && nbytes - off < dcur && nbytes - off < off)
        ci = (uint32_t) cur->ncp - utf8_cp_count(p + off, nbytes - off);           /* desde el final */
    else if (dcur < off)
        ci = (cur->bo <= off) ? cur->ci + utf8_cp_count(p + cur->bo, off - cur->bo)
                              : cur->ci - utf8_cp_count(p + off, cur->bo - off);    /* desde el cursor */
    else
        ci = utf8_cp_count(p, off);                                                 /* desde el principio */
    if (off < nbytes && !BP_CONT(p[off])) { cur->ci = ci; cur->bo = off; }   /* `off` empieza un carácter: ahí queda */
    return (int32_t) ci;
}

int bpvm_prim_substring(bpvm_t* vm, bpvm_strcur_t* cur, bpref_t s, int32_t desde, int32_t hasta,
                        bpref_t* out, char* err, size_t n) {
    bpvm_strcur_t propio = {0};
    uint32_t a = bpref_deref(vm, s);
    uint32_t nbytes = a ? bpvm_read_u32_be(vm->memory + a) : 0u;
    const uint8_t* p = vm->memory + a + BPVM_ARR_DATA_OFF;
    int rapido = bpvm_str_ascii(vm, a);   /* V8/H5: el rápido: los índices SON los bytes */
    uint64_t clave = s.v;
    uint32_t boff, eoff, len, o;
    if (cur == NULL) { cur = &propio; clave = a; }
    if (desde < 0) desde = 0;
    if (hasta < 0) hasta = 0;
    if (rapido) {
        if ((uint32_t) hasta > nbytes) hasta = (int32_t) nbytes;
        if (desde > hasta) desde = hasta;
        boff = (uint32_t) desde;
        eoff = (uint32_t) hasta;
    } else if (desde > hasta) {   /* `desde` se recorta a `hasta` (recortado): vacía */
        boff = eoff = 0u;
    } else {   /* V8/H5: una pasada con los dos contadores: hasta `desde`, y de ahí hasta `hasta` (pasado el final, el final) */
        uint32_t ci0, bo0, d0;
        strcur_de(vm, cur, clave);
        ci0 = cur->ci;   /* el cursor de antes de la llamada: para `hasta` puede estar más cerca que `desde` */
        bo0 = cur->bo;   /* (un indexOf y luego substring(s, 0, i)) */
        boff = strcur_byte_de(vm, cur, clave, p, nbytes, (uint32_t) desde);
        if (boff >= nbytes) {
            eoff = nbytes;
        } else {
            d0 = ((uint32_t) hasta >= ci0) ? (uint32_t) hasta - ci0 : ci0 - (uint32_t) hasta;
            if (d0 < (uint32_t) (hasta - desde)) { cur->ci = ci0; cur->bo = bo0; }
            eoff = strcur_byte_de(vm, cur, clave, p, nbytes, (uint32_t) hasta);
        }
    }
    len = eoff - boff;
    *out = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, len, len); /* V7/H7 #593: la cabecera y el handle, al nacer */
    if (out->v == 0u) { snprintf(err, n, "No space in heap"); return -1; }   /* #355, #430 */
    o = bpref_deref(vm, *out);
    a = bpref_deref(vm, s);   /* otra vez, tras la reserva: el origen lo mantiene vivo quien llama (#350) */
    /* V8/H5: copiar es una copia de bytes en los dos caminos (Eduardo: «Copiar un string debería ser igual con caracteres
     * especiales»); lo que el lento hace distinto es situarse (contar caracteres), arriba. */
    if (len > 0u) memcpy(vm->memory + o + BPVM_ARR_DATA_OFF, vm->memory + a + BPVM_ARR_DATA_OFF + boff, len);
    if (rapido) bpvm_str_nace_ascii(vm, o, 1);   /* un trozo de una cadena ASCII es ASCII */
    else bpvm_str_nace(vm, o);                    /* uno de una cadena con especiales puede serlo: se mira */
    return 0;
}

int bpvm_prim_int_to_string(bpvm_t* vm, int32_t v, bpref_t* out, char* err, size_t n) {
    char buf[16];
    int k = bpvm_format_i32(buf, v);   /* V8 #734: antes, snprintf (vfprintf por la caché del XIP de la RP2350) */
    uint32_t r = bpvm_heap_alloc_string(vm, buf, (size_t) (k > 0 ? k : 0));
    if (r == 0u) { snprintf(err, n, "No space in heap"); return -1; }   /* #355 */
    *out = bpref_regen(vm, r);
    return 0;
}

int bpvm_prim_nuevo_array(bpvm_t* vm, int32_t size, uint32_t ancho, int tipo, const char* pre, const char* post,
                          bpref_t* out, char* err, size_t n) {
    if (size < 0) { snprintf(err, n, "%s%d%s", pre, (int) size, post); return -1; }   /* #570 */
    /* V8 #712: los bytes, en 64 bits. En 32, newIntArray(2^30 + 1) pedía 4·(2^30 + 1) = 2^32 + 4 → 4 B, con una longitud
     * de mil millones, y escribir en él pisaba lo de al lado. Lo que no cabe en la memoria de la VM entera no tiene sitio
     * (sin colectar: no cabría). */
    uint64_t bytes = (uint64_t) (uint32_t) size * ancho;
    if (bytes > (uint64_t) vm->memory_size) { snprintf(err, n, "No space in heap"); return -1; }
    *out = bpvm_heap_nuevo(vm, tipo, (uint32_t) size, (uint32_t) bytes);   /* V7/H7 #593; el payload, a cero */
    if (out->v == 0u) { snprintf(err, n, "No space in heap"); return -1; }   /* H1: OOM atrapable; #430 */
    return 0;
}

void bpvm_prim_texto_indice(char* err, size_t n, int op, int32_t idx, uint32_t len) {
    const char* nombre;
    int estrecho = 1;   /* los de 1 y 2 bytes dicen «idx … (len=…)»; los de 4 y 8, «índice … (length=…)» (miVM) */
    switch (op) {
    case OP_ALOAD:      nombre = "ALOAD";      estrecho = 0; break;
    case OP_ASTORE:     nombre = "ASTORE";     estrecho = 0; break;
    case OP_ALOAD_I64:  nombre = "ALOAD_I64";  estrecho = 0; break;
    case OP_ASTORE_I64: nombre = "ASTORE_I64"; estrecho = 0; break;
    case OP_ALOAD_I8:   nombre = "ALOAD_I8";   break;
    case OP_ALOAD_U8:   nombre = "ALOAD_U8";   break;
    case OP_ALOAD_I16:  nombre = "ALOAD_I16";  break;
    case OP_ALOAD_U16:  nombre = "ALOAD_U16";  break;
    case OP_ASTORE_I8:  nombre = "ASTORE_I8";  break;
    case OP_ASTORE_I16: nombre = "ASTORE_I16"; break;
    default:            nombre = "?";          estrecho = 0; break;
    }
    if (estrecho) snprintf(err, n, "%s: idx fuera de rango %" PRId32 " (len=%d)", nombre, idx, (int) len);
    else          snprintf(err, n, "%s: índice fuera de rango %" PRId32 " (length=%d)", nombre, idx, (int) len);
}

int bpvm_builtin_cede(int id) {
    switch (id) {
    case BUILTIN_SLEEP: case BUILTIN_SLEEP_SEC: case BUILTIN_YIELD:
    case BUILTIN_THREAD_JOIN: case BUILTIN_MUTEX_LOCK:
        return 1;
    default:
        return 0;
    }
}

/* V7/H12 — LOS BUILTINS DE LA GUI: los BUILTIN_GUI_* de builtin_ids.h (entran en gui.c, y por él en LVGL), que con
 * varios runs sólo ejecuta el run 0 (interp.c, OP_CALL_BUILTIN; docs/V7_IDEAS.md §H12 «LVGL con dos runs»). No son un
 * rango —entre ellos hay de Machine, de App, los eventos…—: la lista, por nombre. Uno nuevo de la GUI va aquí: test-cola
 * comprueba que están todos los de builtin_ids.h. */
int bpvm_builtin_de_la_gui(int id) {
    switch (id) {
    case BUILTIN_GUI_SCREEN_ACTIVE: case BUILTIN_GUI_CREATE_OBJ: case BUILTIN_GUI_CREATE_LABEL:
    case BUILTIN_GUI_CREATE_BUTTON: case BUILTIN_GUI_SET_TEXT: case BUILTIN_GUI_SET_WIDTH:
    case BUILTIN_GUI_SET_HEIGHT: case BUILTIN_GUI_ALIGN: case BUILTIN_GUI_SET_BG_COLOR:
    case BUILTIN_GUI_SET_TEXT_COLOR: case BUILTIN_GUI_SET_FONT: case BUILTIN_GUI_CLEAN: case BUILTIN_GUI_DELETE:
    case BUILTIN_GUI_SCREEN_LOAD: case BUILTIN_GUI_RUN: case BUILTIN_GUI_DUMP_TREE: case BUILTIN_GUI_BIND_CLICK:
    case BUILTIN_GUI_CLICK: case BUILTIN_GUI_SET_X: case BUILTIN_GUI_GET_X: case BUILTIN_GUI_SET_Y:
    case BUILTIN_GUI_GET_Y: case BUILTIN_GUI_GET_WIDTH: case BUILTIN_GUI_GET_HEIGHT: case BUILTIN_GUI_SET_SCROLL_DIR:
    case BUILTIN_GUI_GET_SCROLL_DIR: case BUILTIN_GUI_REFRESH: case BUILTIN_GUI_CREATE_CHECKBOX:
    case BUILTIN_GUI_SET_CHECKED: case BUILTIN_GUI_GET_CHECKED: case BUILTIN_GUI_CHANGE:
    case BUILTIN_GUI_CREATE_SWITCH: case BUILTIN_GUI_CREATE_SLIDER: case BUILTIN_GUI_CREATE_BAR:
    case BUILTIN_GUI_SET_VALUE: case BUILTIN_GUI_GET_VALUE: case BUILTIN_GUI_SET_RANGE:
    case BUILTIN_GUI_CREATE_SPINBOX: case BUILTIN_GUI_CREATE_LED: case BUILTIN_GUI_CREATE_DROPDOWN:
    case BUILTIN_GUI_SET_OPTIONS: case BUILTIN_GUI_CREATE_TEXTAREA: case BUILTIN_GUI_GET_TEXT:
    case BUILTIN_GUI_CREATE_LIST: case BUILTIN_GUI_CREATE_KEYBOARD: case BUILTIN_GUI_KEYBOARD_SET_TEXTAREA:
    case BUILTIN_GUI_CREATE_MSGBOX: case BUILTIN_GUI_SET_BUTTONS: case BUILTIN_GUI_CREATE_TABVIEW:
    case BUILTIN_GUI_TABVIEW_ADD_TAB: case BUILTIN_GUI_CREATE_TABLE: case BUILTIN_GUI_TABLE_SET_GRID:
    case BUILTIN_GUI_TABLE_SET_CELL: case BUILTIN_GUI_TABLE_GET_CELL: case BUILTIN_GUI_IMAGE_NEW:
    case BUILTIN_GUI_IMAGE_LOAD_FILE: case BUILTIN_GUI_IMAGE_WIDTH: case BUILTIN_GUI_IMAGE_HEIGHT:
    case BUILTIN_GUI_CREATE_IMAGE_VIEW: case BUILTIN_GUI_IMAGE_VIEW_SET_IMAGE: case BUILTIN_GUI_IMAGE_VIEW_REFRESH:
    case BUILTIN_GUI_SET_FONT_SIZE: case BUILTIN_GUI_GET_FONT_SIZE: case BUILTIN_GUI_TEXTAREA_SET_READONLY:
    case BUILTIN_GUI_TEXTAREA_GET_READONLY: case BUILTIN_GUI_INVOKE_BY_NAME: case BUILTIN_GUI_INVOKE_BY_SLOT:
    case BUILTIN_GUI_LOAD_FONT: case BUILTIN_GUI_SET_ROTATION: case BUILTIN_GUI_CREATE_CHART:
    case BUILTIN_GUI_CHART_SET_POINTS: case BUILTIN_GUI_CHART_ADD_SERIES: case BUILTIN_GUI_CHART_PUSH:
    case BUILTIN_GUI_CHART_SET_VALUE: case BUILTIN_GUI_CHART_SET_TYPE: case BUILTIN_GUI_RUN_ONCE:
    case BUILTIN_GUI_SLOT_OF: case BUILTIN_GUI_GET_ALIGN: case BUILTIN_GUI_GET_ALIGN_DX:
    case BUILTIN_GUI_GET_ALIGN_DY: case BUILTIN_GUI_GET_AUTH_WIDTH: case BUILTIN_GUI_GET_AUTH_HEIGHT:
    case BUILTIN_GUI_SHOT:
        return 1;
    default:
        return 0;
    }
}

bpvm_status_t bpvm_call_builtin(bpvm_t* vm, bpvm_thread_t* tc, int id) {
    /* V7/H12 — un builtin de la GUI FUERA del run 0 no debe pasar: el intérprete hace viajar al hilo (OP_CALL_BUILTIN).
     * Se cuenta —el fin del programa lo dice, y test-cola 18 lo mira— y, llamado desde una native (que no puede viajar:
     * sus marcos de C están debajo), es un error con nombre. Con un run (vm->smp NULL), nada. */
    if (vm->smp != NULL && tc->sched_owner != 0 && bpvm_builtin_de_la_gui(id)) {
        __atomic_fetch_add(&vm->smp->gui_fuera_del_run0, 1u, __ATOMIC_RELAXED);
        if (tc->builtin_de_nativa)
            return builtin_throw(vm, tc, "GUI: un builtin de la GUI desde una native, fuera del run 0 "
                                         "(con dos runs, todavia no)");
    }
    switch (id) {

    case BUILTIN_STRLEN: {
        bpref_t s = bpref_pop(vm, tc);   /* string ref */
        push_i32(vm, tc, bpvm_prim_str_len(vm, &tc->strcur, s.v, bpref_deref(vm, s)));   /* P2b: la primitiva compartida */
        return BPVM_OK;
    }

    /* V6/#412 — EL ARGUMENTO DE EJECUCION, SIEMPRE EN EL HEAP.
     *
     * Lo llama `__startup` pasandole el valor POR DEFECTO que declare el fuente.
     * Devuelve el argumento de ejecucion si lo hay, y si no una COPIA del
     * defecto — alojada en el heap igual. Asi `Main` recibe SIEMPRE una
     * referencia del heap, venga de donde venga.
     *
     * Lo que conserva, y por eso es seguro: sin argumento y sin defecto el
     * programa recibe "" en el heap en vez de "" en la zona de datos. Mismo
     * valor y mismo comportamiento observable — un programa que hoy funciona no
     * se entera. Lo que DESAPARECE es la asimetria: hoy el argumento horneado es
     * un literal de datos y uno de ejecucion seria del heap, las dos formas que
     * #389 tuvo que reconocer en el CHECKCAST. */
    case BUILTIN_RUN_ARG: {
        uint32_t ref = pop_ref(vm, tc);          /* el defecto que dice el fuente */
        char buf[BPVM_RUN_ARG_MAX + 1u];
        size_t n;
        if (vm->run_arg[0] != '\0') {
            n = strlen(vm->run_arg);             /* cabe: bpvm_set_run_arg no guarda uno más largo (#764) */
            memcpy(buf, vm->run_arg, n);
        } else {
            /* V8 #764 — el defecto de un .mod de antes de que el compilador lo mirase: si no cabe, un error con el texto
             * de la VM-Java, en vez de cortarlo (read_bp_string cortaría por caracteres, y sin decirlo). */
            uint32_t nd = (ref == 0u) ? 0u : bpref_arr_len(vm, bpref_from_addr(ref));
            if (nd > BPVM_RUN_ARG_MAX) {
                char m[96];
                snprintf(m, sizeof m, "el valor por defecto del argumento no cabe: %u bytes (el tope es %u)",
                         (unsigned) nd, (unsigned) BPVM_RUN_ARG_MAX);
                return builtin_throw(vm, tc, m);
            }
            n = read_bp_string(vm, ref, buf, sizeof(buf));
        }
        uint32_t out = bpvm_heap_alloc_string(vm, buf, n);
        if (out == 0) return builtin_throw(vm, tc, "No space in heap");
        push_ref(vm, tc, out);
        return BPVM_OK;
    }

    case BUILTIN_EVAL: {   /* H7 — eval("expr") -> float (calc. de constantes) */
        uint32_t ref = pop_ref(vm, tc);   /* H1.2a: string ref = 8 bytes */
        char buf[256];
        size_t n = read_bp_string(vm, ref, buf, sizeof(buf));
        double v = bpvm_eval_calc(buf, (int) n);
        union { float f; int32_t i; } u; u.f = (float) v;
        push_i32(vm, tc, u.i);
        return BPVM_OK;
    }

    /* GAP-1 — Numéricas enteras (ids 16..18). Byte-exactas con la VM-Java:
     * sólo aritmética int32, sin libm, así que la paridad es por construcción
     * (a diferencia de sqrt/sin/cos, donde Java Math vs C libm podrían diferir
     * en el último ULP y romper el stdout byte-idéntico). */
    case BUILTIN_ABS: {
        int32_t x = pop_i32(vm, tc);
        /* Math.abs(INT_MIN) = INT_MIN (overflow). Vía unsigned para evitar el
         * UB de negar INT_MIN y casar exactamente con la VM-Java. */
        int32_t r = (x < 0) ? (int32_t) (0u - (uint32_t) x) : x;
        push_i32(vm, tc, r);
        return BPVM_OK;
    }
    case BUILTIN_MIN: {
        int32_t b = pop_i32(vm, tc);   /* pop b primero, luego a (orden VM-Java) */
        int32_t a = pop_i32(vm, tc);
        push_i32(vm, tc, (a < b) ? a : b);
        return BPVM_OK;
    }
    case BUILTIN_MAX: {
        int32_t b = pop_i32(vm, tc);
        int32_t a = pop_i32(vm, tc);
        push_i32(vm, tc, (a > b) ? a : b);
        return BPVM_OK;
    }

    case BUILTIN_INT_TO_STRING: {
        int32_t v = pop_i32(vm, tc);
        bpref_t h; char em[32];
        if (bpvm_prim_int_to_string(vm, v, &h, em, sizeof em) != 0) return builtin_throw(vm, tc, em);   /* P2b */
        bpref_push(vm, tc, h);
        return BPVM_OK;
    }

    /* L13 — coerciones a string del concat. float/double usan el formateo
     * canónico GAP-4 (bpvm_format_double, el mismo de FPRINT/DPRINT) →
     * byte-idéntico a la VM-Java, y `"" + x` == `print x` siempre. */
    case BUILTIN_FLOAT_TO_STRING: {
        int32_t bits = pop_i32(vm, tc);
        float x;
        memcpy(&x, &bits, 4);
        char buf[64];
        int n = bpvm_format_double(buf, (double) x);
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, (size_t)(n > 0 ? n : 0));
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");   /* #355: OOM ATRAPABLE, antes se empujaba una ref NULA en silencio */
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }
    case BUILTIN_LONG_TO_STRING: {
        uint32_t lo = (uint32_t) pop_i32(vm, tc);
        uint32_t hi = (uint32_t) pop_i32(vm, tc);
        int64_t v = (int64_t) (((uint64_t) hi << 32) | lo);
        char buf[32];
        int n = bpvm_format_i64(buf, v);
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, (size_t)(n > 0 ? n : 0));
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");   /* #355: OOM ATRAPABLE, antes se empujaba una ref NULA en silencio */
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }
    case BUILTIN_DOUBLE_TO_STRING: {
        uint32_t lo = (uint32_t) pop_i32(vm, tc);
        uint32_t hi = (uint32_t) pop_i32(vm, tc);
        uint64_t bits = ((uint64_t) hi << 32) | lo;
        double v;
        memcpy(&v, &bits, 8);
        char buf[64];
        int n = bpvm_format_double(buf, v);
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, (size_t)(n > 0 ? n : 0));
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");   /* #355: OOM ATRAPABLE, antes se empujaba una ref NULA en silencio */
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }

    case BUILTIN_BOOL_TO_STRING: {
        int32_t v = pop_i32(vm, tc);
        const char* s = v ? "true" : "false";
        uint32_t ref = bpvm_heap_alloc_string(vm, s, strlen(s));
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");   /* #355: OOM ATRAPABLE, antes se empujaba una ref NULA en silencio */
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }

    /* H11 (#241) — cliente TCP simple. Puente sobre la fachada bpvm_net
     * (backend sockets del SO en host; NULL en firmwares hasta H11.b/c).
     * Sin backend → RuntimeError ATRAPABLE (paridad con la VM-Java, que
     * siempre tiene java.net). El fallo de CONEXIÓN es un resultado
     * normal (handle 0 → boolean false en Net.Tcp.connect); los errores
     * de send/recv sobre una conexión establecida sí son RuntimeError. */
    case BUILTIN_TCP_CONNECT: {
        int32_t timeout_ms = pop_i32(vm, tc);
        int32_t port       = pop_i32(vm, tc);
        uint32_t href      = pop_ref(vm, tc);
        if (!bpvm_net_available()) {
            return builtin_throw(vm, tc, "Net: sin red en esta plataforma");
        }
        char host[256];
        read_bp_string(vm, href, host, sizeof(host));
        int h = bpvm_net_connect(host, (int) port, (int) timeout_ms);
        push_i32(vm, tc, (int32_t) h);          /* 0 = no conectado */
        return BPVM_OK;
    }
    case BUILTIN_TCP_SEND: {
        uint32_t dref = pop_ref(vm, tc);   /* data: byte[] */
        int32_t  h    = pop_i32(vm, tc);
        if (!bpvm_net_available()) {
            return builtin_throw(vm, tc, "Net: sin red en esta plataforma");
        }
        uint32_t dd = (dref == 0) ? 0 : bpref_deref(vm, bpref_from_addr(dref));   /* V4: handle→addr */
        uint32_t len = (dd == 0) ? 0 : bpvm_read_u32_be(vm->memory + dd);
        const uint8_t* data = (dd == 0) ? NULL : (vm->memory + dd + 4);
        int n = (len == 0) ? 0 : bpvm_net_send((int) h, data, (int) len);
        if (n < 0) return builtin_throw(vm, tc, "Net.send: conexión cerrada o inválida");
        push_i32(vm, tc, (int32_t) n);
        return BPVM_OK;
    }
    case BUILTIN_TCP_RECV: {
        int32_t timeout_ms = pop_i32(vm, tc);
        int32_t max        = pop_i32(vm, tc);
        int32_t h          = pop_i32(vm, tc);
        if (!bpvm_net_available()) {
            return builtin_throw(vm, tc, "Net: sin red en esta plataforma");
        }
        if (max < 0) max = 0;
        if (max > 65536) max = 65536;            /* tope sano por llamada */
        /* V7/H7 #597 — se recibe en un búfer de C y se reserva JUSTO lo recibido, como miVM. Antes se reservaban
         * `max` bytes en el heap y se apuntaba como longitud lo recibido («mismo patrón que READ_FILE», decía; no:
         * readFile escribe lo que reserva): el bloque medía distinto de lo que dice su cabecera —el tamaño no se
         * guarda, se RECALCULA de la longitud (#352)— y el recorrido del siguiente GC descarrilaba (#494), o se
         * resincronizaba en silencio (#593). */
        uint8_t* buf = NULL;
        if (max > 0) {
            buf = (uint8_t*) bpvm_malloc((size_t) max);
            if (buf == NULL) return builtin_throw(vm, tc, "Net.recv: sin memoria");
        }
        int n = (max == 0) ? 0
              : bpvm_net_recv((int) h, buf, (int) max, (int) timeout_ms);
        if (n == BPVM_NET_CLOSED) {
            bpvm_free(buf);
            return builtin_throw(vm, tc, "Net.recv: conexión cerrada por el peer");
        }
        if (n < 0) { bpvm_free(buf); return builtin_throw(vm, tc, "Net.recv: error de red"); }
        /* n==0 (timeout) → byte[] vacío. */
        bpref_t refh = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, (uint32_t) n, (uint32_t) n); /* V7/H7 #593: la cabecera y el handle, al nacer */
        if (refh.v == 0u) { bpvm_free(buf); return builtin_throw(vm, tc, "Net.recv: sin memoria"); }
        if (n > 0) memcpy(vm->memory + bpref_deref(vm, refh) + 4, buf, (size_t) n);
        bpvm_free(buf);
        bpref_push(vm, tc, refh);
        return BPVM_OK;
    }
    case BUILTIN_TCP_CLOSE: {
        int32_t h = pop_i32(vm, tc);
        bpvm_net_close((int) h);                 /* tolerante, sin backend = no-op */
        push_i32(vm, tc, 0);   /* V8 #705: sin resultado → su 0, como los demás (el compilador lo tira tras la llamada) */
        return BPVM_OK;
    }

    /* H19 — App.* introspección del proyecto en ejecución (id 211-213).
     *
     * ⚠️ FUERA del bloque `#ifdef BPVM_GUI`, y es donde debieron nacer: no
     * tienen NADA que ver con la pantalla. Estuvieron dentro hasta el 28-ago,
     * así que `App.mainModule()` y compañía funcionaban SÓLO en las placas con
     * GUI (P4, Discovery) y en el host con LVGL — en la Pico, el S3 y la Nucleo
     * el programa moría con «builtin 211 no soportado en esta VM». Lo que
     * engaña del caso: el `case` SÍ está escrito y el símbolo existe en el
     * fuente, así que leyendo el código parece implementado. */
    case BUILTIN_APP_MAIN_MODULE: {        /* nombre del entry: basename sin ".mod" */
        const char* p = bpvm_fs_main_module_path();
        const char* base = strrchr(p, '/');
        base = base ? base + 1 : p;
        char nm[64]; size_t i = 0;
        while (base[i] && base[i] != '.' && i + 1 < sizeof(nm)) { nm[i] = base[i]; i++; }
        nm[i] = '\0';
        uint32_t r = bpvm_heap_alloc_string(vm, nm, strlen(nm));
        if (r == 0) return builtin_throw(vm, tc, "No space in heap");   /* #355: OOM ATRAPABLE, antes se empujaba una ref NULA en silencio */
        push_ref(vm, tc, r);   /* V4 #8: string = ref 8B (era push_i32 4B → drift + gen=0) */
        return BPVM_OK;
    }
    case BUILTIN_APP_MAIN_MODULE_PATH: {
        const char* s = bpvm_fs_main_module_path();
        uint32_t r = bpvm_heap_alloc_string(vm, s, strlen(s));
        if (r == 0) return builtin_throw(vm, tc, "No space in heap");   /* #355: OOM ATRAPABLE, antes se empujaba una ref NULA en silencio */
        push_ref(vm, tc, r);   /* V4 #8: string = ref 8B (era push_i32 4B → drift + gen=0) */
        return BPVM_OK;
    }
    case BUILTIN_APP_PROJECT_PATH: {
        const char* s = bpvm_fs_basedir();
        uint32_t r = bpvm_heap_alloc_string(vm, s, strlen(s));
        if (r == 0) return builtin_throw(vm, tc, "No space in heap");   /* #355: OOM ATRAPABLE, antes se empujaba una ref NULA en silencio */
        push_ref(vm, tc, r);   /* V4 #8: string = ref 8B (era push_i32 4B → drift + gen=0) */
        return BPVM_OK;
    }

#ifdef BPVM_GUI
    /* ---- V3 / H4.1 — GUI (modelo de comportamiento; paridad por dumpTree).
     * Sólo en el build con GUI. Convención (confirmada por disasm): todo builtin
     * GUI deja UN valor en pila — void → 0 (el emisor lo guarda), valor → result.
     * Orden de pop: último arg en top (igual que el resto). Color/fuente son
     * render-only → no afectan al dump (no-op aquí; LVGL los honrará en H4.2). */
    case BUILTIN_GUI_SCREEN_ACTIVE: {
        /* V7/H6 — el primer paso de todo programa con GUI: si la GUI llega por el
         * pack y no está (o no vale), aquí se dice CON NOMBRE, no en un 0 mudo. */
        const char* motivo = 0;
        if (!bpvm_gui_disponible(&motivo)) {
            char m[192]; snprintf(m, sizeof m, "GUI: %s", motivo ? motivo : "no disponible");
            return builtin_throw(vm, tc, m);
        }
        int pantalla = bpvm_gui_screen_active();
#ifdef BPVM_GUI_RENDER   /* V7/H6: LVGL dentro, o por el pack */
        {
            /* #473 (lote E, 180; 3-oct) — UNA PANTALLA QUE NO ARRANCA YA NO ES MUDA. Si el driver falla al iniciarse
             * (la C5 o la C6 con el bus SPI tomado, el P4 sin panel), sólo lo decía el log y el programa seguía con
             * una pantalla muerta: `Gui.run()` volvía a la primera. Sólo cuando HAY driver —`disp_native_size` dice
             * su tamaño—: sin driver es el modo mudo del pack (el modelo va, no se pinta) y sin ventana a propósito
             * (`--no-screen`, la ventana del PC) tampoco hay tamaño. El motivo exacto (el código del driver) está en
             * el log; traerlo aquí es ampliar la API del pack. */
            int sw = 0, sh = 0;
            if (bpvm_gui_disp_native_size(&sw, &sh) && sw > 0 && sh > 0 && !bpvm_gui_lvgl_window_open())
                return builtin_throw(vm, tc, "GUI: la pantalla no arranco: su driver fallo al iniciarse (el motivo, en el log)");
        }
#endif
        push_i32(vm, tc, pantalla); return BPVM_OK;
    }
    case BUILTIN_GUI_CREATE_OBJ: return gui_make_child(vm, tc, bpvm_gui_create_obj);
    case BUILTIN_GUI_CREATE_LABEL: return gui_make_child(vm, tc, bpvm_gui_create_label);
    case BUILTIN_GUI_CREATE_BUTTON: return gui_make_child(vm, tc, bpvm_gui_create_button);
    case BUILTIN_GUI_SET_TEXT: {
        uint32_t tref = pop_ref(vm, tc);
        int handle = pop_i32(vm, tc);
        char buf[1024];
        read_bp_string(vm, tref, buf, sizeof(buf));
        bpvm_gui_set_text(handle, buf);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_SET_WIDTH:  { int w = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_width(h, w);  push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_SET_HEIGHT: { int v = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_height(h, v); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_ALIGN: {
        int dy = pop_i32(vm, tc); int dx = pop_i32(vm, tc); int a = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_align(h, a, dx, dy); push_i32(vm, tc, 0); return BPVM_OK;
    }
    /* Color/fuente: render-only. En modelo-only son no-op (no tocan el dump);
     * bajo BPVM_LVGL aplican estilo al lv_obj. Se enrutan a gui.c en ambos casos. */
    case BUILTIN_GUI_SET_BG_COLOR:   { uint32_t rgb = (uint32_t) pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_bg_color(h, rgb);   push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_SET_TEXT_COLOR: { uint32_t rgb = (uint32_t) pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_text_color(h, rgb); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_SET_FONT:       { int f = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_font(h, f); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_LOAD_FONT:      { uint32_t ref = pop_ref(vm, tc); char path[256]; read_bp_string(vm, ref, path, sizeof(path));
                                       if (pack_uri_falta(path)) return builtin_throw(vm, tc, recurso_no_esta(path));
                                       { char alt[BPVM_FS_PATH_MAX]; const char* r = bpvm_fs_resolve(path, alt, sizeof alt);   /* V7/H6: resuelto en la VM, antes de cruzar al pack */
                                         push_i32(vm, tc, bpvm_gui_load_font(r ? r : path)); return BPVM_OK; } }
    case BUILTIN_GUI_SET_ROTATION:   { int deg = pop_i32(vm, tc); bpvm_gui_set_rotation(deg); push_i32(vm, tc, 0); return BPVM_OK; }

    case BUILTIN_GUI_CLEAN:       { int h = pop_i32(vm, tc); bpvm_gui_clean(h);  push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_DELETE:      { int h = pop_i32(vm, tc); bpvm_gui_delete(h); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_SCREEN_LOAD: { pop_i32(vm, tc); push_i32(vm, tc, 0); return BPVM_OK; }   /* una sola pantalla por ahora */
    case BUILTIN_GUI_RUN: {
        /* Lazo de eventos. Resuelve por NOMBRE los dos dispatchers BP:
         * Gui.__guiDispatch (click→onClick) y Gui.__guiDispatchChange (change→
         * onChange). Cada evento drenado lleva su `kind` y elige el dispatcher
         * (simétrico a invokeGuiDispatch de miVM). Con LVGL: bombea la ventana SDL
         * hasta cerrarla, drenando eventos reales + sintéticos. Sin LVGL
         * (modelo-only/arnés): drena los inyectados y vuelve. */
        uint32_t disp_click, disp_change;
        gui_dispatchers(vm, &disp_click, &disp_change);   /* #486 - una vez por RUN */
#ifdef BPVM_GUI_RENDER   /* V7/H6: LVGL dentro, o por el pack */
        for (;;) {
            uint32_t objptr; int kind;
            while ((objptr = bpvm_gui_next_event(&kind)) != 0) {
                uint32_t d = (kind == 1) ? disp_change : disp_click;
                if (d) { int32_t a = (int32_t) objptr; bpvm_call_bp_from_builtin(vm, tc, d, &a, 1, 1u); /* #302: arg0 es ref */ }
            }
            /* P-run-stop (#257) — KILL durante Gui.run(): el scheduler no corre
             * quanta mientras este builtin bombea, así que poleamos el wire aquí
             * mismo (el MISMO poll_cb que el scheduler usa entre quanta). Al romper
             * caemos al push+return de abajo → el quantum termina → el scheduler ve
             * kill_requested y devuelve BPVM_KILLED (parada limpia entre opcodes). */
            /* V6/A1 - con el hilo `io` en marcha, el wire NO se mira desde aqui: lo
         * atiende `io` y el KILL llega como `kill_requested`, que se comprueba
         * justo debajo. Sin esta guarda habria DOS lectores del mismo transporte
         * -este builtin desde la tarea `vm` y el lazo de `io`- compartiendo ademas
         * el buffer de linea estatico de la familia. Es la MISMA guarda que
         * src/scheduler.c pone entre cuantos; faltaba aqui, y el camino del GUI es
         * precisamente el que la placa con pantalla recorre. */
        if (vm->io == NULL && vm->poll_cb != NULL
                && vm->poll_cb(vm, vm->poll_user) != 0)
                bpvm_kill_marcar(vm, 1);
            if (bpvm_kill_pedido(vm)) break;
            if (!bpvm_gui_lvgl_window_open()) break;
            /* #462 - este lazo (el __guiRun de legado) SI bloquea por diseno: no
             * hay hilo BP al que cederle el turno porque el builtin no vuelve.
             * Asi que aqui la espera si va a nivel de SO. El lazo bueno es el de
             * Gui.bp, que duerme con un `sleep` de BP. */
            bpvm_platform_thread_sleep_ms(bpvm_gui_lvgl_pump());
        }
#else
        /* Modelo-only (headless): drena los eventos inyectados y vuelve (paridad). */
        uint32_t objptr; int kind;
        while ((objptr = bpvm_gui_next_event(&kind)) != 0) {
            uint32_t d = (kind == 1) ? disp_change : disp_click;
            if (d) { int32_t a = (int32_t) objptr; bpvm_call_bp_from_builtin(vm, tc, d, &a, 1, 1u); /* #302: arg0 es ref */ }
        }
#endif
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_RUN_ONCE: {
        /* #324 — UNA pasada del bombeo. Mismo trabajo que GUI_RUN por dentro,
         * pero SIN el lazo: el lazo vive ahora en Gui.run() (BP), que es lo que
         * da la frontera de instrucción donde el scheduler drena los eventos.
         * Devuelve 1 = "vuelve a llamarme" / 0 = "no queda nada". */
        uint32_t disp_click, disp_change;
        gui_dispatchers(vm, &disp_click, &disp_change);   /* #486 - una vez por RUN */
        int drained = 0;
        uint32_t objptr; int kind;
        while ((objptr = bpvm_gui_next_event(&kind)) != 0) {
            uint32_t d = (kind == 1) ? disp_change : disp_click;
            if (d) { int32_t a = (int32_t) objptr; bpvm_call_bp_from_builtin(vm, tc, d, &a, 1, 1u); /* #302: arg0 es ref */ }
            drained++;
        }
        /* P-run-stop (#257): el KILL se sigue poleando aquí. Ahora además el
         * scheduler ve el kill entre pasadas, así que la parada es más fina que
         * con el lazo dentro del builtin. */
        /* V6/A1 - con el hilo `io` en marcha, el wire NO se mira desde aqui: lo
         * atiende `io` y el KILL llega como `kill_requested`, que se comprueba
         * justo debajo. Sin esta guarda habria DOS lectores del mismo transporte
         * -este builtin desde la tarea `vm` y el lazo de `io`- compartiendo ademas
         * el buffer de linea estatico de la familia. Es la MISMA guarda que
         * src/scheduler.c pone entre cuantos; faltaba aqui, y el camino del GUI es
         * precisamente el que la placa con pantalla recorre. */
        if (vm->io == NULL && vm->poll_cb != NULL
                && vm->poll_cb(vm, vm->poll_user) != 0)
            bpvm_kill_marcar(vm, 1);
        /* #462 - el contrato cambia: -1 = "no queda nada, sal del lazo";
         * >= 0 = "vuelve dentro de N ms". Quien duerme esos ms es el lazo BP de
         * Gui.bp, con un `sleep` de BP -que bloquea ESE hilo BP y le pasa el
         * turno al siguiente-. Antes se devolvia un booleano y la espera se la
         * comia el bombeo, a nivel de SO, congelando a todos los hilos BP. */
        if (bpvm_kill_pedido(vm)) { push_i32(vm, tc, -1); return BPVM_OK; }
#ifdef BPVM_GUI_RENDER   /* V7/H6: LVGL dentro, o por el pack */
        if (!bpvm_gui_lvgl_window_open()) {
            /* SIN VENTANA (--no-screen con LVGL, o la cerro el usuario): la salida
             * es la MISMA regla que el headless sin LVGL de abajo, no un -1 a
             * secas. Un `raise` desde el handler de un clic ENCOLA, y el scheduler
             * solo inyecta el frame del handler entre quanta: salir aqui con
             * ev_count > 0 dejaba el evento anidado sin drenar — lo cazo Eduardo
             * el 12-sep en el micro simulado del IDE (--no-screen) con
             * samples/GuiEvSpike.bp: «1 evento(s) sin atender», y el «3 handler»
             * no salia. Con ventana (o panel) no pasaba porque el lazo sigue
             * girando; sin ella, el lazo se iba a la primera vuelta. */
            push_i32(vm, tc, (drained > 0 || vm->ev_count > 0) ? 0 : -1);
            return BPVM_OK;
        }
        push_i32(vm, tc, (int32_t) bpvm_gui_lvgl_pump());   /* ventana viva: el ocio que pide LVGL */
#else
        /* Headless: "queda trabajo" = clics drenados en ESTA pasada O eventos BP
         * ENCOLADOS sin entregar. Lo segundo es lo que hace que el lazo sirva de
         * algo: `raise` encola, y el scheduler sólo inyecta el frame del handler
         * ENTRE quanta (events.c: "el thread NO puede estar corriendo"). Salir en
         * cuanto no hay clics deja el evento en la cola para siempre — que es
         * exactamente lo que midió samples/GuiEvSpike.bp. Girando mientras
         * ev_count>0, el lazo alcanza la frontera de quantum donde se drena. */
        push_i32(vm, tc, (drained > 0 || vm->ev_count > 0) ? 0 : -1);
#endif
        return BPVM_OK;
    }
    case BUILTIN_GUI_SLOT_OF: {
        /* #324 tanda 2b — REFLEXION MINIMA: nombre de metodo -> slot de vtable,
         * en EJECUCION. Idea de Eduardo, y no hace falta dato nuevo: el .mod ya
         * exporta un simbolo `Clase#metodo#slot` por cada entrada de la vtable
         * (exportVtableMarkers), apuntando al descriptor de la clase. O sea que
         * el mapa nombre->slot YA viaja; solo faltaba preguntarle.
         *
         * Con esto el .win deja de necesitar slots horneados: se acabo el
         * numero congelado en un fichero que miente cuando la clase cambia.
         *
         * Devuelve -1 si no existe (el llamante decide; aqui no se lanza nada:
         * "no hay handler con ese nombre" es normal, no un error).
         *
         * Coste: un barrido lineal de la tabla, en CARGA del form y una vez por
         * nombre. No esta en ningun camino caliente. */
        uint32_t name_ref = pop_ref(vm, tc);
        uint32_t obj      = pop_ref(vm, tc);
        if (obj == 0) { push_i32(vm, tc, -1); return BPVM_OK; }
        char nm[96];
        read_bp_string(vm, name_ref, nm, sizeof(nm));
        uint32_t oa = ref_addr(vm, obj);
        uint32_t class_ptr = (uint32_t) bpvm_read_i32_be(vm->memory + oa);
        size_t nlen = strlen(nm);
        {   /* V7/H2 (#523) — una clase MOD9 no tiene marcadores: su subtabla CLASES si. */
            int s9 = bpvm_link_slot_por_nombre(vm, class_ptr, nm);
            if (s9 >= 0) { push_i32(vm, tc, s9); return BPVM_OK; }
        }
        for (int i = 0; i < vm->symbol_count; i++) {
            if (vm->symbols[i].abs_addr != class_ptr) continue;
            /* Formato: <Clase>#<metodo>#<slot>. Se busca "#<metodo>#" y se lee
             * el numero que va detras. */
            const char* s = bpvm_symbol_name(vm, i);
            const char* h = strchr(s, '#');
            if (h == NULL) continue;
            if (strncmp(h + 1, nm, nlen) != 0 || h[1 + nlen] != '#') continue;
            push_i32(vm, tc, atoi(h + 2 + nlen));
            return BPVM_OK;
        }
        push_i32(vm, tc, -1);
        return BPVM_OK;
    }
    case BUILTIN_GUI_DUMP_TREE: {
        char* buf = NULL;
        size_t n = bpvm_gui_dump_tree(&buf);
        uint32_t ref = bpvm_heap_alloc_string(vm, buf ? buf : "", n);
        /* #355 — el free va ANTES del throw: si no, el camino de OOM se deja
         * `buf` sin liberar, y quedarse sin memoria no es excusa para fugarla. */
        bpvm_free(buf);
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");   /* #355: OOM ATRAPABLE, antes se empujaba una ref NULA en silencio */
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }
#endif  /* BPVM_GUI — los packs NO son de la GUI: se cierra aqui a proposito */

    /* ── #414 — los packs GRABADOS, recorridos desde BP ───────────────────────
     *
     * Sin zona montada no es un error: es «ya estás al final». Eso hace que un
     * mismo programa valga en una placa con packs, en una sin ellos y en miVM
     * (que no tiene zona) sin un solo `if` de plataforma — el bucle no entra y
     * la lista sale vacía. */
    case BUILTIN_PACK_NEXT: {
        int32_t cur = pop_i32(vm, tc);
        uint32_t zsz = 0;
        const uint8_t* zona = bpvm_pack_mounted(&zsz);
        if (zona == NULL) { push_i32(vm, tc, -1); return BPVM_OK; }
        bpvm_pack_src_t s;
        bpvm_pack_src_mem(&s, zona, zsz);
        push_i32(vm, tc, bpvm_pack_iter(&s, cur));
        return BPVM_OK;
    }
    case BUILTIN_PACK_ENTRY_NEXT: {
        /* OJO al orden: el último argumento es el que está ARRIBA en la pila. */
        int32_t cur  = pop_i32(vm, tc);
        int32_t pcur = pop_i32(vm, tc);
        uint32_t zsz = 0;
        const uint8_t* zona = bpvm_pack_mounted(&zsz);
        if (zona == NULL) { push_i32(vm, tc, -1); return BPVM_OK; }
        bpvm_pack_src_t s;
        bpvm_pack_src_mem(&s, zona, zsz);
        push_i32(vm, tc, bpvm_pack_iter_entry(&s, pcur, cur));
        return BPVM_OK;
    }
    case BUILTIN_PACK_INFO:
    case BUILTIN_PACK_ENTRY_INFO: {
        /* Un cursor que no vale devuelve "" y no lanza: el que itera bien nunca
         * llega aquí con basura, y el que la trae ya se enterará por la cadena
         * vacía. Lanzar obligaría a envolver en try/catch un bucle normal. */
        char texto[BPVM_PACK_NAME_LEN + BPVM_PACK_TYPE_LEN + 2];
        texto[0] = '\0';
        int32_t cur  = pop_i32(vm, tc);
        int32_t pcur = (id == BUILTIN_PACK_ENTRY_INFO) ? pop_i32(vm, tc) : 0;
        uint32_t zsz = 0;
        const uint8_t* zona = bpvm_pack_mounted(&zsz);
        if (zona != NULL) {
            bpvm_pack_src_t s;
            bpvm_pack_src_mem(&s, zona, zsz);
            if (id == BUILTIN_PACK_INFO) {
                bpvm_pack_info_t info;
                if (bpvm_pack_iter_info(&s, cur, &info) == 0)
                    snprintf(texto, sizeof texto, "%s", info.nombre);
            } else {
                bpvm_pack_entry_t e;
                if (bpvm_pack_iter_entry_info(&s, pcur, cur, &e) == 0)
                    snprintf(texto, sizeof texto, "%s.%s", e.nombre, e.tipo);
            }
        }
        uint32_t ref = bpvm_heap_alloc_string(vm, texto, strlen(texto));
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }

#ifdef BPVM_GUI   /* se reanuda el bloque de GUI, cortado por los packs */
    case BUILTIN_GUI_INVOKE_BY_NAME: {
        /* H13 — Forms: resuelve `name` como función pública del módulo de `host`
         * (la ventana) y la invoca con `sender` como arg0. Args: host, name, sender.
         * Espejo de GUI_INVOKE_BY_NAME en miVM. */
        uint32_t sender   = pop_ref(vm, tc);
        uint32_t name_ref = pop_ref(vm, tc);
        uint32_t host     = pop_ref(vm, tc);
        char nm[128];
        read_bp_string(vm, name_ref, nm, sizeof(nm));
        uint32_t target = bpvm_resolve_handler(vm, host, nm);
        if (target == 0) {
            /* H13 (decisión de Eduardo): handler no implementado → IGNORAR (sin
             * excepción). El aviso se da una vez al cargar el form (Gui.Window). */
            push_i32(vm, tc, 0);
            return BPVM_OK;
        }
        int32_t a = (int32_t) sender;
        bpvm_call_bp_from_builtin(vm, tc, target, &a, 1, 1u); /* #302: sender es ref */
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_INVOKE_BY_SLOT: {
        /* H13.1 — Forms Camino A: invoca el MÉTODO de la ventana en vtable[slot]
         * (slot horneado por el IDE). Args: win, slot, sender. Espejo de
         * invokeHandlerBySlot en miVM: paseo de vtable con fallback al padre
         * (idéntico a OP_INVOKE_VIRTUAL) → bridge con [this=win, sender] como
         * locals 0/1. slot < 0 o no resoluble → IGNORA (sin excepción). */
        uint32_t sender = pop_ref(vm, tc);
        int32_t  slot   = pop_i32(vm, tc);
        uint32_t win    = pop_ref(vm, tc);
        if (win == 0 || slot < 0) { push_i32(vm, tc, 0); return BPVM_OK; }
        uint8_t* mem = vm->memory;
        uint32_t wa = ref_addr(vm, win);   /* V4: handle→addr (el `this` que se pasa abajo sigue siendo el REF `win`) */
        uint32_t desc = (uint32_t) bpvm_read_i32_be(mem + wa);
        int32_t  method_off = -1;
        uint32_t target_cs  = 0;
        /* La regla del despacho virtual, la de OP_INVOKE_VIRTUAL (era una copia a mano del mismo paseo). */
        if (bpvm_vt_resolver(vm, desc, (uint32_t) slot, &method_off, &target_cs) != 0) {
            push_i32(vm, tc, 0); return BPVM_OK;   /* no resoluble → ignora */
        }
        /* V7/H7 #606 — method_off es CS-relativo; la dirección real va por la base de CÓDIGO del módulo dueño (cb;
         * en RAM, cb == cs), como en OP_INVOKE_VIRTUAL. Sin ella, una ventana cuya clase vive en un pack (XIP)
         * saltaba a basura en el primer evento. */
        uint32_t target_abs = bpvm_cb_for_cs(vm, tc, target_cs) + (uint32_t) method_off;
        int32_t a2[2]; a2[0] = (int32_t) win; a2[1] = (int32_t) sender;
        bpvm_call_bp_from_builtin(vm, tc, target_abs, a2, 2, 3u); /* #302: win+sender son refs */
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_BIND_CLICK: {
        uint32_t self = pop_ref(vm, tc);
        int handle = pop_i32(vm, tc);
        bpvm_gui_bind_click(handle, self);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GUI_CLICK: {
        uint32_t obj = pop_ref(vm, tc);
        bpvm_gui_inject_click(obj);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    /* H6 — geometría (backend = verdad) + scroll (opt-in) + refresh. */
    case BUILTIN_GUI_SET_X:  { int v = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_x(h, v); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_GET_X:  { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_x(h)); return BPVM_OK; }
    case BUILTIN_GUI_SET_Y:  { int v = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_y(h, v); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_GET_Y:  { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_y(h)); return BPVM_OK; }
    case BUILTIN_GUI_GET_WIDTH:  { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_width(h));  return BPVM_OK; }
    case BUILTIN_GUI_GET_HEIGHT: { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_height(h)); return BPVM_OK; }
    case BUILTIN_GUI_SET_SCROLL_DIR: { int d = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_scroll_dir(h, d); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_GET_SCROLL_DIR: { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_scroll_dir(h)); return BPVM_OK; }
    /* V6/G2-4 — geometria autorada (align/dx/dy, w/h con -1 = auto). */
    case BUILTIN_GUI_GET_ALIGN:       { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_align(h)); return BPVM_OK; }
    case BUILTIN_GUI_GET_ALIGN_DX:    { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_align_dx(h)); return BPVM_OK; }
    case BUILTIN_GUI_GET_ALIGN_DY:    { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_align_dy(h)); return BPVM_OK; }
    case BUILTIN_GUI_GET_AUTH_WIDTH:  { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_auth_width(h)); return BPVM_OK; }
    case BUILTIN_GUI_GET_AUTH_HEIGHT: { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_auth_height(h)); return BPVM_OK; }
    /* V6/C1 — Gui.shot(path). El path se usa TAL CUAL, como writeFile. El
     * codigo de retorno lo traduce Gui.bp a RuntimeError (un solo sitio). */
    case BUILTIN_GUI_SHOT: {
        uint32_t pref = pop_ref(vm, tc);
        char path[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        push_i32(vm, tc, bpvm_gui_shot(path));
        return BPVM_OK;
    }
    case BUILTIN_GUI_REFRESH: { int h = pop_i32(vm, tc); bpvm_gui_refresh(h); push_i32(vm, tc, 0); return BPVM_OK; }
    /* H6 — checkbox (1er value-widget). set_checked es programático (no emite
     * onChange); __guiChange inyecta un CHANGE sintético (= toggle del usuario). */
    case BUILTIN_GUI_CREATE_CHECKBOX: return gui_make_child(vm, tc, bpvm_gui_create_checkbox);
    case BUILTIN_GUI_SET_CHECKED:     { int v = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_checked(h, v); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_GET_CHECKED:     { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_checked(h)); return BPVM_OK; }
    case BUILTIN_GUI_CHANGE:          { uint32_t obj = pop_ref(vm, tc); bpvm_gui_inject_change(obj); push_i32(vm, tc, 0); return BPVM_OK; }
    /* H6 — switch + slider + bar (value-widgets enteros; el backend clampa al rango). */
    case BUILTIN_GUI_CREATE_SWITCH: return gui_make_child(vm, tc, bpvm_gui_create_switch);
    case BUILTIN_GUI_CREATE_SLIDER: return gui_make_child(vm, tc, bpvm_gui_create_slider);
    case BUILTIN_GUI_CREATE_BAR: return gui_make_child(vm, tc, bpvm_gui_create_bar);
    case BUILTIN_GUI_SET_VALUE:     { int v = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_value(h, v); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_GET_VALUE:     { int h = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_get_value(h)); return BPVM_OK; }
    case BUILTIN_GUI_SET_RANGE:     { int mx = pop_i32(vm, tc); int mn = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_set_range(h, mn, mx); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_SPINBOX: return gui_make_child(vm, tc, bpvm_gui_create_spinbox);
    case BUILTIN_GUI_CREATE_LED: return gui_make_child(vm, tc, bpvm_gui_create_led);
    case BUILTIN_GUI_CREATE_DROPDOWN: return gui_make_child(vm, tc, bpvm_gui_create_dropdown);
    case BUILTIN_GUI_SET_OPTIONS: {
        uint32_t ref = pop_ref(vm, tc); int h = pop_i32(vm, tc);
        char buf[512]; read_bp_string(vm, ref, buf, sizeof(buf));
        bpvm_gui_set_options(h, buf); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CREATE_TEXTAREA: return gui_make_child(vm, tc, bpvm_gui_create_textarea);
    case BUILTIN_GUI_GET_TEXT: {
        int h = pop_i32(vm, tc);
        const char* s = bpvm_gui_get_text(h);
        uint32_t ref = bpvm_heap_alloc_string(vm, s, strlen(s));
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");   /* #355: OOM ATRAPABLE, antes se empujaba una ref NULA en silencio */
        push_ref(vm, tc, ref); return BPVM_OK;
    }
    case BUILTIN_GUI_CREATE_LIST: return gui_make_child(vm, tc, bpvm_gui_create_list);
    case BUILTIN_GUI_CREATE_KEYBOARD: return gui_make_child(vm, tc, bpvm_gui_create_keyboard);
    case BUILTIN_GUI_KEYBOARD_SET_TEXTAREA: { int ta = pop_i32(vm, tc); int h = pop_i32(vm, tc); bpvm_gui_keyboard_set_textarea(h, ta); push_i32(vm, tc, 0); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_MSGBOX: return gui_make_child(vm, tc, bpvm_gui_create_msgbox);
    case BUILTIN_GUI_SET_BUTTONS: {
        uint32_t ref = pop_ref(vm, tc); int h = pop_i32(vm, tc);
        char buf[256]; read_bp_string(vm, ref, buf, sizeof(buf));
        bpvm_gui_set_buttons(h, buf); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CREATE_TABVIEW: return gui_make_child(vm, tc, bpvm_gui_create_tabview);
    case BUILTIN_GUI_TABVIEW_ADD_TAB: {
        uint32_t ref = pop_ref(vm, tc); int h = pop_i32(vm, tc);
        char buf[128]; read_bp_string(vm, ref, buf, sizeof(buf));
        int t = bpvm_gui_tabview_add_tab(h, buf);
        if (t <= 0) return gui_sin_sitio(vm, tc, t);      /* #568: la pestaña es un nodo más */
        push_i32(vm, tc, t); return BPVM_OK;
    }
    /* H7 — Chart */
    case BUILTIN_GUI_CREATE_CHART: return gui_make_child(vm, tc, bpvm_gui_create_chart);
    case BUILTIN_GUI_CHART_SET_POINTS: {
        int n = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_chart_set_points(h, n); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CHART_ADD_SERIES: {
        int rgb = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        push_i32(vm, tc, bpvm_gui_chart_add_series(h, rgb)); return BPVM_OK;
    }
    case BUILTIN_GUI_CHART_PUSH: {
        int v = pop_i32(vm, tc); int s = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_chart_push(h, s, v); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CHART_SET_VALUE: {
        int v = pop_i32(vm, tc); int i = pop_i32(vm, tc); int s = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_chart_set_value(h, s, i, v); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CHART_SET_TYPE: {
        int t = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_chart_set_type(h, t); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_CREATE_TABLE: return gui_make_child(vm, tc, bpvm_gui_create_table);
    case BUILTIN_GUI_TABLE_SET_GRID: {
        int cols = pop_i32(vm, tc); int rows = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_table_set_grid(h, rows, cols); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_TABLE_SET_CELL: {
        uint32_t ref = pop_ref(vm, tc);
        int col = pop_i32(vm, tc); int row = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        char buf[256]; read_bp_string(vm, ref, buf, sizeof(buf));
        bpvm_gui_table_set_cell(h, row, col, buf); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_TABLE_GET_CELL: {
        int col = pop_i32(vm, tc); int row = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        const char* s = bpvm_gui_table_get_cell(h, row, col);
        uint32_t ref = bpvm_heap_alloc_string(vm, s, strlen(s));
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");   /* #355: OOM ATRAPABLE, antes se empujaba una ref NULA en silencio */
        push_ref(vm, tc, ref); return BPVM_OK;
    }
    case BUILTIN_GUI_IMAGE_NEW: { push_i32(vm, tc, bpvm_gui_image_new()); return BPVM_OK; }
    case BUILTIN_GUI_IMAGE_LOAD_FILE: {
        uint32_t ref = pop_ref(vm, tc); int id = pop_i32(vm, tc);
        char buf[512]; read_bp_string(vm, ref, buf, sizeof(buf));
        if (pack_uri_falta(buf)) return builtin_throw(vm, tc, recurso_no_esta(buf));
        /* V7/H6 — el camino se RESUELVE aquí, en la VM, antes de cruzar: en el
         * pack gui.c no tiene la fachada (su bpvm_fs_resolve devuelve lo que le
         * dan). Resolver dos veces en el host es la identidad. */
        { char alt[BPVM_FS_PATH_MAX]; const char* r = bpvm_fs_resolve(buf, alt, sizeof alt);
          push_i32(vm, tc, bpvm_gui_image_load_file(id, r ? r : buf)); return BPVM_OK; }
    }
    case BUILTIN_GUI_IMAGE_WIDTH:  { int id = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_image_width(id));  return BPVM_OK; }
    case BUILTIN_GUI_IMAGE_HEIGHT: { int id = pop_i32(vm, tc); push_i32(vm, tc, bpvm_gui_image_height(id)); return BPVM_OK; }
    case BUILTIN_GUI_CREATE_IMAGE_VIEW: return gui_make_child(vm, tc, bpvm_gui_create_imageview);
    case BUILTIN_GUI_IMAGE_VIEW_SET_IMAGE: {
        int img = pop_i32(vm, tc); int view = pop_i32(vm, tc);
        bpvm_gui_imageview_set_image(view, img); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_IMAGE_VIEW_REFRESH: {
        int view = pop_i32(vm, tc);
        bpvm_gui_imageview_refresh(view); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_SET_FONT_SIZE: {
        int px = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_set_font_size(h, px); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_GET_FONT_SIZE: {
        int h = pop_i32(vm, tc);
        push_i32(vm, tc, bpvm_gui_get_font_size(h)); return BPVM_OK;
    }
    case BUILTIN_GUI_TEXTAREA_SET_READONLY: {
        int ro = pop_i32(vm, tc); int h = pop_i32(vm, tc);
        bpvm_gui_textarea_set_readonly(h, ro); push_i32(vm, tc, 0); return BPVM_OK;
    }
    case BUILTIN_GUI_TEXTAREA_GET_READONLY: {
        int h = pop_i32(vm, tc);
        push_i32(vm, tc, bpvm_gui_textarea_get_readonly(h)); return BPVM_OK;
    }
#endif /* BPVM_GUI */

    case BUILTIN_PARSE_INT: {   /* V8 #716: la regla de num_entero, la misma que miVM (antes, strtol: "12abc" → 12) */
        bpref_t sref = bpref_pop(vm, tc);   /* H1.2a: string ref = 8 bytes */
        uint32_t sa = bpref_is_null(sref) ? 0u : bpref_deref(vm, sref);
        uint32_t n = sa ? bpvm_read_u32_be(vm->memory + sa) : 0u;
        const uint8_t* p = sa ? vm->memory + sa + BPVM_ARR_DATA_OFF : (const uint8_t*) "";
        uint32_t a = num_ini(p, n), b = num_fin(p, a, n);
        int32_t v = 0;
        int r = (a < b) ? num_entero(p, a, b, &v) : NUM_NO_ES;
        if (r != NUM_VALE)
            return num_throw(vm, tc, "parseInt", p, n, r == NUM_NO_ES ? "no es un número entero" : "no cabe en un integer");
        push_i32(vm, tc, v);
        return BPVM_OK;
    }

    case BUILTIN_PARSE_FLOAT: {   /* string -> float (f32). V8 #716: num_es_decimal decide; lo válido, strtod: con un
                                   * decimal limpio, (float)strtod == (float)Double.parseDouble (f32 correctamente
                                   * redondeado) -> paridad con miVM. */
        bpref_t sref = bpref_pop(vm, tc);   /* H1.2a: string ref = 8 bytes */
        uint32_t sa = bpref_is_null(sref) ? 0u : bpref_deref(vm, sref);
        uint32_t n = sa ? bpvm_read_u32_be(vm->memory + sa) : 0u;
        const uint8_t* p = sa ? vm->memory + sa + BPVM_ARR_DATA_OFF : (const uint8_t*) "";
        uint32_t a = num_ini(p, n), b = num_fin(p, a, n);
        if (!num_es_decimal(p, a, b)) return num_throw(vm, tc, "parseFloat", p, n, "no es un número");
        /* strtod pide el número acabado en '\0': una copia (en la pila; uno de más de 63 caracteres, aparte) */
        char corto[64];
        char* t = corto;
        uint32_t m = b - a;
        if (m + 1u > sizeof corto) {
            t = (char*) bpvm_malloc(m + 1u);
            if (t == NULL) return builtin_throw(vm, tc, "No space in heap");
        }
        memcpy(t, p + a, m);
        t[m] = '\0';
        union { float f; int32_t i; } u;
        /* V7/H12, 2b (la revisión) — strtod, bajo el cerrojo de la VM: el de newlib reparte sus números grandes desde
         * una lista del estado global de la libc (Balloc), y en la Pico ese estado es UNO para todas las tareas (sin
         * configUSE_NEWLIB_REENTRANT): dos runs a la vez la corromperían. Con un run, nada. */
        bpvm_smp_lock(vm);
        u.f = (float) strtod(t, NULL);
        bpvm_smp_unlock(vm);
        if (t != corto) bpvm_free(t);
        if (isinf(u.f)) return num_throw(vm, tc, "parseFloat", p, n, "no cabe en un float");
        push_i32(vm, tc, u.i);
        return BPVM_OK;
    }

    /* File I/O (H10/#247) — puente sobre la fachada bpvm_fs (backend libc en
     * host, fs_get/fs_put en device). Paridad con la VM-Java: lee/escribe los
     * bytes del string BP (UTF-8) tal cual; errores → RuntimeError atrapable.
     * El path se lee a un buffer C; el contenido se escribe/lee directo desde
     * el heap (cualquier tamaño). */
    case BUILTIN_READ_FILE:
    case BUILTIN_READ_FILE_I:         /* V8/H5 (#711): readFile(path, ignorarError), el 285 */
    case BUILTIN_READ_FILE_BYTES: {   /* #247: idéntico — los bytes ya son crudos en heap */
        const char* que = (id == BUILTIN_READ_FILE_BYTES) ? "readFileBytes" : "readFile";   /* #657: el que se llamó */
        int ignorar = (id == BUILTIN_READ_FILE_I) ? (pop_i32(vm, tc) != 0) : 0;
        uint32_t pref = pop_ref(vm, tc);
        char path[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        /* H19-F1 — resuelve relativo al base-dir del proyecto (si lo hay), luego
         * cwd/literal, luego /app (modo plano). Los absolutos no se tocan. */
        char eff[600];
        const char* rpath = bpvm_fs_resolve(path, eff, sizeof(eff));
        bpvm_abierto_breve_t ab;   /* V7 #657: un open para leer, mientras dura */
        if (abrir_breves(vm, &ab, rpath, BPVM_FS_O_READ, NULL, NULL, 0) != 0) {
            char em[576];
            snprintf(em, sizeof(em), "%s('%s'): ya está abierto", que, path);
            return builtin_throw(vm, tc, em);
        }
        uint32_t size = 0;
        if (bpvm_fs_stat(rpath, &size) != 0) {
            char em[576];
            cerrar_breves(vm, &ab, NULL);
            snprintf(em, sizeof(em), "%s('%s'): no se pudo abrir", que, path);
            return builtin_throw(vm, tc, em);
        }
        bpref_t refh = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, size, size); /* V7/H7 #593: la cabecera y el handle, al nacer */
        if (refh.v == 0u) { cerrar_breves(vm, &ab, NULL); return builtin_throw(vm, tc, "readFile: sin memoria"); }
        if (size > 0) {
            long n = bpvm_fs_read(rpath, vm->memory + bpref_deref(vm, refh) + 4, size);
            if (n < 0 || (uint32_t) n != size) {
                char em[576];
                cerrar_breves(vm, &ab, NULL);
                snprintf(em, sizeof(em), "%s('%s'): error de lectura", que, path);
                return builtin_throw(vm, tc, em);
            }
        }
        cerrar_breves(vm, &ab, NULL);
        /* V8/H5 (23-oct, #714) — readFile da una CADENA: nace (su bit ASCII y el de solo lectura), como las demás.
         * readFileBytes da un byte[]: ése no. #711: y se DESCODIFICA (EncodingError, o «�» con el indicador). */
        if (id != BUILTIN_READ_FILE_BYTES) {
            char quien[BPVM_FS_PATH_MAX + 32];   /* «readFileBytes('…')»: el nombre, la ruta y sus comillas */
            snprintf(quien, sizeof quien, "%s('%s')", que, path);
            return nacer_de_fuera(vm, tc, refh, ignorar, quien);
        }
        bpref_push(vm, tc, refh);
        return BPVM_OK;
    }
    case BUILTIN_WRITE_FILE:
    case BUILTIN_WRITE_FILE_BYTES:    /* #247: idéntico — escribe los bytes crudos del array */
    case BUILTIN_APPEND_FILE: {
        int append = (id == BUILTIN_APPEND_FILE);
        const char* que = append ? "appendFile" : (id == BUILTIN_WRITE_FILE_BYTES) ? "writeFileBytes" : "writeFile";   /* #657 */
        uint32_t cref = pop_ref(vm, tc);   /* content (empujado el último) */
        uint32_t pref = pop_ref(vm, tc);   /* path */
        char path[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        uint32_t cad = (cref == 0) ? 0 : bpref_deref(vm, bpref_from_addr(cref));   /* V4: handle→addr */
        uint32_t clen = (cad == 0) ? 0 : bpvm_read_u32_be(vm->memory + cad);
        const uint8_t* cdata = (cad == 0) ? NULL : (vm->memory + cad + 4);
        bpvm_abierto_breve_t ab;   /* V7 #657: un open para escribir, mientras dura */
        if (abrir_breves(vm, &ab, path, append ? BPVM_FS_O_APPEND : BPVM_FS_O_WRITE, NULL, NULL, 0) != 0) {
            char em[576];
            snprintf(em, sizeof(em), "%s('%s'): ya está abierto", que, path);
            return builtin_throw(vm, tc, em);
        }
        int fallo = (bpvm_fs_write(path, cdata, clen, append) != 0);
        cerrar_breves(vm, &ab, NULL);
        if (fallo) {
            char em[576];
            snprintf(em, sizeof(em), "%s('%s'): error de escritura", que, path);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);   /* void → dummy */
        return BPVM_OK;
    }
    /* ── V7/H3 — listDir, que faltaba en la VM-C desde V4 («funciona en el PC y no en el
     * micro», el peor modo de faltar). Los nombres van a la zona de rascar mientras el
     * motor tiene su lock (allí no se puede alojar: el GC cerraría ficheros tomando ese
     * mismo lock), y después se construye el string[] con la raíz temporal en la pila,
     * como `split`. ORDENADOS por bytes: el orden de un listado es del FS, y miVM ordena
     * igual para que el mismo programa imprima lo mismo en el PC y en la placa. */
    case BUILTIN_LIST_DIR: {
        uint32_t pref = pop_ref(vm, tc);
        char path[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0) return builtin_throw(vm, tc, "path demasiado largo");
        /* Un directorio que existe tal cual (mkdir lo crea sin resolver) va tal cual:
         * `bpvm_fs_resolve` pregunta con `stat`, y un directorio no es un fichero. */
        char eff[BPVM_FS_PATH_MAX];
        const char* rpath = bpvm_fs_isdir(path) ? path : bpvm_fs_resolve(path, eff, sizeof(eff));
        /* La zona de rascar COMPARTIDA (#338), con su guardián: se pide y se devuelve. */
        listdir_acc_t acc; acc.cap = bpvm_scratch_capacity(); acc.len = 0; acc.n = 0; acc.desbordado = 0;
        acc.buf = (uint8_t*) zona_para_un_builtin(vm, acc.cap, "LISTDIR");   /* V7/H12, 2b: con dos runs, espera */
        if (!acc.buf) return builtin_throw(vm, tc, "listDir: la zona de rascar está ocupada");
        char em[BPVM_FS_PATH_MAX + 80];
        int rl = bpvm_fs_list(rpath, listdir_cb, &acc);
        if (rl != 0 || acc.desbordado) {
            bpvm_scratch_give("LISTDIR");
            if (rl != 0) snprintf(em, sizeof(em), "listDir('%s'): no se pudo abrir", path);
            else snprintf(em, sizeof(em), "listDir('%s'): demasiadas entradas (más de %u bytes de nombres)", path, (unsigned) acc.cap);
            return builtin_throw(vm, tc, em);
        }
        bpref_t arrh = bpvm_heap_nuevo(vm, BPVM_TYPE_ARRAY_REF, acc.n, acc.n * BPVM_REF_SIZE); /* V7/H7 #593: la cabecera y el handle, al nacer */
        if (arrh.v == 0u) { bpvm_scratch_give("LISTDIR"); return builtin_throw(vm, tc, "listDir: sin memoria"); }
        bpref_push(vm, tc, arrh);          /* raíz temporal: protege los nombres */
        size_t off = 0;
        for (uint32_t i = 0; i < acc.n; i++) {
            size_t l = strlen((const char*) acc.buf + off);
            bpref_t psh = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, (uint32_t) l, (uint32_t) l);   /* V7/H7 #593 */
            if (psh.v == 0u) { bpvm_scratch_give("LISTDIR"); tc->sp -= BPVM_REF_SIZE; return builtin_throw(vm, tc, "listDir: sin memoria"); }
            memcpy(vm->memory + bpref_deref(vm, psh) + 4, acc.buf + off, l);
            bpvm_str_nace(vm, bpref_deref(vm, psh));   /* V8/H5: el bit del nombre */
            bpref_store(vm, bpref_deref(vm, arrh) + BPVM_ARR_DATA_OFF + i * BPVM_REF_SIZE, psh);
            off += l + 1;
        }
        /* inserción: n pequeño, y sin memoria auxiliar */
        for (uint32_t i = 1; i < acc.n; i++) {
            for (uint32_t j = i; j > 0; j--) {
                uint32_t base = bpref_deref(vm, arrh) + BPVM_ARR_DATA_OFF;
                bpref_t a = bpref_load(vm, base + (j - 1) * BPVM_REF_SIZE);
                bpref_t b = bpref_load(vm, base + j * BPVM_REF_SIZE);
                uint32_t la = bpref_arr_len(vm, a), lb = bpref_arr_len(vm, b);
                uint32_t m = (la < lb) ? la : lb;
                int c = memcmp(bpref_arr_elem(vm, a, 0, 1), bpref_arr_elem(vm, b, 0, 1), m);
                if (c < 0 || (c == 0 && la <= lb)) break;
                bpref_store(vm, base + (j - 1) * BPVM_REF_SIZE, b);
                bpref_store(vm, base + j * BPVM_REF_SIZE, a);
            }
        }
        bpvm_scratch_give("LISTDIR");
        tc->sp -= BPVM_REF_SIZE;           /* fuera la raíz temporal */
        bpref_push(vm, tc, arrh);
        return BPVM_OK;
    }
    /* ── V7/H3 (#491) — las nueve primitivas del cursor (IO.File) ── */
    case BUILTIN_FSTATE_SIZE: {
        uint32_t pref = pop_ref(vm, tc);
        char path[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0) return builtin_throw(vm, tc, "path demasiado largo");
        /* La lectura RESUELVE la ruta (basedir → tal cual → /app → packs), como readFile; la
         * escritura la toma tal cual, como writeFile. El bloque tiene que valer para las dos. */
        char eff[BPVM_FS_PATH_MAX];
        uint32_t a = bpvm_fs_file_block_size(bpvm_fs_resolve(path, eff, sizeof(eff)));
        uint32_t b = bpvm_fs_file_block_size(path);
        push_i32(vm, tc, (int32_t) ((a > b) ? a : b));
        return BPVM_OK;
    }
    case BUILTIN_FOPEN: {
        int32_t  mode = pop_i32(vm, tc);
        uint32_t pref = pop_ref(vm, tc);
        uint32_t sref = pop_ref(vm, tc);
        char path[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0) return builtin_throw(vm, tc, "path demasiado largo");
        if (mode < BPVM_FS_O_READ || mode > BPVM_FS_O_RW) return builtin_throw(vm, tc, "File: modo no válido");
        uint32_t cap = 0;
        uint8_t* blk = fblk_de(vm, sref, &cap);
        if (!blk) return builtin_throw(vm, tc, "File: sin bloque de estado");
        if (bpvm_fs_fisopen(blk)) return builtin_throw(vm, tc, "File: el objeto ya tiene un fichero abierto");
        char eff[BPVM_FS_PATH_MAX];
        const char* rpath = (mode == BPVM_FS_O_READ) ? bpvm_fs_resolve(path, eff, sizeof(eff)) : path;   /* como readFile / writeFile */
        char em[BPVM_FS_PATH_MAX + 64];
        /* V7/H12, etapa 2b — mirar si ya está abierto, abrirlo y registrarlo, JUNTOS: con dos runs, dos hilos podían
         * ver libre el mismo fichero y abrirlo los dos para escribir, o pisarse la lista de abiertos. Bajo el cerrojo
         * de la VM (no-op con un run), en el orden de siempre: el de la VM y dentro el del FS (como el GC, que cierra
         * ficheros con el mundo parado). La excepción, ya fuera: lanzarla reserva memoria. */
        int ya, abierto = 0;
        ficheros_cerrar(vm);
        ya = fichero_ya_abierto(vm, rpath, mode);
        if (bpvm_la_cola.ensayo_ficheros) bpvm_platform_thread_yield();   /* SÓLO PRUEBAS: la ventana ensanchada */
        if (!ya && bpvm_fs_fopen(blk, cap, rpath, mode) == 0) {
            fregistrar(vm, sref, (bpvm_fs_file_t*) blk);
            abierto = 1;
        }
        ficheros_abrir(vm);
        if (ya) {
            snprintf(em, sizeof(em), "File('%s', \"%s\"): ya está abierto", path, fmodo_txt(mode));
            return builtin_throw(vm, tc, em);
        }
        if (!abierto) {
            snprintf(em, sizeof(em), "File('%s', \"%s\"): no se pudo abrir", path, fmodo_txt(mode));
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);   /* void → dummy */
        return BPVM_OK;
    }
    case BUILTIN_FCLOSE: {
        uint32_t sref = pop_ref(vm, tc);
        uint8_t* blk = fblk_de(vm, sref, NULL);
        /* V7/H12, 2b (la revisión) — mirar si está abierto, sacarlo de la lista y cerrarlo en el motor, TODO bajo el
         * cerrojo (en el orden de siempre: el de la VM y dentro el del FS). Sacarlo bajo el cerrojo y cerrarlo fuera
         * dejaba una ventana: otro run lo veía libre y lo abría para escribir con el de éste aún sin cerrar —en la SD,
         * FatFs libera sus clusters al abrir para escribir, y el cierre de éste escribía en ellos: la tarjeta,
         * corrompida—. Y dos hilos que cierran el mismo File no lo cierran dos veces. */
        int err = 0;
        ficheros_cerrar(vm);
        if (blk && bpvm_fs_fisopen(blk)) {
            fdesregistrar(vm, sref);
            if (bpvm_la_cola.ensayo_ficheros) bpvm_platform_thread_yield();   /* SÓLO PRUEBAS: la ventana ensanchada */
            err = (bpvm_fs_fclose(blk) != 0);
        }
        ficheros_abrir(vm);
        if (err) return builtin_throw(vm, tc, "File.close(): error al cerrar");
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_FREAD:
    case BUILTIN_FWRITE: {
        int es_lectura = (id == BUILTIN_FREAD);
        int32_t  n    = pop_i32(vm, tc);
        int32_t  off  = pop_i32(vm, tc);
        uint32_t dref = pop_ref(vm, tc);
        uint32_t sref = pop_ref(vm, tc);
        uint8_t* blk = fblk_de(vm, sref, NULL);
        uint32_t dcap = 0;
        uint8_t* d = fblk_de(vm, dref, &dcap);
        /* V7/H12, 2b: lo de siempre, en el mismo orden (los mensajes, los de miVM), bajo el cerrojo de los File */
        const char* fallo = NULL;
        long r = 0;
        ficheros_cerrar(vm);
        if (!blk || !bpvm_fs_fisopen(blk))
            fallo = es_lectura ? "File.read(): fichero cerrado" : "File.write(): fichero cerrado";
        else if (!d)
            fallo = es_lectura ? "File.read(): buffer nulo" : "File.write(): buffer nulo";
        else if (off < 0 || n < 0 || (uint32_t) off > dcap || (uint32_t) n > dcap - (uint32_t) off)
            fallo = es_lectura ? "File.read(): fuera del buffer" : "File.write(): fuera del buffer";
        else if (es_lectura) {
            r = bpvm_fs_fread(blk, d + off, (uint32_t) n);
            if (r < 0) fallo = "File.read(): error de lectura";
        } else if (((bpvm_fs_file_t*) blk)->mode == BPVM_FS_O_READ) {
            fallo = "File.write(): abierto sólo para lectura";
        } else {
            if (bpvm_la_cola.ensayo_ficheros) bpvm_platform_thread_yield();   /* SÓLO PRUEBAS: la ventana ensanchada */
            if (bpvm_fs_fwrite(blk, d + off, (uint32_t) n) < 0) fallo = "File.write(): error de escritura";
        }
        ficheros_abrir(vm);
        if (fallo) return builtin_throw(vm, tc, fallo);
        push_i32(vm, tc, es_lectura ? (int32_t) r : 0);
        return BPVM_OK;
    }
    case BUILTIN_FSEEK: {
        int32_t  pos  = pop_i32(vm, tc);
        uint32_t sref = pop_ref(vm, tc);
        uint8_t* blk = fblk_de(vm, sref, NULL);
        char em[96];
        const char* fallo = NULL;
        ficheros_cerrar(vm);   /* V7/H12, 2b: bajo el cerrojo de los File */
        if (!blk || !bpvm_fs_fisopen(blk)) {
            fallo = "File.seek(): fichero cerrado";
        } else if (pos < 0 || bpvm_fs_fseek(blk, (uint32_t) pos) != 0) {
            snprintf(em, sizeof(em), "File.seek(%ld): fuera del fichero (tamaño %ld)", (long) pos, bpvm_fs_fsize(blk));
            fallo = em;
        }
        ficheros_abrir(vm);
        if (fallo) return builtin_throw(vm, tc, fallo);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_FTELL:
    case BUILTIN_FSIZE: {
        uint32_t sref = pop_ref(vm, tc);
        uint8_t* blk = fblk_de(vm, sref, NULL);
        long v = 0;
        ficheros_cerrar(vm);   /* V7/H12, 2b: bajo el cerrojo de los File */
        int cerrado = (!blk || !bpvm_fs_fisopen(blk));
        if (!cerrado) v = (id == BUILTIN_FTELL) ? bpvm_fs_ftell(blk) : bpvm_fs_fsize(blk);
        ficheros_abrir(vm);
        if (cerrado) return builtin_throw(vm, tc, (id == BUILTIN_FTELL) ? "File.tell(): fichero cerrado" : "File.size(): fichero cerrado");
        push_i32(vm, tc, (int32_t) v);
        return BPVM_OK;
    }
    case BUILTIN_FREAD_LINE:
    case BUILTIN_FREAD_LINE_I: {   /* V8/H5 (#711): el 286, con el indicador de readLine(ignorarError) */
        /* TextFile.readLine en nativo. Dos pasadas sobre el cursor del motor: (1) contar hasta el
         * '\n' (o el fin) leyendo por trozos; (2) volver, leer la línea DIRECTAMENTE en el string
         * y dejar el cursor tras el '\n'. Nada de BP por medio: en la Pico la versión BP costaba
         * 0,9 ms por línea (FileBench). El "\r" final se quita, como hacía la de BP. */
        int ignorar = (id == BUILTIN_FREAD_LINE_I) ? (pop_i32(vm, tc) != 0) : 0;
        uint32_t sref = pop_ref(vm, tc);
        uint8_t* blk = fblk_de(vm, sref, NULL);
        /* V7/H12, 2b — en DOS tramos bajo el cerrojo de los File: contar la línea, y (tras reservar el string, que no
         * puede hacerse con él cogido) leerla. Entre los dos, otro hilo pudo cerrarlo: se vuelve a mirar. */
        const char* fallo = NULL;
        long ini = 0;
        uint8_t tmp[128];
        uint32_t n = 0; int hay_nl = 0; uint8_t ult = 0;
        ficheros_cerrar(vm);
        if (!blk || !bpvm_fs_fisopen(blk)) {
            fallo = "File.readLine(): fichero cerrado";
        } else {
            ini = bpvm_fs_ftell(blk);
            for (;;) {
                long r = bpvm_fs_fread(blk, tmp, sizeof tmp);
                if (r < 0) { fallo = "File.readLine(): error de lectura"; break; }
                if (r == 0) break;
                long k = 0;
                while (k < r && tmp[k] != '\n') k++;
                if (k > 0) ult = tmp[k - 1];
                n += (uint32_t) k;
                if (k < r) { hay_nl = 1; break; }
            }
        }
        ficheros_abrir(vm);
        if (fallo) return builtin_throw(vm, tc, fallo);
        uint32_t len = (n > 0 && ult == '\r') ? n - 1 : n;
        bpref_t refh = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, len, len); /* V7/H7 #593: la cabecera y el handle, al nacer */
        if (refh.v == 0u) return builtin_throw(vm, tc, "File.readLine(): sin memoria");
        uint32_t ref = bpref_deref(vm, refh);
        ficheros_cerrar(vm);
        blk = fblk_de(vm, sref, NULL);   /* el heap no compacta, pero el GC pudo pasar: releer la dirección es gratis */
        if (!blk || !bpvm_fs_fisopen(blk)) {
            fallo = "File.readLine(): fichero cerrado";
        } else if (bpvm_fs_fseek(blk, (uint32_t) ini) != 0) {
            fallo = "File.readLine(): error de lectura";
        } else {
            if (len > 0) {
                long r = bpvm_fs_fread(blk, vm->memory + ref + 4, len);
                if (r < 0 || (uint32_t) r != len) fallo = "File.readLine(): error de lectura";
            }
            if (!fallo && bpvm_fs_fseek(blk, (uint32_t) ini + n + (hay_nl ? 1u : 0u)) != 0)
                fallo = "File.readLine(): error de lectura";
        }
        ficheros_abrir(vm);
        if (fallo) return builtin_throw(vm, tc, fallo);
        /* V8/H5: el bit de la línea, por sus bytes. #711: DESCODIFICADA (EncodingError, o «�» con el indicador); si
         * lanza, la línea queda leída, como en miVM (un bucle con try sigue con la siguiente). */
        return nacer_de_fuera(vm, tc, refh, ignorar, "File.readLine()");
    }
    case BUILTIN_FFLUSH: {
        uint32_t sref = pop_ref(vm, tc);
        uint8_t* blk = fblk_de(vm, sref, NULL);
        const char* fallo = NULL;
        ficheros_cerrar(vm);   /* V7/H12, 2b: bajo el cerrojo de los File */
        if (!blk || !bpvm_fs_fisopen(blk)) fallo = "File.flush(): fichero cerrado";
        else if (bpvm_fs_fflush(blk) != 0) fallo = "File.flush(): error de escritura";
        ficheros_abrir(vm);
        if (fallo) return builtin_throw(vm, tc, fallo);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    /* V8/H4 (27-oct, #708) — EL SONIDO (Sound.bp), siempre en paralelo al programa: lo común está en la HAL BP
     * (bpvm_sonido.c), que toca en su propia tarea y da el error de una placa sin sonido con su motivo; aquí sólo se
     * redactan los textos, los mismos que Sonido.java. La ruta se resuelve como readFile. */
    case BUILTIN_SOUND_PLAY: {
        uint32_t pref = pop_ref(vm, tc);
        /* `em` hace dos cosas, una tras otra, para no agrandar el marco del switch (lo paga cada builtin): primero la
         * ruta resuelta (bpvm_sonido_tocar la copia), y después el texto del error. El más largo, el del formato: 12 + la
         * ruta (255) + 25 + det (127) + 67 + el NUL = 487. */
        char path[BPVM_FS_PATH_MAX], det[128], em[BPVM_FS_PATH_MAX + 240];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        switch (bpvm_sonido_tocar(bpvm_fs_resolve(path, em, sizeof(em)), det, sizeof(det))) {
        case BPVM_SONIDO_OK:
            push_i32(vm, tc, 0);   /* void: el valor que el compilador descarta tras la llamada */
            return BPVM_OK;
        case BPVM_SONIDO_SIN_PLACA:
            snprintf(em, sizeof em, "Sound.play: %s", det);
            break;
        case BPVM_SONIDO_NO_ABRE:
            snprintf(em, sizeof em, "Sound.play('%s'): no se pudo abrir", path);
            break;
        case BPVM_SONIDO_NO_ES_WAV:
            snprintf(em, sizeof em, "Sound.play('%s'): no es un WAV", path);
            break;
        case BPVM_SONIDO_FORMATO:
            snprintf(em, sizeof em, "Sound.play('%s'): formato no admitido (%s); se admite PCM de 8 o 16 bits, mono o "
                     "estereo, de 8000 a 48000 Hz", path, det);
            break;
        default:
            snprintf(em, sizeof em, "Sound.play: no se pudo crear la tarea del sonido");
            break;
        }
        return builtin_throw(vm, tc, em);
    }
    case BUILTIN_SOUND_STOP:
        bpvm_sonido_parar();
        push_i32(vm, tc, 0);   /* void */
        return BPVM_OK;
    case BUILTIN_SOUND_IS_PLAYING:
        push_i32(vm, tc, bpvm_sonido_sonando() ? 1 : 0);
        return BPVM_OK;
    case BUILTIN_SOUND_SET_VOLUME: {
        int32_t v = pop_i32(vm, tc);
        if (v < 0 || v > 100) {
            char em[80];
            snprintf(em, sizeof em, "Sound.setVolume: el volumen va de 0 a 100 (es %" PRId32 ")", v);
            return builtin_throw(vm, tc, em);
        }
        bpvm_sonido_poner_volumen((int) v);
        push_i32(vm, tc, 0);   /* void */
        return BPVM_OK;
    }
    case BUILTIN_SOUND_GET_VOLUME:
        push_i32(vm, tc, bpvm_sonido_volumen());
        return BPVM_OK;
    case BUILTIN_SOUND_IS_AVAILABLE:
        push_i32(vm, tc, bpvm_sonido_hay() ? 1 : 0);
        return BPVM_OK;
    case BUILTIN_SOUND_GET_POSITION:   /* el progreso: bytes del sonido entregados, y los que tiene */
        push_i32(vm, tc, bpvm_sonido_posicion());
        return BPVM_OK;
    case BUILTIN_SOUND_GET_LENGTH:
        push_i32(vm, tc, bpvm_sonido_largo());
        return BPVM_OK;

    case BUILTIN_EOL: {   /* V8 #755 — el fin de línea de la plataforma: "\r\n" en Windows, "\n" aquí y en los micros */
        uint32_t r = bpvm_heap_alloc_string(vm, BPVM_EOL, sizeof(BPVM_EOL) - 1u);
        if (r == 0u) return builtin_throw(vm, tc, "No space in heap");
        bpref_push(vm, tc, bpref_regen(vm, r));
        return BPVM_OK;
    }

    case BUILTIN_FILE_EXISTS: {
        uint32_t pref = pop_ref(vm, tc);
        char path[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        /* [V7/H2, #493] Se RESUELVE como readFile (basedir → tal cual → /app →
         * la zona de packs): «if fileExists(x) then readFile(x)» tiene que ver
         * lo mismo que readFile. Antes pasaba por el disfraz de la zona; la
         * revisión lo cazó con un recurso de un pack: readFile sí, fileExists no. */
        char eff[BPVM_FS_PATH_MAX];
        push_i32(vm, tc, bpvm_fs_exists(bpvm_fs_resolve(path, eff, sizeof(eff))) ? 1 : 0);
        return BPVM_OK;
    }

    /* #240 (logger) — gestión de ficheros (IO.removeFile/rename/fileSize).
     * Mismos contratos que la VM-Java; backend sin la op → RuntimeError
     * atrapable (los firmwares la añaden en su próximo build). */
    case BUILTIN_REMOVE_FILE: {
        uint32_t pref = pop_ref(vm, tc);
        char path[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        bpvm_abierto_breve_t ab;   /* V7 #657: borrar es escribir: un open para escribir, mientras dura */
        if (abrir_breves(vm, &ab, path, BPVM_FS_O_WRITE, NULL, NULL, 0) != 0) {
            char em[576];
            snprintf(em, sizeof(em), "removeFile('%s'): ya está abierto", path);
            return builtin_throw(vm, tc, em);
        }
        int fallo = (bpvm_fs_remove(path) != 0);
        cerrar_breves(vm, &ab, NULL);
        if (fallo) {
            char em[576];
            snprintf(em, sizeof(em), "removeFile('%s'): error al borrar", path);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_RENAME: {
        uint32_t tref = pop_ref(vm, tc);   /* to (empujado el último) */
        uint32_t fref = pop_ref(vm, tc);   /* from */
        char from[BPVM_FS_PATH_MAX], to[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, fref, from, sizeof(from)) != 0 ||
            read_bp_path(vm, tref, to,   sizeof(to))   != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        bpvm_abierto_breve_t a1, a2;   /* V7 #657: los dos se escriben: dos opens para escribir, mientras dura */
        int choca = abrir_breves(vm, &a1, from, BPVM_FS_O_WRITE, &a2, to, BPVM_FS_O_WRITE);
        if (choca != 0) {
            char em[1100];
            snprintf(em, sizeof(em), "rename('%s' -> '%s'): '%s' ya está abierto", from, to, choca == 1 ? from : to);
            return builtin_throw(vm, tc, em);
        }
        int fallo = (bpvm_fs_rename(from, to) != 0);
        cerrar_breves(vm, &a1, &a2);
        if (fallo) {
            char em[1100];
            snprintf(em, sizeof(em), "rename('%s' -> '%s'): error al renombrar", from, to);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_FILE_SIZE: {
        uint32_t pref = pop_ref(vm, tc);
        char path[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        uint32_t sz = 0;
        char eff[BPVM_FS_PATH_MAX];   /* V7/H2 (#493): resuelto como readFile */
        if (bpvm_fs_stat(bpvm_fs_resolve(path, eff, sizeof(eff)), &sz) != 0) {
            char em[576];
            snprintf(em, sizeof(em), "fileSize('%s'): no se pudo leer el tamaño", path);
            return builtin_throw(vm, tc, em);
        }
        if (sz > 0x7FFFFFFFu) {
            char em[576];
            snprintf(em, sizeof(em), "fileSize('%s'): tamaño > 2GB no representable en integer", path);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, (int32_t) sz);
        return BPVM_OK;
    }

    /* #240 (2ª pasada) — resto de IO.bp. En device los backends pueden no
     * implementarlos (FS plano sin directorios) → RuntimeError atrapable. */
    case BUILTIN_MKDIR: {
        uint32_t pref = pop_ref(vm, tc);
        char path[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        if (bpvm_fs_mkdir(path) != 0) {
            char em[576];
            snprintf(em, sizeof(em), "mkdir('%s'): no se pudo crear", path);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_RMDIR: {
        uint32_t pref = pop_ref(vm, tc);
        char path[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        if (bpvm_fs_rmdir(path) != 0) {
            char em[576];
            snprintf(em, sizeof(em), "rmdir('%s'): no se pudo borrar (¿no vacío?)", path);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_COPY_FILE: {
        uint32_t tref = pop_ref(vm, tc);   /* to (empujado el último) */
        uint32_t fref = pop_ref(vm, tc);   /* from */
        char from[BPVM_FS_PATH_MAX], to[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, fref, from, sizeof(from)) != 0 ||
            read_bp_path(vm, tref, to,   sizeof(to))   != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        bpvm_abierto_breve_t a1, a2;   /* V7 #657: el origen se lee y el destino se escribe, mientras dura */
        int choca = abrir_breves(vm, &a1, from, BPVM_FS_O_READ, &a2, to, BPVM_FS_O_WRITE);
        if (choca != 0) {
            char em[1100];
            snprintf(em, sizeof(em), "copyFile('%s' -> '%s'): '%s' ya está abierto", from, to, choca == 1 ? from : to);
            return builtin_throw(vm, tc, em);
        }
        int fallo = (bpvm_fs_copy(from, to) != 0);
        cerrar_breves(vm, &a1, &a2);
        if (fallo) {
            char em[1100];
            snprintf(em, sizeof(em), "copyFile('%s' -> '%s'): error al copiar", from, to);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_IS_DIRECTORY: {
        uint32_t pref = pop_ref(vm, tc);
        char path[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        push_i32(vm, tc, bpvm_fs_isdir(path) ? 1 : 0);   /* sin throw, como Java */
        return BPVM_OK;
    }
    case BUILTIN_LAST_MODIFIED: {
        uint32_t pref = pop_ref(vm, tc);
        char path[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, path, sizeof(path)) != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        long long ms = bpvm_fs_mtime_ms(path);
        if (ms == -2) {
            /* H13 hallazgo 13(a) — la capacidad NO ESTA en esta plataforma, que no
             * es lo mismo que un fallo de lectura: reintentar no va a servir. El
             * texto habla de la PLATAFORMA, no de littlefs, para que no caduque si
             * otra familia lo implementa. */
            char em[576];
            snprintf(em, sizeof(em),
                     "lastModified('%s'): no implementado en esta plataforma", path);
            return builtin_throw(vm, tc, em);
        }
        if (ms < 0) {
            char em[576];
            snprintf(em, sizeof(em), "lastModified('%s'): no se pudo leer", path);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, (int32_t) (ms & 0x7FFFFFFFLL));   /* truncado como Java */
        return BPVM_OK;
    }

    case BUILTIN_THROW_RTE: {
        /* #248 — lanza el RuntimeError NATIVO de la VM con el mensaje dado
         * (mismo path que div0/null deref → atrapable con try/catch BP).
         * Lo usa el compareTo por defecto de Object para no depender del
         * descriptor local de RuntimeError. No retorna valor. */
        uint32_t mref = pop_ref(vm, tc);
        char msg[256];
        read_bp_string(vm, mref, msg, sizeof(msg));
        return builtin_throw(vm, tc, msg);
    }

    case BUILTIN_CHAR_CODE_AT: {
        int32_t i   = pop_i32(vm, tc);
        bpref_t s = bpref_pop(vm, tc);   /* string ref */
        uint32_t sa = bpref_is_null(s) ? 0u : bpref_deref(vm, s);
        int32_t c = bpvm_prim_char_code_at(vm, &tc->strcur, s.v, sa, i);   /* P2b: la primitiva compartida */
        if (c < 0) {   /* V8 #717: fuera de rango, el error de charAt (antes, 0 en silencio) */
            char em[96];
            snprintf(em, sizeof em, "charCodeAt: idx fuera de rango %" PRId32 " (len=%d)", i,
                     (int) bpvm_prim_str_len(vm, &tc->strcur, s.v, sa));
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, c);
        return BPVM_OK;
    }

    case BUILTIN_CHAR_AT: {
        /* charAt(str, idx): string  — devuelve un string de 1 char con
         * el codepoint en esa posición. Si idx fuera de rango, RuntimeError
         * (BUG-7b; antes decía «string vacía», y así se quedó la copia del
         * AOT hasta #569). */
        int32_t i    = pop_i32(vm, tc);
        bpref_t s = bpref_pop(vm, tc);   /* string ref */
        {   /* P2b: la primitiva compartida (con el helper string_char_at del AOT) */
            bpref_t h; char em[96];
            if (bpvm_prim_char_at(vm, &tc->strcur, s.v, bpref_deref(vm, s), i, &h, em, sizeof em) != 0)
                return builtin_throw(vm, tc, em);
            bpref_push(vm, tc, h);
            return BPVM_OK;
        }
    }

    case BUILTIN_SUBSTRING: {
        /* substring(s, start, end): string — copia [start, end) con
         * clamp estilo BP. Idéntico a aot_helpers h_string_substring
         * para que intérprete y AOT den el mismo resultado (#173). */
        int32_t end   = pop_i32(vm, tc);
        int32_t start = pop_i32(vm, tc);
        /* #350 — PEEK, no pop: el bucle de copia de abajo sigue leyendo de `s`
         * DESPUÉS de alocar el resultado, y una reserva puede colectar. Con pop,
         * `s` queda por encima de sp y el marcado conservador no la ve: si el
         * llamante no la tenía además en un local (p.ej. substring(a+b, ...)),
         * el GC se la lleva EN VIVO. Medido: 3.000 vueltas con el heap estrecho
         * daban entre 3 y 6 resultados malos, y subían al encoger el heap. */
        bpref_t s = peek_ref(vm, tc, 0);
        {   /* P2b: la primitiva compartida (con el helper string_substring del AOT); el origen sigue en la pila
             * mientras reserva (#350) y sólo se saca después */
            bpref_t h; char em[64];
            int r = bpvm_prim_substring(vm, &tc->strcur, s, start, end, &h, em, sizeof em);
            tc->sp -= BPVM_REF_SIZE;
            if (r != 0) return builtin_throw(vm, tc, em);
            bpref_push(vm, tc, h);
            return BPVM_OK;
        }
    }

    /* ================================================================
     * #348 — CADENAS DE CÓMPUTO PURO. Estaban sólo en miVM.
     *
     * Semánticas MEDIDAS contra Java (sonda con codepoints y longitudes
     * reales), no deducidas — los tres corners que importan:
     *   split(",a,,b,", ",") -> 5 partes, conserva vacíos delante y detrás
     *   split("abc", "")     -> 4: "a" "b" "c" ""
     *   split("", cualquiera)-> 1: ""
     *   replace("ab", "", "-") -> "-a-b-"
     *   trim quita <= U+0020 (NO quita U+00A0)
     *
     * TODAS leen sus argumentos con peek_ref y los sacan AL FINAL: ver el
     * comentario de peek_ref sobre por qué un pop antes de alocar es un
     * agujero de GC.
     * ================================================================ */

    case BUILTIN_UPPER:
    case BUILTIN_LOWER: {
        /* Latin-1 (ver latin1_upper_cp): la longitud en BYTES no cambia, así
         * que el resultado se aloca del tamaño exacto del origen. */
        int up = (id == BUILTIN_UPPER);
        bpref_t s = peek_ref(vm, tc, 0);
        uint32_t n = bpref_is_null(s) ? 0 : bpref_arr_len(vm, s);
        bpref_t outh = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, n, n); /* V7/H7 #593: la cabecera y el handle, al nacer */
        if (outh.v == 0u)   /* #355 (2a pasada del censo): la reserva fallo. Antes se
             * saltaba el bloque de abajo y se empujaba out=0 —una ref NULA— sin
             * decir nada: el programa comparaba una cadena vacia, no cuadraba, y
             * el fallo aparecia diez pasos mas alla disfrazado de dato malo.
             * REPRODUCIDO en el PC con --nogc: "if1" salia ANTES del throw. */
            return builtin_throw(vm, tc, "No space in heap");
        uint32_t out = bpref_deref(vm, outh);
        {   /* la reserva ya esta comprobada arriba */
            const uint8_t* p = (n > 0) ? bpref_arr_elem(vm, s, 0, 1) : NULL;
            if (bpvm_str_ascii(vm, bpref_deref(vm, s))) {   /* V8/H5: el rápido: en ASCII sólo cambian a..z / A..Z */
                for (uint32_t i = 0; i < n; i++) {
                    uint8_t c = p[i];
                    if (up ? (c >= 'a' && c <= 'z') : (c >= 'A' && c <= 'Z')) c = (uint8_t) (c ^ 0x20u);
                    vm->memory[out + 4 + i] = c;
                }
                bpvm_str_nace_ascii(vm, out, 1);
            } else {
                uint32_t i = 0, o = 0;
                while (i < n) {
                    uint32_t adv = 0;
                    uint32_t cp  = utf8_decode(p + i, n - i, &adv);
                    uint32_t m   = up ? latin1_upper_cp(cp) : latin1_lower_cp(cp);
                    uint8_t enc[4];
                    uint32_t el = utf8_encode(m, enc);
                    for (uint32_t k = 0; k < el && o < n; k++) vm->memory[out + 4 + o++] = enc[k];
                    i += (adv > 0) ? adv : 1;
                }
                if (o < n) memset(vm->memory + out + 4 + o, 0, n - o);   /* #736: lo no escrito, a cero, como antes */
                bpvm_str_nace(vm, out);   /* V8/H5: el bit, por lo que salió */
            }
            out = (uint32_t) outh.v;
        }
        tc->sp -= BPVM_REF_SIZE;
        push_ref(vm, tc, out);
        return BPVM_OK;
    }

    case BUILTIN_TRIM: {
        /* Java String.trim(): quita los caracteres <= U+0020 de los dos
         * extremos. A nivel de byte es exacto — ningún byte de continuación
         * UTF-8 es <= 0x20, así que no se puede cortar un carácter por medio. */
        bpref_t s = peek_ref(vm, tc, 0);
        uint32_t n = bpref_is_null(s) ? 0 : bpref_arr_len(vm, s);
        uint32_t b = 0, e = n;
        if (n > 0) {
            const uint8_t* p = bpref_arr_elem(vm, s, 0, 1);
            while (b < e && p[b] <= 0x20) b++;
            while (e > b && p[e - 1] <= 0x20) e--;
        }
        uint32_t len = e - b;
        bpref_t outh = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, len, len); /* V7/H7 #593: la cabecera y el handle, al nacer */
        if (outh.v == 0u)   /* #355 (2a pasada del censo): la reserva fallo. Antes se
             * saltaba el bloque de abajo y se empujaba out=0 —una ref NULA— sin
             * decir nada: el programa comparaba una cadena vacia, no cuadraba, y
             * el fallo aparecia diez pasos mas alla disfrazado de dato malo.
             * REPRODUCIDO en el PC con --nogc: "if1" salia ANTES del throw. */
            return builtin_throw(vm, tc, "No space in heap");
        uint32_t out = bpref_deref(vm, outh);
        {   /* la reserva ya esta comprobada arriba */
            if (len > 0) memcpy(vm->memory + out + 4, bpref_arr_elem(vm, s, b, 1), len);   /* V8/H5: copia de bytes */
            if (bpvm_str_ascii(vm, bpref_deref(vm, s))) bpvm_str_nace_ascii(vm, out, 1);   /* V8/H5 */
            else bpvm_str_nace(vm, out);
            out = (uint32_t) outh.v;
        }
        tc->sp -= BPVM_REF_SIZE;
        push_ref(vm, tc, out);
        return BPVM_OK;
    }

    case BUILTIN_INDEX_OF: {
        /* Devuelve el índice en CODEPOINTS (no en bytes), para ser coherente
         * con charAt/substring. Se busca por bytes y se convierte al final. */
        bpref_t q = peek_ref(vm, tc, 0);          /* aguja */
        bpref_t s = peek_ref(vm, tc, 1);          /* pajar */
        uint32_t sn = bpref_is_null(s) ? 0 : bpref_arr_len(vm, s);
        uint32_t qn = bpref_is_null(q) ? 0 : bpref_arr_len(vm, q);
        const uint8_t* sp_ = (sn > 0) ? bpref_arr_elem(vm, s, 0, 1) : (const uint8_t*) "";
        const uint8_t* qp_ = (qn > 0) ? bpref_arr_elem(vm, q, 0, 1) : (const uint8_t*) "";
        long off = bp_find_bytes(sp_, sn, qp_, qn, 0);
        int32_t res = (off < 0) ? -1
                    : bpvm_str_ascii(vm, bpref_deref(vm, s)) ? (int32_t) off   /* V8/H5: el rápido, el byte ES el índice */
                    : bpvm_prim_str_cuenta_hasta(vm, &tc->strcur, s.v, bpref_deref(vm, s), (uint32_t) off);
                      /* V8/H5: el lento cuenta desde el cursor y lo deja en lo encontrado (el substring de después) */
        tc->sp -= 2 * BPVM_REF_SIZE;
        push_i32(vm, tc, res);
        return BPVM_OK;
    }

    case BUILTIN_STARTS_WITH:
    case BUILTIN_ENDS_WITH:
    case BUILTIN_CONTAINS: {
        bpref_t q = peek_ref(vm, tc, 0);
        bpref_t s = peek_ref(vm, tc, 1);
        uint32_t sn = bpref_is_null(s) ? 0 : bpref_arr_len(vm, s);
        uint32_t qn = bpref_is_null(q) ? 0 : bpref_arr_len(vm, q);
        const uint8_t* sp_ = (sn > 0) ? bpref_arr_elem(vm, s, 0, 1) : (const uint8_t*) "";
        const uint8_t* qp_ = (qn > 0) ? bpref_arr_elem(vm, q, 0, 1) : (const uint8_t*) "";
        int r;
        if (id == BUILTIN_STARTS_WITH)      r = (qn <= sn) && memcmp(sp_, qp_, qn) == 0;
        else if (id == BUILTIN_ENDS_WITH)   r = (qn <= sn) && memcmp(sp_ + (sn - qn), qp_, qn) == 0;
        else                                r = bp_find_bytes(sp_, sn, qp_, qn, 0) >= 0;
        tc->sp -= 2 * BPVM_REF_SIZE;
        push_i32(vm, tc, r ? 1 : 0);
        return BPVM_OK;
    }

    case BUILTIN_STR_COMPARE: {
        /* V8/H5 (21-oct) — COMPARAR DOS CADENAS POR BYTES, en C (Eduardo: «hazlo en C y a ver que tal»). memcmp compara
         * los bytes sin signo (0..255) y, si una es prefijo de la otra, la más corta va antes: -1/0/1. En UTF-8 ese es el
         * orden de los caracteres (V8_IDEAS §E5, «Comparar por bytes»). null es "", como en Str.compare. Lo usan
         * Str.compare, los <, <=, >, >= entre cadenas (#713) y el contenido de == (__strequals, que escribe el
         * compilador). Las dos formas de una cadena —handle del heap o literal crudo— las resuelve bpref_arr_*. Espejo
         * de STR_COMPARE de miVM. */
        bpref_t b = peek_ref(vm, tc, 0);
        bpref_t a = peek_ref(vm, tc, 1);
        uint32_t na = bpref_is_null(a) ? 0 : bpref_arr_len(vm, a);
        uint32_t nb = bpref_is_null(b) ? 0 : bpref_arr_len(vm, b);
        uint32_t n = (na < nb) ? na : nb;
        int r = (n > 0u) ? memcmp(bpref_arr_elem(vm, a, 0, 1), bpref_arr_elem(vm, b, 0, 1), n) : 0;
        if (r == 0) r = (na < nb) ? -1 : (na > nb) ? 1 : 0;
        tc->sp -= 2 * BPVM_REF_SIZE;
        push_i32(vm, tc, (r < 0) ? -1 : (r > 0) ? 1 : 0);
        return BPVM_OK;
    }

    case BUILTIN_STR_NACE: {
        /* V8/H5 (23-oct, #714) — LA CADENA NACE en el byte[] que se le da, en su sitio y sin copiar: el bloque queda de
         * SOLO LECTURA. Lo usan el StringBuilder (toString) y los __strconcat que escribe el compilador, con un byte[]
         * suyo que muere ahí (el bit ASCII ya lo dejó __strCopy). No es pública: un programa no la ve. La referencia se
         * queda en la pila: es el resultado. Espejo de STR_NACE de miVM, con los mismos textos. */
        bpref_t r = peek_ref(vm, tc, 0);
        if (bpref_is_null(r)) return builtin_throw(vm, tc, "strNace: es null");
        uint32_t cab = bpref_deref(vm, r) - 4u;
        if (cab < vm->heap_start || cab >= vm->heap_next) return builtin_throw(vm, tc, "strNace: no es ref a heap");
        uint32_t tag = bpvm_read_u32_be(vm->memory + cab);
        if ((tag & BPVM_TAG_FREE_BIT) || ((tag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT) != BPVM_TYPE_ARRAY_I8)
            return builtin_throw(vm, tc, "strNace: no es un array de bytes");
        /* V8 #739 — el aviso, como en bpvm_str_nace_ascii: una que empieza por un byte de continuación no puede nacer */
        if (bpvm_read_u32_be(vm->memory + cab + 4u) > 0u && BP_CONT(vm->memory[cab + 8u]))
            bpvm_str_mal_nacida(vm->memory + cab + 8u, bpvm_read_u32_be(vm->memory + cab + 4u));
        bpvm_write_u32_be(vm->memory + cab, tag | BPVM_TAG_SOLO_LECTURA);
        return BPVM_OK;
    }

    case BUILTIN_REPLACE: {
        /* replace(s, target, rep): literal, TODAS las ocurrencias.
         * Target vacío = el corner de Java: "ab" -> "-a-b-", o sea el
         * reemplazo se mete en cada frontera de carácter Y al final. */
        bpref_t rp = peek_ref(vm, tc, 0);
        bpref_t tg = peek_ref(vm, tc, 1);
        bpref_t s  = peek_ref(vm, tc, 2);
        uint32_t sn = bpref_is_null(s)  ? 0 : bpref_arr_len(vm, s);
        uint32_t tn = bpref_is_null(tg) ? 0 : bpref_arr_len(vm, tg);
        uint32_t rn = bpref_is_null(rp) ? 0 : bpref_arr_len(vm, rp);
        const uint8_t* sb = (sn > 0) ? bpref_arr_elem(vm, s,  0, 1) : (const uint8_t*) "";
        const uint8_t* tb = (tn > 0) ? bpref_arr_elem(vm, tg, 0, 1) : (const uint8_t*) "";

        /* 1ª pasada: contar ocurrencias para saber el tamaño exacto. */
        uint32_t hits = 0;
        if (tn == 0) {
            hits = utf8_cp_count(sb, sn) + 1;          /* una por frontera + la final */
        } else {
            for (uint32_t i = 0; i + tn <= sn; ) {
                if (memcmp(sb + i, tb, tn) == 0) { hits++; i += tn; } else i++;
            }
        }
        uint32_t outn = sn - hits * tn + hits * rn;
        bpref_t outh = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, outn, outn); /* V7/H7 #593: la cabecera y el handle, al nacer */
        if (outh.v == 0u)   /* #355 (2a pasada del censo): la reserva fallo. Antes se
             * saltaba el bloque de abajo y se empujaba out=0 —una ref NULA— sin
             * decir nada: el programa comparaba una cadena vacia, no cuadraba, y
             * el fallo aparecia diez pasos mas alla disfrazado de dato malo.
             * REPRODUCIDO en el PC con --nogc: "if1" salia ANTES del throw. */
            return builtin_throw(vm, tc, "No space in heap");
        uint32_t out = bpref_deref(vm, outh);
        {   /* la reserva ya esta comprobada arriba */
            /* 2ª pasada: copiar. Los punteros se re-piden tras la alocación. */
            sb = (sn > 0) ? bpref_arr_elem(vm, s,  0, 1) : (const uint8_t*) "";
            tb = (tn > 0) ? bpref_arr_elem(vm, tg, 0, 1) : (const uint8_t*) "";
            const uint8_t* rb = (rn > 0) ? bpref_arr_elem(vm, rp, 0, 1) : (const uint8_t*) "";
            /* V8/H5 (Eduardo: «Copiar un string debería ser igual con caracteres especiales, se puede hacer como una
             * copia de bytes»): cada trozo, de una vez, con memcpy; el recorrido de la búsqueda es el de siempre. */
            uint8_t* dst = vm->memory + out + 4;
            uint32_t o = 0, i = 0;
            if (tn == 0) {
                if (rn > 0) { memcpy(dst + o, rb, rn); o += rn; }
                while (i < sn) {
                    uint32_t adv = 0;
                    (void) utf8_decode(sb + i, sn - i, &adv);
                    if (adv == 0) adv = 1;
                    memcpy(dst + o, sb + i, adv); o += adv;
                    if (rn > 0) { memcpy(dst + o, rb, rn); o += rn; }
                    i += adv;
                }
            } else {
                uint32_t seg = 0;   /* dónde empieza el trozo sin coincidencias que aún no se ha copiado */
                while (i + tn <= sn) {
                    if (memcmp(sb + i, tb, tn) == 0) {
                        if (i > seg) { memcpy(dst + o, sb + seg, i - seg); o += i - seg; }
                        if (rn > 0) { memcpy(dst + o, rb, rn); o += rn; }
                        i += tn;
                        seg = i;
                    } else {
                        i++;
                    }
                }
                if (sn > seg) { memcpy(dst + o, sb + seg, sn - seg); o += sn - seg; }
            }
            bpvm_str_nace(vm, out);   /* V8/H5: el bit, por lo que salió */
            out = (uint32_t) outh.v;
        }
        tc->sp -= 3 * BPVM_REF_SIZE;
        push_ref(vm, tc, out);
        return BPVM_OK;
    }

    case BUILTIN_SPLIT: {
        /* split(s, sep) -> string[]. Semántica de Java con límite -1: conserva
         * los vacíos de los dos extremos. Casos medidos arriba.
         *
         * ORDEN DE ALOCACIÓN: primero el ARRAY y se EMPUJA como raíz; después
         * cada trozo, guardándolo en el array nada más crearlo. Así, si una
         * alocación dispara GC, los trozos ya creados están dentro de un array
         * que sí es raíz. (miVM lo hace al revés — sus refs viven en un array
         * Java que el GC de BP no escanea. Mismo QUÉ, distinto CÓMO.) */
        bpref_t sep = peek_ref(vm, tc, 0);
        bpref_t s   = peek_ref(vm, tc, 1);
        uint32_t sn = bpref_is_null(s)   ? 0 : bpref_arr_len(vm, s);
        uint32_t pn = bpref_is_null(sep) ? 0 : bpref_arr_len(vm, sep);
        const uint8_t* sb = (sn > 0) ? bpref_arr_elem(vm, s,   0, 1) : (const uint8_t*) "";
        const uint8_t* pb = (pn > 0) ? bpref_arr_elem(vm, sep, 0, 1) : (const uint8_t*) "";

        /* Contar partes (y de paso decidir la forma). */
        uint32_t nparts;
        if (sn == 0)      nparts = 1;                        /* "" -> [""] siempre */
        else if (pn == 0) nparts = utf8_cp_count(sb, sn) + 1;/* "abc" -> a,b,c,"" */
        else {
            nparts = 1;
            for (uint32_t i = 0; i + pn <= sn; ) {
                if (memcmp(sb + i, pb, pn) == 0) { nparts++; i += pn; } else i++;
            }
        }

        bpref_t arrh = bpvm_heap_nuevo(vm, BPVM_TYPE_ARRAY_REF, nparts, nparts * BPVM_REF_SIZE); /* V7/H7 #593: la cabecera y el handle, al nacer */
        if (arrh.v == 0u) return builtin_throw(vm, tc, "No space in heap");
        bpref_push(vm, tc, arrh);          /* raíz temporal: protege los trozos */

        uint32_t done = 0;
        uint32_t i = 0;
        while (done < nparts) {
            /* Recalcular punteros: entre trozo y trozo hay alocaciones. */
            sb = (sn > 0) ? bpref_arr_elem(vm, s,   0, 1) : (const uint8_t*) "";
            pb = (pn > 0) ? bpref_arr_elem(vm, sep, 0, 1) : (const uint8_t*) "";
            uint32_t b = i, e;
            if (sn == 0)      { e = 0; i = 0; }
            else if (pn == 0) {
                uint32_t adv = 0;
                if (i < sn) { (void) utf8_decode(sb + i, sn - i, &adv); if (adv == 0) adv = 1; }
                e = (i < sn) ? i + adv : i;
                i = e;
            } else {
                long off = bp_find_bytes(sb, sn, pb, pn, i);
                if (off < 0 || done == nparts - 1) { e = sn; i = sn; }
                else { e = (uint32_t) off; i = e + pn; }
            }
            uint32_t plen = e - b;
            bpref_t psh = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, plen, plen);   /* V7/H7 #593 */
            if (psh.v == 0u) { tc->sp -= BPVM_REF_SIZE; return builtin_throw(vm, tc, "No space in heap"); }
            uint32_t ps = bpref_deref(vm, psh);
            if (plen > 0) memcpy(vm->memory + ps + 4, bpref_arr_elem(vm, s, b, 1), plen);   /* V8/H5: copia de bytes */
            if (bpvm_str_ascii(vm, bpref_deref(vm, s))) bpvm_str_nace_ascii(vm, ps, 1);   /* V8/H5: trozo de ASCII */
            else bpvm_str_nace(vm, ps);
            bpref_store(vm, bpref_deref(vm, arrh) + BPVM_ARR_DATA_OFF + done * BPVM_REF_SIZE, psh);
            done++;
        }

        tc->sp -= BPVM_REF_SIZE;           /* fuera la raíz temporal */
        tc->sp -= 2 * BPVM_REF_SIZE;       /* fuera los dos argumentos */
        bpref_push(vm, tc, arrh);
        return BPVM_OK;
    }

    /* ================================================================
     * #348 tanda 2 — MATEMÁTICAS DE CÓMPUTO PURO.
     *
     * REGLA que las gobierna todas: calcular en DOUBLE y estrechar a float AL
     * FINAL. miVM lo hace asi porque los Math.* de Java toman double y el
     * resultado se castea a (float). Usar sqrtf/sinf/... daria otro ultimo bit
     * en algunos valores y la paridad se rompe donde nadie mira.
     * ================================================================ */

    case BUILTIN_SQRT: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) sqrt((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_POW: {
        float e = bits_to_f32(pop_i32(vm, tc));     /* exponente arriba */
        float b = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) pow((double) b, (double) e)));
        return BPVM_OK;
    }
    case BUILTIN_LOG: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) log((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_LOG10: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) log10((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_EXP: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) exp((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_SIN: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) sin((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_COS: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) cos((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_TAN: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) tan((double) x)));
        return BPVM_OK;
    }
    /* #347 — random / randomInt.
     *
     * NO son cómputo puro (por eso #348 los dejó fuera): la entropía la pone la
     * familia, en bpvm_platform_random_u32(). Lo de aquí es el MOTOR, y es
     * común a todas a propósito — si cada cintura hiciera su propia conversión,
     * el mismo programa daría distribuciones distintas según la placa.
     *
     * `random()` = [0, 1), como el RND de toda la vida. Se construye dividiendo
     * por 2^24 (no por 2^32) porque el float de 32 bits sólo tiene 24 bits de
     * mantisa: usar los 32 sólo añadiría bits que el float no puede representar
     * y, al redondear hacia arriba, podría devolver 1.0 exacto — rompiendo el
     * "menor que 1" justo en el borde, que es el bicho clásico de esta rutina. */
    case BUILTIN_RANDOM: {
        uint32_t r = bpvm_platform_random_u32() >> 8;      /* 24 bits */
        push_i32(vm, tc, f32_to_bits((float) r / 16777216.0f));
        return BPVM_OK;
    }
    /* `randomInt(min, max)` NO es un generador aparte: es `random()` escalado,
     * que es como se ha hecho siempre en BASIC (`INT(RND*n)`). De ahí sale solo
     * que el intervalo es [min, max) — el alto NO entra:
     *
     *     int(random() * (max - min)) + min
     *
     * Definirlo como la fórmula, y no como "un entero entre a y b", quita de en
     * medio la ambigüedad de si el extremo entra: no hay dos conceptos, hay uno.
     * Y hace que `randomInt(0, len(a))` sea SIEMPRE un índice válido, que es
     * para lo que se usa el 90% de las veces.
     *
     * max <= min devuelve min (rango vacío). Es la respuesta definida y ambas
     * VMs la comparten; lanzar aquí obligaría a envolver en try/catch un uso
     * corriente, y devolver basura sería peor que las dos cosas. */
    case BUILTIN_RANDOM_INT: {
        int32_t hi = pop_i32(vm, tc);
        int32_t lo = pop_i32(vm, tc);
        if (hi <= lo) { push_i32(vm, tc, lo); return BPVM_OK; }
        uint32_t r    = bpvm_platform_random_u32() >> 8;   /* 24 bits */
        float    frac = (float) r / 16777216.0f;           /* [0,1) */
        /* La resta va en 64 bits: hi-lo con hi=INT_MAX y lo=INT_MIN se sale de
         * int32 y daría un ancho negativo. */
        int64_t span = (int64_t) hi - (int64_t) lo;
        int64_t off  = (int64_t) ((double) frac * (double) span);
        if (off >= span) off = span - 1;   /* red por si el redondeo toca el techo */
        push_i32(vm, tc, (int32_t) ((int64_t) lo + off));
        return BPVM_OK;
    }
    case BUILTIN_ASIN: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) asin((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_ACOS: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) acos((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_ATAN: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) atan((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_ATAN2: {
        /* pila (abajo→arriba): y, x. El pop devuelve primero el de arriba. */
        float x = bits_to_f32(pop_i32(vm, tc));
        float y = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) atan2((double) y, (double) x)));
        return BPVM_OK;
    }
    case BUILTIN_PI: {
        /* El double más cercano a pi, luego a f32 — igual que (float) Math.PI. */
        push_i32(vm, tc, f32_to_bits((float) 3.14159265358979323846));
        return BPVM_OK;
    }
    case BUILTIN_E: {
        push_i32(vm, tc, f32_to_bits((float) 2.7182818284590452354));
        return BPVM_OK;
    }
    case BUILTIN_FLOOR: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, java_d2i(floor((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_CEIL: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, java_d2i(ceil((double) x)));
        return BPVM_OK;
    }
    case BUILTIN_ROUND: {
        /* Math.round(float) de Java: el int más cercano, con los empates hacia
         * +infinito — o sea floor(x + 0.5f). NaN → 0 y saturación las pone
         * java_d2i. Ojo: el +0.5 va en FLOAT, como en Java, no en double. */
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, java_d2i(floor((double)(x + 0.5f))));
        return BPVM_OK;
    }
    case BUILTIN_SIGN_I: {
        int32_t x = pop_i32(vm, tc);
        push_i32(vm, tc, (x > 0) - (x < 0));       /* Integer.compare(x,0) */
        return BPVM_OK;
    }
    case BUILTIN_SIGN_F: {
        float x = bits_to_f32(pop_i32(vm, tc));
        int32_t r;
        if (x != x)      r = 0;                    /* convención de miVM: NaN → 0 */
        else if (x > 0)  r = 1;
        else if (x < 0)  r = -1;
        else             r = 0;                    /* ±0 */
        push_i32(vm, tc, r);
        return BPVM_OK;
    }
    /* V6/L1 — los cuatro nuevos de Math. Gemelos EXACTOS de los de
     * VirtualMachine.java: mismas operaciones, mismo orden, cuenta en `double`
     * y estrechado a f32 al final. El orden no es estilo — es el invariante. */
    case BUILTIN_CLAMP_F: {
        /* pila (abajo→arriba): x, lo, hi. El pop devuelve primero el de arriba. */
        double hi = (double) bits_to_f32(pop_i32(vm, tc));
        double lo = (double) bits_to_f32(pop_i32(vm, tc));
        double x  = (double) bits_to_f32(pop_i32(vm, tc));
        /* NaN: las dos comparaciones son falsas y sale NaN, igual que en Java. */
        double r = (x < lo) ? lo : ((x > hi) ? hi : x);
        push_i32(vm, tc, f32_to_bits((float) r));
        return BPVM_OK;
    }
    case BUILTIN_WRAP_F: {
        double hi = (double) bits_to_f32(pop_i32(vm, tc));
        double lo = (double) bits_to_f32(pop_i32(vm, tc));
        double x  = (double) bits_to_f32(pop_i32(vm, tc));
        double r  = hi - lo;
        if (!(r > 0)) {
            /* Sin acentos y SIN NÚMEROS en el texto: `%g` de C y
             * String.valueOf(double) de Java no dan la misma cadena, y este
             * mensaje sale por stdout. */
            return builtin_throw(vm, tc,
                "wrap: el rango (hi - lo) tiene que ser mayor que cero");
        }
        double m = fmod(x - lo, r);      /* == el `%` de Java sobre double */
        if (m < 0) m += r;
        push_i32(vm, tc, f32_to_bits((float) (lo + m)));
        return BPVM_OK;
    }
    case BUILTIN_HYPOT_F: {
        double y = (double) bits_to_f32(pop_i32(vm, tc));
        double x = (double) bits_to_f32(pop_i32(vm, tc));
        /* A propósito NO se llama a hypot() de libm: es otro algoritmo que el
         * Math.hypot() de Java y podrían diferir. Con entradas f32 el cuadrado
         * no desborda un double (f32 máx ≈ 3.4e38 → 1.2e77 contra 1.8e308). */
        push_i32(vm, tc, f32_to_bits((float) sqrt(x * x + y * y)));
        return BPVM_OK;
    }
    case BUILTIN_REMAP_F: {
        double outHi = (double) bits_to_f32(pop_i32(vm, tc));
        double outLo = (double) bits_to_f32(pop_i32(vm, tc));
        double inHi  = (double) bits_to_f32(pop_i32(vm, tc));
        double inLo  = (double) bits_to_f32(pop_i32(vm, tc));
        double x     = (double) bits_to_f32(pop_i32(vm, tc));
        double d = inHi - inLo;
        if (d == 0) {
            return builtin_throw(vm, tc,
                "remap: el rango de entrada es vacio (inHi == inLo)");
        }
        push_i32(vm, tc, f32_to_bits(
            (float) (outLo + (x - inLo) * (outHi - outLo) / d)));
        return BPVM_OK;
    }
    case BUILTIN_FACTORIAL_I: {
        int32_t n = pop_i32(vm, tc);
        char msg[80];
        if (n < 0) {
            snprintf(msg, sizeof msg, "factorial: argumento negativo (%d)", (int) n);
            return builtin_throw(vm, tc, msg);
        }
        if (n > 12) {   /* 13! desborda i32 con signo */
            snprintf(msg, sizeof msg, "factorial: %d desborda integer (máx 12)", (int) n);
            return builtin_throw(vm, tc, msg);
        }
        int32_t r = 1;
        for (int32_t i = 2; i <= n; i++) r *= i;
        push_i32(vm, tc, r);
        return BPVM_OK;
    }
    case BUILTIN_GAMMA_F: {
        float x = bits_to_f32(pop_i32(vm, tc));
        push_i32(vm, tc, f32_to_bits((float) lanczos_gamma((double) x)));
        return BPVM_OK;
    }

    /* ================================================================
     * #348 tanda 3 — RUTAS. Siempre con '/'.
     *
     * miVM las hacía con java.nio.file.Paths, que es DEPENDIENTE DE
     * PLATAFORMA: en Windows pathJoin("a","b") daba "a\b" — o sea rutas que el
     * FS del dispositivo no entiende, y ademas distintas segun el host donde
     * corriera. Mismo patron que el locale de upper/lower.
     *
     * La semantica se DEFINE aqui (y en miVM igual), no se hereda:
     *   join(a,b)  a vacio -> b; b vacio -> a; si no, a sin '/' finales + '/' +
     *              b sin '/' iniciales.  ("/" + "x" -> "/x")
     *   parent(p)  se ignoran los '/' finales; luego hasta el ultimo '/' sin
     *              incluirlo. Sin '/' -> "". Si el ultimo esta en 0 -> "/".
     *   basename(p) se ignoran los '/' finales; luego desde el ultimo '/'.
     *   extension(p) sobre el basename: ultimo '.'; si esta en 0 o al final -> "".
     * ================================================================ */

    case BUILTIN_PATH_JOIN: {
        bpref_t b = peek_ref(vm, tc, 0);
        bpref_t a = peek_ref(vm, tc, 1);
        uint32_t an = bpref_is_null(a) ? 0 : bpref_arr_len(vm, a);
        uint32_t bn = bpref_is_null(b) ? 0 : bpref_arr_len(vm, b);
        const uint8_t* ap = (an > 0) ? bpref_arr_elem(vm, a, 0, 1) : (const uint8_t*) "";
        const uint8_t* bp = (bn > 0) ? bpref_arr_elem(vm, b, 0, 1) : (const uint8_t*) "";
        uint32_t ae = an;                       while (ae > 0 && ap[ae-1] == '/') ae--;
        uint32_t bs = 0;                        while (bs < bn && bp[bs] == '/') bs++;
        uint32_t out_n;
        if (an == 0)      out_n = bn;           /* a vacio -> b tal cual */
        else if (bn == 0) out_n = an;           /* b vacio -> a tal cual */
        else              out_n = ae + 1 + (bn - bs);
        bpref_t outh = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, out_n, out_n); /* V7/H7 #593: la cabecera y el handle, al nacer */
        if (outh.v == 0u)   /* #355 (2a pasada del censo): la reserva fallo. Antes se
             * saltaba el bloque de abajo y se empujaba out=0 —una ref NULA— sin
             * decir nada: el programa comparaba una cadena vacia, no cuadraba, y
             * el fallo aparecia diez pasos mas alla disfrazado de dato malo.
             * REPRODUCIDO en el PC con --nogc: "if1" salia ANTES del throw. */
            return builtin_throw(vm, tc, "No space in heap");
        uint32_t out = bpref_deref(vm, outh);
        {   /* la reserva ya esta comprobada arriba */
            ap = (an > 0) ? bpref_arr_elem(vm, a, 0, 1) : (const uint8_t*) "";
            bp = (bn > 0) ? bpref_arr_elem(vm, b, 0, 1) : (const uint8_t*) "";
            /* V8/H5: los trozos, como copias de bytes */
            uint8_t* dst = vm->memory + out + 4;
            if (an == 0)      { if (bn > 0) memcpy(dst, bp, bn); }
            else if (bn == 0) { memcpy(dst, ap, an); }
            else {
                if (ae > 0) memcpy(dst, ap, ae);
                dst[ae] = '/';
                if (bn > bs) memcpy(dst + ae + 1, bp + bs, bn - bs);
            }
            bpvm_str_nace(vm, out);   /* V8/H5: el bit de la ruta */
            out = (uint32_t) outh.v;
        }
        tc->sp -= 2 * BPVM_REF_SIZE;
        push_ref(vm, tc, out);
        return BPVM_OK;
    }

    case BUILTIN_PATH_PARENT:
    case BUILTIN_PATH_BASENAME:
    case BUILTIN_PATH_EXTENSION: {
        bpref_t s = peek_ref(vm, tc, 0);
        uint32_t n = bpref_is_null(s) ? 0 : bpref_arr_len(vm, s);
        const uint8_t* p = (n > 0) ? bpref_arr_elem(vm, s, 0, 1) : (const uint8_t*) "";
        uint32_t e = n;  while (e > 0 && p[e-1] == '/') e--;   /* '/' finales fuera */
        long slash = -1;
        for (uint32_t i = 0; i < e; i++) if (p[i] == '/') slash = (long) i;
        uint32_t b0, b1;                                        /* rango a copiar */
        if (id == BUILTIN_PATH_PARENT) {
            if (slash < 0)      { b0 = 0; b1 = 0; }             /* sin '/' -> "" */
            else if (slash == 0){ b0 = 0; b1 = 1; }             /* raiz -> "/"   */
            else                { b0 = 0; b1 = (uint32_t) slash; }
        } else {                                                /* basename y ext */
            b0 = (slash < 0) ? 0 : (uint32_t) slash + 1;
            b1 = e;
            if (id == BUILTIN_PATH_EXTENSION) {
                long dot = -1;
                for (uint32_t i = b0; i < b1; i++) if (p[i] == '.') dot = (long) i;
                /* dot en la 1a posicion del nombre (".oculto") o al final: sin ext */
                if (dot < 0 || (uint32_t) dot == b0 || (uint32_t) dot == b1 - 1) b0 = b1 = 0;
                else b0 = (uint32_t) dot + 1;
            }
        }
        uint32_t len = b1 - b0;
        bpref_t outh = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, len, len); /* V7/H7 #593: la cabecera y el handle, al nacer */
        if (outh.v == 0u)   /* #355 (2a pasada del censo): la reserva fallo. Antes se
             * saltaba el bloque de abajo y se empujaba out=0 —una ref NULA— sin
             * decir nada: el programa comparaba una cadena vacia, no cuadraba, y
             * el fallo aparecia diez pasos mas alla disfrazado de dato malo.
             * REPRODUCIDO en el PC con --nogc: "if1" salia ANTES del throw. */
            return builtin_throw(vm, tc, "No space in heap");
        uint32_t out = bpref_deref(vm, outh);
        {   /* la reserva ya esta comprobada arriba */
            if (len > 0) memcpy(vm->memory + out + 4, bpref_arr_elem(vm, s, b0, 1), len);   /* V8/H5: copia de bytes */
            bpvm_str_nace(vm, out);   /* V8/H5: el bit de la extensión */
            out = (uint32_t) outh.v;
        }
        tc->sp -= BPVM_REF_SIZE;
        push_ref(vm, tc, out);
        return BPVM_OK;
    }

    case BUILTIN_NEW_REF_ARRAY: {
        /* P2b: la primitiva compartida (con el helper newarray_ref del AOT). #570: `newObjArray(n)` es su alias
         * público. V4: una ref plana = 8 bytes por elemento; el payload a cero deja los slots nulos. */
        int32_t cap = pop_i32(vm, tc);
        bpref_t h; char em[80];
        if (bpvm_prim_nuevo_array(vm, cap, BPVM_REF_SIZE, BPVM_TYPE_ARRAY_REF, "__newRefArray: capacidad negativa: ",
                                  "", &h, em, sizeof em) != 0)
            return builtin_throw(vm, tc, em);
        bpref_push(vm, tc, h);
        return BPVM_OK;
    }

    case BUILTIN_GROW_REF_ARRAY: {
        /* V4/paridad miVM: crecer el array de refs (backing de List/any[]).
         * Antes ausente en la VM-C (enum saltaba 44→46) → cualquier List que
         * creciera más allá de su capacidad inicial petaba 'builtin desconocido'. */
        int32_t new_cap = pop_i32(vm, tc);
        /* #350 — PEEK por HIGIENE, no por un fallo demostrado. El bucle de
         * copia lee del array VIEJO tras alocar el nuevo, o sea que viola el
         * mismo contrato que substring. Pero HOY NO MUERDE, y conviene que
         * quede escrito por qué: el emisor genera
         *     this ; this.items ; this.cap ; CALL grow ; setField items
         * así que `this.items` sigue apuntando al array viejo hasta DESPUÉS de
         * la llamada — el GC lo ve y no se lo lleva. Lo que protege es el
         * llamante, no el builtin; el día que alguien llame a esto desde otro
         * sitio, lo único que queda de pie es el contrato. */
        bpref_t oldr = peek_ref(vm, tc, 0);   /* array ref = 8 bytes (como miVM popTcRef) */
        if (new_cap < 0) {   /* #570 */
            char em[64]; snprintf(em, sizeof em, "__growRefArray: capacidad negativa: %" PRId32, new_cap);
            tc->sp -= BPVM_REF_SIZE; return builtin_throw(vm, tc, em);
        }
        uint32_t od = bpref_is_null(oldr) ? 0 : bpref_deref(vm, oldr);   /* V4: fuente handle→addr */
        uint32_t old_len = (od == 0) ? 0 : bpvm_read_u32_be(vm->memory + od);
        /* V8 #712: por la primitiva única de los arrays, que multiplica en 64 bits (era new_cap * 8 en 32) */
        bpref_t newh; char emr[64];
        if (bpvm_prim_nuevo_array(vm, new_cap, BPVM_REF_SIZE, BPVM_TYPE_ARRAY_REF, "__growRefArray: capacidad negativa: ",
                                  "", &newh, emr, sizeof emr) != 0) {
            tc->sp -= BPVM_REF_SIZE;
            return builtin_throw(vm, tc, emr);   /* H1: OOM atrapable */
        }
        uint32_t new_ref = bpref_deref(vm, newh);
        od = bpref_is_null(oldr) ? 0 : bpref_deref(vm, oldr);   /* re-derivar tras la reserva */
        uint32_t copy = (old_len < (uint32_t) new_cap) ? old_len : (uint32_t) new_cap;
        for (uint32_t i = 0; i < copy; i++) {   /* ref plana 8B/elem, vía la frontera de codificación */
            bpref_t e = bpref_load(vm, od + BPVM_ARR_DATA_OFF + i * BPVM_REF_SIZE);
            bpref_store(vm, new_ref + BPVM_ARR_DATA_OFF + i * BPVM_REF_SIZE, e);
        }
        tc->sp -= BPVM_REF_SIZE;   /* #350: ya copiado */
        bpref_push(vm, tc, newh);
        return BPVM_OK;
    }

    case BUILTIN_GROW_INT_ARRAY: {
        int32_t new_cap = pop_i32(vm, tc);
        /* #350 — PEEK por higiene; ver el porqué completo en GROW_REF_ARRAY. */
        bpref_t oldr = peek_ref(vm, tc, 0);   /* V4: array ref = 8 bytes (era pop_i32 4B → drift vs miVM popTcRef) */
        if (new_cap < 0) {   /* #570 */
            char em[64]; snprintf(em, sizeof em, "__growIntArray: capacidad negativa: %" PRId32, new_cap);
            tc->sp -= BPVM_REF_SIZE; return builtin_throw(vm, tc, em);
        }
        uint32_t od = bpref_is_null(oldr) ? 0 : bpref_deref(vm, oldr);   /* V4: fuente handle→addr */
        uint32_t old_len = (od == 0) ? 0 : bpvm_read_u32_be(vm->memory + od);
        /* V8 #712: por la primitiva única de los arrays, que multiplica en 64 bits (era new_cap * 4 en 32) */
        bpref_t newh; char emi[64];
        if (bpvm_prim_nuevo_array(vm, new_cap, 4u, BPVM_TYPE_ARRAY_I32, "__growIntArray: capacidad negativa: ", "",
                                  &newh, emi, sizeof emi) != 0) {
            tc->sp -= BPVM_REF_SIZE;
            return builtin_throw(vm, tc, emi);   /* H1: OOM atrapable */
        }
        uint32_t new_ref = bpref_deref(vm, newh);
        od = bpref_is_null(oldr) ? 0 : bpref_deref(vm, oldr);   /* re-derivar tras la reserva */
        uint32_t copy = (old_len < (uint32_t) new_cap) ? old_len : (uint32_t) new_cap;
        for (uint32_t i = 0; i < copy; i++) {
            uint32_t v = bpvm_read_u32_be(vm->memory + od + 4 + i * 4);
            bpvm_write_u32_be(vm->memory + new_ref + 4 + i * 4, v);
        }
        tc->sp -= BPVM_REF_SIZE;   /* #350: ya copiado */
        bpref_push(vm, tc, newh);
        return BPVM_OK;
    }

    case BUILTIN_CHARS_TO_STRING: {
        int32_t len = pop_i32(vm, tc);
        /* #350 — PEEK: el SEGUNDO bucle vuelve a leer de `ca` tras alocar.
         * Como en fromBytes, sin repro que lo demuestre pero con la misma
         * violación de contrato. Las salidas de error sacan la fuente a mano
         * para no dejar la pila descuadrada. */
        bpref_t ca = peek_ref(vm, tc, 0);                  /* array i32 de codepoints (entrada) */
        if (len < 0) {   /* #570: los dos, atrapables y con el texto de miVM */
            char em[64]; snprintf(em, sizeof em, "__charsToString: longitud negativa: %" PRId32, len);
            tc->sp -= BPVM_REF_SIZE; return builtin_throw(vm, tc, em);
        }
        uint32_t avail = bpref_arr_len(vm, ca);
        if ((uint32_t) len > avail) {
            char em[80]; snprintf(em, sizeof em, "__charsToString: longitud %" PRId32 " > capacidad %d", len, (int) avail);
            tc->sp -= BPVM_REF_SIZE; return builtin_throw(vm, tc, em);
        }
        /* H2: input = array i32 de codepoints; output = string byte[] UTF-8. */
        uint32_t total = 0;
        for (uint32_t i = 0; i < (uint32_t) len; i++) {
            uint32_t cp = bpvm_read_u32_be(bpref_arr_elem(vm, ca, i, 4));
            /* V8/H5 (#711): un appendChar con algo que no es un carácter (un sustituto, o fuera de 0..10FFFF) es un
             * fallo del programa: EncodingError, sin parámetro. Antes salían bytes malos. */
            if (!utf8_es_caracter((int32_t) cp)) {
                char em[96];
                snprintf(em, sizeof em, "appendChar: el código %" PRId32 " no es un carácter", (int32_t) cp);
                tc->sp -= BPVM_REF_SIZE; return builtin_throw_codif(vm, tc, em);
            }
            uint8_t tmp[4]; total += utf8_encode(cp, tmp);
        }
        bpref_t newh = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, total, total); /* V7/H7 #593: la cabecera y el handle, al nacer */
        if (newh.v == 0u) {
            tc->sp -= BPVM_REF_SIZE;
            return builtin_throw(vm, tc, "No space in heap");   /* H1: OOM atrapable */
        }
        uint32_t new_ref = bpref_deref(vm, newh);   /* alloc fresca (dirección física) */
        uint32_t w = 0;
        for (uint32_t i = 0; i < (uint32_t) len; i++) {
            uint32_t cp = bpvm_read_u32_be(bpref_arr_elem(vm, ca, i, 4));
            if (!utf8_es_caracter((int32_t) cp)) cp = 0xFFFDu;   /* #711: otro hilo lo cambió entre las dos pasadas */
            uint8_t enc[4]; uint32_t el = utf8_encode(cp, enc);
            /* V7/H12, 2b (la revisión) — nunca más allá de lo reservado: con dos runs, otro hilo puede cambiar el array
             * entre las dos pasadas (un StringBuilder compartido sin Mutex) y la segunda medir más que la primera; con un
             * run, el builtin va entero de una vez. La carrera es del programa (sale lo que salga); el bloque de al lado,
             * intacto. */
            if (w + el > total) break;
            for (uint32_t k = 0; k < el; k++) vm->memory[new_ref + 4 + w++] = enc[k];
        }
        if (w < total) memset(vm->memory + new_ref + 4 + w, 0, total - w);   /* #736: lo no escrito, a cero */
        bpvm_str_nace(vm, new_ref);   /* V8/H5: el bit, por los caracteres que salieron */
        tc->sp -= BPVM_REF_SIZE;   /* #350: ya copiado */
        bpref_push(vm, tc, newh);
        return BPVM_OK;
    }

    case BUILTIN_TO_BYTES:
    case BUILTIN_FROM_BYTES:
    case BUILTIN_FROM_BYTES_I: {   /* V8/H5 (#711): fromBytes(b, ignorarError), el 284 */
        /* H2 (V2): string y byte[] comparten layout (TYPE_ARRAY_I8). La
         * conversión es una copia defensiva (string inmutable / byte[]
         * mutable): mismos bytes, objeto nuevo. */
        /* #350 — PEEK: el bucle de copia lee de la fuente DESPUÉS de alocar.
         * Mismo contrato que substring (ver allí). Aquí NO está demostrado con
         * un repro —el que escribí no lo saca— pero la violación del contrato
         * es idéntica, así que se cierra igual. */
        int ignorar = (id == BUILTIN_FROM_BYTES_I) ? (pop_i32(vm, tc) != 0) : 0;
        bpref_t bref = peek_ref(vm, tc, 0);
        uint32_t rd = bpref_is_null(bref) ? 0 : bpref_deref(vm, bref);
        uint32_t n = (rd == 0) ? 0 : bpvm_read_u32_be(vm->memory + rd);
        /* V8/H5 (#711) — fromBytes es el DESCODIFICADOR: lo que no es UTF-8 válido, EncodingError, o «�» con el
         * indicador (y entonces sale más largo: se mide ANTES de reservar). toBytes copia sin mirar. */
        int32_t malo = -1;
        uint32_t m = n;
        if (id != BUILTIN_TO_BYTES && n > 0) {
            malo = utf8_primer_malo(vm->memory + rd + 4, n);
            if (malo >= 0) {
                if (!ignorar) {
                    uint8_t bm = vm->memory[rd + 4 + (uint32_t) malo];
                    tc->sp -= BPVM_REF_SIZE;   /* saca la fuente también al fallar */
                    return throw_utf8_malo(vm, tc, "fromBytes", malo, bm);
                }
                m = utf8_largo_corregido(vm->memory + rd + 4, n);
            }
        }
        bpref_t outh = bpvm_heap_nuevo_para_llenar(vm, BPVM_TYPE_ARRAY_I8, m, m); /* V7/H7 #593: la cabecera y el handle, al nacer */
        if (outh.v == 0u) {
            tc->sp -= BPVM_REF_SIZE;                 /* saca la fuente también al fallar */
            return builtin_throw(vm, tc, "No space in heap");   /* H1: OOM atrapable */
        }
        uint32_t out = bpref_deref(vm, outh);
        /* Re-derivar TRAS la reserva: hoy el mark-sweep no mueve objetos, pero
         * depender de eso es una trampa para el día que compacte. */
        rd = bpref_is_null(bref) ? 0 : bpref_deref(vm, bref);
        if (malo < 0) {
            if (n > 0) memcpy(vm->memory + out + 4, vm->memory + rd + 4, n);   /* V8/H5: una copia de bytes, de una vez */
            if (id != BUILTIN_TO_BYTES) bpvm_str_nace(vm, out);   /* V8/H5: la cadena nace con su bit (un byte[], nunca) */
        } else {
            utf8_corrige(vm->memory + rd + 4, n, vm->memory + out + 4);   /* #711: con «�» */
            bpvm_str_nace_ascii(vm, out, 0);
        }
        tc->sp -= BPVM_REF_SIZE;   /* #350: ya copiado, ahora sí se saca */
        bpref_push(vm, tc, outh);   /* H1.2a: string ref result = 8 bytes */
        return BPVM_OK;
    }

    case BUILTIN_GC: {
        bpvm_heap_gc(vm);
        push_i32(vm, tc, 0);   /* void → push dummy */
        return BPVM_OK;
    }

    /* ---- F4: tiempo y threading ---- */
    case BUILTIN_NOW: {
        int64_t ms = bpvm_platform_now_ms();
        push_i32(vm, tc, (int32_t) ms);
        return BPVM_OK;
    }

    case BUILTIN_SLEEP: {
        int32_t ms = pop_i32(vm, tc);
        if (ms <= 0) { push_i32(vm, tc, 0); return BPVM_OK; }
        tc->wake_at_ms = bpvm_platform_now_ms() + ms;
        tc->status = BPVM_THREAD_BLOCKED_SLEEP;
        push_i32(vm, tc, 0);   /* void */
        return BPVM_OK;
    }

    case BUILTIN_SLEEP_SEC: {
        /* Misma semántica que SLEEP pero la entrada está en segundos. */
        int32_t s = pop_i32(vm, tc);
        if (s <= 0) { push_i32(vm, tc, 0); return BPVM_OK; }
        int64_t ms = (int64_t) s * 1000LL;
        tc->wake_at_ms = bpvm_platform_now_ms() + ms;
        tc->status = BPVM_THREAD_BLOCKED_SLEEP;
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    case BUILTIN_SLEEP_US: {
        /* Busy-wait que NO cede el thread BP. En Pico usa busy_wait_us
         * del SDK (timer HW, precisión µs); en host usa clock_gettime
         * con spin loop. */
        int32_t us = pop_i32(vm, tc);
        bpvm_platform_busy_wait_us(us);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    case BUILTIN_YIELD: {
        /* Marcar RUNNABLE — el scheduler verá el cambio al consumir
         * el quantum o al detectar el yield_requested al volver. F4 v1
         * con quantum-based scheduler: el yield se manifiesta como un
         * "saltar el resto del quantum". Forzamos terminar el quantum
         * dejando el flag — implementación simple: setear RUNNABLE
         * para que el wrapper del scheduler decida. */
        tc->status = BPVM_THREAD_RUNNABLE;
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    case BUILTIN_EVENT_RAISE: {
        /* Pila (de arriba abajo): recv(8B) dest(4B) nargs(4B) masks(4B) argN-1..arg0.
         * Los anchos de los argumentos los DICE el compilador en `masks`
         * (bits 8-11); la VM no los adivina. */
        bpref_t recv  = bpref_pop(vm, tc);
        int32_t dest  = pop_i32(vm, tc);
        int32_t nargs = pop_i32(vm, tc);
        uint32_t masks = (uint32_t) pop_i32(vm, tc);
        if (nargs < 0 || nargs > BPVM_EVENT_MAX_ARGS) {
            bpvm_diag("[bpvm] __eventRaise: aridad %d fuera de rango", (int) nargs);
            push_i32(vm, tc, 0);
            return BPVM_ERR_RUNTIME;
        }
        int64_t args[BPVM_EVENT_MAX_ARGS];
        for (int i = nargs - 1; i >= 0; i--) {           /* se desapilan al revés */
            if (masks & (1u << (8 + i))) { tc->sp -= 8; args[i] = bpvm_read_i64_be(vm->memory + tc->sp); }
            else                         { tc->sp -= 4; args[i] = (int64_t) bpvm_read_i32_be(vm->memory + tc->sp); }
        }
        /* Sin suscriptor no pasa nada: un evento es una NOTIFICACIÓN, y si
         * nadie escucha no hay error (decisión de diseño, modelo Swing). */
        if (!bpref_is_null(recv) && dest != 0) {
            /* V7/H12, 2b: con varios runs, el planificador saca de esta misma cola con el cerrojo cogido */
            int con = !bpvm_la_cola.sabotaje_eventos;   /* (el sabotaje: SÓLO PRUEBAS, test_cola) */
            if (con) bpvm_smp_lock(vm);
            bpvm_event_enqueue(vm, tc->id, recv.v, dest, nargs, masks, args);
            if (con) bpvm_smp_unlock(vm);
        }
        push_i32(vm, tc, 0);   /* void */
        return BPVM_OK;
    }

    case BUILTIN_THREAD_START: {
        /* handle COMPLETO: el gen viaja hasta el `this` de run(). V8 #740 — se LEE sin sacarlo: la pila del hilo sale
         * del heap y reservarla puede colectar, y en esa colecta el Thread tiene que seguir siendo raíz. Hoy lo es de
         * todos modos —Core.Thread.start() lo tiene en su `this`—, pero el builtin no lo da por hecho. Sale tras crear el
         * hilo (si nació, lo sujeta su propia pila). Como miVM. */
        bpref_t thread_ref = bpref_load(vm, tc->sp - BPVM_REF_SIZE);
        int new_tid = bpvm_thread_spawn(vm, thread_ref);
        tc->sp -= BPVM_REF_SIZE;
        if (new_tid == -2)   /* #530: el mismo texto que miVM (no pasa con un Thread: Core.Thread tiene run()) */
            return builtin_throw(vm, tc, "Thread.start: run() no resoluble en la cadena de herencia");
        /* V7/H7 #604 — los límites, atrapables (antes, el programa moría con un error que no se atrapa): sin sitio para
         * la pila que pide el hilo —con el texto de miVM— o, desde la etapa 2 de H12 (sin tope de hilos: lo pone la
         * memoria), sin memoria para su struct (la RAM del sistema; miVM no tiene ese límite). */
        if (new_tid == -1 || new_tid == -3) {
            char m[80];
            if (new_tid == -1) {
                snprintf(m, sizeof m, "Thread.start: no hay memoria para el hilo");
            } else {
                int32_t pedido = bpvm_read_i32_be(vm->memory + bpref_deref(vm, thread_ref) + 4 + 1 * 4);
                snprintf(m, sizeof m, "Thread.start: no hay memoria para la pila del hilo (%ld B)",
                         (long) (pedido > 0 ? pedido : (int32_t) BPVM_THREAD_STACK_BYTES));
            }
            return builtin_throw(vm, tc, m);
        }
        if (new_tid < 0) {
            bpvm_diag_urgente("[bpvm-c] Thread.start falló");
            push_i32(vm, tc, 0);
            return BPVM_ERR_RUNTIME;
        }
        push_i32(vm, tc, 0);   /* void */
        return BPVM_OK;
    }

    case BUILTIN_THREAD_JOIN: {
        uint32_t thread_ref = pop_ref(vm, tc);
        if (thread_ref == 0) {
            push_i32(vm, tc, 0); return BPVM_OK;
        }
        /* Convención: field[0] del Thread BP guarda el tid (escrito
         * por __threadStart). 0 = no spawneado todavía. */
        int32_t target_tid = bpvm_read_i32_be(vm->memory + ref_addr(vm, thread_ref) + 4 + 0 * 4);
        /* V7/H12, etapa 2 — el __tid es el HANDLE de su struct en la cola: si no resuelve, el hilo acabó y salió (o
         * no ha arrancado: 0). Mirarlo y bloquearse, bajo el cerrojo: otro worker puede estar sacando hilos de la
         * cola a la vez. */
        /* V7/H12, 2b (#595): y espera también si su hilo terminó DEBIENDO eventos —el planificador lo resucitará para
         * atenderlos—, como el despertar del planificador. Con un run nunca coincide (entre el fin del hilo y este join
         * pasa una vuelta del planificador, que ya lo ha resucitado); con dos, el join podía volver antes del handler. */
        /* V7/H12, 2b (la revisión) — y espera si su hilo, ya TERMINATED, sigue en manos del otro run: termina antes de
         * soltar su última línea (la que dejó a medias), y el join no puede adelantarse a ella. Con un run nunca pasa. */
        bpvm_smp_lock(vm);
        const bpvm_thread_t* target = (target_tid > 0) ? bpvm_hilo_por_tid(vm, target_tid) : NULL;
        if (target != NULL && (target->status != BPVM_THREAD_TERMINATED || bpvm_events_con_deuda(vm, target)
                               || (target->sched_owner != -1 && !bpvm_la_cola.sabotaje_suyo))) {
            tc->blocked_on_join = target_tid;
            tc->status = BPVM_THREAD_BLOCKED_JOIN;
        }
        bpvm_smp_unlock(vm);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    case BUILTIN_MUTEX_CREATE: {
        int mid = bpvm_mutex_alloc(vm);
        push_i32(vm, tc, (int32_t) mid);
        return BPVM_OK;
    }

    case BUILTIN_MUTEX_LOCK: {
        /* Recibe el ref del objeto Mutex BP. El mid (id en el pool de
         * la VM) está en field[0] del objeto, escrito por el ctor de
         * Mutex que llama a __mutexCreate. */
        uint32_t mref = pop_ref(vm, tc);
        if (mref == 0) {
            push_i32(vm, tc, 0); return BPVM_ERR_NULL_RECEIVER;
        }
        int32_t mid = bpvm_read_i32_be(vm->memory + ref_addr(vm, mref) + 4 + 0 * 4);
        if (mid < 0 || mid >= vm->mutex_count) {
            bpvm_diag("[bpvm-c] mutex_lock: mid inválido %" PRId32 "", mid);
            push_i32(vm, tc, 0);
            return BPVM_ERR_RUNTIME;
        }
        /* La propiedad la mueve `bpvm_mutex_try_acquire`, que lleva el cerrojo
         * dentro: aqui solo queda lo que es de BP (lanzar y empujar el retorno).
         * Antes esto hacia el check-and-set a pelo y el interp corre SIN vm_lock. */
        int r = bpvm_mutex_try_acquire(vm, mid, tc->id);
        if (r < 0) {
            /* BUG-7b — reentrada → RuntimeError atrapable (paridad VM-Java). */
            char em[96];
            snprintf(em, sizeof em, "mutex.lock: re-entrada por mismo thread tid=%d (los Mutex no son reentrantes)",
                     (int) tc->id);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    case BUILTIN_MUTEX_UNLOCK: {
        uint32_t mref = pop_ref(vm, tc);
        if (mref == 0) {
            push_i32(vm, tc, 0); return BPVM_ERR_NULL_RECEIVER;
        }
        int32_t mid = bpvm_read_i32_be(vm->memory + ref_addr(vm, mref) + 4 + 0 * 4);
        if (mid < 0 || mid >= vm->mutex_count) {
            bpvm_diag("[bpvm-c] mutex_unlock: mid inválido %" PRId32 "", mid);
            push_i32(vm, tc, 0);
            return BPVM_ERR_RUNTIME;
        }
        /* Igual que el lock: la propiedad y el traspaso al waiter van dentro de
         * `bpvm_mutex_release`, bajo el cerrojo. Antes el traspaso se hacia aqui
         * en tres pasos sueltos y el waiter podia perder su despertar. */
        int owner = -1;
        if (bpvm_mutex_release(vm, mid, tc->id, &owner) < 0) {
            /* BUG-7b — unlock por no-propietario → RuntimeError atrapable (paridad VM-Java). */
            char em[96];
            snprintf(em, sizeof em, "mutex.unlock: thread %d no es propietario (owner=%d)",
                     (int) tc->id, owner);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    /* ---- Gpio — todos los handlers delegan al backend de plataforma.
       En host: stubs con logging. En Pico: tabla rellenada por main.c. ---- */
    case BUILTIN_GPIO_INIT: {
        int mode = pop_i32(vm, tc);
        int pin  = pop_i32(vm, tc);
        /* V7/H4 (#479) — se valida contra la tabla y el estado ANTES de tocar el backend, con
         * nombre; ANALOG (3) sólo fija el estado: el hardware lo pone el ADC (AnPin/Adc.Channel). */
        char em[128];
        if (mode < 0 || mode > 3) return builtin_throw(vm, tc, "Gpio.Pin: modo no válido (INPUT, OUTPUT, OPEN_COLLECTOR, ANALOG)");
        /* V7/H12, 2b (la revisión) — LA HAL, BAJO SU CERROJO (hal_cerrar: el suyo, no el de la VM): reclamar un pin,
         * abrir o soltar un bus (la cuenta y el hardware, juntos), iniciar una UART, una tira o un canal del ADC; y leer
         * el ADC (elegir el canal y convertir), las migas y el reloj. Sus tablas —los pines, los buses, las ranuras de
         * la UART, las tiras— y el ADC son de todos: con dos runs, dos hilos a la vez veían libre el mismo pin, o
         * abierto un bus aún sin abrir, o uno leía el canal del otro. Con un run, nada. Y nunca se lanza con él cogido. */
        hal_cerrar(vm);
        int r = bpvm_gpio_reclamar(pin, mode, em, sizeof em);
        if (r == 0 && mode != 3) bpvm_gpio_init(pin, mode);
        hal_abrir(vm);
        if (r != 0) return builtin_throw(vm, tc, em);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GPIO_RELEASE: {
        int pin = pop_i32(vm, tc);
        hal_cerrar(vm);   /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT) */
        (void) bpvm_gpio_soltar(pin);   /* V7 #667: libre Y, en el hardware, como en el reset */
        hal_abrir(vm);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    /* V7/H6.c G4.a1 — el programa SUELTA un bus (Spi.Bus.release / I2c.Bus.release):
     * si nadie más lo usa y no es de la placa (ENV), se cierra y sus pines vuelven a
     * libres, y otro puede establecerlo distinto. Soltar el de la placa, o uno que usa
     * también un pack o la TF, es un error con nombre, no un no-op (V7_IDEAS §H6.11.2). */
    case BUILTIN_SPI_RELEASE:
    case BUILTIN_I2C_RELEASE: {
        int bus = pop_i32(vm, tc);
        int es_spi = (id == BUILTIN_SPI_RELEASE);
        char em[160];
        if (bpvm_gpio_instancia_ok(es_spi ? 2 : 1, bus, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        hal_cerrar(vm);   /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT) */
        int r = es_spi ? bpvm_spi_soltar(bus, BPVM_BUS_PROGRAMA, em, sizeof em)
                       : bpvm_i2c_soltar(bus, BPVM_BUS_PROGRAMA, em, sizeof em);
        hal_abrir(vm);
        if (r != 0) return builtin_throw(vm, tc, em);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GPIO_PULL: {
        int pull = pop_i32(vm, tc);
        int pin  = pop_i32(vm, tc);
        bpvm_gpio_pull(pin, pull);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GPIO_WRITE: {
        int val = pop_i32(vm, tc);
        int pin = pop_i32(vm, tc);
        if (bpvm_gpio_estado(pin) == BPVM_GPIO_ANALOG) {
            char em[96]; snprintf(em, sizeof em, "el pin %d es analógico: no se puede escribir", pin);
            return builtin_throw(vm, tc, em);
        }
        bpvm_gpio_write(pin, val);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_GPIO_READ: {
        int pin = pop_i32(vm, tc);
        if (bpvm_gpio_estado(pin) == BPVM_GPIO_ANALOG) {
            char em[96]; snprintf(em, sizeof em, "el pin %d es analógico: usa read() del AnPin", pin);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, bpvm_gpio_read(pin));
        return BPVM_OK;
    }

    /* ---- I2C ----
       Layout del array TYPE_ARRAY_I32 en memory:
         [0..3] length (i32)
         [4..]  elementos (i32 cada uno)
       Cada elemento transporta un byte en el byte bajo. */
    case BUILTIN_I2C_INIT: {
        int baud = pop_i32(vm, tc);
        int scl  = pop_i32(vm, tc);
        int sda  = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        { char em[128]; if (bpvm_gpio_instancia_ok(1, bus, em, sizeof em) != 0) return builtin_throw(vm, tc, em); }   /* V7/H4 */
        /* V7/H6.c G4.a1 — el bus es COMPARTIDO: la configuración se establece una
         * vez (el ENV o el primero) y otra es un error con nombre; -1 = la que tenga. */
        {
            char em[160];
            hal_cerrar(vm);   /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT): la cuenta del bus y su hardware */
            int r = bpvm_i2c_abrir(bus, sda, scl, baud, BPVM_BUS_PROGRAMA, em, sizeof em);
            hal_abrir(vm);
            if (r != 0) return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_I2C_WRITE: {
        /* write(bus, addr, data: integer[], count): integer */
        int count = pop_i32(vm, tc);
        uint32_t dataRef = pop_ref(vm, tc);
        int addr = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        /* #534 — una transacción: no se parte, y tiene tope (BUS_TRAMO); antes, 64 en silencio. */
        char em[128]; uint32_t dataAddr = 0;
        if (bus_array(vm, dataRef, count, "i2c.write", "el array", BUS_TRAMO, &dataAddr, em, sizeof em) != 0)
            return builtin_throw(vm, tc, em);
        uint8_t buf[BUS_TRAMO];
        push_i32(vm, tc, bus_i2c(vm, 1, bus, addr, dataAddr, count, buf));
        return BPVM_OK;
    }
    case BUILTIN_I2C_READ: {
        /* read(bus, addr, data: integer[], count): integer */
        int count = pop_i32(vm, tc);
        uint32_t dataRef = pop_ref(vm, tc);
        int addr = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        char em[128]; uint32_t dataAddr = 0;   /* #534 */
        if (bus_array(vm, dataRef, count, "i2c.read", "el array", BUS_TRAMO, &dataAddr, em, sizeof em) != 0)
            return builtin_throw(vm, tc, em);
        uint8_t buf[BUS_TRAMO];
        push_i32(vm, tc, bus_i2c(vm, 0, bus, addr, dataAddr, count, buf));
        return BPVM_OK;
    }
    /* ---- V7/H6.c G4.a3 — la TRANSACCIÓN I2C: repeated start entre mensajes (V7_IDEAS §H6.11.9) ----
     * Lo que no deja empezar (el tope, el bus, el turno) es un RuntimeError; un dispositivo que no
     * contesta, -1: es su respuesta, como en read/write. Hasta BUS_TRAMO bytes por transacción. */
    case BUILTIN_I2C_WRITE_READ: {
        int32_t nrx = pop_i32(vm, tc);
        uint32_t rxRef = pop_ref(vm, tc);
        int32_t ntx = pop_i32(vm, tc);
        uint32_t txRef = pop_ref(vm, tc);
        int addr = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        const char* nom = "i2c.writeRead";
        char em[160];
        uint32_t txA = 0, rxA = 0;
        uint8_t buf[BUS_TRAMO];
        bpvm_i2c_msg_t m[2];
        int r;
        if (bpvm_gpio_instancia_ok(1, bus, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        if (bus_array(vm, txRef, ntx, nom, "el array tx", BUS_TRAMO, &txA, em, sizeof em) != 0
            || bus_array(vm, rxRef, nrx, nom, "el array rx", BUS_TRAMO, &rxA, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        if (ntx + nrx > BUS_TRAMO) {
            snprintf(em, sizeof em, "%s: %" PRId32 " bytes supera el máximo de %d bytes por transacción", nom, ntx + nrx, (int) BUS_TRAMO);
            return builtin_throw(vm, tc, em);
        }
        bus_a_bytes(vm, txA, 0, (int) ntx, buf);
        m[0].buf = buf;       m[0].n = (uint32_t) ntx; m[0].lee = 0;
        m[1].buf = buf + ntx; m[1].n = (uint32_t) nrx; m[1].lee = 1;
        r = bpvm_i2c_transaccion(bus, addr, BPVM_BUS_PROGRAMA, m, 2, em, sizeof em);
        if (r == -1) return builtin_throw(vm, tc, em);
        if (r >= 0) bus_de_bytes(vm, rxA, 0, (int) nrx, buf + ntx);
        push_i32(vm, tc, r < 0 ? -1 : nrx);
        return BPVM_OK;
    }
    case BUILTIN_I2C_EXEC: {
        /* I2c.Bus.execute(addr, t): la I2c.Transaction ya reunida por I2c.bp —`ops` = pares
         * (1 escribir · 2 leer, bytes); `tx` y `rx`, seguidos—. */
        uint32_t rxRef  = pop_ref(vm, tc);
        uint32_t txRef  = pop_ref(vm, tc);
        int32_t  nops   = pop_i32(vm, tc);
        uint32_t opsRef = pop_ref(vm, tc);
        int addr = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        const char* nom = "i2c.execute";
        char em[160];
        uint32_t opsA = 0, txA = 0, rxA = 0;
        int32_t i, sale = 0, entra = 0, pt = 0, pr = 0, pos = 0;
        int64_t total = 0;
        uint8_t buf[BUS_TRAMO];
        bpvm_i2c_msg_t m[BPVM_I2C_MENSAJES_MAX];
        /* V7/H12, 2b (la revisión) — cada operación se lee UNA vez (aquí, en la primera pasada) y las demás usan lo
         * leído: con dos runs, otro hilo puede cambiar `ops` entre pasadas (una Transaction compartida sin Mutex), y la
         * tercera copiaba al búfer de la pila lo que la primera no había validado. */
        int32_t ks[BPVM_I2C_MENSAJES_MAX], ns[BPVM_I2C_MENSAJES_MAX];
        int r;
        if (bpvm_gpio_instancia_ok(1, bus, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        if (nops < 1 || nops > BPVM_I2C_MENSAJES_MAX) {
            snprintf(em, sizeof em, "%s: una transacción lleva de 1 a %d mensajes (tiene %" PRId32 ")", nom, BPVM_I2C_MENSAJES_MAX, nops);
            return builtin_throw(vm, tc, em);
        }
        if (bus_array(vm, opsRef, nops * 2, nom, "el array ops", 0, &opsA, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        for (i = 0; i < nops; i++) {
            int32_t k = arr_i32(vm, opsA, 2 * i), n = arr_i32(vm, opsA, 2 * i + 1);
            if (k != 1 && k != 2) { snprintf(em, sizeof em, "%s: la operación %" PRId32 " no es válida (%" PRId32 ")", nom, i + 1, k); return builtin_throw(vm, tc, em); }
            if (n < 0) { snprintf(em, sizeof em, "%s: count negativo (%" PRId32 ")", nom, n); return builtin_throw(vm, tc, em); }
            ks[i] = k; ns[i] = n;
            total += n;
        }
        if (total > BUS_TRAMO) {
            snprintf(em, sizeof em, "%s: %" PRId64 " bytes supera el máximo de %d bytes por transacción", nom, total, (int) BUS_TRAMO);
            return builtin_throw(vm, tc, em);
        }
        for (i = 0; i < nops; i++) {   /* ya caben: sumar ahora no se desborda */
            if (ks[i] == 1) sale += ns[i];
            else            entra += ns[i];
        }
        if (bus_array(vm, txRef, sale, nom, "el array tx", 0, &txA, em, sizeof em) != 0
            || bus_array(vm, rxRef, entra, nom, "el array rx", 0, &rxA, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        for (i = 0; i < nops; i++) {   /* cada mensaje, su trozo del búfer; los de escribir, ya llenos */
            int32_t k = ks[i], n = ns[i];
            m[i].buf = buf + pos; m[i].n = (uint32_t) n; m[i].lee = (k == 2);
            if (k == 1) { bus_a_bytes(vm, txA, pt, (int) n, buf + pos); pt += n; }
            pos += n;
        }
        r = bpvm_i2c_transaccion(bus, addr, BPVM_BUS_PROGRAMA, m, (int) nops, em, sizeof em);
        if (r == -1) return builtin_throw(vm, tc, em);
        if (r >= 0)
            for (i = 0; i < nops; i++)
                if (m[i].lee) { bus_de_bytes(vm, rxA, pr, (int) m[i].n, m[i].buf); pr += (int32_t) m[i].n; }
        push_i32(vm, tc, r < 0 ? -1 : (int32_t) total);
        return BPVM_OK;
    }

    case BUILTIN_MOVE: {
        /* move(src, dst, srcStart, dstStart, count): void — copia `count` elementos de
         * src[srcStart..] a dst[dstStart..]; pueden solaparse (memmove).
         * #535 (V7): ESPEJO de `case MOVE` de miVM, en el mismo orden y con los mismos textos
         * (contrato de paridad). Antes, con un null o count <= 0 volvía en silencio, no miraba
         * ni offsets ni longitudes —un move fuera de rango escribía fuera del array— y copiaba
         * 4 B por elemento aunque el array fuera de bytes o de 8 B (long[], double[], refs). */
        int32_t count    = pop_i32(vm, tc);
        int32_t dstStart = pop_i32(vm, tc);
        int32_t srcStart = pop_i32(vm, tc);
        uint32_t dstRef  = pop_ref(vm, tc);
        uint32_t srcRef  = pop_ref(vm, tc);
        char em[96];
        if (srcRef == 0) return builtin_throw(vm, tc, "move: src es null");
        if (dstRef == 0) return builtin_throw(vm, tc, "move: dst es null");
        uint32_t sd = ref_addr(vm, srcRef);   /* V4: handle → dirección física del dato */
        uint32_t dd = ref_addr(vm, dstRef);
        uint32_t sh = sd - 4u, dh = dd - 4u;  /* la cabecera del bloque (un handle muerto da 0 - 4) */
        if (sh < vm->heap_start || sh >= vm->heap_next) return builtin_throw(vm, tc, "move: src no es ref a heap");
        if (dh < vm->heap_start || dh >= vm->heap_next) return builtin_throw(vm, tc, "move: dst no es ref a heap");
        uint32_t stag = bpvm_read_u32_be(vm->memory + sh);
        uint32_t dtag = bpvm_read_u32_be(vm->memory + dh);
        if (stag & BPVM_TAG_FREE_BIT) return builtin_throw(vm, tc, "move: src apunta a bloque libre");
        if (dtag & BPVM_TAG_FREE_BIT) return builtin_throw(vm, tc, "move: dst apunta a bloque libre");
        int st = (int) ((stag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT);
        int dt = (int) ((dtag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT);
        uint32_t esz = move_elem(st);
        if (esz == 0u) {
            snprintf(em, sizeof em, "move: src no es un array (type=%d)", st);
            return builtin_throw(vm, tc, em);
        }
        if (move_elem(dt) == 0u) {
            snprintf(em, sizeof em, "move: dst no es un array (type=%d)", dt);
            return builtin_throw(vm, tc, em);
        }
        /* V8/H5 (23-oct, #714) — una cadena es de SOLO LECTURA desde que nace (Eduardo: «Que un string es un invariante
         * no se toca»): move no escribe en ella. Mismo texto que miVM. */
        if (dtag & BPVM_TAG_SOLO_LECTURA) return builtin_throw(vm, tc, "move: dst es de solo lectura");
        if (st != dt) {
            snprintf(em, sizeof em, "move: tipos de array distintos (src=%d dst=%d)", st, dt);
            return builtin_throw(vm, tc, em);
        }
        int32_t srcLen = bpvm_read_i32_be(vm->memory + sd);
        int32_t dstLen = bpvm_read_i32_be(vm->memory + dd);
        if (count < 0) {
            snprintf(em, sizeof em, "move: count negativo (%" PRId32 ")", count);
            return builtin_throw(vm, tc, em);
        }
        if (srcStart < 0 || dstStart < 0) {
            snprintf(em, sizeof em, "move: offset negativo (srcStart=%" PRId32 " dstStart=%" PRId32 ")",
                     srcStart, dstStart);
            return builtin_throw(vm, tc, em);
        }
        if ((int64_t) srcStart + count > (int64_t) srcLen) {
            snprintf(em, sizeof em, "move: rango fuera de src (srcStart=%" PRId32 " count=%" PRId32
                     " srcLen=%" PRId32 ")", srcStart, count, srcLen);
            return builtin_throw(vm, tc, em);
        }
        if ((int64_t) dstStart + count > (int64_t) dstLen) {
            snprintf(em, sizeof em, "move: rango fuera de dst (dstStart=%" PRId32 " count=%" PRId32
                     " dstLen=%" PRId32 ")", dstStart, count, dstLen);
            return builtin_throw(vm, tc, em);
        }
        memmove(vm->memory + dd + 4u + (uint32_t) dstStart * esz,
                vm->memory + sd + 4u + (uint32_t) srcStart * esz,
                (size_t) count * esz);
        /* V8/H5: `move` escribe en el destino, que puede ser una cadena (#714): su bit «todo ASCII» deja de estar
         * garantizado. Se quita (va por el lento, que siempre acierta). Y lo que los cursores de los hilos sabían de
         * ella tampoco vale: la época sube, DESPUÉS de escribir (RELEASE: quien vea la época nueva ve los bytes), y los
         * invalida todos. */
        if (dt == BPVM_TYPE_ARRAY_I8) {
            bpvm_write_u32_be(vm->memory + dh, bpvm_read_u32_be(vm->memory + dh) & ~BPVM_TAG_ASCII);
            (void) __atomic_fetch_add(&vm->str_epoca, 1u, __ATOMIC_RELEASE);
        }
        push_i32(vm, tc, 0);   /* void: el valor de relleno, como miVM */
        return BPVM_OK;
    }

    case BUILTIN_STR_COPY: {
        /* #554 (V7/H7, 27-sep) — LA CONCATENACIÓN: copia TODOS los bytes de la cadena `src` en el byte[] `dst` desde
         * `pos`, y devuelve la posición siguiente (así se encadena: la copia siguiente empieza donde acabó ésta). La
         * usan los helpers `__strconcat`/`__strconcatN` que el compilador sintetiza en cada módulo, en vez de copiar
         * byte a byte en bytecode (#554: ~10 ms por concatenación de 2 KB en placa). `src` es una cadena en
         * cualquiera de sus DOS formas —un handle del heap o la referencia cruda de un literal del bloque de datos
         * (LEA_GLOBAL), la de cualquier módulo, compilado antes o después— o null, que es vacía (#512): la
         * indirección resuelve las dos, y la longitud va delante de los bytes en las dos. Por eso es un builtin
         * PROPIO de las cadenas y no `move`, que no sabe qué hay detrás de una referencia sin cabecera de heap (un
         * literal, o un array fijo de un .mod MOD7) y la rechaza. `dst`, un array de bytes del heap: el que acaba de
         * reservar el helper. ESPEJO de `STR_COPY` de miVM, en el mismo orden y con los mismos textos. */
        int32_t  pos    = pop_i32(vm, tc);
        uint32_t dstRef = pop_ref(vm, tc);
        uint32_t srcRef = pop_ref(vm, tc);
        char em[96];
        if (dstRef == 0) return builtin_throw(vm, tc, "strCopy: dst es null");
        uint32_t dd = ref_addr(vm, dstRef);
        uint32_t dh = dd - 4u;
        if (dh < vm->heap_start || dh >= vm->heap_next) return builtin_throw(vm, tc, "strCopy: dst no es ref a heap");
        uint32_t dtag = bpvm_read_u32_be(vm->memory + dh);
        if (dtag & BPVM_TAG_FREE_BIT) return builtin_throw(vm, tc, "strCopy: dst apunta a bloque libre");
        int dt = (int) ((dtag & BPVM_TAG_TYPE_MASK) >> BPVM_TAG_TYPE_SHIFT);
        if (dt != BPVM_TYPE_ARRAY_I8) {
            snprintf(em, sizeof em, "strCopy: dst no es un array de bytes (type=%d)", dt);
            return builtin_throw(vm, tc, em);
        }
        uint32_t sd = (srcRef == 0) ? 0u : ref_addr(vm, srcRef);
        int32_t  n  = (sd == 0u) ? 0 : bpvm_read_i32_be(vm->memory + sd);
        int32_t  dstLen = bpvm_read_i32_be(vm->memory + dd);
        if (pos < 0 || n < 0 || (int64_t) pos + n > (int64_t) dstLen) {
            snprintf(em, sizeof em, "strCopy: fuera de rango (pos=%" PRId32 " bytes=%" PRId32 " dst=%" PRId32 ")",
                     pos, n, dstLen);
            return builtin_throw(vm, tc, em);
        }
        if (n > 0) memmove(vm->memory + dd + 4u + (uint32_t) pos, vm->memory + sd + 4u, (size_t) n);
        {   /* V8/H5 — el bit del destino es el «Y» de los trozos: el que empieza en 0 lo pone, los demás lo pueden
             * quitar (uno que empieza en 0 tras otros vacíos también lo pone: el «Y» con lo vacío es lo mismo). Un
             * trozo del heap con su bit no se mira; uno sin él (un literal, un módulo viejo) se mira al copiarlo. */
            int trozo = (n == 0)
                     || (sd >= vm->heap_start + 4u && (bpvm_read_u32_be(vm->memory + sd - 4u) & BPVM_TAG_ASCII))
                     || bpvm_bytes_ascii(vm->memory + sd + 4u, (uint32_t) n);
            uint32_t t = bpvm_read_u32_be(vm->memory + dh);
            int ascii = trozo && (pos == 0 || (t & BPVM_TAG_ASCII));
            bpvm_write_u32_be(vm->memory + dh, ascii ? (t | BPVM_TAG_ASCII) : (t & ~BPVM_TAG_ASCII));
        }
        push_i32(vm, tc, pos + n);
        return BPVM_OK;
    }

    case BUILTIN_NEW_INT_ARRAY: {
        /* P2b: la primitiva compartida (con el opcode NEWARRAY y el helper newarray_i32 del AOT). #570: un
         * tamaño negativo, atrapable y con el texto de miVM. */
        int32_t size = pop_i32(vm, tc);
        bpref_t h; char em[80];
        if (bpvm_prim_nuevo_array(vm, size, 4u, BPVM_TYPE_ARRAY_I32, "newIntArray: tamaño negativo: ", "",
                                  &h, em, sizeof em) != 0)
            return builtin_throw(vm, tc, em);
        bpref_push(vm, tc, h);
        return BPVM_OK;
    }

    /* ---- SPI ----  igual patrón que I2C, arrays de bytes empaquetados. */
    case BUILTIN_SPI_INIT: {
        int mode = pop_i32(vm, tc);
        int baud = pop_i32(vm, tc);
        int miso = pop_i32(vm, tc);
        int mosi = pop_i32(vm, tc);
        int sck  = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        { char em[128]; if (bpvm_gpio_instancia_ok(2, bus, em, sizeof em) != 0) return builtin_throw(vm, tc, em); }   /* V7/H4 */
        /* V7/H6.c G4.a1 — como el I2C: se establece una vez; -1 = el pin que tenga el bus. */
        {
            char em[160];
            hal_cerrar(vm);   /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT): la cuenta del bus y su hardware */
            int r = bpvm_spi_abrir(bus, sck, mosi, miso, baud, mode, BPVM_BUS_PROGRAMA, NULL, em, sizeof em);
            hal_abrir(vm);
            if (r != 0) return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    /* #534 — SPI sin tope, en tramos de BUS_TRAMO (bus_por_tramos); antes, 256 en silencio. */
    case BUILTIN_SPI_WRITE: {
        int count = pop_i32(vm, tc);
        uint32_t dataRef = pop_ref(vm, tc);
        int bus = pop_i32(vm, tc);
        char em[128]; uint32_t dataAddr = 0;
        if (bus_array(vm, dataRef, count, "spi.write", "el array", 0, &dataAddr, em, sizeof em) != 0)
            return builtin_throw(vm, tc, em);
        uint8_t tx[BUS_TRAMO];
        push_i32(vm, tc, bus_por_tramos(vm, BUS_SPI_WRITE, bus, dataAddr, 0, count, 0, tx, NULL));
        return BPVM_OK;
    }
    case BUILTIN_SPI_READ: {
        int count = pop_i32(vm, tc);
        uint32_t dataRef = pop_ref(vm, tc);
        int bus = pop_i32(vm, tc);
        char em[128]; uint32_t dataAddr = 0;
        if (bus_array(vm, dataRef, count, "spi.read", "el array", 0, &dataAddr, em, sizeof em) != 0)
            return builtin_throw(vm, tc, em);
        uint8_t rx[BUS_TRAMO];
        push_i32(vm, tc, bus_por_tramos(vm, BUS_SPI_READ, bus, 0, dataAddr, count, 0, NULL, rx));
        return BPVM_OK;
    }
    case BUILTIN_SPI_TRANSFER: {
        int count = pop_i32(vm, tc);
        uint32_t rxRef = pop_ref(vm, tc);
        uint32_t txRef = pop_ref(vm, tc);
        int bus = pop_i32(vm, tc);
        /* rx y luego tx (el orden de los null de miVM, #478), cada uno entero */
        char em[128]; uint32_t rxAddr = 0, txAddr = 0;
        if (bus_array(vm, rxRef, count, "spi.transfer", "el array rx", 0, &rxAddr, em, sizeof em) != 0
            || bus_array(vm, txRef, count, "spi.transfer", "el array tx", 0, &txAddr, em, sizeof em) != 0)
            return builtin_throw(vm, tc, em);
        uint8_t tx[BUS_TRAMO], rx[BUS_TRAMO];
        push_i32(vm, tc, bus_por_tramos(vm, BUS_SPI_TRANSFER, bus, txAddr, rxAddr, count, 0, tx, rx));
        return BPVM_OK;
    }

    /* ---- V7/H6.c G4.a3 — Spi.Device (ver dev_listo y V7_IDEAS §H6.11.9) ---- */
    case BUILTIN_SPI_DEV_OPEN: {
        /* Spi.Device(bus, cs, baudrate, mode): se engancha al bus YA establecido y reserva
         * su CS —salida, en reposo alto—. */
        int mode = pop_i32(vm, tc);
        int hz   = pop_i32(vm, tc);
        int cs   = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        char em[160];
        if (bpvm_gpio_instancia_ok(2, bus, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        if (hz <= 0) { snprintf(em, sizeof em, "Spi.Device: velocidad no válida (%d)", hz); return builtin_throw(vm, tc, em); }
        if (mode < 0 || mode > 3) { snprintf(em, sizeof em, "Spi.Device: modo no válido (%d): 0..3", mode); return builtin_throw(vm, tc, em); }
        hal_cerrar(vm);   /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT): engancharse al bus y su CS */
        int r = bpvm_spi_enganchar(bus, BPVM_BUS_PROGRAMA, em, sizeof em);
        if (r == 0) r = bpvm_gpio_reclamar(cs, BPVM_USO_OUTPUT, em, sizeof em);
        if (r == 0) {
            bpvm_gpio_init(cs, 1);
            bpvm_gpio_write(cs, 1);
        }
        hal_abrir(vm);
        if (r != 0) return builtin_throw(vm, tc, em);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_SPI_DEV_XFER: {
        /* write/read/transfer/writeRead de un Spi.Device: UNA transacción. */
        int32_t nrx = pop_i32(vm, tc);
        uint32_t rxRef = pop_ref(vm, tc);
        int32_t ntx = pop_i32(vm, tc);
        uint32_t txRef = pop_ref(vm, tc);
        int op   = pop_i32(vm, tc);
        int mode = pop_i32(vm, tc);
        int hz   = pop_i32(vm, tc);
        int cs   = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        char em[160];
        const char* nom;
        uint32_t txA = 0, rxA = 0;
        int32_t bytes, res;
        uint8_t tx[BUS_TRAMO], rx[BUS_TRAMO];
        int r;
        if (op < DEV_WRITE || op > DEV_WRITE_READ) return builtin_throw(vm, tc, "Spi.Device: operación no válida");
        nom = dev_nombre(op);
        if (dev_listo(bus, cs, nom, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        switch (op) {
        case DEV_WRITE:
            if (bus_array(vm, txRef, ntx, nom, "el array", 0, &txA, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
            bytes = ntx; res = ntx; break;
        case DEV_READ:
            if (bus_array(vm, rxRef, nrx, nom, "el array", 0, &rxA, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
            bytes = nrx; res = nrx; break;
        case DEV_TRANSFER:   /* rx y luego tx: el orden de siempre (#478) */
            if (bus_array(vm, rxRef, ntx, nom, "el array rx", 0, &rxA, em, sizeof em) != 0
                || bus_array(vm, txRef, ntx, nom, "el array tx", 0, &txA, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
            bytes = ntx; res = ntx; break;
        default:
            if (bus_array(vm, txRef, ntx, nom, "el array tx", 0, &txA, em, sizeof em) != 0
                || bus_array(vm, rxRef, nrx, nom, "el array rx", 0, &rxA, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
            bytes = ntx + nrx; res = nrx; break;
        }
        if (bpvm_bus_cabe(BPVM_BUS_SPI, bus, (unsigned long long) bytes * 8ULL, hz, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        if (bpvm_spi_tomar_transaccion(bus, cs, hz, mode, BPVM_BUS_PROGRAMA, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        if (op == DEV_READ)          r = dev_tramos(vm, bus, 0, 1, 0, 0, rxA, 0, nrx, tx, rx);
        else if (op == DEV_TRANSFER) r = dev_tramos(vm, bus, 1, 1, txA, 0, rxA, 0, ntx, tx, rx);
        else {
            r = dev_tramos(vm, bus, 1, 0, txA, 0, 0, 0, ntx, tx, rx);
            if (r == 0 && op == DEV_WRITE_READ) r = dev_tramos(vm, bus, 0, 1, 0, 0, rxA, 0, nrx, tx, rx);
        }
        bpvm_spi_dejar(bus);
        push_i32(vm, tc, r < 0 ? -1 : res);
        return BPVM_OK;
    }
    case BUILTIN_SPI_DEV_EXEC: {
        /* Spi.Device.execute(t): la Spi.Transaction ya reunida por Spi.bp —`ops` = pares
         * (tipo, bytes); `tx` = lo que se escribe, seguido; `rx` = lo que se lee, seguido—. */
        uint32_t rxRef  = pop_ref(vm, tc);
        uint32_t txRef  = pop_ref(vm, tc);
        int32_t  nops   = pop_i32(vm, tc);
        uint32_t opsRef = pop_ref(vm, tc);
        int mode = pop_i32(vm, tc);
        int hz   = pop_i32(vm, tc);
        int cs   = pop_i32(vm, tc);
        int bus  = pop_i32(vm, tc);
        const char* nom = "spi.execute";
        char em[160];
        uint32_t opsA = 0, txA = 0, rxA = 0;
        int32_t i, pt = 0, pr = 0;
        int64_t sale = 0, entra = 0;
        unsigned long long bytes = 0;
        uint8_t tx[BUS_TRAMO], rx[BUS_TRAMO];
        int r = 0;
        if (dev_listo(bus, cs, nom, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        if (nops < 0 || nops > 0x10000) { snprintf(em, sizeof em, "%s: número de operaciones no válido (%" PRId32 ")", nom, nops); return builtin_throw(vm, tc, em); }
        if (bus_array(vm, opsRef, nops * 2, nom, "el array ops", 0, &opsA, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        for (i = 0; i < nops; i++) {
            int32_t k = arr_i32(vm, opsA, 2 * i), n = arr_i32(vm, opsA, 2 * i + 1);
            if (k < DEV_WRITE || k > DEV_TRANSFER) { snprintf(em, sizeof em, "%s: la operación %" PRId32 " no es válida (%" PRId32 ")", nom, i + 1, k); return builtin_throw(vm, tc, em); }
            if (n < 0) { snprintf(em, sizeof em, "%s: count negativo (%" PRId32 ")", nom, n); return builtin_throw(vm, tc, em); }
            if (k != DEV_READ)  sale += n;
            if (k != DEV_WRITE) entra += n;
            bytes += (unsigned long long) n;
        }
        /* el tope ANTES que los arrays: acota los totales (a 100 ms, mucho menos que 2^31 bytes) */
        if (bpvm_bus_cabe(BPVM_BUS_SPI, bus, bytes * 8ULL, hz, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        if (bus_array(vm, txRef, (int32_t) sale, nom, "el array tx", 0, &txA, em, sizeof em) != 0
            || bus_array(vm, rxRef, (int32_t) entra, nom, "el array rx", 0, &rxA, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        if (bpvm_spi_tomar_transaccion(bus, cs, hz, mode, BPVM_BUS_PROGRAMA, em, sizeof em) != 0) return builtin_throw(vm, tc, em);
        int cambiada = 0;
        for (i = 0; i < nops && r == 0; i++) {
            int32_t k = arr_i32(vm, opsA, 2 * i), n = arr_i32(vm, opsA, 2 * i + 1);
            /* V7/H12, 2b (la revisión) — se vuelve a mirar contra lo validado arriba: con dos runs, otro hilo puede
             * cambiar `ops` entre las dos pasadas (una Transaction compartida sin Mutex), y la de transferir leía y
             * escribía fuera de tx/rx y se saltaba el tope de tiempo del bus. `ops` son 2*nops enteros: no caben en la
             * pila para leerlos una sola vez, como el I2C. */
            if (k < DEV_WRITE || k > DEV_TRANSFER || n < 0
                    || (k != DEV_READ && (int64_t) pt + n > sale) || (k != DEV_WRITE && (int64_t) pr + n > entra)) {
                cambiada = 1;
                break;
            }
            r = dev_tramos(vm, bus, k != DEV_READ, k != DEV_WRITE, txA, pt, rxA, pr, n, tx, rx);
            if (k != DEV_READ)  pt += n;
            if (k != DEV_WRITE) pr += n;
        }
        bpvm_spi_dejar(bus);
        if (cambiada) {
            snprintf(em, sizeof em, "%s: la transacción cambió mientras se ejecutaba (¿compartida entre hilos sin Mutex?)", nom);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, r < 0 ? -1 : (int32_t) bytes);
        return BPVM_OK;
    }

    /* ---- UART ---- mismo layout de buffers que I2C/SPI (TYPE_ARRAY_I32,
       byte por slot). 7 argumentos en init para soportar paridad/bits. */
    case BUILTIN_UART_INIT: {
        int parity    = pop_i32(vm, tc);
        int stop_bits = pop_i32(vm, tc);
        int data_bits = pop_i32(vm, tc);
        int baud      = pop_i32(vm, tc);
        int rx        = pop_i32(vm, tc);
        int tx        = pop_i32(vm, tc);
        int bus       = pop_i32(vm, tc);
        { char em[128]; if (bpvm_gpio_instancia_ok(0, bus, em, sizeof em) != 0) return builtin_throw(vm, tc, em); }   /* V7/H4 */
        hal_cerrar(vm);   /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT): su ranura del anillo de RX */
        bpvm_uart_init(bus, tx, rx, baud, data_bits, stop_bits, parity);
        hal_abrir(vm);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    /* #534 — UART sin tope, en tramos de BUS_TRAMO; en read el plazo cuenta POR TRAMO. */
    case BUILTIN_UART_WRITE: {
        int count = pop_i32(vm, tc);
        uint32_t dataRef = pop_ref(vm, tc);
        int bus = pop_i32(vm, tc);
        char em[128]; uint32_t dataAddr = 0;
        if (bus_array(vm, dataRef, count, "uart.write", "el array", 0, &dataAddr, em, sizeof em) != 0)
            return builtin_throw(vm, tc, em);
        uint8_t tx[BUS_TRAMO];
        push_i32(vm, tc, bus_por_tramos(vm, BUS_UART_WRITE, bus, dataAddr, 0, count, 0, tx, NULL));
        return BPVM_OK;
    }
    case BUILTIN_UART_READ: {
        int timeout = pop_i32(vm, tc);
        int count   = pop_i32(vm, tc);
        uint32_t dataRef = pop_ref(vm, tc);
        int bus = pop_i32(vm, tc);
        char em[128]; uint32_t dataAddr = 0;
        if (bus_array(vm, dataRef, count, "uart.read", "el array", 0, &dataAddr, em, sizeof em) != 0)
            return builtin_throw(vm, tc, em);
        uint8_t rx[BUS_TRAMO];
        push_i32(vm, tc, bus_por_tramos(vm, BUS_UART_READ, bus, 0, dataAddr, count, timeout, NULL, rx));
        return BPVM_OK;
    }
    case BUILTIN_UART_AVAILABLE: {
        int bus = pop_i32(vm, tc);
        push_i32(vm, tc, (int32_t) bpvm_uart_available(bus));
        return BPVM_OK;
    }

    /* ---- Pulse counter ----
       init devuelve counterId (>=0) o -1 si pin inválido.
       Los demás reciben counterId y operan sobre él. */
    case BUILTIN_PULSE_INIT: {
        int edgeKind = pop_i32(vm, tc);
        int pin      = pop_i32(vm, tc);
        {
            char em[128];
            hal_cerrar(vm);   /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT) */
            int r = bpvm_gpio_reclamar(pin, BPVM_USO_PULSE, em, sizeof em);   /* V7/H4 */
            int id_cont = (r == 0) ? bpvm_pulse_init(pin, edgeKind) : -1;
            hal_abrir(vm);
            if (r != 0) return builtin_throw(vm, tc, em);
            push_i32(vm, tc, (int32_t) id_cont);
        }
        return BPVM_OK;
    }
    case BUILTIN_PULSE_START: {
        int counterId = pop_i32(vm, tc);
        bpvm_pulse_start(counterId);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_PULSE_STOP: {
        int counterId = pop_i32(vm, tc);
        bpvm_pulse_stop(counterId);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_PULSE_VALUE: {
        int counterId = pop_i32(vm, tc);
        push_i32(vm, tc, (int32_t) bpvm_pulse_value(counterId));
        return BPVM_OK;
    }
    case BUILTIN_PULSE_RESET: {
        int counterId = pop_i32(vm, tc);
        bpvm_pulse_reset(counterId);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    /* ---- PWM ----
       init devuelve sliceId (>=0) o -1 si pin inválido. setDuty
       recibe (sliceId, pin, dutyPct) — el pin para distinguir
       canal A o B del mismo slice. */
    case BUILTIN_PWM_INIT: {
        int freqHz = pop_i32(vm, tc);
        int pin    = pop_i32(vm, tc);
        {
            char em[128];
            hal_cerrar(vm);   /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT) */
            int r = bpvm_gpio_reclamar(pin, BPVM_USO_PWM, em, sizeof em);   /* V7/H4 */
            int slice = (r == 0) ? bpvm_pwm_init(pin, freqHz) : -1;
            hal_abrir(vm);
            if (r != 0) return builtin_throw(vm, tc, em);
            push_i32(vm, tc, (int32_t) slice);
        }
        return BPVM_OK;
    }
    case BUILTIN_PWM_SET_FREQ: {
        int freqHz  = pop_i32(vm, tc);
        int sliceId = pop_i32(vm, tc);
        bpvm_pwm_set_freq(sliceId, freqHz);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_PWM_SET_DUTY: {
        int dutyPct = pop_i32(vm, tc);
        int pin     = pop_i32(vm, tc);
        int sliceId = pop_i32(vm, tc);
        bpvm_pwm_set_duty(sliceId, pin, dutyPct);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_PWM_START: {
        int sliceId = pop_i32(vm, tc);
        bpvm_pwm_start(sliceId);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_PWM_STOP: {
        int sliceId = pop_i32(vm, tc);
        bpvm_pwm_stop(sliceId);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    /* V7 #667 — SOLTAR lo que un objeto tomó (sus destructores y su release()): el backend suelta la unidad y deja
     * el pin como en el reset; el común lo apunta libre. Bajo el cerrojo de la HAL, como al tomarlo. */
    case BUILTIN_PWM_RELEASE: {
        int pin     = pop_i32(vm, tc);
        int sliceId = pop_i32(vm, tc);
        hal_cerrar(vm);
        bpvm_pwm_release(sliceId, pin);
        bpvm_gpio_liberar(pin);
        hal_abrir(vm);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_PULSE_RELEASE: {
        int pin       = pop_i32(vm, tc);
        int counterId = pop_i32(vm, tc);
        hal_cerrar(vm);
        bpvm_pulse_release(counterId, pin);
        bpvm_gpio_liberar(pin);
        hal_abrir(vm);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_NP_RELEASE: {
        int pin = pop_i32(vm, tc);
        hal_cerrar(vm);
        bpvm_neopixel_release(pin);
        bpvm_gpio_liberar(pin);
        hal_abrir(vm);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_UART_RELEASE: {
        int rx  = pop_i32(vm, tc);
        int tx  = pop_i32(vm, tc);
        int bus = pop_i32(vm, tc);
        hal_cerrar(vm);
        bpvm_uart_release(bus, tx, rx);
        hal_abrir(vm);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    /* ---- Pico (info del MCU) ----
       uniqueId y boardName devuelven string (alocado en heap BP);
       tempC devuelve float (bit-cast a u32 para pushar al stack);
       cpuFreqHz y uptimeMs devuelven int. */
    case BUILTIN_MACHINE_UNIQUE_ID: {
        char buf[32];
        bpvm_machine_unique_id(buf, sizeof(buf));
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, strlen(buf));
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");   /* #355: OOM ATRAPABLE, antes se empujaba una ref NULA en silencio */
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_MICRO: {
        /* El MICRO, por la fachada HAL BP — la MISMA funcion que lee el wire. */
        char mbuf[32];
        bpvm_machine_micro_name(mbuf, sizeof(mbuf));
        uint32_t mref = bpvm_heap_alloc_string(vm, mbuf, strlen(mbuf));
        if (mref == 0) return builtin_throw(vm, tc, "No space in heap");
        push_ref(vm, tc, mref);
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_BOARD_NAME: {
        char buf[32];
        bpvm_machine_board_name(buf, sizeof(buf));
        uint32_t ref = bpvm_heap_alloc_string(vm, buf, strlen(buf));
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");   /* #355: OOM ATRAPABLE, antes se empujaba una ref NULA en silencio */
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_RESET_CAUSE: {   /* H10 — causa del último reset (string) */
        const char* s = bpvm_machine_reset_cause();
        uint32_t ref = bpvm_heap_alloc_string(vm, s, strlen(s));
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");   /* #355: OOM ATRAPABLE, antes se empujaba una ref NULA en silencio */
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }
    /* ── V7 #668 — LIBERAR ES DESTRUIR: las primitivas de Core.__destruye y Core.__destruyePendientes (destruir.c). */
    case BUILTIN_LIBERA_SI_SIN_DESTRUCTORES: {   /* ningún destructor en su árbol de owner: lo suelta entero */
        bpref_t x = bpref_pop(vm, tc);
        push_i32(vm, tc, bpvm_destruir_libera_si_sin(vm, x));
        return BPVM_OK;
    }
    case BUILTIN_ORDEN_DESTRUCCION: {   /* su árbol de owner en el orden de destruir: los hijos antes que su padre */
        bpref_t x = bpref_load(vm, tc->sp - BPVM_REF_SIZE);   /* se queda en la pila mientras se reserva: es raíz */
        uint64_t* orden = NULL;
        uint32_t n = 0;
        if (bpvm_destruir_orden(vm, x, &orden, &n) != 0) return builtin_throw(vm, tc, "No space in heap");
        bpref_t arr = bpvm_heap_nuevo(vm, BPVM_TYPE_ARRAY_REF, n, n * BPVM_REF_SIZE);
        if (arr.v == 0u) { bpvm_free(orden); return builtin_throw(vm, tc, "No space in heap"); }
        uint32_t base = bpref_deref(vm, arr) + BPVM_ARR_DATA_OFF;
        for (uint32_t i = 0; i < n; i++) { bpref_t e; e.v = orden[i]; bpref_store(vm, base + i * BPVM_REF_SIZE, e); }
        bpvm_free(orden);
        (void) bpref_pop(vm, tc);
        bpref_push(vm, tc, arr);
        return BPVM_OK;
    }
    case BUILTIN_PREPARA_DESTRUCTOR: {   /* 0, ninguno; 1, de Object; 2, de Thread; y olvida a sus hijos owner destruidos */
        bpref_t x = bpref_pop(vm, tc);
        push_i32(vm, tc, bpvm_destruir_prepara(vm, x));
        return BPVM_OK;
    }
    case BUILTIN_LIBERA_SOLO: {   /* la memoria de ese bloque y nada más: sus hijos ya no están */
        bpref_t x = bpref_pop(vm, tc);
        bpvm_heap_soltar_solo(vm, x);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_AVISA_DESTRUCTOR: {   /* un destructor lanzó: se dice, con la clase, y se sigue */
        uint32_t msg = pop_ref(vm, tc);
        bpref_t x = bpref_pop(vm, tc);
        char clase[160], texto[200];
        bpvm_destruir_nombre(vm, x, clase, sizeof clase);
        (void) read_bp_string(vm, msg, texto, sizeof texto);
        bpvm_diag_urgente("[bpvm] el destructor de %s lanzó: %s — se sigue con la limpieza", clase, texto);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_SIGUIENTE_PENDIENTE: {   /* lo siguiente que hay pendiente de destruir, o null */
        bpref_t r; r.v = bpvm_destruir_siguiente(vm, tc);
        bpref_push(vm, tc, r);
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_GET_ENV: {   /* V7/H10 #626 — una clave del ENV de la placa; "" si no está (el PC no tiene ENV) */
        uint32_t kref = pop_ref(vm, tc);
        char clave[64], valor[256];   /* las del wire: una clave cabe en 63, un valor en 191 (bpvm_bmgr_req_t) */
        int n = -1;
        if (read_bp_path(vm, kref, clave, sizeof clave) == 0)   /* una clave más larga no puede estar */
            n = bpvm_machine_env_get(clave, valor, sizeof valor);
        if (n == -2) return builtin_throw(vm, tc, "Machine.getEnv: el valor de la clave no cabe (mas de 255 bytes)");
        if (n < 0) n = 0;
        uint32_t ref = bpvm_heap_alloc_string(vm, valor, (size_t) n);
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_SET_MARK: {      /* H10 — breadcrumb: deja una miga */
        int32_t code = pop_i32(vm, tc);
        hal_cerrar(vm);   /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT): leer el total y escribir la miga, juntos */
        bpvm_machine_set_mark((int) code);
        hal_abrir(vm);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_MARK_COUNT: {    /* H10 — nº de migas del trail previo */
        push_i32(vm, tc, (int32_t) bpvm_machine_mark_count());
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_MARK_AT: {       /* H10 — i-ésima miga (0 = origen) */
        int32_t i = pop_i32(vm, tc);
        push_i32(vm, tc, (int32_t) bpvm_machine_mark_at((int) i));
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_BOOT_COUNT: {    /* H10 — arranques desde power-on */
        push_i32(vm, tc, (int32_t) bpvm_machine_boot_count());
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_TEMP_C: {
        hal_cerrar(vm);   /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT): el ADC (su canal y sus lecturas) */
        float v = bpvm_machine_temp_c();
        hal_abrir(vm);
        /* Float en pila es bit-cast a u32 — la VM hace el mismo
         * truco para LITERAL_F32. */
        uint32_t bits;
        memcpy(&bits, &v, sizeof(bits));
        push_i32(vm, tc, (int32_t) bits);
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_CPU_FREQ_HZ: {
        push_i32(vm, tc, (int32_t) bpvm_machine_cpu_freq_hz());
        return BPVM_OK;
    }
    /* ── V7/H4 — la tabla de capacidades desde BP (Machine.uarts(), pins(), …) ── */
    case BUILTIN_CAPS_ARRAY: {
        int32_t kind = pop_i32(vm, tc);
        const bpvm_caps_t* c = bpvm_caps();
        /* 0 uart · 1 i2c · 2 spi (instancias); 3 pines que existen · 4 analógicos · 5 PWM · 6 pulsos · 7 colector abierto */
        const uint8_t* lista = NULL; int n = 0; uint16_t bit = 0;
        switch (kind) {
            case 0: lista = c->uart; n = c->n_uart; break;
            case 1: lista = c->i2c;  n = c->n_i2c;  break;
            case 2: lista = c->spi;  n = c->n_spi;  break;
            case 3: bit = 0xFFFFu; break;
            case 4: bit = BPVM_PIN_ANALOG; break;
            case 5: bit = BPVM_PIN_PWM; break;
            case 6: bit = BPVM_PIN_PULSE; break;
            case 7: bit = BPVM_PIN_OPEN_DRAIN; break;
            default: return builtin_throw(vm, tc, "Machine: clase de capacidad desconocida");
        }
        /* #669: por la máscara, que mira las reservas del sistema (la pantalla de la imagen): esos pines no están */
        if (bit) { n = 0; for (int i = 0; i < (int) c->n_pin; i++) if (bpvm_caps_pin_mask(i) & bit) n++; }
        bpref_t arrh = bpvm_heap_nuevo(vm, BPVM_TYPE_ARRAY_I32, (uint32_t) n, (uint32_t) n * 4u); /* V7/H7 #593: la cabecera y el handle, al nacer */
        if (arrh.v == 0u) return builtin_throw(vm, tc, "Machine: sin memoria");
        uint32_t arr = bpref_deref(vm, arrh);
        int k = 0;
        if (bit) { for (int i = 0; i < (int) c->n_pin; i++) if (bpvm_caps_pin_mask(i) & bit) bpvm_write_i32_be(vm->memory + arr + 4 + 4 * (k++), i); }
        else     { for (int i = 0; i < n; i++) bpvm_write_i32_be(vm->memory + arr + 4 + 4 * (k++), lista[i]); }
        bpref_push(vm, tc, arrh);
        return BPVM_OK;
    }
    case BUILTIN_CAPS_INT: {
        int32_t kind = pop_i32(vm, tc);
        const bpvm_caps_t* c = bpvm_caps();
        int32_t v = 0;
        switch (kind) {   /* 0 max Hz · 1 min Hz · 2 ajustable · 3 salidas PWM · 4 unidades ADC · 5 migas · 6 tope del mapa */
            case 0: v = (int32_t) c->cpu_hz_max; break;
            case 1: v = (int32_t) c->cpu_hz_min; break;
            case 2: v = c->cpu_ajustable; break;
            case 3: v = c->n_pwm; break;
            case 4: v = c->n_adc; break;
            case 5: v = c->breadcrumb; break;
            case 6: v = c->n_pin; break;
            default: return builtin_throw(vm, tc, "Machine: clase de capacidad desconocida");
        }
        push_i32(vm, tc, v);
        return BPVM_OK;
    }
    case BUILTIN_CAPS_STR: {
        int32_t kind = pop_i32(vm, tc);
        const bpvm_caps_t* c = bpvm_caps();
        const char* s = "";
        switch (kind) {   /* 0 micro · 1 paquete · 2 fabricante · 3 unidad del PWM */
            case 0: s = c->micro; break;
            case 1: s = c->paquete; break;
            case 2: s = c->fabricante; break;
            case 3: s = c->pwm_unidad; break;
            default: return builtin_throw(vm, tc, "Machine: clase de capacidad desconocida");
        }
        if (!s) s = "";
        uint32_t out = bpvm_heap_alloc_string(vm, s, strlen(s));
        if (out == 0) return builtin_throw(vm, tc, "Machine: sin memoria");
        push_ref(vm, tc, out);
        return BPVM_OK;
    }
    case BUILTIN_CAPS_PIN: {
        int32_t pin = pop_i32(vm, tc);
        push_i32(vm, tc, (int32_t) bpvm_caps_pin_mask(pin));
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_GPIO_COUNT: {
        push_i32(vm, tc, (int32_t) bpvm_machine_gpio_count());
        return BPVM_OK;
    }
    /* V7 (28-sep) — el diagnóstico del heap DESDE BP (Heap.bp): contadores de ESTA VM, sin reservar (heap.c,
     * bpvm_heap_cuentas). Espejo de los HEAP_* de miVM. */
    case BUILTIN_HEAP_SIZE:
    case BUILTIN_HEAP_FREE:
    case BUILTIN_HEAP_LARGEST_FREE:
    case BUILTIN_HEAP_FREE_BLOCKS:
    case BUILTIN_HEAP_HANDLES:
    case BUILTIN_HEAP_COLLECTIONS: {
        bpvm_heap_cuentas_t c;
        bpvm_heap_cuentas(vm, &c);
        uint32_t v = (id == BUILTIN_HEAP_SIZE)         ? c.tamano
                   : (id == BUILTIN_HEAP_FREE)         ? c.libre
                   : (id == BUILTIN_HEAP_LARGEST_FREE) ? c.mayor_libre
                   : (id == BUILTIN_HEAP_FREE_BLOCKS)  ? c.huecos
                   : (id == BUILTIN_HEAP_HANDLES)      ? c.handles
                   :                                     c.colectas;
        push_i32(vm, tc, (int32_t) v);
        return BPVM_OK;
    }
    /* V7/H7 #595 (28-sep) — Heap.compact(): las condiciones, en el MISMO orden y con los MISMOS textos que miVM
     * (heapCompact), y la compactación con el mundo parado (heap.c, bpvm_heap_compactar). */
    case BUILTIN_HEAP_COMPACT: {
        /* 1. DESDE Main: el marco actual lo abrió el CALL de __startup —su bp de vuelta es el fondo de la pila del
         *    hilo principal y su pc de vuelta es código, no un centinela de [0, 0x100) (el de un handler de evento
         *    o el del puente de una native)—. Y ninguna native a medias en este hilo: una native no abre marco BP
         *    (sus variables son de C), así que el marco no lo diría; en miVM la native es bytecode y sí lo dice. */
        uint32_t tbp = tc->bp;
        int desde_main = (tc == vm->hilo_main)
            && tbp > tc->stack_base
            && bpvm_read_u32_be(vm->memory + tbp - 8u) == tc->stack_base
            && bpvm_read_u32_be(vm->memory + tbp - 12u) >= BPVM_INITIAL_FREE_ADDR
            && tc->nativa == NULL && !tc->builtin_de_nativa && bpvm_aot_callctx()->tc == NULL;
        if (!desde_main) return builtin_throw(vm, tc, BPVM_MSG_COMPACT_MAIN);
        /* 2. SIN OTROS HILOS VIVOS —ninguno parado con una dirección cruda (un builtin bloqueado con su array)—, y
         * 3. colecta y compacta: las dos con el mundo parado (heap.c). Si el recorrido se descarrila (#494), la
         *    prefabricada: queda armada y bpvm_throw_runtime_error la entrega, sea cual sea el texto que se le pase. */
        int r = bpvm_heap_compactar(vm, tc);
        if (r == 2) return builtin_throw(vm, tc, BPVM_MSG_COMPACT_HILOS);
        if (r == 1) return builtin_throw(vm, tc, BPVM_MSG_HEAP_DESCARRILADO);
        push_i32(vm, tc, 0);   /* void → push dummy */
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_ADC_CHANNELS: {
        push_i32(vm, tc, (int32_t) bpvm_machine_adc_channels());
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_PWM_SLICES: {
        push_i32(vm, tc, (int32_t) bpvm_machine_pwm_slices());
        return BPVM_OK;
    }
    case BUILTIN_NP_INIT: {
        /* __npInit(pin): void
         *
         * H13 hallazgo 21(b) — ANTES se llamaba y se TIRABA el resultado, asi que
         * un programa pedia un NeoPixel donde no hay driver, le decian que si, y
         * escribia colores al vacio sin forma de enterarse. La fachada nunca
         * minitio: bpvm_neopixel.h documenta "1 OK, 0 fallo" y sin backend
         * devuelve 0. El que mentia era ESTE builtin al no mirarlo.
         *
         * Mismo protocolo que lastModified (13a): no se contesta exito, se dice
         * que aqui NO ESTA IMPLEMENTADO — y se habla de la plataforma, no del
         * chip, porque manana puede haber driver en otra familia. */
        int pin = pop_i32(vm, tc);
        char em[160];
        if (!bpvm_neopixel_disponible()) {
            snprintf(em, sizeof(em),
                     "Neopixel(%d): no implementado en esta plataforma", pin);
            return builtin_throw(vm, tc, em);
        }
        /* V7/H4 (26-sep, Eduardo) — con driver, el pin se valida contra la tabla como el del PWM y el del
         * Pulse (#479): «el pin 25 no existe en esp32s3», «es del SPI 2», «es de la flash o la PSRAM» (#574)…
         * Sin driver no se mira nada: el PC y NeoCatch siguen diciendo lo mismo en las dos VMs. */
        /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT): el pin y la tabla de tiras del driver, de una vez */
        hal_cerrar(vm);
        int r = bpvm_gpio_reclamar(pin, BPVM_USO_OUTPUT, em, sizeof em);
        /* Y si el driver dice que no —sin canal libre—, se dice ESO: hasta hoy también era «no implementado». */
        if (r == 0) {
            int k = snprintf(em, sizeof em, "Neopixel(%d): no se pudo iniciar: ", pin);
            if (!bpvm_neopixel_init(pin, em + k, sizeof em - (size_t) k)) {
                if (!em[k]) snprintf(em + k, sizeof em - (size_t) k, "el driver no lo aceptó");
                r = -1;
            }
        }
        hal_abrir(vm);
        if (r != 0) return builtin_throw(vm, tc, em);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_NP_SHOW: {
        /* __npShow(pin, grb: integer[], count): void — empuja count palabras
         * GRB del array (mismo layout que I2C/SPI: ref+4 = primer i32 BE). */
        int32_t count = pop_i32(vm, tc);
        uint32_t grbRef = pop_ref(vm, tc);
        int pin = pop_i32(vm, tc);
        /* #536 (V7, 25-sep) — el array se valida como los de bus (#534): null, count negativo o mayor
         * que el array → RuntimeError con nombre, con los MISMOS textos en miVM. Antes leía `count`
         * elementos sin mirar la longitud, y recortaba a 256 LEDs en silencio. El tope es ahora la
         * zona de rascar ENTERA (Eduardo, 25-sep): 2048 LEDs en RP2350, ESP32 y el PC, 4096 en STM32;
         * por encima, error con nombre. Una tira NO se parte en tramos como las de bus: el WS2812 hace
         * latch tras ~50 µs de silencio y una segunda ráfaga volvería a empezar por el píxel 0. */
        char em[128];
        uint32_t grbAddr = 0;
        if (bus_array(vm, grbRef, count, "neopixel.show", "el array", 0, &grbAddr, em, sizeof em) != 0)
            return builtin_throw(vm, tc, em);
        int32_t np_max = (int32_t) (bpvm_scratch_capacity() / sizeof(uint32_t));
        if (count > np_max) {
            snprintf(em, sizeof em, "neopixel.show: una tira de %" PRId32 " LEDs no cabe: como mucho %" PRId32 " por trama",
                     count, np_max);
            return builtin_throw(vm, tc, em);
        }
        /* #338 — el buffer de transcodificación (BE del heap BP → nativo para
         * el driver) sale de la zona compartida: 1 KB que sólo hace falta
         * mientras se empuja una trama, no toda la vida del firmware.
         *
         * Soltarlo aquí mismo es SEGURO porque el driver es BLOQUEANTE: el
         * backend del Pico mete las palabras con pio_sm_put_blocking y espera
         * los 60 µs de latch del WS2812, así que al volver el buffer ya está
         * consumido. Si algún día un backend lo hiciera por DMA habría que
         * retener la zona hasta que termine — está dicho aquí para que no se
         * descubra por una tira parpadeando en colores raros.
         *
         * De propina se cierra un latente: esto era `static` con el comentario
         * "single-worker: estático OK". Bajo SMP dos workers llamando a la vez
         * se pisaban EN SILENCIO; ahora el segundo se encuentra la zona cogida
         * y el guardián lo dice con nombres. */
        /* V7/H12, 2b: con dos runs, si la tiene el otro, se espera (zona_para_un_builtin); y después se vuelve a
         * validar el array, que mientras tanto pudo pasar un GC. */
        uint32_t* npbuf = (uint32_t*) zona_para_un_builtin(vm, (size_t) (count > 0 ? count : 1) * sizeof(uint32_t),
                                                           "__npShow");
        if (!npbuf) {          /* el guardián ya ha dicho por qué */
            push_i32(vm, tc, 0);
            return BPVM_OK;
        }
        if (bus_array(vm, grbRef, count, "neopixel.show", "el array", 0, &grbAddr, em, sizeof em) != 0) {
            bpvm_scratch_give("__npShow");
            return builtin_throw(vm, tc, em);
        }
        int n = (int) count;
        for (int i = 0; i < n; i++) {
            npbuf[i] = (uint32_t) bpvm_read_i32_be(vm->memory + grbAddr + 4 + i * 4);
        }
        bpvm_neopixel_show(pin, npbuf, n);
        bpvm_scratch_give("__npShow");
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_UPTIME_MS: {
        push_i32(vm, tc, (int32_t) bpvm_machine_uptime_ms());
        return BPVM_OK;
    }
    case BUILTIN_MACHINE_SET_CPU_FREQ_MHZ: {
        /* Cambia el clk_sys del RP2350. En host es un no-op (stub
         * loguea por stdout). El clamp al máximo soportado lo hace BP
         * en Pico.bp (usando la constante MAX_CPU_MHZ) — aquí solo
         * delegamos al backend. */
        int32_t mhz = pop_i32(vm, tc);
        int ok = bpvm_machine_set_cpu_freq_mhz((int) mhz);
        push_i32(vm, tc, ok ? 1 : 0);
        return BPVM_OK;
    }
    case BUILTIN_RTC_NOW_SEC: {
        /* Wall clock en segundos. Si nadie ha llamado a setNowSec
         * desde el boot, devuelve segundos desde boot — todavía
         * útil como reloj monotónico. */
        hal_cerrar(vm);   /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT): el desfase, 64 bits en una de 32 */
        int64_t now_ms = bpvm_rtc_now_ms();
        hal_abrir(vm);
        push_i32(vm, tc, (int32_t) (now_ms / 1000));
        return BPVM_OK;
    }
    case BUILTIN_RTC_SET_NOW_SEC: {
        int32_t sec = pop_i32(vm, tc);
        hal_cerrar(vm);   /* V7/H12, 2b: ídem */
        bpvm_rtc_set_now_ms((int64_t) sec * 1000LL);
        hal_abrir(vm);
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_ADC_INIT_CHANNEL: {
        /* V7/H4 (#479) — la vista POR CANAL (unidad 1): el canal existe si el mapa tiene un pin
         * con él; ese pin pasa a ANALOG en el común (error si está en uso digital). */
        int32_t ch = pop_i32(vm, tc);
        char em[128];
        int pin = bpvm_caps_pin_de_adc(0, (int) ch);
        if (pin < 0) {
            /* #689 (12-oct, el F1 de V7) — un canal que EXISTE pero cuyo pin es del sistema (el GP47 de la Metro con la
             * PSRAM: su CS) no es «no existe»: se dice el pin y su motivo. Decía «el canal 7 no existe en rp2350b
             * (unidad 1: 8 canales)», que se contradice a sí mismo. */
            const bpvm_caps_t* c = bpvm_caps();
            int n = 0, crudo = -1;
            for (int i = 0; i < (int) c->n_pin; i++)
                if (c->pin_adc_unit && c->pin_adc_unit[i] == 0) { n++; if (crudo < 0 && c->pin_adc_ch[i] == ch) crudo = i; }
            if (crudo >= 0) {
                char porque[96];
                bpvm_caps_pin_ausente(crudo, porque, sizeof porque);
                snprintf(em, sizeof em, "ADC: el canal %d: %s", (int) ch, porque);
            } else
                snprintf(em, sizeof em, "ADC: el canal %d no existe en %s (unidad 1: %d canales)", (int) ch, c->micro, n);
            return builtin_throw(vm, tc, em);
        }
        hal_cerrar(vm);   /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT) */
        int r = bpvm_gpio_reclamar(pin, BPVM_USO_ANALOG, em, sizeof em);
        int ri = (r == 0) ? bpvm_adc_init_channel((int) ch) : 0;
        hal_abrir(vm);
        if (r != 0) return builtin_throw(vm, tc, em);
        if (ri < 0) { snprintf(em, sizeof em, "ADC: el canal %d no se pudo iniciar en esta imagen", (int) ch); return builtin_throw(vm, tc, em); }
        push_i32(vm, tc, pin);
        return BPVM_OK;
    }
    case BUILTIN_ADC_INIT_PIN: {
        /* La vista POR PIN (AnPin): (unidad, canal) del mapa; hoy los backends atienden la unidad 1. */
        int32_t pin = pop_i32(vm, tc);
        char em[128];
        int unit = -1, ch = -1;
        if (bpvm_caps_adc_de_pin((int) pin, &unit, &ch) != 0) {
            if (!bpvm_caps_pin_mask((int) pin)) bpvm_caps_pin_ausente((int) pin, em, sizeof em);   /* #574 */
            else snprintf(em, sizeof em, "el pin %d de %s no es analógico", (int) pin, bpvm_caps()->micro);
            return builtin_throw(vm, tc, em);
        }
        if (unit != 0) { snprintf(em, sizeof em, "el ADC del pin %d es la unidad %d y esta imagen sólo atiende la 1", (int) pin, unit + 1); return builtin_throw(vm, tc, em); }
        hal_cerrar(vm);   /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT) */
        int r = bpvm_gpio_reclamar((int) pin, BPVM_USO_ANALOG, em, sizeof em);
        int ri = (r == 0) ? bpvm_adc_init_channel(ch) : 0;
        hal_abrir(vm);
        if (r != 0) return builtin_throw(vm, tc, em);
        if (ri < 0) { snprintf(em, sizeof em, "ADC: el pin %d no se pudo iniciar en esta imagen", (int) pin); return builtin_throw(vm, tc, em); }
        push_i32(vm, tc, ch);
        return BPVM_OK;
    }
    case BUILTIN_ADC_READ_CHANNEL: {
        int32_t ch = pop_i32(vm, tc);
        /* V7/H12, 2b: la HAL, bajo el cerrojo (GPIO_INIT): elegir el canal y convertir, juntos (el conversor es uno) */
        hal_cerrar(vm);
        int v = bpvm_adc_read_channel((int) ch);
        hal_abrir(vm);
        push_i32(vm, tc, v);
        return BPVM_OK;
    }
    /* #480 - Sin backend NO se finge. Antes la fachada escribia
     * "[wdt] enable(N ms) (stub, no-op)" y seguia: el programa se creia
     * protegido y no lo estaba. Criterio de Eduardo: «el watchdog se ha de
     * implementar completo o no se implementa». Los mensajes son byte a byte
     * los de miVM (VirtualMachine.java, case WDT_*) - es paridad, no estilo. */
    case BUILTIN_WDT_ENABLE: {
        int32_t ms = pop_i32(vm, tc);
        if (bpvm_wdt_enable((int) ms) != 0) {
            char em[96];
            snprintf(em, sizeof(em), "Wdt.enable(%d): no implementado en esta plataforma", (int) ms);
            return builtin_throw(vm, tc, em);
        }
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_WDT_FEED: {
        if (bpvm_wdt_feed() != 0)
            return builtin_throw(vm, tc, "Wdt.feed(): no implementado en esta plataforma");
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }
    case BUILTIN_WDT_DISABLE: {
        if (bpvm_wdt_disable() != 0)
            return builtin_throw(vm, tc, "Wdt.disable(): no implementado en esta plataforma");
        push_i32(vm, tc, 0);
        return BPVM_OK;
    }

    case BUILTIN_PATH_ABSOLUTE: {
        /* #484 - EL NOMBRE QUE EL FS VA A USAR, y nada mas. Espejo exacto de
         * miVM (VirtualMachine.java, case PATH_ABSOLUTE / pathAbsoluteBp):
         *
         *   empieza por '/'        -> tal cual (ya es absoluto)
         *   tiene esquema "x:..."  -> tal cual (direccion exacta, #362)
         *   hay proyecto           -> <projectPath>/<path>
         *   si no                  -> tal cual (relativo se queda relativo)
         *
         * NO mira el disco y NO normaliza "..": es una funcion PURA de (path,
         * projectPath), y por eso las dos VMs pueden dar lo mismo sin depender
         * del sistema de ficheros de debajo. Decision de Eduardo (9-sep):
         * «trabajamos con paths relativos siempre que podamos, eso lo hace mas
         * transportable». */
        uint32_t pref = pop_ref(vm, tc);
        char p[BPVM_FS_PATH_MAX];
        if (read_bp_path(vm, pref, p, sizeof(p)) != 0)
            return builtin_throw(vm, tc, "path demasiado largo");
        const char* base = bpvm_fs_basedir();
        char out[BPVM_FS_PATH_MAX];
        const char* r = p;                       /* por defecto, tal cual */
        if (p[0] != '\0' && p[0] != '/') {
            int esquema = 0;
            for (const char* q = p; *q; q++) {
                if (*q == '/' || *q == '\\') break;      /* ya es ruta */
                if (*q == ':') { esquema = 1; break; }
            }
            if (!esquema && base[0] != '\0') {
                /* #456 - se mide ANTES de componer: dos paths que caben por
                 * separado pueden no caber juntos, y truncar aqui daria un
                 * nombre que apunta a OTRO fichero. (El C3 compila con
                 * -Werror=format-truncation y lo caza; las otras familias no:
                 * los flags son de cada familia.) */
                size_t lb = strlen(base), lp = strlen(p);
                if (lb + 1u + lp + 1u > sizeof(out))
                    return builtin_throw(vm, tc, "path demasiado largo");
                memcpy(out, base, lb);
                out[lb] = '/';
                memcpy(out + lb + 1u, p, lp + 1u);
                r = out;
            }
        }
        uint32_t ref = bpvm_heap_alloc_string(vm, r, strlen(r));
        if (ref == 0) return builtin_throw(vm, tc, "No space in heap");
        push_ref(vm, tc, ref);
        return BPVM_OK;
    }

    case BUILTIN_PROMPT: {
        /* #484 - el mensaje es CONTRATO DE PARIDAD: byte a byte el de miVM
         * (VirtualMachine.java, case PROMPT, rama sin promptSender). */
        (void) pop_ref(vm, tc);            /* spec: se descarta, no hay a quien mandarla */
        return builtin_throw(vm, tc, "prompt: no hay IDE conectado");
    }

    default: {
        /* GAP-1: builtin fuera del subconjunto de esta VM-C. En vez de abortar
         * la VM con BAD_OPCODE (crash duro, no diagnosticable desde BP), lanzamos
         * un RuntimeError BP *atrapable*: el programa puede capturarlo con
         * try/catch o terminar con un mensaje claro. Fallo limpio. */
        char msg[96];
        snprintf(msg, sizeof(msg),
                 "builtin %d no soportado en esta VM (subconjunto C)", id);
        return builtin_throw(vm, tc, msg);
    }
    }
}
