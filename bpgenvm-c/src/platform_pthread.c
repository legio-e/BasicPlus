/*
 * platform_pthread.c — backend de host con libpthread.
 *
 * Cubre: Linux, macOS, MinGW (w64devkit incluye libwinpthread).
 *
 * Los handles opacos `void*` se mapean a `pthread_mutex_t*`,
 * `pthread_cond_t*`, `pthread_t*` alocados en el heap.
 */
#include "bpvm_platform.h"

#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/* ---- Mutex ---- */

int bpvm_platform_mutex_init(bpvm_platform_mutex_handle_t* m) {
    pthread_mutex_t* mx = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!mx) return -1;
    if (pthread_mutex_init(mx, NULL) != 0) { free(mx); return -1; }
    *m = mx;
    return 0;
}

void bpvm_platform_mutex_destroy(bpvm_platform_mutex_handle_t* m) {
    if (!m || !*m) return;
    pthread_mutex_t* mx = (pthread_mutex_t*) *m;
    pthread_mutex_destroy(mx);
    free(mx);
    *m = NULL;
}

void bpvm_platform_mutex_lock(bpvm_platform_mutex_handle_t* m) {
    pthread_mutex_lock((pthread_mutex_t*) *m);
}

void bpvm_platform_mutex_unlock(bpvm_platform_mutex_handle_t* m) {
    pthread_mutex_unlock((pthread_mutex_t*) *m);
}

/* ---- Condvar ---- */

int bpvm_platform_cond_init(bpvm_platform_cond_handle_t* c) {
    pthread_cond_t* cv = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
    if (!cv) return -1;
    if (pthread_cond_init(cv, NULL) != 0) { free(cv); return -1; }
    *c = cv;
    return 0;
}

void bpvm_platform_cond_destroy(bpvm_platform_cond_handle_t* c) {
    if (!c || !*c) return;
    pthread_cond_t* cv = (pthread_cond_t*) *c;
    pthread_cond_destroy(cv);
    free(cv);
    *c = NULL;
}

void bpvm_platform_cond_wait(bpvm_platform_cond_handle_t* c, bpvm_platform_mutex_handle_t* m) {
    pthread_cond_wait((pthread_cond_t*) *c, (pthread_mutex_t*) *m);
}

int bpvm_platform_cond_timed_wait(bpvm_platform_cond_handle_t* c, bpvm_platform_mutex_handle_t* m, int ms) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec  += ms / 1000;
    ts.tv_nsec += (long)(ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) { ts.tv_sec += 1; ts.tv_nsec -= 1000000000L; }
    int r = pthread_cond_timedwait((pthread_cond_t*) *c,
                                    (pthread_mutex_t*) *m, &ts);
    return (r == ETIMEDOUT) ? 1 : 0;
}

void bpvm_platform_cond_signal(bpvm_platform_cond_handle_t* c) {
    pthread_cond_signal((pthread_cond_t*) *c);
}

void bpvm_platform_cond_broadcast(bpvm_platform_cond_handle_t* c) {
    pthread_cond_broadcast((pthread_cond_t*) *c);
}

/* ---- Thread ---- */

typedef struct {
    bpvm_thread_entry_t entry;
    void* arg;
} pthread_trampoline_arg_t;

static void* pthread_trampoline(void* raw) {
    pthread_trampoline_arg_t* t = (pthread_trampoline_arg_t*) raw;
    bpvm_thread_entry_t entry = t->entry;
    void* arg = t->arg;
    free(t);
    entry(arg);
    return NULL;
}

int bpvm_platform_thread_create(bpvm_platform_thread_handle_t* th,
                                 bpvm_thread_entry_t entry, void* arg) {
    pthread_t* p = (pthread_t*) malloc(sizeof(pthread_t));
    if (!p) return -1;
    pthread_trampoline_arg_t* t = (pthread_trampoline_arg_t*) malloc(sizeof(*t));
    if (!t) { free(p); return -1; }
    t->entry = entry;
    t->arg   = arg;
    int r = pthread_create(p, NULL, pthread_trampoline, t);
    if (r != 0) { free(p); free(t); return -1; }
    *th = p;
    return 0;
}

void bpvm_platform_thread_join(bpvm_platform_thread_handle_t* t) {
    if (!t || !*t) return;
    pthread_t* p = (pthread_t*) *t;
    pthread_join(*p, NULL);
    free(p);
    *t = NULL;
}

void bpvm_platform_thread_yield(void) {
#if defined(_POSIX_PRIORITY_SCHEDULING) || defined(__MINGW32__) || defined(__MINGW64__)
    sched_yield();
#else
    /* Fallback: micro-sleep para ceder. */
    struct timespec ts = {0, 100000L};   /* 100 µs */
    nanosleep(&ts, NULL);
#endif
}

void bpvm_platform_thread_sleep_ms(int ms) {
    if (ms <= 0) return;
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

/* ---- Tiempo ---- */

int64_t bpvm_platform_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void bpvm_platform_busy_wait_us(int us) {
    if (us <= 0) return;
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int64_t deadline_ns = (int64_t) start.tv_sec * 1000000000LL
                        + start.tv_nsec + (int64_t) us * 1000LL;
    do {
        clock_gettime(CLOCK_MONOTONIC, &now);
    } while ((int64_t) now.tv_sec * 1000000000LL + now.tv_nsec < deadline_ns);
}
