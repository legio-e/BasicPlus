# Componente `main` del proyecto ESP32-S3.
#
# V6/P1.C3.2 — las fuentes de la FAMILIA viven en `../common/`; aqui solo queda
# este fichero y `chip_cfg.h`, que es lo unico que de verdad cambia por silicio.
# Componente `main` — H4.3. Core VM portable + backend ESP32 + wire v1
# (UART0) + FS RAM + REPL. El parser JSON (src/json_min.c) es C portable
# y se reutiliza directamente.
idf_component_register(
    SRCS
        "../common/main.c"
        "../common/platform_esp32.c"
        "../common/wire_v1.c"
        "../common/wire_red.c"               # V8/H3: el cable de la red
        "../common/wifi_esp32.c"             # V8/H3: la radio de la familia, publicada como 'WIFI'
        "../common/bt_esp32.c"               # V8/H2, paso 5: el Bluetooth de la familia (NimBLE), publicado como 'BT  '
        "../common/net_esp32.c"              # V8/H3: la IP de la familia (esp_netif)
        "../../src/wire_cables.c"            # V8/H3: encima de la UART, y del cable de la red cuando lo tenga
        "../common/fs_lfs_esp32.c"              # H2-B3: littlefs (cintura esp_partition + shim fs.h) — reemplaza fs_ram.c
        "../common/repl_esp32.c"
        "../common/gpio_esp32.c"
        "../common/esp32_mods.c"
        "../../src/json_min.c"               # V6/U1.1: parser JSON comun (era una copia local)
        "../../src/wire_v1_proto.c"               # V6/U2.1: los builders JSON del wire, ya en el comun
        "../../src/bpvm_repl.c"               # V6/U3: los verbos comunes del REPL
        "../../src/bpvm_sd_placa.c"           # V7/H3: la SD de la placa (SD_INFO/SD_MOUNT), en el comun
        "../common/board_mgr_esp32.c"           # H9: gestion de placa (env bpenv + climb + STATE/ENV/PART)
        "../../esp32/common/bios_esp32.c"    # V7/H6: la tabla BIOS y el ancla del pack nativo (era del P4)
        "../../esp32/common/pack_esp32.c"    # V7/H6: el cargador del pack nativo y su bloque de RAM (era del P4)
        # --- core VM (portable C99) ---
        "../../src/bpvm.c"
        "../../src/loader.c"
        "../../src/interp.c"
        "../../src/heap.c"
        "../../src/builtins.c"
        "../../src/bpstr_vm.c"        # V8 #730: la biblioteca de cadenas (bpstr/)
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
        "../../src/bpvm_handles.c"  # V6/U5: la tabla de handles, en su módulo
        "../../src/bpvm_io.c"  # V6/A1: el hilo `io` (segundo hilo de SO: wire + salida por lineas)
        "../../src/bpvm_bios.c"        # V5/I: tabla BIOS prestada al pack nativo
        "../../src/bpvm_bios_fs.c"     # V5/H2: sus ranuras de ficheros (PORTABLE) — faltaba de alta
        "../../src/bpvm_bios_hal.c"    # V7/H6: sus ranuras de HARDWARE (PORTABLE: las fachadas de la VM)
        "../../src/bpvm_bus.c"         # V7/H6.c G4.a1: los buses SPI/I2C, compartidos (la HAL BP)
        "../../src/bpvm_sonido.c"      # V8/H4 (#708): el sonido, lo común de la HAL BP
        "../../src/bpvm_wifi.c"        # V8/H3: la WiFi, el enlace, lo común de la HAL BP
        "../../src/bpvm_bt.c"          # V8/H2: el Bluetooth, lo común de la HAL BP
        "../../src/bpvm_teclado.c"     # V8/H2, paso 2: el teclado del SO, lo común de la HAL BP
        "../../src/bpvm_raton.c"     # V8/H2, paso 3: el ratón del SO, lo común de la HAL BP
        "../../src/bpvm_hid.c"       # V8/H2, paso 5: lo que manda un teclado o un ratón HID, al SO
        "../../src/bpvm_entrada.c"   # V8/H2, paso 5: la tabla 'ENTR', el teclado y el ratón para la GUI
        "../../src/bpvm_red.c"         # V8/H3: la IP de la placa, lo común
        "../../src/bpvm_npack.c"       # V5/I: escalera de validacion del pack nativo
        "../../src/bpvm_npack_carga.c"  # V7/H7 (Boot 2, fase 2): el cargador comun
        "../../src/bpvm_boot2.c"        # V7/H7 (Boot 2, fase 4): el arranque comun
        "../../src/bpvm_mdn_scan.c"    # V5/H4: buscar el .mdn (FS + pack) en UN sitio
        "../../src/bpvm_nativa.c"      # V7/H5 P2b.1: la pila prestada de una native
        "../../src/bpvm_util.c"        # #338/#353: zona de rascar + canal de diagnostico
        "../../src/bpvm_bmgr.c"        # H9: board manager (compone env+part)
        "../../src/bpvm_bmgr_wire.c"   # H9: nucleo del protocolo STATE/ENV/PART
        "../../src/bpvm_pack.c"        # H3 Packs V4: core portable de packs (lo llama bmgr_wire)
        "../common/aot_funcs_stub.c"            # H4 AOT: def fuerte no-op de esp_aot_register (S3 sin AOT baked-in)
        "../../src/net.c"              # H11 (#241): facade Net — sin backend hasta H11.c (lwIP) → RuntimeError atrapable
        "../../src/link.c"
        "../../src/scheduler.c"
        "../../src/events.c"           # H5.c: cola de eventos + inyección del handler entre quanta
        "../../src/destruir.c"         # V7 #668: liberar es destruir (el registro, la cola, la fase del final)
        "../../src/bpvm_dbg_wire.c"   # #326: ramo de depuracion del wire (portable, ex-Pico)
        "../../src/bpvm_log.c"         # log post-mortem: núcleo portable (idéntico en las 3 familias)
        "../../src/bpvm_listdir.c"     # V5/H6: LIST_DIR comun (salio del repl del Pico)
        "../common/log_esp32.c"                  # ... y su cintura ESP32 (esp_timer + bpenv sector 2)
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
        "../../src/bpvm_machine.c"
        "../../src/rtc.c"
        "../../src/adc.c"
        "../../src/wdt.c"
        "../../src/bpvm_neopixel.c"   # fachada no-op sin backend (H7.4)
    INCLUDE_DIRS
        "."                            # V6/P1.C3.2: aqui vive el chip_cfg.h de ESTE silicio
        "../common"                    # V6/P1.C3.2: las fuentes y cabeceras de la familia
        "../../include"
        "../../src"
        "../../third_party/littlefs"   # H2-B3: lfs.h
    REQUIRES
        esp_wifi         # V8/H3: la WiFi
        bt               # V8/H2, paso 5: el Bluetooth (NimBLE)
        esp_netif
        esp_event
        lwip
        esp_timer
        esp_driver_uart
        esp_partition
        app_update bootloader_support   # V7/H9: el sello de la imagen (imageCrc): la app que corre y su longitud
        esp_driver_gpio
        esp_driver_spi   # H16: backend SPI (spi_master)
        esp_driver_i2c   # H16: backend I2C (i2c_master, IDF v6)
        esp_driver_ledc  # H14: backend PWM (LEDC)
        esp_adc          # H14: backend ADC (esp_adc oneshot)
        esp_driver_pcnt  # H14: backend contador de pulsos (PCNT)
        esp_driver_rmt   # V7/H4: backend NeoPixel (WS2812) por RMT, en gpio_esp32.c
        esp_driver_tsens # V8 #826: el sensor de temperatura del chip (Machine.tempC y el INFO), en gpio_esp32.c
        spi_flash        # INFO: esp_flash_get_size (flash real montada)
        esp_hw_support   # INFO: esp_mac.h (uniqueId desde efuse)
        esp_psram        # #670: esp_psram_is_initialized (los GPIO 33-37 de la PSRAM octal)
        esp_mm           # #676: la zona de packs por el bus de DATOS (esp_mmu_paddr_to_vaddr) y su caché (esp_cache_msync)
)

# H2-B3: littlefs — buffers estáticos (LFS_NO_MALLOC) + trazas silenciadas
# (LFS_NO_DEBUG/WARN/ERROR: por stdout romperían consola/paridad, como en el RP2350).
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LFS_NO_DEBUG LFS_NO_WARN LFS_NO_ERROR LFS_NO_MALLOC
    BPVM_WIRE_CABLES=1)   # V8/H3: src/wire_cables.c

# #473 (lote D, 264; 3-oct): aquí se relajaba -Wformat-truncation para fs_lfs_esp32.c, por el snprintf
# del BFS de fs_list; fs_list se fue al común (#461) y el fichero ya no tiene ninguno.

# V8 #730: la biblioteca de cadenas (bpgenvm-c/bpstr/, por src/bpstr_vm.c), con -O2 (decidido por Eduardo el
# 3-nov; el resto, con lo del proyecto) y sin que GCC cambie su bucle de rellenar por una llamada a memset.
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../../src/bpstr_vm.c"
    PROPERTIES COMPILE_OPTIONS "-O2;-fno-tree-loop-distribute-patterns")
