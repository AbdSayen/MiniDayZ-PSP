// Lists every Tilemap instance stored in C2 layouts (data.js).
// Usage: node tools/extract_world_tilemaps.js [--dump outdir]
const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '..');
const raw = fs.readFileSync(path.join(root, 'original_docs/data.js'), 'utf8');
const json = JSON.parse(raw.slice(raw.indexOf('{')));
const project = json.project;

const types = project[3];
const typeImage = {};
types.forEach((t, i) => {
  if (Array.isArray(t) && Array.isArray(t[6]) && typeof t[6][0] === 'string') typeImage[i] = t[6][0];
});

const layouts = project[5];
const found = [];
for (const L of layouts) {
  const layoutName = L[0];
  const layers = L[6] || [];
  for (const layer of layers) {
    const layerName = layer[0];
    const insts = layer[14] || [];
    for (const inst of insts) {
      const geo = inst[0];
      if (!Array.isArray(geo) || !Array.isArray(geo[13]) || geo[13].length !== 3) continue;
      const [cols, rows, rle] = geo[13];
      if (typeof rle !== 'string') continue;
      const props = inst[inst.length - 1];
      found.push({
        layout: layoutName, layer: layerName, type: inst[1], image: typeImage[inst[1]] || '?',
        x: geo[0], y: geo[1], w: geo[3], h: geo[4], cols, rows,
        tileW: props[1], tileH: props[2], rle,
      });
    }
  }
}

function decode(rle, n) {
  const out = new Int16Array(n).fill(-1);
  let i = 0;
  for (const part of rle.split(',')) {
    if (!part) continue;
    const x = part.indexOf('x');
    if (x > 0) {
      const count = parseInt(part.slice(0, x), 10);
      const v = parseInt(part.slice(x + 1), 10);
      for (let k = 0; k < count && i < n; ++k) out[i++] = v;
    } else if (i < n) out[i++] = parseInt(part, 10);
  }
  return out;
}

for (const f of found) {
  const nonEmpty = decode(f.rle, f.cols * f.rows).filter((v) => v >= 0).length;
  console.log(`${f.layout} | ${f.layer} | ${f.image} | pos ${f.x},${f.y} | ${f.cols}x${f.rows} @${f.tileW}x${f.tileH} | tiles ${nonEmpty}`);
}

const di = process.argv.indexOf('--dump');
if (di > 0) {
  const outDir = process.argv[di + 1];
  const want = process.argv[di + 2];
  fs.mkdirSync(outDir, { recursive: true });
  const L = layouts.find((l) => l[0] === want);
  const layoutW = L[1], layoutH = L[2];
  // Instances whose origin lies outside the layout are Map_generation templates, not world tiles.
  const sel = found.filter((f) => f.layout === want && f.image !== '?' &&
    f.x > -200 && f.y > -200 && f.x < layoutW && f.y < layoutH);
  const pngWidth = (img) => fs.readFileSync(path.join(root, 'original_docs', img)).readUInt32BE(16);
  // Text format read by cpp/src/world/world_tilemaps.cpp:
  // line 1: count; per map: image x y cols rows tileW tileH atlasCols, then cols*rows ids (-1 = empty).
  // Each map is cropped to the bounding box of its non-empty tiles.
  const blocks = [];
  for (const f of sel) {
    const ids = decode(f.rle, f.cols * f.rows);
    let c0 = f.cols, r0 = f.rows, c1 = -1, r1 = -1;
    for (let r = 0; r < f.rows; ++r)
      for (let c = 0; c < f.cols; ++c)
        if (ids[r * f.cols + c] >= 0) {
          c0 = Math.min(c0, c); c1 = Math.max(c1, c); r0 = Math.min(r0, r); r1 = Math.max(r1, r);
        }
    if (c1 < 0) continue;
    const cols = c1 - c0 + 1, rows = r1 - r0 + 1;
    const out = [];
    for (let r = r0; r <= r1; ++r) for (let c = c0; c <= c1; ++c) out.push(ids[r * f.cols + c]);
    const img = path.basename(f.image, '.png');
    const x = Math.round(f.x) + c0 * f.tileW, y = Math.round(f.y) + r0 * f.tileH;
    const atlasCols = Math.floor(pngWidth(f.image) / f.tileW);
    blocks.push(`${img} ${x} ${y} ${cols} ${rows} ${f.tileW} ${f.tileH} ${atlasCols}\n${out.join(' ')}\n`);
  }
  fs.writeFileSync(path.join(outDir, 'world_tilemaps.txt'), `${blocks.length}\n${blocks.join('')}`);
  console.log(`wrote ${blocks.length} tilemaps from layout "${want}"`);
}
