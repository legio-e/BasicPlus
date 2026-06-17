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

SRC     = src/bpvm.c src/loader.c src/interp.c src/heap.c src/builtins.c \
          src/link.c src/scheduler.c src/threading.c src/exceptions.c \
          src/platform_pthread.c src/gpio.c src/i2c.c src/spi.c src/uart.c \
          src/pulse.c src/pwm.c src/pico.c src/bpvm_neopixel.c src/rtc.c src/adc.c src/wdt.c \
          src/aot_registry.c src/bpvm_aot_helpers.c src/mdn_loader.c \
          src/scheduler_smp.c src/comm_common.c src/comm_host.c \
          src/fs_facade.c src/fs_host.c \
          src/net.c src/net_host.c

# V3/H4 — backend GUI. Eje de FEATURE (build-time), no de placa: dos imágenes,
# con y sin GUI (las dos del micro). GUI=1 (default host) incluye src/gui.c +
# -DBPVM_GUI; GUI=0 deja la VM lean → los builtins GUI caen al "no implementado".
GUI ?= 1
ifeq ($(GUI),1)
  CFLAGS += -DBPVM_GUI
  SRC    += src/gui.c
endif

LDFLAGS = -lpthread
TEST    = test/main.c
OBJ     = $(SRC:%.c=build/%.o) build/test/main.o
BIN     = build/bpgenvm-c

ifeq ($(OS),Windows_NT)
  BIN := $(BIN).exe
  LDFLAGS += -lws2_32    # H11 — Winsock2 (net_host.c)
endif

.PHONY: all clean smoke test-callbp test-xmodule test-throwmsg test-throwuser test-method test-debug test-debug-mt test-listen test-bytenat test-xmodnat test-compressnat test-compressbench test-kill test-net gui-smoke

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: %.c
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

# #240 — Compress.decompress (LZSS) en variante NATIVE. Compila el thunk AOT
# (byte[] + shl/shr/mod, helpers array_load_u8 / array_store_i8) y corre
# CompressNat.mod. La salida (n = 6 / out = ABABAB) debe ser byte-idéntica a la
# VM-Java y a la versión plain-BP de bpstdlib/Compress.bp. Ver test/test_compressnat.c.
COMPRESSNAT_BIN = build/test_compressnat.exe
test-compressnat: $(LIBOBJ)
	@mkdir -p build
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
	@if [ ! -f samples/TcpEchoTest.mod ]; then \
	  echo "(skip test-net: falta samples/TcpEchoTest.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/TcpEchoTest.bp --compile samples --backend=mivm)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_net.c \
	    $(LIBOBJ) -o $(NET_BIN) $(LDFLAGS); \
	  echo "--- TCP echo H11 (esperado: conectado / 2 ecos / timeout-len: 0 / fin, PASS) ---"; \
	  ./$(NET_BIN) samples/TcpEchoTest.mod ../bpstdlib/Net.mod ../bpstdlib/Core.mod; \
	fi

KILL_BIN = build/test_kill.exe
test-kill: $(LIBOBJ)
	@mkdir -p build
	@if [ ! -f samples/KillLoop.mod ] || [ ! -f samples/KillSleep.mod ]; then \
	  echo "(skip test-kill: faltan samples/KillLoop.mod / KillSleep.mod — regenéralos con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/KillLoop.bp --compile samples --backend=mivm  (ídem KillSleep))"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_kill.c \
	    $(LIBOBJ) -o $(KILL_BIN) $(LDFLAGS); \
	  echo "--- KILL cooperativo (esperado: 3x 'terminado por KILL ... PASS') ---"; \
	  (cd samples && ../$(KILL_BIN) KillLoop.mod KillSleep.mod); \
	fi

# #175 — throw RuntimeError(<msg computado>) desde native, cazado por try/catch
# BP. test/aot_ThrowMsg.c es un SNAPSHOT del .c que genera AotCEmitter (usa
# throw_str con el string-handle computado). Salida byte-idéntica a la VM-Java.
THROW_BIN = build/test_throwmsg.exe
test-throwmsg: $(LIBOBJ)
	@mkdir -p build
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
LVGL_DIR  = third_party/lvgl
SDL2_DIR  = third_party/SDL2/x86_64-w64-mingw32
LVGL_INC  = -I$(LVGL_DIR) -Iinclude -DLV_CONF_INCLUDE_SIMPLE -I$(SDL2_DIR)/include
# OJO: el `find` va en la RECETA (shell Unix), no en $(shell ...) (que en este
# w64devkit resuelve al find.exe de Windows/System32 → lista vacía).
gui-smoke:
	@mkdir -p build
	$(CC) -std=gnu11 -O2 $(LVGL_INC) test/gui_smoke.c $$(find $(LVGL_DIR)/src -name '*.c') \
	  -L$(SDL2_DIR)/lib -lSDL2 -o build/gui_smoke.exe
	cp $(SDL2_DIR)/bin/SDL2.dll build/
	./build/gui_smoke.exe
