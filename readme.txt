# Proyecto BASICPLUS

Diseño de un lenguaje de programación moderno con raíces en BASIC: clases, propiedades, manejo de errores, módulos, herencia, miembros estáticos.

## Archivos del proyecto

- `basicplus_grammar.ebnf.txt` — gramática EBNF completa del lenguaje, con notación uniforme, reglas semánticas documentadas y un ejemplo completo al final.
- `README.md` — este archivo.

## Estado actual

**Gramática v1 — completa y consistente.** Una sola versión consolidada con todas las decisiones de diseño aplicadas y documentadas.

## Decisiones de diseño clave (resumen)

### Estilo y léxico
- Notación EBNF uniforme con `::=` y `;`.
- Palabras clave **case-insensitive**; identificadores **case-sensitive**.
- Asignación e inicialización: `:=` en todo.
- Igualdad / desigualdad: `==` / `!=`.
- Comentarios: `//` (línea), `/* */` (bloque).
- Literales: decimal, hex (`0x`), binario (`0b`), float con exponente, escapes `\n \t \r \\ \" \0`.

### Estructura del programa
- Módulos con `import`, definiciones a nivel módulo y clases.
- Visibilidad **privada por defecto**, `public` para exportar. Sin `protected`.
- **Inicializador de módulo**: función con el mismo nombre que el módulo se ejecuta al cargar.
- **Punto de entrada**: el módulo principal debe definir `function Main()`.

### Tipos y memoria
- Escalares (`integer`, `float`, `string`, `boolean`) **no nullables**.
- Referencias (clases, arrays) **nullables** con `null`.
- Arrays **0-based**, acceso fuera de rango lanza `IndexOutOfRangeError`.

### Clases
- Herencia simple (`extends`). Sin `protected` (subclase ve todo).
- Constructores estilo Python: función con el mismo nombre de la clase, sin `new`. Construcción `Perro("Rex")`.
- `this` para la instancia actual. `super(args)` para constructor padre, `super.miembro` para miembro padre.
- Sobreescritura: **virtual por defecto**, `final` para prohibir.
- **Miembros estáticos sin `static`**: se declaran con nombre cualificado `NombreClase.miembro` dentro del cuerpo de la clase.

### Propiedades
- Get/set implícitos con valor inicial opcional. Forma corta y forma extendida con `endprop`.
- Setter requiere parámetro explícito.
- `field` (palabra contextual) para acceder al backing field.
- Válidas a nivel módulo y a nivel clase.

### Control de flujo
- `if/elseif/else/endif`, `while/endwh`, `for := to step do/next` y `for in/next` (foreach).
- `do/loop [cond]` — bucle infinito o do-while según haya condición.
- `switch/case/default/endsw` — sin fall-through, múltiples valores por `case`.
- `break`, `continue`, `return`.

### Errores
- `try/catch/finally/endtry`.
- `catch err: Tipo`, `catch err` (cualquier tipo) o `catch` (sin enlazar). El catch-all debe ir el último.

### Operadores
- Aritméticos, lógicos (`and`/`or`/`not`/`xor` con **corto-circuito** en `and`/`or`).
- Bitwise: `shl`, `shr`, `&`, `|`, `xor`, `mod`.
- Comparación: `==`, `!=`, `<`, `>`, `<=`, `>=`.
- Asignación compuesta: `+=`, `-=`.

### Enums
- Estilo C: lista de nombres con valor entero implícito (incrementa desde 0).
- Valor explícito sobreescribible (`MIEMBRO := 10`).
- Usable como tipo y en `switch`.

## Roadmap v2 (deferido)

Documentado dentro del propio archivo de gramática. Incluye: interpolación de strings, alias de import, type aliases, tuplas, lambdas, mapas nativos, genéricos, interfaces, asignaciones compuestas extra, rangos en `case`, lectura de stdin.

## Próximos pasos sugeridos

1. **Lexer** (en C# o Java, ya elegido) — analizador léxico que convierte código BASICPLUS en tokens.
2. **Parser** — produce un AST a partir de los tokens.
3. **Análisis semántico** — chequeo de tipos, visibilidad, reglas de constructor / `this` / `super` / `final` / `null` / estáticos.
4. **Intérprete o compilador** — ejecución directa o generación de código.

## Cómo retomar

Abrir `basicplus_grammar.ebnf.txt` y leer:
- Las secciones marcadas como **regla semántica** documentan reglas no expresables en EBNF.
- El bloque "EJEMPLO COMPLETO" al final ilustra la mayoría de características en un único módulo.
- La sección "ROADMAP V2" lista lo deferido.