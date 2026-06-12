# BASICPLUS — nota histórica

Este fichero era el README de la FASE DE DISEÑO del lenguaje (mayo
2026), cuando el proyecto consistía únicamente en la gramática EBNF y
aún no existían ni el lexer ni las VMs. Se conserva como recuerdo del
punto de partida.

El proyecto actual está documentado en:

- `README.md` — portada del proyecto (empezar por aquí).
- `docs/manual.html` — manual del lenguaje implementado.
- `docs/referencia.html` — biblioteca estándar, CLI y artefactos.
- `docs/QUICKSTART.md` — de cero a blink por plataforma.
- `basicplus_grammar.ebnf.txt` — la gramática de aquella fase de
  diseño, con una nota de estado actualizada (qué entregó la V2 y qué
  quedó como idea para v3).

Casi todo lo que aquel diseño imaginó se construyó — y bastante más:
dos VMs gemelas con paridad byte-idéntica, AOT nativo, debugger
on-device, tres familias de microcontroladores y un IDE.
