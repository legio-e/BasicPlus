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
# #627 (2-oct) — setjmp/longjmp SIN el desenrollado SEH de Windows. En x86_64, el `setjmp` de MinGW guarda el marco
# del que llama (`__builtin_frame_address(0)`), y con ese marco el `longjmp` de la CRT deshace la pila con el
# mecanismo de excepciones del sistema. Cuando el marco del `setjmp` caía a 432 B o menos de la base de la pila del
# hilo —la aleatorización de Windows lo deja ahí de vez en cuando—, el `longjmp` reventaba: `test-packhelp`, ~2 % de
# las veces (medido: 22 de 22 vueltas en esa zona, y ninguna fuera). Con esto `setjmp` guarda un marco nulo y
# `longjmp` sólo restaura los registros, como en Linux y en las placas, que es lo que la VM necesita (C, sin nada que
# deshacer). Lo usan el límite de las native (interp.c) y el fondo del hilo de una native (bpvm_nativa.c), cuyo marco
# está, como el de `main`, arriba del todo de su pila. Fuera de MinGW no hace nada.
CFLAGS  += -D__USE_MINGW_SETJMP_NON_SEH
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
SRC     = src/bpvm.c src/bpvm_util.c src/loader.c src/interp.c src/heap.c src/builtins.c src/bpstr_vm.c \
          src/link.c src/scheduler.c src/threading.c src/exceptions.c src/events.c src/destruir.c \
          src/platform_pthread.c src/gpio.c src/i2c.c src/spi.c src/uart.c \
          src/pulse.c src/pwm.c src/bpvm_machine.c src/bpvm_neopixel.c src/rtc.c src/adc.c src/wdt.c \
          src/aot_registry.c src/bpvm_aot_helpers.c src/mdn_loader.c \
          src/bpvm_log.c \
          src/scheduler_smp.c src/comm_common.c src/comm_host.c src/bpvm_io.c \
          src/fs_facade.c src/fs_host.c src/crc32.c src/bpvm_env.c src/bpvm_part.c src/bpvm_boot.c src/bpvm_sqlmem.c src/bpvm_mem.c src/bpvm_mods.c src/bpvm_handles.c src/bpvm_bios.c src/bpvm_bios_fs.c src/bpvm_bios_hal.c src/bpvm_bus.c src/bpvm_npack.c src/bpvm_npack_carga.c src/bpvm_bmgr.c src/bpvm_bmgr_wire.c src/bpvm_dbg_wire.c \
          src/bpvm_pack.c src/bpvm_mdn_scan.c src/bpvm_nativa.c \
          src/net.c src/net_host.c \
          src/bpvm_sonido.c src/sonido_host.c \
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
  LDFLAGS += -lwinmm     # V8/H4 (#708) — el audio de Windows (sonido_host.c)
  # V8 #763 (2-nov) — el MANIFIESTO de los .exe que corren programas, el CLI y el simulador (`sim`): su página de
  # códigos, UTF-8 (win/utf8.manifest). Sin él, el argumento, los nombres de `listDir` y los de lo que se crea iban en
  # la del sistema, la 1252. Los tests no lo llevan: sus nombres son ASCII.
  WIN_RES = build/win/utf8.res.o
  OBJ    += $(WIN_RES)
endif

.PHONY: all clean smoke medir-llamada test-ejecutor test-aotdirecta test-mdnv3 test-aotswitch test-aottuplas test-aotstr test-aotarr test-aotanida test-aotpuente test-aottryvm test-aottry test-aotnombres test-aotprivado test-aotestrecha test-aotglobales test-aotcede test-aotexpropia test-aotkill test-aotbuiltin test-aotcompara test-aotdo test-aotcompuestos test-aotfloat test-wirerecorte test-salida test-sd-turnos test-sd-turnos-control test-mdnreceta test-callbp test-xmodule test-throwmsg test-throwuser test-method test-debug test-debug-mt test-listen test-bytenat test-xmodnat test-compressnat test-compressbench test-kill test-dbgkill test-tablasim test-flash test-heapdescarrilado test-compactsmp test-smpfallo test-oqentero test-net test-smphandles test-oom test-excmsggc test-lfs test-lfsfile test-lfsfault test-fslfs test-fscursor test-fsvfs test-fslfsmt test-iomt test-fspos test-blk test-sdio test-listdir test-env test-part test-boot test-boot2 test-bmgr test-pack test-crc test-fsfallo test-gchole test-gchondo test-ownerhondo test-coresiempre test-anidadalarga test-trazasbus test-runtrasrun test-importhondo test-hilos test-cola test-logmt test-reserva test-palabras test-frcond test-rand test-freelist test-runguard sim sim-smoke test-listtrunc boardsim boardsim-smoke gui-smoke gui-imgdecode check-builtins

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
# V7 #668 (8-oct) — y si el COMPILADOR es más nuevo que el .mod, también: el guardián miraba el .bp y el formato,
# no quién lo compiló. samples/OwnerHondo.mod era del 7-oct, de antes de #668 (soltaba con FREE_REF), y
# test-ownerhondo seguía en verde sin pasar por la destrucción nueva —que en la Pico 2 se quedaba sin memoria con
# ese mismo programa—. Un artefacto cachea por fuente; el que lo hace, también cuenta.
define fixture
@for s in ../bpstdlib/*.mod; do b=$$(basename $$s); if [ -f samples/$$b ] && ! cmp -s $$s samples/$$b; then echo "  (samples/$$b era otra version de la stdlib: se copia la de bpstdlib)"; cp $$s samples/$$b; fi; done
@fmt=$(if $(2),$(2),9); want=$$(sed -n "s/.*BPVM_MAGIC_V$$fmt[[:space:]]*0x\([0-9A-Fa-f]\{8\}\).*/\1/p" include/bpvm_internal.h | head -1 | tr a-z A-Z); \
for m in $(1); do \
  why=""; \
  if [ ! -f samples/$$m.mod ]; then why="no existe"; \
  elif [ samples/$$m.bp -nt samples/$$m.mod ]; then why="el .bp es mas nuevo"; \
  elif [ ../lexer-java/target/basicplus-frontend.jar -nt samples/$$m.mod ]; then why="el compilador es mas nuevo"; \
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
@java -cp "../lexer-java/target/classes;../pack/target/classes;../miVM/target/classes" basicplus.frontend.AotMain samples/$(1).bp $(2) >$(2)/aot.log 2>&1 || { cat $(2)/aot.log; exit 1; }
@# #588 (27-sep): con las clases de miVM: el AOT pregunta al compilador dónde queda cada global (ModWriter).
@# #542 (26-sep): un import sin interfaz ya no es un aviso que haya que buscar en el log: AotMain sale
@# con 4 y «falta el modulo 'X'», y lo para el `||` de arriba.
endef

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

ifeq ($(OS),Windows_NT)
$(WIN_RES): win/utf8.rc win/utf8.manifest
	@mkdir -p $(dir $@)
	windres -I win win/utf8.rc -O coff -o $@
endif

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
# Y la VERSIÓN de las banderas comunes: se sube cuando cambia una que vale para todos los sabores (#627: el setjmp
# sin SEH), y así los .o de antes se tiran solos. Va delante: la red busca `.flavor-*lvgl1`.
BUILD_FLAGS_VER := 2
BUILD_FLAVOR := v$(BUILD_FLAGS_VER)-gui$(GUI)-lvgl$(LVGL)
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

# V8 #730 — la biblioteca de cadenas (bpstr/, por src/bpstr_vm.c): -O2, como el resto en el host, y que GCC no cambie su
# bucle de rellenar por una llamada a memset. Sus dependencias, bpstr/bpstr.c y bpstr.h, las apunta -MMD.
build/src/bpstr_vm.o: CFLAGS += -O2 -fno-tree-loop-distribute-patterns

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
	@if [ ! -f samples/LongNat.mod ]; then 	  echo "(skip test-longnat: falta samples/LongNat.mod)"; 	else 	  java -cp "../lexer-java/target/classes;../pack/target/classes;../miVM/target/classes" 	    basicplus.frontend.AotMain samples/LongNat.bp build/aotlong >/dev/null; 	  cp build/aotlong/aot_LongNat.c test/aot_LongNat.c; 	  $(CC) $(CFLAGS) -Isrc test/test_longnat.c test/aot_LongNat.c 	    $(LIBOBJ) -o $(LONGNAT_BIN) $(LDFLAGS); 	  echo "--- #381 long en native (la salida debe ser la de la VM-Java) ---"; 	  ./$(LONGNAT_BIN) samples/LongNat.mod; 	fi

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
	$(call stdlibdep,Net Core Heap)
	@if [ ! -f samples/TcpEchoTest.mod ]; then \
	  echo "(skip test-net: falta samples/TcpEchoTest.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/TcpEchoTest.bp --compile samples --backend=mivm)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_net.c \
	    $(LIBOBJ) -o $(NET_BIN) $(LDFLAGS); \
	  echo "--- TCP echo H11 (esperado: conectado / 2 ecos / timeout-len: 0 / heap: recorrido entero / fin, PASS) ---"; \
	  ./$(NET_BIN) samples/TcpEchoTest.mod samples/Net.mod samples/Core.mod; \
	fi

KILL_BIN = build/test_kill.exe
test-kill: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,KillLoop KillSleep KillDestruye KillDestruyeLento)
	@if [ ! -f samples/KillLoop.mod ] || [ ! -f samples/KillSleep.mod ]; then \
	  echo "(skip test-kill: no hay samples/KillLoop.mod / KillSleep.mod y no se han"; \
	  echo " podido generar — ¿está construido ../lexer-java/target/basicplus-frontend.jar?)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_kill.c \
	    $(LIBOBJ) -o $(KILL_BIN) $(LDFLAGS); \
	  echo "--- KILL cooperativo (esperado: 12x PASS —de la 9 a la 12, el programa matado que se destruye: #668, #695— y la linea OK) ---"; \
	  (cd samples && ../$(KILL_BIN) KillLoop.mod KillSleep.mod KillDestruye.mod) || exit 1; \
	  echo "--- y el sabotaje del KILL que despierta (V7/H12 2a.2: sin el aviso a la cola, el programa dormido no se para; esperado: ROJO) ---"; \
	  if (cd samples && timeout 15 ../$(KILL_BIN) KillLoop.mod KillSleep.mod --sabotaje-despertar); then \
	    echo "FAIL test-kill: el sabotaje del aviso pasa por verde"; exit 1; \
	  else echo "  (rojo, como debe: el KILL sin aviso no despierta al programa dormido)"; fi; \
	fi

# V7 #667 (8-oct) — LO QUE UN OBJETO TOMA DEL HARDWARE LO SUELTA SU DESTRUCTOR, y lo que nadie suelta, la red del
# final del RUN: con backends FALSOS que cuentan (PWM, contador, tira, UART y pines). Dos programas: con objetos (los
# destructores) y sin ellos (la red).
SUELTA_BIN = build/test_suelta.exe
test-suelta: $(LIBOBJ)
	@mkdir -p build
	@for m in Gpio Machine Pwm Pulse Neopixel Uart; do cp ../bpstdlib/$$m.mod samples/; done   # las suyas, junto al programa
	$(call fixture,SueltaTodo SueltaSinObjetos)
	$(CC) $(CFLAGS) -Isrc test/test_suelta.c $(LIBOBJ) -o $(SUELTA_BIN) $(LDFLAGS)
	@echo "--- V7 #667: soltar los recursos (esperado: solo 'ok', [status=OK]) ---"
	(cd samples && ../$(SUELTA_BIN) SueltaTodo.mod SueltaSinObjetos.mod)

# V7 #668 con POCA memoria de plataforma (8-oct; la ronda de V7 en la Pico 2: OwnerHondo moría con «No space in heap»):
# soltar un árbol de owner grande no pide memoria por nodo. Con las reservas de destruir.c a 4 KB como mucho, OwnerHondo
# (sin destructores) y OwnerHondoDtor (con un vigía que sí tiene uno: hay que mirar los árboles), hasta el final.
# V7 #668 (8-oct) — LO QUE SÓLO SE VE AL ACABAR: DestruyeFinal y DestruyeTrasError destruyen DESPUÉS del main (bien, o
# tras un error), donde el sample ya no puede comprobarse solo, y la paridad no ve un fallo que las dos VMs tengan
# igual —el atajo de esta mañana se comía el destructor del ÚLTIMO pendiente en las dos, y DestruyeFinal seguía en
# PARIDAD OK sin «~global»—. Aquí, su salida contra la esperada, en las dos VMs.
test-destruyefinal: $(BIN)
	$(call fixture,DestruyeFinal DestruyeTrasError)
	$(call stdlibdep,Core)
	@ef=$$(printf 'fin del main (suelto-del-main)\n~propio\n~suelto-del-main\n~global'); \
	 ee=$$(printf 'antes del error\n~propio\n~global'); malo=0; \
	 for vm in c java; do \
	   if [ $$vm = c ]; then run="../$(BIN)"; else run="java -jar ../../miVM/target/bpgenvm-1.0.jar"; fi; \
	   of=$$(cd samples && timeout 60 $$run DestruyeFinal.mod 2>/dev/null | tr -d '\r' | grep -E '^(fin del main|~)'); \
	   oe=$$(cd samples && timeout 60 $$run DestruyeTrasError.mod 2>/dev/null | tr -d '\r' | grep -E '^(antes del error|~)'); \
	   if [ "$$of" = "$$ef" ]; then echo "  ok  $$vm: DestruyeFinal, los tres destructores, el más nuevo primero"; \
	   else echo "  FAIL $$vm: DestruyeFinal dio: $$(printf '%s' "$$of" | tr '\n' '|')"; malo=1; fi; \
	   if [ "$$oe" = "$$ee" ]; then echo "  ok  $$vm: DestruyeTrasError, tras el error, los dos destructores"; \
	   else echo "  FAIL $$vm: DestruyeTrasError dio: $$(printf '%s' "$$oe" | tr '\n' '|')"; malo=1; fi; \
	 done; \
	 if [ $$malo -eq 0 ]; then echo "[status=OK]"; else echo "[status=FAIL]"; exit 1; fi

DESPOCA_BIN = build/test_destruirpoca.exe
test-destruirpoca: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,OwnerHondo OwnerHondoDtor)
	$(call stdlibdep,Core)
	$(CC) $(CFLAGS) -Isrc test/test_destruirpoca.c $(LIBOBJ) -o $(DESPOCA_BIN) $(LDFLAGS)
	@echo "--- V7 #668 con poca memoria: soltar un árbol grande sin pedir memoria por nodo (esperado: solo 'ok', [status=OK]) ---"
	(cd samples && ../$(DESPOCA_BIN) OwnerHondo.mod OwnerHondoDtor.mod)

# V7 #651 (8-oct) — EL CENSO DE LA CARGA: lo que se lleva CARGAR un programa, por quién lo pide (MEMORIA.md, «por qué
# un módulo cuesta de 3 a 6 KB aunque venga de un pack»). Una medida, sin veredicto: Nada, Importa5, GuiWinJson y
# GuiArbolG2, cada uno en una VM nueva, con el censo del guardián antes de correrlo.
CENSO_BIN = build/censo_carga.exe
censo-carga: $(LIBOBJ)
	@mkdir -p build/censo
	@cp ../bpstdlib/*.mod build/censo/
	@for f in tools/consumo/Nada.bp tools/consumo/Importa5.bp ../samples/GuiWinJson.bp samples/GuiArbolG2.bp; do \
	   java -jar ../lexer-java/target/basicplus-frontend.jar $$f --compile build/censo --backend=mivm > /dev/null 2>&1 || echo "  (no compila: $$f)"; \
	 done
	$(CC) $(CFLAGS) -Isrc test/censo_carga.c $(LIBOBJ) -o $(CENSO_BIN) $(LDFLAGS)
	(cd build/censo && ../censo_carga.exe Nada.mod Importa5.mod GuiWinJson.mod GuiArbolG2.mod)

# V7 #651 (8-oct) — y como PRUEBA, en la red: cargados, Importa5, GuiWinJson y GuiArbolG2 no pasan de 100 bloques de la
# VM (con un bloque por nombre eran 436, 818 y 881; con uno por módulo, 67, 54 y 54). En la placa cada bloque lleva la
# cabecera del guardián de #339: que el cargador no vuelva a pedir uno por nombre sin que se note.
test-censo: $(LIBOBJ)
	@mkdir -p build/censo
	@cp ../bpstdlib/*.mod build/censo/
	@for f in tools/consumo/Importa5.bp ../samples/GuiWinJson.bp samples/GuiArbolG2.bp; do \
	   java -jar ../lexer-java/target/basicplus-frontend.jar $$f --compile build/censo --backend=mivm > /dev/null 2>&1 || echo "  FAIL no compila: $$f"; \
	 done
	@$(CC) $(CFLAGS) -Isrc test/censo_carga.c $(LIBOBJ) -o $(CENSO_BIN) $(LDFLAGS)
	@(cd build/censo && ../censo_carga.exe --tope=100 Importa5.mod GuiWinJson.mod GuiArbolG2.mod > ../censo_tope.txt); rc=$$?; \
	 grep -E '^(ok|FAIL|\[status)' build/censo_tope.txt; exit $$rc

# V7/H10 #634 (3-oct) — UN KILL CON EL PROGRAMA EN PAUSA: el lazo de pausa del depurador no miraba la bandera que pone
# `io` al recibir el KILL, y el Stop del IDE no paraba un programa detenido en un breakpoint (en placa). KillLoop parado
# en su primer opcode y un `next_cmd` de mentira; sin el fixture, ROJO (no un «saltado» que pasa por verde).
test-dbgkill: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,KillLoop)
	@if [ ! -f samples/KillLoop.mod ]; then echo "  FAIL: no hay samples/KillLoop.mod [status=FAIL]"; exit 1; fi
	$(CC) $(CFLAGS) -Isrc test/test_dbgkill.c $(LIBOBJ) -o build/test_dbgkill.exe $(LDFLAGS)
	@echo "--- #634: un KILL con el programa en pausa (esperado: solo 'ok', [status=OK]) ---"
	cd samples && ../build/test_dbgkill.exe KillLoop.mod

# #639 (3-oct) — la TABLA DE SIMBOLOS con poca memoria: si el doble no cabe, crece menos (x1,5, x1,25, lo justo) y, si
# ni lo justo cabe, la carga lo DICE (antes el fallo se tiraba y el enlace acusaba a una version vieja: la C6 con la
# GUI). Json.mod con un malloc que a link.c no le da mas de N bytes de un bloque.
# V7/H10, el board_mgr común (#637, 3-oct): la regla de la zona de la flash que se puede escribir
# (include/bpvm_flash_zona.h), con los números de la Discovery, la Nucleo y la Pico. En la placa no se puede probar:
# aquí, la regla que usan stm32_flash.c y (a su modo) pico_flash.c.
test-flash:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_flash.c -o build/test_flash.exe
	@echo "--- #637 la zona de la flash que se puede escribir (esperado: solo 'ok', [status=OK]) ---"
	./build/test_flash.exe

test-tablasim: $(LIBOBJ)
	@mkdir -p build
	$(call stdlibdep,Json Collections Str Core)
	$(CC) $(CFLAGS) -Isrc test/test_tablasim.c $(LIBOBJ) -o build/test_tablasim.exe $(LDFLAGS)
	@echo "--- #639: la tabla de simbolos con poca memoria (esperado: solo 'ok', [status=OK]) ---"
	cd samples && ../build/test_tablasim.exe Json.mod

# V7 #494 (27-sep) — el HEAP DESCARRILADO es UNA excepcion (decision de Eduardo): un gancho de depuracion
# estropea la cabecera del primer bloque y el GC siguiente no barre; la reserva que lo pidio lanza la
# prefabricada. Con su control (sin estropear). Ver test/test_heapdescarrilado.c. Y V7 #659 (7-oct): la marca de esa
# reserva es de la TAREA que reservó —con dos runs, el `throw` de otra se la llevaba—: la quinta pasada, dos tareas.
HEAPD_BIN = build/test_heapdescarrilado.exe
test-heapdescarrilado: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,HeapRoto HeapCrece)
	@if [ ! -f samples/HeapRoto.mod ] || [ ! -f samples/HeapCrece.mod ]; then \
	  echo "(skip test-heapdescarrilado: no hay samples/HeapRoto.mod o HeapCrece.mod y no se han podido generar)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_heapdescarrilado.c \
	    $(LIBOBJ) -o $(HEAPD_BIN) $(LDFLAGS); \
	  echo "--- el heap descarrilado (esperado: control PASS, estropeado PASS, sin mapa PASS y la linea OK) ---"; \
	  (cd samples && ../$(HEAPD_BIN) HeapRoto.mod HeapCrece.mod); \
	fi

# V7/H7 #595 (28-sep) — Heap.compact() con el scheduler SMP: el caso de la revision (un hilo que muere
# debiendo un evento; join; compactar), 30 veces con dos workers y una sin SMP, y siempre lo que dice
# miVM. Ver test/test_compactsmp.c.
COMPSMP_BIN = build/test_compactsmp.exe
test-compactsmp: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,HeapCompactEv)
	@if [ ! -f samples/HeapCompactEv.mod ]; then \
	  echo "(skip test-compactsmp: no hay samples/HeapCompactEv.mod y no se ha podido generar)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_compactsmp.c \
	    $(LIBOBJ) -o $(COMPSMP_BIN) $(LDFLAGS); \
	  echo "--- Heap.compact() y el scheduler SMP (esperado: sin SMP PASS, 30/30 con 2 workers y la linea OK) ---"; \
	  (cd samples && ../$(COMPSMP_BIN) HeapCompactEv.mod); \
	fi

# V7/H7 #596 (28-sep) — una excepcion sin atrapar acaba el RUN en error tambien con el scheduler SMP (antes
# «status=OK», salida 0): SmpFallo sin excepcion, lanzando en Main y en un hilo, sin SMP y con dos
# workers. Ver test/test_smpfallo.c.
SMPFALLO_BIN = build/test_smpfallo.exe
test-smpfallo: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,SmpFallo)
	@if [ ! -f samples/SmpFallo.mod ]; then \
	  echo "(skip test-smpfallo: no hay samples/SmpFallo.mod y no se ha podido generar)"; \
	else \
	  $(CC) $(CFLAGS) -Isrc test/test_smpfallo.c \
	    $(LIBOBJ) -o $(SMPFALLO_BIN) $(LDFLAGS); \
	  echo "--- una excepcion sin atrapar con SMP (esperado: OK / RuntimeError / RuntimeError y la linea OK) ---"; \
	  (cd samples && ../$(SMPFALLO_BIN) SmpFallo.mod); \
	fi

# V7/H7 #600 (28-sep) — lo que se empuja DE UNA VEZ a la cola de salida sale SEGUIDO (bpvm_oq_pushv): la línea de
# un `print` y un bloque de varias líneas impreso con un solo `print`. Cuatro productores, una cola de 256 B casi
# siempre llena, y cada registro de tres trozos tiene que salir entero y en orden. Ver test/test_oqentero.c.
OQENTERO_BIN = build/test_oqentero.exe
test-oqentero: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_oqentero.c $(LIBOBJ) -o $(OQENTERO_BIN) $(LDFLAGS)
	./$(OQENTERO_BIN)

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
# V7/H7 (#599, 28-sep) — la tabla ES SMP-safe desde 7a (0 corrupciones con 4 y 8),
# así que las pasadas concurrentes ya no pueden fallar sin que se note: sin el `-`,
# con su línea OK, y en la red (check-testc), que no la corría.
SMPH_BIN = build/test_smp_handles.exe
test-smphandles: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_smp_handles.c $(LIBOBJ) -o $(SMPH_BIN) $(LDFLAGS)
	@echo "--- CONTROL 1 thread (esperado: corrupciones=0) ---"
	./$(SMPH_BIN) 1 200000
	@echo "--- concurrente 4 y 8 threads (0 = tabla SMP-safe) ---"
	./$(SMPH_BIN) 4 200000
	./$(SMPH_BIN) 8 200000
	@echo "OK: la tabla de handles aguanta 4 y 8 hilos a la vez sin una corrupcion"

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
# AOT y, con --interp, sin el (GC en cada alocacion en los dos): la misma salida, 46/46 y ningun
# FALLO. El AOT, con la stdlib fresca (aotmain, #541): sin la interfaz de Core, rojo. En check-testc.
test-aotswitch: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatSwitch)
	$(call stdlibdep,Core)
	$(call aotmain,NatSwitch,build/aotswitch)
	$(CC) $(CFLAGS) -Isrc test/test_aotswitch.c build/aotswitch/aot_NatSwitch.c $(LIBOBJ) \
	  -o build/test_aotswitch.exe $(LDFLAGS)
	@echo "--- #546/#548 el switch y el continue en una native (esperado: AOT == interpretado, 46/46, sin FALLO) ---"
	@./build/test_aotswitch.exe samples/NatSwitch.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotswitch.aot; \
	 ./build/test_aotswitch.exe samples/NatSwitch.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotswitch.int; \
	 n=$$(grep -c '^ok ' build/aotswitch.aot); \
	 if grep -q '^FALLO' build/aotswitch.aot build/aotswitch.int; then grep -h '^FALLO' build/aotswitch.aot build/aotswitch.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotswitch.int build/aotswitch.aot; then diff build/aotswitch.int build/aotswitch.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 46 ]; then cat build/aotswitch.aot; echo "  ROJO: $$n de 46 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 46/46 con el AOT, igual que interpretado"

# V7 #577 (7-oct) — LAS TUPLAS DENTRO DE UNA NATIVE (Eduardo: «getters, sin ABI»; «la variante»): desestructurar y
# construir, por los accesores ocultos del módulo (__tup_get_/__tup_new_) que la native llama por el puente; también
# la tupla de OTRO módulo (Str.parseInt). El MISMO .mod con el AOT y, con --interp, sin él (GC en cada alocación en los
# dos): la misma salida, 4/4 y ningún FALLO. Ver test/test_aottuplas.c.
test-aottuplas: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatTuplas)
	$(call stdlibdep,Core Str)
	$(call aotmain,NatTuplas,build/aottuplas)
	$(CC) $(CFLAGS) -Isrc test/test_aottuplas.c build/aottuplas/aot_NatTuplas.c $(LIBOBJ) \
	  -o build/test_aottuplas.exe $(LDFLAGS)
	@echo "--- #577 las tuplas en una native (esperado: AOT == interpretado, 4/4, sin FALLO) ---"
	@./build/test_aottuplas.exe samples/NatTuplas.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aottuplas.aot; \
	 ./build/test_aottuplas.exe samples/NatTuplas.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aottuplas.int; \
	 n=$$(grep -c '^ok ' build/aottuplas.aot); \
	 if grep -q '^FALLO' build/aottuplas.aot build/aottuplas.int; then grep -h '^FALLO' build/aottuplas.aot build/aottuplas.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aottuplas.int build/aottuplas.aot; then diff build/aottuplas.int build/aottuplas.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 4 ]; then cat build/aottuplas.aot; echo "  ROJO: $$n de 4 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 4/4 con el AOT, igual que interpretado"

# V7/H5 (#569/#570, 26-sep) — LA LISTA A MANO DEL AOT contra el camino interpretado: charAt,
# charCodeAt, substring, intToString, len y los newXArray iban a COPIAS en C de sus builtins, y una copia
# se quedaba atrás sin que nada lo dijera (charAt fuera de rango: "" con el AOT, un error interpretado).
# Desde P2b (la opción 3) helper y builtin llaman a la MISMA función (bpvm_prims.h): la prueba vigila las
# entradas, y lo que falle DENTRO de la primitiva lo caza la paridad contra miVM, no ésta.
test-aotstr: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatStr)
	$(call stdlibdep,Core)
	$(call aotmain,NatStr,build/aotstr)
	$(CC) $(CFLAGS) -Isrc test/test_aotstr.c build/aotstr/aot_NatStr.c $(LIBOBJ) \
	  -o build/test_aotstr.exe $(LDFLAGS)
	@echo "--- #569/#570 la lista a mano del AOT (esperado: AOT == interpretado, 17/17, sin FALLO) ---"
	@./build/test_aotstr.exe samples/NatStr.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotstr.aot; \
	 ./build/test_aotstr.exe samples/NatStr.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotstr.int; \
	 n=$$(grep -c '^ok ' build/aotstr.aot); \
	 if grep -q '^FALLO' build/aotstr.aot build/aotstr.int; then grep -h '^FALLO' build/aotstr.aot build/aotstr.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotstr.int build/aotstr.aot; then diff build/aotstr.int build/aotstr.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 17 ]; then cat build/aotstr.aot; echo "  ROJO: $$n de 17 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 17/17 con el AOT, igual que interpretado"

# V7/H5 (#573, 26-sep) — LOS OPERADORES de arrays y cadenas desde una native contra el camino interpretado: `a[i]`,
# `a[i] := v` y el `+` de cadenas iban a helpers de C con su PROPIO texto de error («array_load_i32: index out of
# bounds» donde el intérprete y miVM dicen «ALOAD: índice fuera de rango 7 (length=3)») y el concat sin memoria daba un
# null callado. Ahora el texto sale de UNA función (bpvm_prim_texto_indice) que usan el opcode y el helper, cada helper
# con SU opcode; un tipo de elemento por helper, al leer y al escribir, null y negativo. Que la salida interpretada sea
# la de miVM lo mira la paridad: NatArr está en el corpus de compat.sh.
test-aotarr: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatArr)
	$(call stdlibdep,Core)
	$(call aotmain,NatArr,build/aotarr)
	$(CC) $(CFLAGS) -Isrc test/test_aotarr.c build/aotarr/aot_NatArr.c $(LIBOBJ) \
	  -o build/test_aotarr.exe $(LDFLAGS)
	@echo "--- #573 los operadores de arrays del AOT (esperado: AOT == interpretado, 21/21, sin FALLO) ---"
	@./build/test_aotarr.exe samples/NatArr.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotarr.aot; \
	 ./build/test_aotarr.exe samples/NatArr.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotarr.int; \
	 n=$$(grep -c '^ok ' build/aotarr.aot); \
	 if grep -q '^FALLO' build/aotarr.aot build/aotarr.int; then grep -h '^FALLO' build/aotarr.aot build/aotarr.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotarr.int build/aotarr.aot; then diff build/aotarr.int build/aotarr.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 21 ]; then cat build/aotarr.aot; echo "  ROJO: $$n de 21 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 21/21 con el AOT, igual que interpretado"

# V7/H5 (P2b.3.a, 27-sep) — EL EJECUTOR de una native y EL TESTIGO, las dos piezas de la plataforma, solas:
# idas y vueltas con estado vivo (y lo que cuesta un paso), terminar uno parado a mitad, un marco grande.
test-ejecutor: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_ejecutor.c $(LIBOBJ) -o build/test_ejecutor.exe $(LDFLAGS)
	@echo "--- P2b.3.a el ejecutor y el testigo (esperado: solo 'ok', [status=OK]) ---"
	@timeout 60 ./build/test_ejecutor.exe || { echo "  ROJO: el ejecutor o el testigo (¿colgado? lo cortó el tope)"; exit 1; }

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

# V7/H12 — LAS NATIVES CON DOS RUNS (samples/NatDosRuns.bp; docs/V7_IDEAS.md §H12 «Las natives con dos runs»), con
# el GC colectando en cada reserva: una native DIRECTA con dos reservas encadenadas, llamada a la vez desde los dos runs
# (si una reserva encuentra al otro run colectando, espera dentro y el colector escanea su pila de C); dos natives con
# bucle a la vez (dos ejecutores del depósito, nunca el mismo); y una native con bucle que espera a un hilo del otro run
# que reserva (sin ejecutor, espera al mundo en su línea de bucle). El MISMO .mod con el AOT, con y sin ejecutor, da lo
# que interpretado; y los tres sabotajes, rojos (el de la línea del bucle, colgado: lo corta el plazo). En check-testc.
# (6-oct) Y la pasada normal EJERCITA N3 —el colector ve natives aparcadas: al menos 100; medido, ~1 900—, con la
# reserva desde una native 300 µs más lenta (test_nat2runs.c): sin eso N3 se daba ~1 vez de cada 100 llamadas, y su
# sabotaje salió verde en la red (medido: 8 y 6 de 10 rojas); con eso, 10 de 10, con ~500 etiquetas malas de 600.
NAT2_BIN = build/test_nat2runs.exe
test-nat2runs: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatDosRuns)
	$(call stdlibdep,Core)
	$(call aotmain,NatDosRuns,build/aotnat2)
	@$(CC) $(CFLAGS) -Isrc test/test_nat2runs.c build/aotnat2/aot_NatDosRuns.c $(LIBOBJ) -o $(NAT2_BIN) $(LDFLAGS)
	@echo "--- V7/H12 las natives con dos runs (esperado: AOT == interpretado, con y sin ejecutor, con N3 ejercitado; y los tres sabotajes, ROJOS) ---"
	@timeout 120 ./$(NAT2_BIN) samples/NatDosRuns.mod --interp 2>/dev/null > build/nat2.int; \
	 if ! grep -q '^fin' build/nat2.int; then echo "  ROJO: la pasada interpretada no acaba"; cat build/nat2.int; exit 1; fi
	@for m in "" "--sin-ejecutor"; do \
	   timeout 120 ./$(NAT2_BIN) samples/NatDosRuns.mod $$m 2>build/nat2.err > build/nat2.aot; st=$$?; \
	   if [ $$st -ne 0 ] || ! cmp -s build/nat2.int build/nat2.aot; then \
	     echo "  ROJO (AOT $$m, salida $$st):"; diff build/nat2.int build/nat2.aot; grep -a 'status\|runtime_error' build/nat2.err; exit 1; fi; \
	   vistas=$$(grep -ao 'nativas_aparcadas=[0-9]*' build/nat2.err | grep -o '[0-9]*$$'); \
	   if [ "$${vistas:-0}" -lt 100 ]; then \
	     echo "  ROJO (AOT $$m): N3 sin ejercitar: el colector vio $${vistas:-0} natives aparcadas (menos de 100)"; exit 1; fi; \
	   echo "  ok: con el AOT y dos runs $$m, lo mismo que interpretado (N3, ejercitado $$vistas veces)"; \
	 done
	@for sab in "--sabotaje-aparcada" "--sabotaje-deposito" "--sin-ejecutor --sabotaje-anidada"; do \
	   timeout 30 ./$(NAT2_BIN) samples/NatDosRuns.mod $$sab 2>/dev/null > build/nat2.sab; st=$$?; \
	   if [ $$st -ne 0 ] || ! cmp -s build/nat2.int build/nat2.sab; then echo "  (rojo, como debe: $$sab; salida $$st)"; \
	   else echo "FAIL test-nat2runs: el sabotaje $$sab no se caza"; cat build/nat2.sab; exit 1; fi; \
	 done
	@echo "PASS test-nat2runs"

# V7/H5 (P2b.1.d, 26-sep) — el KILL de una native en un bucle sin fin (con tope de tiempo).
test-aotpesada: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatPesada)
	$(call stdlibdep,Core)
	$(call aotmain,NatPesada,build/aotpesada)
	$(CC) $(CFLAGS) -Isrc test/test_aotpesada.c build/aotpesada/aot_NatPesada.c $(LIBOBJ) \
	  -o build/test_aotpesada.exe $(LDFLAGS)
	@echo "--- V7 #592 una native con vueltas LENTAS cede a tiempo por la marca del 2º temporizador (esperado: las dos KILLED bajo el tope, [status=OK]) ---"
	@timeout 60 ./build/test_aotpesada.exe samples/NatPesada.mod 2>/dev/null || { echo "  ROJO: alguna pasada no salió KILLED bajo el tope (la native no cedió a tiempo)"; exit 1; }

test-aotkill: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatKill NatRecurKill)
	$(call stdlibdep,Core)
	$(call aotmain,NatKill,build/aotkill)
	$(call aotmain,NatRecurKill,build/aotkill2)
	$(CC) $(CFLAGS) -Isrc test/test_aotkill.c build/aotkill/aot_NatKill.c build/aotkill2/aot_NatRecurKill.c $(LIBOBJ) \
	  -o build/test_aotkill.exe $(LDFLAGS)
	@echo "--- P2b.1.d el kill de una native en un bucle sin fin, y #580 de una recursiva sin bucles (esperado: las cinco KILLED —la 3ª sin ejecutor, #640—, [status=OK]) ---"
	@timeout 60 ./build/test_aotkill.exe samples/NatKill.mod samples/NatRecurKill.mod 2>/dev/null || { echo "  ROJO: alguna pasada no salió KILLED (o el kill no llegó y la cortó el tope)"; exit 1; }

# V7/H5 (P2b.2, 26-sep) — CUALQUIER INTRÍNSECA desde una native, por el despachador de la VM (call_builtin):
# de Core y de otro módulo, una que lanza y un sleep que suspende la native mientras otro hilo avanza.
test-aotbuiltin: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatBuiltin)
	$(call stdlibdep,Core Math)
	$(call aotmain,NatBuiltin,build/aotbuiltin)
	$(CC) $(CFLAGS) -Isrc test/test_aotbuiltin.c build/aotbuiltin/aot_NatBuiltin.c $(LIBOBJ) \
	  -o build/test_aotbuiltin.exe $(LDFLAGS)
	@echo "--- P2b.2 cualquier intrínseca desde una native (esperado: AOT == interpretado, 8/8, sin FALLO) ---"
	@timeout 60 ./build/test_aotbuiltin.exe samples/NatBuiltin.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotbuiltin.aot; \
	 timeout 60 ./build/test_aotbuiltin.exe samples/NatBuiltin.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotbuiltin.int; \
	 n=$$(grep -c '^ok ' build/aotbuiltin.aot); \
	 if grep -q '^FALLO' build/aotbuiltin.aot build/aotbuiltin.int; then grep -h '^FALLO' build/aotbuiltin.aot build/aotbuiltin.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotbuiltin.int build/aotbuiltin.aot; then diff build/aotbuiltin.int build/aotbuiltin.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 8 ]; then cat build/aotbuiltin.aot; echo "  ROJO: $$n de 8 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 8/8 con el AOT, igual que interpretado"

# V8/H5 (#713, 21-oct) — <, <=, >, >=, == y != entre cadenas DENTRO de una native: los relacionales llaman al builtin
# __strCompare (el 282, un memcmp) por el despachador común, call_builtin, sin tocar la ABI; == y !=, por string_eq. El
# MISMO .mod con el AOT y sin él: la misma salida, 490 comprobaciones contra una referencia interpretada, ningún FALLO.
test-aotcompara: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatCompara)
	$(call stdlibdep,Core)
	$(call aotmain,NatCompara,build/aotcompara)
	@c=build/aotcompara/aot_NatCompara.c; \
	 n=$$(grep -c "bpvm_aot_register_by_name" $$c); \
	 if [ "$$n" -ne 6 ]; then cat build/aotcompara/aot.log; echo "  ROJO: el AOT tradujo $$n de las 6 natives de NatCompara"; exit 1; fi; \
	 r=$$(grep -o "call_builtin(vm, 282, " $$c | wc -l); \
	 if [ "$$r" -ne 4 ]; then echo "  ROJO: los 4 relacionales tenian que llamar al builtin 282 (salen $$r)"; exit 1; fi
	$(CC) $(CFLAGS) -Isrc test/test_aotcompara.c build/aotcompara/aot_NatCompara.c $(LIBOBJ) \
	  -o build/test_aotcompara.exe $(LDFLAGS)
	@echo "--- <, <=, >, >=, == y != entre cadenas en una native (esperado: AOT == interpretado, 494/494, sin FALLO) ---"
	@timeout 60 ./build/test_aotcompara.exe samples/NatCompara.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotcompara.aot; \
	 timeout 60 ./build/test_aotcompara.exe samples/NatCompara.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotcompara.int; \
	 n=$$(grep -c '^ok ' build/aotcompara.aot); \
	 if grep -q '^FALLO' build/aotcompara.aot build/aotcompara.int; then grep -h '^FALLO' build/aotcompara.aot build/aotcompara.int | head; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotcompara.int build/aotcompara.aot; then diff build/aotcompara.int build/aotcompara.aot | head; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 494 ]; then tail -3 build/aotcompara.aot; echo "  ROJO: $$n de 494 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 494/494 con el AOT, igual que interpretado"

# V8 #699 (24-oct) — `continue` dentro de un `do … loop c` va a la CONDICIÓN, en el intérprete como en el C del AOT
# (`do { … } while (c)`). Hasta #699 el intérprete volvía arriba sin mirarla: hacer `native` una función le cambiaba
# el resultado. Las cinco natives de NatDoContinue (en el corpus corren interpretadas) con el AOT y sin él (--interp):
# la misma salida, completa y sin ningún FALLO.
test-aotdo: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatDoContinue)
	$(call stdlibdep,Core)
	$(call aotmain,NatDoContinue,build/aotdo)
	@c=build/aotdo/aot_NatDoContinue.c; \
 n=$$(grep -c "bpvm_aot_register_by_name" $$c); \
 if [ "$$n" -ne 5 ]; then cat build/aotdo/aot.log; echo "  ROJO: el AOT tradujo $$n de las 5 natives de NatDoContinue"; exit 1; fi
	$(CC) $(CFLAGS) -Isrc test/test_aotdo.c build/aotdo/aot_NatDoContinue.c $(LIBOBJ) \
  -o build/test_aotdo.exe $(LDFLAGS)
	@echo "--- #699 el continue de un do ... loop c va a la condicion, en una native y en el interprete (esperado: AOT == interpretado, 5/5, sin FALLO) ---"
	@timeout 60 ./build/test_aotdo.exe samples/NatDoContinue.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotdo.aot; \
 timeout 60 ./build/test_aotdo.exe samples/NatDoContinue.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotdo.int; \
 n=$$(grep -c '^ok ' build/aotdo.aot); \
 if grep -q '^FALLO' build/aotdo.aot build/aotdo.int; then grep -h '^FALLO' build/aotdo.aot build/aotdo.int | head; echo "  ROJO: FALLO"; exit 1; fi; \
 if ! cmp -s build/aotdo.int build/aotdo.aot; then diff build/aotdo.int build/aotdo.aot | head; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
 if [ "$$n" -ne 5 ]; then tail -3 build/aotdo.aot; echo "  ROJO: $$n de 5 comprobaciones"; exit 1; fi; \
 echo "[status=OK] 5/5 con el AOT, igual que interpretado"

# V8/H7.1 (#747, 31-oct) — LOS COMPUESTOS NUEVOS Y EL TERNARIO EN UNA NATIVE: el AOT escribe `x op= v` como `x = x op v`
# por el camino de la expresión binaria (los helpers de la división de long, de double y de float) y `iif` como el `?:`
# de C, que sólo calcula la rama que toca. Las 10 natives de NatCompuestos, con el AOT y sin él: lo mismo, 13 de 13.
test-aotcompuestos: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatCompuestos)
	$(call stdlibdep,Core)
	$(call aotmain,NatCompuestos,build/aotcompuestos)
	@c=build/aotcompuestos/aot_NatCompuestos.c; \
 n=$$(grep -c "bpvm_aot_register_by_name" $$c); \
 if [ "$$n" -ne 10 ]; then cat build/aotcompuestos/aot.log; echo "  ROJO: el AOT tradujo $$n de las 10 natives de NatCompuestos"; exit 1; fi
	$(CC) $(CFLAGS) -Isrc test/test_aotcompuestos.c build/aotcompuestos/aot_NatCompuestos.c $(LIBOBJ) \
  -o build/test_aotcompuestos.exe $(LDFLAGS)
	@echo "--- #747 los compuestos nuevos y el ternario en una native (esperado: AOT == interpretado, 13/13, sin FALLO) ---"
	@timeout 60 ./build/test_aotcompuestos.exe samples/NatCompuestos.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotcompuestos.aot; \
 timeout 60 ./build/test_aotcompuestos.exe samples/NatCompuestos.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotcompuestos.int; \
 n=$$(grep -c '^ok ' build/aotcompuestos.aot); \
 if grep -q '^FALLO' build/aotcompuestos.aot build/aotcompuestos.int; then grep -h '^FALLO' build/aotcompuestos.aot build/aotcompuestos.int | head; echo "  ROJO: FALLO"; exit 1; fi; \
 if ! cmp -s build/aotcompuestos.int build/aotcompuestos.aot; then diff build/aotcompuestos.int build/aotcompuestos.aot | head; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
 if [ "$$n" -ne 13 ]; then tail -3 build/aotcompuestos.aot; echo "  ROJO: $$n de 13 comprobaciones"; exit 1; fi; \
 echo "[status=OK] 13/13 con el AOT, igual que interpretado"

# V8 #724 (27-oct) — EL TEXTO QUE DEVUELVE UN PACK, por el puente que escribe el compilador, con un pack de mentira
# ('PRUE', lo publica el test): entero —sin el corte de 512 de antes, que además partía una ñ—, descodificado si la
# native devuelve `string` (EncodingError, como readFile), tal cual si devuelve `byte[]`, y sin NUL en 1 MB, un error
# con nombre. Lo que tiene que salir lo escribe a mano el sample: 14 comprobaciones y ningún FALLO. Y las dos natives,
# DIRECTAS: la copia y fromBytes van en una función aparte del puente.
test-packtexto: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,PackTexto)
	$(call stdlibdep,Core)
	$(call aotmain,PackTexto,build/packtexto)
	@c=build/packtexto/aot_PackTexto.c; \
 for f in prb_texto prb_bytes; do grep -q "PackTexto\.$$f\", .*BPVM_AOT_DIRECTA" $$c || { echo "  ROJO: $$f (del pack) tenia que salir DIRECTA"; exit 1; }; done
	$(CC) $(CFLAGS) -Isrc test/test_packtexto.c build/packtexto/aot_PackTexto.c $(LIBOBJ) \
  -o build/test_packtexto.exe $(LDFLAGS)
	@echo "--- #724 el texto que devuelve un pack: entero y descodificado (esperado: 14/14, sin FALLO) ---"
	@timeout 120 ./build/test_packtexto.exe samples/PackTexto.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/packtexto.out; \
 n=$$(grep -c '^ok ' build/packtexto.out); \
 if grep -q '^FALLO' build/packtexto.out; then grep '^FALLO' build/packtexto.out | cut -c1-300; echo "  ROJO: FALLO"; exit 1; fi; \
 if [ "$$n" -ne 14 ]; then cut -c1-300 build/packtexto.out; echo "  ROJO: $$n de 14 comprobaciones"; exit 1; fi; \
 echo "[status=OK] 14/14"

# V8 #724 (27-oct) — EL TEXTO DE SQLITE, de punta a punta en el PC: SqlTexto.bp → SQLite.bp → el puente que escribe
# el compilador de HOY (AotMain en cada vuelta: test-sqldemo enlaza uno guardado) → el pack 'SQLI' → SQLite. Un texto
# de 600 bytes, entero; uno en Latin-1 (un CAST de un blob), con EncodingError o «�»; el nombre de una columna, y el
# mensaje de un error. Lo que tiene que salir lo escribe a mano el sample: 9 comprobaciones y ningún FALLO. Y SqlDemo,
# el del 10-ago, con el mismo puente: status=OK, sus 25 líneas (las de la Metro) y ningún bloque sin liberar.
SQLTEXTO = build/sqltexto
test-sqltexto: $(LIBOBJ) build/sql/sqlite3.o
	@rm -rf $(SQLTEXTO) && mkdir -p $(SQLTEXTO)
	@cp ../bpstdlib/*.mod ../bpstdlib/sqlite/SQLite.bp samples/SqlTexto.bp ../samples/SqlDemo.bp $(SQLTEXTO)/
	@cd $(SQLTEXTO) && for f in SQLite SqlTexto SqlDemo; do \
   java -jar ../../../lexer-java/target/basicplus-frontend.jar $$f.bp --compile . --backend=mivm >> compila.log 2>&1; \
   [ -f $$f.mod ] || { cat compila.log; echo "  ROJO: $$f no compila"; exit 1; }; \
 done
	@java -cp "../lexer-java/target/classes;../pack/target/classes;../miVM/target/classes" basicplus.frontend.AotMain \
   $(SQLTEXTO)/SQLite.bp $(SQLTEXTO) > $(SQLTEXTO)/aot.log 2>&1 || { cat $(SQLTEXTO)/aot.log; exit 1; }
	$(CC) $(CFLAGS) -Isrc -Ibuild/sql -I$(SQLSRC) test/test_sqldemo.c $(SQLTEXTO)/aot_SQLite.c \
  $(SQLSRC)/sqlite_shim.c $(SQLSRC)/vfs_bp.c $(SQLSRC)/packglue.c \
  build/sql/sqlite3.o $(LIBOBJ) -o build/sqltexto.exe $(LDFLAGS) $(SQLDEF)
	@echo "--- #724 el texto de SQLite, de punta a punta (esperado: 9/9 sin FALLO, y SqlDemo entero) ---"
	@cd $(SQLTEXTO) && timeout 120 ../sqltexto.exe SqlTexto.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > ../sqltexto.out; \
 n=$$(grep -c '^ok ' ../sqltexto.out); \
 if grep -q '^FALLO' ../sqltexto.out; then grep '^FALLO' ../sqltexto.out | cut -c1-300; echo "  ROJO: FALLO"; exit 1; fi; \
 if [ "$$n" -ne 9 ]; then cut -c1-300 ../sqltexto.out; echo "  ROJO: $$n de 9 comprobaciones"; exit 1; fi; \
 echo "[status=OK] 9/9"
	@cd $(SQLTEXTO) && rm -f medidas.db && timeout 120 ../sqltexto.exe SqlDemo.mod > ../sqldemo.out 2> ../sqldemo.err; \
 n=$$(wc -l < ../sqldemo.out); \
 if ! grep -q '^\[status=OK\]' ../sqldemo.err || ! grep -q '(0 bloques sin liberar' ../sqldemo.err || [ "$$n" -ne 25 ]; then \
   tail -20 ../sqldemo.out ../sqldemo.err; echo "  ROJO: SqlDemo ($$n lineas)"; exit 1; fi; \
 echo "[status=OK] SqlDemo: 25 lineas, sin fugas"

# V8/H4 (#708, 27-oct) — LO COMÚN DEL SONIDO (bpvm_sonido.c), en el PC y sin que suene nada: la línea `audio` del ENV
# y la tarea con una cintura que lo APUNTA todo —el formato, el volumen, cada muestra (las de 8 bits a 16 con signo)
# y cómo se cierra (drenando al acabar; en seco al parar o al cambiar de sonido)—. Lo que tiene que salir, escrito a
# mano en la prueba: ningún FALLO.
test-sonido: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_sonido.c $(LIBOBJ) -o build/test_sonido.exe $(LDFLAGS)
	@echo "--- V8/H4 #708 lo comun del sonido (esperado: [status=OK], sin FALLO) ---"
	./build/test_sonido.exe

# V7/H5 (P2b.3, 27-sep) — el .mdn v3: la marca DIRECTA (bit alto de thunk_offset) llega al registro de natives
# y no se cuela en la dirección del thunk; un v2 las manda todas al ejecutor; un v4 se rechaza.
test-mdnv3: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatDirecta)
	$(call stdlibdep,Core)
	$(CC) $(CFLAGS) -Isrc test/test_mdnv3.c $(LIBOBJ) -o build/test_mdnv3.exe $(LDFLAGS)
	@echo "--- P2b.3 la marca DIRECTA del .mdn v3 (esperado: solo 'ok', [status=OK]) ---"
	@./build/test_mdnv3.exe samples/NatDirecta.mod 2>/dev/null

# V7/H5 (P2b.3, 27-sep) — las natives DIRECTAS: sin puntos de cesión (ni bucles, ni builtins por el despachador,
# ni BP, ni packs; ni a través de otra native del módulo) corren en la pila del worker, sin ejecutor. La
# clasificación, mirada en el C que emite el AOT (el registro de las pruebas de host lleva la marca), y el MISMO
# .mod con el AOT y sin él.
test-aotdirecta: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatDirecta)
	$(call stdlibdep,Core)
	$(call aotmain,NatDirecta,build/aotdirecta)
	$(CC) $(CFLAGS) -Isrc test/test_aotdirecta.c build/aotdirecta/aot_NatDirecta.c $(LIBOBJ) \
	  -o build/test_aotdirecta.exe $(LDFLAGS)
	@echo "--- P2b.3 las natives directas (esperado: 3 directas, 6 al ejecutor —fibR, recursiva, desde #580—; AOT == interpretado, 9/9) ---"
	@c=build/aotdirecta/aot_NatDirecta.c; mal=0; \
	 for f in hoja llamaHoja texto; do \
	   grep -q "NatDirecta\.$$f\", .*BPVM_AOT_DIRECTA" $$c || { echo "  ROJO: $$f tenia que salir DIRECTA"; mal=1; }; \
	 done; \
	 for f in conBucle pequena llamaPequena conRaiz llamaBp fibR; do \
	   grep -q "bpvm_aot_register_by_name(vm, \"NatDirecta\.$$f\"" $$c || { echo "  ROJO: $$f tenia que ir a un EJECUTOR"; mal=1; }; \
	 done; \
	 if [ $$mal -ne 0 ]; then exit 1; fi; \
	 echo "  ok  : hoja, llamaHoja y texto, DIRECTAS; conBucle, pequena, llamaPequena, conRaiz, llamaBp y fibR (#580), al ejecutor"
	$(call aotmain,NatDirectaPack,build/aotdirectapack)
	@c=build/aotdirectapack/aot_NatDirectaPack.c; mal=0; \
	 for f in suma nombre; do \
	   grep -q "NatDirectaPack\.$$f\", .*BPVM_AOT_DIRECTA" $$c || { echo "  ROJO: $$f (del pack) tenia que salir DIRECTA"; mal=1; }; \
	 done; \
	 if [ $$mal -ne 0 ]; then exit 1; fi; \
	 echo "  ok  : llamar a un pack no cede: suma y nombre (del pack), DIRECTAS"
	@./build/test_aotdirecta.exe samples/NatDirecta.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotdirecta.aot; \
	 ./build/test_aotdirecta.exe samples/NatDirecta.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotdirecta.int; \
	 n=$$(grep -c '^ok ' build/aotdirecta.aot); \
	 if grep -q '^FALLO' build/aotdirecta.aot build/aotdirecta.int; then grep -h '^FALLO' build/aotdirecta.aot build/aotdirecta.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotdirecta.int build/aotdirecta.aot; then diff build/aotdirecta.int build/aotdirecta.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 9 ]; then cat build/aotdirecta.aot; echo "  ROJO: $$n de 9 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 9/9 con el AOT, igual que interpretado"

# V7/H5 (P2b.3, 27-sep) — LO QUE CUESTA LLAMAR a una native desde código interpretado: 200 000 llamadas a una
# native diminuta, con el AOT y sin él. No es una prueba (no entra en check-testc): es la medida de antes y
# después de pasar las natives a un ejecutor.
medir-llamada: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatLlamada)
	$(call stdlibdep,Core)
	$(call aotmain,NatLlamada,build/aotllamada)
	$(CC) $(CFLAGS) -Isrc test/test_aotllamada.c build/aotllamada/aot_NatLlamada.c $(LIBOBJ) \
	  -o build/test_aotllamada.exe $(LDFLAGS)
	@./build/test_aotllamada.exe samples/NatLlamada.mod 2>/dev/null
	@./build/test_aotllamada.exe samples/NatLlamada.mod --interp 2>/dev/null

# V7/H5 (#588, 27-sep) — LAS GLOBALES DEL MÓDULO desde una native, con globales de OTROS tipos por delante (una string
# con valor inicial, un long, un array, otra string): el AOT contaba la posición de cada una por su cuenta (-4, -8…,
# sólo las de 4 bytes) y leía otra palabra sin un error. Ahora la toma del compilador (MivmEmitter.offsetsDeGlobales).
test-aotglobales: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatGlobales)
	$(call stdlibdep,Core)
	$(call aotmain,NatGlobales,build/aotglobales)
	@n=$$(grep -c "bpvm_aot_register_by_name" build/aotglobales/aot_NatGlobales.c); \
	 if [ "$$n" -ne 4 ]; then cat build/aotglobales/aot.log; echo "  ROJO: el AOT tradujo $$n de las 4 natives de NatGlobales"; exit 1; fi
	$(CC) $(CFLAGS) -Isrc test/test_aotglobales.c build/aotglobales/aot_NatGlobales.c $(LIBOBJ) \
	  -o build/test_aotglobales.exe $(LDFLAGS)
	@echo "--- #588 globales del modulo desde una native (esperado: AOT == interpretado, 7/7, sin FALLO) ---"
	@./build/test_aotglobales.exe samples/NatGlobales.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotglobales.aot; \
	 ./build/test_aotglobales.exe samples/NatGlobales.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotglobales.int; \
	 n=$$(grep -c '^ok ' build/aotglobales.aot); \
	 if grep -q '^FALLO' build/aotglobales.aot build/aotglobales.int; then grep -h '^FALLO' build/aotglobales.aot build/aotglobales.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotglobales.int build/aotglobales.aot; then diff build/aotglobales.int build/aotglobales.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 7 ]; then cat build/aotglobales.aot; echo "  ROJO: $$n de 7 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 7/7 con el AOT, igual que interpretado"

# V7/H5 (27-sep) — LAS CONVERSIONES ESTRECHAS dentro de una native (`byte(x)`, `int8`, `word`, `int16`, `short`):
# el emisor las escribe en el C del módulo con el rango y el TEXTO de OP_I32_TO_U8 y compañía (la opción A de Eduardo:
# sin tocar la ABI). El texto está en la VM y en el emisor, y esto es lo que lo vigila: los dos bordes de cada una y
# uno fuera por cada lado, con el AOT y sin él, y la misma salida. Se cuentan las natives traducidas (una que se
# cayera correría interpretada y daría igual) y se mira que la del bucle vaya a un EJECUTOR y las demás, directas.
test-aotestrecha: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatEstrecha)
	$(call stdlibdep,Core)
	$(call aotmain,NatEstrecha,build/aotestrecha)
	@c=build/aotestrecha/aot_NatEstrecha.c; \
	 n=$$(grep -c "bpvm_aot_register_by_name" $$c); \
	 if [ "$$n" -ne 7 ]; then cat build/aotestrecha/aot.log; echo "  ROJO: el AOT tradujo $$n de las 7 natives de NatEstrecha"; exit 1; fi; \
	 grep -q "bpvm_aot_register_by_name(vm, \"NatEstrecha\.sumaBytes\"" $$c || { echo "  ROJO: sumaBytes (con bucle) tenia que ir a un EJECUTOR"; exit 1; }; \
	 grep -q "NatEstrecha\.aByte\", .*BPVM_AOT_DIRECTA" $$c || { echo "  ROJO: aByte tenia que salir DIRECTA"; exit 1; }
	$(CC) $(CFLAGS) -Isrc test/test_aotestrecha.c build/aotestrecha/aot_NatEstrecha.c $(LIBOBJ) \
	  -o build/test_aotestrecha.exe $(LDFLAGS)
	@echo "--- conversiones estrechas en una native (esperado: AOT == interpretado, 23/23, sin FALLO) ---"
	@./build/test_aotestrecha.exe samples/NatEstrecha.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotestrecha.aot; \
	 ./build/test_aotestrecha.exe samples/NatEstrecha.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotestrecha.int; \
	 n=$$(grep -c '^ok ' build/aotestrecha.aot); \
	 if grep -q '^FALLO' build/aotestrecha.aot build/aotestrecha.int; then grep -h '^FALLO' build/aotestrecha.aot build/aotestrecha.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotestrecha.int build/aotestrecha.aot; then diff build/aotestrecha.int build/aotestrecha.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 23 ]; then cat build/aotestrecha.aot; echo "  ROJO: $$n de 23 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 23/23 con el AOT, igual que interpretado"

# V7/H5 (#572, 27-sep) — LOS IDENTIFICADORES DE BP EN EL C DEL AOT: una native con parámetros, locales e iterador
# llamados `vm`, `mem`, `cs`, `int`, `char`, `static`, `register`, `bool`, `INT32_MAX`… (en un módulo con globales), y el
# puente a un pack con parámetros que se llaman como lo que el puente declara. Con los nombres tal cual, ninguno de los
# dos C compilaba. Y se CUENTAN las natives traducidas: una que se cayera correría interpretada y la salida daría igual.
test-aotnombres: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatNombres)
	$(call stdlibdep,Core)
	$(call aotmain,NatNombres,build/aotnombres)
	@n=$$(grep -c "bpvm_aot_register_by_name" build/aotnombres/aot_NatNombres.c); \
	 if [ "$$n" -ne 4 ]; then cat build/aotnombres/aot.log; echo "  ROJO: el AOT tradujo $$n de las 4 natives de NatNombres"; exit 1; fi
	$(CC) $(CFLAGS) -Isrc test/test_aotnombres.c build/aotnombres/aot_NatNombres.c $(LIBOBJ) \
	  -o build/test_aotnombres.exe $(LDFLAGS)
	$(call aotmain,NatNombresPack,build/aotnombrespack)
	$(CC) $(CFLAGS) -Isrc -c build/aotnombrespack/aot_NatNombresPack.c -o build/aotnombrespack/aot_NatNombresPack.o
	@# el caso MUDO de antes: con un parámetro `r`, el puente escribía `int32_t r = fn(…, r)` y el `r` del inicializador
	@# ya era el nuevo, sin valor (el pack recibía basura sin un error). Ahora tiene que pasar el suyo, `bp_r`.
	@grep -q "int32_t r = fn(.*, bp_r);" build/aotnombrespack/aot_NatNombresPack.c || { grep -n " r = fn(" build/aotnombrespack/aot_NatNombresPack.c; echo "  ROJO: el puente a un pack no pasa bp_r"; exit 1; }
	@echo "--- #572 nombres de BP que son de C (esperado: los dos C compilan; AOT == interpretado, 6/6, sin FALLO) ---"
	@./build/test_aotnombres.exe samples/NatNombres.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotnombres.aot; \
	 ./build/test_aotnombres.exe samples/NatNombres.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotnombres.int; \
	 n=$$(grep -c '^ok ' build/aotnombres.aot); \
	 if grep -q '^FALLO' build/aotnombres.aot build/aotnombres.int; then grep -h '^FALLO' build/aotnombres.aot build/aotnombres.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotnombres.int build/aotnombres.aot; then diff build/aotnombres.int build/aotnombres.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 6 ]; then cat build/aotnombres.aot; echo "  ROJO: $$n de 6 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 6/6 con el AOT, igual que interpretado; el puente a un pack compila"

# V8 (#752, 1-nov) — UN MÉTODO PRIVADO NATIVE SOBRE NULL. Un privado va fuera de la vtable y se llama con un CALL
# directo; el interpretado mira el receptor al entrar, pero con el AOT la VM secuestra ese CALL hacia la native y el
# prólogo no corre: la native lo mira al entrar. AOT == interpretado, con los dos casos de null (uno que no toca `this`:
# sin la comprobación devolvía su suma sin decir nada). Y se CUENTAN las natives traducidas y las que miran el receptor.
test-aotprivado: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatPrivado)
	$(call stdlibdep,Core)
	$(call aotmain,NatPrivado,build/aotprivado)
	@n=$$(grep -c "bpvm_aot_register_by_name" build/aotprivado/aot_NatPrivado.c); \
	 if [ "$$n" -ne 2 ]; then cat build/aotprivado/aot.log; echo "  ROJO: el AOT tradujo $$n de las 2 natives de NatPrivado"; exit 1; fi
	@n=$$(grep -c "if (bp_this == 0) vm->aot_helpers->throw_runtime" build/aotprivado/aot_NatPrivado.c); \
	 if [ "$$n" -ne 2 ]; then echo "  ROJO: $$n de las 2 natives miran el receptor al entrar"; exit 1; fi
	$(CC) $(CFLAGS) -Isrc test/test_aotprivado.c build/aotprivado/aot_NatPrivado.c $(LIBOBJ) \
	  -o build/test_aotprivado.exe $(LDFLAGS)
	@echo "--- #752 un método privado native sobre null (esperado: AOT == interpretado, 4/4, los dos de null lanzan) ---"
	@./build/test_aotprivado.exe samples/NatPrivado.mod 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotprivado.aot; \
	 ./build/test_aotprivado.exe samples/NatPrivado.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotprivado.int; \
	 n=$$(grep -c '^ok ' build/aotprivado.aot); \
	 if grep -q '^FALLO' build/aotprivado.aot build/aotprivado.int; then grep -h '^FALLO' build/aotprivado.aot build/aotprivado.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotprivado.int build/aotprivado.aot; then diff build/aotprivado.int build/aotprivado.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 4 ]; then cat build/aotprivado.aot; echo "  ROJO: $$n de 4 comprobaciones"; exit 1; fi; \
	 if [ "$$(grep -c 'de null: lanza: referencia nula: llamada a un m' build/aotprivado.aot)" -ne 2 ]; then cat build/aotprivado.aot; echo "  ROJO: los dos de null no lanzan"; exit 1; fi; \
	 echo "[status=OK] 4/4 con el AOT, igual que interpretado; los dos de null lanzan"

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

# V7 #672 (9-oct) — UNA EXCEPCIÓN QUE LANZA EL CÓDIGO BP AL QUE LLAMA UNA NATIVE, sin atraparla: sale por la native
# hasta el `try` de fuera, como en el intérprete (el suelo del puente). Antes buscaba manejador por debajo de la native
# con sus marcos de C vivos: el resto del programa corría dos veces y acababa con «la función BP cedió/falló». Ocho
# casos (cruzar una y dos natives, una clase del usuario, un BP que la atrapa entre dos natives, la native otra vez
# sin excepción y un throw de Main después: ni un suelo rancio), con GC en cada reserva; AOT == interpretado.
test-aotpuente: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatPuente)
	$(call stdlibdep,Core)
	$(call aotmain,NatPuente,build/aotpuente)
	$(CC) $(CFLAGS) -Isrc test/test_aotpuente.c build/aotpuente/aot_NatPuente.c $(LIBOBJ) \
	  -o build/test_aotpuente.exe $(LDFLAGS)
	@echo "--- #672 una excepcion de BP cruza la native (esperado: AOT == interpretado, 9/9, sin FALLO) ---"
	@./build/test_aotpuente.exe samples/NatPuente.mod 2>build/aotpuente.err | grep -a -E '^(ok|FALLO) ' > build/aotpuente.aot; \
	 ./build/test_aotpuente.exe samples/NatPuente.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aotpuente.int; \
	 n=$$(grep -c '^ok ' build/aotpuente.aot); \
	 if ! grep -a -q '^\[status=OK\]' build/aotpuente.err; then grep -a '^\[status\|^\[runtime_error' build/aotpuente.err; echo "  ROJO: el programa no acabo en OK con el AOT"; exit 1; fi; \
	 if grep -q '^FALLO' build/aotpuente.aot build/aotpuente.int; then grep -h '^FALLO' build/aotpuente.aot build/aotpuente.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aotpuente.int build/aotpuente.aot; then diff build/aotpuente.int build/aotpuente.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 9 ]; then cat build/aotpuente.aot; echo "  ROJO: $$n de 9 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 9/9 con el AOT, igual que interpretado"

# V7 #578 (9-oct) — EL `try` DENTRO DE UNA NATIVE, el lado de la VM. Natives ESCRITAS A MANO (el C que emitirá el AOT:
# el aterrizaje con __builtin_setjmp, las cuatro operaciones del límite por call_builtin y el salto desde una función del
# mismo fichero) para las funciones de samples/NatTryVm.bp: lo lanzan la native, un builtin, una función BP por el
# puente y otra native detrás de BP; un try dentro de otro, un catch que no es suyo, un `return` dentro, un try en un
# bucle y un throw desde un catch; GC en cada reserva. A -O0, -O2 y -Os: la salida del intérprete (12 de 12), 17
# llamadas a las natives y 13 aterrizajes (se usaron), y acaba en OK. Y un aterrizaje desparejado: su error, con nombre.
test-aottryvm: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatTryVm)
	$(call stdlibdep,Core)
	@for o in O0 O2 Os; do \
	  $(CC) $(CFLAGS) -$$o -Isrc test/test_aottryvm.c $(LIBOBJ) -o build/test_aottryvm_$$o.exe $(LDFLAGS) || exit 1; \
	 done
	@echo "--- #578 try dentro de una native, la VM (esperado: el interprete, 12/12, a -O0 -O2 -Os; 17 llamadas, 13 aterrizajes) ---"
	@./build/test_aottryvm_O2.exe samples/NatTryVm.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) ' > build/aottryvm.int; \
	 n=$$(grep -c '^ok ' build/aottryvm.int); \
	 if [ "$$n" -ne 12 ] || grep -q '^FALLO' build/aottryvm.int; then cat build/aottryvm.int; echo "  ROJO: $$n de 12 interpretado"; exit 1; fi; \
	 for o in O0 O2 Os; do \
	   ./build/test_aottryvm_$$o.exe samples/NatTryVm.mod 2>build/aottryvm.err | grep -a -E '^(ok|FALLO) ' > build/aottryvm.nat; \
	   if ! cmp -s build/aottryvm.int build/aottryvm.nat; then diff build/aottryvm.int build/aottryvm.nat; echo "  ROJO: -$$o no da lo que el interprete"; exit 1; fi; \
	   if ! grep -a -q '^\[nativas=17 aterrizajes=13\]' build/aottryvm.err; then grep -a '^\[nativas' build/aottryvm.err; echo "  ROJO: -$$o: las natives no se usaron o no aterrizaron (17 y 13)"; exit 1; fi; \
	   if ! grep -a -q '^\[status=OK\]' build/aottryvm.err; then grep -a '^\[status\|^\[runtime_error' build/aottryvm.err; echo "  ROJO: -$$o no acabo en OK"; exit 1; fi; \
	 done; \
	 ./build/test_aottryvm_O2.exe samples/NatTryVm.mod --desparejado 2>build/aottryvm.err >/dev/null; \
	 if ! grep -a -q 'se cierra uno que no es el mas interno' build/aottryvm.err; then grep -a '^\[status\|^\[runtime_error' build/aottryvm.err; echo "  ROJO: el aterrizaje desparejado no dio su error"; exit 1; fi; \
	 echo "[status=OK] 12/12 a -O0, -O2 y -Os, igual que interpretado; 17 llamadas y 13 aterrizajes; el desparejado, con su error"

# V7 #578 (9-oct) — EL `try` DENTRO DE UNA NATIVE, traducido por el AOT (AotCEmitter): samples/NatTry.bp —un throw de
# la native con un local cambiado, el mensaje leído en el catch, una clase del usuario por el puente, un catch que no
# es suyo, el finally en sus tres salidas, break y continue con finally (#673), un builtin, catch sin tipo y sin
# variable, un try en un bucle, lo que lanza un catch con finally, de una native a otra y relanzar con `throw e`—, con
# GC en cada reserva. Las 12 natives, todas traducidas; el mismo .mod con el AOT y sin él: la misma salida, entera
# (con los `print` de los finally, en su sitio), y acaba en OK.
test-aottry: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,NatTry)
	$(call stdlibdep,Core)
	$(call aotmain,NatTry,build/aottry)
	$(CC) $(CFLAGS) -Isrc test/test_aottry.c build/aottry/aot_NatTry.c $(LIBOBJ) \
	  -o build/test_aottry.exe $(LDFLAGS)
	@echo "--- #578 try dentro de una native, el AOT (esperado: 12 natives; AOT == interpretado, 13/13, sin FALLO) ---"
	@t=$$(grep -c 'bpvm_aot_register_by_name' build/aottry/aot_NatTry.c); \
	 if [ "$$t" -ne 12 ]; then echo "  ROJO: el AOT tradujo $$t de 12 natives"; exit 1; fi; \
	 ./build/test_aottry.exe samples/NatTry.mod 2>build/aottry.err | grep -a -E '^(ok|FALLO) |^  \(' > build/aottry.aot; \
	 ./build/test_aottry.exe samples/NatTry.mod --interp 2>/dev/null | grep -a -E '^(ok|FALLO) |^  \(' > build/aottry.int; \
	 n=$$(grep -c '^ok ' build/aottry.aot); \
	 if ! grep -a -q '^\[status=OK\]' build/aottry.err; then grep -a '^\[status\|^\[runtime_error' build/aottry.err; echo "  ROJO: el programa no acabo en OK con el AOT"; exit 1; fi; \
	 if grep -q '^FALLO' build/aottry.aot build/aottry.int; then grep -h '^FALLO' build/aottry.aot build/aottry.int; echo "  ROJO: FALLO"; exit 1; fi; \
	 if ! cmp -s build/aottry.int build/aottry.aot; then diff build/aottry.int build/aottry.aot; echo "  ROJO: el AOT no da lo que el interprete"; exit 1; fi; \
	 if [ "$$n" -ne 13 ]; then cat build/aottry.aot; echo "  ROJO: $$n de 13 comprobaciones"; exit 1; fi; \
	 echo "[status=OK] 13/13 con las 12 natives traducidas, igual que interpretado"

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
	@java -cp "../lexer-java/target/classes;../pack/target/classes;../miVM/target/classes" basicplus.frontend.AotMain ../samples/FloatNat.bp build/aotfloat >build/aotfloat/aot.log 2>&1 || { cat build/aotfloat/aot.log; exit 1; }
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

# V7/H10, el RUN común, paso 1 (3-oct) — LA SALIDA DEL PROGRAMA, COMÚN (wire_v1_salida): un NUL, 512 controles, un
# carácter partido entre dos llamadas, un carácter y un escape que no caben al final de una trama, un byte que no es
# UTF-8 y 10 KB en trozos raros; cada trama, JSON entero y UTF-8 válido, y se reconstruye lo impreso.
test-salida:
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_salida.c src/wire_v1_proto.c -o build/test_salida.exe $(LDFLAGS)
	@echo "--- el RUN comun, paso 1: la salida del programa, comun (esperado: solo 'ok', [status=OK]) ---"
	@./build/test_salida.exe

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

# V8/H5 (#711) — la rutina de validar y corregir UTF-8 (bpvm_internal.h) contra un ORÁCULO escrito
# de otra forma (la tabla 3-7 del estándar, fila a fila): todas las secuencias de 1 a 3 bytes y un
# millón al azar de 4 a 12, como Utf8Test de miVM. Host-only: la cabecera y, desde V8 #730, bpstr, que es quien lo hace.
test-utf8:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_utf8.c src/bpstr_vm.c -o build/test_utf8.exe
	@echo "--- V8/H5 #711 UTF-8: la rutina contra la tabla del estandar (esperado: solo 'ok', [status=OK]) ---"
	./build/test_utf8.exe

# V8 #739 (2-nov) — una cadena que nace empezando por un byte de continuacion, forzada a mano (ningun programa la crea):
# el aviso de su nacimiento, sus bytes, solo las tres primeras, y el cursor de las cadenas sin salirse de ella.
test-malnacida: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_malnacida.c $(LIBOBJ) -o build/test_malnacida.exe $(LDFLAGS)
	@echo "--- #739: la cadena mal nacida, el aviso y el cursor (esperado: solo 'ok', [status=OK]) ---"
	./build/test_malnacida.exe

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

# V7/H7 (Boot 2, fase 4) — el ARRANQUE COMUN: el orden, las puertas de cada capa (con el FS,
# la zona, /lib y montar la SD; con la app sana, el autorun) y las claves del ENV, con una
# familia de mentira que apunta lo que se le llama. bpvm_boot2.c es capa de placa, como
# bpvm_repl.c: no va en LIBOBJ; la SD de la placa (bpvm_sd_placa.c), de mentira en la prueba.
test-boot2: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_boot2.c src/bpvm_boot2.c $(LIBOBJ) -o build/test_boot2.exe $(LDFLAGS)
	@echo "--- V7/H7 fase 4 el arranque comun (esperado: solo 'ok', [status=OK]) ---"
	./build/test_boot2.exe

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

# #648 (4-oct) — el GC marca SIN recursión. Marcaba llamándose a sí mismo por cada referencia: una lista de N nodos
# bajaba N niveles de pila, y la Pico 2 (16 KB de pila, 32 B por nivel) colgaba con ~360 sin decir nada; en el PC no
# se veía, porque la pila de un hilo es de megas. Aquí, las DOS VMs con la pila acotada: la VM-C con --pila=65536 —la
# carga y el RUN en un hilo de 64 KB, como la tarea de la placa— y la VM-Java con -Xss256k. El sabotaje, medido el
# 4-oct: con la recursión, las dos caen antes de la primera línea (y la VM-C sin --pila pasa: eso escondía el PC).
test-gchondo: $(BIN)
	$(call fixture,GcHondo)
	$(call stdlibdep,Core)
	@echo "--- #648 el GC sin recursión: una lista de 4000 nodos con la pila acotada (esperado: las dos VMs hasta 'fin', con la misma salida) ---"
	@oc=$$(cd samples && timeout 60 ../$(BIN) --pila=65536 GcHondo.mod 2>/dev/null); stc=$$?; \
	 oj=$$(cd samples && timeout 60 java -Xss256k -jar ../../miVM/target/bpgenvm-1.0.jar GcHondo.mod 2>/dev/null); stj=$$?; \
	 pc=$$(printf '%s\n' "$$oc" | grep -E '^(honda|ancha|tras|fin)'); pj=$$(printf '%s\n' "$$oj" | grep -E '^(honda|ancha|tras|fin)'); \
	 printf '%s\n' "$$pc"; \
	 if [ $$stc -eq 0 ] && [ $$stj -eq 0 ] && printf '%s\n' "$$pc" | grep -q '^fin$$' && [ "$$pc" = "$$pj" ]; then \
	   echo "PASS test-gchondo"; \
	 else \
	   echo "FAIL test-gchondo (VM-C con 64 KB: salida $$stc; VM-Java con -Xss256k: salida $$stj)"; \
	   [ "$$pc" = "$$pj" ] || { echo "  la VM-Java dijo otra cosa:"; printf '%s\n' "$$pj" | sed 's/^/    /'; }; \
	   exit 1; \
	 fi

# #656 (7-oct) — SOLTAR UN OWNER SIN RECURSIÓN: una cadena de 4000 owners y un array owner de 600 con un hijo cada uno,
# con la pila acotada en las dos VMs (la VM-C en un hilo de 64 KB, como una tarea de placa; la VM-Java con -Xss256k):
# con la recursión de antes, la VM-C caía al soltar la cadena y la VM-Java, ya con su pila por defecto
# (StackOverflowError). Las dos hasta 'fin', con la misma salida —los testigos encuentran muertos los nodos del fondo:
# la cascada llegó—; y en la VM-C, el REPASO del heap tiene que haber corrido (el array ancho llena su pila de trabajo):
# un camino que nadie recorre no está probado.
test-ownerhondo: $(BIN)
	$(call fixture,OwnerHondo)
	$(call stdlibdep,Core)
	@echo "--- #656 soltar un owner sin recursión: 4000 en cadena y 600 en un array, con la pila acotada (esperado: las dos VMs hasta 'fin', con la misma salida, y el repaso en la VM-C) ---"
	@oc=$$(cd samples && timeout 60 ../$(BIN) --pila=65536 OwnerHondo.mod 2>build_ownerhondo.err); stc=$$?; \
	 oj=$$(cd samples && timeout 60 java -Xss256k -jar ../../miVM/target/bpgenvm-1.0.jar OwnerHondo.mod 2>/dev/null); stj=$$?; \
	 pc=$$(printf '%s\n' "$$oc" | grep -E '^(honda|ancha|otra|fin)'); pj=$$(printf '%s\n' "$$oj" | grep -E '^(honda|ancha|otra|fin)'); \
	 rep=$$(grep -ao 'soltar un owner: [0-9]* repaso' samples/build_ownerhondo.err | head -1); rm -f samples/build_ownerhondo.err; \
	 printf '%s\n' "$$pc"; \
	 if [ $$stc -eq 0 ] && [ $$stj -eq 0 ] && printf '%s\n' "$$pc" | grep -q '^fin$$' && [ "$$pc" = "$$pj" ] && [ -n "$$rep" ]; then \
	   echo "  ($$rep del heap en la VM-C)"; echo "PASS test-ownerhondo"; \
	 else \
	   echo "FAIL test-ownerhondo (VM-C con 64 KB: salida $$stc; VM-Java con -Xss256k: salida $$stj; repaso: '$$rep')"; \
	   [ "$$pc" = "$$pj" ] || { echo "  la VM-Java dijo otra cosa:"; printf '%s\n' "$$pj" | sed 's/^/    /'; }; \
	   exit 1; \
	 fi

# #652 (7-oct) — TODO PROGRAMA LLEVA CORE (Eduardo: «no puede haber programas sin Core»): un .mod de la cápsula de V6
# que no lo importa (Hello: no usa nada de él) se cargaba SIN Core en las dos VMs («enlace: 1 modulos»; la VM-C, «OOM
# sin prefabricar»: sin RuntimeError con el que construir una excepción). Ahora el cargador lo trae (discover_deps,
# con_core; asegurarCore en miVM): las dos, «enlace: 2 modulos».
test-coresiempre: $(BIN)
	@echo "--- #652 todo programa lleva Core: el Hello de la capsula de V6, que no lo importa (esperado: las dos VMs lo enlazan con Core) ---"
	@d=build/coresiempre; rm -rf $$d; mkdir -p $$d; cp ../compat/v6/golden/Hello.mod ../bpstdlib/Core.mod $$d/; \
	 ec=$$(cd $$d && timeout 30 ../../$(BIN) Hello.mod 2>&1 >/dev/null); \
	 ej=$$(cd $$d && timeout 30 java -jar ../../../miVM/target/bpgenvm-1.0.jar Hello.mod 2>&1 >/dev/null); \
	 if printf '%s\n' "$$ec" | grep -q 'enlace: 2 modulos' && ! printf '%s\n' "$$ec" | grep -q 'OOM sin prefabricar' \
	    && printf '%s\n' "$$ej" | grep -q 'enlace: 2 modulos'; then \
	   echo "  las dos VMs: enlace de 2 modulos (Hello y Core)"; echo "PASS test-coresiempre"; \
	 else \
	   echo "FAIL test-coresiempre"; printf '%s\n' "$$ec" | grep -E 'enlace|OOM|Core' | sed 's/^/  VM-C: /'; \
	   printf '%s\n' "$$ej" | grep -E 'enlace|Core' | sed 's/^/  VM-Java: /'; exit 1; \
	 fi

# #664 (7-oct) — UNA LLAMADA ANIDADA LARGA, CON DOS WORKERS: Gui.__guiInvokeBySlot corre el método DENTRO del builtin,
# y la VM-Java corría UN cuanto (1024 instrucciones, #531) y abandonaba el resto en silencio —y el GC de otro worker la
# abandonaba igual—. InvocaRanura (también en la paridad): la VM-C con dos runs y la VM-Java con dos workers, la salida
# de la VM-C con uno; y en la VM-Java el APARCAMIENTO tiene que haber ocurrido (el hilo que reserva sin parar para el
# mundo en mitad de la llamada): un camino que nadie recorre no está probado. Sin aparcarse, la VM-Java se cuelga.
test-anidadalarga: $(BIN)
	$(call fixture,InvocaRanura)
	$(call stdlibdep,Core Gui Json Collections Str)
	@echo "--- #664 una llamada anidada larga con dos workers (esperado: las dos VMs, la salida de la VM-C con un run; y en la VM-Java, aparcada para el GC) ---"
	@o1=$$(cd samples && timeout 60 ../$(BIN) InvocaRanura.mod 2>/dev/null); st1=$$?; \
	 oc=$$(cd samples && timeout 60 ../$(BIN) --smp=2 InvocaRanura.mod 2>/dev/null); stc=$$?; \
	 oj=$$(cd samples && timeout 60 java -Djava.awt.headless=true -jar ../../miVM/target/bpgenvm-1.0.jar --workers=2 InvocaRanura.mod 2>build_anidada.err); stj=$$?; \
	 ap=$$(grep -ao 'aparcadas por el GC de otro worker: [0-9]*' samples/build_anidada.err | grep -o '[0-9]*$$'); rm -f samples/build_anidada.err; \
	 p1=$$(printf '%s\n' "$$o1" | grep -E '^(largo|heredado|fin)'); pc=$$(printf '%s\n' "$$oc" | grep -E '^(largo|heredado|fin)'); \
	 pj=$$(printf '%s\n' "$$oj" | grep -E '^(largo|heredado|fin)'); \
	 printf '%s\n' "$$p1"; \
	 if [ $$st1 -eq 0 ] && [ $$stc -eq 0 ] && [ $$stj -eq 0 ] && printf '%s\n' "$$p1" | grep -q '^fin$$' \
	    && [ "$$pc" = "$$p1" ] && [ "$$pj" = "$$p1" ] && [ -n "$$ap" ] && [ "$$ap" -gt 0 ]; then \
	   echo "  (la VM-Java, aparcada $$ap veces para el GC del otro worker)"; echo "PASS test-anidadalarga"; \
	 else \
	   echo "FAIL test-anidadalarga (VM-C con un run: salida $$st1; con dos: $$stc; VM-Java con dos workers: $$stj; aparcadas: '$$ap')"; \
	   [ "$$pc" = "$$p1" ] || { echo "  la VM-C con dos runs dijo otra cosa:"; printf '%s\n' "$$pc" | sed 's/^/    /'; }; \
	   [ "$$pj" = "$$p1" ] || { echo "  la VM-Java dijo otra cosa:"; printf '%s\n' "$$pj" | sed 's/^/    /'; }; \
	   exit 1; \
	 fi

# #659 (7-oct) — UNA TRAZA DE BUS ES UNA LÍNEA, también con dos runs: el stub del PC escribía cada traza a trozos (un
# bpvm_out por byte), y con dos hilos en dos buses las piezas de una línea se colaban entre las de otra en el stdout —el
# oráculo de la paridad— (10 de 10 pasadas con líneas mezcladas). BusDosHilos (también en la paridad: tres hilos, SPI,
# I2C y UART, las cinco trazas de varias piezas): CINCO pasadas con dos runs, y todas, ordenadas, lo de un run. Una sola
# pasada con el arreglo quitado sale bien 1 de cada 20 veces: con cinco, el sabotaje no se escapa.
test-trazasbus: $(BIN)
	$(call fixture,BusDosHilos)
	$(call stdlibdep,Core Spi I2c Uart Gpio Machine)
	@echo "--- #659 las trazas de bus, enteras con dos runs (esperado: cinco pasadas con dos runs, ordenadas, iguales a la de un run) ---"
	@# V8 #769 (3-nov): la primera pasada distinta, guardada (build/trazasbus.diff) y su principio en el FAIL: una vez
	@# salió en rojo y no se supo qué traza era la distinta.
	@mkdir -p build; rm -f build/trazasbus.diff; \
	 ref=$$(cd samples && timeout 60 ../$(BIN) BusDosHilos.mod 2>/dev/null | grep -E '^\[(spi|i2c|uart)\]' | sort); \
	 n=$$(printf '%s\n' "$$ref" | grep -c ''); malas=0; \
	 for i in 1 2 3 4 5; do \
	   o=$$(cd samples && timeout 60 ../$(BIN) --smp=2 BusDosHilos.mod 2>/dev/null | grep -E '^\[(spi|i2c|uart)\]' | sort); \
	   if [ "$$o" != "$$ref" ]; then malas=$$((malas+1)); \
	     if [ ! -f build/trazasbus.diff ]; then printf '%s\n' "$$ref" > build/trazasbus.ref; \
	       printf '%s\n' "$$o" > build/trazasbus.dist; diff build/trazasbus.ref build/trazasbus.dist > build/trazasbus.diff; fi; \
	   fi; \
	 done; \
	 if [ $$n -ge 300 ] && [ $$malas -eq 0 ]; then \
	   echo "  $$n trazas, enteras en las cinco pasadas con dos runs"; echo "PASS test-trazasbus"; \
	 else \
	   echo "FAIL test-trazasbus ($$n trazas con un run; $$malas de 5 pasadas con dos runs, distintas; la primera, en build/trazasbus.diff)"; \
	   if [ -f build/trazasbus.diff ]; then head -12 build/trazasbus.diff | sed 's/^/  /'; fi; exit 1; \
	 fi

# #659 (7-oct) — UN RUN TRAS OTRO EMPIEZA DE CERO: en una placa o en el micro simulado un proceso corre un RUN tras otro,
# y valían por PROCESO la rampa del ADC del stub (el segundo RUN leía otros valores; la de la VM-Java empieza con cada
# VM), el aviso de «sin backend de ADC» (sólo en el primero) y los avisos de un `throw` (los cinco primeros de toda la
# sesión). RunTrasRun, dos veces en el mismo proceso: lo mismo, y los avisos en las dos. Ver test/test_runtrasrun.c.
RUNTRAS_BIN = build/test_runtrasrun.exe
test-runtrasrun: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,RunTrasRun)
	$(call stdlibdep,Core Adc)
	$(CC) $(CFLAGS) -Isrc test/test_runtrasrun.c $(LIBOBJ) -o $(RUNTRAS_BIN) $(LDFLAGS)
	@echo "--- #659 un RUN tras otro empieza de cero (esperado: los seis 'ok' y la linea OK) ---"
	cd samples && ../$(RUNTRAS_BIN) RunTrasRun.mod

# #653 (4-oct) — EL LOG CON DOS ESCRITORES: mirar si la línea cabe y copiarla no se pueden separar (la sección crítica
# de la cintura). Dos hilos escriben sin parar en una región de 512 B con una guarda detrás: con la sección, intacta;
# sin ella (--sin-seccion, el sabotaje: el log de antes), pisada —rojo—. Ver test/test_logmt.c.
# Y #658 (5-oct): con dos hilos más que vuelcan sin parar a una flash de mentira (grabar encima deja el AND), ningún
# volcado encima de otro y todas las imágenes buenas; con el volcado de antes (--volcado --de-antes), rojo.
LOGMT_BIN = build/test_logmt.exe
test-logmt: $(LIBOBJ)
	@mkdir -p build
	@$(CC) $(CFLAGS) -Isrc test/test_logmt.c $(LIBOBJ) -o $(LOGMT_BIN) $(LDFLAGS)
	@echo "--- #653: el log con dos escritores (esperado: PASS y la linea OK) ---"
	@timeout 120 ./$(LOGMT_BIN) || { echo "FAIL test-logmt"; exit 1; }
	@echo "--- y su sabotaje (sin la seccion: esperado ROJO) ---"
	@if timeout 120 ./$(LOGMT_BIN) --sin-seccion; then echo "FAIL test-logmt: sin la seccion pasa por verde"; exit 1; \
	 else echo "  (rojo, como debe: sin la seccion, el log se sale de su region)"; fi
	@echo "--- #658: el volcado con dos a la vez (esperado: PASS y la linea OK) ---"
	@timeout 120 ./$(LOGMT_BIN) --volcado || { echo "FAIL test-logmt (#658)"; exit 1; }
	@echo "--- y su sabotaje (el volcado de antes: esperado ROJO) ---"
	@if timeout 120 ./$(LOGMT_BIN) --volcado --de-antes; then echo "FAIL test-logmt: el volcado de antes pasa por verde"; exit 1; \
	 else echo "  (rojo, como debe: el volcado de antes graba encima de otro o pisa lo que se anade)"; fi
	@echo "PASS test-logmt (y sus dos sabotajes, rojos)"

# #355, V7/H12 2b (5-oct) — LA RESERVA DE EMERGENCIA la gasta SÓLO una reserva para construir un error (test/
# test_reserva.c): con el heap lleno, las normales no la tocan; el objeto y el mensaje del error, sí, y caben. Antes lo
# decía una bandera de la VM, que con dos runs veía también el otro. Y su sabotaje (el objeto del error por el camino
# normal), que tiene que dar rojo.
RESERVA_BIN = build/test_reserva.exe
test-reserva: $(LIBOBJ)
	@mkdir -p build
	@$(CC) $(CFLAGS) -Isrc test/test_reserva.c $(LIBOBJ) -o $(RESERVA_BIN) $(LDFLAGS)
	@echo "--- #355: la reserva de emergencia, solo para construir un error (esperado: todo ok y la linea OK) ---"
	@timeout 60 ./$(RESERVA_BIN) 2>/dev/null || { echo "FAIL test-reserva"; exit 1; }
	@echo "--- y su sabotaje (el error por el camino normal: esperado ROJO) ---"
	@if timeout 60 ./$(RESERVA_BIN) --sabotaje >build/reserva_sab.out 2>/dev/null; then \
	   echo "FAIL test-reserva: el sabotaje pasa por verde"; cat build/reserva_sab.out; exit 1; \
	 else echo "  (rojo, como debe: por el camino normal, el error no cabe con el heap lleno)"; fi
	@echo "PASS test-reserva (y su sabotaje, rojo)"

# V7/H12 (5-oct), #660 y #661 — EL CAMINO FINO: las tres piezas que impiden que un programa que comparte memoria sin
# Mutex rompa la VM (V7_IDEAS §H12 «El camino fino»): (a) las palabras alineadas, de una vez (dos hilos de verdad:
# uno escribe, otro lee, y ninguna a medias); (b) con varios runs, el número de un handle soltado, de vuelta en el GC;
# (c) la referencia que no puede existir, muerta. Un sabotaje por pieza, cada uno ROJO.
PALABRAS_BIN = build/test_palabras.exe
test-palabras: $(LIBOBJ)
	@mkdir -p build
	@$(CC) $(CFLAGS) -Isrc test/test_palabras.c $(LIBOBJ) -o $(PALABRAS_BIN) $(LDFLAGS)
	@echo "--- #660/#661: el camino fino (esperado: todo ok y la linea OK) ---"
	@timeout 120 ./$(PALABRAS_BIN) 2>/dev/null || { echo "FAIL test-palabras"; exit 1; }
	@echo "--- y sus tres sabotajes (esperado: los tres ROJOS) ---"
	@for s in bytes reciclar desgarro; do \
	   if timeout 120 ./$(PALABRAS_BIN) --sabotaje-$$s >build/palabras_sab.out 2>/dev/null; then \
	     echo "FAIL test-palabras: el sabotaje --sabotaje-$$s pasa por verde"; cat build/palabras_sab.out; exit 1; \
	   else echo "  (rojo, como debe: --sabotaje-$$s: $$(grep -c '^FALLO' build/palabras_sab.out) comprobacion(es) mal)"; fi; \
	 done
	@echo "PASS test-palabras (y sus tres sabotajes, rojos)"

# V7/H12, etapa 2b (5-oct) — LA VARIABLE DE CONDICIÓN DE FREERTOS (src/platform_freertos_cond.inc, la de las tres
# familias), con el kernel de FreeRTOS DE VERDAD en el PC: su port de Windows (portable/MSVC-MingW), que no es el de
# pthreads —ése no pierde avisos—. test/test_frcond.c: broadcast con dos esperando, un aviso que no se lleva otro, el
# plazo, el estrés con y sin plazo y ninguna ficha suelta. Con la condición de ANTES como control (--sabotaje), que
# tiene que dar rojo. El kernel, el de la Pico y la STM32 (FreeRTOS-LTS), junto al repo.
FREERTOS_KERNEL ?= ../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel
FRCOND_BIN = build/test_frcond.exe
FRCOND_INC = -Itest/frcond -I$(FREERTOS_KERNEL)/include -I$(FREERTOS_KERNEL)/portable/MSVC-MingW
test-frcond:
	@if [ ! -f $(FREERTOS_KERNEL)/tasks.c ]; then \
	   echo "FAIL test-frcond: no esta el kernel de FreeRTOS en $(FREERTOS_KERNEL) (FREERTOS_KERNEL=...)"; exit 1; fi
	@mkdir -p build/frcond
	@for f in tasks queue list; do $(CC) -O1 -w $(FRCOND_INC) -c $(FREERTOS_KERNEL)/$$f.c -o build/frcond/$$f.o || exit 1; done
	@$(CC) -O1 -w $(FRCOND_INC) -c $(FREERTOS_KERNEL)/portable/MSVC-MingW/port.c -o build/frcond/port.o
	@$(CC) -O1 -w $(FRCOND_INC) -c $(FREERTOS_KERNEL)/portable/MemMang/heap_4.c -o build/frcond/heap_4.o
	@$(CC) -O1 -Wall -Wextra -Werror $(FRCOND_INC) -Iinclude -Isrc -c test/test_frcond.c -o build/frcond/test_frcond.o
	@$(CC) build/frcond/test_frcond.o build/frcond/tasks.o build/frcond/queue.o build/frcond/list.o \
	   build/frcond/port.o build/frcond/heap_4.o -o $(FRCOND_BIN) -lwinmm
	@echo "--- H12 2b: la condicion de FreeRTOS, con el kernel de verdad (esperado: 1 a 5b PASS y la linea OK) ---"
	@timeout 120 ./$(FRCOND_BIN) || { echo "FAIL test-frcond"; exit 1; }
	@echo "--- y su control: la condicion de ANTES (esperado ROJO: 1, 2 y 4b) ---"
	@timeout 120 ./$(FRCOND_BIN) --sabotaje >build/frcond_sab.out; st=$$?; cat build/frcond_sab.out; \
	 if [ $$st -ne 0 ] && grep -q '^\[frcond\] 1\..*FAIL' build/frcond_sab.out && grep -q '^\[frcond\] 2\..*FAIL' build/frcond_sab.out; then \
	   echo "  (rojo, como debe: la de antes pierde avisos y deja que otro se lleve el de uno)"; \
	 else echo "FAIL test-frcond: la condicion de antes pasa por verde (salida $$st)"; exit 1; fi
	@echo "PASS test-frcond (y su control, rojo)"

# V7/H12, etapa 2a.2 (4-oct) — LA COLA y su planificador, en lo que la red no ve por sí sola (test/test_cola.c): el
# interbloqueo de verdad con uno y dos runs (Interbloqueo: RUNTIME_ERROR; con dos, antes de la 2a.2, colgado); el
# interbloqueo FALSO que no puede salir (MutexGc con dos runs, el heap justo y la ventana ensanchada); y el fin, del
# último run que suelta su hilo (FinJunto con dos runs y la ventana ensanchada); y que al avisar a quien lo colgó no
# quede ningún run dentro (la revisión de la 2a.2). Y sus tres sabotajes, que tienen que dar rojo: --sabotaje-contador
# (algún interbloqueo falso), --sabotaje-fin (algún fin antes del último cuanto) y --sabotaje-salida (algún run dentro).
# La 2b (5-oct), con dos runs: el gc() del programa con el mundo parado (GcConcurrente), los eventos (EvDosRuns,
# EvHiloMuere) y los ficheros abiertos (FicherosDosRuns); con sus sabotajes: --sabotaje-gc, --sabotaje-eventos,
# --sabotaje-revive y --sabotaje-ficheros. Y lo de la revisión de la 2b (las partes 9 a 15): el owner en carrera, las
# líneas que no caben en la cola, el join y la última línea, la zona de rascar, el gc() tras un mundo parado ajeno,
# el error del programa y un File compartido; con --sabotaje-owner, -lineas, -suyo, -zona, -colecta y -error.
COLA_BIN = build/test_cola.exe
test-cola: $(LIBOBJ)
	@mkdir -p build
	$(call fixture,Interbloqueo MutexGc FinJunto GcConcurrente EvDosRuns EvHiloMuere FicherosDosRuns OwnerCarrera LineasLargas JoinLinea ListDirDos ColectaAjena ErrorPrimero FileCompartido UnHiloCalcula CadenasUnHilo GuiDosRuns GuiAnidadaGc GuiMismoPadre ReservaAjena)
	$(call stdlibdep,Core Gui Json Collections Str)
	@if [ ! -f samples/Interbloqueo.mod ] || [ ! -f samples/MutexGc.mod ] || [ ! -f samples/FinJunto.mod ] || [ ! -f samples/GcConcurrente.mod ] || [ ! -f samples/EvDosRuns.mod ] || [ ! -f samples/EvHiloMuere.mod ] || [ ! -f samples/FicherosDosRuns.mod ] || [ ! -f samples/OwnerCarrera.mod ] || [ ! -f samples/LineasLargas.mod ] || [ ! -f samples/JoinLinea.mod ] || [ ! -f samples/ListDirDos.mod ] || [ ! -f samples/ColectaAjena.mod ] || [ ! -f samples/ErrorPrimero.mod ] || [ ! -f samples/FileCompartido.mod ] || [ ! -f samples/UnHiloCalcula.mod ] || [ ! -f samples/CadenasUnHilo.mod ] || [ ! -f samples/GuiDosRuns.mod ] || [ ! -f samples/GuiAnidadaGc.mod ] || [ ! -f samples/GuiMismoPadre.mod ] || [ ! -f samples/ReservaAjena.mod ]; then \
	  echo "FAIL test-cola: faltan los .mod de la prueba (Interbloqueo, MutexGc, FinJunto, GcConcurrente, EvDosRuns, EvHiloMuere, FicherosDosRuns y los de la revisión: OwnerCarrera, LineasLargas, JoinLinea, ListDirDos, ColectaAjena, ErrorPrimero, FileCompartido; y UnHiloCalcula, de #662; y CadenasUnHilo, de la 3.2; y GuiDosRuns y GuiAnidadaGc, de LVGL con dos runs) y no se han podido generar"; exit 1; \
	fi
	@# V7/H12, LVGL con dos runs: la lista de los builtins de la GUI que sólo ejecuta el run 0 (builtins.c,
	@# bpvm_builtin_de_la_gui) tiene TODOS los BUILTIN_GUI_* de builtin_ids.h: uno nuevo que se olvide ahí tocaría LVGL
	@# desde otro run.
	@faltan=$$(for n in $$(grep -o 'BUILTIN_GUI_[A-Z0-9_]*' src/builtin_ids.h | sort -u); do \
	   awk '/^int bpvm_builtin_de_la_gui/,/^}/' src/builtins.c | grep -qw "$$n" || echo "$$n"; done); \
	 if [ -n "$$faltan" ]; then echo "FAIL test-cola: builtins de la GUI que no viajan al run 0 (bpvm_builtin_de_la_gui): $$faltan"; exit 1; \
	 else echo "--- los $$(grep -o 'BUILTIN_GUI_[A-Z0-9_]*' src/builtin_ids.h | sort -u | wc -l) builtins de la GUI, todos en la lista del run 0 ---"; fi
	@$(CC) $(CFLAGS) -Isrc test/test_cola.c $(LIBOBJ) -o $(COLA_BIN) $(LDFLAGS)
	@echo "--- H12 2a.2: la cola (esperado: 1. dos PASS, 2. a 21. PASS y la linea OK) ---"
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod 2>/dev/null) || { echo "FAIL test-cola"; exit 1; }
	@echo "--- y sus sabotajes (esperado: los dieciocho, ROJOS, cada uno por lo suyo) ---"
	@# El del contador: un interbloqueo FALSO dicho en stderr (detrás, el programa se descuelga con un run aún dentro
	@# de su cuanto, que al despertar ya no lo encuentra: suele acabar en un fallo de segmento; por eso se mira el aviso).
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-contador \
	   >../build/cola_sab1.out 2>../build/cola_sab1.err); st=$$?; \
	 if [ $$st -ne 0 ] && grep -q 'deadlock' build/cola_sab1.err; then \
	   echo "  (rojo, como debe: con el contador del GC sale un interbloqueo que no existe; salida $$st)"; \
	 else echo "FAIL test-cola: el sabotaje del contador no se caza (salida $$st)"; cat build/cola_sab1.out; exit 1; fi
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-fin \
	   >../build/cola_sab2.out 2>/dev/null); st=$$?; cat build/cola_sab2.out; \
	 if [ $$st -ne 0 ] && grep -q 'SABOTEADO.*FAIL' build/cola_sab2.out; then \
	   echo "  (rojo, como debe: el fin, decidido antes de que vuelva el último cuanto)"; \
	 else echo "FAIL test-cola: el sabotaje del fin no se caza (salida $$st)"; exit 1; fi
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-salida \
	   >../build/cola_sab3.out 2>/dev/null); st=$$?; cat build/cola_sab3.out; \
	 if [ $$st -ne 0 ] && grep -q 'SABOTEADO.*FAIL' build/cola_sab3.out; then \
	   echo "  (rojo, como debe: el aviso llega con otro run todavía dentro del programa)"; \
	 else echo "FAIL test-cola: el sabotaje de la salida no se caza (salida $$st)"; exit 1; fi
	@# El del gc(): con el camino de antes, colectas con el otro run dentro de su cuanto (la regla, contada por
	@# heap.c), o el proceso muerto por lo que eso rompe (salida 128 o mas).
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-gc \
	   >../build/cola_sab4.out 2>/dev/null); st=$$?; cat build/cola_sab4.out; \
	 if [ $$st -ne 0 ] && { grep -q 'SABOTEADO.*FAIL' build/cola_sab4.out || [ $$st -ge 128 ]; }; then \
	   echo "  (rojo, como debe: el gc() de antes colecta con el otro run ejecutando; salida $$st)"; \
	 else echo "FAIL test-cola: el sabotaje del gc() no se caza (salida $$st)"; exit 1; fi
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-eventos \
	   >../build/cola_sab5.out 2>/dev/null); st=$$?; cat build/cola_sab5.out; \
	 if [ $$st -ne 0 ] && grep -q 'SABOTEADO.*FAIL' build/cola_sab5.out; then \
	   echo "  (rojo, como debe: sin el cerrojo, dos raise a la vez pierden o pisan eventos)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-eventos no se caza (salida $$st)"; exit 1; fi
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-revive \
	   >../build/cola_sab6.out 2>/dev/null); st=$$?; cat build/cola_sab6.out; \
	 if [ $$st -ne 0 ] && grep -q 'SABOTEADO.*FAIL' build/cola_sab6.out; then \
	   echo "  (rojo, como debe: resucitado con otro run encima, el hilo que muere corta el programa)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-revive no se caza (salida $$st)"; exit 1; fi
	@# El de los ficheros: sin el cerrojo, los dos hilos ven libre el fichero y lo abren a la vez (choques), o se
	@# pisan la lista de abiertos (el proceso, muerto: salida 128 o mas).
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-ficheros \
	   >../build/cola_sab7.out 2>/dev/null); st=$$?; cat build/cola_sab7.out; rm -f samples/comun_2b.txt samples/compartido_2b.txt; \
	 if [ $$st -ne 0 ] && { grep -q 'SABOTEADO.*FAIL' build/cola_sab7.out || [ $$st -ne 1 ]; }; then \
	   echo "  (rojo, como debe: sin el cerrojo, los dos abren el mismo fichero para escribir; salida $$st)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-ficheros no se caza (salida $$st)"; exit 1; fi
	@# La revisión de la 2b (5-oct): el owner soltado dos veces puede dejar la reserva dando vueltas con el
	@# cerrojo cogido (el proceso, colgado: el plazo lo para, salida 124) o romperlo (128 o más).
	@(cd samples && timeout 90 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-owner \
	   >../build/cola_sab8.out 2>/dev/null); st=$$?; cat build/cola_sab8.out; \
	 if [ $$st -ne 0 ] && { grep -q 'SABOTEADO.*FAIL' build/cola_sab8.out || [ $$st -ne 1 ]; }; then \
	   echo "  (rojo, como debe: dos hilos sueltan a la vez el mismo owner: el heap, roto o colgado; salida $$st)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-owner no se caza (salida $$st)"; exit 1; fi
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-lineas \
	   >../build/cola_sab9.out 2>/dev/null); st=$$?; cat build/cola_sab9.out; \
	 if [ $$st -ne 0 ] && { grep -q 'SABOTEADO.*FAIL' build/cola_sab9.out || [ $$st -ne 1 ]; }; then \
	   echo "  (rojo, como debe: sin esperar al que escribe a trozos, una línea larga sale partida; salida $$st)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-lineas no se caza (salida $$st)"; exit 1; fi
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-suyo \
	   >../build/cola_sab10.out 2>/dev/null); st=$$?; cat build/cola_sab10.out; \
	 if [ $$st -ne 0 ] && { grep -q 'SABOTEADO.*FAIL' build/cola_sab10.out || [ $$st -ne 1 ]; }; then \
	   echo "  (rojo, como debe: el join se adelanta a la última línea del hilo; salida $$st)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-suyo no se caza (salida $$st)"; exit 1; fi
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-zona \
	   >../build/cola_sab11.out 2>/dev/null); st=$$?; cat build/cola_sab11.out; \
	 if [ $$st -ne 0 ] && { grep -q 'SABOTEADO.*FAIL' build/cola_sab11.out || [ $$st -ne 1 ]; }; then \
	   echo "  (rojo, como debe: la zona de rascar del otro run es un error en un programa correcto; salida $$st)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-zona no se caza (salida $$st)"; exit 1; fi
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-colecta \
	   >../build/cola_sab12.out 2>/dev/null); st=$$?; cat build/cola_sab12.out; \
	 if [ $$st -ne 0 ] && { grep -q 'SABOTEADO.*FAIL' build/cola_sab12.out || [ $$st -ne 1 ]; }; then \
	   echo "  (rojo, como debe: un gc() vuelve sin colecta tras un mundo parado ajeno; salida $$st)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-colecta no se caza (salida $$st)"; exit 1; fi
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-error \
	   >../build/cola_sab13.out 2>/dev/null); st=$$?; cat build/cola_sab13.out; \
	 if [ $$st -ne 0 ] && { grep -q 'SABOTEADO.*FAIL' build/cola_sab13.out || [ $$st -ne 1 ]; }; then \
	   echo "  (rojo, como debe: el EXITED dice el error de una excepción atrapada; salida $$st)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-error no se caza (salida $$st)"; exit 1; fi
	@# #662: avisar a la cola tras CADA cuanto, como antes: el run ocioso se despierta en vano cientos de veces.
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-aviso \
	   >../build/cola_sab14.out 2>/dev/null); st=$$?; cat build/cola_sab14.out; \
	 if [ $$st -ne 0 ] && grep -q 'SABOTEADO.*FAIL' build/cola_sab14.out; then \
	   echo "  (rojo, como debe: con un hilo, el otro run se despierta en vano tras cada cuanto)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-aviso no se caza (salida $$st)"; exit 1; fi
	@# V7/H12, la 3.2: el mundo parado avisa también a la cola, como antes: cada colecta despierta al run ocioso.
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-mundo \
	   >../build/cola_sab15.out 2>/dev/null); st=$$?; cat build/cola_sab15.out; \
	 if [ $$st -ne 0 ] && grep -q 'SABOTEADO.*FAIL' build/cola_sab15.out; then \
	   echo "  (rojo, como debe: con un hilo que reserva, cada colecta despierta al otro run en vano)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-mundo no se caza (salida $$st)"; exit 1; fi
	@# V7/H12, LVGL con dos runs: el builtin de la GUI se ejecuta en el run que lo encuentra, como antes: LVGL y gui.c,
	@# tocados desde el run 1.
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-gui \
	   >../build/cola_sab16.out 2>/dev/null); st=$$?; cat build/cola_sab16.out; \
	 if [ $$st -ne 0 ] && grep -q 'SABOTEADO.*FAIL' build/cola_sab16.out; then \
	   echo "  (rojo, como debe: sin el viaje, builtins de la GUI ejecutados en el run 1)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-gui no se caza (salida $$st)"; exit 1; fi
	@# V7/H12: el puente (una llamada anidada de la GUI) vuelve a entrar sin fin con el mundo parado, como antes: el
	@# programa se queda colgado —el run 0 gira, el colector del otro lo espera— y lo para el plazo (salida 124).
	@(cd samples && timeout 30 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-anidada \
	   >../build/cola_sab17.out 2>/dev/null); st=$$?; cat build/cola_sab17.out; \
	 if [ $$st -eq 124 ] || { [ $$st -ne 0 ] && grep -q 'SABOTEADO.*FAIL' build/cola_sab17.out; }; then \
	   echo "  (rojo, como debe: sin aparcarse, el puente gira con el mundo parado y el programa se cuelga; salida $$st)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-anidada no se caza (salida $$st)"; exit 1; fi
	@# V8 #762: la reserva se rinde tras la colecta de otro, como antes: «sin sitio» con el heap lleno de basura.
	@(cd samples && timeout 240 ../$(COLA_BIN) Interbloqueo.mod MutexGc.mod FinJunto.mod GcConcurrente.mod EvDosRuns.mod EvHiloMuere.mod FicherosDosRuns.mod --sabotaje-reserva \
	   >../build/cola_sab18.out 2>/dev/null); st=$$?; cat build/cola_sab18.out; \
	 if [ $$st -ne 0 ] && grep -q 'SABOTEADO.*FAIL' build/cola_sab18.out; then \
	   echo "  (rojo, como debe: sin una colecta propia, un No space in heap falso)"; \
	 else echo "FAIL test-cola: el sabotaje --sabotaje-reserva no se caza (salida $$st)"; exit 1; fi
	@echo "PASS test-cola (y sus dieciocho sabotajes, rojos)"

# V7/H12, etapa 2 (4-oct) — LA COLA DE HILOS: cada hilo, una struct pedida al arrancarlo y devuelta al acabar (sin
# array y sin tope; el diseño, de Eduardo). HilosMuchos arranca 120 hilos —40 de uno en uno, 40 a la vez esperando en
# un Mutex del main, otros 40 de uno en uno— y se exige que, a la vez, no haya habido más de 41 (los 40 y el main): si
# un hilo acabado no saliera de la cola, serían 121. El sabotaje (la recogida apagada), medido el 4-oct: rojo.
test-hilos: $(BIN)
	$(call fixture,HilosMuchos)
	$(call stdlibdep,Core)
	@echo "--- H12 etapa 2: la cola de hilos decrece (esperado: los 40 unidos, 120 arrancados y como mucho 41 a la vez) ---"
	@out=$$(cd samples && timeout 60 ../$(BIN) HilosMuchos.mod 2>build_hilos.err); st=$$?; \
	 err=$$(cat samples/build_hilos.err); rm -f samples/build_hilos.err; \
	 printf '%s\n' "$$out" | grep -vE 'INICIANDO|FIN DE'; printf '%s\n' "$$err" | grep -a 'hilos:'; \
	 if [ $$st -eq 0 ] && printf '%s\n' "$$out" | grep -q 'unidos:  40' \
	    && printf '%s\n' "$$err" | grep -q 'hilos: 120 arrancados, como mucho 41 a la vez'; then \
	   echo "PASS test-hilos"; \
	 else echo "FAIL test-hilos (salida $$st)"; exit 1; fi

# #649 (4-oct) — la CARGA resuelve los imports SIN recursión. discover_deps se llamaba a sí misma por cada módulo que
# cargaba, con ~1,7 KB de marco en la placa (sus rutas, en la pila): la Pico 2 gastaba ~12 KB de los 16 de la tarea del
# REPL cargando TimeDemo, y una cadena de 16 módulos (el tope) pedía ~27. Aquí, ImportHondo —una cadena de 13 imports:
# 15 módulos con Core— con la MARCA DE AGUA de --pila (el hilo pinta su pila y mira hasta dónde se borró). Reventar no
# sirve de prueba: en Windows la reserva se redondea a 64 KB y la recursión, con el tope de módulos, no llega. Se exige
# que la cadena no cueste más de 8 KB de pila que `hello`, un módulo. Medido el 4-oct en el PC: la recursión, 26,7 KB
# más (el sabotaje: rojo); el recorrido nuevo, 2,9.
test-importhondo: $(BIN)
	$(call fixture,hello)
	$(call stdlibdep,Core)
	@rm -rf build/importhondo && mkdir -p build/importhondo && cp ../bpstdlib/Core.mod build/importhondo/
	@java -jar ../lexer-java/target/basicplus-frontend.jar samples/importhondo/ImportHondo.bp --compile build/importhondo --backend=mivm >build/importhondo/compila.log 2>&1; \
	 if [ ! -f build/importhondo/ImportHondo.mod ] || [ ! -f build/importhondo/Eslabon13.mod ]; then \
	   echo "  ✗ ImportHondo no compila:"; tail -5 build/importhondo/compila.log; exit 1; fi
	@echo "--- #649 la carga sin recursión: una cadena de 13 imports no cuesta más pila que un módulo (esperado: hasta 'fin', y menos de 8 KB más que hello) ---"
	@(cd build/importhondo && timeout 60 ../../$(BIN) --pila=131072 ImportHondo.mod >salida.txt 2>pila.txt); st=$$?; \
	 (cd samples && timeout 60 ../$(BIN) --pila=131072 hello.mod >/dev/null 2>../build/importhondo/pila_hello.txt); sth=$$?; \
	 u=$$(sed -n 's/^\[pila\] el hilo de --pila=[0-9]* uso \([0-9]*\) B$$/\1/p' build/importhondo/pila.txt); \
	 h=$$(sed -n 's/^\[pila\] el hilo de --pila=[0-9]* uso \([0-9]*\) B$$/\1/p' build/importhondo/pila_hello.txt); \
	 grep -vE 'INICIANDO|FIN DE' build/importhondo/salida.txt; \
	 echo "  la pila: ImportHondo $${u:-?} B, hello $${h:-?} B"; \
	 if [ $$st -eq 0 ] && [ $$sth -eq 0 ] && grep -q '^cadena de 13 imports: la suma de 1 a 13 es 91$$' build/importhondo/salida.txt \
	    && grep -q '^fin$$' build/importhondo/salida.txt && [ -n "$$u" ] && [ -n "$$h" ] && [ $$((u - h)) -lt 8192 ]; then \
	   echo "PASS test-importhondo"; \
	 else \
	   echo "FAIL test-importhondo (salida $$st, la de hello $$sth; la cadena tiene que costar menos de 8192 B más que hello)"; \
	   exit 1; \
	 fi

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
	@if [ ! -f build/sql/aot_SQLite.c ]; then 	  echo "(falta build/sql/aot_SQLite.c — generalo con:"; 	  echo "   java -cp '../lexer-java/target/classes;../pack/target/classes;../miVM/target/classes' \\"; 	  echo "        basicplus.frontend.AotMain ../bpstdlib/sqlite/SQLite.bp build/sql)"; 	  exit 1; 	fi
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
	@echo "--- huecos (#718): las listas por tamano, el mas bajo que vale, la astilla, el orden y el barrido (esperado: solo 'ok', [status=OK]) ---"
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

# V7/H10 #628 (2-oct): la fachada con el motor del PC y el log, y SIN littlefs. No enlazaba desde el 31-ago (#461:
# la fachada llamaba a bpvm_fs_log_fail, que vivía en fs_lfs.c) y ninguna red la corría; ahora el motivo de un fallo
# vive en la fachada, y que esto enlace con esta lista es la prueba de que no depende de littlefs.
test-crc:
	@mkdir -p build
	$(CC) $(CFLAGS) test/test_crc.c src/crc32.c src/fs_facade.c src/fs_host.c src/bpvm_log.c -o build/test_crc.exe
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

sim: $(LIBOBJ) $(WIN_RES) $(if $(filter 1,$(LVGL)),build/liblvgl.a build/SDL2.dll,) $(if $(filter 1,$(SQLITE)),build/sql/sqlite3.o build/sql/aot_SQLite.c,)
	@mkdir -p build
	$(CC) $(CFLAGS) $(if $(filter 1,$(SQLITE)),$(SIM_SQL_FLAGS),) -Ipico -Isrc -Ithird_party/fatfs tools/bpvm_sim.c src/json_min.c src/wire_v1_proto.c src/bpvm_repl.c src/bpvm_boot2.c src/bpvm_listdir.c $(SIM_SD_SRC) $(if $(filter 1,$(SQLITE)),$(SIM_SQL_SRC),) $(LIBOBJ) $(WIN_RES) -o $(SIM_OUT) $(LDFLAGS)
	@echo "sim listo: $(SIM_OUT) [--port=N] [--mem=N] [--psram=N] [--flash=N] [--fs=IMG] [--sd=IMG]"

# Los ingredientes de SQLITE=1, si no estan ya (los comparte con test-sqldemo).
build/sql/sqlite3.o:
	@mkdir -p build/sql
	@[ -f build/sql/sqlite3.c ] || unzip -o -j $(SQLZIP) "*/sqlite3.c" "*/sqlite3.h" -d build/sql >/dev/null
	$(CC) -O1 -w -c build/sql/sqlite3.c -o build/sql/sqlite3.o -Ibuild/sql $(SQLDEF)

build/sql/aot_SQLite.c:
	@echo "falta build/sql/aot_SQLite.c — generalo con:"
	@echo "   java -cp '../lexer-java/target/classes;../pack/target/classes;../miVM/target/classes' basicplus.frontend.AotMain ../bpstdlib/sqlite/SQLite.bp build/sql"
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
# V7 #657 (7-oct) — el compartido, por el CANAL Log (Eduardo: «el log es diferente, es un canal de comunicación…
# ahí sí que puede haber concurrencia como en el I2C o en el SPI»): un fichero abierto para escribir no se vuelve a
# abrir, y tres appendFile que se cruzaban en él eran «ya está abierto». 300 líneas «[INFO ] x»: 3000 bytes.
# V7/H7 (#599, 28-sep) — en la red (check-testc), y el verde que dice el comentario:
# hasta hoy sólo se miraban las líneas rotas; ahora también cada tamaño y cada marca.
# H12 (5-oct) — la SALIDA del programa, sin los avisos de la VM (stderr, a build/iomt.err): con 2>&1, el aviso
# del final de un RUN con varios runs se colaba en mitad de una línea (la escribe otro hilo, la comm task) y la
# comprobación de línea exacta fallaba con los valores bien (tras #662, que cambió el ritmo del final).
test-iomt: $(BIN)
	$(call fixture,IoMt)
	$(call stdlibdep,Core IO Log)
	@if [ ! -f samples/IoMt.mod ]; then \
	  echo "(skip test-iomt: falta samples/IoMt.mod — regenéralo con:"; \
	  echo "   java -jar ../lexer-java/target/basicplus-frontend.jar \\"; \
	  echo "        samples/IoMt.bp --compile samples --backend=mivm)"; \
	else \
	  echo "--- B1.4b Threads BP + littlefs + --smp=4 (esperado: 200/200/200/3000, 0 rotas) ---"; \
	  rm -f build/iomt.img; \
	  out=$$(cd samples && ../$(BIN) --fs=lfs:../build/iomt.img --smp=4 IoMt.mod 2>../build/iomt.err); \
	  echo "$$out" | grep -vE 'INICIANDO|FIN DE'; \
	  if echo "$$out" | grep -qx "own1 = 200" && echo "$$out" | grep -qx "own2 = 200" && \
	     echo "$$out" | grep -qx "own3 = 200" && echo "$$out" | grep -qx "shared = 3000" && \
	     echo "$$out" | grep -qx "marcas a/b/c = 100 100 100" && echo "$$out" | grep -qx "lineas rotas = 0"; then \
	    echo "OK: 3 hilos BP escriben 4 ficheros a la vez sobre littlefs con 4 workers: tamanos exactos, 0 lineas rotas"; \
	  else echo "FAIL test-iomt (los avisos de la VM, abajo)"; tail -15 build/iomt.err; exit 1; fi; \
	  rm -f build/iomt.img build/iomt.err; \
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

# V7/H10 #628 (2-oct) — EL MOTIVO DE UN FALLO DEL FS: cada clase forzada en el motor del PC y en littlefs, y que un
# fallo NO herede el motivo de otro (un motor de mentira que falla sin decir nada). FAT, por el cable: sd_smoke.
test-fsfallo: $(LIBOBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/test_fsfallo.c $(LIBOBJ) -o build/test_fsfallo.exe $(LDFLAGS)
	@echo "--- #628 el motivo de un fallo del FS, en los motores y en la fachada (esperado: solo 'ok', [status=OK]) ---"
	./build/test_fsfallo.exe

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
