<p align="center">
  <img src="https://raw.githubusercontent.com/Gadlele/Ditto/gh-pages/img/splash.gif" width="240" alt="Ditto's power-on animation">
</p>

<h1 align="center">Ditto</h1>

<p align="center">
  <b>An eighties sampler for the M-VAVE FM-1</b><br>
  after the E-mu Emulator II: two sounds layered, eight bits, an analog filter
</p>

<p align="center">
  <a href="https://gadlele.github.io/Ditto/install/"><b>Install</b></a> ·
  <a href="https://gadlele.github.io/Ditto/try/"><b>Try it in your browser</b></a> ·
  <a href="https://gadlele.github.io/Ditto/">Website</a> ·
  <a href="https://gadlele.github.io/Ditto/firmware/Ditto-1.0.fwsc">Download 1.0</a>
</p>

<p align="center">
  <img alt="version 1.0" src="https://img.shields.io/badge/version-1.0-ffb547?style=flat-square&labelColor=0e1118">
  <img alt="FM-1" src="https://img.shields.io/badge/for-M--VAVE%20FM--1-f07a9c?style=flat-square&labelColor=0e1118">
  <img alt="GPL-3.0" src="https://img.shields.io/badge/license-GPL--3.0-b092ff?style=flat-square&labelColor=0e1118">
</p>

**Ditto** turns the M-VAVE FM-1 into a sampler of the mid-eighties, after the E-mu Emulator II: a library of
sixteen sounds (strings, a choir, an orchestra hit, piano, brass, flute…), two of them layered on every note, played
through the grain of an 8-bit companded sampler into a 24 dB analog-style filter, with two envelopes, two LFOs, a
patch bay, an arpeggiator, a step sequencer and effects. It is a fork of [Golana](https://github.com/Gadlele/Golana),
itself a fork of [Felucca](https://github.com/hugelton/Felucca).

## The sampler

| Section | |
| --- | --- |
| **SOUND A** | a sound of the library, or one of three user slots (USR1-3, your own samples from the web editor) · START, where the notes begin in the sample · LOOP |
| **SOUND B** | OFF or a second sound, SEMI and TUNE · LAYER: STACK (both), VEL X (soft notes A, hard notes B), SPLIT (B below the SPLIT key) |
| **MIXER** | the two sounds and NOISE · **CHARACTER**: **EII** (8-bit μ-law, no interpolation: the Emulator II's grain), **EMU 1** (8-bit linear, rougher), **CLEAN** (the recordings as they are) |
| **SPLIT / DRIFT** | the split key · DRIFT, an old machine's clocks: every note a little out of tune, its own way |
| **FILTER** | LPF24 (the Emulator II's SSM 24 dB), LPF12, BPF, HPF and MS20 · EG INT, KEY TRACK |
| **EG / LFO** | FILTER EG and AMP EG · two LFOs (SAW, SQR, TRI, SIN, S&H) with key sync and tempo sync |
| **PATCH 1–4** | FEG, AEG, LFO 1, LFO 2, velocity or keyboard to PITCH, B PITCH, BAL (between the two sounds), NOISE, CUTOFF, AMP or LFO 2's rate |
| **VOICE** | POLY (eight voices; five when SOUND B is stacked or crossfaded), MONO, LEGATO, UNISON · DETUNE, PORTA, SPREAD |
| **ARP / SEQ** | ten arp types over 1–4 octaves with latch and swing · a 16-step sequencer with ties, accents and slides |
| **EFFECTS** | MOD FX (chorus, flanger, ensemble, phaser, METAL), delay, reverb, in three orders |
| **PROGRAMS** | 32: STRINGS, CHOIR, ORCH HIT, PIANO, BRASS, FLUTE, CHOIR STRING, TUBULAR BELL, TIMPANI, SHAKUHACHI, BOW + PLUCK, BASS / PIANO, HIT ARP… 24 factory sounds and eight free |

## The library

PIANO, STRINGS, CHOIR, BRASS, FLUTE, CELLO, ORGAN, HARPSI, MARIMBA, BELLS, PIZZ, BASS, TIMPANI, ORCH HIT, RECORDER and
HARP: 41 multisampled zones, 27 seconds of sound in 283 KB. The recordings are CC0 (Versilian Studios' VSCO-2 CE and
VCSL); CHOIR has no free recording and is synthesized (eight voices singing "ah"), ORCH HIT is struck together from
trumpets, violins and a timpani. None of E-mu's own samples are used: the Emulator II's sound comes from how
Ditto plays them.

## The controls

| Control | |
| --- | --- |
| **HOME** | PERFORM: cutoff, resonance, EG int, LFO 1 rate |
| **EDIT / SCL / ENV / LFO** | SOUND A, SOUND B, MIXER, SPLIT (split key, DRIFT) / the filter / the envelopes / the LFOs and the patch bay |
| **GLO / REC / SEQ / FX** | amp, voice, global, setup / the sequencer / the arpeggiator / the effects |
| **KNOB 1–4** | the four cards on the screen · **ALGORITHM**: the cutoff from any page · **PRESETS**: the program |
| **ARP / PLAY / SAVE** | arpeggiator on and off / the sequencer / write a program |

## Install

1. Open the **[web installer](https://gadlele.github.io/Ditto/install/)** in Chrome or Edge on a computer.
2. Connect the FM-1 with a USB **data** cable, straight into the computer.
3. Follow the five steps: the checklist, a backup, the build, connect, install. About two minutes.

From a terminal instead: `python3 tools/fm1_install.py Ditto-1.0.fwsc` (needs `pip install mido python-rtmidi`).

**If something goes wrong:** switch the FM-1 on while holding **OCT−**. It starts **DITTO USB RESCUE**, with only
USB, ready for a new install (it also starts by itself after two failed starts). The firmware never writes the FM-1's
boot code, and the installer can put the official M-VAVE firmware back at any time.

## The code

| | |
| --- | --- |
| `main` | the latest release, now **1.0** |
| `next` | the work on the next version |
| [Releases](https://github.com/Gadlele/Ditto/releases) | every version: its notes and its package |
| [`gh-pages`](https://github.com/Gadlele/Ditto/tree/gh-pages) | the website: project page, installer, emulator, package |

- [The manual](docs/MANUAL.md): every section, page and control
- [What changed](CHANGELOG.md), version by version · [How a version is released](RELEASING.md)
- Building: [BUILDING.md](BUILDING.md) (`./build.sh --release 1.0`); the tests: `sh tests/run_tests.sh`
- The sound library: `tools/fetch_eii.py` (the CC0 recordings), `tools/gen_ditto_samples.py` (the library, at build time)

Inside, Ditto keeps the frame it shares with Golana under Golana's names (`eng_golana.c`, `golana_ui.c`, the
`GOLANA` build switch), so the two can still trade fixes; `core.h` names what each slot means here (`DT_*`).

## Credits

Ditto is by **Gadlele**. It is a fork of [Golana](https://github.com/Gadlele/Golana), a fork of
[Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (@kurogedelic), Hügelton Instruments. The sound
library is built from Versilian Studios' CC0 recordings ([VSCO-2 CE](https://github.com/sgossner/VSCO-2-CE),
[VCSL](https://github.com/sgossner/VCSL)). The USB rescue follows zvenson's [dinghy](https://github.com/zvenson/dinghy).
See [LICENSING.md](LICENSING.md) for every bundled work and its licence.

Ditto is free software under the [GPL-3.0](LICENSE), with no warranty. M-VAVE and FM-1 are trademarks of their
respective owners, Emulator and E-mu are trademarks of their owners; Ditto is independent and not affiliated
with them, and contains none of E-mu's samples or code.
