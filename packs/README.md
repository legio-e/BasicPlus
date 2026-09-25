# packs/ — la biblioteca de packs que acompaña al IDE

Aquí viven los packs que distribuimos con BasicPlus. El compilador los mira al
resolver `import <Módulo> from pack <Pack>`, y desde el IDE se graban en la
placa (o en el micro simulado) desde el panel de Packs.

| Pack | Qué trae | Se construye con |
|---|---|---|
| `Stdlib.pack` | La librería estándar completa (29 módulos el 23-sep: Core, Str, Collections, Json, IO, Math, Time, Gui, Net, App, Packs, Machine y los de hardware) — los MISMOS `.mod` que `bpstdlib/`, byte a byte, y eso lo **comprueba la red** (`compat/stdlib_pack.sh`, `#537`); se reconstruye con **cada cambio de la stdlib**, no sólo con cada versión (el del 20-ago era MOD6 y tapaba al `/lib` fresco del P4, `#493`; el del 18-sep no tenía las clases del lenguaje en Core y daba `LINK_ERROR` en el micro simulado, `#537`) | `bpstdlib/Stdlib.bpbuild` |
| `SQLite.pack` | El motor SQLite y su ORM: los módulos `SQLite` y `Orm`, más el código nativo precompilado (`.npk` + `.mdn`) para **ARM y RISC-V** | `bpstdlib/sqlite/SQLite.bpbuild` |

## Para qué sirve el `Stdlib.pack` en una placa (V7/H6, `#468`)

**Para ahorrar RAM en las placas sin PSRAM** (la C6, la C3, la Pico 2, la Nucleo). La stdlib de fábrica viene
dentro de la imagen y el sistema la copia a `/lib`; de ahí se carga EN RAM. Con el `Stdlib.pack` grabado, cada módulo
que el pack sirve con los mismos bytes que la imagen se carga **en sitio** desde la flash (XIP): a la RAM sólo va su
parte de datos, no el código. El instalador de la imagen lo sabe: al arrancar mira la zona de packs y quita de `/lib`
su copia de lo que el pack ya sirve igual (una más nueva que alguien haya puesto en `/lib` se queda, y tapa al pack).

**Borrar el pack no deja la placa sin librería**: la imagen sigue siendo la fuente. Al borrarlo (o al formatear la
zona), `/lib` se repone desde la imagen en el acto, sin esperar a reiniciar; y lo mismo al revés: al grabarlo, las
copias que sobran salen de `/lib` en ese momento.

## Cómo se reconstruye

La librería **no** se empaqueta con un script aparte: es un proyecto BP normal,
con las mismas herramientas que cualquier programa de usuario.

```bash
java -jar lexer-java/target/basicplus-frontend.jar \
     --project bpstdlib/Stdlib.bpbuild --backend=mivm --prune-bpi
cp bpstdlib/out/Stdlib.pack packs/
```

(o **Project → Build Project** abriendo `bpstdlib/Stdlib.bpbuild` en el IDE).
Esas dos órdenes, y la comprobación de después, en una:

```bash
bash compat/stdlib_pack.sh --regenerar
```

El truco está en `bpstdlib/Stdlib.bp`: un módulo sin código cuyo único cometido
es **importar** toda la librería. Como el compilador arrastra los imports
transitivamente, compilar ese main compila toda la librería, y `"out": "pack"`
los empaqueta. **Al añadir un módulo nuevo a la librería hay que añadir su
import en `Stdlib.bp`** — si no, no entra en el pack (y la guarda lo dice:
«falta X.mod»).

## La guarda (`#537`)

`bash compat/stdlib_pack.sh` comprueba, con el lector canónico (`PackCli`), que
cada `.mod` del pack es el de `bpstdlib/` byte a byte y que no falta ni sobra
ninguno. La corren `compat/compat.sh check` (frente `check-sim`, junto a los
smokes del micro simulado, que arrancan el sim con este pack como el IDE) y
`scripts/montar-zip.sh` (un ZIP no se monta con un pack que no sea la stdlib que
lleva). Hacía falta: este pack se quedó rancio tres veces sin que nada lo viera,
porque es un solo fichero y las guardas miraban `.mod` sueltos.

## Dónde lo busca el IDE

`IdePrefs.packsDir`, configurable en el engranaje del micro simulado
(*Librería de packs*). Si no se configura, se autodetecta una carpeta `packs/`
junto al jar del IDE y, si no, la del directorio de trabajo — que es esta.

Un pack es **portable**: el mismo fichero vale para las 3 familias de micro
(el bloque de 8 KB del formato es múltiplo del borrado de todas ellas).
