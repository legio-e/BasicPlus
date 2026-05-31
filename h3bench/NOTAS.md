# H3 — Cuaderno de mediciones del GC (VM-Java)

Herramientas: `heapFrag()` y `heapMap(cols)` (builtins solo-Java, commit 093db78).
Allocator actual (baseline) = **mark-sweep no-moving, free-list first-fit + split
+ coalescing de huecos adyacentes en cada sweep**.

`frag = 1 − mayorHueco/totalLibre` (externa): 0 = libre contiguo; →1 = añicos.
`util = vivos/committed`. `bumpRemain` = reserva contigua al final (no es frag).

---

## Baseline (allocator actual)

### Workload: churn (basura entre 2 vivos) — `heaptest.bp`
50 arrays muertos entre `a` y `b` vivos. Tras gc():
```
frag=0.000 util=0.228 | committed=8140 vivos=1856(4obj) libres=6284 holes=1 mayorHole=6284
```
→ Los muertos consecutivos **coalescen en 1 hueco**. Sin fragmentación. El
free-list+coalescing maneja el churn perfecto.

### Workload A: supervivientes DISPERSOS (1 de cada 3) — `fragsurv.bp`
150 arrays de 20 ints; 1 de cada 3 retenido en una List (disperso). Tras gc():
```
frag=0.965 util=0.348 | committed=13732 vivos=4772(53obj) libres=8960 holes=51 mayorHole=312
:#:..::..::..::..::..::..:#:.:#:.:#:...::..::..::..::..:#:.:#:.:#:..::....::..::
..::..:#:.:#:.:#:..::..::..::..::..::..:#:.:#:.:#:..::..:####:..::..::..:#:.:#:.
:#:..::..::..::..::..::..:#:.:#:.:#:..::..::..:##:::.:#
```
→ **Fragmenta fuerte**: 8960 B libres pero el mayor hueco es 312 B. Los
supervivientes intercalados impiden coalescer. Con el bump-tail agotado esto
sería **OOM prematuro** (no cabe nada > 312 B pese a 8.7 KB libres).

---

## Lectura provisional

- El no-moving + coalescing es **suficiente** para churn puro (basura efímera).
- **Fragmenta** cuando hay objetos longevos DISPERSOS entre efímeros → la
  pregunta moving-vs-no-moving está VIVA.
- La compactación (moving) eliminaría los 51 huecos; el coste es la indirección
  (tabla de handles) o el fixup de refs (stack maps).

## Pendiente de medir
- [ ] Workload B: tamaños MIXTOS dispersos (fragmentación externa peor: huecos
      de tamaños dispares, first-fit deja restos inservibles).
- [ ] Premura de OOM: heap pequeño (--mem) + fragmentar + alocar > mayorHole.
- [ ] Tiempo de cada workload (now()) — coste del first-fit O(n) en free list.
- [ ] Candidatos a comparar: (a) free-list segregado / TLSF (no-moving, mejor
      colocación y O(1)); (b) compactación (moving) — medir frag final = 0 pero
      coste de pausa/indirección.

---

## Actualización: + workloads C y D, tabla baseline

### Workload C: REALISTA (control loop) — `fragreal.bp`
working-set fijo (10) + churn (500×2 transitorios) + ring de 8 resultados
(retención ACOTADA). Tras gc() final:
```
frag=0.637 util=0.033 | committed=146104 vivos=4764(24obj) libres=141340 holes=11 mayorHole=51344 bumpRemain=113685
```
→ Frag MODERADA (11 huecos) pero el mayor hueco son 51 KB → un alloc grande AÚN
CABE → la fragmentación NO muerde aquí. El dato gordo es otro: **committed=146 KB
para 4.7 KB vivos** (util 0.03). El heap se infló por BUMP porque **el GC solo
dispara al agotar el bump** (nunca corrió a mitad del lazo). Over-commit por GC
perezoso, no por fragmentación.

### Workload D: tamaños MIXTOS dispersos — `fragmixed.bp`
```
frag=0.966 util=0.342 | committed=32644 holes=51 mayorHole=720
```
→ Como A (frag 0.97) con huecos de tamaños dispares (mayorHole 720 vs 312).
Supervivientes dispersos fragmentan; el mixto no lo empeora drásticamente.

### Tabla baseline (allocator actual = no-moving free-list + coalescing)
| workload | frag | holes | mayorHole | committed | nota |
|---|---|---|---|---|---|
| churn | 0.000 | 1 | 6284 | 8140 | coalesce perfecto |
| superv. dispersos | 0.965 | 51 | 312 | 13732 | fragmenta fuerte |
| realista (loop) | 0.637 | 11 | 51344 | 146104 | frag benigna; OVER-COMMIT |
| tamaños mixtos disp. | 0.966 | 51 | 720 | 32644 | fragmenta fuerte |

### Lectura consolidada
1. **Fragmentación patológica** (frag 0.96+) SOLO con muchos supervivientes
   longevos DISPERSOS. Ningún no-moving (ni TLSF) lo arregla — solo compactación.
2. El patrón REALISTA (working-set + churn + retención acotada) NO la produce
   (pocos longevos, agrupados) → frag benigna (0.64, hueco mayor 51 KB).
3. Problema DISTINTO e independiente del moving: **over-commit por GC perezoso**
   (heap crece a su pico de bump; el no-moving no devuelve memoria — heapNext no
   retrocede).

### Insights accionables (NO tocan el modelo moving/no-moving)
- **(barato, alto impacto) Política de disparo de GC**: además de "bump lleno",
  disparar por umbral (p.ej. bump avanzó N KB desde el último GC) → mantiene
  committed cerca del working-set.
- **(barato) heapNext retrocede**: si el último run libre del sweep toca
  heapNext, bajar heapNext (devolver al bump). Recupera memoria sin compactar.

### Conclusión provisional para la DECISIÓN
- Para patrones embebidos típicos (working-set + churn + retención acotada) el
  **no-moving + coalescing es adecuado** en fragmentación; el dolor real es el
  over-commit, atacable con disparo-por-umbral + heapNext-retreat (baratos).
- La compactación (moving) solo se justifica si un workload REAL acumula muchos
  longevos dispersos; recordar además el coste de PINNING (native) del lado
  moving.
- Candidato recomendado a prototipar primero: **no-moving mejorado** (las 2
  mejoras baratas; TLSF/segregado opcional para O(1)), dejando moving como
  opción solo si un workload real exhibe la patología. (Decisión final: usuario.)

## Pendiente
- [ ] Implementar las 2 mejoras baratas y re-medir over-commit en el realista.
- [ ] (opc.) Prototipo TLSF no-moving: comparar tiempos/frag vs first-fit.
- [ ] (opc.) Premura de OOM con heap pequeño (--mem) bajo supervivientes disp.
