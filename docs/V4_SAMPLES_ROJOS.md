# 🏛️ HISTÓRICO — el censo de samples rojos del 15-jul-2026 (V4)

> **Esto NO dice el estado de hoy.** Es la foto de un día concreto: 343 samples,
> 225 verdes, 80 skip-HW, 36 rojos. Se marcó como histórico el 19-ago al limpiar
> el árbol para cerrar V5, porque tenerlo sin fecha visible invitaba a leerlo como
> si fuera actual — y para entonces ya se habían cerrado bugs que aquí salen vivos.
>
> **Dónde está el estado de verdad:** `docs/FICHAS.md`. El censo vigente de samples
> es el del 3-ago (247/262), y el arnés que lo produce es `compat/compat.sh check`.
>
> Se conserva porque el análisis por grupos sigue valiendo como método —y porque un
> censo viejo, fechado, es una referencia; sin fechar es una trampa.

---

## El censo, tal como se escribió el 15-jul


Arnés: compila cada sample y lo corre en LAS DOS VMs. Ya corregidos 4 fallos del arnés
(deps locales al compilar, estado compartido, colisión de nombres, .bpi ausentes).

⚠️ AVISO IMPORTANTE sobre el oráculo: **la miVM corre a 1 worker (#235). No puede tener
carreras. Para bugs de concurrencia NO es oráculo** — que ella pase no prueba nada.

---------------------------------------------------------------------------------------
## GRUPO 1 — DE VERDAD (8). Esto es lo que te toca mirar.
---------------------------------------------------------------------------------------

### A. Un HANDLE usado como ÍNDICE — solo VM-C          [el más nítido]
  synclisttest      C: ASTORE_I64: índice fuera de rango 1073742866 (length=0)
                    J: "OK: cuentas cuadran" — pasa entero
  n5_popblocking    C: ASTORE_I64: índice fuera de rango 1073741826 (length=0)
                    J: "OK: popBlocking recibió todos los items y en orden"

  1073742866 = 0x40000412   1073741826 = 0x40000002   BPVM_HANDLE_TAG = 0x40000000
  → Los dos "índices" son handles con el tag puesto. Y el array dice length=0.
  → Los dos son SyncList + concurrencia. synclisttest revienta en la FASE 2
    (productor + 2 consumidores); la fase 1 (2 productores x 1000) pasa limpia.
  → Huele a la pila de operandos en el bloqueo/reanudación de popBlocking.

### B. La sync property de módulo NO sincroniza en la VM-C
  modpropsync       C: tick final = 2017        J: tick final = 4000
  → La respuesta correcta es 4000 (2 bumpers x 2000). La VM-C se come ~la mitad.
  → El sample NO lo detecta: su assert es `v >= 0 and v <= 2*iters` → dice
    "OK: valor coherente" con cualquier número. Lleva mintiendo desde siempre.
  → La J da 4000 porque va a 1 worker, no porque el sync funcione.
  → Es EXACTAMENTE el constructo del bug #18 de hoy (Mutex de sync property de módulo).

### C. Segfault de la VM-C
  JsonDemo          C: muere tras "--- writeJson (compacto) ---". Sin diagnóstico. SIGSEGV.
                    J: excepción no atrapada cuyo mensaje es el NÚMERO 1879048196
  → 1879048196 = 0x70000004. Un mensaje que es un número = una ref leída como int.
  → Fallan las dos, distinto síntoma. Mismo olor que A.

### D. "No space in heap" con el heap casi vacío
  ConcatObjTest     C y J: No space in heap
  ToStrTest         C y J: No space in heap
  → El GC de la J imprime `bump_remain=259273` justo antes de decir que no hay sitio.
  → 259 KB libres y la alocación falla → pide un tamaño absurdo. ¿longitud leída de
    una ref? Misma familia que A y C. (O el sample necesita más heap: sin verificar.)

### E. Los dos coinciden — no es de VM, es del .mod
  ownertest         C y J: INVOKE_VIRTUAL slot 2 no resoluble en la cadena de herencia
  → Las dos VMs dicen lo mismo → el problema está en el .mod → compilador o sample rancio.

### F. Cuelgue en las dos
  ownerlistremove   C y J: TIMEOUT, las dos paradas en "estado heap antes de removeAndFree(1):"

---------------------------------------------------------------------------------------
## GRUPO 2 — RANCIOS (10). El lenguaje cambió, el sample no. NO son bugs: hay que ACTUALIZARLOS.
---------------------------------------------------------------------------------------
  regla del auto-super  →  threads, smp_fib_bench, smp_fib_bench_pico,
                           smp_heap_stress, smp_heap_stress_pico, smp_print_stress
       "el constructor de 'X' debe llamar a 'super(...)'"
  modelo #248 de exc.   →  trytest, fase3, fase4, stacktrace
       "solo se puede lanzar una instancia de Exception (#248)"

---------------------------------------------------------------------------------------
## GRUPO 3 — GAP-1 (10). Hueco DOCUMENTADO del subconjunto de builtins de la VM-C.
---------------------------------------------------------------------------------------
  trim2, trimtest (builtin 8)   idxtest (10)   fase5b (19)   floortest (31)
  SplitTest, fase5c (36)        iotest (64)    stdtest (66)  prompttest (77)
  → Conocido y decidido. Solo mathtest estaba anotado; los otros 9 no. Anotarlos.

---------------------------------------------------------------------------------------
## GRUPO 4 — CULPA MÍA (8). No los mires, son ruido de mi arnés / mis etiquetas.
---------------------------------------------------------------------------------------
  logapi, logapiv2        `module interface` — son interfaces, no programas ejecutables
  badlogger               test NEGATIVO: "no implementa 'log' requerida por LogApi" ← correcto
  stealthlogger           test NEGATIVO: "'log' debe ser public"                    ← correcto
  faultprop               necesita build AOT (.mdn) para que el fault sea "en native"
  mutextest, paralleltest_sugar, preempttest, CompressBench   NONDET, mi expect.tsv mal

  Ya corregidos y VERDES tras arreglar el arnés (eran 5 falsos rojos):
  ExcCatchTest, appwithfromimpl, diamondtest, hello, l2app
