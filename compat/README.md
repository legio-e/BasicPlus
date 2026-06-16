# compat/ — Arnés de no-regresión V3 → V2

Las herramientas que hacen **verificable** el principio rector nº 7 de V3
(`docs/V3_ROADMAP.md` §4): *un programa de V2 corre sin cambios en V3*. La
versión nueva ejecuta lo viejo; nunca al revés (como `.class` de Java 8 en el
JRE 17).

## Estructura

```
compat/
  compat.sh          el runner (gen | check)
  v2/
    bin/             CÁPSULA V2 — binarios congelados, INMUTABLES (no tocar):
                       basicplus-frontend.jar  (frontend V2)
                       bpgenvm-1.0.jar         (miVM V2)
                       bpgenvm-c.exe           (VM-C V2)
    golden/          GOLDENS capturados con V2:
                       <Modulo>.mod  + <Modulo>.out   (por sample del corpus)
                       Core.mod                        (dep de clases/excepciones)
                       opcodes_java.txt / opcodes_c.txt
```

## Uso

```sh
bash compat/compat.sh check    # GATE: V3 actual contra los goldens V2
bash compat/compat.sh gen      # recaptura goldens con la cápsula V2 (solo si V2 cambia — raro)
```

`check` sale con código 0 si **todo verde**. Hoy (V3 == V2 en código) debe darlo:
es la **línea base** que valida el propio arnés. A partir de ahí, **cualquier rojo
= regresión real de V3 contra V2**.

## Los tres frentes que verifica `check`

1. **Comportamiento** — cada `.mod` de V2 ejecutado en las VMs V3 da la **misma
   salida** que en V2 (filtrando banners), en los tres ejes: `V3-Java == golden`,
   `V3-C == golden`, `V3-Java == V3-C`.
2. **Opcodes** — los ids de opcode de V2 siguen **intactos** en `OpCode.java` y
   `bpvm_opcodes.h`; ids nuevos están permitidos (aditivo); mover/borrar uno, o
   que Java y C diverjan en un opcode compartido, es ROJO. (`OP_NATIVE_RETURN`
   existe solo en C: es un sentinela interno, no se emite — permitido.)
3. **Emisión** — el **frontend V3** emite el `.mod` **byte-idéntico** al golden V2
   para las mismas fuentes. Un `EMIT-DIFF` salta para que el cambio del emisor sea
   **consciente**, no accidental.

## Qué hacer ante un fallo

- **Comportamiento u opcodes en rojo** → regresión: V3 rompió algo de V2. Arréglalo
  (es justo lo que el arnés existe para cazar).
- **`EMIT-DIFF`** → el frontend V3 emite bytes distintos. Si es **accidental**,
  arréglalo. Si es **intencional** (mejora legítima del emisor, sin romper el
  loader), regenera el golden afectado de forma consciente y commitéalo.
- **La cápsula `v2/bin/` NO se regenera** — es la referencia inmutable de V2. Si
  algún día hay que recrearla, es desde el tag `v2.0`.

## El contrato sagrado vs el guardián

- **Loader (sagrado)**: la VM V3 *debe* cargar y ejecutar un `.mod` de V2 dando lo
  mismo. Inviolable → frentes 1 y 2.
- **Emisor (vigilado)**: el frontend V3 *puede* emitir distinto (opcodes nuevos,
  mejoras); el frente 3 solo avisa para que sea deliberado.

## Corpus

Definido en `compat.sh` (variable `CORPUS`): samples de `bpgenvm-c/samples/`
elegidos por cobertura de features V2 (escalares, `byte[]`/`long`/`double`, UTF-8,
OO, excepciones, casts). Se omiten automáticamente los que ya divergían dual-VM en
V2 (p. ej. `idxtest`, por un builtin del subconjunto VM-C). Ampliarlo: añadir el
nombre del `.bp` a `CORPUS` y re-`gen`. Pendiente (V3_BACKLOG): tanda multi-módulo
(herencia cross-module, `Net`, `Compress`).
