/* npk.ld — el reparto de un MOTOR NATIVO (.npk) al enlazarlo en el PC (V7/H6.c G2).
 *
 * Un motor se enlaza en DOS bases de referencia —la flash en 0 y la RAM en
 * 0x20000000— y `NpackBuild`/`NpackReloc` clasifica cada relocalización por la
 * DIRECCIÓN: por encima del umbral es RAM, por debajo flash. Eso sólo vale si
 * las secciones de RAM están de verdad por encima.
 *
 * Con los flags de siempre (`-Ttext=0x0 -Tdata=0x20000000`) lo están… mientras
 * exista `.data`. `-Tdata` coloca la sección `.data`; las demás de RAM la siguen.
 * Un motor SIN NINGUNA variable inicializada no tiene `.data`, y entonces su
 * `.sbss`/`.bss` se queda pegada al código: le pasó al primer driver de pantalla
 * (23-sep), cuyo `g_bios` acabó en 0x1940, realojado como si fuera flash — y la
 * C5 se reinició con un «Load access fault» la primera vez que lo leyó. Aquí la
 * RAM empieza en 0x20000000 PORQUE LO DICE EL GUION, haya `.data` o no.
 *
 * Los nombres de las secciones de salida son los que `NpackReloc` espera
 * (flash: .text .rodata .eh_frame · datos: .data .sdata · a cero: .sbss .bss), y
 * `NpackReloc` además COMPRUEBA que cada una esté en su lado del umbral.
 *
 * Lo usan los drivers de pantalla (bpstdlib/gui/drivers/build_driver.sh). El
 * mini, SQLite y LVGL siguen con los flags: todos tienen `.data`, y cambiarles el
 * enlazado sin necesidad sería mover lo que funciona.
 */
ENTRY(bp_pack_init)

SECTIONS
{
    . = 0x00000000;
    .text     : { *(.text .text.*) }
    .rodata   : { *(.rodata .rodata.* .srodata .srodata.*) }
    .eh_frame : { *(.eh_frame) }

    . = 0x20000000;
    .data     : { *(.data .data.*) }
    .sdata    : { *(.sdata .sdata.*) }
    .sbss     : { *(.sbss .sbss.* .scommon) }
    .bss      : { *(.bss .bss.* COMMON) }
}
