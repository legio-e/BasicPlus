# Componente `main` — ENSAYO ESP32-C3 (V6/P1).
#
# NO tiene fuentes propias salvo main.c: TODA la cintura ESP32 se toma del S3
# por ruta relativa (`../../esp32/common/...`). Esa es justamente la medida que
# este ensayo persigue — cuanto de una familia nueva es codigo y cuanto es
# una lista de ficheros.
#
# Original del S3: H4.3. Core VM portable + backend ESP32 + wire v1
# (UART0) + FS RAM + REPL. El parser JSON (src/json_min.c) es C portable
# y se reutiliza directamente.
idf_component_register(
    SRCS
        "../../esp32/common/main.c"   # V6/P1.C3.2: el arranque de la FAMILIA; lo propio, en chip_cfg.h
        "../../esp32/common/platform_esp32.c"
        # V6/P1.C3.3 — EL CABLE DE ESTA PLACA VA POR EL USB, no por UART0.
        # Esta placa tiene UN solo conector USB, y el modo descarga se entra a
        # mano con los pulsadores BOOT+RESET: o sea que en marcha el USB esta
        # libre. Si esta libre lo ocupa el WIRE, que es lo que necesita el
        # usuario (un cable y el IDE conecta); la consola se va a UART0, que es
        # donde tiene sentido lo que solo miramos nosotros cuando algo falla.
        # El S3 y el P4 tienen dos puertos y NO se tocan.
        "../../esp32/common/wire_v1_usbjtag.c"
        "c3_board_id.c"                       # V6/P1.C3.3: identidad de placa (pisa el default del S3)
        "../../esp32/common/fs_lfs_esp32.c"              # H2-B3: littlefs (cintura esp_partition + shim fs.h) — reemplaza fs_ram.c
        "../../esp32/common/repl_esp32.c"
        "../../esp32/common/gpio_esp32.c"
        "../../esp32/common/esp32_mods.c"
        "../../src/json_min.c"               # V6/U1.1: parser JSON comun (era una copia local)
        "../../src/wire_v1_proto.c"               # V6/U2.1: los builders JSON del wire, ya en el comun
        "../../src/bpvm_repl.c"               # V6/U3: los verbos comunes del REPL
        "../../esp32/common/board_mgr_esp32.c"           # H9: gestion de placa (env bpenv + climb + STATE/ENV/PART)
        # --- core VM (portable C99) ---
        "../../src/bpvm.c"
        "../../src/loader.c"
        "../../src/interp.c"
        "../../src/heap.c"
        "../../src/builtins.c"
        "../../src/fs_facade.c"        # #247: facade file I/O (backend ESP32 = fs_lfs_esp32.c)
        "../../src/fs_lfs.c"                       # H2-B3: backend littlefs de la fachada (portable)
        "../../third_party/littlefs/lfs.c"        # H2: motor littlefs v2.11.3 (in-tree)
        "../../third_party/littlefs/lfs_util.c"
        "../../src/crc32.c"            # paso 4 cierre: CRC del FS en LS (== java.util.zip.CRC32)
        # --- H9 kernel por capas (portable C99, host-testable) ---
        "../../src/bpvm_env.c"         # H9: bloque de env A/B
        "../../src/bpvm_part.c"        # H9: particiones (FS+Packs, un mando)
        "../../src/bpvm_boot.c"        # H9: maquina de estados del arranque
        "../../src/bpvm_sqlmem.c"      # V5/H: regla del bloque de memoria de la BD
        "../../src/bpvm_mem.c"      # V6/U6: el planificador de memoria de la VM
        "../../src/bpvm_mods.c"     # V6/#466: la regla del /lib de la imagen (común)
        "../../src/bpvm_bios.c"        # V5/I: tabla BIOS prestada al pack nativo
        "../../src/bpvm_bios_fs.c"     # V5/H2: sus ranuras de ficheros (PORTABLE) — faltaba de alta
        "../../src/bpvm_npack.c"       # V5/I: escalera de validacion del pack nativo
        "../../src/bpvm_mdn_scan.c"    # V5/H4: buscar el .mdn (FS + pack) en UN sitio
        "../../src/bpvm_util.c"        # #338/#353: zona de rascar + canal de diagnostico
        "../../src/bpvm_bmgr.c"        # H9: board manager (compone env+part)
        "../../src/bpvm_bmgr_wire.c"   # H9: nucleo del protocolo STATE/ENV/PART
        "../../src/bpvm_pack.c"        # H3 Packs V4: core portable de packs (lo llama bmgr_wire)
        "../../esp32/common/aot_funcs_stub.c"            # H4 AOT: def fuerte no-op de esp_aot_register (S3 sin AOT baked-in)
        "../../src/net.c"              # H11 (#241): facade Net — sin backend hasta H11.c (lwIP) → RuntimeError atrapable
        "../../src/link.c"
        "../../src/scheduler.c"
        "../../src/events.c"           # H5.c: cola de eventos + inyección del handler entre quanta
        "../../src/bpvm_dbg_wire.c"   # #326: ramo de depuracion del wire (portable, ex-Pico)
        "../../src/bpvm_log.c"         # log post-mortem: núcleo portable (idéntico en las 3 familias)
        "../../src/bpvm_listdir.c"     # V5/H6: LIST_DIR comun (salio del repl del Pico)
        "../../esp32/common/log_esp32.c"                  # ... y su cintura ESP32 (esp_timer + bpenv sector 2)
        "../../src/threading.c"
        "../../src/exceptions.c"
        "../../src/aot_registry.c"
        # H11: aquí vive bpvm_mdn_host_arch(), la ISA que publica el INFO — sale de
        # los MISMOS macros que gatean el loader, así que van juntos (una sola
        # fuente de verdad). El S3 aún no carga .mdn: --gc-sections tira lo que no use.
        "../../src/mdn_loader.c"
        "../../src/bpvm_aot_helpers.c"
        "../../src/scheduler_smp.c"
        "../../src/comm_common.c"
        "../../src/comm_host.c"        # RELLENO del contrato SMP (bpvm_comm.h): esta familia no ofrece SMP; si algun dia lo ofrece, necesita su comm_esp32.c que hable por el WIRE
        # --- fachadas HW (stubs sin backend) ---
        "../../src/gpio.c"
        "../../src/i2c.c"
        "../../src/spi.c"
        "../../src/uart.c"
        "../../src/pulse.c"
        "../../src/pwm.c"
        "../../src/pico.c"
        "../../src/rtc.c"
        "../../src/adc.c"
        "../../src/wdt.c"
        "../../src/bpvm_neopixel.c"   # fachada no-op sin backend (H7.4)
    INCLUDE_DIRS
        "."
        "../../esp32/common"   # V6/P1.C3.2: fuentes y cabeceras de la familia ESP32
        "../../include"
        "../../src"
        "../../third_party/littlefs"   # H2-B3: lfs.h
    REQUIRES
        esp_timer
        esp_driver_uart
        esp_driver_usb_serial_jtag   # V6/P1.C3.3: el cable del wire va por el USB nativo
        esp_partition
        esp_driver_gpio
        esp_driver_spi   # H16: backend SPI (spi_master)
        esp_driver_i2c   # H16: backend I2C (i2c_master, IDF v6)
        esp_driver_ledc  # H14: backend PWM (LEDC)
        esp_adc          # H14: backend ADC (esp_adc oneshot)
        esp_driver_pcnt  # H14: backend contador de pulsos (PCNT)
        spi_flash        # INFO: esp_flash_get_size (flash real montada)
        esp_hw_support
        esp_mm            # V6: esp_cache.h — el gate del .mdn AOT (el C3 es RISC-V)   # INFO: esp_mac.h (uniqueId desde efuse)
)

# H2-B3: littlefs — buffers estáticos (LFS_NO_MALLOC) + trazas silenciadas
# (LFS_NO_DEBUG/WARN/ERROR: por stdout romperían consola/paridad, como en el RP2350).
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LFS_NO_DEBUG LFS_NO_WARN LFS_NO_ERROR LFS_NO_MALLOC)

# fs_lfs_esp32.c: el BFS de fs_list hace snprintf de paths (acotados por la
# estructura del FS, pero GCC no lo ve → -Wformat-truncation). ESP-IDF fuerza
# -Werror (el Pico no) → relajamos SOLO este fichero (mismo código probado en RP2350).
set_source_files_properties("../../esp32/common/fs_lfs_esp32.c" PROPERTIES COMPILE_FLAGS -Wno-format-truncation)
