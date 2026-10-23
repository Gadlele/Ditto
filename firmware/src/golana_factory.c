/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele */
/* Ditto's 24 factory sounds (golana_prog.c gl_factory): each one's values that differ from INIT. 1..16 the
 * library's sounds as a mid-eighties sampler played them (strings, the choir, the orchestra hit, ..), 17..24 what
 * the sampler does with two of them: layers, a velocity crossfade, a split, START, the characters, DRIFT.
 * Time values: 40 ~ 20 ms, 60 ~ 80 ms, 80 ~ 0.3 s, 90 ~ 0.7 s, 100 ~ 1.4 s, 110 ~ 3 s, 120 ~ 6 s (gen_tables.py).
 * LFO FREQ: 20 ~ 0.15 Hz, 40 ~ 0.4 Hz, 60 ~ 1.2 Hz, 85 ~ 4.4 Hz, 90 ~ 5.6 Hz.
 * SOUND A: DT_S_* (ditto_samples.h); SOUND B: B(DT_S_*) (0 is OFF).
 * PATCH k (0..3): SRC FEG AEG LFO1 LFO2 VELO KBD, DST PITCH BPITCH BAL NOISE CUTOFF AMP LFO2F, INT +-63 */
#define S(i, v) {(uint8_t)(i), (int16_t)(v)}
#define PROG(n, a) {n, a, (uint8_t)NELEM(a)}
#define PSRC(k) (GL_P1SRC + 3 * (k))
#define PDST(k) (GL_P1DST + 3 * (k))
#define PINT(k) (GL_P1INT + 3 * (k))
#define B(s) ((s) + 1)
enum { L_SAW, L_SQR, L_TRI, L_SIN, L_SH };
enum { F_LPF24, F_LPF12, F_BPF, F_HPF, F_MS20 };
enum { Q_FEG, Q_AEG, Q_LFO1, Q_LFO2, Q_VELO, Q_KBD };
enum { D_PITCH, D_BPITCH, D_BAL, D_NOISE, D_CUT, D_AMP, D_LFO2F };
enum { Y_STACK, Y_VELX, Y_SPLIT };
enum { C_EII, C_EMU1, C_CLEAN };

/* a gentle vibrato: LFO 1 (TRI, ~4.4 Hz, from 0 on every note) on the pitch, d of 63 (1 ~ 3 ct) */
#define VIBRATO(k, d) S(GL_L1WAVE, L_TRI), S(GL_L1FREQ, 85), S(PSRC(k), Q_LFO1), S(PDST(k), D_PITCH), S(PINT(k), d)

/* ------------------------------------------------------------- the library --- */
static const gl_set_t F_STRINGS[] = {             /* the violins, a slow bow, a hall */
    S(DT_SNDA, DT_S_STRINGS), S(GL_CUTOFF, 92), S(GL_RESO, 8), S(GL_FEGINT, 14), S(GL_FA, 85), S(GL_FD, 95),
    S(GL_FS, 90), S(GL_FR, 95), S(P_ATK, 78), S(P_SUS, 127), S(P_REL, 90), S(P_LEVEL, 100),
    S(PSRC(1), Q_VELO), S(PDST(1), D_CUT), S(PINT(1), 16),
    S(GL_RTYPE, 2), S(GL_RSIZE, 100), S(P_REV, 50),
};
static const gl_set_t F_CHOIR[] = {               /* the eighties' voices: "ah", a long hall, a slow swell */
    S(DT_SNDA, DT_S_CHOIR), S(GL_CUTOFF, 96), S(GL_RESO, 6), S(P_ATK, 82), S(P_SUS, 127), S(P_REL, 96),
    S(P_LEVEL, 104), S(GL_RTYPE, 2), S(GL_RSIZE, 110), S(GL_RDAMP, 50), S(P_REV, 62),
    S(GL_MFX, 3), S(P_IA, 20), S(P_IB, 50), S(P_IMIX, 60),
};
static const gl_set_t F_ORCHHIT[] = {             /* the stab: an orchestra struck at once */
    S(DT_SNDA, DT_S_ORCH_HIT), S(P_ATK, 0), S(P_SUS, 127), S(P_REL, 80), S(P_LEVEL, 112),
    S(PSRC(1), Q_VELO), S(PDST(1), D_CUT), S(PINT(1), 20), S(GL_CUTOFF, 100),
    S(GL_RTYPE, 2), S(GL_RSIZE, 90), S(P_REV, 40),
};
static const gl_set_t F_PIANO[] = {               /* the grand, as eight bits keep it */
    S(DT_SNDA, DT_S_PIANO), S(GL_CUTOFF, 108), S(GL_FEGINT, 12), S(GL_FD, 92), S(GL_FS, 40), S(GL_KTRK, 40),
    S(PSRC(1), Q_VELO), S(PDST(1), D_CUT), S(PINT(1), 28),
    S(P_SUS, 127), S(P_REL, 72), S(P_LEVEL, 108), S(GL_RTYPE, 0), S(GL_RSIZE, 70), S(P_REV, 22),
};
static const gl_set_t F_BRASS[] = {               /* trumpets, the filter opening on each note */
    S(DT_SNDA, DT_S_BRASS), S(GL_CUTOFF, 58), S(GL_RESO, 12), S(GL_FEGINT, 40), S(GL_FA, 62), S(GL_FD, 88),
    S(GL_FS, 70), S(GL_FR, 70), S(PSRC(1), Q_VELO), S(PDST(1), D_CUT), S(PINT(1), 24),
    S(P_ATK, 30), S(P_SUS, 127), S(P_REL, 62), S(P_LEVEL, 104), S(P_REV, 28),
};
static const gl_set_t F_FLUTE[] = {               /* breathy, a vibrato */
    S(DT_SNDA, DT_S_FLUTE), S(GL_NOISE, 10), S(GL_CUTOFF, 96), VIBRATO(0, 3), S(P_ATK, 45), S(P_REL, 66),
    S(GL_L1KEY, 2), S(P_LEVEL, 100), S(GL_DTYPE, 0), S(GL_DTIME, 1), S(GL_DFDBK, 40), S(P_DLY, 25), S(P_REV, 35),
};
static const gl_set_t F_CHOIRSTR[] = {            /* the choir and the strings together: the classic pad */
    S(DT_SNDA, DT_S_CHOIR), S(DT_SNDB, B(DT_S_STRINGS)), S(DT_LVLA, 120), S(DT_LVLB, 96),
    S(GL_CUTOFF, 94), S(P_ATK, 84), S(P_REL, 96), S(P_LEVEL, 98), S(GL_RTYPE, 2), S(GL_RSIZE, 108), S(P_REV, 58),
};
static const gl_set_t F_HARPSI[] = {              /* plucked, bright, a small room */
    S(DT_SNDA, DT_S_HARPSI), S(GL_CUTOFF, 116), S(P_SUS, 127), S(P_REL, 66), S(P_LEVEL, 104),
    S(GL_RTYPE, 0), S(GL_RSIZE, 60), S(P_REV, 24),
};
static const gl_set_t F_MARIMBA[] = {             /* wood, soft mallets, echoes */
    S(DT_SNDA, DT_S_MARIMBA), S(GL_CUTOFF, 104), S(PSRC(1), Q_VELO), S(PDST(1), D_CUT), S(PINT(1), 30),
    S(P_SUS, 127), S(P_REL, 70), S(P_LEVEL, 110), S(GL_DTYPE, 1), S(GL_DTIME, 10), S(GL_DFDBK, 38), S(P_DLY, 30),
    S(P_REV, 26),
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
    S(DT_SNDA, DT_S_TIMPANI), S(GL_CUTOFF, 100), S(P_SUS, 127), S(P_REL, 92), S(P_LEVEL, 116),
    S(GL_RTYPE, 2), S(GL_RSIZE, 104), S(P_REV, 40),
};
static const gl_set_t F_ORGAN[] = {               /* pipes, quiet stops, a cathedral */
    S(DT_SNDA, DT_S_ORGAN), S(GL_CUTOFF, 100), S(P_ATK, 40), S(P_REL, 82), S(P_LEVEL, 100),
    S(GL_RTYPE, 2), S(GL_RSIZE, 120), S(GL_RDAMP, 40), S(P_REV, 55),
};
static const gl_set_t F_CELLO[] = {               /* the cellos, low and slow */
    S(DT_SNDA, DT_S_CELLO), S(GL_CUTOFF, 90), S(GL_FEGINT, 10), S(GL_FA, 82), S(P_ATK, 70), S(P_REL, 88),
    S(P_LEVEL, 106), S(GL_RTYPE, 2), S(GL_RSIZE, 96), S(P_REV, 40),
};
static const gl_set_t F_SHAKU[] = {               /* a bamboo flute after the famous one: breath, a bent attack */
    S(DT_SNDA, DT_S_RECORDER), S(GL_NOISE, 30), S(GL_CUTOFF, 86), S(GL_RESO, 18), S(GL_FTYPE, F_LPF12),
    S(PSRC(1), Q_FEG), S(PDST(1), D_PITCH), S(PINT(1), -6), S(GL_FA, 0), S(GL_FD, 70), S(GL_FS, 0),
    VIBRATO(0, 4), S(GL_L1KEY, 2), S(P_ATK, 50), S(P_REL, 70), S(P_LEVEL, 106),
    S(GL_DTYPE, 0), S(GL_DTIME, 0), S(GL_DFDBK, 50), S(P_DLY, 34), S(GL_RTYPE, 2), S(P_REV, 48),
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
    S(GL_CUTOFF, 80), S(P_ATK, 86), S(P_REL, 100), S(P_LEVEL, 100), S(GL_DRIFT, 30),
    S(GL_RTYPE, 2), S(GL_RSIZE, 120), S(P_REV, 66),
};
static const gl_set_t F_HITARP[] = {              /* the orchestra hit in 16ths, up and down two octaves */
    S(DT_SNDA, DT_S_ORCH_HIT), S(GL_CUTOFF, 104), S(P_REL, 60), S(P_LEVEL, 108),
    S(GL_ARPON, 1), S(GL_ARPTYPE, 2), S(P_AOCT, 2), S(P_AGATE, 70),
    S(GL_DTYPE, 1), S(GL_DTIME, 4), S(GL_DFDBK, 40), S(P_DLY, 30), S(P_REV, 30),
};
static const gl_set_t F_EMUKEYS[] = {             /* EMU 1: the piano rougher, a phaser, a tape-like wobble */
    S(DT_SNDA, DT_S_PIANO), S(DT_CHAR, C_EMU1), S(GL_CUTOFF, 96), S(GL_DRIFT, 50),
    S(PSRC(1), Q_VELO), S(PDST(1), D_CUT), S(PINT(1), 24), S(P_REL, 70), S(P_LEVEL, 108),
    S(GL_MFX, 4), S(P_IA, 30), S(P_IB, 60), S(P_IMIX, 80), S(P_REV, 26),
};
static const gl_set_t F_AIRCHOIR[] = {            /* START in the middle: the choir without its attack, filtered */
    S(DT_SNDA, DT_S_CHOIR), S(DT_START, 60), S(GL_CUTOFF, 60), S(GL_RESO, 40), S(GL_FTYPE, F_BPF),
    S(PSRC(3), Q_LFO2), S(PDST(3), D_CUT), S(PINT(3), 26), S(GL_L2WAVE, L_TRI), S(GL_L2FREQ, 30),
    S(P_ATK, 92), S(P_REL, 104), S(P_LEVEL, 112), S(GL_RTYPE, 2), S(GL_RSIZE, 124), S(P_REV, 76),
};
static const gl_set_t F_TAPEFLUTE[] = {           /* the flute as a tape replay keyboard would play it */
    S(DT_SNDA, DT_S_FLUTE), S(DT_SNDB, B(DT_S_STRINGS)), S(DT_LVLB, 50), S(DT_LOOP, 0), S(GL_DRIFT, 80),
    S(GL_CUTOFF, 78), S(P_ATK, 30), S(P_REL, 72), S(P_LEVEL, 104), S(GL_MFX, 1), S(P_IA, 18), S(P_IB, 40),
    S(P_IMIX, 70), S(P_REV, 34),
};
static const gl_set_t F_HARPBELL[] = {            /* the harp and the bells a twelfth above, glass */
    S(DT_SNDA, DT_S_HARP), S(DT_SNDB, B(DT_S_BELLS)), S(DT_BSEMI, 19), S(DT_LVLB, 56), S(DT_CHAR, C_CLEAN),
    S(GL_CUTOFF, 112), S(P_REL, 92), S(P_LEVEL, 104), S(GL_DTYPE, 1), S(GL_DTIME, 5), S(GL_DFDBK, 50), S(P_DLY, 36),
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
    PROG("EMU 1 KEYS", F_EMUKEYS),
    PROG("AIR CHOIR", F_AIRCHOIR),
    PROG("TAPE FLUTE", F_TAPEFLUTE),
    PROG("HARP + BELL", F_HARPBELL),
};
_Static_assert(NELEM(GL_FACTORY) == GL_NFACT, "24 factory sounds");
#undef S
#undef PROG
#undef PSRC
#undef PDST
#undef PINT
#undef B
#undef VIBRATO
