# BasicPlus.Frontend (C#)

Frontend completo del lenguaje BASICPLUS en .NET 8: **lexer + parser + análisis semántico + AST anotado**.

A pesar del nombre del directorio (`lexer/`) y del proyecto (`BasicPlus.Lexer.csproj`), el contenido cubre las tres fases del frontend.

## Estructura

| Archivo | Propósito |
|---|---|
| `BasicPlus.Lexer.csproj` | Proyecto .NET 8 (Exe). |
| **Léxico** | |
| `TokenType.cs` | Enum con todos los tipos de token. |
| `Token.cs` | Token (tipo, lexema, valor, línea, columna). |
| `LexerError.cs` | Diagnóstico léxico. |
| `Lexer.cs` | Analizador léxico. |
| **Sintáctico** | |
| `Ast.cs` | Nodos del AST (records, marcadores `ITopLevelDecl`/`IStmt`/`IExpr`). |
| `ParserError.cs` | Diagnóstico sintáctico. |
| `Parser.cs` | Analizador sintáctico (recursive-descent). |
| `AstPrinter.cs` | Vuelca el AST en árbol indentado. |
| **Semántico** | |
| `BpType.cs` | Sistema de tipos resueltos (`PrimitiveType`, `ClassType`, `EnumType`, `ArrayType`, `NullType`, `VoidType`, `ErrorType`). |
| `Symbols.cs` | Tabla de símbolos: `ModuleSymbol`, `ClassSymbol`, `FunctionSymbol`, `PropertySymbol`, `VarSymbol`, `ConstSymbol`, `EnumSymbol`, `ParamSymbol` + `Scope`. |
| `SemanticDiagnostic.cs` | Diagnóstico (error o aviso) con línea/columna. |
| `SemanticInfo.cs` | Side-tables: tipos resueltos por expresión, símbolos por identificador, símbolo por declaración, diagnósticos. |
| `SemanticAnalyzer.cs` | Tres pases (declaraciones → tipos → cuerpos). |
| **Backend (Fase 1)** | |
| `IlEmitter.cs` | Compilador a IL .NET 8 con Mono.Cecil. Produce `.dll` + `.runtimeconfig.json`. |
| **Demo** | |
| `Program.cs` | CLI: lex → parse → analyze → opcionalmente compilar. |
| `samples/hello.bp` | Programa que ejercita el frontend completo (con clases). |
| `samples/fase1.bp` | Sample procedural compatible con el emisor IL Fase 1. |
| `smoke_test.py` | Réplica del lexer en Python (validación sin .NET). |
| `structure_check.py` | Verificador estructural (balance de bloques). |

## Cómo ejecutarlo

Requisito: **.NET 8 SDK**.

```sh
cd lexer
dotnet run                                       # samples/hello.bp completo
dotnet run -- archivo.bp                         # otro archivo
dotnet run -- archivo.bp --tokens                # solo tokens
dotnet run -- archivo.bp --ast                   # solo AST
dotnet run -- archivo.bp --quiet                 # solo diagnósticos semánticos
dotnet run -- archivo.bp --compile out/prog.dll  # compila a IL .NET
```

### Compilación a IL (Fase 1)

```sh
dotnet run -- samples/fase1.bp --compile out/fase1.dll
dotnet out/fase1.dll
```

El emisor genera dos archivos: `out/fase1.dll` (el binario IL) y `out/fase1.runtimeconfig.json` (necesario para que `dotnet` encuentre el runtime). La primera vez `dotnet restore` descargará Mono.Cecil de NuGet.

**Alcance Fase 1**: funciones top-level, primitivas (integer/float/string/boolean), locales/parámetros, aritmética/comparación/lógica con corto-circuito, `if`/`while`/`do-loop`/`for := to step`, `print`, `return`, `break`/`continue`, vars y consts a nivel módulo, inicializador de módulo + entry point en `Main`.

**Fuera de Fase 1** (avisa el emisor): clases, herencia, propiedades, miembros estáticos, enums, switch, try/catch, throw, this/super, arrays, for-in, null. Estas se añadirán en fases siguientes.

## Lo que comprueba el análisis semántico

### Declaraciones (pase 1)
- Construcción de la tabla de símbolos del módulo y de cada clase (separando miembros de instancia y estáticos).
- Detección de **nombres duplicados** en cualquier scope.
- Identificación de **constructor** (función con el mismo nombre que la clase) y **inicializador de módulo** (función con el mismo nombre que el módulo).
- Validación del prefijo estático: `var Mi.x` solo es válido dentro de `class Mi`.
- Prohibición de clases/enums anidados dentro de clases.
- `final` solo permitido en miembros de clase.

### Tipos y herencia (pase 2)
- Resolución de cada `TypeRef` a un `BpType`.
- Resolución de `extends` a una `ClassSymbol`.
- **Detección de ciclos de herencia**.
- Asignación de tipo a parámetros, vars, consts, propiedades y retorno de funciones.

### Cuerpos (pase 3)
- **Resolución de identificadores** por scope (locales → instancia (vía `this` implícito) → módulo).
- **Chequeo de tipos** en asignaciones, operadores, llamadas, retorno, condiciones, índices, literales de array.
- **Promoción numérica** integer → float donde aplica.
- **Concatenación de strings** con `+`.
- **`+=` y `-=`**: chequeo de tipos coherentes (numéricos o string para `+=`).
- **`null` solo en referencias**: error si se intenta asignar a escalar.
- **Igualdad** comparable (mismo tipo, numéricos entre sí, null vs referencia).
- **Visibilidad**: miembros privados accesibles solo desde la propia clase o sus subclases.
- **Reglas contextuales**:
  - `this` solo en método de instancia.
  - `super` solo en subclase y en método de instancia.
  - `super(...)` solo en constructor y como primera sentencia del cuerpo.
  - `field` solo dentro de `get`/`set`.
- **Constructores**: no admiten tipo de retorno; `return expr` prohibido.
- **Sobreescritura**: detección de violaciones de `final`, firmas incompatibles, retornos incompatibles.
- **Constructor heredado**: si la subclase no define constructor, se usa el de la base.
- **Switch**: detección de **valores duplicados** en cases; **exhaustividad** sobre enum (aviso si faltan miembros y no hay `default`).
- **Catch**: orden — `catch` sin tipo (catch-all) debe ir el último; los siguientes serían inalcanzables.
- **`break`/`continue`** solo dentro de bucle (o switch para `break`).
- **`for in`** requiere array; el iterador toma el tipo del elemento.
- **Código inalcanzable** tras `return` en el mismo cuerpo (aviso).
- **Punto de entrada**: aviso si no hay `Main`; error si `Main` tiene parámetros o tipo de retorno.

## AST anotado

El analizador NO muta los nodos del AST. Toda la información semántica vive en `SemanticInfo`:

```csharp
SemanticInfo info = new SemanticAnalyzer().Analyze(module);

info.ExprTypes[someExprNode]      // BpType resuelto de la expresión
info.ExprSymbols[someIdentExpr]   // Symbol al que resuelve un identificador
info.DeclSymbols[someDeclNode]    // Symbol declarado por una declaración
info.Module                        // ModuleSymbol del módulo procesado
info.Diagnostics                   // List<SemanticDiagnostic>
```

Las claves usan `ReferenceEqualityComparer.Instance` para identificar nodos por referencia (no por igualdad estructural de records).

## Validación sin .NET

Los smoke tests Python validan léxico y estructura del sample:

```sh
python3 smoke_test.py        # OK: 438 tokens, 0 errores
python3 structure_check.py   # OK: estructura balanceada
```

Para validar el análisis semántico hace falta compilar y ejecutar el C#.

## Próximos pasos

- **Intérprete o backend de compilación**: el AST + `SemanticInfo` proporciona toda la información necesaria para ejecutar o generar código.
- Mejoras del análisis: type inference más rico, comprobación de inicialización antes de uso, garantía de retorno en todas las ramas, control de flujo más fino.
