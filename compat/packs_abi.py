# -*- coding: utf-8 -*-
"""packs_abi.py — V7/H6 G5.b0 (#575): los `.mdn` de los packs, en la ABI que habla la VM.

POR QUE EXISTE. La VM rechaza un `.mdn` cuya ABI no sea EXACTAMENTE la suya
(`mdn_loader.c`: `h->abi_version != MDN_ABI_VERSION`), y las `native` externas de
un pack nativo (SQLite.bp, Gui2.bp: `import v1 from pack "XXXX"`) no tienen otro
cuerpo que uno que LANZA «falta el codigo nativo del pack». O sea que subir la ABI
sin regenerar los `.mdn` de los packs deja SQLite y Gui2 sin funcionar en TODAS las
placas con imagen nueva. Pasó el 20-ago (#504: ABI 4 contra 6) y otra vez el 26-sep
(la 9 de P2b.1.d), aunque el LEEME de SQLite lo decía: nada lo miraba. Esto lo mira.

QUE COMPRUEBA
  1. Las dos puntas dicen la misma ABI: `MDN_ABI_VERSION` en bpgenvm-c/src/mdn_format.h
     (la VM) y en lexer-java/.../MdnPack.java (quien escribe el `.mdn`).
  2. Cada `.mdn` VERSIONADO (los ingredientes de `bpstdlib/**/nativo/`: de ahí se
     rehace cada pack) está en esa ABI.
  3. Cada `.pack` que se puede grabar —`packs/`, los `out/` de los proyectos de pack
     y los versionados— lleva sus entradas `mdn` en esa ABI. Se leen por el ÍNDICE
     del pack (cabecera de 128 B, entradas de 48 B, big-endian: bpvm_pack.h), no
     buscando la marca a ciegas.

    python compat/packs_abi.py              la comprobación (0 = al día, 1 = no, 2 = no se pudo leer)
    python compat/packs_abi.py --listar P   las entradas del pack P (tipo, nombre, bytes, ABI si es mdn)

REMEDIO cuando falla: regenerar los `.mdn` con PackAot (el mismo paso que el IDE) y
reconstruir los packs — la receta está en bpstdlib/sqlite/LEEME.md y en el
comentario de cada .bpbuild.
"""
import glob
import os
import re
import struct
import subprocess
import sys

RAIZ = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
MDN_FORMAT = os.path.join(RAIZ, 'bpgenvm-c', 'src', 'mdn_format.h')
MDN_PACK = os.path.join(RAIZ, 'lexer-java', 'src', 'main', 'java', 'basicplus', 'frontend', 'MdnPack.java')

PACK_MAGIC = 0x4250414B        # 'BPAK'
PACK_VERFMT = 1
PACK_HEADER = 128
ENTRY_HSIZE = 48


def rel(p):
    return os.path.relpath(p, RAIZ).replace(os.sep, '/')


def abi_de(fichero, patron):
    txt = open(fichero, encoding='utf-8', errors='replace').read()
    m = re.search(patron, txt, re.M)
    return int(m.group(1)) if m else None


_MDN_VERSION = []


def mdn_version_max():
    """Hasta qué versión de cabecera carga la VM (mdn_loader.c: de 1 a MDN_VERSION), leída de mdn_format.h como la
    ABI. V7/H5 P2b.3: con un 1-2 fijo, la v3 (la marca DIRECTA) salía «ABI None» y la red daba rojo por la razón
    equivocada."""
    if not _MDN_VERSION:
        _MDN_VERSION.append(abi_de(MDN_FORMAT, r'^#define\s+MDN_VERSION\s+(\d+)') or 2)
    return _MDN_VERSION[0]


def abi_mdn(b):
    """La ABI de un .mdn (cabecera little-endian: magic, versión, abi), o None si no lo es."""
    if len(b) < 8 or b[:4] != b'MDN\x00':
        return None
    ver, abi = struct.unpack('<HH', b[4:8])
    return abi if 1 <= ver <= mdn_version_max() else None


def entradas(b):
    """Las entradas de un .pack, recorriendo su índice: [(tipo, nombre, bytes)]. ValueError si no es un pack."""
    if len(b) < PACK_HEADER or struct.unpack('>I', b[0:4])[0] != PACK_MAGIC:
        raise ValueError('no es un pack (magic)')
    if struct.unpack('>H', b[4:6])[0] != PACK_VERFMT:
        raise ValueError('formato de pack desconocido')
    total = min(struct.unpack('>I', b[8:12])[0], len(b))
    out, off = [], PACK_HEADER
    while off + 4 <= total:
        tipo = b[off:off + 4]
        if tipo == b'\xff\xff\xff\xff':
            break                                   # el hueco borrado: fin de entradas
        if off + ENTRY_HSIZE > total:
            raise ValueError('entrada truncada en %d' % off)
        nombre = b[off + 4:off + 36].split(b'\x00', 1)[0].decode('utf-8', 'replace')
        largo = struct.unpack('>I', b[off + 36:off + 40])[0]
        datos = off + ENTRY_HSIZE
        if largo > total - datos:
            raise ValueError('la entrada %s se sale del pack' % nombre)
        out.append((tipo.split(b'\x00', 1)[0].decode('ascii', 'replace'), nombre, b[datos:datos + largo]))
        off = datos + ((largo + 3) & ~3)
    return out


def versionados(patron):
    try:
        r = subprocess.run(['git', '-C', RAIZ, 'ls-files', patron], capture_output=True, text=True)
        if r.returncode == 0:
            return [os.path.join(RAIZ, l) for l in r.stdout.splitlines() if l.strip()]
    except OSError:
        pass
    return []


def listar(p):
    for tipo, nombre, datos in entradas(open(p, 'rb').read()):
        a = abi_mdn(datos) if tipo == 'mdn' else None
        print('  %-4s %-32s %8d B%s' % (tipo, nombre, len(datos), ('   ABI %d' % a) if a is not None else ''))


def main(argv):
    # En UTF-8, como el resto de compat.sh: con la consola en cp1252 el «✗» reventaba el print y
    # la red daba rojo POR LA RAZÓN EQUIVOCADA (visto al sabotearla).
    try:
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    except (AttributeError, ValueError):
        pass
    if len(argv) > 2 and argv[1] == '--listar':
        listar(argv[2])
        return 0
    vm = abi_de(MDN_FORMAT, r'^#define\s+MDN_ABI_VERSION\s+(\d+)')
    pc = abi_de(MDN_PACK, r'MDN_ABI_VERSION\s*=\s*(\d+)\s*;')
    if vm is None or pc is None:
        print('  ✗ no se pudo leer MDN_ABI_VERSION (VM %s, MdnPack %s)' % (vm, pc))
        return 2
    malos, vistos = [], 0
    if vm != pc:
        malos.append('las dos puntas no dicen lo mismo: la VM habla la ABI %d y MdnPack escribe la %d' % (vm, pc))

    # 2 — los ingredientes versionados.
    for f in sorted(set(versionados('*.mdn.*') + versionados('*.mdn'))):
        vistos += 1
        a = abi_mdn(open(f, 'rb').read())
        if a != vm:
            malos.append('%s: ABI %s, la VM habla la %d — regeneralo con PackAot' % (rel(f), a, vm))

    # 3 — los packs que se pueden grabar.
    packs = glob.glob(os.path.join(RAIZ, 'packs', '*.pack'))
    packs += glob.glob(os.path.join(RAIZ, 'bpstdlib', '**', 'out', '*.pack'), recursive=True)
    packs += glob.glob(os.path.join(RAIZ, 'bpgenvm-c', 'tools', '**', 'out', '*.pack'), recursive=True)
    packs += versionados('*.pack')
    for p in sorted(set(os.path.abspath(x) for x in packs)):
        try:
            es = entradas(open(p, 'rb').read())
        except ValueError as e:
            malos.append('%s: no se puede leer (%s)' % (rel(p), e))
            continue
        for tipo, nombre, datos in es:
            if tipo != 'mdn':
                continue
            vistos += 1
            a = abi_mdn(datos)
            if a != vm:
                malos.append('%s: su %s.mdn va en la ABI %s y la VM habla la %d — reconstruye el pack'
                             % (rel(p), nombre, a, vm))

    for m in malos:
        print('  ✗ ' + m)
    print('  packs-abi: %d .mdn mirados, %d fuera de la ABI %d' % (vistos, len(malos), vm))
    return 1 if malos else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
