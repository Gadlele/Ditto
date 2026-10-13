/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele, based on Felucca (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Golana's interface: the MATRIX. In place of Felucca's UI (main.c and the browser build call gu_input, gu_leds and
 * gu_draw instead of ui_input, ui_leds and ui_draw; the rest of Felucca's UI code is never called).
 *
 * The screen (240 x 240): a header (the program, its name, ARP / LATCH / the octave), on the left the matrix's
 * sections (the one being edited lit), on the right four cards, one per knob: the section's parameters.
 *   SELECT     the section (and its pages: FILTER 1-2, PATCH 1-4, ARP 1-3), one turn a page
 *   KNOB 1..4  the four parameters on the cards
 *   ALGORITHM  CUTOFF, whatever the page (as the original's cutoff knob)
 *   PRESETS    the program, 1..32: it loads as the knob turns (edits not written are lost, as on the original)
 *   HOME       the PERFORM page; EDIT OSC 1 / OSC 2 / MIXER; SCL FILTER; ENV FILTER EG / AMP EG; LFO LFO 1 / LFO 2;
 *              GLO AMP / VOICE / GLOBAL; REC PATCH 1..4; SEQ ARP pages; FX MOD FX / DELAY / REVERB
 *              (a second press: the next one of the group)
 *   ARP        the arpeggiator on / off;  PLAY  LATCH on / off;  OCT- OCT+  the keys' octave
 *   SAVE       WRITE: PRESETS picks the slot, KNOB 1 the letter, KNOB 2 changes it, SAVE writes, HOME cancels
 * Programs: 32 slots in the user preset store (upreset.c); slots 1..16 play the factory sounds until written over,
 * 17..32 INIT. MIDI Program Change 0..31 loads a program. */

/* ---------------------------------------------------------------- colours --- */
#define GC_BG RGB(14, 17, 24)
#define GC_SURF RGB(26, 30, 40)
#define GC_RAISE RGB(38, 44, 58)
#define GC_LINE RGB(48, 54, 70)
#define GC_TEXT RGB(232, 230, 225)
#define GC_MID RGB(150, 156, 172)
#define GC_DIM RGB(88, 94, 110)
#define GC_AMBER RGB(255, 181, 71)
#define GC_TEAL RGB(92, 200, 192)
#define GC_PINK RGB(232, 122, 144)
#define GC_INK RGB(14, 17, 24)

/* ------------------------------------------------------------ the matrix --- */
/* virtual parameters (not a track parameter of part 1) */
enum { GV_NONE = 0xFF, GV_RES = 200, GV_BPM, GV_TUNE, GV_CLOCK, GV_STEP, GV_STEPON, GV_MIDI };
typedef struct { uint8_t id; const char *label; const char *const *names; } gu_par_t;
typedef struct { uint8_t btn; const char *tab, *tshort; gu_par_t k[4]; } gu_page_t;
static const char *const N_GVOICE[] = {"POLY", "MONO", "LEGATO", "UNISON"};
static const char *const N_GRES[] = {"1/32", "1/24", "1/16", "1/12", "1/8", "1/4"};
static const uint8_t GU_RES_DIV[6] = {3, 5, 2, 4, 1, 0};    /* N_GRES -> P_ARATE (params.c N_DIV) */
static const char *const N_GCLOCK[] = {"INT", "USB", "TRS"};
static const char *const N_GMIDI[] = {"ALL", "CH 1"};
/* the pages, grouped by the button that opens them (its pages side by side: the tabs). tab: the page's name on its
 * tab when it is open (after the button's name), tshort: on its tab while another page of the button is open */
static const gu_page_t GU_PAGES[] = {
    {B_HOME, "PERFORM", "PERFORM",
     {{GL_CUTOFF, "CUTOFF", 0}, {GL_RESO, "RESO", 0}, {GL_FEGINT, "EG INT", 0}, {GL_L1FREQ, "LFO1 FREQ", 0}}},
    {B_EDIT, "OSC 1", "OSC 1", {{GL_O1WAVE, "WAVE", 0}, {GL_O1C1, "CTRL 1", 0}, {GL_O1C2, "CTRL 2", 0}, {GV_NONE, 0, 0}}},
    {B_EDIT, "OSC 2", "OSC 2",
     {{GL_O2WAVE, "WAVE", 0}, {GL_O2MOD, "MOD", 0}, {GL_O2SEMI, "SEMITONE", 0}, {GL_O2TUNE, "TUNE", 0}}},
    {B_EDIT, "MIXER", "MIXER", {{GL_MIX1, "OSC 1", 0}, {GL_MIX2, "OSC 2", 0}, {GL_NOISE, "NOISE", 0}, {GV_NONE, 0, 0}}},
    {B_SCL, "FILTER", "FILTER",
     {{GL_FTYPE, "TYPE", 0}, {GL_CUTOFF, "CUTOFF", 0}, {GL_RESO, "RESO", 0}, {GL_FEGINT, "EG INT", 0}}},
    {B_SCL, "KEY TRACK", "KEY TRK", {{GL_KTRK, "KEY TRACK", 0}, {GV_NONE, 0, 0}, {GV_NONE, 0, 0}, {GV_NONE, 0, 0}}},
    {B_ENV, "FILTER EG", "FILTER EG",
     {{GL_FA, "ATTACK", 0}, {GL_FD, "DECAY", 0}, {GL_FS, "SUSTAIN", 0}, {GL_FR, "RELEASE", 0}}},
    {B_ENV, "AMP EG", "AMP EG", {{P_ATK, "ATTACK", 0}, {P_DEC, "DECAY", 0}, {P_SUS, "SUSTAIN", 0}, {P_REL, "RELEASE", 0}}},
    {B_LFO, "LFO 1", "LFO 1",
     {{GL_L1WAVE, "WAVE", 0}, {GL_L1FREQ, "FREQ", 0}, {GL_L1KEY, "KEY SYNC", 0}, {GL_L1SYNC, "TEMPO", 0}}},
    {B_LFO, "LFO 2", "LFO 2",
     {{GL_L2WAVE, "WAVE", 0}, {GL_L2FREQ, "FREQ", 0}, {GL_L2KEY, "KEY SYNC", 0}, {GL_L2SYNC, "TEMPO", 0}}},
    {B_GLO, "AMP", "AMP", {{P_LEVEL, "LEVEL", 0}, {P_PAN, "PANPOT", 0}, {GL_DIST, "DIST", 0}, {GV_NONE, 0, 0}}},
    {B_GLO, "VOICE", "VOICE",
     {{P_VOICE, "ASSIGN", N_GVOICE}, {P_DETUNE, "DETUNE", 0}, {P_GLIDE, "PORTA", 0}, {GL_EGRESET, "EG RESET", 0}}},
    {B_GLO, "GLOBAL", "GLOB",
     {{GV_BPM, "TEMPO", 0}, {GV_TUNE, "TUNE", 0}, {GV_CLOCK, "CLOCK", N_GCLOCK}, {GV_MIDI, "MIDI IN", N_GMIDI}}},
    {B_GLO, "ROUTE", "ROUTE", {{GL_ROUTE, "FX ORDER", 0}, {GV_NONE, 0, 0}, {GV_NONE, 0, 0}, {GV_NONE, 0, 0}}},
#define GU_PATCH(k, n, sh) {B_REC, n, sh, {{GL_P1SRC + 3 * (k), "SOURCE", 0}, {GL_P1DST + 3 * (k), "DEST", 0}, \
                                          {GL_P1INT + 3 * (k), "INTENSITY", 0}, {GV_NONE, 0, 0}}}
    GU_PATCH(0, "PATCH 1", "P1"), GU_PATCH(1, "PATCH 2", "P2"), GU_PATCH(2, "PATCH 3", "P3"),
    GU_PATCH(3, "PATCH 4", "P4"),
#undef GU_PATCH
    {B_SEQ, "ARP", "ARP", {{GL_ARPON, "ARP", 0}, {GL_ARPTYPE, "TYPE", 0}, {P_AOCT, "RANGE", 0}, {GV_RES, "RES", N_GRES}}},
    {B_SEQ, "TIMING", "TIMING",
     {{P_AGATE, "GATE", 0}, {P_ASWING, "SWING", 0}, {P_AHOLD, "LATCH", N_GONOFF}, {GL_ARPKSYNC, "KEY SYNC", 0}}},
    {B_SEQ, "STEPS", "STEPS",
     {{GL_ARPLAST, "LAST STEP", 0}, {GV_STEP, "STEP", 0}, {GV_STEPON, "ON / OFF", N_GONOFF}, {GV_NONE, 0, 0}}},
    {B_FX, "MOD FX", "MOD FX", {{GL_MFX, "TYPE", 0}, {P_IA, "SPEED", 0}, {P_IB, "DEPTH", 0}, {P_IMIX, "MIX", 0}}},
    {B_FX, "DELAY", "DELAY",
     {{GL_DTYPE, "TYPE", 0}, {GL_DTIME, "TIME", 0}, {GL_DFDBK, "FEEDBACK", 0}, {P_DLY, "LEVEL", 0}}},
    {B_FX, "REVERB", "REVERB",
     {{GL_RTYPE, "TYPE", 0}, {GL_RSIZE, "SIZE", 0}, {GL_RDAMP, "DAMP", 0}, {P_REV, "LEVEL", 0}}},
};
#define GU_NPAGE NELEM(GU_PAGES)
/* each page button's own colour: its tabs and its cards' meters, every time its pages are open */
static uint16_t gu_btn_col(uint32_t b)
{
    switch (b) {
    case B_HOME: return RGB(255, 181, 71);          /* amber */
    case B_EDIT: return RGB(255, 128, 92);          /* coral */
    case B_SCL: return RGB(236, 214, 96);           /* yellow */
    case B_ENV: return RGB(240, 122, 156);          /* pink */
    case B_LFO: return RGB(176, 146, 255);          /* violet */
    case B_GLO: return RGB(92, 206, 170);           /* green-teal */
    case B_REC: return RGB(150, 214, 104);          /* lime */
    case B_SEQ: return RGB(102, 170, 255);          /* blue */
    default: return RGB(112, 214, 238);             /* FX: cyan */
    }
}

static struct {
    uint8_t page, prog, dirty, ready;
    uint8_t save, save_slot, cur;                /* WRITE: open, its slot, the letter KNOB 2 changes */
    uint8_t step;                                /* ARP 3: the step KNOB 2 points at (0..7) */
    char name[13], sname[13];
    char toast[24];
    uint32_t toast_t, hot_t;
    int8_t hot;                                  /* the card last turned (lit for a moment), -1 none */
    uint32_t sig_head, sig_list, sig_card[4], sig_body;
    uint8_t shown_save, shown_uboot;
} gu;

static track_t *gu_t(void) { return &trk[0]; }

/* --------------------------------------------------------------- programs --- */
static void gu_prog_values(uint32_t k, int16_t *v, char *name)
{
    if (up_used(k) && up_rec(k)->engine == ENGI_GOLANA) {
        up_values(up_rec(k), v);
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
    rc = up_put(k, &r);
    return rc == 0 || rc == 3 ? 0 : rc;
}

/* ------------------------------------------------------------- parameters --- */
static const param_desc_t *gu_desc(uint32_t id) { return param_desc_of(ENGI_GOLANA, id); }

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
    default:
        if (q->names) {
            uint32_t n = 0;
            while (q->names[n])
                n++;
            *lo = 0;
            *hi = (int32_t)n - 1;
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
static uint32_t gu_hash(uint32_t h, const char *s)
{
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h * 16777619u;
}
static uint32_t gu_hashv(uint32_t h, int32_t v) { return (h ^ (uint32_t)v) * 16777619u; }

static int gu_fits_l(const char *s)               /* every character in AF_L (gen_aa_font.py L_CHARS) */
{
    for (; *s; s++)
        if (!((*s >= '0' && *s <= '9') || (*s >= 'A' && *s <= 'Y') || *s == '+' || *s == '-' || *s == '.' ||
              *s == '/' || *s == '#' || *s == ' '))
            return 0;
    return 1;
}

static int gu_toast_on(void) { return gu.toast[0] && fm1_ms - gu.toast_t < 1400u; }
static void gu_say(const char *a, const char *b)
{
    str_cpy(gu.toast, a, sizeof gu.toast);
    if (b)
        str_cpy(gu.toast + str_len(gu.toast), b, sizeof gu.toast - str_len(gu.toast));
    gu.toast_t = fm1_ms;
}

static void gu_chip(int32_t xr, int32_t y, const char *s, uint16_t bg, uint16_t fg, int32_t *xo)
{
    int32_t w = text_w(&AF_S, s) + 10;
    cv_rrect(xr - w, y, w, 17, -5, bg, GC_BG);
    cv_text_on(xr - w + 5, y + 1, &AF_S, s, fg, bg);
    *xo = xr - w - 4;
}

static void gu_draw_head(void)
{
    const track_t *t = gu_t();
    char b[32], n[4];
    uint32_t h = 0x811C9DC5u;
    int32_t x;
    int toast = gu_toast_on();
    n[0] = 'P';
    n[1] = (char)('0' + (gu.prog + 1u) / 10u);
    n[2] = (char)('0' + (gu.prog + 1u) % 10u);
    n[3] = 0;
    h = gu_hash(h, toast ? gu.toast : gu.name);
    h = gu_hashv(h, (int32_t)(gu.prog | gu.dirty << 8 | toast << 9 | (t->p[GL_ARPON] != 0) << 10 |
                              (t->p[P_AHOLD] != 0) << 11 | (uint32_t)(song.octave + 8) << 12));
    if (h == gu.sig_head)
        return;
    gu.sig_head = h;
    cv_begin(240, 32, GC_BG);
    x = 240 - 6;
    if (song.octave) {
        b[0] = 'O';
        b[1] = 'C';
        b[2] = 'T';
        b[3] = song.octave > 0 ? '+' : '-';
        b[4] = (char)('0' + (song.octave > 0 ? song.octave : -song.octave));
        b[5] = 0;
        gu_chip(x, 8, b, GC_RAISE, GC_TEXT, &x);
    }
    if (t->p[P_AHOLD])
        gu_chip(x, 8, "LATCH", GC_TEAL, GC_INK, &x);
    gu_chip(x, 8, "ARP", t->p[GL_ARPON] ? GC_AMBER : GC_RAISE, t->p[GL_ARPON] ? GC_INK : GC_DIM, &x);
    if (toast) {
        cv_text(8, 7, &AF_M, gu.toast, GC_AMBER);
    } else {
        int32_t xn = cv_text(8, 7, &AF_M, n, GC_AMBER) + 8;
        xn = cv_text(xn, 7, &AF_M, gu.name, GC_TEXT);
        if (gu.dirty)
            cv_rrect(xn + 4, 13, 6, 6, -3, GC_PINK, GC_BG);
    }
    cv_rect(0, 31, 240, 1, GC_LINE);
    cv_blit(0, 0);
}

/* the tabs, as on a card index: the pages of the button whose page is open. The open one in the button's colour,
 * "BUTTON · PAGE", joined to the line under the tabs; the button's other pages beside it, dark, by short names */
/* card k of page pg as it works now: MOD FX's METAL turns SPEED DEPTH MIX into GAIN MID MID FREQ */
static const gu_par_t *gu_pk(const gu_page_t *pg, uint32_t k)
{
    static const gu_par_t METAL[3] = {{P_IA, "GAIN", 0}, {P_IB, "MID", 0}, {P_IC, "MID FREQ", 0}};
    if (pg->k[0].id == GL_MFX && k && gu_t()->p[GL_MFX] == 5)
        return &METAL[k - 1u];
    return &pg->k[k];
}
#define GU_TAB_Y 32
#define GU_TAB_H 26
static void gu_draw_tabs(void)
{
    uint32_t btn = GU_PAGES[gu.page].btn, pi, h = 0x811C9DC5u;
    uint16_t col = gu_btn_col(btn);
    int32_t x = 4, ax = 4, aw = 0;
    char lab[32];
    h = gu_hashv(h, (int32_t)gu.page);
    if (h == gu.sig_list)
        return;
    gu.sig_list = h;
    cv_begin(240, GU_TAB_H, GC_BG);
    str_cpy(lab, btn < NB ? B_NAME[btn] : "", sizeof lab);
    str_cpy(lab + str_len(lab), " \xB7 ", sizeof lab - str_len(lab));
    str_cpy(lab + str_len(lab), GU_PAGES[gu.page].tab, sizeof lab - str_len(lab));
    for (pi = 0; pi < GU_NPAGE; pi++) {
        int32_t w;
        if (GU_PAGES[pi].btn != btn)
            continue;
        if (pi == gu.page) {
            ax = x;
            aw = text_w(&AF_S, lab) + 16;
            x += aw + 3;
            continue;
        }
        w = text_w(&AF_S, GU_PAGES[pi].tshort) + 14;
        cv_rrect(x, 6, w, GU_TAB_H, -6, GC_RAISE, GC_BG);
        cv_text_on(x + 7, 8, &AF_S, GU_PAGES[pi].tshort, GC_MID, GC_RAISE);
        x += w + 3;
    }
    cv_rect(0, GU_TAB_H - 2, 240, 2, col);
    cv_rrect(ax, 1, aw, GU_TAB_H + 6, -6, col, GC_BG);
    cv_text_on(ax + 8, 5, &AF_S, lab, GC_INK, col);
    cv_blit(0, GU_TAB_Y);
}

/* card k of the page: the label, the value (big when it fits), a meter (a bar, a centred bar, or a dot per name) */
#define GU_CW 114
#define GU_CH 86
static void gu_draw_card(uint32_t k)
{
    const gu_page_t *pg = &GU_PAGES[gu.page];
    const gu_par_t *q = gu_pk(pg, k);
    char b[16];
    int32_t v = gu_get(q), lo = 0, hi = 0, x = 4 + (int32_t)(k & 1u) * (GU_CW + 4);
    int32_t y = GU_TAB_Y + GU_TAB_H + 4 + (int32_t)(k >> 1) * (GU_CH + 4);
    uint32_t h = 0x811C9DC5u;
    int hot = gu.hot == (int8_t)k && fm1_ms - gu.hot_t < 700u;
    uint16_t acc = gu_btn_col(pg->btn), surf = hot ? GC_RAISE : GC_SURF;
    gu_text(q, b);
    h = gu_hash(h, b);
    h = gu_hash(h, q->label ? q->label : "");
    h = gu_hashv(h, v * 3 + hot);
    h = gu_hashv(h, (int32_t)(gu.page | k << 8));
    if (q->id == GV_STEP || q->id == GV_STEPON || q->id == GL_ARPLAST)
        h = gu_hashv(h, (int32_t)(gu_steps() | (uint32_t)gu_t()->p[GL_ARPLAST] << 8 | (uint32_t)gu.step << 12));
    if (h == gu.sig_card[k])
        return;
    gu.sig_card[k] = h;
    cv_begin(GU_CW, GU_CH, GC_BG);
    if (q->id == GV_NONE) {
        cv_rrect(0, 0, GU_CW, GU_CH, -8, ux_mix(GC_BG, GC_SURF, 50), GC_BG);
        cv_blit((uint32_t)x, (uint32_t)y);
        return;
    }
    cv_rrect(0, 0, GU_CW, GU_CH, -8, surf, GC_BG);
    if (hot)
        cv_rect(14, 0, GU_CW - 28, 2, acc);
    cv_text_on(10, 6, &AF_S, q->label, GC_MID, surf);
    gu_range(q, &lo, &hi);
    {
        int big = gu_fits_l(b) && text_w(&AF_L, b) <= GU_CW - 12;
        const aafont_t *f = big ? &AF_L : &AF_M;
        int32_t w = text_w(f, b);
        if (w > GU_CW - 8) {
            f = &AF_S;
            w = text_w(f, b);
        }
        cv_text_on((GU_CW - w) / 2, big ? 22 : 32, f, b, GC_TEXT, surf);
    }
    if (q->id == GV_STEP || q->id == GV_STEPON || q->id == GL_ARPLAST) {   /* the 8 steps: on, off, past LAST */
        uint32_t st, m = gu_steps(), last = (uint32_t)clamp(gu_t()->p[GL_ARPLAST], 1, 8);
        for (st = 0; st < 8u; st++) {
            int32_t sx = 13 + (int32_t)st * 11;
            uint16_t c = st >= last ? GC_LINE : (m >> st) & 1u ? acc : GC_DIM;
            cv_rect(sx, 64, 8, 9, c);
            if (st == gu.step && q->id != GL_ARPLAST)
                cv_rect(sx, 76, 8, 2, GC_TEXT);
        }
    } else if (q->names || (q->id < P_COUNT && gu_desc(q->id)->fmt == F_ENUM) || hi - lo < 2) {
        int32_t n = hi - lo + 1, i, dx = n > 1 ? (GU_CW - 24) / (n - 1) : 0;
        if (dx > 14)
            dx = 14;
        for (i = 0; i < n && n <= 12; i++) {
            int32_t cx = GU_CW / 2 - (dx * (n - 1)) / 2 + i * dx;
            cv_rrect(cx - 3, 68, 6, 6, -3, i == v - lo ? acc : GC_LINE, surf);
        }
    } else if (lo < 0) {                             /* bipolar: from the middle */
        int32_t mid = GU_CW / 2, len = (v * (GU_CW / 2 - 12)) / (hi > 0 ? hi : 1);
        cv_rect(12, 70, GU_CW - 24, 3, GC_LINE);
        cv_rect(mid - 1, 67, 2, 9, GC_MID);
        if (len > 0)
            cv_rect(mid, 70, len, 3, acc);
        else if (len < 0)
            cv_rect(mid + len, 70, -len, 3, acc);
    } else {
        int32_t len = ((v - lo) * (GU_CW - 24)) / (hi - lo > 0 ? hi - lo : 1);
        cv_rect(12, 70, GU_CW - 24, 3, GC_LINE);
        cv_rect(12, 70, len, 3, acc);
    }
    cv_blit((uint32_t)x, (uint32_t)y);
}

/* WRITE: the slot, its name with the letter being changed, what each control does */
static void gu_draw_save(void)
{
    uint32_t h = gu_hash(0x811C9DC5u, gu.sname), i;
    char n[8];
    h = gu_hashv(h, (int32_t)(gu.save_slot | gu.cur << 8 | gu_user(gu.save_slot) << 16));
    if (h == gu.sig_body && gu.shown_save)
        return;
    gu.sig_body = h;
    gu.shown_save = 1;
    gu.sig_list = gu.sig_card[0] = gu.sig_card[1] = gu.sig_card[2] = gu.sig_card[3] = 0;
    cv_begin(240, 104, GC_BG);
    cv_text(12, 6, &AF_M, "WRITE PROGRAM", GC_AMBER);
    n[0] = 'P';
    n[1] = (char)('0' + (gu.save_slot + 1u) / 10u);
    n[2] = (char)('0' + (gu.save_slot + 1u) % 10u);
    n[3] = 0;
    cv_text(12, 34, &AF_L, n, GC_TEXT);
    cv_text(90, 46, &AF_S, gu_user(gu.save_slot) ? "OVERWRITES ITS SOUND" :
                           gu.save_slot < GL_NFACT ? "OVER A FACTORY SOUND" : "EMPTY SLOT", GC_MID);
    {   /* the name, one box per letter, the one KNOB 2 changes lit */
        int32_t x = 12;
        for (i = 0; i < 12u; i++) {
            char c[2] = {gu.sname[i] ? gu.sname[i] : ' ', 0};
            uint16_t bg = i == gu.cur ? GC_AMBER : GC_SURF;
            cv_rrect(x, 74, 17, 24, -4, bg, GC_BG);
            if (c[0] != ' ')
                cv_text_on(x + (17 - text_w(&AF_M, c)) / 2, 77, &AF_M, c, i == gu.cur ? GC_INK : GC_TEXT, bg);
            x += 18;
        }
    }
    cv_blit(0, 32);
    cv_begin(240, 104, GC_BG);
    cv_text(12, 8, &AF_S, "PRESETS   the slot", GC_MID);
    cv_text(12, 28, &AF_S, "KNOB 1   the letter", GC_MID);
    cv_text(12, 48, &AF_S, "KNOB 2   change it", GC_MID);
    cv_text(12, 68, &AF_S, "SAVE   write", GC_TEXT);
    cv_text(130, 68, &AF_S, "HOME   cancel", GC_TEXT);
    cv_blit(0, 136);
}

static void gu_draw_uboot(void)
{
    char d[2] = {(char)('0' + ui.uboot % 10u), 0};
    if (gu.shown_uboot == ui.uboot)
        return;
    gu.shown_uboot = ui.uboot;
    cv_begin(240, 104, GC_BG);
    cv_text((240 - text_w(&AF_M, "UPDATE MODE IN")) / 2, 20, &AF_M, "UPDATE MODE IN", GC_TEXT);
    cv_text((240 - text_w(&AF_L, d)) / 2, 48, &AF_L, d, GC_AMBER);
    cv_blit(0, 32);
    cv_begin(240, 104, GC_BG);
    cv_text((240 - text_w(&AF_S, "LET GO OF OCT- OCT+ TO CANCEL")) / 2, 10, &AF_S, "LET GO OF OCT- OCT+ TO CANCEL",
            GC_MID);
    cv_blit(0, 136);
}

static void gu_redraw_all(void)
{
    uint32_t k;
    gu.sig_head = gu.sig_list = gu.sig_body = 0;
    for (k = 0; k < 4u; k++)
        gu.sig_card[k] = 0;
    gu.shown_save = 0;
    gu.shown_uboot = 0xFF;
    lcd_fill(0, 0, 240, 240, GC_BG);
}

static void gu_draw(void)
{
    uint32_t k;
    if (!gu.ready)
        return;
    if (ux.mono || ux.light || ux.style != ST_FLAT) {   /* Golana's own colours: full colour, dark, rounded (a stored */
        ux.mono = 0;                                     /* Felucca palette, GREY or PAPER, would grey or invert them) */
        ux.light = 0;
        ux.style = ST_FLAT;
        ux.gen++;
        ui.force = 1;
    }
    if (ui.force) {                                  /* (main.c: after the splash, an update, a cancelled UBOOT) */
        ui.force = 0;
        gu_redraw_all();
    }
    if (ui.uboot) {
        gu_draw_uboot();
        gu.sig_list = gu.sig_card[0] = gu.sig_card[1] = gu.sig_card[2] = gu.sig_card[3] = 0;
        gu.shown_save = 0;
        return;
    }
    if (gu.shown_uboot != 0xFF) {
        gu.shown_uboot = 0xFF;
        gu_redraw_all();
    }
    gu_draw_head();
    if (gu.save) {
        gu_draw_save();
        return;
    }
    if (gu.shown_save) {
        gu.shown_save = 0;
        lcd_fill(0, 32, 240, 208, GC_BG);
    }
    gu_draw_tabs();
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
    led_put(nl, panel.btn[B_PLAY], gu_t()->p[P_AHOLD] != 0);
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
    const gu_par_t *q = gu_pk(&GU_PAGES[gu.page], k);
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
    uint32_t pressed = fm1_input_edges(0), id, b, k;
    int32_t s;
    (void)fm1_input_note_edges();                    /* (the keys play in the audio ISR: seq.c) */
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
        if (gu.save) {
            if (b == B_SAVE) {
                int rc;
                if (!gu.sname[0])
                    str_cpy(gu.sname, "PROGRAM", 13);
                rc = gu_store(gu.save_slot, gu.sname);
                if (!rc) {
                    gu.prog = gu.save_slot;
                    str_cpy(gu.name, gu.sname, 13);
                    gu.dirty = 0;
                    gu_say("WRITTEN", 0);
                } else if (rc != 2) {
                    gu_say("NOT WRITTEN", 0);
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
        case B_PLAY: {
            static const gu_par_t L = {P_AHOLD, "LATCH", 0};
            gu_set(&L, !gu_get(&L));
            gu_say("LATCH ", gu_get(&L) ? "ON" : "OFF");
            break;
        }
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
