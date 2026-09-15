#!/usr/bin/env python3
# sim_sombra.py -- V7/H2 (#493): LA ZONA DE PACKS POR EL WIRE, contra el micro simulado
# (bpvm-sim: la VM-C + littlefs + wire v1, la misma cintura que las placas).
#
# Lo que el CLI del host no ve: los cuatro REPL (sim, Pico, ESP32, STM32) SONDEAN con
# bpvm_entry_resolve antes de cargar, y la revision adversarial del 15-sep cazo que
# un principal que solo vive en la zona contestaba NOT_FOUND al RUN (por el CLI iba).
#
#   python sim_sombra.py <bpvm-sim.exe> <X.pack> <Core.mod> <NombrePack> <ModPrincipal> <textoEsperado> [--port=N]
#
# Casos (cada uno imprime "  ok  " o "  FAIL"; sale 1 si alguno falla):
#   1. STAT name=<Mod>.mod -> path pack:<Pack>/<Mod>.mod con magic y crc
#   2. RUN <Mod>.mod  -> RUN_REPLY, OUTPUT con la Str del pack, y el diag dice XIP
#   3. RUN pack:<Pack>/<Mod>.mod -> el MISMO OUTPUT que 2 (y sin aviso falso)
#   4. Carga FALLIDA (sin /lib/Core.mod) y despues STAT path=/app/Str.mod -> NOT_FOUND
#      (el overlay de un pack muerto no contesta: antes era un use-after-free)
import json, socket, subprocess, sys, os, time, tempfile

class Wire:
    def __init__(self, s): self.s = s; self.buf = b""; self.id = 0
    def _line(self):
        while b"\n" not in self.buf:
            c = self.s.recv(65536)
            if not c: raise EOFError("sim cerro")
            self.buf += c
        line, self.buf = self.buf.split(b"\n", 1)
        return json.loads(line.decode("utf-8", "replace"))
    def call(self, typ, **f):
        self.id += 1; f.update(type=typ, id=self.id)
        self.s.sendall((json.dumps(f) + "\n").encode()); return self._line()
    def call_bulk(self, typ, data, **f):
        self.id += 1; f.update(type=typ, id=self.id, bulk=len(data))
        self.s.sendall((json.dumps(f) + "\n").encode() + data); return self._line()
    def run(self, path, timeout=30.0):
        r = self.call("RUN", path=path)
        if r.get("type") != "RUN_REPLY": return r, "", None
        out, t0 = [], time.time()
        while time.time() - t0 < timeout:
            m = self._line()
            if m.get("type") == "OUTPUT": out.append(m.get("data", ""))
            elif m.get("type") == "EXITED": return r, "".join(out), m
        return r, "".join(out), None

fallos = 0
def ok(c, m):
    global fallos
    print(("  ok  : " if c else "  FAIL: ") + m)
    if not c: fallos += 1

def arrancar(sim, packs, port, tmp):
    flash = os.path.join(tmp, "sim.flash")
    errf = open(os.path.join(tmp, "sim.stderr.txt"), "w+b")
    proc = subprocess.Popen([sim, "--port=%d" % port, "--flash-file=" + flash,
                             "--mem=1M", "--flash=4M", "--no-screen"] + ["--pack=" + p for p in packs],
                            stdout=subprocess.DEVNULL, stderr=errf)
    sock = None
    for _ in range(50):
        try: sock = socket.create_connection(("127.0.0.1", port), timeout=1.0); break
        except OSError: time.sleep(0.1)
    if sock is None:
        proc.kill(); raise SystemExit("FAIL: el sim no conecta")
    sock.settimeout(60.0)
    w = Wire(sock)
    w.call("HELLO", protoVersion=1, clientName="sombra")
    w.call("FORMAT", confirm="YES")
    return proc, sock, errf, w

def stderr_de(errf):
    errf.flush(); errf.seek(0)
    return errf.read().decode("utf-8", "replace")

def main():
    sim, pack, core = sys.argv[1], sys.argv[2], sys.argv[3]
    pk, main, token = sys.argv[4], sys.argv[5], sys.argv[6]
    port = 5416
    for a in sys.argv[7:]:
        if a.startswith("--port="): port = int(a[7:])
    ruta = "pack:%s/%s.mod" % (pk, main)
    tmp = tempfile.mkdtemp(prefix="sombra_")
    core_bytes = open(core, "rb").read()

    # --- sesion 1: con /lib/Core.mod ---
    proc, sock, errf, w = arrancar(sim, [pack], port, tmp)
    try:
        w.call_bulk("PUT", core_bytes, path="/lib/Core.mod")
        st = w.call("STAT", name=main + ".mod", crc=True)
        ok(st.get("type") == "STAT_REPLY" and st.get("path") == ruta
           and st.get("magic", "").startswith("MOD") and isinstance(st.get("crc"), int) and st.get("crc") >= 0,
           "STAT name= del principal que solo vive en la zona -> %s con magic y crc (%s)" % (ruta, json.dumps(st)[:120]))
        r1, out1, ex1 = w.run(main + ".mod")
        ok(r1.get("type") == "RUN_REPLY" and ex1 is not None and ex1.get("status") == "OK" and token in out1,
           "RUN %s.mod por el wire: corre y usa la Str de su pack (%s)" % (main, out1.strip().replace("\n", "|")[:80]))
        r2, out2, ex2 = w.run(ruta)
        ok(r2.get("type") == "RUN_REPLY" and ex2 is not None and out2 == out1,
           "RUN %s: el MISMO OUTPUT que por nombre" % ruta)
        ok("aviso:" not in out1 and "aviso:" not in out2, "sin aviso falso de la sombra")
    finally:
        sock.close(); proc.kill(); proc.wait()
    err = stderr_de(errf); errf.close()
    ok(err.count("cargado XIP desde el pack '%s' de la zona" % pk) >= 2 and "dep 'Str' -> pack EN EJECUCION" in err,
       "el diag dice XIP desde el pack y su Str primero (las dos veces)")

    # --- sesion 2: SIN /lib/Core.mod: la carga falla; despues el FS dice la verdad ---
    proc, sock, errf, w = arrancar(sim, [pack], port + 1, tmp)
    try:
        r3, out3, ex3 = w.run(main + ".mod")
        ok(ex3 is not None and ex3.get("status") != "OK" and "falta el modulo 'Core'" in json.dumps(ex3),
           "sin /lib/Core.mod la carga falla nombrando a Core (%s)" % json.dumps(ex3)[:100])
        s2 = w.call("STAT", path="/app/Str.mod")
        ok(s2.get("type") == "ERROR" and s2.get("code") == "NOT_FOUND",
           "tras la carga fallida, STAT path=/app/Str.mod -> NOT_FOUND (el overlay del pack muerto no contesta)")
        s3 = w.call("STAT", name="Str.mod", crc=True)
        ok(s3.get("type") == "STAT_REPLY" and s3.get("path") == "pack:%s/Str.mod" % pk,
           "y STAT name=Str.mod sigue diciendo la verdad: pack:%s/Str.mod" % pk)
    finally:
        sock.close(); proc.kill(); proc.wait()
    errf.close()
    print("[status=%s]" % ("OK" if fallos == 0 else "FAIL"))
    return 1 if fallos else 0

if __name__ == "__main__":
    sys.exit(main())
