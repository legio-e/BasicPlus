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
