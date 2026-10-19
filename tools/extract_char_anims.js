/**
 * Dump C2 character animation frames for skins we render in C++.
 * Output: cpp/data/char_anims.txt
 * Format:
 *   N
 *   base anim_name frame_count speed loop
 *   sheet sx sy sw sh duration
 *   ...
 */
const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(__dirname, '../original_docs/data.js'), 'utf8');
const p = JSON.parse(raw.slice(raw.indexOf('{'))).project;
const types = p[3];

const want = [
  'player_skin_def_1', 'player_hands_white',
  'player_skin_veteran', 'player_skin_survivalist', 'player_skin_redneck', 'player_skin_maniac',
  'zed_normal_skin1', 'zed_fast_skin1', 'zed_army_skin1', 'zed_buried_skin',
  'zed_tank_skin', 'zed_screamer_skin',
  'wolf_skin', 'deer_skin', 'rabbit_skin', 'bandit_skin', 'dog_skin'
];

function sheetIndex(img) {
  const m = String(img).match(/-sheet(\d+)\.png$/);
  return m ? parseInt(m[1], 10) : 0;
}

const blocks = [];
for (const t of types) {
  if (!Array.isArray(t) || typeof t[0] !== 'string' || !Array.isArray(t[7])) continue;
  const blob = JSON.stringify(t[7]);
  let base = null;
  for (const w of want) {
    if (blob.includes(w + '-sheet')) { base = w; break; }
  }
  if (!base) continue;
  // Prefer the type whose first frame image matches base
  const firstImg = t[7][0] && t[7][0][7] && t[7][0][7][0] && String(t[7][0][7][0][0]);
  if (!firstImg || !firstImg.includes(base)) continue;

  for (const a of t[7]) {
    if (!Array.isArray(a) || !Array.isArray(a[7]) || !a[7].length) continue;
    const aname = a[0];
    const speed = typeof a[1] === 'number' ? a[1] : 5;
    const loop = !!a[2];
    const frames = a[7].map(f => ({
      sheet: sheetIndex(f[0]),
      sx: f[2]|0, sy: f[3]|0, sw: f[4]|0, sh: f[5]|0,
      dur: (typeof f[6] === 'number' && f[6] > 0) ? f[6] : 1
    }));
    blocks.push({ base, aname, speed, loop, frames });
  }
}

// Death anims: merge die1/die2 from any type whose sheet matches a wanted base.
const have = new Set(blocks.map(b => `${b.base}\0${b.aname}`));
for (const t of types) {
  if (!Array.isArray(t) || !Array.isArray(t[7])) continue;
  for (const a of t[7]) {
    if (!Array.isArray(a) || !Array.isArray(a[7]) || !a[7].length) continue;
    const aname = String(a[0]);
    if (aname !== 'die1' && aname !== 'die2') continue;
    const img = a[7][0] && a[7][0][0] ? String(a[7][0][0]) : '';
    let base = null;
    for (const w of want) {
      if (img.includes(w)) { base = w; break; }
    }
    if (!base) continue;
    const key = `${base}\0${aname}`;
    if (have.has(key)) continue;
    have.add(key);
    const speed = typeof a[1] === 'number' ? a[1] : 5;
    const loop = !!a[2];
    const frames = a[7].map(f => ({
      sheet: sheetIndex(f[0]),
      sx: f[2]|0, sy: f[3]|0, sw: f[4]|0, sh: f[5]|0,
      dur: (typeof f[6] === 'number' && f[6] > 0) ? f[6] : 1
    }));
    blocks.push({ base, aname, speed, loop, frames });
  }
}

// wolf_skin living type has no die — attach wolf_dead die1 as wolf_skin die1.
{
  const t = types[470];
  if (t && Array.isArray(t[7])) {
    const a = t[7].find(x => x && x[0] === 'die1');
    if (a && !have.has('wolf_skin\0die1')) {
      const frames = a[7].map(f => ({
        sheet: sheetIndex(f[0]),
        sx: f[2]|0, sy: f[3]|0, sw: f[4]|0, sh: f[5]|0,
        dur: (typeof f[6] === 'number' && f[6] > 0) ? f[6] : 1
      }));
      blocks.push({ base: 'wolf_skin', aname: 'die1', speed: a[1] || 5, loop: false, frames });
    }
  }
}

let out = blocks.length + '\n';
for (const b of blocks) {
  out += `${b.base} ${b.aname} ${b.frames.length} ${b.speed} ${b.loop ? 1 : 0}\n`;
  for (const f of b.frames) {
    out += `${f.sheet} ${f.sx} ${f.sy} ${f.sw} ${f.sh} ${f.dur}\n`;
  }
}
const dest = path.join(__dirname, '../cpp/data/char_anims.txt');
fs.writeFileSync(dest, out);
console.log('wrote', dest, 'anims', blocks.length);
const sample = blocks.filter(b => b.base === 'player_skin_def_1').map(b => b.aname);
console.log('player_skin_def_1:', sample.join(', '));
