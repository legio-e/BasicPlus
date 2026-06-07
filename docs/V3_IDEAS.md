# V3 — cajón de ideas (informal)

> **Nada formal todavía.** Solo apuntar puntos sueltos. Cuando se cierre V2 y se
> abra V3, se organiza en condiciones (como se hizo con V2). Iniciado 2026-06-07.

## Objetivo cabecera: GUI gráfica

GUI para dispositivos pequeños. **LVGL** (C) evaluado como viable; se usaría un
subconjunto razonable, de forma incremental. **Dos imágenes**: con y sin LVGL (los
dispositivos sin pantalla no pagan el coste). PSRAM/SDRAM como habilitador
(framebuffers). **Placa de trabajo ya disponible: STM32F769I-DISCO** (LCD 4" táctil
por DSI + 16 MB SDRAM) — perfecta para esto.

## Infraestructura que la GUI arrastra (movida desde V2)

- **Dual-core**: dedicar un núcleo a lo gráfico para un rendimiento general
  equilibrado (#153). Con él viaja el **fix de B1** (la race solo aparece con
  paralelismo real; en V2 está mitigada a 1 worker, el device single-core es inmune).
- **Eventos**: §9 interrupciones HW → eventos BP (Modelo B). La GUI los necesita.
- **Callbacks / función-valor**: §8 (`CALL_INDIRECT`) — mecanismo de los listeners.

## Otras cosas (no descartadas)

- **TCP/IP ampliado**: V2 deja un cliente simple; V3 amplía (servidor, más API, TLS…).
- (lo que vaya surgiendo)
