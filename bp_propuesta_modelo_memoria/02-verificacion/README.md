# 02 — Verificación y conformidad

**Pregunta:** ¿Cómo comprobamos que BasicPlus cumple lo que promete?

Es el "estamos haciendo las cosas bien" hecho **medible**. Partimos de la
especificación de [`../01-especificacion/`](../01-especificacion/) y diseñamos
cómo demostrar que la implementación la satisface.

## Qué vive aquí
- Estrategia de pruebas: qué se prueba, a qué nivel, con qué criterio de éxito.
- **Tests diferenciales miVM (Java) vs bpgenvm-c (C99):** el corazón del
  invariante de salida byte-idéntica.
- Suites de conformidad por área del lenguaje y de la stdlib.
- Casos límite y de error (overflow, índices, división por cero, red…).
- Cobertura: qué features tienen prueba y cuáles no (matriz de trazabilidad
  requisito → prueba).

## Principio
Todo requisito de `01-especificacion/` debería poder trazarse a una prueba. Los
requisitos sin prueba son un hallazgo.
