# SQLite.pack — todo lo suyo, en una carpeta

Decisión de Eduardo (14-ago): *«todo el tema del SQLite.pack debería tener una
carpeta propia»*, y dentro va todo — fuentes, el nativo de cada familia y el
build. Antes estaba repartido entre `bpstdlib/` (la librería BP), `samples/` (el
ORM) y `notas/p4/` (el proyecto), y eso costaba dos cosas concretas:

- una librería que **convive con los ejemplos ECLIPSA a su propio pack** sin que
  nadie lo pida (el FS gana al pack, por diseño). El 13-ago la prueba del
  ORM-desde-el-pack no probaba nada por eso;
- y el pack **no se podía reconstruir desde un clon limpio**: los ingredientes
  vivían en carpetas ignoradas por git.

## Qué hay aquí

    SQLite.bp        la librería BP que envuelve el motor
    Orm.bp           el ORM que va encima
    SQLite.bpbuild   el proyecto del pack
    nativo/          los INGREDIENTES que no se pueden regenerar sin toolchain
    out/             el build (ignorado por git)

## La cadena, de arriba abajo

    sqlite-amalgamation-3530400.zip        de sqlite.org — SQLite 3.53.4
      (NO está en el repo: es de terceros, oficial y se identifica por versión)
            │  compilar con el toolchain de cada familia
            ▼
    sqlite.elf  (~653 KB ARM · ~1,5 MB RISC-V)      intermedio, no se guarda
            │  java basicplus.frontend.NpackBuild <elf> \
            │       <arm-cortex-m33|riscv32-esp-p4> <salida.npk>
            ▼
    nativo/sqlite.npk.ARMV8   464.127 B     ← el motor, con su tabla de relocs
    nativo/sqlite.npk.RISCV   618.168 B
    nativo/SQLite.mdn.ARMV8     3.180 B     ← el puente AOT (lo emite el
    nativo/SQLite.mdn.RISCV     4.054 B        compilador, pero necesita el
                                               toolchain cruzado para enlazar)
            │  + SQLite.bp y Orm.bp compilados, + Core.mod y Str.mod
            ▼
    out/SQLite.pack          1.122.304 B

**Son DOS por familia**: el binario (`.npk`) y el puente (`.mdn`). Los cuatro
están versionados a propósito, por la misma razón por la que lo están los `.mod`
de la stdlib: hacen falta para trabajar y **no todo el mundo puede regenerarlos**
—hacen falta dos toolchains cruzados—. Lo que no se guarda es lo que sale de
ellos (`out/`, y el `.elf` intermedio).

## Cómo se construye

    copy nativo\*.* out\
    java -jar lexer-java/target/basicplus-frontend.jar \
         --project bpstdlib/sqlite/SQLite.bpbuild --backend=mivm

El copiado NO es un descuido: el pack recoge del `outDir` los tipos
`mod`/`mdn`/`npk` (`PackStep.OUTDIR_TYPES`) y el compilador sólo genera los dos
primeros. El nativo es un ingrediente, no una salida.

⚠️ **El AOT es MUDO por línea de comandos**: si no puede generar los `.mdn` —por
ejemplo, sin toolchain— no dice nada, y el pack sale 8 KB más pequeño sin una
sola advertencia. Por eso los `.mdn` están en `nativo/`. Si tocas `SQLite.bp` o
`Orm.bp`, hay que **regenerarlos con toolchain** y actualizarlos aquí.

## Cómo se comprueba que un cambio no rompió nada

Reconstruir y comparar el pack con el anterior. Si sale de otro tamaño, falta
algo: la última vez faltaban `Core.mod`, `Str.mod` y los dos `.mdn`, y la
diferencia (16 KB) lo cantó antes de que nadie lo grabara en una placa.

Los `.mod` sí pueden cambiar de tamaño legítimamente cuando cambia el compilador
—al regenerar el 15-ago, `Orm.mod` pasó de tener `any` en su interfaz a tener
`Object` (#389)—; lo que no puede cambiar es **qué entradas** lleva el pack.
