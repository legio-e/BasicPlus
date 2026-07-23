#!/usr/bin/env python3
# boardsim_smoke.py — H9: smoke end-to-end del sim de gestión de placa (boardsim.c).
# Arranca build/bpvm-boardsim.exe en un puerto de test con una "flash" virgen, recorre
# el flujo de placa virgen por el wire (el mismo que hará FrmBoard) y verifica cada
# respuesta. No necesita placa ni el IDE. Sale 0 si todo pasa.
#
#   python tools/boardsim_smoke.py
import json, socket, subprocess, sys, os, time, tempfile

try: sys.stdout.reconfigure(encoding="utf-8")   # consola Windows cp1252 → UTF-8
except Exception: pass

PORT = 5107
HERE = os.path.dirname(os.path.abspath(__file__))
SIM  = os.path.join(HERE, "..", "build", "bpvm-boardsim.exe")
if not os.path.exists(SIM):
    SIM = SIM[:-4]  # sin .exe (Linux/mac)

fails = 0
def check(cond, msg):
    global fails
    print(("  ok  : " if cond else "  FAIL: ") + msg)
    if not cond: fails += 1

class Wire:
    def __init__(self, sock):
        self.s = sock; self.buf = b""; self.id = 0
    def _reply(self):
        while b"\n" not in self.buf:
            chunk = self.s.recv(4096)
            if not chunk: raise EOFError("sim cerró la conexión")
            self.buf += chunk
        line, self.buf = self.buf.split(b"\n", 1)
        return json.loads(line.decode())
    def call(self, typ, **fields):
        self.id += 1
        fields.update(type=typ, id=self.id)
        self.s.sendall((json.dumps(fields) + "\n").encode())
        return self._reply()
    def call_bulk(self, typ, data, **fields):
        """Línea JSON con "bulk":N + N bytes crudos detrás (framing del PUT/BURN)."""
        self.id += 1
        fields.update(type=typ, id=self.id, bulk=len(data))
        self.s.sendall((json.dumps(fields) + "\n").encode() + data)
        return self._reply()

def main():
    flash = os.path.join(tempfile.gettempdir(), "boardsim_smoke.flash")
    if os.path.exists(flash): os.remove(flash)   # placa VIRGEN
    proc = subprocess.Popen([SIM, str(PORT), flash, "4194304"],
                            stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    try:
        # esperar a que el sim escuche
        sock = None
        for _ in range(50):
            try:
                sock = socket.create_connection(("127.0.0.1", PORT), timeout=1.0); break
            except OSError:
                time.sleep(0.1)
        if sock is None:
            print("FAIL: el sim no aceptó conexión"); return 1
        w = Wire(sock)

        print("=== boardsim smoke (H9 protocolo de gestión de placa) ===")

        # 1. handshake
        r = w.call("HELLO", protoVersion=1, clientName="smoke")
        caps = r.get("capabilities", [])
        check(r.get("type") == "HELLO_REPLY" and "BOARDMGR" in caps and "PACKS" in caps,
              "HELLO → HELLO_REPLY con capabilities BOARDMGR + PACKS")

        # 2. placa VIRGEN: escrutinio
        r = w.call("STATE")
        check(r.get("state") == 0 and r.get("name") == "kernel", "STATE virgen → estado 0 (kernel)")
        r = w.call("ENV_LS")
        check(r.get("entries") == [], "ENV_LS virgen → 0 variables")
        r = w.call("PART_LS")
        check(r.get("missing") is True, "PART_LS virgen → missing (el IDE abre el asistente)")

        # 3. asistente paso 1: entorno
        for k, v in [("board", "Pico2"), ("flashSizeBytes", "4194304"),
                     ("psram", "0"), ("gpioCount", "26")]:
            r = w.call("ENV_SET", key=k, value=v)
            check(r.get("type") == "ENV_SET_REPLY", f"ENV_SET {k}={v}")
        r = w.call("ENV_LS")
        check(len(r.get("entries", [])) == 4, "ENV_LS → 4 variables")
        r = w.call("ENV_GET", key="board")
        check(r.get("value") == "Pico2", "ENV_GET board → Pico2")
        r = w.call("STATE")
        check(r.get("state") == 1 and r.get("name") == "particiones",
              "STATE con env pero sin particiones → estado 1")

        # 4. asistente paso 2: particiones
        r = w.call("PART_DEFAULTS")
        sizes = {p["name"]: p["size"] for p in r["parts"]}
        check(sizes["fs"] + sizes["packs"] == 0x400000 - 0x100000, "PART_DEFAULTS reparte los 3 MB")
        r = w.call("PART_APPLY", fs=0x100000, packs=0x100000)
        check(r.get("type") == "PART_APPLY_REPLY", "PART_APPLY (1M+1M) → OK")
        r = w.call("PART_LS")
        parts = {p["name"]: p for p in r["parts"]}
        check(r.get("missing") is False
              and parts["fs"]["offset"] == 0x100000
              and parts["packs"]["offset"] == 0x200000,
              "PART_LS → offsets derivados (fs@1M, packs@2M)")
        r = w.call("STATE")
        check(r.get("state") == 3, "STATE tras aplicar → estado 3 (app)")

        # 5. editar / borrar entorno
        r = w.call("ENV_SET", key="psram", value="1")
        r = w.call("ENV_GET", key="psram")
        check(r.get("value") == "1", "editar psram 0→1 persiste")
        r = w.call("ENV_DEL", key="gpioCount")
        r = w.call("ENV_GET", key="gpioCount")
        check(r.get("type") == "ERROR" and r.get("code") == "NOT_FOUND", "ENV_DEL gpioCount → borrada")

        # 6. validación fallida NO toca nada. OJO semántica 'un mando' (80503d5):
        #    packs = EL RESTO (lo que se mande se ignora) → para desbordar hay que
        #    pasarse con la KNOB (fs), no con la suma.
        r = w.call("PART_APPLY", fs=0x340000)   # 3.25M > 3M disponibles
        check(r.get("type") == "ERROR", "PART_APPLY inválido (fs 3.25M > 3M) → ERROR")
        r = w.call("PART_LS")
        check(r["parts"][0]["size"] == 0x100000, "el layout bueno sobrevive a la validación fallida")
        # 6b. degenerado VÁLIDO por diseño: fs se lo lleva todo → packs = resto = 0
        r = w.call("PART_APPLY", fs=0x300000)
        check(r.get("type") == "PART_APPLY_REPLY", "PART_APPLY fs=3M → OK (packs=resto puede ser 0)")
        r = w.call("PART_LS")
        check(r["parts"][1]["size"] == 0, "packs derivado = 0")
        r = w.call("PART_APPLY", fs=0x100000)   # restaurar el layout para lo que sigue
        check(r.get("type") == "PART_APPLY_REPLY", "restaurar fs=1M")

        # 7. persistencia: reconectar y comprobar que el estado sigue
        sock.close()
        sock = socket.create_connection(("127.0.0.1", PORT), timeout=1.0)
        w = Wire(sock)
        w.call("HELLO", protoVersion=1)
        r = w.call("STATE")
        check(r.get("state") == 3, "reconexión: el estado persiste (flash-file A/B) → sigue en 3")
        r = w.call("ENV_GET", key="psram")
        check(r.get("value") == "1", "reconexión: psram=1 persistió")

        # 8. H3 — PACK_LS: zona de packs del sim (virgen → 0 packs, cadena sana).
        #    El listado con packs reales lo cubren make test-pack (núcleo C, paridad
        #    Pack.jar) y el arranque del sim con --pack=<f.pack>.
        r = w.call("PACK_LS")
        check(r.get("type") == "PACK_LS_REPLY" and r.get("count") == 0
              and r.get("chainOk") is True and r.get("free") == r.get("regionSize"),
              "PACK_LS región virgen → 0 packs, cadena sana, todo libre")
        r = w.call("PACK_ENTRIES", offset=0)
        check(r.get("type") == "ERROR" and r.get("code") == "NOT_FOUND",
              "PACK_ENTRIES en región virgen → NOT_FOUND")
        r = w.call("PACK_ENTRIES")
        check(r.get("type") == "ERROR" and r.get("code") == "INVALID_PARAM",
              "PACK_ENTRIES sin offset → INVALID_PARAM")

        # 9. H3 — BURN por chunks: graba la fixture (Pack.jar) en la región
        #    virgen por el wire (BEGIN → DATA×n con bulk crudo → END) y aparece.
        fix = os.path.join(HERE, "..", "samples", "PackFixB.pack")
        img = open(fix, "rb").read()
        r = w.call("PACK_BURN_BEGIN", size=len(img))
        check(r.get("type") == "PACK_BURN_BEGIN_REPLY" and r.get("offset") == 0
              and r.get("chunkMax", 0) >= 1000, "BURN_BEGIN → offset 0 + chunkMax")
        okd = True
        for o in range(0, len(img), 1024):          # trozos de 1024 (multi-chunk, mult. de 16)
            r = w.call_bulk("PACK_BURN_DATA", img[o:o+1024])
            okd = okd and r.get("type") == "PACK_BURN_DATA_REPLY"
        check(okd, "BURN_DATA: todos los chunks aceptados")
        r = w.call("PACK_BURN_END")
        check(r.get("type") == "PACK_BURN_END_REPLY" and r.get("offset") == 0,
              "BURN_END → verificado en flash y ACTIVADO en 0")
        r = w.call("PACK_LS")
        check(r.get("count") == 1 and r["packs"][0]["name"] == "PackFixB"
              and r["packs"][0]["crcOk"] is True and r["packs"][0]["active"] is True,
              "PACK_LS tras burn → PackFixB valido y activo")
        r = w.call("PACK_BURN_END")
        check(r.get("type") == "ERROR" and r.get("code") == "INVALID_STATE",
              "BURN_END sin sesion → INVALID_STATE")

        # 10. H3 — PACK_FORMAT: estreno/recuperación de la zona (borra TODO,
        #     con confirmación explícita — nunca automático).
        r = w.call("PACK_FORMAT")
        check(r.get("type") == "ERROR" and r.get("code") == "INVALID_PARAM",
              "PACK_FORMAT sin confirm → INVALID_PARAM")
        r = w.call("PACK_FORMAT", confirm="YES")
        check(r.get("type") == "PACK_FORMAT_REPLY", "PACK_FORMAT confirm=YES → OK")
        r = w.call("PACK_LS")
        check(r.get("count") == 0 and r.get("chainOk") is True
              and r.get("free") == r.get("regionSize"),
              "tras format: 0 packs, cadena sana, todo libre")

        # 11. H3 fase 3 — PACK_DEL (tombstone): re-graba la fixture y la tumba.
        #     Se MARCA borrado (RMW de la 1ª página); el espacio no baja hasta
        #     compactar desde el PC.
        w.call("PACK_BURN_BEGIN", size=len(img))
        for o in range(0, len(img), 1024):
            w.call_bulk("PACK_BURN_DATA", img[o:o+1024])
        r = w.call("PACK_BURN_END")
        check(r.get("type") == "PACK_BURN_END_REPLY", "re-burn para probar el DEL")
        r = w.call("PACK_DEL", offset=0)
        check(r.get("type") == "PACK_DEL_REPLY", "PACK_DEL → tombstone")
        r = w.call("PACK_LS")
        check(r.get("count") == 1 and r["packs"][0]["active"] is False
              and r["packs"][0]["crcOk"] is True,
              "tras DEL: borrado pero cabecera valida (RMW)")
        r = w.call("PACK_DEL", offset=0)
        check(r.get("type") == "ERROR" and r.get("code") == "INVALID_STATE",
              "DEL repetido → INVALID_STATE (ya borrado)")
        r = w.call("PACK_DEL", offset=8192)
        check(r.get("type") == "ERROR" and r.get("code") == "NOT_FOUND",
              "DEL donde no hay pack → NOT_FOUND")

        print("[status=OK]" if fails == 0 else f"[status=FAIL: {fails}]")
        return 0 if fails == 0 else 1
    finally:
        proc.terminate()
        try: proc.wait(timeout=3)
        except subprocess.TimeoutExpired: proc.kill()

if __name__ == "__main__":
    sys.exit(main())
