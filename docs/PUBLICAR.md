# Checklist de publicación — BasicPlus v2.0

Doc de mantenedor (no enlazado desde la portada). Pasos para sacar la release.
Pre-requisitos técnicos YA resueltos: `.mod` skew (3 familias canónicas vía
`bpgenvm-c/scripts/regen_all_mods.sh`) y `/docs` servible como Pages (`.nojekyll`).

## 1 · Pre-flight (antes de etiquetar)

- [ ] **Stdlib canónica**: si tocaste `bpstdlib/*.bp`, recompílala (IDE/frontend) y
      luego `bash bpgenvm-c/scripts/regen_all_mods.sh` (resincroniza los blobs de las
      3 familias). Hoy: al día, idempotente (0 diff).
- [ ] **Suite verde**: `cd lexer-java && mvn test`.
- [ ] **Paridad dual-VM** de los samples clave en host (ver `verificacion-paridad`).
- [ ] **Recompilar los 3 firmwares** con los blobs canónicos:
  - Pico/Metro: `cd bpgenvm-c/pico/build && ninja bpvm_pico` → `bpvm_pico.uf2`
  - ESP32-S3:  `cd bpgenvm-c/esp32 && idf.py build` (+ `merge_bin`) → `bpvm_esp32_merged.bin`
  - STM32:     STM32CubeIDE → `bpvm_stm32.bin`  ← **obligatorio**: `stm32_mods.c` cambió (skew)
- [ ] **BpIde shaded jar**: el IDE empaqueta su copia del frontend → `install` de
      `miVM` + `lexer-java` y luego `package` de `BpIde` (ver memoria
      `rebuild-bpide-fatjar-tras-frontend`). El pom de `BpIde` ya está en `2.0`
      → el jar sale `BpIde-2.0.jar` (el shade reemplaza el principal, sin classifier).

## 2 · Repositorio (lo hace Eduardo)

- [ ] Crear repo público en GitHub.
- [ ] `.gitignore` ya cubre `target/`, `**/build/`, `pico-sdk/`, `FreeRTOS-*`,
      `*.mod/.bpi/.dbg` de samples, prefs del IDE. (Revisado.)
- [ ] `git remote add origin <url>` + `git push -u origin main`.
- [ ] Confirmar que NO se subió nada gordo/privado (SDK, FreeRTOS, binarios de build).

## 3 · GitHub Pages

- [ ] Settings → Pages → Source: branch `main`, carpeta `/docs`.
- [ ] `docs/.nojekyll` presente → sirve estático (los 5 `.html` salen perfectos;
      los `.md` enlazados se ven como texto plano legible).
- [ ] Abrir la URL de Pages: `index.html` como portada; comprobar que los enlaces
      a `manual/referencia/guia-ide/bp-desde-dentro/opcodes` (.html) renderizan y las
      imágenes de la guía (`img/`) cargan.

## 4 · Release v2.0

- [ ] Tag `v2.0` sobre el commit publicado.
- [ ] Cuerpo de la release = contenido de `docs/RELEASES.md`.
- [ ] Adjuntar binarios: `bpvm_pico.uf2`, `bpvm_esp32_merged.bin`, `bpvm_stm32.bin`,
      `BpIde-2.0.jar`.

## Pendientes menores (no bloquean la publicación; → v3)

- Recompilar/reflashear STM32 para que su `.bin` lleve los blobs canónicos.
- ESP32: `tempC`=stub, capacidades de bus del módulo `Pico` genérico, `Esp32.bp` propio.
- Mejora docs Pages: renderizar los `.md` de `/docs` (Jekyll + `href` `.html`) en vez
  de servirlos como texto plano.
