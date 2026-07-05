# Checklist de publicación — BasicPlus v3.0

Doc de mantenedor (no enlazado desde la portada). Pasos para sacar la release v3.0.
El repo público **ya existe** (v2.0 publicada 15-jun); v3.0 es un `push` sobre él.
Pre-requisitos técnicos YA resueltos: `.mod` skew de los CORE (3 familias canónicas
vía `bpgenvm-c/scripts/regen_all_mods.sh`) y `/docs` servible como Pages (`.nojekyll`).

## 1 · Pre-flight (antes de etiquetar)

- [ ] **Stdlib canónica**: si tocaste `bpstdlib/*.bp`, recompílala (IDE/frontend) y
      luego `bash bpgenvm-c/scripts/regen_all_mods.sh` (resincroniza los blobs de las
      familias). `Gui.bp`/`Json.bp` se tocaron en V3 → **obligatorio**.
- [ ] **Recompilar los demos de `bpstdlib`** contra el `Gui` actual: los `.mod` de los
      `Gui*Demo` compilados antes del 2-jul referencian slots viejos de `Gui.Component`
      (skew — p.ej. `GuiColorDemo.mod` daba `setBounds#18` no exportado). No van en el
      firmware, pero deja `bpstdlib/` coherente para el host/VM-C. (Regenéralos con el
      frontend: `java -jar lexer-java/target/basicplus-frontend.jar samples/<Demo>.bp
      --compile bpstdlib --backend=mivm`.)
- [ ] **Suite verde**: `cd lexer-java && mvn test`.
- [ ] **Paridad dual-VM** en host: `bash compat/compat.sh check` (todo verde).
- [ ] **Recompilar los firmwares** con los blobs canónicos (5 binarios):
  - Pico/Metro: `cd bpgenvm-c/pico/build && ninja bpvm_pico` → `bpvm_pico.uf2`
    (lleva el fix ADC board-aware). **NO lleva** el fix del cuelgue `/app` lleno (→ v4).
  - ESP32-S3: `cd bpgenvm-c/esp32 && idf.py build` (+ `merge_bin`) → `bpvm_esp32_merged.bin`
  - **ESP32-P4**: `cd bpgenvm-c/esp32p4 && idf.py build` → `bpvm_esp32p4.bin` (imagen única
    EV/Waveshare; el panel se elige por `/sys/board.json`).
  - STM32 Nucleo: STM32CubeIDE → `bpvm_stm32_nucleo.bin`
  - **STM32 DK2**: STM32CubeIDE → `bpvm_stm32_dk2.bin` (lleva el FS a 496 KB, `.ld` nuevo).
- [ ] **VM-C de host con GUI** (7º artefacto, preview de forms en el PC): `cd bpgenvm-c &&
      make clean && make LVGL=1` → `build/bpgenvm-c.exe` + `build/SDL2.dll`. Empaqueta
      **ambos juntos** (zip `bpgenvm-c-win.zip`): el exe necesita la DLL a su lado.
      Necesita `third_party/lvgl` + `third_party/SDL2` vendorizados (ver su README).
- [ ] **BpIde shaded jar**: `install` de `miVM` + `lexer-java` y luego `package` de
      `BpIde` (ver memoria `rebuild-bpide-fatjar-tras-frontend`). El pom de `BpIde` ya
      está en `3.0` → el jar sale `BpIde-3.0.jar`.
- [ ] **Captura gráfica**: `docs/img/guicolordemo.png` presente (hero de la portada).

## 2 · Repositorio (lo hace Eduardo)

- [ ] El repo público `legio-e/BasicPlus` ya existe (v2.0). V3 es un `git push` sobre `main`.
- [ ] `.gitignore` cubre `target/`, `**/build/`, `pico-sdk/`, `FreeRTOS-*`,
      `**/managed_components/`, `sdkconfig`, `third_party/{lvgl,SDL2}`, `*.mod/.bpi/.dbg`
      de samples, prefs del IDE. **Revisar** que NO se sube nada gordo/privado (SDKs,
      LVGL/SDL vendorizados, binarios de build).
- [ ] `git push origin main`.

## 3 · GitHub Pages

- [ ] Settings → Pages → Source: branch `main`, carpeta `/docs` (ya configurado en v2).
- [ ] `docs/.nojekyll` presente → sirve estático.
- [ ] Abrir la URL de Pages: `index.html` como portada; comprobar que **la captura de
      GuiColorDemo carga** (`img/guicolordemo.png`), que la **guía gráfica** (`gui.html`,
      ES+EN) renderiza, y que el resto de enlaces (`manual/referencia/guia-ide/
      bp-desde-dentro/opcodes`) siguen bien.

## 4 · Release v3.0

- [ ] Tag `v3.0` sobre el commit publicado.
- [ ] Cuerpo de la release = sección **v3.0** de `docs/RELEASES.md`.
- [ ] Adjuntar los **7 binarios**:
  - `BpIde-3.0.jar`
  - `bpvm_pico.uf2` (Pico 2 + Metro)
  - `bpvm_esp32_merged.bin` (ESP32-S3)
  - `bpvm_esp32p4.bin` (ESP32-P4-EV + Waveshare)
  - `bpvm_stm32_nucleo.bin` (Nucleo-U575)
  - `bpvm_stm32_dk2.bin` (Discovery DK2)
  - `bpgenvm-c-win.zip` (VM-C + LVGL/SDL para preview gráfico en el PC)

## Limitaciones conocidas que van en las notas (no bloquean)

- Cuelgue del FS del pico con `/app` lleno (edge-case) → v4 (`V4_BACKLOG` B-fs-pico-hang).
- `LIST` truncado a ~14 en STM32 (cosmético) → v4 (`V4_BACKLOG` L-list-stm32-trunc).
- DK2 sin rotación de pantalla; modelo lógico ↔ panel físico sin unificar; preview de
  forms `.win` solo por VM-C (la vista Swing no copia `resources/`).
