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
          src/fs_facade.c src/fs_host.c src/crc32.c src/bpvm_env.c src/bpvm_part.c src/bpvm_boot.c src/bpvm_bmgr.c src/bpvm_bmgr_wire.c \
          src/bpvm_pack.c \
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

.PHONY: all clean smoke test-callbp test-xmodule test-throwmsg test-throwuser test-method test-debug test-debug-mt test-listen test-bytenat test-xmodnat test-compressnat test-compressbench test-kill test-net test-smphandles test-oom test-excmsggc test-lfs test-lfsfile test-lfsfault test-fslfs test-fsvfs test-fslfsmt test-iomt test-env test-part test-boot test-bmgr test-pack boardsim boardsim-smoke gui-smoke gui-imgdecode

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

# #169 — método de clase EXTERNA desde native: XMethodNat.useBox (native) llama a
# XClassLib.Box.getDoubled() vía call_method_i32 con el slot resuelto del .bpi
# (ClassSig/slotOf, base H13.1). El loader auto-carga XClassLib.mod vía discover_deps.
XMETHODNAT_BIN = build/test_xmethodnat.exe
test-xmethodnat: $(LIBOBJ)
	@mkdir -p build
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

# H9 — board manager: núcleo portable del protocolo de gestión de placa (ENV_*/
# PART_*). Compone env+part. Host-only: opera sobre sectores A/B en RAM; simula el
# flujo virgen→defaults→apply→edit y el volcado del slot rancio. La cintura de flash
# (volcar wrote_slot) y el adaptador JSON del firmware llegan en la fase de placa.
test-bmgr:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_bmgr.c src/bpvm_bmgr.c src/bpvm_part.c src/bpvm_env.c src/crc32.c -o build/test_bmgr.exe
	@echo "--- H9 board manager (esperado: solo 'ok', [status=OK]) ---"
	./build/test_bmgr.exe

# H3 — zona de packs (bpvm_pack): LIST/ADD/REMOVE sobre una región RAM que hace
# de flash. PARIDAD: las fixtures samples/PackFix*.pack las genera Pack.jar
# (lado PC) — el test recalcula los CRCs en C sobre esos bytes; un byte distinto
# entre C y Java = rojo (el micro rechazaría los packs del PC).
test-pack:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_pack.c src/bpvm_pack.c -o build/test_pack.exe
	@echo "--- H3 zona de packs (esperado: solo 'ok', [status=OK]) ---"
	./build/test_pack.exe

# H9 — "device" simulado para el IDE (FrmBoard) SIN placa: servidor TCP wire v1 que
# despacha STATE/ENV_*/PART_* al bpvm_bmgr REAL, con el env respaldado por un fichero
# A/B = la "flash". Una sola fuente de verdad (misma lógica y mismo wire que placa).
# Reusa pico/json_min.c (parser JSON portable). Winsock via $(LDFLAGS) (-lws2_32).
boardsim:
	@mkdir -p build
	$(CC) $(CFLAGS) -Ipico tools/boardsim.c src/bpvm_bmgr.c src/bpvm_bmgr_wire.c src/bpvm_part.c src/bpvm_env.c src/bpvm_boot.c src/bpvm_pack.c src/crc32.c pico/json_min.c -o build/bpvm-boardsim.exe $(LDFLAGS)
	@echo "boardsim listo: build/bpvm-boardsim.exe [puerto] [flash-file] [flashSizeBytes]"

# H9 — smoke end-to-end del sim por el wire (arranca el sim, recorre el flujo de
# placa virgen, verifica cada reply). Necesita python3. No necesita placa ni IDE.
boardsim-smoke: boardsim
	@echo "--- H9 boardsim smoke (esperado: solo 'ok', [status=OK]) ---"
	python tools/boardsim_smoke.py

test-oom: $(BIN)
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
