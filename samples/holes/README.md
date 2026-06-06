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

## BUG-2 — excepciones cross-module — ✅ TAPADO (#233)
- `HoleLibA.bp`: define `class MyError` y la lanza (`throw MyError(...)`).
- `HoleAppB.bp`: `import HoleLibA` + `catch e: HoleLibA.MyError` → **atrapa OK**.
  Salida en AMBAS VMs (byte-idéntica): `OK: atrapado MyError cross-module` / `fin`.

Arreglo: opcode **aditivo** `TRY_BEGIN_EXT` (0xAB) — el `TRY_BEGIN` local (i16) no
cambia, así que los `.mod`/firmware existentes siguen válidos (sin bump de MAGIC).
Capas: parser acepta `Mod.Clase` en el `catch`; el semántico lo resuelve
(local / importado / auto-cualificado) vía `resolveCatchClass`; el emisor emite
`TRY_BEGIN_EXT` con `clsOff` i32 parcheado en link-time (`ehClassFixups`, §4.4 de
la sección exports) con la dirección del descriptor de la clase cross-module;
ambas VMs decodifican el i32 y ambos linkers parchean por nombre cualificado.
Bonus: `Mod.Clase` ahora también resuelve dentro del propio `Mod`.
