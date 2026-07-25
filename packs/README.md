# packs/ — la biblioteca de packs que acompaña al IDE

Aquí viven los packs que distribuimos con BasicPlus. El compilador los mira al
resolver `import <Módulo> from pack <Pack>`, y desde el IDE se graban en la
placa (o en el micro simulado) desde el panel de Packs.

| Pack | Qué trae | Se construye con |
|---|---|---|
| `Stdlib.pack` | La librería estándar completa (26 módulos: Str, Collections, Json, IO, Math, Gui, Net, y los de hardware) | `bpstdlib/Stdlib.bpbuild` |

## Cómo se reconstruye

La librería **no** se empaqueta con un script aparte: es un proyecto BP normal,
con las mismas herramientas que cualquier programa de usuario.

```bash
java -jar lexer-java/target/basicplus-frontend.jar \
     --project bpstdlib/Stdlib.bpbuild --backend=mivm --prune-bpi
cp bpstdlib/out/Stdlib.pack packs/
```

(o **Project → Build Project** abriendo `bpstdlib/Stdlib.bpbuild` en el IDE).

El truco está en `bpstdlib/Stdlib.bp`: un módulo sin código cuyo único cometido
es **importar** toda la librería. Como el compilador arrastra los imports
transitivamente, compilar ese main compila los 26 módulos, y `"out": "pack"`
los empaqueta. **Al añadir un módulo nuevo a la librería hay que añadir su
import en `Stdlib.bp`** — si no, no entra en el pack.

## Dónde lo busca el IDE

`IdePrefs.packsDir`, configurable en el engranaje del micro simulado
(*Librería de packs*). Si no se configura, se autodetecta una carpeta `packs/`
junto al jar del IDE y, si no, la del directorio de trabajo — que es esta.

Un pack es **portable**: el mismo fichero vale para las 3 familias de micro
(el bloque de 8 KB del formato es múltiplo del borrado de todas ellas).
