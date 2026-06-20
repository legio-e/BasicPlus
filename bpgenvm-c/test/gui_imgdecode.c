/*
 * gui_imgdecode.c — verificación HEADLESS del decode de image (V3/H6).
 *
 * Sin abrir ventana: monta el mismo lv_image_dsc_t que gui.c (src VARIABLE con
 * el PNG en RAM) y pide lv_image_decoder_get_info → el decoder lodepng debe
 * reconocerlo y devolver las dimensiones. Valida la cadena set_src→decoder que
 * en la placa pinta los píxeles (no comprobable por dump). Espera: r=0, 8x4.
 */
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>

void lv_lodepng_init(void);

int main(int argc, char** argv) {
    const char* path = (argc > 1) ? argv[1] : "../bpstdlib/testimg.png";
    lv_init();
    lv_lodepng_init();

    FILE* f = fopen(path, "rb");
    if (!f) { printf("ERROR: no se abre %s\n", path); return 1; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t* buf = (uint8_t*) malloc(sz);
    if (fread(buf, 1, (size_t) sz, f) != (size_t) sz) { printf("ERROR: read\n"); return 1; }
    fclose(f);

    lv_image_dsc_t dsc; lv_memzero(&dsc, sizeof(dsc));
    dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc.header.cf    = LV_COLOR_FORMAT_RAW;
    dsc.header.w     = 8;
    dsc.header.h     = 4;
    dsc.data         = buf;
    dsc.data_size    = (uint32_t) sz;

    lv_image_header_t h; lv_memzero(&h, sizeof(h));
    lv_result_t r = lv_image_decoder_get_info(&dsc, &h);
    printf("decoder_get_info: r=%d  w=%d h=%d cf=%d\n", (int) r, (int) h.w, (int) h.h, (int) h.cf);

    int ok = (r == LV_RESULT_OK && h.w == 8 && h.h == 4);
    printf("%s\n", ok ? "IMG DECODE OK" : "IMG DECODE FAIL");
    return ok ? 0 : 2;
}
