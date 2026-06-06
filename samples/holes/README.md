# samples/holes — repros de "agujeros" (consolidación V2)

Programas mínimos que reproducen bugs/limitaciones concretos. Sirven de
verificación al taparlos y de guardia anti-regresión. Compilar con:

    java -jar lexer-java/target/basicplus-frontend.jar samples/holes/<f>.bp

(Diagnósticos en formato `[línea:col] error ...`.)

## Cascada del compilador — ✅ TAPADO (#232)
Un error real no debe generar errores espurios derivados.

| Repro | Antes | Ahora | Qué prueba |
|---|---|---|---|
| `cascade_undef.bp`   | 6 | 1 | variable indefinida usada 4× → 1 error (dedup) |
| `cascade_type.bp`    | 3 | 1 | tipo inexistente usado 3× → 1 error (dedup de tipos) |
| `cascade_badtype.bp` | 4 | 2 | tipo+función inexistentes; sin errores de operador espurios |
| `cascade_sem.bp`     | 1 | 1 | arg-count incorrecto (ya estaba bien) |
| `cascade_syn.bp`     | 1 | 1 | falta `then` (parser ya recuperaba bien) |

Arreglo (en `SemanticAnalyzer.java`): (a) `analyzeBinary` propaga `ErrorType`
en silencio si un operando ya es `<error>` (poisoning); (b) dedup por nombre
de "identificador no resuelto" y "tipo no encontrado".

## BUG-2 — excepciones cross-module — ⏳ PENDIENTE (siguiente)
- `HoleLibA.bp`: define `class MyError` y la lanza (`throw MyError(...)`). Compila.
- `HoleAppB.bp`: `import HoleLibA` + `catch e: HoleLibA.MyError`. **No compila**:
  el parser no acepta un tipo cualificado `Mod.Clase` en la cabecera del `catch`.
  Detrás (análisis): aunque parseara, la identidad de clase cross-module no
  casaría en runtime (solo `RuntimeError` se resuelve por nombre exportado).
- Bonus menor: `Mod.Clase` no resuelve ni dentro del propio `Mod`.
