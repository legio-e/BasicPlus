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
          src/pulse.c src/pwm.c src/pico.c src/rtc.c src/adc.c src/wdt.c \
          src/aot_registry.c src/bpvm_aot_helpers.c \
          src/scheduler_smp.c src/comm_common.c src/comm_host.c
LDFLAGS = -lpthread
TEST    = test/main.c
OBJ     = $(SRC:%.c=build/%.o) build/test/main.o
BIN     = build/bpgenvm-c

ifeq ($(OS),Windows_NT)
  BIN := $(BIN).exe
endif

.PHONY: all clean smoke test-callbp test-xmodule test-throwmsg test-method test-debug test-debug-mt

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
