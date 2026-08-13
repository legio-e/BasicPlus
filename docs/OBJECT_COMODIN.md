# `Object` como comodín — decisión de diseño

**Estado:** decidido por Eduardo el 13-ago-2026. Sin implementar.
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
