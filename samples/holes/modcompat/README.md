# modcompat — compatibilidad de módulos por interfaz (N6)

Guardia de regresión del modelo de compatibilidad de dependencias:

> Un módulo es compatible con la interfaz que necesitas si **implementa esa
> interfaz O una que la extiende** (más nuevo / superset). Si solo implementa una
> **ancestra** (más antiguo, le faltan cosas) → **incompatible**, y es **error
> fatal** (no se produce `.mod`).

Compilar en orden (interfaces primero); `$F` = `lexer-java/target/basicplus-frontend.jar`:

    java -jar $F samples/holes/modcompat/IBase.bp    --interface <dir>
    java -jar $F samples/holes/modcompat/IExt.bp     --interface <dir>
    java -jar $F samples/holes/modcompat/ImplExt.bp  --compile   <dir>
    java -jar $F samples/holes/modcompat/ImplBase.bp --compile   <dir>
    java -jar $F samples/holes/modcompat/UseOK.bp    --compile   <dir>   # COMPILA
    java -jar $F samples/holes/modcompat/UseBad.bp   --compile   <dir>   # ERROR + aborta

| Fichero | Rol |
|---|---|
| `IBase`    | `module interface` con `foo()` |
| `IExt`     | `module interface IExt extends IBase` (+ `bar()`) — interfaz más nueva |
| `ImplExt`  | `implements IExt` (foo+bar) — provee lo nuevo (superset) |
| `ImplBase` | `implements IBase` (solo foo) — provee solo lo viejo |
| `UseOK`    | `import IBase:ImplExt` → ImplExt cumple IExt que extiende IBase → **compila** |
| `UseBad`   | `import IExt:ImplBase` → ImplBase solo cumple IBase (ancestra) → **error fatal** |

Maquinaria: `Main.implSatisfies()` recorre la cadena `implements → extends`;
`verifyImplementsContract()` valida que un `implements` provea todas las firmas;
N6 (2026-06-07) hizo **fatal** el import incompatible (`ctx.totalErrors++` → aborta).
