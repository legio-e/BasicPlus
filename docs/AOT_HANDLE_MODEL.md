# Modelo de memoria de handles en la frontera BP↔native (charla de diseño, 18-jul)

> Charla de diseño (método *analizar-antes-de-programar*). Fija el modelo que
> guía **#302** (puente native→BP handle-aware = el antiguo H4.b, §H4 de
> `V4_BACKLOG.md`). Desbloquea la familia entera: #301 (clic de Forms VM-C),
> `test-throwmsg`, `test-throwuser` y los 9 rojos de la suite.

## El invariante de partida (Eduardo): la dualidad BP-native

El **mismo AST** de una `function native` se compila de **dos** maneras: a
**bytecode** para la VM de BP, y a **código máquina** para el micro (ARM hoy,
ARM+RISC-V mañana). No es "código C": es una función BP con dos backends.

Consecuencia directa sobre la **cabecera** (paso de parámetros + retorno): si usa
referencias, esas referencias deben ser **las mismas** en ambos lados — y hoy son
de **8 bytes**, no de 4. Y es recursivo: vale en la cabecera *y* en cada sitio
donde ese código **llama** a otra función/método (ahí somos el lado de la
llamada). Por tanto:

> **La dirección física de memoria es algo interno, totalmente temporal — NO una
> vía de comunicación entre métodos.** (Eduardo)

## El principio, en una frase

Hay **UN solo ABI**, realizado de dos maneras (layout de pila de bytecode /
registros+pila de ARM). En ese ABI:

- El **handle** (64b: `[gen:32 | idx|TAG:32]`) es el **tipo de la referencia en
  la interfaz**. Cruza toda frontera (BP↔native, native↔native, native↔runtime).
- La **dirección plana** es la **representación temporal en la implementación**.
  Solo existe entre un `deref` y el siguiente *safepoint*. **Nunca** cruza una
  frontera.

Por qué el handle y no la dirección: un handle es una **identidad que el GC ve**;
una dirección plana no. De ahí, (1) pasar adelante sin reconstruir, (2) el
gen-check al derefiar **grita** en vez de UAF silencioso, (3) los handles vivos
pueden registrarse como **raíces del GC**.

## La regla de la dirección transitoria (safepoints)

Una dirección derivada vale **solo hasta el siguiente safepoint** = cualquier
cosa que pueda mover/liberar memoria: una **alocación**, un **GC**, o una
**llamada** que pueda hacer cualquiera de las dos. Regla para el emisor:
**re-derefiar el handle tras cualquier safepoint; nunca retener una dirección a
través de una llamada o una alocación.**

Esto **reconcilia con la velocidad del AOT**: dentro de una región **sin
safepoint** (bucle caliente que no aloca ni llama), el native **puede** cachear
el puntero crudo — es demostrablemente estable. El re-deref (una lookup barata)
solo ocurre tras llamadas/alocaciones, no en cada acceso.

## Las dos superficies (misma ley, distinto sitio)

1. **Frontera de función BP/native** — params y retorno. Handles de 8 bytes.
   Hoy roto: el puente `bridge_run_bp_frame` (interp.c) empuja cada arg a **4
   bytes** y **sin `bpref_regen`** → un ref llega con gen basura →
   "referencia a objeto eliminado".
2. **Helpers de runtime** (`array_load`, `new_object`, `print_string`…): native
   llamando a *servicios*, no a otra función BP. Hoy el contrato
   (`bpvm_aot_helpers.h`) es `uint32_t ref` y el comentario lo dice literal:
   *"el handle 'ref' es el offset al heap"* → es el modelo **pre-handles (V3)**
   que nunca se migró. Debe pasar a handle de 8 bytes y derefiar dentro.

## Las dos capas del arreglo

- **Capa 1 — representación / ABI**: los refs cruzan toda frontera como handle de
  8 bytes (no dirección plana de 4). Arregla lo **visible** (test-throwmsg,
  test-throwuser, #301), que fallan por **representación equivocada**, no por
  *cuándo* ocurre un GC.
- **Capa 2 — raíces de GC**: los handles que native retiene son raíces a través
  de llamadas que alocan. Evita el UAF si un GC ocurre **mientras** native tiene
  un handle vivo. Más profunda; solo muerde bajo presión de GC durante native.

### La asimetría de la dualidad (localiza la parte difícil)

En cada safepoint, lo único vivo son handles. ¿Dónde están?

- **Native interpretado** (→ bytecode): en la **pila de operandos BP** → el GC ya
  la recorre → **raíces gratis**. Este lado no necesita nada de la Capa 2.
- **Native compilado** (→ ARM/RISC-V): en **registros y pila C** → el GC **no**
  los ve → **no** son raíces automáticamente.

Así que la Capa 2 se localiza **exactamente en el native compilado**. Dos caminos:

1. **Shadow stack / frame de handles** — el código emitido mantiene sus handles
   vivos en un frame visible al GC alrededor de las llamadas. **Agnóstico del
   backend** (misma lógica para ARM y RISC-V), GC simple. *Preferido* por
   coherencia con "mismo AST, dos backends sin casos especiales" (portabilidad /
   soberanía). Coste solo en los safepoints, no en el bucle caliente.
2. **Stack maps** — el emisor registra por safepoint dónde están los handles
   vivos; el GC camina la pila con el mapa. Más rápido, pero específico del
   backend y con un GC más listo.

## Secuenciación (Eduardo: cimientos hacia arriba, evitar conflicto AOT-inline)

El puente y la convención de llamada son **compartidos** entre inline y AOT. Se
arregla el **inline primero** para que el AOT se apoye en un cimiento asentado y
verificado (oráculo interpretado + paridad dual-VM), no co-evolucionando.

1. ✅ **HECHO (79ab1b9 + 541db61)** — **Inline, representación-correcta**:
   `bridge_run_bp_frame` pasa refs como handle de 8 bytes + `bpref_regen`.
   Greeneó #301 (builtin GUI → función BP) y, con las raíces GC del GUI, cerró
   #20 del censo. Cimiento y oráculo.
2. ✅ **HECHO (bf42bed)** — **AOT, representación-correcta**: `aot_helpers v1→v2`
   (refs = handle EMPAQUETADO, deref dentro; slots read_ref/write_ref;
   call_bp/call_method con ref_mask+ret_is_ref) + el emisor (thunk 8B por
   ref según occupies8Bytes; ref_mask desde la firma del callee) + bump
   MDN_ABI_VERSION→2. **Los 9 rojos VERDES** (throwmsg/throwuser/method/callbp/
   bytenat/xmodule/xmodnat/xmethodnat/compressnat), paridad 17/1/0, GUI intacto.
3. 🔴 **PENDIENTE — y el argumento del aplazamiento QUEDÓ REFUTADO el
   16-ago-2026** (`make test-aotgc`, test rojo que es el criterio de
   aceptación): en V4 el GC corre DENTRO de `bpvm_heap_alloc` (#357) —también
   cuando aloca un helper llamado desde native— y recicla de verdad. Un
   intermedio cuyo único handle vive en un temporal de C se recolecta en mitad
   de la expresión: `"valor " + intToString(n)` imprime NULs con status=OK,
   mientras el control interpretado (mismo GC agresivo) imprime bien. O sea que
   NO es «solo para AOT-en-placa»: es host también. El párrafo siguiente se
   conserva como estaba, para el registro de por qué se creyó lo contrario:
   *(texto original)* Capa 2, solo para AOT-en-placa; el lado inline y
   el AOT-en-host la tienen gratis (el native corre síncrono dentro de un quantum
   sin GC asíncrono y F2 no compacta). Se empareja con la fase de AOT-en-device.

## Disciplina de ABI (decisión de implementación, alineada con #284)

`aot_helpers_v1 → v2` es **romper ABI** (el `.mdn` referencia helpers por firma
versionada). Se trata como el gate de formato de #284: **bump de versión + el
loader del `.mdn` RECHAZA** un `.mdn` de ABI vieja con mensaje accionable, en vez
de correr con firmas de 4 bytes y corromper. Un contrato versionado más que grita.

## Criterio de VERDE

`test-throwmsg` + `test-throwuser` pasan · los 9 rojos verdes · #301 (clic de
Forms) sin UAF en las 2 VMs · paridad dual-VM intacta · sin regresión en el corpus
de host (el AOT-en-placa / Capa 2 se verifica en su propia fase).

## Política (Eduardo, 18-jul)

Cualquier bug **nuevo** de la familia "native→BP con ref" se **APARCA** en #302,
**no se parchea** — para no acumular parches duplicados de la misma raíz.
