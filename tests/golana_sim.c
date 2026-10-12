/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele, based on Felucca's web/emu/native_check.c */
/* Golana on the simulated FM-1 (web/emu/felucca_web.c built natively): the whole firmware, its main loop and the
 * audio ISR, driven by a script of presses and turns; screenshots (PPM) of every page and the WRITE dialog, the
 * keys played, a program written and read back, and checks.
 *   cc -O2 -w -Ibuild/gen -Ifirmware/src -o build/host/golana_sim tests/golana_sim.c -lm
 *   build/host/golana_sim OUTDIR */
#include <stdio.h>
#include "../web/emu/felucca_web.c"

static const char *dir = ".";
static int fails;
static float peak;

static void render(double ms)
{
    uint32_t n = (uint32_t)(ms * 44.1 + 0.5), k, i;
    for (k = 0; k < n; k += 128u) {
        uint32_t m = n - k < 128u ? n - k : 128u;
        web_render(m);
        for (i = 0; i < m; i++) {
            float a = out_l[i] < 0 ? -out_l[i] : out_l[i];
            if (a > peak)
                peak = a;
        }
    }
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
    render(40);
}

static void turn(uint32_t role, int32_t n)
{
    web_enc(role, n);
    render(40);
}

static void check(int ok, const char *what)
{
    if (!ok) {
        printf("FAIL %s\n", what);
        fails++;
    }
}

int main(int argc, char **argv)
{
    uint32_t p;
    char nm[64];
    if (argc > 1)
        dir = argv[1];
    web_nor_erase();
    web_boot();
    render(700);
    shot("00_boot");
    check(trk[0].eng_req == ENGI_GOLANA && trk[0].engine == ENGI_GOLANA, "part 1 plays GOLANA");
    check(gu.prog == 0 && gu.ready, "program 1 at power-on");
    /* every page, by SELECT */
    for (p = 0; p < GU_NPAGE; p++) {
        snprintf(nm, sizeof nm, "%02u_page_%s", p + 1u, GU_SEC[GU_PAGES[gu.page].sec]);
        for (char *c = nm; *c; c++)
            if (*c == ' ')
                *c = '_';
        render(60);
        shot(nm);
        check(gu.page == p, "SELECT walks the pages");
        turn(EN_SELECT, 1);
    }
    check(gu.page == 0, "SELECT wraps");
    /* a key plays: part 1 sounds */
    peak = 0;
    web_keys(1u << 12);
    render(300);
    check(peak > 0.01f, "a key sounds");
    web_keys(0);
    render(800);
    /* the knobs: CUTOFF on PERFORM */
    {
        int16_t c0 = trk[0].p[GL_CUTOFF];
        turn(EN_K1, -5);
        check(trk[0].p[GL_CUTOFF] < c0, "KNOB 1 moves CUTOFF");
        check(gu.dirty, "an edit marks the program");
        render(100);
        shot("30_edited");
    }
    /* the buttons */
    press(B_EDIT);
    check(GU_PAGES[gu.page].sec == SE_OSC1, "EDIT: OSC 1");
    press(B_EDIT);
    check(GU_PAGES[gu.page].sec == SE_OSC2, "EDIT again: OSC 2");
    press(B_FX);
    check(GU_PAGES[gu.page].sec == SE_MFX, "FX: MOD FX");
    press(B_ARP);
    check(trk[0].p[GL_ARPON] == 1 && trk[0].p[P_AMODE] != 0, "ARP on");
    render(100);
    shot("31_arp_on_toast");
    peak = 0;
    web_keys((1u << 0) | (1u << 4) | (1u << 7));
    render(1500);
    check(peak > 0.01f, "the arp plays");
    web_keys(0);
    press(B_ARP);
    check(trk[0].p[GL_ARPON] == 0 && trk[0].p[P_AMODE] == 0, "ARP off");
    render(1500);
    press(B_OCTUP);
    check(song.octave == 1, "OCT+");
    press(B_OCTDN);
    check(song.octave == 0, "OCT-");
    /* PRESETS: another program, back */
    turn(EN_PRESET, 3);
    check(gu.prog == 3 && !gu.dirty, "PRESETS loads program 4");
    render(100);
    shot("32_program4");
    turn(EN_PRESET, -3);
    check(gu.prog == 0, "PRESETS back to 1");
    /* WRITE: an edit into slot 20, named */
    turn(EN_K1, 0);
    press(B_HOME);
    turn(EN_K2, 7);                               /* RESO */
    {
        int16_t reso = trk[0].p[GL_RESO];
        press(B_SAVE);
        check(gu.save, "SAVE opens WRITE");
        turn(EN_PRESET, 19);
        check(gu.save_slot == 19, "PRESETS picks the slot");
        turn(EN_K2, 1);                           /* the first letter one on */
        render(100);
        shot("33_write");
        press(B_SAVE);
        check(!gu.save && gu.prog == 19, "SAVE writes");
        check(gu_user(19) && up_rec(19)->engine == ENGI_GOLANA, "slot 20 holds a Golana program");
        turn(EN_PRESET, -5);
        turn(EN_PRESET, 5);
        check(gu.prog == 19 && trk[0].p[GL_RESO] == reso, "the written program loads back");
        render(100);
        shot("34_written");
    }
    /* MIDI program change 2 -> program 3 */
    web_midi(0xC0, 2, 0);
    render(100);
    check(gu.prog == 2, "Program Change loads program 3");
    /* (UPDATE MODE's countdown is main.c's loop: not in the browser build) */
    printf("%s: %d failures\n", fails ? "FAIL" : "ok", fails);
    return fails ? 1 : 0;
}
