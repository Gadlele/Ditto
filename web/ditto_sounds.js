// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Gadlele; the protocol and the sample code after Felucca's web/editor.html (C) 2026 Leo Kuroshita
// (@kurogedelic), Hügelton Instruments
/* Ditto's SOUNDS page (web/site/sounds.html), the parts that need no page: your files -> a sound for a user slot of
 * the FM-1 (USR1-3, eng_sample.c), and the editor protocol to send it (web/EDITOR_PROTOCOL.md, SMP_*).
 *
 *   decodeAudio(buf, name, ctx)     WAV / AIFF read here; MP3, OGG, FLAC .. by the browser -> {sr, x (mono Float64)}
 *   analyse(x, sr, name)            trimmed, normalised; its root (from the name, else heard: YIN), sustained or not
 *   prepare(zones, rate, slotBytes) every zone at the rate, looped or not, its keys, shortened to fit -> a sound
 *   buildSlot(name, sound)          -> {hdr (480 B), data}: the slot as SMP_END / SMP_WRITE take it
 *   renderNote(zone, note, opt)     the note as the FM-1 plays it (the IMA ADPCM decoded, EII 8-bit mu-law, held)
 *   Link, req, parse                the protocol: one request in flight, replies matched
 * Node runs the tests (web/test_sounds.mjs). */

/* ---------------------------------------------------------------- protocol --- */
export const HDR = [0x7D, 0x46, 0x4C];
export const CMD = { INFO: 1, SMP_BEGIN: 11, SMP_WRITE: 12, SMP_END: 13, SMP_ERASE: 14, SMP_INFO: 15 };
export function frame(cmd, args = []) {
  for (const b of args) if (!(Number.isInteger(b) && b >= 0 && b <= 0x7F)) throw new Error("bad data byte " + b);
  return [0xF0, ...HDR, cmd & 0x7F, ...args, 0xF7];
}
export function unframe(d) {
  if (!d || d.length < 6 || d[0] !== 0xF0 || d[d.length - 1] !== 0xF7) return null;
  if (d[1] !== HDR[0] || d[2] !== HDR[1] || d[3] !== HDR[2]) return null;
  return { cmd: d[4], a: Array.from(d.slice(5, d.length - 1)) };
}
/* groups of up to 7 bytes, each preceded by their top bits */
export function pack7(b) {
  const out = [];
  for (let i = 0; i < b.length; i += 7) {
    const g = Array.from(b.slice(i, i + 7));
    out.push(g.reduce((m, x, j) => m | (((x >> 7) & 1) << j), 0));
    for (const x of g) out.push(x & 0x7F);
  }
  return out;
}
class Reader {
  constructor(a) { this.a = a; this.i = 0; }
  b() { if (this.i >= this.a.length) throw new Error("short reply"); return this.a[this.i++]; }
  s() { let s = ""; for (;;) { const c = this.b(); if (!c) return s; s += String.fromCharCode(c); } }
}
export const parse = {
  [CMD.INFO](a) {
    const r = new Reader(a), o = { version: r.s(), nengines: r.b(), engines: [] };
    r.b(); r.b(); r.b(); r.b();                       /* (pcount, gcount, nstep, pe0) */
    for (let i = 0; i < o.nengines; i++) o.engines.push(r.s());
    return o;
  },
  [CMD.SMP_BEGIN](a) { const r = new Reader(a); return { slot: r.b(), rc: r.b() }; },
  [CMD.SMP_WRITE](a) { const r = new Reader(a); return { slot: r.b(), offset: r.b() | (r.b() << 7) | (r.b() << 14), rc: r.b() }; },
  [CMD.SMP_END](a) { const r = new Reader(a); return { slot: r.b(), rc: r.b() }; },
  [CMD.SMP_ERASE](a) { const r = new Reader(a); return { slot: r.b(), rc: r.b() }; },
  [CMD.SMP_INFO](a) {
    const r = new Reader(a), o = { nslots: r.b(), slotKiB: r.b(), slots: [] };
    for (let i = 0; i < o.nslots; i++) o.slots.push({ zones: r.b(), name: r.s(), kib: r.b() });
    return o;
  },
};
export const req = {
  info: () => [CMD.INFO, []],
  smpBegin: (s) => [CMD.SMP_BEGIN, [s & 0x7F]],
  smpWrite: (s, off, bytes) => [CMD.SMP_WRITE, [s & 0x7F, off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F, ...pack7(bytes)]],
  smpEnd: (s, hdr) => [CMD.SMP_END, [s & 0x7F, ...pack7(hdr)]],
  smpErase: (s) => [CMD.SMP_ERASE, [s & 0x7F]],
  smpInfo: () => [CMD.SMP_INFO, []],
};
function replyMatches(cmd, args, a) {
  if (cmd === CMD.SMP_WRITE) return a[0] === args[0] && a[1] === args[1] && a[2] === args[2] && a[3] === args[3];
  if (cmd === CMD.SMP_BEGIN || cmd === CMD.SMP_END || cmd === CMD.SMP_ERASE) return a[0] === args[0];
  return true;
}
export const SMP_RC = { 1: "size", 2: "header", 3: "data CRC", 4: "flash", 5: "zones" };
/* one request in flight, the rest queued; a reply matched to its request, everything else ignored */
export class Link {
  constructor(send, opts = {}) { this.send = send; this.q = []; this.cur = null; this.closed = false; this.timeout = opts.timeout || 400; }
  request([cmd, args], opt = {}) {
    if (this.closed) return Promise.reject(new Error("closed"));
    return new Promise((res, rej) => {
      this.q.push({ cmd, args, res, rej, timeout: opt.timeout || this.timeout, retries: opt.retries ?? 1 });
      this.pump();
    });
  }
  pump() { if (!this.cur && !this.closed && this.q.length) { this.cur = this.q.shift(); this.fire(); } }
  fire() {
    const c = this.cur;
    try { this.send(frame(c.cmd, c.args)); } catch (e) { this.finish(null, e); return; }
    c.timer = setTimeout(() => {
      if (this.cur !== c) return;
      if (c.retries-- > 0) { this.fire(); return; }
      this.finish(null, new Error("no reply (cmd " + c.cmd + ")"));
    }, c.timeout);
  }
  finish(val, err) {
    const c = this.cur;
    if (!c) return;
    clearTimeout(c.timer);
    this.cur = null;
    if (err) c.rej(err); else c.res(val);
    this.pump();
  }
  receive(data) {
    const f = unframe(data), c = this.cur;
    if (f && c && f.cmd === c.cmd && replyMatches(c.cmd, c.args, f.a)) this.finish(f.a);
  }
  close() {
    this.closed = true;
    const all = (this.cur ? [this.cur] : []).concat(this.q);
    if (this.cur) clearTimeout(this.cur.timer);
    this.cur = null; this.q = [];
    for (const r of all) r.rej(new Error("closed"));
  }
}

/* ------------------------------------------------------------- reading --- */
/* WAV: PCM 8/16/24/32 or float, any channel count -> {sr, x} (mono) */
export function parseWav(buf) {
  const d = new Uint8Array(buf), v = new DataView(d.buffer, d.byteOffset, d.byteLength);
  const id = (i) => String.fromCharCode(d[i], d[i + 1], d[i + 2], d[i + 3]);
  if (d.length < 12 || id(0) !== "RIFF" || id(8) !== "WAVE") throw new Error("not a WAV file");
  let i = 12, fmt = null, data = null;
  while (i + 8 <= d.length) {
    const cid = id(i), n = v.getUint32(i + 4, true);
    if (cid === "fmt ") {
      let tag = v.getUint16(i + 8, true);
      const ch = v.getUint16(i + 10, true), sr = v.getUint32(i + 12, true), bits = v.getUint16(i + 22, true);
      if (tag === 0xFFFE) tag = v.getUint16(i + 32, true);
      fmt = { tag, ch, sr, bits };
    } else if (cid === "data") { data = [i + 8, Math.min(d.length, i + 8 + n)]; break; }
    i += 8 + n + (n & 1);
  }
  if (!fmt || !data) throw new Error("WAV without fmt/data");
  const { tag, ch, sr, bits } = fmt, bps = bits >> 3, fr = bps * ch;
  if (!(tag === 1 || tag === 3) || !bps || !ch) throw new Error("unsupported WAV format");
  const nfr = Math.floor((data[1] - data[0]) / fr), x = new Float64Array(nfr);
  for (let f = 0; f < nfr; f++) {
    let acc = 0;
    for (let c = 0; c < ch; c++) {
      const o = data[0] + f * fr + c * bps;
      let s;
      if (tag === 3) s = bps === 4 ? v.getFloat32(o, true) : v.getFloat64(o, true);
      else if (bps === 2) s = v.getInt16(o, true) / 32768;
      else if (bps === 3) s = (((d[o] | (d[o + 1] << 8) | (d[o + 2] << 16)) << 8) >> 8) / 8388608;
      else if (bps === 4) s = v.getInt32(o, true) / 2147483648;
      else s = (d[o] - 128) / 128;
      acc += s;
    }
    x[f] = acc / ch;
  }
  return { sr, x };
}
/* AIFF / AIFC (uncompressed, 'NONE' or 'sowt'): PCM 8..32 bit -> {sr, x} (mono) */
export function parseAiff(buf) {
  const d = new Uint8Array(buf), v = new DataView(d.buffer, d.byteOffset, d.byteLength);
  const id = (i) => String.fromCharCode(d[i], d[i + 1], d[i + 2], d[i + 3]);
  if (d.length < 12 || id(0) !== "FORM" || !["AIFF", "AIFC"].includes(id(8))) throw new Error("not an AIFF file");
  let i = 12, comm = null, ssnd = null, le = false;
  while (i + 8 <= d.length) {
    const cid = id(i), n = v.getUint32(i + 4);
    if (cid === "COMM") {
      const ch = v.getUint16(i + 8), bits = v.getUint16(i + 14);
      const e = v.getUint16(i + 16) & 0x7FFF, hi = v.getUint32(i + 18), lo = v.getUint32(i + 22);
      const sr = Math.round((hi * 2 ** 32 + lo) * 2 ** (e - 16383 - 63));
      if (id(8) === "AIFC" && n >= 22) { const c = id(i + 26); if (c === "sowt") le = true; else if (c !== "NONE") throw new Error("compressed AIFC"); }
      comm = { ch, bits, sr };
    } else if (cid === "SSND") { const off = v.getUint32(i + 8); ssnd = [i + 16 + off, Math.min(d.length, i + 8 + n)]; }
    i += 8 + n + (n & 1);
  }
  if (!comm || !ssnd) throw new Error("AIFF without COMM/SSND");
  const bps = (comm.bits + 7) >> 3, fr = bps * comm.ch, nfr = Math.floor((ssnd[1] - ssnd[0]) / fr), x = new Float64Array(nfr);
  for (let f = 0; f < nfr; f++) {
    let acc = 0;
    for (let c = 0; c < comm.ch; c++) {
      const o = ssnd[0] + f * fr + c * bps;
      let s;
      if (bps === 1) s = ((d[o] << 24) >> 24) / 128;
      else if (bps === 2) s = v.getInt16(o, le) / 32768;
      else if (bps === 3) s = le ? (((d[o] | (d[o + 1] << 8) | (d[o + 2] << 16)) << 8) >> 8) / 8388608
                                 : (((d[o] << 16 | d[o + 1] << 8 | d[o + 2]) << 8) >> 8) / 8388608;
      else s = v.getInt32(o, le) / 2147483648;
      acc += s;
    }
    x[f] = acc / comm.ch;
  }
  return { sr: comm.sr, x };
}
/* any file: WAV and AIFF here, the rest by the browser (ctx: an AudioContext / OfflineAudioContext, or null) */
export async function decodeAudio(buf, name, ctx) {
  try { return parseWav(buf); } catch (e) { /* next */ }
  try { return parseAiff(buf); } catch (e) { /* next */ }
  if (!ctx) throw new Error(name + ": cannot read this file");
  const ab = await ctx.decodeAudioData(buf.slice(0));
  const x = new Float64Array(ab.length);
  for (let c = 0; c < ab.numberOfChannels; c++) { const ch = ab.getChannelData(c); for (let i = 0; i < ch.length; i++) x[i] += ch[i] / ab.numberOfChannels; }
  return { sr: ab.sampleRate, x };
}

/* ------------------------------------------------------------ analysing --- */
/* a moving average over the ratio when going down, then linear interpolation */
export function resample(x, sr, to) {
  if (sr === to) return Float64Array.from(x);
  if (sr > to) {
    const k = Math.max(1, Math.round(sr / to));
    if (k > 1) {
      const y = new Float64Array(x.length);
      let s = 0;
      for (let i = 0; i < x.length; i++) { s += x[i]; if (i >= k) s -= x[i - k]; y[i] = s / Math.min(k, i + 1); }
      x = y;
    }
  }
  const step = sr / to, n = Math.max(1, Math.floor((x.length - 1) / step)), out = new Float64Array(n);
  for (let j = 0; j < n; j++) { const p = j * step, i = Math.floor(p), f = p - i; out[j] = x[i] * (1 - f) + x[Math.min(x.length - 1, i + 1)] * f; }
  return out;
}
const NOTE = { C: 0, D: 2, E: 4, F: 5, G: 7, A: 9, B: 11 };
export const NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
export const noteName = (n) => NOTE_NAMES[((n % 12) + 12) % 12] + (Math.floor(n / 12) - 1);   /* 60 = C4 */
/* the root a file's name says ("vox_C4", "F#3", "Bb2"): C4 = 60; null when it says none */
export function rootFromName(name) {
  const m = /(?<![A-Za-z])([A-G])([#b]?)(-?\d)(?!\d)/.exec(name.replace(/\.[^.]*$/, ""));
  if (!m) return null;
  const n = NOTE[m[1]] + (m[2] === "#" ? 1 : m[2] === "b" ? -1 : 0) + (parseInt(m[3], 10) + 1) * 12;
  return n >= 0 && n <= 127 ? n : null;
}
/* YIN over w (a few periods): the fundamental in Hz, or 0 when it does not find one clearly */
export function yinHz(w, sr, fmin = 40, fmax = 2000, th = 0.15) {
  const tmin = Math.max(2, Math.floor(sr / fmax)), tmax = Math.min(Math.floor(w.length / 2), Math.ceil(sr / fmin));
  if (tmax <= tmin + 2) return 0;
  const W = w.length - tmax, d = new Float64Array(tmax + 1);
  for (let t = 1; t <= tmax; t++) { let s = 0; for (let i = 0; i < W; i++) { const e = w[i] - w[i + t]; s += e * e; } d[t] = s; }
  let run = 0, best = -1;
  const c = new Float64Array(tmax + 1);
  c[0] = 1;
  for (let t = 1; t <= tmax; t++) { run += d[t]; c[t] = run ? d[t] * t / run : 1; }
  for (let t = tmin; t <= tmax; t++) if (c[t] < th) { while (t + 1 <= tmax && c[t + 1] < c[t]) t++; best = t; break; }
  if (best < 0) return 0;
  const a = c[best - 1] ?? c[best], b = c[best], e = c[best + 1] ?? c[best], den = a - 2 * b + e;
  const tb = den ? best + (a - e) / (2 * den) : best;
  return sr / tb;
}
export const hzToMidi = (hz) => 69 + 12 * Math.log2(hz / 440);
function rms(x, a, b) { let s = 0; for (let i = a; i < b; i++) s += x[i] * x[i]; return Math.sqrt(s / Math.max(1, b - a)); }
/* a file as a zone: the sound itself (the silence before and after cut, a 5 ms pre-roll), its peak at 1; its root:
 * the name's, else heard (YIN over the steady part), else C4; sustained when its second half is still loud */
export function analyse(x, sr, name) {
  let pk = 0;
  for (const v of x) pk = Math.max(pk, Math.abs(v));
  if (!pk) throw new Error(name + ": silent");
  const th = pk * 0.02;
  let a = 0, b = x.length - 1;
  while (a < x.length && Math.abs(x[a]) < th) a++;
  while (b > a && Math.abs(x[b]) < th) b--;
  a = Math.max(0, a - Math.round(sr * 0.005));
  b = Math.min(x.length, b + 1 + Math.round(sr * 0.02));
  const y = new Float64Array(b - a);
  for (let i = 0; i < y.length; i++) y[i] = x[a + i] / pk;
  const n = y.length, named = rootFromName(name);
  let root = named, from = "name";
  if (root == null) {
    const at = Math.floor(n * 0.3), len = Math.min(n - at, Math.round(sr * 0.08));
    const hz = len > sr * 0.02 ? yinHz(y.subarray(at, at + len), sr) : 0;
    if (hz) { root = Math.round(hzToMidi(hz) * 16) / 16; from = "heard"; }
    else { root = 60; from = "none"; }
  }
  const early = rms(y, Math.floor(n * 0.1), Math.floor(n * 0.3)), late = rms(y, Math.floor(n * 0.6), Math.floor(n * 0.9));
  const sustained = n > sr * 0.4 && late > early * 0.35;
  return { name, sr, x: y, root, rootFrom: from, loop: sustained, sec: n / sr };
}

/* -------------------------------------------------------------- the sound --- */
export const RATES = [22050, 16000, 11025];
export const SLOT = { DATA_OFF: 512, HDR_LEN: 32 + 16 * 28, MAX_ZONES: 16 };
const XF = 0.08;                                     /* a loop's crossfade, s */
/* a loop in y (Float64, peak 1): its start ls where the samples before it are most like the ones before the end
 * (searched in 35..60 % of the sound), its end the last sample; the end crossfaded into the start over XF */
export function findLoop(y, sr, at = null) {
  const n = y.length, le = n - 1, w = Math.max(16, Math.min(Math.round(sr * 0.006), 256));
  const xf = Math.min(Math.round(sr * XF), Math.floor(n * 0.2));
  let best = Math.floor(n * 0.45), bestE = Infinity;
  /* at (0..1, the page's marker): searched within 3 % of it */
  const lo = at == null ? Math.floor(n * 0.35) : Math.floor(n * (at - 0.03)), hi = at == null ? Math.floor(n * 0.6) : Math.floor(n * (at + 0.03));
  for (let ls = Math.max(xf + w, lo); ls < Math.min(hi, le - xf); ls++) {
    let e = 0;
    for (let k = 1; k <= w && e < bestE; k++) { const d = y[le + 1 - k] - y[ls - k]; e += d * d; }
    if (e < bestE) { bestE = e; best = ls; }
  }
  const z = Float64Array.from(y);
  for (let i = 0; i < xf; i++) {                      /* the end's last xf samples melt into the ones before ls */
    const g = (i + 1) / xf;
    z[le - xf + 1 + i] = y[le - xf + 1 + i] * (1 - g) + y[best - xf + 1 + i] * g;
  }
  return { x: z, ls: best, le };
}
function fadeOut(x, sec, sr) {
  const n = x.length, f = Math.min(n >> 2, Math.round(sec * sr));
  for (let i = 0; i < f; i++) x[n - f + i] *= 1 - (i + 1) / f;
}
/* zones (analyse's, each with loop on / off and its root) -> the sound at rate: each zone resampled, shortened so the
 * whole fits slotBytes of ADPCM (one-shots lose their tails, looped zones their length after the loop is kept
 * shorter), looped or faded, peak 30000 int16, its keys halfway to its neighbours. {zones, bytes, fit, rate}
 * A zone may carry the page's markers: end (0..1, where it is cut), loopAt (0..1, near where its loop begins) */
export function prepare(zones, rate, slotBytes) {
  if (!zones.length) throw new Error("no zones");
  if (zones.length > SLOT.MAX_ZONES) throw new Error("16 files at most");
  const max = slotBytes - SLOT.DATA_OFF;
  const rs = zones.map((z) => resample(z.x, z.sr, rate));
  const total = rs.reduce((s, x) => s + x.length, 0);
  let keep = Math.min(1, (max * 2 - 64 * zones.length) / total);
  const out = zones.map((z, k) => {
    let x = rs[k];
    if (z.end != null && z.end < 1) x = x.slice(0, Math.max(Math.round(rate * 0.05), Math.floor(x.length * z.end)));   /* the page's END */
    const n = Math.max(Math.round(rate * 0.05), Math.floor(x.length * keep));
    if (n < x.length) x = x.slice(0, n);
    let ls = 0, le = 0, looped = false;
    if (z.loop && x.length > rate * 0.15) { const l = findLoop(x, rate, z.loopAt); x = l.x; ls = l.ls; le = l.le; looped = true; }
    else { x = Float64Array.from(x); fadeOut(x, n < rs[k].length ? 0.06 : 0.01, rate); }
    let pk = 1e-9;
    for (const v of x) pk = Math.max(pk, Math.abs(v));
    const s = Int16Array.from(x, (v) => Math.max(-32768, Math.min(32767, Math.trunc(v / pk * 30000))));
    return { name: z.name, s, ls, le: looped ? le : s.length - 1, looped, root: z.root, rate };
  });
  out.sort((a, b) => a.root - b.root);
  out.forEach((z, j) => {
    z.lo = j === 0 ? 0 : Math.floor((out[j - 1].root + z.root) / 2) + 1;
    z.hi = j === out.length - 1 ? 127 : Math.floor((z.root + out[j + 1].root) / 2);
  });
  const bytes = out.reduce((s, z) => s + ((z.s.length + 1) >> 1) + ((z.s.length + 1) >> 1 & 1), 0);
  return { zones: out, bytes, max, fit: keep >= 1, rate };
}

/* IMA ADPCM, 4 bit, low nibble first, from predictor 0 / index 0 -> {data, pred, idx} (the state at loopStart) */
const IMA_STEP = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88,
  97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
  724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660,
  4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818,
  18500, 20350, 22385, 24623, 27086, 29794, 32767];
const IMA_IDX = [-1, -1, -1, -1, 2, 4, 6, 8];
export function imaEncode(s, loopStart = 0) {
  let pred = 0, idx = 0, atLoop = [0, 0];
  const nib = new Uint8Array(s.length + (s.length & 1));
  for (let n = 0; n < s.length; n++) {
    if (n === loopStart) atLoop = [pred, idx];
    const step = IMA_STEP[idx];
    let diff = s[n] - pred, code = 0;
    if (diff < 0) { code = 8; diff = -diff; }
    let vd = step >> 3;
    if (diff >= step) { code |= 4; diff -= step; vd += step; }
    if (diff >= step >> 1) { code |= 2; diff -= step >> 1; vd += step >> 1; }
    if (diff >= step >> 2) { code |= 1; vd += step >> 2; }
    pred = Math.max(-32768, Math.min(32767, code & 8 ? pred - vd : pred + vd));
    idx = Math.max(0, Math.min(88, idx + IMA_IDX[code & 7]));
    nib[n] = code;
  }
  const out = new Uint8Array(nib.length >> 1);
  for (let k = 0; k < nib.length; k += 2) out[k >> 1] = nib[k] | (nib[k + 1] << 4);
  return { data: out, pred: atLoop[0], idx: atLoop[1] };
}
/* the firmware's decoder (eng_golana.c fo_next): data -> int16, n samples */
export function imaDecode(data, n) {
  const out = new Int16Array(n);
  let pred = 0, idx = 0;
  for (let p = 0; p < n; p++) {
    const b = data[p >> 1], code = p & 1 ? b >> 4 : b & 15, step = IMA_STEP[idx];
    let vd = step >> 3;
    if (code & 4) vd += step;
    if (code & 2) vd += step >> 1;
    if (code & 1) vd += step >> 2;
    pred = Math.max(-32768, Math.min(32767, code & 8 ? pred - vd : pred + vd));
    idx = Math.max(0, Math.min(88, idx + IMA_IDX[code & 7]));
    out[p] = pred;
  }
  return out;
}
const CRC_TABLE = (() => {
  const t = new Uint32Array(256);
  for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; t[n] = c >>> 0; }
  return t;
})();
export function crc32(b) { let c = 0xFFFFFFFF; for (let i = 0; i < b.length; i++) c = CRC_TABLE[(c ^ b[i]) & 0xFF] ^ (c >>> 8); return (c ^ 0xFFFFFFFF) >>> 0; }
/* the sound -> the slot: {hdr: 480 B (eng_sample.c smp_user_hdr_t), data: the zones' ADPCM, each on an even offset} */
export function buildSlot(name, sound) {
  const parts = [], zs = [];
  let len = 0;
  for (const z of sound.zones) {
    const e = imaEncode(z.s, z.looped ? z.ls : 0);
    zs.push({ ...z, off: len, pred: e.pred, idx: e.idx });
    parts.push(e.data);
    len += e.data.length + (e.data.length & 1);
  }
  if (len > sound.max) throw new Error(`too long: ${len} B of ${sound.max}`);
  const data = new Uint8Array(len);
  zs.forEach((z, k) => data.set(parts[k], z.off));
  const hdr = new Uint8Array(SLOT.HDR_LEN), v = new DataView(hdr.buffer);
  v.setUint32(0, 0x504D5346, true);                  /* "FSMP" */
  v.setUint16(4, 1, true);
  hdr[6] = zs.length;
  const nm = slotName(name);
  for (let i = 0; i < nm.length; i++) hdr[8 + i] = nm.charCodeAt(i);
  v.setUint32(16, len, true);
  v.setUint32(20, crc32(data), true);
  zs.forEach((z, j) => {
    const b = 32 + j * 28, rate = Math.round(z.rate / 44100 * 65536);
    [z.off, z.s.length, z.looped ? z.ls : 0, z.le, rate].forEach((x, k) => v.setUint32(b + k * 4, x >>> 0, true));
    v.setInt16(b + 20, Math.round(z.root * 16), true);
    v.setInt16(b + 22, z.pred, true);
    hdr[b + 24] = z.idx; hdr[b + 25] = z.lo; hdr[b + 26] = z.hi; hdr[b + 27] = z.looped ? 1 : 0;
  });
  return { hdr, data };
}
export const slotName = (s) => String(s || "SOUND").toUpperCase().replace(/[^ -_]/g, "").slice(0, 8);

/* ------------------------------------------------------------- the preview --- */
/* G.711 mu-law, 8 bits, as eng_golana.c fo_char */
export function muLaw8(s) {
  let a = Math.min(Math.abs(s), 32635) + 132, e = 7;
  while (e > 0 && !(a & (0x80 << e))) e--;
  a = ((((a >> (e + 3)) & 15) << 3) + 132 << e) - 132;
  return s < 0 ? -a : a;
}
/* a zone's samples as the FM-1 holds them: encoded and decoded (the ADPCM's own grain) */
export function zonePlayback(z) {
  if (!z.dec) z.dec = imaDecode(imaEncode(z.s, z.looped ? z.ls : 0).data, z.s.length);
  return z.dec;
}
/* note on zone z, sec seconds at 44.1 kHz: the sample stepped at its rate x the pitch, EII (mu-law, each sample held)
 * or CLEAN (interpolated), looped from le back to ls; -> {buf: Float32Array, loopA, loopB (two wrap points, to loop
 * the buffer on while the key is held; -1 none)} */
export function renderNote(z, note, opt = {}) {
  const sec = opt.sec || 6, eii = opt.eii !== false, out = new Float32Array(Math.round(44100 * sec));
  const s = zonePlayback(z), step = z.rate / 44100 * Math.pow(2, (note - z.root) / 12);
  let pos = 0, wraps = [];
  for (let i = 0; i < out.length; i++) {
    const p = Math.floor(pos);
    if (p >= s.length) break;
    let v;
    if (eii) v = muLaw8(s[p]);
    else { const f = pos - p, q = p + 1 <= (z.looped ? z.le : s.length - 1) ? p + 1 : z.looped ? z.ls : p; v = s[p] * (1 - f) + s[q] * f; }
    out[i] = v / 32768 * 0.8;
    pos += step;
    if (z.looped && pos > z.le + 1) { pos = z.ls + (pos - z.le - 1); wraps.push(i + 1); }
  }
  return { buf: out, loopA: wraps.length >= 2 ? wraps[0] : -1, loopB: wraps.length >= 2 ? wraps[wraps.length - 1] : -1 };
}
export const zoneOf = (sound, note) => sound.zones.find((z) => note >= z.lo && note <= z.hi) || null;

/* ---------------------------------------------------------- the .ditto file --- */
/* a sound kept: its name, quality and zones (the prepared samples, base64 int16) */
export function soundToFile(name, sound) {
  const b64 = (s) => { const u = new Uint8Array(s.buffer, s.byteOffset, s.byteLength); let t = ""; for (let i = 0; i < u.length; i += 0x8000) t += String.fromCharCode.apply(null, u.subarray(i, i + 0x8000)); return btoa(t); };
  return JSON.stringify({ format: "ditto-sound", version: 1, name: slotName(name), rate: sound.rate, max: sound.max,
    zones: sound.zones.map((z) => ({ name: z.name, root: z.root, lo: z.lo, hi: z.hi, looped: z.looped, ls: z.ls, le: z.le, rate: z.rate, s: b64(z.s) })) });
}
export function soundFromFile(text) {
  const j = JSON.parse(text);
  if (j.format !== "ditto-sound" || j.version !== 1) throw new Error("not a Ditto sound file");
  const u8 = (t) => { const b = atob(t), u = new Uint8Array(b.length); for (let i = 0; i < b.length; i++) u[i] = b.charCodeAt(i); return u; };
  const zones = j.zones.map((z) => { const u = u8(z.s); return { ...z, s: new Int16Array(u.buffer, 0, u.length >> 1) }; });
  const bytes = zones.reduce((s, z) => s + ((z.s.length + 1) >> 1) + ((z.s.length + 1) >> 1 & 1), 0);
  return { name: j.name, sound: { zones, bytes, max: j.max, fit: bytes <= j.max, rate: j.rate } };
}
