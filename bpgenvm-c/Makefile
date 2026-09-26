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
# #472 — el host es el UNICO build con varios workers, asi que es el unico que
# necesita TLS para el fault-slot del AOT. Se declara por CAPACIDAD y aqui,
# no por familia dentro del .c: asi una familia nueva que no defina nada cae
# en la rama del micro (global plano), que es la correcta. Quitar esto deja
# a los N workers del host compartiendo un jmp_buf: corrupcion SILENCIOSA
# bajo carga, que ni el compilador ni el diff de paridad ven.
CFLAGS  += -DBPVM_TIENE_TLS
CFLAGS  += $(CFLAGS_EXTRA)
# H3: genera ficheros .d con las dependencias de cabecera de cada .o, para que
# editar un header (p.ej. bpvm_internal.h, la struct) recompile los .c que lo
# incluyen. Sin esto, un cambio de struct dejaba .o desalineados (build stale).
CFLAGS  += -MMD -MP

# ⚠️ src/bpvm_repl.c NO va en LIBOBJ a proposito (V6/U3): es capa-WIRE, no core
# de VM. Desde U3.24 SI lo enlaza el `sim` (abajo), que es el quinto consumidor:
# tener el REPL comun en el host es lo que convierte una prueba de placa en un
# `make sim-smoke`. Original:
# ⚠️ src/bpvm_repl.c NO va aqui A PROPOSITO (V6/U3): es capa-WIRE, no core de
# VM — llama al cable (wire_v1_send_*) que el CLI de host no tiene. Su sitio
# son los CUATRO firmwares (+ el simulador cuando migre). Ver bpvm_repl.h.
SRC     = src/bpvm.c src/bpvm_util.c src/loader.c src/interp.c src/heap.c src/builtins.c \
          src/link.c src/scheduler.c src/threading.c src/exceptions.c src/events.c \
          src/platform_pthread.c src/gpio.c src/i2c.c src/spi.c src/uart.c \
          src/pulse.c src/pwm.c src/bpvm_machine.c src/bpvm_neopixel.c src/rtc.c src/adc.c src/wdt.c \
          src/aot_registry.c src/bpvm_aot_helpers.c src/mdn_loader.c \
          src/bpvm_log.c \
          src/scheduler_smp.c src/comm_common.c src/comm_host.c src/bpvm_io.c \
          src/fs_facade.c src/fs_host.c src/crc32.c src/bpvm_env.c src/bpvm_part.c src/bpvm_boot.c src/bpvm_sqlmem.c src/bpvm_mem.c src/bpvm_mods.c src/bpvm_handles.c src/bpvm_bios.c src/bpvm_bios_fs.c src/bpvm_bios_hal.c src/bpvm_bus.c src/bpvm_npack.c src/bpvm_bmgr.c src/bpvm_bmgr_wire.c src/bpvm_dbg_wire.c \
          src/bpvm_pack.c src/bpvm_mdn_scan.c src/bpvm_ctx.c src/bpvm_nativa.c \
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

.PHONY: all clean smoke test-ctx test-aotswitch test-aotstr test-aotanida test-aotcede test-aotexpropia test-aotkill test-aotfloat test-wirerecorte test-sd-turnos test-sd-turnos-control test-mdnreceta test-callbp test-xmodule test-throwmsg test-throwuser test-method test-debug test-debug-mt test-listen test-bytenat test-xmodnat test-compressnat test-compressbench test-kill test-net test-smphandles test-oom test-excmsggc test-lfs test-lfsfile test-lfsfault test-fslfs test-fscursor test-fsvfs test-fslfsmt test-iomt test-fspos test-blk test-sdio test-listdir test-env test-part test-boot test-bmgr test-pack test-crc test-gchole test-rand test-freelist test-runguard sim sim-smoke test-listtrunc boardsim boardsim-smoke gui-smoke gui-imgdecode check-builtins

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
#   3. su MAGIC no es el que EMITE el compilador de hoy (BPVM_MAGIC_V9) -> regenerar
#
# El magic sale de include/bpvm_internal.h, que es donde el loader lo lee: si
# mañana el formato sube a v7, este guardián se entera solo. Comparación en
# HEX (od) para no depender de printf \xHH, que no es POSIX.
#
# Un test que falla por su andamio enseña a ignorarlo, que es la peor avería.
# ---------------------------------------------------------------------------
# V7/H2 (#523 V11, 17-sep) — $(2) = formato del .mod (9 si se omite): el guardián
# compara con el magic de ESE formato y compila con --formato=$(2). Y tras regenerar
# vuelve a mirar el magic: si el compilador falló, el .mod viejo seguía ahí y el
# `|| true` lo tapaba (así se coló un XMethodNat MOD8 «regenerado» a MOD9 — la
# native no traduce en MOD9 y el .mod no se escribe). Ahora lo canta y para.
# (26-sep) Antes de recompilar se BORRA el .mod: si el compilador falla, el viejo —con el formato bueno—
# pasaba la comprobación de abajo y la prueba corría con él, callada. Lo destapó test-aotcede: un .bp
# que no compilaba, un .mod de antes y un C del AOT nuevo — y el fallo salió como una diferencia de
# salida, no como «el compilador ha fallado».
define fixture
@for s in ../bpstdlib/*.mod; do b=$$(basename $$s); if [ -f samples/$$b ] && ! cmp -s $$s samples/$$b; then echo "  (samples/$$b era otra version de la stdlib: se copia la de bpstdlib)"; cp $$s samples/$$b; fi; done
@fmt=$(if $(2),$(2),9); want=$$(sed -n "s/.*BPVM_MAGIC_V$$fmt[[:space:]]*0x\([0-9A-Fa-f]\{8\}\).*/\1/p" include/bpvm_internal.h | head -1 | tr a-z A-Z); \
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
    rm -f samples/$$m.mod; \
    java -jar ../lexer-java/target/basicplus-frontend.jar \
         samples/$$m.bp --compile samples --backend=mivm --formato=$$fmt >/dev/null 2>&1 || true; \
    got=$$(od -An -tx1 -N4 samples/$$m.mod 2>/dev/null | tr -d ' \n' | tr a-z A-Z); \
    if [ "$$got" != "$$want" ]; then \
      echo "  ✗ samples/$$m.mod NO se ha regenerado como MOD$$fmt (el compilador ha fallado; míralo a mano):"; \
      java -jar ../lexer-java/target/basicplus-frontend.jar samples/$$m.bp --compile samples --backend=mivm --formato=$$fmt 2>&1 | grep -i "error\|no compilado" | head -3 | sed 's/^/     /'; \
      exit 1; \
    fi; \
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

# $(call aotmain,Mod,outDir) — el AOT de samples/Mod.bp para una prueba de host,
# con la stdlib FRESCA en su outDir (#541). AotMain sólo busca la stdlib en el
# BpVM.cfg del cwd, y sin la interfaz de Core —el import implícito de H1 P3— no
# da error: emite ranuras fijas para los métodos de una clase (areaDe salía 0
# con status=OK) o rechaza lo que usa sus primitivas (#542). En el checkout
# principal lo tapaba un samples/Core.mod suelto que encontraba junto al fuente.
# Lo que AotMain dice queda en outDir/aot.log (iba a /dev/null) y un aviso de
# «sin interfaz» es rojo.
define aotmain
@rm -rf $(2) && mkdir -p $(2) && cp ../bpstdlib/*.mod $(2)/
@java -cp "../lexer-java/target/classes;../pack/target/classes" basicplus.frontend.AotMain samples/$(1).bp $(2) >$(2)/aot.log 2>&1 || { cat $(2)/aot.log; exit 1; }
@# #542 (26-sep): un import sin interfaz ya no es un aviso que haya que buscar en el log: AotMain sale
@# con 4 y «falta el modulo 'X'», y lo para el `||` de arriba.
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
# V6/C1: la lib depende de lv_conf.h — tocar un interruptor (LZ4, snapshot...)
# la rehace entera en vez de dejar una lib rancia que enlaza en silencio.
build/liblvgl.a: include/lv_conf.h
	@rm -rf build/lvgl
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

# V6/#476 — LAS DOS TABLAS DE BUILTINS, QUE SE MANTIENEN A MANO.
# El id es el ordinal() del enum Builtin de miVM y aqui van los mismos numeros
# escritos a mano: si divergen, la VM-C ejecuta OTRO builtin y el sintoma no
# apunta a esto. Texto contra texto, sin build ni placa. Andamio hasta que el
# modulo raiz (V7) las GENERE de un fichero unico.
check-builtins:
	python ../scripts/gen_builtins.py   # H1 P2: las tablas se GENERAN de Core.bp; esto comprueba que casan

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
	$(call fixture,BridgeLib BridgeApp)
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
	$(call fixture,Helper XmodNat)
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
# V7/H2 (#523 V11, 17-sep) — en --formato=8 A PROPÓSITO: es el AOT de un método de clase
# EXTERNA por ranura horneada, que en MOD9 la fase 1 del AOT rechaza (la ranura la da el
# cargador). La fase 2 (la ranura por nombre desde el .mdn) es la ficha #533; cuando
# esté, este fixture pasa a 9 y el .mdn deja de hornear.
XMETHODNAT_BIN = build/test_xmethodnat.exe
test-xmethodnat: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,XClassLib XMethodNat)
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
# V6/#489 — la cola de eventos del GUI descartaba en silencio. Este test FUERZA el
# desbordamiento (600 eventos en 512 huecos, sin drenar) y comprueba que el primer
# descarte se dice y el total al terminar. Ver test/test_guidrop.c.
GUIDROP_BIN = build/test_guidrop.exe
# V7/H6 P2 — la COSTURA VM <-> pack de la GUI: los trampolines GENERADOS (gui_pack.c,
# de bpvm_gui.h por scripts/gen_gui_pack.py) contra un DOBLE del pack (generado
# tambien). Sin la VM: la prueba es de la frontera. Primero comprueba que lo
# generado esta al dia con la cabecera: un trampolin rancio es una ranura
# equivocada sin error de compilacion.
test-guipack:
	@mkdir -p build
	python ../scripts/gen_gui_pack.py --comprobar
	$(CC) $(CFLAGS) -DBPVM_GUI_PACK -Isrc test/test_guipack.c test/gui_pack_stubs.c src/gui_pack.c src/bpvm_bios.c -o build/test_guipack.exe
	@echo "--- V7/H6: la costura VM <-> pack de la GUI (esperado: solo 'ok', [status=OK]) ---"
	./build/test_guipack.exe

# V7/H6.c G4.a1 — los buses SPI e I2C como recurso COMPARTIDO de la HAL BP: la
# configuración se establece una vez (el ENV o el primero), otra es un error con
# nombre, se puede soltar (salvo la del ENV o si la usa otro) y el fin de RUN suelta
# lo del programa; contra una cintura que CUENTA aperturas y cierres (el fallo de
# antes: cada init reabría el bus). Ver test/test_bus.c y V7_IDEAS §H6.11.
test-bus: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_bus.c $(LIBOBJ) -o build/test_bus.exe $(LDFLAGS)
	@echo "--- V7/H6.c G4.a1-G4.a2: el bus compartido y su turno (esperado: solo 'ok', [status=OK]) ---"
	./build/test_bus.exe

# V7/H6.c G4.a2 — EL ÁRBITRO de los buses, con DOS HILOS de verdad: la pantalla (CS 10,
# 40 MHz) y la TF (CS 23, 20 MHz) machacando el mismo SPI 2 contra una cintura que graba
# cada trozo con los dos CS y la velocidad del instante. Ninguno puede salir mezclado.
test-arbitro: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_arbitro.c $(LIBOBJ) -o build/test_arbitro.exe $(LDFLAGS)
	@echo "--- V7/H6.c G4.a2: el arbitro con dos hilos (esperado: solo 'ok', [status=OK]) ---"
	./build/test_arbitro.exe

# ...y su CONTROL: el mismo test con el árbitro APAGADO (-DBPVM_SIN_ARBITRO, sólo en
# un bpvm_bus.o aparte) TIENE que fallar. Si pasara, la prueba no probaría nada.
test-arbitro-control: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -DBPVM_SIN_ARBITRO -c src/bpvm_bus.c -o build/bpvm_bus_sin_arbitro.o
	$(CC) $(CFLAGS) -Isrc test/test_arbitro.c $(filter-out build/src/bpvm_bus.o,$(LIBOBJ)) build/bpvm_bus_sin_arbitro.o -o build/test_arbitro_control.exe $(LDFLAGS)
	@echo "--- CONTROL: el arbitro APAGADO (esperado: que la prueba FALLE) ---"
	@./build/test_arbitro_control.exe > build/test_arbitro_control.txt 2>&1; \
	 if grep -q "status=FAIL" build/test_arbitro_control.txt; then \
	   grep -E "FAIL|primer trozo" build/test_arbitro_control.txt; \
	   echo "  ok  : sin arbitro la cinta sale MEZCLADA: el control falla, como debe [status=OK]"; \
	 else cat build/test_arbitro_control.txt; echo "  FAIL: sin arbitro la prueba PASA: no prueba nada [status=FAIL]"; exit 1; fi

# V7/H3 SD.2 (24-sep) — LA TF SUELTA EL BUS EN SUS ESPERAS (ACMD41 al arrancar, la tarjeta grabando):
# una tarjeta de mentira detras de la cintura SPI y una pantalla en otro hilo del SO pidiendo turnos;
# se mide cuantos consigue la pantalla MIENTRAS la TF arranca y MIENTRAS la tarjeta graba.
test-sd-turnos: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_sd_turnos.c src/bpvm_sd.c $(LIBOBJ) -o build/test_sd_turnos.exe $(LDFLAGS)
	@echo "--- V7/H3 SD.2: la TF suelta el bus en sus esperas (esperado: solo 'ok', [status=OK]) ---"
	./build/test_sd_turnos.exe

# ...y su CONTROL: la TF de antes (-DBPVM_SD_TURNO_UNICO), que retenia el bus. TIENE que fallar.
test-sd-turnos-control: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -DBPVM_SD_TURNO_UNICO -Isrc test/test_sd_turnos.c src/bpvm_sd.c $(LIBOBJ) -o build/test_sd_turnos_control.exe $(LDFLAGS)
	@echo "--- CONTROL: la TF de antes, que retenia el bus (esperado: que la prueba FALLE) ---"
	@./build/test_sd_turnos_control.exe > build/test_sd_turnos_control.txt 2>&1; \
	 if grep -q "status=FAIL" build/test_sd_turnos_control.txt; then \
	   grep -E "FAIL" build/test_sd_turnos_control.txt; \
	   echo "  ok  : con la TF de antes la pantalla se queda sin bus: el control falla, como debe [status=OK]"; \
	 else cat build/test_sd_turnos_control.txt; echo "  ROJO: el control PASA: la prueba no mide nada"; exit 1; fi

# V7/H6.c G3 — el driver de pantalla GENÉRICO (MIPI-DCS por SPI, la placa en el ENV)
# contra el de la C5, en el PC: los dos contra una BIOS que GRABA los cables. Con el
# perfil de la C5 tienen que mandar lo mismo; los desfases del C6 y de las 135x240
# salen de ram/off; y cada errata del ENV o del .seq se dice con nombre. Los drivers
# se compilan tal cual, renombrando su entrada y su g_bios para que convivan. El
# .seq es el de verdad (el que va en el pack). Ver test/test_driverspi.c.
DRIVERS = ../bpstdlib/gui/drivers
test-driverspi:
	@mkdir -p build
	$(CC) $(CFLAGS) -Dg_bios=c5_g_bios -Dbp_pack_init=c5_pack_init -c $(DRIVERS)/c5/nativo/src/driver_c5.c -o build/drv_c5.o
	$(CC) $(CFLAGS) -Dg_bios=spi_g_bios -Dbp_pack_init=spi_pack_init -c $(DRIVERS)/spi/nativo/src/driver_spi.c -o build/drv_spi.o
	$(CC) $(CFLAGS) test/test_driverspi.c build/drv_c5.o build/drv_spi.o -o build/test_driverspi.exe
	@echo "--- V7/H6.c G3: el driver generico contra el de la C5 (esperado: solo 'ok', [status=OK]) ---"
	./build/test_driverspi.exe $(DRIVERS)/spi/resources/st7789.seq

test-guidrop: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_guidrop.c $(LIBOBJ) -o $(GUIDROP_BIN) $(LDFLAGS)
	@echo "--- #489: descartes de la cola del GUI (esperado: PASS + 2 avisos) ---"
	./$(GUIDROP_BIN)

# V7/H6.b #568 — la tabla de widgets LLENA ya no es un widget que no aparece: el builtin lanza
# «Gui: no caben mas widgets: esta GUI tiene sitio para N». Punta a punta —el modelo (gui.c) da la
# capacidad en negativo y el builtin la convierte en el error—, con samples/GuiLleno.bp, que crea
# etiquetas hasta pasarse y atrapa el error. SOLO VM-C: miVM no tiene tope, asi que no va al corpus de
# paridad. Con 512 nodos y la pantalla, caben 511. En check-testc.
test-guilleno: $(LIBOBJ) $(BIN)
	@mkdir -p build
	$(call fixture,GuiLleno)
	$(call stdlibdep,Core Gui Json Collections Str)
	@echo "--- #568: la tabla de widgets llena (esperado: el error con nombre tras 511 etiquetas) ---"
	@./$(BIN) samples/GuiLleno.mod > build/guilleno.out 2>/dev/null; \
	 if grep -qF "tras 511 etiquetas: Gui: no caben mas widgets: esta GUI tiene sitio para 512" build/guilleno.out; \
	 then echo "[status=OK] la tabla llena se dice con nombre (511 etiquetas + la pantalla = 512)"; \
	 else echo "  ROJO: la tabla de widgets llena no lanza el error esperado"; cat build/guilleno.out; exit 1; fi

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
test-fspos: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_fs_posicional.c $(LIBOBJ) -o build/test_fspos.exe $(LDFLAGS)
	@echo "--- V5/H2 escritura posicional (esperado: 17/17, 0 fallos) ---"
	@cd build && ./test_fspos.exe

# #423 — el INTERRUPTOR del log (`log=0|1` en el entorno). Cuatro cosas, y las
# cuatro importan: que por defecto este ENCENDIDO (el arranque queda registrado
# siempre, que es cuando menos se puede preguntar nada), que apagado no anada,
# que apagar NO BORRE lo ya escrito, y que se pueda volver a encender.
# #421 — que un fallo de carga DIGA QUE HA PASADO. Fija los cuatro mensajes y,
# sobre todo, que sean DISTINTOS entre si: antes los cuatro eran «IO error», y
# eso costo media mañana con un .mod rancio (15-ago).
# #302 paso 3 — raices GC del native COMPILADO. Nacio ROJO (16-ago) y se puso
# VERDE el 17 con el escaneo conservador de la pila de C (idea de Eduardo: que
# el GC mire donde el native YA tiene sus handles, en vez de shadow stack).
# Lo que vigila: un native que encadena dos alocaciones ("valor " +
# intToString(n)) con el GC forzado POR ALOCACION — la ventana convertida en
# certeza. Si esto vuelve a rojo, un handle retenido solo en un local/registro
# de C se esta reciclando en mitad de una expresion: corrupcion MUDA (NULs con
# status=OK). El detalle del arreglo: heap.c paso 2d + aot_call_guarded.
test-aotgc: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,RoTest)
	$(call stdlibdep,Core)
	$(call aotmain,RoTest,build/aotro)
	@cp build/aotro/aot_RoTest.c test/aot_RoTest.c
	$(CC) $(CFLAGS) -Isrc test/test_aotgc.c test/aot_RoTest.c $(LIBOBJ) 	  -o build/test_aotgc.exe $(LDFLAGS)
	@echo "--- #302p3 native compilado + GC por alocacion (VERDE = arreglado) ---"
	@./build/test_aotgc.exe samples/RoTest.mod 2>/dev/null | grep -q "valor 7" 	  && echo "[status=OK] el intermedio sobrevive al GC" 	  || { echo "[status=ROJO] el GC recicla el intermedio del native (esperado hoy)"; exit 1; }

# [V6/N1.5] El gemelo de test-aotgc para lo que una native FABRICA. Regenera el
# .c desde el .bp (nunca un artefacto rancio checkeado a mano) y lo EJECUTA con
# el GC colectando en cada alocacion.
# (#541, 24-sep) Y el word[] que escribe la native lo relee el INTERPRETE: con los
# helpers de 4 bytes (el fallo que cazo esta prueba) la native se leia bien a si
# misma y tocaPalabras daba 6000 igual; el interprete lee `0 1000 0`.
test-aotnew: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatNew)
	$(call stdlibdep,Core)
	$(call aotmain,NatNew,build/aotnew)
	@cp build/aotnew/aot_NatNew.c test/aot_NatNew.c
	$(CC) $(CFLAGS) -Isrc test/test_aotnew.c test/aot_NatNew.c $(LIBOBJ) 	  -o build/test_aotnew.exe $(LDFLAGS)
	@echo "--- N1.5: la native CREA array y objeto, con GC en cada alocacion ---"
	@./build/test_aotnew.exe samples/NatNew.mod 2>/dev/null > build/aotnew.out; 	 ok=1; 	 grep -q "sumaArray(5)  = 105" build/aotnew.out || { echo "  ROJO: sumaArray"; ok=0; }; 	 grep -q "sumaLargos()  = 7000000000" build/aotnew.out || { echo "  ROJO: sumaLargos"; ok=0; }; 	 grep -q "tocaPalabras()= 6000" build/aotnew.out || { echo "  ROJO: tocaPalabras (word[], 2 bytes/casilla)"; ok=0; }; grep -qF "w[0], w[1], w[2] = 1000 2000 3000" build/aotnew.out || { echo "  ROJO: el word[] que escribe tocaPalabras, releido por el interprete (otro ancho de casilla)"; ok=0; }; grep -q "mediaDobles() = 4" build/aotnew.out || { echo "  ROJO: mediaDobles (double[], f64)"; ok=0; }; grep -q "largoDeDos()  = 8" build/aotnew.out || { echo "  ROJO: largoDeDos (string[], refs con generacion + literal)"; ok=0; }; grep -q "sumaFloats()  = 4" build/aotnew.out || { echo "  ROJO: sumaFloats (float[])"; ok=0; }; grep -q "areaDe(6,7)   = 42" build/aotnew.out || { echo "  ROJO: areaDe"; ok=0; }; 	 if [ $$ok = 1 ]; then echo "[status=OK] arrays i32/i64/i16/f64/f32/ref y objeto sobreviven al GC"; 	 else cat build/aotnew.out; exit 1; fi

test-aotgc-control: test-aotgc
	@true

# V7/#546 — el switch DENTRO de una native, traducido a C: sujeto string (C no conmuta cadenas:
# el numero del primer case que casa, por string_eq), un case long con sujeto integer (en 64 bits),
# break/continue y un var al empezar un case; y (#548) continue en un for. El MISMO .mod con el
# AOT y, con --interp, sin el (GC en cada alocacion en los dos): la misma salida, 11/11 y ningun
# FALLO. El AOT, con la stdlib fresca (aotmain, #541): sin la interfaz de Core, rojo. En check-testc.
test-aotswitch: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatSwitch)
	$(call stdlibdep,Core)
	$(call aotmain,NatSwitch,build/aotswitch)
	$(CC) $(CFLAGS) -Isrc test/test_aotswitch.c build/aotswitch/aot_NatSwitch.c $(LIBOBJ) \
	  -o build/test_aotswitch.exe $(LDFLAGS)
	@echo "--- #546/#548 el switch y el continue en una native (esperado: AOT == interpretado, 11/11, sin FALLO) ---"
	@./build/test_aotswitch.exe samples/NatSwitch.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotswitch.aot; \
	 ./build/test_aotswitch.exe samples/NatSwitch.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotswitch.int; \
	 n=$$(grep -c '^ok ' build/aotswitch.aot); \
	 if grep -q '^FALLO' build/aotswitch.aot build/aotswitch.int; then grep -h '^FALLO' build/aotswitch.aot build/aotswitch.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotswitch.int build/aotswitch.aot; then diff build/aotswitch.int build/aotswitch.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 11 ]; then cat build/aotswitch.aot; echo "  ROJO: $$n de 11 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 11/11 con el AOT, igual que interpretado"

# V7/H5 (#569/#570, 26-sep) — LA LISTA A MANO DEL AOT contra el camino interpretado: charAt,
# charCodeAt, substring, intToString y los newXArray van a COPIAS en C de sus builtins, y una copia se
# quedaba atrás sin que nada lo dijera (charAt fuera de rango: "" con el AOT, un error interpretado).
test-aotstr: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatStr)
	$(call stdlibdep,Core)
	$(call aotmain,NatStr,build/aotstr)
	$(CC) $(CFLAGS) -Isrc test/test_aotstr.c build/aotstr/aot_NatStr.c $(LIBOBJ) \
	  -o build/test_aotstr.exe $(LDFLAGS)
	@echo "--- #569/#570 la lista a mano del AOT (esperado: AOT == interpretado, 15/15, sin FALLO) ---"
	@./build/test_aotstr.exe samples/NatStr.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotstr.aot; \
	 ./build/test_aotstr.exe samples/NatStr.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotstr.int; \
	 n=$$(grep -c '^ok ' build/aotstr.aot); \
	 if grep -q '^FALLO' build/aotstr.aot build/aotstr.int; then grep -h '^FALLO' build/aotstr.aot build/aotstr.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotstr.int build/aotstr.aot; then diff build/aotstr.int build/aotstr.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 15 ]; then cat build/aotstr.aot; echo "  ROJO: $$n de 15 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 15/15 con el AOT, igual que interpretado"

# V7/H5 (P2b.1.a, 26-sep) — EL CAMBIO DE CONTEXTO (bpvm_ctx), la pieza en ensamblador de la suspensión de
# natives: 100 000 saltos con estado vivo en los registros no volátiles, setjmp/longjmp y un marco grande
# DENTRO de la pila prestada.
test-ctx: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_ctx.c $(LIBOBJ) -o build/test_ctx.exe $(LDFLAGS)
	@echo "--- P2b.1.a el cambio de contexto (esperado: solo 'ok', [status=OK]) ---"
	./build/test_ctx.exe

# V7/H5 (P2b.1.c, 26-sep) — una native que llama a una función BP que se BLOQUEA mientras otro hilo avanza
# y reserva (GC en cada reserva): la native se suspende en su pila prestada y lo que guarda en C sobrevive.
test-aotcede: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatCede)
	$(call stdlibdep,Core)
	$(call aotmain,NatCede,build/aotcede)
	$(CC) $(CFLAGS) -Isrc test/test_aotcede.c build/aotcede/aot_NatCede.c $(LIBOBJ) \
	  -o build/test_aotcede.exe $(LDFLAGS)
	@echo "--- P2b.1.c suspender una native (esperado: AOT == interpretado, 7/7, sin FALLO) ---"
	@./build/test_aotcede.exe samples/NatCede.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotcede.aot; \
	 ./build/test_aotcede.exe samples/NatCede.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotcede.int; \
	 n=$$(grep -c '^ok ' build/aotcede.aot); \
	 if grep -q '^FALLO' build/aotcede.aot build/aotcede.int; then grep -h '^FALLO' build/aotcede.aot build/aotcede.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotcede.int build/aotcede.aot; then diff build/aotcede.int build/aotcede.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 7 ]; then cat build/aotcede.aot; echo "  ROJO: $$n de 7 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 7/7 con el AOT, igual que interpretado"

# V7/H5 (P2b.1.d, 26-sep) — LA LÍNEA DEL BUCLE: una native en un bucle que sólo termina si otro hilo corre.
# Sin la línea, la pasada con el AOT se cuelga: cada pasada lleva un tope de tiempo.
test-aotexpropia: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatExpropia)
	$(call stdlibdep,Core)
	$(call aotmain,NatExpropia,build/aotexpropia)
	$(CC) $(CFLAGS) -Isrc test/test_aotexpropia.c build/aotexpropia/aot_NatExpropia.c $(LIBOBJ) \
	  -o build/test_aotexpropia.exe $(LDFLAGS)
	@echo "--- P2b.1.d la línea del bucle: expropiar una native (esperado: AOT == interpretado, 4/4) ---"
	@timeout 30 ./build/test_aotexpropia.exe samples/NatExpropia.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotexpropia.aot; \
	 timeout 30 ./build/test_aotexpropia.exe samples/NatExpropia.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotexpropia.int; \
	 n=$$(grep -c '^ok ' build/aotexpropia.aot); \
	 if grep -q '^FALLO' build/aotexpropia.aot build/aotexpropia.int; then grep -h '^FALLO' build/aotexpropia.aot build/aotexpropia.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotexpropia.int build/aotexpropia.aot; then diff build/aotexpropia.int build/aotexpropia.aot; echo "  ROJO: el AOT no da lo que el interprete (¿se colgó y lo cortó el tope?)"; exit 1; fi; \
	 if [ "$$n" -ne 4 ]; then cat build/aotexpropia.aot; echo "  ROJO: $$n de 4 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 4/4 con el AOT, igual que interpretado"

# V7/H5 (P2b.1.d, 26-sep) — el KILL de una native en un bucle sin fin (con tope de tiempo).
test-aotkill: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatKill)
	$(call stdlibdep,Core)
	$(call aotmain,NatKill,build/aotkill)
	$(CC) $(CFLAGS) -Isrc test/test_aotkill.c build/aotkill/aot_NatKill.c $(LIBOBJ) \
	  -o build/test_aotkill.exe $(LDFLAGS)
	@echo "--- P2b.1.d el kill de una native en un bucle sin fin (esperado: los dos KILLED, [status=OK]) ---"
	@timeout 60 ./build/test_aotkill.exe samples/NatKill.mod 2>/dev/null || { echo "  ROJO: el kill no llegó (¿colgado? lo cortó el tope)"; exit 1; }

# V7/H5 (#571, 26-sep) — ANIDAR natives: native → BP → native, y la de fuera lanza DESPUÉS. El límite
# de fallos era un jmp_buf por worker que la interior dejaba apuntando a su marco muerto.
test-aotanida: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatAnida)
	$(call stdlibdep,Core)
	$(call aotmain,NatAnida,build/aotanida)
	$(CC) $(CFLAGS) -Isrc test/test_aotanida.c build/aotanida/aot_NatAnida.c $(LIBOBJ) \
	  -o build/test_aotanida.exe $(LDFLAGS)
	@echo "--- #571 anidar natives (esperado: AOT == interpretado, 3/3, sin FALLO) ---"
	@./build/test_aotanida.exe samples/NatAnida.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotanida.aot; \
	 ./build/test_aotanida.exe samples/NatAnida.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotanida.int; \
	 n=$$(grep -c '^ok ' build/aotanida.aot); \
	 if grep -q '^FALLO' build/aotanida.aot build/aotanida.int; then grep -h '^FALLO' build/aotanida.aot build/aotanida.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotanida.int build/aotanida.aot; then diff build/aotanida.int build/aotanida.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 3 ]; then cat build/aotanida.aot; echo "  ROJO: $$n de 3 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 3/3 con el AOT, igual que interpretado"

# V7/H5 (#559, 25-sep) — LA COMA FLOTANTE DENTRO DE UNA NATIVE, ejecutada en el PC por sus DOS
# caminos. El .c que el AOT genera se compila tal cual (la FPU del PC hace los float, como el P4 y
# los Cortex-M33) y con -DBPVM_AOT_FLOAT_SOFT=1 (cada operacion y conversion de float por su
# helper, como en la C5, la C3 y la C6: el camino que hasta hoy solo existia en esas placas). Los
# dos binarios corren el MISMO .mod que el interprete (--interp) y la salida ENTERA tiene que ser
# la suya —asi cuenta tambien el print de un float hecho desde la native— sin ningun FALLO.
# La fuente es la de la tanda (../samples/FloatNat.bp): una sola copia.
AOTFLOAT_OK = 31
test-aotfloat: $(LIBOBJ)
	@rm -rf build/aotfloat && mkdir -p build/aotfloat && cp ../bpstdlib/*.mod build/aotfloat/
	@java -jar ../lexer-java/target/basicplus-frontend.jar ../samples/FloatNat.bp --compile build/aotfloat --backend=mivm >build/aotfloat/compile.log 2>&1 || { cat build/aotfloat/compile.log; exit 1; }
	@test -f build/aotfloat/FloatNat.mod || { cat build/aotfloat/compile.log; echo "  ROJO: FloatNat.bp no compila"; exit 1; }
	@java -cp "../lexer-java/target/classes;../pack/target/classes" basicplus.frontend.AotMain ../samples/FloatNat.bp build/aotfloat >build/aotfloat/aot.log 2>&1 || { cat build/aotfloat/aot.log; exit 1; }
	@if grep -q "sin interfaz" build/aotfloat/aot.log; then echo "  ROJO: el AOT de FloatNat no ve la interfaz de un import (#542):"; grep "sin interfaz" build/aotfloat/aot.log; exit 1; fi
	$(CC) $(CFLAGS) -Isrc test/test_aotfloat.c build/aotfloat/aot_FloatNat.c $(LIBOBJ) \
	  -o build/test_aotfloat.exe $(LDFLAGS)
	$(CC) $(CFLAGS) -Isrc -DBPVM_AOT_FLOAT_SOFT=1 test/test_aotfloat.c build/aotfloat/aot_FloatNat.c $(LIBOBJ) \
	  -o build/test_aotfloat_soft.exe $(LDFLAGS)
	@echo "--- #559 float y double en una native: FPU del PC y helpers de las placas sin FPU (esperado: los dos == interpretado, $(AOTFLOAT_OK) ok, sin FALLO) ---"
	@./build/test_aotfloat.exe build/aotfloat/FloatNat.mod --interp 2>/dev/null > build/aotfloat/int.out; \
	 ./build/test_aotfloat.exe build/aotfloat/FloatNat.mod 2>/dev/null > build/aotfloat/fpu.out; \
	 ./build/test_aotfloat_soft.exe build/aotfloat/FloatNat.mod 2>/dev/null > build/aotfloat/soft.out; \
	 if grep -h '^FALLO' build/aotfloat/fpu.out build/aotfloat/soft.out build/aotfloat/int.out; then echo "  ROJO: FALLO"; exit 1; fi; \
	 for c in fpu soft; do \
	   if ! cmp -s build/aotfloat/int.out build/aotfloat/$$c.out; then diff build/aotfloat/int.out build/aotfloat/$$c.out; echo "  ROJO: el AOT ($$c) no da lo que el interprete"; exit 1; fi; \
	   n=$$(grep -c '^ok ' build/aotfloat/$$c.out); \
	   if [ "$$n" -ne $(AOTFLOAT_OK) ]; then cat build/aotfloat/$$c.out; echo "  ROJO: $$c: $$n de $(AOTFLOAT_OK) comprobaciones"; exit 1; fi; \
	 done; \
	 echo "[status=OK] $(AOTFLOAT_OK)/$(AOTFLOAT_OK) con la FPU y con los helpers, igual que interpretado"

# V7 (#557, 25-sep) — el campo de texto del wire que se RECORTA (wire_v1_field_string_recortado):
# el EXITED tiene que salir SIEMPRE, y recortado tiene que seguir siendo JSON sano (ni medio
# caracter UTF-8 ni medio escape). Solo el constructor comun: el cable es de cada familia.
test-wirerecorte:
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_wirerecorte.c src/wire_v1_proto.c -o build/test_wirerecorte.exe $(LDFLAGS)
	@echo "--- #557 el errorMessage del EXITED, recortado si no cabe (esperado: solo 'ok', [status=OK]) ---"
	@./build/test_wirerecorte.exe

test-loaderr: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_loaderr.c $(LIBOBJ) -o build/test_loaderr.exe $(LDFLAGS)
	@echo "--- #421 el fallo de carga se explica (esperado: solo 'ok', [status=OK]) ---"
	@cd build && ./test_loaderr.exe

# V5/H7 — cuando NO hace falta barrer la zona buscando el ancla (338 ms medidos
# en la P4). Protege sobre todo el caso POSITIVO: un falso «no hay pack» dejaria
# uno grabado sin cargar, en silencio.
# (V7/H2, 15-sep) Los tres targets de packs (packskip, fsfb, pack) enlazan con
# LIBOBJ: la lista suelta de .c se quedo corta el 31-ago (#461: fs_facade.c
# llama a bpvm_fs_log_fail, que vive en fs_lfs.c) y llevaban desde entonces sin
# poder ejecutarse — el que «consagraba la sombra» de #493 entre ellos.
test-packskip: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_packskip.c $(LIBOBJ) -o build/test_packskip.exe $(LDFLAGS)
	@echo "--- V5/H7 saltarse el barrido sin packs (esperado: solo 'ok') ---"
	@./build/test_packskip.exe samples/PackFixA.pack

# La regla de la fachada: si `stat` dice que un fichero existe, se tiene que
# poder LEER — entero y POR TROZOS. Se rompio (read_at no miraba la zona de
# packs) y costo una tarde en la P4 con el SQLite.pack grabado.
test-fsfb: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_fsfallback.c $(LIBOBJ) -o build/test_fsfb.exe $(LDFLAGS)
	@echo "--- V7/H2 (#493) la zona solo en su espacio; stat/read/read_at/read_stream/crc32 miran lo mismo (esperado: solo 'ok') ---"
	@./build/test_fsfb.exe samples/PackFixA.pack

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

# V6/U6 — el planificador de memoria contra los numeros MEDIDOS de cada placa
# (C3, S3, Metro con PSRAM, Pico sin ella). Host-only: solo <stddef.h>/<stdio.h>.
test-mem:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_mem.c src/bpvm_mem.c -o build/test_mem.exe
	@echo "--- V6/U6 planificador de memoria vs placas MEDIDAS (esperado: solo 'ok', [status=OK]) ---"
	./build/test_mem.exe

# V6/#466 — la regla del /lib de la imagen (falta/anterior/CRC → repone; más nuevo
# o /app → deja), contra un littlefs REAL en fichero. Enlaza la lib entera.
# V7/H6 (#468) — y la ZONA: un pack con los mismos bytes que la imagen hace sobrar la copia
# de /lib; los packs, del escritor de verdad (PackCli), como en test-packres.
test-mods: $(LIBOBJ)
	@mkdir -p build/mods468/igual build/mods468/otro
	@printf 'MOD7--la Json de la imagen, 468---' > build/mods468/igual/Json.mod
	@printf 'MOD7--otra Json, de otro build-----' > build/mods468/otro/Json.mod
	@java -cp "$(PACKJAR)" basicplus.pack.PackCli build --out build/mods468/stdlib.pack --name Stdlib --version 1 --date 0 build/mods468/igual/Json.mod
	@java -cp "$(PACKJAR)" basicplus.pack.PackCli build --out build/mods468/otro.pack  --name Otro   --version 1 --date 0 build/mods468/otro/Json.mod
	$(CC) $(CFLAGS) -Isrc test/test_mods.c $(LIBOBJ) -o build/test_mods.exe $(LDFLAGS)
	@echo "--- V6/#466 y V7/#468 el instalador de la stdlib, con y sin zona (esperado: solo 'ok', [status=OK]) ---"
	./build/test_mods.exe build/mods468/igual/Json.mod build/mods468/stdlib.pack build/mods468/otro.pack

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
# (V7/H2, 15-sep) Se enlaza con LIBOBJ entero: la lista suelta de tres .c se
# quedo corta cuando mdn_loader.c paso a usar el log comun y bpvm_pack (V6), y el
# target llevaba desde entonces sin enlazar — un guardian mudo.
test-npack: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_npack.c $(LIBOBJ) -o build/test_npack.exe $(LDFLAGS)
	@echo "--- V5/I escalera del pack nativo (esperado: solo 'ok', [status=OK]) ---"
	./build/test_npack.exe

# V7/H2 (#441) — el gate de la RECETA del .mdn. El PC no tiene receta, asi que
# mdn_loader.c se compila APARTE con una forzada (la costura de
# bpvm_aot_recetas.h) y se fabrican blobs de la arch del host: v1 se carga a
# ciegas, v2 con mi huella se carga, v2 con otra se RECHAZA (MDN_ERR_RECETA).
test-mdnreceta: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc -DBPVM_AOT_HOST_RECETA_FORZADA=BPVM_AOT_RECETA_ARMV8M_MAIN_SOFTFP \
	  test/test_mdnreceta.c src/mdn_loader.c $(filter-out build/src/mdn_loader.o,$(LIBOBJ)) \
	  -o build/test_mdnreceta.exe $(LDFLAGS)
	@echo "--- V7/H2 (#441) el gate de la receta del .mdn (esperado: solo 'ok', [status=OK]) ---"
	./build/test_mdnreceta.exe

# V5/H1 — decodificadores del CSD/CID de la tarjeta SD. Host-only y a proposito:
# el dialogo con la tarjeta necesita tarjeta, pero sacar la CAPACIDAD del CSD es
# funcion pura, son DOS formulas distintas (v1 y v2) con campos partidos entre
# bytes, y equivocarse ahi no revienta: devuelve un numero. Una tarjeta de 32 GB
# que dice tener 4 no se nota hasta que el FS escribe fuera.
# V7/H6.c G4.a1: la HAL suelta necesita ya el bus, la tabla de capacidades y dos
# dobles de la VM (test/hal_stubs.c). Con la lista de antes no enlazaba desde #478
# (bpvm_out) y H4 (la tabla): fuera de las redes, mudo. No se enlaza la VM entera
# a propósito: el test tapa el reloj A CERO (ver test/test_sd.c).
test-sd:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_sd.c src/bpvm_sd.c src/spi.c src/gpio.c src/bpvm_bus.c src/bpvm_machine.c test/hal_stubs.c -o build/test_sd.exe
	@echo "--- V5/H1 CSD/CID de la SD (esperado: solo 'ok', [status=OK]) ---"
	./build/test_sd.exe

# V5/H6 - el contrato de dispositivo de bloque. Lo que de verdad se prueba aqui
# es DONDE EMPIEZA EL SISTEMA DE FICHEROS: esa decodificacion estaba enterrada
# dentro del montaje, donde no se podia ejercer sin placa Y sin tarjeta, y es
# justo la clase de codigo que al equivocarse no revienta - devuelve un numero.
# Un numero equivocado ahi manda a reformatear una tarjeta que estaba bien.
# Los dos desplazamientos del test son los MEDIDOS en el banco (2048 y 32768).
test-sdsc:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_sdsc.c src/bpvm_sd.c -o build/test_sdsc.exe
	@echo "--- H2-P5: el camino SDSC forzado sin tarjeta (esperado: solo 'ok') ---"
	./build/test_sdsc.exe

test-blk:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_blk.c src/bpvm_blk.c src/bpvm_sd_blk.c src/bpvm_sd.c src/spi.c src/gpio.c src/bpvm_bus.c src/bpvm_machine.c test/hal_stubs.c -o build/test_blk.exe
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
test-listdir: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_listdir.c src/bpvm_listdir.c $(LIBOBJ) -o build/test_listdir.exe $(LDFLAGS)
	@echo "--- V5/H6 LIST_DIR comun (esperado: solo 'ok', [status=OK]) ---"
	@cd build && ./test_listdir.exe

# H9 — board manager: núcleo portable del protocolo de gestión de placa (ENV_*/
# PART_*). Compone env+part. Host-only: opera sobre sectores A/B en RAM; simula el
# flujo virgen→defaults→apply→edit y el volcado del slot rancio. La cintura de flash
# (volcar wrote_slot) y el adaptador JSON del firmware llegan en la fase de placa.
test-bmgr:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_bmgr.c src/bpvm_bmgr.c src/bpvm_bmgr_wire.c src/bpvm_part.c src/bpvm_env.c src/bpvm_pack.c src/bpvm_npack.c src/bpvm_boot.c src/bpvm_util.c src/crc32.c -o build/test_bmgr.exe
	@echo "--- H9 board manager (esperado: solo 'ok', [status=OK]) ---"
	./build/test_bmgr.exe

# H3 — zona de packs (bpvm_pack): LIST/ADD/REMOVE sobre una región RAM que hace
# de flash. PARIDAD: las fixtures samples/PackFix*.pack las genera Pack.jar
# (lado PC) — el test recalcula los CRCs en C sobre esos bytes; un byte distinto
# entre C y Java = rojo (el micro rechazaría los packs del PC).
test-pack: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_pack.c $(LIBOBJ) -o build/test_pack.exe $(LDFLAGS)
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
# Las fuentes del pack de SQLite: el pegamento en C y los scripts de build.
# Estuvieron en ../notas/v5-sqlite-prueba —fuera del arbol y fuera de git— y se
# perdieron al limpiar el 19-ago. Ahora viven donde les toca y versionadas.
SQLSRC = ../bpstdlib/sqlite/nativo/src
SQLZIP   = /c/lenguajes/sqlite-amalgamation-3530400.zip
SQLDEF   = -DSQLITE_OS_OTHER=1 -DSQLITE_THREADSAFE=0 -DSQLITE_ENABLE_MEMSYS5            -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_TEMP_STORE=3

test-sqldemo: $(LIBOBJ)
	@mkdir -p build/sql
	@[ -f build/sql/sqlite3.c ] || unzip -o -j $(SQLZIP) "*/sqlite3.c" "*/sqlite3.h" 	    -d build/sql >/dev/null
	@if [ ! -f build/sql/aot_SQLite.c ]; then 	  echo "(falta build/sql/aot_SQLite.c — generalo con:"; 	  echo "   java -cp '../lexer-java/target/classes;../pack/target/classes' \\"; 	  echo "        basicplus.frontend.AotMain ../bpstdlib/sqlite/SQLite.bp build/sql)"; 	  exit 1; 	fi
	@[ -f build/sql/sqlite3.o ] || $(CC) -O1 -w -c build/sql/sqlite3.c 	    -o build/sql/sqlite3.o -Ibuild/sql $(SQLDEF)
	$(CC) $(CFLAGS) -Isrc -Ibuild/sql -I$(SQLSRC) 	    test/test_sqldemo.c build/sql/aot_SQLite.c 	    $(SQLSRC)/sqlite_shim.c $(SQLSRC)/vfs_bp.c $(SQLSRC)/packglue.c 	    build/sql/sqlite3.o $(LIBOBJ) -o build/sqldemo.exe $(LDFLAGS) $(SQLDEF)
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

# V7/H2 — LA CABECERA DEL .mod Y SUS SECCIONES contra el cargador (test/test_modhdr.c):
# para .mod con TODAS las secciones (native ARM y RISC-V con relleno de 0, 1 y 3 B,
# library no vacia), la suma de secciones es el fichero, el cargador deja en RAM
# los bytes exactos de code/data (CRC) y la seccion native se recorre blob a blob.
# Es la segunda opinion del formato ANTES de subirlo a MOD8. Fixtures congelados en
# test/fixtures/modhdr/ (NatV7/NatNew: samples con native, blob .mdn v1; Bench_riscv:
# nativo RISC-V del P4; acme.LibHdr: library, fuente al lado; Ocho: MOD8 con deps;
# NatNew8: MOD8 con blob .mdn v2 — la huella de la receta, V7/H2 #441); la stdlib
# fresca al lado.
test-modhdr: $(LIBOBJ)
	@mkdir -p build/modhdr
	$(CC) $(CFLAGS) -Isrc test/test_modhdr.c $(LIBOBJ) -o build/test_modhdr.exe $(LDFLAGS)
	@cp ../bpstdlib/*.mod build/modhdr/ && cp test/fixtures/modhdr/*.mod build/modhdr/
	@echo "--- V7/H2 cabecera y secciones del .mod vs cargador (esperado: solo 'ok', [status=OK]) ---"
	@cd build/modhdr && ../test_modhdr.exe NatV7.mod NatNew.mod Bench_riscv.mod acme.LibHdr.mod Core.mod Ocho.mod NatNew8.mod

test-crc:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_crc.c src/crc32.c src/fs_facade.c src/fs_host.c -o build/test_crc.exe
	@echo "--- #305 CRC por trozos (esperado: solo 'ok', [status=OK]) ---"
	@cd build && ./test_crc.exe

# H10 — el MICRO SIMULADO del IDE: servidor TCP wire v1 COMPLETO (META + FILES +
# TERMINAL + gestión de placa + packs) con FS littlefs sobre imagen y la VM-C de
# verdad ejecutando. El IDE lo trata como una placa más. Nació en H9 como
# boardsim (sólo STATE/ENV_*/PART_*); H10 le enchufa el FS y la VM → enlaza la
# lib entera ($(LIBOBJ)). El parser JSON vive en src/ desde V6/U1.1.
# Winsock via $(LDFLAGS) (-lws2_32). Con LVGL=1 arrastra la ventana SDL.
# V6 — `make sim SQLITE=1`: el simulador CON base de datos.
#
# Criterio de Eduardo (6-sep): «¿es útil para el programador? Si la respuesta es
# sí, se hace». Y lo es: poder probar en el PC un programa que consulta una BD,
# sin grabar una placa.
#
# POR QUE ENLAZADO Y NO POR PACK: el pack nativo de una placa se reubica con
# NpackReloc, que es ELF32 de arriba abajo (`Elf32.java:169`: «solo ELF32
# soportado»), y x86-64 es ELF64. Traer eso al PC es un lector nuevo y el juego
# de reubicaciones de x86-64 — un proyecto, no una tarde. Enlazarlo da HOY lo que
# el programador pide, y no cambia nada de lo que él ve: `SQLite.mod` y `Orm.mod`
# viajan IGUAL que a una placa (son bytecode: 2 bytes de sección nativa), y sus
# 16 `native` se casan POR NOMBRE con `bpvm_aot_register_by_name`, venga el thunk
# de un `.npk` o de aquí.
#
# ⚠️ Lo que hay que saber: así el simulador tiene SQLite SIEMPRE y una placa sólo
# si le grabas el pack. Un programa que va aquí y no allí falla RUIDOSAMENTE
# («falta el codigo nativo del pack 'SQLI'»), no en silencio.
SIM_SQL_SRC = build/sql/aot_SQLite.c $(SQLSRC)/sqlite_shim.c $(SQLSRC)/vfs_bp.c \
              $(SQLSRC)/packglue.c build/sql/sqlite3.o
SIM_SQL_FLAGS = -DBPVM_SQLITE -Ibuild/sql -I$(SQLSRC) $(SQLDEF)

# SIM_OUT: donde se deja el binario. `compat.sh check-sim` construye el SUYO
# (build/bpvm-sim-red.exe) y no pisa el de siempre: el IDE puede tenerlo abierto
# (en Windows, un .exe en marcha no se sobrescribe) y puede ser LVGL=1/SQLITE=1.
SIM_OUT ?= build/bpvm-sim.exe

# V7/H3 (24-sep) — el micro simulado con SD (`--sd=IMAGEN`: un fichero hace de tarjeta): el registro
# comun de la SD, el contrato de bloque y FAT, los mismos que el firmware de una placa con ranura.
SIM_SD_SRC = src/bpvm_sd_placa.c src/bpvm_blk.c src/fs_fat.c third_party/fatfs/ff.c third_party/fatfs/ffunicode.c

sim: $(LIBOBJ) $(if $(filter 1,$(LVGL)),build/liblvgl.a build/SDL2.dll,) $(if $(filter 1,$(SQLITE)),build/sql/sqlite3.o build/sql/aot_SQLite.c,)
	@mkdir -p build
	$(CC) $(CFLAGS) $(if $(filter 1,$(SQLITE)),$(SIM_SQL_FLAGS),) -Ipico -Isrc -Ithird_party/fatfs tools/bpvm_sim.c src/json_min.c src/wire_v1_proto.c src/bpvm_repl.c src/bpvm_listdir.c $(SIM_SD_SRC) $(if $(filter 1,$(SQLITE)),$(SIM_SQL_SRC),) $(LIBOBJ) -o $(SIM_OUT) $(LDFLAGS)
	@echo "sim listo: $(SIM_OUT) [--port=N] [--mem=N] [--psram=N] [--flash=N] [--fs=IMG] [--sd=IMG]"

# Los ingredientes de SQLITE=1, si no estan ya (los comparte con test-sqldemo).
build/sql/sqlite3.o:
	@mkdir -p build/sql
	@[ -f build/sql/sqlite3.c ] || unzip -o -j $(SQLZIP) "*/sqlite3.c" "*/sqlite3.h" -d build/sql >/dev/null
	$(CC) -O1 -w -c build/sql/sqlite3.c -o build/sql/sqlite3.o -Ibuild/sql $(SQLDEF)

build/sql/aot_SQLite.c:
	@echo "falta build/sql/aot_SQLite.c — generalo con:"
	@echo "   java -cp '../lexer-java/target/classes;../pack/target/classes' basicplus.frontend.AotMain ../bpstdlib/sqlite/SQLite.bp build/sql"
	@exit 1

# Alias histórico (H9): el boardsim ES el sim, con otro nombre.
boardsim: sim

# Smoke end-to-end del sim por el wire (arranca el sim, recorre el flujo de placa
# virgen, verifica cada reply). Necesita python3. No necesita placa ni IDE.
boardsim-smoke: sim
	@echo "--- boardsim smoke (protocolo de gestión de placa) ---"
	python tools/boardsim_smoke.py

# H10 — smoke del wire de FICHEROS + EJECUCIÓN contra el sim (PUT/LIST/GET/RUN…).
# El sim arranca como lo arranca el IDE: con ../packs/Stdlib.pack grabado (desde
# H1 P3 todo módulo importa Core). Estos cuatro smokes están en la red de cada
# día: `compat/compat.sh check` (frente check-sim), con la guarda del pack.
sim-smoke: sim
	@echo "--- H10 sim smoke (FILES + TERMINAL) ---"
	python tools/sim_smoke.py

# V6/A1.1 - lo que el hilo `io` viene a arreglar, contra el simulador: el KILL
# llega mientras el programa calcula Y mientras imprime a chorro, y un `print`
# de varios argumentos sale en UN mensaje OUTPUT (antes, uno por argumento).
io-smoke: sim
	@echo "--- V6/A1.1 io smoke (dos hilos: KILL en marcha + OUTPUT por linea) ---"
	python tools/io_smoke.py

# #425 — el listado DECLARA lo que deja fuera. Contra el micro simulado, que
# tiene el mismo agujero y los mismos topes en pequeño. Trae su CONTROL (por
# debajo del tope omitted DEBE ser 0), que es lo que lo hace valer.
test-listtrunc: sim
	@echo "--- #425: el LIST_REPLY declara lo omitido (esperado: solo 'ok', [status=OK]) ---"
	python tools/list_trunc_smoke.py

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
FSCURSOR_BIN = build/test_fs_cursor.exe
test-fscursor: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_fs_cursor.c $(LIBOBJ) -o $(FSCURSOR_BIN) $(LDFLAGS)
	@echo "--- V7/H3 (#491) el cursor de la fachada: mismo guion en host y littlefs (esperado: 0 fallos) ---"
	./$(FSCURSOR_BIN)

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
# list/clear. DBGBP = el programa: cualquier .bp que sólo dependa de la stdlib
# (default arith.bp, el módulo Arith).
# (#541, 24-sep) Las tres pruebas del depurador COMPILAN su .mod en cada
# ejecución, con el frontend de hoy, a build/dbgmod/<prueba>/ y con la stdlib al
# lado (el import Core implícito de H1 P3 la pide). Antes cargaban
# samples/Arith.mod, fuera de git desde el 6-ago: en el checkout principal el
# MOD5 del 22-may —la VM lo rechaza, con razón— y en uno limpio ninguno, con las
# tres SALTADAS con salida 0. En build/ y no en samples/, donde se pisaría el
# Arith.dbg versionado. DBGMOD (un .mod ya hecho) no se usa: si llega, es error.
DEBUG_BIN = build/test_debug.exe
DBGBP    ?= samples/arith.bp
DBGDIR    = build/dbgmod/$@
ifneq ($(DBGMOD),)
$(error DBGMOD ya no existe (#541): las pruebas del depurador compilan su .mod en cada ejecucion; pasa el fuente con DBGBP=ruta/X.bp)
endif
define dbgmod
@rm -rf $(DBGDIR) && mkdir -p $(DBGDIR) && cp ../bpstdlib/*.mod $(DBGDIR)/
@java -jar ../lexer-java/target/basicplus-frontend.jar $(DBGBP) --compile $(DBGDIR) --backend=mivm --stdlibDir=$(DBGDIR) >$(DBGDIR)/compila.log 2>&1 || { echo "  ✗ $(DBGBP) no compila:"; grep -iE "error|abortada" $(DBGDIR)/compila.log | head -5; exit 1; }
@m=$$(sed -nE 's/^[[:space:]]*module[[:space:]]+([A-Za-z_][A-Za-z0-9_]*).*/\1/p' $(DBGBP) | head -1); \
 if [ -z "$$m" ] || [ ! -f $(DBGDIR)/$$m.mod ]; then echo "  ✗ $(DBGBP) no ha dejado su .mod en $(DBGDIR) (módulo '$$m')"; exit 1; fi; \
 echo $$m.mod > $(DBGDIR)/.mod
endef
test-debug: $(LIBOBJ)
	@mkdir -p build
	$(call dbgmod)
	@$(CC) $(CFLAGS) -Isrc test/test_debug.c $(LIBOBJ) -o $(DEBUG_BIN) $(LDFLAGS)
	@echo "--- núcleo debugger VM-C (esperado: OK al final) ---"
	@cd $(DBGDIR) && $(abspath $(DEBUG_BIN)) $$(cat .mod)

# H6.b.2.a — pausa/resume del debugger cruzando threads (corazón del server
# host, sin sockets). El intérprete corre en un thread y bloquea en el pause_cb;
# el thread controlador inyecta STEP/CONTINUE por mutex+cond. Ver test_debug_mt.c.
DEBUGMT_BIN = build/test_debug_mt.exe
test-debug-mt: $(LIBOBJ)
	@mkdir -p build
	$(call dbgmod)
	@$(CC) $(CFLAGS) -Isrc test/test_debug_mt.c $(LIBOBJ) -o $(DEBUGMT_BIN) $(LDFLAGS)
	@echo "--- pausa/resume cruzando threads (esperado: OK al final) ---"
	@cd $(DBGDIR) && $(abspath $(DEBUGMT_BIN)) $$(cat .mod)

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
	$(call dbgmod)
	@$(CC) $(CFLAGS) -Isrc test/debug_listen.c $(LIBOBJ) -o $(LISTEN_BIN) $(DBG_LDFLAGS)
	@echo "--- wire debugger host (cliente Python) ---"
	@$(PYTHON) test/dbg_client.py "$(abspath $(LISTEN_BIN))" "$$(cat $(DBGDIR)/.mod)" $(DBGDIR)

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

# ── #446 — LO QUE EL ARNÉS NO PODÍA VER ────────────────────────────────────────
# El host lleva `GUI ?= 1`, así que SIEMPRE compila con BPVM_GUI. Las 535 líneas
# del core bajo ese flag están siempre presentes en el PC, y por eso el arnés no
# puede detectar que a una placa pequeña le falte algo de ahí: fue #445, tres
# builtins de `App` (211/212/213) que vivían dentro del `#ifdef` sin tener nada
# que ver con la pantalla. Rotos en TODA placa sin GUI, y el fuente los enseñaba
# implementados.
#
# El control: preprocesar de verdad —con y sin el flag— y comprobar que todo lo
# que sólo existe con GUI se llama `GUI_*`. Cualquier otra cosa es un colado.
#
# 📌 Se preprocesa, NO se lee el fuente con grep/awk: al escribir esto un awk que
# contaba `#ifdef`/`#endif` a mano dijo que los tres `APP_*` seguían dentro (se
# le coló un `#ifdef` de la línea 33) — un instrumento mintiendo sobre el propio
# bug que persigue. El preprocesador no opina.
check-nogui:
	@echo "== #446: builtins que solo existen con BPVM_GUI =="
	@ln() { $(CC) -E -Iinclude -Isrc -Ithird_party/littlefs $$1 src/builtins.c 2>/dev/null | \
	          awk '/^# [0-9]+ ".*builtins\.c"/ { split($$0,a," "); l=a[2]; next } \
	               { if (/case BUILTIN_/) print l; l++ }' | sort -u; }; \
	 con=$$(ln "-DBPVM_GUI" | wc -l); sin=$$(ln "" | wc -l); \
	 echo "   con GUI: $$con | sin GUI: $$sin | solo-GUI: $$((con-sin))"; \
	 colados=$$(comm -13 <(ln "") <(ln "-DBPVM_GUI") | while read l; do sed -n "$${l}p" src/builtins.c; done \
	            | grep -oE "BUILTIN_[A-Z_0-9]+" | sed 's/BUILTIN_//' | sort -u | grep -v "^GUI_" || true); \
	 if [ -n "$$colados" ]; then \
	   echo "   FAIL: estos NO son de la GUI y solo existen con ella:"; \
	   echo "$$colados" | sed 's/^/     /'; \
	   echo "   (rotos en toda placa sin pantalla, y el fuente los ensena implementados)"; \
	   exit 1; \
	 fi; \
	 echo "   PASS check-nogui: todo lo solo-GUI se llama GUI_*"
