# 01 — Especificación / requisitos

**Pregunta:** ¿Qué *debe* hacer BasicPlus?

La fuente de verdad para responderla son los manuales y la gramática de
[`../fuentes/`](../fuentes/). Aquí destilamos de ese material una especificación
analizable: qué se promete, con qué contrato, y dónde queda ambiguo.

## Qué vive aquí
- Semántica del lenguaje (tipos, OO, excepciones, concurrencia, módulos).
- Gramática: lectura crítica del EBNF, ambigüedades, reglas no cubiertas.
- Contratos de la biblioteca estándar (precondiciones, errores, efectos).
- El **invariante de las dos VMs** (salida byte-idéntica) como requisito central.
- Requisitos por plataforma (Pico, ESP32, STM32).

## Principio
Separar lo **especificado** (está en el manual) de lo **implícito** (se deduce
del ejemplo) de lo **no definido** (nadie lo dice). Cada hueco relevante → un
hallazgo en [`../hallazgos/`](../hallazgos/).
