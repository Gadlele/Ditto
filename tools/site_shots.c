/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele */
/* The website's pictures (web/site/img), from the whole firmware on a simulated FM-1 (as tests/golana_preview.c):
 * the gallery's screens (project.json "gallery"), the power-on's frames for splash.gif and the 64 px icon.
 *   cc -DGOLANA=1 -O1 -w -Ibuild/gen -Ifirmware/src -o build/host/site_shots tools/site_shots.c -lm
 *   build/host/site_shots OUTDIR          -> OUTDIR/NAME.ppm, OUTDIR/splash_NNN.ppm
 * tools/site_shots.sh turns them into web/site/img's PNGs and GIF. */
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
static void page(uint32_t btn, uint32_t nth)
{
    uint32_t i;
    press(B_HOME);
    press(btn);
    for (i = 0; i < nth; i++)
        press(btn);
    render(150);
}

int main(int argc, char **argv)
{
    uint32_t t;
    char nm[64];
    if (argc > 1)
        dir = argv[1];
    web_nor_erase();
    web_boot();
    for (t = 0; t < 3000u; t += 50u) {              /* the splash, a frame each 50 ms */
        gu_splash_frame(t);
        snprintf(nm, sizeof nm, "splash_%03u", t / 50u);
        shot(nm);
    }
    render(3500);
    gu_load(1);                                      /* CHOIR: a chord held, HOME drawing it */
    web_keys((1u << 7) | (1u << 11) | (1u << 14));
    render(1500);
    shot("home_notes");
    web_keys(0);
    render(600);
    gu_load(0);                                      /* STRINGS */
    page(B_EDIT, 0);
    shot("edit");
    gu_load(16);                                     /* BOW + PLUCK: SOUND B, VEL X */
    page(B_EDIT, 1);
    shot("sndb");
    gu_load(20);                                     /* EMU 1 KEYS: the character */
    page(B_EDIT, 2);
    shot("mixer");
    gu_load(4);                                      /* BRASS: the filter, its EG */
    page(B_SCL, 0);
    shot("scl");
    page(B_ENV, 0);
    shot("env");
    gu_load(5);                                      /* FLUTE: the vibrato */
    page(B_LFO, 0);
    render(400);
    shot("lfo");
    gu_load(0);
    page(B_LFO, 3);                                  /* PATCH 2: VELO -> CUTOFF */
    shot("patch");
    gu_load(1);
    page(B_FX, 2);                                   /* the choir's hall */
    shot("fx");
    gu_load(21);                                     /* AIR CHOIR: START */
    page(B_EDIT, 0);
    shot("start");
    press(B_SAVE);
    render(200);
    shot("save");
    press(B_HOME);
    printf("site shots: %s\n", dir);
    return 0;
}
