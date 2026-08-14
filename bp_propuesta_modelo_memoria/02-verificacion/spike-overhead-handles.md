# Spike — medir el overhead de la indirección por handles

**Origen:** decisión de diseño en
[`03-arquitectura/heap/objetivo-modelo-memoria-smp.md`](../03-arquitectura/heap/objetivo-modelo-memoria-smp.md)
(tabla de handles con generación). **Tipo:** experimento desechable (*spike*), en
la cultura "primero el spike" del proyecto. **Estado: ✅ EJECUTADO 2026-07-08 → handles
uniformes viables.** Host x86 (OoO+caché) **~0 %** · Pico Cortex-M33 (SRAM) **+2–5 %** · Metro
(heap PSRAM + tabla SRAM) **+2–4 %** — la PSRAM **no** lo empeora (tabla en SRAM). Hipótesis
confirmada; en `native` se resuelve-1-vez-y-pin → ~0. Detalle en `docs/V4_BACKLOG.md` H1.1;
plan de la migración en
[`../03-arquitectura/plan-h1.2a-ensanchado-refs.md`](../03-arquitectura/plan-h1.2a-ensanchado-refs.md).

## Hipótesis a confirmar
El coste de acceder a un objeto vía **handle + comparación de generación** (en vez
de puntero plano) es:
1. **Despreciable en código interpretado** (se ahoga en el coste por-opcode del
   intérprete) — se espera overhead de un solo dígito %.
2. **Evitable en código `native`** resolviendo el handle una vez en el borde de la
   llamada y operando con puntero crudo sobre el objeto fijado (pin).

Es **hipótesis, no dato** (Eduardo: "habría que confirmarlo"). El modelo de memoria
uniforme (todas las refs = handles) se apoya en que (1) sea cierto.

## Qué medir
| Escenario | Comparar | Métrica |
|---|---|---|
| Interpretado — acceso a campo en bucle | ref plana vs. handle+generación | ns/acceso, % overhead |
| Interpretado — dispatch de método (vtable vía handle) | plano vs. handle | % overhead |
| `native` — kernel de cómputo con acceso a array | plano vs. handle-resuelto-una-vez+pin | ¿se conserva el 10-100×? |
| Presión: micro de gama baja (~100-150 MHz) vs. alta (~800 MHz) | — | ¿cambia la conclusión en el extremo lento? |

## Criterio de decisión
- Si (1) se confirma (overhead interp bajo) y (2) el kernel native conserva su
  ganancia → **handles uniformes** viable; se cierra la bifurcación.
- Si el overhead interpretado resultara alto en la gama baja → reconsiderar
  **híbrido** (handles solo para compartidos/`owner`).

## Notas
- Aprovechar la infraestructura de medición existente (estilo `h3bench`) y la
  **paridad dual-VM**: medir en las dos VMs para que la conclusión no dependa de una.
- No hace falta implementar el modelo completo: basta un prototipo del acceso
  (plano vs. indirecto) representativo.
