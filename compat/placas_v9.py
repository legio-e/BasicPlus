#!/usr/bin/env python3
# compat/placas_v9.py -- V7/H2 (#523), V8/V9 en placa (17-sep): MOD8 contra MOD9 en la Pico y en la DK2.
# Una conexion por placa; el mismo programa en los dos formatos; la stdlib que haga falta va a /app por
# `puts` (#294) construida FRESCA de la fuente en el formato de cada pasada (los .mod versionados de
# bpstdlib/ son MOD7 y no casan con el Core embebido: medido el 17-sep, «no exporta Core.__cls_m_List#add»).
# De cada RUN: el stdout, el EXITED y las lineas NUEVAS del log de la placa («enlace: ... us», «enlace
# tardio: ...», el aviso «tapa al de /lib»). El log de la placa se enciende (env log=1 + RESET) si estaba
# apagado (#423) y se deja como estaba. Al final se borra lo subido: un Core MOD9 en /app taparia al de /lib.
#   python compat/placas_v9.py [--pico=COM22] [--dk2=COM12] [--work=DIR] [--embebida]
# --embebida (V11, 17-sep): la imagen ya lleva la stdlib MOD9 en /lib. No se sube Core: CoreRefs MOD9 tiene que
# enlazar contra el de /lib (sin el aviso «tapa»), GuiTableDemo MOD9 en la DK2 solo con Gui/Json/Collections/Str,
# y un CoreRefs MOD8 tiene que RECHAZARSE nombrando el simbolo (hacia delante no, D10).
# Pico: samples/CoreRefs.bp (refs a Core: List, excepciones, una clase que extiende Exception).
# DK2:  CoreRefs y samples/GuiTableDemo.bp (Gui+Json+Collections+Str a /app; Gui.run() se mata a los 6 s).
import io, os, re, sys, time, json, shutil, tempfile, datetime
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE); sys.path.insert(0, ROOT + '/bpgenvm-c/tools')
import v9_medidas as v9m, wire_serie

AVISO_SOMBRA = re.compile(r"^aviso: '\w+' cargado desde /app, tapa al de /lib$")
def filt(t, sin_aviso=True):
    return '\n'.join(ln.rstrip() for ln in t.splitlines() if ln.strip() and not re.search(r'INICIANDO|FIN DE|heapStart', ln) and not (sin_aviso and AVISO_SOMBRA.match(ln.strip())))

class Placa:
    def __init__(self, nombre, puerto):
        self.nombre, self.puerto = nombre, puerto
        self.subidos = []; self.log_toque = False; self.log_era = None
        self.conectar()
        self.w.send('ENV_LS'); r = self.w.esperar('ENV_LS_REPLY', 10) or {}
        env = {e.get('key'): str(e.get('value')) for e in (r.get('entries') or r.get('vars') or [])}
        if env.get('log', '0').lower() not in ('1', 'true', 'on'):
            self.log_toque = True; self.log_era = env.get('log')
            self.w.send('ENV_SET', key='log', value='1'); self.w.esperar('ENV_SET_REPLY', 10)
            print('  [%s] log apagado (#423): env log=1 y RESET' % nombre)
            self.w.send('RESET'); self.w.esperar('RESET_REPLY', 5); self.w.s.close(); time.sleep(4)
            self.conectar()
    def conectar(self):
        for intento in range(6):
            try:
                self.w = wire_serie.Wire(self.puerto); break
            except Exception as e:
                if intento == 5: raise
                time.sleep(2)
        self.w.send('HELLO', protoVersion=1); h = self.w.esperar('HELLO_REPLY', 5) or {}
        self.w.send('INFO'); i = self.w.esperar('INFO_REPLY', 5) or {}
        print('[%s] %s | %s | build %s | uptime %s ms' % (self.nombre, i.get('boardName', '?'), h.get('serverName', '?'), h.get('build') or i.get('serverBuild', '?'), i.get('uptimeMs')))
    def put(self, local, remoto):
        data = open(local, 'rb').read()
        if len(data) > 12000:
            wire_serie.cmd_puts(self.w, [local, remoto])
        else:
            self.w.id += 1
            self.w.s.write((json.dumps({'type': 'PUT', 'id': self.w.id, 'path': remoto, 'bulk': len(data)}) + '\n').encode() + data)
            if self.w.esperar('PUT_REPLY', 20) is None:
                raise SystemExit('[%s] PUT %s fallo: %s' % (self.nombre, remoto, [m.get('message') for m in self.w.otros][-2:]))
        if remoto not in self.subidos: self.subidos.append(remoto)
        print('  %-22s <- %-18s %6d B' % (remoto, os.path.basename(local), len(data)))
    def log(self):
        self.w.send('LOG_DUMP'); r = self.w.esperar('LOG_DUMP_REPLY', 15)
        return ((r or {}).get('text', '') or '').splitlines()
    def run(self, mod, secs, matar=False):
        antes = set(self.log())   # el log es un ANILLO: lo nuevo se distingue por contenido (llevan sello de tiempo), no por posicion
        self.w.send('RUN', path=mod)
        r = self.w.esperar('RUN_REPLY', 10)
        if r is None: return None, {'status': 'NO ARRANCO', 'errorMessage': str([(m.get('code'), m.get('message')) for m in self.w.otros][-2:])}, []
        out, exited, t0 = '', None, time.time()
        while time.time() - t0 < secs and exited is None:
            for m in self.w.lines(0.3):
                if m.get('type') == 'OUTPUT': out += m.get('data', '')
                elif m.get('type') == 'EXITED': exited = m
        if exited is None and matar:
            self.w.send('KILL', session=r.get('session', 0))
            for m in self.w.lines(3):
                if m.get('type') == 'EXITED': exited = m
        nuevas = [ln for ln in self.log() if ln not in antes]
        return out, exited, [ln.strip() for ln in nuevas if re.search(r'enlace|LINK|tapa al|dep ', ln)][-5:]
    def cerrar(self):
        for p in self.subidos:
            self.w.send('DEL', path=p); self.w.esperar('DEL_REPLY', 5)
        print('  [%s] borrados de la placa: %s' % (self.nombre, ', '.join(self.subidos))); self.subidos = []
        if self.log_toque:
            if not self.log_era: self.w.send('ENV_DEL', key='log'); self.w.esperar('ENV_DEL_REPLY', 5)
            else: self.w.send('ENV_SET', key='log', value=self.log_era); self.w.esperar('ENV_SET_REPLY', 5)
            print('  [%s] env log restaurado (%s); surte efecto al reiniciar' % (self.nombre, self.log_era or 'borrado'))
        self.w.s.close()

def caso(placa, etiqueta, mods, raiz, esperado, secs, matar):
    print('\n== %s: %s ==' % (placa.nombre, etiqueta))
    for local, remoto in mods: placa.put(local, remoto)
    out, ex, log = placa.run(raiz, secs, matar)
    salida = filt(out or ''); sombra = any(AVISO_SOMBRA.match(ln.strip()) for ln in (out or '').splitlines())
    ex = ex or {}
    print('  EXITED: %s' % json.dumps({k: ex.get(k) for k in ('status', 'exitCode', 'elapsedMs', 'errorMessage') if ex.get(k) is not None}, ensure_ascii=False))
    if esperado is None: veredicto = 'sin esperado fijo'
    else: veredicto = 'ESPERADO OK' if salida == esperado else 'DISTINTO DE LO ESPERADO'
    print('  stdout: %s -> %s%s' % (repr(salida[:150]), veredicto, ' (+ el aviso «tapa al de /lib»)' if sombra else ''))
    if esperado is not None and veredicto != 'ESPERADO OK': print('  esperado: %r' % esperado[:150])
    for ln in log: print('  log: ' + ln)
    return {'placa': placa.nombre, 'caso': etiqueta, 'status': ex.get('status'), 'elapsedMs': ex.get('elapsedMs'), 'veredicto': veredicto, 'log': ' | '.join(log), 'stdout': salida}

def main():
    pico = dk2 = None; work = None; embebida = False
    for a in sys.argv[1:]:
        if a.startswith('--pico='): pico = a[7:]
        elif a.startswith('--dk2='): dk2 = a[6:]
        elif a.startswith('--work='): work = a[7:]
        elif a == '--embebida': embebida = True
        else: raise SystemExit('argumento desconocido: ' + a)
    if not pico and not dk2: raise SystemExit('dame --pico=COMx y/o --dk2=COMy')
    work = work or tempfile.mkdtemp(prefix='placas9_')
    std = {8: v9m.stdlib_de(work, 8), 9: v9m.stdlib_de(work, 9)}
    print('trabajo en', work, '| stdlib fresca en MOD8 y MOD9 (%d modulos)' % len([f for f in os.listdir(std[9]) if f.endswith('.mod')]))
    prog = {}
    for item in ('samples/CoreRefs.bp', 'samples/GuiTableDemo.bp'):
        for f in (8, 9):
            d, raiz = v9m.compilar(work, item, f, std[f])
            if d is None: raise SystemExit('%s no compila en MOD%d: %s' % (item, f, raiz))
            prog[(item, f)] = (d, raiz)
    d, raiz = prog[('samples/CoreRefs.bp', 9)]
    esp_core = filt(v9m.sh(['java', '-Djava.awt.headless=true', '-jar', v9m.JVM, os.path.basename(raiz)], d, 30)[1])
    print('esperado CoreRefs (miVM host, MOD9):', repr(esp_core))
    def mod(item, f, nombre): return os.path.join(prog[(item, f)][0], nombre + '.mod')
    def libs(f, nombres): return [(os.path.join(std[f], m + '.mod'), '/app/%s.mod' % m) for m in nombres]
    filas = []
    try:
        if embebida:
            for nombre, puerto in (('Pico', pico), ('DK2', dk2)):
                if not puerto: continue
                b = Placa(nombre, puerto)
                filas.append(caso(b, 'CoreRefs MOD9 contra el Core MOD9 EMBEBIDO (/lib)', [(mod('samples/CoreRefs.bp', 9, 'CoreRefs'), '/app/CoreRefs.mod')], '/app/CoreRefs.mod', esp_core, 30, False))
                filas.append(caso(b, 'CoreRefs MOD8 contra el Core MOD9 embebido (hacia delante NO: rechazo con nombre)', [(mod('samples/CoreRefs.bp', 8, 'CoreRefs'), '/app/CoreRefs.mod')], '/app/CoreRefs.mod', None, 30, False))
                if nombre == 'DK2':
                    filas.append(caso(b, 'GuiTableDemo MOD9 (Str/Collections/Json/Gui MOD9 en /app; Core de /lib)', libs(9, ['Str', 'Collections', 'Json', 'Gui']) + [(mod('samples/GuiTableDemo.bp', 9, 'GuiTableDemo'), '/app/GuiTableDemo.mod')], '/app/GuiTableDemo.mod', None, 6, True))
                b.cerrar()
            pico = dk2 = None
        if pico:
            p = Placa('Pico', pico)
            filas.append(caso(p, 'CoreRefs MOD8 (Core embebido de /lib)', [(mod('samples/CoreRefs.bp', 8, 'CoreRefs'), '/app/CoreRefs.mod')], '/app/CoreRefs.mod', esp_core, 30, False))
            filas.append(caso(p, 'CoreRefs MOD9 (Core MOD9 en /app por puts)', libs(9, ['Core']) + [(mod('samples/CoreRefs.bp', 9, 'CoreRefs'), '/app/CoreRefs.mod')], '/app/CoreRefs.mod', esp_core, 30, False))
            p.cerrar()
        if dk2:
            q = Placa('DK2', dk2)
            filas.append(caso(q, 'CoreRefs MOD8 (Core embebido de /lib)', [(mod('samples/CoreRefs.bp', 8, 'CoreRefs'), '/app/CoreRefs.mod')], '/app/CoreRefs.mod', esp_core, 30, False))
            # el Core embebido de la imagen es mas viejo que la fuente (no exporta List#add$LObject; que pide la Collections de hoy): la linea base MOD8 lleva su Core fresco a /app
            filas.append(caso(q, 'GuiTableDemo MOD8 (Core/Str/Collections/Json/Gui MOD8 frescos en /app)', libs(8, ['Core', 'Str', 'Collections', 'Json', 'Gui']) + [(mod('samples/GuiTableDemo.bp', 8, 'GuiTableDemo'), '/app/GuiTableDemo.mod')], '/app/GuiTableDemo.mod', None, 6, True))
            filas.append(caso(q, 'CoreRefs MOD9 (Core MOD9 en /app)', libs(9, ['Core']) + [(mod('samples/CoreRefs.bp', 9, 'CoreRefs'), '/app/CoreRefs.mod')], '/app/CoreRefs.mod', esp_core, 30, False))
            filas.append(caso(q, 'GuiTableDemo MOD9 (Core/Str/Collections/Json/Gui MOD9 en /app)', libs(9, ['Core', 'Str', 'Collections', 'Json', 'Gui']) + [(mod('samples/GuiTableDemo.bp', 9, 'GuiTableDemo'), '/app/GuiTableDemo.mod')], '/app/GuiTableDemo.mod', None, 6, True))
            q.cerrar()
    finally:
        g = [f for f in filas if f['caso'].startswith('GuiTableDemo')]
        if len(g) == 2:
            igual = g[0]['stdout'] == g[1]['stdout'] and bool(g[0]['stdout'])
            for f in g: f['veredicto'] = 'MOD8 == MOD9 (%d lineas)' % len(g[0]['stdout'].splitlines()) if igual else 'MOD8 != MOD9 (o vacio)'
            print('\nGuiTableDemo en la DK2: %s' % g[0]['veredicto'])
            if not igual:
                import difflib
                print('\n'.join(list(difflib.unified_diff(g[0]['stdout'].splitlines(), g[1]['stdout'].splitlines(), 'MOD8', 'MOD9', lineterm=''))[:20]))
        out = os.path.join(ROOT, 'compat', 'informes', 'placas_v9_%s.tsv' % datetime.date.today().isoformat())
        io.open(out, 'w', encoding='utf-8', newline='\n').write('placa\tcaso\tstatus\telapsedMs\tveredicto\tlog\n' + '\n'.join('\t'.join(str(f[k]) for k in ('placa', 'caso', 'status', 'elapsedMs', 'veredicto', 'log')) for f in filas) + '\n')
        print('\ninforme en', out)
    shutil.rmtree(work, ignore_errors=True)

if __name__ == '__main__':
    main()
