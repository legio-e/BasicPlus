#!/usr/bin/env python3
# sim_smoke.py — H10: smoke end-to-end del MICRO SIMULADO (tools/bpvm_sim.c).
# Hermano de boardsim_smoke.py: aquél cubre la GESTIÓN DE PLACA, éste el resto del
# wire v1 — FILES (PUT/LIST/STAT/GET/DF/DEL/MKDIR/FORMAT + streaming) y TERMINAL
# (RUN → OUTPUT → EXITED, y KILL). Es el camino EXACTO que recorre el IDE al hacer
# "Run on Device" contra el sim, así que si esto pasa, el IDE funciona sin placa.
# No necesita hardware ni el IDE. Sale 0 si todo pasa.
#
#   python tools/sim_smoke.py
import json, socket, subprocess, sys, os, time, tempfile, shutil

try: sys.stdout.reconfigure(encoding="utf-8")   # consola Windows cp1252 → UTF-8
except Exception: pass

PORT = 5108
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
SIM  = os.path.join(ROOT, "build", "bpvm-sim.exe")
if not os.path.exists(SIM):
    SIM = SIM[:-4]  # sin .exe (Linux/mac)
JAR  = os.path.join(ROOT, "..", "lexer-java", "target", "basicplus-frontend.jar")

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
            if not chunk: raise EOFError("el sim cerró la conexión")
            self.buf += chunk
        line, self.buf = self.buf.split(b"\n", 1)
        return json.loads(line.decode())
    def _bulk(self, n):
        while len(self.buf) < n:
            chunk = self.s.recv(65536)
            if not chunk: raise EOFError("el sim cerró la conexión durante el bulk")
            self.buf += chunk
        data, self.buf = self.buf[:n], self.buf[n:]
        return data
    def call(self, typ, **fields):
        self.id += 1
        fields.update(type=typ, id=self.id)
        self.s.sendall((json.dumps(fields) + "\n").encode())
        return self._line()
    def call_bulk(self, typ, data, **fields):
        """Línea JSON con "bulk":N + N bytes crudos detrás (framing del PUT)."""
        self.id += 1
        fields.update(type=typ, id=self.id, bulk=len(data))
        self.s.sendall((json.dumps(fields) + "\n").encode() + data)
        return self._line()
    def get(self, path):
        """GET = reply con "bulk":N y los bytes pegados detrás."""
        r = self.call("GET", path=path)
        if r.get("type") != "GET_REPLY": return r, b""
        return r, self._bulk(r["bulk"])
    def drain_run(self, timeout=30.0):
        """Consume los eventos OUTPUT hasta el EXITED. Devuelve (texto, exited)."""
        out, t0 = [], time.time()
        while time.time() - t0 < timeout:
            m = self._line()
            if m.get("type") == "OUTPUT": out.append(m.get("data", ""))
            elif m.get("type") == "EXITED": return "".join(out), m
        return "".join(out), None

def build_hello(dest):
    """Compila samples/Hello.bp con el frontend. None si no se puede (sin java/jar)."""
    src = os.path.join(ROOT, "samples", "Hello.bp")
    if not os.path.exists(JAR) or not os.path.exists(src): return None
    if shutil.which("java") is None: return None
    rc = subprocess.run(["java", "-jar", JAR, src, "--compile", dest, "--backend=mivm"],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    mod = os.path.join(dest, "Hello.mod")
    return mod if rc.returncode == 0 and os.path.exists(mod) else None

def main():
    tmp   = tempfile.mkdtemp(prefix="sim_smoke_")
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
            print("FAIL: el sim no aceptó conexión"); return 1
        sock.settimeout(60.0)
        w = Wire(sock)

        print("=== H10 sim smoke (FILES + TERMINAL) ===")

        # 1. handshake: el sim se anuncia como device completo
        r = w.call("HELLO", protoVersion=1, clientName="smoke")
        caps = r.get("capabilities", [])
        check(r.get("type") == "HELLO_REPLY" and "FILES" in caps and "TERMINAL" in caps,
              "HELLO → capabilities con FILES + TERMINAL")

        # 2. INFO refleja el silicio que le pedimos por línea de comandos (H10)
        r = w.call("INFO")
        check(r.get("type") == "INFO_REPLY"
              and r.get("sramBytes") == 1024 * 1024
              and r.get("psramBytes") == 8 * 1024 * 1024
              and r.get("flashBytes") == 4 * 1024 * 1024,
              "INFO → RAM 1M / PSRAM 8M / flash 4M (los del --mem/--psram/--flash)")

        # 3. FS virgen
        r = w.call("FORMAT", confirm="YES")
        check(r.get("type") == "FORMAT_REPLY", "FORMAT confirm=YES → OK")
        r = w.call("FORMAT")
        check(r.get("code") == "MISSING_CONFIRM", "FORMAT sin confirm → MISSING_CONFIRM")
        r = w.call("LIST")
        check(r.get("type") == "LIST_REPLY" and r.get("entries") == [],
              "LIST tras formatear → vacío")

        # 4. PUT + LIST + STAT + GET (round-trip de bytes)
        blob = bytes(range(256)) * 5          # 1280 B con todos los valores
        r = w.call_bulk("PUT", blob, path="/app/demo/data.bin")
        check(r.get("type") == "PUT_REPLY", "PUT en ruta anidada → crea los directorios")
        r = w.call("LIST")
        ent = {e["name"]: e for e in r.get("entries", [])}
        check("/app/demo/data.bin" in ent and ent["/app/demo/data.bin"]["size"] == len(blob),
              "LIST → ruta COMPLETA y tamaño correcto")
        check(ent.get("/app/demo/data.bin", {}).get("crc", 0) != 0,
              "LIST → trae crc (el IDE lo usa para saltarse PUTs)")
        r = w.call("STAT", path="/app/demo/data.bin")
        check(r.get("size") == len(blob), "STAT → tamaño")
        r, got = w.get("/app/demo/data.bin")
        check(got == blob, "GET → los MISMOS bytes que subimos")
        r = w.call("DF")
        check(r.get("type") == "DF_REPLY" and r.get("fileCount") == 1
              and r.get("usedBytes") == len(blob),
              "DF → 1 fichero, bytes usados")

        # 5. streaming PUT (#294): BEGIN + N×DATA + END
        big = bytes((i * 7) & 0xFF for i in range(40000))
        r = w.call("PUT_BEGIN", path="/app/big.bin", size=len(big))
        check(r.get("type") == "PUT_BEGIN_REPLY", "PUT_BEGIN → sesión abierta")
        step, sent = 8192, 0
        okd = True
        while sent < len(big):
            chunk = big[sent:sent + step]
            r = w.call_bulk("PUT_DATA", chunk)
            sent += len(chunk)
            if r.get("received") != sent: okd = False
        check(okd, "PUT_DATA → 'received' acumulado correcto en cada trozo")
        r = w.call("PUT_END", size=len(big))
        check(r.get("type") == "PUT_END_REPLY" and r.get("size") == len(big),
              "PUT_END → tamaño verificado")
        r, got = w.get("/app/big.bin")
        check(got == big, "GET del fichero subido por streaming → bytes idénticos")

        # 6. DEL
        r = w.call("DEL", path="/app/big.bin")
        check(r.get("type") == "DEL_REPLY", "DEL → OK")
        r = w.call("STAT", path="/app/big.bin")
        check(r.get("code") == "NOT_FOUND", "STAT del borrado → NOT_FOUND")

        # 7. RUN: la VM de verdad ejecutando desde el FS del sim
        mod = build_hello(tmp)
        if mod is None:
            print("  (skip RUN: no encuentro java o basicplus-frontend.jar)")
        else:
            with open(mod, "rb") as f: data = f.read()
            r = w.call_bulk("PUT", data, path="/app/Hello.mod")
            check(r.get("type") == "PUT_REPLY", "PUT del .mod")
            r = w.call("RUN", path="/app/Hello.mod")
            check(r.get("type") == "RUN_REPLY" and r.get("session", 0) > 0,
                  "RUN → RUN_REPLY con sesión")
            text, exited = w.drain_run()
            check(exited is not None and exited.get("status") == "OK",
                  "EXITED → status OK")
            nums = [l for l in text.splitlines() if l.strip()]
            check(nums[:3] == ["0", "1", "1"] and nums[-1] == "999",
                  "OUTPUT → la serie de Fibonacci y el 999 final")
            # segunda ejecución: el estado no se arrastra entre RUNs
            r = w.call("RUN", path="/app/Hello.mod")
            text2, exited2 = w.drain_run()
            check(exited2 is not None and exited2.get("status") == "OK"
                  and text2 == text,
                  "2º RUN → misma salida (sin estado arrastrado)")

        # 8. errores limpios
        r = w.call("RUN", path="/app/NoExiste.mod")
        check(r.get("code") == "NOT_FOUND", "RUN de un módulo que no está → NOT_FOUND")
        r = w.call("GET", path="/nope")
        check(r.get("code") == "NOT_FOUND", "GET inexistente → NOT_FOUND")

        # 9. el FS PERSISTE entre conexiones (es una imagen en disco, como la flash)
        sock.close()
        sock = socket.create_connection(("127.0.0.1", PORT), timeout=5.0)
        sock.settimeout(30.0)
        w = Wire(sock)
        w.call("HELLO", protoVersion=1, clientName="smoke2")
        r, got = w.get("/app/demo/data.bin")
        check(got == blob, "reconexión: el fichero sigue ahí (FS persistente)")

        print("[status=%s]" % ("OK" if fails == 0 else "FAIL"))
        return 1 if fails else 0
    finally:
        proc.terminate()
        try: proc.wait(timeout=5)
        except Exception: proc.kill()
        shutil.rmtree(tmp, ignore_errors=True)

if __name__ == "__main__":
    sys.exit(main())
