# bpstr — la biblioteca de cadenas de BasicPlus (V8 `#730`)

Eduardo, 21-oct-2026: *«una librería en C para manejar nuestros strings. La librería debería estar optimizada a nivel
de algoritmo y de codificación y verificada.»* Hecha aparte, y al acabar se incorpora a la VM. El diseño y lo
decidido, en `docs/V8_IDEAS.md` §E5, «`#730`: el diseño concreto»; el estado, en `docs/FICHAS.md` (`#730`).

## Lo que hay

| Fichero | Qué |
|---|---|
| `bpstr.h`, `bpstr.c` | la biblioteca: el UTF-8, buscar, comparar, mayúsculas y minúsculas, `trim`, copiar y rellenar. Dentro, con sus avisos de licencia, código adaptado de musl (Two-Way, MIT) y de newlib, escrito por Arm (buscar por parejas y la `memcpy` de Cortex-M, BSD-3) |
| `oraculo.h` | los bucles de la VM de antes, copiados tal cual: lo que bpstr tiene que dar, byte a byte |
| `prueba.c` | las pruebas, contra el oráculo |
| `banco.c` | lo que tarda cada función, frente a la libc y frente al bucle de antes |
| `placa.c` | las pruebas y el banco en una placa: lo que los une a una imagen de prueba de la VM (abajo) |
| `Makefile` | `make prueba`, `make prueba-larga` y `make banco`, en el PC |

## En una placa (el paso 2)

Con `BPSTR_PLACA`, `prueba.c` y `banco.c` se compilan para una placa: las pruebas, más cortas (la RAM y el tiempo no
dan para las del PC: de las cadenas de 3 bytes, una de cada 4099; pajares y agujas más pequeños; 3000 al azar; sin
los bordes, que piden páginas protegidas), y el banco, con los búferes que quepan (en las ESP con PSRAM, otra vuelta con
los datos allí). Una **imagen de prueba** (no se queda en el repositorio) lleva además `bpstr.c` y `placa.c`, y
`BPSTR_PRUEBA_PLACA`: entonces el arranque común (`src/bpvm_boot2.c`) llama a `bpstr_en_placa` con el FS montado, y
lo que dicen va, línea a línea, a `/app/bpstr.txt` (se baja con `wire_serie.py PUERTO get`). El catálogo de Kuhn tiene
que estar antes en `/app/UTF-8-test.txt`. Los números se escriben con enteros: el `printf` de una placa puede no tener
`%f` ni `%llu` (las ESP del IDF 6.1 llevan picolibc). Con `BPSTR_CAMINOS` (en el PC, `make banco` ya lo pone), el banco
añade «buscar camino a camino»: cada camino por separado, con agujas de 2 a 300 bytes de tres clases según su primer
byte (el espacio, una letra, uno que no está), para elegir los topes por la medida.

Lo que se aprendió al pasarlas (3-nov):
- **corren en la tarea del arranque, antes del REPL.** En una tarea suya, en la Pico reventaban antes de la primera
  línea (el FS desde una tarea que no es la de la VM);
- **en la RP2350, el perro guardián del arranque** (15 s, armado antes del planificador con dos núcleos y apagado al
  llegar al REPL) reiniciaba la placa en mitad de la prueba, una y otra vez: `placa.c` lo apaga al empezar;
- **cada línea va también a la salida estándar,** que en la Pico es el USB serie y se ve en directo aunque el REPL no
  esté; pero la SDK la tira si el ordenador no ha levantado DTR: el lector tiene que abrir el puerto con DTR.

## Cómo es

- **No reserva memoria y no conoce la VM:** todo sobre un puntero y una longitud. Quien crea la cadena reserva y le
  pide que la llene.
- **El mismo resultado que antes,** también con bytes que no son UTF-8.
- **De 4 en 4:** lee palabras de 4 bytes alineadas y opera con enteros sobre los cuatro a la vez; la cabeza y la cola,
  byte a byte, así que nunca lee más allá del final. Sin instrucciones especiales: el mismo C en ARM, RISC-V, Xtensa y
  el PC.
- **Lo de Arm que gana, dentro** (Eduardo, 3-nov: *«lo que hay que hacer es un mix, mezclar nuestras librerías y las
  de ARM. En nuestras librerías, copiamos las funciones de ARM que sean mejores»*; y *«debería dar resultados mejores o
  iguales pero nunca peores, al menos en teoría»*). Lo que se midió en el paso 2 que ganaba la newlib de la RP2350:
  - **buscar por parejas** (su `memmem`), en C, igual en las tres arquitecturas;
  - **copiar** (su `memcpy` de Cortex-M), en ensamblador dentro del C y sólo en las ARM de Thumb-2. Comparada con la de
    la `libg.a` de la cadena de ARM, es la misma byte a byte (236 bytes). En las demás, la `memcpy` de su libc, *inline*
    en `bpstr.h`.
- **Buscar nunca es cuadrático,** con tres caminos según el largo de la aguja:
  - la **corta**, hasta 16 bytes (el tope, por decidir con la medida): su primer byte, de 4 en 4, y comparar el resto;
  - por **parejas** (el de Arm), hasta 256;
  - **Two-Way** (adaptado del `memmem` de musl), por encima.

  Los dos primeros son lineales porque la aguja no pasa de su tope; Two-Way, siempre.
- **Se compila con `-O2`** (decidido por Eduardo el 3-nov) y con `-fno-tree-loop-distribute-patterns`, para que GCC
  no cambie el bucle de rellenar por una llamada a `memset`.

## Las pruebas

`make prueba` las pasa cuatro veces: con los topes de hoy, todo por Two-Way, todo por la corta y todo por parejas (así
cada camino se prueba con agujas pequeñas). Cada vez:

1. exhaustivo: todas las cadenas de 0 a 3 bytes, en las cuatro alineaciones por turno;
2. buscar:
   - todos los pajares y agujas pequeños de dos alfabetos (`a b 80 C3` y `a b`);
   - agujas largas periódicas;
   - agujas de 3 a 300 bytes (la frontera de 256, byte a byte) en tres alfabetos. Uno hace chocar las parejas en la
     tabla: dos bytes que se llevan 32, como la `a` y la `A`, se mezclan igual delante del mismo byte;
3. comparar: todas las parejas cortas; copiar y rellenar, de 0 a 300 bytes; las dos, en las 16 combinaciones de
   alineación;
4. los bordes: de 0 a 64 bytes, con la cadena pegada a una página de memoria protegida (leer un byte de más revienta);
5. al azar: 200 000 cadenas con una semilla fija;
6. el catálogo de UTF-8 de Markus Kuhn (`third_party/utf8-kuhn/`), entero y línea a línea.

`make prueba-larga` añade el validador de `#711` con todas las cadenas de 4 bytes (4 300 millones).

Para saber que ven los fallos: veinte sabotajes en el paso 1 y once en el camino por parejas (3-nov). Las once se
pillan: ocho salen en rojo, dos revientan al leer fuera y una se queda dando vueltas (la tabla de saltos sin poner a
cero: con basura, un salto mayor que la aguja la hace retroceder).

## El catálogo de Kuhn

`third_party/utf8-kuhn/UTF-8-test.txt`: *UTF-8 decoder capability and stress test*, de Markus Kuhn (2015-08-28),
licencia CC BY 4.0, descargado el 3-nov-2026 de `https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt`
(22 781 bytes; SHA-256 `b51cfe9a8d2689c90b10a13a3624092d546e0837c6ff835b6e5d713c5749c8c6`). Va marcado como
binario en `.gitattributes`: sus bytes malos son a propósito, y git no debe tocarle los finales de línea.
