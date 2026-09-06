# -*- coding: utf-8 -*-
"""flash_pico.py — graba la Pico 2 / Metro SIN tocar el boton BOOTSEL.

    python tools/flash_pico.py COM22 [pico/build/bpvm_pico.uf2]

El firmware entiende el verbo BOOTSEL del wire: reinicia en modo cargador y la
placa aparece como unidad USB (RPI-RP2). Copiar el .uf2 ahi la graba y arranca
sola. Es lo que hace el IDE, y quita la unica parte del ciclo que exigia una mano.
"""
import sys, time, json, os, glob, shutil, subprocess

PUERTO = sys.argv[1] if len(sys.argv) > 1 else "COM22"
UF2    = sys.argv[2] if len(sys.argv) > 2 else os.path.join("bpgenvm-c", "pico", "build", "bpvm_pico.uf2")

def unidades_rp2():
    out = []
    try:
        r = subprocess.run(["powershell", "-NoProfile", "-Command",
                            "Get-Volume | Where-Object { $_.FileSystemLabel -like 'RP2*' -or $_.FileSystemLabel -like 'RPI*' } "
                            "| ForEach-Object { $_.DriveLetter }"],
                           capture_output=True, text=True, timeout=20)
        for l in r.stdout.split():
            if l.strip(): out.append(l.strip() + ":\\")
    except Exception:
        pass
    return out

if not os.path.exists(UF2):
    sys.exit("no existe el .uf2: " + UF2)
print("uf2: %s (%d B, %s)" % (UF2, os.path.getsize(UF2),
                              time.strftime("%d-%b %H:%M", time.localtime(os.path.getmtime(UF2)))))

antes = set(unidades_rp2())
import serial
try:
    s = serial.Serial(PUERTO, 115200, timeout=0)
    time.sleep(0.3)
    s.write(b'{"type":"BOOTSEL","id":1}\n'); s.flush()
    time.sleep(0.5)
    s.close()
    print("BOOTSEL enviado por", PUERTO)
except Exception as e:
    print("no se pudo mandar BOOTSEL (%s); si la placa ya esta en BOOTSEL, seguimos" % str(e)[:60])

destino = None
fin = time.time() + 20
while time.time() < fin:
    hay = [u for u in unidades_rp2() if u not in antes] or unidades_rp2()
    if hay: destino = hay[0]; break
    time.sleep(0.5)
if not destino:
    sys.exit("no aparecio la unidad del cargador (RP2350/RPI-RP2): la placa no entro en BOOTSEL")

print("unidad de cargador:", destino)
shutil.copyfile(UF2, os.path.join(destino, os.path.basename(UF2)))
print("copiado; la placa se reinicia sola")
