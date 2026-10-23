/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele, based on Felucca (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* DITTO: the one engine of Ditto, a sampler after the E-mu Emulator II (1984), kept as dry as it was: per voice one or
 * two sounds of its library, an 8-bit companded sample player, a 24 dB low-pass with its ADSR, a VCA with its ADSR and
 * one LFO; the routings fixed, as the EII's (no patch bay). (The file keeps the name of Golana's engine, whose frame
 * it shares: the envelopes, the arpeggiator, the step sequencer, delay and reverb.)
 *
 *   VOICE DEF   SOUND A (the library: PIANO, STRINGS, CHOIR .. or a user slot USR1-3), START (0 the sample's beginning
 *               .. 127 near its end), LOOP; CHARACTER: EII (8-bit mu-law, each sample held, no interpolation) or CLEAN;
 *               VELOCITY -> LEVEL and -> FILTER; VIBRATO (the LFO on the pitch)
 *   PRESET DEF  SOUND B (OFF or a sound) with its SEMI and TUNE; LAYER: STACK (both), VEL SW (B from a velocity on),
 *               VEL X (crossfaded by the velocity), SPLIT (B below a key), POS X (crossfaded over a range of keys)
 *   FILTER      FREQ, RES (resonance: the EII's Q), ENV AMOUNT, KEYBOARD, LFO AMOUNT; its ADSR
 *   LFO / VCA   the VCA's ADSR; the LFO: WAVE, RATE (or a note value of the tempo), DELAY, KEY SYNC; LFO -> VCA
 * The slots: core.h GL_* (Golana's names) and DT_* (what they mean here). 8 voices, 5 when two sounds play (dt_nvoice). */

#include "ditto_samples.h"                       /* the library (tools/gen_ditto_samples.py) */
static const char *const N_FOSND[] = {DT_SET_NAMES_INIT, "USR1", "USR2", "USR3"};
static const char *const N_FOSNDB[] = {"OFF", DT_SET_NAMES_INIT, "USR1", "USR2", "USR3"};
#define DT_NSND (DT_NSETS + SMP_USER_SLOTS)          /* SOUND A's choices; SOUND B's: OFF and these */
static const char *const N_FOLAYER[] = {"STACK", "VEL SW", "VEL X", "SPLIT", "POS X"};
static const char *const N_FOCHAR[] = {"EII", "CLEAN"};
static const char *const N_GLWAVE[] = {"TRI", "SIN", "SQR", "SAW", "S&H"};
static const char *const N_GLKEY[] = {"OFF", "TIMBRE", "VOICE"};
static const char *const N_GLSYNC[] = {"OFF", "4BAR", "2BAR", "1/1", "1/2", "1/4", "1/8", "1/8T", "1/16", "16T", "1/32"};
static const uint8_t GL_SYNC_DIV[11] = {0, 9, 8, 7, 6, 0, 1, 4, 2, 5, 3};   /* N_GLSYNC -> fx.c div_samples */
static const char *const N_GARPT[] = {"UP", "DOWN", "ALT1", "ALT2", "RANDOM", "TRIGGER",
                                      "ORDER", "CONVERGE", "PEDAL", "DOUBLE"};
static const char *const N_GDTYPE[] = {"STEREO", "CROSS", "L/R"};
static const char *const N_GRTYPE[] = {"ROOM", "SPRING", "HALL"};
static const char *const N_GONOFF[] = {"OFF", "ON"};
static const char *const N_GDTIME[] = {"1/4", "1/8", "1/16", "1/32", "1/8T", "16T", "1/2", "1/1", "2BAR", "4BAR",
                                       "1/8D", "16D", "1/4D"};
enum { FL_STACK, FL_VELSW, FL_VELX, FL_SPLIT, FL_POSX };
enum { FC_EII, FC_CLEAN };

#define GPD(l, f, mn, mx, df) {l, f, mn, mx, df, 0, 0}
#define GPE(l, n, df) {l, F_ENUM, 0, (int16_t)(sizeof(n) / sizeof(n[0]) - 1), df, n, 0}
/* Ditto's meaning of the slots it borrows (core.h GL_* / DT_*): params.c param_desc_of returns these for the
 * engine (label 0: the slot keeps Felucca's own descriptor) */
static const param_desc_t GOL_TP[P_COUNT] = {
    [DT_LVLA] = GPD("LVL A", F_PCT, 0, 127, 127),
    [DT_CHAR] = GPE("CHAR", N_FOCHAR, FC_EII),
    [DT_LVLB] = GPD("LVL B", F_PCT, 0, 127, 100),
    [GL_RESO] = GPD("RES", F_PCT, 0, 127, 10),
    [GL_FEGINT] = GPD("ENV", F_BIPCT, -63, 63, 0),
    [GL_KTRK] = GPD("KBD", F_BIPCT, -63, 63, 32),
    [GL_FA] = GPD("ATK", F_TIME, 0, 127, 0),
    [GL_FD] = GPD("DEC", F_TIME, 0, 127, 64),
    [GL_FS] = GPD("SUS", F_PCT, 0, 127, 127),
    [GL_FR] = GPD("REL", F_TIME, 0, 127, 50),
    [GL_EGRESET] = GPE("RESET", N_GONOFF, 0),
    [GL_L1WAVE] = GPE("WAVE", N_GLWAVE, 0),
    [GL_L1FREQ] = GPD("RATE", F_LFOHZ, 0, 127, 85),
    [GL_L1KEY] = GPE("KEY", N_GLKEY, 2),
    [GL_L1SYNC] = GPE("SYNC", N_GLSYNC, 0),
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
    [DT_SPLIT] = GPD("SPLIT", F_NOTE, 0, 127, 60),
    [DT_LFODLY] = GPD("DELAY", F_TIME, 0, 127, 0),
    [DT_VIB] = GPD("VIB", F_PCT, 0, 127, 0),
    [DT_LFOCUT] = GPD("LFO", F_PCT, 0, 127, 0),
    [DT_LFOVCA] = GPD("VCA", F_PCT, 0, 127, 0),
    [DT_VELLVL] = GPD("V>LVL", F_PCT, 0, 127, 96),
    [DT_VELCUT] = GPD("V>FLT", F_BIPCT, -63, 63, 0),
    [DT_XWIDTH] = GPD("WIDTH", F_INT, 1, 48, 12),
    [DT_VELSW] = GPD("V SW", F_INT, 1, 127, 100),
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
 * the step grows with the level, a 14-bit range in 8 bits, the quiet tails smooth, the loud parts grainy); CLEAN as
 * stored */
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
    int32_t f1, f2, f3, f4;      /* the filter: two stages */
    int32_t feg;                 /* filter EG, Q24 */
    int32_t ldf;                 /* the LFO's fade-in after DELAY, Q24 */
    uint32_t l1;                 /* the LFO's phase */
    int32_t s1;                  /* its S&H value, Q15 */
    int32_t xa, xb;              /* the note's layer gains (LAYER), Q15 */
    uint8_t fst;                 /* filter EG stage: 0 off, 1 attack, 2 decay / sustain, 3 release */
} glv_t;
typedef struct {
    glv_t v[NVOICE];
    uint32_t free1, tim1;        /* KEY SYNC OFF: the part's free clock; TIMBRE: from 0 when the first key goes down */
    uint32_t inc1;               /* this block's LFO increment */
    int32_t seed;
} glp_t;
ENG_MEM_FITS("DITTO", sizeof(glp_t));
static glp_t *gl_part(const track_t *t) { return (glp_t *)ENG_MEM((uint32_t)(t - trk) % NPART); }
static uint32_t div_samples(uint32_t div);         /* (fx.c) */

static inline int32_t gp(const track_t *t, uint32_t id, int32_t lo, int32_t hi) { return clamp(t->p[id], lo, hi); }

/* the LFO's increment per block: RATE, or one cycle per SYNC note value */
static __attribute__((noinline)) uint32_t gl_lfo_inc(const track_t *t)
{
    int32_t s = gp(t, GL_L1SYNC, 0, 10);
    if (s) {
        uint32_t cyc = div_samples(GL_SYNC_DIV[s]), q, r;
        cyc = cyc ? cyc : 1u;
        q = 0xFFFFFFFFu / cyc;
        r = 0xFFFFFFFFu % cyc;
        return q * CTL + (r * CTL + CTL) / cyc;
    }
    return LFO_INC[gp(t, GL_L1FREQ, 0, 127)];
}

/* the LFO a block on: TRI SIN SQR SAW S&H (a new value each cycle), Q15 bipolar */
static int32_t gl_lfo_step(glv_t *s, uint32_t inc, uint32_t wave, int32_t *seed)
{
    uint32_t old = s->l1, ph;
    s->l1 += inc;
    ph = s->l1;
    if (s->l1 < old || !s->s1)
        s->s1 = (int32_t)(noise32(seed) >> 16) - 32768;
    switch (wave) {
    case 0: return osc_tri(ph);
    case 1: return osc_sine(ph);
    case 2: return ph < 0x80000000u ? 32767 : -32767;
    case 3: return (int32_t)(ph >> 16) - 32768;
    default: return s->s1;
    }
}

static void gol_block(track_t *t)
{
    glp_t *g = gl_part(t);
    g->inc1 = gl_lfo_inc(t);
    g->free1 += g->inc1;
    g->tim1 += g->inc1;
    if (!g->seed)
        g->seed = 0x5EED1234;
}

/* the layers of a note: their zones (by the key, SOUND B's by its sounding key), their gains (LAYER, the key's
 * velocity: not UNISON's share of it) */
static void fo_note_layers(track_t *t, voice_t *v, glv_t *s)
{
    int32_t note = v->note, lay = gp(t, DT_LAYER, 0, 4), sb = gp(t, DT_SNDB, 0, DT_NSND);
    int32_t loop = t->p[DT_LOOP] != 0, st = gp(t, DT_START, 0, 127), vel = v->mvel ? v->mvel : v->vel;
    int32_t key = t->p[DT_SPLIT], x;
    s->xa = s->xb = 32767;
    switch (lay) {
    case FL_VELSW:                                      /* from VEL SW on: B alone */
        x = vel >= t->p[DT_VELSW];
        s->xa = x ? 0 : 32767;
        s->xb = x ? 32767 : 0;
        break;
    case FL_VELX:                                       /* soft notes A, hard notes B */
        s->xa = 32767 - vel * 258;
        s->xb = vel * 258;
        break;
    case FL_SPLIT:                                      /* B below the key, A from it */
        x = note < key;
        s->xa = x ? 0 : 32767;
        s->xb = x ? 32767 : 0;
        break;
    case FL_POSX: {                                     /* over WIDTH keys centred on the key: B into A */
        int32_t w = gp(t, DT_XWIDTH, 1, 48), lo = key - w / 2;
        x = clamp((note - lo) * 32767 / w, 0, 32767);
        s->xa = x;
        s->xb = 32767 - x;
        break;
    }
    default:
        break;
    }
    if (!sb)
        s->xa = 32767;                                  /* (no SOUND B: A plays every note) */
    fo_start(&s->a, s->xa ? fo_pick((uint32_t)gp(t, DT_SNDA, 0, DT_NSND - 1), note) : 0xFFFFu, st, loop);
    fo_start(&s->b, sb && s->xb ? fo_pick((uint32_t)sb - 1u, clamp(note + gp(t, DT_BSEMI, -24, 24), 0, 127)) : 0xFFFFu,
             st, loop);
}

static void gol_note_on(track_t *t, voice_t *v)
{
    glp_t *g = gl_part(t);
    glv_t *s = &g->v[(uint32_t)(v - t->v) % NVOICE];
    uint32_t i, others = 0, fresh = v->env == 0 && v->env_out == 0, ks = (uint32_t)gp(t, GL_L1KEY, 0, 2);
    for (i = 0; i < NVOICE; i++)
        others |= t->v[i].gate && &t->v[i] != v;
    if (!others)                                        /* TIMBRE: the first key down starts the LFO from 0 */
        g->tim1 = 0;
    fo_note_layers(t, v, s);                            /* a sampler: every note from the sample's start */
    if (fresh) {
        s->f1 = s->f2 = s->f3 = s->f4 = 0;
        s->feg = 0;
        s->l1 = ks == 2u ? 0u : ks == 1u ? g->tim1 : g->free1;
        s->s1 = 0;
    } else {                                            /* retrigger: the LFO runs on, KEY SYNC VOICE restarts it */
        if (ks == 2u)
            s->l1 = 0;
        if (t->p[GL_EGRESET]) {                         /* EG RESET: both envelopes from 0 */
            s->feg = 0;
            v->env = 0;
        }
    }
    s->ldf = t->p[DT_LFODLY] > 0 ? 0 : 1 << 24;         /* DELAY: the LFO fades in again on every note */
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
    uint32_t ch = (uint32_t)gp(t, DT_CHAR, 0, 1), i;
    int32_t feg, lfo, vel = v->mvel ? v->mvel : v->vel, cut, reso, xa, xb, g12, vib;
    tsvf_t f1c, f2c;
    int32_t loop = t->p[DT_LOOP] != 0;
    const smp_zone_t *za = fo_zone(s->a.zi), *zb = fo_zone(s->b.zi);
    const uint8_t *da = fo_data(s->a.zi), *db = fo_data(s->b.zi);
    uint32_t sta = 0, stb = 0;
    if (!s->a.on && !s->b.on) {                         /* both layers played out: the voice is free */
        v->active = 0;
        return;
    }
    /* control rate: the filter EG; the LFO, faded in after DELAY */
    feg = gl_feg_tick(t, v, s);
    lfo = gl_lfo_step(s, g->inc1, (uint32_t)gp(t, GL_L1WAVE, 0, 4), &g->seed);
    if (s->ldf < (1 << 24)) {
        s->ldf += (int32_t)ENV_LIN[gp(t, DT_LFODLY, 0, 127)];
        if (s->ldf > (1 << 24))
            s->ldf = 1 << 24;
        lfo = (lfo * (s->ldf >> 9)) >> 15;
    }
    /* the layers' steps: the voice's pitch (glide, unison, tune, bend in m) and the vibrato (up to +-2 semitones);
     * SOUND B its SEMI and TUNE, UNISON's detune the other way */
    vib = (lfo * gp(t, DT_VIB, 0, 127)) >> 15;          /* -127..127 */
    vib = vib * 32 / 127;                               /* 1/16 semitone */
    if (s->a.on)
        sta = fo_step(za, m->pitch16 + vib, m->fine);
    if (s->b.on) {
        int32_t tn = gp(t, DT_BTUNE, -63, 63), t16 = tn * 16 / 100, rem = tn * 16 - t16 * 100;
        stb = fo_step(zb, m->pitch16 + vib + gp(t, DT_BSEMI, -24, 24) * 16 + t16,
                      (t->p[P_VOICE] == V_UNISON ? m->fine - 2 * v->fine : m->fine) + rem * 2367 / 16000);
    }
    xa = mulq15(gp(t, DT_LVLA, 0, 127) * 258, s->xa);
    xb = mulq15(gp(t, DT_LVLB, 0, 127) * 258, s->xb);
    /* the filter: FREQ, the EG, KEYBOARD (63: the keys exactly), velocity, the LFO */
    cut = (gp(t, GL_CUTOFF, 0, 127) << 8) + ((feg * gp(t, GL_FEGINT, -63, 63)) >> 6) +
          ((gp(t, GL_KTRK, -63, 63) * (m->pitch16 - 60 * 16) * 19) >> 6) + ((vel * 258 * gp(t, DT_VELCUT, -63, 63)) >> 6) +
          ((lfo * gp(t, DT_LFOCUT, 0, 127)) >> 7);
    reso = gp(t, GL_RESO, 0, 127);
    tsvf_coef(&f2c, cut, reso);
    tsvf_coef_k(&f1c, cut, 6000);                       /* the first stage without the peak: 24 dB in all */
    /* the level, Q12: velocity -> level (voice.c applies the whole velocity: brought back to (127 - d) + d x vel,
     * UNISON's share kept), the LFO -> VCA (a tremolo down from full) */
    {
        int32_t d = gp(t, DT_VELLVL, 0, 127), want = (127 - d) * 127 + d * vel;   /* x 1/16129 */
        int32_t lv = gp(t, DT_LFOVCA, 0, 127);
        g12 = d == 127 ? 4096 : clamp(want * 4096 / (127 * (vel > 0 ? vel : 1)), 0, 16384);
        if (lv)
            g12 = (g12 * (32767 - (((lfo + 32767) >> 1) * lv >> 7))) >> 15;
    }
    {
        int32_t a1 = s->f1, a2 = s->f2, b1 = s->f3, b2 = s->f4;
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
            sx = (sx * 3) >> 3;                         /* a full sample ~11 k into the filter, both ~22 k */
            y = tsvf_lp(&f1c, sx, &a1, &a2);
            y = tsvf_lp(&f2c, y, &b1, &b2);
            y = soft_knee(y, 16000) << 1;
            if (g12 != 4096)
                y = (clamp(y, -65535, 65535) * g12) >> 12;
            out[i] += voice_amp(clamp(y, -65535, 65535), m, i) << 1;
        }
        s->f1 = a1;
        s->f2 = a2;
        s->f3 = b1;
        s->f4 = b2;
    }
}

/* the voices a part may use: 8 when each note plays one sound (SOUND B OFF, SPLIT, VEL SW), DT_POLY2 when it may play
 * two (STACK, VEL X, POS X): a layer costs about what a voice of Golana does (host, 8 notes held: ~185 ns a sample
 * with one sound, as Felucca's heaviest mix; 5 voices of two ~165, 8 of two ~240: too many) */
#define DT_POLY2 5u
static uint32_t dt_nvoice(const track_t *t)
{
    int32_t l = t->p[DT_LAYER];
    return t->p[DT_SNDB] > 0 && l != FL_SPLIT && l != FL_VELSW ? DT_POLY2 : NVOICE;
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
        {"LAYER", F_ENUM, 0, 4, 0, N_FOLAYER, 0},
        {"SEMI", F_SEMI, -24, 24, 0, 0, 0},
        {"TUNE", F_INT, -63, 63, 0, 0, "ct"},
        {"FREQ", F_CUTOFF, 0, 127, 127, 0, 0},
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
