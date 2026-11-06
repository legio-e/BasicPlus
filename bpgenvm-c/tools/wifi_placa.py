"""wifi_placa.py — escribe /sys/wifi.json en una placa, por el cable (V8/H3).

La red y su clave van de aqui a la placa, y a ningun sitio mas: la clave no se escribe en la pantalla ni en un fichero del
PC, y por eso esta herramienta no enseña lo que escribe. En la placa va en claro: cualquier programa de la placa, o quien
tenga el cable, puede leerla (Eduardo, 19-oct: «la seguridad, de momento no»).

    python tools/wifi_placa.py COM15 --ssid MiRed                 pide la clave sin enseñarla
    python tools/wifi_placa.py COM15 --desde red_privada.h        RED_SSID y RED_CLAVE, de un .h

    --pais ES       dos letras: los canales y la potencia de ese pais
    --arranca       "autoStart": true: se enciende al arrancar la placa (o `wifi=1` en el Entorno)
    --apagado N     "autoOff": segundos sin actividad para apagarse; 0, no se apaga sola (por defecto, 600)

La placa lo lee al encender la WiFi: para que entre en una placa ya encendida, hay que reiniciarla.
Usa el python del ESP-IDF, que trae pyserial.
"""
import argparse
import getpass
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from wire_serie import Wire

REMOTO = "/sys/wifi.json"


def de_cabecera(ruta):
    texto = open(ruta, encoding="utf-8").read()
    valores = {}
    for nombre in ("RED_SSID", "RED_CLAVE"):
        m = re.search(r'#define\s+' + nombre + r'\s+"((?:[^"\\]|\\.)*)"', texto)
        if not m:
            sys.exit("%s no define %s" % (ruta, nombre))
        valores[nombre] = m.group(1).replace('\\"', '"').replace('\\\\', '\\')
    return valores["RED_SSID"], valores["RED_CLAVE"]


def main():
    a = argparse.ArgumentParser(description="Escribe /sys/wifi.json en una placa, por el cable.")
    a.add_argument("puerto")
    a.add_argument("--ssid")
    a.add_argument("--desde", help="un .h con RED_SSID y RED_CLAVE")
    a.add_argument("--pais")
    a.add_argument("--arranca", action="store_true")
    a.add_argument("--apagado", type=int)
    o = a.parse_args()
    if bool(o.ssid) == bool(o.desde):
        sys.exit("hace falta --ssid o --desde, uno de los dos")
    ssid, clave = de_cabecera(o.desde) if o.desde else (o.ssid, getpass.getpass("clave de %s: " % o.ssid))
    if not 1 <= len(ssid.encode("utf-8")) <= 32:
        sys.exit("el ssid son de 1 a 32 bytes")
    if len(clave.encode("utf-8")) > 64:
        sys.exit("la clave son 64 bytes como mucho")

    cfg = {"ssid": ssid, "password": clave}
    if o.pais:
        cfg["country"] = o.pais
    if o.arranca:
        cfg["autoStart"] = True
    if o.apagado is not None:
        cfg["autoOff"] = o.apagado
    datos = json.dumps(cfg, ensure_ascii=False).encode("utf-8")

    w = Wire(o.puerto)
    try:
        w.hola()
        w.id += 1
        cabecera = json.dumps({"type": "PUT", "id": w.id, "path": REMOTO, "bulk": len(datos)})
        w.s.write((cabecera + "\n").encode() + datos)      # la cabecera y los bytes, crudos detras
        r = w.esperar("PUT_REPLY", 20)
    finally:
        w.close()
    if r is None:
        sys.exit("la placa no ha aceptado el fichero")
    print("%s, en la placa de %s: la red '%s'%s%s" % (
        REMOTO, o.puerto, ssid, ", se enciende al arrancar" if o.arranca else "",
        "" if o.apagado is None else ", autoOff %d" % o.apagado))


if __name__ == "__main__":
    main()
