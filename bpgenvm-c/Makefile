# Makefile para bpgenvm-c en Linux/macOS/Windows (con MinGW o WSL).
# F1: build sencillo con gcc, sin dependencias externas más allá de libc.
#
#   make            → build/bpgenvm-c (ejecutable CLI)
#   make clean      → limpia build/
#   make smoke      → ejecuta build/bpgenvm-c sobre samples/F1.mod
#   make CFLAGS_EXTRA="-DBPVM_DEBUG"   build con flags extra

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -O2 -g
CFLAGS  += -Iinclude
CFLAGS  += $(CFLAGS_EXTRA)
# H3: genera ficheros .d con las dependencias de cabecera de cada .o, para que
# editar un header (p.ej. bpvm_internal.h, la struct) recompile los .c que lo
# incluyen. Sin esto, un cambio de struct dejaba .o desalineados (build stale).
CFLAGS  += -MMD -MP

SRC     = src/bpvm.c src/bpvm_util.c src/loader.c src/interp.c src/heap.c src/builtins.c \
          src/link.c src/scheduler.c src/threading.c src/exceptions.c src/events.c \
          src/platform_pthread.c src/gpio.c src/i2c.c src/spi.c src/uart.c \
          src/pulse.c src/pwm.c src/pico.c src/bpvm_neopixel.c src/rtc.c src/adc.c src/wdt.c \
          src/aot_registry.c src/bpvm_aot_helpers.c src/mdn_loader.c \
          src/scheduler_smp.c src/comm_common.c src/comm_host.c \
          src/fs_facade.c src/fs_host.c src/crc32.c src/bpvm_env.c src/bpvm_part.c src/bpvm_boot.c src/bpvm_sqlmem.c src/bpvm_bios.c src/bpvm_bios_fs.c src/bpvm_npack.c src/bpvm_bmgr.c src/bpvm_bmgr_wire.c src/bpvm_dbg_wire.c \
          src/bpvm_pack.c src/bpvm_mdn_scan.c \
          src/net.c src/net_host.c \
          src/fs_lfs.c src/fs_lfs_host.c \
          third_party/littlefs/lfs.c third_party/littlefs/lfs_util.c \
          third_party/littlefs/bd/lfs_filebd.c

# Paths de las deps GUI vendorizadas (third_party/, gitignored; ver su README).
LVGL_DIR  = third_party/lvgl
SDL2_DIR  = third_party/SDL2/x86_64-w64-mingw32
LVGL_INC  = -I$(LVGL_DIR) -Iinclude -DLV_CONF_INCLUDE_SIMPLE -I$(SDL2_DIR)/include

# littlefs vendorizado (H2 fase A — motor del FS de flash). third_party/, ver su README.
# In-tree y compilado EN la lib host (SRC incluye lfs core + filebd + fs_lfs*):
# la VM-C de PC lleva el modo oráculo dentro (B1.2: seleccionable por flag).
LFS_DIR   = third_party/littlefs
LFS_INC   = -I$(LFS_DIR) -I$(LFS_DIR)/bd
LFS_CORE  = $(LFS_DIR)/lfs.c $(LFS_DIR)/lfs_util.c
CFLAGS   += $(LFS_INC)
# Trazas internas de littlefs SILENCIADAS en la lib de la VM: imprimen por
# stdout (p.ej. "Corrupted dir pair" en el 1er mount de una imagen virgen,
# ESPERADO antes del format) y romperían la paridad byte a byte de la salida.
# El canal de error real es la fachada (-1 → RuntimeError BP atrapable). Los
# tests B0 (test-lfs*) compilan littlefs aparte SIN esto → conservan trazas.
CFLAGS   += -DLFS_NO_DEBUG -DLFS_NO_WARN -DLFS_NO_ERROR

# V3/H4 — backend GUI. Dos ejes de FEATURE (build-time), no de placa:
#   GUI=1 (default): contrato Gui.* (modelo + dumpTree). GUI=0 -> VM lean (los
#                    builtins GUI caen al "no implementado", como un micro sin-GUI).
#   LVGL=1:          ademas, render real LVGL v9 + ventana SDL en host (implica GUI).
GUI  ?= 1
LVGL ?= 0
ifeq ($(LVGL),1)
  GUI := 1
  CFLAGS += -DBPVM_LVGL $(LVGL_INC)
  SRC    += src/gui_display_sdl.c   # backend de display SDL (host); el micro usa LTDC (port/)
endif
ifeq ($(GUI),1)
  CFLAGS += -DBPVM_GUI
  SRC    += src/gui.c
endif

LDFLAGS = -lpthread
ifeq ($(LVGL),1)
  LDFLAGS += build/liblvgl.a -L$(SDL2_DIR)/lib -lSDL2
endif
TEST    = test/main.c
OBJ     = $(SRC:%.c=build/%.o) build/test/main.o
BIN     = build/bpgenvm-c

ifeq ($(OS),Windows_NT)
  BIN := $(BIN).exe
  LDFLAGS += -lws2_32    # H11 — Winsock2 (net_host.c)
endif

.PHONY: all clean smoke test-callbp test-xmodule test-throwmsg test-throwuser test-method test-debug test-debug-mt test-listen test-bytenat test-xmodnat test-compressnat test-compressbench test-kill test-net test-smphandles test-oom test-excmsggc test-lfs test-lfsfile test-lfsfault test-fslfs test-fsvfs test-fslfsmt test-iomt test-fspos test-blk test-sdio test-listdir test-env test-part test-boot test-bmgr test-pack test-crc test-gchole test-rand test-freelist test-runguard sim sim-smoke boardsim boardsim-smoke gui-smoke gui-imgdecode

# ---------------------------------------------------------------------------
# $(call fixture,Mod1 Mod2 ...) — asegura que los .mod de una prueba SIRVEN.
#
# Los .mod de samples/ son ARTEFACTOS (gitignored). Los targets se limitaban a
# saltarse la prueba si FALTABAN, pero el modo de fallo caro es otro: que el
# .mod ESTÉ y sea rancio. Entonces el gate de ABI (#284) lo rechaza y el target
# sale en ROJO por una razón que no tiene nada que ver con lo que prueba.
#
# Ya ha pasado tres veces. test-kill llevaba semanas rojo por eso y se le puso
# un parche por mtime; el 31-jul se encontró que test-oom, test-excmsggc,
# test-net y test-iomt estaban rojos IGUAL — y a esos el parche por mtime NO
# los habría salvado, porque su .mod es más NUEVO que su .bp: lo que caducó no
# fue el fuente, fue el FORMATO (se quedaron en v5 al pasar a v6 en H6.a).
#
# Por eso el guardián mira las tres cosas, y la tercera es la que importa:
#   1. no está           -> generar
#   2. el .bp es más nuevo -> regenerar
#   3. su MAGIC no es el que habla ESTA VM -> regenerar
#
# El magic sale de include/bpvm_internal.h, que es donde el loader lo lee: si
# mañana el formato sube a v7, este guardián se entera solo. Comparación en
# HEX (od) para no depender de printf \xHH, que no es POSIX.
#
# Un test que falla por su andamio enseña a ignorarlo, que es la peor avería.
# ---------------------------------------------------------------------------
define fixture
@want=$$(sed -n 's/.*BPVM_MAGIC_V6[[:space:]]*0x\([0-9A-Fa-f]\{8\}\).*/\1/p' include/bpvm_internal.h | head -1 | tr a-z A-Z); \
for m in $(1); do \
  why=""; \
  if [ ! -f samples/$$m.mod ]; then why="no existe"; \
  elif [ samples/$$m.bp -nt samples/$$m.mod ]; then why="el .bp es mas nuevo"; \
  else \
    got=$$(od -An -tx1 -N4 samples/$$m.mod | tr -d ' \n' | tr a-z A-Z); \
    if [ "$$got" != "$$want" ]; then why="formato $$got != $$want (rancio)"; fi; \
  fi; \
  if [ -n "$$why" ]; then \
    echo "  (regenerando samples/$$m.mod — $$why)"; \
    java -jar ../lexer-java/target/basicplus-frontend.jar \
         samples/$$m.bp --compile samples --backend=mivm >/dev/null 2>&1 || true; \
  fi; \
done
endef

# $(call stdlibdep,Mod1 Mod2 ...) — deja en samples/ los .mod de stdlib que un
# fixture necesita. Desde H6.a el .mod trae su interfaz dentro, asi que al
# cargar el raiz el loader busca SUS imports AL LADO del fichero: pasarlos
# ademas por la linea de comandos ya no basta, porque el fallo ocurre antes.
# (Se vio en test-net: con TcpEchoTest.mod al dia seguia rojo, y era esto.)
# Fuente de la verdad = ../bpstdlib/*.mod; aqui solo se copia si falta o difiere.
define stdlibdep
@for m in $(1); do \
  if [ ! -f samples/$$m.mod ] || ! cmp -s ../bpstdlib/$$m.mod samples/$$m.mod; then \
    echo "  (copiando ../bpstdlib/$$m.mod -> samples/)"; \
    cp ../bpstdlib/$$m.mod samples/$$m.mod; \
  fi; \
done
endef

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

# H4.2 — con LVGL=1 el binario necesita la lib de LVGL (compilada una vez, flags
# relajados) y SDL2.dll junto al exe. El `find` va en la receta (shell Unix), no
# en $(shell ...) (que en w64devkit resuelve al find.exe de Windows).
ifeq ($(LVGL),1)
$(BIN): build/liblvgl.a build/SDL2.dll
build/liblvgl.a:
	@mkdir -p build/lvgl
	@echo "[lvgl] compilando fuentes de LVGL (una vez, ~1 min)..."
	@for f in $$(find $(LVGL_DIR)/src -name '*.c'); do \
	   o=build/lvgl/$$(echo "$$f" | tr '/' '_').o; \
	   $(CC) -std=gnu11 -O2 $(LVGL_INC) -c "$$f" -o "$$o" || exit 1; \
	 done
	ar rcs $@ build/lvgl/*.o
build/SDL2.dll:
	@mkdir -p build
	cp $(SDL2_DIR)/bin/SDL2.dll $@
endif

# ── SABOR del build (GUI/LVGL) ────────────────────────────────────────────
# Un .o NO recuerda con qué flags se compiló, y aquí los flags cambian lo que
# el fichero ES: con -DBPVM_LVGL, gui.c llama al display; sin él, ni lo menciona.
# Al pasar de `make sim` a `make LVGL=1 sim`, make veía gui.o más nuevo que
# gui.c y NO lo recompilaba → el binario enlazaba gui_display_sdl.o pero nadie
# lo inicializaba: la ventana no aparecía jamás, sin un solo aviso. Un día
# perdido buscándolo en el sitio equivocado.
# Solución: un sello por combinación de flags. Si cambia, se tiran los .o del
# sabor anterior y se recompila entero (~30 s, y correcto).
BUILD_FLAVOR := gui$(GUI)-lvgl$(LVGL)
FLAVOR_STAMP := build/.flavor-$(BUILD_FLAVOR)

$(FLAVOR_STAMP):
	@mkdir -p build
	@if ls build/.flavor-* >/dev/null 2>&1; then \
	   echo "== el build cambia de sabor a $(BUILD_FLAVOR): se recompila todo =="; \
	   rm -rf build/src build/test build/third_party build/pico build/tools; \
	 fi
	@rm -f build/.flavor-*
	@touch $@

build/%.o: %.c $(FLAVOR_STAMP)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build

# H3: incluye los .d generados por -MMD (deps de cabecera). En el primer build
# aún no existen → el prefijo '-' los ignora silenciosamente.
-include $(OBJ:.o=.d)

# smoke test: ejecuta el .mod indicado en MOD=... (default samples/F1.mod
# si existe en el subdir samples/ del propio bpgenvm-c).
MOD ?= samples/F1.mod
smoke: $(BIN)
	@if [ ! -f "$(MOD)" ]; then echo "(skip smoke: $(MOD) no existe)"; else \
	  echo "--- ejecutando $(MOD) ---"; \
	  ./$(BIN) "$(MOD)"; fi

# P-aot-call-bp — test del puente native→BP. Compila el driver + el thunk a
# mano contra los .o de la lib y corre NativeBridge.mod. La salida debe ser
# byte-idéntica a la de la VM-Java (compute via thunk→puente→helper vs
# compute interpretado). Ver test/test_callbp.c.
CALLBP_BIN = build/test_callbp.exe
LIBOBJ = $(SRC:%.c=build/%.o)
test-callbp: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NativeBridge)
	@if [ ! -f samples/NativeBridge.mod ]; then \
	  echo "(skip test-callbp: falta samples/NativeBridge.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/NativeBridge.bp --compile samples --backend=mivm)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_callbp.c test/aot_NativeBridge.c \
	    $(LIBOBJ) -o $(CALLBP_BIN) $(LDFLAGS); \
	  echo "--- puente native->BP (esperado: helper BP, 54, helper BP, 84, status=OK) ---"; \
	  ./$(CALLBP_BIN) samples/NativeBridge.mod; \
	fi

# #193 — test de los helpers AOT byte[] (array_load_u8 / array_store_i8). Compila
# el driver + el thunk snapshot contra los .o de la lib y corre ByteNat.mod. La
# salida (sum = 150) debe ser byte-idéntica a la de la VM-Java (sumBytes/copyBytes
# native vs interpretado). Ver test/test_bytenat.c.
BYTENAT_BIN = build/test_bytenat.exe
test-bytenat: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,ByteNat)
	@if [ ! -f samples/ByteNat.mod ]; then \
	  echo "(skip test-bytenat: falta samples/ByteNat.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/ByteNat.bp --compile samples --backend=mivm)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_bytenat.c test/aot_ByteNat.c \
	    $(LIBOBJ) -o $(BYTENAT_BIN) $(LDFLAGS); \
	  echo "--- helpers AOT byte[] #193 (esperado: sum = 150, status=OK) ---"; \
	  ./$(BYTENAT_BIN) samples/ByteNat.mod; \
	fi

# #381 — `long` CRUZANDO A UNA FUNCION `native` (8 bytes por la pila BP).
#
# Hasta esta ficha un `native` solo movia valores de 4 bytes. El programa esta
# escrito para que un marshalling roto SE NOTE: numeros que no caben en 32 bits
# (con valores pequenos, leer 4 bytes daria el mismo resultado y pasaria en
# verde), anchos mezclados en una firma, y llamadas encadenadas — si el `sp`
# avanzara 4 donde debe avanzar 8, lo que falla es la llamada SIGUIENTE.
#
# El oraculo es la VM-Java sobre el mismo .mod: la salida tiene que ser
# byte-identica. Ver test/test_longnat.c.
LONGNAT_BIN = build/test_longnat.exe
test-longnat: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,LongNat)
	@if [ ! -f samples/LongNat.mod ]; then 	  echo "(skip test-longnat: falta samples/LongNat.mod)"; 	else 	  java -cp "../lexer-java/target/classes;../pack/target/classes" 	    basicplus.frontend.AotMain samples/LongNat.bp build/aotlong >/dev/null; 	  cp build/aotlong/aot_LongNat.c test/aot_LongNat.c; 	  $(CC) $(CFLAGS) -Isrc test/test_longnat.c test/aot_LongNat.c 	    $(LIBOBJ) -o $(LONGNAT_BIN) $(LDFLAGS); 	  echo "--- #381 long en native (la salida debe ser la de la VM-Java) ---"; 	  ./$(LONGNAT_BIN) samples/LongNat.mod; 	fi

# #240 — Compress.decompress (LZSS) en variante NATIVE. Compila el thunk AOT
# (byte[] + shl/shr/mod, helpers array_load_u8 / array_store_i8) y corre
# CompressNat.mod. La salida (n = 6 / out = ABABAB) debe ser byte-idéntica a la
# VM-Java y a la versión plain-BP de bpstdlib/Compress.bp. Ver test/test_compressnat.c.
COMPRESSNAT_BIN = build/test_compressnat.exe
test-compressnat: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,CompressNat)
	@if [ ! -f samples/CompressNat.mod ]; then \
	  echo "(skip test-compressnat: falta samples/CompressNat.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/CompressNat.bp --compile samples --backend=mivm)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_compressnat.c test/aot_CompressNat.c \
	    $(LIBOBJ) -o $(COMPRESSNAT_BIN) $(LDFLAGS); \
	  echo "--- AOT LZSS native #240 (esperado: n = 6 / out = ABABAB, status=OK) ---"; \
	  ./$(COMPRESSNAT_BIN) samples/CompressNat.mod; \
	fi

# #240 — benchmark LZSS interpretado vs native sobre el MISMO CompressBench.mod.
# Corre la ruta INTERPRETADA (bpgenvm-c.exe) y la NATIVE (test_compressbench.exe,
# que registra el thunk AOT) y muestra ambas. checksum debe coincidir; elapsed_ms
# es lo que comparamos. Ver test/test_compressbench.c y samples/CompressBench.bp.
COMPRESSBENCH_BIN = build/test_compressbench.exe
test-compressbench: $(LIBOBJ) $(BIN)
	@mkdir -p build
	$(call fixture,CompressBench)
	@if [ ! -f samples/CompressBench.mod ]; then \
	  echo "(skip test-compressbench: falta samples/CompressBench.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/CompressBench.bp --compile samples --backend=mivm)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_compressbench.c test/aot_CompressBench.c \
	    $(LIBOBJ) -o $(COMPRESSBENCH_BIN) $(LDFLAGS); \
	  echo "=== LZSS #240 — INTERPRETADO (VM-C) ==="; \
	  ./$(BIN) samples/CompressBench.mod 2>/dev/null | grep -E "outSize|iters|n =|checksum|elapsed_ms"; \
	  echo "=== LZSS #240 — NATIVE (AOT) ==="; \
	  ./$(COMPRESSBENCH_BIN) samples/CompressBench.mod 2>/dev/null | grep -E "outSize|iters|n =|checksum|elapsed_ms"; \
	fi

# #169 — test del puente native->BP CROSS-MODULE. compute (native, BridgeApp)
# llama a triple (BP, BridgeLib) por el puente con nombre cualificado. El loader
# auto-carga BridgeLib.mod vía discover_deps. Salida byte-idéntica a la VM-Java.
XMOD_BIN = build/test_xmodule.exe
test-xmodule: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,BridgeApp)
	@if [ ! -f samples/BridgeApp.mod ]; then \
	  echo "(skip test-xmodule: falta samples/BridgeApp.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/BridgeApp.bp --compile samples --backend=mivm)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_xmodule.c test/aot_BridgeApp.c \
	    $(LIBOBJ) -o $(XMOD_BIN) $(LDFLAGS); \
	  echo "--- puente native->BP cross-module (esperado: triple 5, 16, triple 8, 25, OK) ---"; \
	  (cd samples && ../$(XMOD_BIN) BridgeApp.mod); \
	fi

# #212 — AotMain resuelve imports: una native (XmodNat.compute) llama a Helper.dbl
# (BP, OTRO módulo) por el puente, con la llamada cross-module emitida AUTOMÁTICAMENTE
# (AotMain carga la .bpi del import → símbolo external → AotCEmitter enruta el puente).
# El loader auto-carga Helper.mod vía discover_deps. Salida byte-idéntica a la VM-Java.
XMODNAT_BIN = build/test_xmodnat.exe
test-xmodnat: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,XmodNat)
	@if [ ! -f samples/XmodNat.mod ]; then \
	  echo "(skip test-xmodnat: falta samples/XmodNat.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/XmodNat.bp --compile samples --backend=mivm)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_xmodnat.c test/aot_XmodNat.c \
	    $(LIBOBJ) -o $(XMODNAT_BIN) $(LDFLAGS); \
	  echo "--- AOT cross-module #212 (esperado: compute(5) = 11, OK) ---"; \
	  (cd samples && ../$(XMODNAT_BIN) XmodNat.mod); \
	fi

# #169 — método de clase EXTERNA desde native: XMethodNat.useBox (native) llama a
# XClassLib.Box.getDoubled() vía call_method_i32 con el slot resuelto del .bpi
# (ClassSig/slotOf, base H13.1). El loader auto-carga XClassLib.mod vía discover_deps.
XMETHODNAT_BIN = build/test_xmethodnat.exe
test-xmethodnat: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,XMethodNat)
	@if [ ! -f samples/XMethodNat.mod ]; then \
	  echo "(skip test-xmethodnat: falta samples/XMethodNat.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/XMethodNat.bp --compile samples --backend=mivm)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_xmethodnat.c test/aot_XMethodNat.c \
	    $(LIBOBJ) -o $(XMETHODNAT_BIN) $(LDFLAGS); \
	  echo "--- AOT metodo de clase externa #169 (esperado: useBox = 11, OK) ---"; \
	  (cd samples && ../$(XMETHODNAT_BIN) XMethodNat.mod); \
	fi

# P-run-stop (#257) — KILL cooperativo: poll_cb entre quanta + esperas con
# tope de 50 ms. Tres fases: bucle infinito con bpvm_run, ídem con
# bpvm_run_smp(1), y programa DORMIDO (sleepSec(3600)) que debe morir
# en << 2 s gracias al tope de las esperas.
# H11 (#241) — cliente TCP de BP contra echo server local (thread propio).
# `make test-net` = self-test; `build/test_net.exe serve N` = solo el server,
# para la pasada de paridad dual-VM (Java y C contra el mismo server).
NET_BIN = build/test_net.exe
test-net: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,TcpEchoTest)
	$(call stdlibdep,Net Core)
	@if [ ! -f samples/TcpEchoTest.mod ]; then \
	  echo "(skip test-net: falta samples/TcpEchoTest.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/TcpEchoTest.bp --compile samples --backend=mivm)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_net.c \
	    $(LIBOBJ) -o $(NET_BIN) $(LDFLAGS); \
	  echo "--- TCP echo H11 (esperado: conectado / 2 ecos / timeout-len: 0 / fin, PASS) ---"; \
	  ./$(NET_BIN) samples/TcpEchoTest.mod samples/Net.mod samples/Core.mod; \
	fi

KILL_BIN = build/test_kill.exe
test-kill: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,KillLoop KillSleep)
	@if [ ! -f samples/KillLoop.mod ] || [ ! -f samples/KillSleep.mod ]; then \
	  echo "(skip test-kill: no hay samples/KillLoop.mod / KillSleep.mod y no se han"; \
	  echo " podido generar — ¿está construido ../lexer-java/target/basicplus-frontend.jar?)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_kill.c \
	    $(LIBOBJ) -o $(KILL_BIN) $(LDFLAGS); \
	  echo "--- KILL cooperativo (esperado: 3x 'terminado por KILL ... PASS') ---"; \
	  (cd samples && ../$(KILL_BIN) KillLoop.mod KillSleep.mod); \
	fi

# #339 — el GUARDIÁN DE FIN DE RUN, probado contra una fuga DE VERDAD. Que la
# VM no fugue lo dice el guardián en cada ejecución; lo que hay que demostrar
# aquí es que el guardián no está mudo: se le pone delante un bloque sin liberar
# y tiene que verlo, medirlo y señalar el fichero. Ver test/test_runguard.c.
RUNGUARD_BIN = build/test_runguard.exe
test-runguard: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,MethodCall)
	$(CC) $(CFLAGS) -Isrc test/test_runguard.c $(LIBOBJ) -o $(RUNGUARD_BIN) $(LDFLAGS)
	@echo "--- guardian de fin de RUN #339 (esperado: PASS) ---"
	./$(RUNGUARD_BIN) samples/MethodCall.mod
# #175 — throw RuntimeError(<msg computado>) desde native, cazado por try/catch
# BP. test/aot_ThrowMsg.c es un SNAPSHOT del .c que genera AotCEmitter (usa
# throw_str con el string-handle computado). Salida byte-idéntica a la VM-Java.
THROW_BIN = build/test_throwmsg.exe
# Paso 7 (V4) — estrés de la tabla de handles bajo concurrencia. N pthreads
# martillean handle_register/handle_kill sobre un vm compartido. CONTROL 1 thread
# = 0 corrupciones (test sano); >1 thread = corrupciones por la carrera de la
# free-list (ROJO hasta 7a: hacer la tabla SMP-safe).
SMPH_BIN = build/test_smp_handles.exe
test-smphandles: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_smp_handles.c $(LIBOBJ) -o $(SMPH_BIN) $(LDFLAGS)
	@echo "--- CONTROL 1 thread (esperado: corrupciones=0) ---"
	./$(SMPH_BIN) 1 200000
	@echo "--- concurrente 4 y 8 threads (0 = tabla SMP-safe) ---"
	-./$(SMPH_BIN) 4 200000
	-./$(SMPH_BIN) 8 200000

# H1 (V4) — OOM ATRAPABLE. Al agotarse el heap, la VM lanza un RuntimeError
# "No space in heap" que el BP puede cazar con try/catch; NUNCA se cuelga mudo
# (el bug del "cuelgue silencioso con una lista de N referencias"). OomCatch.bp
# crece una cadena de nodos vivos hasta OOM y lo caza. Verde = imprime
# "cazado OOM" + "vivo tras OOM" y sale 0 (no exit-4 mudo, no hang).
# H9 — bloque de env (identidad de placa + luego particiones). Test host-only,
# dependencia mínima (solo bpvm_env + crc32, sin VM ni pthread): prueba el
# formato, el CRC, A/B por seq y el flujo de actualización sobre buffers en RAM
# que hacen de flash. La cintura de flash por-micro llega en su fase (placa).
# V5/H2 — escritura POSICIONAL y recorte de la fachada. Sobre el backend de
# host, pero el contrato es el mismo que deben cumplir FatFs y littlefs: un
# rojo aquí es un rojo en las placas.
test-fspos:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_fs_posicional.c src/fs_facade.c src/fs_host.c \
	  src/crc32.c -o build/test_fspos.exe
	@echo "--- V5/H2 escritura posicional (esperado: 17/17, 0 fallos) ---"
	@cd build && ./test_fspos.exe

# #423 — el INTERRUPTOR del log (`log=0|1` en el entorno). Cuatro cosas, y las
# cuatro importan: que por defecto este ENCENDIDO (el arranque queda registrado
# siempre, que es cuando menos se puede preguntar nada), que apagado no anada,
# que apagar NO BORRE lo ya escrito, y que se pueda volver a encender.
test-logsw:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_log_switch.c src/bpvm_log.c -o build/test_logsw.exe
	@echo "--- #423 interruptor del log (esperado: solo 'ok', [status=OK]) ---"
	@cd build && ./test_logsw.exe

test-env:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_env.c src/bpvm_env.c src/crc32.c -o build/test_env.exe
	@echo "--- H9 bloque de env (esperado: solo 'ok', [status=OK]) ---"
	./build/test_env.exe

# H9 — tabla de particiones sobre el env (estado 1). Host-only: parse + fachada
# kind→región + validación del layout (alineación, no-solape, clamp #292).
test-part:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_part.c src/bpvm_part.c src/bpvm_env.c src/crc32.c -o build/test_part.exe
	@echo "--- H9 tabla de particiones (esperado: solo 'ok', [status=OK]) ---"
	./build/test_part.exe

# H9 — máquina de estados del arranque escalonado + STATE. Host-only: las capas
# son callbacks de test que fallan a demanda → subida 0→3, parada en el 1er fallo,
# modo seguro (tope), fallo de runtime (drop) y reporte STATE.
test-boot:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_boot.c src/bpvm_boot.c -o build/test_boot.exe
	@echo "--- H9 maquina de estados + STATE (esperado: solo 'ok', [status=OK]) ---"
	./build/test_boot.exe

# V5/H — regla del bloque de memoria de la BD. Host-only, dependencia CERO (la
# regla es aritmetica sobre dos numeros). Prueba los bordes: minimo medido (2 MB),
# suelo que la VM conserva, desbordamiento del valor del entorno, y que cada NO
# tenga su MOTIVO distinguible.
test-sqlmem:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_sqlmem.c src/bpvm_sqlmem.c -o build/test_sqlmem.exe
	@echo "--- V5/H bloque de memoria de la BD (esperado: solo 'ok', [status=OK]) ---"
	./build/test_sqlmem.exe

# V5/I — la tabla BIOS que se le presta al pack nativo. Host-only, dependencia
# CERO. Lo que se prueba es el CHIVATO: que un hueco se detecte ANTES de ejecutar
# nada del pack y que diga QUE falta (no solo que algo falta), y que `log` —la voz
# del pack— se reporte antes que ninguna otra: sin ella el resto es indiagnosticable.
test-bios:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_bios.c src/bpvm_bios.c -o build/test_bios.exe
	@echo "--- V5/I tabla BIOS del pack (esperado: solo 'ok', [status=OK]) ---"
	./build/test_bios.exe

# V5/I — escalera de validacion del PACK NATIVO. Host-only. Lo que se prueba no
# es que un pack bueno pase, sino que CADA forma de estar mal se detecte ANTES de
# saltar y se distinga de las demas: "no cabe" y "otra arquitectura" llevan a
# sitios distintos. Incluye el bit Thumb de la direccion de salto (olvidarlo es
# hard fault inmediato).
test-npack:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_npack.c src/bpvm_npack.c src/mdn_loader.c src/aot_registry.c -o build/test_npack.exe
	@echo "--- V5/I escalera del pack nativo (esperado: solo 'ok', [status=OK]) ---"
	./build/test_npack.exe

# V5/H1 — decodificadores del CSD/CID de la tarjeta SD. Host-only y a proposito:
# el dialogo con la tarjeta necesita tarjeta, pero sacar la CAPACIDAD del CSD es
# funcion pura, son DOS formulas distintas (v1 y v2) con campos partidos entre
# bytes, y equivocarse ahi no revienta: devuelve un numero. Una tarjeta de 32 GB
# que dice tener 4 no se nota hasta que el FS escribe fuera.
test-sd:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_sd.c src/bpvm_sd.c src/spi.c src/gpio.c \
	      -o build/test_sd.exe
	@echo "--- V5/H1 CSD/CID de la SD (esperado: solo 'ok', [status=OK]) ---"
	./build/test_sd.exe

# V5/H6 - el contrato de dispositivo de bloque. Lo que de verdad se prueba aqui
# es DONDE EMPIEZA EL SISTEMA DE FICHEROS: esa decodificacion estaba enterrada
# dentro del montaje, donde no se podia ejercer sin placa Y sin tarjeta, y es
# justo la clase de codigo que al equivocarse no revienta - devuelve un numero.
# Un numero equivocado ahi manda a reformatear una tarjeta que estaba bien.
# Los dos desplazamientos del test son los MEDIDOS en el banco (2048 y 32768).
test-blk:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_blk.c src/bpvm_blk.c src/bpvm_sd_blk.c \
	      src/bpvm_sd.c src/spi.c src/gpio.c -o build/test_blk.exe
	@echo "--- V5/H6 contrato de bloque (esperado: solo 'ok', [status=OK]) ---"
	./build/test_blk.exe

# V5/H6 paso 2 - la linea 'sd' en variante SDIO. La cintura del P4 necesita
# idf.py y NO se puede compilar aqui; esto es la parte portable, y es donde
# viven los fallos mudos: una clave que se come a otra por ser su prefijo deja
# un pin plausible y equivocado, y eso no se ve hasta que la placa no contesta.
test-sdio:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_sdio.c src/bpvm_blk_sdmmc_cfg.c -o build/test_sdio.exe
	@echo "--- V5/H6 entrada 'sd' SDIO (esperado: solo 'ok', [status=OK]) ---"
	./build/test_sdio.exe

# V5/H6 paso 3 - LIST_DIR, el nucleo comun. Vivia dentro del despachador del
# Pico, donde solo se podia ejercer con placa; al sacarlo a src/ se corre sobre
# el backend de HOST y se comprueba lo que importa: que el JSON es el que el IDE
# espera, que los nombres van escapados y que un truncado nunca sale en silencio.
test-listdir:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_listdir.c src/bpvm_listdir.c src/fs_facade.c src/fs_host.c src/crc32.c src/bpvm_util.c src/bpvm_log.c -o build/test_listdir.exe
	@echo "--- V5/H6 LIST_DIR comun (esperado: solo 'ok', [status=OK]) ---"
	@cd build && ./test_listdir.exe

# H9 — board manager: núcleo portable del protocolo de gestión de placa (ENV_*/
# PART_*). Compone env+part. Host-only: opera sobre sectores A/B en RAM; simula el
# flujo virgen→defaults→apply→edit y el volcado del slot rancio. La cintura de flash
# (volcar wrote_slot) y el adaptador JSON del firmware llegan en la fase de placa.
test-bmgr:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_bmgr.c src/bpvm_bmgr.c src/bpvm_bmgr_wire.c src/bpvm_part.c src/bpvm_env.c src/bpvm_pack.c src/bpvm_boot.c src/bpvm_util.c src/crc32.c -o build/test_bmgr.exe
	@echo "--- H9 board manager (esperado: solo 'ok', [status=OK]) ---"
	./build/test_bmgr.exe

# H3 — zona de packs (bpvm_pack): LIST/ADD/REMOVE sobre una región RAM que hace
# de flash. PARIDAD: las fixtures samples/PackFix*.pack las genera Pack.jar
# (lado PC) — el test recalcula los CRCs en C sobre esos bytes; un byte distinto
# entre C y Java = rojo (el micro rechazaría los packs del PC).
test-pack:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_pack.c src/bpvm_pack.c src/bpvm_util.c \
	      src/bpvm_bmgr_wire.c src/bpvm_bmgr.c src/bpvm_env.c src/bpvm_part.c \
	      src/bpvm_boot.c src/crc32.c src/fs_facade.c -o build/test_pack.exe
	@echo "--- H3 zona de packs (esperado: solo 'ok', [status=OK]) ---"
	./build/test_pack.exe

# #305 — el CRC por trozos DEBE dar lo mismo que el de una tacada: el IDE se salta
# el PUT comparándolo con java.util.zip.CRC32, así que una divergencia deja
# ficheros rancios en la placa EN SILENCIO. Vectores canónicos + tamaños frontera.
# #350 — agujero de GC en los builtins de cadenas ANTERIORES a #348: pop del
# origen -> alocar -> seguir leyendo del origen. Test NEGATIVO permanente.
# El heap ESTRECHO es parte del experimento (con holgura no colecta y pasa
# aunque el bug este vivo); y BPVM_GC_EVERY NO vale: enmascara en vez de
# destapar. Antes del fix daba sub_malos=3..6 sobre 3.000 vueltas.
test-gchole: $(BIN)
	$(call fixture,GcHoleStr)
	@echo "--- #350 GC en builtins de cadenas (esperado: sub_malos=0, fb_malos=0, fin) ---"
	@out=$$(cd samples && ../$(BIN) --mem=40960 GcHoleStr.mod 2>&1); st=$$?; 	 echo "$$out" | grep -vE 'INICIANDO|FIN DE'; 	 if echo "$$out" | grep -q 'sub_malos=0' && echo "$$out" | grep -q 'fb_malos=0' 	    && echo "$$out" | grep -q '^fin' && [ $$st -eq 0 ]; then 	   echo "PASS test-gchole"; 	 else echo "FAIL test-gchole"; exit 1; fi

# #347 — random / randomInt. Un generador NO se comprueba por igualdad (cada
# familia tiene su fuente, y la paridad dual-VM es CIEGA a esto). Se comprueba
# lo que SI es invariante: rango, que el extremo alto NO entre, cobertura de
# todos los valores, y el rango vacio.
test-rand: $(BIN)
	$(call fixture,RandRange)
	@echo "--- #347 random/randomInt (esperado: 0 malos, 0 altos, 0 sin cubrir) ---"
	@out=$$(cd samples && ../$(BIN) RandRange.mod 2>&1); st=$$?; 	 echo "$$out" | grep -vE 'INICIANDO|FIN DE'; 	 if echo "$$out" | grep -q 'rango_malos=0' && echo "$$out" | grep -q 'vio_el_alto=0' 	    && echo "$$out" | grep -q 'sin_cubrir=0' && echo "$$out" | grep -q 'vacio=5' 	    && [ $$st -eq 0 ]; then 	   echo "PASS test-rand"; 	 else echo "FAIL test-rand"; exit 1; fi

# #355/#357 — la lista de libres: ORDENADA por dirección y con FUSIÓN de huecos
# contiguos. Existe porque al instrumentarlo salió `fusion_izq=0 fusion_der=0`
# con el sample real: las dos ramas estaban escritas y SIN EJECUTAR NUNCA (ese
# programa apenas fragmenta y los huecos del barrido van separados por bloques
# vivos por construcción). Este test fuerza la adyacencia que el sample no da.
# V5/H4 — los DOS helpers con los que un `native` llama a un pack. Se prueban
# SOLOS, antes de que exista el emisor que los usa: si la conversion de cadena
# trunca o el resolutor devuelve el simbolo equivocado, el fallo sale dentro del
# pack —en la placa, con una BD abierta— y no se parece a lo que es.
# V5/H4 — EL CICLO ENTERO EN EL PC: un programa BP que consulta una base de
# datos de verdad. Junta lo que hasta ahora estaba probado por separado —la
# librería, el puente generado, el pack, SQLite y el VFS— porque mientras no
# estén juntos, "funciona" es una suma de promesas.
#
# La amalgama NO se copia al repo (9,5 MB): se saca del zip ya descargado, igual
# que hace el arnés de notas/. Las opciones son las MISMAS del binario de la
# placa, leídas de él y no supuestas.
SQLNOTAS = ../notas/v5-sqlite-prueba
SQLZIP   = /c/lenguajes/sqlite-amalgamation-3530400.zip
SQLDEF   = -DSQLITE_OS_OTHER=1 -DSQLITE_THREADSAFE=0 -DSQLITE_ENABLE_MEMSYS5            -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_TEMP_STORE=3

test-sqldemo: $(LIBOBJ)
	@mkdir -p build/sql
	@[ -f build/sql/sqlite3.c ] || unzip -o -j $(SQLZIP) "*/sqlite3.c" "*/sqlite3.h" 	    -d build/sql >/dev/null
	@if [ ! -f build/sql/aot_SQLite.c ]; then 	  echo "(falta build/sql/aot_SQLite.c — generalo con:"; 	  echo "   java -cp '../lexer-java/target/classes;../pack/target/classes' \\"; 	  echo "        basicplus.frontend.AotMain ../bpstdlib/SQLite.bp build/sql)"; 	  exit 1; 	fi
	@[ -f build/sql/sqlite3.o ] || $(CC) -O1 -w -c build/sql/sqlite3.c 	    -o build/sql/sqlite3.o -Ibuild/sql $(SQLDEF)
	$(CC) $(CFLAGS) -Isrc -Ibuild/sql -I$(SQLNOTAS)/H 	    test/test_sqldemo.c build/sql/aot_SQLite.c 	    $(SQLNOTAS)/I/sqlite_shim.c $(SQLNOTAS)/I/vfs_bp.c $(SQLNOTAS)/H/packglue.c 	    build/sql/sqlite3.o $(LIBOBJ) -o build/sqldemo.exe $(LDFLAGS) $(SQLDEF)
	@echo "--- V5/H4: un programa BP consultando una base de datos ---"
	@cd build/sql && rm -f medidas.db && ../sqldemo.exe SqlDemo.mod

test-packhelp: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_packhelp.c $(LIBOBJ) -o build/test_packhelp.exe $(LDFLAGS)
	@echo "--- V5/H4 string_to_cstr + pack_sym (esperado: solo 'ok', [status=OK]) ---"
	./build/test_packhelp.exe

# V5/H4 — el .mdn dentro del pack. Construye un pack de PRUEBA con el escritor
# REAL (Pack.jar) a partir del .mdn REAL, y comprueba que el escaneo lo
# encuentra ahi y que el del FS lo eclipsa DICIENDOLO. Ver la cabecera del .c
# para por que esto se prueba en host y no en placa.
PACKJAR = ../pack/target/pack-1.0.jar

test-mdnscan: $(LIBOBJ)
	@mkdir -p build/sql
	@if [ ! -f build/sql/SQLite.mdn ]; then 	  echo "(falta build/sql/SQLite.mdn — generalo con AotMain, como test-sqldemo)"; 	  exit 1; 	fi
	@cp -f build/sql/SQLite.mdn build/sql/_mdntest/SQLite.mdn 2>/dev/null || 	  (mkdir -p build/sql/_mdntest && cp -f build/sql/SQLite.mdn build/sql/_mdntest/SQLite.mdn)
	@java -cp "$(PACKJAR)" basicplus.pack.PackCli build 	    --out build/sql/mdntest.pack --name MdnTest --version 1 --date 0 	    build/sql/_mdntest/SQLite.mdn
	$(CC) $(CFLAGS) -Isrc test/test_mdnscan.c $(LIBOBJ) -o build/test_mdnscan.exe $(LDFLAGS)
	@echo "--- V5/H4: el .mdn se busca tambien en el pack (esperado: solo 'ok', [status=OK]) ---"
	./build/test_mdnscan.exe build/sql/mdntest.pack build/sql/SQLite.mdn

test-freelist: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_freelist.c $(LIBOBJ) -o build/test_freelist.exe $(LDFLAGS)
	@echo "--- lista de libres: orden + fusion izq/der (esperado: solo 'ok', [status=OK]) ---"
	./build/test_freelist.exe

# #362 — la ZONA de packs sirve RECURSOS (fuentes, iconos), no solo modulos.
# Los dos packs se construyen con PackCli (el escritor DE VERDAD) y traen el
# mismo 'logo.png' con contenido distinto: es el unico montaje donde se
# distingue 'cualificar funciona' de 'cualificar se ignora'.
test-packres: $(LIBOBJ)
	@mkdir -p build/packres/a build/packres/b
	@printf 'ICONOS' > build/packres/a/logo.png
	@printf 'OTROS'  > build/packres/b/logo.png
	@java -cp "$(PACKJAR)" basicplus.pack.PackCli build --out build/packres/iconos.pack --name Iconos --version 1 --date 0 build/packres/a/logo.png
	@java -cp "$(PACKJAR)" basicplus.pack.PackCli build --out build/packres/otros.pack  --name Otros  --version 1 --date 0 build/packres/b/logo.png
	$(CC) $(CFLAGS) -Isrc test/test_packres.c $(LIBOBJ) -o build/test_packres.exe $(LDFLAGS)
	@echo "--- #362 recursos desde la zona de packs (esperado: solo 'ok', [status=OK]) ---"
	@cd build && ./test_packres.exe packres/iconos.pack packres/otros.pack

test-crc:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_crc.c src/crc32.c src/fs_facade.c src/fs_host.c -o build/test_crc.exe
	@echo "--- #305 CRC por trozos (esperado: solo 'ok', [status=OK]) ---"
	@cd build && ./test_crc.exe

# H10 — el MICRO SIMULADO del IDE: servidor TCP wire v1 COMPLETO (META + FILES +
# TERMINAL + gestión de placa + packs) con FS littlefs sobre imagen y la VM-C de
# verdad ejecutando. El IDE lo trata como una placa más. Nació en H9 como
# boardsim (sólo STATE/ENV_*/PART_*); H10 le enchufa el FS y la VM → enlaza la
# lib entera ($(LIBOBJ)). Reusa pico/json_min.c (parser JSON portable).
# Winsock via $(LDFLAGS) (-lws2_32). Con LVGL=1 arrastra la ventana SDL.
sim: $(LIBOBJ) $(if $(filter 1,$(LVGL)),build/liblvgl.a build/SDL2.dll,)
	@mkdir -p build
	$(CC) $(CFLAGS) -Ipico -Isrc tools/bpvm_sim.c pico/json_min.c $(LIBOBJ) -o build/bpvm-sim.exe $(LDFLAGS)
	@echo "sim listo: build/bpvm-sim.exe [--port=N] [--mem=N] [--psram=N] [--flash=N] [--fs=IMG]"

# Alias histórico (H9): el boardsim ES el sim, con otro nombre.
boardsim: sim

# Smoke end-to-end del sim por el wire (arranca el sim, recorre el flujo de placa
# virgen, verifica cada reply). Necesita python3. No necesita placa ni IDE.
boardsim-smoke: sim
	@echo "--- boardsim smoke (protocolo de gestión de placa) ---"
	python tools/boardsim_smoke.py

# H10 — smoke del wire de FICHEROS + EJECUCIÓN contra el sim (PUT/LIST/GET/RUN…).
sim-smoke: sim
	@echo "--- H10 sim smoke (FILES + TERMINAL) ---"
	python tools/sim_smoke.py

test-oom: $(BIN)
	$(call fixture,OomCatch)
	@if [ ! -f samples/OomCatch.mod ]; then \
	  echo "(skip test-oom: falta samples/OomCatch.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/OomCatch.bp --compile samples --backend=mivm)"; \
	else \
	  echo "--- OOM atrapable (esperado: 'cazado OOM: No space in heap' + 'vivo tras OOM' + exit 0) ---"; \
	  out=$$(cd samples && ../$(BIN) OomCatch.mod 2>&1); st=$$?; \
	  echo "$$out" | grep -vE 'INICIANDO|FIN DE'; \
	  if echo "$$out" | grep -q "cazado OOM: No space in heap" && [ $$st -eq 0 ]; then \
	    echo "PASS test-oom"; \
	  else \
	    echo "FAIL test-oom (status=$$st — OOM no atrapable o cuelgue)"; exit 1; \
	  fi; \
	fi

# V4/GC — test negativo permanente: el mensaje de un RuntimeError NATIVO debe
# sobrevivir al GC (bug de la gen truncada en el campo `msg` del objeto
# RuntimeError, exceptions.c: heap_alloc_string devuelve el handle 64b truncado a
# uint32 → gen=0; si el slot del msg fue reciclado, leer e.msg gritaba
# "referencia a objeto eliminado" sobre un mensaje vivo). Corre con GC agresivo +
# memoria acotada para FORZAR el reciclaje de slots que destapa el bug.
# Esperado: 'caught msg=[ALOAD...' + 'fin', SIN 'use-after-free'.
test-excmsggc: $(BIN)
	$(call fixture,ExcMsgGc)
	@if [ ! -f samples/ExcMsgGc.mod ]; then \
	  echo "(skip test-excmsggc: falta samples/ExcMsgGc.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/ExcMsgGc.bp --compile samples --backend=mivm --libdir ../bpstdlib)"; \
	else \
	  echo "--- msg de RuntimeError sobrevive al GC (esperado: 'caught msg=[ALOAD...' + 'fin') ---"; \
	  out=$$(cd samples && BPVM_GC_EVERY=1 ../$(BIN) --mem=65536 ExcMsgGc.mod 2>&1); st=$$?; \
	  echo "$$out" | grep -vE 'INICIANDO|FIN DE'; \
	  if echo "$$out" | grep -q "caught msg=\[ALOAD" && ! echo "$$out" | grep -q "use-after-free" && [ $$st -eq 0 ]; then \
	    echo "PASS test-excmsggc"; \
	  else \
	    echo "FAIL test-excmsggc (status=$$st — msg del RuntimeError corrupto tras GC)"; exit 1; \
	  fi; \
	fi

# H2 fase A · B0.1 — smoke de littlefs vendorizado (third_party/littlefs). Sobre
# el block-device de RAM: format/mount/mkdir/write/unmount+remount/read+verify.
# Motor puro en host, aún sin tocar la VM (el futuro oráculo). Flags relajados
# para el código vendorizado (mismo criterio que gui-smoke).
LFS_SMOKE_BIN = build/test_lfs_smoke.exe
test-lfs:
	@mkdir -p build
	@if [ ! -f $(LFS_DIR)/lfs.c ]; then \
	  echo "(skip test-lfs: falta third_party/littlefs — vendorízalo, ver third_party/README.md)"; \
	else \
	  $(CC) -std=c99 -O2 $(LFS_INC) test/test_lfs_smoke.c $(LFS_CORE) \
	    $(LFS_DIR)/bd/lfs_rambd.c -o $(LFS_SMOKE_BIN); \
	  echo "--- littlefs B0.1 smoke (rambd: format/mount/mkdir/write/remount/read) ---"; \
	  ./$(LFS_SMOKE_BIN); \
	fi

# #330 — banco de FLASH SUCIA / BORRADO QUE MIENTE. Nació para comprobar la
# explicación que dimos al #328 ("la región no estaba borrada") ANTES de escribir
# el arreglo… y la REFUTÓ: con el erase funcionando, la basura previa no molesta.
# Lo que sí rompe es que el borrado no ocurra. Usa un block-device con semántica
# NOR de verdad (borrar→0xFF, programar→AND); lfs_filebd NO vale para esto.
test-lfsdirty:
	@mkdir -p build
	@if [ ! -f $(LFS_DIR)/lfs.c ]; then \
	  echo "(skip test-lfsdirty: falta third_party/littlefs)"; \
	else \
	  $(CC) -std=c99 -O2 $(LFS_INC) test/test_lfs_dirty.c $(LFS_CORE) \
	    -o build/test_lfs_dirty.exe; \
	  echo "--- littlefs: flash sucia vs borrado que miente (#330) ---"; \
	  ./build/test_lfs_dirty.exe; \
	fi

# H2 fase A · B0.2 — block-device RESPALDADO POR FICHERO (lfs_filebd): la base
# del oráculo VM-C-PC. write y read corren como DOS PROCESOS separados → la
# imagen .img debe persistir entre procesos, no solo entre mounts.
LFS_FILEBD_BIN = build/test_lfs_filebd.exe
LFS_IMG        = build/lfs_test.img
test-lfsfile:
	@mkdir -p build
	@if [ ! -f $(LFS_DIR)/lfs.c ]; then \
	  echo "(skip test-lfsfile: falta third_party/littlefs — vendorízalo, ver third_party/README.md)"; \
	else \
	  $(CC) -std=c99 -O2 $(LFS_INC) test/test_lfs_filebd.c $(LFS_CORE) \
	    $(LFS_DIR)/bd/lfs_filebd.c -o $(LFS_FILEBD_BIN); \
	  echo "--- littlefs B0.2 filebd (imagen en fichero, write y read en procesos separados) ---"; \
	  rm -f $(LFS_IMG); \
	  ./$(LFS_FILEBD_BIN) write $(LFS_IMG) && \
	  ./$(LFS_FILEBD_BIN) read  $(LFS_IMG) && \
	  rm -f $(LFS_IMG); \
	fi

# H2 fase A · B0.3 — INYECCIÓN DE FALLOS (lfs_emubd): el oráculo de robustez.
# Barrido exhaustivo de cortes de corriente en cada op de escritura (NOOP +
# OOO out-of-order) con invariantes de atomicidad/orden/usabilidad, + bloques
# dañados (PROGERROR → littlefs reubica). Determinista, imposible en placa.
LFS_FAULT_BIN = build/test_lfs_fault.exe
test-lfsfault:
	@mkdir -p build
	@if [ ! -f $(LFS_DIR)/lfs.c ]; then \
	  echo "(skip test-lfsfault: falta third_party/littlefs — vendorízalo, ver third_party/README.md)"; \
	else \
	  $(CC) -std=c99 -O2 $(LFS_INC) test/test_lfs_fault.c $(LFS_CORE) \
	    $(LFS_DIR)/bd/lfs_emubd.c $(LFS_DIR)/bd/lfs_rambd.c $(LFS_DIR)/bd/lfs_filebd.c \
	    -o $(LFS_FAULT_BIN); \
	  echo "--- littlefs B0.3 fault-injection (cortes exhaustivos + bloques malos) ---"; \
	  ./$(LFS_FAULT_BIN); \
	fi

# H2 fase A · B1.3 — dirs reales + op `list` + MONTAJES (andamiaje fase B):
# list sobre littlefs y host (contrato único), resolve/base-dir sobre dirs
# reales del oráculo, ruteo por prefijo (stub en /mnt), cross-mount → -1.
FSVFS_BIN = build/test_fs_vfs.exe
test-fsvfs: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_fs_vfs.c $(LIBOBJ) -o $(FSVFS_BIN) $(LDFLAGS)
	@echo "--- B1.3 dirs reales + list + montajes ---"
	./$(FSVFS_BIN)

# H2 fase A · B1.4a — estrés MULTITAREA C-level de la fachada sobre littlefs:
# N pthreads (modelan Threads BP + comm task) martillean write/append/stat
# sobre ficheros propios + un log compartido. Verificación por PROPIEDADES
# (byte-exacto + línea-atómica + conteos). Rojo→verde: sin el lock grueso,
# littlefs (no reentrante) asserta con 4 threads; con él, 0 corrupciones.
FSLFSMT_BIN = build/test_fs_lfs_mt.exe
test-fslfsmt: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_fs_lfs_mt.c $(LIBOBJ) -o $(FSLFSMT_BIN) $(LDFLAGS)
	@echo "--- B1.4a lock grueso del FS: control 1 thread + 4 + 8 (0 corrupciones) ---"
	./$(FSLFSMT_BIN) build/mt.img 1 60
	./$(FSLFSMT_BIN) build/mt.img 4 60
	./$(FSLFSMT_BIN) build/mt.img 8 60

# H2 fase A · B1.4b — el requisito de multitarea A NIVEL BP: 3 Threads BP
# escribiendo 4 ficheros a la vez sobre el ORÁCULO littlefs con 4 workers
# REALES (--smp=4). Verde = tamaños exactos + 0 líneas rotas.
test-iomt: $(BIN)
	$(call fixture,IoMt)
	@if [ ! -f samples/IoMt.mod ]; then \
	  echo "(skip test-iomt: falta samples/IoMt.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/IoMt.bp --compile samples --backend=mivm)"; \
	else \
	  echo "--- B1.4b Threads BP + littlefs + --smp=4 (esperado: 200/200/200/600, 0 rotas) ---"; \
	  rm -f build/iomt.img; \
	  out=$$(cd samples && ../$(BIN) --fs=lfs:../build/iomt.img --smp=4 IoMt.mod 2>&1); \
	  echo "$$out" | grep -vE 'INICIANDO|FIN DE'; \
	  if echo "$$out" | grep -q "lineas rotas = 0"; then echo "PASS test-iomt"; \
	  else echo "FAIL test-iomt"; exit 1; fi; \
	  rm -f build/iomt.img; \
	fi

# H2 fase A · B1.1 — la FACHADA bpvm_fs sobre littlefs (backend fs_lfs.c +
# constructor host fs_lfs_host.c, ya DENTRO de la lib). Afirma la semántica
# documentada (= espejo de fs_host.c) + persistencia entre attach + el
# contrato mount-primero-no-reformatear.
FSLFS_BIN = build/test_fs_lfs.exe
test-fslfs: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_fs_lfs.c $(LIBOBJ) -o $(FSLFS_BIN) $(LDFLAGS)
	@echo "--- B1.1 fachada bpvm_fs sobre littlefs (semántica espejo de fs_host) ---"
	./$(FSLFS_BIN)

test-throwmsg: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,ThrowMsg)
	@if [ ! -f samples/ThrowMsg.mod ]; then \
	  echo "(skip test-throwmsg: falta samples/ThrowMsg.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/ThrowMsg.bp --compile samples --backend=mivm)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_throwmsg.c test/aot_ThrowMsg.c \
	    $(LIBOBJ) -o $(THROW_BIN) $(LDFLAGS); \
	  echo "--- throw msg computado native->BP (esperado: 10, caught: valor negativo: -3, fin) ---"; \
	  (cd samples && ../$(THROW_BIN) ThrowMsg.mod); \
	fi

# #213 — throw de CLASE DE USUARIO desde native, cazado por try/catch BP con
# la clase del usuario. La instancia se construye en el INTÉRPRETE: el thunk
# cruza el puente call_bp_i32 a la factory ThrowUser.__cls_new_RangoError
# (corre el ctor real, incluido super() a Core.Exception) y throw_ref propaga
# el ref por pending_ref del boundary → eh_unwind. test/aot_ThrowUser.c es un
# SNAPSHOT del .c que genera AotCEmitter. Necesita Core.mod junto al .mod
# (RangoError extiende Core.Exception). Salida byte-idéntica a la VM-Java.
THROWUSER_BIN = build/test_throwuser.exe
test-throwuser: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,ThrowUser)
	@if [ ! -f samples/ThrowUser.mod ]; then \
	  echo "(skip test-throwuser: falta samples/ThrowUser.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/ThrowUser.bp --compile samples --backend=mivm)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_throwuser.c test/aot_ThrowUser.c \
	    $(LIBOBJ) -o $(THROWUSER_BIN) $(LDFLAGS); \
	  echo "--- throw clase de usuario native->BP (esperado: 10, caught: valor negativo (valor=-7), fin) ---"; \
	  (cd samples && ../$(THROWUSER_BIN) ThrowUser.mod); \
	fi

# #174 (mitad-VM) — native invoca un MÉTODO virtual de un objeto vía
# call_method_i32 (despacho por vtable). test/aot_MethodCall.c es un thunk a
# mano con el slot hard-coded (la mitad-compilador, que pondría el slot, es
# #174b). Salida byte-idéntica a la VM-Java.
METHOD_BIN = build/test_method.exe
test-method: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,MethodCall)
	@if [ ! -f samples/MethodCall.mod ]; then \
	  echo "(skip test-method: falta samples/MethodCall.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/MethodCall.bp --compile samples --backend=mivm)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_method.c test/aot_MethodCall.c \
	    $(LIBOBJ) -o $(METHOD_BIN) $(LDFLAGS); \
	  echo "--- método virtual desde native (esperado: [Box.getDoubled] v= 5, 11) ---"; \
	  (cd samples && ../$(METHOD_BIN) MethodCall.mod); \
	fi

# H6.b.1 — núcleo portable del debugger (breakpoints por pc + pausa/step/stop).
# Inyecta un pause_cb (sin wire ni hardware) y verifica step/breakpoint/stop +
# list/clear. DBGMOD = cualquier .mod sin deps externas (default Arith.mod).
DEBUG_BIN = build/test_debug.exe
DBGMOD   ?= samples/Arith.mod
test-debug: $(LIBOBJ)
	@mkdir -p build
	@if [ ! -f "$(DBGMOD)" ]; then \
	  echo "(skip test-debug: falta $(DBGMOD))"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_debug.c $(LIBOBJ) -o $(DEBUG_BIN) $(LDFLAGS); \
	  echo "--- núcleo debugger VM-C (esperado: OK al final) ---"; \
	  (cd samples && ../$(DEBUG_BIN) $(notdir $(DBGMOD))); \
	fi

# H6.b.2.a — pausa/resume del debugger cruzando threads (corazón del server
# host, sin sockets). El intérprete corre en un thread y bloquea en el pause_cb;
# el thread controlador inyecta STEP/CONTINUE por mutex+cond. Ver test_debug_mt.c.
DEBUGMT_BIN = build/test_debug_mt.exe
test-debug-mt: $(LIBOBJ)
	@mkdir -p build
	@if [ ! -f "$(DBGMOD)" ]; then \
	  echo "(skip test-debug-mt: falta $(DBGMOD))"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_debug_mt.c $(LIBOBJ) -o $(DEBUGMT_BIN) $(LDFLAGS); \
	  echo "--- pausa/resume cruzando threads (esperado: OK al final) ---"; \
	  (cd samples && ../$(DEBUGMT_BIN) $(notdir $(DBGMOD))); \
	fi

# H6.b.2.b — servidor wire del debugger en host (stand-in de la Pico, sin HW).
# Compila bpvm_dbgserver (TCP + wire-v1 device-role) y lo conduce con un cliente
# Python que verifica HELLO/PING/PAUSE/RUN/BP_HIT/SET_BP/STEP/CONTINUE/EXITED.
LISTEN_BIN  = build/bpvm_dbgserver.exe
DBG_LDFLAGS = $(LDFLAGS)
ifeq ($(OS),Windows_NT)
  DBG_LDFLAGS += -lws2_32
endif
PYTHON ?= python
test-listen: $(LIBOBJ)
	@mkdir -p build
	@if [ ! -f "$(DBGMOD)" ]; then \
	  echo "(skip test-listen: falta $(DBGMOD))"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/debug_listen.c $(LIBOBJ) -o $(LISTEN_BIN) $(DBG_LDFLAGS); \
	  echo "--- wire debugger host (cliente Python) ---"; \
	  $(PYTHON) test/dbg_client.py "$(abspath $(LISTEN_BIN))" "$(notdir $(DBGMOD))" samples; \
	fi

# H4.2 — smoke de toolchain LVGL v9 + SDL2 (vendorizados en third_party/, gitignored).
# Compila las 311 fuentes de LVGL (flags relajados, sin -Wpedantic) + el driver SDL
# + el smoke, enlaza SDL2 y abre una ventana. Valida el cimiento de build ANTES de
# integrar LVGL en gui.c. SDL2.dll se copia junto al exe.
# LVGL_DIR/SDL2_DIR/LVGL_INC se definen arriba (sección backend GUI).
# OJO: el `find` va en la RECETA (shell Unix), no en $(shell ...) (que en este
# w64devkit resuelve al find.exe de Windows/System32 → lista vacía).
gui-smoke:
	@mkdir -p build
	$(CC) -std=gnu11 -O2 $(LVGL_INC) test/gui_smoke.c $$(find $(LVGL_DIR)/src -name '*.c') \
	  -L$(SDL2_DIR)/lib -lSDL2 -o build/gui_smoke.exe
	cp $(SDL2_DIR)/bin/SDL2.dll build/
	./build/gui_smoke.exe

# H6 (image) — verificación HEADLESS del decode (lodepng): SIN abrir ventana,
# valida que el decoder reconoce el lv_image_dsc_t en RAM (src VARIABLE) y saca
# las dimensiones del PNG → la cadena que en placa pinta los píxeles. Necesita
# liblvgl.a (con LV_USE_LODEPNG=1) y SDL2.dll: úsalo como `make LVGL=1 gui-imgdecode`.
gui-imgdecode: build/liblvgl.a build/SDL2.dll
	$(CC) -std=gnu11 -O2 $(LVGL_INC) test/gui_imgdecode.c build/liblvgl.a \
	  -L$(SDL2_DIR)/lib -lSDL2 -o build/gui_imgdecode.exe
	./build/gui_imgdecode.exe ../bpstdlib/testimg.png
