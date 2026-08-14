# ADR-0001 — Modelo de memoria: allocator no-moving, compactación diferida (H3, V2)

- **Estado:** Aceptada (2026-05-31) · reconstruida por análisis, no vinculante
- **Ámbito:** VM (miVM Java + bpgenvm-c). No afecta al lenguaje ni al `.mod`.

> ADR reconstruido a partir de la documentación del proyecto (`V2_BACKLOG.md`,
> `h3bench/NOTAS.md`, `HEAP_LAYOUT.md`). Captura el *porqué* de una decisión ya
> tomada, para poder revisarla con criterio. No modifica nada en `pm`.

## Contexto

En V1 el GC de la VM-C era **mark-sweep bump-sin-reuse**: marcaba bloques como
`FREE` pero nunca los reutilizaba, así que el heap solo crecía hasta agotarse
("el acantilado" → OOM en el MCU). Antes de V2 se eleva el GC a hito propio (H3)
con una regla: **reimplementar el modelo de memoria manteniendo la interfaz
GC↔VM** (alloc, layout de cabecera, enumeración de raíces), para que la VM cambie
lo mínimo — misma filosofía que el AOT y el HAL.

Se construyeron dos herramientas de diagnóstico (`heapFrag()`, `heapMap()`) y se
caracterizó el baseline con cuatro workloads de estrés (ver
`analisis-modelo-memoria.md §3`).

## Decisión

1. **Allocator NO-MOVING**: free-list first-fit + split + **coalescing** de
   huecos adyacentes en cada sweep.
2. **+ 2 mejoras model-agnostic**: (a) disparo de GC **por umbral** de
   crecimiento del bump; (b) **heapNext-retreat**.
3. Aplicado en **ambas VMs** (la VM-C se subió del bump-sin-reuse al nivel de la
   Java).
4. **Compactación (moving): DIFERIDA** — solo si un workload real exhibe la
   patología de supervivientes dispersos.

## Alternativas consideradas

| Alternativa | Por qué no (ahora) |
|---|---|
| **Compactación (moving)** | Elimina la fragmentación (frag→0), pero exige **pinning** para las funciones `native` (recorren buffers crudos) → complejidad; la evidencia no mostró la patología en workloads realistas |
| **Free-list segregado / TLSF** | Mejor colocación y O(1) frente al first-fit O(n); queda como opción no-moving futura, no como bloqueante |
| **Seguir en bump-sin-reuse** | Es el statu quo roto (OOM); descartado |

## Consecuencias

**Positivas**
- Se elimina el acantilado OOM en churn (reuso real; validado: 4.4 MB en heap de
  254 KB sin OOM).
- −59 % de pico de heap en el workload realista (umbral + retreat).
- Paridad dual-VM mantenida (salida byte-idéntica); sin tocar `.mod`.

**Negativas / deuda aceptada**
- **Fragmentación patológica** posible con longevos dispersos (frag 0.96 medida
  en banco) — **no resuelta**, solo evitada por hipótesis de workload.
- **Sin cota de pausa** del GC stop-the-world (afecta lazos de control).
- La decisión de compactar queda con **criterio cualitativo**, sin monitor.
- First-fit **O(n)** en la free-list (coste no medido en tiempo).

## Revisión

Reabrir esta ADR si: (a) aparece un workload real con frag alta sostenida; (b) se
fija una cota de pausa que el STW actual no cumpla; (c) PSRAM abundante cambia el
cálculo (con 8 MB de heap la urgencia baja). Ver hallazgos H-004 y H-005.
