/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele */
/* A slot made by the SOUNDS page (web/ditto_sounds.js, written by web/test_sounds.mjs: an A4 tone, looped, and an E4
 * pluck, at 16 kHz) in USR1 of the whole firmware on a simulated FM-1: the firmware takes its header, the looped zone
 * holds as long as the key, at its pitch, the pluck ends by itself.
 *   cc -DGOLANA=1 -O1 -w -Ibuild/gen -Ifirmware/src -o build/host/sounds_slot_test tests/sounds_slot_test.c -lm
 *   build/host/sounds_slot_test build/sounds_test.slot */
#include <stdio.h>
#include "../web/emu/felucca_web.c"

static int fails;
static void check(int ok, const char *what)
{
    printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    fails += !ok;
}
static float rec[44100 * 4];
static uint32_t nrec;
static void render(double ms, int keep)
{
    uint32_t n = (uint32_t)(ms * 44.1 + 0.5), k, i;
    for (k = 0; k < n; k += 128u) {
        uint32_t m = n - k < 128u ? n - k : 128u;
        web_render(m);
        for (i = 0; keep && i < m && nrec < NELEM(rec); i++)
            rec[nrec++] = out_l[i];
    }
}
static float peak(uint32_t a, uint32_t b)
{
    float p = 0;
    for (; a < b && a < nrec; a++)
        p = rec[a] > p ? rec[a] : -rec[a] > p ? -rec[a] : p;
    return p;
}
/* the period by the zero crossings going up, over [a, b): Hz */
static double hz(uint32_t a, uint32_t b)
{
    uint32_t i, n = 0, first = 0, last = 0;
    for (i = a + 1; i < b; i++)
        if (rec[i - 1] < 0 && rec[i] >= 0) {
            if (!n)
                first = i;
            last = i;
            n++;
        }
    return n > 1 ? 44100.0 * (n - 1) / (last - first) : 0;
}

int main(int argc, char **argv)
{
    FILE *f = fopen(argc > 1 ? argv[1] : "build/sounds_test.slot", "rb");
    size_t n;
    uint32_t v, act;
    if (!f) {
        printf("sounds_slot_test: no slot file (node web/test_sounds.mjs build/sounds_test.slot)\n");
        return 1;
    }
    web_nor_erase();
    n = fread(nor + SMP_USER_BASE, 1, SMP_USER_SIZE, f);
    fclose(f);
    web_boot();
    render(4000, 0);
    check(n > 512 && usr_nz[0] == 2u, "USR1: the page's slot taken, two zones");
    check(usr_zone[0][1].looped && usr_zone[0][1].ls > 0 && !usr_zone[0][0].looped && usr_zone[0][1].rate == 23777u,
          "  the A4 looped, the E4 not, at 16 kHz");
    gu_load(0);
    trk[0].p[DT_SNDA] = (int16_t)DT_NSETS;          /* USR1 */
    trk[0].p[GL_CUTOFF] = 127;
    trk[0].p[P_REV] = trk[0].p[P_DLY] = 0;
    trk[0].p[P_ATK] = 0;                              /* (STRINGS bows slowly: a pluck under it is faint) */
    gl_sync(&trk[0]);
    web_midi(0x90, 69, 110);                          /* A4 held 3 s: its sample, 1.9 s at 16 kHz, would end at 1.9 s .. */
    render(3000, 1);                                  /* .. its loop holds it as long as the key */
    check(peak(44100u * 25u / 10u, 44100u * 3u) > 0.05f, "the A4 still sounds after 2.5 s held (its loop)");
    check(fabs(hz(44100u / 2u, 44100u) - 440.0) < 6.0 && fabs(hz(44100u * 2u, 44100u * 3u) - 440.0) < 6.0, "  at 440 Hz, on the loop too");
    web_midi(0x80, 69, 0);
    render(800, 0);
    nrec = 0;
    web_midi(0x90, 64, 110);                          /* E4 held 2.5 s: the pluck's sample ends, its voice too */
    render(2500, 1);
    for (act = 0, v = 0; v < NVOICE; v++)
        act += trk[0].v[v].active;
    check(peak(0, 44100u / 2u) > 0.05f && act == 0u, "the E4 pluck ends by itself, the key held");
    web_midi(0x80, 64, 0);
    printf("%s: %d failures\n", fails ? "FAIL" : "ok", fails);
    return fails ? 1 : 0;
}
