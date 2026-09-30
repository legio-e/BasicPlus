"""medir_margen.py — el protocolo de medida del MARGEN DEL SISTEMA (V6/U6.4), para las
cinco familias.

QUÉ MIDE: lo que consume lo que NO es la VM —el cargador, las tareas, el FS, el wire—
mientras la placa trabaja. Cada familia lo dice en su línea `mem:`, que sale tras cada
RUN (antes de soltar la VM, que es el pico):
  · ESP32: la DRAM interna libre y su MÍNIMO HISTÓRICO (desde #336);
  · Pico y STM32 (V7/H7, 30-sep): lo MÁXIMO que ha llegado `malloc` sobre lo que tiene
    (en la Pico, los 64 KB de VM_SRAM_MALLOC_MARGIN; en la STM32, lo que deja el
    enlazador), el heap de FreeRTOS con su mínimo histórico y, en la STM32, la pila del
    MSP que usan las interrupciones.

POR QUÉ EXISTE. `medir_margen.ps1` (U6.4, 31-ago) hablaba sólo con las ESP32 —sus
interruptores eran los del C3 y el S3— y la Pico y la STM32 no tenían línea `mem:`: el
margen de esas dos era un número puesto a mano y nunca medido. Éste usa la conexión de
`wire_serie.py`, que ya sabe hablar con todas (DTR/RTS apagados; DTR encendido para el
CDC de la Pico).

EL PROTOCOLO, el mismo en todas las placas (dos medidas del mismo instrumento no se
pueden comparar si no se sabe qué se ejecutó):
  1. LOG_CLEAR y LUEGO RESET  (log sólo de esta pasada, CON las líneas del arranque)
  2. PUT de un fichero        (el búfer del wire; el .ps1 subía BenchMem.mod, que desde
                               el muro de la pila ya no compila —128 KB de locales—: ahora
                               bpstdlib/Time.mod, 6,5 KB, que sólo se guarda)
  3. RUN ×2                   (el MISMO programa en todas las placas: Bench, fib(28))
  4. LIST                     (recorre el FS entero)
  5. LOG_DUMP                 (las líneas `vm:` del arranque y `mem:` de cada RUN)

Uso (desde Git Bash, con MSYS_NO_PATHCONV=1 por las rutas de la placa; la carga, compilada
con el compilador de HEAD: `samples/benchmarks/Bench.bp`):
    python tools/medir_margen.py COM22 --carga <dir>/Bench.mod
Y con una carga de verdad (la tanda de V7): la línea `mem:` lleva el MÁXIMO desde el
arranque, así que la última de la tanda es el pico de toda ella:
    python tools/medir_margen.py COM22 --limpiar-log
    python tools/tanda.py --lista tools/tanda_v7.json --puertos COM22 --informe <md>
    python tools/medir_margen.py COM22 --solo-log
"""
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from wire_serie import Wire   # noqa: E402

RAIZ = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
FILTRO = re.compile(r"vm: |mem: |finished|fin de RUN|tabla de simbolos|RUN/v1|PANIC|Out of memory")


def conectar(puerto, segs=25):
    """Abre y espera al HELLO. Tras un RESET la Pico se re-enumera: el puerto tarda en volver."""
    t0 = time.time()
    while time.time() - t0 < segs:
        try:
            w = Wire(puerto)
        except Exception:
            time.sleep(1)
            continue
        if w.hola() is not None:
            return w
        w.close()
        time.sleep(1)
    sys.exit("%s: la placa no contesta al HELLO tras %d s" % (puerto, segs))


def put(w, local, remoto):
    data = open(local, "rb").read()
    w.id += 1
    cab = '{"type": "PUT", "id": %d, "path": "%s", "bulk": %d}\n' % (w.id, remoto, len(data))
    w.s.write(cab.encode() + data)
    r = w.esperar("PUT_REPLY", 20)
    print("2. PUT %s (%d B) -> %s" % (remoto, len(data),
          "ok" if r else "FALLO %s" % [m.get("message") for m in w.otros][:2]))
    return r is not None


def correr(w, remoto, segs):
    w.send("RUN", path=remoto)
    if w.esperar("RUN_REPLY", 10) is None:
        return "no arrancó: %s" % [(m.get("code"), m.get("message")) for m in w.otros][:2]
    t0 = time.time()
    while time.time() - t0 < segs:
        for m in w.lines(0.3):
            if m.get("type") == "EXITED":
                return "EXITED %s exit=%s %s ms %s" % (m.get("status"), m.get("exitCode"),
                                                       m.get("elapsedMs"), m.get("errorMessage") or "")
    return "sin EXITED tras %d s" % segs


def log_encendido(w):
    """#423: tras el arranque el log lo manda el ENV (`log=1`), apagado por defecto. La línea
    `mem:` es rastro de EJECUCIÓN: sin `log=1` la placa no la escribe y la medida sale MUDA
    (30-sep, la DK2: sólo las líneas del arranque, ni una del RUN)."""
    w.send("ENV_LS")
    r = w.esperar("ENV_LS_REPLY", 10) or {}
    for e in (r.get("entries") or r.get("vars") or []):
        if e.get("key") == "log":
            return str(e.get("value")).strip() in ("1", "true", "si", "on")
    return False


def volcar_log(w):
    w.send("LOG_DUMP")
    r = w.esperar("LOG_DUMP_REPLY", 15)
    if r is None:
        print("   (sin LOG_DUMP_REPLY)")
        return
    for ln in (r.get("text") or "").splitlines():
        if FILTRO.search(ln):
            print("   " + ln)


def main():
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    puerto = a[0]
    relleno = os.path.join(RAIZ, "bpstdlib", "Time.mod")
    carga = a[a.index("--carga") + 1] if "--carga" in a else None
    if "--limpiar-log" in a:      # antes de una tanda: que el log sea sólo de ella
        w = conectar(puerto)
        if not log_encendido(w):
            print("AVISO: el ENV no tiene log=1: la tanda no dejará líneas `mem:` (#423)")
        w.send("LOG_CLEAR")
        print("LOG_CLEAR -> %s" % ("ok" if w.esperar("LOG_CLEAR_REPLY", 5) else "sin respuesta"))
        w.close()
        return
    if "--solo-log" in a:
        w = conectar(puerto)
        print("LOG (lo que haya desde el último LOG_CLEAR):")
        volcar_log(w)
        w.close()
        return
    if carga is None:
        sys.exit("falta --carga <Bench.mod> (compilado con el compilador de HEAD)")

    w = conectar(puerto)
    if not log_encendido(w):
        w.close()
        sys.exit("%s: el ENV no tiene log=1 y la línea `mem:` no se escribiría (#423: el rastro de "
                 "ejecución está apagado por defecto). `wire_serie.py %s env log 1` y `reset`." % (puerto, puerto))
    w.send("LOG_CLEAR")
    print("1. LOG_CLEAR -> %s" % ("ok" if w.esperar("LOG_CLEAR_REPLY", 5) else "sin respuesta"))
    w.send("RESET")
    try:
        print("   RESET     -> %s" % ("ok" if w.esperar("RESET_REPLY", 5) else "sin RESET_REPLY"))
    except Exception as e:        # la Pico se re-enumera: el puerto muere a medio leer
        print("   RESET     -> el puerto se fue (%s): la placa reinicia" % type(e).__name__)
    w.close()
    time.sleep(6)                 # la P4 y la DK2 tardan más de 3 s en contestar tras un reset
    w = conectar(puerto)

    put(w, relleno, "/app/_margen.mod")
    remoto = "/app/" + os.path.basename(carga)
    put(w, carga, remoto)
    for n in (1, 2):
        print("3.%d RUN %s -> %s" % (n, remoto, correr(w, remoto, 180)))
    w.send("LIST")
    r = w.esperar("LIST_REPLY", 15)
    print("4. LIST -> %s" % ("%d entradas" % len(r.get("entries") or []) if r else "sin respuesta"))
    print("5. LOG (sólo esta pasada):")
    volcar_log(w)
    w.send("INFO")
    r = w.esperar("INFO_REPLY", 6) or {}
    extra = {k: r[k] for k in ("rtosHeapMinFreeBytes", "vmTaskStackFreeBytes", "vmHeapBytes") if k in r}
    if extra:
        print("6. INFO: %s" % extra)
    w.close()


if __name__ == "__main__":
    main()
