# Semántica de `owner` — modelo de propiedad

**Foco:** especificación · **Fecha:** 2026-07-05 · **Fuente de la intención:**
manual §12 + aclaración de diseño (Eduardo, 2026-07-05). Documento vivo.

## 1. Intención de diseño

`owner` traslada al lenguaje la **composición** de un diagrama de clases (UML: el
rombo relleno). Cuando la relación de la clase A hacia la B es de **propiedad**
—B es "parte de" A— se marca con `owner`. La consecuencia que el modelo garantiza:

> **Si desaparece el propietario, desaparece el objeto poseído.**

Es decir, la vida de la parte está **acotada por** la del todo. Es lo contrario
de una asociación normal, donde B existe con independencia de A.

## 2. El invariante del modelo

| | Cardinalidad | Controla la vida |
|---|---|---|
| Referencia **owner** | **exactamente 1** por objeto | **Sí** |
| Referencia **normal** | **0..N** por objeto | No |

- Un objeto tiene **como máximo un** owner (composición: una parte pertenece a un
  solo todo).
- Puede tener **muchas** referencias normales (asociaciones): otras clases lo
  usan sin poseerlo.
- Solo el owner decide cuándo muere el objeto.

## 3. Reglas operacionales (manual §12)

- **`var owner x: T`** — al reasignar `x`, el valor anterior se libera (`FREE_REF`)
  **antes**; al salir de scope, se libera el valor actual.
- **Transferencia (`owner → owner`)**: transfiere la propiedad; la fuente queda
  `null` **sin liberar**. → preserva el invariante "1 owner".
- **Copia (`owner → normal`)**: copia la ref **sin transferir**; sigue habiendo un
  solo owner. → crea una de las N referencias normales.
- **Fields `owner`** + `owner_bitmap` en el descriptor → liberación en cascada al
  destruir la instancia (recursiva; ver estado real en [H-006](../hallazgos/H-006-freeref-cascada.md)).

## 4. La pregunta abierta que define la robustez del modelo

El invariante "1 owner controla la vida + N referencias normales" implica, por
construcción, que **una referencia normal puede sobrevivir al objeto** (el owner
lo liberó, la referencia normal sigue viva). El modelo **no especifica hoy qué
significa esa referencia tras la liberación**. Tres contratos posibles:

| Contrato | Qué hace un deref tras la liberación | Coste | Encaja con |
|---|---|---|---|
| **A. Responsabilidad del programador** (estilo puntero crudo C++) | comportamiento indefinido (UAF) | cero | "VM mínima"; **rompe** el pilar de *predecibilidad* de V2 |
| **B. Fail-fast / tombstone** ✅ | `FREE_REF` marca el bloque como lápida; el deref de una lápida lanza `RuntimeError` | bajo (un bit de tag + chequeo en deref) | predecibilidad; diagnosticable |
| **C. Auto-null / weak refs** | las referencias normales se anulan al liberar | alto (back-refs o tabla global) | seguridad total; **choca** con "VM mínima" |

> **✅ DECISIÓN (Eduardo, 2026-07-05): contrato B.** Al usar una referencia a un
> objeto ya liberado por su owner, se produce un error del tipo **"referencia a
> objeto eliminado"**. Es la opción que preserva la *predecibilidad* de V2 al
> menor coste (aprovecha el `tag` del header como lápida). Hoy el comportamiento
> efectivo es el A (UAF); B es el objetivo. Ver [H-007](../hallazgos/H-007-owner-aliasing-uaf.md).

## 5. Preguntas de especificación a cerrar

1. **¿Se garantiza estáticamente el "máximo 1 owner"?** Las reglas de
   transferencia lo preservan para variables `owner`, pero: ¿puede el compilador
   evitar que una referencia **normal** se reintroduzca en un slot `owner` de otra
   clase (creando un segundo dueño → doble free)? → verificar. **(Abierta — es el
   reverso de H-007; sin constancia de análisis previo, ver §6.)**
2. ~~**¿Cuál es el contrato del §4?**~~ → **Resuelta: contrato B** (§4).
3. **¿Vale `owner` cross-module?** `HECHO_V2 §L7` / `PENDIENTES L7` dicen que
   `owner` no aplica a *property de módulo* (solo a campos de instancia). ¿Es una
   limitación temporal o parte del modelo? → aclarar en la especificación.

## 6. Constancia previa (qué se registró en V2 y qué no)

Las **reglas** de asignación `owner` sí se decidieron y registraron:
- `manual.html §12.1` — reglas de usuario (transferir / copiar / liberar).
- `OPCODES.md §0x5F` (`FREE_REF`) y `§0x60` (`SET_FIELD_OWNER`) — nivel opcode.
- `PENDIENTES.md L7` — límite en property de módulo.

Lo que **no** quedó registrado es el **análisis de los modos de fallo** de esas
reglas: el dangling de una referencia normal tras liberar el owner (H-007), el
doble-free por reintroducir una ref normal en un slot owner (§5.1), y el contrato
de una referencia muerta (ahora fijado = B). Se documentó el *qué*, no el *qué
puede salir mal*.

⚠️ Además, la regla registrada y la implementación **divergen**: `OPCODES.md §0x5F`
describe `FREE_REF` como **recursivo** (cascada de owners), pero `B-freeref` (V4)
dice que solo libera la raíz → ver [H-006](../hallazgos/H-006-freeref-cascada.md).

## Relación con el GC

`owner` y el GC gestionan la **misma** memoria con políticas distintas. Las reglas
de coexistencia (un bloque `FREE` por `FREE_REF` que el escaneo conservador
re-marca como vivo) no están especificadas — ver el dossier de arquitectura
[`analisis-modelo-memoria.md §3-bis`](../03-arquitectura/heap/analisis-modelo-memoria.md)
(H-008, H-009). H-007 y H-009 son, en el fondo, **la misma colisión** vista desde
el lenguaje y desde la VM.
