#!/usr/bin/env python3
"""fat_imagen.py — V7/H3 (24-sep): una tarjeta SD de mentira para el micro simulado (`--sd=IMAGEN`).

El firmware NO formatea (`FF_USE_MKFS 0`: una tarjeta la prepara el ordenador), así que para probar la SD sin
placa hace falta una imagen hecha fuera. Ésta la escribe como vienen las de verdad: una TABLA DE PARTICIONES
(MBR) en el bloque 0 y un volumen FAT16 que empieza en el 2048 —así se ejerce también el camino del MBR, que
es el de las tarjetas de fábrica—, con etiqueta y un fichero dentro para que `SD_MOUNT` tenga «primera
entrada» que enseñar.

    python tools/fat_imagen.py SALIDA.img [--mb 8] [--etiqueta BPSD] [--superfloppy]

Sin dependencias: la estructura se escribe a mano, campo a campo, con los nombres del BPB de la especificación
de Microsoft (FAT: General Overview of On-Disk Format).
"""
import argparse
import struct
import sys

SECTOR = 512
LBA_PARTICION = 2048          # 1 MB: donde la ponen las tarjetas de fábrica y los formateadores de hoy
SPC = 2                       # sectores por cluster: 1 KB
RESERVADOS = 4
NFATS = 2
ENTRADAS_RAIZ = 512           # 32 sectores de raíz
FICHERO = ("HOLA", "TXT", b"hola desde la SD del micro simulado\n")


def volumen_fat16(sectores, lba_oculto, etiqueta):
    """Los `sectores` de un volumen FAT16 (bytes), con su etiqueta y un fichero."""
    raiz_sec = ENTRADAS_RAIZ * 32 // SECTOR
    fatsz = 1
    while True:   # el tamaño de la FAT depende de los clusters, y los clusters de la FAT: se itera hasta fijo
        clusters = (sectores - RESERVADOS - NFATS * fatsz - raiz_sec) // SPC
        nuevo = -(-((clusters + 2) * 2) // SECTOR)
        if nuevo == fatsz:
            break
        fatsz = nuevo
    if not 4085 <= clusters < 65525:
        sys.exit(f"fat_imagen: {clusters} clusters no es FAT16 (4085..65524): cambia --mb")

    img = bytearray(sectores * SECTOR)
    bs = bytearray(SECTOR)
    bs[0:3] = b"\xEB\x3C\x90"
    bs[3:11] = b"BPSIM1.0"                               # el «formateador»: lo enseña SD_INFO (oemFs)
    struct.pack_into("<HBHBHHBHHHII", bs, 11,
                     SECTOR, SPC, RESERVADOS, NFATS, ENTRADAS_RAIZ,
                     sectores if sectores < 65536 else 0, 0xF8, fatsz, 63, 255,
                     lba_oculto, sectores if sectores >= 65536 else 0)
    bs[36] = 0x80                                        # unidad
    bs[38] = 0x29                                        # firma de BPB extendido
    struct.pack_into("<I", bs, 39, 0x20260924)           # número de serie del volumen
    bs[43:54] = etiqueta.upper().encode("ascii")[:11].ljust(11, b" ")
    bs[54:62] = b"FAT16   "
    bs[510:512] = b"\x55\xAA"
    img[0:SECTOR] = bs

    fat = bytearray(fatsz * SECTOR)
    struct.pack_into("<HHH", fat, 0, 0xFFF8, 0xFFFF, 0xFFFF)   # medio, reservada, y el cluster 2: el fichero, fin
    for n in range(NFATS):
        ini = (RESERVADOS + n * fatsz) * SECTOR
        img[ini:ini + len(fat)] = fat

    fecha = ((2026 - 1980) << 9) | (9 << 5) | 24
    raiz = (RESERVADOS + NFATS * fatsz) * SECTOR
    img[raiz:raiz + 11] = etiqueta.upper().encode("ascii")[:11].ljust(11, b" ")
    img[raiz + 11] = 0x08                                # la etiqueta del volumen, también en la raíz
    nombre, ext, datos = FICHERO
    e = raiz + 32
    img[e:e + 11] = nombre.ljust(8).encode("ascii") + ext.ljust(3).encode("ascii")
    img[e + 11] = 0x20                                   # archivo
    struct.pack_into("<HH", img, e + 22, 0, fecha)       # hora y fecha de escritura
    struct.pack_into("<HI", img, e + 26, 2, len(datos))  # primer cluster y tamaño
    datos_sec = RESERVADOS + NFATS * fatsz + raiz_sec
    img[datos_sec * SECTOR:datos_sec * SECTOR + len(datos)] = datos
    return img, clusters


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("salida")
    ap.add_argument("--mb", type=int, default=8)
    ap.add_argument("--etiqueta", default="BPSD")
    ap.add_argument("--superfloppy", action="store_true", help="sin MBR: el volumen empieza en el bloque 0")
    a = ap.parse_args()

    total = a.mb * 1024 * 1024 // SECTOR
    if a.superfloppy:
        img, clusters = volumen_fat16(total, 0, a.etiqueta)
    else:
        vol, clusters = volumen_fat16(total - LBA_PARTICION, LBA_PARTICION, a.etiqueta)
        img = bytearray(total * SECTOR)
        mbr = bytearray(SECTOR)                          # sin código: los bytes 3..10 a cero dicen «MBR»
        mbr[446] = 0x00                                  # no arrancable
        mbr[447:450] = b"\xFE\xFF\xFF"                   # CHS: «usa el LBA»
        mbr[450] = 0x06 if (total - LBA_PARTICION) * SECTOR >= 32 * 1024 * 1024 else 0x04   # FAT16
        mbr[451:454] = b"\xFE\xFF\xFF"
        struct.pack_into("<II", mbr, 454, LBA_PARTICION, total - LBA_PARTICION)
        mbr[510:512] = b"\x55\xAA"
        img[0:SECTOR] = mbr
        img[LBA_PARTICION * SECTOR:] = vol
    with open(a.salida, "wb") as f:
        f.write(img)
    print(f"fat_imagen: {a.salida}: {a.mb} MB, FAT16 de {clusters} clusters,"
          f" {'sin MBR' if a.superfloppy else f'particion en el bloque {LBA_PARTICION}'},"
          f" etiqueta {a.etiqueta.upper()}, 1 fichero")


if __name__ == "__main__":
    main()
