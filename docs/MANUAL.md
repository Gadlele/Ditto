# Ditto manual

Ditto is a fork of [Golana](https://github.com/Gadlele/Golana) (itself a fork of
[Felucca](https://github.com/hugelton/Felucca)) for the M-VAVE FM-1 by Gadlele: a sampler of the mid-eighties after the
E-mu Emulator II, with a panel after the instrument's. Ditto is free software under the GPL-3.0, like Felucca.
Felucca's own README, for the parts Ditto shares (the hardware, the credits), is [FELUCCA.md](FELUCCA.md).

## The sampler

Every note plays one or two sounds of the library (SOUND A, SOUND B) through the sampler's CHARACTER into a 24 dB
low-pass and a VCA, each with its ADSR; one LFO with a delay goes to the pitch, the filter and the level. As on the
Emulator II, a note starts its sample from the beginning every time and the routings are fixed: there is no patch bay,
and the only effects are a delay and a reverb.

## The panel

The screen is the Emulator II's panel: a dark strip (DITTO, the preset in red, the battery), a two-line LCD of 16
characters in 5 × 7 dots, four sliders (KNOB 1–4), and the modules' LEDs. The LCD's first line names the module and
its function's number (`FILTER 1`), the second the function (`FREQ Q ENV`); a slider moved shows its value there for a
moment (`FREQ 64`), and messages appear there too. On HOME the first line is the preset (`P01 STRINGS`), the second
the tempo and the arpeggiator.

| Button | Module | Function | Sliders (KNOB 1 2 3 4) |
| --- | --- | --- | --- |
| HOME | PERFORM | | FREQ, Q, ENV, LFO RATE |
| GLO | MASTER | 1 TEMPO TUNE | TEMPO, TUNE, CLOCK (INT, USB, TRS), MIDI (ALL, CH 1) |
| | | 2 ASSIGN | MODE (POLY, MONO, LEGATO, UNISON), DETUNE, PORTA, RESET (both envelopes from 0 on a retrigger) |
| | | 3 OUTPUT | LEVEL, PAN, SPREAD |
| SCL | FILTER | 1 FREQ Q ENV | FREQ, Q, ENV (the filter ADSR's amount, ±) |
| | | 2 LFO KBD | LFO (the LFO on the cutoff), KBD (key tracking, ±; +63 follows the keys) |
| | | 3 ADSR | ATTACK, DECAY, SUSTAIN, RELEASE |
| ENV | LFO/VCA | 1 VCA ADSR | ATTACK, DECAY, SUSTAIN, RELEASE (a one-shot sound ends with its sample, the key held or not) |
| (LFO) | | 2 LFO | WAVE (TRI, SIN, SQR, SAW, S&H), RATE, DELAY (it fades in after each note starts), VCA (a tremolo) |
| | | 3 LFO SYNC | KEY (OFF, TIMBRE: from the first key down, VOICE: each note), TEMPO (a note value of the tempo) |
| EDIT | VOICE DEF | 11 SAMPLE | SOUND (the library or USR1-3), START (0 the sample's beginning .. 127 near its end), LOOP, CHAR (EII: 8-bit μ-law, each sample held, no interpolation; CLEAN: the recordings as stored, interpolated) |
| | | 21 VELOCITY | LEVEL (0 every note as loud, 127 as hard as played), FILTER (±) |
| | | 23 VIBRATO | DEPTH (up to ±2 semitones), RATE, DELAY (the LFO's) |
| SEQ | PRESET DEF | 22 SOUND B | SOUND (OFF or a sound; B picks the sample of the key it sounds), SEMI (±24), TUNE (±63 cents), LEVEL |
| | | 25 LAYER | MODE (STACK both; VEL SW: B from the VEL SW velocity on; VEL X: crossfaded by the velocity; SPLIT: B below KEY; POS X: B into A over WIDTH keys centred on KEY), KEY, WIDTH, VEL SW |
| | | 29 ARP | ARP, TYPE (UP, DOWN, ALT1, ALT2, RANDOM, TRIGGER, ORDER, CONVERGE, PEDAL, DOUBLE), RANGE 1-4, RES |
| | | 30 ARP TIME | GATE, SWING, LATCH, KSYNC |
| | | 31 ARP STEPS | LAST, STEP, ON/OFF (the 8 steps on the LCD) |
| REC | SEQUENCER | 1 PATTERN | STEP, NOTE, TYPE (REST, NOTE, TIE), ACC/SLD; a key writes its note into the step and moves on. The 16 steps on the LCD: `#` a note, `-` a tie, `.` a rest, `>` the one playing |
| | | 2 SETUP | LENGTH 1-16, RES, GATE, SWING |
| FX | EFFECTS | 1 DELAY | TYPE (STEREO, CROSS, L/R), TIME (tempo), FDBK, LEVEL |
| | | 2 REVERB | TYPE (ROOM, SPRING, HALL), SIZE, DAMP, LEVEL |
| SAVE | DISK | | WRITE PRESET: PRESETS the slot, KNOB 1 the letter (blinking on the LCD), KNOB 2 changes it, SAVE writes, HOME cancels |

A button opens its module's first function, pressed again the next; SELECT walks every function; LFO opens LFO/VCA 2
directly. ALGORITHM is the filter's FREQ from any function; PRESETS loads a preset as it turns (edits not written are
lost); ARP turns the arpeggiator on and off; PLAY starts and stops the sequencer; OCT- OCT+ move the keys an octave.

Voices: eight, five when each note plays two sounds (SOUND B on, LAYER STACK, VEL X or POS X; SPLIT and VEL SW keep
eight: each note plays one).

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

## The presets

32 presets: 1-24 hold the factory sounds until written over, 25-32 start as INIT. MIDI Program Change 0-31 picks one.

1-16, the library as the sampler played it: STRINGS, CHOIR, ORCH HIT, PIANO, BRASS, FLUTE (a delayed vibrato), CHOIR
STRING (the choir and the strings stacked), HARPSICHORD, MARIMBA, TUBULAR BELL, PIZZICATO, UPRIGHT BASS (mono),
TIMPANI, PIPE ORGAN, CELLOS, SHAKUHACHI (the recorder, a slow vibrato).

17-24, PRESET DEF at work: BOW + PLUCK (VEL X: soft notes bowed, hard notes plucked), BASS / PIANO (SPLIT at middle C),
DEEP CHOIR (the choir and itself an octave down), HIT ARP (the orchestra hit on the arpeggiator), BRASS / HIT (VEL SW:
played hard, the orchestra hit), AIR CHOIR (START past the attack, the LFO on the filter), CELLO>CHOIR (POS X: the
cellos low, the choir high, crossfaded over two octaves), HARP + BELL (CLEAN, the bells a twelfth above).

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
