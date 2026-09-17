#!/usr/bin/env python3
# compat/v9_medidas.py -- V7/H2 (#523), V9 (17-sep): MOD8 contra MOD9 medido en el host, sample a sample.
# El corpus de compat.sh compilado en los dos formatos (MOD9 contra la stdlib compilada como MOD9), ejecutado
# en las dos VMs, y de cada ejecucion: el tamano del .mod raiz, el coste del enlace que cada VM imprime por
# stderr («enlace: N modulos, B busquedas, S strcmp, T us») y lo que construyo el enlace tardio («enlace
# tardio: C clases construidas, R refs, A bytes de arena»). Uso:
#   python compat/v9_medidas.py [--work=DIR] [--solo=nombre,nombre]
# Escribe compat/informes/v9_medidas_<fecha>.tsv y un resumen por pantalla.
import io, os, re, shutil, subprocess, sys, tempfile, datetime, statistics
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FE = ROOT + '/lexer-java/target/basicplus-frontend.jar'
JVM = ROOT + '/miVM/target/bpgenvm-1.0.jar'
CVM = ROOT + '/bpgenvm-c/build/bpgenvm-c.exe'
STD8 = ROOT + '/bpstdlib'
SAMPLES = ROOT + '/bpgenvm-c/samples'

def sh(args, cwd, timeout=60):
    try:
        p = subprocess.run(args, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
    except subprocess.TimeoutExpired as e:
        return -9, (e.stdout or b'').decode('utf-8', 'replace'), (e.stderr or b'').decode('utf-8', 'replace')
    return p.returncode, p.stdout.decode('utf-8', 'replace'), p.stderr.decode('utf-8', 'replace')

def corpus():
    s = io.open(ROOT + '/compat/compat.sh', encoding='utf-8').read()
    m = re.search(r'^CORPUS="([^"]*)"', s, re.M)
    return [x for x in m.group(1).split() if x != '\\']   # las barras de continuacion de linea

def bp_path(item):
    return ROOT + '/' + item if '/' in item else SAMPLES + '/' + item + '.bp'

def modulo_de(bp):
    s = io.open(bp, encoding='utf-8', errors='replace').read()
    m = re.search(r'^\s*module\s+([A-Za-z_][A-Za-z0-9_]*)', s, re.M)
    lib = re.search(r'^\s*library\s+"([A-Za-z0-9_.]+)"', s, re.M)
    return (m.group(1) if m else None), (lib.group(1) if lib else None)

def stdlib9(work):
    d = os.path.join(work, 'std9')
    if os.path.isdir(d): return d
    os.makedirs(os.path.join(d, 'src'))
    for f in os.listdir(STD8):
        if f.endswith('.bp'): shutil.copy(os.path.join(STD8, f), os.path.join(d, 'src', f))
    io.open(os.path.join(d, 'p.bpbuild'), 'w', encoding='utf-8', newline='\n').write(
        '{ "sourceDir": "src", "outDir": ".", "main": "Stdlib", "dependencies": [] }\n')
    rc, out, err = sh(['java', '-jar', FE, '--project', 'p.bpbuild', '--backend=mivm', '--formato=9'], d, 600)
    if rc != 0 or not os.path.isfile(os.path.join(d, 'Core.mod')): raise SystemExit('la stdlib no compila como MOD9:\n' + out + err)
    return d

def compilar(work, item, formato, std):
    """Como compat.sh: suelto; si no, por proyecto. Devuelve (dir, .mod raiz) o (None, motivo)."""
    bp = bp_path(item)
    if not os.path.isfile(bp): return None, 'no existe'
    nombre, lib = modulo_de(bp)
    if not nombre: return None, 'sin module'
    d = os.path.join(work, 'm%d_%s' % (formato, item.replace('/', '_').replace('.bp', '')))
    if os.path.isdir(d): shutil.rmtree(d)
    os.makedirs(d)
    for rec in re.findall(r'^//\s*recurso:\s*(\S+)', io.open(bp, encoding='utf-8', errors='replace').read(), re.M):
        if os.path.isfile(ROOT + '/' + rec): shutil.copy(ROOT + '/' + rec, d)
    for f in os.listdir(std):
        if f.endswith('.mod'): shutil.copy(os.path.join(std, f), d)
    fmt = '--formato=%d' % formato
    rc, out, err = sh(['java', '-jar', FE, bp, '--compile', '.', '--backend=mivm', fmt], d, 120)
    raiz = os.path.join(d, (lib + '.' if lib else '') + nombre + '.mod')
    if rc != 0 or not os.path.isfile(raiz):
        os.makedirs(os.path.join(d, 'src'), exist_ok=True); shutil.copy(bp, os.path.join(d, 'src'))
        io.open(os.path.join(d, 'p.bpbuild'), 'w', encoding='utf-8', newline='\n').write(
            '{ "sourceDir": "src", "outDir": ".", "main": "%s", "dependencies": ["%s"] }\n' % (nombre, std.replace('\\', '/')))
        rc, out, err = sh(['java', '-jar', FE, '--project', 'p.bpbuild', '--backend=mivm', fmt], d, 120)
        if rc != 0 or not os.path.isfile(raiz): return None, 'no compila'
    magic = io.open(raiz, 'rb').read(4)
    if magic != b'MOD%d' % formato: return None, 'magic %r' % magic
    return d, raiz

RX_ENLACE = re.compile(r'enlace: (\d+) modulos, (\d+) busquedas, (\d+) strcmp, (\d+) us')
RX_TARDIO = re.compile(r'enlace tardio: (\d+) clases? construidas?, (\d+) refs, (\d+) bytes de arena')
def medir(vm, d, raiz):
    env = dict(os.environ); env['BPVM_GUI_SHOT_MS'] = '1'
    args = ['java', '-Djava.awt.headless=true', '-jar', JVM, os.path.basename(raiz)] if vm == 'java' else [CVM, os.path.basename(raiz)]
    try:
        p = subprocess.run(args, cwd=d, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=20, env=env)
        err = p.stderr.decode('utf-8', 'replace')
    except subprocess.TimeoutExpired as e:
        err = (e.stderr or b'').decode('utf-8', 'replace')
    e = RX_ENLACE.search(err); t = RX_TARDIO.search(err)
    return ((int(e.group(1)), int(e.group(2)), int(e.group(3)), int(e.group(4))) if e else None,
            (int(t.group(1)), int(t.group(2)), int(t.group(3))) if t else (0, 0, 0))

def main():
    work = None; solo = None
    for a in sys.argv[1:]:
        if a.startswith('--work='): work = a[7:]
        elif a.startswith('--solo='): solo = a[7:].split(',')
        else: raise SystemExit('argumento desconocido: ' + a)
    for f in (FE, JVM, CVM):
        if not os.path.isfile(f): raise SystemExit('falta ' + f)
    work = work or tempfile.mkdtemp(prefix='v9m_')
    std9 = stdlib9(work)
    print('trabajo en', work, '| stdlib MOD9:', len([f for f in os.listdir(std9) if f.endswith('.mod')]), 'modulos')
    filas = []; saltados = []
    for item in corpus():
        if solo and item not in solo: continue
        fila = {'sample': item}
        ok = True
        for formato, std in ((8, STD8), (9, std9)):
            d, raiz = compilar(work, item, formato, std)
            if d is None: saltados.append((item, formato, raiz)); ok = False; break
            fila['bytes%d' % formato] = os.path.getsize(raiz)
            for vm in ('java', 'c'):
                e, t = medir(vm, d, raiz)
                if e is None: saltados.append((item, formato, 'sin linea de enlace en ' + vm)); ok = False; break
                fila['%s%d_mods' % (vm, formato)], fila['%s%d_busq' % (vm, formato)], fila['%s%d_strcmp' % (vm, formato)], fila['%s%d_us' % (vm, formato)] = e
                fila['%s%d_clases' % (vm, formato)], fila['%s%d_refs' % (vm, formato)], fila['%s%d_arena' % (vm, formato)] = t
            if not ok: break
        if ok:
            filas.append(fila)
            print('  %-30s .mod %6d -> %6d B | C enlace %5d -> %5d us, strcmp %5d -> %5d | Java %5d -> %5d us | construidas %d, refs %d, arena %d B' % (
                item, fila['bytes8'], fila['bytes9'], fila['c8_us'], fila['c9_us'], fila['c8_strcmp'], fila['c9_strcmp'],
                fila['java8_us'], fila['java9_us'], fila['c9_clases'], fila['c9_refs'], fila['c9_arena']))
    cols = ['sample', 'bytes8', 'bytes9'] + ['%s%d_%s' % (vm, f, k) for vm in ('c', 'java') for f in (8, 9) for k in ('mods', 'busq', 'strcmp', 'us', 'clases', 'refs', 'arena')]
    fecha = datetime.date.today().isoformat()
    out = os.path.join(ROOT, 'compat', 'informes', 'v9_medidas_%s.tsv' % fecha)
    io.open(out, 'w', encoding='utf-8', newline='\n').write('\t'.join(cols) + '\n' + '\n'.join('\t'.join(str(f.get(c, '')) for c in cols) for f in filas) + '\n')
    n = len(filas)
    if n:
        def med(k): return statistics.median(f[k] for f in filas)
        def tot(k): return sum(f[k] for f in filas)
        print('\n== RESUMEN (%d samples; mediana salvo que se diga) ==' % n)
        print('  .mod raiz:        %6d -> %6d B (total %d -> %d B, %+.1f %%)' % (med('bytes8'), med('bytes9'), tot('bytes8'), tot('bytes9'), 100.0 * (tot('bytes9') - tot('bytes8')) / tot('bytes8')))
        print('  VM-C  enlace:     %6d -> %6d us | strcmp %d -> %d | busquedas %d -> %d' % (med('c8_us'), med('c9_us'), med('c8_strcmp'), med('c9_strcmp'), med('c8_busq'), med('c9_busq')))
        print('  miVM  enlace:     %6d -> %6d us' % (med('java8_us'), med('java9_us')))
        print('  enlace tardio (MOD9, VM-C): clases construidas %d, refs %d, arena %d B (medianas); maximos %d / %d / %d B' % (
            med('c9_clases'), med('c9_refs'), med('c9_arena'), max(f['c9_clases'] for f in filas), max(f['c9_refs'] for f in filas), max(f['c9_arena'] for f in filas)))
        dif = [f['sample'] for f in filas if (f['c9_clases'], f['c9_refs']) != (f['java9_clases'], f['java9_refs'])]
        print('  clases/refs distintas entre VMs: %s' % (', '.join(dif) if dif else 'ninguna'))
    if saltados:
        print('\n  saltados (%d): %s' % (len(saltados), '; '.join('%s MOD%d: %s' % s for s in saltados)))
    print('tabla en', out)
    shutil.rmtree(work, ignore_errors=True)

if __name__ == '__main__':
    main()
