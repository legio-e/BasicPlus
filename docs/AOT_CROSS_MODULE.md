# AOT — llamadas cross-module (#169)

Diseño consensuado para resucitar #169 P-aot-cross-module-call.
Captura el insight clave: **el trabajo de compilación es el mismo
que para llamadas BP→BP entre módulos.** Solo hace falta una pieza
runtime nueva.

## 1. Cómo funcionan HOY las llamadas BP→BP cross-module

Pipeline (lo que ya hace MivmEmitter + loader):

### Compile-time

1. El módulo declara `import B [from path]`. El frontend resuelve `B`:
   - Si `B.mod` ya compilado existe: lo usa.
   - Si solo existe `B.bp`: compila primero su **interfaz** (`B.bpi`)
     — modelo Modula-2 DEFINITION. La `.bpi` lleva la lista de
     funciones públicas con `(nombre, tipos de parámetros, tipo de
     retorno, índice en la tabla pública del módulo)`.

2. Cuando MivmEmitter encuentra una `CallExpr` que es `B.foo(args)`:
   - Mira en el namespace import: `B.foo` resuelve a `(módulo=B,
     índice=k)` vía la `.bpi`.
   - Emite OP_CALL_EXT con el slot del import (un u16). El slot
     queda registrado en la sección imports del `.mod` como
     `qualified="B.foo"`.

3. Resultado: el `.mod` lleva una tabla `imports[]` de strings
   cualificados. El `code[]` referencia esos slots con
   `OP_CALL_EXT idx`.

### Runtime

4. `bpvm_link_all` resuelve cada import del `.mod` cargado:
   - Para cada slot k del import-table, hace
     `bpvm_link_lookup(vm, imports[k])` → obtiene la dirección
     absoluta de la función target.
   - Escribe esa dirección en `ext_table[k]` (en `memory[]` del
     módulo).

5. El intérprete ejecuta `OP_CALL_EXT idx`:
   - Lee `ext_table[idx]` → dirección absoluta de la función.
   - Push frame, salta. Igual que un CALL local.

**La dirección absoluta es `module.code_start + function.offset`**,
donde `function.offset` viene del símbolo público del módulo target.
Esto resuelve a la pregunta original del usuario:

> CS+ offset positivo son las funciones. Desde fuera necesitamos
> la referencia al módulo y el índice de la función.

## 2. Aplicación a AOT (#169)

Lo que descubrimos: **el AotCEmitter necesita exactamente la misma
información que MivmEmitter**:

- Para una `CallExpr` que es `B.foo(args)`, necesita:
  - Resolver `B.foo` vía la `.bpi` → `(módulo=B, índice=k, signature
    (paramTypes, retType))`.
  - La signature le permite generar el código C tipo-seguro (no
    hace falta asumir todo como `i32`).

Es decir, **el trabajo de compilación se comparte con MivmEmitter**
si AotCEmitter tiene acceso a las `.bpi` importadas. No es un
problema nuevo — es el mismo que ya resolvimos para BP→BP.

## 3. Lo que SÍ es nuevo: el trampolín runtime native→externa

En BP→BP: el intérprete hace todo (OP_CALL_EXT → ext_table → jump).

En AOT nativo: necesitamos una función del **runtime** que el código
nativo emitido invoca, y que internamente hace lo equivalente al
`OP_CALL_EXT`. Esa función VM:

1. Recibe identificación del target (dirección absoluta, o índice de
   import, o nombre cualificado — a decidir).
2. Recibe los args.
3. Computa la dirección de la función target (`vm->modules[k].code_start
   + offset`, o lookup en `bpvm_link_lookup` con caché).
4. Decide qué hacer según si el target tiene thunk AOT registrado:
   - **Thunk AOT existe** (target es `native` AOT-compilado y
     registrado): push args al BP stack del thread actual, llama
     `thunk(vm, &sp, &bp)`, pop resultado.
   - **No hay thunk** (target es BP interpretado): re-entra el
     intérprete con esa PC como objetivo. Push frame de "return
     sentinel", set tc.pc = target, corre interp_run_quantum hasta
     que vuelva al sentinel, pop resultado.

Llamémosle por ahora:

```c
/* En aot_helpers_v2 (no rompe ABI de v1). */
int32_t (*call_external_i32)(struct bpvm* vm,
                              uint32_t target_abs_addr,
                              int32_t* args, int n_args);
/* Variantes paralelas para float / void / etc. según signature. */
```

O — más elegante — un único helper que toma signature dinámica vía
varargs o "tagged args":

```c
int32_t (*call_external)(struct bpvm* vm,
                          uint32_t target_abs_addr,
                          const char* sig,    /* "ii→i", "if→f", ... */
                          ...);
```

A decidir cuando codifiquemos.

## 4. Cómo AotCEmitter obtiene `target_abs_addr`

Dos opciones:

**Opción A — Resolver una vez en runtime, cachear en static.**
Cada call-site emitido tiene su `static uint32_t s_addr_B_foo = 0;`,
inicializado lazily con `H->find_external(vm, "B.foo")` la primera
vez. Análogo al `s_module_cs` de #172. Coste por call: un check del
cache + una llamada indirecta.

Ventaja: AotCEmitter NO necesita compartir slot-numbering con
MivmEmitter. Solo necesita los nombres cualificados (que ya tiene
del AST).

**Opción B — Compartir slot con MivmEmitter.**
AotCEmitter usa el mismo número de slot que el bytecode usa con
OP_CALL_EXT. El helper lee `ext_table[slot]` directamente.

Ventaja: Más rápido (un memory read vs un cache check).
Desventaja: Coordinación frágil. Si MivmEmitter cambia su esquema
de slot allocation, AotCEmitter rompe.

**Recomendación**: Opción A (`H->find_external + cache`). El coste
es despreciable comparado con la llamada misma.

## 5. Caso especial: native → native cross-module

Subset que vale la pena soportar primero porque NO requiere re-entrar
el intérprete (es el caso más complejo del helper):

- Target es `native function` en otro módulo, registrada en
  `aot_registry`.
- El helper detecta el thunk y lo invoca directamente. Sin nested
  interp.
- Push args, llamar thunk, pop resultado. Casi tan rápido como
  una llamada C directa.

Para v1 de #169 podemos restringir al caso "ambos lados son AOT".
Si el target NO está AOT-registrado, lanzamos RuntimeError BP.
Eso cubre el caso útil real (encadenar funciones críticas en
varios módulos) sin pagar el coste de implementar la re-entrada
al intérprete.

La re-entrada (native → BP interpretado) queda como v2 si surge
necesidad real.

## 6. Resumen del plan cuando resucitemos #169

| Pieza | Trabajo |
|---|---|
| **Compile-time** | AotCEmitter lee `.bpi` de imports (igual que MivmEmitter). Para cada `CallExpr` cross-module, genera marshalling de args + llamada al helper con nombre cualificado + reads del resultado. |
| **Runtime helper nuevo** | `H->call_external_*` con cache de dirección. Mira `aot_registry`; si hay thunk, llama. Si no, lanza RuntimeError BP ("target X no es AOT — v1 limitación"). |
| **Validación AOT compile-time** | El check de #178 (validate-aot-in-compile) detecta cross-module call y verifica que el módulo importado existe en outDir. Si su `.bpi` indica que la función no es `native`, el compilador puede avisar: "X.foo es BP interpretada — el AOT fallará si la llamas desde código native". |
| **Sample test** | Dos módulos: `Math` con `native function sqrt(x)`, `User` con `native function compute(x)` que llama `Math.sqrt(x)`. Verificar end-to-end. |

## 7. Por qué quedó en v2 inicialmente

La discusión original encontró 3 escollos:

1. **Signatures**: cómo sabe AotCEmitter qué tipo tiene `B.foo(x)`. **Resuelto**: lo lee de `B.bpi` igual que MivmEmitter.
2. **`extern` C symbols**: para call directo C→C necesitaríamos
   símbolos externos en .o, que rompe el modelo `.mdn` (PIC).
   **Resuelto**: NO usamos call directo C→C. Todo pasa por el helper
   runtime que mira el registry. El `.mdn` mantiene su position-
   independence.
3. **Cross-module a través del intérprete**: re-entrar interp desde
   C. **Resuelto en parte**: restricción v1 = ambos lados AOT, no
   hay nested interp. El caso BP-target queda para v2.

Con esos 3 escollos resueltos, **#169 vuelve a ser viable y limpio**.
No urge para v1, pero si surge hueco después de H2+H4, es ataque
mucho más enfocado que cuando lo deferimos.

## 8. El opcode-puente native → BP  *(apunte del usuario, 2026-06-02)*

**Problema recurrente** (nos ha mordido varias veces): una función
**native** (AOT, código C compilado) **no puede llamar a una función BP
interpretada**. Casos donde aparece:
- native que necesita devolver/usar **tuplas** (el constructor de tupla es
  BP: NEW_OBJECT + SET_FIELD).
- native → **método** de objeto (#174): el dispatch virtual es BP.
- el caso "target BP interpretado" de **#169** (§3-paso-4, §5): hoy
  restringido a "ambos lados AOT" justo por no tener esta pieza.
- cualquier helper de stdlib que quiera ser native pero llamar a otra
  función BP.

**Diagnóstico del usuario** (correcto): si desde el código native
"saltamos" a ejecutar la función BP, al hacer `RET` la VM creería que
sigue dentro de una función BP (restauraría pc/bp/cs del frame del
*caller BP imaginario*) y **no sabría volver al mundo native**.

**Solución propuesta — un opcode-puente entre los dos mundos.** Es la
misma idea que §3-paso-4 ("return sentinel") pero hecha explícita como
**opcode dedicado**, no como PC mágica. Mecánica:

1. El código native, para llamar a la función BP `f`, invoca un helper de
   runtime `aot_call_bp(vm, tc, f_abs_addr, args…)`.
2. El helper **monta un frame BP** para `f` en el stack del `tc`, pero
   pone como dirección de retorno guardada un **sentinela = el opcode-
   puente** (p.ej. `OP_NATIVE_RETURN`, en una celda conocida del code/heap).
3. El helper corre un **bucle de intérprete anidado** desde `f.pc`.
4. Cuando `f` hace `RET`, restaura pc = sentinela → el dispatch encuentra
   `OP_NATIVE_RETURN` → **rompe el bucle anidado** y devuelve el control
   (y el valor de retorno, leído del stack BP) al helper, que lo entrega
   a la función native C. **El opcode es el puente.**

**Precedente que lo valida** (ya en el código): `bpvm_thread_spawn`
(threading.c) arranca `run()` con un frame falso cuyo pc/bp/cs guardados
apuntan a `memory[0]`, donde vive un sentinela `THREAD_EXIT`; al volver
`run()`, ese sentinela termina el hilo limpiamente sin tumbar la VM. El
puente native→BP **generaliza ese patrón**: en vez de "terminar hilo", el
sentinela significa "volver al `aot_call_bp` que inició la interpretación
anidada".

**Consideraciones al implementarlo**:
- **Re-entrancia**: el bucle anidado corre sobre el MISMO `tc`. Cadenas
  native→BP→native→BP anidan sub-bucles, cada uno acotado por su
  sentinela. Hay que asegurar que el inner loop del intérprete es
  re-entrante (estado por-tc, no estáticos globales del loop).
- **Excepciones / boundary setjmp (#186)**: si la `f` BP lanza y no hay
  handler dentro de la sub-llamada, el throw debe propagar **a través**
  del frame native hasta un try/catch BP exterior. Componer con el
  `setjmp`/`longjmp` de #186 (que ya cruza native↔BP para faults).
- **Aditivo**: opcode nuevo ⇒ no rompe el contenedor `.mod` (mismo
  criterio que GET_FIELD_LONG, etc.). Solo lo emite/usa el runtime; el
  compilador no genera `OP_NATIVE_RETURN` en código de usuario.
- **Solo VM-C / paridad**: el gap es del mundo AOT, que es C-VM/MCU. La
  VM-Java interpreta los cuerpos `function native` como BP normal, así que
  allí native→BP "ya funciona" — la paridad se mantiene.

**Desbloquea**: el caso BP-target de #169, #174 (métodos desde native),
y tuplas/objetos devueltos desde native. Es la pieza runtime que faltaba.

### 8.1 Estado (P-aot-call-bp) y el WARNING de rendimiento

**Runtime — HECHO.** El puente está implementado y probado en la VM-C:
`OP_NATIVE_RETURN` (0xAA) + sentinela en `mem[1]`, contexto AOT por-worker
(TLS, espejo del fault-slot de #186), y `bpvm_aot_call_bp_i32` (frame falso +
`run_quantum` anidado). Expuesto en la tabla `aot_helpers_v1` como
`find_function` + `call_bp_i32`. Prueba manual: `samples/NativeBridge.bp` +
thunk a mano (`samples/out/aot_NativeBridge.c`) + `test/test_callbp.c` →
paridad byte-idéntica VM-Java (compute interpretado) == VM-C (compute via
thunk→puente→helper). Restricción v1: la función BP llamada no debe ceder al
scheduler (sleep/mutex-contended/join); bloqueo mid-puente bajo SMP → v2.

**Compile-time — PENDIENTE (cableado AotCEmitter).** Hoy la validación AOT
del frontend (#178) ABORTA con error cuando una `native function` llama a una
función no-native ("AOT: call a función no-native 'X'"). Con el puente eso ya
NO es ilegal. El follow-up:

1. Relajar esa validación de **error → permitido**: AotCEmitter emite
   `vm->aot_helpers->call_bp_i32(...)` (con `find_function` cacheado en un
   `static`, §4 Opción A) en vez de una llamada C directa, marshalling de args
   incluido.
2. **Emitir un WARNING (apunte del usuario, 2026-06-03):** cada llamada
   native→BP cruza al intérprete por el puente y **NO se beneficia de la
   velocidad AOT**. El compilador debe avisar para que el usuario sea
   consciente de la pérdida de rendimiento y pueda decidir hacer también
   `native` a la función llamada. Texto sugerido:

   > AVISO: la función native 'compute' llama a la función BP interpretada
   > 'helper' (línea 25). Esa llamada cruza al intérprete por el puente
   > native→BP y NO se acelera por AOT. Para máximo rendimiento, declara
   > 'helper' también como native.

   Es un WARNING, no un error: la llamada es correcta y a veces deseada
   (tuplas, métodos, helpers que no compensa portar a native).
