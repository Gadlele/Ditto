# Ditto manual

Ditto is a fork of [Golana](https://github.com/Gadlele/Golana) (itself a fork of
[Felucca](https://github.com/hugelton/Felucca)) for the M-VAVE FM-1 by Gadlele: a sampler of the mid-eighties after the
E-mu Emulator II, with Golana's interface (the MATRIX). Ditto is free software under the GPL-3.0, like Felucca.
Felucca's own README, for the parts Ditto shares (the hardware, the credits), is [FELUCCA.md](FELUCCA.md).

## The sampler

Every note plays one or two sounds of the library (SOUND A, SOUND B), each through the sampler's CHARACTER, mixed with
noise into the filter, then the amp. Like the Emulator II, a note starts its sample from the beginning every time.

| Section | Parameters |
| --- | --- |
| SOUND A | SOUND (the library's sixteen, or USR1-3: a user slot, your own samples loaded from the web editor), START (0 the sample's beginning .. 127 near its end: the notes start further in, past the attack; the library's sounds only), LOOP (ON: a sustained sound loops while the key is held; OFF: it plays once through and fades) |
| SOUND B | SOUND (OFF, or a sound), LAYER (STACK: both sounds on every note; VEL X: soft notes play A, hard notes B, crossfaded by the velocity; SPLIT: B below the SPLIT key, A from it), SEMI (±24, B's transposition: B picks the sample of the key it sounds), TUNE (±63 cents) |
| MIXER | SOUND A, SOUND B (their levels), NOISE, CHARACTER: **EII** (each sample through an 8-bit μ-law compander, 255 steps finer for quiet sounds, and held until the next one, no interpolation: the grain and the aliasing of the Emulator II's variable-clock DACs), **EMU 1** (8-bit linear and held: rougher, the quiet tails gritty, as the Emulator I), **CLEAN** (the recordings as stored, interpolated) |
| SPLIT | SPLIT KEY (LAYER SPLIT: the lowest key of SOUND A), DRIFT (an old machine's clocks: each note's two sounds a little out of tune, each its own way, slowly wandering, its cutoff a little off; 0 off) |
| FILTER | TYPE (LPF24: the Emulator II's SSM 24 dB low-pass; LPF12, BPF, HPF, MS20: a driven 12 dB low-pass whose resonance screams), CUTOFF, RESO, EG INT, KEY TRACK |
| FILTER EG / AMP EG | ATTACK, DECAY, SUSTAIN, RELEASE. A one-shot sound (the piano, the bells, the hit…) ends when its sample does, the key held or not |
| AMP | LEVEL, PANPOT, DIST, SPREAD (the voices across the stereo field) |
| VOICE | ASSIGN (POLY with eight voices, five when each note plays two sounds: SOUND B on, LAYER STACK or VEL X; SPLIT keeps eight. MONO, LEGATO, UNISON: every voice on one note, SOUND B detuned the other way), DETUNE, PORTA, EG RESET |
| LFO 1 / LFO 2 | WAVE (SAW, SQR, TRI, SIN, S&H), FREQ, KEY SYNC (OFF, TIMBRE, VOICE), TEMPO (sync to a note value) |
| PATCH 1-4 | SOURCE (FEG, AEG, LFO1, LFO2, VELO, KBD), DEST (PITCH, B PITCH: SOUND B's alone, BAL: the balance between the two sounds, + toward B and − toward A, NOISE, CUTOFF, AMP, LFO2F), INTENSITY. VELO → BAL is a velocity crossfade with the two levels kept; an LFO → BAL a slow one |
| SEQUENCER | 16 steps saved with the program. PATTERN: STEP, NOTE, TYPE (REST, NOTE, TIE), ACC / SLIDE; a key writes its note into the step and moves on. SETUP: LENGTH 1-16, RES, GATE, SWING |
| ARP | ON, TYPE (UP, DOWN, ALT1, ALT2, RANDOM, TRIGGER, ORDER, CONVERGE, PEDAL, DOUBLE), RANGE 1-4, RES, GATE, SWING, LATCH, KEY SYNC, LAST STEP and steps on/off |
| MOD FX | TYPE (CHORUS, FLANGER, ENSEMBLE, PHASER), SPEED, DEPTH, MIX; TYPE METAL (a high-gain distortion with a strong mid hump): GAIN, MID, MID FREQ (200 Hz - 5 kHz) |
| DELAY | TYPE (STEREO, CROSS, L/R), TIME (tempo), FEEDBACK, LEVEL |
| REVERB | TYPE (ROOM, SPRING, HALL), SIZE, DAMP, LEVEL |
| GLOBAL | TEMPO, TUNE, CLOCK (INT, USB, TRS), MIDI IN (ALL, CH 1) |
| SETUP | FX ORDER, saved with the program: MFX>DLY>REV (the default), DLY>MFX>REV, DLY>REV>MFX; PALETTE; HOME's drawing |

## The library

| Sound | Zones | |
| --- | --- | --- |
| PIANO | 4, one-shot | a Kawai grand (VCSL) |
| STRINGS | 3, looped | a violin section, bowed with vibrato (VSCO-2 CE) |
| CHOIR | 3, looped | eight voices singing "ah", men below A3, women above: synthesized (no free choir recording exists) |
| BRASS | 3, looped | a trumpet, sustained (VSCO-2 CE) |
| FLUTE | 3, looped | a flute with vibrato (VSCO-2 CE) |
| CELLO | 2, looped | a cello section (VSCO-2 CE) |
| ORGAN | 3, looped | a pipe organ's quiet stops (VSCO-2 CE) |
| HARPSI | 3, one-shot | an English harpsichord (VCSL) |
| MARIMBA | 3, one-shot | (VSCO-2 CE) |
| BELLS | 2, one-shot | tubular bells (VCSL) |
| PIZZ | 3, one-shot | the violin section plucked (VSCO-2 CE) |
| BASS | 3, one-shot | a double bass plucked (VSCO-2 CE) |
| TIMPANI | 1, one-shot | a timpani stroke (VSCO-2 CE) |
| ORCH HIT | 1, one-shot | trumpets in octaves, the violins' spiccato on the root and the fifth, a timpani: struck together |
| RECORDER | 2, looped | a baroque bass recorder (VCSL): breathy, the SHAKUHACHI program's |
| HARP | 2, one-shot | a concert harp (VSCO-2 CE) |

Each zone covers the keys halfway to its neighbours; a note far from its zone's root is pitched up or down by
playing the sample faster or slower, as a sampler does (up to three octaves up).

USR1-3 are the SAMPLE engine's user slots of Felucca: samples sent from the web editor (`web/editor.html`, or
`tools/fm1_sample_upload.py`) into the FM-1's flash, up to 80 KB each, kept across updates. A slot with nothing in it
is silent, and the SOUND page says EMPTY SLOT. START works on the library's sounds only.

## The programs

32 programs: 1-24 hold the factory sounds until written over, 25-32 start as INIT. MIDI Program Change 0-31 picks a
program.

1-16, the library as the sampler played it: STRINGS, CHOIR, ORCH HIT, PIANO, BRASS, FLUTE, CHOIR STRING (the choir and
the strings stacked), HARPSICHORD, MARIMBA, TUBULAR BELL, PIZZICATO, UPRIGHT BASS (mono), TIMPANI, PIPE ORGAN, CELLOS,
SHAKUHACHI (the recorder with breath noise and a bent attack).

17-24, the sampler's tricks: BOW + PLUCK (VEL X: soft notes bowed, hard notes plucked), BASS / PIANO (SPLIT at
middle C), DEEP CHOIR (the choir and itself an octave down, DRIFT), HIT ARP (the orchestra hit on the arpeggiator),
EMU 1 KEYS (the piano in EMU 1, a phaser), AIR CHOIR (START past the attack, a band-pass moving), TAPE FLUTE (LOOP
OFF and DRIFT: a tape replay keyboard), HARP + BELL (CLEAN, the bells a twelfth above).

## The controls

| Control | What it does |
| --- | --- |
| SELECT | every page, one after another |
| KNOB 1-4 | the four parameters on the cards |
| ALGORITHM | CUTOFF, from any page |
| PRESETS | the program (it loads as you turn; edits not written are lost, as on the original) |
| HOME | PERFORM: cutoff, resonance, EG int, LFO 1 rate |
| EDIT / SCL / ENV / LFO | SOUND A, SOUND B, MIXER, SPLIT (split key, DRIFT) / FILTER, KEY TRACK / FILTER EG, AMP EG / LFO 1, LFO 2, PATCH 1-4 (press again for the next) |
| GLO / REC / SEQ / FX | AMP, VOICE, GLOBAL, SETUP (FX ORDER, PALETTE, HOME) / the sequencer: PATTERN, SETUP / the arpeggiator: ARP, TIMING, STEPS / MOD FX, DELAY, REVERB |
| ARP | the arpeggiator on / off |
| PLAY | the step sequencer: play / stop (LATCH is on SEQ · TIMING) |
| SAVE | WRITE: PRESETS picks the slot, KNOB 1 the letter, KNOB 2 changes it, SAVE writes, HOME cancels |
| OCT- / OCT+ | the keys' octave (both held 5 s: update mode, as in Felucca) |

The pages that set a shape draw it above their four knobs, from the values themselves: SOUND A and SOUND B the
sample the middle C plays, whole (its root and length above it, the loop shaded, START marked), FILTER its response
curve, FILTER EG and AMP EG the envelope, LFO 1 and LFO 2 the running wave. HOME draws the sound playing, three ways
(GLO · SETUP · HOME): RIDGE, the default (the last waves stacked), RING (the wave round a circle) and ORBIT (the wave
against itself a moment later). GLO · SETUP · PALETTE: NIGHT, CRT, PAPER, NEON (kept with the settings).

The screen shows the open page as a tab, "BUTTON · PAGE" in that button's own colour, with the button's other pages
as dark tabs beside it. The header shows the program, its name and the battery (a bolt on USB power); a thin strip at
the bottom shows the arpeggiator (a light, ARP, LATCH) on the left and the tempo (BPM, EXT on an external clock) on
the right.

## When something goes wrong

- **USB rescue** (after [dinghy](https://github.com/zvenson/dinghy)'s): hold **OCT-** alone while switching on. The FM-1
  starts with no sound and no interface, only the updater: install a working build from the web installer or the
  FM-1 Firmware Hub. If Ditto crashes twice in a row while starting, the FM-1 goes there by itself; if the rescue
  itself fails, the next start goes to the chip's own update mode (UBOOT).
- A crash shows DITTO CRASH with where it happened, then the FM-1 restarts.

## Building and testing

The firmware builds exactly as Felucca does (`./build.sh`, see BUILDING.md); the package is still called
`felucca.fwsc`. The build makes the sound library from `assets/samples-eii/` (`tools/gen_ditto_samples.py`, cached).
Host tests of Ditto:

```
cc -DGOLANA=1 -O2 -w -Ibuild/gen -Ifirmware/src -o build/host/golana_sound tests/golana_sound.c -lm
build/host/golana_sound build/ditto_wav          # every factory program to a WAV, with levels
cc -DGOLANA=1 -O2 -w -Ibuild/gen -Ifirmware/src -o build/host/golana_sim tests/golana_sim.c -lm
build/host/golana_sim build/golana_sim           # the whole firmware on a simulated FM-1, screenshots
web/emu/build_wasm.sh                            # the browser version: build/emu/ditto.html
```
