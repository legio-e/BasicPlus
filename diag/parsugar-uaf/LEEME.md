# Repro determinista del hallazgo 34 — UAF en el azúcar `parallel/case`

**Qué reproduce:** `parallel/case/default/endpar` lanza un
`RuntimeError: referencia a objeto eliminado (use-after-free)` y las ramas `case`
no llegan a ejecutarse. El bloque `default` sí corre (por eso el síntoma es
«imprime el default y muere»).

## Cómo se dispara

    java -jar lexer-java/target/basicplus-frontend.jar diag/parsugar-uaf/MemThenPar.bp \
         --compile <dir> --backend=mivm
    cp samples/out/Core.mod <dir>
    cd <dir> && bpgenvm-c.exe --mem=786432 MemThenPar.mod    # VM-C  → UAF
    cd <dir> && java -jar miVM/target/bpgenvm-1.0.jar MemThenPar.mod   # miVM → UAF

**LAS DOS VMs FALLAN** ⇒ el defecto está en el **bytecode que emite el
compilador**, no en un runtime.

## Por qué hace falta agotar el heap antes

No hace falta *el heap lleno*: hace falta que los **slots de la tabla de handles
se hayan RECICLADO**. Un handle es `(idx, gen)` en 64 bits. Mientras nadie ha
reutilizado un slot, **todas las generaciones valen 0**, y un handle truncado a
32 bits *sigue funcionando por casualidad* — porque los 32 bits que se pierden
son justo los que valen 0.

En cuanto un slot se recicla, su `gen` sube a 1 y el handle truncado se detecta:

    [bpvm] UAF: idx=100  gen del handle=0  gen del slot=1  (slots en uso=101)
    [bpvm]   gen 0 con el slot en 1: el handle perdió sus 32 bits ALTOS.
             NO es una referencia caducada: es un TRUNCAMIENTO
             (un bpref_t de 64b guardado en 32 en algún sitio).

O sea: **el truncamiento está SIEMPRE ahí; agotar el heap sólo lo hace visible.**
Cualquier programa que recicle bastantes objetos y luego use `parallel/case`
puede toparse con esto. Por eso en placa la receta es:

    reset + MemInfo + paralleltest_sugar   →   error 11
    reset +           paralleltest_sugar   →   OK

## Familia

Es el mismo animal que **#369** (*el frame inicial del thread se quedó en 4
bytes*), **#281** (tuplas) y **#293** (objptr del GUI): un `bpref_t` de 8 bytes
guardado o leído como 4, de la campaña 4→8B. Lo que cambia es el sitio: aquí es
el camino del **azúcar** `parallel/case`, que fabrica **subclases anónimas de
`Thread`**. El gemelo `paralleltest`, con una clase **declarada por el usuario**,
NO falla.
