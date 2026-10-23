/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele */
/* Screenshots of Ditto's screens from the whole firmware on a simulated FM-1 (as the browser build runs it):
 *   cc -O2 -w -Ibuild/gen -Ifirmware/src -o build/host/golana_preview tests/golana_preview.c -lm
 *   build/host/golana_preview OUTDIR       -> OUTDIR/*.ppm */
#include <stdio.h>
#include "../web/emu/felucca_web.c"

static const char *dir = ".";
static void render(double ms)
{
    uint32_t n = (uint32_t)(ms * 44.1 + 0.5), k;
    for (k = 0; k < n; k += 128u)
        web_render(n - k < 128u ? n - k : 128u);
}
static void shot(const char *name)
{
    char path[256];
    FILE *f;
    uint32_t i;
    snprintf(path, sizeof path, "%s/%s.ppm", dir, name);
    if (!(f = fopen(path, "wb")))
        return;
    fprintf(f, "P6\n240 240\n255\n");
    for (i = 0; i < 240u * 240u; i++) {
        uint16_t c = (uint16_t)((web_fb[i] >> 8) | (web_fb[i] << 8));
        uint8_t rgb[3] = {(uint8_t)((c >> 11) << 3), (uint8_t)(((c >> 5) & 63u) << 2), (uint8_t)((c & 31u) << 3)};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}
static void press(uint32_t b)
{
    web_buttons(1u << b);
    render(40);
    web_buttons(0);
    render(60);
}
static void go_page(uint32_t btn, uint32_t nth)
{
    uint32_t i;
    press(B_HOME);
    press(btn);
    for (i = 0; i < nth; i++)
        press(btn);
}
static void setv(uint32_t id, int32_t v) { trk[0].p[id] = (int16_t)v; gl_sync(&trk[0]); }

int main(int argc, char **argv)
{
    char nm[64];
    uint32_t p;
    if (argc > 1)
        dir = argv[1];
    web_nor_erase();
    web_boot();
    render(4000);                                   /* (past the power-on) */
    gu_load(0);                                     /* HOME, a chord held */
    web_keys((1u << 0) | (1u << 7));
    render(600);
    shot("home");
    web_keys(0);
    render(300);
    for (p = 1; p < GU_NPAGE; p++) {                /* every function of every module */
        gu.page = (uint8_t)p;
        render(150);
        snprintf(nm, sizeof nm, "page_%02u", p);
        shot(nm);
    }
    go_page(B_SCL, 0);                              /* a slider just moved: its value on the LCD */
    setv(GL_CUTOFF, 40);
    gu.hot = 0;
    gu.hot_t = fm1_ms;
    render(60);
    shot("slider_moved");
    gu_load(17);                                    /* BASS / PIANO: SPLIT */
    go_page(B_SEQ, 1);
    shot("layer_split");
    go_page(B_REC, 0);                              /* the sequencer's steps on the LCD */
    shot("sequencer");
    press(B_SAVE);
    render(500);
    shot("write");
    printf("done\n");
    return 0;
}
