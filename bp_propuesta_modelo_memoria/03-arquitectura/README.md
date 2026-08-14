# 03 — Arquitectura y decisiones

**Pregunta:** ¿Está BasicPlus bien estructurado?

Análisis crítico de la arquitectura y registro de las decisiones de diseño y
sus trade-offs.

## Qué vive aquí
- Mapa de componentes: lexer, generador, las dos VMs, IDE, stdlib, AOT, GUI.
- Dependencias entre módulos y direcciones de acoplamiento.
- **ADRs** (Architecture Decision Records): decisión, contexto, alternativas,
  consecuencias. Una por decisión relevante.
- Riesgos técnicos y deuda observada.

## Principio
Reconstruir el *porqué* de cada decisión a partir de los manuales (`PHILOSOPHY`,
`bp-desde-dentro`, `SMP_ARCH`, `MOD_FORMAT`…). Donde el porqué no esté
documentado, es una pregunta abierta → hallazgo.

## Formato ADR
`adr-NNNN-titulo.md` con: Estado · Contexto · Decisión · Alternativas ·
Consecuencias.
