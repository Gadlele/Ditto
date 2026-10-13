/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele */
/* Golana's drawings (golana_ui.c): above the cards of the pages that have one, a picture of what the page sets, from
 * the values themselves, redrawn as they change:
 *   HOME       the sound playing, from the scope buffer (fx.c scope_buf: what goes out), three ways (GLO SETUP HOME):
 *              RING the wave wrapped round a circle, ORBIT the wave against itself a moment later (a phase portrait:
 *              every timbre draws its own figure), RIDGE the last waves stacked, the newest in front
 *   OSC 1/2    a cycle and a half of the oscillator as its WAVE, CTRL 1, MOD and SEMITONE make it
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

/* -------------------------------------------------------------- OSC 1 / 2 --- */
static int32_t gu_osc1_at(const int16_t *p, uint32_t ph, uint32_t ph2)
{
    int32_t c1 = clamp(p[GL_O1C1], 0, 127);
    switch (clamp(p[GL_O1WAVE], 0, 3)) {
    case 1:
        return ph < 0x80000000u + (uint32_t)c1 * 0xF00000u ? 26000 : -26000;
    case 2: {
        int32_t u = (osc_tri(ph) * ((32768 + c1 * 768) >> 4)) >> 11;
        u = (int32_t)((uint32_t)(u + 32768) & 0x1FFFFu);
        return u < 65536 ? u - 32768 : 98303 - u;
    }
    case 3:
        return osc_sine(ph + (((uint32_t)gl_osc2(0, ph2, 0) * (uint32_t)(c1 * 380)) << 1));
    default: {
        int32_t a = (int32_t)(ph >> 16) - 32768, b = (int32_t)((ph << 1) >> 16) - 32768;
        return a + (((b - a) * c1) >> 8);
    }
    }
}
static void gu_osc(int32_t w, int32_t h, uint32_t two, uint16_t acc, uint16_t surf)
{
    const int16_t *p = gu_t()->p;
    int16_t y[224];
    int32_t i, cy = h / 2, k = h / 2 - 8;
    int32_t semi = clamp(p[GL_O2SEMI], -24, 24), md = clamp(p[GL_O2MOD], 0, 3);
    uint32_t r2 = pitch_inc((uint32_t)(60 * 16 + semi * 16)), r1 = pitch_inc(60u * 16u);
    cv_rect(8, cy, w - 16, 1, ux_mix(surf, acc, 25));
    {
        uint32_t ratio = (r2 >> 8) * 256u / ((r1 >> 16) | 1u);   /* OSC 2 / OSC 1, Q16 (32-bit: no 64-bit divide) */
        for (i = 0; i < 224; i++) {                    /* 1.5 cycles of OSC 1 */
            uint64_t tot = (uint64_t)(uint32_t)i * (0xFFFFFFFFu / 149u);
            uint32_t ph = (uint32_t)tot, ph2;
            int32_t v;
            ph2 = md >= 2 ? (uint32_t)(((uint64_t)ph * ratio) >> 16)     /* SYNC: from OSC 1's start, every cycle */
                          : (uint32_t)((tot * ratio) >> 16);
            if (!two) {
                v = gu_osc1_at(p, ph, ph2);
            } else {
                v = gl_osc2((uint32_t)clamp(p[GL_O2WAVE], 0, 2), ph2, 0);
                if (md == 1 || md == 3)
                    v = mulq15(v, gu_osc1_at(p, ph, ph2));
            }
            y[i] = (int16_t)(cy - (clamp(v, -32767, 32767) * k >> 15));
        }
    }
    for (i = 1; i < 224; i++) {
        cv_line(8 + i - 1, y[i - 1], 8 + i, y[i], acc);
        cv_line(8 + i - 1, y[i - 1] + 1, 8 + i, y[i] + 1, acc);
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
    static const uint8_t IDS[] = {GL_O1WAVE, GL_O1C1, GL_O2WAVE, GL_O2MOD, GL_O2SEMI, GL_FTYPE, GL_CUTOFF, GL_RESO,
                                  GL_FA, GL_FD, GL_FS, GL_FR, P_ATK, P_DEC, P_SUS, P_REL, GL_L1WAVE, GL_L1FREQ, GL_L1SYNC,
                                  GL_L2WAVE, GL_L2FREQ, GL_L2SYNC};
    gu_frame++;
    for (i = 0; i < sizeof IDS; i++)
        sig = gu_hashv(sig, p[IDS[i]]);
    sig = gu_hashv(sig, (int32_t)(g | gu_pi << 8 | gu_view << 12));
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
    case GG_OSC1: case GG_OSC2: gu_osc(w, h, g == GG_OSC2, acc, surf); break;
    case GG_FILT: gu_filter(w, h, acc, surf); break;
    case GG_FEG: case GG_AEG: gu_eg(w, h, g == GG_AEG, acc, surf); break;
    default: gu_lfo(w, h, g == GG_LFO2, acc, surf); break;
    }
    cv_blit(0, 60);
}
