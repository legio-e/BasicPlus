"""#473/R16 - el hilo que se borra a si mismo, ¿filtra?

El comun cambio a vTaskSuspend(NULL)+borrado desde join porque en la Nucleo cada
RUN se quedaba con la pila del hilo `io` y al quinto no habia heap. La Pico y el
ESP32 siguen con vTaskDelete(NULL) DENTRO de la tarea. Esto lo MIDE.

CONTROL primero: N INFO seguidos SIN ejecutar nada. Si la marca de agua ya se
mueve sin RUNs, la medida no vale para nada.
"""
import sys, time
sys.path.insert(0, "C:/lenguajes/pm/bpgenvm-c/tools")
from wire_serie import Wire

port, mod, n = sys.argv[1], sys.argv[2], int(sys.argv[3])
w = Wire(port)
if w.hola() is None: sys.exit("sin HELLO_REPLY")

def info():
    w.send("INFO")
    r = w.esperar("INFO_REPLY", 6)
    if r is None: return None
    return (r.get("uptimeMs"), r.get("rtosHeapMinFreeBytes"), r.get("vmTaskStackFreeBytes"))

print("--- CONTROL: 5 INFO sin ejecutar nada ---")
for i in range(5):
    v = info(); print("  info %d: uptime=%s  rtosHeapMinFree=%s  vmStackFree=%s" % ((i+1,)+v))
    time.sleep(0.3)

print("--- %d x RUN %s ---" % (n, mod))
base = None
for i in range(1, n + 1):
    w.send("RUN", path=mod)
    if w.esperar("RUN_REPLY", 8) is None:
        print("  run %d: NO ARRANCO. otros=%s" % (i, [(m.get("type"), m.get("code"), m.get("message")) for m in w.otros][:2]))
        break
    ex = w.esperar("EXITED", 20)
    v = info()
    if v is None:
        print("  run %d: EXITED=%s pero el INFO NO CONTESTA (¿sin heap?)" % (i, ex.get("status") if ex else "NO"))
        break
    if base is None: base = v[1]
    print("  run %2d: EXITED %-14s  rtosHeapMinFree=%6s (delta %+d)  vmStackFree=%s" % (
        i, (ex.get("status") if ex else "SIN EXITED"), v[1], (v[1] - base) if v[1] is not None else 0, v[2]))
w.close()
