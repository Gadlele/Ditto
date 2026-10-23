/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele */
/* Ditto's power-on (golana_ui.c includes it; main.c and the browser build run it). */

/* ---------------------------------------------------------------- SPLASH --- */
/* The power-on splash (3 s, NIGHT's colours whatever the palette): a record comes up and turns, the light on its
 * grooves going round; the tone arm swings in and sets the needle down; DITTO in wide heavy letters is uncovered by a
 * wipe and a chrome glint crosses it, and the version comes up. Drawn at ~30 frames a second in two halves of
 * 240 x 120 (the canvas). main.c and the browser build run it (gu_splash_step); a key or a button pressed skips it
 * (the key stays silent: seq.c kb_boot_hold) */
#define GSP_MS 3000u
#define GSP_BG 0x1C346Eu                               /* the panel's blue */
#define GSP_TEXT 0xE8E6E1u
#define GSP_MID 0xAABEE6u
#define GSP_DISC 0x1A1E28u                              /* the vinyl, its grooves, the light on them */
#define GSP_GROOVE 0x252A36u
#define GSP_SHEEN 0x6A7186u
#define GSP_LABEL 0xFFB547u                             /* NIGHT's HOME amber */
#define GSP_ARM 0xC9CCD6u
#define GSP_CX 120
#define GSP_CY 82
#define GSP_R 60
#include "ditto_logo.h"                                /* DITTO, wide and heavy (tools/gen_logo.py) */
static uint16_t gsp_rgb(uint32_t c) { return RGB((c >> 16) & 255u, (c >> 8) & 255u, c & 255u); }
static uint16_t gsp_mix(uint32_t a, uint32_t b, int32_t k)   /* a .. b, k 0..1000 */
{
    uint32_t i, o = 0;
    k = clamp(k, 0, 1000);
    for (i = 0; i < 24u; i += 8u) {
        int32_t x = (int32_t)((a >> i) & 255u), y = (int32_t)((b >> i) & 255u);
        o |= (uint32_t)(x + (y - x) * k / 1000) << i;
    }
    return gsp_rgb(o);
}
static int32_t gsp_ease(int32_t t, int32_t t0, int32_t len)  /* smoothstep of (t - t0) / len, 0..1000 */
{
    int32_t p = clamp((t - t0) * 1000 / len, 0, 1000);
    return p * p / 1000 * (3000 - 2 * p) / 1000;
}
static int32_t gsp_isqrt(int32_t v)                    /* floor(sqrt(v)), v >= 0 */
{
    uint32_t r = 0, b = 1u << 14, x = (uint32_t)v;
    for (; b; b >>= 2) {
        if (x >= r + b) {
            x -= r + b;
            r = (r >> 1) + b;
        } else {
            r >>= 1;
        }
    }
    return (int32_t)r;
}
static void gsp_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)   /* a filled circle, row by row */
{
    int32_t dy;
    for (dy = -r; dy <= r; dy++) {
        int32_t hw = gsp_isqrt(r * r - dy * dy);
        cv_rect(cx - hw, cy + dy, 2 * hw + 1, 1, c);
    }
}
/* the point at radius r, angle ph (a full turn 2^32, 0 to the right, clockwise on the screen) from cx, cy */
static void gsp_at(int32_t cx, int32_t cy, int32_t r, uint32_t ph, int32_t *x, int32_t *y)
{
    *x = cx + ((sine_i(ph + 0x40000000u) * r) >> 15);
    *y = cy + ((sine_i(ph) * r) >> 15);
}
/* the whole picture at t ms, on the canvas (cv_oy: the half drawn) */
static void gsp_draw(int32_t t)
{
    int32_t grow = gsp_ease(t, 0, 700), r = GSP_R * grow / 1000, wipe = gsp_ease(t, 1300, 600) * 240 / 1000, i;
    uint32_t turn = (uint32_t)t * 0x1C71C7u;           /* 33 1/3 turns a minute: 2^32 / 1800 ms, a turn per 1.8 s */
    if (r > 2) {                                        /* the record: grooves, the light on them, the label */
        for (i = r; i > r / 3; i -= 3)
            gsp_disc(GSP_CX, GSP_CY, i, gsp_rgb((i / 3) & 1 ? GSP_DISC : GSP_GROOVE));
        for (i = 0; i < 2; i++) {                       /* two lights opposite, turning with it */
            int32_t k, x0, y0, x1, y1;
            for (k = -2; k <= 2; k++) {
                uint32_t ph = turn + (uint32_t)i * 0x80000000u + (uint32_t)k * 0x01000000u;
                gsp_at(GSP_CX, GSP_CY, r / 3 + 4, ph, &x0, &y0);
                gsp_at(GSP_CX, GSP_CY, r - 3, ph, &x1, &y1);
                cv_line(x0, y0, x1, y1, gsp_mix(GSP_DISC, GSP_SHEEN, 1000 - (k < 0 ? -k : k) * 300));
            }
        }
        gsp_disc(GSP_CX, GSP_CY, r / 3, gsp_rgb(GSP_LABEL));
        {   /* a mark on the label, turning, and the spindle */
            int32_t x, y;
            gsp_at(GSP_CX, GSP_CY, r / 5, turn, &x, &y);
            gsp_disc(x, y, 2, gsp_rgb(GSP_BG));
        }
        gsp_disc(GSP_CX, GSP_CY, 2, gsp_rgb(GSP_TEXT));
    }
    if (t > 500) {                                      /* the tone arm: from its rest (down the right edge) onto the */
        int32_t sw = gsp_ease(t, 500, 700), x, y;      /* record's outer grooves */
        uint32_t ph = 0x40000000u + (uint32_t)sw * 310000u;   /* straight down, then 26 degrees on: the needle */
        gsp_disc(214, 18, 7, gsp_rgb(GSP_MID));                /* near (170, 108), 56 px from the centre */
        gsp_at(214, 18, 100, ph, &x, &y);
        cv_line_t(214, 18, x, y, gsp_rgb(GSP_ARM), 3);
        cv_rect(x - 4, y - 3, 9, 7, gsp_rgb(GSP_ARM));  /* the head */
    }
    if (t > 1300) {                                     /* DITTO, uncovered by the wipe, the glint crossing it */
        int32_t sweep = t < 1800 ? -30 : t > 2300 ? 400 : (t - 1800) * 300 / 500 - 30, x, y;
        int32_t x0 = (240 - LOGO_W) / 2, y0 = 150;
        for (y = 0; y < LOGO_H; y++)
            for (x = 0; x < LOGO_W && x0 + x <= wipe; x++) {
                uint32_t a = (LOGO[(y * LOGO_W + x) >> 1] >> ((x & 1) * 4)) & 15u, c;
                int32_t d = x0 + x - sweep;
                if (!a)
                    continue;
                /* silver until the glint (a white band, 14 px each side) has passed over it, then the text's */
                c = d > -14 && d < 14 ? 0xFFFFFFu : GSP_TEXT;
                cv_rect(x0 + x, y0 + y, 1, 1, d >= 14 ? gsp_mix(GSP_BG, GSP_TEXT, (int32_t)a * 580 / 15)
                                                       : gsp_mix(GSP_BG, c, (int32_t)a * 1000 / 15));
            }
    }
    if (t > 2300) {                                     /* the version, faded in under the name */
#if GSP_NO_VERSION                                       /* the website's GIF (tools/site_shots.sh): no number to keep up */
        static const char V[] = "by Gadlele";
#else
        static const char V[] = FELUCCA_VERSION " \xB7 by Gadlele";
#endif
        cv_text(120 - text_w(&AF_S, V) / 2, 200, &AF_S, V, gsp_mix(GSP_BG, GSP_MID, gsp_ease(t, 2300, 400)));
    }
}
static void gu_splash_frame(uint32_t t)
{
    uint32_t h;
    for (h = 0; h < 2u; h++) {
        cv_begin(240, 120, gsp_rgb(GSP_BG));
        cv_oy = -(int32_t)(h * 120u);
        gsp_draw((int32_t)t);
        cv_oy = 0;
        cv_blit(0, h * 120u);
    }
    lcd_sync();
}
static struct { uint32_t last, notes, btn; } gsp;
static void gu_splash_start(void)
{
    gsp.notes = fm1_in.notes;                           /* (held at power-on: a press after it skips) */
    gsp.btn = fm1_in.buttons;
    gsp.last = 0xFFFFFFFFu;
}
/* t ms into the splash: the frame due drawn (one each 33 ms); 1 = over (its 3 s, or a key / button pressed) */
static int gu_splash_step(uint32_t t)
{
    if (t >= GSP_MS || (fm1_in.notes & ~gsp.notes) || (fm1_in.buttons & ~gsp.btn)) {
        uint32_t cur, ch, k;
        fm1_irq_off();                                  /* the keys down now stay silent until let go, as under the */
        cur = fm1_in.notes;                             /* splash (seq.c keyboard_block, kb_boot_hold): the key that */
        ch = cur ^ kb_prev;                             /* skipped it plays nothing */
        for (k = 0; k < 27u; k++)
            if ((ch & cur) >> k & 1u)
                kb_note[k] = KB_SILENT;
        kb_prev = cur;
        fm1_irq_on();
        return 1;
    }
    gsp.notes &= fm1_in.notes;                          /* (let go: pressed again, it skips) */
    gsp.btn &= fm1_in.buttons;
    if (gsp.last == 0xFFFFFFFFu || t - gsp.last >= 33u) {
        gsp.last = t;
        gu_splash_frame(t);
    }
    return 0;
}
