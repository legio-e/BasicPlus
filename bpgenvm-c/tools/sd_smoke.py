#!/usr/bin/env python3
# sd_smoke.py — V7/H3 (24-sep): LA SD DEL MICRO SIMULADO, por el wire. El sim con `--sd=IMAGEN` registra un
# fichero como tarjeta (el mismo contrato de bloque que el SPI de la Metro o el SDMMC del P4) y todo lo de
# encima es el código de las placas: el registro común (`bpvm_sd_placa`), FAT y los dos verbos. Tres sims:
#   · CON tarjeta (una imagen de `fat_imagen.py`: MBR + FAT16 en el bloque 2048, etiqueta y un fichero):
#     SD_INFO la identifica y lee el MBR; el arranque ya la montó; SD_MOUNT da la etiqueta y el fichero; y el
#     FS de la fachada lee, escribe y lista en /sd, y lo escrito SIGUE ahí tras volver a montar;
#   · SIN tarjeta: los dos verbos dicen que no hay SD, con el motivo —no «type no implementado»—;
#   · con una imagen ROTA (sin FS): SD_MOUNT falla con el motivo y el LBA, y el wire sigue vivo.
#
#   python tools/sd_smoke.py
import json, os, socket, subprocess, sys, tempfile, time

try: sys.stdout.reconfigure(encoding="utf-8")
except Exception: pass

HERE = os.path.dirname(os.path.abspath(__file__))
SIM = os.path.join(HERE, "..", "build", "bpvm-sim.exe")
if not os.path.exists(SIM):
    SIM = SIM[:-4]
SIM = os.environ.get("BPVM_SIM") or SIM
PORT = 5111

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
        self.id += 1
        fields.update(type=typ, id=self.id, bulk=len(data))
        self.s.sendall((json.dumps(fields) + "\n").encode() + data)
        return self._line()
    def get(self, path):
        r = self.call("GET", path=path)
        if r.get("type") != "GET_REPLY": return r, b""
        return r, self._bulk(r["bulk"])

def arranca(tmp, nombre, extra):
    """Un sim en una placa virgen, con `extra` en la línea de órdenes. Devuelve (proceso, wire)."""
    flash = os.path.join(tmp, nombre + ".flash")
    proc = subprocess.Popen([SIM, str(PORT), flash, "4194304"] + extra,
                            stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    for _ in range(80):
        try:
            sock = socket.create_connection(("127.0.0.1", PORT), timeout=2.0)
            w = Wire(sock)
            w.call("HELLO", protoVersion=1, clientName="sd_smoke")
            return proc, w
        except OSError:
            time.sleep(0.1)
    proc.kill()
    raise RuntimeError("el sim no acepta conexión")

def para(proc):
    proc.terminate()
    try: proc.wait(timeout=3)
    except subprocess.TimeoutExpired: proc.kill()

def main():
    tmp = tempfile.mkdtemp(prefix="sd_smoke_")
    img = os.path.join(tmp, "tarjeta.img")
    subprocess.run([sys.executable, os.path.join(HERE, "fat_imagen.py"), img], check=True,
                   stdout=subprocess.DEVNULL)
    print("=== sd_smoke (V7/H3: la SD del micro simulado, por el wire) ===")

    # ── 1. CON tarjeta ───────────────────────────────────────────────────────────────────────────────
    proc, w = arranca(tmp, "con", ["--sd=" + img])
    try:
        r = w.call("SD_INFO")
        check(r.get("type") == "SD_INFO_REPLY" and r.get("ok") is True and r.get("medio") == "fichero",
              "SD_INFO con tarjeta → ok, por el medio «fichero» (dice %r, %r)" % (r.get("ok"), r.get("motivo")))
        check(r.get("producto") == "BPSIM" and r.get("oem") == "BP" and r.get("serie") == 20260924
              and r.get("anno") == 2026 and r.get("mes") == 9 and r.get("version") == 2,
              "SD_INFO → la identidad de la tarjeta, campo a campo")
        check(r.get("bloques") == 8 * 1024 * 1024 // 512, "SD_INFO → la capacidad: %r bloques" % r.get("bloques"))
        check(r.get("traza") == "", "SD_INFO → sin traza: el medio no la tiene (un «00 00…» diría «línea flotante»)")
        check(r.get("leeSector0") is True and r.get("firma") == "55 AA" and r.get("clase") == "MBR",
              "SD_INFO → lee el sector 0: 55 AA, una TABLA DE PARTICIONES")
        check(r.get("particiones") == 1 and r.get("parteLba") == 2048 and r.get("parteTipo") == 4
              and r.get("oemFs") == "BPSIM1.0",
              "SD_INFO → la partición: 1, en el bloque 2048, FAT16, formateada por «BPSIM1.0»")

        # el arranque ya la montó: /sd está sin pedir nada
        r = w.call("LIST_DIR", path="/sd")
        nombres = {e.get("name") for e in r.get("entries", [])}
        check(r.get("type") == "LIST_DIR_REPLY" and "HOLA.TXT" in nombres,
              "el ARRANQUE la montó: LIST_DIR /sd ve HOLA.TXT sin un SD_MOUNT (dice %r)" % sorted(nombres))
        r, datos = w.get("/sd/HOLA.TXT")
        check(datos == b"hola desde la SD del micro simulado\n", "GET /sd/HOLA.TXT → su contenido, byte a byte")

        r = w.call("SD_MOUNT")
        check(r.get("type") == "SD_MOUNT_REPLY" and r.get("ok") is True and r.get("prefijo") == "/sd"
              and r.get("lba") == 2048,
              "SD_MOUNT → ok, en /sd, partición en el bloque 2048")
        check(r.get("etiqueta") == "BPSD" and r.get("primera") == "HOLA.TXT" and r.get("entradasRaiz") == 1,
              "SD_MOUNT → la etiqueta y la primera entrada de la raíz (dice %r, %r, %r)"
              % (r.get("etiqueta"), r.get("primera"), r.get("entradasRaiz")))
        check((r.get("kbTotal") or 0) > 6000 and 0 < (r.get("kbLibres") or 0) <= r.get("kbTotal"),
              "SD_MOUNT → el tamaño: %r KB, %r libres" % (r.get("kbTotal"), r.get("kbLibres")))

        blob = bytes((i * 7) & 0xFF for i in range(5000))
        r = w.call_bulk("PUT", blob, path="/sd/datos.bin")
        check(r.get("type") == "PUT_REPLY", "PUT /sd/datos.bin (5000 B, 5 clusters) → OK")
        r = w.call("SD_MOUNT")   # volver a montar: lo escrito tiene que estar EN LA TARJETA, no en una caché
        r, datos = w.get("/sd/datos.bin")
        check(datos == blob, "…y tras volver a montar, GET lo devuelve igual: está en la tarjeta")
        r = w.call("SD_MOUNT")
        check(r.get("entradasRaiz") == 2, "SD_MOUNT → la raíz tiene ya 2 entradas")
    finally:
        para(proc)

    # ── 2. SIN tarjeta ───────────────────────────────────────────────────────────────────────────────
    proc, w = arranca(tmp, "sin", [])
    try:
        r = w.call("SD_INFO")
        check(r.get("type") == "SD_INFO_REPLY" and r.get("ok") is False
              and "no tiene SD" in (r.get("motivo") or ""),
              "SD_INFO sin tarjeta → ok=false y el porqué: %r" % r.get("motivo"))
        r = w.call("SD_MOUNT")
        check(r.get("type") == "SD_MOUNT_REPLY" and r.get("ok") is False
              and "no tiene SD" in (r.get("motivo") or ""),
              "SD_MOUNT sin tarjeta → lo mismo, no «type no implementado»")
    finally:
        para(proc)

    # ── 3. una tarjeta ROTA: sin sistema de ficheros ─────────────────────────────────────────────────
    rota = os.path.join(tmp, "rota.img")
    with open(rota, "wb") as f:
        f.write(bytes(4 * 1024 * 1024))
    proc, w = arranca(tmp, "rota", ["--sd=" + rota])
    try:
        r = w.call("SD_INFO")
        check(r.get("ok") is True and r.get("leeSector0") is True and r.get("firma") == "00 00",
              "SD_INFO con una tarjeta sin formatear → la identifica, y el sector 0 no es 55 AA")
        r = w.call("SD_MOUNT")
        check(r.get("ok") is False and r.get("motivo") and r.get("lba") == 0,
              "SD_MOUNT → falla con su motivo (%r) y el LBA" % r.get("motivo"))
        r = w.call("PING")
        check(r.get("type") == "PONG", "…y el wire sigue vivo")
    finally:
        para(proc)

    print("[status=OK]" if fails == 0 else f"[status=FAIL: {fails}]")
    return 0 if fails == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
