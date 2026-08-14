# Plan de validación de la maqueta del modelo de memoria

**Origen:** [`03-arquitectura/heap/objetivo-modelo-memoria-smp.md`](../03-arquitectura/heap/objetivo-modelo-memoria-smp.md).
**Método (Eduardo):** pensar → candidato → **maqueta que valida** → si pasa,
documentar y pasar a Claude-Code; si falla, iterar. **Estado:** propuesto.

Qué debe **demostrar** la maqueta antes de dar el modelo por bueno. No es "compila y
va"; apunta a los riesgos reales.

## Criterios de aceptación

| # | Qué demuestra | Criterio de éxito |
|---|---|---|
| 1 | **Batir el fallo conocido (la prueba reina)** — reproducir las condiciones de B1 (`SyncListTest` o equivalente, workers reales, w2/w4) sobre el modelo nuevo | **0 % de fallo** en N ejecuciones altas (p. ej. 100), donde el modelo viejo daba 25 %/100 % |
| 2 | **Contrato B** — owner + alias normal → liberar owner → deref del alias | error determinista **"referencia a objeto eliminado"**; nunca basura ni crash |
| 3 | **Reclamación diferida** — owner-free no reclama en el sitio; safepoint reclama; caché de puntero por-quantum | sin corrupción bajo churn + alloc concurrente; la memoria **se recupera** (no fuga) |
| 4 | **Cascada owner (H-006)** — árbol de owners ≥2 niveles → liberar raíz | todos liberados en el paso de reclamación |
| 5 | **long[]/double[] (H-011) y raíces completas (H-012)** — objeto alcanzable solo por global de módulo; long[]/double[] vivo | sobreviven al GC (no recolección prematura) |
| 6 | **Paridad** — todo lo anterior en las dos VMs | salida **byte-idéntica** Java↔C |
| 7 | **Coste** — overhead de indirección (enlaza el [spike](spike-overhead-handles.md)) | dentro de lo esperado (bajo en interp; native conserva su ganancia) |

## Honestidad epistemológica (punto 1)

Para lo concurrente, **pasar el test da confianza, no prueba** — las carreras son
probabilísticas y pueden esconderse. El criterio se sostiene sobre **tres patas**:
1. **Reproducir el fallo conocido** (lo más potente: partimos de un fallo medido).
2. **Volumen** alto de ejecuciones (y, si se puede, herramientas de estrés/detección).
3. **Argumento de diseño** (la barrera está en el único punto de publicación).
Ninguna pata sola basta.

## Secuencia sugerida

1. **Maqueta en VM-Java primero.** Es donde B1 se reproduce (workers en hilos de SO
   reales) y es la VM de referencia. Valida "¿el modelo es correcto?".
2. **Barreras C/ARM después, en la VM-C sobre hardware real.** Una barrera mal puesta
   puede "funcionar" en x86 y fallar en ARM (modelos de memoria distintos). Valida
   "¿las barreras son correctas en el chip?".

## Al terminar
- Si pasa todo → consolidar el modelo como **propuesta de diseño limpia** y pasarla a
  **Claude-Code** para la implementación de producción.
- Si algo falla → es un hallazgo del modelo; se corrige el diseño y se re-valida.
