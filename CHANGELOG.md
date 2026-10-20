# Changelog

Golana's versions, newest first. The numbers: **X.Y** adds something (saved programs still open), **X.Y.Z** only
fixes, a new **X** may change how things are saved.

## 1.2 — unreleased

- **SUB oscillator** (EDIT › SUB/DRIFT): a square or a sine one or two octaves below OSC 1, in step with it, with its
  own level into the mixer. Deep basses without giving up OSC 2.
- **DRIFT** (EDIT › SUB/DRIFT): analog age. Each note's two oscillators a little out of tune, each its own way, slowly
  wandering, and its filter a little off: from a touch of life to a worn-out machine.
- **Eight new factory sounds** in programs 17-24, made with them: SUB BASS, REESE, POLY CHORUS, OLD STRINGS,
  WORN TAPE, DRIFT LEAD, SUB ARP, AGED PAD. A program you wrote in 17-24 stays as it is.
- Programs saved with 1.0 and 1.1 load exactly as before (SUB and DRIFT off).

## 1.1 — 2026-10-19

- **Four new arpeggiator types** (SEQ › ARP › TYPE), after the six of 1.0, which play as before:
  - **ORDER**: the notes in the order you pressed them.
  - **CONVERGE**: from the outside in, the lowest, the highest, the second lowest, the second highest…
  - **PEDAL**: the lowest note between each of the others, as a pedal (C E C G C C′…).
  - **DOUBLE**: each note twice, then its octave twice (C C C′ C′, E E E′ E′, G G G′ G′).

## 1.0 — 2026-10-16

The first release. Golana is [Felucca](https://github.com/hugelton/Felucca) 1.5.1 turned into one analog-style synth,
laid out like the classic microKORG:

- **Two oscillators**: OSC 1 SAW, PULSE, TRI (wavefold) or SINE (cross modulation); OSC 2 SAW, SQR or TRI with RING
  and SYNC; noise and a wavefolder in the mixer.
- **Five filters**: LPF24, LPF12, BPF, HPF and MS20, with key tracking and its own envelope.
- **FILTER EG, AMP EG, two LFOs** with key and tempo sync, and **four patch cords**.
- **POLY, MONO, LEGATO, UNISON** (eight detuned oscillators), detune, portamento, spread.
- **Arpeggiator** and a **16-step sequencer** saved with each program.
- **Effects**: MOD FX (chorus, flanger, ensemble, phaser, METAL), delay and reverb in three orders.
- **32 programs**, 16 factory sounds; MIDI Program Change; CC74 and CC71 move cutoff and resonance.
- **Pages that draw** the sound; four palettes; a three-second power-on with the eighties' stripes.
- **USB rescue**: OCT− held at power-on, or after two failed starts.
