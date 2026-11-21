#!/usr/bin/env python3
# seo_paginas.py — 21-nov-2026 (#851): la cabecera de cada página de la web (docs/*.html y docs/en/*.html) para el
# buscador, y el mapa del sitio (docs/sitemap.xml).
#
# Lo que pone en el <head>, detrás del <title> y entre dos marcas (así se puede repetir: la segunda vez sustituye):
#   - la descripción (lo que Google enseña bajo el título);
#   - la dirección canónica de la página;
#   - si la página está en los dos idiomas, las dos (hreflang es/en, y en para quien no lee ninguno de los dos);
#   - la tarjeta para cuando se comparte un enlace (Open Graph, y la de X).
# Y el título, sólo en las páginas que lo cambian en la tabla (las dos portadas).
# El texto de las páginas no lo toca: fuera del <head>, cada fichero queda byte a byte como estaba.
#
# Quien lo necesite tiene que encontrarlo (Eduardo, 21-nov): las descripciones dicen lo que buscaría esa persona
# —BASIC, su placa, microcontrolador— y lo que hace único al proyecto: el lenguaje, la interfaz gráfica y la base de
# datos, juntos.
#
#   python -I -X utf8 scripts/seo_paginas.py               escribe las páginas y docs/sitemap.xml
#   python -I -X utf8 scripts/seo_paginas.py --comprobar   sólo dice qué cambiaría, y qué página no está en la tabla
#
# Una página nueva en docs/: su fila en PAGINAS (si no, --comprobar la nombra y no se escribe nada).
import html
import os
import re
import sys

RAIZ = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
DOCS = os.path.join(RAIZ, "docs")
WEB = "https://micro32-suite.github.io/BasicPlus/"
IMAGEN = WEB + "img/BpIde_VM_GuiColorDemo.png"
SITIO = "BasicPlus · Micro32 Suite"
MARCA_INI = "<!-- seo: lo escribe scripts/seo_paginas.py; no editar a mano -->"
MARCA_FIN = "<!-- /seo -->"

# Cada página: su ruta dentro de docs/ y, si cambia, su título; y la descripción. Las que están en los dos
# idiomas (X.html y en/X.html) se enlazan solas por hreflang.
PAGINAS = {
    "index.html": {
        "titulo": "BasicPlus — BASIC orientado a objetos para microcontroladores de 32 bits",
        "desc": "Lenguaje, interfaz gráfica y base de datos, juntos, para microcontroladores: depura en el PC y el "
                "mismo programa corre en Raspberry Pi Pico 2, ESP32 y STM32."},
    "en/index.html": {
        "titulo": "BasicPlus — Object-oriented BASIC for 32-bit microcontrollers",
        "desc": "Language, graphical interface and database, together, for microcontrollers: debug on the PC and "
                "the same program runs on Raspberry Pi Pico 2, ESP32 and STM32."},
    "manual.html": {
        "desc": "Manual de BasicPlus, un BASIC orientado a objetos para microcontroladores: tipos, clases, "
                "interfaces y genéricos, excepciones, módulos, hilos y funciones nativas."},
    "en/manual.html": {
        "desc": "Manual of BasicPlus, an object-oriented BASIC for microcontrollers: types, classes, interfaces "
                "and generics, exceptions, modules, threads and native functions."},
    "referencia.html": {
        "desc": "Referencia de BasicPlus: la biblioteca estándar —GPIO, I2C, SPI, UART, PWM, ADC, ficheros, JSON, "
                "red, WiFi y Bluetooth—, la línea de comandos y los ficheros que genera."},
    "en/referencia.html": {
        "desc": "BasicPlus reference: the standard library — GPIO, I2C, SPI, UART, PWM, ADC, files, JSON, network, "
                "WiFi and Bluetooth —, the command line and the files it produces."},
    "gui.html": {
        "desc": "Interfaz gráfica en microcontroladores con BasicPlus: el módulo Gui sobre LVGL, widgets, pantallas "
                "diseñadas en JSON y táctil; el mismo programa en el PC y en la placa."},
    "en/gui.html": {
        "desc": "Graphical interfaces on microcontrollers with BasicPlus: the Gui module over LVGL, widgets, screens "
                "designed in JSON and touch; the same program on the PC and on the board."},
    "basedatos.html": {
        "desc": "SQLite y su ORM en microcontroladores con BasicPlus: el mismo motor, el mismo fichero .db y el "
                "mismo código en el PC, la Metro RP2350, la ESP32-P4 y la STM32 Discovery."},
    "en/basedatos.html": {
        "desc": "SQLite and its ORM on microcontrollers with BasicPlus: the same engine, the same .db file and the "
                "same code on the PC, the Metro RP2350, the ESP32-P4 and the STM32 Discovery."},
    "guia-ide.html": {
        "desc": "Guía del IDE de BasicPlus: compilar y ejecutar en el PC o en la placa —por cable o por WiFi—, el "
                "explorador de la placa, la consola, el depurador y el micro simulado."},
    "en/guia-ide.html": {
        "desc": "BasicPlus IDE guide: compiling and running on the PC or on the board — by cable or over WiFi —, "
                "the board explorer, the console, the debugger and the simulated micro."},
    "jc.html": {
        "desc": "JC, BasicPlus con la sintaxis de Java y C#, para microcontroladores: cómo se escribe cada cosa, qué "
                "es en BasicPlus y cómo se traduce un programa de uno a otro."},
    "en/jc.html": {
        "desc": "JC, BasicPlus with the syntax of Java and C#, for microcontrollers: how each thing is written, "
                "what it is in BasicPlus and how a program is translated from one to the other."},
    "pruebas.html": {
        "desc": "Cómo se prueba BasicPlus: la salida, byte a byte la misma en la VM Java y en la VM C; las pruebas "
                "en las once placas; y una prueba de 48 horas sin un error ni un reinicio."},
    "en/pruebas.html": {
        "desc": "How BasicPlus is tested: the output, byte-identical on the Java VM and on the C VM; the tests on "
                "all eleven boards; and a 48-hour run without a single error or restart."},
    "creditos.html": {
        "desc": "Créditos de BasicPlus: el trabajo de otros en el que se apoya —sistemas de ficheros, bibliotecas "
                "gráficas, un sistema operativo en tiempo real, los SDK y las herramientas—."},
    "en/creditos.html": {
        "desc": "BasicPlus credits and acknowledgements: the work of others it rests on — filesystems, graphics "
                "libraries, a real-time operating system, the manufacturers' SDKs and the tools."},
    "bp-desde-dentro.html": {
        "desc": "Cómo está hecho BasicPlus por dentro: el compilador, el bytecode, las dos máquinas virtuales "
                "(Java y C), el GC, el código nativo (AOT) y el firmware de las placas."},
    "opcodes.html": {
        "desc": "Catálogo de los opcodes de la máquina virtual de BasicPlus: qué hace cada instrucción del "
                "bytecode."},
    "cheatsheet.html": {
        "desc": "BasicPlus on one page: a small, statically typed language for microcontrollers; one bytecode runs "
                "on the PC and on RP2350, ESP32 and STM32 boards."},
}


def url_de(ruta):
    """La dirección pública de una página: la portada de cada idioma, sin index.html."""
    if ruta == "index.html":
        return WEB
    if ruta == "en/index.html":
        return WEB + "en/"
    return WEB + ruta


def pareja(ruta):
    """(es, en) si la página está en los dos idiomas; None si sólo en uno."""
    es = ruta[3:] if ruta.startswith("en/") else ruta
    en = "en/" + es
    if es in PAGINAS and en in PAGINAS and os.path.exists(os.path.join(DOCS, es)) \
            and os.path.exists(os.path.join(DOCS, en)):
        return es, en
    return None


def idioma(texto):
    m = re.search(r'<html[^>]*\blang="([a-z]+)', texto)
    return m.group(1) if m else "es"


def bloque(ruta, texto, nl):
    d = PAGINAS[ruta]
    titulo = d.get("titulo") or html.unescape(re.search(r"<title>(.*?)</title>", texto, re.S).group(1)).strip()
    a = lambda s: html.escape(s, quote=True)
    lang = idioma(texto)
    lineas = [MARCA_INI,
              '<meta name="description" content="%s">' % a(d["desc"]),
              '<link rel="canonical" href="%s">' % url_de(ruta)]
    p = pareja(ruta)
    if p:
        lineas += ['<link rel="alternate" hreflang="es" href="%s">' % url_de(p[0]),
                   '<link rel="alternate" hreflang="en" href="%s">' % url_de(p[1]),
                   '<link rel="alternate" hreflang="x-default" href="%s">' % url_de(p[1])]
    lineas += ['<meta property="og:type" content="website">',
               '<meta property="og:site_name" content="%s">' % a(SITIO),
               '<meta property="og:title" content="%s">' % a(titulo),
               '<meta property="og:description" content="%s">' % a(d["desc"]),
               '<meta property="og:url" content="%s">' % url_de(ruta),
               '<meta property="og:image" content="%s">' % IMAGEN,
               '<meta property="og:locale" content="%s">' % ("en_US" if lang == "en" else "es_ES"),
               '<meta name="twitter:card" content="summary_large_image">',
               MARCA_FIN]
    return nl.join(lineas)


def nueva(ruta, original):
    """El fichero con su cabecera puesta (bytes), o una excepción si no se sabe dónde ponerla."""
    texto = original.decode("utf-8")
    # El bloque de antes, si lo hay, fuera (con su salto de línea delante).
    texto = re.sub(r"\r?\n?" + re.escape(MARCA_INI) + r".*?" + re.escape(MARCA_FIN), "", texto, flags=re.S)
    m = re.search(r"<title>.*?</title>[^\r\n]*(\r?\n)", texto, re.S)
    if not m:
        raise ValueError("sin <title> seguido de un fin de línea")
    nl = m.group(1)
    if "titulo" in PAGINAS[ruta]:
        texto = texto[:m.start()] + re.sub(r"<title>.*?</title>", "<title>%s</title>"
                                           % html.escape(PAGINAS[ruta]["titulo"], quote=False),
                                           texto[m.start():m.end()], count=1, flags=re.S) + texto[m.end():]
        m = re.search(r"<title>.*?</title>[^\r\n]*(\r?\n)", texto, re.S)
    texto = texto[:m.end()] + bloque(ruta, texto, nl) + nl + texto[m.end():]
    return texto.encode("utf-8")


def sitemap():
    urls = [url_de(r) for r in sorted(PAGINAS) if os.path.exists(os.path.join(DOCS, r))]
    cuerpo = "\n".join("  <url><loc>%s</loc></url>" % u for u in urls)
    return ('<?xml version="1.0" encoding="UTF-8"?>\n'
            '<!-- lo escribe scripts/seo_paginas.py; no editar a mano -->\n'
            '<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n%s\n</urlset>\n' % cuerpo).encode("utf-8")


def main():
    comprobar = "--comprobar" in sys.argv[1:]
    en_disco = sorted(os.path.relpath(p, DOCS).replace(os.sep, "/")
                      for d in (DOCS, os.path.join(DOCS, "en"))
                      for p in (os.path.join(d, f) for f in os.listdir(d)) if p.endswith(".html"))
    sin_fila = [r for r in en_disco if r not in PAGINAS]
    sin_pagina = [r for r in PAGINAS if r not in en_disco]
    for r in sin_fila:
        print("SIN FILA EN LA TABLA: docs/%s (ponle su descripción en PAGINAS)" % r)
    for r in sin_pagina:
        print("FILA SIN PÁGINA: %s (¿se borró o se renombró?)" % r)
    if sin_fila or sin_pagina:
        print("no se escribe nada")
        return 1
    cambian = 0
    for r in en_disco:
        ruta = os.path.join(DOCS, r)
        antes = open(ruta, "rb").read()
        despues = nueva(r, antes)
        if despues != antes:
            cambian += 1
            print("%s docs/%s" % ("cambiaría" if comprobar else "escrita  ", r))
            if not comprobar:
                open(ruta, "wb").write(despues)
    ruta_mapa = os.path.join(DOCS, "sitemap.xml")
    mapa = sitemap()
    antes = open(ruta_mapa, "rb").read() if os.path.exists(ruta_mapa) else b""
    if mapa != antes:
        cambian += 1
        print("%s docs/sitemap.xml (%d páginas)" % ("cambiaría" if comprobar else "escrito  ", len(PAGINAS)))
        if not comprobar:
            open(ruta_mapa, "wb").write(mapa)
    print("%d páginas en la tabla; %s: %d" % (len(PAGINAS), "por cambiar" if comprobar else "cambiadas", cambian))
    return 0


if __name__ == "__main__":
    sys.exit(main())
