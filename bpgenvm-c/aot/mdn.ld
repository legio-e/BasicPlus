/*
 * mdn.ld — el guión de enlace de un `.mdn`. Compartido por ARM y RISC-V.
 *
 * ─── QUÉ RESUELVE ───────────────────────────────────────────────────────────
 *
 * Un `.mdn` se lleva UNA sección —`.text`— y se carga sin enlazador, así que
 * todo lo que el código referencie tiene que estar DENTRO de ella. Los
 * literales no lo estaban: `return "hola" + intToString(n)` en una función
 * `native` deja un `.rodata.str1.1` aparte, y `MdnPack` lo rechaza con razón
 * (en la placa esa referencia sería un puntero a ninguna parte).
 *
 * Aquí `.rodata` se fusiona DENTRO de `.text`. Con el enlace a dirección 0 y el
 * código compilado PC-relativo, las referencias al literal quedan resueltas
 * como desplazamientos, y el resultado sigue siendo cargable en cualquier
 * dirección — que es lo que un `.mdn` exige.
 *
 * COMPROBADO (16-ago, #428) enlazando el mismo objeto a 0x00000000 y a
 * 0x20001000: el `.text` sale **byte-idéntico**. Si algún día deja de serlo, es
 * que algo metió una dirección absoluta y el `.mdn` dejó de ser relocatable —
 * esa comparación es la prueba que hay que repetir, no la intuición.
 *
 * ─── POR QUÉ UN GUIÓN Y NO UNA OPCIÓN DE gcc ────────────────────────────────
 *
 * Porque no existe: `-fmerge-constants` y `-fsection-anchors` no hacen esto, y
 * `objcopy --rename-section` exige nombrar una a una todas las variantes
 * (`.rodata`, `.rodata.str1.1`, `.rodata.cst4`…). Aquí un comodín las coge
 * todas.
 *
 * ─── LO QUE NO SE PUEDE METER AQUÍ ──────────────────────────────────────────
 *
 * `.data` y `.bss` NO: un `.mdn` no tiene dónde ponerlos en la placa (por eso
 * una `native` no puede tener variables estáticas, y el guardián de `MdnPack`
 * lo sigue diciendo). Si aparecen, es un error del código generado, no algo que
 * este guión deba tapar — y taparlo daría basura en silencio, que es peor.
 */
SECTIONS
{
    . = 0;

    .text : {
        *(.text .text.*)
        . = ALIGN(4);
        /* Los literales, DENTRO del código. Ver arriba. */
        *(.rodata .rodata.*)
        . = ALIGN(4);
    }

    /* Ruido del toolchain que no pinta nada en un `.mdn`. Descartarlo aquí
     * mantiene el ELF limpio y hace evidente lo que sí viaja. */
    /DISCARD/ : {
        *(.comment)
        *(.ARM.attributes)
        *(.riscv.attributes)
        *(.note*)
    }
}
