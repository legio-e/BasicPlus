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

## GAP-2 — tipos built-in cross-module — ✅ TAPADO (#236)
- `HoleLibD.bp`: `public function makeList(): List` (devuelve un `List`).
- `HoleAppD.bp`: `var l: List := HoleLibD.makeList()` → **compila y corre**.
  Salida en AMBAS VMs (byte-idéntica): `length = 3`.

Antes: `error: valor de tipo 'List' no asignable a variable de tipo 'List'`. Los
built-in (List/Map/SyncList…) se sintetizan idénticos por módulo, pero el tipo de
retorno de una función importada cruza vía el `.bpi` como `UnresolvedClassRef`
(clase referida por nombre, sin símbolo local), y `ClassType.isAssignableFrom`
sólo aceptaba `ClassType` → lo rechazaba (asimetría: `UnresolvedClassRef` era laxo
en su dirección, `ClassType` no reciprocaba). Fix solo-semántico en
`BpType.ClassType`: acepta un `UnresolvedClassRef` del mismo nombre — general,
cubre built-in Y clases de usuario retornadas/pasadas cross-module. El runtime ya
funcionaba (síntesis idéntica = mismos slots; `INVOKE_VIRTUAL` usa el class_ptr
del receiver).
