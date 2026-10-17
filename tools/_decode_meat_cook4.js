const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(__dirname, '../original_docs/data.js'), 'utf8');
const project = JSON.parse(raw.slice(raw.indexOf('{'))).project;

function walk(evts, fn) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    fn(e);
    for (const x of e) walk(x, fn);
  }
}

// Find every event that has CompareParam param0==-1 AND Spawn_drop_from_player
const hits = [];
project[6].forEach((sh, si) =>
  walk(sh[1], (e) => {
    if (e[0] !== 0) return;
    const s = JSON.stringify(e);
    if (!s.includes('[7,[0,-1]]')) return;
    if (!s.includes('Spawn_drop_from_player')) return;
    if (s.length > 20000) return;
    const strs = [...new Set([...s.matchAll(/\[2,"([^"]{1,50})"\]/g)].map((m) => m[1]))];
    // parent OnFunction name nearby in s
    const onFns = [...s.matchAll(/\[2,"(Check_space_[^"]+|Spawn_drop[^"]*|item_[^"]*)"\]/g)].map(
      (m) => m[1]
    );
    hits.push({ si, len: s.length, strs: strs.slice(0, 20), onFns: [...new Set(onFns)] });
  })
);
hits.sort((a, b) => a.len - b.len);
console.log('events with -1 + Spawn_drop_from_player:', hits.length);
hits.slice(0, 25).forEach((h, i) =>
  console.log(i, 'len', h.len, 'onFns', h.onFns.join(','), 'strs', h.strs.join('|'))
);

// Decode the smallest such event fully
function findSmallest() {
  let best = null;
  let bestL = 1e12;
  project[6].forEach((sh) =>
    walk(sh[1], (e) => {
      if (e[0] !== 0) return;
      const s = JSON.stringify(e);
      if (!s.includes('[7,[0,-1]]') || !s.includes('Spawn_drop_from_player')) return;
      if (s.length < bestL) {
        best = e;
        bestL = s.length;
      }
    })
  );
  return best;
}
const small = findSmallest();
console.log('\n=== smallest -1/Spawn_drop_from_player len', JSON.stringify(small).length, '===');
function dump(e, d) {
  if (!Array.isArray(e) || d > 10) return;
  if (typeof e[0] === 'number' && typeof e[1] === 'number') {
    const s = JSON.stringify(e);
    const strs = [...s.matchAll(/\[2,"([^"]+)"\]/g)].map((m) => m[1]);
    const nums = [...s.matchAll(/\[0,(-?\d+)\]/g)].map((m) => m[1]).slice(0, 12);
    if (strs.length || nums.length)
      console.log('  '.repeat(d) + `p${e[0]}a${e[1]} ${strs.join(',')} nums:${nums.join(',')}`);
  }
  for (const x of e) if (Array.isArray(x)) dump(x, d + 1);
}
dump(small, 0);

// Also: does Check_space_inventory when bag full cascade still reach -1?
// Search Check_space_* OnFunctions for CompareParam -1
for (const name of [
  'Check_space_inventory',
  'Check_space_pants',
  'Check_space_outerwear',
  'Check_space_vest',
  'Check_space_backpack',
]) {
  let body = null;
  let bl = 0;
  project[6].forEach((sh) =>
    walk(sh[1], (e) => {
      if (e[0] !== 0) return;
      const s = JSON.stringify(e);
      const head = s.slice(0, 180);
      if (!head.includes('[2,"' + name + '"]')) return;
      if (!s.includes(',40,')) return;
      if (s.length > bl && s.length < 50000) {
        body = e;
        bl = s.length;
      }
    })
  );
  const s = JSON.stringify(body || []);
  console.log(
    name,
    'len',
    bl,
    'has-1',
    s.includes('[7,[0,-1]]'),
    'hasSpawn_drop_from_player',
    s.includes('Spawn_drop_from_player'),
    'hasSpawn_drop',
    s.includes('"Spawn_drop"')
  );
}

// Object t253 animations
const t253 = project[3][253];
console.log('\nt253 full keys sample', JSON.stringify(t253).slice(0, 500));
// Animations often at t253[7] or nested
function findAnims(n, out) {
  if (!Array.isArray(n)) return;
  if (typeof n[0] === 'string' && (n[0] === 'on' || n[0] === 'off' || /fire|burn/i.test(n[0])))
    out.push(n[0]);
  for (const x of n) if (Array.isArray(x)) findAnims(x, out);
}
const anims = [];
findAnims(t253, anims);
console.log('t253 anim-like', [...new Set(anims)]);

// Pan 210 all [0,210] contexts with function names
let p = 0;
const panCtx = [];
while ((p = raw.indexOf('[0,210]', p)) >= 0 && panCtx.length < 20) {
  const slice = raw.slice(Math.max(0, p - 500), p + 200);
  const names = [...slice.matchAll(/\[2,"([^"]{2,40})"\]/g)].map((m) => m[1]);
  panCtx.push([...new Set(names)].slice(0, 10).join('|'));
  p += 7;
}
console.log('\n[0,210] contexts:');
[...new Set(panCtx)].forEach((x) => console.log(' ', x));
