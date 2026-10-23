// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Gadlele
/* The SOUNDS page in Chromium against a simulated FM-1 running Ditto (the editor protocol's SMP_* as editor.c answers
 * them: 3 slots of 92 KiB, a header taken only with its data's CRC): two files dropped in, the quality changed,
 * connected, sent to USR2 (what arrives checked byte by byte), emptied.
 *   node web/site/test_sounds_page.mjs SITE_DIR [SHOT.png]      (the site from make_pages.py)
 * Needs Playwright (PLAYWRIGHT=path/to/playwright/index.mjs if it is not found). */
const { chromium } = await import(process.env.PLAYWRIGHT || "playwright");
import fs from "fs";
import os from "os";
import path from "path";

const [, , site, shot] = process.argv;
let fails = 0;
const ok = (c, m) => { console.log((c ? "ok   " : "FAIL ") + m); if (!c) fails++; };

function wav(x, sr) {
  const n = x.length, b = Buffer.alloc(44 + n * 2);
  b.write("RIFF", 0); b.writeUInt32LE(36 + n * 2, 4); b.write("WAVE", 8); b.write("fmt ", 12); b.writeUInt32LE(16, 16);
  b.writeUInt16LE(1, 20); b.writeUInt16LE(1, 22); b.writeUInt32LE(sr, 24); b.writeUInt32LE(sr * 2, 28);
  b.writeUInt16LE(2, 32); b.writeUInt16LE(16, 34); b.write("data", 36); b.writeUInt32LE(n * 2, 40);
  for (let i = 0; i < n; i++) b.writeInt16LE(Math.round(Math.max(-1, Math.min(1, x[i])) * 32767), 44 + i * 2);
  return b;
}
function tone(hz, sec, pluck) {
  const sr = 44100, n = Math.round(sec * sr), x = new Float64Array(n);
  for (let i = 4410; i < n; i++) { const t = (i - 4410) / sr; x[i] = Math.min(1, t / 0.02) * (pluck ? Math.exp(-t * 6) : 0.7) * Math.sin(2 * Math.PI * hz * t); }
  return x;
}
const dir = fs.mkdtempSync(path.join(os.tmpdir(), "ditto-sounds-"));
const fA = path.join(dir, "vox_ah_C3.wav"), fB = path.join(dir, "pluck.wav");
fs.writeFileSync(fA, wav(tone(130.81, 2.5, false), 44100));
fs.writeFileSync(fB, wav(tone(523.25, 1.2, true), 44100));

/* the simulated FM-1: Web MIDI with SysEx, Ditto's slots */
const FAKE = `(() => {
  const SLOT = 0x17000, slots = [null, { name: "DRUMLOOP", zones: 1, kib: 88 }, null, null].slice(0, 3);
  const st = window.__fm1 = { slots, writes: {}, ended: {}, sysex: false };
  const unpack7 = (a) => { const o = []; for (let i = 0; i < a.length; i += 8) { const t = a[i]; for (let j = 0; j < 7 && i + 1 + j < a.length; j++) o.push(a[i + 1 + j] | ((t >> j) & 1) << 7); } return o; };
  const s = (t) => [...new TextEncoder().encode(t), 0];
  let input;
  const tx = (cmd, args) => setTimeout(() => input.onmidimessage && input.onmidimessage({ data: Uint8Array.from([240, 125, 70, 76, cmd, ...args, 247]) }), 2);
  const crc = (b) => { let c = 0xFFFFFFFF; for (const x of b) { c ^= x; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; } return (c ^ 0xFFFFFFFF) >>> 0; };
  function rx(d) {
    if (d[0] !== 240 || d[1] !== 125) return;
    const cmd = d[4], a = d.slice(5, -1);
    if (cmd === 1) tx(1, [...s("FELUCCA v1.0"), 2, 115, 27, 64, 107, ...s("ANALOG"), ...s("DITTO"), 4]);
    else if (cmd === 15) tx(15, [3, SLOT / 1024, ...slots.flatMap((x) => x ? [x.zones, ...s(x.name), x.kib] : [0, 0, 0])]);
    else if (cmd === 11) { st.writes[a[0]] = new Uint8Array(SLOT).fill(0xFF); slots[a[0]] = null; tx(11, [a[0], 0]); }
    else if (cmd === 12) {
      const k = a[0], off = a[1] | a[2] << 7 | a[3] << 14, bytes = unpack7(a.slice(4));
      const bad = !st.writes[k] || off < 512 || off % 256 || off + bytes.length > SLOT;
      if (!bad) st.writes[k].set(bytes, off);
      tx(12, [k, a[1], a[2], a[3], bad ? 1 : 0]);
    } else if (cmd === 13) {
      const k = a[0], h = Uint8Array.from(unpack7(a.slice(1))), v = new DataView(h.buffer), len = v.getUint32(16, true);
      const data = st.writes[k] ? st.writes[k].subarray(512, 512 + len) : null;
      const rc = h.length !== 480 ? 1 : v.getUint32(0, true) !== 0x504D5346 ? 2 : !data || crc(data) !== v.getUint32(20, true) ? 3 : 0;
      if (!rc) { const nm = String.fromCharCode(...h.subarray(8, 16)).replace(/\\0+$/, ""); slots[k] = { name: nm, zones: h[6], kib: Math.ceil((len + 512) / 1024) }; st.ended[k] = { hdr: Array.from(h), len }; }
      tx(13, [k, rc]);
    } else if (cmd === 14) { slots[a[0]] = null; tx(14, [a[0], 0]); }
  }
  input = { id: "i1", name: "Felucca", state: "connected", onmidimessage: null, open: async () => {} };
  const output = { id: "o1", name: "Felucca", state: "connected", open: async () => {}, send: (d) => setTimeout(() => rx(Array.from(d)), 1) };
  const access = { inputs: new Map([["i1", input]]), outputs: new Map([["o1", output]]), onstatechange: null };
  navigator.requestMIDIAccess = async (o) => { st.sysex = !!(o && o.sysex); return access; };
})();`;

const b = await chromium.launch();
const ctx = await b.newContext({ viewport: { width: 1280, height: 1000 } });
await ctx.addInitScript(FAKE);
const p = await ctx.newPage();
p.on("pageerror", (e) => { console.log("PAGEERROR", e.message); fails++; });
p.on("dialog", (d) => d.accept());
await p.goto("file://" + path.resolve(site, "sounds", "index.html"));
ok(/NEW SOUND/.test(await p.textContent("#lcd")), "the page opens: the LCD waits for files");
await p.setInputFiles("#files", [fA, fB]);
await p.waitForFunction(() => document.querySelectorAll("#zones tbody tr").length === 2, null, { timeout: 15000 });
const rows = await p.$$eval("#zones tbody tr", (rs) => rs.map((r) => (r.cells[0].innerText + " " + r.querySelector("input.root").value + " " +
  r.cells[1].innerText + " " + r.cells[4].innerText).replace(/\s+/g, " ")));
ok(/vox_ah_C3\.wav C3 from the name/.test(rows[0]) && /LOOP/.test(rows[0]), "vox_ah_C3.wav: C3 from its name, looped (" + rows[0] + ")");
ok(/pluck\.wav C5 heard/.test(rows[1]) && /ONE-SHOT/.test(rows[1]), "pluck.wav: C5 heard, one-shot (" + rows[1] + ")");
ok(/2Z/.test(await p.textContent("#lcd")) && /VOX AH/.test(await p.inputValue("#name")), "the LCD: 2 zones; the name from the file: " + await p.inputValue("#name"));
const sp16 = await p.textContent("#space");
await p.click("#rate button:nth-child(3)");
const sp11 = await p.textContent("#space");
ok(parseFloat(sp11) < parseFloat(sp16), `11 kHz takes less room (${sp16} -> ${sp11})`);
await p.click("#keys [data-n='60']");
ok(true, "a key of the preview plays (no error)");

await p.click("#connect");
await p.waitForFunction(() => document.querySelectorAll("#slots .slot").length === 3, null, { timeout: 5000 });
ok(await p.evaluate(() => window.__fm1.sysex), "MIDI asked with SysEx");
ok(/DRUMLOOP/.test(await p.textContent("#slots")) && /92 KB free/.test(await p.textContent("#slots")), "the slots: USR2 DRUMLOOP, USR1 / USR3 92 KB free");
await p.fill("#name", "my vox");
await p.click("#slots .slot:nth-child(2) .send");
await p.waitForFunction(() => /is in USR2/.test(document.getElementById("status").textContent), null, { timeout: 60000 });
const got = await p.evaluate(() => window.__fm1.ended[1]);
const h = Buffer.from(got.hdr);
ok(h.readUInt32LE(0) === 0x504D5346 && h[6] === 2 && h.toString("latin1", 8, 14) === "MY VOX", "USR2 got the sound: FSMP, 2 zones, MY VOX");
ok(h.readUInt32LE(32 + 16) === Math.round(11025 / 44100 * 65536) && h[32 + 27] === 1 && h[32 + 28 + 27] === 0,
   "  at 11 kHz, C3 looped, the pluck one-shot (CRC checked by the FM-1)");
ok(/MY VOX/.test(await p.textContent("#slots")), "  the slots read again: USR2 MY VOX");
if (shot) await p.screenshot({ path: shot, fullPage: true });
await p.click("#slots .slot:nth-child(2) .erase");
await p.waitForFunction(() => /USR2 is empty/.test(document.getElementById("status").textContent), null, { timeout: 5000 });
ok(/empty/.test(await p.textContent("#slots .slot:nth-child(2) h3")), "USR2 emptied");
await b.close();
fs.rmSync(dir, { recursive: true, force: true });
console.log(fails ? `sounds page: ${fails} FAILED` : "sounds page: all ok");
process.exit(fails ? 1 : 0);
