/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Gadlele */
/* Golana 1.1's arpeggiator types ORDER, CONVERGE, PEDAL and DOUBLE (seq.c arp_pick, golana_prog.c GL_AM): G, C, E
 * pressed in that order and held, the notes the arp plays step after step, RANGE 1 and 2; the six of 1.0 as they were.
 *   cc -DGOLANA=1 -O1 -w -Ibuild/gen -Ifirmware/src -o build/host/golana_arp_test tests/golana_arp_test.c -lm */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int bad;
static void setup(uint32_t type, uint32_t range)
{
    int16_t v[P_COUNT];
    host_tracks_init();
    gl_factory(0xFF, v, 0);                         /* INIT */
    v[GL_ARPON] = 1;
    v[GL_ARPTYPE] = (int16_t)type;
    v[P_AOCT] = (int16_t)range;
    gl_apply(&trk[0], v);
    trk[0].engine = trk[0].eng_req;
    eng_mem_claim(&trk[0]);
    song.sel = 0;
    trk[0].arp_idx = 0xFFFFFFFFu;                   /* (as seq.c on a first key: the order from its start) */
    trk[0].arp_st = 0;
}
static void expect(uint32_t type, uint32_t range, const uint8_t *want, uint32_t n)
{
    static const uint8_t PRESS[3] = {67, 60, 64};   /* G, C, E: as pressed */
    uint32_t i;
    int ok = 1;
    char got[256] = "";
    setup(type, range);
    for (i = 0; i < 3u; i++)
        input_on(&trk[0], PRESS[i], 100);
    for (i = 0; i < n; i++) {
        arp_fire(&trk[0], 1000);
        snprintf(got + strlen(got), sizeof got - strlen(got), " %u", trk[0].arp_note);
        ok &= trk[0].arp_note == want[i];
    }
    printf("golana arp: %-8s RANGE %u:%s %s\n", N_GARPT[type], range, got, ok ? "ok" : "FAIL");
    bad += !ok;
}

int main(void)
{
    static const uint8_t UP1[] = {60, 64, 67, 60, 64, 67};
    static const uint8_t ORD1[] = {67, 60, 64, 67, 60, 64}, ORD2[] = {67, 60, 64, 79, 72, 76, 67};
    static const uint8_t CONV1[] = {60, 67, 64, 60, 67, 64};
    static const uint8_t PED1[] = {60, 64, 60, 67, 60, 64, 60, 67};
    static const uint8_t PED2[] = {60, 64, 60, 67, 60, 72, 60, 76, 60, 79, 60, 64};
    static const uint8_t DBL1[] = {60, 60, 72, 72, 64, 64, 76, 76, 67, 67, 79, 79, 60, 60};
    static const uint8_t DBL2[] = {60, 60, 72, 72, 64, 64, 76, 76, 67, 67, 79, 79, 72, 72, 84, 84, 76, 76, 88, 88, 79, 79, 91, 91, 60};
    expect(0, 1, UP1, NELEM(UP1));                  /* (UP, as in 1.0) */
    expect(6, 1, ORD1, NELEM(ORD1));
    expect(6, 2, ORD2, NELEM(ORD2));
    expect(7, 1, CONV1, NELEM(CONV1));
    expect(8, 1, PED1, NELEM(PED1));
    expect(8, 2, PED2, NELEM(PED2));
    expect(9, 1, DBL1, NELEM(DBL1));
    expect(9, 2, DBL2, NELEM(DBL2));
    {   /* the types of 1.0 keep their values (a stored program plays as before) */
        static const uint8_t AM[6] = {1, 2, 3, 15, 4, 14};
        uint32_t k;
        int ok = NELEM(GL_AM) == NELEM(N_GARPT);
        for (k = 0; k < 6u; k++)
            ok &= GL_AM[k] == AM[k];
        printf("golana arp: the six of 1.0 unchanged, a name for every type %s\n", ok ? "ok" : "FAIL");
        bad += !ok;
    }
    printf(bad ? "golana arp: %d FAILED\n" : "golana arp tests passed\n", bad);
    return bad != 0;
}
