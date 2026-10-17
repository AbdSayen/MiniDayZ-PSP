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

// Find Check_space_inventory OnFunction and decode -1 branch
function findOnFn(name) {
  let best = null;
  let bestL = 0;
  project[6].forEach((sh) =>
    walk(sh[1], (e) => {
      if (e[0] !== 0) return;
      const s = JSON.stringify(e);
      // OnFunction condition: [180,40,...,[[1,[2,"name"]]]]
      if (!s.includes('[2,"' + name + '"]')) return;
      if (!s.includes(',40,')) return;
      // prefer event whose first condition is OnFunction name
      const head = s.slice(0, 200);
      if (!head.includes(name)) return;
      if (s.length > bestL && s.length < 200000) {
        best = e;
        bestL = s.length;
      }
    })
  );
  return { e: best, len: bestL };
}

for (const name of [
  'Check_space_inventory',
  'Check_space_pants',
  'Spawn_drop_from_player',
  'Spawn_drop',
]) {
  const { e, len } = findOnFn(name);
  console.log('OnFunction', name, 'len', len);
  if (!e) continue;
  const s = JSON.stringify(e);
  // -1 branches
  if (s.includes('[7,[0,-1]]')) {
    const idx = s.indexOf('[7,[0,-1]]');
    console.log('  -1 ctx:', s.slice(Math.max(0, idx - 100), idx + 350));
  }
  const strs = [...new Set([...s.matchAll(/\[2,"([^"]{1,60})"\]/g)].map((m) => m[1]))];
  console.log('  strs:', strs.slice(0, 25).join(' | '));
}

// Pan 210: is it in Spawn_drop craft recipes? melee weapon list?
const spawnDrops = [];
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    if (e[0] !== 0) return;
    const s = JSON.stringify(e);
    if (!s.includes('Spawn_drop') || !s.includes('[0,210]')) return;
    if (s.length > 15000) return;
    const strs = [...new Set([...s.matchAll(/\[2,"([^"]{1,50})"\]/g)].map((m) => m[1]))];
    spawnDrops.push({ len: s.length, strs: strs.slice(0, 15) });
  })
);
spawnDrops.sort((a, b) => b.len - a.len);
console.log('--- Spawn_drop events mentioning 210 ---');
const seen = new Set();
for (const b of spawnDrops) {
  const k = b.strs.join('|');
  if (seen.has(k)) continue;
  seen.add(k);
  console.log(b.len, b.strs.join(' | '));
  if (seen.size >= 15) break;
}

// Object name for fireplace: search animation "fireplace" / name containing fire
const fireNames = [];
function scanTypes(n) {
  if (!Array.isArray(n)) return;
  if (typeof n[0] === 'string' && /fire|camp|hearth|grill/i.test(n[0])) {
    fireNames.push(n[0] + ' sid?' + n[1]);
  }
  for (const x of n) if (Array.isArray(x)) scanTypes(x);
}
scanTypes(project[3]);
console.log('fire-named types:', fireNames.slice(0, 30));

// Find which object type index is used as 253 in events - in C2 export,
// action [4,253] is object type index in project[3]
const t253 = project[3][253];
console.log('project[3][253] name', t253 && t253[0], 'plugin', t253 && t253[1]);
const t252 = project[3][252];
console.log('project[3][252] name', t252 && t252[0]);
const t172 = project[3][172];
console.log('project[3][172] name', t172 && t172[0]);

// Confirm all cook mappings from item_activate
const cookMap = {};
let ia = null;
let iaL = 0;
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    const s = JSON.stringify(e);
    if (e[0] === 0 && s.includes('[2,"item_activate"]') && s.length > iaL && s.length < 250000) {
      ia = e;
      iaL = s.length;
    }
  })
);
const iaS = JSON.stringify(ia);
for (const id of [60, 71, 91, 92, 93, 94]) {
  const re = new RegExp(
    '\\[0,\\[0,0\\]\\],\\[8,0\\],\\[7,\\[0,' +
      id +
      '\\]\\][\\s\\S]{0,3500}?\\[2,"Check_space_inventory"\\][\\s\\S]{0,200}?\\[0,-1\\][\\s\\S]{0,80}?\\[0,(\\d+)\\]'
  );
  const m = iaS.match(re);
  cookMap[id] = m ? +m[1] : null;
}
console.log('cook map', cookMap);

// pan in item_activate2?
let ia2 = null;
let ia2L = 0;
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    const s = JSON.stringify(e);
    if (e[0] === 0 && s.includes('[2,"item_activate2"]') && s.length > ia2L && s.length < 250000) {
      ia2 = e;
      ia2L = s.length;
    }
  })
);
const ia2s = JSON.stringify(ia2 || []);
console.log('item_activate2 len', ia2L);
console.log('item_activate2 has 210', ia2s.includes('[7,[0,210]]'));
console.log('item_activate2 meat_cook', ia2s.includes('meat_cook'));
console.log('item_activate2 pan', /[Pp]an/.test(ia2s));

// Melee: pan damage from weapons list already known. Search IV0/wpn type for 210
console.log('raw count of [0,210] in data.js', (raw.match(/\[0,210\]/g) || []).length);
