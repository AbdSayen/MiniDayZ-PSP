const fs = require('fs');
const raw = fs.readFileSync('original_docs/data.js', 'utf8');
const p = JSON.parse(raw.slice(raw.indexOf('{'))).project;
const types = p[3];
let hits = 0;
for (const t of types) {
  if (!Array.isArray(t) || typeof t[0] !== 'string') continue;
  if (!Array.isArray(t[7])) continue;
  const name = t[0];
  const blob = JSON.stringify(t[7]);
  if (!blob.includes('player_skin_def_1') && !blob.includes('zed_normal_skin1') &&
      !blob.includes('player_hands_white') && !blob.includes('wolf')) continue;
  console.log('=== type', name);
  for (const a of t[7]) {
    if (!Array.isArray(a)) continue;
    const frames = a[7];
    if (!Array.isArray(frames) || !frames.length) continue;
    console.log(' anim', JSON.stringify(a[0]), 'frames', frames.length,
                'meta', a[1], a[2], a[3], a[4], a[5], a[6]);
    for (let i = 0; i < Math.min(frames.length, 8); i++) {
      const f = frames[i];
      console.log('  ', i, String(f[0]).replace(/^images\//, ''),
                  'src', f[2], f[3], f[4], f[5], 'dur', f[6]);
    }
  }
  if (++hits > 8) break;
}
console.log('hits', hits);
