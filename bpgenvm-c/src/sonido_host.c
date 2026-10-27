/*
 * sonido_host.c — V8/H4 (27-oct, #708): la cintura del sonido EN EL PC (Eduardo: «Que en el PC suene, si»). Lo común,
 * en bpvm_sonido.c; esto sólo abre el audio del sistema, le da PCM de 16 bits y lo cierra.
 *
 *   - Windows: winmm (waveOut), que viene con el sistema. Cuatro búferes en cola: la tarea del sonido espera a que
 *     quede uno libre, y eso la lleva al ritmo del sonido. Al parar, waveOutReset corta en seco; al acabar el fichero,
 *     se espera a que suene lo encolado.
 *   - Con BPVM_SONIDO=mudo en el entorno, EN SILENCIO: la misma tarea y al mismo ritmo (contra el reloj, no a base de
 *     dormir trozos: un sleep de Windows dura lo que quiere), sin tocar el audio. Lo usan las pruebas de la red, para
 *     que no suene nada mientras corre.
 *   - Fuera de Windows, sin sonido (lo dice la HAL BP), salvo en silencio.
 *
 * El volumen, como en la P4 (esp_codec_dev): de 0 a 100 son de -50 a 0 dB, medio dB por paso; 0, nada. Aquí se aplica
 * a las muestras. En una placa, este fichero no hace nada (el STM32 compila todo `src/`).
 */
#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)

#include "bpvm_sonido.h"
#include "bpvm_platform.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── En silencio ──────────────────────────────────────────────────────────────────────────────────────────────────── */

static uint32_t m_hz;
static int      m_canales;
static int64_t  m_t0;
static uint64_t m_marcos;

static int mudo_abrir(uint32_t hz, int canales, char* err, size_t n) {
    (void) err; (void) n;
    m_hz = hz;
    m_canales = canales;
    m_t0 = bpvm_platform_now_ms();
    m_marcos = 0;
    return 0;
}
static int mudo_escribir(const int16_t* pcm, uint32_t muestras) {
    int64_t objetivo, ahora;
    (void) pcm;
    m_marcos += muestras / (uint32_t) m_canales;
    objetivo = m_t0 + (int64_t) ((m_marcos * 1000u) / m_hz);   /* cuándo acaba de sonar lo escrito */
    ahora = bpvm_platform_now_ms();
    if (objetivo > ahora) bpvm_platform_thread_sleep_ms((int) (objetivo - ahora));
    return 0;
}
static void mudo_volumen(int v) { (void) v; }
static void mudo_cerrar(int drenar) { (void) drenar; }

static const bpvm_sonido_cintura_t CINTURA_MUDO = { "mudo", mudo_abrir, mudo_escribir, mudo_volumen, mudo_cerrar };

/* ── Windows: winmm ───────────────────────────────────────────────────────────────────────────────────────────────── */

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>

#define W_NBUF   4
#define W_BUFSZ  8192    /* bytes por búfer, como mucho: la tarea da trozos de unos 50 ms */
#define W_ESPERA 2000    /* ms que se espera, como mucho, a que suene lo encolado al cerrar */

static HWAVEOUT w_h;
static HANDLE   w_ev;
static WAVEHDR  w_hdr[W_NBUF];
static int16_t* w_mem[W_NBUF];
static int      w_sig;
static int32_t  w_ganancia = 32768;   /* Q15 */

static int win_libre(const WAVEHDR* h) { return !(h->dwFlags & WHDR_PREPARED) || (h->dwFlags & WHDR_DONE); }

static int win_abrir(uint32_t hz, int canales, char* err, size_t n) {
    WAVEFORMATEX f;
    MMRESULT r;
    int i;
    memset(&f, 0, sizeof f);
    f.wFormatTag      = WAVE_FORMAT_PCM;
    f.nChannels       = (WORD) canales;
    f.nSamplesPerSec  = hz;
    f.wBitsPerSample  = 16;
    f.nBlockAlign     = (WORD) (canales * 2);
    f.nAvgBytesPerSec = hz * f.nBlockAlign;
    if (!w_ev && !(w_ev = CreateEvent(NULL, FALSE, FALSE, NULL))) { snprintf(err, n, "CreateEvent"); return -1; }
    for (i = 0; i < W_NBUF; i++) {
        if (!w_mem[i] && !(w_mem[i] = (int16_t*) malloc(W_BUFSZ))) { snprintf(err, n, "sin memoria"); return -1; }
        memset(&w_hdr[i], 0, sizeof w_hdr[i]);
        w_hdr[i].lpData = (LPSTR) w_mem[i];
    }
    r = waveOutOpen(&w_h, WAVE_MAPPER, &f, (DWORD_PTR) w_ev, 0, CALLBACK_EVENT);
    if (r != MMSYSERR_NOERROR) { snprintf(err, n, "waveOutOpen -> %u", (unsigned) r); return -1; }
    w_sig = 0;
    return 0;
}

static int win_escribir(const int16_t* pcm, uint32_t muestras) {
    while (muestras > 0) {
        WAVEHDR* h = &w_hdr[w_sig];
        int16_t* d = (int16_t*) h->lpData;
        uint32_t k = muestras, i;
        while (!win_libre(h)) WaitForSingleObject(w_ev, 100);
        if (h->dwFlags & WHDR_PREPARED) waveOutUnprepareHeader(w_h, h, sizeof *h);
        if (k > W_BUFSZ / 2u) k = W_BUFSZ / 2u;
        for (i = 0; i < k; i++) d[i] = (int16_t) (((int32_t) pcm[i] * w_ganancia) >> 15);
        h->dwBufferLength = k * 2u;
        h->dwFlags = 0;
        if (waveOutPrepareHeader(w_h, h, sizeof *h) != MMSYSERR_NOERROR) return -1;
        if (waveOutWrite(w_h, h, sizeof *h) != MMSYSERR_NOERROR) { waveOutUnprepareHeader(w_h, h, sizeof *h); return -1; }
        pcm += k;
        muestras -= k;
        w_sig = (w_sig + 1) % W_NBUF;
    }
    return 0;
}

/* El PC, MÁS BAJO que la P4 (Eduardo: «el volumen en el PC y en la P4 no se parecen. Si reproduces un sonido en el PC
 * al 85% vas a dejar sordo a más de uno»): en la P4 el 85 pasa por el códec y un altavoz pequeño; aquí iría al audio del
 * sistema casi a escala completa. La misma escala, desplazada hacia abajo: 20 dB, ELEGIDOS DE OÍDO por Eduardo el 27-oct,
 * como el 85 de la P4 (el canon a 85 con 40, 30 y 20 dB menos, dos vueltas: «la 3»). El espejo, Sonido.java
 * (ATENUACION_PC_DB). */
#define W_ATENUACION_DB 20.0

static void win_volumen(int v) {
    w_ganancia = (v <= 0) ? 0
               : (int32_t) (32768.0 * pow(10.0, ((double) (v - 100) / 2.0 - W_ATENUACION_DB) / 20.0));   /* -50..0 dB, y lo del PC */
}

static void win_cerrar(int drenar) {
    int i;
    if (drenar) {
        DWORD t0 = GetTickCount();
        for (;;) {
            int todos = 1;
            for (i = 0; i < W_NBUF; i++) if (!win_libre(&w_hdr[i])) todos = 0;
            if (todos || GetTickCount() - t0 > W_ESPERA) break;
            WaitForSingleObject(w_ev, 100);
        }
    }
    waveOutReset(w_h);   /* lo que quede, fuera (al drenar ya no queda nada) */
    for (i = 0; i < W_NBUF; i++)
        if (w_hdr[i].dwFlags & WHDR_PREPARED) waveOutUnprepareHeader(w_h, &w_hdr[i], sizeof w_hdr[i]);
    waveOutClose(w_h);
}

static const bpvm_sonido_cintura_t CINTURA_WINMM = { "winmm", win_abrir, win_escribir, win_volumen, win_cerrar };
#endif /* _WIN32 */

void bpvm_sonido_register_host(void) {
    const char* modo = getenv("BPVM_SONIDO");
    if (modo && strcmp(modo, "mudo") == 0) { bpvm_sonido_registrar(&CINTURA_MUDO); return; }
#ifdef _WIN32
    bpvm_sonido_registrar(&CINTURA_WINMM);
#else
    bpvm_sonido_sin_cintura("este PC no tiene sonido: la VM-C solo lo toca en Windows (o en silencio, con BPVM_SONIDO=mudo)");
#endif
}

#else  /* una placa: el sonido lo registra su familia, si lo tiene */

#include "bpvm_sonido.h"
void bpvm_sonido_register_host(void) { }

#endif
