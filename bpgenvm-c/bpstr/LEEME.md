# bpstr — la biblioteca de cadenas de BasicPlus (V8 `#730`)

Eduardo, 21-oct-2026: *«una librería en C para manejar nuestros strings. La librería debería estar optimizada a nivel
de algoritmo y de codificación y verificada.»* Hecha aparte, y al acabar se incorpora a la VM. El diseño y lo
decidido, en `docs/V8_IDEAS.md` §E5, «`#730`: el diseño concreto»; el estado, en `docs/FICHAS.md` (`#730`).

## Lo que hay

| Fichero | Qué |
|---|---|
| `bpstr.h`, `bpstr.c` | la biblioteca: el UTF-8, buscar, comparar, mayúsculas y minúsculas, `trim`, copiar y rellenar |
| `oraculo.h` | los bucles de la VM de antes, copiados tal cual: lo que bpstr tiene que dar, byte a byte |
| `prueba.c` | las pruebas, contra el oráculo |
| `banco.c` | lo que tarda cada función, frente a la libc y frente al bucle de antes |
| `Makefile` | `make prueba`, `make prueba-larga` y `make banco`, en el PC |

## Cómo es

- **No reserva memoria y no conoce la VM:** todo sobre un puntero y una longitud. Quien crea la cadena reserva y le
  pide que la llene.
- **El mismo resultado que antes,** también con bytes que no son UTF-8.
- **De 4 en 4:** lee palabras de 4 bytes alineadas y opera con enteros sobre los cuatro a la vez; la cabeza y la cola,
  byte a byte, así que nunca lee más allá del final. Sin instrucciones especiales: el mismo C en ARM, RISC-V, Xtensa y
  el PC.
- **Buscar nunca es cuadrático:** una aguja de más de 16 bytes va por Two-Way (adaptado del `memmem` de musl, con su
  licencia MIT en `bpstr.c`).
- **Se compila con `-O2`** (decidido por Eduardo el 3-nov) y con `-fno-tree-loop-distribute-patterns`, para que GCC
  no cambie los bucles de copiar y rellenar por llamadas a `memcpy` y `memset`.

## Las pruebas

`make prueba` las pasa tres veces: con el tope de hoy para las agujas cortas, con todo por Two-Way y con todo por la
corta (así los dos caminos se prueban con agujas pequeñas). Cada vez:

1. exhaustivo: todas las cadenas de 0 a 3 bytes, en las cuatro alineaciones por turno;
2. buscar: todos los pajares y agujas pequeños de dos alfabetos (`a b 80 C3` y `a b`), y agujas largas periódicas;
3. comparar, copiar y rellenar: todas las parejas cortas, en las 16 combinaciones de alineación;
4. los bordes: de 0 a 64 bytes, con la cadena pegada a una página de memoria protegida (leer un byte de más revienta);
5. al azar: 200 000 cadenas con una semilla fija;
6. el catálogo de UTF-8 de Markus Kuhn (`third_party/utf8-kuhn/`), entero y línea a línea.

`make prueba-larga` añade el validador de `#711` con todas las cadenas de 4 bytes (4 300 millones).

## El catálogo de Kuhn

`third_party/utf8-kuhn/UTF-8-test.txt`: *UTF-8 decoder capability and stress test*, de Markus Kuhn (2015-08-28),
licencia CC BY 4.0, descargado el 3-nov-2026 de `https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt`
(22 781 bytes; SHA-256 `b51cfe9a8d2689c90b10a13a3624092d546e0837c6ff835b6e5d713c5749c8c6`). Va marcado como
binario en `.gitattributes`: sus bytes malos son a propósito, y git no debe tocarle los finales de línea.
