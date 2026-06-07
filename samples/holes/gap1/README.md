# gap1 — paridad de builtins VM-C (GAP-1)

La VM-C implementa un *subconjunto* de los ~126 builtins de la VM-Java
(decisión de alcance, no bug). GAP-1 (2026-06-07) endurece ese subconjunto:

1. **Fallo limpio** — un builtin ausente lanza un `RuntimeError` BP *atrapable*
   en vez de abortar la VM con `BAD_OPCODE` (crash duro).
2. **Portados byte-exactos** — `abs`/`min`/`max` (enteras) → paridad idéntica.
3. **Caveat float** — `sqrt`/`sin`/… NO se portan a la ligera (Java `Math` vs
   `libm` pueden diferir en el último ULP → rompería el stdout byte-idéntico).

Detalle y clasificación completa: `docs/BUILTINS.md` § *Paridad VM-C (GAP-1)*.

Compilar (`$F` = `lexer-java/target/basicplus-frontend.jar`):

    java -jar $F samples/holes/gap1/GapBuiltins.bp    --compile <dir> --backend=mivm
    java -jar $F samples/holes/gap1/MissingBuiltin.bp --compile <dir> --backend=mivm

Ejecutar y comparar (VM-Java vs VM-C):

    java -jar miVM/target/bpgenvm-1.0.jar <dir>/GapBuiltins.mod
    bpgenvm-c/build/bpgenvm-c.exe        <dir>/GapBuiltins.mod

| Fichero | Qué prueba |
|---|---|
| `GapBuiltins`    | `abs`/`min`/`max` → **byte-idéntico** en VM-Java y VM-C (`7 7 3 9 -5 -2`) |
| `MissingBuiltin` | `sqrt` (ausente en VM-C) → RuntimeError **atrapado** + el programa sigue vivo (`status=OK`, exit 0) |
