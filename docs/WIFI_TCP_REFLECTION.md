# Reflexión: WiFi/TCP en Pico 2 W

> Documento de reflexión, no de spec. Recoge las decisiones que
> conviene tener pensadas para añadir comunicación TCP/IP al
> firmware bpvm-pico cuando llegue la Raspberry Pi Pico 2 W con
> el chip WiFi CYW43439. La comunicación USB CDC actual se
> mantiene; TCP es una vía adicional, no un reemplazo.

---

## Hardware

**Raspberry Pi Pico 2 W** = RP2350 + chip CYW43439 (WiFi 802.11 b/g/n
2.4GHz + Bluetooth 5.2). El RP2350 y el CYW43 se comunican por SPI
internamente; el SDK del Pico abstrae todo.

## Stack de software: lwIP, no FreeRTOS+TCP

El Pico SDK no usa FreeRTOS+TCP. Trae **lwIP** integrado con un
"arch wrapper" llamado `pico_cyw43_arch`. Tres modos disponibles:

| Modo | Descripción |
|---|---|
| `lwip_threadsafe_background` | No-FreeRTOS, polling desde main loop |
| `lwip_poll` | No-FreeRTOS, llamadas manuales a `cyw43_arch_poll()` |
| `lwip_sys_freertos` | **Nuestra opción** — lwIP en task FreeRTOS dedicada |

En `lwip_sys_freertos`, lwIP levanta su `tcpip_thread` que procesa
los paquetes recibidos. Las aplicaciones (otra task) usan **BSD
sockets** (`lwip/sockets.h`): `socket`, `bind`, `listen`, `accept`,
`recv`, `send`, `close`. Es la API estándar. Cualquier ejemplo de
servidor TCP en C compila casi tal cual.

## Footprint estimado

| Componente | Flash | RAM |
|---|---|---|
| lwIP core + sockets | 60-80 KB | 8-12 KB heap + buffers |
| Driver CYW43 (incluye firmware del chip) | 90-120 KB | 16-24 KB |
| Task tcpip_thread (stack) | — | 4-8 KB stack |
| Buffers para TCP/RX/TX | — | 4-8 KB |
| **Total estimado** | **~150-200 KB** | **~40-50 KB** |

Estado actual del firmware: **flash 317 KB, BSS 470 KB de 512 KB
SRAM**. Tras añadir WiFi:

- Flash: 317 + 200 = ~520 KB. Cabe en 4 MB sobradamente.
- BSS: 470 + 50 = ~520 KB. **Excede los 512 KB SRAM.**

Conclusión: añadir WiFi va a requerir reducir alguno de los
big-spenders actuales:
- VM buffer (128 KB) → 64 KB (las apps típicas usan mucho menos).
- FS_DATA_SIZE (128 KB) → 64 KB (con la jerarquía /lib /app, basta).
- O subir a la arquitectura "FS-XIP direct" que ya estaba en el
  futuro y eliminar el mirror de 128 KB del FS.

Lo más limpio sería medir REAL tras integrar y luego decidir.

---

## Arquitectura propuesta

### Mismo protocolo, distinto transport

USB CDC y TCP **comparten el handler de comandos**. HELLO, LS, PUT,
GET, DEL, RUN, MEM, SAVE, FORMAT, LOG, TIME, RESET, BOOTSEL —
todos funcionan idénticos por las dos vías.

Esto es exactamente la abstracción de **#137 transport-abstract**.
La interfaz natural:

```c
typedef struct {
    int     (*read_line)(transport_t*, char* buf, size_t cap, int timeout_ms);
    int     (*read_bytes)(transport_t*, uint8_t* buf, size_t n, int timeout_ms);
    int     (*write)(transport_t*, const void* data, size_t n);
    void    (*flush)(transport_t*);
    int     (*is_connected)(transport_t*);
} transport_ops_t;

typedef struct {
    const transport_ops_t* ops;
    void* impl;     // CDC handle o socket
} transport_t;
```

El loop del REPL ya no lee de `stdin` directo, sino de un
`transport_t*`. Cada transport corre en su propia task FreeRTOS.

### Tasks FreeRTOS resultantes

```
┌──────────────────────────────────────────┐
│ Task: cdc_repl                           │
│  - lee/escribe en transport USB CDC      │
│  - mismo handler de comandos             │
└──────────────────────────────────────────┘
┌──────────────────────────────────────────┐
│ Task: tcp_server                         │
│  - listen en puerto 7332                 │
│  - accept → crear task tcp_session       │
└──────────────────────────────────────────┘
┌──────────────────────────────────────────┐
│ Task: tcp_session (por cliente conectado)│
│  - lee/escribe en transport TCP          │
│  - mismo handler de comandos             │
└──────────────────────────────────────────┘
┌──────────────────────────────────────────┐
│ Task: tcpip_thread (lwIP, interno)       │
└──────────────────────────────────────────┘
┌──────────────────────────────────────────┐
│ Task: vm_task                            │
│  - ejecuta VM cuando RUN lo dispara      │
└──────────────────────────────────────────┘
```

Encaja directamente con **#136 arch-tasks** (separar firmware en
tasks FreeRTOS independientes).

### Concurrencia y mutex

Dos vías de comandos hablando al mismo FS y al mismo VM. Riesgo
real de race condition. Necesitamos:

- **Mutex en el FS**: PUT/DEL/SAVE serializados — solo uno a la vez.
- **Mutex en el VM**: RUN solo permitido cuando vm_task está idle.
  Si está corriendo, el segundo cliente recibe "VM busy".
- **Output streaming**: solo un cliente recibe el output del RUN
  en curso (el que lo lanzó). Los demás ven "VM busy" si lo intentan.

---

## Discovery: cómo el IDE encuentra la Pico

Tres alternativas, ordenadas por preferencia:

### A) mDNS (Bonjour/Zeroconf)

La Pico anuncia `bpvm-pico-<unique-id>.local` en la LAN. El IDE
escanea servicios `_bpvm._tcp.local`, recoge las Picos disponibles,
las muestra en un combo.

Ventajas: "just works" en macOS, Linux y Windows 10+. No requiere
configurar nada en el router.
Desventajas: añadir una librería mDNS (lwIP trae `mdns_resp`
opcional, ~5 KB flash). Configuración inicial del módulo
ligeramente más compleja.

### B) Broadcast UDP

La Pico envía un paquete UDP a 255.255.255.255:7333 cada 5
segundos con `{name, ip, unique_id, build}`. El IDE escucha en
ese puerto, llena su lista.

Ventajas: implementación trivial (~30 líneas).
Desventajas: tráfico de broadcast en la LAN; algunos routers lo
filtran; conflictos potenciales si hay otras herramientas
broadcasting.

### C) Manual

El usuario configura la IP en el IDE. La Pico no se anuncia.

Ventajas: cero código en la Pico.
Desventajas: el usuario tiene que averiguar la IP (router admin
o serial console).

**Decisión sugerida**: implementar B (broadcast UDP) primero por
ser más simple. Migrar a A (mDNS) si el broadcast da problemas
o cuando queramos pulir la UX.

---

## Provisioning WiFi

SSID y password **no** pueden venir hardcoded. Tres opciones:

### A) Fichero `/sys/wifi.json` configurado por USB CDC

```json
{ "ssid": "MiRed", "password": "secreta", "country": "ES" }
```

El IDE tiene un dialog "Configurar WiFi" que escribe este fichero
vía USB CDC. Al siguiente boot la Pico lee y se conecta.

Ventajas: simple, configuración persistente.
Desventajas: password en plano. Aceptable en LAN doméstica.

### B) Modo AP fallback

Si la Pico no encuentra red al boot, levanta su propio AP
`bpvm-setup-<id>` y un mini servidor HTTP en 192.168.4.1 con un
form para configurar SSID + password.

Ventajas: configuración sin necesidad de USB.
Desventajas: 30-50 KB extra de código (servidor HTTP).

**Decisión sugerida**: A primero (vía USB CDC + IDE), B como
mejora futura si se hace tedioso flashear y configurar.

---

## Sincronización de tiempo

Hoy el IDE manda `TIME <epochsec>` por CDC al conectar. Con WiFi:

- **NTP** desde la propia Pico: query a `pool.ntp.org` al boot
  tras conectarse al WiFi. Más limpio, sin dependencia del IDE.
- TIME por USB CDC se mantiene como fallback para escenarios sin
  red.

Implementación: lwIP trae SNTP cliente integrado (`apps/sntp.h`).
~3 KB flash.

---

## Bluetooth (futuro)

El CYW43439 también soporta Bluetooth 5.2 / BLE. Casos de uso:

- **Provisioning WiFi por BLE**: app móvil escanea, conecta, envía
  SSID/password. Más cómodo que el modo AP fallback.
- **Telemetría BLE**: enviar sensores a una app móvil sin necesidad
  de IP/WiFi.

Stack: el SDK del Pico trae BTstack integrado. Otros ~100-150 KB
de flash. Coste alto para casos puntuales — diferir hasta que
haya un caso de uso concreto.

---

## Orden razonable de implementación

Si algún día se aborda:

1. **Smoke test cyw43_arch**: programa mínimo que enciende el
   chip, conecta a una red conocida, hace ping. Sin VM, sin nada.
   Solo medir footprint real y validar que cabemos.
2. **Decidir reducciones**: VM buffer, FS_DATA_SIZE, o pasar a
   FS-XIP-direct. Basado en lo medido en el paso 1.
3. **Transport abstraction** (#137): refactor del REPL para que
   lea de un `transport_t*` en lugar de stdin directo.
4. **Servidor TCP básico**: una task que listen+accept, crea
   tcp_session por cliente. Reusa el handler de comandos del REPL.
5. **Mutex en FS y VM**: serializar PUT/DEL/SAVE/RUN entre los dos
   transports.
6. **Provisioning WiFi**: fichero `/sys/wifi.json` + comando del
   REPL para configurarlo.
7. **Discovery**: broadcast UDP primero, mDNS después si compensa.
8. **NTP**: sincronización automática al conectar la red.
9. **IDE**: combo de Picos detectadas, conexión TCP transparente.
   La interfaz `transport` en el lado del IDE permite reutilizar
   PicoClient para TCP también.

Pasos 1-2 son de evaluación, no compromiso. Si tras medir vemos
que añadir WiFi requiere sacrificar más de lo que aceptable,
re-evaluamos prioridad.

---

## Conclusión

WiFi/TCP es **complementario al USB CDC**, no reemplazo. El mismo
protocolo, el mismo handler de comandos, dos vías que pueden
estar activas simultáneamente. Encaja con la arquitectura de
transport abstraction que ya estaba planeada (#137).

La principal incógnita es **footprint**. Probablemente vamos a
tener que sacrificar algo (VM buffer o FS mirror) para hacer
sitio a lwIP + cyw43_arch + buffers. Hasta no medir, no
decidimos.

Beneficios esperados:
- IDE puede conectar sin cable USB → desarrollo más cómodo.
- Múltiples Picos en la LAN visibles en el IDE.
- NTP automático: tiempo real sin necesidad del IDE.
- Sienta las bases para **debug-on-Pico** (#140), telemetría,
  control remoto, etc.

---

*Conversación entre Eduardo y Claude Opus 4.7, 24-mayo-2026.
Eduardo acaba de pedir un Pico 2 W; hardware aún no en mano.*
