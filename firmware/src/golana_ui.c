/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele, based on Felucca (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Ditto's interface, after the Emulator II's panel: grey, a two-line character LCD, the modules and their numbered
 * functions, four sliders. In place of Felucca's UI (main.c and the browser build call gu_input, gu_leds and gu_draw
 * instead of ui_input, ui_leds and ui_draw; the rest of Felucca's UI code is never called).
 *
 * The screen (240 x 240): the name strip (DITTO, the preset, the battery); the LCD, 2 lines of 16 characters (the
 * module and its function, or the preset; under it what the function sets, the value just turned, or a message);
 * the four sliders (KNOB 1-4); the modules' LEDs.
 *   the modules (a button opens its first function, again the next):
 *     HOME  PERFORM      FREQ, Q, ENV, LFO RATE
 *     GLO   MASTER       1 TEMPO/TUNE, 2 ASSIGN, 3 OUTPUT
 *     SCL   FILTER       1 FREQ Q ENV, 2 LFO KBD, 3 ADSR
 *     ENV   LFO/VCA      1 VCA ADSR, 2 LFO, 3 LFO SYNC (LFO: straight to 2 LFO)
 *     EDIT  VOICE DEF    11 SAMPLE, 21 VELOCITY, 23 VIBRATO
 *     SEQ   PRESET DEF   22 SOUND B, 25 LAYER, 29 ARP, 30 ARP TIME, 31 ARP STEPS
 *     REC   SEQUENCER    1 PATTERN (a key writes its note into the step), 2 SETUP
 *     FX    EFFECTS      1 DELAY, 2 REVERB
 *     SAVE  DISK         WRITE: PRESETS the slot, KNOB 1 the letter, KNOB 2 changes it, SAVE writes, HOME cancels
 *   SELECT every function in turn; KNOB 1..4 the four sliders; ALGORITHM the filter's FREQ from anywhere; PRESETS the
 *   preset (it loads as the knob turns); ARP on / off; PLAY the sequencer; OCT- OCT+ the keys' octave.
 * Presets: 32 slots in the user preset store (upreset.c); 1..24 play the factory sounds until written over, 25..32
 * INIT. MIDI Program Change 0..31 loads a preset. */

/* ---------------------------------------------------------------- colours --- */
#define GC_PANEL RGB(178, 175, 166)                 /* the panel's grey */
#define GC_PANEL2 RGB(160, 157, 148)                /* its grooves, recesses */
#define GC_EDGE RGB(206, 203, 195)                  /* a lit edge */
#define GC_STRIP RGB(54, 53, 50)                    /* the dark strips: name, LEDs */
#define GC_INK RGB(36, 36, 34)                      /* printing on the panel */
#define GC_MIDI RGB(96, 94, 88)
#define GC_PRINT RGB(214, 211, 202)                 /* printing on the dark strips */
#define GC_BEZEL RGB(26, 26, 24)
#define GC_LCD RGB(150, 168, 64)                    /* the LCD: its ground, its unlit dots' cells, its dots */
#define GC_LCD_OFF RGB(143, 161, 60)
#define GC_LCD_ON RGB(32, 42, 22)
#define GC_LED RGB(236, 52, 36)
#define GC_LED_OFF RGB(96, 42, 36)
#define GC_CAP RGB(226, 224, 216)                   /* a slider's cap */

/* ------------------------------------------------------------ the modules --- */
/* virtual parameters (not a track parameter of part 1) */
enum { GV_NONE = 0xFF, GV_RES = 200, GV_BPM, GV_TUNE, GV_CLOCK, GV_STEP, GV_STEPON, GV_MIDI,
       GV_SSTEP, GV_SNOTE, GV_STYPE, GV_SFLAG, GV_SLEN };   /* (the step sequencer: REC) */
typedef struct { uint8_t id; const char *label; const char *const *names; uint8_t nn; } gu_par_t;   /* nn: names' count */
#define GN(a) a, (uint8_t)NELEM(a)                    /* (the lists have no 0 at their end: GOL_TP's GPE counts them) */
/* a function: its module's button, the module's name, the function's number and name (the LCD), its sliders */
typedef struct { uint8_t btn; const char *tab, *tshort; gu_par_t k[4]; } gu_page_t;
static const char *const N_GVOICE[] = {"POLY", "MONO", "LEGATO", "UNISON"};
static const char *const N_GRES[] = {"1/32", "1/24", "1/16", "1/12", "1/8", "1/4"};
static const uint8_t GU_RES_DIV[6] = {3, 5, 2, 4, 1, 0};    /* N_GRES -> P_ARATE (params.c N_DIV) */
static const char *const N_GCLOCK[] = {"INT", "USB", "TRS"};
static const char *const N_GMIDI[] = {"ALL", "CH 1"};
static const char *const N_GSTYPE[] = {"REST", "NOTE", "TIE"};
static const char *const N_GSFLAG[] = {"OFF", "ACCENT", "SLIDE", "BOTH"};
#define NO {GV_NONE, 0, 0}
static const gu_page_t GU_PAGES[] = {
    {B_HOME, "PERFORM", "", {{GL_CUTOFF, "FREQ", 0}, {GL_RESO, "Q", 0}, {GL_FEGINT, "ENV", 0}, {GL_L1FREQ, "RATE", 0}}},
    {B_GLO, "MASTER", "1 TEMPO TUNE",
     {{GV_BPM, "TEMPO", 0}, {GV_TUNE, "TUNE", 0}, {GV_CLOCK, "CLOCK", GN(N_GCLOCK)}, {GV_MIDI, "MIDI", GN(N_GMIDI)}}},
    {B_GLO, "MASTER", "2 ASSIGN",
     {{P_VOICE, "MODE", GN(N_GVOICE)}, {P_DETUNE, "DETUNE", 0}, {P_GLIDE, "PORTA", 0}, {GL_EGRESET, "RESET", 0}}},
    {B_GLO, "MASTER", "3 OUTPUT", {{P_LEVEL, "LEVEL", 0}, {P_PAN, "PAN", 0}, {P_SPRD, "SPREAD", 0}, NO}},
    {B_SCL, "FILTER", "1 FREQ Q ENV", {{GL_CUTOFF, "FREQ", 0}, {GL_RESO, "Q", 0}, {GL_FEGINT, "ENV", 0}, NO}},
    {B_SCL, "FILTER", "2 LFO KBD", {{DT_LFOCUT, "LFO", 0}, {GL_KTRK, "KBD", 0}, NO, NO}},
    {B_SCL, "FILTER", "3 ADSR", {{GL_FA, "ATTACK", 0}, {GL_FD, "DECAY", 0}, {GL_FS, "SUSTAIN", 0}, {GL_FR, "RELEASE", 0}}},
    {B_ENV, "LFO/VCA", "1 VCA ADSR", {{P_ATK, "ATTACK", 0}, {P_DEC, "DECAY", 0}, {P_SUS, "SUSTAIN", 0}, {P_REL, "RELEASE", 0}}},
    {B_ENV, "LFO/VCA", "2 LFO", {{GL_L1WAVE, "WAVE", 0}, {GL_L1FREQ, "RATE", 0}, {DT_LFODLY, "DELAY", 0}, {DT_LFOVCA, "VCA", 0}}},
    {B_ENV, "LFO/VCA", "3 LFO SYNC", {{GL_L1KEY, "KEY", 0}, {GL_L1SYNC, "TEMPO", 0}, NO, NO}},
    {B_EDIT, "VOICE DEF", "11 SAMPLE", {{DT_SNDA, "SOUND", 0}, {DT_START, "START", 0}, {DT_LOOP, "LOOP", 0}, {DT_CHAR, "CHAR", 0}}},
    {B_EDIT, "VOICE DEF", "21 VELOCITY", {{DT_VELLVL, "LEVEL", 0}, {DT_VELCUT, "FILTER", 0}, NO, NO}},
    {B_EDIT, "VOICE DEF", "23 VIBRATO", {{DT_VIB, "DEPTH", 0}, {GL_L1FREQ, "RATE", 0}, {DT_LFODLY, "DELAY", 0}, NO}},
    {B_SEQ, "PRESET DEF", "22 SOUND B", {{DT_SNDB, "SOUND", 0}, {DT_BSEMI, "SEMI", 0}, {DT_BTUNE, "TUNE", 0}, {DT_LVLB, "LEVEL", 0}}},
    {B_SEQ, "PRESET DEF", "25 LAYER", {{DT_LAYER, "MODE", 0}, {DT_SPLIT, "KEY", 0}, {DT_XWIDTH, "WIDTH", 0}, {DT_VELSW, "VEL SW", 0}}},
    {B_SEQ, "PRESET DEF", "29 ARP", {{GL_ARPON, "ARP", 0}, {GL_ARPTYPE, "TYPE", 0}, {P_AOCT, "RANGE", 0}, {GV_RES, "RES", GN(N_GRES)}}},
    {B_SEQ, "PRESET DEF", "30 ARP TIME",
     {{P_AGATE, "GATE", 0}, {P_ASWING, "SWING", 0}, {P_AHOLD, "LATCH", GN(N_GONOFF)}, {GL_ARPKSYNC, "KSYNC", 0}}},
    {B_SEQ, "PRESET DEF", "31 ARP STEPS", {{GL_ARPLAST, "LAST", 0}, {GV_STEP, "STEP", 0}, {GV_STEPON, "ON/OFF", GN(N_GONOFF)}, NO}},
    {B_REC, "SEQUENCER", "1 PATTERN",
     {{GV_SSTEP, "STEP", 0}, {GV_SNOTE, "NOTE", 0}, {GV_STYPE, "TYPE", GN(N_GSTYPE)}, {GV_SFLAG, "ACC/SLD", GN(N_GSFLAG)}}},
    {B_REC, "SEQUENCER", "2 SETUP", {{GV_SLEN, "LENGTH", 0}, {P_SDIV, "RES", 0}, {P_SGATE, "GATE", 0}, {P_SSWING, "SWING", 0}}},
    {B_FX, "EFFECTS", "1 DELAY", {{GL_DTYPE, "TYPE", 0}, {GL_DTIME, "TIME", 0}, {GL_DFDBK, "FDBK", 0}, {P_DLY, "LEVEL", 0}}},
    {B_FX, "EFFECTS", "2 REVERB", {{GL_RTYPE, "TYPE", 0}, {GL_RSIZE, "SIZE", 0}, {GL_RDAMP, "DAMP", 0}, {P_REV, "LEVEL", 0}}},
};
#undef NO
#define GU_NPAGE NELEM(GU_PAGES)

static struct {
    uint8_t page, prog, dirty, ready;
    uint8_t save, save_slot, cur;                /* WRITE: open, its slot, the letter KNOB 2 changes */
    uint8_t step;                                /* ARP STEPS: the step KNOB 2 points at (0..7) */
    uint8_t sstep;                               /* SEQUENCER: the step being edited (0..15) */
    char name[13], sname[13];
    char toast[24];
    uint32_t toast_t, hot_t;
    int8_t hot;                                  /* the slider last moved (its value on the LCD a moment), -1 none */
    uint32_t sig_head, sig_lcd, sig_card[4], sig_foot;
    uint8_t shown_save, shown_uboot;
} gu;

static track_t *gu_t(void) { return &trk[0]; }

/* --------------------------------------------------------------- programs --- */
/* 1.2: the values a record keeps past its P_COUNT (packed[GU_XTRA_AT + k], the value itself: nothing before read or
 * wrote there, so a program of 1.0 / 1.1 has 0, SUB and DRIFT off). Their slots in the record hold what they held
 * before (the SLICER's defaults), so firmware before loads a 1.2 program as it would have, without them */
static const uint8_t GU_XTRA[3] = {GL_SUBTYPE, GL_SUB, GL_DRIFT};
#define GU_XTRA_AT P_COUNT
_Static_assert(GU_XTRA_AT + NELEM(GU_XTRA) <= UP_CAT_AT, "Golana's extra values: before the category byte");

static void gu_prog_values(uint32_t k, int16_t *v, char *name)
{
    if (up_used(k) && up_rec(k)->engine == ENGI_GOLANA) {
        const up_rec_t *r = up_rec(k);
        uint32_t i;
        up_values(r, v);
        for (i = 0; i < NELEM(GU_XTRA); i++)
            v[GU_XTRA[i]] = (int16_t)param_fit(param_desc_of(ENGI_GOLANA, GU_XTRA[i]),
                                               r->ver >= 4u && r->np <= GU_XTRA_AT ? r->packed[GU_XTRA_AT + i] : 0);
        up_name(k, name);
    } else {
        gl_factory(k < GL_NFACT ? k : 0xFFu, v, name);
    }
}
static int gu_user(uint32_t k) { return up_used(k) && up_rec(k)->engine == ENGI_GOLANA; }

static void gu_load(uint32_t k)
{
    int16_t v[P_COUNT];
    char nm[16];
    k %= GL_NPROG;
    gu_prog_values(k, v, nm);
    panic_req |= 1u;                              /* what part 1 sounds ends (the voice mode may change) */
    fm1_irq_off();
    gl_apply(gu_t(), v);
    if (gu_user(k)) {                                 /* its sequencer pattern (16 steps), its LENGTH */
        load_pat16(gu_t(), up_rec(k)->note, up_rec(k)->flags);
    } else {
        static const uint8_t NONE[16];
        const uint8_t *nt = NONE, *fl = NONE;
        load_pat16(gu_t(), nt, fl);
    }
    gu_t()->p[P_SLEN] = (int16_t)clamp(v[P_SLEN], 1, 16);
    fm1_irq_on();
    gu.prog = (uint8_t)k;
    str_cpy(gu.name, nm, 13);
    gu.dirty = 0;
}

/* WRITE: part 1's sound -> slot k, named name; 0 ok */
static int gu_store(uint32_t k, const char *name)
{
    up_rec_t r;
    uint32_t i;
    int rc;
    memset(&r, 0, sizeof r);
    r.used = UP_USED;
    r.ver = UP_VER;
    r.engine = ENGI_GOLANA;
    r.np = P_COUNT;
    up_set_name(&r, k, name);
    for (i = 0; i < P_COUNT; i++)
        up_set_value(&r, i, (int16_t)clamp(gu_t()->p[i], -64, 191));
    for (i = 0; i < NELEM(GU_XTRA); i++) {            /* (1.2: SUB, DRIFT) */
        r.packed[GU_XTRA_AT + i] = (uint8_t)clamp(gu_t()->p[GU_XTRA[i]], 0, 127);
        up_set_value(&r, GU_XTRA[i], TP[GU_XTRA[i]].def);
    }
    up_pat_from(&r, gu_t()->step);                    /* the sequencer's 16 steps with the sound */
    rc = up_put(k, &r);
    return rc == 0 || rc == 3 ? 0 : rc;
}

/* ------------------------------------------------------------- parameters --- */
static const param_desc_t *gu_desc(uint32_t id) { return param_desc_of(ENGI_GOLANA, id); }

/* the step sequencer (seq.c plays part 1's steps): 16 steps, saved with the program (upreset.c's pattern) */
static uint32_t gu_slen(void) { return (uint32_t)clamp(gu_t()->p[P_SLEN], 1, 16); }
static step_t *gu_sst(void) { return &gu_t()->step[gu.sstep % 16u]; }
static int32_t gu_stype(const step_t *st) { return st->time == ST_TIE ? 2 : st->time == ST_NOTE && st->n ? 1 : 0; }
static void gu_snote_set(step_t *st, uint32_t note)   /* the step a NOTE of note (velocity 96, as a pattern's) */
{
    st->note[0] = (uint8_t)note;
    st->n = 1;
    st->vel = 96;
    st->time = ST_NOTE;
}
static void gu_range(const gu_par_t *q, int32_t *lo, int32_t *hi)
{
    switch (q->id) {
    case GV_RES: *lo = 0; *hi = 5; return;
    case GV_BPM: *lo = GP[G_BPM].min; *hi = GP[G_BPM].max; return;
    case GV_TUNE: *lo = GP[G_TUNE].min; *hi = GP[G_TUNE].max; return;
    case GV_CLOCK: *lo = 0; *hi = 2; return;
    case GV_STEP: *lo = 1; *hi = 8; return;
    case GV_STEPON: *lo = 0; *hi = 1; return;
    case GV_MIDI: *lo = 0; *hi = 1; return;
    case GV_SSTEP: *lo = 1; *hi = gu_slen(); return;
    case GV_SNOTE: *lo = 0; *hi = 127; return;
    case GV_STYPE: *lo = 0; *hi = 2; return;
    case GV_SFLAG: *lo = 0; *hi = 3; return;
    case GV_SLEN: *lo = 1; *hi = 16; return;
    default:
        if (q->names) {
            *lo = 0;
            *hi = (int32_t)q->nn - 1;
            if (q->id < P_COUNT && gu_desc(q->id)->max < *hi)
                *hi = gu_desc(q->id)->max;
            return;
        }
        *lo = gu_desc(q->id)->min;
        *hi = gu_desc(q->id)->max;
    }
}

static uint32_t gu_steps(void)
{
    const int16_t *p = gu_t()->p;
    return (uint32_t)(p[GL_ARPST_A] & 15) | (uint32_t)(p[GL_ARPST_B] & 15) << 4;
}

static int32_t gu_get(const gu_par_t *q)
{
    const int16_t *p = gu_t()->p;
    uint32_t i;
    switch (q->id) {
    case GV_RES:
        for (i = 0; i < 6u && GU_RES_DIV[i] != p[P_ARATE]; i++)
            ;
        return i < 6u ? (int32_t)i : 2;
    case GV_BPM: return song.g[G_BPM];
    case GV_TUNE: return song.g[G_TUNE];
    case GV_CLOCK: return clamp(song.g[G_CLOCK], 0, 2);
    case GV_STEP: return gu.step + 1;
    case GV_STEPON: return (int32_t)((gu_steps() >> gu.step) & 1u);
    case GV_MIDI: return song.g[G_ROUTE] == 1 ? 0 : 1;
    case GV_SSTEP: return gu.sstep + 1;
    case GV_SNOTE: return gu_sst()->n ? gu_sst()->note[0] : 60;
    case GV_STYPE: return gu_stype(gu_sst());
    case GV_SFLAG: return gu_sst()->flags & 3;
    case GV_SLEN: return (int32_t)gu_slen();
    case GV_NONE: return 0;
    default: return p[q->id];
    }
}

static void gu_set(const gu_par_t *q, int32_t v)
{
    track_t *t = gu_t();
    int32_t lo, hi;
    gu_range(q, &lo, &hi);
    v = clamp(v, lo, hi);
    switch (q->id) {
    case GV_RES: t->p[P_ARATE] = GU_RES_DIV[v]; break;
    case GV_BPM: song.g[G_BPM] = (int16_t)v; return;
    case GV_TUNE: song.g[G_TUNE] = (int16_t)v; return;
    case GV_CLOCK: song.g[G_CLOCK] = (int16_t)v; return;
    case GV_STEP: gu.step = (uint8_t)(v - 1); return;
    case GV_MIDI: song.g[G_ROUTE] = (int16_t)(v ? 0 : 1); return;
    case GV_SSTEP: gu.sstep = (uint8_t)(v - 1); return;
    case GV_SNOTE: gu_snote_set(gu_sst(), (uint32_t)v); gu.dirty = 1; return;
    case GV_STYPE: {
        step_t *st = gu_sst();
        if (v == 1)
            gu_snote_set(st, st->n ? st->note[0] : 60);
        else {
            st->time = v == 2 ? ST_TIE : ST_REST;
            st->n = 0;
            st->flags &= (uint8_t)~3u;
        }
        gu.dirty = 1;
        return;
    }
    case GV_SFLAG: {
        step_t *st = gu_sst();
        if (gu_stype(st) == 1)
            st->flags = (uint8_t)((st->flags & ~3u) | (uint32_t)v);
        gu.dirty = 1;
        return;
    }
    case GV_SLEN: t->p[P_SLEN] = (int16_t)v; gu.sstep = (uint8_t)(gu.sstep % (uint32_t)v); gu.dirty = 1; return;
    case GV_STEPON: {
        uint32_t m = (gu_steps() & ~(1u << gu.step)) | (uint32_t)(v != 0) << gu.step;
        t->p[GL_ARPST_A] = (int16_t)(m & 15u);
        t->p[GL_ARPST_B] = (int16_t)(m >> 4);
        break;
    }
    case GV_NONE: return;
    default:
        if (q->id == P_VOICE && t->p[P_VOICE] != v)
            panic_req |= 1u;                        /* the voices restart in the new mode */
        t->p[q->id] = (int16_t)v;
        break;
    }
    gl_sync(t);
    gu.dirty = 1;
}

/* the value as shown: a name, or the number (signed when the range is) */
static void gu_text(const gu_par_t *q, char *b)
{
    int32_t v = gu_get(q), lo, hi;
    const char *const *names = q->names;
    if (q->id == GV_NONE) {
        b[0] = 0;
        return;
    }
    if (q->id == GV_SNOTE || q->id == DT_SPLIT) {      /* a note name (C4 = 60), "--" on a rest or a tie */
        static const char *const NN[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        if (q->id == GV_SNOTE && gu_stype(gu_sst()) != 1) {
            str_cpy(b, "--", 4);
            return;
        }
        str_cpy(b, NN[v % 12], 4);
        fmt_int(b + str_len(b), v / 12 - 1);
        return;
    }
    if (!names && q->id < P_COUNT && gu_desc(q->id)->fmt == F_ENUM)
        names = gu_desc(q->id)->names;
    gu_range(q, &lo, &hi);
    if (names && v >= 0 && v <= hi) {
        str_cpy(b, names[v], 12);
        return;
    }
    if (lo < 0 && v > 0) {
        b[0] = '+';
        fmt_int(b + 1, v);
    } else {
        fmt_int(b, v);
    }
}

/* -------------------------------------------------------------- drawing --- */
#include "lcd_font.h"                           /* the LCD's 5 x 7 characters (tools/gen_lcd_font.py) */
#include "golana_gfx.c"                         /* the power-on */
static uint32_t gu_hash(uint32_t h, const char *s)
{
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h * 16777619u;
}
static uint32_t gu_hashv(uint32_t h, int32_t v) { return (h ^ (uint32_t)v) * 16777619u; }

static int gu_toast_on(void) { return gu.toast[0] && fm1_ms - gu.toast_t < 1400u; }
static void gu_say(const char *a, const char *b)
{
    str_cpy(gu.toast, a, sizeof gu.toast);
    if (b)
        str_cpy(gu.toast + str_len(gu.toast), b, sizeof gu.toast - str_len(gu.toast));
    gu.toast_t = fm1_ms;
}

/* ---- the LCD: 2 lines of 16 cells, each a 5 x 7 dot matrix of 2 x 2 px dots, 13 px apart */
#define LCD_X 14                                     /* the first cell, on the canvas of the LCD (x 4, y 26) */
#define LCD_Y 8
#define LCD_DX 13
#define LCD_DY 22
static void lcd_char(int32_t col, int32_t row, char c, int inv)
{
    int32_t x0 = LCD_X + col * LCD_DX, y0 = LCD_Y + row * LCD_DY, x, y;
    const uint8_t *g;
    if (c >= 'a' && c <= 'z')
        c = (char)(c - 32);                          /* (no lower case on the LCD) */
    if (c < 32 || c > 95)
        c = ' ';
    g = LCD_FONT[c - 32];
    cv_rect(x0 - 1, y0 - 1, 12, 16, inv ? GC_LCD_ON : GC_LCD_OFF);
    for (y = 0; y < 7; y++)
        for (x = 0; x < 5; x++)
            if (((g[y] >> (4 - x)) & 1u) != (inv != 0))
                cv_rect(x0 + 2 * x, y0 + 2 * y, 2, 2, GC_LCD_ON);
            else if (inv)
                cv_rect(x0 + 2 * x, y0 + 2 * y, 2, 2, GC_LCD);
}
/* a line: s from column 0, the rest blank; inv: the columns lit inverse (bit k = column k) */
static void lcd_line(int32_t row, const char *s, uint32_t inv)
{
    int32_t k, end = 0;
    for (k = 0; k < 16; k++) {
        char c = s[end] ? s[end++] : ' ';
        lcd_char(k, row, c, (int)((inv >> k) & 1u));
    }
}
/* a: the left part, b: right-aligned in 16 columns -> out (17 bytes) */
static void lcd_lr(char *out, const char *a, const char *b)
{
    uint32_t i, la = str_len(a), lb = str_len(b);
    for (i = 0; i < 16u; i++)
        out[i] = ' ';
    out[16] = 0;
    for (i = 0; i < la && i < 16u; i++)
        out[i] = a[i];
    for (i = 0; i < lb && i < 16u; i++)
        out[16u - lb + i] = b[i];
}

/* ---- the name strip: DITTO, the preset (P01, a dot when edited), the battery */
static void gu_draw_head(void)
{
    char n[4];
    uint32_t h = gu_hashv(0x811C9DC5u, (int32_t)(gu.prog | gu.dirty << 8 | (uint32_t)batt_shown() << 12 |
                                               (uint32_t)(song.octave + 8) << 16 | (uint32_t)(song.playing != 0) << 20));
    int32_t x;
    if (h == gu.sig_head)
        return;
    gu.sig_head = h;
    cv_begin(240, 24, GC_STRIP);
    x = cv_text(8, 3, &AF_M, "DITTO", GC_PRINT);
    cv_text(x + 8, 7, &AF_S, "SAMPLER", RGB(150, 148, 140));
    {   /* the battery, right; the preset number before it, in the LEDs' red */
        int32_t lvl = batt_shown();
        cv_icon_mid(240 - 22, 12, 24, batt_icon(lvl), lvl == 1 ? GC_LED : GC_PRINT, GC_STRIP);
    }
    n[0] = 'P';
    n[1] = (char)('0' + (gu.prog + 1u) / 10u);
    n[2] = (char)('0' + (gu.prog + 1u) % 10u);
    n[3] = 0;
    x = 240 - 42 - text_w(&AF_M, n);
    cv_text(x, 3, &AF_M, n, GC_LED);
    if (gu.dirty)
        cv_rect(x - 8, 11, 4, 4, GC_LED);
    if (song.playing)
        cv_text(x - 50, 7, &AF_S, "PLAY", GC_LED);
    else if (song.octave) {
        char o[6] = {'O', 'C', 'T', song.octave > 0 ? '+' : '-', (char)('0' + (song.octave > 0 ? song.octave : -song.octave)), 0};
        cv_text(x - 50, 7, &AF_S, o, GC_PRINT);
    }
    cv_blit(0, 0);
}

static void gu_text(const gu_par_t *q, char *b);
/* the sequencer's 16 steps on the LCD's second line: a note #, a tie -, a rest .; past LENGTH blank; the step being
 * edited lit inverse, the one playing as > */
static uint32_t gu_steps_line(char *l)
{
    uint32_t st, len = gu_slen(), play = song.playing ? gu_t()->seq_idx % len : 99u;
    for (st = 0; st < 16u; st++) {
        int32_t ty = gu_stype(&gu_t()->step[st]);
        l[st] = st >= len ? ' ' : st == play ? '>' : ty == 1 ? '#' : ty == 2 ? '-' : '.';
    }
    l[16] = 0;
    return 1u << gu.sstep;
}
static uint32_t gu_steps(void);
static void gu_draw_lcd(void)
{
    const gu_page_t *pg = &GU_PAGES[gu.page];
    char l1[17], l2[17], v[16], b[24];
    uint32_t inv = 0, h;
    int hot = gu.hot >= 0 && fm1_ms - gu.hot_t < 1200u && pg->k[gu.hot & 3].id != GV_NONE;
    if (ui.uboot) {
        char d[2] = {(char)('0' + ui.uboot % 10u), 0};
        lcd_lr(l1, "UPDATE MODE IN", d);
        lcd_lr(l2, "RELEASE OCT-/+", "");
    } else if (gu.save) {
        static const char BL[] = "                ";
        lcd_lr(l1, "WRITE PRESET", "");
        l1[13] = 'P';
        l1[14] = (char)('0' + (gu.save_slot + 1u) / 10u);
        l1[15] = (char)('0' + (gu.save_slot + 1u) % 10u);
        str_cpy(l2, BL, 17);
        for (h = 0; h < 12u && gu.sname[h]; h++)
            l2[h] = gu.sname[h];
        inv = (fm1_ms / 400u) & 1u ? 1u << gu.cur : 0u;  /* the letter KNOB 2 changes, blinking */
    } else {
        if (pg->btn == B_HOME) {
            str_cpy(b, "P", 2);
            b[1] = (char)('0' + (gu.prog + 1u) / 10u);
            b[2] = (char)('0' + (gu.prog + 1u) % 10u);
            b[3] = ' ';
            b[4] = 0;
            str_cpy(b + 4, gu.name, 13);
            lcd_lr(l1, b, "");
        } else {
            const char *sp = pg->tshort, *fn = sp;
            char num[4] = {0};
            uint32_t k;
            for (k = 0; sp[k] && sp[k] != ' ' && k < 3u; k++)   /* the function's number, right on line 1 */
                num[k] = sp[k];
            fn = sp + k + (sp[k] == ' ');
            lcd_lr(l1, pg->tab, num);
            lcd_lr(l2, fn, "");
        }
        if (gu_toast_on()) {
            lcd_lr(l2, gu.toast, "");
        } else if (hot) {                               /* the slider just moved: its name = its value */
            const gu_par_t *q = &pg->k[gu.hot & 3];
            gu_text(q, v);
            str_cpy(b, q->label, 12);
            str_cpy(b + str_len(b), " ", 2);
            lcd_lr(l2, b, v);
        } else if (pg->k[0].id == GV_SSTEP || pg->k[0].id == GV_SLEN) {
            inv = gu_steps_line(l2);
            if (pg->k[0].id == GV_SLEN)
                inv = 0;
        } else if (pg->k[1].id == GV_STEP) {           /* ARP STEPS: the 8 steps, on, off, past LAST */
            uint32_t st, m = gu_steps(), last = (uint32_t)clamp(gu_t()->p[GL_ARPLAST], 1, 8);
            lcd_lr(l2, "STEPS", "");
            for (st = 0; st < 8u; st++)
                l2[8 + st] = st >= last ? ' ' : (m >> st) & 1u ? '#' : '.';
            inv = 1u << (8u + gu.step);
        } else if (pg->btn == B_HOME) {
            char t[8];
            fmt_int(t, song.g[G_BPM]);
            str_cpy(t + str_len(t), " BPM", 5);
            lcd_lr(l2, gu_t()->p[GL_ARPON] ? (gu_t()->p[P_AHOLD] ? "ARP LATCH" : "ARP") : "", t);
        }
    }
    h = gu_hash(gu_hash(0x811C9DC5u, l1), l2);
    h = gu_hashv(h, (int32_t)inv);
    if (h == gu.sig_lcd)
        return;
    gu.sig_lcd = h;
    cv_begin(232, 54, GC_PANEL);
    cv_rrect(0, 0, 232, 54, 6, GC_BEZEL, GC_PANEL);
    cv_rect(6, 4, 220, 46, GC_LCD);
    lcd_line(0, l1, 0);
    lcd_line(1, l2, inv);
    cv_blit(4, 28);
}

/* ---- the sliders (KNOB 1-4): the name printed above, a slot with its cap at the value, the value under it */
#define GU_SL_Y 88
#define GU_SL_H 110
static void gu_draw_card(uint32_t k)
{
    const gu_page_t *pg = &GU_PAGES[gu.page];
    const gu_par_t *q = &pg->k[k];
    char b[16];
    int32_t v = 0, lo = 0, hi = 0, top = 28, bot = 82, cy, w = 60, cx = 30;
    uint32_t h = 0x811C9DC5u;
    int hot = gu.hot == (int8_t)k && fm1_ms - gu.hot_t < 1200u;
    if (q->id != GV_NONE) {
        v = gu_get(q);
        gu_range(q, &lo, &hi);
        gu_text(q, b);
        h = gu_hash(gu_hash(h, b), q->label);
        h = gu_hashv(h, v * 2 + hot);
    }
    h = gu_hashv(h, (int32_t)(gu.page | k << 8 | (q->id == GV_NONE) << 12));
    if (h == gu.sig_card[k])
        return;
    gu.sig_card[k] = h;
    cv_begin((uint32_t)w, GU_SL_H, GC_PANEL);
    if (q->id == GV_NONE) {                             /* an unused slider: its slot, no cap */
        cv_rect(cx - 2, top, 4, bot - top, GC_PANEL2);
        cv_blit(k * 60u, GU_SL_Y);
        return;
    }
    cv_text(cx - text_w(&AF_S, q->label) / 2, 2, &AF_S, q->label, GC_INK);
    {   /* the slot, ticks beside it every eighth */
        int32_t t;
        cv_rect(cx - 3, top, 6, bot - top, GC_STRIP);
        for (t = 0; t <= 8; t++)
            cv_rect(cx + 8, top + (bot - top) * t / 8, t % 4 ? 4 : 7, 1, GC_MIDI);
    }
    if (lo < 0 && hi > 0)                               /* bipolar: the middle marked */
        cv_rect(cx - 12, (top + bot) / 2, 6, 1, GC_INK);
    cy = bot - (hi > lo ? (v - lo) * (bot - top) / (hi - lo) : 0);
    cv_rect(cx - 13, cy - 5, 26, 11, hot ? GC_EDGE : GC_CAP);   /* the cap, its grip line */
    cv_rect(cx - 13, cy - 5, 26, 1, GC_EDGE);
    cv_rect(cx - 13, cy + 5, 26, 1, GC_MIDI);
    cv_rect(cx - 9, cy, 18, 1, GC_INK);
    {
        int32_t tw = text_w(&AF_S, b);
        cv_text(cx - (tw < w - 2 ? tw : w - 2) / 2, 88, &AF_S, b, hot ? GC_LED : GC_INK);
    }
    cv_blit(k * 60u, GU_SL_Y);
}

/* ---- the modules' LEDs: the one whose function is open lit */
static const uint8_t GU_MODS[8] = {B_GLO, B_SCL, B_ENV, B_EDIT, B_SEQ, B_REC, B_FX, B_SAVE};
static const char *const GU_MODN[8] = {"MASTER", "FILTER", "LFO/VCA", "VOICE", "PRESET", "SEQ", "FX", "DISK"};
#define GU_FOOT_Y 202
static void gu_draw_foot(void)
{
    uint32_t b = gu.save ? B_SAVE : GU_PAGES[gu.page].btn, k;
    uint32_t h = gu_hashv(0x811C9DC5u, (int32_t)b);
    if (h == gu.sig_foot)
        return;
    gu.sig_foot = h;
    cv_begin(240, 38, GC_STRIP);
    for (k = 0; k < 8u; k++) {                          /* two rows of four: an LED, the module's name */
        int32_t x = (int32_t)(k & 3u) * 60 + 8, y = (int32_t)(k >> 2) * 18 + 2;
        cv_rrect(x, y + 4, 7, 7, -3, GU_MODS[k] == b ? GC_LED : GC_LED_OFF, GC_STRIP);
        cv_text(x + 11, y, &AF_S, GU_MODN[k], GU_MODS[k] == b ? RGB(250, 248, 240) : GC_PRINT);
    }
    cv_blit(0, GU_FOOT_Y);
}

/* WRITE: the LCD says the slot and the name; under it what each control does */
static void gu_draw_save(void)
{
    if (gu.shown_save)
        return;
    gu.shown_save = 1;
    gu.sig_card[0] = gu.sig_card[1] = gu.sig_card[2] = gu.sig_card[3] = 0;
    cv_begin(240, GU_SL_H, GC_PANEL);
    cv_text(12, 8, &AF_S, "PRESETS   the slot", GC_INK);
    cv_text(12, 28, &AF_S, "KNOB 1   the letter", GC_INK);
    cv_text(12, 48, &AF_S, "KNOB 2   change it", GC_INK);
    cv_text(12, 76, &AF_S, "SAVE   write", GC_INK);
    cv_text(130, 76, &AF_S, "HOME   cancel", GC_INK);
    cv_blit(0, GU_SL_Y);
}

static void gu_redraw_all(void)
{
    uint32_t k;
    gu.sig_head = gu.sig_lcd = gu.sig_foot = 0;
    for (k = 0; k < 4u; k++)
        gu.sig_card[k] = 0;
    gu.shown_save = 0;
    lcd_fill(0, 0, 240, 240, GC_PANEL);
}

static void gu_draw(void)
{
    uint32_t k;
    if (!gu.ready)
        return;
    if (ux.mono || ux.light || ux.style != ST_FLAT) {   /* Ditto's own colours (a stored Felucca palette, GREY or */
        ux.mono = 0;                                     /* PAPER, would grey or invert them) */
        ux.light = 0;
        ux.style = ST_FLAT;
        ux.gen++;
        ui.force = 1;
    }
    if (ui.force) {                                  /* (main.c: after the splash, an update, a cancelled UBOOT) */
        ui.force = 0;
        gu_redraw_all();
    }
    gu_draw_head();
    gu_draw_lcd();
    gu_draw_foot();
    if (ui.uboot)
        return;
    if (gu.save) {
        gu_draw_save();
        return;
    }
    if (gu.shown_save) {
        gu.shown_save = 0;
        lcd_fill(0, GU_SL_Y, 240, GU_SL_H, GC_PANEL);
    }
    for (k = 0; k < 4u; k++)
        gu_draw_card(k);
}

/* ---------------------------------------------------------------- LEDs --- */
static uint32_t gu_group_btn(void) { return GU_PAGES[gu.page].btn; }

static void gu_leds(void)
{
    uint8_t nl[FM1_NCOL] = {0}, nd[FM1_NCOL] = {0};
    uint32_t k, c;
    static uint8_t ready;
    if (fm1_led_anim_on())
        return;
    if (!ready) {
        led_pos_init();
        ready = 1;
    }
    for (k = 0; k < NB; k++)
        led_put(nd, panel.btn[k], 1);
    for (k = 0; k < 27u; k++)
        led_put(nd, 14u + k, 1);
    led_put(nl, panel.btn[gu.save ? B_SAVE : gu_group_btn()], 1);
    led_put(nl, panel.btn[B_ARP], gu_t()->p[GL_ARPON] != 0);
    if (song.playing)                                   /* PLAY: its green LED while the sequencer plays */
        nl[LED_PLAY_GREEN >> 3] |= (uint8_t)(1u << (LED_PLAY_GREEN & 7u));
    led_put(nl, panel.btn[B_OCTDN], song.octave < 0);
    led_put(nl, panel.btn[B_OCTUP], song.octave > 0);
    for (k = 0; k < 27u; k++)
        led_put(nl, 14u + k, (int)((fm1_in.notes >> k) & 1u));
    if (GU_PAGES[gu.page].k[1].id == GV_STEP) {           /* SEQ STEPS: the first 8 keys show the steps */
        uint32_t m = gu_steps(), last = (uint32_t)clamp(gu_t()->p[GL_ARPLAST], 1, 8);
        for (k = 0; k < 8u; k++)
            led_put(nl, 14u + k, k < last && ((m >> k) & 1u));
    }
    fm1_led_dim_level(0);
    for (c = 0; c < FM1_NCOL; c++) {
        nd[c] &= (uint8_t)~nl[c];
        fm1_led_dim[c] = nd[c];
        fm1_led_breath[c] = 0;
        fm1_led_mid[c] = 0;
    }
    for (c = 0; c < FM1_NCOL; c++)
        fm1_led[c] = nl[c];
}

/* --------------------------------------------------------------- input --- */
static void gu_go(uint32_t page)
{
    gu.page = (uint8_t)(page % GU_NPAGE);
}

/* a page button: its first page, or (on one of its pages) the next of them, round */
static void gu_button(uint32_t b)
{
    uint32_t pi, first = GU_NPAGE;
    if (b == B_LFO) {                                   /* LFO: straight to LFO/VCA's LFO, then on through the module */
        for (pi = 0; pi < GU_NPAGE && GU_PAGES[pi].k[0].id != GL_L1WAVE; pi++)
            ;
        if (GU_PAGES[gu.page].btn != B_ENV && pi < GU_NPAGE) {
            gu_go(pi);
            return;
        }
        b = B_ENV;
    }
    for (pi = 0; pi < GU_NPAGE; pi++)
        if (GU_PAGES[pi].btn == b) {
            first = pi;
            break;
        }
    if (first == GU_NPAGE)
        return;
    if (GU_PAGES[gu.page].btn == b) {
        pi = gu.page + 1u;
        gu_go(pi < GU_NPAGE && GU_PAGES[pi].btn == b ? pi : first);
    } else {
        gu_go(first);
    }
}

static int32_t gu_accel(int32_t s, int32_t range)
{
    int32_t a = s < 0 ? -s : s;
    if (range > 32 && a >= 2)
        s *= a >= 4 ? 4 : 2;
    return s;
}

static void gu_turn(uint32_t k, int32_t s)
{
    const gu_par_t *q = &GU_PAGES[gu.page].k[k];
    int32_t lo, hi;
    if (q->id == GV_NONE || !s)
        return;
    gu_range(q, &lo, &hi);
    gu_set(q, gu_get(q) + gu_accel(s, hi - lo));
    gu.hot = (int8_t)k;
    gu.hot_t = fm1_ms;
}

static void gu_save_open(void)
{
    gu.save = 1;
    gu.save_slot = gu.prog;
    gu.cur = 0;
    memset(gu.sname, 0, sizeof gu.sname);
    str_cpy(gu.sname, gu.name, 13);
}

static const char GU_ABC[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-./#+";
static void gu_letter(int32_t s)
{
    uint32_t n = sizeof GU_ABC - 1u, i, len;
    char c = gu.sname[gu.cur] ? gu.sname[gu.cur] : ' ';
    for (i = 0; i < n && GU_ABC[i] != c; i++)
        ;
    i = (uint32_t)(((int32_t)(i % n) + s % (int32_t)n + (int32_t)n) % (int32_t)n);
    for (len = str_len(gu.sname); len < gu.cur; len++)
        gu.sname[len] = ' ';                         /* (letters before the cursor: spaces) */
    gu.sname[gu.cur] = GU_ABC[i];
    for (len = 12u; len > 0u && (gu.sname[len - 1u] == ' ' || !gu.sname[len - 1u]); len--)
        gu.sname[len - 1u] = 0;                      /* (no trailing spaces) */
}

static void gu_input(void)
{
    uint32_t pressed = fm1_input_edges(0), id, b, k, notes;
    int32_t s;
    notes = fm1_input_note_edges();                  /* (the keys sound from the audio ISR: seq.c) */
    if (!gu.ready)
        return;
    for (k = 0; k < NTRK; k++)                       /* MIDI Program Change: a program */
        if (ui.mpc[k]) {
            uint32_t v;
            fm1_irq_off();
            v = ui.mpc[k];
            ui.mpc[k] = 0;
            fm1_irq_on();
            gu_load((v & 127u) % GL_NPROG);
        }
    if (ui.uboot)
        return;
    if (notes && !gu.save && GU_PAGES[gu.page].k[0].id == GV_SSTEP) {   /* REC PATTERN: a key writes its note */
        for (k = 0; k < 27u; k++)                         /* into the step and moves on (step entry) */
            if ((notes >> k) & 1u) {
                gu_snote_set(gu_sst(), kb_map(gu_t(), k));
                gu.sstep = (uint8_t)((gu.sstep + 1u) % gu_slen());
                gu.dirty = 1;
            }
    }
    s = panel_enc(EN_PRESET);
    if (s) {
        if (gu.save) {
            gu.save_slot = (uint8_t)(((int32_t)gu.save_slot + s % (int32_t)GL_NPROG + (int32_t)GL_NPROG) %
                                     (int32_t)GL_NPROG);
        } else {
            gu_load((uint32_t)(((int32_t)gu.prog + s % (int32_t)GL_NPROG + (int32_t)GL_NPROG) % (int32_t)GL_NPROG));
        }
    }
    if (gu.save) {
        s = panel_enc(EN_K1);
        if (s)
            gu.cur = (uint8_t)clamp((int32_t)gu.cur + s, 0, 11);
        s = panel_enc(EN_K2);
        if (s)
            gu_letter(s);
        (void)panel_enc(EN_K3);
        (void)panel_enc(EN_K4);
        (void)panel_enc(EN_SELECT);
        (void)panel_enc(EN_ALGO);
    } else {
        s = panel_enc(EN_SELECT);
        if (s)
            gu_go((uint32_t)(((int32_t)gu.page + s % (int32_t)GU_NPAGE + (int32_t)GU_NPAGE) % (int32_t)GU_NPAGE));
        for (k = 0; k < 4u; k++)
            gu_turn(k, panel_enc(EN_K1 + k));
        s = panel_enc(EN_ALGO);
        if (s) {                                     /* CUTOFF, from any page */
            static const gu_par_t CUT = {GL_CUTOFF, "CUTOFF", 0};
            char v[8];
            gu_set(&CUT, gu_get(&CUT) + gu_accel(s, 127));
            gu_text(&CUT, v);
            if (GU_PAGES[gu.page].k[1].id != GL_CUTOFF && GU_PAGES[gu.page].k[0].id != GL_CUTOFF)
                gu_say("CUTOFF ", v);
        }
    }
    for (id = 0; id < 32u; id++) {
        if (!((pressed >> id) & 1u))
            continue;
        b = panel_btn_of(id);
        if (b >= NB)
            continue;
        if (gu.save && b != B_PLAY) {                /* (PLAY works on WRITE: STOP TO SAVE can be done there) */
            if (b == B_SAVE) {
                int rc;
                if (transport_busy()) {                  /* (no flash erase while playing: up_put would refuse it, its */
                    gu_say("STOP TO SAVE", 0);           /* message is Felucca's UI's) the WRITE screen stays */
                    continue;
                }
                if (!gu.sname[0])
                    str_cpy(gu.sname, "PROGRAM", 13);
                rc = gu_store(gu.save_slot, gu.sname);
                if (!rc) {
                    gu.prog = gu.save_slot;
                    str_cpy(gu.name, gu.sname, 13);
                    gu.dirty = 0;
                    gu_say("WRITTEN", 0);
                } else {
                    gu_say("NOT WRITTEN", 0);            /* (a flash error: the slot holds what it held) */
                }
                gu.save = 0;
            } else if (b == B_HOME) {
                gu.save = 0;
                gu_say("CANCELLED", 0);
            }
            continue;
        }
        switch (b) {
        case B_HOME:
        case B_EDIT:
        case B_SCL:
        case B_ENV:
        case B_LFO:
        case B_GLO:
        case B_REC:
        case B_SEQ:
        case B_FX:
            gu_button(b);
            break;
        case B_ARP: {
            static const gu_par_t A = {GL_ARPON, "ARP", 0};
            gu_set(&A, !gu_get(&A));
            gu_say("ARP ", gu_get(&A) ? "ON" : "OFF");
            break;
        }
        case B_PLAY:                                     /* the sequencer: play / stop */
            transport_req = song.playing || seq_counting() ? 2u : 1u;
            gu_say(song.playing ? "STOP" : "PLAY", 0);
            break;
        case B_OCTDN:
        case B_OCTUP:
            if ((fm1_in.buttons & ((1u << panel.btn[B_OCTDN]) | (1u << panel.btn[B_OCTUP]))) ==
                ((1u << panel.btn[B_OCTDN]) | (1u << panel.btn[B_OCTUP])))
                break;                               /* (both: the UPDATE MODE gesture, main.c) */
            song.octave += b == B_OCTDN ? (song.octave > -3 ? -1 : 0) : (song.octave < 3 ? 1 : 0);
            {
                char o[4] = {song.octave > 0 ? '+' : song.octave < 0 ? '-' : ' ',
                             (char)('0' + (song.octave < 0 ? -song.octave : song.octave)), 0, 0};
                gu_say("OCTAVE ", o[0] == ' ' ? o + 1 : o);
            }
            break;
        case B_SAVE:
            gu_save_open();
            break;
        default:
            break;
        }
    }
}

/* power-on (main.c, after felucca_init): MIDI from every channel, program 1 */
static void gu_boot(void)
{
    song.g[G_ROUTE] = 1;                             /* SEL: every channel plays part 1 */
    song.sel = 0;
    gu.hot = -1;
    gu.shown_uboot = 0xFF;
    gu_load(0);
    gu.ready = 1;
}
