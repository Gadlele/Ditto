/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele */
/* Ditto's drawings (golana_ui.c): above the cards of the pages that have one, a picture of what the page sets, from
 * the values themselves, redrawn as they change:
 *   HOME       the sound playing, from the scope buffer (fx.c scope_buf: what goes out), three ways (GLO SETUP HOME):
 *              RING the wave wrapped round a circle, ORBIT the wave against itself a moment later (a phase portrait:
 *              every timbre draws its own figure), RIDGE the last waves stacked, the newest in front
 *   SOUND A/B  the sample the middle C plays, whole: its peaks, the loop shaded, START marked
 *   FILTER     the response, dB over a log frequency axis (30 Hz .. 16 kHz), the cutoff marked
 *   FILTER EG, AMP EG   the four stages, their lengths as the times, the sustain as its level
 *   LFO 1/2    the wave, as many cycles as its rate, running
 * Integer only (no FPU); the live ones (HOME, the LFOs) redraw every GU_LIVE_FRAMES frames, the others on a change. */

enum { GG_NONE, GG_HOME, GG_OSC1, GG_OSC2, GG_FILT, GG_FEG, GG_AEG, GG_LFO1, GG_LFO2 };
#define GU_LIVE_FRAMES 3u

static uint32_t gu_graph_of(const gu_page_t *pg)
{
    switch (pg->k[0].id) {
    case GL_CUTOFF: return pg->btn == B_HOME ? GG_HOME : GG_NONE;
    case GL_O1WAVE: return GG_OSC1;
    case GL_O2WAVE: return GG_OSC2;
    case GL_FTYPE: return GG_FILT;
    case GL_FA: return GG_FEG;
    case P_ATK: return GG_AEG;
    case GL_L1WAVE: return GG_LFO1;
    case GL_L2WAVE: return GG_LFO2;
    default: return GG_NONE;
    }
}
static int32_t gu_graph_h(uint32_t g) { return g == GG_HOME ? 84 : g ? 72 : 0; }

/* a polyline of n points (x[], y[]) 2 px thick */
static void gu_poly(const int16_t *x, const int16_t *y, uint32_t n, uint16_t c)
{
    uint32_t i;
    for (i = 1; i < n; i++) {
        cv_line(x[i - 1], y[i - 1], x[i], y[i], c);
        cv_line(x[i - 1] + 1, y[i - 1], x[i] + 1, y[i], c);
    }
}
/* a curve over the columns x0..x0+n-1 (y[] per column), with the area under it down to yb tinted */
static void gu_curve(int32_t x0, const int16_t *y, uint32_t n, int32_t yb, uint16_t c, uint16_t tint)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        if (y[i] < yb)
            cv_rect(x0 + (int32_t)i, y[i], 1, yb - y[i], tint);
    for (i = 1; i < n; i++) {
        cv_line(x0 + (int32_t)i - 1, y[i - 1], x0 + (int32_t)i, y[i], c);
        cv_line(x0 + (int32_t)i - 1, y[i - 1] + 1, x0 + (int32_t)i, y[i] + 1, c);
    }
}

/* ------------------------------------------------------------------ HOME --- */
static struct {
    int32_t gain;                 /* the auto gain, Q8 (the scope's quiet sounds made visible) */
    int16_t ridge[7][56];         /* RIDGE: the last waves, [0] the oldest */
    uint8_t nridge, tick;
} gh;

/* the last n scope samples from a rising zero crossing (a still picture of a steady wave) -> s[]; their peak */
static int32_t gu_scope_take(int16_t *s, uint32_t n)
{
    uint32_t w = scope_w, i, start = w - n - 1u, best = start;
    int32_t pk = 0;
    for (i = 0; i < 160u; i++) {                       /* a rising crossing in the 160 samples before */
        uint32_t a = start - i;
        if (scope_buf[(a - 1u) & (SCOPE_N - 1u)] < 0 && scope_buf[a & (SCOPE_N - 1u)] >= 0) {
            best = a;
            break;
        }
    }
    for (i = 0; i < n; i++) {
        s[i] = scope_buf[(best + i) & (SCOPE_N - 1u)];
        pk = s[i] > pk ? s[i] : -s[i] > pk ? -s[i] : pk;
    }
    return pk;
}
/* the scale for peak pk, Q8, glided (louder at once, quieter slowly): a full picture whatever the level */
static int32_t gu_scope_gain(int32_t pk)
{
    int32_t want = pk > 400 ? (24000 << 8) / pk : (24000 << 8) / 400;
    if (want > 16 << 8)
        want = 16 << 8;
    gh.gain = !gh.gain || want < gh.gain ? want : gh.gain + ((want - gh.gain) >> 4);
    return gh.gain;
}

static void gu_home_ring(int32_t w, int32_t h, uint16_t acc, uint16_t surf)
{
    int16_t s[128], x[129], y[129];
    int32_t g = gu_scope_gain(gu_scope_take(s, 128)), cx = w / 2, cy = h / 2, r0 = h / 2 - 16, i;
    uint32_t a;
    for (a = 0; a < 64u; a++) {                        /* the circle at rest, dotted */
        uint32_t ph = a << 26;
        cv_pset(cx + (r0 * osc_sine(ph + 0x40000000u) >> 15), cy + (r0 * osc_sine(ph) >> 15), ux_mix(surf, acc, 40));
    }
    for (i = 0; i <= 128; i++) {
        int32_t v = clamp((s[i & 127] * g) >> 8, -32767, 32767), r = r0 + (v * 13 >> 15);
        uint32_t ph = (uint32_t)i << 25;               /* 128 points: a whole turn */
        x[i] = (int16_t)(cx + (r * osc_sine(ph + 0x40000000u) >> 15));
        y[i] = (int16_t)(cy + (r * osc_sine(ph) >> 15));
    }
    gu_poly(x, y, 129, acc);
}

static void gu_home_orbit(int32_t w, int32_t h, uint16_t acc, uint16_t surf)
{
    int16_t s[200], x[192], y[192];
    int32_t g = gu_scope_gain(gu_scope_take(s, 200)), cx = w / 2, cy = h / 2, k = h / 2 - 6, i;
    cv_rect(cx - k, cy, 2 * k, 1, ux_mix(surf, acc, 25));
    cv_rect(cx, cy - k, 1, 2 * k, ux_mix(surf, acc, 25));
    for (i = 0; i < 192; i++) {                        /* x: the wave now, y: 8 samples (0.36 ms) later */
        int32_t a = clamp((s[i] * g) >> 8, -32767, 32767), b = clamp((s[i + 8] * g) >> 8, -32767, 32767);
        x[i] = (int16_t)(cx + (a * (k + 20) >> 15));
        y[i] = (int16_t)(cy - (b * k >> 15));
    }
    gu_poly(x, y, 96, ux_mix(surf, acc, 55));          /* the older half fainter */
    gu_poly(x + 95, y + 95, 97, acc);
}

static void gu_home_ridge(int32_t w, int32_t h, uint16_t acc, uint16_t surf)
{
    int16_t s[224], y[56];
    int32_t g, j, i, step = (h - 24) / 6, amp = step + 6;
    if (!(gh.tick++ & 1u) || !gh.nridge) {             /* a new wave every other drawing */
        g = gu_scope_gain(gu_scope_take(s, 224));
        if (gh.nridge == 7u)
            for (j = 0; j < 6; j++)                     /* (no memmove in the firmware's libc) */
                memcpy(gh.ridge[j], gh.ridge[j + 1], sizeof gh.ridge[0]);
        else
            gh.nridge++;
        for (i = 0; i < 56; i++)
            gh.ridge[gh.nridge - 1u][i] = (int16_t)clamp((s[i * 4] * g) >> 8, -32767, 32767);
    }
    for (j = 0; j < (int32_t)gh.nridge; j++) {         /* the oldest at the top, behind; each one hides what is under */
        int32_t base = 14 + (j + 7 - (int32_t)gh.nridge) * step;
        uint16_t c = ux_mix(surf, acc, 30 + j * 10);
        for (i = 0; i < 56; i++) {
            int32_t v = gh.ridge[j][i], env = i < 8 ? i : i > 47 ? 55 - i : 8;   /* the ends calm, as the cover's */
            y[i] = (int16_t)(base - ((v < 0 ? -v : v) * amp * env >> 18) - (v * amp >> 17));
        }
        for (i = 0; i < 55; i++) {
            int32_t x0 = 8 + i * (w - 16) / 55, x1 = 8 + (i + 1) * (w - 16) / 55, xx;
            for (xx = x0; xx < x1; xx++) {
                int32_t yy = y[i] + (y[i + 1] - y[i]) * (xx - x0) / (x1 - x0);
                cv_rect(xx, yy, 1, h - yy, surf);      /* (the painter's way: the band under the line hidden) */
            }
            cv_line(x0, y[i], x1, y[i + 1], c);
            cv_line(x0, y[i] + 1, x1, y[i + 1] + 1, c);
        }
    }
}

/* ---------------------------------------------------------- SOUND A / B --- */
/* the sample the middle C plays (SOUND B: its sounding key), the whole of it: its peaks per column, the loop shaded,
 * START marked; OFF or an empty user slot said in words */
static void gu_sample(int32_t w, int32_t h, uint32_t two, uint16_t acc, uint16_t surf)
{
    const int16_t *p = gu_t()->p;
    int32_t si = two ? p[DT_SNDB] - 1 : p[DT_SNDA], cy = h / 2 + 6, k = h / 2 - 14, x, lo = 0, hi = 0;
    uint32_t zi, i, col = 0, nc = (uint32_t)w - 16u, loop = p[DT_LOOP] != 0;
    const smp_zone_t *z;
    fo_lay_t l;
    if (si < 0 || si >= DT_NSND) {
        cv_text_on((w - text_w(&AF_M, "OFF")) / 2, cy - 10, &AF_M, "OFF", GC_DIM, surf);
        return;
    }
    zi = fo_pick((uint32_t)si, two ? clamp(60 + p[DT_BSEMI], 0, 127) : 60);
    z = zi == 0xFFFFu ? 0 : fo_zone(zi);
    if (!z || !z->n) {
        cv_text_on((w - text_w(&AF_M, "EMPTY SLOT")) / 2, cy - 10, &AF_M, "EMPTY SLOT", GC_DIM, surf);
        return;
    }
    {   /* above the wave: the zone's root and length left, LOOP right (over the loop, shaded below) */
        static const char *const NN[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        char b[16];
        int32_t r = (z->root16 + 8) >> 4, ms = (int32_t)((uint64_t)z->n * 1000u * 65536u / 44100u / z->rate);
        str_cpy(b, NN[clamp(r, 0, 127) % 12], 4);
        fmt_int(b + str_len(b), clamp(r, 0, 127) / 12 - 1);
        str_cpy(b + str_len(b), "  ", 4);
        fmt_int(b + str_len(b), ms / 1000);
        str_cpy(b + str_len(b), ".", 2);
        fmt_int(b + str_len(b), ms % 1000 / 100);
        str_cpy(b + str_len(b), "S", 2);
        cv_text_on(10, 2, &AF_S, b, GC_MID, surf);
        if (z->looped && loop) {
            int32_t x0 = 8 + (int32_t)((uint64_t)z->ls * nc / z->n), x1 = 8 + (int32_t)((uint64_t)z->le * nc / z->n);
            cv_rect(x0, 20, x1 - x0, h - 24, ux_mix(surf, acc, 18));
            cv_text_on(x1 - text_w(&AF_S, "LOOP"), 2, &AF_S, "LOOP", ux_mix(surf, acc, 80), surf);
        }
    }
    cv_rect(8, cy, w - 16, 1, ux_mix(surf, acc, 25));
    memset(&l, 0, sizeof l);
    l.on = 1;
    for (i = 0; i < z->n; i++) {                        /* the whole zone decoded: its peaks per column */
        int32_t v = fo_next(&l, z, fo_data(zi), 0, FC_CLEAN);
        uint32_t c = (uint32_t)((uint64_t)i * nc / z->n);
        if (c != col) {
            cv_rect(8 + (int32_t)col, cy - (hi * k >> 15), 1, ((hi - lo) * k >> 15) + 1, acc);
            col = c;
            lo = hi = 0;
        }
        lo = v < lo ? v : lo;
        hi = v > hi ? v : hi;
    }
    cv_rect(8 + (int32_t)col, cy - (hi * k >> 15), 1, ((hi - lo) * k >> 15) + 1, acc);
    if (p[DT_START] > 0 && zi < 0x8000u) {              /* START (as fo_start places it) */
        uint32_t lim = z->looped && loop ? z->le : z->n - z->n / 8u, at = (uint32_t)p[DT_START] * lim / 128u;
        x = 8 + (int32_t)((uint64_t)at * nc / z->n);
        cv_rect(x, 20, 2, h - 24, GC_TEXT);
    }
}

/* ----------------------------------------------------------------- FILTER --- */
static void gu_filter(int32_t w, int32_t h, uint16_t acc, uint16_t surf)
{
    const int16_t *p = gu_t()->p;
    int16_t y[224];
    int32_t ft = clamp(p[GL_FTYPE], 0, 4), cut = clamp(p[GL_CUTOFF], 0, 127), res = clamp(p[GL_RESO], 0, 127);
    int32_t xc = cut * 223 / 127, i, top = 8, zero = top + 18;   /* 0 dB at zero; 1 px = 1 dB */
    int32_t peak = ft == 4 ? res * res / 420 : res * res / 640;  /* the resonance, dB */
    int32_t slope = ft == 0 ? 24 : 12;                  /* dB per octave; 26.6 px an octave */
    for (i = 0; i < 6; i++)                             /* octave lines */
        cv_rect(8 + i * 40, top, 1, h - top - 6, ux_mix(surf, acc, 12));
    cv_rect(8, zero, w - 16, 1, ux_mix(surf, acc, 25));
    for (i = 0; i < 224; i++) {
        int32_t d = i - xc, db, bump;
        bump = d > -14 && d < 14 ? peak * (196 - d * d) / 196 : 0;   /* the resonance: a hump round the cutoff */
        if (ft == 3)                                    /* HPF */
            db = (d < 0 ? d * slope * 10 / 266 : 0) + bump;
        else if (ft == 2)                               /* BPF: 6 dB / octave each side, its peak the resonance */
            db = -(d < 0 ? -d : d) * 60 / 266 + bump - 6 + res / 12;
        else
            db = (d > 0 ? -d * slope * 10 / 266 : 0) + bump;
        y[i] = (int16_t)clamp(zero - db, top, h - 6);
    }
    gu_curve(8, y, 224, h - 6, acc, ux_mix(surf, acc, 16));
    cv_rect(8 + xc, top, 1, h - top - 6, ux_mix(surf, acc, 60));
}

/* ----------------------------------------------------------------- the EGs --- */
static void gu_eg(int32_t w, int32_t h, uint32_t amp, uint16_t acc, uint16_t surf)
{
    const int16_t *p = gu_t()->p;
    int16_t y[224];
    int32_t a = clamp(p[amp ? P_ATK : GL_FA], 0, 127), d = clamp(p[amp ? P_DEC : GL_FD], 0, 127);
    int32_t s = clamp(p[amp ? P_SUS : GL_FS], 0, 127), r = clamp(p[amp ? P_REL : GL_FR], 0, 127);
    int32_t wa = 2 + a * 56 / 127, wd = 3 + d * 56 / 127, wr = 3 + r * 56 / 127, ws, top = 8, bot = h - 8, i, n = 0;
    int32_t yt = bot - top, ys = bot - s * yt / 127, lv;
    ws = 224 - wa - wd - wr;
    for (i = 0; i < wa && n < 224; i++)                 /* attack: linear */
        y[n++] = (int16_t)(bot - (i + 1) * yt / wa);
    lv = 32767;                                         /* decay, release: exponential, ~99 % over their length */
    for (i = 0; i < wd && n < 224; i++) {
        lv -= lv * 150 / (wd * 32 + 1);
        y[n++] = (int16_t)(ys - (ys - top) * lv / 32767);
    }
    for (i = 0; i < ws && n < 224; i++)
        y[n++] = (int16_t)ys;
    lv = 32767;
    for (i = 0; i < wr && n < 224; i++) {
        lv -= lv * 150 / (wr * 32 + 1);
        y[n++] = (int16_t)(bot - (bot - ys) * lv / 32767);
    }
    while (n < 224)
        y[n++] = (int16_t)bot;
    gu_curve(8, y, 224, bot, acc, ux_mix(surf, acc, 16));
    cv_rect(8 + wa + wd, top, 1, bot - top, ux_mix(surf, acc, 35));            /* the key let go */
    cv_rect(8 + wa + wd + ws, top, 1, bot - top, ux_mix(surf, acc, 35));
}

/* ------------------------------------------------------------------ LFOs --- */
static uint32_t gu_lfo_ph[2], gu_lfo_n[2];
static void gu_lfo(int32_t w, int32_t h, uint32_t two, uint16_t acc, uint16_t surf)
{
    const int16_t *p = gu_t()->p;
    int16_t y[224];
    uint32_t wave = (uint32_t)clamp(p[two ? GL_L2WAVE : GL_L1WAVE], 0, 4), i;
    int32_t f = clamp(p[two ? GL_L2FREQ : GL_L1FREQ], 0, 127), sync = p[two ? GL_L2SYNC : GL_L1SYNC];
    uint32_t cyc = sync ? 2u : 1u + (uint32_t)f / 32u, inc = 0xFFFFFFFFu / 224u * cyc;
    int32_t cy = h / 2, k = h / 2 - 9;
    {
        uint32_t old = gu_lfo_ph[two];
        gu_lfo_ph[two] += LFO_INC[f] * 8u;              /* it runs (a slow LFO slowly) */
        if (gu_lfo_ph[two] < old)
            gu_lfo_n[two]++;
    }
    cv_rect(8, cy, w - 16, 1, ux_mix(surf, acc, 25));
    {
        uint32_t prev = gu_lfo_ph[two], c = gu_lfo_n[two];
        for (i = 0; i < 224u; i++) {
            uint32_t ph = gu_lfo_ph[two] + i * inc;
            if (ph < prev)
                c++;                                    /* a new cycle: S&H a new value */
            prev = ph;
            y[i] = (int16_t)(cy - (gl_wave(wave, ph, (int32_t)((c * 2654435761u) >> 16) - 32768) * k >> 15));
        }
    }
    for (i = 1; i < 224u; i++) {
        cv_line(8 + (int32_t)i - 1, y[i - 1], 8 + (int32_t)i, y[i], acc);
        cv_line(8 + (int32_t)i - 1, y[i - 1] + 1, 8 + (int32_t)i, y[i] + 1, acc);
    }
}

/* the page's drawing (y 60 .. 60 + its height), when something it shows changed or it is a live one */
static uint32_t gu_sig_graph, gu_frame;
static void gu_draw_graph(uint32_t g)
{
    const int16_t *p = gu_t()->p;
    int32_t h = gu_graph_h(g), w = 240;
    uint16_t acc = gu_btn_col(GU_PAGES[gu.page].btn), surf = GC_SURF;
    uint32_t sig = 0x811C9DC5u, live = g == GG_HOME || g == GG_LFO1 || g == GG_LFO2, i;
    static const uint8_t IDS[] = {GL_O1WAVE, GL_O1C1, GL_O1C2, GL_O2WAVE, GL_O2MOD, GL_O2SEMI, GL_FTYPE, GL_CUTOFF, GL_RESO,
                                  GL_FA, GL_FD, GL_FS, GL_FR, P_ATK, P_DEC, P_SUS, P_REL, GL_L1WAVE, GL_L1FREQ, GL_L1SYNC,
                                  GL_L2WAVE, GL_L2FREQ, GL_L2SYNC};
    gu_frame++;
    for (i = 0; i < sizeof IDS; i++)
        sig = gu_hashv(sig, p[IDS[i]]);
    sig = gu_hashv(sig, (int32_t)(g | gu_pi << 8 | gu_view << 12 | smp_user_gen << 16));   /* (an upload) */
    if (live ? gu_frame % GU_LIVE_FRAMES && sig == gu_sig_graph : sig == gu_sig_graph)
        return;
    gu_sig_graph = sig;
    cv_begin((uint32_t)w, (uint32_t)h, GC_BG);
    cv_rrect(3, 0, w - 6, h, -10, surf, GC_BG);
    switch (g) {
    case GG_HOME:
        if (gu_view == 1u)
            gu_home_orbit(w, h, acc, surf);
        else if (gu_view == 2u)
            gu_home_ridge(w, h, acc, surf);
        else
            gu_home_ring(w, h, acc, surf);
        break;
    case GG_OSC1: case GG_OSC2: gu_sample(w, h, g == GG_OSC2, acc, surf); break;
    case GG_FILT: gu_filter(w, h, acc, surf); break;
    case GG_FEG: case GG_AEG: gu_eg(w, h, g == GG_AEG, acc, surf); break;
    default: gu_lfo(w, h, g == GG_LFO2, acc, surf); break;
    }
    cv_blit(0, 60);
}

/* ---------------------------------------------------------------- SPLASH --- */
/* The power-on splash (3 s, NIGHT's colours whatever the palette): a record comes up and turns, the light on its
 * grooves going round; the tone arm swings in and sets the needle down; DITTO in block letters is uncovered by a
 * wipe and a chrome glint crosses it, and the version comes up. Drawn at ~30 frames a second in two halves of
 * 240 x 120 (the canvas). main.c and the browser build run it (gu_splash_step); a key or a button pressed skips it
 * (the key stays silent: seq.c kb_boot_hold) */
#define GSP_MS 3000u
#define GSP_BG 0x0E1118u
#define GSP_TEXT 0xE8E6E1u
#define GSP_MID 0x969CACu
#define GSP_DISC 0x1A1E28u                              /* the vinyl, its grooves, the light on them */
#define GSP_GROOVE 0x252A36u
#define GSP_SHEEN 0x6A7186u
#define GSP_LABEL 0xFFB547u                             /* NIGHT's HOME amber */
#define GSP_ARM 0xC9CCD6u
#define GSP_CX 120
#define GSP_CY 82
#define GSP_R 60
static const uint8_t GSP_GLYPH[5][7] = {                /* D I T T O, 5 x 7 (bit 4 the left column) */
    {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}, {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
};
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
    if (t > 1300) {                                     /* DITTO: 6 x 6 px blocks (5 lit), 36 px a letter, from 33 */
        int32_t sweep = t < 1800 ? -30 : t > 2300 ? 400 : (t - 1800) * 300 / 500 - 30, k, x, y;
        for (k = 0; k < 5; k++)
            for (y = 0; y < 7; y++)
                for (x = 0; x < 5; x++) {
                    int32_t px = 33 + k * 36 + x * 6, d = px - sweep;
                    if (!((GSP_GLYPH[k][y] >> (4 - x)) & 1u) || px > wipe)
                        continue;
                    /* silver until the glint (a white band, 14 px each side) has passed over it, then the text's */
                    cv_rect(px, 156 + y * 6, 5, 5, d > -14 && d < 14 ? gsp_mix(GSP_TEXT, 0xFFFFFFu, 1000 - (d < 0 ? -d : d) * 1000 / 14)
                                                  : d >= 14 ? gsp_mix(GSP_TEXT, GSP_BG, 420) : gsp_rgb(GSP_TEXT));
                }
    }
    if (t > 2300) {                                     /* the version, faded in under the name */
        static const char V[] = FELUCCA_VERSION " \xB7 by Gadlele";
        cv_text(120 - text_w(&AF_S, V) / 2, 206, &AF_S, V, gsp_mix(GSP_BG, GSP_MID, gsp_ease(t, 2300, 400)));
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
