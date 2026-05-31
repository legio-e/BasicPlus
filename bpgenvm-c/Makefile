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

.PHONY: all clean smoke

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
