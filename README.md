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
  <a href="https://gadlele.github.io/Ditto/firmware/Ditto-1.0.1.fwsc">Download 1.0.1</a>
</p>

<p align="center">
  <img alt="version 1.0.1" src="https://img.shields.io/badge/version-1.0.1-ffb547?style=flat-square&labelColor=0e1118">
  <img alt="FM-1" src="https://img.shields.io/badge/for-M--VAVE%20FM--1-f07a9c?style=flat-square&labelColor=0e1118">
  <img alt="GPL-3.0" src="https://img.shields.io/badge/license-GPL--3.0-b092ff?style=flat-square&labelColor=0e1118">
</p>

**Ditto** turns the M-VAVE FM-1 into a sampler of the mid-eighties after the E-mu Emulator II, as dry as it was: a
library of sixteen sounds (strings, a choir, an orchestra hit, piano, brass, flute…), one or two of them on every note,
through an 8-bit companded sample player into a 24 dB low-pass and a VCA, each with its ADSR, and one LFO. A grey panel
with a two-line LCD, the Emulator II's modules and their numbered functions, four sliders. Delay and reverb, an
arpeggiator and a small step sequencer. Your own sounds too: the
[SOUNDS page](https://gadlele.github.io/Ditto/sounds/) turns your files into samples for its three user slots. It is a fork of [Golana](https://github.com/Gadlele/Golana), itself a fork of
[Felucca](https://github.com/hugelton/Felucca).

## The modules

| Button | Module | Functions (press again for the next) |
| --- | --- | --- |
| **HOME** | PERFORM | FREQ, RES, ENV, LFO RATE |
| **GLO** | MASTER | 1 TEMPO TUNE · 2 ASSIGN (POLY, MONO, LEGATO, UNISON; DETUNE, PORTA) · 3 OUTPUT |
| **SCL** | FILTER | 1 FREQ RES ENV · 2 LFO KBD · 3 ADSR (the Emulator II's SSM 24 dB low-pass) |
| **ENV** / **LFO** | LFO/VCA | 1 VCA ADSR · 2 LFO (WAVE, RATE, DELAY, VCA) · 3 LFO SYNC (LFO opens 2 directly) |
| **EDIT** | VOICE DEF | 11 SAMPLE (SOUND, START, LOOP, CHARACTER: EII 8-bit μ-law or CLEAN) · 21 VELOCITY (to level, to filter) · 23 VIBRATO |
| **SEQ** | PRESET DEF | 22 SOUND B (SEMI, TUNE, LEVEL) · 25 LAYER (STACK, VEL SW, VEL X, SPLIT, POS X) · 29–31 the arpeggiator |
| **REC** | SEQUENCER | 1 PATTERN (16 steps, drawn on the LCD; a key writes its note) · 2 SETUP |
| **FX** | EFFECTS | 1 DELAY · 2 REVERB |
| **SAVE** | DISK | write a preset: PRESETS the slot, KNOB 1 / 2 the name |

KNOB 1–4 are the four sliders, ALGORITHM is the filter's FREQ from anywhere, PRESETS picks the preset (32: 24 factory
sounds and eight free), ARP and PLAY as printed. Eight voices; five when SOUND B is stacked or crossfaded.

## The library

PIANO, STRINGS, CHOIR, BRASS, FLUTE, CELLO, ORGAN, HARPSI, MARIMBA, BELLS, PIZZ, BASS, TIMPANI, ORCH HIT, RECORDER and
HARP: 41 multisampled zones, 27 seconds of sound in 283 KB. The recordings are CC0 (Versilian Studios' VSCO-2 CE and
VCSL); CHOIR has no free recording and is synthesized (eight voices singing "ah"), ORCH HIT is struck together from
trumpets, violins and a timpani. None of E-mu's own samples are used: the Emulator II's sound comes from how
Ditto plays them.

## Install

1. Open the **[web installer](https://gadlele.github.io/Ditto/install/)** in Chrome or Edge on a computer.
2. Connect the FM-1 with a USB **data** cable, straight into the computer.
3. Follow the five steps: the checklist, a backup, the build, connect, install. About two minutes.

From a terminal instead: `python3 tools/fm1_install.py Ditto-1.0.1.fwsc` (needs `pip install mido python-rtmidi`).

**If something goes wrong:** switch the FM-1 on while holding **OCT−**. It starts **DITTO USB RESCUE**, with only
USB, ready for a new install (it also starts by itself after two failed starts). The firmware never writes the FM-1's
boot code, and the installer can put the official M-VAVE firmware back at any time.

## The code

| | |
| --- | --- |
| `main` | the latest release, now **1.0.1** |
| `next` | the work on the next version |
| [Releases](https://github.com/Gadlele/Ditto/releases) | every version: its notes and its package |
| [`gh-pages`](https://github.com/Gadlele/Ditto/tree/gh-pages) | the website: project page, installer, emulator, package |

- [The manual](docs/MANUAL.md): every section, page and control
- [What changed](CHANGELOG.md), version by version · [How a version is released](RELEASING.md)
- Building: [BUILDING.md](BUILDING.md) (`./build.sh --release 1.0.1`); the tests: `sh tests/run_tests.sh`
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
