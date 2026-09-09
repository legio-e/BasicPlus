"""wire_serie.py — cliente del wire v1 POR SERIE, y que NO es el IDE.

POR QUE EXISTE (#379). La ficha decia que el wire «se desincroniza tras el Stop»
y su paso 1 era: *¿esta colgado el device o el IDE?* — y nunca se hizo, porque no
habia forma de preguntarle a la placa sin el IDE por medio. Con esto, un cuelgue
se parte en dos en diez segundos: si la placa contesta, lo colgado es el IDE.

No pretende ser un cliente completo: son los verbos que hacen falta para
diagnosticar (HELLO, PING, LIST, INFO, RUN, KILL, LOG_DUMP).

    python tools/wire_serie.py COM3 info
    python tools/wire_serie.py COM3 log
    python tools/wire_serie.py COM3 ciclo /app/Bench.mod 8

⚠️ Desde Git Bash, un argumento que empieza por "/" lo convierte MSYS en una ruta
de Windows y la placa contesta NOT_FOUND. Usa `MSYS_NO_PATHCONV=1` o cmd.
"""
import json, sys, time

try:
    import serial
except ImportError:
    sys.exit("falta pyserial — usa el python del ESP-IDF, que ya lo trae")


class Wire:
    def __init__(self, port, baud=115200):
        self.s = serial.Serial(port, baud, timeout=0.2)
        self.buf = b""
        self.id = 0
        self.otros = []
        time.sleep(0.3)
        self.s.reset_input_buffer()

    def send(self, typ, **f):
        self.id += 1
        f.update(type=typ, id=self.id)
        self.s.write((json.dumps(f) + "\n").encode())
        return self.id

    def lines(self, secs):
        out, t0 = [], time.time()
        while time.time() - t0 < secs:
            c = self.s.read(4096)
            if c:
                self.buf += c
            while b"\n" in self.buf:
                ln, self.buf = self.buf.split(b"\n", 1)
                ln = ln.strip()
                if ln.startswith(b"{"):
                    try:
                        out.append(json.loads(ln.decode("utf-8", "replace")))
                    except Exception:
                        pass
        return out

    def esperar(self, typ, secs):
        """Espera un tipo. GUARDA lo demas en self.otros: tirar la respuesta que
        no encaja es como se pierden los diagnosticos (me paso escribiendo esto:
        la placa contestaba ERROR/NOT_FOUND y yo veia 'sin respuesta')."""
        self.otros = []
        t0 = time.time()
        while time.time() - t0 < secs:
            for m in self.lines(0.2):
                if m.get("type") == typ:
                    return m
                self.otros.append(m)
        return None

    def hola(self):
        self.send("HELLO", client="wire_serie", version=1)
        return self.esperar("HELLO_REPLY", 3)

    def close(self):
        self.s.close()


def cmd_info(w, _):
    print(json.dumps(w.esperar("INFO_REPLY", 5) if w.send("INFO") else None, indent=2,
                     ensure_ascii=False))


def cmd_log(w, _):
    w.send("LOG_DUMP")
    r = w.esperar("LOG_DUMP_REPLY", 10)
    print(r.get("text", "") if r else "sin respuesta")


def cmd_list(w, _):
    w.send("LIST")
    r = w.esperar("LIST_REPLY", 10)
    for e in (r.get("entries") or []) if r else []:
        print("  %8s  %s" % (e.get("size"), e.get("name")))


def cmd_ciclo(w, args):
    """#379 — run -> stop -> INFO, N veces. Si el wire se desincronizara tras el
    Stop, el INFO de despues no llegaria."""
    mod = args[0] if args else "/app/Bench.mod"
    n = int(args[1]) if len(args) > 1 else 5
    w.send("KILL"); w.lines(1.5)          # por si quedo algo de una prueba anterior
    malos = 0
    for i in range(1, n + 1):
        w.send("RUN", path=mod)
        if w.esperar("RUN_REPLY", 6) is None:
            print("ciclo %d: el RUN no arranco. Llego: %s" % (
                i, [(m.get("type"), m.get("code"), m.get("message")) for m in w.otros][:3]))
            malos += 1
            continue
        time.sleep(0.6)                    # que este calculando de verdad
        t0 = time.time(); w.send("KILL"); ex = w.esperar("EXITED", 12)
        t_kill = (time.time() - t0) * 1000
        t1 = time.time(); w.send("INFO"); inf = w.esperar("INFO_REPLY", 6)
        t_info = (time.time() - t1) * 1000
        if ex is None or inf is None:
            malos += 1
        print("ciclo %d: EXITED %-8s (%4.0f ms)  INFO %-3s (%4.0f ms)  uptime=%s" % (
            i, (ex.get("status") if ex else "SIN RESPUESTA"), t_kill,
            ("ok" if inf else "NO"), t_info, (inf.get("uptimeMs") if inf else "-")))
    print("--- %d ciclos, %d con fallo ---" % (n, malos))


COMANDOS = {"info": cmd_info, "log": cmd_log, "list": cmd_list, "ciclo": cmd_ciclo}

if __name__ == "__main__":
    if len(sys.argv) < 3 or sys.argv[2] not in COMANDOS:
        sys.exit(__doc__ + "\ncomandos: " + " ".join(COMANDOS))
    w = Wire(sys.argv[1])
    if w.hola() is None:
        print("(sin HELLO_REPLY — la placa no contesta o no es el puerto del wire)")
    COMANDOS[sys.argv[2]](w, sys.argv[3:])
    w.close()
