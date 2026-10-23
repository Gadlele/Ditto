// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Gadlele
/* The SOUNDS page's conversion (web/ditto_sounds.js) without a browser: files made here (WAV, AIFF), read, analysed,
 * prepared at the three qualities, built into a slot; the slot written to OUT (the firmware plays it in
 * tests/sounds_slot_test.c), the protocol framing.  node web/test_sounds.mjs [OUT.slot] */
import fs from "fs";
import * as S from "./ditto_sounds.js";

let bad = 0;
const ok = (c, what) => { console.log((c ? "ok   " : "FAIL ") + what); if (!c) bad++; };

function wav(x, sr, bits = 16, ch = 1) {
  const bps = bits >> 3, n = x.length, b = Buffer.alloc(44 + n * bps * ch);
  b.write("RIFF", 0); b.writeUInt32LE(36 + n * bps * ch, 4); b.write("WAVE", 8); b.write("fmt ", 12);
  b.writeUInt32LE(16, 16); b.writeUInt16LE(1, 20); b.writeUInt16LE(ch, 22); b.writeUInt32LE(sr, 24);
  b.writeUInt32LE(sr * bps * ch, 28); b.writeUInt16LE(bps * ch, 32); b.writeUInt16LE(bits, 34); b.write("data", 36);
  b.writeUInt32LE(n * bps * ch, 40);
  for (let i = 0; i < n; i++) for (let c = 0; c < ch; c++) {
    const o = 44 + (i * ch + c) * bps, v = Math.max(-1, Math.min(1, x[i]));
    if (bits === 16) b.writeInt16LE(Math.round(v * 32767), o); else b.writeIntLE(Math.round(v * 8388607), o, 3);
  }
  return b.buffer.slice(b.byteOffset, b.byteOffset + b.length);
}
function aiff(x, sr) {                                  /* 16-bit big-endian, the rate as an 80-bit float */
  const n = x.length, b = Buffer.alloc(54 + n * 2);
  b.write("FORM", 0); b.writeUInt32BE(46 + n * 2, 4); b.write("AIFF", 8); b.write("COMM", 12); b.writeUInt32BE(18, 16);
  b.writeUInt16BE(1, 20); b.writeUInt32BE(n, 22); b.writeUInt16BE(16, 26);
  const e = Math.floor(Math.log2(sr)), m = sr / 2 ** e;
  b.writeUInt16BE(16383 + e, 28); b.writeUInt32BE(Math.floor(m * 2 ** 31), 30); b.writeUInt32BE(0, 34);
  b.write("SSND", 38); b.writeUInt32BE(8 + n * 2, 42); b.writeUInt32BE(0, 46); b.writeUInt32BE(0, 50);
  for (let i = 0; i < n; i++) b.writeInt16BE(Math.round(x[i] * 32767), 54 + i * 2);
  return b.buffer.slice(b.byteOffset, b.byteOffset + b.length);
}
/* a tone: a little silence, an attack, a sustain with harmonics (sustained) or a decay (a pluck) */
function tone(hz, sec, sr, pluck = false) {
  const n = Math.round(sec * sr), x = new Float64Array(n), pre = Math.round(sr * 0.1);
  for (let i = pre; i < n; i++) {
    const t = (i - pre) / sr, env = Math.min(1, t / 0.02) * (pluck ? Math.exp(-t * 6) : 0.8);
    x[i] = env * (0.6 * Math.sin(2 * Math.PI * hz * t) + 0.25 * Math.sin(4 * Math.PI * hz * t) + 0.1 * Math.sin(6 * Math.PI * hz * t));
  }
  return x;
}

const SLOT_BYTES = 0x17000;
const a4 = await S.decodeAudio(wav(tone(440, 2.0, 44100), 44100), "a.wav", null);
ok(a4.sr === 44100 && a4.x.length === 88200, "WAV 16-bit read");
const st = await S.decodeAudio(wav(tone(440, 0.5, 48000), 48000, 24, 2), "s.wav", null);
ok(st.sr === 48000 && Math.abs(st.x[12000] - tone(440, 0.5, 48000)[12000]) < 1e-4, "WAV 24-bit stereo read, mixed to mono");
const ai = await S.decodeAudio(aiff(tone(220, 1.0, 44100), 44100), "b.aif", null);
ok(ai.sr === 44100 && ai.x.length === 44100, "AIFF read (the rate: " + ai.sr + ")");

ok(S.rootFromName("vox_ah_C3.wav") === 48 && S.rootFromName("Piano F#4.aif") === 66 && S.rootFromName("Bb2") === 46 &&
   S.rootFromName("breath high.wav") === null, "the root from a name (C4 = 60)");
const z1 = S.analyse(a4.x, a4.sr, "lead.wav");
ok(z1.rootFrom === "heard" && Math.abs(z1.root - 69) < 0.2, "the root heard: A4 (" + z1.root + ")");
ok(z1.loop && Math.abs(z1.sec - 1.9) < 0.05, "a sustained tone: looped, its silence cut (" + z1.sec.toFixed(2) + " s)");
const z2 = S.analyse(tone(330, 1.5, 44100, true), 44100, "pluck_E4.wav");
ok(z2.root === 64 && z2.rootFrom === "name" && !z2.loop, "a pluck: one-shot, the name's root E4");

for (const rate of S.RATES) {
  const snd = S.prepare([z1, z2], rate, SLOT_BYTES);
  ok(snd.fit && snd.bytes <= snd.max && snd.zones[0].root === 64 && snd.zones[0].lo === 0 && snd.zones[0].hi === 66 &&
     snd.zones[1].lo === 67 && snd.zones[1].hi === 127, `${rate} Hz: fits (${snd.bytes} of ${snd.max} B), the keys split at F#4`);
}
const long = S.analyse(tone(110, 40, 44100), 44100, "drone_A2.wav");
const snd = S.prepare([long, z2], 22050, SLOT_BYTES);
ok(!snd.fit && snd.bytes <= snd.max, `40 s at 22 kHz: shortened to fit (${snd.bytes} of ${snd.max} B)`);
const looped = snd.zones.find((z) => z.looped);
ok(looped && looped.ls > 0 && looped.le === looped.s.length - 1 && looped.ls < looped.le, "  the drone looped, its loop inside it");

const good = S.prepare([z1, z2], 16000, SLOT_BYTES), slot = S.buildSlot("my vox!", good);
const v = new DataView(slot.hdr.buffer);
ok(slot.hdr.length === 480 && v.getUint32(0, true) === 0x504D5346 && v.getUint16(4, true) === 1 && slot.hdr[6] === 2,
   "the header: FSMP, version 1, 2 zones");
ok(String.fromCharCode(...slot.hdr.subarray(8, 16)).replace(/\0+$/, "") === "MY VOX!", "  the name, upper case");
ok(v.getUint32(16, true) === slot.data.length && v.getUint32(20, true) === S.crc32(slot.data), "  the data's length and CRC");
const zb = 32 + 28;                                   /* zone 1: the looped A4 */
ok(v.getUint32(zb + 16, true) === Math.round(16000 / 44100 * 65536) && v.getInt16(zb + 20, true) === Math.round(good.zones[1].root * 16) &&
   slot.hdr[zb + 27] === 1 && v.getUint32(zb + 8, true) === good.zones[1].ls, "  zone 2: 16 kHz, its root, looped from ls");
const dec = S.imaDecode(slot.data.subarray(v.getUint32(zb, true)), good.zones[1].s.length);
let err = 0;
for (let i = 0; i < dec.length; i++) err = Math.max(err, Math.abs(dec[i] - good.zones[1].s[i]));
ok(err < 6000, "  the ADPCM decodes close to the sound (worst " + err + ")");

const r = S.renderNote(good.zones[1], 81, { sec: 4 });
let pk = 0;
for (const x of r.buf) pk = Math.max(pk, Math.abs(x));
ok(pk > 0.3 && r.loopA > 0 && r.loopB > r.loopA, "the preview: an octave up sounds, held on its loop");
ok(S.muLaw8(32767) === 32124 && S.muLaw8(-100) === -104 && S.muLaw8(0) === 0, "mu-law as the firmware's");

const back = S.soundFromFile(S.soundToFile("my vox", good));
ok(back.name === "MY VOX" && back.sound.zones.length === 2 && back.sound.zones[1].s.length === good.zones[1].s.length &&
   back.sound.zones[1].s[1234] === good.zones[1].s[1234], ".ditto file: written and read back");

const f = S.frame(S.CMD.SMP_INFO, []);
ok(f.join() === [0xF0, 0x7D, 0x46, 0x4C, 15, 0xF7].join() && S.unframe(Uint8Array.from(f)).cmd === 15, "the protocol's frames");
ok(S.pack7([0xFF, 1, 2, 3, 4, 5, 6, 0x80]).join() === [1, 127, 1, 2, 3, 4, 5, 6, 1, 0].join(), "pack7");

if (process.argv[2]) {                                /* the slot as the flash holds it: header at 0, data at 512 */
  const img = new Uint8Array(512 + slot.data.length).fill(0);
  img.set(slot.hdr, 0); img.set(slot.data, 512);
  fs.writeFileSync(process.argv[2], img);
  console.log("slot -> " + process.argv[2]);
}
console.log(bad ? `sounds: ${bad} FAILED` : "sounds: all ok");
process.exit(bad ? 1 : 0);
