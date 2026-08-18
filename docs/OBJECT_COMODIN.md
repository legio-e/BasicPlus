# `Object` como comodín — decisión de diseño

**Estado:** decidido por Eduardo el 13-ago-2026. **Implementada la mitad estática el
14-ago** (H9): `Object` es la raíz real, subir es implícito y bajar se escribe con el
nombre del tipo — `Cosa(o)`, `string(o)`. **Falta**: que esa conversión COMPRUEBE en
ejecución (#389, sigue abierto; toca las dos VMs y el AOT) y la clase contenedora, que
no se ha escrito.

⚠️ Una precisión que este documento tenía mal, y que cambió el arreglo: `Object` **no
era** "un alias de `any`" a secas. La clase existía desde H5.1.a con su vtable
(`toString` slot 0, `compareTo` slot 1) y toda clase de usuario colgaba de ella — pero
sólo en el EMISOR. Lo que faltaba era su símbolo en el analizador, y por eso el *nombre*
`Object` resolvía a `any`. La cita de `SemanticAnalyzer.java:1377` también quedó vieja:
tras #390 esa línea era la 1407, y hoy ya no existe.
**Relacionado:** #389 (el downcast se traga cualquier cosa), #52 (`instanceof` en
ejecución, ya hecho), #236 (tipos built-in cross-module).

---

## La pregunta

> «En Python todas las variables sin declarar tipo son de cualquier tipo. En Java
> hay un tipo que puede guardar diferentes tipos de valores. Nosotros no tenemos
> eso, pero en algún momento tendremos que abordarlo. ¿Podemos [tener] un tipo
> Object que pueda guardar enteros o long o float o double o string u Object?»

## La decisión

> «Un objeto con una referencia a otros objetos, envoltorios de escalares u
> objetos normales. Es para tener un comodín, **no tiene que ser rápido** (el que
> quiera rapidez que declare el tipo). Pero bueno, hay veces que estás
> programando y no sabes el tipo, y con estas variables puedes resolver el
> problema.»

O sea: **`Object` es SIEMPRE una referencia.** Un escalar entra envuelto. El
coste (una alocación por escalar) es aceptado a propósito, porque el comodín no
compite con el tipo declarado: compite con no poder escribir el programa.

---

## Lo que hay HOY (medido el 13-ago, no supuesto)

`Object` es un **alias de `any`** — `SemanticAnalyzer.java:1377`,
`case "Object": return AnyType.INSTANCE;` — y `any` es **un hueco de 64 bits sin
etiqueta**. Quien lo lee decide qué hay dentro. Resultado:

```
var o: Object := 5        →  5          OK (cabe tal cual: funciona por accidente)
var s: Object := "hola"   →  376        MAL — imprime el HANDLE, no la cadena
var d: Object := 2.5d     →  0          MAL — el double se pierde entero
List: add(7), add("texto")→  7   316    el 7 bien, la cadena otra vez el handle
```

Y el downcast no se comprueba (#389, `samples/ProbeMal.bp`): un DAO que dice
devolver `Cosa` y devuelve `Otra` imprime **504** — leyendo el slot de otra
clase. Basura plausible, que es la peor clase de fallo.

### Por qué no puede ser de otra forma sin tocar el GC

El GC distingue referencia de escalar por el **`field_bitmap` del descriptor de
clase** (`docs/HEAP_LAYOUT.md`): bit k=1 ⇒ el campo k es una ref y hay que
trazarlo. Eso se decide **en compilación y por campo**. Un valor NO lleva
etiqueta consigo, así que un hueco no puede saber qué le metieron.

De ahí que sólo haya dos formas de tener un comodín de verdad:

| | (A) referencia siempre, escalares envueltos | (B) valor etiquetado |
|---|---|---|
| GC | sin cambios: es un objeto normal | tendría que mirar el tag, no el bitmap |
| VMs | sin cambios | corazón de las dos |
| AOT | sin cambios | a revisar entero |
| Formato `.mod` | sólo cómo serializa `Object` | representación de valores |
| Coste runtime | una alocación por escalar | cero |
| `instanceof` para el downcast | **ya existe** (#52) | habría que definirlo sobre el tag |

**Elegida (A).** No sólo por barata: encaja con «la base es FINITA» y con que V4
sea la base y lo demás aditivo.

---

## Lo que ya está construido y no hay que inventar

- **Los envoltorios existen**: `Integer`, `Long`, `Double`, `Float`, `Boolean`,
  todas `extends Comparable`, en `bpstdlib/Collections.bp:67` y siguientes.
- **`instanceof` en ejecución** está hecho (#52), que es lo que necesita el
  downcast comprobado.
- **`any` se queda donde está**: interno, sin garantías, para que la VM meta
  escalares en `List`/`Map`. Deja de expresarse en fuente en cuanto `Object`
  deje de ser su alias. Esto es literalmente la distinción que dio Eduardo el
  10-ago: *«Object y any no son lo mismo; any es interno y se utiliza sobre todo
  para poder entrar escalares»*.

---

## Qué habría que hacer

1. **Separar los dos tipos en el analizador.** `Object` deja de ser
   `AnyType.INSTANCE` y pasa a ser la raíz del modelo de objetos.
2. **Envolver al asignar.** `Object := 5` mete un `Integer`; ídem long, double,
   float, boolean, string. Lo pone el compilador; el usuario no escribe el
   envoltorio. (Decidir si `string` se envuelve o ya es una ref utilizable tal
   cual — hoy es `TYPE_ARRAY_I8`, o sea que YA es una referencia.)
3. **Desenvolver al leer con tipo.** `var n: integer := o` comprueba y extrae.
4. **`Object → Clase` es un DOWNCAST y se comprueba**: diagnóstico en
   compilación cuando se sepa, y comprobación en ejecución que LANCE cuando no
   (estilo `ClassCastException`, sobre `instanceof`).
5. **Serialización.** Hoy `Object` viaja como `"any"` en la interfaz del `.mod`;
   tiene que viajar como `Object`. Es cambio de interfaz, no sólo del analizador.
6. **Censo de quién depende de la permisividad actual.** Medido: `Collections.bp`
   usa `Object` en 21 sitios (Map entero: `put/get/containsKey/remove/keyAt/…`),
   `Gui.bp` 4, `Core.bp` 1, y 9 samples. Ninguno es una firma nueva: son los que
   hay que hacer pasar por el modelo nuevo sin romperlos.

### Dos bugs propios que salieron midiendo, y que NO son el downcast

- `print` de un `Object` que lleva una cadena imprime **el handle** (`376`).
- Un `double` metido en un `Object` sale **0**.

Los dos son «número plausible en vez de fallo». Con (A) desaparecen solos —el
valor pasa a ser un objeto con su clase— pero conviene que estén escritos por si
se aborda antes alguna parte suelta.

---

## Refinamiento de Eduardo: envolver es trabajo de la LIBRERÍA, no del compilador

> «El objeto de dentro se podría asignar con un `set` sobrecargado, y que cada
> `set` se encargue de construir el objeto contenedor. En cuanto al `get`, podría
> devolver el objeto, y en el caso de los escalares necesitaría un `getValue()`
> adicional. Son ideas para **no tener que modificar el compilador con casos
> especiales**.»

Es la línea correcta, y además ahora es POSIBLE: la sobrecarga cross-module se
arregló el 13-ago (#387). Antes de eso, un `set` sobrecargado importado ni
siquiera resolvía bien.

### Dónde va ese `set`: NO en `Object`

`Object` es la raíz REAL y tiene vtable: `toString` en el slot 0, `compareTo` en
el 1, y los métodos propios de toda clase de usuario numeran **desde el 2**
(medido al cerrar #392). Añadirle dos métodos correría todos los slots — el mismo
mecanismo de #324, *«quitar un método de una base de stdlib CORRE los slots»*— y
obligaría a recompilar cada `.mod` existente.

Así que:

- **`Object` se queda como raíz mínima**, sin métodos nuevos. Cero coste de ABI.
- **El contenedor es una clase APARTE** con el `set` sobrecargado:
  `set(v: integer)`, `set(v: long)`, `set(v: double)`, `set(v: string)`,
  `set(v: Object)`… y cada uno construye su envoltorio. Escrito en BP, en la
  stdlib, sin un solo caso especial en el compilador.

Queda por decidir el nombre (`Box`, `Valor`, …) y si el contenedor guarda un
`Object` a secas o distingue «vacío» de «null».

### Lo que ese esquema NO puede cubrir solo

BP sobrecarga **por parámetros, no por tipo de retorno**. Así que `get()` sólo
puede devolver `Object`: sacar de ahí una `Cosa` sigue siendo un **downcast**, y
ahí es donde vive la comprobación de #389. La maquinaria ya está — `instanceof`
en ejecución (#52).

`getValue()` resuelve la mitad de los escalares; la otra mitad —los objetos
normales— la resuelve el downcast comprobado.

**Reparto final:** envolver = librería (sin tocar el compilador);
desenvolver con seguridad = lenguaje (con lo que ya hay).

---

## Lo que NO cambia

- El que declara el tipo no paga nada: esto sólo afecta a quien escribe `Object`.
- `any` sigue siendo interno y no se toca.
- Las VMs, el GC y el AOT no se tocan.

---

## Refinamiento del 18-ago: sin `Box`, y las sobrecargas van en `List.add`

> Eduardo: *«#438 la clase `Box`, de momento no la necesitamos. Nuestro comodín
> será `Object`. El punto 8, las listas, sobrecargamos el método `add`: habrá un
> `add(i:integer)`, `add(l:long)`, `add(f:float)`, etc. Los otros `list` igual
> (no sé si pueden heredar los `add`).»*

Es la misma decisión (A) de arriba —envolver, no etiquetar— con **menos
superficie**: en vez de un contenedor general con `set` sobrecargado, las
sobrecargas se ponen **donde de verdad se usan**. Nadie escribe `Integer(5)`.

### Lo que dice el código (leído, no supuesto)

**1. La pregunta de la herencia tiene dos respuestas distintas.**

| clase | ¿hereda los `add`? | por qué |
|---|---|---|
| `OwnerList` | **SÍ** | sólo declara `removeAndFree`; `add/get/set/length/remove` vienen de `List` (`SemanticAnalyzer`, *«el resto se heredan del baseClass List»*) |
| `SyncList` | **NO** | redeclara las cinco con las mismas firmas, **a propósito**: *«overrides explícitos para documentar que el método del subtipo se llama (con locking)»* |

O sea: `OwnerList` gana las sobrecargas gratis; en `SyncList` hay que replicarlas
—o dejar de redeclarar y perder esa documentación deliberada.

**2. El obstáculo que cancelar `Box` no quita, sólo mueve.** Una casilla de
`List` es un **handle**: `items` se hace con `__growRefArray`, se escribe con
`ASTORE_I64`, el parámetro se declara con `declareParamRef`, y el GC la traza
por el `field_bitmap`. Un `integer` **no cabe ahí**. Así que `add(i:integer)`
tiene que envolver — que es exactamente lo que `Box` iba a hacer. La decisión de
Eduardo no elimina el envoltorio: elimina que el USUARIO tenga que escribirlo.

**3. Los envoltorios YA EXISTEN y no hay que inventarlos.**
`bpstdlib/Collections.bp` tiene `Integer`, `Long`, `Double`, `Float` y
`Boolean`, todas `extends Comparable`, cada una con constructor desde el
primitivo, `value()`, `compareTo` y `toString`.

**4. El problema de capas tiene un patrón ya probado.** `List` la **sintetiza el
compilador** (`MivmEmitter.synthesizeListClass`) y se usa **sin import**;
`Collections` es un módulo importable. Pero `#248` ya resolvió justo esto para
`RuntimeError`: `Main.injectImplicitCoreImport` inyecta `import Core` **sólo si
el módulo usa excepciones**, *«así los módulos sin excepciones no ganan ninguna
dependencia»*. El mismo mecanismo, con la misma condición perezosa: inyectar el
import **sólo si se llama a un `add` con un primitivo**.

**5. Compatibilidad de ABI — cuál es la PRIMERA firma importa.** Por la regla de
`H5.a`, *la 1ª firma lleva el nombre pelado y el resto van mangleadas*. Si
`add(item: Object)` se queda como primera, el símbolo `add` sigue significando
lo de hoy y **ningún `.mod` existente se rompe**. Si se pone `add(integer)` la
primera, cambia la ABI de todo lo compilado.

### Lo que queda por decidir (es de Eduardo)

1. **Dónde viven los envoltorios.** ¿Se quedan en `Collections` (y el import
   implícito lo trae) o suben a `Core`, que ya viaja implícito? `Core` es más
   barato de resolver pero engorda lo que llega siempre.
2. **Cómo se saca el escalar.** BP no sobrecarga por tipo de retorno, así que no
   puede haber `get(): integer`. Sale `Object`, y de ahí `Integer(o).value()`
   con el downcast ya comprobado de `#389`. ¿Basta, o se quiere un
   `getInt(idx)` / `getLong(idx)` explícito —que sí se puede, porque se
   distinguen por NOMBRE, no por retorno?
3. **`SyncList`**: replicar las sobrecargas, o dejar de redeclarar los métodos.
4. **El alcance**: `integer`, `long`, `float`, `double`, `boolean`, `byte`… ¿los
   cinco/seis, o sólo los que se usan? Cada uno son dos entradas por clase.

### Lo que NO cambia

Ni el GC, ni las VMs, ni el AOT, ni el formato del `.mod`. Todo esto es
frontend + stdlib.

### Y la pregunta de Eduardo que lo cambia todo: ¿por qué `List` es sintetizada?

> *«¿Y por qué no pones `List` en el `Core` y deja de ser una clase sintetizada?
> Ya no hace falta que lo sea.»*

**El motivo por el que se sintetizaba ya no existe.** El comentario del emisor lo
dice: *«emitidas siempre… a cambio cualquier programa puede usarlas sin import
explícito»*. Eso hoy lo da el **import implícito perezoso** de `#248`, que ya
funciona para `RuntimeError` desde `Core`. Y si `List` viviera en `Core`:

- los envoltorios y `List` estarían en el mismo sitio → **el problema de capas
  desaparece**, no hay que inventar nada;
- las sobrecargas de `add` serían **BP normal**, sin tocar el emisor;
- `SyncList`/`OwnerList` heredarían o no según lo que se escriba, a la vista;
- se podrían **borrar ~600 líneas** de síntesis del `MivmEmitter`.

**Pero hoy no se puede, y por una razón concreta y medida (18-ago):**

```
var items: integer[] := __newRefArray(4)
this.items[this.size] := o     ← ACEPTADO sin una palabra (o es un Object)
return this.items[idx]         ← rechazado: «'integer' incompatible con 'Object'»
```

BP **no tiene arrays de referencias como TIPO**. Sólo existen los builtins
`__newRefArray` / `__growRefArray`, tipados como `integer[]`. La `List`
sintetizada se sale con la suya porque el emisor escribe `ASTORE_I64` (8 bytes)
a mano sobre ese array — un truco que el lenguaje no ofrece. Peor: la
**escritura pasa en silencio**, y una casilla de `integer[]` son 4 bytes contra
los 8 de un handle (ficha aparte; falta medir si trunca de verdad).

**Conclusión: la pieza que falta es un tipo `Object[]`**, con su
`newObjArray(n)` y carga/guarda de 8 bytes. Con él, `List` es BP normal, vive en
`Core`, las sobrecargas salen gratis, `Box` no hace falta y sobran ~600 líneas
del emisor. Sin él, hay que seguir sintetizando y meter la construcción del
envoltorio dentro del emisor, que es más código y más frágil.

**Es una decisión de alcance, y es de Eduardo**: `Object[]` es una pieza de
lenguaje en una versión que se está cerrando. La alternativa —sobrecargas
sintetizadas en el emisor— es fea pero contenida.
