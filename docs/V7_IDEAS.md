# V7 — ideas y diseño

> Documento abierto el 21-ago con la visión de Eduardo sobre el código nativo en packs.
> **Va en `docs/` y no en `notas/` a propósito**: `notas/` está en el `.gitignore` y por
> esa vía ya se perdió un fichero (el `SqlDemo.bpbuild`, reconstruido el 19-ago).

---

## Código nativo empotrable: cuántos, cómo publican su API, y si hablan entre ellos

### La visión, en palabras de Eduardo (21-ago)

> *«En Linux el núcleo del sistema te lo puedes hacer un poco a tu gusto, pero obliga una
> vez decidido qué va dentro a compilarlo todo. **Nosotros vamos en otra dirección:
> código independiente que ya sabemos recolocar en una dirección concreta.** Lo que hay
> que mejorar es **cómo se publica su API** y **cómo interactúa con el resto del
> sistema**.»*

Y el encargo: primer diseño en V7, **poniéndolo en marcha con LVGL**.

📌 **Precisión suya que cambia la unidad de cuenta**: la memoria y la identidad no son
*por pack* sino **por código nativo**. Un pack puede llevar más de uno.

### Lo que YA está construido (V5/H4) — y es más de lo que parece

El **punto de encuentro** de la tabla BIOS, dos ranuras:

```c
int         (*publica)(uint32_t marca, const void* tabla);
const void* (*busca)  (uint32_t marca);
```

Y su comentario ya declara la propiedad que Eduardo persigue, con LVGL nombrado:

> *«son LO ÚNICO que la VM sabe de todo esto. No conoce SQLite, ni LVGL, ni ningún driver
> de pantalla: sólo sabe guardar un puntero bajo una marca y devolverlo. **Por eso añadir
> un pack nuevo NO toca la VM**»*

- **Marca de 4 bytes** (`'SQLI'`, `'LVGL'`, `'PANT'`), comparada con `==`. Se eligió
  frente a un nombre para no depender de una convención que alguien escribiría mal.
- **Se ve funcionando hoy**: `pack: sqlite: API publicada como 'SQLI' — 17 simbolos, v1`.
- **`busca()` ES el mecanismo de que hablen entre sí.** La pregunta *«¿se podrían hablar
  unos con otros?»* está contestada en el mecanismo; lo que falta es la disciplina
  alrededor (ver abajo).
- **¿Cuántos? Hoy CUATRO**: `#define BPVM_PACK_MAX 4`, un array fijo de (marca, tabla).

### Lo que hay que mejorar, que es justo lo que dice Eduardo

**1. La API se publica como `const void*` — la VM no puede comprobar NADA.**
El contrato dice que la tabla *«DEBE empezar por su propia marca y versión»*, pero es una
**convención, no un chequeo**: `publica` acepta cualquier puntero. Si un pack publica una
tabla con otro layout, el que la consume salta a donde no debe.
⏭️ Lo barato y de mucho valor: que `publica` **lea esos primeros bytes y verifique** que
la marca declarada coincide con la que se pasa, y que la versión es la esperada. Convierte
la convención en reja, y encaja con el criterio que el proyecto ya aplica en tres sitios
(el gate del `.mod` `#284`, el `magic`/`version` de la propia BIOS, y el sello del `.npk`).

**2. Nadie ordena la carga.** Si LVGL necesita el driver de pantalla, hoy no hay nada que
garantice que el driver publicó antes.
⏭️ **Propuesta: resolver PEREZOSAMENTE, no al cargar.** Que cada uno haga `busca()` en su
primer uso y no en su arranque. Así el orden deja de importar y desaparece la necesidad de
declarar dependencias entre packs — que sería el camino de Linux, el que Eduardo
descarta. Es también más honesto: el fallo aparece cuando de verdad hace falta la pieza.

**3. Cuatro ranuras.** Subir el número es una línea, pero la pregunta buena no es cuántas
sino **si el array fijo es la forma correcta** cuando el catálogo crezca (SQLite, LVGL, el
panel, red…). Con 4 y comparación por `==` el coste es cero; conviene medir antes de
cambiarlo por algo con más ceremonia.

**4. La RAM, y la corrección de Eduardo.** Ver la nota de `notas/V6_IDEAS.md` sobre la
arena: la tercera vía ya está elegida y corriendo, pero **la unidad debe ser el código
nativo, no el pack**, y hoy la arena es singular y se llama `SQLite`.

### Por qué la dirección es distinta a la de Linux, y qué implica

Linux te deja elegir qué entra en el núcleo, pero **una vez elegido hay que compilarlo
todo junto**. Aquí el código nativo se compila aparte, se graba, y se **recoloca** — eso
ya funciona y está probado en placa (el `.npk` de SQLite, ARM y RISC-V).

⚠️ **Lo que esa elección hace caro, y hay que asumir**: sin enlazado global no hay quien
compruebe que las dos partes encajan. Todo lo que un enlazador te da gratis —firmas,
símbolos, versiones— aquí hay que **declararlo y comprobarlo en runtime**. De ahí que el
proyecto ya tenga cuatro rejas distintas (ABI del `.mod`, `magic`/`version` de la BIOS,
sello del `.npk` con `float_abi`, y la marca del punto de encuentro). El diseño de V7 es,
en el fondo, **decidir cuánta reja más hace falta** para que la API publicada sea tan
segura como una llamada enlazada — sin volver a compilarlo todo junto.

### Empezar por LVGL: por qué es un buen primer cliente

- Es **grande y ajeno**, como SQLite, así que vuelve a probar el camino completo.
- Tiene **dos** interlocutores naturales: la VM (que hoy le llama por builtins) y el
  driver de pantalla. O sea que ejercita `busca()` de verdad, que SQLite no hizo.
- Y ya hay una ficha que empuja en la misma dirección: **`#434`, desacoplar los eventos
  del lazo de LVGL**. Conviene mirarlas juntas.
