/*
 * bpstr_vm.c — la biblioteca de cadenas (bpgenvm-c/bpstr/, V8 #730, el paso 3) dentro de la VM. bpstr vive aparte, con
 * sus pruebas, su banco y su LEEME; aquí sólo se compila su bpstr.c. Así la llevan también los builds que recogen src/
 * entera sin listarla (el de la STM32, por su carpeta enlazada), y en las demás listas se nombra este fichero, con -O2
 * y -fno-tree-loop-distribute-patterns (decidido por Eduardo el 3-nov). Lo que la VM usa de ella: las funciones de
 * bpvm_internal.h (utf8_cp_count, utf8_primer_malo…, que la llaman) y los builtins de las cadenas (builtins.c).
 */
#include "../bpstr/bpstr.c"
