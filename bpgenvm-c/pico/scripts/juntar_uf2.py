#!/usr/bin/env python3
"""juntar_uf2.py — V8/H5 (22-oct): LA IMAGEN de la RP2350, con sus dos firmwares en un .uf2.

Uso (lo llama CMakeLists.txt al compilar):
    juntar_uf2.py salida.uf2 p.uf2 m.uf2 p.elf m.elf

P es el firmware de siempre, al principio de la flash; M, el mismo con la VM caliente y el cerrojo de FreeRTOS en la
SRAM, en 0x10100000 (CMakeLists.txt, bp_memmap.ld; el salto, main.c: pico_pasar_a_m). Antes de juntarlos comprueba:
  · que sean PAREJA: la firma de cada uno (.bp_firma, a 4 KB del principio de su zona 1) dice P y M, con el mismo
    número de la configuración del build;
  · que lo que SOBREVIVE A UN REINICIO en la RAM (.ram_vector_table y .uninitialized_data: el log, las migas, la marca
    del fallo fatal) esté en el mismo sitio en los dos: P pasa a M sin reiniciar, y M lee lo que dejó el arranque de
    antes —el suyo o el de P— donde éste lo dejó;
  · que no se pisen.
Y los junta. El bloque del erratum E10 (en 0x10FFFF00, el primero de cada .uf2 de picotool) es una descarga APARTE, con
su propia numeración: va una vez, tal cual. Los de la flash, de los dos, UNA serie 0..n-1 de n: numerados todos juntos,
el ROM no da la grabación por terminada y la placa se queda en BOOTSEL (probado en la Metro el 22-oct).
"""
import struct
import sys

MAG0, MAG1, MAGF = 0x0A324655, 0x9E5D5157, 0x0AB16F30
ESPECIALES = 0x10FF0000      # de aquí arriba no es la flash: el bloque del E10
FIRMA_DESPL = 0x1000         # bp_memmap.ld: .bp_firma, en ORIGIN(FLASH_BOOT) + 0x1000
FIRMA_MAGIA = 0x4D465042     # "BPFM" (main.c)
RAM_QUE_SOBREVIVE = (".ram_vector_table", ".uninitialized_data")


def falla(msg):
    sys.exit("juntar_uf2: " + msg)


def bloques(nombre):
    datos = open(nombre, "rb").read()
    if not datos or len(datos) % 512:
        falla("%s no es un .uf2 (vacio, o no es multiplo de 512)" % nombre)
    res = []
    for i in range(0, len(datos), 512):
        b = datos[i:i + 512]
        m0, m1, _flags, addr, size = struct.unpack_from("<5I", b, 0)
        mf, = struct.unpack_from("<I", b, 508)
        if m0 != MAG0 or m1 != MAG1 or mf != MAGF:
            falla("%s: el bloque %d no lleva las firmas del UF2" % (nombre, i // 512))
        res.append({"addr": addr, "size": size, "raw": b})
    return res


def flash_de(bs):
    return [b for b in bs if b["addr"] < ESPECIALES]


def firma(nombre, bs):
    """(magia, pareja, imagen) de la firma del firmware de este .uf2: a 4 KB de su primer bloque de flash."""
    fl = flash_de(bs)
    if not fl:
        falla("%s no tiene bloques de flash" % nombre)
    en = min(b["addr"] for b in fl) + FIRMA_DESPL
    for b in fl:
        if b["addr"] <= en and en + 16 <= b["addr"] + b["size"]:
            return struct.unpack_from("<3I", b["raw"], 32 + en - b["addr"])
    falla("%s no lleva firma en 0x%08x" % (nombre, en))


def elf(nombre):
    """Las secciones {nombre: (direccion, tamano, indice)} y los simbolos [(nombre, valor, tamano, seccion)] de un ELF32."""
    d = open(nombre, "rb").read()
    if d[:4] != b"\x7fELF" or d[4] != 1 or d[5] != 1:
        falla("%s no es un ELF de 32 bits little-endian" % nombre)
    shoff, = struct.unpack_from("<I", d, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from("<3H", d, 0x2E)
    # sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size, sh_link, sh_info, sh_addralign, sh_entsize
    shs = [struct.unpack_from("<10I", d, shoff + i * shentsize) for i in range(shnum)]

    def cadena(off):
        return d[off:d.index(b"\0", off)].decode("latin-1")

    secs = {cadena(shs[shstrndx][4] + sh[0]): (sh[3], sh[5], i) for i, sh in enumerate(shs)}
    syms = []
    for sh in shs:
        if sh[1] == 2:   # SHT_SYMTAB
            strtab = shs[sh[6]][4]
            for j in range(sh[5] // 16):
                st_name, st_value, st_size, _info, _other, st_shndx = struct.unpack_from("<3I2BH", d, sh[4] + j * 16)
                syms.append((cadena(strtab + st_name), st_value, st_size, st_shndx))
    return secs, syms


def misma_ram(elf_p, elf_m):
    sp, yp = elf(elf_p)
    sm, ym = elf(elf_m)
    for s in RAM_QUE_SOBREVIVE:
        a, b = sp.get(s), sm.get(s)
        if (a[:2] if a else None) != (b[:2] if b else None):
            falla("%s no esta en el mismo sitio en P y en M (P: %s; M: %s)" % (
                s, "%08x+%d" % a[:2] if a else "no hay", "%08x+%d" % b[:2] if b else "no hay"))
        if a:
            en_p = sorted((n, v, t) for n, v, t, i in yp if i == a[2] and n)
            en_m = sorted((n, v, t) for n, v, t, i in ym if i == b[2] and n)
            if en_p != en_m:
                falla("en %s, P y M no tienen lo mismo en el mismo sitio: %s" % (s, sorted(set(en_p) ^ set(en_m))[:6]))


def main():
    if len(sys.argv) != 6:
        falla("uso: juntar_uf2.py salida.uf2 p.uf2 m.uf2 p.elf m.elf")
    salida, uf2_p, uf2_m, elf_p, elf_m = sys.argv[1:]
    bp, bm = bloques(uf2_p), bloques(uf2_m)
    fp, fm = firma(uf2_p, bp), firma(uf2_m, bm)
    for nombre, f, letra in ((uf2_p, fp, "P"), (uf2_m, fm, "M")):
        if f[0] != FIRMA_MAGIA or f[2] != ord(letra):
            falla("%s no es %s: su firma dice %08x/%08x" % (nombre, letra, f[0], f[2]))
    if fp[1] != fm[1]:
        falla("P y M no son pareja (P: %08x; M: %08x): son de dos configuraciones del build" % (fp[1], fm[1]))
    misma_ram(elf_p, elf_m)

    especiales, vistos = [], set()
    for b in bp + bm:
        clave = (b["addr"], b["raw"][32:32 + b["size"]])
        if b["addr"] >= ESPECIALES and clave not in vistos:
            vistos.add(clave)
            especiales.append(b)
    flash = flash_de(bp) + flash_de(bm)
    ocupado = set()
    for b in flash:
        for a in range(b["addr"], b["addr"] + b["size"], 256):
            if a in ocupado:
                falla("P y M se pisan en 0x%08x" % a)
            ocupado.add(a)
    n = len(flash)
    with open(salida, "wb") as f:
        for b in especiales:
            f.write(b["raw"])
        for i, b in enumerate(flash):
            raw = bytearray(b["raw"])
            struct.pack_into("<2I", raw, 20, i, n)   # blockNo, numBlocks
            f.write(bytes(raw))

    def tramo(bs):
        fl = flash_de(bs)
        return "%d bloques, 0x%08x-0x%08x" % (len(fl), min(b["addr"] for b in fl),
                                               max(b["addr"] + b["size"] for b in fl))
    print("juntar_uf2: %s = P (%s) + M (%s), pareja %08x; la RAM que sobrevive, en el mismo sitio" % (
        salida, tramo(bp), tramo(bm), fp[1]))


main()
