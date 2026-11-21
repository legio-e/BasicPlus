"""resistencia.py — la prueba de resistencia de días (V8, 15-nov), en las placas, por el cable serie.

Eduardo (15-nov): «La prueba de resistencia es para ver si salen problemas que en las tandas cortas que hacemos no se
ven. Si sale algún problema lo estudiamos y vemos que hay que hacer.» El programa, Resistencia.bp (al lado), corre solo
con el autorun y lo apunta todo en /data/resistencia/log.txt (y log.1). En una placa con pantalla, con `gui`, corre
ResistenciaGui.bp: la misma prueba, pintando en la pantalla (Resistencia.bp no importa Gui: a la Pico 2 no le cabe).
Este script lo prepara, lo mira y lo recoge:

    resistencia.py COMx fallos [gui]        compila, sube y lo ejecuta con «fallos»: un error de cada clase, que se
                                            tiene que atrapar y apuntar (para ver la placa antes de dejarla días)
    resistencia.py COMx empezar <carpeta> [gui]
                                            compila y sube; borra el log; pone el autorun; guarda el INFO en
                                            <carpeta>/info_inicio.json y reinicia: la prueba arranca sola
    resistencia.py COMx mirar [n]           sin pararla, las n últimas líneas del log (8 si no se dice). Sólo el log:
                                            con un programa en marcha, la placa contesta BUSY a casi todo (al INFO,
                                            también); sólo atiende HELLO, KILL, RESET, STAT y GET/LIST por trozos
    resistencia.py COMx acabar <carpeta>    la para, baja log.txt, log.1, el log de la placa (log_placa.txt) y el INFO
                                            (info_fin.json) a <carpeta>, y quita el autorun

Si una placa aparece colgada: `wire_serie.py COMx reset` (o `resetcable`, si el wire no contesta). El autorun vuelve a
lanzar el programa, y su línea ARRANQUE lleva el rastro: las marcas de la vida anterior, y la última es la fase en la
que estaba. Tras un fallo fatal (un pánico, un HardFault) la placa no lanza el autorun —a propósito: un programa que
falla al arrancar sería un bucle de reinicios—, así que el log se para; el motivo, en el log de la placa.

El log de la placa y el INFO pueden llevar el nombre de la red WiFi: antes de subir lo bajado al repo, hay que taparlo.

Con el python del ESP-IDF (trae pyserial); desde Git Bash, con MSYS_NO_PATHCONV=1 (las rutas de la placa empiezan por /).
"""
import importlib.util
import json
import os
import subprocess
import sys
import tempfile
import time

AQUI = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(AQUI))
spec = importlib.util.spec_from_file_location('wire_serie', os.path.join(REPO, 'bpgenvm-c', 'tools', 'wire_serie.py'))
ws = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ws)

CARPETA = '/app/Resistencia/'
DIR = '/data/resistencia/'
LOGS = ('log.1', 'log.txt')


def programa(gui):
    return 'ResistenciaGui' if gui else 'Resistencia'


def compila():
    """ResistenciaGui.bp, compilado desde la raíz del repo (allí está el BpVM.cfg) a un temporal; con él sale también
    Resistencia.mod. Devuelve la carpeta."""
    destino = tempfile.mkdtemp(prefix='resistencia')
    r = subprocess.run(['java', '-jar', os.path.join(REPO, 'toolchain', 'lexer-java', 'target', 'basicplus-frontend.jar'),
                        os.path.join(AQUI, 'ResistenciaGui.bp'), '--compile', destino, '--backend=mivm'],
                       cwd=REPO, capture_output=True)   # en bytes: Java escribe en la página de códigos de Windows
    if r.returncode != 0 or not all(os.path.exists(os.path.join(destino, programa(g) + '.mod')) for g in (0, 1)):
        sys.exit('no compila:\n' + (r.stdout[-800:] + r.stderr[-800:]).decode('utf-8', 'replace'))
    return destino


def info(w):
    w.send('INFO')
    return w.esperar('INFO_REPLY', 6)


def resumen(i):
    if not i:
        return 'sin INFO'
    s = int(i.get('uptimeMs', 0)) // 1000
    return '%s, encendida %dd %02d:%02d:%02d, %.1f C, último reinicio: %s' % (
        i.get('micro', '?'), s // 86400, s % 86400 // 3600, s % 3600 // 60, s % 60,
        int(i.get('tempMilliC', 0)) / 1000.0, i.get('resetReason', '?'))


def guarda(carpeta, nombre, datos):
    os.makedirs(carpeta, exist_ok=True)
    with open(os.path.join(carpeta, nombre), 'w', encoding='utf-8') as f:
        f.write(datos)


def baja(w, remoto):
    """El fichero entero, o None si no está. Por trozos (GET_PART), como el IDE: el cable lo comparte con el programa."""
    ident = w.send('GET', path=remoto, chunked=True)
    trozos, tope = {}, time.time() + 120
    while time.time() < tope:
        for m in ws_mensajes(w):
            if m.get('id') != ident:
                continue
            if m.get('type') == 'GET_PART':
                trozos[m['off']] = m['_datos']
            elif m.get('type') == 'GET_END':
                return b''.join(trozos[k] for k in sorted(trozos))
            else:
                return None
    return None


def ws_mensajes(w):
    """Lo que llegue por el puerto, en mensajes; a un GET_PART le pega sus bytes crudos (m['_datos'])."""
    c = w.s.read(w.s.in_waiting or 1)
    if c:
        w.buf += c
    out = []
    while b'\n' in w.buf:
        i = w.buf.index(b'\n')
        ln = w.buf[:i].strip()
        if not ln.startswith(b'{'):
            w.buf = w.buf[i + 1:]
            continue
        try:
            m = json.loads(ln.decode('utf-8', 'replace'))
        except ValueError:
            w.buf = w.buf[i + 1:]
            continue
        n = int(m.get('bulk', 0) or 0)
        if len(w.buf) < i + 1 + n:
            break                                   # faltan bytes del trozo: el resto, en la vuelta siguiente
        m['_datos'] = w.buf[i + 1:i + 1 + n]
        w.buf = w.buf[i + 1 + n:]
        out.append(m)
    return out


def sube(w, gui):
    """Sube Resistencia.mod y, con gui, ResistenciaGui.mod; devuelve la ruta en la placa del que se ejecuta."""
    carpeta = compila()
    for g in sorted({0, gui}):
        ws.cmd_puts(w, [os.path.join(carpeta, programa(g) + '.mod'), CARPETA + programa(g) + '.mod'])
    return CARPETA + programa(gui) + '.mod'


def fallos(w, gui):
    mod = sube(w, gui)
    w.send('RUN', path=mod, arg='fallos')
    if w.esperar('RUN_REPLY', 10) is None:
        sys.exit('el RUN no arrancó: %s' % [(m.get('code'), m.get('message')) for m in w.otros][:2])
    tope = time.time() + 240
    while time.time() < tope:
        for m in w.lines(0.3):
            if m.get('type') == 'OUTPUT':
                print(m.get('data', ''), end='')
            elif m.get('type') == 'EXITED':
                print('[EXITED %s exit=%s] %s' % (m.get('status'), m.get('exitCode'), m.get('errorMessage') or ''))
                return
    print('[sin EXITED en 240 s]')


def empezar(w, carpeta, gui):
    w.send('KILL')
    w.lines(1.5)                                    # por si corría algo
    mod = sube(w, gui)
    for f in LOGS:
        w.send('DEL', path=DIR + f)
        w.esperar('DEL_REPLY', 10)                  # si no está, NOT_FOUND: da igual
    auto = (mod + '\n').encode()
    w.id += 1
    w.s.write((json.dumps({'type': 'PUT', 'id': w.id, 'path': '/sys/auto.txt', 'bulk': len(auto)}) + '\n').encode()
              + auto)
    if w.esperar('PUT_REPLY', 10) is None:
        sys.exit('no se pudo escribir /sys/auto.txt: %s' % [m.get('message') for m in w.otros][:2])
    i = info(w)
    guarda(carpeta, 'info_inicio.json', json.dumps(i, indent=2, ensure_ascii=False))
    print('antes de empezar:', resumen(i))
    ws.cmd_reset(w, [])
    print('autorun puesto (%s); la placa se reinicia y la prueba arranca sola' % mod)


def mirar(w, n):
    log = baja(w, DIR + 'log.txt')
    if log is None:
        print('(sin log.txt)')
        return
    for linea in log.decode('utf-8', 'replace').splitlines()[-n:]:
        print('  ' + linea)


def acabar(w, carpeta):
    w.send('KILL')
    w.esperar('EXITED', 12)
    for f in LOGS:
        datos = baja(w, DIR + f)
        print('%s: %s' % (f, 'no está' if datos is None else '%d B' % len(datos)))
        if datos is not None:
            guarda(carpeta, f, datos.decode('utf-8', 'replace'))
    w.send('LOG_DUMP')
    r = w.esperar('LOG_DUMP_REPLY', 10)
    guarda(carpeta, 'log_placa.txt', r.get('text', '') if r else '(sin respuesta)')
    i = info(w)
    guarda(carpeta, 'info_fin.json', json.dumps(i, indent=2, ensure_ascii=False))
    print('al acabar:', resumen(i))
    w.send('DEL', path='/sys/auto.txt')
    print('autorun quitado' if w.esperar('DEL_REPLY', 10) else 'el autorun no se pudo quitar: %s' %
          [m.get('message') for m in w.otros][:1])
    print('todo en', carpeta)


def main():
    if len(sys.argv) < 3 or sys.argv[2] not in ('fallos', 'empezar', 'mirar', 'acabar'):
        sys.exit(__doc__)
    puerto, orden = sys.argv[1], sys.argv[2]
    gui = 1 if sys.argv[-1] == 'gui' else 0
    resto = [a for a in sys.argv[3:] if a != 'gui']
    w = ws.Wire(puerto)
    if w.hola() is None:
        sys.exit('%s: la placa no contesta' % puerto)
    try:
        if orden == 'fallos':
            fallos(w, gui)
        elif orden == 'mirar':
            mirar(w, int(resto[0]) if resto else 8)
        elif not resto:
            sys.exit('%s: falta la carpeta' % orden)
        elif orden == 'empezar':
            empezar(w, resto[0], gui)
        else:
            acabar(w, resto[0])
    finally:
        w.close()


if __name__ == '__main__':
    main()
