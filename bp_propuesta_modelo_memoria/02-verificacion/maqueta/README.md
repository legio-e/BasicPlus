# Maqueta de validación del modelo de memoria — paquete para Claude-Code

**Qué es:** un modelo de referencia ejecutable + suite de pruebas que **valida el
diseño** del nuevo modelo de memoria de BasicPlus (handles + generación +
reclamación diferida + owner + GC preciso) **antes** de implementarlo en producción.
No es la VM real; es la prueba de que el modelo se sostiene.

**Diseño validado:** [`../../03-arquitectura/heap/objetivo-modelo-memoria-smp.md`](../../03-arquitectura/heap/objetivo-modelo-memoria-smp.md)

## Cómo ejecutar
```
javac Maqueta.java MaquetaGc.java
java Maqueta            # estrés concurrente (UAF); acepta: java Maqueta <hilos> <ms>
java MaquetaGc          # corrección determinista (cascada owner + GC); PASS/FAIL
```

## Las dos piezas
| Fichero | Tipo | Qué valida |
|---|---|---|
| `Maqueta.java` | estrés concurrente (hilos reales) | UAF / contrato B / reclamación diferida bajo paralelismo, barrido 1→16 hilos |
| `MaquetaGc.java` | corrección determinista (aserciones) | cascada owner ≥2 niveles + GC preciso (raíces de módulo, tipos) |

## Cobertura de criterios (ver `../plan-validacion-maqueta.md`)
| # | Criterio | Estado | Evidencia |
|---|---|---|---|
| C1 | Batir una carrera conocida | ✅ **parcial** | UAF: BROKEN 7,4M corrupciones → MODEL **0**, en 1→16 hilos. (La data-race de B1 es fase ARM, ver abajo) |
| C2 | Contrato B ("objeto eliminado") | ✅ | 6,8M colgantes detectados, 0 lecturas erróneas |
| C3 | Reclamación diferida + generación | ✅ | 0 corrupción bajo churn concurrente |
| C4 | Cascada owner ≥2 niveles | ✅ | `MaquetaGc`: A→B→C liberados; D (no-owned) sobrevive; doble-free no-op |
| C5 | long[]/double[] + raíces de módulo | ✅ | `MaquetaGc`: long[] (kind 5) alcanzable desde global sobrevive; inalcanzable recolectado |
| C6 | Paridad dual-VM byte-idéntica | ⬜ | requiere la VM-C / segunda VM |
| C7 | Coste de indirección | ⬜ | ver `../spike-overhead-handles.md` |

## Refinamientos del diseño que la validación PRODUJO (importante para la implementación)
1. **La liberación debe ser GENERATION-CHECKED, no solo la lectura.** Solo la primera
   liberación de un handle vivo tiene efecto; stale/doble = no-op seguro
   (`gen.compareAndSet(slot, g, g+1)`). Sin esto: liberar un handle colgante bumpea la
   generación del objeto vivo del slot → doble-free → doble-alloc → **corrupción**. La
   maqueta pasó de 22.432 corrupciones a **0** con este arreglo. **Debe ir en el diseño
   de producción.**

## Propiedades que el modelo hace IMPOSIBLES POR CONSTRUCCIÓN (no "arregladas")
- **H-011** (long[]/double[] invisibles al GC) y **H-008** (raíces falsas por escaneo
  conservador): el GC es **preciso** (traza handles por la tabla) y **type-agnostic**
  (no mira el tipo) → no puede haber falsos negativos ni positivos por tipo.
- **H-012** (raíces de módulo no escaneadas): un global es una raíz que sostiene un
  handle como cualquier otra → se traza igual.
- **H-006/H-007/H-009/H-010**: cerrados por generación + reclamación diferida + free
  chequeado (ver el documento de diseño).

## Alcance honesto — lo que esta maqueta NO prueba (y por qué)
- **La data-race de B1 y la barrera de publicación A1** → **fase de hardware/ARM**.
  x86 tiene modelo de memoria fuerte y esconde las reordenaciones de store que rompen
  la publicación en ARM. Lo que se reproduce aquí es la clase **use-after-free lógica**
  (ocurre incluso con 1 hilo), no la data-race (que da w1=0% y necesita ≥2 cores).
- **Paridad dual-VM (C6)** → requiere ejecutar el modelo también en la VM-C.
- **Coste de indirección (C7)** → el spike de medición.
- Y en general: pasar tests de concurrencia da **confianza, no prueba**; aquí se apoya
  en batir un fallo reproducible + volumen (millones de ops) + el argumento de diseño.

## Veredicto
El **núcleo del modelo se sostiene**: handles + generación + reclamación diferida +
free generation-checked + owner con cascada + GC preciso. 5 de 7 criterios validados
en software; los 2 restantes son inherentemente de hardware/segunda-VM. Listo para
consolidar como propuesta de diseño y pasar a Claude-Code, con las fases de hardware
explícitamente marcadas como pendientes.
