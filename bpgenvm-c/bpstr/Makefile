# bpstr — la biblioteca de cadenas de BasicPlus (V8 #730): sus pruebas y su banco, en el PC.
#
#   make prueba         las pruebas, en las cuatro formas de buscar: con los topes de hoy (la corta hasta 16 bytes, por
#                       parejas hasta 256), todo por Two-Way, todo por la corta y todo por parejas (un minuto o dos)
#   make prueba-larga   y además el validador de #711 con todas las cadenas de 4 bytes (unos minutos)
#   make banco          lo que tarda cada función, frente a la libc y frente a los bucles de antes
#
# -O2, decidido por Eduardo el 3-nov (#730) aunque el resto de la VM vaya con -Os en las placas.
# -fno-tree-loop-distribute-patterns: que GCC no cambie los bucles de bpstr_copia y bpstr_rellena por memcpy y memset.

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -O2 -g
CFLAGS  += -fno-tree-loop-distribute-patterns
ifeq ($(OS),Windows_NT)
  CFLAGS += -D__USE_MINGW_ANSI_STDIO=1
  EXE     = .exe
endif
KUHN    = ../third_party/utf8-kuhn/UTF-8-test.txt
B       = build

.PHONY: prueba prueba-larga banco clean

$(B):
	mkdir -p $(B)

$(B)/prueba$(EXE): bpstr.c bpstr.h oraculo.h prueba.c | $(B)
	$(CC) $(CFLAGS) bpstr.c prueba.c -o $@

$(B)/prueba_2c$(EXE): bpstr.c bpstr.h oraculo.h prueba.c | $(B)
	$(CC) $(CFLAGS) -DBPSTR_AGUJA_CORTA=1u -DBPSTR_AGUJA_PAREJAS=0u bpstr.c prueba.c -o $@

$(B)/prueba_corta$(EXE): bpstr.c bpstr.h oraculo.h prueba.c | $(B)
	$(CC) $(CFLAGS) -DBPSTR_AGUJA_CORTA=0xFFFFFFFFu bpstr.c prueba.c -o $@

$(B)/prueba_parejas$(EXE): bpstr.c bpstr.h oraculo.h prueba.c | $(B)
	$(CC) $(CFLAGS) -DBPSTR_AGUJA_CORTA=2u bpstr.c prueba.c -o $@

$(B)/banco$(EXE): bpstr.c bpstr.h oraculo.h banco.c | $(B)
	$(CC) $(CFLAGS) -DBPSTR_CAMINOS bpstr.c banco.c -o $@

prueba: $(B)/prueba$(EXE) $(B)/prueba_2c$(EXE) $(B)/prueba_corta$(EXE) $(B)/prueba_parejas$(EXE)
	$(B)/prueba$(EXE) $(KUHN)
	$(B)/prueba_2c$(EXE) $(KUHN)
	$(B)/prueba_corta$(EXE) $(KUHN)
	$(B)/prueba_parejas$(EXE) $(KUHN)

prueba-larga: prueba
	$(B)/prueba$(EXE) larga $(KUHN)

banco: $(B)/banco$(EXE)
	$(B)/banco$(EXE)

clean:
	rm -rf $(B)
