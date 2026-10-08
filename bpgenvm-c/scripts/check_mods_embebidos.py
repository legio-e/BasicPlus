"""check_mods_embebidos.py — ¿la stdlib EMBEBIDA en las imágenes es la de bpstdlib? (8-oct-2026)

POR QUÉ EXISTE. La imagen de cada familia lleva su stdlib de fábrica en un C GENERADO (pico/pico_mods.c,
esp32/common/esp32_mods.c, stm32/port/stm32_mods.c: scripts/regen_mods.sh con el envoltorio de cada familia), y al
arrancar REPONE en /lib su copia cuando la de la placa es de la misma generación con otro CRC (src/bpvm_mods.c, #466).
El 8-oct, al preparar el lote de placas de #668 y #667, los tres generados eran del 2-oct: una placa grabada con ellos
habría vuelto en cada arranque a un Core sin __destruye y a unas clases sin destructores —#668 y #667 apagados en la
placa sin decirlo más que en el log—. Nada lo miraba: los generados cachean por fuente, como los .mod de las pruebas.

QUÉ MIRA. Cada módulo de la tabla de cada generado (`{ "/lib/X.mod", x_mod, sizeof x_mod }`) contra bpstdlib/X.mod,
byte a byte. Los que no van a /lib (el Hello.mod de muestra de la Pico, en /app) no son de la stdlib: no se miran.

    python bpgenvm-c/scripts/check_mods_embebidos.py      # 0: todo igual; 1: alguno distinto (y cómo regenerarlo)
"""
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
BPGEN = os.path.dirname(AQUI)
RAIZ = os.path.dirname(BPGEN)
STDLIB = os.path.join(RAIZ, "bpstdlib")
FAMILIAS = [
    ("pico", os.path.join(BPGEN, "pico", "pico_mods.c"), "bash bpgenvm-c/pico/scripts/regen_pico_mods.sh"),
    ("esp32", os.path.join(BPGEN, "esp32", "common", "esp32_mods.c"), "bash bpgenvm-c/esp32/scripts/regen_esp32_mods.sh"),
    ("stm32", os.path.join(BPGEN, "stm32", "port", "stm32_mods.c"), "bash bpgenvm-c/stm32/scripts/regen_stm32_mods.sh"),
]
BLOB = re.compile(r"static const unsigned char (\w+)\[\] = \{(.*?)\};", re.S)
TABLA = re.compile(r'\{\s*"(/lib/[^"]+\.mod)",\s*(\w+),\s*sizeof\s+\w+\s*\}')


def blobs(texto):
    out = {}
    for nombre, cuerpo in BLOB.findall(texto):
        out[nombre] = bytes(int(h, 16) for h in re.findall(r"0x([0-9a-fA-F]{2})", cuerpo))
    return out


def main():
    malas = 0
    for fam, ruta, regen in FAMILIAS:
        if not os.path.exists(ruta):
            print("FALLO %s: no existe %s" % (fam, ruta))
            malas += 1
            continue
        texto = open(ruta, "r", encoding="utf-8", errors="replace").read()
        b = blobs(texto)
        tabla = TABLA.findall(texto)
        distintos = []
        for destino, var in tabla:
            nombre = os.path.basename(destino)
            fuente = os.path.join(STDLIB, nombre)
            if not os.path.exists(fuente):
                distintos.append("%s (no está en bpstdlib)" % nombre)
                continue
            if b.get(var) != open(fuente, "rb").read():
                distintos.append(nombre)
        if distintos:
            malas += 1
            print("FALLO %s: %d de %d módulos embebidos NO son los de bpstdlib: %s — regenerar con: %s"
                  % (fam, len(distintos), len(tabla), ", ".join(distintos), regen))
        else:
            print("ok  %s: los %d módulos embebidos son los de bpstdlib" % (fam, len(tabla)))
    return 1 if malas else 0


if __name__ == "__main__":
    sys.exit(main())
