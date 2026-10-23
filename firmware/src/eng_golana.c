/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele, based on Felucca (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* DITTO: the one engine of Ditto, a sampler after the E-mu Emulator II (1984): two sounds of its library
 * layered, each through the character of an eighties sampler, into an analog-style filter, two envelopes and LFOs.
 * (The file keeps the name of Golana's engine, whose frame it shares: the filter, the EGs, the LFOs, the patch,
 * DRIFT, the arpeggiator and the effects are Golana's; the oscillators are gone.)
 *
 *   SOUND A  a sound of the library (ditto_samples.h: PIANO, STRINGS, CHOIR, BRASS ..) or a user slot USR1-3 (the
 *            web editor's samples, eng_sample.c); START, where the note starts in the sample (a built-in sound:
 *            0 the beginning .. 127 near its end); LOOP OFF plays a sustained sound once through
 *   SOUND B  OFF or a second sound, SEMI +-24, TUNE +-63 ct; LAYER: STACK (both), VEL X (soft notes A, hard B,
 *            crossfaded by the velocity), SPLIT (B below the SPLIT key, A from it)
 *   MIXER    A, B and NOISE levels into the filter; CHAR the sampler's character:
 *            EII    8-bit companded (mu-law) samples, no interpolation: each sample held until the next, the grain
 *                   and the aliasing of a variable-clock DAC (the Emulator II)
 *            EMU 1  8-bit linear, no interpolation: rougher, the quiet tails grittier (the Emulator I)
 *            CLEAN  the recordings as stored (16-bit, interpolated)
 *   FILTER   LPF24 (the Emulator II's SSM 24 dB low-pass), LPF12, BPF, HPF, MS20; CUTOFF, RESO, EG INT, KEY TRACK
 *   FILTER EG, AMP, LFO 1/2, DRIFT, PATCH as Golana's: PATCH destinations PITCH, B PITCH (SOUND B's), BAL (+ more B,
 *            - more A: VELO -> BAL is a velocity crossfade, an LFO a slow one), NOISE, CUTOFF, AMP, LFO2F
 * The slots: core.h GL_* (Golana's names) and DT_* (what they mean here). At most 8 voices. */

#include "ditto_samples.h"                       /* the library (tools/gen_ditto_samples.py) */
static const char *const N_FOSND[] = {DT_SET_NAMES_INIT, "USR1", "USR2", "USR3"};
static const char *const N_FOSNDB[] = {"OFF", DT_SET_NAMES_INIT, "USR1", "USR2", "USR3"};
#define DT_NSND (DT_NSETS + SMP_USER_SLOTS)          /* SOUND A's choices; SOUND B's: OFF and these */
static const char *const N_FOLAYER[] = {"STACK", "VEL X", "SPLIT"};
static const char *const N_FOCHAR[] = {"EII", "EMU 1", "CLEAN"};
static const char *const N_GFTYPE[] = {"LPF24", "LPF12", "BPF", "HPF", "MS20"};
static const char *const N_GLWAVE[] = {"SAW", "SQR", "TRI", "SIN", "S&H"};
static const char *const N_GLKEY[] = {"OFF", "TIMBRE", "VOICE"};
static const char *const N_GLSYNC[] = {"OFF", "4BAR", "2BAR", "1/1", "1/2", "1/4", "1/8", "1/8T", "1/16", "16T", "1/32"};
static const uint8_t GL_SYNC_DIV[11] = {0, 9, 8, 7, 6, 0, 1, 4, 2, 5, 3};   /* N_GLSYNC -> fx.c div_samples */
static const char *const N_GPSRC[] = {"FEG", "AEG", "LFO1", "LFO2", "VELO", "KBD"};
static const char *const N_GPDST[] = {"PITCH", "B PITCH", "BAL", "NOISE", "CUTOFF", "AMP", "LFO2F"};
static const char *const N_GARPT[] = {"UP", "DOWN", "ALT1", "ALT2", "RANDOM", "TRIGGER",
                                      "ORDER", "CONVERGE", "PEDAL", "DOUBLE"};
static const char *const N_GDTYPE[] = {"STEREO", "CROSS", "L/R"};
static const char *const N_GRTYPE[] = {"ROOM", "SPRING", "HALL"};
static const char *const N_GMFX[] = {"OFF", "CHORUS", "FLANGER", "ENSEMBL", "PHASER", "METAL"};
static const char *const N_GROUTE[] = {"MFX>DLY>REV", "DLY>MFX>REV", "DLY>REV>MFX"};
static const char *const N_GONOFF[] = {"OFF", "ON"};
static const char *const N_GDTIME[] = {"1/4", "1/8", "1/16", "1/32", "1/8T", "16T", "1/2", "1/1", "2BAR", "4BAR",
                                       "1/8D", "16D", "1/4D"};
enum { GS_FEG, GS_AEG, GS_LFO1, GS_LFO2, GS_VELO, GS_KBD, GS_N };
enum { GD_PITCH, GD_BPITCH, GD_BAL, GD_NOISE, GD_CUT, GD_AMP, GD_LFO2F, GD_N };
enum { FL_STACK, FL_VELX, FL_SPLIT };
enum { FC_EII, FC_EMU1, FC_CLEAN };

#define GPD(l, f, mn, mx, df) {l, f, mn, mx, df, 0, 0}
#define GPE(l, n, df) {l, F_ENUM, 0, (int16_t)(sizeof(n) / sizeof(n[0]) - 1), df, n, 0}
/* Ditto's meaning of the slots it borrows (core.h GL_* / DT_*): params.c param_desc_of returns these for the
 * engine (label 0: the slot keeps Felucca's own descriptor) */
static const param_desc_t GOL_TP[P_COUNT] = {
    [DT_LVLA] = GPD("LVL A", F_PCT, 0, 127, 127),
    [DT_CHAR] = GPE("CHAR", N_FOCHAR, FC_EII),
    [DT_LVLB] = GPD("LVL B", F_PCT, 0, 127, 100),
    [GL_NOISE] = GPD("NOISE", F_PCT, 0, 127, 0),
    [GL_FTYPE] = GPE("TYPE", N_GFTYPE, 0),
    [GL_RESO] = GPD("RESO", F_PCT, 0, 127, 10),
    [GL_FEGINT] = GPD("EGINT", F_BIPCT, -63, 63, 0),
    [GL_KTRK] = GPD("KEYTR", F_BIPCT, -63, 63, 32),
    [GL_FA] = GPD("ATK", F_TIME, 0, 127, 0),
    [GL_FD] = GPD("DEC", F_TIME, 0, 127, 64),
    [GL_FS] = GPD("SUS", F_PCT, 0, 127, 127),
    [GL_FR] = GPD("REL", F_TIME, 0, 127, 50),
    [GL_DIST] = GPE("DIST", N_GONOFF, 0),
    [GL_EGRESET] = GPE("RESET", N_GONOFF, 0),
    [GL_L1WAVE] = GPE("WAVE", N_GLWAVE, 2),
    [GL_L1FREQ] = GPD("FREQ", F_LFOHZ, 0, 127, 85),
    [GL_L1KEY] = GPE("KEY", N_GLKEY, 1),
    [GL_L1SYNC] = GPE("SYNC", N_GLSYNC, 0),
    [GL_L2WAVE] = GPE("WAVE", N_GLWAVE, 3),
    [GL_L2FREQ] = GPD("FREQ", F_LFOHZ, 0, 127, 70),
    [GL_L2KEY] = GPE("KEY", N_GLKEY, 1),
    [GL_L2SYNC] = GPE("SYNC", N_GLSYNC, 0),
    [GL_ARPON] = GPE("ARP", N_GONOFF, 0),
    [GL_ARPTYPE] = GPE("TYPE", N_GARPT, 0),
    [GL_ARPKSYNC] = GPE("KSYNC", N_GONOFF, 0),
    [GL_ARPLAST] = GPD("LAST", F_INT, 1, 8, 8),
    [GL_ARPST_A] = GPD("STP14", F_INT, 0, 15, 15),
    [GL_ARPST_B] = GPD("STP58", F_INT, 0, 15, 15),
    [GL_DTYPE] = GPE("DTYPE", N_GDTYPE, 0),
    [GL_DTIME] = GPE("DTIME", N_GDTIME, 1),
    [GL_DFDBK] = GPD("DFDBK", F_PCT, 0, 120, 55),
    [GL_RTYPE] = GPE("RTYPE", N_GRTYPE, 2),
    [GL_RSIZE] = GPD("RSIZE", F_PCT, 0, 127, 95),
    [GL_RDAMP] = GPD("RDAMP", F_PCT, 0, 127, 60),
    [GL_MFX] = GPE("MFX", N_GMFX, 0),
    [GL_ROUTE] = GPE("ROUTE", N_GROUTE, 0),
    [DT_SPLIT] = GPD("SPLIT", F_NOTE, 0, 127, 60),
    [GL_DRIFT] = GPD("DRIFT", F_PCT, 0, 127, 0),
#define GPATCH(k, s, d) [GL_P1SRC + 3 * (k)] = GPE("SRC", N_GPSRC, s), [GL_P1DST + 3 * (k)] = GPE("DST", N_GPDST, d), \
                        [GL_P1INT + 3 * (k)] = GPD("INT", F_BIPCT, -63, 63, 0)
    GPATCH(0, GS_LFO1, GD_PITCH), GPATCH(1, GS_VELO, GD_CUT), GPATCH(2, GS_VELO, GD_BAL), GPATCH(3, GS_LFO2, GD_CUT),
#undef GPATCH
};
#undef GPD
#undef GPE

/* ---------------------------------------------------------------- the library --- */

/* a layer of a voice: one zone of a sound, decoded as it plays (IMA ADPCM, eng_sample.c's zones) */
typedef struct {
    uint32_t pos, frac;          /* the next sample to decode; the fraction of the step, Q16 */
    int32_t pred, idx;           /* the decoder's state */
    int32_t prev, cur;           /* the last two samples decoded, through the character */
    uint16_t zi;                 /* the zone: < 0x8000 DT_ZONES[zi], else a user slot's (0x8000 | slot << 5 | zone) */
    uint8_t on;                  /* 0 silent (no zone, or played to its end), 1 playing, 2 its last sample held */
    uint8_t rsv;
} fo_lay_t;

static inline const smp_zone_t *fo_zone(uint32_t zi)
{
    return zi < 0x8000u ? &DT_ZONES[zi % NELEM(DT_ZONES)] : &usr_zone[(zi >> 5) & 3u][zi & 15u];
}
static inline const uint8_t *fo_data(uint32_t zi) { return zi < 0x8000u ? DT_DATA : SMP_DATA; }

/* sound si (0..DT_NSND - 1)'s zone for the note, 0xFFFF none (an empty user slot) */
static uint32_t fo_pick(uint32_t si, int32_t note)
{
    uint32_t i, zi = 0xFFFFu, width = 128u;
    if (si < DT_NSETS) {
        const smp_set_t *set = &DT_SETS[si];
        for (i = 0; i < set->nz; i++)
            if (note >= DT_ZONES[set->z0 + i].lo && note <= DT_ZONES[set->z0 + i].hi)
                return set->z0 + i;
        return set->z0;
    }
    si -= DT_NSETS;
    for (i = 0; si < SMP_USER_SLOTS && i < usr_nz[si]; i++) {   /* a user slot: the narrowest zone playing it */
        const smp_zone_t *z = &usr_zone[si][i];
        if (note >= z->lo && note <= z->hi && (uint32_t)(z->hi - z->lo) <= width) {
            zi = 0x8000u | si << 5 | i;
            width = (uint32_t)(z->hi - z->lo);
        }
    }
    return zi;
}

/* the sampler's character on a decoded sample: EII mu-law 8 bits (G.711: sign, 3 exponent bits, 4 mantissa bits;
 * the step grows with the level, a 14-bit range in 8 bits, the quiet tails smooth, the loud parts grainy), EMU 1
 * the top 8 bits (linear: the quiet tails coarse) */
static const uint8_t DT_MUEXP[256] = {          /* the exponent of a biased magnitude a (132..32767): DT_MUEXP[a >> 7] */
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
static inline int32_t fo_char(int32_t s, uint32_t ch)
{
    if (ch == FC_EII) {
        int32_t a = (s < 0 ? -s : s), e;
        a = (a > 32635 ? 32635 : a) + 132;
        e = DT_MUEXP[a >> 7];
        a = (((((a >> (e + 3)) & 15) << 3) + 132) << e) - 132;   /* the step's level, back to 16 bits */
        return s < 0 ? -a : a;
    }
    if (ch == FC_EMU1)
        return clamp(s + 128, -32768, 32767) & ~255;
    return s;
}

/* the next sample of layer l (zone z, data d): decoded, looped (when loop), faded over the last 512 samples of a
 * zone that ends without a fade of its own (a sustain played once through: LOOP OFF) */
static inline int32_t fo_next(fo_lay_t *l, const smp_zone_t *z, const uint8_t *d, int loop, uint32_t ch)
{
    uint32_t pos = l->pos, b = d[z->off + (pos >> 1)];
    uint32_t code = (pos & 1u) ? (b >> 4) : (b & 15u);
    int32_t step = IMA_STEP[l->idx], vd = step >> 3, s, ix;   /* (idx kept in 0..88) */
    if (code & 4u)
        vd += step;
    if (code & 2u)
        vd += step >> 1;
    if (code & 1u)
        vd += step >> 2;
    s = l->pred + ((code & 8u) ? -vd : vd);
    l->pred = s < -32768 ? -32768 : s > 32767 ? 32767 : s;
    ix = l->idx + IMA_IDX[code & 7u];
    l->idx = ix < 0 ? 0 : ix > 88 ? 88 : ix;
    s = l->pred;
    pos++;
    if (loop) {
        if (pos > z->le) {
            pos = z->ls;
            l->pred = z->pred;
            l->idx = z->idx;
        }
    } else if (z->looped && z->n - pos < 512u) {
        s = (s * (int32_t)(z->n - pos)) >> 9;
    }
    l->pos = pos;
    return fo_char(s, ch);
}

/* a layer starts: zone zi, START st (0..127: a built-in zone's start, from its decoder grid) */
static void fo_start(fo_lay_t *l, uint32_t zi, int32_t st, int loop)
{
    const smp_zone_t *z;
    l->zi = (uint16_t)zi;
    l->frac = 0;
    l->prev = l->cur = 0;
    l->pos = 0;
    l->pred = l->idx = 0;
    l->on = zi != 0xFFFFu;
    if (!l->on)
        return;
    z = fo_zone(zi);
    if (!z->n) {
        l->on = 0;
        return;
    }
    if (st > 0 && zi < 0x8000u) {
        uint32_t lim = z->looped && loop ? z->le : z->n - z->n / 8u, at = (uint32_t)st * lim / 128u, g, gp;
        uint32_t gs;
        g = at * DT_GRID / z->n;
        gp = g * z->n / DT_GRID;
        gs = DT_ZGRID[zi][g];
        l->pos = gp;
        l->pred = (int16_t)(gs & 0xFFFFu);
        l->idx = (int32_t)(gs >> 16) > 88 ? 88 : (int32_t)(gs >> 16);
        while (l->pos < at)                         /* (< n / DT_GRID samples: a few hundred) */
            (void)fo_next(l, z, DT_DATA, 0, FC_CLEAN);
    }
}

/* one output sample of layer l at step stepq (Q16 native samples per output sample). Inlined: on the target a call
 * per sample and layer cost more than the layer (tests/target_budget.py counts its fetch loop, ~0.5 a sample, as a
 * nested loop: the static cost looks high, the host measure is the one to read, see dt_nvoice) */
static inline __attribute__((always_inline)) int32_t fo_play(fo_lay_t *l, const smp_zone_t *z, const uint8_t *d,
                                                            uint32_t stepq, int loop, uint32_t ch)
{
    if (!l->on)
        return 0;
    l->frac += stepq;
    while (l->frac >= 65536u) {
        l->frac -= 65536u;
        l->prev = l->cur;
        if (l->on == 2u) {                          /* played to its end: silent from here */
            l->on = 0;
            l->prev = l->cur = 0;
            return 0;
        }
        if (l->pos >= z->n) {
            l->on = 2;
            l->cur = 0;
        } else {
            l->cur = fo_next(l, z, d, loop && z->looped, ch);
        }
    }
    if (ch != FC_CLEAN)                             /* no interpolation: the sample held */
        return l->prev;
    return l->prev + (((l->cur - l->prev) * (int32_t)(l->frac >> 1)) >> 15);
}

/* the layer's step for a pitch (1/16 semitone) and a fine residual (1/4096) */
static uint32_t fo_step(const smp_zone_t *z, int32_t pitch16, int32_t fine)
{
    int32_t d16 = clamp(pitch16 - z->root16, -1536, 576);   /* <= 3 octaves up: a bounded decode load */
    uint32_t r = pow2_q16(d16), st = (r >> 8) * (z->rate >> 8);
    return st + (uint32_t)((int32_t)(st >> 12) * fine);
}

/* ------------------------------------------------------------------- voices --- */
/* the voice state beyond voice_t (the part's engine memory, engines.c eng_mem) */
typedef struct {
    fo_lay_t a, b;               /* SOUND A, SOUND B */
    int32_t f1, f2, f3, f4;      /* the filter: stage 1 (LPF24's first), stage 2 */
    int32_t feg;                 /* filter EG, Q24 */
    uint32_t l1, l2;             /* LFO phases */
    int32_t s1, s2;              /* their S&H values, Q15 */
    int32_t nst;                 /* noise */
    int32_t xa, xb;              /* the note's layer gains (LAYER: VEL X, SPLIT), Q15 */
    uint8_t fst;                 /* filter EG stage: 0 off, 1 attack, 2 decay / sustain, 3 release */
    uint16_t dn;                 /* DRIFT: blocks until the wander's next target */
    int16_t d1, d2, dc;          /* DRIFT: this note's offsets, A, B and the cutoff (Q15, -1..1) */
    int16_t dw, dt;              /* DRIFT: the slow wander and its target (Q15) */
} glv_t;
typedef struct {
    glv_t v[NVOICE];
    uint32_t free1, free2;       /* KEY SYNC OFF: the part's free clocks */
    uint32_t tim1, tim2;         /* TIMBRE: from 0 when the first key goes down */
    uint32_t inc1, inc2;         /* this block's base increments */
    int32_t seed;
} glp_t;
ENG_MEM_FITS("DITTO", sizeof(glp_t));
static glp_t *gl_part(const track_t *t) { return (glp_t *)ENG_MEM((uint32_t)(t - trk) % NPART); }
static uint32_t div_samples(uint32_t div);         /* (fx.c) */

static inline int32_t gp(const track_t *t, uint32_t id, int32_t lo, int32_t hi) { return clamp(t->p[id], lo, hi); }

/* an LFO's base increment per block: FREQ, or one cycle per SYNC note value */
static __attribute__((noinline)) uint32_t gl_lfo_inc(const track_t *t, uint32_t freq_id, uint32_t sync_id, int32_t fm)
{
    int32_t s = gp(t, sync_id, 0, 10);
    if (s) {
        uint32_t cyc = div_samples(GL_SYNC_DIV[s]), q, r;
        cyc = cyc ? cyc : 1u;
        q = 0xFFFFFFFFu / cyc;
        r = 0xFFFFFFFFu % cyc;
        return q * CTL + (r * CTL + CTL) / cyc;
    }
    return LFO_INC[clamp(t->p[freq_id] + fm, 0, 127)];
}

static int32_t gl_wave(uint32_t wave, uint32_t ph, int32_t sh)
{
    switch (wave) {
    case 0:
        return (int32_t)(ph >> 16) - 32768;
    case 1:
        return ph < 0x80000000u ? 32767 : -32767;
    case 2:
        return osc_tri(ph);
    case 3:
        return osc_sine(ph);
    default:
        return sh;
    }
}

/* one LFO of a voice, a block on: phase += inc, a new S&H value each cycle; its value (Q15, bipolar) */
static int32_t gl_lfo_step(uint32_t *ph, int32_t *sh, uint32_t inc, uint32_t wave, int32_t *seed)
{
    uint32_t old = *ph;
    *ph += inc;
    if (*ph < old || !*sh)
        *sh = (int32_t)(noise32(seed) >> 16) - 32768;
    return gl_wave(wave, *ph, *sh);
}

static void gol_block(track_t *t)
{
    glp_t *g = gl_part(t);
    g->inc1 = gl_lfo_inc(t, GL_L1FREQ, GL_L1SYNC, 0);
    g->inc2 = gl_lfo_inc(t, GL_L2FREQ, GL_L2SYNC, 0);
    g->free1 += g->inc1;
    g->free2 += g->inc2;
    g->tim1 += g->inc1;
    g->tim2 += g->inc2;
    if (!g->seed)
        g->seed = 0x5EED1234;
}

/* the layers of a note: their zones (by the key, SOUND B's by its sounding key), their gains (LAYER) */
static void fo_note_layers(track_t *t, voice_t *v, glv_t *s)
{
    int32_t note = v->note, lay = gp(t, DT_LAYER, 0, 2), sb = gp(t, DT_SNDB, 0, DT_NSND);
    int32_t loop = t->p[DT_LOOP] != 0, st = gp(t, DT_START, 0, 127), x = (v->mvel ? v->mvel : v->vel) * 258;
    s->xa = s->xb = 32767;
    if (lay == FL_VELX) {                               /* soft notes A, hard notes B (the key's velocity, not
                                                         * UNISON's share of it) */
        s->xa = 32767 - x;
        s->xb = x;
    } else if (lay == FL_SPLIT) {
        int32_t lo = note < t->p[DT_SPLIT];
        s->xa = lo ? 0 : 32767;
        s->xb = lo ? 32767 : 0;
    }
    fo_start(&s->a, s->xa ? fo_pick((uint32_t)gp(t, DT_SNDA, 0, DT_NSND - 1), note) : 0xFFFFu, st, loop);
    fo_start(&s->b, sb && s->xb ? fo_pick((uint32_t)sb - 1u, clamp(note + gp(t, DT_BSEMI, -24, 24), 0, 127)) : 0xFFFFu,
             st, loop);
}

static void gol_note_on(track_t *t, voice_t *v)
{
    glp_t *g = gl_part(t);
    glv_t *s = &g->v[(uint32_t)(v - t->v) % NVOICE];
    uint32_t i, others = 0, fresh = v->env == 0 && v->env_out == 0;
    for (i = 0; i < NVOICE; i++)
        others |= t->v[i].gate && &t->v[i] != v;
    if (!others) {                                      /* TIMBRE: the first key down starts the LFOs from 0 */
        g->tim1 = 0;
        g->tim2 = 0;
    }
    fo_note_layers(t, v, s);                            /* a sampler: every note from the sample's start */
    if (fresh) {
        s->f1 = s->f2 = s->f3 = s->f4 = 0;
        s->feg = 0;
        if (t->p[GL_DRIFT] > 0) {                       /* DRIFT: each note its own offsets */
            s->d1 = (int16_t)(noise32(&g->seed) >> 16);
            s->d2 = (int16_t)(noise32(&g->seed) >> 16);
            s->dc = (int16_t)(noise32(&g->seed) >> 16);
        }
        s->l1 = gp(t, GL_L1KEY, 0, 2) == 2 ? 0u : gp(t, GL_L1KEY, 0, 2) == 1 ? g->tim1 : g->free1;
        s->l2 = gp(t, GL_L2KEY, 0, 2) == 2 ? 0u : gp(t, GL_L2KEY, 0, 2) == 1 ? g->tim2 : g->free2;
        s->s1 = s->s2 = 0;
    } else {                                            /* retrigger: LFOs run on, KEY SYNC VOICE restarts them */
        if (gp(t, GL_L1KEY, 0, 2) == 2)
            s->l1 = 0;
        if (gp(t, GL_L2KEY, 0, 2) == 2)
            s->l2 = 0;
        if (t->p[GL_EGRESET]) {                         /* EG RESET: both envelopes from 0 */
            s->feg = 0;
            v->env = 0;
        }
    }
    if (!s->nst)
        s->nst = 0x2468ACE1 + (int32_t)v->age;
    s->fst = 1;
}

/* the filter EG, one control tick (as voice.c env_tick: linear attack, exponential decay and release) */
static int32_t gl_feg_tick(const track_t *t, voice_t *v, glv_t *s)
{
    if (!v->gate && s->fst && s->fst < 3u)
        s->fst = 3;
    switch (s->fst) {
    case 1:
        s->feg += (int32_t)ENV_LIN[gp(t, GL_FA, 0, 127)];
        if (s->feg >= (1 << 24)) {
            s->feg = 1 << 24;
            s->fst = 2;
        }
        break;
    case 2:
        s->feg += mulq16((gp(t, GL_FS, 0, 127) << 17) - s->feg, ENV_EXP[gp(t, GL_FD, 0, 127)]);
        break;
    case 3:
        s->feg -= mulq16(s->feg, ENV_EXP[gp(t, GL_FR, 0, 127)]);
        if (s->feg < (1 << 10)) {
            s->feg = 0;
            s->fst = 0;
        }
        break;
    default:
        s->feg = 0;
        break;
    }
    return s->feg >> 9;
}

static void gol_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    glp_t *g = gl_part(t);
    glv_t *s = &g->v[(uint32_t)(v - t->v) % NVOICE];
    uint32_t ft = (uint32_t)gp(t, GL_FTYPE, 0, 4), ch = (uint32_t)gp(t, DT_CHAR, 0, 2), k, i;
    int32_t feg, lfo1, lfo2, src[GS_N], d[GD_N], cut, reso, xa, xb, xn, gain, bal;
    tsvf_t f1c, f2c;
    int32_t k2, loop = t->p[DT_LOOP] != 0;
    int32_t dist = t->p[GL_DIST] != 0, drift = gp(t, GL_DRIFT, 0, 127), dw, df1 = 0, df2 = 0, dcut = 0;
    const smp_zone_t *za = fo_zone(s->a.zi), *zb = fo_zone(s->b.zi);
    const uint8_t *da = fo_data(s->a.zi), *db = fo_data(s->b.zi);
    uint32_t sta = 0, stb = 0;
    if (!s->a.on && !s->b.on) {                         /* both layers played out: the voice is free */
        v->active = 0;
        return;
    }
    /* control rate: the envelopes, the LFOs, the patch */
    feg = gl_feg_tick(t, v, s);
    for (k = 0; k < GD_N; k++)
        d[k] = 0;
    src[GS_FEG] = feg;
    src[GS_AEG] = m->envq15;
    src[GS_VELO] = v->vel * 258;
    src[GS_KBD] = clamp(((int32_t)v->note - 60) * 512, -32767, 32767);
    lfo1 = gl_lfo_step(&s->l1, &s->s1, g->inc1, (uint32_t)gp(t, GL_L1WAVE, 0, 4), &g->seed);
    src[GS_LFO1] = lfo1;
    {   /* LFO 2's rate may be patched: the routes to LFO2F first, from the sources known so far */
        int32_t f2 = 0;
        for (k = 0; k < 4u; k++) {
            int32_t a = gp(t, GL_P1INT + 3u * k, -63, 63), sr = gp(t, GL_P1SRC + 3u * k, 0, GS_N - 1);
            if (a && gp(t, GL_P1DST + 3u * k, 0, GD_N - 1) == GD_LFO2F && sr != GS_LFO2)
                f2 += (src[sr] * a) >> 14;
        }
        lfo2 = gl_lfo_step(&s->l2, &s->s2, f2 ? gl_lfo_inc(t, GL_L2FREQ, GL_L2SYNC, f2) : g->inc2,
                           (uint32_t)gp(t, GL_L2WAVE, 0, 4), &g->seed);
    }
    src[GS_LFO2] = lfo2;
    for (k = 0; k < 4u; k++) {
        int32_t a = gp(t, GL_P1INT + 3u * k, -63, 63);
        uint32_t ds = (uint32_t)gp(t, GL_P1DST + 3u * k, 0, GD_N - 1);
        if (a && ds != GD_LFO2F)
            d[ds] += src[gp(t, GL_P1SRC + 3u * k, 0, GS_N - 1)] * a;     /* Q15 x 63 */
    }
    /* DRIFT: the note's offsets and the wander (as Golana's: up to +-12 ct each layer, +-5 ct the wander), as an
     * old sampler's clocks drift; in 1/4096 of the pitch and in cutoff units */
    if (drift) {
        if (!s->dn--) {
            s->dn = 511;
            s->dt = (int16_t)(noise32(&g->seed) >> 16);
        }
        s->dw = (int16_t)(s->dw + ((s->dt - s->dw) >> 9));
        dw = (s->dw * drift) >> 15;
        df1 = (((s->d1 * drift) >> 15) * 28 >> 7) + (dw * 12 >> 7);
        df2 = (((s->d2 * drift) >> 15) * 28 >> 7) + (dw * 12 >> 7);
        dcut = ((s->dc * drift) >> 15) * 5 + dw;
    }
    /* the layers' steps: the voice's pitch (glide, unison, tune, bend in m), the patch; SOUND B its SEMI and TUNE,
     * UNISON's detune the other way (each voice two detuned layers, as Golana's two oscillators) */
    if (s->a.on)
        sta = fo_step(za, m->pitch16 + ((d[GD_PITCH] * 3) >> 15), m->fine + df1);
    if (s->b.on) {
        int32_t tn = gp(t, DT_BTUNE, -63, 63), t16 = tn * 16 / 100, rem = tn * 16 - t16 * 100;
        stb = fo_step(zb, m->pitch16 + ((d[GD_PITCH] * 3) >> 15) + gp(t, DT_BSEMI, -24, 24) * 16 + t16 +
                              ((d[GD_BPITCH] * 3) >> 15),
                      (t->p[P_VOICE] == V_UNISON ? m->fine - 2 * v->fine : m->fine) + df2 + rem * 2367 / 16000);
    }
    /* the mixer: levels Q15, the note's LAYER gains, BAL from the patch (+ toward B, - toward A) */
    bal = clamp(d[GD_BAL] >> 6, -32767, 32767);
    xa = mulq15(gp(t, DT_LVLA, 0, 127) * 258, s->xa);
    xb = mulq15(gp(t, DT_LVLB, 0, 127) * 258, s->xb);
    if (bal > 0)
        xa = mulq15(xa, 32767 - bal);
    else if (bal < 0)
        xb = mulq15(xb, 32767 + bal);
    xn = clamp(gp(t, GL_NOISE, 0, 127) + (d[GD_NOISE] >> 14), 0, 127) * 258;
    /* the filter: CUTOFF, the EG, KEY TRACK (63: the keys exactly, ~18.7 per 1/16 semitone), the patch */
    cut = (gp(t, GL_CUTOFF, 0, 127) << 8) + ((feg * gp(t, GL_FEGINT, -63, 63)) >> 6) +
          ((gp(t, GL_KTRK, -63, 63) * (m->pitch16 - 60 * 16) * 19) >> 6) + (d[GD_CUT] >> 6) + dcut;
    reso = gp(t, GL_RESO, 0, 127);
    tsvf_coef(&f2c, cut, reso);
    k2 = 8192 - reso * 7600 / 127;
    if (ft == 0u)
        tsvf_coef_k(&f1c, cut, 6000);                   /* LPF24: the first stage without the peak */
    else if (ft == 4u)
        tsvf_coef_k(&f1c, cut, 8192 - reso * 8140 / 127);   /* MS20: up to a ringing, near self-oscillating peak */
    gain = clamp(32767 + (d[GD_AMP] >> 6), 0, 49151);  /* AMP: the patch's gain, up to 1.5 */
    {
        int32_t a1 = s->f1, a2 = s->f2, b1 = s->f3, b2 = s->f4, nst = s->nst;
        int la = s->a.on && xa, lb = s->b.on && xb;
        for (i = 0; i < n; i++) {
            int32_t sx = 0, y;
            if (la)
                sx = mulq15(fo_play(&s->a, za, da, sta, loop, ch), xa);
            else if (s->a.on)                           /* (silent by its gain: it still plays on, in time) */
                (void)fo_play(&s->a, za, da, sta, loop, ch);
            if (lb)
                sx += mulq15(fo_play(&s->b, zb, db, stb, loop, ch), xb);
            else if (s->b.on)
                (void)fo_play(&s->b, zb, db, stb, loop, ch);
            if (xn)
                sx += mulq15((int32_t)(noise32(&nst) >> 17) - 16384, xn);
            sx = (sx * 3) >> 3;                         /* a full sample ~11 k into the filter, both ~22 k */
            switch (ft) {
            case 0:
                y = tsvf_lp(&f1c, sx, &a1, &a2);
                y = tsvf_lp(&f2c, y, &b1, &b2);
                break;
            case 2:
                y = analog_tap(&f2c, k2, sx, &b1, &b2, 1u);
                break;
            case 3:
                y = analog_tap(&f2c, k2, sx, &b1, &b2, 2u);
                break;
            case 4: {                                   /* MS20: driven in, the resonating state saturated */
                int32_t in = softclip(clamp(sx * 2, -65535, 65535)) >> 1, v1, v2, v3 = in - b2;
                v1 = (f1c.a1 * b1 + f1c.a2 * v3) >> 13;
                v2 = b2 + ((f1c.a2 * b1 + f1c.a3 * v3) >> 13);
                b1 = softclip(clamp((2 * v1 - b1) >> 1, -65535, 65535)) << 1;   /* the scream: saturated, not runaway */
                b2 = clamp(2 * v2 - b2, -150000, 150000);
                y = clamp(v2, -65535, 65535);
                break;
            }
            default:
                y = tsvf_lp(&f2c, sx, &b1, &b2);
                break;
            }
            y = soft_knee(y, 16000) << 1;
            if (dist)
                y = softclip(clamp(y, -32767, 32767) * 4);
            if (gain != 32767)
                y = mulq15(y, gain);
            out[i] += voice_amp(clamp(y, -65535, 65535), m, i) << 1;
        }
        s->f1 = a1;
        s->f2 = a2;
        s->f3 = b1;
        s->f4 = b2;
        s->nst = nst;
    }
}

/* the voices a part may use: 8 when each note plays one sound (SOUND B OFF, or SPLIT), DT_POLY2 when it plays two (STACK,
 * VEL X): a layer costs about what a voice of Golana does, and the two-layer voice nearly twice (host, 8 notes held:
 * ~185 ns a sample with one sound, as Felucca's heaviest mix; 5 voices of two ~165, 8 of two ~240: too many) */
#define DT_POLY2 5u
static uint32_t dt_nvoice(const track_t *t)
{
    return t->p[DT_SNDB] > 0 && t->p[DT_LAYER] != FL_SPLIT ? DT_POLY2 : NVOICE;
}

/* the voice ends with its amp envelope's release, or when its layers have played out (a one-shot) */
static const preset_t GOLANA_PRESETS[] = {
    {"INIT", {0, 0, 1, 0, 0, 0, 0, 127}, {0, 64, 127, 40}, 0, 0, FX(0, 0, 0, 0), 0},
};

static const engine_t ENG_GOLANA = {
    .name = "DITTO",
    .page_title = {"SOUND", "SND B"},
    .edit = {
        {"SND A", F_ENUM, 0, DT_NSND - 1, 0, N_FOSND, 0},
        {"START", F_INT, 0, 127, 0, 0, 0},
        {"LOOP", F_ENUM, 0, 1, 1, N_GONOFF, 0},
        {"SND B", F_ENUM, 0, DT_NSND, 0, N_FOSNDB, 0},
        {"LAYER", F_ENUM, 0, 2, 0, N_FOLAYER, 0},
        {"SEMI", F_SEMI, -24, 24, 0, 0, 0},
        {"TUNE", F_INT, -63, 63, 0, 0, "ct"},
        {"CUT", F_CUTOFF, 0, 127, 127, 0, 0},
    },
    .presets = GOLANA_PRESETS,
    .npresets = NELEM(GOLANA_PRESETS),
    .knob = {P_E7, GL_RESO, GL_FEGINT, P_E1},
    .poly = 8,
    .keep = 0,
    .note_on = gol_note_on,
    .render = gol_render,
    .block = gol_block,
};
