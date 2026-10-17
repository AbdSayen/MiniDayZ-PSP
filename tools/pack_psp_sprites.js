#!/usr/bin/env node
/**
 * Pack original_docs/images/*.png into a single sprites.pak for PSP deploy.
 * Avoids 1000+ loose files next to EBOOT (XMB/PPSSPP scan issues).
 *
 * Format MDZSPR01:
 *   magic[8] "MDZSPR01"
 *   u32 count (LE)
 *   u32 reserved
 *   Entry[count]: name[48], u32 offset, u32 size
 *   raw PNG bytes...
 *
 * Usage: node tools/pack_psp_sprites.js [imagesDir] [outPak]
 */
const fs = require('fs');
const path = require('path');

const root = path.join(__dirname, '..');
const imagesDir = path.resolve(process.argv[2] || path.join(root, 'original_docs', 'images'));
const outPak = path.resolve(process.argv[3] || path.join(root, 'cpp', 'platform', 'psp', 'deploy', 'MINIDAYZ', 'data', 'sprites.pak'));

if (!fs.existsSync(imagesDir)) {
  console.error('images dir missing:', imagesDir);
  process.exit(1);
}

const files = fs.readdirSync(imagesDir)
  .filter((f) => f.toLowerCase().endsWith('.png'))
  .sort();

const entries = [];
const blobs = [];
let cursor = 0;
for (const f of files) {
  const name = f.replace(/\.png$/i, '');
  if (name.length >= 48) {
    console.warn('skip long name', name);
    continue;
  }
  const buf = fs.readFileSync(path.join(imagesDir, f));
  entries.push({ name, offset: 0, size: buf.length, buf });
}

const headerSize = 16 + entries.length * (48 + 8);
cursor = headerSize;
for (const e of entries) {
  e.offset = cursor;
  cursor += e.size;
}

const out = Buffer.alloc(cursor);
out.write('MDZSPR01', 0, 8, 'ascii');
out.writeUInt32LE(entries.length, 8);
out.writeUInt32LE(0, 12);
let off = 16;
for (const e of entries) {
  out.fill(0, off, off + 48);
  out.write(e.name, off, 'ascii');
  out.writeUInt32LE(e.offset, off + 48);
  out.writeUInt32LE(e.size, off + 52);
  off += 56;
  e.buf.copy(out, e.offset);
}

fs.mkdirSync(path.dirname(outPak), { recursive: true });
fs.writeFileSync(outPak, out);
console.log('Wrote', outPak);
console.log('Sprites:', entries.length, 'Size MB:', (out.length / (1024 * 1024)).toFixed(2));
