#!/usr/bin/env python3
# list_trunc_smoke.py — #425: EL LISTADO DECLARA LO QUE DEJA FUERA.
#
# El recorrido plano del device tiene topes (entradas por directorio,
# directorios a visitar y —en el STM32— ficheros del FS entero). Al pasarse
# recortaba EN SILENCIO: el firmware lo anotaba en su log, pero por el wire
# salia una lista corta y el arbol del IDE la pintaba como si fuera todo. Un
# listado corto que no se declara corto es una mentira, y de las que se creen.
#
# Esta prueba lo fuerza contra el MICRO SIMULADO (tools/bpvm_sim.c), que tiene
# el mismo agujero y los mismos topes en pequeño (64 dirs / 512 ficheros). No
# hace falta placa: el camino que recorre es el MISMO que el del IDE.
#
# Trae su CONTROL, que es lo que la hace valer: primero por DEBAJO del tope
# —donde omitted TIENE que ser 0— y luego por encima. Sin el control, un
# `omitted` que siempre dijera "faltan cosas" pasaria por bueno.
#
#   python tools/list_trunc_smoke.py
import json, socket, subprocess, sys, os, time, tempfile, shutil

try: sys.stdout.reconfigure(encoding="utf-8")   # consola Windows cp1252 -> UTF-8
except Exception: pass

PORT = 5111
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
SIM  = os.path.join(ROOT, "build", "bpvm-sim.exe")
if not os.path.exists(SIM):
    SIM = SIM[:-4]   # sin .exe (Linux/mac)

WALK_MAX_DIRS = 64   # tiene que coincidir con tools/bpvm_sim.c

fails = 0
def check(cond, msg):
    global fails
    print(("  ok  : " if cond else "  FAIL: ") + msg)
    if not cond: fails += 1

class Wire:
    def __init__(self, sock):
        self.s = sock; self.buf = b""; self.id = 0
    def _line(self):
        while b"\n" not in self.buf:
            chunk = self.s.recv(65536)
            if not chunk: raise EOFError("el sim cerro la conexion")
            self.buf += chunk
        line, self.buf = self.buf.split(b"\n", 1)
        return json.loads(line.decode())
    def call(self, typ, **fields):
        self.id += 1
        fields.update(type=typ, id=self.id)
        self.s.sendall((json.dumps(fields) + "\n").encode())
        return self._line()
    def put(self, path, data):
        self.id += 1
        f = {"type": "PUT", "id": self.id, "path": path, "bulk": len(data)}
        self.s.sendall((json.dumps(f) + "\n").encode() + data)
        return self._line()

def carpetas(w, desde, hasta):
    """Una carpeta por fichero: en el FS del device el '/' es namespace."""
    for i in range(desde, hasta):
        w.put("/d%03d/x.txt" % i, b"x")

def listado(w):
    r = w.call("LIST", path="")
    return len(r.get("entries") or []), r.get("omitted")

def main():
    tmp   = tempfile.mkdtemp(prefix="list_trunc_")
    flash = os.path.join(tmp, "sim.flash")
    proc = subprocess.Popen([SIM, "--port=%d" % PORT, "--flash-file=" + flash,
                             "--mem=1M", "--psram=8M", "--flash=4M"],
                            stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    try:
        sock = None
        for _ in range(50):
            try:
                sock = socket.create_connection(("127.0.0.1", PORT), timeout=1.0); break
            except OSError:
                time.sleep(0.1)
        if sock is None:
            print("FAIL: el sim no acepto conexion"); return 1
        sock.settimeout(60.0)
        w = Wire(sock)

        # --- CONTROL: por DEBAJO del tope, el listado esta entero ---
        n1 = WALK_MAX_DIRS // 4
        carpetas(w, 0, n1)
        ents, om = listado(w)
        check(om is not None,
              "el LIST_REPLY trae el campo 'omitted' (si falta, el listado puede mentir)")
        check(ents == n1, "control: %d carpetas -> %d entradas" % (n1, ents))
        check(om == 0, "control: nada fuera de los topes -> omitted=0 (dice %r)" % om)

        # --- EL CASO: por ENCIMA del tope, lo tiene que DECIR ---
        n2 = WALK_MAX_DIRS + 16
        carpetas(w, n1, n2)
        ents, om = listado(w)
        check(om and om > 0,
              "pasado el tope de %d dirs, lo DECLARA (omitted=%r)" % (WALK_MAX_DIRS, om))
        check(ents + (om or 0) == n2,
              "la cuenta CUADRA: %d listadas + %s omitidas = %d creadas"
              % (ents, om, n2))

        print("[status=%s]" % ("OK" if fails == 0 else "FAIL"))
        return 1 if fails else 0
    finally:
        proc.terminate()
        try: proc.wait(timeout=5)
        except Exception: proc.kill()
        shutil.rmtree(tmp, ignore_errors=True)

if __name__ == "__main__":
    sys.exit(main())
