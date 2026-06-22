#!/usr/bin/env python3
# pc_logserver.py — servidor TCP de log para el bring-up del ESP32-P4.
#
# El P4 (IP estática 192.168.2.2) abre una conexión TCP a este PC
# (192.168.2.1:5555) y vuelca líneas de log/traza. Este server las
# imprime con timestamp. Sirve como canal de observabilidad ANTES de
# portar la VM (ver V3_IDEAS §6 / plan P4).
#
# Uso:  python pc_logserver.py [puerto]      (por defecto 5555)
#       Ctrl+C para salir.
#
# Nota Windows: si el P4 conecta pero no se ve nada, suele ser el
# Firewall bloqueando el puerto entrante -> permitir Python en la red
# privada, o abrir el puerto 5555/TCP entrante.
import socket
import sys
import time
import threading

HOST = "0.0.0.0"          # escucha en todas las interfaces (incl. 192.168.2.1)
DEFAULT_PORT = 5555


def handle(conn, addr):
    print(f"[+] P4 conectado desde {addr[0]}:{addr[1]}", flush=True)
    buf = b""
    try:
        while True:
            data = conn.recv(4096)
            if not data:
                break
            buf += data
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                txt = line.decode("utf-8", "replace").rstrip("\r")
                print(f"  {time.strftime('%H:%M:%S')} | {txt}", flush=True)
        if buf:
            print(f"  (parcial) | {buf.decode('utf-8', 'replace')}", flush=True)
    except (ConnectionResetError, OSError):
        pass
    finally:
        print(f"[-] P4 desconectado ({addr[0]})", flush=True)
        conn.close()


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_PORT
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((HOST, port))
    srv.listen(5)
    print(f"== log-server P4 escuchando en {HOST}:{port} ==  (Ctrl+C para salir)",
          flush=True)
    try:
        while True:
            conn, addr = srv.accept()
            threading.Thread(target=handle, args=(conn, addr), daemon=True).start()
    except KeyboardInterrupt:
        print("\n== fin ==", flush=True)
    finally:
        srv.close()


if __name__ == "__main__":
    main()
