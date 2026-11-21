# BasicPlus Frontend (Java)

Port a Java del lexer del lenguaje **BASICPLUS**. Espejo del proyecto C# de `..\lexer\`. Misma gramática, misma cobertura léxica, misma estrategia de errores acumulados.

## Estructura

```
lexer-java/
├── pom.xml
├── README.md
├── samples/
│   ├── hello.bp
│   └── fase1.bp
└── src/main/java/basicplus/frontend/
    ├── TokenType.java   (enum)
    ├── Token.java       (record)
    ├── LexerError.java  (clase)
    ├── Lexer.java       (analizador léxico)
    └── Main.java        (CLI demo)
```

## Requisitos

- **Java 8** o superior. El código evita features de Java 9+ (no usa `record`, `Map.of`, `var`, `String.repeat`, switch expressions ni pattern matching) para máxima portabilidad.
- **Maven 3.6+**.

## Cómo construir y ejecutar

```sh
cd lexer-java
mvn package                              # genera target/basicplus-frontend.jar
java -jar target/basicplus-frontend.jar samples/hello.bp
java -jar target/basicplus-frontend.jar samples/fase1.bp
```

Salida (extracto):

```
=== Tokenizando: .../samples/hello.bp (2168 caracteres) ===

-- Tokens (xxx) --
line:col   TYPE          lexema  =>  valor
----------------------------------------------------------------------
   1:1    MODULE        'module'
   1:8    IDENTIFIER    'Hello'  =>  Hello
   1:13   NEWLINE       '\n'
   ...
```

## Lo que cubre

- **Palabras clave case-insensitive** (`module ≡ MODULE ≡ Module`).
- **Identificadores case-sensitive** (`miVar ≠ MiVar`).
- **Literales numéricos**:
  - decimal: `42`
  - hex: `0xFF`, `0XFF`
  - binario: `0b1010`, `0B1010`
  - float con exponente: `1.5e-3`, `2.0E+10`
- **Strings con escapes**: `\n \t \r \\ \" \0`. Salto de línea sin escapar dentro del string → error.
- **Comentarios**:
  - línea: `// hasta fin de línea` (no consume el NEWLINE)
  - bloque: `/* ... */` (multilínea, sin anidar)
- **NEWLINE significativo** (`\n` o `\r\n`) — terminador de sentencias.
- **Operadores**: `:= += -= == != < > <= >= + - * / | & ( ) [ ] , ; : .`
- **Palabras-operador**: `and or not xor mod shl shr`
- **Errores acumulados** (no excepciones): el lexer continúa tras un error y reporta todos los problemas al final.

## Diseño en 30 segundos

`tokenize()` recorre el fuente carácter a carácter:

1. Salta espacios horizontales y comentarios (`//`, `/* */`).
2. Si ve `\r` o `\n` → emite un token `NEWLINE`.
3. Si ve una letra → escanea identificador y mira el diccionario de keywords (case-insensitive).
4. Si ve un dígito → escanea número (detecta hex/binario por prefijo `0x`/`0b`, float por `.` o `e`).
5. Si ve `"` → escanea string con escapes.
6. Si no, intenta operador/signo (primero los de dos caracteres, luego los de uno).
7. Si nada coincide → registra error y avanza un carácter para no quedarse atascado.
8. Al final emite `EOF`.

Las posiciones (`line`, `column`) se actualizan por cada `advance()`. Los `\n` y `\r\n` resetean la columna y suman a la línea.

## Diferencias clave con la versión C#

- **Naming**: `MODULE` (mayúsculas) en lugar de `Module` (PascalCase).
- **Token** es una clase final con campos `public final` y getters al estilo record (`type()`, `lexeme()`, `value()`, `line()`, `column()`).
- **Sin `out` parameters**: el método `KEYWORDS.get(...)` devuelve `null` si no encuentra; el caller comprueba.
- **Errores no excepciones**: igual que C#, lista mutable acumulando `LexerError`.

## Próximos pasos

Este proyecto solo trae el **lexer**. Para tener el frontend completo (paridad con la versión C#) habría que portar:

- `Ast.java` + `Parser.java` + `ParserError.java` + `AstPrinter.java`.
- `BpType.java`, `Symbols.java`, `Scope.java`, `SemanticDiagnostic.java`, `SemanticInfo.java`, `SemanticAnalyzer.java`.

El lexer es la pieza más estable, así que tiene sentido estabilizarla primero antes de seguir.
