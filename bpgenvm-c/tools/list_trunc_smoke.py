#!/usr/bin/env python3
# list_trunc_smoke.py — #425: EL LISTADO DECLARA LO QUE DEJA FUERA.
#
# El recorrido plano del device tiene topes (entradas por directorio,
# directorios a visitar y —en el STM32— ficheros del FS entero). Al pasarse
# recortaba EN SILENCIO: el firmware lo anotaba en su log, pero por el wire
# salia una lista corta y el arbol del IDE la pintaba como si fuera todo. Un
# listado corto que no se declara corto es una mentira, y de las que se creen.
#
# Esta prueba lo fuerza contra el MICRO SIMULADO (tools/bpvm_sim.c). No hace
# falta placa: desde U3.24 (31-ago) el LIST del sim lo sirve el REPL COMUN
# (src/bpvm_repl.c), el MISMO codigo que el de las placas, con su tope de
# directorios pendientes (REPL_LIST_MAX_DIRS, contando la raiz).
#
# Trae su CONTROL, que es lo que la hace valer: primero JUSTO en el tope
# —donde omitted TIENE que ser 0—, luego una carpeta mas —la primera que no
# cabe, y se DECLARA— y luego muy por encima. Sin el control, un `omitted` que
# siempre dijera "faltan cosas" pasaria por bueno.
#
# El tope se LEE del fuente, no se copia: hasta el 23-sep aqui ponia 64 (el del
# recorredor PROPIO que el sim tenia antes de U3.24) y el control estuvo en rojo
# tres semanas midiendo contra un tope que ya no era el del listado.
#
#   python tools/list_trunc_smoke.py
import json, re, socket, subprocess, sys, os, time, tempfile, shutil

try: sys.stdout.reconfigure(encoding="utf-8")   # consola Windows cp1252 -> UTF-8
except Exception: pass

PORT = 5111
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
SIM  = os.path.join(ROOT, "build", "bpvm-sim.exe")
if not os.path.exists(SIM):
    SIM = SIM[:-4]   # sin .exe (Linux/mac)
SIM  = os.environ.get("BPVM_SIM") or SIM   # compat.sh check-sim: el sim que construye la red

def tope_del_listado():
    """REPL_LIST_MAX_DIRS de src/bpvm_repl.c: directorios en la cola del LIST,
    CONTANDO la raiz "/" (que ocupa el primer hueco). None si no se encuentra."""
    try:
        src = open(os.path.join(ROOT, "src", "bpvm_repl.c"), encoding="utf-8").read()
    except OSError:
        return None
    m = re.search(r"#define\s+REPL_LIST_MAX_DIRS\s+(\d+)", src)
    return int(m.group(1)) if m else None

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

# V8 #816 (13-nov) — la flash del sim, como la de una placa, trae sus carpetas al montar: /sys, /lib, /app y /data.
# Ocupan su hueco en la cola del LIST, y vacías no salen como entradas (el listado es de ficheros). Las de la prueba se
# llaman zNNN para ir DETRÁS en el recorrido (va por orden alfabético): las de la flash entran siempre, lo que se queda
# fuera es de la prueba, y la cuenta cuadra igual que con la raíz vacía de antes.
CARPETAS_DE_LA_FLASH = 4

def carpetas(w, desde, hasta):
    """Una carpeta por fichero: en el FS del device el '/' es namespace."""
    for i in range(desde, hasta):
        w.put("/z%03d/x.txt" % i, b"x")

def listado(w):
    r = w.call("LIST", path="")
    return len(r.get("entries") or []), r.get("omitted")

def main():
    tope = tope_del_listado()
    if tope is None or tope < 3:
        print("FAIL: no encuentro REPL_LIST_MAX_DIRS en src/bpvm_repl.c (el instrumento no sabe contra que medir)")
        return 1
    caben = tope - 1 - CARPETAS_DE_LA_FLASH   # la raiz ocupa un hueco de la cola, y las de la flash, los suyos (#816)
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

        print("=== #425 el listado declara lo omitido (tope del LIST comun: %d dirs con la raiz) ===" % tope)
        # --- CONTROL: JUSTO en el tope, el listado esta entero ---
        n1 = caben
        carpetas(w, 0, n1)
        ents, om = listado(w)
        check(om is not None,
              "el LIST_REPLY trae el campo 'omitted' (si falta, el listado puede mentir)")
        check(ents == n1, "control: %d carpetas (justo el tope) -> %d entradas" % (n1, ents))
        check(om == 0, "control: nada fuera de los topes -> omitted=0 (dice %r)" % om)

        # --- LA FRONTERA: una mas, y la primera que no cabe se DECLARA ---
        carpetas(w, n1, n1 + 1)
        ents, om = listado(w)
        check(ents == n1 and om == 1,
              "una carpeta mas -> %d listadas y omitted=1 (dice %d y %r)" % (n1, ents, om))

        # --- EL CASO: muy por ENCIMA del tope, lo tiene que DECIR ---
        n2 = tope + 16
        carpetas(w, n1 + 1, n2)
        ents, om = listado(w)
        check(om and om > 0,
              "pasado el tope de %d dirs, lo DECLARA (omitted=%r)" % (tope, om))
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
