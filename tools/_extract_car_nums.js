const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

function walk(evts, fn, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    if (e[0] === 0 && JSON.stringify(e).includes('"' + fn + '"')) out.push(e);
    for (const x of e) if (Array.isArray(x)) walk(x, fn, out);
  }
}

const fns = [
  'Car_respawn_uaz',
  'Car_respawn_volga',
  'Spawn_uaz',
  'Spawn_volga',
  'Spawn_truck',
  'Drivable vehicles',
];
for (const fn of fns) {
  const hits = [];
  project[6].forEach((sh) => walk(sh[1], fn, hits));
  hits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
  const pick = hits[hits.length - 1];
  if (!pick) {
    console.log(fn, 'NO HIT');
    continue;
  }
  const nums = [];
  (function cn(n, ctx) {
    if (typeof n === 'number' && n > 0 && n <= 2000 && Number.isInteger(n)) nums.push(n);
    else if (Array.isArray(n)) n.forEach((x) => cn(x, ctx));
  })(pick);
  const freq = {};
  for (const n of nums) freq[n] = (freq[n] || 0) + 1;
  const top = Object.entries(freq)
    .sort((a, b) => b[1] - a[1])
    .slice(0, 25)
    .map(([k, v]) => k + ':' + v)
    .join(' ');
  console.log(fn, 'len', JSON.stringify(pick).length, 'top nums', top);
}
