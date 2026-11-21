# Un programa que carga a otro

El ejemplo de los overlays (`Overlays.bp`). El programa A carga el programa B, lo ejecuta y lo descarga, dos veces: una
acaba bien y otra falla.

| Fichero | Qué hace |
|---|---|
| `ProgramaA.bp` | carga a B con un `Overlay`. Usa M1, M2 y M3 |
| `ProgramaB.bp` | un programa normal, que también se puede ejecutar solo. Usa M1 y M4 |
| `M1.bp` | lo comparten los dos: hay un solo M1, con sus globales |
| `M2.bp`, `M3.bp` | sólo los usa A |
| `M4.bp` | sólo lo usa B: se carga con él y se descarga con él. Tiene una clase de excepción y un destructor |

Para probarlo, hay que compilar los dos programas en la misma carpeta:

```sh
java -jar toolchain/lexer-java/target/basicplus-frontend.jar samples/overlay/ProgramaB.bp --compile samples/overlay/out --backend=mivm
java -jar toolchain/lexer-java/target/basicplus-frontend.jar samples/overlay/ProgramaA.bp --compile samples/overlay/out --backend=mivm
bpgenvm-c/build/bpgenvm-c samples/overlay/out/ProgramaA.mod
```

Lo que hay que mirar en la salida:

- B ve M1 como lo dejó A, y lo que B le cambia se queda para A;
- el nombre que B deja en M1 sigue ahí cuando B ya no está: en un programa cargado, un literal de cadena es un string del
  heap;
- el destructor del sensor de M4 corre al descargar;
- cuando B falla con una clase de excepción suya, a A sólo le llega el texto: el mensaje y el nombre de la clase;
- el punto de A sigue igual, aunque sus objetos se hayan movido para hacerle sitio a B.
