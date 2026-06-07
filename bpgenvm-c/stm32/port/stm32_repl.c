/*
 * stm32_repl.c — REPL wire v1 bare-metal (H9.2.a: conectar).
 *
 * Implementa el subconjunto del protocolo (docs/BPVM_WIRE_PROTOCOL.md)
 * necesario para que el IDE CONECTE y reconozca el dispositivo:
 *   META:  HELLO, INFO, TIME, PING, RESET
 *   FILES: LIST, DF   (FS vacío de momento → entries:[], 0 bytes)
 * El resto responde ERROR/UNSUPPORTED. PUT/RUN/GET/DEL llegan en H9.2.b.
 *
 * Single-thread, sin FreeRTOS: super-bucle de polling. Heartbeat por LED.
 */
#include "stm32_repl.h"
#include "stm32_wire.h"
#include "json_min.h"

#include "main.h"
#include "stm32u5xx_nucleo.h"   /* BSP_LED_Toggle, LED_GREEN */

#include <string.h>
#include <stdio.h>

#define SERVER_NAME "bpvm-stm32"
#define BOARD_NAME  "nucleo-u575zi"

static char s_line[WIRE_LINE_MAX];

/* ---- Handlers ---- */

static void reply_empty(const char* type, long id) {
    char buf[96];
    int n = snprintf(buf, sizeof(buf), "{\"type\":\"%s\",\"id\":%ld}", type, id);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void handle_hello(long id) {
    char buf[256];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"HELLO_REPLY\",\"id\":%ld,\"protoVersion\":1,"
        "\"serverName\":\"%s\",\"serverBuild\":\"%s %s\","
        "\"capabilities\":[\"META\",\"FILES\",\"TERMINAL\"]}",
        id, SERVER_NAME, __DATE__, __TIME__);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void handle_info(long id) {
    /* 96-bit unique ID del STM32 (UID_BASE en el device header). */
    uint32_t u0 = *(volatile uint32_t*) (UID_BASE + 0U);
    uint32_t u1 = *(volatile uint32_t*) (UID_BASE + 4U);
    uint32_t u2 = *(volatile uint32_t*) (UID_BASE + 8U);
    char buf[320];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"INFO_REPLY\",\"id\":%ld,"
        "\"uniqueId\":\"%08lX%08lX%08lX\","
        "\"boardName\":\"%s\",\"cpuFreqHz\":%lu,\"uptimeMs\":%lu,\"tempC\":0,"
        "\"fsTotalBytes\":0,\"fsUsedBytes\":0}",
        id,
        (unsigned long) u2, (unsigned long) u1, (unsigned long) u0,
        BOARD_NAME,
        (unsigned long) SystemCoreClock, (unsigned long) HAL_GetTick());
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void handle_list(long id) {
    char buf[96];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"LIST_REPLY\",\"id\":%ld,\"entries\":[]}", id);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void handle_df(long id) {
    char buf[160];
    int n = snprintf(buf, sizeof(buf),
        "{\"type\":\"DF_REPLY\",\"id\":%ld,\"totalBytes\":0,\"usedBytes\":0,"
        "\"freeBytes\":0,\"fileCount\":0}", id);
    if (n > 0) stm32_wire_send_line(buf, (size_t) n);
}

static void dispatch(int first_char) {
    int len = stm32_wire_recv_line(first_char, s_line, sizeof(s_line));
    if (len < 0) { stm32_wire_send_fatal("PROTOCOL_ERROR", "line too long"); return; }

    json_obj_t obj;
    if (json_parse(s_line, (size_t) len, &obj) != 0) {
        stm32_wire_send_fatal("PROTOCOL_ERROR", "bad JSON");
        return;
    }
    long id = json_get_long(&obj, "id", 0);
    char type[40];
    if (json_get_str(&obj, "type", type, sizeof(type)) < 0) {
        stm32_wire_send_error(id, "PROTOCOL_ERROR", "missing type");
        return;
    }

    if      (strcmp(type, "HELLO") == 0) handle_hello(id);
    else if (strcmp(type, "INFO")  == 0) handle_info(id);
    else if (strcmp(type, "PING")  == 0) reply_empty("PONG", id);
    else if (strcmp(type, "TIME")  == 0) reply_empty("TIME_REPLY", id);
    else if (strcmp(type, "LIST")  == 0) handle_list(id);
    else if (strcmp(type, "DF")    == 0) handle_df(id);
    else if (strcmp(type, "RESET") == 0) {
        reply_empty("RESET_REPLY", id);
        HAL_Delay(50);
        NVIC_SystemReset();
    } else {
        stm32_wire_send_error(id, "UNSUPPORTED", "not implemented (H9.2.a)");
    }
}

void stm32_repl_run(void) {
    /* Banner de boot (el IDE lo drena en el connect; ayuda en un terminal). */
    stm32_wire_send_cstr("=== bpvm-stm32 REPL (wire v1) listo ===");

    uint32_t last_blink = HAL_GetTick();
    for (;;) {
        int c = stm32_wire_getchar();
        if (c == '{') {
            dispatch(c);
        }
        /* resto de bytes: ignorar (sin REPL de texto humano en el MVP). */

        uint32_t now = HAL_GetTick();
        if (now - last_blink >= 500U) {     /* heartbeat: "estoy vivo" */
            last_blink = now;
            BSP_LED_Toggle(LED_GREEN);
        }
    }
}
