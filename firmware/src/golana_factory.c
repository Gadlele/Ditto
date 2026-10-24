/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele */
/* Ditto's 24 factory sounds (golana_prog.c gl_factory): each one's values that differ from INIT. 1..16 the
 * library's sounds as a mid-eighties sampler played them (strings, the choir, the orchestra hit, ..), 17..24 what
 * the sampler does with two of them: a stack, VEL SW, VEL X, SPLIT, POS X, START, CLEAN.
 * Time values: 40 ~ 20 ms, 60 ~ 80 ms, 80 ~ 0.3 s, 90 ~ 0.7 s, 100 ~ 1.4 s, 110 ~ 3 s, 120 ~ 6 s (gen_tables.py).
 * LFO FREQ: 20 ~ 0.15 Hz, 40 ~ 0.4 Hz, 60 ~ 1.2 Hz, 85 ~ 4.4 Hz, 90 ~ 5.6 Hz.
 * SOUND A: DT_S_* (ditto_samples.h); SOUND B: B(DT_S_*) (0 is OFF). */
#define S(i, v) {(uint8_t)(i), (int16_t)(v)}
#define PROG(n, a) {n, a, (uint8_t)NELEM(a)}
#define B(s) ((s) + 1)
enum { L_TRI, L_SIN, L_SQR, L_SAW, L_SH };
enum { Y_STACK, Y_VELSW, Y_VELX, Y_SPLIT, Y_POSX };
enum { C_EII, C_CLEAN };

/* a vibrato that comes in: the LFO (TRI, ~4.4 Hz, from 0 on every note) on the pitch after DELAY (d of 127 ~ 2 st) */
#define VIBRATO(d, dly) S(GL_L1WAVE, L_TRI), S(GL_L1FREQ, 85), S(DT_VIB, d), S(DT_LFODLY, dly)

/* ------------------------------------------------------------- the library --- */
static const gl_set_t F_STRINGS[] = {             /* the violins, a slow bow, a hall */
    S(DT_SNDA, DT_S_STRINGS), S(GL_CUTOFF, 92), S(GL_RESO, 8), S(GL_FEGINT, 14), S(GL_FA, 85), S(GL_FD, 95),
    S(GL_FS, 90), S(GL_FR, 95), S(P_ATK, 78), S(P_SUS, 127), S(P_REL, 90), S(P_LEVEL, 100), S(DT_VELCUT, 16),
    S(GL_RTYPE, 2), S(GL_RSIZE, 100), S(P_REV, 50),
};
static const gl_set_t F_CHOIR[] = {               /* the eighties' voices: "ah", a long hall, a slow swell */
    S(DT_SNDA, DT_S_CHOIR), S(GL_CUTOFF, 96), S(GL_RESO, 6), S(P_ATK, 82), S(P_SUS, 127), S(P_REL, 96),
    S(P_LEVEL, 104), S(DT_VELLVL, 60), S(GL_RTYPE, 2), S(GL_RSIZE, 110), S(GL_RDAMP, 50), S(P_REV, 62),
};
static const gl_set_t F_ORCHHIT[] = {             /* the stab: an orchestra struck at once */
    S(DT_SNDA, DT_S_ORCH_HIT), S(GL_CUTOFF, 100), S(DT_VELCUT, 20), S(P_ATK, 0), S(P_SUS, 127), S(P_REL, 80),
    S(P_LEVEL, 112), S(DT_VELLVL, 127), S(GL_RTYPE, 2), S(GL_RSIZE, 90), S(P_REV, 40),
};
static const gl_set_t F_PIANO[] = {               /* the grand, as eight bits keep it */
    S(DT_SNDA, DT_S_PIANO), S(GL_CUTOFF, 108), S(GL_FEGINT, 12), S(GL_FD, 92), S(GL_FS, 40), S(GL_KTRK, 40),
    S(DT_VELCUT, 28), S(DT_VELLVL, 120), S(P_SUS, 127), S(P_REL, 72), S(P_LEVEL, 108), S(GL_RTYPE, 0),
    S(GL_RSIZE, 70), S(P_REV, 22),
};
static const gl_set_t F_BRASS[] = {               /* trumpets, the filter opening on each note */
    S(DT_SNDA, DT_S_BRASS), S(GL_CUTOFF, 58), S(GL_RESO, 12), S(GL_FEGINT, 40), S(GL_FA, 62), S(GL_FD, 88),
    S(GL_FS, 70), S(GL_FR, 70), S(DT_VELCUT, 24), S(P_ATK, 30), S(P_SUS, 127), S(P_REL, 62), S(P_LEVEL, 104),
    S(P_REV, 28),
};
static const gl_set_t F_FLUTE[] = {               /* a vibrato that comes in */
    S(DT_SNDA, DT_S_FLUTE), S(GL_CUTOFF, 96), VIBRATO(6, 90), S(P_ATK, 45), S(P_REL, 66), S(P_LEVEL, 100),
    S(GL_DTYPE, 0), S(GL_DTIME, 1), S(GL_DFDBK, 40), S(P_DLY, 50), S(P_REV, 35),
};
static const gl_set_t F_CHOIRSTR[] = {            /* the choir and the strings together: the classic pad */
    S(DT_SNDA, DT_S_CHOIR), S(DT_SNDB, B(DT_S_STRINGS)), S(DT_LVLA, 120), S(DT_LVLB, 96),
    S(GL_CUTOFF, 94), S(P_ATK, 84), S(P_REL, 96), S(P_LEVEL, 98), S(GL_RTYPE, 2), S(GL_RSIZE, 108), S(P_REV, 58),
};
static const gl_set_t F_HARPSI[] = {              /* plucked, bright, a small room */
    S(DT_SNDA, DT_S_HARPSI), S(GL_CUTOFF, 116), S(DT_VELLVL, 40), S(P_SUS, 127), S(P_REL, 66), S(P_LEVEL, 104),
    S(GL_RTYPE, 0), S(GL_RSIZE, 60), S(P_REV, 24),
};
static const gl_set_t F_MARIMBA[] = {             /* wood, soft mallets, echoes */
    S(DT_SNDA, DT_S_MARIMBA), S(GL_CUTOFF, 104), S(DT_VELCUT, 30), S(P_SUS, 127), S(P_REL, 70), S(P_LEVEL, 110),
    S(GL_DTYPE, 1), S(GL_DTIME, 10), S(GL_DFDBK, 38), S(P_DLY, 60), S(P_REV, 26),
};
static const gl_set_t F_BELLS[] = {               /* tubular bells in a church */
    S(DT_SNDA, DT_S_BELLS), S(GL_CUTOFF, 118), S(P_SUS, 127), S(P_REL, 100), S(P_LEVEL, 100),
    S(GL_RTYPE, 2), S(GL_RSIZE, 118), S(P_REV, 60),
};
static const gl_set_t F_PIZZ[] = {                /* the violins plucked, short */
    S(DT_SNDA, DT_S_PIZZ), S(GL_CUTOFF, 110), S(P_SUS, 127), S(P_REL, 56), S(P_LEVEL, 112),
    S(GL_RTYPE, 2), S(GL_RSIZE, 90), S(P_REV, 36),
};
static const gl_set_t F_BASS[] = {                /* the double bass plucked, one note at a time */
    S(P_VOICE, V_MONO), S(DT_SNDA, DT_S_BASS), S(GL_CUTOFF, 84), S(GL_KTRK, 20), S(GL_FEGINT, 16), S(GL_FD, 84),
    S(GL_FS, 30), S(P_SUS, 127), S(P_REL, 60), S(P_LEVEL, 118), S(P_REV, 8),
};
static const gl_set_t F_TIMPANI[] = {             /* the kettle drums: the low keys a roll of thunder */
    S(DT_SNDA, DT_S_TIMPANI), S(GL_CUTOFF, 100), S(DT_VELCUT, 24), S(P_SUS, 127), S(P_REL, 92), S(P_LEVEL, 116),
    S(GL_RTYPE, 2), S(GL_RSIZE, 104), S(P_REV, 40),
};
static const gl_set_t F_ORGAN[] = {               /* pipes, quiet stops, a cathedral */
    S(DT_SNDA, DT_S_ORGAN), S(GL_CUTOFF, 100), S(DT_VELLVL, 0), S(P_ATK, 40), S(P_REL, 82), S(P_LEVEL, 100),
    S(GL_RTYPE, 2), S(GL_RSIZE, 120), S(GL_RDAMP, 40), S(P_REV, 55),
};
static const gl_set_t F_CELLO[] = {               /* the cellos, low and slow */
    S(DT_SNDA, DT_S_CELLO), S(GL_CUTOFF, 90), S(GL_FEGINT, 10), S(GL_FA, 82), S(P_ATK, 70), S(P_REL, 88),
    S(P_LEVEL, 106), S(GL_RTYPE, 2), S(GL_RSIZE, 96), S(P_REV, 40),
};
static const gl_set_t F_SHAKU[] = {               /* a bamboo flute after the famous one: breathy, a slow vibrato */
    S(DT_SNDA, DT_S_RECORDER), S(GL_CUTOFF, 86), S(GL_RESO, 18), VIBRATO(8, 100), S(GL_L1FREQ, 80), S(P_ATK, 50),
    S(P_REL, 70), S(P_LEVEL, 106), S(GL_DTYPE, 0), S(GL_DTIME, 0), S(GL_DFDBK, 50), S(P_DLY, 68), S(GL_RTYPE, 2),
    S(P_REV, 48),
};
/* ------------------------------------------------------------ the sampler --- */
static const gl_set_t F_BOWPLUCK[] = {            /* VEL X: soft notes bowed, hard notes plucked */
    S(DT_SNDA, DT_S_STRINGS), S(DT_SNDB, B(DT_S_PIZZ)), S(DT_LAYER, Y_VELX), S(DT_LVLB, 127),
    S(GL_CUTOFF, 100), S(P_ATK, 50), S(P_REL, 80), S(P_LEVEL, 108), S(GL_RTYPE, 2), S(P_REV, 42),
};
static const gl_set_t F_BASSPNO[] = {             /* SPLIT: the double bass below middle C, the piano from it */
    S(DT_SNDA, DT_S_PIANO), S(DT_SNDB, B(DT_S_BASS)), S(DT_LAYER, Y_SPLIT), S(DT_SPLIT, 60), S(DT_BSEMI, -12),
    S(DT_LVLB, 127), S(GL_CUTOFF, 106), S(P_REL, 70), S(P_LEVEL, 108), S(P_REV, 20),
};
static const gl_set_t F_DEEPCHOIR[] = {           /* the choir and itself an octave down: monks */
    S(DT_SNDA, DT_S_CHOIR), S(DT_SNDB, B(DT_S_CHOIR)), S(DT_BSEMI, -12), S(DT_BTUNE, 6), S(DT_LVLB, 110),
    S(GL_CUTOFF, 80), S(P_ATK, 86), S(P_REL, 100), S(P_LEVEL, 100), VIBRATO(3, 110), S(GL_L1FREQ, 70),
    S(GL_RTYPE, 2), S(GL_RSIZE, 120), S(P_REV, 66),
};
static const gl_set_t F_HITARP[] = {              /* the orchestra hit in 16ths, up and down two octaves */
    S(DT_SNDA, DT_S_ORCH_HIT), S(GL_CUTOFF, 104), S(P_REL, 60), S(P_LEVEL, 108),
    S(GL_ARPON, 1), S(GL_ARPTYPE, 2), S(P_AOCT, 2), S(P_AGATE, 70),
    S(GL_DTYPE, 1), S(GL_DTIME, 4), S(GL_DFDBK, 40), S(P_DLY, 60), S(P_REV, 30),
};
static const gl_set_t F_BRASSHIT[] = {            /* VEL SW: the brass, and played hard the orchestra hit */
    S(DT_SNDA, DT_S_BRASS), S(DT_SNDB, B(DT_S_ORCH_HIT)), S(DT_LAYER, Y_VELSW), S(DT_VELSW, 110), S(DT_LVLB, 120),
    S(GL_CUTOFF, 70), S(GL_FEGINT, 30), S(GL_FA, 60), S(GL_FD, 88), S(GL_FS, 70), S(P_ATK, 20), S(P_REL, 62),
    S(P_LEVEL, 106), S(P_REV, 32),
};
static const gl_set_t F_AIRCHOIR[] = {            /* START in the middle: the choir without its attack, a slow filter */
    S(DT_SNDA, DT_S_CHOIR), S(DT_START, 60), S(GL_CUTOFF, 60), S(GL_RESO, 40), S(GL_L1WAVE, L_TRI),
    S(GL_L1FREQ, 30), S(GL_L1KEY, 0), S(DT_LFOCUT, 40), S(P_ATK, 92), S(P_REL, 104), S(P_LEVEL, 112),
    S(GL_RTYPE, 2), S(GL_RSIZE, 124), S(P_REV, 76),
};
static const gl_set_t F_CELLOCHOIR[] = {          /* POS X: the cellos low, the choir high, crossfaded over two octaves */
    S(DT_SNDA, DT_S_CHOIR), S(DT_SNDB, B(DT_S_CELLO)), S(DT_LAYER, Y_POSX), S(DT_SPLIT, 60), S(DT_XWIDTH, 24),
    S(DT_LVLB, 120), S(GL_CUTOFF, 92), S(P_ATK, 76), S(P_REL, 94), S(P_LEVEL, 104), S(GL_RTYPE, 2),
    S(GL_RSIZE, 112), S(P_REV, 56),
};
static const gl_set_t F_HARPBELL[] = {            /* the harp and the bells a twelfth above, glass */
    S(DT_SNDA, DT_S_HARP), S(DT_SNDB, B(DT_S_BELLS)), S(DT_BSEMI, 19), S(DT_LVLB, 56), S(DT_CHAR, C_CLEAN),
    S(GL_CUTOFF, 112), S(P_REL, 92), S(P_LEVEL, 104), S(GL_DTYPE, 1), S(GL_DTIME, 5), S(GL_DFDBK, 50), S(P_DLY, 72),
    S(GL_RTYPE, 2), S(GL_RSIZE, 112), S(P_REV, 52),
};

static const gl_prog_t GL_FACTORY[] = {
    PROG("STRINGS", F_STRINGS),
    PROG("CHOIR", F_CHOIR),
    PROG("ORCH HIT", F_ORCHHIT),
    PROG("PIANO", F_PIANO),
    PROG("BRASS", F_BRASS),
    PROG("FLUTE", F_FLUTE),
    PROG("CHOIR STRING", F_CHOIRSTR),
    PROG("HARPSICHORD", F_HARPSI),
    PROG("MARIMBA", F_MARIMBA),
    PROG("TUBULAR BELL", F_BELLS),
    PROG("PIZZICATO", F_PIZZ),
    PROG("UPRIGHT BASS", F_BASS),
    PROG("TIMPANI", F_TIMPANI),
    PROG("PIPE ORGAN", F_ORGAN),
    PROG("CELLOS", F_CELLO),
    PROG("SHAKUHACHI", F_SHAKU),
    PROG("BOW + PLUCK", F_BOWPLUCK),
    PROG("BASS / PIANO", F_BASSPNO),
    PROG("DEEP CHOIR", F_DEEPCHOIR),
    PROG("HIT ARP", F_HITARP),
    PROG("BRASS / HIT", F_BRASSHIT),
    PROG("AIR CHOIR", F_AIRCHOIR),
    PROG("CELLO>CHOIR", F_CELLOCHOIR),
    PROG("HARP + BELL", F_HARPBELL),
};
_Static_assert(NELEM(GL_FACTORY) == GL_NFACT, "24 factory sounds");
#undef S
#undef PROG
#undef B
#undef VIBRATO
