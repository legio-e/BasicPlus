# Resultado de la maqueta v1 — validación del modelo de memoria

**Fecha:** 2026-07-05 · **Artefacto:** [`maqueta/Maqueta.java`](maqueta/Maqueta.java)
(Java 8, hilos de SO reales). **Plan:** [`plan-validacion-maqueta.md`](plan-validacion-maqueta.md).

> Este documento cubre la parte de **concurrencia/UAF** (`Maqueta.java`). La cascada
> owner (C4) y el GC preciso (C5) se validan en `MaquetaGc.java`. El paquete completo
> y la cobertura de criterios: [`maqueta/README.md`](maqueta/README.md).

## Qué se construyó
Un modelo mínimo (no la VM real) del heap propuesto: tabla de slots con **contador
de generación**, handle = `(índice, generación)`, **reclamación diferida** a un
safepoint stop-the-world (modelado con un `ReadWriteLock`: read = mutadores en
paralelo, write = safepoint), y publicación vía la generación con semántica volatile
(`AtomicIntegerArray`). Dos modos corriendo el MISMO test concurrente (4 hilos, heap
de 64 slots, 3 s/modo):
- **BROKEN** = modelo viejo (handle = índice, reuso inmediato, sin chequeo).
- **MODEL** = propuesto (generación + reclamación diferida + free chequeado).

El test estresa el escenario UAF: cada hilo aloca objetos con un valor conocido, los
publica en una estructura compartida, libera los suyos (semántica owner) dejando la
referencia publicada como **colgante**, y todos leen referencias al azar (vivas o
colgantes).

## Hallazgo del spike (lo que "si falla, se corrige")
La **v0** del MODEL dio 22.432 corrupciones (no 0). Diagnóstico: `free()` no validaba
la generación → liberar un handle **colgante** bumpeaba la generación del objeto
**vivo** que ahora ocupaba el slot y provocaba **doble-free → doble-alloc → dos
objetos vivos en el mismo slot → corrupción de valor**.

> **Refinamiento del modelo (nuevo requisito):** la liberación debe ser
> **generation-checked** — solo la primera liberación de un handle vivo tiene efecto;
> una liberación stale o doble es un **no-op seguro**. Implementado con un
> compare-and-set atómico sobre la generación (`gen.compareAndSet(s, g, g+1)`).
> No basta chequear la generación al **leer**; hay que chequearla también al **liberar**.

Con el arreglo → **0 corrupciones**.

## Resultado (v1, tras el arreglo)
| Modo | allocs | lecturas | vivas-OK | **corrupciones** | colgantes detectados |
|---|---|---|---|---|---|
| BROKEN | 2,4M | 8,3M | 940K | **7.410.356** | 0 |
| MODEL | 164K | 6,8M | 26.725 | **0** | 6.818.257 |

**PASA:** el modelo lee correctamente los objetos vivos, detecta **todos** los
accesos colgantes como "objeto eliminado" (contrato B), y elimina por completo la
corrupción que el modelo viejo sufre millones de veces.

## Barrido de hilos (1→16, máquina de 20 procesadores lógicos, 1200 ms/modo)

| hilos | BROKEN corrupciones | MODEL corrupciones | MODEL colgantes detectados |
|---|---|---|---|
| 1 | 11.627.771 | **0** | 2.442.018 |
| 2 | 3.773.891 | **0** | 2.823.842 |
| 4 | 2.746.900 | **0** | 2.925.384 |
| 8 | 2.208.870 | **0** | 2.926.439 |
| 16 | 2.079.172 | **0** | 2.309.823 |

**Lecturas:**
1. **MODEL = 0 corrupciones en TODO el rango** → el mecanismo (generación + free
   chequeado) no depende del nº de hilos. Resultado robusto.
2. **BROKEN se corrompe incluso con 1 hilo** (11,6M) → confirma que lo reproducido es
   la clase **use-after-free LÓGICA** (reuso de slot), que ocurre sin paralelismo.
   **NO** es la data-race de B1 (esa daría 0 a 1 hilo, el `w1=0%` medido en su día);
   la data-race/publicación de B1 es el asunto de la **barrera A1 → fase de hardware/ARM**.
3. **Cuidado interpretando los recuentos de BROKEN:** bajan al subir hilos por
   **contención** (menos throughput bruto/seg), no porque haya menos carreras. El
   recuento depende del rendimiento; lo que vale es la columna del medio (0 siempre).

> **Ajuste de honestidad sobre C1:** la maqueta valida sólidamente el eje **UAF /
> generación / reclamación diferida** (contrato B). **No** reproduce el `w1/w2/w4` de
> B1 porque B1 es una data-race (necesita ≥2 cores + barreras), y eso es la fase ARM.

## Criterios del plan cubiertos
- ✅ **C1 (parcial)** — batir una carrera conocida: BROKEN reproduce la corrupción
  (7,4M), MODEL la lleva a 0. *Parcial:* es la carrera LÓGICA (UAF/reuso), no el
  `SyncListTest` real ni la publicación-reordenada de ARM.
- ✅ **C2** — contrato B: 6,8M colgantes detectados limpiamente, 0 lecturas erróneas.
- ✅ **C3** — reclamación diferida + generación: sin corrupción bajo churn concurrente.
- ⬜ **C4** (cascada owner), **C5** (long[]/raíces), **C6** (paridad dual-VM),
  **C7** (coste/spike de indirección) — pendientes.

## Limitaciones (honestidad)
1. **Es un modelo, no la VM real.** Valida la LÓGICA, no la implementación de BP.
2. **x86 esconde la publicación-reordenada de ARM.** Aquí la generación se publica con
   semántica volatile (correcto), pero la ausencia de barrera **no** se puede estresar
   en x86. La barrera de publicación en ARM es la **fase de hardware** pendiente.
3. **Pasar da confianza, no prueba** — pero se apoya en batir un fallo reproducible
   (BROKEN) + volumen (millones de ops) + el argumento de diseño.

## Conclusión
El núcleo del modelo (handles + generación + reclamación diferida + **free
generation-checked**) **se sostiene** bajo paralelismo real para las carreras lógicas.
Siguiente: extender a C4/C5, y la fase de hardware para la barrera de publicación (C1
completo + C6).
