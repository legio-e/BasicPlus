#!/usr/bin/env python3
# test/dbg_client.py — H6.b.2.b: cliente de prueba del servidor wire del
# debugger de la VM-C (test/debug_listen.c). Arranca el servidor, conduce un
# guion de debug por el wire-v1 y verifica el comportamiento end-to-end SIN
# hardware. Es el oraculo de la ruta host-first (cascada Java->C->Pico).
#
# Guion: HELLO -> PING -> PAUSE -> RUN -> BP_HIT -> STEP -> STEP -> CONTINUE
#        -> OUTPUT(s) -> EXITED(OK).
#
# Uso: python3 dbg_client.py <ruta_servidor_exe> <fichero.mod> [cwd]
import json, socket, subprocess, sys, time, os

def free_port():
    s = socket.socket(); s.bind(("127.0.0.1", 0)); p = s.getsockname()[1]; s.close()
    return p

class Wire:
    def __init__(self, sock):
        self.s = sock; self.buf = b""
    def send(self, obj):
        self.s.sendall((json.dumps(obj) + "\n").encode("utf-8"))
    def recv(self, timeout=5.0):
        self.s.settimeout(timeout)
        while b"\n" not in self.buf:
            chunk = self.s.recv(4096)
            if not chunk:
                raise EOFError("servidor cerro la conexion")
            self.buf += chunk
        line, self.buf = self.buf.split(b"\n", 1)
        return json.loads(line.decode("utf-8"))
    def recv_until(self, mtype, timeout=5.0):
        """Lee mensajes hasta uno con type==mtype; acumula OUTPUT/otros."""
        others = []
        while True:
            m = self.recv(timeout)
            if m.get("type") == mtype:
                return m, others
            others.append(m)

def main():
    if len(sys.argv) < 3:
        print("uso: dbg_client.py <servidor_exe> <fichero.mod> [cwd]"); return 2
    exe, mod = sys.argv[1], sys.argv[2]
    cwd = sys.argv[3] if len(sys.argv) > 3 else "."
    port = free_port()

    proc = subprocess.Popen([exe, str(port), mod], cwd=cwd,
                            stderr=subprocess.DEVNULL)
    fails = []
    try:
        # conectar con reintento (el server tarda en abrir el socket)
        sock = None
        for _ in range(50):
            try:
                sock = socket.create_connection(("127.0.0.1", port), timeout=1.0); break
            except OSError:
                time.sleep(0.05)
        if sock is None:
            print("FAIL: no se pudo conectar al servidor"); return 1
        w = Wire(sock)

        # 1) HELLO
        w.send({"type": "HELLO", "id": 1, "protoVersion": 1, "clientName": "pytest"})
        hr = w.recv()
        if hr.get("type") != "HELLO_REPLY": fails.append(f"HELLO_REPLY ausente: {hr}")
        if hr.get("serverName") != "bpvm-c": fails.append(f"serverName != bpvm-c: {hr.get('serverName')}")
        print("HELLO_REPLY:", hr.get("serverName"), hr.get("serverBuild"))

        # 2) PING/PONG
        w.send({"type": "PING", "id": 2}); pong = w.recv()
        if pong.get("type") != "PONG": fails.append(f"PONG ausente: {pong}")

        # 3) PAUSE antes de RUN -> rompe en el 1er opcode
        w.send({"type": "PAUSE", "id": 3}); pr = w.recv()
        if pr.get("type") != "PAUSE_REPLY": fails.append(f"PAUSE_REPLY ausente: {pr}")

        # 4) RUN -> RUN_REPLY, luego BP_HIT (por la pausa pedida)
        w.send({"type": "RUN", "id": 4})
        rr, _ = w.recv_until("RUN_REPLY")
        bh, _ = w.recv_until("BP_HIT")
        pcs = [bh["pc"]]
        if "sp" not in bh or "bp" not in bh: fails.append("BP_HIT sin sp/bp (frame crudo)")
        # avanza algunos steps hasta entrar en un frame con locales (sp>bp), para
        # que el cross-check READ_INT==LOCALS[0] ejerza de verdad.
        for k in range(20):
            if bh.get("sp", 0) > bh.get("bp", 0): break
            w.send({"type": "STEP", "id": 30 + k}); bh, _ = w.recv_until("BP_HIT"); pcs.append(bh["pc"])
        print("BP_HIT pc=%s sp=%s bp=%s" % (bh.get("pc"), bh.get("sp"), bh.get("bp")))
        bp0, sp0 = bh.get("bp"), bh.get("sp")

        # 4b) Inspección CRUDA mientras pausados (H6.b.2.c): LOCALS/STACK/READ_INT.
        w.send({"type": "LOCALS", "id": 41}); loc, _ = w.recv_until("LOCALS_REPLY")
        locals_arr = loc.get("locals", [])
        exp = (sp0 - bp0) // 4 if (sp0 is not None and bp0 is not None) else -1
        if len(locals_arr) != exp: fails.append(f"LOCALS len {len(locals_arr)} != (sp-bp)/4 {exp}")
        print("LOCALS:", locals_arr)

        w.send({"type": "STACK", "id": 42}); stk, _ = w.recv_until("STACK_REPLY")
        frames = stk.get("frames", [])
        if len(frames) < 1: fails.append("STACK sin frames")
        print("STACK frames:", frames)

        # READ_INT en bp+0 debe coincidir con LOCALS[0] (cross-check del wire crudo).
        if locals_arr:
            w.send({"type": "READ_INT", "id": 43, "addr": bp0}); ri, _ = w.recv_until("READ_INT_REPLY")
            if ri.get("value") != locals_arr[0]:
                fails.append(f"READ_INT(bp) {ri.get('value')} != LOCALS[0] {locals_arr[0]}")

        # READ_STRING(0) debe devolver cadena vacía (ref nula), sin colgarse.
        w.send({"type": "READ_STRING", "id": 44, "ref": 0}); rs, _ = w.recv_until("READ_STRING_REPLY")
        if rs.get("value", None) is None: fails.append("READ_STRING sin 'value'")

        # 5) SET_BP en un pc conocido (ejercita el comando)
        w.send({"type": "SET_BP", "id": 5, "pc": pcs[0]})
        sb, _ = w.recv_until("SET_BP_REPLY")
        if sb.get("bpId", -1) <= 0: fails.append(f"SET_BP_REPLY bpId<=0: {sb}")

        # 6) STEP x2 -> dos BP_HIT mas
        for k in (6, 7):
            w.send({"type": "STEP", "id": k})
            bh, _ = w.recv_until("BP_HIT")
            pcs.append(bh["pc"])
        print("pcs tras step:", pcs)
        if len(pcs) < 3: fails.append("esperaba >=3 BP_HIT")

        # 7) CONTINUE -> corre al final -> EXITED(OK), con OUTPUT por el camino
        w.send({"type": "CONTINUE", "id": 8})
        ex, outs = w.recv_until("EXITED", timeout=8.0)
        if ex.get("status") != "OK": fails.append(f"EXITED status != OK: {ex}")
        nout = sum(1 for m in outs if m.get("type") == "OUTPUT")
        print("EXITED status=%s (OUTPUT msgs=%d)" % (ex.get("status"), nout))

        sock.close()
    finally:
        try: proc.wait(timeout=3)
        except Exception: proc.kill()

    if fails:
        print("FALLOS:"); [print("  -", f) for f in fails]; return 1
    print("OK: H6.b.2.b wire del debugger host (HELLO/PING/PAUSE/RUN/BP_HIT/SET_BP/STEP/CONTINUE/EXITED)")
    return 0

if __name__ == "__main__":
    sys.exit(main())
