// Dumps Sprite instances of one C2 layout (first animation frame of each) for C++ rendering.
// Usage: node tools/extract_layout_sprites.js <Layout> <out.txt> [layer,layer,...]
const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '..');
const raw = fs.readFileSync(path.join(root, 'original_docs/data.js'), 'utf8');
const project = JSON.parse(raw.slice(raw.indexOf('{'))).project;
const types = project[3];

const [layoutName, outPath, layerArg] = process.argv.slice(2);
const wantLayers = layerArg ? new Set(layerArg.split(',')) : null;
const L = project[5].find((l) => l[0] === layoutName);
if (!L) throw new Error(`no layout ${layoutName}`);

// Sprite object types: [name, pluginIndex, ..., animations at [7]]; frame = [path, size, x, y, w, h, ...].
function firstFrame(typeIndex, animName, frameIndex) {
  const t = types[typeIndex];
  const anims = t && t[7];
  if (!Array.isArray(anims) || !anims.length) return null;
  const anim = anims.find((a) => a[0] === animName) || anims[0];
  const frames = anim[7];
  if (!Array.isArray(frames) || !frames.length) return null;
  const f = frames[Math.min(frameIndex || 0, frames.length - 1)];
  if (typeof f[0] !== 'string') return null;
  return { image: path.basename(f[0], '.png'), sx: f[2], sy: f[3], sw: f[4], sh: f[5] };
}

const lines = [];
L[6].forEach((layer, li) => {
  if (wantLayers && !wantLayers.has(layer[0])) return;
  for (const inst of layer[14] || []) {
    const g = inst[0];
    if (!Array.isArray(g) || g.length < 10 || (Array.isArray(g[13]) && g[13].length === 3)) continue;
    // Sprite instance props: [initially invisible, initial animation, initial frame, collisions].
    const props = inst[inst.length - 1];
    if (!Array.isArray(props) || props[0] === 1) continue;
    const fr = firstFrame(inst[1], props[1], props[2]);
    if (!fr) continue;
    const [x, y, , w, h, angle, , opacity, hx, hy] = g;
    if (opacity <= 0) continue;
    const deg = (angle * 180) / Math.PI;
    lines.push([li, fr.image, fr.sx, fr.sy, fr.sw, fr.sh, Math.round(x), Math.round(y),
      Math.round(w), Math.round(h), deg.toFixed(1), hx.toFixed(3), hy.toFixed(3), opacity.toFixed(2)].join(' '));
  }
});
// Format: count, then per line: layer image sx sy sw sh x y w h angle_deg hotx hoty opacity
fs.writeFileSync(outPath, `${lines.length}\n${lines.join('\n')}\n`);
console.log(`wrote ${lines.length} sprites from ${layoutName} → ${outPath}`);
