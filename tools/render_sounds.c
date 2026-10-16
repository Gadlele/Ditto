/* SPDX-License-Identifier: GPL-3.0-only
 * The website's Sounds section (web/site/make_pages.py): eight short pieces, each one factory program of Golana played
 * as the FM-1 plays it (one synth, four voices, its own arpeggiator, delay and reverb; tests/hostsim.c builds the
 * firmware's engine for the computer, as tests/golana_sound.c): OUT/N.wav and OUT/sounds.json (each piece's file,
 * title, text and program). Mostly chord progressions; one line with the cutoff moving, as ALGORITHM would.
 *   cc -O2 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/render_sounds tools/render_sounds.c -lm
 *   build/host/render_sounds OUT          (tools/render_sounds.sh: as MP3) */
#define main hostsim_main
#include "hostsim.c"
#undef main

static FILE *w;
static uint32_t frames;
static int16_t cut_from = -1, cut_to;               /* a cutoff sweep over the piece (ALGORITHM), -1 = none */
static uint64_t nsamp;                              /* the samples rendered */
static uint32_t total;                              /* the score's length, ms */
static uint32_t cur_ms(void) { return (uint32_t)(nsamp * 1000u / FS); }
static void run_ms(uint32_t ms)
{
    int32_t o[2 * CTL];
    uint32_t n = ms * (FS / 1000u) / CTL, i;
    while (n--) {
        if (cut_from >= 0 && total) {               /* up and back down: a triangle over the piece */
            uint32_t ph = cur_ms() * 2000u / total;    /* 0..2000 */
            int32_t k = ph < 1000u ? (int32_t)ph : 2000 - (int32_t)ph;
            trk[0].p[GL_CUTOFF] = (int16_t)(cut_from + (cut_to - cut_from) * (k < 0 ? 0 : k) / 1000);
        }
        mix_block(o, CTL);
        nsamp += CTL;
        if (!w)
            continue;
        for (i = 0; i < CTL; i++)
            wav_put(w, o[2u * i], o[2u * i + 1u]);
        frames += CTL;
    }
}
static void midi_note(uint32_t on, uint32_t note, uint32_t vel)
{
    uint32_t st = on ? 0x90u : 0x80u;
    midi_enqueue((st >> 4) | st << 8 | (note & 127u) << 16 | (vel & 127u) << 24, 1);
}

/* ---- a score: notes in ms (a 16th of the piece's tempo: s16) ---- */
typedef struct { uint32_t t; uint8_t note, vel, on; } ev_t;
static ev_t ev[2048];
static uint32_t nev, s16;                           /* s16: a 16th, in 0.01 ms */
static uint32_t at(uint32_t bar, uint32_t six) { return (bar * 16u + six) * s16 / 100u; }
static void nt_ms(uint32_t t, uint32_t note, uint32_t dur, uint32_t vel)
{
    if (nev + 2u > NELEM(ev) || dur < 2u)
        return;
    ev[nev++] = (ev_t){t * 2u + 1u, (uint8_t)note, (uint8_t)vel, 1};
    ev[nev++] = (ev_t){(t + dur) * 2u, (uint8_t)note, 0, 0};
}
static void nt(uint32_t bar, uint32_t six, uint32_t note, uint32_t len, uint32_t vel)   /* len in 16ths, a little short */
{
    nt_ms(at(bar, six), note, len * s16 / 100u - 10u, vel);
}
static void tie(uint32_t bar, uint32_t six, uint32_t note, uint32_t len, uint32_t vel)  /* into the next note: a slide */
{
    nt_ms(at(bar, six), note, len * s16 / 100u + 20u, vel);
}
static void chord(uint32_t bar, uint32_t six, const uint8_t *c, uint32_t n, uint32_t len, uint32_t vel)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        nt(bar, six, c[i], len, vel);
}
static void line(uint32_t bar, const uint8_t (*m)[2], uint32_t n, uint32_t vel)   /* {note, 16ths}, 0 = rest */
{
    uint32_t i, six = 0;
    for (i = 0; i < n; i++) {
        if (m[i][0])
            nt(bar + six / 16u, six % 16u, m[i][0], m[i][1], vel);
        six += m[i][1];
    }
}

static const char *prog_name;
static void prog(const char *name, uint32_t bpm)
{
    int16_t v[P_COUNT];
    char nm[16];
    uint32_t k, pi;
    for (pi = 0; pi < NELEM(GL_FACTORY); pi++) {
        gl_factory(pi, v, nm);
        if (!strcmp(nm, name))
            break;
    }
    if (pi == NELEM(GL_FACTORY)) {
        fprintf(stderr, "render_sounds: no program %s\n", name);
        exit(1);
    }
    host_tracks_init();
    for (k = 0; k < NTRK; k++) {                    /* (as tests/golana_sound.c load_prog) */
        gl_factory(0xFF, v, 0);
        gl_apply(&trk[k], v);
        trk[k].engine = trk[k].eng_req;
        eng_mem_claim(&trk[k]);
    }
    gl_factory(pi, v, nm);
    gl_apply(&trk[0], v);
    trk[0].engine = trk[0].eng_req;
    song.sel = 0;
    song.g[G_ROUTE] = 1;                            /* SEL: every channel plays the synth */
    song.g[G_BPM] = (int16_t)bpm;
    s16 = 1500000u / bpm;
    prog_name = GL_FACTORY[pi].name;
}

/* ---- the eight pieces ---- */
static void p_sync(void)                            /* SYNC PAD: Dmaj7 Bm9 Gmaj7 Aadd9 */
{
    static const uint8_t C[4][4] = {{50, 57, 61, 64}, {47, 54, 57, 61}, {43, 50, 54, 59}, {45, 52, 57, 59}};
    uint32_t b;
    prog("SYNC PAD", 84);
    for (b = 0; b < 8u; b++)
        chord(b, 0, C[b % 4u], 4, 16, 96);
}
static void p_arp(void)                             /* BRIGHT ARP: its arpeggiator over Cm Ab Eb Bb */
{
    static const uint8_t C[4][3] = {{48, 51, 55}, {44, 48, 51}, {51, 55, 58}, {46, 50, 53}};
    uint32_t b;
    prog("BRIGHT ARP", 118);
    for (b = 0; b < 8u; b++)
        chord(b, 0, C[b % 4u], 3, 16, 100);
}
static void p_brass(void)                           /* 80S BRASS: stabs on Eb Bb Cm Ab */
{
    static const uint8_t C[4][4] = {{51, 55, 58, 63}, {50, 53, 58, 62}, {48, 51, 55, 60}, {48, 51, 56, 60}};
    static const uint8_t HIT[5][2] = {{0, 3}, {3, 3}, {6, 2}, {10, 2}, {12, 4}};
    uint32_t b, i;
    prog("80S BRASS", 112);
    for (b = 0; b < 8u; b++)
        for (i = 0; i < (b % 2u ? 5u : 3u); i++)
            chord(b, HIT[i][0], C[b % 4u], 4, HIT[i][1], i ? 92 : 110);
}
static void p_organ(void)                           /* ORGAN: a gospel turn, C F/C C G7 */
{
    static const uint8_t C[4][4] = {{48, 52, 55, 60}, {48, 53, 57, 60}, {48, 52, 55, 64}, {47, 50, 53, 55}};
    uint32_t b, i;
    prog("ORGAN", 92);
    for (b = 0; b < 8u; b++)
        for (i = 0; i < 4u; i++)
            chord(b, i * 4u, C[b % 4u], 4, i == 3u ? 2u : 3u, 96);
}
static void p_phase(void)                           /* PHASE PAD: Am9 Fmaj7 Cmaj7 Em7, slow */
{
    static const uint8_t C[4][4] = {{45, 52, 55, 59}, {41, 52, 55, 57}, {48, 52, 55, 59}, {40, 50, 55, 59}};
    uint32_t b;
    prog("PHASE PAD", 72);
    for (b = 0; b < 8u; b++)
        chord(b, 0, C[b % 4u], 4, 16, 92);
}
static void p_slowarp(void)                         /* SLOW ARP: Am7 Dm7 Gsus4 C, two bars each */
{
    static const uint8_t C[4][4] = {{45, 52, 55, 60}, {50, 53, 57, 60}, {43, 50, 55, 60}, {48, 52, 55, 60}};
    uint32_t b;
    prog("SLOW ARP", 90);
    for (b = 0; b < 8u; b += 2u)
        chord(b, 0, C[b / 2u], 4, 32, 96);
}
static void p_bells(void)                           /* SOFT BELLS: Gmaj7 Em7 Cmaj7 D, a tune on top */
{
    static const uint8_t C[4][3] = {{55, 59, 66}, {52, 55, 62}, {48, 55, 59}, {50, 54, 57}};
    static const uint8_t TUNE[][2] = {{74, 4}, {71, 4}, {74, 4}, {78, 4}, {79, 8}, {76, 8}, {76, 4}, {74, 4}, {71, 8},
                                      {69, 8}, {71, 4}, {74, 4}, {78, 4}, {79, 4}, {78, 8}, {74, 8}, {71, 8}, {69, 4}, {71, 4},
                                      {74, 8}, {66, 8}, {67, 8}};
    uint32_t b;
    prog("SOFT BELLS", 80);
    for (b = 0; b < 8u; b++)
        chord(b, 0, C[b % 4u], 3, 16, 80);
    line(0, TUNE, NELEM(TUNE), 100);
}
static void p_acid(void)                            /* ACID LINE: a sixteenth line, the cutoff opening and closing */
{
    static const int8_t PAT[16] = {0, 0, 12, 0, 3, 0, 12, 10, 0, 0, 7, 0, 12, 3, 5, 7};
    static const uint8_t SLIDE[16] = {0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1}, ROOT[2] = {45, 41};
    uint32_t b, i;
    prog("ACID LINE", 124);
    cut_from = 18; cut_to = 92;
    for (b = 0; b < 8u; b++)
        for (i = 0; i < 16u; i++) {
            uint32_t n = (uint32_t)(ROOT[(b / 2u) % 2u] + PAT[i]);
            if (SLIDE[i]) tie(b, i, n, 1, 100);
            else nt(b, i, n, 1, i % 4u ? 88 : 110);
        }
}

typedef struct { const char *file, *title, *text; void (*score)(void); } piece_t;
static const piece_t PIECES[] = {
    {"1_sync", "Sync Pad", "Oscillator sync swept by an envelope, four voices wide: Dmaj7, Bm9, Gmaj7, Aadd9.", p_sync},
    {"2_arp", "Bright Arp", "The arpeggiator on held chords (Cm, Ab, Eb, Bb), in time with the tempo, the delay in time too.", p_arp},
    {"3_brass", "80s Brass", "Stabs of the brass patch on Eb, Bb, Cm, Ab: filter envelope on every hit.", p_brass},
    {"4_organ", "Organ", "A gospel turn on the organ patch: C, F/C, C, G7.", p_organ},
    {"5_phase", "Phase Pad", "The phaser on a slow pad: Am9, Fmaj7, Cmaj7, Em7.", p_phase},
    {"6_slowarp", "Slow Arp", "A slow arpeggio with long echoes: Am7, Dm7, Gsus4, C, two bars each.", p_slowarp},
    {"7_bells", "Soft Bells", "Bell chords with a tune on top, four voices at once: Gmaj7, Em7, Cmaj7, D.", p_bells},
    {"8_acid", "Acid Sweep", "The squelchy line, with the cutoff opening and closing as the ALGORITHM knob does.", p_acid},
};

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    char p[512];
    uint32_t n, i, a, b;
    FILE *js;
    snprintf(p, sizeof p, "%s/sounds.json", dir);
    js = fopen(p, "w");
    if (!js)
        return 1;
    fprintf(js, "[");
    for (n = 0; n < NELEM(PIECES); n++) {
        const piece_t *pc = &PIECES[n];
        nev = 0;
        cut_from = -1;
        pc->score();
        for (a = 1; a < nev; a++)                   /* in time (stable: an off before an on at the same time) */
            for (b = a; b > 0 && ev[b - 1u].t > ev[b].t; b--) {
                ev_t x = ev[b]; ev[b] = ev[b - 1u]; ev[b - 1u] = x;
            }
        total = nev ? ev[nev - 1u].t / 2u : 1u;
        nsamp = 0;
        w = 0;
        {
            int16_t keep = cut_from;
            cut_from = -1;
            run_ms(80);
            cut_from = keep;
        }
        nsamp = 0;
        snprintf(p, sizeof p, "%s/%s.wav", dir, pc->file);
        w = fopen(p, "wb");
        frames = 0;
        wav_hdr(w, 0);
        for (i = 0; i < nev; i++) {
            uint32_t t = ev[i].t / 2u;
            if (t > cur_ms())
                run_ms(t - cur_ms());
            midi_note(ev[i].on, ev[i].note, ev[i].vel);
        }
        cut_from = -1;
        run_ms(4000);                                /* the tails */
        fseek(w, 0, SEEK_SET);
        wav_hdr(w, frames);
        fclose(w);
        w = 0;
        fprintf(js, "%s\n {\"file\": \"%s.mp3\", \"title\": \"%s\", \"text\": \"%s\", \"new\": false, \"seconds\": %u, \"parts\": [\"GOLANA %s\"]}",
                n ? "," : "", pc->file, pc->title, pc->text, frames / FS, prog_name);
    }
    fprintf(js, "\n]\n");
    fclose(js);
    return 0;
}
