#!/usr/bin/env python3
# La Pico carga y ejecuta MOD9: una conexion, el mismo programa (Lib+Ext de mezcla) en MOD8 y en MOD9,
# stdout + EXITED + la linea «enlace:» del log (cronometro de V0/V9 en ciclos/us de la placa).
import sys, time, json, os
sys.path.insert(0, 'C:/lenguajes/pm/bpgenvm-c/tools'); import wire_serie
S = 'C:/Users/Eduardo/AppData/Local/Temp/claude/C--lenguajes-pm-miVM/576fc834-e260-4649-ba6c-131c549812a9/scratchpad'
ESPERADO = "hija/k3/3/6\n9\n5\nhija/k3/3/6\n9\nfin"
w = wire_serie.Wire(sys.argv[1] if len(sys.argv) > 1 else 'COM22')
w.send('HELLO', protoVersion=1); h = w.esperar('HELLO_REPLY', 5)
print('HELLO:', json.dumps({k: h.get(k) for k in ('serverName', 'version', 'build', 'caps') if k in h}, ensure_ascii=False) if h else 'sin respuesta')
w.send('INFO'); i = w.esperar('INFO_REPLY', 5)
print('INFO: uptimeMs=%s reset=%s' % (i.get('uptimeMs'), i.get('resetReason')) if i else 'sin INFO')

def put(local, remoto):
    data = open(local, 'rb').read()
    w.id += 1
    w.s.write((json.dumps({'type': 'PUT', 'id': w.id, 'path': remoto, 'bulk': len(data)}) + '\n').encode() + data)
    r = w.esperar('PUT_REPLY', 20)
    print('  put %-28s -> %-16s %s' % (os.path.basename(local), remoto, 'ok %d B' % len(data) if r else 'FALLO ' + str([m.get('message') for m in w.otros][:2])))
    return r is not None
def run(mod, secs=30):
    w.send('RUN', path=mod)
    if w.esperar('RUN_REPLY', 10) is None:
        print('  el RUN no arranco:', [(m.get('code'), m.get('message')) for m in w.otros][:2]); return None, None
    out, exited, t0 = '', None, time.time()
    while time.time() - t0 < secs and exited is None:
        for m in w.lines(0.3):
            if m.get('type') == 'OUTPUT': out += m.get('data', '')
            elif m.get('type') == 'EXITED': exited = m
    return out, exited
def log_enlace():
    w.send('LOG_DUMP'); r = w.esperar('LOG_DUMP_REPLY', 10)
    t = (r or {}).get('text', '')
    return [ln for ln in t.splitlines() if 'enlace' in ln or 'ERROR DE LINK' in ln or 'tapa' in ln][-4:]

for f in (8, 9):
    print('\n== MOD%d ==' % f)
    d = S + '/pico_m%d' % f
    if f == 9: put(S + '/std9/Core.mod', '/app/Core.mod')          # el Core MOD9 al lado (tapa al de /lib, con aviso)
    put(d + '/lib_igual_g7/Lib.mod', '/app/Lib.mod'); put(d + '/app_Ext_v1/Ext.mod', '/app/Ext.mod')
    out, ex = run('/app/Ext.mod')
    salida = '\n'.join(ln.rstrip() for ln in (out or '').splitlines() if ln.strip())
    print('  stdout:', repr(salida))
    print('  EXITED:', json.dumps({k: ex.get(k) for k in ('status', 'exitCode', 'elapsedMs', 'errorMessage')}) if ex else 'NO LLEGO')
    print('  ' + ('ESPERADO OK' if salida == ESPERADO else 'DISTINTO DE LO ESPERADO'))
    for ln in log_enlace(): print('  log:', ln)
print('\nbasura:', w.basura[-3:] if w.basura else 'ninguna')
