#!/usr/bin/env python3
"""
bpvm-pico.py — cliente CLI para el firmware bpvm-pico (RP2350).

Habla el protocolo line-based descrito en repl.c sobre USB CDC. Permite
subir, ejecutar y gestionar ficheros .mod en la Pico sin reflashear.

Requiere pyserial:
    pip install pyserial

Uso:
    python bpvm-pico.py --port COM5 ls
    python bpvm-pico.py --port COM5 put samples/BenchCpu.mod
    python bpvm-pico.py --port COM5 run BenchCpu.mod
    python bpvm-pico.py --port COM5 del BenchCpu.mod
    python bpvm-pico.py --port COM5 mem
    python bpvm-pico.py --port COM5 save
    python bpvm-pico.py --port COM5 repl     # terminal interactivo

Si no pasas --port intenta auto-detectar buscando un puerto con
"Pico" o "Board CDC" en la descripción.
"""

from __future__ import annotations
import argparse
import os
import sys
import time

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    sys.stderr.write(
        "ERROR: necesita pyserial.\n"
        "  pip install pyserial\n")
    sys.exit(2)


# ----- helpers ---------------------------------------------------- #

def autodetect_port() -> str | None:
    for p in serial.tools.list_ports.comports():
        desc = (p.description or "").lower()
        if "pico" in desc or "board cdc" in desc or "rp2" in desc:
            return p.device
        # VID/PID de Raspberry Pi (0x2E8A)
        if p.vid == 0x2E8A:
            return p.device
    return None


def open_port(port: str, baud: int = 115200) -> serial.Serial:
    s = serial.Serial(port, baud, timeout=1.0, dsrdtr=True)
    s.dtr = True          # importante: el firmware mira DTR
    s.rts = False
    return s


def send_line(s: serial.Serial, line: str) -> None:
    s.write((line + "\n").encode("utf-8"))
    s.flush()


def read_line(s: serial.Serial, timeout: float = 5.0) -> str:
    """Lee una línea (hasta \n) del puerto. Levanta TimeoutError."""
    end = time.monotonic() + timeout
    buf = bytearray()
    while time.monotonic() < end:
        c = s.read(1)
        if not c:
            continue
        if c == b"\n":
            return buf.decode("utf-8", errors="replace").rstrip("\r")
        buf.extend(c)
    raise TimeoutError(f"readline timeout: got={bytes(buf)!r}")


def drain(s: serial.Serial, secs: float = 0.2) -> None:
    """Consume cualquier basura pendiente."""
    end = time.monotonic() + secs
    while time.monotonic() < end:
        chunk = s.read(256)
        if not chunk:
            break


def expect_ok(s: serial.Serial) -> str:
    """Lee la respuesta y exige que empiece por OK. Devuelve el resto."""
    # El firmware envía un prompt "> " antes de leer comandos; lo saltamos
    # si lo vemos. Y puede haber ecos / banners residuales: leemos hasta
    # encontrar una línea que empiece por OK o ERR.
    for _ in range(20):
        line = read_line(s)
        if line.startswith("OK"):
            return line[3:].strip()
        if line.startswith("ERR"):
            raise RuntimeError(line)
        # ignora prompt, banner, etc.
    raise RuntimeError("no se vio OK/ERR")


# ----- comandos --------------------------------------------------- #

def cmd_hello(s: serial.Serial) -> None:
    send_line(s, "HELLO")
    print(expect_ok(s))


def cmd_ls(s: serial.Serial) -> None:
    send_line(s, "LS")
    head = expect_ok(s)                # "N"
    n = int(head)
    print(f"{n} fichero(s):")
    for _ in range(n):
        print("  " + read_line(s))


def cmd_mem(s: serial.Serial) -> None:
    send_line(s, "MEM")
    print(expect_ok(s))


def cmd_put(s: serial.Serial, path: str, remote_name: str | None = None) -> None:
    if not os.path.isfile(path):
        sys.exit(f"no existe: {path}")
    data = open(path, "rb").read()
    name = remote_name or os.path.basename(path)
    print(f"PUT {name} ({len(data)} bytes)...")
    send_line(s, f"PUT {name} {len(data)}")

    # Esperar "READY <size>"
    line = read_line(s)
    while not line.startswith("READY"):
        if line.startswith("ERR"):
            sys.exit(line)
        line = read_line(s)

    # Stream binario
    s.write(data)
    s.flush()

    print(expect_ok(s))


def cmd_get(s: serial.Serial, name: str, out_path: str | None = None) -> None:
    send_line(s, f"GET {name}")
    rest = expect_ok(s)          # "<size>"
    size = int(rest)
    data = bytearray()
    end = time.monotonic() + 10
    while len(data) < size and time.monotonic() < end:
        chunk = s.read(min(4096, size - len(data)))
        data.extend(chunk)
    if len(data) != size:
        sys.exit(f"GET truncado: {len(data)}/{size}")
    out_path = out_path or name
    with open(out_path, "wb") as f:
        f.write(data)
    print(f"GET {name} → {out_path} ({size} bytes)")


def cmd_del(s: serial.Serial, name: str) -> None:
    send_line(s, f"DEL {name}")
    print(expect_ok(s) or "OK")


def cmd_run(s: serial.Serial, name: str) -> None:
    send_line(s, f"RUN {name}")
    # El RUN imprime "--- VM output ---" y luego output libre hasta
    # "--- VM finished: STATUS ---". Streameamos todo.
    in_output = False
    end_marker = "--- VM finished:"
    print(f"RUN {name}")
    while True:
        line = read_line(s, timeout=30.0)
        if line.startswith("ERR"):
            sys.exit(line)
        if line.startswith("--- VM output ---"):
            in_output = True
            print(line)
            continue
        if line.startswith(end_marker):
            print(line)
            break
        if in_output:
            print(line)
        # antes del marker de inicio puede haber prompts; los ignoramos


def cmd_save(s: serial.Serial) -> None:
    send_line(s, "SAVE")
    print(expect_ok(s) or "OK")


def cmd_format(s: serial.Serial) -> None:
    send_line(s, "FORMAT")
    print(expect_ok(s) or "OK")


def cmd_reset(s: serial.Serial) -> None:
    send_line(s, "RESET")
    try:
        print(expect_ok(s))
    except Exception:
        pass


def cmd_bootsel(s: serial.Serial) -> None:
    send_line(s, "BOOTSEL")
    try:
        print(expect_ok(s))
    except Exception:
        pass


def cmd_repl(s: serial.Serial) -> None:
    """Modo interactivo bidireccional. Ctrl-C para salir."""
    import threading
    stop = threading.Event()

    def reader():
        while not stop.is_set():
            try:
                line = read_line(s, timeout=0.3)
                print(line)
            except TimeoutError:
                continue
            except Exception:
                break

    t = threading.Thread(target=reader, daemon=True)
    t.start()
    print("[REPL] Ctrl-C para salir.")
    try:
        while True:
            line = input()
            send_line(s, line)
    except (KeyboardInterrupt, EOFError):
        stop.set()
        print("\n[REPL] bye")


# ----- main ------------------------------------------------------- #

def main() -> int:
    ap = argparse.ArgumentParser(description="Cliente para bpvm-pico")
    ap.add_argument("--port", help="Puerto COM (auto si se omite)")
    ap.add_argument("--baud", type=int, default=115200)
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("hello")
    sub.add_parser("ls")
    sub.add_parser("mem")
    sub.add_parser("save")
    sub.add_parser("format")
    sub.add_parser("reset")
    sub.add_parser("bootsel")
    sub.add_parser("repl")

    p_put = sub.add_parser("put"); p_put.add_argument("path")
    p_put.add_argument("--as", dest="remote_name", default=None)

    p_get = sub.add_parser("get"); p_get.add_argument("name")
    p_get.add_argument("--out", default=None)

    p_del = sub.add_parser("del"); p_del.add_argument("name")
    p_run = sub.add_parser("run"); p_run.add_argument("name")

    args = ap.parse_args()

    port = args.port or autodetect_port()
    if not port:
        sys.exit("No se detectó la Pico. Pasa --port COMn manualmente.")

    print(f"# puerto: {port}")
    s = open_port(port, args.baud)
    drain(s, 0.3)

    try:
        if   args.cmd == "hello":   cmd_hello(s)
        elif args.cmd == "ls":      cmd_ls(s)
        elif args.cmd == "mem":     cmd_mem(s)
        elif args.cmd == "save":    cmd_save(s)
        elif args.cmd == "format":  cmd_format(s)
        elif args.cmd == "reset":   cmd_reset(s)
        elif args.cmd == "bootsel": cmd_bootsel(s)
        elif args.cmd == "put":     cmd_put(s, args.path, args.remote_name)
        elif args.cmd == "get":     cmd_get(s, args.name, args.out)
        elif args.cmd == "del":     cmd_del(s, args.name)
        elif args.cmd == "run":     cmd_run(s, args.name)
        elif args.cmd == "repl":    cmd_repl(s)
    finally:
        s.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
