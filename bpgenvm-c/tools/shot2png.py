# -*- coding: utf-8 -*-
"""shot2png.py — convierte una captura `.shot` de BasicPlus (Gui.shot) a PNG.

    python tools/shot2png.py entrada.shot [salida.png]

Sin salida, escribe junto a la entrada con extensión `.png`. Código de salida 0
si convierte, 1 si algo falla (mensaje en stderr). Sólo biblioteca estándar:
el LZ4 de bloque y el escritor PNG van aquí dentro (no hace falta `lz4` ni PIL).

El formato está en docs/SHOT_FORMAT.md (cabecera de 16 B + N bloques, todo
little-endian, píxel RGB565). Resumen de lo que hace este lector:

- Valida magic "BPSH", version 1, fmt 1 (RGB565) y codec 0 (crudo) o 1 (LZ4).
- Coloca cada bloque en el rectángulo que dice su cabecera: NO supone franjas
  de ancho completo ni orden de `y`. Avisa (sin fallar) si quedan píxeles sin
  cubrir o cubiertos dos veces.
- codec 1: descomprime el bloque LZ4 (formato de bloque estándar, sin frame) y
  exige que salgan EXACTAMENTE rawLen bytes.
- RGB565 -> RGB888 por REPLICACIÓN DE BITS: r8 = r5<<3 | r5>>2,
  g8 = g6<<2 | g6>>4, b8 = b5<<3 | b5>>2. Es la expansión que usan LVGL y el
  driver del host: 0 -> 0 y 31 -> 255 exactos, sin redondeo intermedio.
- La imagen es la LÓGICA (lo que LVGL pintó). Si la cabecera trae rot != 0 no
  se gira: se dice en el resumen y ya.
"""
import os
import struct
import sys
import zlib
from array import array

MAGIC = b"BPSH"
VERSION = 1
FMT_RGB565 = 1
CODEC_CRUDO = 0
CODEC_LZ4 = 1
NOMBRE_CODEC = {CODEC_CRUDO: "crudo", CODEC_LZ4: "LZ4"}
GRADOS_ROT = {0: 0, 1: 90, 2: 180, 3: 270}

CAB_FICHERO = struct.Struct("<4sBBBBHHI")   # magic, version, fmt, codec, rot, w, h, nblocks
CAB_BLOQUE = struct.Struct("<HHHHII")      # x1, y1, x2, y2, rawLen, compLen


class ShotError(Exception):
    """Error de formato o de datos: se imprime y se sale con 1."""


# ---------------------------------------------------------------------------
# LZ4 de bloque (sin frame), en Python puro
# ---------------------------------------------------------------------------

def lz4_descomprimir_bloque(src, raw_len):
    """Descomprime un bloque LZ4 estándar y devuelve exactamente `raw_len` bytes.

    Secuencia = token (4 bits literales | 4 bits match) + [extensión literales:
    bytes de 255 y un último < 255] + literales + offset u16 LE + [extensión
    match] ; longitud de match = nibble + 4 (+ extensión). La última secuencia
    sólo lleva literales y termina el bloque. Un match con offset menor que su
    longitud se solapa: se copia como lo haría un bucle byte a byte (el patrón
    de `offset` bytes se repite), que es lo que define el formato.
    """
    dst = bytearray()
    n = len(src)
    i = 0

    def leer_extension(i, base):
        while True:
            if i >= n:
                raise ShotError("LZ4: bloque truncado leyendo una longitud extendida")
            b = src[i]
            i += 1
            base += b
            if b != 255:
                return i, base

    while i < n:
        token = src[i]
        i += 1
        lit = token >> 4
        if lit == 15:
            i, lit = leer_extension(i, lit)
        if i + lit > n:
            raise ShotError("LZ4: bloque truncado dentro de los literales "
                            "(pide %d B, quedan %d)" % (lit, n - i))
        dst += src[i:i + lit]
        i += lit
        if i == n:
            break                      # última secuencia: sólo literales
        if i + 2 > n:
            raise ShotError("LZ4: bloque truncado leyendo el offset")
        offset = src[i] | (src[i + 1] << 8)
        i += 2
        if offset == 0:
            raise ShotError("LZ4: offset 0 (invalido)")
        if offset > len(dst):
            raise ShotError("LZ4: offset %d apunta antes del principio (hay %d B)"
                            % (offset, len(dst)))
        mlen = token & 0x0F
        if mlen == 15:
            i, mlen = leer_extension(i, mlen)
        mlen += 4
        ini = len(dst) - offset
        if offset >= mlen:
            dst += dst[ini:ini + mlen]
        else:
            # solape: el patrón de `offset` bytes se repite hasta cubrir mlen
            patron = bytes(dst[ini:])
            rep, resto = divmod(mlen, offset)
            dst += patron * rep + patron[:resto]
        if len(dst) > raw_len:
            raise ShotError("LZ4: el bloque descomprime a mas de rawLen=%d B" % raw_len)

    if len(dst) != raw_len:
        raise ShotError("LZ4: descomprimio %d B y el bloque declara rawLen=%d"
                        % (len(dst), raw_len))
    return bytes(dst)


# ---------------------------------------------------------------------------
# RGB565 -> RGB888
# ---------------------------------------------------------------------------

def tabla_565_a_888():
    """LUT de 65536 entradas: valor u16 -> 3 bytes RGB (replicación de bits)."""
    lut = [None] * 65536
    for v in range(65536):
        r5 = (v >> 11) & 0x1F
        g6 = (v >> 5) & 0x3F
        b5 = v & 0x1F
        lut[v] = bytes(((r5 << 3) | (r5 >> 2),
                        (g6 << 2) | (g6 >> 4),
                        (b5 << 3) | (b5 >> 2)))
    return lut


# ---------------------------------------------------------------------------
# PNG mínimo: RGB 8 bits, filtro 0 en cada fila
# ---------------------------------------------------------------------------

def _chunk(tipo, datos):
    return (struct.pack(">I", len(datos)) + tipo + datos
            + struct.pack(">I", zlib.crc32(tipo + datos) & 0xFFFFFFFF))


def escribir_png(ruta, w, h, rgb):
    """`rgb` son w*h*3 bytes, filas contiguas de arriba abajo."""
    stride = w * 3
    filas = bytearray()
    for y in range(h):
        filas.append(0)                             # filtro None
        filas += rgb[y * stride:(y + 1) * stride]
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)   # 8 bits, color type 2 = RGB
    png = (b"\x89PNG\r\n\x1a\n"
           + _chunk(b"IHDR", ihdr)
           + _chunk(b"IDAT", zlib.compress(bytes(filas), 9))
           + _chunk(b"IEND", b""))
    with open(ruta, "wb") as f:
        f.write(png)
    return len(png)


# ---------------------------------------------------------------------------
# Lectura del .shot
# ---------------------------------------------------------------------------

def leer_shot(ruta):
    """Devuelve (w, h, rot, codec, nblocks, bytes_comprimidos, rgb888, avisos)."""
    with open(ruta, "rb") as f:
        datos = f.read()
    if len(datos) < CAB_FICHERO.size:
        raise ShotError("fichero demasiado corto para la cabecera (%d B)" % len(datos))
    magic, version, fmt, codec, rot, w, h, nblocks = CAB_FICHERO.unpack_from(datos, 0)
    if magic != MAGIC:
        raise ShotError("magic %r: no es un .shot (se esperaba %r)" % (magic, MAGIC))
    if version != VERSION:
        raise ShotError("version %d no soportada (esta herramienta lee la %d)" % (version, VERSION))
    if fmt != FMT_RGB565:
        raise ShotError("fmt %d no soportado (solo 1 = RGB565)" % fmt)
    if codec not in NOMBRE_CODEC:
        raise ShotError("codec %d desconocido (0 = crudo, 1 = LZ4)" % codec)
    if rot not in GRADOS_ROT:
        raise ShotError("rot %d fuera de rango (0..3)" % rot)
    if w == 0 or h == 0:
        raise ShotError("pantalla de %dx%d: dimension nula" % (w, h))

    lut = tabla_565_a_888()
    rgb = bytearray(w * h * 3)
    cobertura = bytearray(w * h)          # cuántas veces se pinta cada píxel
    stride = w * 3
    pos = CAB_FICHERO.size
    comprimidos = 0
    avisos = []

    for k in range(nblocks):
        if pos + CAB_BLOQUE.size > len(datos):
            raise ShotError("bloque %d: fichero truncado en su cabecera (offset %d)" % (k, pos))
        x1, y1, x2, y2, raw_len, comp_len = CAB_BLOQUE.unpack_from(datos, pos)
        pos += CAB_BLOQUE.size
        if x1 > x2 or y1 > y2:
            raise ShotError("bloque %d: rectangulo invertido (%d,%d)-(%d,%d)" % (k, x1, y1, x2, y2))
        if x2 >= w or y2 >= h:
            raise ShotError("bloque %d: rectangulo (%d,%d)-(%d,%d) fuera de la pantalla %dx%d"
                            % (k, x1, y1, x2, y2, w, h))
        bw = x2 - x1 + 1
        bh = y2 - y1 + 1
        esperado = bw * bh * 2
        if raw_len != esperado:
            raise ShotError("bloque %d: rawLen=%d no cuadra con el rectangulo %dx%d (%d B)"
                            % (k, raw_len, bw, bh, esperado))
        if pos + comp_len > len(datos):
            raise ShotError("bloque %d: fichero truncado en los datos (compLen=%d, quedan %d)"
                            % (k, comp_len, len(datos) - pos))
        cuerpo = datos[pos:pos + comp_len]
        pos += comp_len
        comprimidos += comp_len

        if codec == CODEC_CRUDO:
            if comp_len != raw_len:
                raise ShotError("bloque %d: codec crudo pero compLen=%d != rawLen=%d"
                                % (k, comp_len, raw_len))
            crudo = cuerpo
        else:
            try:
                crudo = lz4_descomprimir_bloque(cuerpo, raw_len)
            except ShotError as e:
                raise ShotError("bloque %d: %s" % (k, e))

        pix = array("H", crudo)
        if sys.byteorder == "big":
            pix.byteswap()
        for fila in range(bh):
            y = y1 + fila
            base = fila * bw
            trozo = b"".join([lut[v] for v in pix[base:base + bw]])
            ini = y * stride + x1 * 3
            rgb[ini:ini + bw * 3] = trozo
            cini = y * w + x1
            for cx in range(cini, cini + bw):
                cobertura[cx] += 1

    if pos != len(datos):
        avisos.append("sobran %d B tras el ultimo bloque" % (len(datos) - pos))
    sin_pintar = cobertura.count(0)
    if sin_pintar:
        avisos.append("%d pixeles sin cubrir por ningun bloque (quedan en negro)" % sin_pintar)
    repetidos = w * h - sin_pintar - cobertura.count(1)
    if repetidos:
        avisos.append("%d pixeles cubiertos por mas de un bloque (gana el ultimo)" % repetidos)

    return w, h, rot, codec, nblocks, comprimidos, bytes(rgb), avisos


# ---------------------------------------------------------------------------

def main(argv):
    if len(argv) < 2 or len(argv) > 3 or argv[1] in ("-h", "--help"):
        sys.stderr.write("uso: python shot2png.py entrada.shot [salida.png]\n")
        return 1
    entrada = argv[1]
    salida = argv[2] if len(argv) == 3 else os.path.splitext(entrada)[0] + ".png"
    try:
        w, h, rot, codec, nblocks, comprimidos, rgb, avisos = leer_shot(entrada)
        tam = escribir_png(salida, w, h, rgb)
    except ShotError as e:
        sys.stderr.write("shot2png: %s: %s\n" % (entrada, e))
        return 1
    except OSError as e:
        sys.stderr.write("shot2png: %s\n" % e)
        return 1

    crudo = w * h * 2
    ratio = (float(crudo) / comprimidos) if comprimidos else 0.0
    linea = ("%s: %dx%d rot=%d codec=%d (%s) %d bloques, %d B comprimidos de %d B crudos (%.1fx) -> %s (%d B)"
             % (entrada, w, h, rot, codec, NOMBRE_CODEC[codec], nblocks, comprimidos, crudo, ratio,
                salida, tam))
    if rot != 0:
        linea += " [la cabecera pide rot=%d (%d grados); la imagen es la LOGICA, no se gira]" % (
            rot, GRADOS_ROT[rot])
    print(linea)
    for a in avisos:
        sys.stderr.write("shot2png: aviso: %s\n" % a)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
