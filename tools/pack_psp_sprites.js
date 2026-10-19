#!/usr/bin/env node
/**
 * Pack original_docs/images/*.png into sprites.pak for the PSP GU renderer.
 *
 * Format MDZSPR02 (2026-10-19): textures pre-converted to the GE's native formats so the PSP
 * only inflates (no PNG decode / colour conversion at runtime).
 *   magic[8] "MDZSPR02", u32 count, u32 reserved
 *   Entry[count] (72 bytes): name[48], u32 offset, u32 csize, u32 rawsize,
 *                            u16 w, u16 h, u16 stride(texels), u8 fmt, u8[5] pad
 *   payload = zlib(raw)
 *   fmt 0: T8 + CLUT8888, raw = 1024-byte CLUT (R,G,B,A bytes) + stride*h indices.
 *          Used when the image has <= 256 distinct RGBA colours -> LOSSLESS.
 *          (fully transparent pixels are normalised to (0,0,0,0) first; invisible anyway)
 *   fmt 1: RGBA5551 (alpha only 0/255), fmt 2: RGBA4444 (soft alpha); raw = stride*h*2.
 *   stride: T8 multiple of 16 texels, 16-bit multiple of 8 texels (16-byte rows for the GE).
 *
 * Usage: node tools/pack_psp_sprites.js [imagesDir] [outPak] [extraDir...]
 */
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');

const root = path.join(__dirname, '..');
const imagesDir = path.resolve(process.argv[2] || path.join(root, 'original_docs', 'images'));
const outPak = path.resolve(process.argv[3] || path.join(root, 'cpp', 'platform', 'psp', 'deploy', 'MINIDAYZ', 'data', 'sprites.pak'));

// ---- minimal PNG decoder (non-interlaced; all colour types, bit depths 1..8 / 16) ----
function decodePng(buf) {
  const sig = [137, 80, 78, 71, 13, 10, 26, 10];
  for (let i = 0; i < 8; i++) if (buf[i] !== sig[i]) throw new Error('not a PNG');
  let off = 8, w = 0, h = 0, depth = 0, ctype = 0, interlace = 0;
  let palette = null, trns = null;
  const idat = [];
  while (off < buf.length) {
    const len = buf.readUInt32BE(off); const type = buf.toString('ascii', off + 4, off + 8);
    const data = buf.subarray(off + 8, off + 8 + len);
    if (type === 'IHDR') {
      w = data.readUInt32BE(0); h = data.readUInt32BE(4); depth = data[8]; ctype = data[9]; interlace = data[12];
    } else if (type === 'PLTE') palette = data;
    else if (type === 'tRNS') trns = data;
    else if (type === 'IDAT') idat.push(data);
    else if (type === 'IEND') break;
    off += 12 + len;
  }
  if (interlace) throw new Error('interlaced PNG not supported');
  const chans = { 0: 1, 2: 3, 3: 1, 4: 2, 6: 4 }[ctype];
  const bpp = Math.max(1, (chans * depth) >> 3);
  const rowBytes = (w * chans * depth + 7) >> 3;
  const raw = zlib.inflateSync(Buffer.concat(idat));
  const cur = Buffer.alloc(rowBytes), prev = Buffer.alloc(rowBytes);
  const out = Buffer.alloc(w * h * 4);
  let p = 0;
  for (let y = 0; y < h; y++) {
    const ft = raw[p++];
    for (let i = 0; i < rowBytes; i++) {
      const x = raw[p++];
      const a = i >= bpp ? cur[i - bpp] : 0, b = prev[i], c = i >= bpp ? prev[i - bpp] : 0;
      let v;
      switch (ft) {
        case 0: v = x; break;
        case 1: v = x + a; break;
        case 2: v = x + b; break;
        case 3: v = x + ((a + b) >> 1); break;
        case 4: { const pp = a + b - c, pa = Math.abs(pp - a), pb = Math.abs(pp - b), pc = Math.abs(pp - c);
          v = x + (pa <= pb && pa <= pc ? a : pb <= pc ? b : c); break; }
        default: throw new Error('bad filter ' + ft);
      }
      cur[i] = v & 255;
    }
    const sample = (idx) => { // idx-th sample in row, scaled to 8 bit
      if (depth === 8) return cur[idx];
      if (depth === 16) return cur[idx * 2];
      const bit = idx * depth, byte = cur[bit >> 3], shift = 8 - depth - (bit & 7);
      const v = (byte >> shift) & ((1 << depth) - 1);
      return ctype === 3 ? v : Math.round(v * 255 / ((1 << depth) - 1));
    };
    const rawSample = (idx) => { // unscaled (for tRNS grey/rgb compare)
      if (depth === 16) return cur.readUInt16BE(idx * 2);
      if (depth === 8) return cur[idx];
      const bit = idx * depth, byte = cur[bit >> 3], shift = 8 - depth - (bit & 7);
      return (byte >> shift) & ((1 << depth) - 1);
    };
    for (let x = 0; x < w; x++) {
      let r, g, b, al = 255;
      if (ctype === 3) {
        const i = sample(x); r = palette[i * 3]; g = palette[i * 3 + 1]; b = palette[i * 3 + 2];
        if (trns && i < trns.length) al = trns[i];
      } else if (ctype === 0) {
        r = g = b = sample(x);
        if (trns && rawSample(x) === trns.readUInt16BE(0)) al = 0;
      } else if (ctype === 4) { r = g = b = sample(x * 2); al = sample(x * 2 + 1); }
      else if (ctype === 2) {
        r = sample(x * 3); g = sample(x * 3 + 1); b = sample(x * 3 + 2);
        if (trns && rawSample(x * 3) === trns.readUInt16BE(0) && rawSample(x * 3 + 1) === trns.readUInt16BE(2) &&
            rawSample(x * 3 + 2) === trns.readUInt16BE(4)) al = 0;
      } else { r = sample(x * 4); g = sample(x * 4 + 1); b = sample(x * 4 + 2); al = sample(x * 4 + 3); }
      const o = (y * w + x) * 4;
      out[o] = r; out[o + 1] = g; out[o + 2] = b; out[o + 3] = al;
    }
    cur.copy(prev);
  }
  return { w, h, rgba: out };
}

function convert(img) {
  const { w, h, rgba } = img;
  for (let i = 0; i < w * h; i++) if (rgba[i * 4 + 3] === 0) rgba.writeUInt32LE(0, i * 4);
  const map = new Map(); let t8 = true;
  for (let i = 0; i < w * h && t8; i++) {
    const k = rgba.readUInt32LE(i * 4);
    if (!map.has(k)) { if (map.size >= 256) t8 = false; else map.set(k, map.size); }
  }
  if (t8) {
    const stride = (w + 15) & ~15;
    const raw = Buffer.alloc(1024 + stride * h);
    for (const [k, idx] of map) raw.writeUInt32LE(k >>> 0, idx * 4); // bytes R,G,B,A
    for (let y = 0; y < h; y++)
      for (let x = 0; x < w; x++) raw[1024 + y * stride + x] = map.get(rgba.readUInt32LE((y * w + x) * 4));
    return { fmt: 0, stride, raw };
  }
  let binary = true;
  for (let i = 0; i < w * h; i++) { const a = rgba[i * 4 + 3]; if (a !== 0 && a !== 255) { binary = false; break; } }
  const stride = (w + 7) & ~7;
  const raw = Buffer.alloc(stride * h * 2);
  for (let y = 0; y < h; y++)
    for (let x = 0; x < w; x++) {
      const o = (y * w + x) * 4, r = rgba[o], g = rgba[o + 1], b = rgba[o + 2], a = rgba[o + 3];
      const v = binary
        ? ((r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10) | ((a >= 128 ? 1 : 0) << 15))
        : ((r >> 4) | ((g >> 4) << 4) | ((b >> 4) << 8) | ((a >> 4) << 12));
      raw.writeUInt16LE(v, (y * stride + x) * 2);
    }
  return { fmt: binary ? 1 : 2, stride, raw };
}

if (!fs.existsSync(imagesDir)) { console.error('images dir missing:', imagesDir); process.exit(1); }
// Extra dirs (argv[4..]): port-only assets such as cpp/data/intro (Bonds Guru intro).
const srcDirs = [imagesDir].concat(process.argv.slice(4).map((d) => path.resolve(d)));
const files = [];
for (const d of srcDirs) {
  if (!fs.existsSync(d)) { console.warn('extra dir missing', d); continue; }
  for (const f of fs.readdirSync(d).filter((f) => f.toLowerCase().endsWith('.png')).sort()) files.push({ d, f });
}
const entries = [];
const stats = { t8: 0, a5551: 0, a4444: 0, rawBytes: 0 };
for (const { d, f } of files) {
  const name = f.replace(/\.png$/i, '');
  if (name.length >= 48) { console.warn('skip long name', name); continue; }
  let img;
  try { img = decodePng(fs.readFileSync(path.join(d, f))); }
  catch (e) { console.warn('skip', f, e.message); continue; }
  if (img.w > 65535 || img.h > 65535) { console.warn('skip huge', f); continue; }
  const c = convert(img);
  const comp = zlib.deflateSync(c.raw, { level: 9 });
  entries.push({ name, w: img.w, h: img.h, stride: c.stride, fmt: c.fmt, rawsize: c.raw.length, comp });
  stats.rawBytes += c.raw.length;
  if (c.fmt === 0) stats.t8++; else if (c.fmt === 1) stats.a5551++; else stats.a4444++;
}
const headerSize = 16 + entries.length * 72;
let cursor = headerSize;
for (const e of entries) { e.offset = cursor; cursor += e.comp.length; cursor = (cursor + 3) & ~3; }
const out = Buffer.alloc(cursor);
out.write('MDZSPR02', 0, 8, 'ascii');
out.writeUInt32LE(entries.length, 8);
out.writeUInt32LE(0, 12);
let off = 16;
for (const e of entries) {
  out.write(e.name, off, 'ascii');
  out.writeUInt32LE(e.offset, off + 48);
  out.writeUInt32LE(e.comp.length, off + 52);
  out.writeUInt32LE(e.rawsize, off + 56);
  out.writeUInt16LE(e.w, off + 60);
  out.writeUInt16LE(e.h, off + 62);
  out.writeUInt16LE(e.stride, off + 64);
  out[off + 66] = e.fmt;
  off += 72;
  e.comp.copy(out, e.offset);
}
fs.mkdirSync(path.dirname(outPak), { recursive: true });
fs.writeFileSync(outPak, out);
console.log('Wrote', outPak);
console.log('Sprites:', entries.length, 'Size MB:', (out.length / (1024 * 1024)).toFixed(2),
  '| T8 (lossless):', stats.t8, '5551:', stats.a5551, '4444:', stats.a4444,
  '| decoded MB:', (stats.rawBytes / (1024 * 1024)).toFixed(2));
