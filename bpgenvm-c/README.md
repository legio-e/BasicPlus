# bpgenvm-c

Implementación C99 de la VM BasicPlus. Pensada para targets pequeños
(MCU con FreeRTOS) y para entornos hosted (Linux/macOS/Windows como
plataforma de desarrollo). La VM Java en `../miVM/` es la
implementación de referencia; **si las dos divergen, gana la spec en
`../docs/`** (`MOD_FORMAT.md`, `OPCODES.md`, `HEAP_LAYOUT.md`,
`BUILTINS.md`).

## Estado

- **F1 (en curso)** — loader + intérprete single-thread, sin heap. Subset
  de opcodes: aritmética entera, comparaciones, locales/globales,
  control flow, calls, print int. **Suficiente para programas BP
  puramente computacionales sin strings ni clases.**
- **F2** — heap + GC mark-sweep. Strings, arrays, alocador con type
  tags. Cubre samples como movetest y print de strings literales.
- **F3** — clases + INVOKE_VIRTUAL (paridad L2 v1, con fall-back al
  parent L2 v3).
- **F4** — threading + Mutex sobre FreeRTOS.
- **F5** — try/catch + paridad funcional con la VM Java.

## Layout

```
bpgenvm-c/
├── README.md        (este fichero)
├── Makefile         (build con gcc)
├── include/
│   ├── bpvm.h            API pública
│   ├── bpvm_internal.h   tipos/constantes compartidos entre TUs
│   └── bpvm_opcodes.h    códigos de los opcodes (extraídos de docs/)
├── src/
│   ├── bpvm.c       init/destroy/run
│   ├── loader.c     parser del .mod
│   └── interp.c     while-switch del intérprete
└── test/
    └── main.c       CLI smoke
```

## Cómo construir

```
make
```

Produce `build/bpgenvm-c` (o `.exe` en Windows). Sin dependencias salvo
libc y un compilador C99 (gcc / clang).

## Cómo ejecutar

```
build/bpgenvm-c samples/foo.mod
build/bpgenvm-c --trace samples/foo.mod      # trace per-instrucción
build/bpgenvm-c --mem=131072 samples/foo.mod # 128 KiB de RAM
```

## Decisiones de diseño

- **C99 puro**, sin C++. Facilita freestanding builds para MCU.
- **Buffer de memoria provisto por el caller** (`bpvm_init(mem, size, …)`).
  La VM no llama `malloc` en runtime (en F1 sí para la estructura
  de control; F2 expondrá una variante "all-static").
- **Big-endian en memoria**: los `.mod` viajan en BE; las helpers
  `bpvm_read_u32_be` / `bpvm_write_u32_be` cubren el swap en cada
  acceso. Coste pequeño en MCUs little-endian; gana portabilidad.
- **FreeRTOS** como plataforma de threading objetivo (F4+). Linux dev
  vía FreeRTOS-POSIX para tener UNA sola implementación.
- **Caller decide la salida** vía `bpvm_set_output(cb, user)`. Sin
  callback, los `PRINT_*` van a `stdout` con `fwrite()`.

## Compatibilidad con la VM Java

Los mismos `.mod` que ejecuta `miVM` deben ejecutarse aquí. Cualquier
divergencia es un bug — en cualquiera de las dos. Como referencia:

```
# Compilar un .bp con el frontend Java
cd ..
java -jar lexer-java/target/basicplus-frontend-1.0.0-shaded.jar \
     samples/foo.bp --compile out --backend=mivm

# Ejecutar con la VM Java
java -jar miVM/target/bpgenvm-1.0.jar out/Foo.mod

# Ejecutar con la VM C
cd bpgenvm-c
make && ./build/bpgenvm-c ../out/Foo.mod
```

La salida debe ser idéntica byte por byte (módulo el banner inicial
"=== INICIANDO EJECUCION ===" que cada VM emite a su manera).
