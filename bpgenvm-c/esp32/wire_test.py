#!/usr/bin/env python3
# wire_test.py — prueba directa del wire v1 del firmware ESP32 (H4.3).
#
# Abre el puerto SIN resetear el chip (DTR=RTS=False → EN/IO0 en alto),
# manda un HELLO y muestra lo que responda. Aísla si el problema es el
# firmware (no contesta) o el IDE (handshake/DTR-RTS).
#
# Uso (en la ESP-IDF PowerShell, que tiene pyserial):
#   python wire_test.py COM11
import serial, time, sys

PORT = sys.argv[1] if len(sys.argv) > 1 else 'COM11'
s = serial.Serial()
s.port = PORT
s.baudrate = 115200
s.timeout = 0.1
s.dtr = False          # no tocar el auto-reset
s.rts = False
s.open()
print(f"[open] {PORT} @115200  dtr=rts=False")
time.sleep(0.5)

pre = s.read(8192)
print("[pre ]", repr(pre) if pre else "(nada — el REPL está en silencio, bien)")
s.reset_input_buffer()

msg = b'{"type":"HELLO","id":1}\n'
s.write(msg); s.flush()
print("[tx  ]", msg)

buf = b''
t0 = time.time()
while time.time() - t0 < 2.0:
    chunk = s.read(256)
    if chunk:
        buf += chunk
print("[rx  ]", repr(buf) if buf else "(NADA — el firmware no respondió en 2s)")

# Segundo intento por si el primero coincidió con ruido de boot.
s.reset_input_buffer()
s.write(msg); s.flush()
buf2 = b''
t0 = time.time()
while time.time() - t0 < 2.0:
    chunk = s.read(256)
    if chunk:
        buf2 += chunk
print("[rx2 ]", repr(buf2) if buf2 else "(NADA)")
s.close()
