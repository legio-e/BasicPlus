#!/usr/bin/env python3
# enlace_v0.py -- V7/H2 (#523): LA LINEA BASE DEL ENLACE, con numero (V0 del plan de validacion).
#
# Ejecuta cada programa N veces en las dos VMs (host) y recoge las lineas de diag
#   [bpvm] tabla de simbolos: S simbolos, B B de nombres, T B en total
#   [bpvm] enlace: M modulos, Q busquedas, C strcmp, U us
# (miVM imprime las mismas con [miVM]). Imprime una tabla con la MEDIANA de us y los
# demas numeros, y los tamanos de los .mod de la stdlib. Se guarda en compat/informes/
# para comparar despues del enlace tardio: «no mas lento» se demuestra contra esto.
#
#   python compat/enlace_v0.py <dir con los .mod> <Prog1> [<Prog2> ...] [--n=7] [--out=fichero]
import io, os, re, statistics, subprocess, sys, time

sys.stdout.reconfigure(encoding='utf-8', errors='replace')
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
JVM = os.path.join(ROOT, 'toolchain', 'miVM', 'target', 'bpgenvm-1.0.jar')
CVM = os.path.join(ROOT, 'bpgenvm-c', 'build', 'bpgenvm-c.exe')
RX_SIM = re.compile(r'tabla de simbolos: (\d+) simbolos(?:, (\d+) B de nombres, (\d+) B en total)?')
RX_ENL = re.compile(r'enlace: (\d+) modulos, (\d+) busquedas, (\d+) strcmp, (\d+) us')

def corre(vm, d, mod):
    args = ['java', '-Djava.awt.headless=true', '-jar', vm, '--no-config', mod] if vm.endswith('.jar') else [vm, mod]
    p = subprocess.run(args, cwd=d, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, timeout=60)
    err = p.stderr.decode('utf-8', 'replace')
    ms, me = RX_SIM.search(err), RX_ENL.search(err)
    if not ms or not me: raise SystemExit('sin diag de enlace en %s para %s:\n%s' % (os.path.basename(vm), mod, err[-800:]))
    return (int(ms.group(1)), int(ms.group(2) or 0), int(ms.group(3) or 0),
            int(me.group(1)), int(me.group(2)), int(me.group(3)), int(me.group(4)))

def main():
    d = sys.argv[1]; progs = [a for a in sys.argv[2:] if not a.startswith('--')]
    n = 7; out = None
    for a in sys.argv[2:]:
        if a.startswith('--n='): n = int(a[4:])
        if a.startswith('--out='): out = a[6:]
    lineas = ['# Linea base del enlace (V0, #523) -- %s' % time.strftime('%Y-%m-%d %H:%M'),
              '# host: %d ejecuciones por programa y VM; us = mediana' % n, '']
    lineas.append('%-14s %-5s %8s %10s %8s %8s %10s %10s %8s' % ('programa', 'VM', 'modulos', 'simbolos', 'nombres', 'total', 'busquedas', 'strcmp', 'us'))
    for prog in progs:
        for vm, et in ((JVM, 'java'), (CVM, 'c')):
            filas = [corre(vm, d, prog + '.mod') for _ in range(n)]
            us = statistics.median(f[6] for f in filas); f = filas[0]
            lineas.append('%-14s %-5s %8d %10d %8d %8d %10d %10d %8d' % (prog, et, f[3], f[0], f[1], f[2], f[4], f[5], us))
    lineas.append(''); lineas.append('# tamanos de la stdlib (bpstdlib/*.mod)')
    tot = 0
    for f in sorted(os.listdir(os.path.join(ROOT, 'bpstdlib'))):
        if f.endswith('.mod'):
            sz = os.path.getsize(os.path.join(ROOT, 'bpstdlib', f)); tot += sz
            lineas.append('%-20s %7d' % (f, sz))
    lineas.append('%-20s %7d' % ('TOTAL', tot))
    txt = '\n'.join(lineas) + '\n'
    print(txt)
    if out:
        os.makedirs(os.path.dirname(out) or '.', exist_ok=True)
        io.open(out, 'w', encoding='utf-8', newline='\n').write(txt); print('guardado en', out)

if __name__ == '__main__':
    main()
