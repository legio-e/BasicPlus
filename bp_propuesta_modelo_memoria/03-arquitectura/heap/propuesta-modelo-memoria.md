# Propuesta de diseño — Nuevo modelo de memoria de BasicPlus (handles + generación)

**Estado:** propuesta lista para implementar. Núcleo **validado en software**
(maqueta); las fases de hardware están marcadas. **Para:** Claude-Code.
**Origen del análisis:** [`objetivo-modelo-memoria-smp.md`](objetivo-modelo-memoria-smp.md)
· **Maqueta de referencia ejecutable:** [`../../02-verificacion/maqueta/`](../../02-verificacion/maqueta/README.md).

---

## Paquete de handoff (qué entregar a Claude-Code)

| Nivel | Artefacto | Papel |
|---|---|---|
| **Leer primero** | este documento | la especificación (qué construir) |
| **Tener y EJECUTAR** | [`../../02-verificacion/maqueta/`](../../02-verificacion/maqueta/README.md) — `Maqueta.java` + `MaquetaGc.java` | **especificación ejecutable** + tests de aceptación (0 corrupciones; 9/9 aserciones) |
| **Para profundidad** | [`objetivo-modelo-memoria-smp.md`](objetivo-modelo-memoria-smp.md), [`../../hallazgos/`](../../hallazgos/), [`../../01-especificacion/semantica-owner.md`](../../01-especificacion/semantica-owner.md) | el porqué, las decisiones abiertas, los casos a batir |

**Sobre las maquetas:** son **modelo/prototipo en Java, NO código de producción**. NO
copiar dentro de la VM — modelan la *lógica* aislada. Sirven para (a) desambiguar el
diseño leyendo código que funciona, y (b) definir "qué pasa" como arnés de aceptación
(adaptarlas como tests del código real). Validan corrección lógica **en x86**, no las
barreras ARM (fase de hardware). Definición de "hecho": el código real (miVM + VM-C)
pasa equivalentes de estos tests, con paridad byte-idéntica.

---

## 0. Para el implementador (TL;DR)

- **Qué cambia (el corazón):** una referencia BP deja de ser una **dirección plana**
  en `memory[]` y pasa a ser un **HANDLE** = `(índice, generación)` a una **tabla de
  handles**. Todo deref pasa por la tabla.
- **Por qué:** robustez bajo **SMP real de 2 cores** (alloc + `owner` + GC pueden
  crear/destruir a la vez), contrato de use-after-free **exacto**, e independencia de
  ubicación para la jerarquía **RAM/PSRAM/Flash**.
- **Principio rector (no romper):** la VM es un autómata; **el compilador ya emite lo
  que hace falta** (seguimiento de `owner`, `FREE_REF` antes del `RET`, etc.). Lo que
  cambia es **cómo la VM interpreta una "referencia"**, no la inteligencia del front.
- **Tres invariantes sagrados** (§5): (1) liberar es **generation-checked**;
  (2) la memoria **solo se reclama/reusa en un safepoint**; (3) publicar una referencia
  lleva **barrera release/acquire** en el slot.

---

## 1. Motivación (resumen)

El modelo actual (un `memory[]` plano, interp lock-free) se corrompe bajo paralelismo
real (bug **B1**), tiene huecos de `owner`/GC (H-006/07/09/10) y no aprovecha la
jerarquía de memoria. Detalle y evidencia en `objetivo-modelo-memoria-smp.md`.

---

## 2. El modelo

### 2.1 Tabla de handles
Estructura central, **hot y pequeña → vive en RAM**. Un array de slots; cada slot:

```
slot {
  addr   : dirección física del objeto (RAM / PSRAM / Flash — puede cambiar)
  gen    : contador de generación (u32)
  flags  : {live, pinned, ...}
}
```

Los objetos (cabecera + payload) viven donde toque (RAM/PSRAM/Flash); el slot es su
identidad estable. Mover un objeto = actualizar `slot.addr` (habilita compactación
sin pinning general).

### 2.2 Handle (formato) — **decisión abierta, §9**
`handle = (índice, generación)`. Empaquetado a decidir:
- **32 bits** (p. ej. 20b índice + 12b gen): las referencias siguen ocupando 4 bytes
  → **`.mod` y layout de pila intactos**, pero límite de ~1M objetos y gen de 12 bits
  (riesgo de wraparound, §9).
- **64 bits** (32+32): sin esos límites, pero las referencias pasan a 8 bytes → toca
  pila/formato.

> **✅ DECISIÓN (Eduardo, 2026-07-05): 64 bits (32b índice + 32b gen).** V4 va de
> *asegurar* lo hecho con soluciones sólidas y duraderas, **sin parches**; se acepta
> tocar el código generado y el `.mod`. El coste (refs 4B→8B: memoria + algo de tiempo)
> se paga a cambio de **fiabilidad**: nada de errores mudos que corrompan memoria — si
> hay un fallo, que **salte y grite**. Con 32b de generación el wraparound deja de ser
> un punto de corrección (≈imposible en la práctica) y **se elimina su maquinaria**
> (retiro de slots, épocas). Reutiliza el carril de 8 bytes de `long`/`double` (H1.2).
> Ver §9.

### 2.3 Operaciones (pseudocódigo — la maqueta las implementa y valida)

```
alloc(kind, size):
    reservar objeto físico (arena del core; ver §4)
    inicializar cabecera + payload         # (escritor)
    s = tomar un slot libre
    slot[s].addr = dir ; slot[s].live = true
    PUBLICAR: store-release slot[s]         # barrera (§4.1)
    return handle(s, slot[s].gen)

deref(h) -> dirección:                      # en GET_FIELD/SET_FIELD/ALOAD/INVOKE_VIRTUAL/…
    s = index(h)
    g = load-acquire slot[s].gen            # barrera (§4.1)
    if !slot[s].live || g != gen(h): throw "referencia a objeto eliminado"   # contrato B
    return slot[s].addr

free(h):                                    # owner-free / FREE_REF — GENERATION-CHECKED
    s = index(h)
    if !CAS(slot[s].gen, gen(h), gen(h)+1): return   # stale/doble → NO-OP seguro (§5)
    for cada campo OWNER del objeto: free(campo)      # cascada (owner_bitmap del descriptor)
    encolar s para reclamar en el safepoint           # reclamación DIFERIDA (§4.3)

gc():                                       # PRECISO, corre en safepoint (STW)
    marcar desde raíces (pilas + globales de módulo), trazando handles por la tabla
    para cada slot vivo NO marcado: bump gen ; encolar para reclamar
    # type-agnostic: NO se mira el tipo del objeto al trazar

safepoint():                                # el único punto donde se reclama/reusa
    parar el mundo (los 2 cores en un punto seguro)
    drenar la cola de reclamación: liberar el objeto físico ; slot -> libre
```

### 2.4 Los tres actores, unificados
`alloc`, `owner`-free y `gc` comparten la **misma autoridad** (la tabla/heap) y la
**misma reclamación diferida**: liberar (por `owner` o por GC) es "marcar + encolar";
la reclamación física ocurre **solo en el safepoint**. Así `owner` y GC dejan de
competir: son el mismo paso.

---

## 3. La migración: de referencias planas a handles

### Qué CAMBIA (en la VM-C y la VM-Java)
- **Todo opcode que desreferencia** resuelve el handle primero (via `deref(h)`):
  `GET_FIELD`, `SET_FIELD`, `SET_FIELD_OWNER`, `ALOAD*`, `ASTORE*`, `INVOKE_VIRTUAL`,
  `INSTANCEOF`, `FREE_REF`, `THROW` (lee el objeto excepción), `PRINT_STR`, etc.
- **El allocator** (`NEW_OBJECT`, `NEWARRAY*`) devuelve **handles**, no direcciones.
- **`FREE_REF` / `SET_FIELD_OWNER`**: pasan a ser **generation-checked + diferidos +
  con cascada real** (hoy `FREE_REF` marca un bit y no cascadea → H-006/H-010).
- **El GC**: de mark-sweep conservador sobre `memory[]` a **preciso vía la tabla**.

### Qué NO cambia
- **El formato `.mod`** (si el handle es de 32 bits): una referencia sigue siendo un
  valor opaco de 4 bytes en la pila; solo cambia **cómo la VM lo interpreta**.
- **El lenguaje** y **el compilador**: el front sigue emitiendo el mismo bytecode,
  incluido el seguimiento de `owner` y los `FREE_REF` antes de cada salida de función
  (ya lo hace bien; ver `03-arquitectura/heap/destruccion-owner-compilador-vm.md`).
- **La paridad** como regla sagrada (§7).

### El coste de la indirección, resuelto
- **Interpretado:** el deref extra (tabla + comparar gen) se ahoga en el coste
  por-opcode → despreciable.
- **`native`:** resolver el handle **una vez** en el borde de la llamada y operar con
  **puntero crudo** sobre el objeto **fijado (pin)**; los kernels nativos no alocan →
  puntero estable. Sin indirección por acceso en el bucle caliente.
- **Seguro por el modelo:** como nada se reclama fuera del safepoint, un puntero
  resuelto es estable **durante todo el quantum** → se puede cachear.

---

## 4. Concurrencia (SMP real, 2 cores)

### 4.1 A1 — publicación segura (release/acquire)
Al **publicar** una referencia (escribir el slot), el objeto debe estar **totalmente
inicializado antes** (store-**release**); al **leer** el slot en `deref`, garantizar
ver el objeto completo (load-**acquire**). Es el apretón de manos que **ya vive dentro
de un mutex**. El slot de la tabla es el **único punto de publicación** → la barrera va
ahí, una vez.
- Java: `volatile` / `VarHandle` release-acquire (o `AtomicIntegerArray` sobre `gen`).
- C/ARM: atomics C11 `memory_order_release`/`acquire` → barreras `dmb`.
- ⚠️ **Fase de hardware:** validar en la VM-C sobre ARM real (x86 lo esconde).

### 4.2 A2 — objetos thread-safe
El programador de BASIC no carga con la sincronización: se ofrecen **objetos
thread-safe** de librería (estilo `java.util.concurrent`; ya existe `SyncList`) +
primitivas (`Mutex`/`synchronized`/`sync`) para el experto.

### 4.3 Reclamación diferida + safepoint
Ningún objeto se reclama/reusa mientras los mutadores corren. `owner`-free y GC solo
**marcan + encolan**; el **safepoint** (stop-the-world entre quanta, maquinaria que ya
existe) reclama. Sincronización barata con 2 cores: **una arena de alloc por core**;
la reclamación es el único punto que coordina.

---

## 5. Invariantes de corrección (la lista que NO se puede violar)

1. **Liberar es generation-checked.** Solo la **primera** liberación de un handle vivo
   tiene efecto (`CAS(gen, g, g+1)`); stale/doble = **no-op**. *(Sin esto: doble-free →
   doble-alloc → corrupción. La maqueta lo demostró: 22.432 corrupciones → 0.)*
2. **La memoria solo se reclama/reusa en un safepoint**, nunca en el sitio mientras
   corren mutadores.
3. **Publicar lleva release; deref lleva acquire.** (A1.)
4. **`deref` valida la generación** antes de tocar el objeto → contrato B exacto.
5. **La cascada `owner`** recorre el `owner_bitmap` del descriptor **en runtime** (no
   la puede emitir el compilador por polimorfismo) y es recursiva.
6. **El GC es preciso y type-agnostic** (traza handles, no mira el tipo).

---

## 6. Lo que el modelo hace IMPOSIBLE por construcción

No se "arreglan"; no pueden ocurrir:
- **H-007** (UAF por alias de `owner`) y **H-010** (bloque liberado inconsistente):
  generación + reclamación diferida + free chequeado.
- **H-006/H-009** (cascada, coexistencia owner↔GC): unificados en el paso diferido.
- **H-011/H-008** (long[]/double[] invisibles / raíces falsas): GC preciso y
  type-agnostic → sin falsos negativos/positivos por tipo.
- **H-012** (raíces de módulo no escaneadas): un global es una raíz que sostiene un
  handle como cualquier otra.

---

## 7. Paridad dual-VM (regla sagrada)
Mismas reglas explícitas en la VM-Java y la VM-C → misma salida observable, incluidos
los fallos "referencia a objeto eliminado". Nada puede depender de la coherencia del
hardware ni del scheduling. La independencia de ubicación (handles) permite que un
objeto viva en distinto nivel (RAM/PSRAM/Flash) en cada VM **sin** romper la paridad.

---

## 8. Coste y mitigaciones
Ver §3 (indirección: despreciable en interp, puenteada en native). Overhead de la
tabla ~8 B/objeto vivo, en RAM (su sitio). Medir con el spike (C7,
[`../../02-verificacion/spike-overhead-handles.md`](../../02-verificacion/spike-overhead-handles.md)).

---

## 9. Decisiones abiertas (cerrar con el usuario / medir antes de fijar)
1. **Uniforme vs. híbrido** — *lean: uniforme* (todas las refs = handles). El coste que
   lo lastraba se puentea (§3); confirmar con el spike.
2. ~~**Anchura del handle** (§2.2)~~ → **✅ CERRADA (Eduardo, 2026-07-05): 64 bits
   (32b índice + 32b gen).** Se acepta el coste (refs 4B→8B, toca pila/`.mod`/ABI native)
   a cambio de fiabilidad y de **eliminar** el wraparound como punto de corrección
   (32b de gen ≈ nunca wrappea → fuera la maquinaria de retiro de slots). Reutiliza el
   carril de 8 bytes de `long`/`double` (H1.2). Detalle y razonamiento en §2.2. Queda por
   medir sólo el coste de indirección (spike, decisión 1).
3. **Compactación (moving GC)** — **reabierta** por los handles (mover = actualizar
   `slot.addr`, sin pinning general; solo pin durante `native`). Ataca la fragmentación
   (H-005). ¿Se incluye en v1 o se difiere?
4. **Colocación en la jerarquía** — estática (por tipo) vs. dinámica (por temperatura).
5. **Arrays locales de tamaño fijo sin handle** (Eduardo, 2026-07-06) — un array de
   tamaño fijo que vive en el **frame local** (pila, no heap) NO tiene entrada en la
   tabla. Hoy (modelo plano) pasarlo como parámetro es trivial: se pasa su dirección
   cruda. Con handles no vale: una dirección de pila no es un handle y no hay slot que la
   represente. **Abierto:** ¿cómo se pasa/referencia un array local en el modelo de
   tablas? Opciones a estudiar: **(a)** slot transitorio en la tabla apuntando a la
   dirección del frame, invalidado (bump de gen) al hacer pop → mantiene "todo es handle"
   a costa de un alta/baja de slot por array local pasado; **(b)** copia / paso por valor
   → cambia la semántica si hoy es por referencia mutable; **(c)** refs no uniformes
   (handle para heap + ref-de-frame para locales) → rompe la uniformidad; **(d)** alocar
   TODO array en el heap → mata el problema pero pierde el array de pila barato y
   determinista. Decidir al implementar V4; ligado a la decisión 1 (uniforme vs. híbrido).

---

## 10. Estado de validación
| Criterio | Estado | Dónde |
|---|---|---|
| UAF / contrato B / reclamación diferida (C1-lógico, C2, C3) | ✅ software | `Maqueta.java` (1→16 hilos: 7,4M→0) |
| Cascada owner ≥2 niveles (C4) | ✅ software | `MaquetaGc.java` (9/9) |
| long[]/raíces de módulo (C5) | ✅ software | `MaquetaGc.java` |
| Barrera de publicación A1 en ARM (C1 completo) | ⬜ **hardware** | VM-C sobre ARM |
| Paridad dual-VM (C6) | ⬜ | ejecutar en ambas VMs |
| Coste de indirección (C7) | ⬜ | spike |

---

## 11. Fases de implementación sugeridas (para Claude-Code)
1. **Tabla de handles + `deref` + allocator que devuelve handles** (VM-Java primero, la
   de referencia). Migrar los opcodes que desreferencian (§3).
2. **Generación + contrato B** (`deref` valida; `free` con CAS — invariante 1).
3. **Reclamación diferida + safepoint** (invariante 2).
4. **`owner` cascada + `SET_FIELD_OWNER`** vía `owner_bitmap` (invariante 5).
5. **GC preciso** vía la tabla (invariante 6).
6. **A1 barreras** release/acquire (invariante 3) — y **validación en ARM**.
7. **Portar a la VM-C** manteniendo paridad byte-idéntica en cada paso.
8. *(Opcional)* compactación (§9.3).

En cada paso: **verificar contra la maqueta y la paridad dual-VM** antes de seguir. La
maqueta (`Maqueta.java` + `MaquetaGc.java`) es la **especificación ejecutable** del
comportamiento esperado.

---

## 12. Referencias
Diseño y evidencia: [`objetivo-modelo-memoria-smp.md`](objetivo-modelo-memoria-smp.md) ·
Maqueta y resultados: [`../../02-verificacion/maqueta/README.md`](../../02-verificacion/maqueta/README.md) ·
Hallazgos: [`../../hallazgos/`](../../hallazgos/) ·
Semántica `owner`: [`../../01-especificacion/semantica-owner.md`](../../01-especificacion/semantica-owner.md).
